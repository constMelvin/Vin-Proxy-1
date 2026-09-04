#pragma once
#include "../packet/tank_packet.hpp"
#include "../packet/packet_variant.hpp"
#include "../packet/packet_types.hpp"
#include "../utils/byte_stream.hpp"
#include "../core/core.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <spdlog/spdlog.h>

namespace utils {

enum class WeaponType {
    BOW,
    SWORD,
    GUN,
    HAMMER,
    STAFF,
    SCYTHE,
    RAYMAN,
    FLAMING_GLOVES,
    SPRING_FIST,
    BOXING,
    TOOL,
    GENERIC
};

struct WeaponProfile {
    WeaponType type{ WeaponType::GENERIC };
    uint8_t anim_type{ 3 };
    uint32_t anim_item_id{ 0 };
    std::string sound_file{ "audio/punch.wav" };
    std::vector<uint32_t> particle_ids{};
    bool is_projectile{ false };
};

struct CapturedPunchProfile {
    uint32_t hand_id{ 0 };
    uint8_t packet_type{ 0 };
    uint8_t animation_type{ 0 };
    uint32_t flags{ 0 };
    uint32_t int_data{ 0 };
    float float_var{ 0.0f };
    float vec_x2{ 0.0f };
    float vec_y2{ 0.0f };
    bool is_valid{ false };
};

class WeaponAnimationManager {
public:
    static WeaponAnimationManager& get_instance() {
        static WeaponAnimationManager instance;
        return instance;
    }

    void capture_real_punch(uint32_t hand_id, const packet::TankUpdatePacket* tank) {
        if (!tank) return;
        CapturedPunchProfile cap;
        cap.hand_id = hand_id;
        cap.packet_type = tank->type;
        cap.animation_type = tank->animation_type;
        cap.flags = tank->flags;
        cap.int_data = tank->int_data;
        cap.float_var = tank->float_var;
        cap.vec_x2 = tank->vec_x2;
        cap.vec_y2 = tank->vec_y2;
        cap.is_valid = true;

        captured_profiles_[hand_id] = cap;
        last_captured_ = cap;

        spdlog::info("\033[32m[ANIMATION-COPIER] Captured authentic real punch profile for item {}!\033[0m", hand_id);
        spdlog::info("  Flags: 0x{:X} | anim_type: {} | int_data: {} | vec2: ({:.2f}, {:.2f})", 
                     cap.flags, (int)cap.animation_type, cap.int_data, cap.vec_x2, cap.vec_y2);
    }

    void set_server_character_state(const packet::TankUpdatePacket* pkt) {
        if (pkt) {
            last_server_char_state_ = *pkt;
            has_server_char_state_ = true;
        }
    }

    const packet::TankUpdatePacket* get_server_character_state() const {
        return has_server_char_state_ ? &last_server_char_state_ : nullptr;
    }

    const CapturedPunchProfile* get_captured_profile(uint32_t hand_id) const {
        auto it = captured_profiles_.find(hand_id);
        if (it != captured_profiles_.end() && it->second.is_valid) {
            return &it->second;
        }
        if (last_captured_.is_valid) {
            return &last_captured_;
        }
        return nullptr;
    }

    const CapturedPunchProfile& get_last_captured() const {
        return last_captured_;
    }

    WeaponProfile get_profile(uint32_t hand_id) const {
        WeaponProfile prof;

        // Specific high-profile hand items
        switch (hand_id) {
            // Bows
            case 4136: // Heatbow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = 4136;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 217, 40, 304 }; // Flaming whirlwind, fire burst, burn
                prof.is_projectile = true;
                return prof;

            case 366: // Heartbow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = 366;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 190 }; // Hearts
                prof.is_projectile = true;
                return prof;

            case 1464: // Golden Heartbow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = 1464;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 191, 192 }; // Golden hearts, golden slash
                prof.is_projectile = true;
                return prof;

            case 594: // Elvish Longbow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = 594;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 603 };
                prof.is_projectile = true;
                return prof;

            case 2720: // Electric Bow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = 2720;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 88 }; // Sparks
                prof.is_projectile = true;
                return prof;

            case 5424: // Winter Frost Bow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = 5424;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 603, 562 }; // Frost crystals
                prof.is_projectile = true;
                return prof;

            case 5456: // Silverstar Bow
            case 10052: // Eldritch Crossbow
            case 10130: // Sun Shooter Bow
            case 10670: // Bow of the Rainbow
            case 11380: // Black Bow of the Rainbow
            case 11438: // Digital Bow
            case 11440: // Mystic Bow
            case 11442: // Royal Mystic Bow
            case 12870: // Candy Cane Bow
            case 14834: // Primal Bow
                prof.type = WeaponType::BOW;
                prof.anim_type = 4;
                prof.anim_item_id = hand_id;
                prof.sound_file = "audio/arrow_thwip.wav";
                prof.particle_ids = { 603 };
                prof.is_projectile = true;
                return prof;

            // Rayman's Fist
            case 5480:
            case 5482:
                prof.type = WeaponType::RAYMAN;
                prof.anim_type = 32;
                prof.anim_item_id = 5480;
                prof.sound_file = "audio/punch.wav";
                prof.particle_ids = { 112 }; // Flying fist
                prof.is_projectile = true;
                return prof;

            // Flaming Boxing Gloves
            case 7940:
                prof.type = WeaponType::FLAMING_GLOVES;
                prof.anim_type = 3;
                prof.anim_item_id = 7940;
                prof.sound_file = "audio/fireball.wav";
                prof.particle_ids = { 217, 40 };
                return prof;

            // Spring-Loaded Fists
            case 8960:
                prof.type = WeaponType::SPRING_FIST;
                prof.anim_type = 33;
                prof.anim_item_id = 8960;
                prof.sound_file = "audio/swoosh.wav";
                prof.particle_ids = { 88 };
                return prof;

            // Swords
            case 94: // Classic Sword
            case 604: // Crystal Sword
            case 7830: // Heartsword
            case 7832: // Golden Heartsword
            case 9116: // Red Laser Scimitar
            case 9118: // Green Laser Scimitar
            case 9120: // Blue Laser Scimitar
            case 9122: // Purple Laser Scimitar
            case 9396: // Balrog's Tail
            case 10334: // Black Balrog's Tail
            case 10402: // Christmas Tree Sword
            case 12342: // Mageblade
            case 14018: // Dual Swords
            case 14764: // Blade of Revolt
                prof.type = WeaponType::SWORD;
                prof.anim_type = 3;
                prof.anim_item_id = hand_id;
                prof.sound_file = "audio/slash.wav";
                prof.particle_ids = (hand_id == 7830 || hand_id == 7832) ? std::vector<uint32_t>{ 190, 192 } : std::vector<uint32_t>{ 253, 192 };
                return prof;

            // Guns / Blasters
            case 472: // Tommy Gun
            case 768: // Six Shooter
            case 10626: // Rose Rifle
            case 10724: // Baaaa Blaster
            case 11312: // Spider Sniper
                prof.type = WeaponType::GUN;
                prof.anim_type = 4;
                prof.anim_item_id = hand_id;
                prof.sound_file = "audio/laser.wav";
                prof.particle_ids = { 285 };
                prof.is_projectile = true;
                return prof;

            // Hammers
            case 7912: // War Hammers of Darkness
                prof.type = WeaponType::HAMMER;
                prof.anim_type = 33;
                prof.anim_item_id = 7912;
                prof.sound_file = "audio/hammer_rumble.wav";
                prof.particle_ids = { 216 };
                return prof;

            // Staves
            case 7826: // Heartstaff
            case 7828: // Golden Heartstaff
            case 10406: // Staff of Winter
                prof.type = WeaponType::STAFF;
                prof.anim_type = 33;
                prof.anim_item_id = hand_id;
                prof.sound_file = "audio/swoosh.wav";
                prof.particle_ids = { 190 };
                return prof;

            // Scythes / Lances / Spears
            case 7736: // Dragon Knight's Spear
            case 8492: // Wormtouch
            case 9758: // Neptune's Trident
            case 10498: // Candy Cane Scythe
            case 10578: // Mystic Battle Lance
            case 10680: // Finias' Red Javelin
            case 12886: // Icicle Lance
                prof.type = WeaponType::SCYTHE;
                prof.anim_type = 33;
                prof.anim_item_id = hand_id;
                prof.sound_file = "audio/buster.wav";
                prof.particle_ids = { 193, 603 };
                return prof;

            // Tools / Pickaxes
            case 98: // Pickaxe
            case 4840: // Golden Pickaxe
            case 10780: // Giving Growaxe
            case 12368: // Strange Eddie Axe
                prof.type = WeaponType::TOOL;
                prof.anim_type = 3;
                prof.anim_item_id = hand_id;
                prof.sound_file = "audio/slash.wav";
                prof.particle_ids = { 109 };
                return prof;

            default:
                break;
        }

        // Default fallback for any other hand item
        prof.type = WeaponType::GENERIC;
        prof.anim_type = 3;
        prof.sound_file = "audio/punch.wav";
        prof.particle_ids = { 143 };
        return prof;
    }

    uint8_t get_anim_type(uint32_t hand_id) const {
        return get_profile(hand_id).anim_type;
    }

    void play_weapon_effects(core::Core* core, uint32_t hand_id, uint32_t net_id, const packet::TankUpdatePacket* tank) {
        if (!core || !core->get_server() || !core->get_server()->get_player() || !tank) {
            return;
        }

        auto* client_player = core->get_server()->get_player();
        WeaponProfile prof = get_profile(hand_id);

        // 1. Play weapon sound effect (both packet types to guarantee playback)
        if (!prof.sound_file.empty()) {
            // Variant method: OnPlaySound
            packet::Variant snd_var{};
            snd_var.add("OnPlaySound");
            snd_var.add(prof.sound_file);
            snd_var.add(0);

            std::vector<std::byte> snd_data = snd_var.serialize();
            packet::GameUpdatePacket snd_pkt{};
            snd_pkt.type = packet::PACKET_CALL_FUNCTION;
            snd_pkt.net_id = 0xFFFFFFFF;
            snd_pkt.flags.extended = 1;
            snd_pkt.data_size = static_cast<uint32_t>(snd_data.size());

            ByteStream<std::uint16_t> snd_bs{};
            snd_bs.write(packet::NET_MESSAGE_GAME_PACKET);
            snd_bs.write(snd_pkt);
            snd_bs.write_data(snd_data.data(), snd_data.size());
            client_player->send_packet(snd_bs.get_data(), 0);
        }

        // 2. Trigger OnItemEffect for visual aura / particles
        {
            packet::Variant eff_var{};
            eff_var.add("OnItemEffect");
            eff_var.add(static_cast<int32_t>(net_id));
            eff_var.add(static_cast<int32_t>(hand_id));

            std::vector<std::byte> eff_data = eff_var.serialize();
            packet::GameUpdatePacket eff_pkt{};
            eff_pkt.type = packet::PACKET_CALL_FUNCTION;
            eff_pkt.net_id = 0xFFFFFFFF;
            eff_pkt.flags.extended = 1;
            eff_pkt.data_size = static_cast<uint32_t>(eff_data.size());

            ByteStream<std::uint16_t> eff_bs{};
            eff_bs.write(packet::NET_MESSAGE_GAME_PACKET);
            eff_bs.write(eff_pkt);
            eff_bs.write_data(eff_data.data(), eff_data.size());

            client_player->send_packet(eff_bs.get_data(), 0);
        }

        // 3. Spawn projectile / impact particles
        float px = tank->vec_x;
        float py = tank->vec_y;
        float tx = px;
        float ty = py;

        if (tank->int_x > 0 && tank->int_y > 0) {
            tx = tank->int_x * 32.0f + 16.0f;
            ty = tank->int_y * 32.0f + 16.0f;
        } else {
            bool facing_left = (tank->flags & packet::PACKET_FLAG_ROTATE_LEFT) != 0;
            tx = px + (facing_left ? -96.0f : 96.0f);
            ty = py;
        }

        if (prof.is_projectile) {
            // Send smooth moving projectile particle from player to target
            uint32_t proj_p = prof.particle_ids.empty() ? 217 : prof.particle_ids[0];
            send_projectile(client_player, proj_p, px, py, tx, ty);

            // Send impact burst at target tile
            uint32_t impact_p = prof.particle_ids.size() > 1 ? prof.particle_ids[1] : 143;
            send_particle(client_player, impact_p, tx, ty);
        } else {
            // Melee slash / smash effect directly on target tile
            if (!prof.particle_ids.empty()) {
                send_particle(client_player, prof.particle_ids[0], tx, ty);
            }
        }

        // 4. Send visual_punch PACKET_STATE to client so Growtopia renders the authentic weapon swing!
        {
            packet::TankUpdatePacket visual_punch = *tank;
            visual_punch.type = static_cast<uint8_t>(packet::PACKET_STATE);
            visual_punch.net_id = static_cast<int32_t>(net_id);

            // Keep direction and movement without forcing extended fist punch flags for melee
            visual_punch.flags = (tank->flags & (packet::PACKET_FLAG_ROTATE_LEFT | packet::PACKET_FLAG_ON_SOLID | packet::PACKET_FLAG_ON_JUMP));

            auto captured = get_captured_profile(hand_id);
            if (captured && captured->is_valid) {
                // Exact 1:1 replica of authentic punch captured from real item!
                visual_punch.animation_type = captured->animation_type;
                visual_punch.int_data = captured->int_data;
                visual_punch.flags |= captured->flags;
            } else {
                visual_punch.int_data = (prof.anim_item_id != 0) ? prof.anim_item_id : hand_id;
                if (prof.type == WeaponType::SWORD || prof.type == WeaponType::TOOL) {
                    visual_punch.animation_type = 0;
                } else {
                    visual_punch.animation_type = prof.anim_type;
                    visual_punch.flags |= (packet::PACKET_FLAG_ON_PUNCHED | packet::PACKET_FLAG_ON_TILE_ACTION);
                }
            }

            ByteStream<std::uint16_t> bs{};
            bs.write(packet::NET_MESSAGE_GAME_PACKET);
            bs.write(visual_punch);

            client_player->send_packet(bs.get_data(), 0);
            spdlog::info("\033[35m[WeaponAnimationManager] Sent visual_punch (net_id={}, anim_type={}, flags=0x{:X}, int_data={})\033[0m",
                         visual_punch.net_id, (int)visual_punch.animation_type, visual_punch.flags, visual_punch.int_data);
        }

        spdlog::info("[WeaponAnimationManager] Played animation (hand_id={}, anim_item_id={}, anim_type={}, sound={})",
                     hand_id, prof.anim_item_id, static_cast<int>(prof.anim_type), prof.sound_file);
    }

private:
    void send_projectile(player::Player* client_player, uint32_t particle_id, float px, float py, float tx, float ty) {
        if (!client_player) return;

        packet::TankUpdatePacket pkt{};
        pkt.type = static_cast<uint8_t>(packet::PACKET_SEND_PARTICLE_EFFECT); // 17
        pkt.net_id = -1;
        pkt.flags = 0;
        pkt.int_data = particle_id;
        pkt.vec_x = px;
        pkt.vec_y = py;
        pkt.vec_x2 = tx;
        pkt.vec_y2 = ty;
        pkt.particle_time = 0.35f;

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);

        client_player->send_packet(bs.get_data(), 0);
    }

    void send_particle(player::Player* client_player, uint32_t particle_id, float x, float y) {
        if (!client_player) return;

        packet::Variant var{};
        var.add("OnParticleEffect");
        var.add(particle_id);
        var.add(glm::vec2(x, y));
        var.add(static_cast<uint32_t>(0));
        var.add(static_cast<uint32_t>(0));

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = -1;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        client_player->send_packet(bs.get_data(), 0);
    }

    std::unordered_map<uint32_t, CapturedPunchProfile> captured_profiles_;
    CapturedPunchProfile last_captured_{};
    packet::TankUpdatePacket last_server_char_state_{};
    bool has_server_char_state_ = false;
};

} // namespace utils
