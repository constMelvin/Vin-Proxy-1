#include "clothes_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/inventory_manager.hpp"
#include "../../utils/weapon_animation_manager.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <unordered_map>

namespace command {

std::unordered_map<int, int> g_clothing_slots;
std::unordered_map<int, int> g_clothing_anim_types;
static core::Core* g_core = nullptr;

ClothesCommand::ClothesCommand() : CommandBase(
    {"clothes", "wear", "visual"},
    {},
    "Apply the selected visual clothing items",
    0
) {}

std::unique_ptr<CommandBase> ClothesCommand::clone() const {
    return std::make_unique<ClothesCommand>(*this);
}

void ClothesCommand::set_core(core::Core* core) {
    g_core = core;
}

void ClothesCommand::set_pending_item(int item_id, int clothing_type, int anim_type) {
    if (g_clothing_slots.find(clothing_type) != g_clothing_slots.end()) {
        int old_item = g_clothing_slots[clothing_type];
        g_clothing_slots[clothing_type] = item_id;
        spdlog::info("ClothesCommand: Replaced item {} with {} in slot {}", old_item, item_id, clothing_type);
    } else {
        g_clothing_slots[clothing_type] = item_id;
        spdlog::info("ClothesCommand: Added item {} to slot {}", item_id, clothing_type);
    }
    
    g_clothing_anim_types[clothing_type] = anim_type;
    spdlog::info("ClothesCommand: Current outfit has {} items", g_clothing_slots.size());
}

bool ClothesCommand::toggle_item(int item_id, int clothing_type, int anim_type) {
    auto it = g_clothing_slots.find(clothing_type);
    if (it != g_clothing_slots.end() && it->second == item_id) {
        g_clothing_slots.erase(it);
        g_clothing_anim_types.erase(clothing_type);
        spdlog::info("ClothesCommand: Unequipped item {} from slot {}", item_id, clothing_type);
        return false; // unequipped
    } else {
        g_clothing_slots[clothing_type] = item_id;
        g_clothing_anim_types[clothing_type] = anim_type;
        spdlog::info("ClothesCommand: Equipped item {} in slot {}", item_id, clothing_type);
        return true; // equipped
    }
}

bool ClothesCommand::is_equipped(int item_id) {
    for (const auto& [slot, id] : g_clothing_slots) {
        if (id == item_id) return true;
    }
    return false;
}

void ClothesCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) {
        spdlog::error("ClothesCommand: No client or player!");
        return;
    }

    if (!g_core) {
        spdlog::error("ClothesCommand: No core set!");
        return;
    }

    spdlog::info("ClothesCommand: Applying visual clothing for {} items", g_clothing_slots.size());
    send_clothing_change(client);
}

void ClothesCommand::send_clothing_change(client::Client* client) {
    auto& player_tracker = utils::PlayerTracker::get_instance();
    auto player_info = player_tracker.get_local_player();
    
    if (player_info.netID == 0) {
        spdlog::warn("ClothesCommand: player netID not found. Try after spawning.");
        return;
    }

    // Start with player's real captured clothing to preserve hair, face, clothes, and skin!
    auto base = player_tracker.get_clothing();
    int hat = base.hat, shirt = base.shirt, pants = base.pants;
    int shoes = base.shoes, face = base.face, hand = base.hand;
    int back = base.back, hair = base.hair, neck = base.neck, ances = base.ances;
    uint32_t skin_color = base.skin_color != 0 ? base.skin_color : 2190853119;
    
    // Apply visual overrides from g_clothing_slots
    for (const auto& [slot_type, item_id] : g_clothing_slots) {
        switch (slot_type) {
            case 0: hat = item_id; break;
            case 1: shirt = item_id; break;
            case 2: pants = item_id; break;
            case 3: shoes = item_id; break;
            case 4: face = item_id; break;
            case 5: hand = item_id; break;
            case 6: back = item_id; break;
            case 7: hair = item_id; break;
            case 8: neck = item_id; break;
            case 9: ances = item_id; break;
            default:
                spdlog::warn("ClothesCommand: Unknown clothing_type: {}", slot_type);
                break;
        }
    }
    
    spdlog::info("ClothesCommand: Outfit - Hat:{} Shirt:{} Pants:{} Shoes:{} Face:{} Hand:{} Back:{} Hair:{} Neck:{} Ances:{}", 
                 hat, shirt, pants, shoes, face, hand, back, hair, neck, ances);
    
    auto* client_player = (g_core->get_server() && g_core->get_server()->get_player()) 
                          ? g_core->get_server()->get_player() : nullptr;
    if (!client_player) return;

    // ===== STEP 1: Add the visual weapon to the client's inventory FIRST =====
    if (hand > 0) {
        // 1a. Send PACKET_MODIFY_ITEM_INVENTORY to add the item
        packet::TankUpdatePacket mod_inv{};
        mod_inv.type = static_cast<uint8_t>(packet::PACKET_MODIFY_ITEM_INVENTORY); // 13
        mod_inv.net_id = -1; // 0xFFFFFFFF for local inventory
        mod_inv.target_net_id = static_cast<int32_t>(hand); // item ID
        mod_inv.int_data = 1; // count = 1
        mod_inv.flags = 1; // equipped flag

        ByteStream<std::uint16_t> inv_bs{};
        inv_bs.write(packet::NET_MESSAGE_GAME_PACKET);
        inv_bs.write(mod_inv);
        client_player->send_packet(inv_bs.get_data(), 0);
        spdlog::info("ClothesCommand: Sent PACKET_MODIFY_ITEM_INVENTORY for item {}", hand);

        // 1b. Synchronize full inventory state with visual weapon marked equipped
        utils::InventoryManager::get_instance().send_inventory(g_core, 0xFFFFFFFF, static_cast<uint16_t>(hand));
    }

    // ===== STEP 2: Send OnSetClothing =====
    packet::Variant clothing_variant{};
    clothing_variant.add("OnSetClothing");
    
    // Vector 0: (hat, shirt, pants)
    clothing_variant.add(glm::vec3((float)hat, (float)shirt, (float)pants));
    
    // Vector 1: (shoes, face, hand)
    clothing_variant.add(glm::vec3((float)shoes, (float)face, (float)hand));
    
    // Vector 2: (back, hair, neck)
    clothing_variant.add(glm::vec3((float)back, (float)hair, (float)neck));
    
    // Skin Color: preserved authentic skin
    clothing_variant.add(skin_color);
    
    // Vector 4: (ances, 0, 0)
    clothing_variant.add(glm::vec3((float)ances, 0.0f, 0.0f));
    
    std::vector<std::byte> clothing_data = clothing_variant.serialize();
    
    packet::GameUpdatePacket clothing_packet{};
    clothing_packet.type = packet::PACKET_CALL_FUNCTION;
    clothing_packet.net_id = player_info.netID;
    clothing_packet.flags.extended = 1;
    clothing_packet.data_size = static_cast<uint32_t>(clothing_data.size());
    
    ByteStream<std::uint16_t> clothing_stream{};
    clothing_stream.write(packet::NET_MESSAGE_GAME_PACKET);
    clothing_stream.write(clothing_packet);
    clothing_stream.write_data(clothing_data.data(), clothing_data.size());
    
    client_player->send_packet(clothing_stream.get_data(), 0);
    spdlog::info("ClothesCommand: Sent OnSetClothing to CLIENT with netid: {}", player_info.netID);

    // ===== STEP 4: Ensure player is unfrozen =====
    packet::Variant unfreeze_variant{};
    unfreeze_variant.add("OnSetFreezeState");
    unfreeze_variant.add(0);

    std::vector<std::byte> unfreeze_data = unfreeze_variant.serialize();
    packet::GameUpdatePacket unfreeze_packet{};
    unfreeze_packet.type = packet::PACKET_CALL_FUNCTION;
    unfreeze_packet.net_id = player_info.netID;
    unfreeze_packet.flags.extended = 1;
    unfreeze_packet.data_size = static_cast<uint32_t>(unfreeze_data.size());

    ByteStream<std::uint16_t> unfreeze_stream{};
    unfreeze_stream.write(packet::NET_MESSAGE_GAME_PACKET);
    unfreeze_stream.write(unfreeze_packet);
    unfreeze_stream.write_data(unfreeze_data.data(), unfreeze_data.size());

    client_player->send_packet(unfreeze_stream.get_data(), 0);

    // ===== STEP 5: Send OnFlagMay2019(0) matching official equip sequence =====
    {
        packet::Variant flag_variant{};
        flag_variant.add("OnFlagMay2019");
        flag_variant.add(static_cast<int32_t>(0));

        std::vector<std::byte> flag_data = flag_variant.serialize();
        packet::GameUpdatePacket flag_packet{};
        flag_packet.type = packet::PACKET_CALL_FUNCTION;
        flag_packet.net_id = player_info.netID;
        flag_packet.flags.extended = 1;
        flag_packet.data_size = static_cast<uint32_t>(flag_data.size());

        ByteStream<std::uint16_t> flag_stream{};
        flag_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        flag_stream.write(flag_packet);
        flag_stream.write_data(flag_data.data(), flag_data.size());
        client_player->send_packet(flag_stream.get_data(), 0);
    }

    // ===== STEP 6: Send PACKET_SET_CHARACTER_STATE (Type 20) =====
    {
        const auto* saved_state = utils::WeaponAnimationManager::get_instance().get_server_character_state();
        packet::TankUpdatePacket char_pkt{};
        if (saved_state) {
            char_pkt = *saved_state;
        } else {
            char_pkt.type = static_cast<uint8_t>(packet::PACKET_SET_CHARACTER_STATE);
        }
        char_pkt.net_id = static_cast<int32_t>(player_info.netID);

        ByteStream<std::uint16_t> char_stream{};
        char_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        char_stream.write(char_pkt);
        client_player->send_packet(char_stream.get_data(), 0);
        spdlog::info("ClothesCommand: Sent PACKET_SET_CHARACTER_STATE (Type 20) to netid {}", player_info.netID);
    }

    // ===== STEP 7: Force select the visual weapon as the active hotbar item =====
    // This is the KEY fix: Growtopia hardcodes Fist (Item 18) animation regardless of
    // punchParameters in items.dat. By selecting the visual weapon as the active hotbar item,
    // the client reads the WEAPON's authentic punchParameters (e.g. UP_SPINARM2 for swords).
    if (hand > 0) {
        packet::TankUpdatePacket arrow_pkt{};
        arrow_pkt.type = static_cast<uint8_t>(packet::PACKET_ACTIVE_ARROW_TO_ITEM);
        arrow_pkt.net_id = -1;
        arrow_pkt.int_data = static_cast<uint32_t>(hand);

        ByteStream<std::uint16_t> arrow_bs{};
        arrow_bs.write(packet::NET_MESSAGE_GAME_PACKET);
        arrow_bs.write(arrow_pkt);
        client_player->send_packet(arrow_bs.get_data(), 0);
        spdlog::info("ClothesCommand: Sent PACKET_ACTIVE_ARROW_TO_ITEM for hand item {}", hand);
    }
    
    spdlog::info("ClothesCommand: Clothing change completed for {} items", g_clothing_slots.size());
}

}

