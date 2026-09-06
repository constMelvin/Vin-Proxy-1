#pragma once
#include "player_state_tracker.hpp"
#include "../../core/core.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/text_parse.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../utils/player_tracker.hpp"  
#include "../../packet/itemsdat_parser.hpp"
#include "../../utils/weapon_animation_manager.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <unordered_map>

namespace command {
    extern std::unordered_map<int, int> g_clothing_slots;
}

namespace extension::player_state_tracker {

class PlayerStateTrackerExtension final : public IPlayerStateTrackerExtension {
    core::Core* core_;
    
    int32_t local_netid_ = -1;            
    float local_x_ = 0.0f;                 
    float local_y_ = 0.0f;                 
    bool is_spawned_ = false;              
    
    bool last_on_ground_ = true;
    uint32_t consecutive_jumps_ = 0;

public:
    uint32_t get_local_netid() const {
        if (local_netid_ > 0) return static_cast<uint32_t>(local_netid_);
        auto lp = utils::PlayerTracker::get_instance().get_local_player();
        if (lp.netID > 0) return lp.netID;
        int cfg_netid = core_->get_config().get<int>("player.netid");
        if (cfg_netid > 0) return static_cast<uint32_t>(cfg_netid);
        return 0;
    }

    explicit PlayerStateTrackerExtension(core::Core* core)
        : core_{ core }
    {
    }

    ~PlayerStateTrackerExtension() override = default;

    void init() override {
        core_->get_config().set("features.double_jump", true);
        core_->get_config().set("features.immune_damage", false);
        
        core_->get_event_dispatcher().prependListener(
            core::EventType::Packet,
            [this](const core::EventPacket& event) {
                handle_packet(event);
            }
        );
    }

private:
    void handle_packet(const core::EventPacket& event) {
        const auto& game_packet = event.get_packet();
        
        if (game_packet.type == packet::PACKET_CALL_FUNCTION && event.from == core::EventFrom::FromServer) {
            try {
                const auto& ext_data = event.get_ext_data();
                packet::Variant variant{};
                variant.deserialize(ext_data);
                
                if (variant.size() >= 2) {
                    std::string function_name = variant.get<std::string>(0);
                    if (function_name == "OnSpawn") {
                        std::string spawn_data = variant.get<std::string>(1);
                        TextParse text_parse{ spawn_data };
                        
                        if (text_parse.get("type") == "local") {
                            local_netid_ = text_parse.get<int32_t>("netID");
                            is_spawned_ = true;
                            core_->get_config().set<int>("player.netid", local_netid_);
                            spdlog::info("[PlayerStateTracker] OnSpawn - Local NetID: {} detected and saved.", local_netid_);
                        }
                    }
                }
            } catch (...) {}
        }
        
        const auto& ext_data = event.get_ext_data();
        const packet::TankUpdatePacket* tank = nullptr;
        if (ext_data.size() >= sizeof(packet::TankUpdatePacket)) {
            tank = reinterpret_cast<const packet::TankUpdatePacket*>(ext_data.data());
        } else {
            // Fix: Clean binary pointer to game packet payload
            tank = reinterpret_cast<const packet::TankUpdatePacket*>(&game_packet);
        }
        if (!tank) return;

        if (game_packet.type == packet::PACKET_ITEM_ACTIVATE_REQUEST) {
            return;
        }

        if (game_packet.type != packet::PACKET_STATE && game_packet.type != packet::PACKET_TILE_CHANGE_REQUEST) {
            return;
        }
        
        if (event.from == core::EventFrom::FromClient) {
            if (tank->vec_x >= 0 && tank->vec_y >= 0) {
                local_x_ = tank->vec_x;
                local_y_ = tank->vec_y;
                core_->get_config().set<std::string>("player.position.x", std::to_string(tank->vec_x));
                core_->get_config().set<std::string>("player.position.y", std::to_string(tank->vec_y));
            }
            handle_client_actions(event, tank);
            return;
        }

        if (event.from == core::EventFrom::FromServer) {
            if (tank->vec_x >= 0 && tank->vec_y >= 0) {
                utils::PlayerTracker::get_instance().update_player_position(
                    tank->net_id, tank->vec_x, tank->vec_y);
            }
            handle_server_updates(event, tank);
            return;
        }
    }

    bool is_punch_action(const packet::TankUpdatePacket* tank) const {
        if (tank->type == packet::PACKET_TILE_CHANGE_REQUEST) return true;
        if ((tank->flags & packet::PACKET_FLAG_ON_PUNCHED) != 0) return true;
        if ((tank->flags & packet::PACKET_FLAG_ON_TILE_ACTION) != 0) return true;
        if (tank->animation_type == 3) return true;
        return false;
    }

    void handle_client_actions(const core::EventPacket& event, const packet::TankUpdatePacket* tank) {
        bool needs_modification = false;
        packet::TankUpdatePacket modified_tank = *tank;
        
        // Double Jump Mod
        if (core_->get_config().get<bool>("features.double_jump")) {
            bool jumping = (tank->flags & packet::PACKET_FLAG_ON_JUMP) != 0;
            bool on_solid = (tank->flags & packet::PACKET_FLAG_ON_SOLID) != 0;
            
            if (jumping) {
                if (!on_solid) {
                    consecutive_jumps_++;
                } else {
                    consecutive_jumps_ = 1;
                }
                
                modified_tank.flags |= packet::PACKET_FLAG_ON_SOLID;
                needs_modification = true;
            } else {
                if (consecutive_jumps_ > 0) {
                    consecutive_jumps_ = 0;
                }
            }
        }
        
        if (needs_modification) {
            send_modified_packet(event, modified_tank);
        }

        // Visual weapon animation on client punch
        auto hand_it = command::g_clothing_slots.find(5);
        if (hand_it != command::g_clothing_slots.end() && hand_it->second > 0) {
            if (is_punch_action(tank)) {
                uint32_t hand_id = static_cast<uint32_t>(hand_it->second);
                uint32_t my_netid = get_local_netid();
                uint8_t anim = utils::WeaponAnimationManager::get_instance().get_anim_type(hand_id);

                packet::TankUpdatePacket punch_tank = *tank;
                if (punch_tank.vec_x <= 0.0f || punch_tank.vec_y <= 0.0f) {
                    punch_tank.vec_x = local_x_;
                    punch_tank.vec_y = local_y_;
                }
                utils::WeaponAnimationManager::get_instance().play_weapon_effects(core_, hand_id, my_netid, &punch_tank);

                // Play predicted weapon animation locally for client avatar
                if (core_ && core_->get_server() && core_->get_server()->get_player()) {
                    packet::TankUpdatePacket anim_tank = *tank;
                    anim_tank.type = packet::PACKET_STATE;
                    anim_tank.net_id = static_cast<int32_t>(my_netid);
                    anim_tank.animation_type = anim;
                    anim_tank.int_data = hand_id;
                    anim_tank.flags |= packet::PACKET_FLAG_ON_PUNCHED;
                    if (anim_tank.vec_x <= 0.0f || anim_tank.vec_y <= 0.0f) {
                        anim_tank.vec_x = local_x_;
                        anim_tank.vec_y = local_y_;
                    }
                    if (anim == 4 && anim_tank.vec_x2 == 0.0f && anim_tank.vec_y2 == 0.0f) {
                        bool left = (anim_tank.flags & packet::PACKET_FLAG_ROTATE_LEFT) != 0;
                        anim_tank.vec_x2 = left ? -200.0f : 200.0f;
                        anim_tank.vec_y2 = 0.0f;
                    }

                    ByteStream<std::uint16_t> bs{};
                    bs.write(packet::NET_MESSAGE_GAME_PACKET);
                    bs.write(anim_tank);
                    core_->get_server()->get_player()->send_packet(bs.get_data(), 0);
                }
            }
        }
    }

    void handle_server_updates(const core::EventPacket& event, const packet::TankUpdatePacket* tank) {
        bool needs_modification = false;
        packet::TankUpdatePacket modified_tank = *tank;
        
        uint32_t my_netid = get_local_netid();
        bool is_our_player = (my_netid > 0 && tank->net_id == static_cast<int32_t>(my_netid)) || (tank->net_id == local_netid_) || (my_netid == 0);
        
        // Custom weapon swing logic on server echo back to client
        auto hand_it = command::g_clothing_slots.find(5);
        if (hand_it != command::g_clothing_slots.end() && hand_it->second > 0 && is_our_player) {
            uint32_t hand_id = static_cast<uint32_t>(hand_it->second);
            if (is_punch_action(tank)) {
                uint8_t anim = utils::WeaponAnimationManager::get_instance().get_anim_type(hand_id);
                modified_tank.int_data = hand_id;
                modified_tank.animation_type = anim;
                modified_tank.flags |= packet::PACKET_FLAG_ON_PUNCHED;
                if (anim == 4 && modified_tank.vec_x2 == 0.0f && modified_tank.vec_y2 == 0.0f) {
                    bool left = (modified_tank.flags & packet::PACKET_FLAG_ROTATE_LEFT) != 0;
                    modified_tank.vec_x2 = left ? -200.0f : 200.0f;
                    modified_tank.vec_y2 = 0.0f;
                }
                needs_modification = true;
            }
        } else if (is_our_player && is_punch_action(tank)) {
            auto clothing = utils::PlayerTracker::get_instance().get_clothing();
            uint32_t real_hand = static_cast<uint32_t>(clothing.hand);
            if (real_hand > 0) {
                utils::WeaponAnimationManager::get_instance().capture_real_punch(real_hand, tank);
            }
        }

        // Damage Immunity
        if (is_our_player && core_->get_config().get<bool>("features.immune_damage")) {
            bool fire_damage = (tank->flags & packet::PACKET_FLAG_ON_FIRE_DAMAGE) != 0;
            bool acid_damage = (tank->flags & packet::PACKET_FLAG_ON_ACID_DAMAGE) != 0;
            
            if (fire_damage) {
                modified_tank.flags &= ~packet::PACKET_FLAG_ON_FIRE_DAMAGE;
                needs_modification = true;
            }
            if (acid_damage) {
                modified_tank.flags &= ~packet::PACKET_FLAG_ON_ACID_DAMAGE;
                needs_modification = true;
            }
        }
        
        if (needs_modification) {
            send_modified_packet(event, modified_tank);
        }
    }

    void send_modified_packet(const core::EventPacket& event, const packet::TankUpdatePacket& modified_tank) {
        ByteStream<std::uint16_t> byte_stream{};
        byte_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        
        const std::byte* tank_bytes = reinterpret_cast<const std::byte*>(&modified_tank);
        byte_stream.write_data(tank_bytes, sizeof(packet::TankUpdatePacket));

        const auto& ext_data = event.get_ext_data();
        if (!ext_data.empty()) {
            byte_stream.write_data(ext_data.data(), ext_data.size());
        }
        
        event.get_target().send_packet(byte_stream.get_data(), 0);
        const_cast<core::EventPacket&>(event).canceled = true;
    }
};

}