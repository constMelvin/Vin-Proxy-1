#include "clearclothes_command.hpp"
#include "clothes_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/inventory_manager.hpp"
#include "../../utils/visual_items_manager.hpp"
#include <spdlog/spdlog.h>

namespace command {

extern std::unordered_map<int, int> g_clothing_slots;

ClearClothesCommand::ClearClothesCommand() : CommandBase(
    {"clearclothes", "clearvisual", "resetclothes"},
    {},
    "Clear all saved visual clothing items",
    0
) {}

std::unique_ptr<CommandBase> ClearClothesCommand::clone() const {
    return std::make_unique<ClearClothesCommand>(*this);
}

void ClearClothesCommand::clear_all_items() {
    g_clothing_slots.clear();
    g_all_visual_items.clear();
    utils::VisualItemsManager::get_instance().clear_visual_items();
    spdlog::info("ClearClothesCommand: Cleared all saved clothing items");
}

void ClearClothesCommand::execute_clear(client::Client* client, player::Player* client_player) {
    auto* core = command::ClothesCommand::get_core();
    if (!client && core) {
        client = core->get_client();
    }
    if (!client_player) {
        client_player = (core && core->get_server() && core->get_server()->get_player())
                        ? core->get_server()->get_player() : (client ? client->get_player() : nullptr);
    }

    std::unordered_set<uint32_t> items_to_remove = g_all_visual_items;
    for (const auto& [slot, id] : g_clothing_slots) {
        if (id > 0) items_to_remove.insert(static_cast<uint32_t>(id));
    }
    for (uint32_t id : utils::VisualItemsManager::get_instance().get_all_visual_items()) {
        if (id > 0) items_to_remove.insert(id);
    }

    // 1. Force client to erase all visual items from backpack UI using lost_item_count (jump_count = 255)
    if (client_player) {
        for (uint32_t item_id : items_to_remove) {
            packet::TankUpdatePacket inv_pkt{};
            inv_pkt.type = packet::PACKET_MODIFY_ITEM_INVENTORY;
            inv_pkt.net_id = -1;
            inv_pkt.int_data = item_id;
            inv_pkt.jump_count = 255;      // lost_item_count = 255 -> completely removes item from backpack UI!
            inv_pkt.animation_type = 0;   // gained_item_count = 0
            inv_pkt.float_var = 0.0f;
            inv_pkt.target_net_id = 0;
            inv_pkt.flags = 0;

            ByteStream<std::uint16_t> inv_stream{};
            (void)inv_stream.write(packet::NET_MESSAGE_GAME_PACKET);
            (void)inv_stream.write(inv_pkt);
            client_player->send_packet(inv_stream.get_data(), 0);
        }
    }

    size_t items_cleared = items_to_remove.size();
    
    // 2. Clear all visual clothing slots and tracked visual items
    clear_all_items();
    utils::PlayerTracker::get_instance().reset_to_server_clothing();
    
    // 3. Re-apply base clothing to client avatar
    command::ClothesCommand::send_clothing_change(client);
    
    // 4. Reload clean authentic server inventory without visual items
    if (core) {
        utils::InventoryManager::get_instance().send_inventory(core);
    }

    if (client_player) {
        utils::PacketUtils::send_chat_message(client_player, 
            "`2[VIN] Cleared all visual clothes and removed them from backpack!``");
    }
    
    spdlog::info("ClearClothesCommand: Cleared {} visual items for user", items_cleared);
}

void ClearClothesCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    execute_clear(client, client ? client->get_player() : nullptr);
}

} 
