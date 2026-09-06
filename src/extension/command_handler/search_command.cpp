#include "search_command.hpp"
#include "../item_finder/item_finder.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../core/core.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/text_parse.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../proxy_imgui_gui.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace command {

static core::Core* g_core = nullptr;

struct PlayerSearchState {
    std::string query;
    int page = 0;
};

static std::unordered_map<uint32_t, PlayerSearchState> g_player_search_state;

static std::string get_item_type_name(int type) {
    switch (type) {
        case 0: return "Block";
        case 1: return "Door";
        case 2: return "Lock";
        case 3: return "Gems";
        case 4: return "Sign";
        case 5: return "SFX Foreground";
        case 6: return "Toggleable Foreground";
        case 7: return "Main Door";
        case 8: return "Platform";
        case 9: return "Bedrock";
        case 10: return "Lava";
        case 11: return "Foreground";
        case 12: return "Background";
        case 13: return "Seed";
        case 14: return "Clothing";
        case 15: return "Animated Block";
        case 16: return "SFX Background";
        case 17: return "Toggleable Background";
        case 18: return "Bouncy";
        case 19: return "Checkpoint";
        case 20: return "Gateway";
        case 21: return "Treasure";
        case 22: return "Deadly Block";
        case 23: return "Trampoline";
        case 24: return "Consumable";
        default: return "Unknown";
    }
}

SearchCommand::SearchCommand() : CommandBase(
    {"search"},
    {"[item_name or item_id]"},
    "Search all items in the database without limit",
    0
) {}

std::unique_ptr<CommandBase> SearchCommand::clone() const {
    return std::make_unique<SearchCommand>();
}

void SearchCommand::set_core(core::Core* core) {
    g_core = core;
}

core::Core* SearchCommand::get_core() {
    return g_core;
}

void SearchCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    spdlog::info("=== SEARCH COMMAND EXECUTED ===");

    if (!g_core) {
        spdlog::error("SearchCommand: No core initialized!");
        return;
    }

    auto* server = g_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("SearchCommand: No server player available!");
        return;
    }

    player::Player* player = server->get_player();

    // Parse query and optional page number from args
    std::string query;
    int page = 0;

    if (args.size() > 1) {
        // Check if last argument is a numeric page number
        const std::string& last_arg = args.back();
        bool is_num = !last_arg.empty() && std::all_of(last_arg.begin(), last_arg.end(), [](unsigned char c) {
            return std::isdigit(c);
        });

        size_t end_arg_idx = args.size();
        if (is_num && args.size() > 2) {
            try {
                int requested_page = std::stoi(last_arg);
                if (requested_page >= 1) {
                    page = requested_page - 1;
                    end_arg_idx = args.size() - 1;
                }
            } catch (...) {
                page = 0;
            }
        }

        for (size_t i = 1; i < end_arg_idx; ++i) {
            if (i > 1) query += " ";
            query += args[i];
        }
    }

    show_search_dialog(player, query, page);
}

void SearchCommand::show_search_dialog(player::Player* player, const std::string& query, int page) {
    if (!player) return;

    auto* db = GetItemDatabase();
    if (!db) {
        utils::PacketUtils::send_chat_message(player, "`4[VIN] Item database not loaded!");
        return;
    }

    uint32_t host = 0;
    if (player->get_peer()) {
        host = player->get_peer()->address.host;
    }

    // Search all items without limit
    std::vector<extension::item_finder::ItemInfo> all_results;
    if (!query.empty()) {
        all_results = db->search_items(query, 100000, "all");
    }

    const int ITEMS_PER_PAGE = 20;
    int total_items = static_cast<int>(all_results.size());
    int total_pages = total_items == 0 ? 1 : (total_items + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;

    if (page < 0) page = 0;
    if (page >= total_pages) page = total_pages - 1;

    g_player_search_state[host] = { query, page };

    int start_idx = page * ITEMS_PER_PAGE;
    int end_idx = std::min(start_idx + ITEMS_PER_PAGE, total_items);

    std::ostringstream dialog;
    dialog << "set_default_color|`o\n";
    dialog << "add_label_with_icon|big|`wItem Search (All Items)``|left|6016|\n";
    dialog << "add_spacer|small|\n";
    dialog << "add_text_input|search_query|Search:|" << query << "|30|\n";
    dialog << "add_spacer|small|\n";

    if (query.empty()) {
        dialog << "add_textbox|`oEnter an item name or ID to search all items.``|left|\n";
        dialog << "add_textbox|`oExample: wing, dragon, sword, cape, 362``|left|\n";
        dialog << "add_spacer|small|\n";
    } else if (all_results.empty()) {
        dialog << "add_textbox|`4No items found matching: `w" << query << "``|left|\n";
        dialog << "add_spacer|small|\n";
    } else {
        if (total_pages > 1) {
            dialog << fmt::format("add_textbox|`2Found `w{} `2items matching '`w{}`2' `o(Page {} of {})``|left|\n",
                total_items, query, page + 1, total_pages);
        } else {
            dialog << fmt::format("add_textbox|`2Found `w{} `2items matching '`w{}`2' (All items shown)``|left|\n",
                total_items, query);
        }
        dialog << "add_spacer|small|\n";

        // Navigation controls at top if multiple pages
        if (total_pages > 1) {
            if (page > 0) {
                dialog << "add_button|search_prev|`5<<< Previous Page``|noflags|0|0|\n";
            }
            if (page + 1 < total_pages) {
                dialog << "add_button|search_next|`5Next Page >>>``|noflags|0|0|\n";
            }
            dialog << "add_spacer|small|\n";
        }

        // List items on current page matching reference design
        for (int i = start_idx; i < end_idx; ++i) {
            const auto& item = all_results[i];
            dialog << fmt::format("add_label_with_icon|big|`w{}``|left|{}|\n", item.name, item.id);
            dialog << fmt::format("add_smalltext|`oType: {}``|\n", get_item_type_name(item.type));
            dialog << fmt::format("add_smalltext|`5ID: {}``|\n", item.id);
            dialog << fmt::format("add_button|viewdetail_{}|`2View Full Details``|\n", item.id);
            dialog << "add_spacer|small|\n";
        }

        // Navigation controls at bottom if multiple pages
        if (total_pages > 1) {
            if (page > 0) {
                dialog << "add_button|search_prev|`5<<< Previous Page``|noflags|0|0|\n";
            }
            if (page + 1 < total_pages) {
                dialog << "add_button|search_next|`5Next Page >>>``|noflags|0|0|\n";
            }
            dialog << "add_spacer|small|\n";
        }
    }

    dialog << "add_quick_exit|\n";
    dialog << "end_dialog|search_dialog|Close|Search|\n";

    try {
        packet::Variant var{};
        var.add("OnDialogRequest");
        var.add(dialog.str());

        std::vector<std::byte> ext = var.serialize();

        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = -1;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext.size());

        ByteStream<std::uint16_t> stream{};
        stream.write(packet::NET_MESSAGE_GAME_PACKET);
        stream.write(pkt);
        stream.write_data(ext.data(), ext.size());

        player->send_packet(stream.get_data(), 0);
        spdlog::info("SearchCommand: Sent search dialog for '{}' (page {}/{}, total={})", query, page + 1, total_pages, total_items);
    } catch (const std::exception& e) {
        spdlog::error("SearchCommand: Failed to send search dialog: {}", e.what());
    }
}

void SearchCommand::show_item_details_dialog(player::Player* player, const extension::item_finder::ItemInfo* item, const std::string& return_query, int return_page) {
    if (!player || !item) return;

    std::ostringstream dialog;
    dialog << "set_default_color|`o\n";
    dialog << "add_label_with_icon|big|`w" << item->name << "``|left|" << item->id << "|\n";
    dialog << "add_spacer|small|\n";

    dialog << "add_smalltext|`5Item ID: `w" << item->id << "``|\n";
    dialog << "add_smalltext|`5Rarity: `w" << static_cast<int>(item->rarity) << "``|\n";
    dialog << "add_smalltext|`5Type: `w" << get_item_type_name(item->type) << "``|\n";

    if (!item->info.empty() && item->info != "No info.") {
        dialog << "add_spacer|small|\n";
        dialog << "add_textbox|`5Info:``|left|\n";
        dialog << "add_smalltext|`o" << item->info << "``|\n";
    }

    if (item->clothing_type > 0) {
        std::string clothing_name;
        switch (item->clothing_type) {
            case 0: clothing_name = "Hat / Mask"; break;
            case 1: clothing_name = "Shirt"; break;
            case 2: clothing_name = "Pants"; break;
            case 3: clothing_name = "Feet"; break;
            case 4: clothing_name = "Face"; break;
            case 5: clothing_name = "Hand"; break;
            case 6: clothing_name = "Back"; break;
            case 7: clothing_name = "Hair"; break;
            case 8: clothing_name = "Necklace"; break;
            case 9: clothing_name = "Ancestral"; break;
            default: clothing_name = "Clothing"; break;
        }
        dialog << "add_smalltext|`5Clothing Type: `w" << clothing_name << "``|\n";
    }

    dialog << "add_spacer|small|\n";
    dialog << "add_button|back_to_search|`2Back to Search``|\n";
    dialog << "add_quick_exit|\n";
    dialog << "end_dialog|search_detail|Close||\n";

    try {
        packet::Variant var{};
        var.add("OnDialogRequest");
        var.add(dialog.str());

        std::vector<std::byte> ext = var.serialize();

        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = -1;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext.size());

        ByteStream<std::uint16_t> stream{};
        stream.write(packet::NET_MESSAGE_GAME_PACKET);
        stream.write(pkt);
        stream.write_data(ext.data(), ext.size());

        player->send_packet(stream.get_data(), 0);
        spdlog::info("SearchCommand: Sent details dialog for item: {}", item->name);
    } catch (const std::exception& e) {
        spdlog::error("SearchCommand: Failed to send item details dialog: {}", e.what());
    }
}

void SearchCommand::handle_dialog_return(player::Player* player, const std::string& button_clicked, const TextParse& tp) {
    if (!player) return;

    uint32_t host = 0;
    if (player->get_peer()) {
        host = player->get_peer()->address.host;
    }

    PlayerSearchState state = g_player_search_state[host];
    std::string new_query = tp.get("search_query");

    spdlog::info("SearchCommand: Dialog return button='{}', query='{}'", button_clicked, new_query);

    if (button_clicked == "Close" || button_clicked == "search_dialog_close") {
        spdlog::info("SearchCommand: Dialog closed by player");
        return;
    }

    if (button_clicked == "back_to_search") {
        show_search_dialog(player, state.query, state.page);
        return;
    }

    if (button_clicked == "search_prev") {
        state.page = std::max(0, state.page - 1);
        show_search_dialog(player, state.query, state.page);
        return;
    }

    if (button_clicked == "search_next") {
        state.page = state.page + 1;
        show_search_dialog(player, state.query, state.page);
        return;
    }

    if (button_clicked.rfind("viewdetail_", 0) == 0) {
        try {
            int item_id = std::stoi(button_clicked.substr(11));
            auto* db = GetItemDatabase();
            const extension::item_finder::ItemInfo* item = db ? db->get_item_by_id(item_id) : nullptr;
            if (item) {
                show_item_details_dialog(player, item, state.query, state.page);
            }
        } catch (const std::exception& e) {
            spdlog::error("SearchCommand: Error opening viewdetail: {}", e.what());
        }
        return;
    }

    // Player submitted search query via Search button or Enter key
    if (button_clicked == "Search" || button_clicked == "search" || button_clicked == "search_dialog" || button_clicked.empty()) {
        show_search_dialog(player, new_query, 0);
        return;
    }

    // Default fallback
    show_search_dialog(player, new_query.empty() ? state.query : new_query, 0);
}

} // namespace command
