#include "visual_items_manager.hpp"
#include "player_tracker.hpp"
#include "inventory_manager.hpp"
#include "../player/player.hpp"
#include "../core/core.hpp"
#include "../utils/byte_stream.hpp"
#include "../utils/text_parse.hpp"
#include "../extension/item_finder/item_finder.hpp"
#include "../proxy_imgui_gui.hpp"
#include "weapon_animation_manager.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace utils {

VisualItemsManager& VisualItemsManager::get_instance() {
    static VisualItemsManager instance;
    return instance;
}

VisualItemsManager::VisualItemsManager() = default;

bool VisualItemsManager::is_enabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_enabled_;
}

void VisualItemsManager::set_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    is_enabled_ = enabled;
}

bool VisualItemsManager::toggle_enabled() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_enabled_ = !is_enabled_;
    return is_enabled_;
}

bool VisualItemsManager::has_any_visual_equipped() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !visual_slots_.empty();
}

void VisualItemsManager::update_state_flags_locked() {
    auto hand_it = visual_slots_.find(5);
    if (hand_it != visual_slots_.end() && hand_it->second > 0) {
        current_punch_effect_ = get_punch_id(static_cast<uint32_t>(hand_it->second));
    } else {
        current_punch_effect_ = 0;
    }

    auto back_it = visual_slots_.find(6);
    double_jump_ = (back_it != visual_slots_.end() && back_it->second > 0);
}

void VisualItemsManager::set_visual_item(int clothing_type, int item_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (item_id > 0) {
        visual_slots_[clothing_type] = item_id;
        all_visual_items_.insert(static_cast<uint32_t>(item_id));
    } else {
        visual_slots_.erase(clothing_type);
    }
    update_state_flags_locked();
    spdlog::info("[VisualItemsManager] Set slot {} to item {} (punch_effect={}, double_jump={})",
                 clothing_type, item_id, (int)current_punch_effect_, double_jump_);
}

bool VisualItemsManager::toggle_visual_item(int clothing_type, int item_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (item_id > 0) {
        all_visual_items_.insert(static_cast<uint32_t>(item_id));
    }
    auto it = visual_slots_.find(clothing_type);
    if (it != visual_slots_.end() && it->second == item_id) {
        visual_slots_.erase(it);
        update_state_flags_locked();
        spdlog::info("[VisualItemsManager] Unequipped item {} from slot {}", item_id, clothing_type);
        return false;
    } else {
        visual_slots_[clothing_type] = item_id;
        update_state_flags_locked();
        spdlog::info("[VisualItemsManager] Equipped item {} in slot {} (punch_effect={}, double_jump={})",
                     item_id, clothing_type, (int)current_punch_effect_, double_jump_);
        return true;
    }
}

void VisualItemsManager::clear_visual_items() {
    std::lock_guard<std::mutex> lock(mutex_);
    visual_slots_.clear();
    all_visual_items_.clear();
    current_punch_effect_ = 0;
    double_jump_ = false;
    spdlog::info("[VisualItemsManager] Cleared all visual items.");
}

bool VisualItemsManager::is_item_equipped(int item_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [slot, id] : visual_slots_) {
        if (id == item_id) return true;
    }
    return false;
}

int VisualItemsManager::get_equipped_item(int slot) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = visual_slots_.find(slot);
    if (it != visual_slots_.end()) return it->second;
    return -1;
}

const std::unordered_map<int, int>& VisualItemsManager::get_visual_slots() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return visual_slots_;
}

const std::unordered_set<uint32_t>& VisualItemsManager::get_all_visual_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return all_visual_items_;
}

uint8_t VisualItemsManager::get_current_punch_effect() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_punch_effect_;
}

bool VisualItemsManager::has_double_jump() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return double_jump_;
}

uint8_t VisualItemsManager::get_punch_id(uint32_t item_id) const {
    switch (item_id) {
        case 138: return 1;
        case 366:
        case 1464: return 2;
        case 472: return 3;
        case 594:
        case 10130:
        case 5424:
        case 5456:
        case 4136:
        case 10052: return 4;
        case 768: return 5;
        case 900:
        case 7760:
        case 9272:
        case 5002:
        case 7758: return 6;
        case 910:
        case 4332: return 7;
        case 930:
        case 1010:
        case 6382: return 8;
        case 1016:
        case 6058: return 9;
        case 1204:
        case 9534: return 10;
        case 1378: return 11;
        case 1440: return 12;
        case 1484:
        case 5160:
        case 9802: return 13;
        case 1512:
        case 1648: return 14;
        case 1542: return 15;
        case 1576: return 16;
        case 1676:
        case 7504: return 17;
        case 1710:
        case 4644:
        case 1714:
        case 1712:
        case 6044:
        case 1570: return 18;
        case 1748:
        case 8006:
        case 8008:
        case 8010:
        case 8012: return 19;
        case 1780: return 20;
        case 1782:
        case 5156:
        case 9776:
        case 9782:
        case 9810: return 21;
        case 1804:
        case 5194:
        case 9784: return 22;
        case 1868:
        case 1998: return 23;
        case 1874: return 24;
        case 1946:
        case 2800: return 25;
        case 1952:
        case 2854: return 26;
        case 1956: return 27;
        case 1960: return 28;
        case 2908:
        case 6312:
        case 9496:
        case 8554:
        case 3162:
        case 9536:
        case 4956:
        case 3466:
        case 4166:
        case 4506:
        case 2952:
        case 9520:
        case 9522:
        case 8440:
        case 3932:
        case 3934:
        case 8732:
        case 3108:
        case 9766:
        case 12368: return 29;
        case 1980: return 30;
        case 2066:
        case 4150:
        case 11082:
        case 11080:
        case 11078: return 31;
        case 2212:
        case 5174:
        case 5004:
        case 5006:
        case 5008: return 32;
        case 2218: return 33;
        case 2220: return 34;
        case 2266: return 35;
        case 2386: return 36;
        case 2388: return 37;
        case 2450: return 38;
        case 2476:
        case 4208:
        case 12308:
        case 10336:
        case 9804: return 39;
        case 4748:
        case 4294: return 40;
        case 2512:
        case 9732:
        case 6338: return 41;
        case 2572: return 42;
        case 2592:
        case 9396:
        case 2596:
        case 9548:
        case 9812: return 43;
        case 2720: return 44;
        case 2752: return 45;
        case 2754: return 46;
        case 2756: return 47;
        case 2802: return 49;
        case 2866: return 50;
        case 2876: return 51;
        case 2878:
        case 2880: return 52;
        case 2906:
        case 4170:
        case 4278: return 53;
        case 2886: return 54;
        case 2890: return 55;
        case 2910: return 56;
        case 3066: return 57;
        case 3124: return 58;
        case 3168: return 59;
        case 3214:
        case 9194: return 60;
        case 7408:
        case 3238: return 61;
        case 3274: return 62;
        case 3300: return 64;
        case 3418: return 65;
        case 3476: return 66;
        case 3596: return 67;
        case 3686: return 68;
        case 3716: return 69;
        case 4290: return 71;
        case 4474: return 72;
        case 4464:
        case 9500: return 73;
        case 4746: return 75;
        case 4778:
        case 6026:
        case 7784: return 76;
        case 4996:
        case 3680:
        case 5176: return 77;
        case 4840: return 78;
        case 5206: return 79;
        case 5480:
        case 9770:
        case 9772: return 80;
        case 6110: return 81;
        case 6308: return 82;
        case 6310: return 83;
        case 6298: return 84;
        case 6756: return 85;
        case 7044: return 86;
        case 6892: return 87;
        case 6966: return 88;
        case 7088:
        case 11020: return 89;
        case 7098:
        case 9032: return 90;
        case 7192: return 91;
        case 7136:
        case 9738: return 92;
        case 3166: return 93;
        case 7216: return 94;
        case 7196:
        case 9340: return 95;
        case 7392:
        case 9604: return 96;
        case 7384: return 98;
        case 7414: return 99;
        case 7402: return 100;
        case 7424: return 101;
        case 7470: return 102;
        case 7488: return 103;
        case 7586:
        case 7646:
        case 9778: return 104;
        case 7650: return 105;
        case 6804:
        case 6358: return 106;
        case 7568:
        case 7570:
        case 7572:
        case 7574: return 107;
        case 7668: return 108;
        case 7660:
        case 9060: return 109;
        case 7584: return 110;
        case 7736:
        case 9116:
        case 9118:
        case 7826:
        case 7828:
        case 11440:
        case 11442:
        case 11312:
        case 7830:
        case 7832:
        case 10670:
        case 9120:
        case 9122:
        case 10680:
        case 10626:
        case 10578:
        case 10334:
        case 11380:
        case 11326:
        case 7912:
        case 11298:
        case 10498:
        case 12342: return 111;
        case 7836:
        case 7838:
        case 7840:
        case 7842: return 112;
        case 7950: return 113;
        case 8002: return 114;
        case 8022: return 116;
        case 8036: return 118;
        case 9348:
        case 8372: return 119;
        case 8038: return 120;
        case 8816:
        case 8818:
        case 8820:
        case 8822: return 128;
        case 8910: return 129;
        case 8942: return 130;
        case 8944:
        case 5276: return 131;
        case 8432:
        case 8434:
        case 8436:
        case 8950: return 132;
        case 8946:
        case 9576: return 133;
        case 8960: return 134;
        case 9006: return 135;
        case 9058: return 136;
        case 9082:
        case 9304: return 137;
        case 9066: return 138;
        case 9136: return 139;
        case 9138: return 140;
        case 9172: return 141;
        case 9254: return 143;
        case 9256: return 144;
        case 9236: return 145;
        case 9342: return 146;
        case 9542: return 147;
        case 9378: return 148;
        case 9376: return 149;
        case 9410: return 150;
        case 9462: return 151;
        case 9606: return 152;
        case 9716:
        case 5192: return 153;
        case 10048: return 167;
        case 10064: return 168;
        case 10046: return 169;
        case 10050: return 170;
        case 10128: return 171;
        case 10210:
        case 9544: return 172;
        case 10330: return 178;
        case 10398: return 179;
        case 10388:
        case 9524:
        case 9598: return 180;
        case 10442: return 184;
        case 10506: return 185;
        case 10652: return 188;
        case 10676: return 191;
        case 10694: return 193;
        case 10714: return 194;
        case 10724: return 195;
        case 10722: return 196;
        case 10754: return 197;
        case 10800: return 198;
        case 10888: return 199;
        case 10886:
        case 11308: return 200;
        case 10890: return 202;
        case 10922:
        case 9550: return 203;
        case 10990: return 205;
        case 10998: return 206;
        case 10952: return 207;
        case 11000: return 208;
        case 11006: return 209;
        case 11046: return 210;
        case 11052: return 211;
        case 10960: return 212;
        case 10956:
        case 9774: return 213;
        case 10958: return 214;
        case 10954: return 215;
        case 11076: return 216;
        case 11084: return 217;
        case 11118:
        case 9546:
        case 9574: return 218;
        case 11120: return 219;
        case 11116: return 220;
        case 11158: return 221;
        case 11162: return 222;
        case 11142: return 223;
        case 11232: return 224;
        case 11140: return 225;
        case 11248:
        case 9596:
        case 9636: return 226;
        case 11240: return 227;
        case 11250: return 228;
        case 11284: return 229;
        case 11292: return 231;
        case 11314: return 233;
        case 11316: return 234;
        case 11324: return 235;
        case 11354: return 236;
        case 11760:
        case 11464:
        case 11438:
        case 12230:
        case 11716:
        case 11718:
        case 11674:
        case 11630:
        case 11786:
        case 11872:
        case 11762:
        case 11994:
        case 12172:
        case 12184:
        case 11460:
        case 12014:
        case 12016:
        case 12018:
        case 12020:
        case 12022:
        case 12024:
        case 12246:
        case 12248:
        case 12176:
        case 12242:
        case 11622:
        case 12350:
        case 12300:
        case 12374:
        case 12356: return 237;
        case 11814:
        case 12232:
        case 12302: return 241;
        case 11548:
        case 11552: return 242;
        case 11704:
        case 11706: return 243;
        case 12180:
        case 12346:
        case 12344: return 244;
        case 11506:
        case 11508:
        case 11562:
        case 11768:
        case 11882:
        case 11720:
        case 11884: return 245;
        case 12432:
        case 12434: return 246;
        case 11818:
        case 11876:
        case 12000:
        case 12240:
        case 12642:
        case 12644: return 248;
        default: break;
    }

    // Dynamic database check fallback for newer items
    auto* db = GetItemDatabase();
    if (db) {
        const auto* item = db->get_item_by_id(item_id);
        if (item && item->clothing_type == 5 && item->anim_type > 0) {
            return static_cast<uint8_t>(item->anim_type);
        }
    }

    return 0;
}

void VisualItemsManager::restore_character_state(player::Player* client_player, uint32_t net_id) {
    if (!client_player) return;

    packet::TankUpdatePacket tank{};
    const auto* server_state = WeaponAnimationManager::get_instance().get_server_character_state();
    if (server_state) {
        tank = *server_state;
    } else {
        tank.flags = 0;
        tank.float_var = 200.0f;
        tank.vec_x = 1000.0f;
        tank.vec_y = 400.0f;
        tank.vec_x2 = 250.0f;
        tank.vec_y2 = 1000.0f;
        tank.jump_count = 128;
        tank.animation_type = 128;
    }

    tank.type = packet::PACKET_SET_CHARACTER_STATE;
    tank.net_id = static_cast<int32_t>(net_id);
    tank.target_net_id = 0;

    // Safety checks: ensure speeds are never 0 or negative
    if (tank.vec_x2 <= 0.0f) tank.vec_x2 = 250.0f;
    if (tank.vec_y2 <= 0.0f) tank.vec_y2 = 1000.0f;
    if (tank.vec_x <= 0.0f) tank.vec_x = 1000.0f;
    if (tank.vec_y <= 0.0f) tank.vec_y = 400.0f;
    if (tank.float_var <= 0.0f) tank.float_var = 200.0f;

    ByteStream<std::uint16_t> bs{};
    (void)bs.write(packet::NET_MESSAGE_GAME_PACKET);
    (void)bs.write(tank);
    client_player->send_packet(bs.get_data(), 0);

    spdlog::info("[VisualItemsManager] Restored character state to clean server state for netID={}", net_id);
}

void VisualItemsManager::send_character_state(player::Player* client_player, uint32_t net_id) {
    if (!client_player) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_enabled_ || visual_slots_.empty()) return;

    packet::TankUpdatePacket tank{};
    const auto* server_state = WeaponAnimationManager::get_instance().get_server_character_state();
    if (server_state) {
        tank = *server_state;
    } else {
        tank.flags = 0;
        tank.float_var = 200.0f;
        tank.vec_x = 1000.0f;
        tank.vec_y = 400.0f;
        tank.vec_x2 = 250.0f;
        tank.vec_y2 = 1000.0f;
        tank.jump_count = 128;
        tank.animation_type = 128;
    }

    tank.type = packet::PACKET_SET_CHARACTER_STATE; // 20 (0x14)
    tank.net_id = static_cast<int32_t>(net_id);
    tank.target_net_id = 0;

    if (current_punch_effect_ > 0) {
        tank.object_type = current_punch_effect_;
        tank.jump_count = 128;
        tank.animation_type = 128;
    }
    if (double_jump_) {
        tank.int_data |= 2; // double jump bit (1 << 1)
    }

    // Crucial: never let horizontal speed (vec_x2) or vertical gravity (vec_y2) be 0
    if (tank.vec_x2 <= 0.0f) tank.vec_x2 = 250.0f;
    if (tank.vec_y2 <= 0.0f) tank.vec_y2 = 1000.0f;
    if (tank.vec_x <= 0.0f) tank.vec_x = 1000.0f;
    if (tank.vec_y <= 0.0f) tank.vec_y = 400.0f;
    if (tank.float_var <= 0.0f) tank.float_var = 200.0f;
    if (tank.jump_count == 0) tank.jump_count = 128;
    if (tank.animation_type == 0) tank.animation_type = 128;

    ByteStream<std::uint16_t> bs{};
    (void)bs.write(packet::NET_MESSAGE_GAME_PACKET);
    (void)bs.write(tank);
    client_player->send_packet(bs.get_data(), 0);

    spdlog::info("[VisualItemsManager] Dispatched PACKET_SET_CHARACTER_STATE (netID={}, punch_effect={}, double_jump={}, xspeed={}, yspeed={})",
                 net_id, (int)current_punch_effect_, double_jump_, tank.vec_x2, tank.vec_y2);
}

void VisualItemsManager::send_visual_clothing(player::Player* client_player, uint32_t net_id, core::Core* core) {
    if (!client_player) return;

    if (net_id == 0) {
        net_id = PlayerTracker::get_instance().get_local_netid();
    }
    if (net_id == 0) {
        spdlog::warn("[VisualItemsManager] NetID is 0, cannot send visual clothing yet.");
        return;
    }

    std::unordered_map<int, int> slots_copy;
    std::unordered_set<uint32_t> visual_items_copy;
    bool enabled = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        slots_copy = visual_slots_;
        visual_items_copy = all_visual_items_;
        enabled = is_enabled_;
    }

    auto base = PlayerTracker::get_instance().get_server_clothing();
    int hat = base.hat, shirt = base.shirt, pants = base.pants;
    int shoes = base.shoes, face = base.face, hand = base.hand;
    int back = base.back, hair = base.hair, neck = base.neck, ances = base.ances;
    uint32_t skin_color = base.skin_color != 0 ? base.skin_color : 2190853119;

    if (enabled) {
        for (const auto& [slot, id] : slots_copy) {
            switch (slot) {
                case 0: hat = id; break;
                case 1: shirt = id; break;
                case 2: pants = id; break;
                case 3: shoes = id; break;
                case 4: face = id; break;
                case 5: hand = id; break;
                case 6: back = id; break;
                case 7: hair = id; break;
                case 8: neck = id; break;
                case 9: ances = id; break;
                default: break;
            }
        }
    }

    // 1. Send PACKET_MODIFY_ITEM_INVENTORY for all visual items
    for (uint32_t item_id : visual_items_copy) {
        if (item_id == 0) continue;
        bool is_equipped = (enabled && is_item_equipped(static_cast<int>(item_id)));

        packet::TankUpdatePacket inv_pkt{};
        inv_pkt.type = packet::PACKET_MODIFY_ITEM_INVENTORY;
        inv_pkt.net_id = -1;
        inv_pkt.int_data = item_id;
        inv_pkt.float_var = 1.0f;
        inv_pkt.target_net_id = 0;
        inv_pkt.flags = is_equipped ? 1 : 0; // 1 = equipped checkmark

        ByteStream<std::uint16_t> inv_stream{};
        (void)inv_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        (void)inv_stream.write(inv_pkt);
        client_player->send_packet(inv_stream.get_data(), 0);
    }

    // 1b. Remove checkmarks from real server clothing items overridden by visual items,
    //     or restore checkmarks on real server clothing items if not overridden.
    std::vector<std::pair<int, int>> server_slot_items = {
        {0, base.hat},   {1, base.shirt}, {2, base.pants},
        {3, base.shoes}, {4, base.face},  {5, base.hand},
        {6, base.back},  {7, base.hair},  {8, base.neck},
        {9, base.ances}
    };

    for (const auto& [slot, server_item_id] : server_slot_items) {
        if (server_item_id <= 0) continue;

        bool slot_has_visual = enabled && (slots_copy.find(slot) != slots_copy.end()) && (slots_copy[slot] > 0);
        uint32_t target_flags = slot_has_visual ? 0 : 1;

        packet::TankUpdatePacket inv_pkt{};
        inv_pkt.type = packet::PACKET_MODIFY_ITEM_INVENTORY;
        inv_pkt.net_id = -1;
        inv_pkt.int_data = static_cast<uint32_t>(server_item_id);
        inv_pkt.float_var = 1.0f;
        inv_pkt.target_net_id = 0;
        inv_pkt.flags = target_flags;

        ByteStream<std::uint16_t> inv_stream{};
        (void)inv_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        (void)inv_stream.write(inv_pkt);
        client_player->send_packet(inv_stream.get_data(), 0);
    }

    // 2. Send clean OnSetClothing
    packet::Variant clothing_variant{};
    clothing_variant.add("OnSetClothing");
    clothing_variant.add(glm::vec3(static_cast<float>(hat), static_cast<float>(shirt), static_cast<float>(pants)));
    clothing_variant.add(glm::vec3(static_cast<float>(shoes), static_cast<float>(face), static_cast<float>(hand)));
    clothing_variant.add(glm::vec3(static_cast<float>(back), static_cast<float>(hair), static_cast<float>(neck)));
    clothing_variant.add(skin_color);
    clothing_variant.add(glm::vec3(static_cast<float>(ances), 0.0f, 0.0f));

    std::vector<std::byte> clothing_data = clothing_variant.serialize();
    packet::GameUpdatePacket clothing_packet{};
    clothing_packet.type = packet::PACKET_CALL_FUNCTION;
    clothing_packet.net_id = net_id;
    clothing_packet.flags.extended = 1;
    clothing_packet.data_size = static_cast<uint32_t>(clothing_data.size());

    ByteStream<std::uint16_t> clothing_stream{};
    (void)clothing_stream.write(packet::NET_MESSAGE_GAME_PACKET);
    (void)clothing_stream.write(clothing_packet);
    (void)clothing_stream.write_data(clothing_data.data(), clothing_data.size());
    client_player->send_packet(clothing_stream.get_data(), 0);

    // 3. Play wear clothing sound
    packet::Variant sound_var{};
    sound_var.add("OnPlayPositioned");
    sound_var.add("audio/wear_clothes.wav");
    std::vector<std::byte> sound_data = sound_var.serialize();
    packet::GameUpdatePacket sound_packet{};
    sound_packet.type = packet::PACKET_CALL_FUNCTION;
    sound_packet.net_id = 0xFFFFFFFF;
    sound_packet.flags.extended = 1;
    sound_packet.data_size = static_cast<uint32_t>(sound_data.size());
    ByteStream<std::uint16_t> sound_stream{};
    (void)sound_stream.write(packet::NET_MESSAGE_GAME_PACKET);
    (void)sound_stream.write(sound_packet);
    (void)sound_stream.write_data(sound_data.data(), sound_data.size());
    client_player->send_packet(sound_stream.get_data(), 0);

    // 4. Send native character state (punch effect, double jump, ranges)
    if (enabled && !slots_copy.empty()) {
        send_character_state(client_player, net_id);
    } else {
        restore_character_state(client_player, net_id);
    }
}

void VisualItemsManager::inject_spawn_clothing(std::string& spawn_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_enabled_ || visual_slots_.empty()) return;

    auto replace_or_add_field = [](std::string& data, const std::string& field, const std::string& value) {
        std::string search_pattern = field + "|";
        size_t pos = data.find(search_pattern);
        if (pos != std::string::npos) {
            size_t value_start = pos + search_pattern.length();
            size_t value_end = data.find('\n', value_start);
            if (value_end == std::string::npos) {
                value_end = data.length();
            }
            data.replace(value_start, value_end - value_start, value);
        } else {
            size_t type_pos = data.find("type|local");
            if (type_pos != std::string::npos) {
                data.insert(type_pos, field + "|" + value + "\n");
            }
        }
    };

    for (const auto& [slot, id] : visual_slots_) {
        std::string val_str = std::to_string(id);
        switch (slot) {
            case 0: replace_or_add_field(spawn_data, "cloth_mask", val_str); break;
            case 1: replace_or_add_field(spawn_data, "cloth_shirt", val_str); break;
            case 2: replace_or_add_field(spawn_data, "cloth_pants", val_str); break;
            case 3: replace_or_add_field(spawn_data, "cloth_feet", val_str); break;
            case 4: replace_or_add_field(spawn_data, "cloth_face", val_str); break;
            case 5: replace_or_add_field(spawn_data, "cloth_hand", val_str); break;
            case 6: replace_or_add_field(spawn_data, "cloth_back", val_str); break;
            case 7: replace_or_add_field(spawn_data, "cloth_hair", val_str); break;
            case 8: replace_or_add_field(spawn_data, "cloth_necklace", val_str); break;
            case 9: replace_or_add_field(spawn_data, "cloth_ances", val_str); break;
            default: break;
        }
    }
}

bool VisualItemsManager::patch_server_clothing(packet::Variant& variant, uint32_t net_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_enabled_ || visual_slots_.empty()) return false;

    auto local_netid = PlayerTracker::get_instance().get_local_netid();
    if (net_id != local_netid && local_netid > 0) return false;

    if (variant.size() >= 5) {
        glm::vec3 v0 = variant.get<glm::vec3>(1); // hat, shirt, pants
        glm::vec3 v1 = variant.get<glm::vec3>(2); // shoes, face, hand
        glm::vec3 v2 = variant.get<glm::vec3>(3); // back, hair, neck
        glm::vec3 v4 = (variant.size() >= 6) ? variant.get<glm::vec3>(5) : glm::vec3(0); // ances

        for (const auto& [slot, id] : visual_slots_) {
            float fid = static_cast<float>(id);
            switch (slot) {
                case 0: v0.x = fid; break;
                case 1: v0.y = fid; break;
                case 2: v0.z = fid; break;
                case 3: v1.x = fid; break;
                case 4: v1.y = fid; break;
                case 5: v1.z = fid; break;
                case 6: v2.x = fid; break;
                case 7: v2.y = fid; break;
                case 8: v2.z = fid; break;
                case 9: v4.x = fid; break;
                default: break;
            }
        }

        variant.set(1, v0);
        variant.set(2, v1);
        variant.set(3, v2);
        if (variant.size() >= 6) {
            variant.set(5, v4);
        }
        return true;
    }
    return false;
}

void VisualItemsManager::patch_server_character_state(packet::TankUpdatePacket* tank) {
    if (!tank) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_enabled_ || visual_slots_.empty()) return;

    if (current_punch_effect_ > 0) {
        tank->object_type = current_punch_effect_;
        tank->jump_count = 128;
        tank->animation_type = 128;
    }
    if (double_jump_) {
        tank->int_data |= 2; // double jump bit
    }
}

} // namespace utils
