#include "clothes_command.hpp"
#include "clearclothes_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/inventory_manager.hpp"
#include "../../utils/weapon_animation_manager.hpp"
#include "../../utils/visual_items_manager.hpp"
#include "../../proxy_imgui_gui.hpp"
#include "../../extension/item_finder/item_finder.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <algorithm>
#include <cctype>

namespace command {

std::unordered_map<int, int> g_clothing_slots;
std::unordered_set<uint32_t> g_all_visual_items;
static core::Core* g_core = nullptr;
static bool s_show_other_visual = true;

static const std::string kVisualSetsFile = "visual_sets.json";

static std::string sanitize_set_name(std::string name) {
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t' || name.front() == '\r' || name.front() == '\n')) {
        name.erase(name.begin());
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r' || name.back() == '\n')) {
        name.pop_back();
    }
    std::string clean;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ' ') {
            clean += c;
        }
    }
    if (clean.length() > 20) {
        clean = clean.substr(0, 20);
    }
    return clean;
}

ClothesCommand::ClothesCommand() : CommandBase(
    {"clothes", "wear", "visual"},
    {},
    "Visual clothes manager, slot sets, and clothing overrides",
    0
) {}

std::unique_ptr<CommandBase> ClothesCommand::clone() const {
    return std::make_unique<ClothesCommand>(*this);
}

void ClothesCommand::set_core(core::Core* core) {
    g_core = core;
}

core::Core* ClothesCommand::get_core() {
    return g_core;
}

void ClothesCommand::set_pending_item(int item_id, int clothing_type, int /*anim_type*/) {
    if (item_id > 0) {
        g_all_visual_items.insert(static_cast<uint32_t>(item_id));
        g_clothing_slots[clothing_type] = item_id;
        utils::VisualItemsManager::get_instance().set_visual_item(clothing_type, item_id);
    } else {
        g_clothing_slots.erase(clothing_type);
        utils::VisualItemsManager::get_instance().set_visual_item(clothing_type, 0);
    }
}

bool ClothesCommand::toggle_item(int item_id, int clothing_type, int /*anim_type*/) {
    if (item_id > 0) {
        g_all_visual_items.insert(static_cast<uint32_t>(item_id));
    }
    bool equipped = utils::VisualItemsManager::get_instance().toggle_visual_item(clothing_type, item_id);
    if (equipped) {
        g_clothing_slots[clothing_type] = item_id;
        spdlog::info("ClothesCommand: Added item {} to slot {}", item_id, clothing_type);
    } else {
        g_clothing_slots.erase(clothing_type);
        spdlog::info("ClothesCommand: Unequipped item {} from slot {}", item_id, clothing_type);
    }
    return equipped;
}

bool ClothesCommand::is_equipped(int item_id) {
    return utils::VisualItemsManager::get_instance().is_item_equipped(item_id);
}

std::map<std::string, std::unordered_map<int, int>> ClothesCommand::get_saved_sets() {
    std::map<std::string, std::unordered_map<int, int>> result;
    if (!std::filesystem::exists(kVisualSetsFile)) {
        return result;
    }
    try {
        std::ifstream file(kVisualSetsFile);
        if (!file.is_open()) return result;
        nlohmann::json j;
        file >> j;
        if (j.contains("sets") && j["sets"].is_object()) {
            for (auto& [set_name, slots_obj] : j["sets"].items()) {
                if (!slots_obj.is_object()) continue;
                std::unordered_map<int, int> slots;
                for (auto& [slot_key, item_val] : slots_obj.items()) {
                    try {
                        int slot = std::stoi(slot_key);
                        int item_id = item_val.get<int>();
                        if (item_id > 0) {
                            slots[slot] = item_id;
                        }
                    } catch (...) {}
                }
                if (!slots.empty()) {
                    result[set_name] = slots;
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("ClothesCommand::get_saved_sets error: {}", e.what());
    }
    return result;
}

bool ClothesCommand::save_visual_set(const std::string& raw_name) {
    std::string name = sanitize_set_name(raw_name);
    if (name.empty()) {
        return false;
    }

    // Only save visual slots, strictly ignoring real server items
    const auto& visual_slots = utils::VisualItemsManager::get_instance().get_visual_slots();
    std::unordered_map<int, int> items_to_save;
    for (const auto& [slot, id] : visual_slots) {
        if (id > 0) items_to_save[slot] = id;
    }
    for (const auto& [slot, id] : g_clothing_slots) {
        if (id > 0 && items_to_save.find(slot) == items_to_save.end()) {
            items_to_save[slot] = id;
        }
    }

    if (items_to_save.empty()) {
        return false;
    }

    nlohmann::json j;
    if (std::filesystem::exists(kVisualSetsFile)) {
        try {
            std::ifstream file(kVisualSetsFile);
            if (file.is_open()) {
                file >> j;
            }
        } catch (...) {}
    }

    if (!j.contains("sets") || !j["sets"].is_object()) {
        j["sets"] = nlohmann::json::object();
    }

    nlohmann::json set_obj = nlohmann::json::object();
    for (const auto& [slot, item_id] : items_to_save) {
        set_obj[std::to_string(slot)] = item_id;
    }
    j["sets"][name] = set_obj;

    try {
        std::ofstream file(kVisualSetsFile);
        if (!file.is_open()) return false;
        file << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("ClothesCommand::save_visual_set error: {}", e.what());
        return false;
    }
}

bool ClothesCommand::load_visual_set(const std::string& name, client::Client* client, player::Player* player) {
    auto saved_sets = get_saved_sets();
    auto it = saved_sets.find(name);
    if (it == saved_sets.end()) {
        return false;
    }

    if (!client && g_core) {
        client = g_core->get_client();
    }

    // Clear old visual clothes
    g_clothing_slots.clear();
    utils::VisualItemsManager::get_instance().clear_visual_items();

    // Apply the set
    for (const auto& [slot, item_id] : it->second) {
        g_clothing_slots[slot] = item_id;
        g_all_visual_items.insert(static_cast<uint32_t>(item_id));
        utils::VisualItemsManager::get_instance().set_visual_item(slot, item_id);
    }

    // Enable visuals and send clothing update
    utils::VisualItemsManager::get_instance().set_enabled(true);
    send_clothing_change(client);

    return true;
}

bool ClothesCommand::delete_visual_set(const std::string& name) {
    if (!std::filesystem::exists(kVisualSetsFile)) return false;
    try {
        nlohmann::json j;
        std::ifstream file(kVisualSetsFile);
        if (!file.is_open()) return false;
        file >> j;
        file.close();

        if (j.contains("sets") && j["sets"].is_object() && j["sets"].contains(name)) {
            j["sets"].erase(name);
            std::ofstream out(kVisualSetsFile);
            if (!out.is_open()) return false;
            out << j.dump(2);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("ClothesCommand::delete_visual_set error: {}", e.what());
    }
    return false;
}

void ClothesCommand::send_clothes_dialog(player::Player* player) {
    if (!g_core) {
        spdlog::error("ClothesCommand::send_clothes_dialog: No core set!");
        return;
    }

    auto* server = g_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("ClothesCommand::send_clothes_dialog: Local player not connected!");
        return;
    }

    // ALWAYS send to the local player's game window
    player = server->get_player();

    bool is_enabled = utils::VisualItemsManager::get_instance().is_enabled();

    std::ostringstream dialog;
    dialog << "set_default_color|`o\n";
    // Title Header with Angel Wings icon (ID 362)
    dialog << "add_label_with_icon|big|`9Visual Clothes``|left|1784|\n";
    dialog << "add_spacer|small|\n";

    // Toggle button 1 line text
    if (is_enabled) {
        dialog << "add_button|toggle_visual|`4Disable `pVisual Clothes|\n";
    } else {
        dialog << "add_button|toggle_visual|`2Enable `pVisual Clothes|\n";
    }

    // Show Other Proxy Users Checkbox
    dialog << fmt::format("add_checkbox|show_other_visual|`2Show Other Proxy Users Visual Clothes|{}|\n", s_show_other_visual ? 1 : 0);

    // Load Section
    dialog << "add_textbox|`1These Buttons Below will `$Load `1Previuosly Saved Clothes & `2Enable `1Visual Clothes Equip.|left|\n";
    dialog << "add_spacer|small|\n";
    dialog << "add_button|load_slot_1|`eLoad `@Slot 1 Set|\n";
    dialog << "add_button|load_slot_2|`eLoad `@Slot 2 Set|\n";
    dialog << "add_button|load_slot_3|`eLoad `@Slot 3 Set|\n";
    dialog << "add_button|load_slot_4|`eLoad `@Slot 4 Set|\n";
    dialog << "add_spacer|small|\n";

    // Save Section
    dialog << "add_textbox|`1These Buttons Below will `2Save ``Current Visual Set To Selected `@Slot.|left|\n";
    dialog << "add_spacer|small|\n";
    dialog << "add_button|save_slot_1|`2Save `9Current Equiped Visual Set To `@Slot 1|\n";
    dialog << "add_button|save_slot_2|`2Save `9Current Equiped Visual Set To `@Slot 2|\n";
    dialog << "add_button|save_slot_3|`2Save `9Current Equiped Visual Set To `@Slot 3|\n";
    dialog << "add_button|save_slot_4|`2Save `9Current Equiped Visual Set To `@Slot 4|\n";
    dialog << "add_spacer|small|\n";

    // Clear button & Close
    dialog << "add_button|clear_all_visual|`4Clear `$Visual Clothes|\n";
    dialog << "add_quick_exit|\n";
    dialog << "end_dialog|clothes_dialog|Close||\n";

    spdlog::info("ClothesCommand: Sending visual clothes dialog to local player (size={})", dialog.str().size());

    packet::Variant var{};
    var.add("OnDialogRequest");
    var.add(dialog.str());
    std::vector<std::byte> ext = var.serialize();

    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = -1;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext.data(), ext.size());
    player->send_packet(bs.get_data(), 0);
    spdlog::info("ClothesCommand: Dialog packet sent successfully to local GT client");
}

void ClothesCommand::handle_dialog_response(player::Player* player, const std::string& button_clicked, const TextParse& tp) {
    if (!g_core) return;
    auto* server = g_core->get_server();
    if (server && server->get_player()) {
        player = server->get_player();
    }
    if (!player) return;

    client::Client* client = g_core->get_client();

    spdlog::info("ClothesCommand: Dialog response button: '{}'", button_clicked);

    // Update checkbox state if sent
    std::string show_others = tp.get("show_other_visual");
    if (!show_others.empty()) {
        s_show_other_visual = (show_others == "1");
    }

    if (button_clicked == "toggle_visual") {
        bool now_enabled = utils::VisualItemsManager::get_instance().toggle_enabled();
        utils::PacketUtils::send_chat_message(player, 
            now_enabled ? "`2[VIN] Visual items & animations enabled!``" : "`4[VIN] Visual items & animations disabled!``");
        send_clothing_change(client);
        send_clothes_dialog(player);
        return;
    }

    // Handle Load Slot 1..4
    for (int i = 1; i <= 4; ++i) {
        if (button_clicked == fmt::format("load_slot_{}", i)) {
            std::string slot_name = fmt::format("Slot {}", i);
            bool ok = load_visual_set(slot_name, client, player);
            if (ok) {
                auto sets = get_saved_sets();
                size_t count = sets[slot_name].size();
                utils::PacketUtils::send_chat_message(player, 
                    fmt::format("`2[VIN] Loaded `eSlot {} Set `2({} items)! Visual clothes enabled!``", i, count));
            } else {
                utils::PacketUtils::send_chat_message(player, 
                    fmt::format("`4[VIN] Slot {} is empty! Equip visual clothes and save to Slot {} first.``", i, i));
            }
            send_clothes_dialog(player);
            return;
        }
    }

    // Handle Save Slot 1..4
    for (int i = 1; i <= 4; ++i) {
        if (button_clicked == fmt::format("save_slot_{}", i)) {
            std::string slot_name = fmt::format("Slot {}", i);
            bool ok = save_visual_set(slot_name);
            if (ok) {
                auto sets = get_saved_sets();
                size_t count = sets[slot_name].size();
                utils::PacketUtils::send_chat_message(player, 
                    fmt::format("`2[VIN] Successfully saved current visual set to `pSlot {} `2({} items)!``", i, count));
            } else {
                utils::PacketUtils::send_chat_message(player, 
                    fmt::format("`4[VIN] No visual items equipped! Equip visual items first to save to Slot {}.``", i));
            }
            send_clothes_dialog(player);
            return;
        }
    }

    if (button_clicked == "clear_all_visual") {
        ClearClothesCommand::execute_clear(client, player);
        send_clothes_dialog(player);
        return;
    }
}

void ClothesCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!g_core) {
        spdlog::error("ClothesCommand: No core set!");
        return;
    }

    auto* server = g_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("ClothesCommand: No local server player!");
        return;
    }

    player::Player* local_player = server->get_player();

    std::string cmd_name = (!args.empty()) ? args[0] : "";
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::tolower);

    // Bare /clothes or /visual with no arguments opens the dialog box
    if (args.size() <= 1) {
        send_clothes_dialog(local_player);
        return;
    }

    if (args.size() >= 2) {
        std::string first_arg = args[1];
        std::transform(first_arg.begin(), first_arg.end(), first_arg.begin(), ::tolower);

        if (first_arg == "menu" || first_arg == "gui" || first_arg == "dialog") {
            send_clothes_dialog(local_player);
            return;
        }

        if (first_arg == "toggle") {
            bool now_enabled = utils::VisualItemsManager::get_instance().toggle_enabled();
            if (now_enabled) {
                utils::PacketUtils::send_chat_message(local_player, "`2[VIN] Visual items & animations enabled!``");
            } else {
                utils::PacketUtils::send_chat_message(local_player, "`4[VIN] Visual items & animations disabled!``");
            }
            send_clothing_change(client);
            return;
        }

        if (first_arg == "on" || first_arg == "enable") {
            utils::VisualItemsManager::get_instance().set_enabled(true);
            utils::PacketUtils::send_chat_message(local_player, "`2[VIN] Visual items & animations enabled!``");
            send_clothing_change(client);
            return;
        }

        if (first_arg == "off" || first_arg == "disable") {
            utils::VisualItemsManager::get_instance().set_enabled(false);
            utils::PacketUtils::send_chat_message(local_player, "`4[VIN] Visual items & animations disabled!``");
            send_clothing_change(client);
            return;
        }

        if (first_arg == "clear" || first_arg == "reset") {
            ClearClothesCommand::execute_clear(client, local_player);
            return;
        }

        // Quick slot loading: /clothes 1..4 or /clothes load 1..4
        int slot_to_load = -1;
        if (first_arg.size() == 1 && std::isdigit(first_arg[0])) {
            slot_to_load = std::stoi(first_arg);
        } else if (first_arg == "load" && args.size() >= 3 && args[2].size() == 1 && std::isdigit(args[2][0])) {
            slot_to_load = std::stoi(args[2]);
        }
        if (slot_to_load >= 1 && slot_to_load <= 4) {
            std::string slot_name = fmt::format("Slot {}", slot_to_load);
            bool ok = load_visual_set(slot_name, client, local_player);
            if (ok) {
                auto sets = get_saved_sets();
                size_t count = sets[slot_name].size();
                utils::PacketUtils::send_chat_message(local_player, 
                    fmt::format("`2[VIN] Loaded `eSlot {} Set `2({} items)!``", slot_to_load, count));
            } else {
                utils::PacketUtils::send_chat_message(local_player, 
                    fmt::format("`4[VIN] Slot {} is empty!``", slot_to_load));
            }
            return;
        }

        // Quick slot saving: /clothes save 1..4
        if (first_arg == "save" && args.size() >= 3) {
            std::string target = args[2];
            std::string slot_name = target;
            if (target.size() == 1 && std::isdigit(target[0])) {
                slot_name = fmt::format("Slot {}", target);
            }
            bool ok = save_visual_set(slot_name);
            if (ok) {
                auto sets = get_saved_sets();
                size_t count = sets[slot_name].size();
                utils::PacketUtils::send_chat_message(local_player, 
                    fmt::format("`2[VIN] Saved current visual set to `p{} `2({} items)!``", slot_name, count));
            } else {
                utils::PacketUtils::send_chat_message(local_player, 
                    "`4[VIN] Failed to save visual set. Equip visual items first!``");
            }
            return;
        }

        if ((first_arg == "delete" || first_arg == "del") && args.size() >= 3) {
            std::string target = args[2];
            std::string slot_name = target;
            if (target.size() == 1 && std::isdigit(target[0])) {
                slot_name = fmt::format("Slot {}", target);
            }
            bool ok = delete_visual_set(slot_name);
            if (ok) {
                utils::PacketUtils::send_chat_message(local_player, 
                    fmt::format("`4[VIN] Deleted visual set `w'{}'``!", slot_name));
            } else {
                utils::PacketUtils::send_chat_message(local_player, 
                    fmt::format("`4[VIN] Visual set `w'{}' `4not found!``", slot_name));
            }
            return;
        }

        if (first_arg == "list") {
            auto sets = get_saved_sets();
            if (sets.empty()) {
                utils::PacketUtils::send_chat_message(local_player, "`4[VIN] No saved visual sets found!``");
            } else {
                utils::PacketUtils::send_chat_message(local_player, 
                    fmt::format("`2[VIN] Saved visual sets ({}):``", sets.size()));
                for (const auto& [name, items] : sets) {
                    utils::PacketUtils::send_chat_message(local_player, 
                        fmt::format("`o - `w{} `o({} visual items)``", name, items.size()));
                }
            }
            return;
        }

        if (args.size() == 2) {
            // Format: /wear <item_id> or /clothes <item_id>
            try {
                int item_id = std::stoi(first_arg);
                if (item_id > 0) {
                    int slot = 5; // Default to hand slot
                    auto* db = GetItemDatabase();
                    if (db) {
                        const auto* itm = db->get_item_by_id(item_id);
                        if (itm) {
                            slot = itm->clothing_type;
                            std::string lower_name = itm->name;
                            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
                            if (lower_name.find("ancestral") != std::string::npos ||
                                lower_name.find("samille") != std::string::npos ||
                                lower_name.find("chakram") != std::string::npos) {
                                slot = 9;
                            }
                        }
                    }
                    utils::VisualItemsManager::get_instance().set_enabled(true);
                    toggle_item(item_id, slot);
                    send_clothing_change(client);
                    utils::PacketUtils::send_chat_message(local_player, 
                        fmt::format("`2[VIN] Equipped visual item `w{} `2in slot `w{}``!", item_id, slot));
                    return;
                }
            } catch (...) {}
        } else if (args.size() >= 3) {
            // Format: /clothes <slot> <item_id>
            try {
                std::string slot_str = first_arg;
                int slot = -1;
                if (slot_str == "hat") slot = 0;
                else if (slot_str == "shirt") slot = 1;
                else if (slot_str == "pants") slot = 2;
                else if (slot_str == "shoes" || slot_str == "shoe" || slot_str == "feet") slot = 3;
                else if (slot_str == "face" || slot_str == "mask") slot = 4;
                else if (slot_str == "hand" || slot_str == "weapon") slot = 5;
                else if (slot_str == "back" || slot_str == "wing" || slot_str == "wings") slot = 6;
                else if (slot_str == "hair") slot = 7;
                else if (slot_str == "neck") slot = 8;
                else if (slot_str == "ances" || slot_str == "artifact") slot = 9;
                else {
                    slot = std::stoi(slot_str);
                }

                int item_id = std::stoi(args[2]);
                if (slot >= 0 && slot <= 9 && item_id >= 0) {
                    utils::VisualItemsManager::get_instance().set_enabled(true);
                    if (item_id == 0) {
                        g_clothing_slots.erase(slot);
                        utils::VisualItemsManager::get_instance().set_visual_item(slot, 0);
                    } else {
                        set_pending_item(item_id, slot);
                    }
                    send_clothing_change(client);
                    utils::PacketUtils::send_chat_message(local_player, 
                        fmt::format("`2[VIN] Set visual slot `w{} `2to item `w{}``!", slot, item_id));
                    return;
                }
            } catch (...) {}
        }
    }

    spdlog::info("ClothesCommand: Applying visual clothing for {} items", g_clothing_slots.size());
    send_clothing_change(client);
}

void ClothesCommand::send_clothing_change(client::Client* client) {
    auto& player_tracker = utils::PlayerTracker::get_instance();
    uint32_t my_netid = player_tracker.get_local_netid();
    if (my_netid == 0) {
        my_netid = player_tracker.get_local_player().netID;
    }
    if (my_netid == 0) {
        for (const auto& [nid, p] : player_tracker.get_all_players()) {
            if (p.is_local && nid > 0) {
                my_netid = nid;
                break;
            }
        }
    }
    
    if (my_netid == 0) {
        spdlog::warn("ClothesCommand: player netID not found. Try after spawning.");
        return;
    }

    auto* client_player = (g_core && g_core->get_server() && g_core->get_server()->get_player()) 
                          ? g_core->get_server()->get_player() : (client ? client->get_player() : nullptr);
    if (!client_player) return;

    // Ensure VisualItemsManager has current g_clothing_slots
    for (const auto& [slot, item_id] : g_clothing_slots) {
        utils::VisualItemsManager::get_instance().set_visual_item(slot, item_id);
    }

    utils::VisualItemsManager::get_instance().send_visual_clothing(client_player, my_netid, g_core);
}

} // namespace command
