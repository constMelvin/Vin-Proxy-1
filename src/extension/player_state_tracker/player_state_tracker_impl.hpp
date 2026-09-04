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
    extern std::unordered_map<int, int> g_clothing_anim_types;
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
        spdlog::trace("`2[PlayerStateTracker]`` Initializing");
        
        
        core_->get_config().set("features.double_jump", true);
        core_->get_config().set("features.immune_damage", false);
        
        spdlog::trace("`2[PlayerStateTracker]`` Double jump `2ENABLED`` by default");
        spdlog::trace("`o[PlayerStateTracker]`` Damage immunity `4DISABLED`` - use /immune to enable");

        
        core_->get_event_dispatcher().prependListener(
            core::EventType::Packet,
            [this](const core::EventPacket& event) {
                handle_packet(event);
            }
        );
        
        spdlog::trace("`2[PlayerStateTracker]`` System ready");
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
            tank = reinterpret_cast<const packet::TankUpdatePacket*>(&game_packet);
        }
        if (!tank) return;

        // Log item activation (equip / click)
        if (game_packet.type == packet::PACKET_ITEM_ACTIVATE_REQUEST) {
            uint32_t item_id = tank->int_data != 0 ? tank->int_data : static_cast<uint32_t>(tank->target_net_id);
            spdlog::info("\033[33m====================================================\033[0m");
            spdlog::info("\033[33m[DEBUG: ITEM ACTIVATE REQUEST]\033[0m From: {}",
                         event.from == core::EventFrom::FromClient ? "CLIENT" : "SERVER");
            spdlog::info("  Item ID: {} | target_net_id: {} | flags: 0x{:X}", item_id, tank->target_net_id, tank->flags);
            spdlog::info("\033[33m====================================================\033[0m");
            return;
        }

        if (game_packet.type != packet::PACKET_STATE && game_packet.type != packet::PACKET_TILE_CHANGE_REQUEST) {
            return;
        }
        
        if (event.from == core::EventFrom::FromClient) {
            spdlog::debug("\033[36m[STATE-CLIENT]\033[0m NetID: {} | vec_x: {:.1f}, vec_y: {:.1f} | flags: 0x{:X}", 
                       tank->net_id, tank->vec_x, tank->vec_y, tank->flags);
        } else {
            spdlog::debug("\033[35m[STATE-SERVER]\033[0m NetID: {} | vec_x: {:.1f}, vec_y: {:.1f} | flags: 0x{:X}", 
                       tank->net_id, tank->vec_x, tank->vec_y, tank->flags);
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
                int tile_x = static_cast<int>(tank->vec_x / 32.0f);
                int tile_y = static_cast<int>(tank->vec_y / 32.0f);
                uint32_t my_netid = get_local_netid();
                bool is_local_player = (my_netid > 0 && tank->net_id == static_cast<int32_t>(my_netid));
                std::string player_type = is_local_player ? "[LOCAL]" : "[OTHER]";
                
                spdlog::info("\033[34m[SERVER POS]\033[0m {} NetID: {} | Tile ({}, {}) | Pixels ({:.1f}, {:.1f})", 
                           player_type, tank->net_id, tile_x, tile_y, tank->vec_x, tank->vec_y);
                
                utils::PlayerTracker::get_instance().update_player_position(
                    tank->net_id, tank->vec_x, tank->vec_y);
            }
            handle_server_updates(event, tank);
            return;
        }
    }

    void trigger_hand_particle_effect(uint32_t hand_id, float x, float y) {
        if (!core_ || !core_->get_server() || !core_->get_server()->get_player()) return;
        
        uint32_t particle_id = 0;
        if (hand_id == 5480 || hand_id == 5482) particle_id = 112;
        else if (hand_id == 2754 || hand_id == 2756) particle_id = 88;
        else if (hand_id == 1748 || hand_id == 1750) particle_id = 40;
        else if (hand_id == 550 || hand_id == 8960) particle_id = 88;
        else if (hand_id == 7940) particle_id = 40;
        else return;
        
        packet::Variant var{};
        var.add("OnParticleEffect");
        var.add(particle_id);
        var.add(x);
        var.add(y);
        
        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_netid_ > 0 ? local_netid_ : -1;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());
        
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());
        
        core_->get_server()->get_player()->send_packet(bs.get_data(), 0);
    }

    uint8_t get_hand_wire_anim_type(uint32_t hand_id) const {
        auto anim_it = command::g_clothing_anim_types.find(5);
        int stored = (anim_it != command::g_clothing_anim_types.end()) ? anim_it->second : 0;
        if (stored > 0) {
            return static_cast<uint8_t>(stored & 0xFF);
        }
        if (hand_id == 5480 || hand_id == 5482) return 32; // Rayman's fist
        if (hand_id == 7940) return 3; // Flaming boxing gloves
        if (hand_id == 8960) return 33; // Spring-loaded fists
        return 0;
    }

    bool is_punch_action(const packet::TankUpdatePacket* tank) const {
        if (tank->type == packet::PACKET_TILE_CHANGE_REQUEST) return true;
        if ((tank->flags & packet::PACKET_FLAG_ON_PUNCHED) != 0) return true;
        if (tank->animation_type == 3) return true;
        return false;
    }

    void handle_client_actions(const core::EventPacket& event, const packet::TankUpdatePacket* tank) {
        bool needs_modification = false;
        packet::TankUpdatePacket modified_tank = *tank;
        
        spdlog::debug("[CLIENT STATE] Processing - flags: 0x{:X}", tank->flags);
        
        // Custom punch animation & weapon effects for visual hand items
        auto hand_it = command::g_clothing_slots.find(5);
        if (hand_it != command::g_clothing_slots.end() && hand_it->second > 0) {
            uint32_t hand_id = static_cast<uint32_t>(hand_it->second);
            if (is_punch_action(tank)) {
                uint32_t my_netid = get_local_netid();
                spdlog::info("\033[36m====================================================\033[0m");
                spdlog::info("\033[36m[VISUAL ITEM PUNCH - CLIENT ACTION]\033[0m");
                spdlog::info("  Visual Hand Item: {} (netID={})", hand_id, my_netid);
                spdlog::info("  Packet Type: {} | int_data: {} | anim_type: {}", (int)tank->type, tank->int_data, (int)tank->animation_type);
                spdlog::info("  Flags: 0x{:X} | Tile: ({}, {}) | Pos: ({:.2f}, {:.2f})", tank->flags, tank->int_x, tank->int_y, tank->vec_x, tank->vec_y);
                spdlog::info("  Vec2: ({:.2f}, {:.2f}) | float_var: {} | target: {}", tank->vec_x2, tank->vec_y2, tank->float_var, tank->target_net_id);
                spdlog::info("\033[36m====================================================\033[0m");

                utils::WeaponAnimationManager::get_instance().play_weapon_effects(core_, hand_id, my_netid, tank);

                // Real server must receive 18 (fist) so it accepts the action
                if (modified_tank.int_data != 18 && modified_tank.int_data != 0) {
                    modified_tank.int_data = 18;
                    needs_modification = true;
                }
            }
        } else if (is_punch_action(tank)) {
            // REAL ITEM PUNCH: User is punching with their own real item!
            auto clothing = utils::PlayerTracker::get_instance().get_clothing();
            uint32_t real_hand = static_cast<uint32_t>(clothing.hand);

            spdlog::info("\033[32m====================================================\033[0m");
            spdlog::info("\033[32m[REAL OWN ITEM PUNCH - CLIENT ACTION]\033[0m");
            spdlog::info("  Real Hand Item: {} | Active int_data: {}", real_hand, tank->int_data);
            spdlog::info("  Packet Type: {} | anim_type: {} | flags: 0x{:X}", (int)tank->type, (int)tank->animation_type, tank->flags);
            spdlog::info("  Tile: ({}, {}) | Pos: ({:.2f}, {:.2f})", tank->int_x, tank->int_y, tank->vec_x, tank->vec_y);
            spdlog::info("  Vec2: ({:.2f}, {:.2f}) | float_var: {} | target: {}", tank->vec_x2, tank->vec_y2, tank->float_var, tank->target_net_id);
            spdlog::info("\033[32m====================================================\033[0m");

            // Capture the real item's punch profile for 1:1 reproduction!
            utils::WeaponAnimationManager::get_instance().capture_real_punch(real_hand, tank);
        }

        if (tank->vec_x > 0 && tank->vec_y > 0) {
            int tile_x = static_cast<int>(tank->vec_x / 32.0f);
            int tile_y = static_cast<int>(tank->vec_y / 32.0f);
            spdlog::debug("[CLIENT POS] Tile ({}, {}) | Pixels ({:.0f}, {:.0f})", 
                        tile_x, tile_y, tank->vec_x, tank->vec_y);
        }
        
        bool jumping = (tank->flags & packet::PACKET_FLAG_ON_JUMP) != 0;
        bool on_solid = (tank->flags & packet::PACKET_FLAG_ON_SOLID) != 0;
        
        spdlog::info("\033[33m[FLAG CHECK]\033[0m jumping={}, on_solid={}", jumping, on_solid);
        
        if (core_->get_config().get<bool>("features.double_jump")) {
            if (jumping) {
                if (!on_solid) {
                    consecutive_jumps_++;
                    spdlog::info("\033[32m[DOUBLE JUMP]\033[0m Mid-air jump #{} at tile ({}, {})", 
                               consecutive_jumps_, 
                               static_cast<int>(tank->vec_x / 32.0f),
                               static_cast<int>(tank->vec_y / 32.0f));
                } else {
                    consecutive_jumps_ = 1;
                    spdlog::debug("[JUMP] Ground jump at tile ({}, {})", 
                                static_cast<int>(tank->vec_x / 32.0f),
                                static_cast<int>(tank->vec_y / 32.0f));
                }
                
                modified_tank.flags |= packet::PACKET_FLAG_ON_SOLID;
                needs_modification = true;
                
                spdlog::info("\033[32m[DOUBLE JUMP]\033[0m Modified - Original: 0x{:X} -> New: 0x{:X}", 
                           tank->flags, modified_tank.flags);
            } else {
                if (consecutive_jumps_ > 0) {
                    spdlog::debug("[JUMP] Sequence ended ({} jumps total)", consecutive_jumps_);
                    consecutive_jumps_ = 0;
                }
            }
        }
        
        if (needs_modification) {
            spdlog::info("\033[32m[SENDING MODIFIED PACKET]\033[0m For client action modification");
            send_modified_packet(event, modified_tank);
        }
    }

    void handle_server_updates(const core::EventPacket& event, const packet::TankUpdatePacket* tank) {
        bool needs_modification = false;
        packet::TankUpdatePacket modified_tank = *tank;
        
        uint32_t my_netid = get_local_netid();
        bool is_our_player = (my_netid > 0 && tank->net_id == static_cast<int32_t>(my_netid)) || (tank->net_id == local_netid_) || (my_netid == 0);
        
        spdlog::info("\033[33m[SERVER STATE]\033[0m NetID: {} (our={}) - flags: 0x{:X}", tank->net_id, my_netid, tank->flags);
        
        // Custom punch animation for visual hand items on server updates
        auto hand_it = command::g_clothing_slots.find(5);
        if (hand_it != command::g_clothing_slots.end() && hand_it->second > 0 && is_our_player) {
            uint32_t hand_id = static_cast<uint32_t>(hand_it->second);
            if (is_punch_action(tank)) {
                modified_tank.int_data = hand_id;
                uint8_t anim = utils::WeaponAnimationManager::get_instance().get_anim_type(hand_id);
                modified_tank.animation_type = (anim > 0) ? anim : 3;
                needs_modification = true;
                spdlog::info("[PUNCH ANIMATION] Server punch echo -> hand item: {}, anim_type: {}", hand_id, static_cast<int>(modified_tank.animation_type));
            }
        } else if (is_our_player && is_punch_action(tank)) {
            spdlog::info("\033[32m====================================================\033[0m");
            spdlog::info("\033[32m[REAL OWN ITEM PUNCH - SERVER RESPONSE]\033[0m");
            spdlog::info("  NetID: {} | int_data: {} | anim_type: {} | flags: 0x{:X}", 
                         tank->net_id, tank->int_data, static_cast<int>(tank->animation_type), tank->flags);
            spdlog::info("  Vec2: ({:.2f}, {:.2f}) | Pos: ({:.2f}, {:.2f}) | float_var: {}", 
                         tank->vec_x2, tank->vec_y2, tank->vec_x, tank->vec_y, tank->float_var);
            spdlog::info("\033[32m====================================================\033[0m");
        }

        if (is_our_player && tank->vec_x > 0 && tank->vec_y > 0) {
            int tile_x = static_cast<int>(tank->vec_x / 32.0f);
            int tile_y = static_cast<int>(tank->vec_y / 32.0f);
            spdlog::debug("[SERVER UPDATE] Position confirmed: tile ({}, {})", tile_x, tile_y);
        }
        
        bool fire_damage = (tank->flags & packet::PACKET_FLAG_ON_FIRE_DAMAGE) != 0;
        bool acid_damage = (tank->flags & packet::PACKET_FLAG_ON_ACID_DAMAGE) != 0;
        
        spdlog::info("\033[33m[DAMAGE FLAGS]\033[0m fire={}, acid={}", fire_damage, acid_damage);
        
        if (is_our_player && (fire_damage || acid_damage)) {
            spdlog::info("\033[31m[DAMAGE DETECTED]\033[0m Fire: {} | Acid: {} at tile ({}, {})", 
                       fire_damage, acid_damage,
                       static_cast<int>(tank->vec_x / 32.0f),
                       static_cast<int>(tank->vec_y / 32.0f));
        }
        
        if (is_our_player && core_->get_config().get<bool>("features.immune_damage")) {
            spdlog::info("\033[33m[IMMUNITY CHECK]\033[0m Immunity is ENABLED");
            if (fire_damage) {
                modified_tank.flags &= ~packet::PACKET_FLAG_ON_FIRE_DAMAGE;
                needs_modification = true;
                spdlog::info("\033[32m[IMMUNE]\033[0m Blocked FIRE damage!");
            }
            
            if (acid_damage) {
                modified_tank.flags &= ~packet::PACKET_FLAG_ON_ACID_DAMAGE;
                needs_modification = true;
                spdlog::info("\033[32m[IMMUNE]\033[0m Blocked ACID damage!");
            }
            
            if (needs_modification) {
                spdlog::info("\033[32m[IMMUNE]\033[0m Modified packet - Original: 0x{:X} -> New: 0x{:X}", 
                           tank->flags, modified_tank.flags);
            }
        }
        
        if (needs_modification) {
            spdlog::info("\033[32m[SENDING MODIFIED PACKET]\033[0m For server update modification");
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
        
        spdlog::debug("[PlayerStateTracker] Sent modified packet, canceled original");
    }
};

} 
