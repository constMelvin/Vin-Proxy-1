#include "utility_commands.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/world_parser_v2.h"
#include "../../utils/astar_pathfinding.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/text_parse.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#ifdef _WIN32
#include <Windows.h>
#endif
#include "../item_finder/item_finder.hpp"
#include "../../utils/weapon_animation_manager.hpp"
#include "clothes_command.hpp"


bool g_teleport_mode = false;

namespace command {

#pragma pack(push, 1)
struct MoriStatePacket {
    uint8_t type;
    uint8_t object_type;
    uint8_t jump_count;
    uint8_t animation_type;
    uint32_t net_id;
    int32_t target_net_id;
    uint32_t flags;
    float float_variable;
    uint32_t value;
    float vector_x;
    float vector_y;
    float vector_x2;
    float vector_y2;
    float particle_rotation;
    int32_t int_x;
    int32_t int_y;
    uint32_t extended_data_length;
};
#pragma pack(pop)

static_assert(sizeof(MoriStatePacket) == 56, "MoriStatePacket must be 56 bytes");

core::Core* FindPathCommand::s_core = nullptr;
bool FindPathCommand::s_click_mode_enabled = true;
int FindPathCommand::s_cooldown_ms = 2000;
std::chrono::steady_clock::time_point FindPathCommand::s_last_tp_time{};
std::atomic<uint64_t> FindPathCommand::s_current_path_id{0};
uint32_t FindPathCommand::s_last_click_tile_x{0};
uint32_t FindPathCommand::s_last_click_tile_y{0};
core::Core* PlayerTPCommand::s_core = nullptr;
core::Core* FlagCommand::s_core = nullptr;
core::Core* InvisCommand::s_core = nullptr;
bool InvisCommand::s_invis_enabled = false;
core::Core* MstateCommand::s_core = nullptr;
bool MstateCommand::s_mstate_enabled = false;
core::Core* SmCommand::s_core = nullptr;
bool SmCommand::s_sm_enabled = false;
core::Core* BetaCommand::s_core = nullptr;
bool BetaCommand::s_beta_enabled = false;
core::Core* FreezeCommand::s_core = nullptr;
core::Core* AntiGravityCommand::s_core = nullptr;
core::Core* AntiPunchCommand::s_core = nullptr;

void send_console(player::Player* player, const std::string& msg) {
    if (!player) return;
    packet::Variant var{};
    var.add("OnConsoleMessage");
    var.add(msg);

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

    player->send_packet(bs.get_data(), 0);
}

void send_overlay(player::Player* player, const std::string& msg) {
    if (!player) return;
    packet::Variant var{};
    var.add("OnTextOverlay");
    var.add(msg);

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

    player->send_packet(bs.get_data(), 0);
}


void send_generic(player::Player* p, const std::string& raw) {
    if (!p) return;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(raw, false);
    p->send_packet(bs.get_data(), 0);
}




static extension::item_finder::ItemDatabase* g_path_item_db = nullptr;

static bool is_lock_id(uint16_t item_id) {
    return (item_id == 202 || item_id == 204 || item_id == 206 || item_id == 242 || 
            item_id == 1796 || item_id == 2408 || item_id == 4994 || item_id == 7188 || 
            item_id == 8468 || item_id == 11550 || item_id == 4992);
}

static bool is_jammer_id(uint16_t item_id) {
    return (item_id == 226 || item_id == 228 || item_id == 1000 || item_id == 1002 || 
            item_id == 2830 || item_id == 4820 || item_id == 4822);
}

static bool is_weather_machine_id(uint16_t item_id) {
    // Weather Machine item IDs in Growtopia (Night 934, Sunset 936, Sunny 938, Undersea 1162, etc.)
    if (item_id == 934 || item_id == 936 || item_id == 938 || item_id == 1162 || 
        item_id == 1500 || item_id == 1798 || item_id == 2008 || item_id == 2480 ||
        item_id == 2972 || item_id == 3244 || item_id == 3834) {
        return true;
    }
    return false;
}

static bool is_solid_item(uint16_t item_id) {
    if (item_id == 0 || item_id == 18) return false; // Air / Punch
    if (item_id % 2 != 0) return false; // Seeds / Saplings (Odd item IDs are always seeds)

    // ALL Locks & ALL Jammers in Growtopia are ALWAYS SOLID
    if (is_lock_id(item_id) || is_jammer_id(item_id)) {
        return true;
    }

    // 1. Primary: Use items.dat collision_type from decoded database
    //    collision_type 1 = SOLID (blocks movement/teleport)
    //    collision_type 0 = PASSABLE (doors, signs, weather, etc.)
    //    collision_type 2 = Platform/Ladder (passable for teleport)
    if (g_path_item_db) {
        const auto* info = g_path_item_db->get_item_by_id(static_cast<int>(item_id));
        if (info) {
            // collision_type 1 = solid block in Growtopia's items.dat
            if (info->collision_type == 1) {
                return true;
            }
            // collision_type 0, 2, 3, 4+ = not solid (passable for teleport)
            if (info->collision_type != 1) {
                return false;
            }
        }
    }

    // 2. Fallback: hardcoded rules when database is unavailable
    if (is_weather_machine_id(item_id)) return false;

    // Doors, Portals & Gateways (Passable)
    if (item_id == 6 || item_id == 12 || item_id == 26 || item_id == 32 || item_id == 546 ||
        item_id == 740 || item_id == 858 || item_id == 1684 || item_id == 2272 || item_id == 3804 || item_id == 3806) {
        return false;
    }

    // Signs, Vending Machines, Displays, Donation Boxes, Furniture, Storage, Weather Machines (Passable)
    if (item_id == 20 || item_id == 24 || item_id == 30 || item_id == 50 || item_id == 608 ||
        item_id == 656 || item_id == 658 || item_id == 1420 || item_id == 1452 ||
        item_id == 1682 || item_id == 2978 || item_id == 3898 || item_id == 8706 ||
        item_id == 934 || item_id == 936 || item_id == 938 || item_id == 1162) {
        return false;
    }

    // ALL Platforms, Ladders, Ropes, Vines, Trampolines, Checkpoints (Passable)
    if (item_id == 382 || item_id == 452 || item_id == 756 || item_id == 928 || item_id == 1114 ||
        item_id == 1558 || item_id == 3230) {
        return false;
    }

    // Standard solid building blocks (Dirt, Lava, Stone, Bedrock, Bricks, Glass, Spikes, etc.)
    return true;
}

static bool is_deadly_or_solid_tile(uint16_t fg) {
    return is_solid_item(fg);
}

static bool is_tile_solid(uint32_t tile_x, uint32_t tile_y) {
    auto& world_mgr = utils::WorldManager::get_instance();
    uint32_t width = world_mgr.get_world_width();
    uint32_t height = world_mgr.get_world_height();
    if (width == 0 || height == 0 || tile_x >= width || tile_y >= height) {
        return false;
    }

    const auto world_v2 = world_mgr.get_world_v2();
    if (world_v2.is_valid && !world_v2.tiles.empty()) {
        size_t idx = static_cast<size_t>(tile_y) * width + tile_x;
        if (idx < world_v2.tiles.size()) {
            const auto& t = world_v2.tiles[idx];
            // Air (0), Punch (18), Seeds (odd IDs)
            if (t.fg == 0 || t.fg == 18 || (t.fg % 2 != 0)) {
                return false;
            }
            // Locks and Jammers in Growtopia are ALWAYS SOLID
            if (t.is_lock() || is_lock_id(t.fg) || is_jammer_id(t.fg)) {
                return true;
            }
            // Weather Machines (extra_type == 34) are ALWAYS PASSABLE
            if (t.extra_type == 34 || is_weather_machine_id(t.fg)) {
                return false;
            }
            return is_deadly_or_solid_tile(t.fg);
        }
    }

    const auto& tiles = world_mgr.get_tiles();
    size_t idx = static_cast<size_t>(tile_y) * width + tile_x;
    if (idx < tiles.size()) {
        uint16_t fg = tiles[idx].Fg;
        return is_deadly_or_solid_tile(fg);
    }
    return false;
}

FindPathCommand::FindPathCommand() : CommandBase(
    {"path", "pathf", "findpath"},
    {"<x>", "<y>"},
    "Walk to coordinates using A* pathfinding (fast).",
    0
) {}

std::unique_ptr<CommandBase> FindPathCommand::clone() const {
    return std::make_unique<FindPathCommand>(*this);
}

void FindPathCommand::set_core(core::Core* core) {
    s_core = core;
}

void FindPathCommand::set_item_database(extension::item_finder::ItemDatabase* database) {
    g_path_item_db = database;
}

bool FindPathCommand::is_click_mode_enabled() {
    return s_click_mode_enabled; 
}

void FindPathCommand::toggle_click_mode() {
    s_click_mode_enabled = !s_click_mode_enabled;
    std::string msg = s_click_mode_enabled 
        ? "`2Shift+Click pathfinding: `bON`2"
        : "`4Shift+Click pathfinding: `9OFF";
    if (s_core && s_core->get_server() && s_core->get_server()->get_player()) {
        send_console(s_core->get_server()->get_player(), msg);
    }
    spdlog::info("[ClickMode] {}", msg);
}

int FindPathCommand::get_cooldown_ms() {
    return s_cooldown_ms;
}

void FindPathCommand::set_cooldown_ms(int ms) {
    s_cooldown_ms = std::clamp(ms, 100, 10000);
}

void FindPathCommand::show_settings_dialog(player::Player* player) {
    if (!player) return;

    std::string is_enabled_str = s_click_mode_enabled ? "1" : "0";
    std::string status_color = s_click_mode_enabled ? "`2" : "`4";
    std::string status_text = s_click_mode_enabled ? "ON" : "OFF";

    std::string dialog = 
        "set_default_color|`o\n"
        "add_label_with_icon|big|`wPathfinding Settings``|left|32|\n"
        "add_spacer|small|\n"
        "add_textbox|`oCurrent Status: " + status_color + status_text + "``|left|\n"
        "add_checkbox|pathfind_enable|`oEnable Pathfinding|" + is_enabled_str + "|\n"
        "add_spacer|small|\n"
        "add_textbox|`oCooldown delay between teleports (100-10000ms):``|left|\n"
        "add_text_input|pathfind_delay|Delay (ms):|" + std::to_string(s_cooldown_ms) + "|6|\n"
        "add_spacer|small|\n"
        "add_button|apply_pathfind|`2Apply Settings``|\n"
        "add_button|back_pathfind|`5Back to Main``|\n"
        "add_quick_exit|\n"
        "end_dialog|pathfind_gui|Close||";

    packet::Variant var{};
    var.add("OnDialogRequest");
    var.add(dialog);

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

    player->send_packet(bs.get_data(), 0);
    spdlog::info("[PathGUI] Settings dialog sent to player");
}

void FindPathCommand::handle_dialog_response(player::Player* player, const std::string& button_clicked, const std::string& dialog_data) {
    if (!player) return;

    spdlog::info("[PathGUI] Dialog response - button='{}', data='{}'", button_clicked, dialog_data);

    if (button_clicked == "apply_pathfind" || button_clicked.empty()) {
        apply_dialog_settings(dialog_data);
        show_settings_dialog(player);
        return;
    }
    if (button_clicked == "back_pathfind") {
        return;
    }
}

void FindPathCommand::apply_dialog_settings(const std::string& dialog_data) {
    TextParse tp{dialog_data};

    // 1. Apply Enable / Disable checkbox state
    std::string enable_val = tp.get("pathfind_enable");
    if (!enable_val.empty()) {
        s_click_mode_enabled = (enable_val == "1");
    }

    // 2. Apply Cooldown Delay input
    std::string delay_str = tp.get("pathfind_delay");
    if (!delay_str.empty()) {
        try {
            int delay = std::stoi(delay_str);
            set_cooldown_ms(delay);
        } catch (...) {
            spdlog::warn("[PathGUI] Invalid delay value: {}", delay_str);
        }
    }

    // 3. Send console confirmation message
    std::string status_str = s_click_mode_enabled ? "`2ON" : "`4OFF";
    if (s_core && s_core->get_server() && s_core->get_server()->get_player()) {
        send_console(s_core->get_server()->get_player(), 
            fmt::format("`2Pathfinding settings applied! Status: {}`2, Cooldown: `b{}ms`2.", status_str, s_cooldown_ms));
    }
    spdlog::info("[PathGUI] Settings applied: enabled={}, cooldown={}ms", s_click_mode_enabled, s_cooldown_ms);
}

int FindPathCommand::run_path(client::Client* client, uint32_t target_x, uint32_t target_y, bool show_console, int delay_ms, uint64_t path_id) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("FindPathCommand: No core or player!");
        return -1;
    }

    if (path_id != 0 && s_current_path_id != path_id) {
        return -1;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("FindPathCommand: No server player!");
        return -1;
    }

    
    std::string pos_x_str = s_core->get_config().get<std::string>("player.position.x");
    std::string pos_y_str = s_core->get_config().get<std::string>("player.position.y");
    
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    bool has_pos = false;
    try {
        if (!pos_x_str.empty() && !pos_y_str.empty()) {
            pos_x = std::stof(pos_x_str);
            pos_y = std::stof(pos_y_str);
            if (pos_x > 0.0f || pos_y > 0.0f) {
                has_pos = true;
            }
        }
    } catch (...) {}

    if (!has_pos) {
        auto& tracker = utils::PlayerTracker::get_instance();
        if (tracker.has_local_player()) {
            auto lp = tracker.get_local_player();
            auto ppos = tracker.get_player_position(lp.netID);
            if (ppos.x > 0.0f || ppos.y > 0.0f) {
                pos_x = ppos.x;
                pos_y = ppos.y;
                has_pos = true;
            }
        }
    }

    if (!has_pos) {
        if (show_console) send_console(server->get_player(), "`4Position not tracked - move first!");
        return -1;
    }
    
    uint32_t current_x = static_cast<uint32_t>(pos_x / 32.0f);
    uint32_t current_y = static_cast<uint32_t>(pos_y / 32.0f);
    
    
    auto& world_manager = utils::WorldManager::get_instance();
    uint32_t WORLD_WIDTH = world_manager.get_world_width();
    uint32_t WORLD_HEIGHT = world_manager.get_world_height();
    if (WORLD_WIDTH == 0 || WORLD_HEIGHT == 0) {
        WORLD_WIDTH = 100;
        WORLD_HEIGHT = 60;
    }
    
    
    pathfinding::AStar astar(WORLD_WIDTH, WORLD_HEIGHT);

    const auto& tiles = world_manager.get_tiles();
    std::vector<uint8_t> collision_data(static_cast<size_t>(WORLD_WIDTH) * WORLD_HEIGHT, 0);

    if (!tiles.empty() && tiles.size() >= static_cast<size_t>(WORLD_WIDTH) * WORLD_HEIGHT) {
        for (uint32_t y = 0; y < WORLD_HEIGHT; ++y) {
            for (uint32_t x = 0; x < WORLD_WIDTH; ++x) {
                size_t i = static_cast<size_t>(y) * WORLD_WIDTH + x;
                collision_data[i] = is_tile_solid(x, y) ? 1 : 0;
            }
        }
    }

    
    if (current_x < WORLD_WIDTH && current_y < WORLD_HEIGHT) {
        collision_data[static_cast<size_t>(current_y) * WORLD_WIDTH + current_x] = 0;
    }
    if (target_x < WORLD_WIDTH && target_y < WORLD_HEIGHT) {
        collision_data[static_cast<size_t>(target_y) * WORLD_WIDTH + target_x] = 0;
    }

    astar.set_collision_data(collision_data);
    spdlog::debug("[Path] Collision map loaded from world tiles: {} entries", collision_data.size());
    
    
    bool has_access = true;  
    

    
    auto path = astar.find_path(current_x, current_y, target_x, target_y, has_access);
    
    if (path.empty()) {
        if (show_console) send_console(server->get_player(), "`4No path found!");
        spdlog::warn("[Path] No path from ({},{}) to ({},{})", 
                    current_x, current_y, target_x, target_y);
        return -1;
    }
    
    
    size_t max_steps = 200;
    if (path.size() > max_steps) {
        if (show_console) {
            send_console(server->get_player(),
                        fmt::format("`6Path has {} nodes, limiting to {} steps", path.size(), max_steps));
        }
        path.resize(max_steps);
    }

    
    std::vector<decltype(path)::value_type> waypoints(path.begin(), path.end());

    if (show_console) {
        send_console(server->get_player(), fmt::format("`2Walking {} -> {} steps", path.size(), waypoints.size()));
    }

    
    int findpath_delay_ms = s_core->get_config().get<int>("command.path.delay_ms", 55);
    if (delay_ms >= 0) {
        findpath_delay_ms = delay_ms;
    }
    float current_px = pos_x;
    float current_py = pos_y;
    for (size_t i = 0; i < waypoints.size(); ++i) {
        if (path_id != 0 && s_current_path_id != path_id) {
            spdlog::info("[Path] Pathfinding #{} cancelled by new pathfinding #{}", path_id, s_current_path_id.load());
            return -1;
        }

        const auto& node = waypoints[i];
        
        const float px = static_cast<float>(node.x * 32);
        const float py = static_cast<float>(node.y * 32);
        const float prev_px = current_px;
        const float prev_py = current_py;
        current_px = px;
        current_py = py;

        uint32_t player_netid = 0;
        auto local = utils::PlayerTracker::get_instance().get_local_player();
        if (local.netID != 0) {
            player_netid = local.netID;
        }

        MoriStatePacket tank_pkt{};
        tank_pkt.type = static_cast<uint8_t>(packet::PACKET_STATE);
        tank_pkt.object_type = 0;
        tank_pkt.jump_count = 0;
        tank_pkt.animation_type = 0;
        tank_pkt.net_id = player_netid;
        tank_pkt.target_net_id = 0;
        tank_pkt.vector_x = current_px;
        tank_pkt.vector_y = current_py + 2.0f;
        tank_pkt.vector_x2 = 0.0f;
        tank_pkt.vector_y2 = 0.0f;
        tank_pkt.int_x = -1;
        tank_pkt.int_y = -1;
        tank_pkt.extended_data_length = 0;
        
        
        tank_pkt.flags = 0x1u | packet::PACKET_FLAG_ON_SOLID;
        const bool face_left = prev_px > px;
        if (face_left) {
            tank_pkt.flags |= 0x10u; 
        }
        const bool going_up = (py < prev_py);
        if (going_up) {
            tank_pkt.flags |= 0x80u; 
        }

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(tank_pkt);
        
        client->get_player()->send_packet_unreliable(bs.get_data(), 0);

        
        if (going_up) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client->get_player()->send_packet_unreliable(bs.get_data(), 0);
        }
        spdlog::info("[Path] Step {}/{} -> tile ({},{}) mode=state", i + 1, waypoints.size(), node.x, node.y);
        std::this_thread::sleep_for(std::chrono::milliseconds(findpath_delay_ms));
    }

    if (show_console) {
        send_console(server->get_player(), "`2Path walk complete!");
    }
    return static_cast<int>(waypoints.size());
}

bool FindPathCommand::handle_shift_click(client::Client* client, uint32_t tile_x, uint32_t tile_y) {
    if (!s_click_mode_enabled) {
        return false;
    }

    if (!client || !client->get_player()) {
        return false;
    }

    auto* server = s_core ? s_core->get_server() : nullptr;
    if (!server || !server->get_player()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_tp_time).count();

    // Ignore duplicate packets sent by GT client for the same single click
    if (tile_x == s_last_click_tile_x && tile_y == s_last_click_tile_y && elapsed_ms < 150) {
        return true;
    }

    // Cooldown to prevent rapid clicking
    if (elapsed_ms < s_cooldown_ms) {
        send_overlay(server->get_player(), "`6Waiting for next Pathfinding.");
        return true;
    }

    // Solid Block Detector
    if (is_tile_solid(tile_x, tile_y)) {
        send_overlay(server->get_player(), "`4Teleport cancelled: target is a solid block!");
        return true;
    }

    s_last_click_tile_x = tile_x;
    s_last_click_tile_y = tile_y;
    s_last_tp_time = now;

    auto local = utils::PlayerTracker::get_instance().get_local_player();
    uint32_t netid = local.netID;
    const float target_px = static_cast<float>(tile_x * 32);
    const float target_py = static_cast<float>(tile_y * 32);

    // Calculate blocks traveled from current position
    float cur_px = 0.0f, cur_py = 0.0f;
    bool has_pos = false;

    // Try config first (most reliable)
    std::string pos_x_str = s_core->get_config().get<std::string>("player.position.x");
    std::string pos_y_str = s_core->get_config().get<std::string>("player.position.y");
    try {
        if (!pos_x_str.empty() && !pos_y_str.empty()) {
            cur_px = std::stof(pos_x_str);
            cur_py = std::stof(pos_y_str);
            if (cur_px > 0.0f || cur_py > 0.0f) has_pos = true;
        }
    } catch (...) {}

    // Fallback to player tracker
    if (!has_pos && netid > 0) {
        auto ppos = utils::PlayerTracker::get_instance().get_player_position(netid);
        if (ppos.x > 0.0f || ppos.y > 0.0f) {
            cur_px = ppos.x;
            cur_py = ppos.y;
            has_pos = true;
        }
    }

    uint32_t blocks = 0;
    if (has_pos) {
        uint32_t cur_tx = static_cast<uint32_t>(cur_px / 32.0f);
        uint32_t cur_ty = static_cast<uint32_t>(cur_py / 32.0f);
        int32_t dx = static_cast<int32_t>(tile_x) - static_cast<int32_t>(cur_tx);
        int32_t dy = static_cast<int32_t>(tile_y) - static_cast<int32_t>(cur_ty);
        blocks = static_cast<uint32_t>(std::abs(dx) + std::abs(dy));
    }

    // 1. Instantly update client visual position so character moves immediately on screen
    auto tp_start = std::chrono::steady_clock::now();

    SetPosCommand::set_position(server->get_player(), netid, target_px, target_py);

    // 2. Update player tracker position to target coordinates
    if (netid > 0) {
        utils::PlayerTracker::get_instance().update_player_position(netid, target_px, target_py);
    }

    // 3. Send state packet to real server with dynamic ground physics flag
    bool ground_below = is_tile_solid(tile_x, tile_y + 1);

    MoriStatePacket tank_pkt{};
    tank_pkt.type = static_cast<uint8_t>(packet::PACKET_STATE);
    tank_pkt.net_id = netid;
    tank_pkt.vector_x = target_px;
    tank_pkt.vector_y = target_py + 2.0f;
    tank_pkt.int_x = -1;
    tank_pkt.int_y = -1;
    tank_pkt.flags = ground_below ? 0x1u : 0x0u;

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(tank_pkt);
    if (client && client->get_player()) {
        client->get_player()->send_packet_unreliable(bs.get_data(), 0);
    }

    // 4. Re-send SetPos to local client to lock character position at destination
    SetPosCommand::set_position(server->get_player(), netid, target_px, target_py);

    auto tp_end = std::chrono::steady_clock::now();
    double tp_secs = std::chrono::duration<double>(tp_end - tp_start).count();

    // 5. Send overlay notification with travel distance and execution time
    send_overlay(server->get_player(), fmt::format("`2Pathfinding to `6{} `wblocks. `6{:.2f} `wsecs.", blocks, tp_secs));

    return true;
}

void FindPathCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("FindPathCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("FindPathCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        show_settings_dialog(server->get_player());
        return;
    }

    std::string arg1 = args[1];
    std::transform(arg1.begin(), arg1.end(), arg1.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (arg1 == "enable" || arg1 == "on" || arg1 == "1") {
        s_click_mode_enabled = true;
        send_console(server->get_player(), "`2Pathfinding enabled.");
        return;
    }
    if (arg1 == "disable" || arg1 == "off" || arg1 == "0") {
        s_click_mode_enabled = false;
        send_console(server->get_player(), "`4Pathfinding disabled.");
        return;
    }
    if (arg1 == "toggle") {
        toggle_click_mode();
        return;
    }

    // Single numeric argument: e.g. /path 500 -> set cooldown delay to 500ms
    if (args.size() == 2) {
        try {
            int delay = std::stoi(arg1);
            set_cooldown_ms(delay);
            send_console(server->get_player(), fmt::format("`2Pathfinding cooldown set to `b{}ms`2.", s_cooldown_ms));
            return;
        } catch (...) {
            show_settings_dialog(server->get_player());
            return;
        }
    }

    uint32_t target_x = 0, target_y = 0;
    try {
        target_x = static_cast<uint32_t>(std::stoi(args[1]));
        target_y = static_cast<uint32_t>(std::stoi(args[2]));
    } catch (const std::exception&) {
        send_console(server->get_player(), "`4Invalid coordinates");
        return;
    }

    uint64_t my_path_id = ++s_current_path_id;
    const int used_steps = run_path(client, target_x, target_y, true, 1, my_path_id);
    if (used_steps > 0 && s_current_path_id == my_path_id) {
        send_overlay(server->get_player(),
            fmt::format("`2Pathfind:`w {} steps to (`9{},{} `w) `o(fast)", used_steps, target_x, target_y));
    }
}


PlayerTPCommand::PlayerTPCommand() : CommandBase(
    {"player", "tpplayer"},
    {"<name>"},
    "Teleport to player by name",
    1
) {}

std::unique_ptr<CommandBase> PlayerTPCommand::clone() const {
    return std::make_unique<PlayerTPCommand>(*this);
}

void PlayerTPCommand::set_core(core::Core* core) {
    s_core = core;
}

void PlayerTPCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("PlayerTPCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("PlayerTPCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /player <name>");
        return;
    }

    
    std::string player_name;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) player_name += " ";
        player_name += args[i];
    }

    
    size_t pos = 0;
    while ((pos = player_name.find('`')) != std::string::npos) {
        if (pos + 1 < player_name.length()) {
            player_name.erase(pos, 2);
        } else {
            break;
        }
    }

    
    
    
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Searching for player: `b{}", player_name);
    send_console(server->get_player(), msg);
    
    
    
    
    spdlog::info("Attempting to TP to player: {}", player_name);
}


FlagCommand::FlagCommand() : CommandBase(
    {"flag", "country"},
    {"<flag_id>"},
    "Change country flag display",
    1
) {}

std::unique_ptr<CommandBase> FlagCommand::clone() const {
    return std::make_unique<FlagCommand>(*this);
}

void FlagCommand::set_core(core::Core* core) {
    s_core = core;
}

void FlagCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("FlagCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("FlagCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /flag <flag_id>");
        return;
    }

    std::string flag_id = args[1];

    
    s_core->get_config().set("player.country_flag", flag_id);
    s_core->get_config().save();

    
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();
    int32_t local_netid = local_player.netID;
    
    if (local_netid == 0 || local_netid == -1) {
        send_console(server->get_player(), "`4Error: Player not tracked yet. Try again after spawning.");
        return;
    }

    
    packet::Variant var{};
    var.add("OnCountryState");
    var.add(flag_id);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_netid;  
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    server->get_player()->send_packet(bs.get_data(), 0);
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Flag changed to `b{}", flag_id);
    send_console(server->get_player(), msg);
    
    spdlog::info("Flag changed to: {} (netid: {})", flag_id, local_netid);
}


InvisCommand::InvisCommand() : CommandBase(
    {"invis"},
    {},
    "Toggle invisibility mode",
    0
) {}

std::unique_ptr<CommandBase> InvisCommand::clone() const {
    return std::make_unique<InvisCommand>(*this);
}

void InvisCommand::set_core(core::Core* core) {
    s_core = core;
}

void InvisCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("InvisCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("InvisCommand: No server player!");
        return;
    }

    
    s_invis_enabled = !s_invis_enabled;
    
    
    s_core->get_config().set("player.invis_enabled", s_invis_enabled);
    s_core->get_config().save();
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Invisibility: `b{}\n`6Rejoin world to apply!", 
                                   s_invis_enabled ? "ON" : "OFF");
    send_console(server->get_player(), msg);
    
    spdlog::info("Invisibility toggled: {} (rejoin to apply)", s_invis_enabled ? "ON" : "OFF");
}

bool InvisCommand::is_invis_enabled() {
    return s_invis_enabled;
}


SmCommand::SmCommand() : CommandBase(
    {"sm"},
    {},
    "Toggle SuperMod state",
    0
) {}

std::unique_ptr<CommandBase> SmCommand::clone() const {
    return std::make_unique<SmCommand>(*this);
}

void SmCommand::set_core(core::Core* core) {
    s_core = core;
}

void SmCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("SmCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("SmCommand: No server player!");
        return;
    }

    
    s_sm_enabled = !s_sm_enabled;
    
    
    s_core->get_config().set("player.sm_enabled", s_sm_enabled);
    s_core->get_config().save();

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        
        packet::Variant var{};
        var.add("OnSuperSupportState");
        var.add(s_sm_enabled ? 1 : 0);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9SuperMod: `b{} `9(applied immediately!)\n`4Note: Server validates mod permissions", 
                                       s_sm_enabled ? "ON" : "OFF");
        send_console(server->get_player(), msg);
        spdlog::info("SuperMod state toggled to {} with OnSuperSupportState variant", s_sm_enabled ? "ON" : "OFF");
    } else {
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9SuperMod: `b{}\n`6Rejoin world to apply!", 
                                       s_sm_enabled ? "ON" : "OFF");
        send_console(server->get_player(), msg);
        spdlog::info("SuperMod state set to {} in config (player not tracked, rejoin required)", s_sm_enabled ? "ON" : "OFF");
    }
}

bool SmCommand::is_sm_enabled() {
    return s_sm_enabled;
}


FreezeCommand::FreezeCommand() : CommandBase(
    {"freeze"},
    {"[netid]"},
    "Freeze yourself (toggle) or try to freeze a player by netID",
    0
) {}

std::unique_ptr<CommandBase> FreezeCommand::clone() const {
    return std::make_unique<FreezeCommand>(*this);
}

void FreezeCommand::set_core(core::Core* core) {
    s_core = core;
}

void FreezeCommand::freeze_player(player::Player* player, uint32_t netid, const std::string& name, bool freeze_state) {
    if (!player) return;

    
    packet::Variant var{};
    var.add("OnSetFreezeState");
    var.add(freeze_state ? 1 : 0);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = netid;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    player->send_packet(bs.get_data(), 0);
    
    spdlog::info("Freeze packet sent to {} (netid: {}) - state: {}", name, netid, freeze_state ? "frozen" : "unfrozen");
}

void FreezeCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("FreezeCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("FreezeCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    
    if (args.size() < 2) {
        if (local_player.netID == 0) {
            send_console(server->get_player(), "`4Error: Player not tracked yet. Try after spawning.");
            return;
        }

        
        static bool is_frozen = false;
        is_frozen = !is_frozen;

        freeze_player(server->get_player(), local_player.netID, local_player.name, is_frozen);

        std::string msg = fmt::format("`0[ `bVinProxy `0] `9You are now `b{}`9!", 
                                       is_frozen ? "FROZEN" : "UNFROZEN");
        send_console(server->get_player(), msg);
        
        spdlog::info("Self freeze toggled: {}", is_frozen ? "ON" : "OFF");
        return;
    }

    
    uint32_t target_netid = 0;
    try {
        target_netid = static_cast<uint32_t>(std::stoi(args[1]));
    } catch (const std::exception& e) {
        send_console(server->get_player(), "`4Invalid netID!");
        
        
        auto players = tracker.get_all_players();
        if (!players.empty()) {
            send_console(server->get_player(), "`9Available players:");
            for (const auto& [netid, info] : players) {
                std::string msg = fmt::format("`b{} `0- NetID: `9{}{}", 
                    info.name, netid, info.is_local ? " `6(YOU)" : "");
                send_console(server->get_player(), msg);
            }
        }
        return;
    }

    
    auto target_player = tracker.get_player_by_netid(target_netid);
    
    if (target_player.netID == 0) {
        send_console(server->get_player(), "`4Player not found!");
        return;
    }

    
    freeze_player(server->get_player(), target_netid, target_player.name, true);
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Freeze packet sent to `b{} `9(NetID: `b{})`9!", 
                                   target_player.name, target_netid);
    send_console(server->get_player(), msg);
    
    if (target_player.is_local) {
        send_console(server->get_player(), "`6This is you!");
    } else {
        send_console(server->get_player(), "`4Note: May only work with mod permissions");
    }
}


core::Core* GhostCharCommand::s_core = nullptr;
bool GhostCharCommand::s_ghost_enabled = false;

GhostCharCommand::GhostCharCommand() : CommandBase(
    {"ghostchardddddddddddddddddd"},
    {},
    "Toggle ghost character mode (send ghost packets and modify animations)",
    0
) {}

std::unique_ptr<CommandBase> GhostCharCommand::clone() const {
    return std::make_unique<GhostCharCommand>(*this);
}

void GhostCharCommand::set_core(core::Core* core) {
    s_core = core;
}

bool GhostCharCommand::is_enabled() {
    return s_ghost_enabled;
}

void GhostCharCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    spdlog::info("[GhostChar] execute called (s_core={}, client={}, player={})",
                 (void*)s_core, (void*)client,
                 client && client->get_player() ? (void*)client->get_player() : nullptr);

    if (!s_core || !client || !client->get_player()) {
        spdlog::warn("[GhostChar] missing core or client/player");
        return;
    }
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::warn("[GhostChar] missing server or server player");
        return;
    }

    
    auto send_feedback = [&](const std::string& msg) {
        send_console(server->get_player(), msg);
    };

    
    if (s_ghost_enabled) {
        s_ghost_enabled = false;
        send_feedback("`6Ghost mode `4OFF`6.");
        spdlog::info("[GhostChar] disabled");

        
        
        send_generic(client->get_player(), "action|setDeath\nanimDeath|0");
        send_generic(client->get_player(), "action|setRespawn\nanimRespawn|0");
        return;
    }

    
    s_ghost_enabled = true;
    send_feedback("`2Ghost mode `bON`2. Go haunt some shit.");
    spdlog::info("[GhostChar] enabled");

    
    
    
    
    send_generic(client->get_player(), "action|setDeath\nanimDeath|1");
    send_generic(client->get_player(), "action|setRespawn\nanimRespawn|1");

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    constexpr uint32_t GHOST_FLAGS =
        packet::PACKET_FLAG_ON_RESPAWNED |   
        packet::PACKET_FLAG_ON_SOLID;        

    std::thread([]() {
        while (GhostCharCommand::s_ghost_enabled) {
            
            
            float x = 0.0f, y = 0.0f;
            {
                auto& tracker = utils::PlayerTracker::get_instance();
                auto local = tracker.get_local_player();
                x = local.position.x;
                y = local.position.y;
            }

            spdlog::debug("[GhostThread] keepalive at ({:.1f}, {:.1f})", x, y);

            auto* core = GhostCharCommand::s_core;
            if (core) {
                auto* srv = core->get_server();
                auto* cli = core->get_client();

                
                packet::TankUpdatePacket state{};
                state.type           = static_cast<uint8_t>(packet::PACKET_STATE);
                state.object_type    = 0;
                state.jump_count     = 0;
                state.animation_type = 0;
                state.net_id         = 0;   
                state.target_net_id  = 0;
                state.flags          = GHOST_FLAGS;
                state.float_var      = 0.0f;
                state.int_data       = 0;
                state.vec_x          = x;
                state.vec_y          = y;
                state.vec_x2         = 0.0f;
                state.vec_y2         = 0.0f;
                state.particle_time  = 0.0f;
                state.int_x          = -1;
                state.int_y          = -1;
                state.extra_data_size = 0;

                
                ByteStream<std::uint16_t> bs{};
                bs.write(packet::NET_MESSAGE_GAME_PACKET);
                bs.write(state);  

                
                
                if (cli && cli->get_player())
                    cli->get_player()->send_packet_unreliable(bs.get_data(), 0);

                
                
                if (srv && srv->get_player())
                    srv->get_player()->send_packet(bs.get_data(), 0);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        spdlog::info("[GhostThread] thread exiting");
    }).detach();
}


core::Core* HitVFXCommand::s_core = nullptr;

HitVFXCommand::HitVFXCommand() : CommandBase(
    {"hitvfx", "vfx"},
    {"<effectid>"},
    "Trigger hit visual effects (0=default, 1-100=custom effects)",
    0
) {}

std::unique_ptr<CommandBase> HitVFXCommand::clone() const {
    return std::make_unique<HitVFXCommand>(*this);
}

void HitVFXCommand::set_core(core::Core* core) {
    s_core = core;
}

void HitVFXCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("HitVFXCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("HitVFXCommand: No server player!");
        return;
    }

    
    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /hitvfx <effectid>");
        send_console(server->get_player(), "`9Effect IDs:");
        send_console(server->get_player(), "`b0 `0- Default/None");
        send_console(server->get_player(), "`b1-100 `0- Various custom effects (experiment!)");
        send_console(server->get_player(), "`9Example: `b/hitvfx 5 `9to try effect 5");
        return;
    }

    int effect_id = 0;
    try {
        effect_id = std::stoi(args[1]);
    } catch (const std::exception& e) {
        send_console(server->get_player(), "`4Invalid effect ID! Must be a number.");
        return;
    }

    if (effect_id < 0 || effect_id > 100) {
        send_console(server->get_player(), "`4Effect ID must be between 0-100!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID == 0) {
        send_console(server->get_player(), "`4Error: Player not tracked yet. Try after spawning.");
        return;
    }

    
    
    packet::Variant var{};
    var.add("OnHitVFX");
    var.add(effect_id);
    var.add(1); 

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_player.netID;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    server->get_player()->send_packet(bs.get_data(), 0);

    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Hit VFX effect `b{} `9applied!", effect_id);
    send_console(server->get_player(), msg);
    send_console(server->get_player(), "`6Try punching blocks to see the effect!");
    
    spdlog::info("Hit VFX packet sent with effect ID: {}", effect_id);
}


core::Core* AuctionBidCommand::s_core = nullptr;

AuctionBidCommand::AuctionBidCommand() : CommandBase(
    {"bid", "auctionbid"},
    {"<amount>"},
    "Place an auction bid with specified amount",
    0
) {}

std::unique_ptr<CommandBase> AuctionBidCommand::clone() const {
    return std::make_unique<AuctionBidCommand>(*this);
}

void AuctionBidCommand::set_core(core::Core* core) {
    s_core = core;
}

void AuctionBidCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("AuctionBidCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("AuctionBidCommand: No server player!");
        return;
    }

    
    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /bid <amount>");
        send_console(server->get_player(), "`9Example: `b/bid 10 `9to bid 10 WLs");
        send_console(server->get_player(), "`6Note: You must be at an auction block!");
        return;
    }

    int bid_amount = 0;
    try {
        bid_amount = std::stoi(args[1]);
    } catch (const std::exception& e) {
        send_console(server->get_player(), "`4Invalid bid amount! Must be a number.");
        return;
    }

    if (bid_amount <= 0) {
        send_console(server->get_player(), "`4Bid amount must be positive!");
        return;
    }

    
    std::string action_text = fmt::format("action|auction_place_bid\nbid_amount|{}", bid_amount);
    
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(action_text);

    server->get_player()->send_packet(bs.get_data(), 0);

    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Auction bid sent: `b{} WLs`9!", bid_amount);
    send_console(server->get_player(), msg);
    send_console(server->get_player(), "`6Make sure you're at an active auction block!");
    
    spdlog::info("Auction bid packet sent with amount: {}", bid_amount);
}


core::Core* UbiclubCommand::s_core = nullptr;

void UbiclubCommand::set_core(core::Core* core) {
    s_core = core;
}

UbiclubCommand::UbiclubCommand() : CommandBase(
    {"ubiclub", "ubisoft"},
    {},
    "Opens Ubisoft Club/Connect interface",
    0
) {}

std::unique_ptr<CommandBase> UbiclubCommand::clone() const {
    return std::make_unique<UbiclubCommand>(*this);
}

void UbiclubCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("UbiclubCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("UbiclubCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID == 0) {
        send_console(server->get_player(), "`4Error: Player not tracked yet. Try after spawning.");
        return;
    }

    spdlog::info("UbiclubCommand: Sending OnUbiclubRequest variant packet");

    
    packet::Variant var{};
    var.add("OnUbiclubRequest");
    
    
    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_player.netID;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());
    
    
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());
    
    server->get_player()->send_packet(bs.get_data(), 0);
    
    send_console(server->get_player(), "`0[ `bVinProxy `0] `9Opening Ubisoft Club interface...");
    send_console(server->get_player(), "`6If nothing happens, this feature may require special permissions.");
    
    spdlog::info("OnUbiclubRequest packet sent");
}


core::Core* WorldLockStorageCommand::s_core = nullptr;

void WorldLockStorageCommand::set_core(core::Core* core) {
    s_core = core;
}

WorldLockStorageCommand::WorldLockStorageCommand() : CommandBase(
    {"wlbank", "wlstorage"},
    {"<amount>"},
    "Modify World Lock storage amount (positive=deposit, negative=withdraw)",
    0
) {}

std::unique_ptr<CommandBase> WorldLockStorageCommand::clone() const {
    return std::make_unique<WorldLockStorageCommand>(*this);
}

void WorldLockStorageCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("WorldLockStorageCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("WorldLockStorageCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /wlbank <amount>");
        send_console(server->get_player(), "`9Positive amount = deposit, Negative = withdraw");
        send_console(server->get_player(), "`9Example: `b/wlbank 10 `9or `b/wlbank -5");
        return;
    }

    int amount = 0;
    try {
        amount = std::stoi(args[1]);
    } catch (const std::exception& e) {
        send_console(server->get_player(), "`4Invalid amount! Must be a number.");
        return;
    }

    if (amount == 0) {
        send_console(server->get_player(), "`4Amount cannot be zero!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID == 0) {
        send_console(server->get_player(), "`4Error: Player not tracked yet. Try after spawning.");
        return;
    }

    
    
    int type = (amount > 0) ? 1 : -1;
    int abs_amount = std::abs(amount);
    
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Spamming for 30 seconds: `b{} WLs", abs_amount).c_str());
    
    if (type == 1) {
        send_console(server->get_player(), "`2Open WL storage UI NOW - will deposit repeatedly!");
    } else {
        send_console(server->get_player(), "`6Open WL storage UI NOW - will withdraw repeatedly!");
    }
    
    send_console(server->get_player(), "`9This will run for 30 seconds - enough to empty most banks!");
    
    spdlog::info("WorldLockStorageCommand: Spamming amount: {}, type: {} for 30 seconds", abs_amount, type);

    
    for (int i = 0; i < 150; i++) {
        std::string action_text = fmt::format("action|worldlock_storage_modify_amount\namount|{}\ntype|{}", abs_amount, type);
        
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
        bs.write(action_text);

        server->get_player()->send_packet(bs.get_data(), 0);
        
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    send_console(server->get_player(), "`9WL storage spam completed!");
    spdlog::info("WorldLockStorageCommand: Spam completed");
}


core::Core* EventCommand::s_core = nullptr;

EventCommand::EventCommand() : CommandBase(
    {"event"},
    {"<event_name>"},
    "Activate special events (valentines, easter, halloween, etc)",
    0
) {}

std::unique_ptr<CommandBase> EventCommand::clone() const {
    return std::make_unique<EventCommand>(*this);
}

void EventCommand::set_core(core::Core* core) {
    s_core = core;
}

void EventCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("EventCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("EventCommand: No server player!");
        return;
    }

    
    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /event <event_name>");
        send_console(server->get_player(), "`9Available events:");
        send_console(server->get_player(), "`b- valentines `0(Valentine's Day)");
        send_console(server->get_player(), "`b- easter `0(Easter Eggs)");
        send_console(server->get_player(), "`b- halloween `0(Halloween)");
        send_console(server->get_player(), "`b- stpatrick `0(St. Patrick's Day)");
        send_console(server->get_player(), "`b- cinco `0(Cinco de Mayo)");
        send_console(server->get_player(), "`b- summer `0(Summerfest)");
        send_console(server->get_player(), "`b- lunar `0(Lunar New Year)");
        return;
    }

    std::string event_name = args[1];
    std::transform(event_name.begin(), event_name.end(), event_name.begin(), ::tolower);

    
    std::string event_json;
    std::string event_display_name;

    if (event_name == "valentines" || event_name == "valentine") {
        event_json = R"([{"active":true,"buttonAction":"ShowValentinesQuestDialog","buttonTemplate":"EventButtonWithCounter","counter":0,"counterMax":100,"name":"ValentinesButton","order":50,"rcssClass":"valentines_day","text":""}])";
        event_display_name = "Valentine's Day";
    } else if (event_name == "easter") {
        event_json = R"([{"active":true,"buttonAction":"showegseeventui","buttonTemplate":"EventButtonWithCounter","counter":0,"counterMax":20,"name":"EasterButton","order":50,"rcssClass":"easter_event","text":""}])";
        event_display_name = "Easter";
    } else if (event_name == "halloween") {
        event_json = R"([{"active":true,"buttonAction":"claimprogressbar","buttonTemplate":"EventButtonWithCounter","counter":0,"counterMax":15,"name":"HalloweenButton","order":50,"rcssClass":"halloween","text":""}])";
        event_display_name = "Halloween";
    } else if (event_name == "stpatrick" || event_name == "patrick") {
        event_json = R"([{"active":true,"buttonAction":"openStPatrickPiggyBank","buttonTemplate":"BaseEventButton","name":"StPatrickPBButton","order":50,"rcssClass":"st_patrick_event","text":""}])";
        event_display_name = "St. Patrick's Day";
    } else if (event_name == "cinco") {
        event_json = R"([{"active":true,"buttonAction":"dailyrewardmenu","buttonTemplate":"BaseEventButton","counter":0,"counterMax":1,"name":"CincoPinataButton","order":50,"rcssClass":"cinco_pinata_event","text":""}])";
        event_display_name = "Cinco de Mayo";
    } else if (event_name == "summer" || event_name == "summerfest") {
        event_json = R"([{"active":true,"buttonAction":"claimprogressbar","buttonTemplate":"EventButtonWithCounter","counter":0,"counterMax":0,"name":"SummerfestButton","order":50,"rcssClass":"summerfest","text":""}])";
        event_display_name = "Summerfest";
    } else if (event_name == "lunar" || event_name == "lny") {
        event_json = R"([{"active":true,"buttonAction":"openLnySparksPopup","buttonTemplate":"BaseEventButton","counter":0,"counterMax":5,"name":"LnyButton","order":50,"rcssClass":"cny","text":""}])";
        event_display_name = "Lunar New Year";
    } else {
        send_console(server->get_player(), "`4Unknown event! Use /event for list of available events.");
        return;
    }

    
    std::string text_format = "eventButtons|" + event_json;
    
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(text_format);

    server->get_player()->send_packet(bs.get_data(), 0);

    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Event sent: `b{}`9!", event_display_name);
    send_console(server->get_player(), msg);
    send_console(server->get_player(), "`6If nothing happens, events may be server-side only.");
    
    spdlog::info("Event packet sent: {} ({}), format: {}", event_display_name, event_name, text_format.substr(0, 100));
}


MstateCommand::MstateCommand() : CommandBase(
    {"mstate"},
    {},
    "Toggle mod state (long punch, zoom, extended range)",
    0
) {}

std::unique_ptr<CommandBase> MstateCommand::clone() const {
    return std::make_unique<MstateCommand>(*this);
}

void MstateCommand::set_core(core::Core* core) {
    s_core = core;
}

void MstateCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("MstateCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("MstateCommand: No server player!");
        return;
    }

    
    s_mstate_enabled = !s_mstate_enabled;
    
    
    s_core->get_config().set("player.mstate_enabled", s_mstate_enabled);
    s_core->get_config().save();
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Mod State (Long Punch): `b{}\n`6Rejoin world to apply!\n`4Warning: May be detected by server!", 
                                   s_mstate_enabled ? "ON" : "OFF");
    send_console(server->get_player(), msg);
    
    spdlog::info("Mod state toggled: {} (rejoin to apply)", s_mstate_enabled ? "ON" : "OFF");
}

bool MstateCommand::is_mstate_enabled() {
    return s_mstate_enabled;
}


BetaCommand::BetaCommand() : CommandBase(
    {"betatest", "beta"},
    {},
    "Toggle Beta Tester mode",
    0
) {}

std::unique_ptr<CommandBase> BetaCommand::clone() const {
    return std::make_unique<BetaCommand>(*this);
}

void BetaCommand::set_core(core::Core* core) {
    s_core = core;
}

void BetaCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("BetaCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("BetaCommand: No server player!");
        return;
    }

    
    s_beta_enabled = !s_beta_enabled;
    
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        
        packet::Variant var{};
        var.add("OnSetBetaMode");
        var.add(s_beta_enabled ? 1 : 0);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;  
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Beta Mode: `b{}", 
                                       s_beta_enabled ? "ON" : "OFF");
        send_console(server->get_player(), msg);
        spdlog::info("Beta mode toggled to {} with OnSetBetaMode variant (netID: {})", s_beta_enabled ? "ON" : "OFF", local_player.netID);
    } else {
        std::string msg = "`0[ `bVinProxy `0] `4Cannot toggle beta - not spawned in world yet!";
        send_console(server->get_player(), msg);
        spdlog::warn("Beta mode toggle failed - player not tracked (netID 0)");
    }
}

bool BetaCommand::is_beta_enabled() {
    return s_beta_enabled;
}


core::Core* InitWorldCommand::s_core = nullptr;

InitWorldCommand::InitWorldCommand() : CommandBase(
    {"initworld", "initnew"},
    {},
    "Reinitialize world state with OnInitNewWorld",
    0
) {}

void InitWorldCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> InitWorldCommand::clone() const {
    return std::make_unique<InitWorldCommand>(*this);
}

void InitWorldCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("InitWorldCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("InitWorldCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        
        packet::Variant var{};
        var.add("OnInitNewWorld");

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = "`0[ `bVinProxy `0] `9Sent OnInitNewWorld (no visible effect expected)";
        send_console(server->get_player(), msg);
        spdlog::info("Sent OnInitNewWorld variant to netID: {}", local_player.netID);
    } else {
        std::string msg = "`0[ `bVinProxy `0] `4Cannot init world - not spawned yet!";
        send_console(server->get_player(), msg);
        spdlog::warn("InitWorld failed - player not tracked (netID 0)");
    }
}


core::Core* GemsCommand::s_core = nullptr;

GemsCommand::GemsCommand() : CommandBase(
    {"gems"},
    {},
    "Attempt to set gems with OnGemsCountChange",
    1
) {}

void GemsCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> GemsCommand::clone() const {
    return std::make_unique<GemsCommand>(*this);
}

void GemsCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("GemsCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("GemsCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /gems <amount>");
        return;
    }

    int amount = 0;
    try {
        amount = std::stoi(args[1]);
    } catch (...) {
        send_console(server->get_player(), "`4Invalid amount!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        
        packet::Variant var{};
        var.add("OnGemsCountChange");
        var.add(amount);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Sent OnGemsCountChange: `b{}gems `4(server-validated!)", amount);
        send_console(server->get_player(), msg);
        spdlog::info("Sent OnGemsCountChange {} to netID: {}", amount, local_player.netID);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* BuxCommand::s_core = nullptr;

BuxCommand::BuxCommand() : CommandBase(
    {"bux", "growtokens"},
    {},
    "Attempt to set Growtokens with OnSetBux",
    1
) {}

void BuxCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> BuxCommand::clone() const {
    return std::make_unique<BuxCommand>(*this);
}

void BuxCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("BuxCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("BuxCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /bux <amount>");
        return;
    }

    int amount = 0;
    try {
        amount = std::stoi(args[1]);
    } catch (...) {
        send_console(server->get_player(), "`4Invalid amount!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        
        packet::Variant var{};
        var.add("OnSetBux");
        var.add(amount);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Sent OnSetBux: `b{}GT `4(server-validated!)", amount);
        send_console(server->get_player(), msg);
        spdlog::info("Sent OnSetBux {} to netID: {}", amount, local_player.netID);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* BubbleCommand::s_core = nullptr;

BubbleCommand::BubbleCommand() : CommandBase(
    {"bubble", "talk"},
    {},
    "Show talk bubble with OnTalkBubble",
    1
) {}

void BubbleCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> BubbleCommand::clone() const {
    return std::make_unique<BubbleCommand>(*this);
}

void BubbleCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("BubbleCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("BubbleCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /bubble <text>");
        return;
    }

    
    std::string text;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) text += " ";
        text += args[i];
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        packet::Variant var{};
        var.add("OnTalkBubble");
        var.add(local_player.netID);
        var.add(text);

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

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Talk bubble: `b{}", text);
        send_console(server->get_player(), msg);
        spdlog::info("Sent OnTalkBubble: {}", text);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* OverlayCommand::s_core = nullptr;

OverlayCommand::OverlayCommand() : CommandBase(
    {"overlay", "textoverlay"},
    {},
    "Show text overlay with OnTextOverlay",
    1
) {}

void OverlayCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> OverlayCommand::clone() const {
    return std::make_unique<OverlayCommand>(*this);
}

void OverlayCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("OverlayCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("OverlayCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /overlay <text>");
        return;
    }

    std::string text;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) text += " ";
        text += args[i];
    }

    packet::Variant var{};
    var.add("OnTextOverlay");
    var.add(text);

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

    server->get_player()->send_packet(bs.get_data(), 0);
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Text overlay: `b{}", text);
    send_console(server->get_player(), msg);
    spdlog::info("Sent OnTextOverlay: {}", text);
}


core::Core* ZoomCommand::s_core = nullptr;

ZoomCommand::ZoomCommand() : CommandBase(
    {"zoom"},
    {},
    "Control camera zoom with OnZoomCamera",
    1
) {}

void ZoomCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> ZoomCommand::clone() const {
    return std::make_unique<ZoomCommand>(*this);
}

void ZoomCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("ZoomCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("ZoomCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /zoom <level>");
        return;
    }

    float zoom = 1.0f;
    try {
        zoom = std::stof(args[1]);
    } catch (...) {
        send_console(server->get_player(), "`4Invalid zoom value!");
        return;
    }

    packet::Variant var{};
    var.add("OnZoomCamera");
    var.add(zoom);

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

    server->get_player()->send_packet(bs.get_data(), 0);
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Camera zoom: `b{}", zoom);
    send_console(server->get_player(), msg);
    spdlog::info("Sent OnZoomCamera: {}", zoom);
}


core::Core* RainbowCommand::s_core = nullptr;
bool RainbowCommand::s_rainbow_enabled = false;

RainbowCommand::RainbowCommand() : CommandBase(
    {"rainbow", "pure"},
    {},
    "Toggle rainbow mode (OnChangePureBeingMode)",
    0
) {}

void RainbowCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> RainbowCommand::clone() const {
    return std::make_unique<RainbowCommand>(*this);
}

void RainbowCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    s_rainbow_enabled = !s_rainbow_enabled;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        packet::Variant var{};
        var.add("OnChangePureBeingMode");
        var.add(s_rainbow_enabled ? 1 : 0);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Rainbow Mode: `b{}", s_rainbow_enabled ? "ON" : "OFF");
        send_console(server->get_player(), msg);
        spdlog::info("Rainbow mode: {}", s_rainbow_enabled);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* InfinityCommand::s_core = nullptr;

InfinityCommand::InfinityCommand() : CommandBase(
    {"infinity"},
    {},
    "Activate Infinity effects (crown/aura/weapon)",
    1
) {}

void InfinityCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> InfinityCommand::clone() const {
    return std::make_unique<InfinityCommand>(*this);
}

void InfinityCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /infinity <crown/aura/weapon>");
        return;
    }

    std::string type = args[1];
    std::string variant_name;
    
    if (type == "crown") variant_name = "OnInfinityCrown";
    else if (type == "aura") variant_name = "OnInfinityAura";
    else if (type == "weapon") variant_name = "OnInfinityWeapon";
    else {
        send_console(server->get_player(), "`4Invalid type! Use: crown, aura, or weapon");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        packet::Variant var{};
        var.add(variant_name);
        var.add(1);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Infinity {}: `bACTIVATED", type);
        send_console(server->get_player(), msg);
        spdlog::info("Sent {}", variant_name);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* DragonCommand::s_core = nullptr;
bool DragonCommand::s_dragon_enabled = false;

DragonCommand::DragonCommand() : CommandBase(
    {"dragon"},
    {},
    "Toggle Daylight Dragon effect",
    0
) {}

void DragonCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> DragonCommand::clone() const {
    return std::make_unique<DragonCommand>(*this);
}

void DragonCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    s_dragon_enabled = !s_dragon_enabled;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        packet::Variant var{};
        var.add("OnDaylightDragon");
        var.add(s_dragon_enabled ? 1 : 0);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = local_player.netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Daylight Dragon: `b{}", s_dragon_enabled ? "ON" : "OFF");
        send_console(server->get_player(), msg);
        spdlog::info("Dragon mode: {}", s_dragon_enabled);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* RiftWingsCommand::s_core = nullptr;
bool RiftWingsCommand::s_riftwings_enabled = false;

RiftWingsCommand::RiftWingsCommand() : CommandBase(
    {"riftwings", "rift"},
    {},
    "Toggle Rift Wings/Cape effects",
    0
) {}

void RiftWingsCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> RiftWingsCommand::clone() const {
    return std::make_unique<RiftWingsCommand>(*this);
}

void RiftWingsCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    s_riftwings_enabled = !s_riftwings_enabled;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID != 0) {
        
        packet::Variant var_wings{};
        var_wings.add("OnRiftWings");
        var_wings.add(s_riftwings_enabled ? 1 : 0);

        packet::Variant var_cape{};
        var_cape.add("OnRiftCape");
        var_cape.add(s_riftwings_enabled ? 1 : 0);

        
        std::vector<std::byte> ext_data_wings = var_wings.serialize();
        packet::GameUpdatePacket pkt_wings{};
        pkt_wings.type = packet::PACKET_CALL_FUNCTION;
        pkt_wings.net_id = local_player.netID;
        pkt_wings.flags.extended = 1;
        pkt_wings.data_size = static_cast<uint32_t>(ext_data_wings.size());

        ByteStream<std::uint16_t> bs_wings{};
        bs_wings.write(packet::NET_MESSAGE_GAME_PACKET);
        bs_wings.write(pkt_wings);
        bs_wings.write_data(ext_data_wings.data(), ext_data_wings.size());

        server->get_player()->send_packet(bs_wings.get_data(), 0);

        
        std::vector<std::byte> ext_data_cape = var_cape.serialize();
        packet::GameUpdatePacket pkt_cape{};
        pkt_cape.type = packet::PACKET_CALL_FUNCTION;
        pkt_cape.net_id = local_player.netID;
        pkt_cape.flags.extended = 1;
        pkt_cape.data_size = static_cast<uint32_t>(ext_data_cape.size());

        ByteStream<std::uint16_t> bs_cape{};
        bs_cape.write(packet::NET_MESSAGE_GAME_PACKET);
        bs_cape.write(pkt_cape);
        bs_cape.write_data(ext_data_cape.data(), ext_data_cape.size());

        server->get_player()->send_packet(bs_cape.get_data(), 0);
        
        std::string msg = fmt::format("`0[ `bVinProxy `0] `9Rift Wings/Cape: `b{}", s_riftwings_enabled ? "ON" : "OFF");
        send_console(server->get_player(), msg);
        spdlog::info("Rift wings: {}", s_riftwings_enabled);
    } else {
        send_console(server->get_player(), "`4Not spawned yet!");
    }
}


core::Core* SetPosCommand::s_core = nullptr;

void SetPosCommand::set_position(player::Player* player, uint32_t netid, float x, float y) {
    if (!player) return;

    packet::Variant var{};
    var.add("OnSetPos");
    var.add(glm::vec2(x, y));

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = netid;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    player->send_packet(bs.get_data(), 0);
    
    spdlog::info("OnSetPos packet sent (netid: {}) - x: {}, y: {}", netid, x, y);
}

SetPosCommand::SetPosCommand() : CommandBase(
    {"setpos"},
    {},
    "Teleport to position (Usage: /setpos <x> <y>)",
    2
) {}

void SetPosCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> SetPosCommand::clone() const {
    return std::make_unique<SetPosCommand>(*this);
}

void SetPosCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("SetPosCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("SetPosCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (local_player.netID == 0) {
        send_console(server->get_player(), "`4Error: Player not tracked yet. Try after spawning.");
        return;
    }

    
    if (args.size() < 3) {
        send_console(server->get_player(), "`4Usage: /setpos <x> <y>");
        send_console(server->get_player(), "`9Example: /setpos 50 30");
        return;
    }

    float x_pos = 0.0f;
    float y_pos = 0.0f;

    try {
        x_pos = std::stof(args[1]);
        y_pos = std::stof(args[2]);
    } catch (const std::exception& e) {
        send_console(server->get_player(), "`4Invalid coordinates! Use numbers.");
        return;
    }

    
    set_position(server->get_player(), local_player.netID, x_pos, y_pos);

    std::string msg = fmt::format("`0[ `bVinProxy `0] `9OnSetPos sent: `bX={}, Y={}", x_pos, y_pos);
    send_console(server->get_player(), msg);
    send_console(server->get_player(), "`6Note: Server likely validates this (may not work)");
    
    spdlog::info("SetPos command executed: x={}, y={}", x_pos, y_pos);
}


core::Core* ConsoleMessageCommand::s_core = nullptr;

ConsoleMessageCommand::ConsoleMessageCommand() : CommandBase(
    {"cmsg"},
    {},
    "Send console message (Usage: /cmsg <text>)",
    1
) {}

void ConsoleMessageCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> ConsoleMessageCommand::clone() const {
    return std::make_unique<ConsoleMessageCommand>(*this);
}

void ConsoleMessageCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("ConsoleMessageCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("ConsoleMessageCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /cmsg <message>");
        return;
    }

    
    std::string msg;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) msg += " ";
        msg += args[i];
    }

    packet::Variant var{};
    var.add("OnConsoleMessage");
    var.add(msg);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_player.netID;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    server->get_player()->send_packet(bs.get_data(), 0);
    
    spdlog::info("OnConsoleMessage sent: {}", msg);
    send_console(server->get_player(), "`9OnConsoleMessage packet sent");
}


core::Core* DialogCommand::s_core = nullptr;

DialogCommand::DialogCommand() : CommandBase(
    {"dialog"},
    {},
    "Open custom dialog (Usage: /dialog <text>)",
    1
) {}

void DialogCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> DialogCommand::clone() const {
    return std::make_unique<DialogCommand>(*this);
}

void DialogCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("DialogCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("DialogCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /dialog <dialog_str>");
        send_console(server->get_player(), "`9Example: /dialog add_label_with_icon|big|Test Dialog|left|242|");
        return;
    }

    std::string dialog_str;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) dialog_str += " ";
        dialog_str += args[i];
    }

    packet::Variant var{};
    var.add("OnDialogRequest");
    var.add(dialog_str);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_player.netID;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    server->get_player()->send_packet(bs.get_data(), 0);
    
    spdlog::info("OnDialogRequest sent: {}", dialog_str);
    send_console(server->get_player(), "`9OnDialogRequest packet sent");
}


core::Core* TradeCommand::s_core = nullptr;

TradeCommand::TradeCommand() : CommandBase(
    {"trade"},
    {},
    "Trade commands (Usage: /trade <start|end> [netid])",
    1
) {}

void TradeCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> TradeCommand::clone() const {
    return std::make_unique<TradeCommand>(*this);
}

void TradeCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("TradeCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("TradeCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /trade <start|end> [netid]");
        send_console(server->get_player(), "`9/trade start <netid> - Force start trade");
        send_console(server->get_player(), "`9/trade end - Force end trade");
        return;
    }

    std::string action = args[1];
    packet::Variant var{};

    if (action == "start") {
        if (args.size() < 3) {
            send_console(server->get_player(), "`4Usage: /trade start <netid>");
            return;
        }

        uint32_t target_netid = 0;
        try {
            target_netid = static_cast<uint32_t>(std::stoi(args[2]));
        } catch (...) {
            send_console(server->get_player(), "`4Invalid netID!");
            return;
        }

        var.add("OnStartTrade");
        var.add(static_cast<int>(target_netid));
        send_console(server->get_player(), fmt::format("`9OnStartTrade sent to netID: `b{}", target_netid));
        
    } else if (action == "end") {
        var.add("OnForceTradeEnd");
        send_console(server->get_player(), "`9OnForceTradeEnd sent");
        
    } else {
        send_console(server->get_player(), "`4Unknown action! Use 'start' or 'end'");
        return;
    }

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_player.netID;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    server->get_player()->send_packet(bs.get_data(), 0);
}


core::Core* ActionCommand::s_core = nullptr;

ActionCommand::ActionCommand() : CommandBase(
    {"action"},
    {},
    "Trigger action (Usage: /action <type>)",
    1
) {}

void ActionCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> ActionCommand::clone() const {
    return std::make_unique<ActionCommand>(*this);
}

void ActionCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("ActionCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("ActionCommand: No server player!");
        return;
    }

    auto& tracker = utils::PlayerTracker::get_instance();
    auto local_player = tracker.get_local_player();

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /action <0-10>");
        send_console(server->get_player(), "`9Actions: 0=Idle, 1=Walk, 2=Jump, 3=Punch, 4=Use, etc.");
        return;
    }

    int action_type = 0;
    try {
        action_type = std::stoi(args[1]);
    } catch (...) {
        send_console(server->get_player(), "`4Invalid action type!");
        return;
    }

    packet::Variant var{};
    var.add("OnAction");
    var.add(action_type);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = local_player.netID;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());

    server->get_player()->send_packet(bs.get_data(), 0);
    
    send_console(server->get_player(), fmt::format("`9OnAction sent: type `b{}", action_type));
}


AntiGravityCommand::AntiGravityCommand() : CommandBase(
    {"antigravity", "ag"},
    {},
    "Place + activate antigravity using raw packets (item_id fixed: 4992)",
    0
) {}

void AntiGravityCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> AntiGravityCommand::clone() const {
    return std::make_unique<AntiGravityCommand>(*this);
}

void AntiGravityCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client) {
        spdlog::error("AntiGravityCommand: No core/client!");
        return;
    }

    auto* server = s_core->get_server();
    auto* to_game_client = s_core->get_client();
    auto* to_game_player = (to_game_client ? to_game_client->get_player() : nullptr);
    auto* to_server_player = (server ? server->get_player() : nullptr);
    if (!to_game_player) {
        spdlog::error("AntiGravityCommand: No game-server route player!");
        return;
    }

    
    constexpr int32_t item_id = 4992;
    constexpr int32_t tile_x = 99;
    constexpr int32_t tile_y = 56;

    if (args.size() != 1) {
        if (to_server_player) {
            send_console(to_server_player, "`4Usage: /antigravity");
        }
        return;
    }

    auto send_raw_client = [&](uint8_t pkt_type, int32_t int_data, int32_t x, int32_t y) {
        MoriStatePacket pkt{};
        pkt.type = pkt_type;
        pkt.net_id = 0; 
        pkt.value = static_cast<uint32_t>(int_data);
        pkt.vector_x = 0.0f; 
        pkt.vector_y = 0.0f;
        pkt.int_x = x;
        pkt.int_y = y;
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        to_game_player->send_packet(bs.get_data(), 0);
        if (to_server_player) {
            to_server_player->send_packet(bs.get_data(), 0);
        }
    };

    
    send_raw_client(static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST), item_id, tile_x, tile_y);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    send_raw_client(static_cast<uint8_t>(packet::PACKET_TILE_APPLY_DAMAGE), 0, tile_x, tile_y);

    if (to_server_player) {
        send_overlay(to_server_player, "Antigravity Enabled");
    }
}


AntiPunchCommand::AntiPunchCommand() : CommandBase(
    {"antipunch", "ap"},
    {},
    "Place + activate antipunch visual jammer using raw packets (item_id fixed: 1276)",
    0
) {}

void AntiPunchCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> AntiPunchCommand::clone() const {
    return std::make_unique<AntiPunchCommand>(*this);
}

void AntiPunchCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client) {
        spdlog::error("AntiPunchCommand: No core/client!");
        return;
    }

    auto* server = s_core->get_server();
    auto* to_game_client = s_core->get_client();
    auto* to_game_player = (to_game_client ? to_game_client->get_player() : nullptr);
    auto* to_server_player = (server ? server->get_player() : nullptr);
    if (!to_game_player) {
        spdlog::error("AntiPunchCommand: No game-server route player!");
        return;
    }

    
    constexpr int32_t item_id = 1276;
    constexpr int32_t tile_x = 99;
    constexpr int32_t tile_y = 57;

    if (args.size() != 1) {
        if (to_server_player) {
            send_console(to_server_player, "`4Usage: /antipunch");
        }
        return;
    }

    auto send_raw_client = [&](uint8_t pkt_type, int32_t int_data, int32_t x, int32_t y) {
        MoriStatePacket pkt{};
        pkt.type = pkt_type;
        pkt.net_id = 0;
        pkt.value = static_cast<uint32_t>(int_data);
        pkt.vector_x = 0.0f;
        pkt.vector_y = 0.0f;
        pkt.int_x = x;
        pkt.int_y = y;
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        to_game_player->send_packet(bs.get_data(), 0);
        if (to_server_player) {
            to_server_player->send_packet(bs.get_data(), 0);
        }
    };

    send_raw_client(static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST), item_id, tile_x, tile_y);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    send_raw_client(static_cast<uint8_t>(packet::PACKET_TILE_APPLY_DAMAGE), 0, tile_x, tile_y);

    if (to_server_player) {
        send_overlay(to_server_player, "Antipunch Enabled");
    }
}


core::Core* TitleIconCommand::s_core = nullptr;

TitleIconCommand::TitleIconCommand() : CommandBase(
    {"titleicon", "ticon"},
    {},
    "Set title icon (Usage: /titleicon <0-200>)",
    1
) {}

void TitleIconCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> TitleIconCommand::clone() const {
    return std::make_unique<TitleIconCommand>(*this);
}

void TitleIconCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("TitleIconCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("TitleIconCommand: No server player!");
        return;
    }

    if (args.size() < 2) {
        int current = s_core->get_config().get<int>("player.title_icon");
        send_console(server->get_player(), fmt::format("`9Current titleIcon: `b{}", current));
        send_console(server->get_player(), "`4Usage: /titleicon <0-200>");
        send_console(server->get_player(), "`90=None, try values 1-200 to test icons");
        send_console(server->get_player(), "`6Rejoin world to apply changes");
        return;
    }

    int icon_id = 0;
    try {
        icon_id = std::stoi(args[1]);
        if (icon_id < 0 || icon_id > 200) {
            send_console(server->get_player(), "`4Icon ID must be 0-200!");
            return;
        }
    } catch (...) {
        send_console(server->get_player(), "`4Invalid icon ID!");
        return;
    }

    s_core->get_config().set<int>("player.title_icon", icon_id);
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Title icon set to: `b{}", icon_id);
    send_console(server->get_player(), msg);
    send_console(server->get_player(), "`6Rejoin the world to see changes!");
    
    if (icon_id == 0) {
        send_console(server->get_player(), "`9Title icon disabled");
    }
    
    spdlog::info("Title icon set to: {}", icon_id);
}


core::Core* SetWeatherCommand::s_core = nullptr;

SetWeatherCommand::SetWeatherCommand() : CommandBase(
    {"setweather"},
    {},
    "Set world weather (0=none, 1-30=types)",
    1
) {}

void SetWeatherCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> SetWeatherCommand::clone() const {
    return std::make_unique<SetWeatherCommand>(*this);
}

void SetWeatherCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /setweather <type 0-30>");
        return;
    }

    int weather = 0;
    try {
        weather = std::stoi(args[1]);
    } catch (...) {
        send_console(server->get_player(), "`4Invalid weather type!");
        return;
    }

    packet::Variant var{};
    var.add("OnSetCurrentWeather");
    var.add(weather);

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

    server->get_player()->send_packet(bs.get_data(), 0);
    
    std::string msg = fmt::format("`0[ `bVinProxy `0] `9Weather set to: `b{}", weather);
    send_console(server->get_player(), msg);
    spdlog::info("Set weather: {}", weather);
}


core::Core* VisionCommand::s_core = nullptr;
bool VisionCommand::s_vision_enabled = false;

VisionCommand::VisionCommand() : CommandBase(
    {"vision"},
    {},
    "Apply visual background vision (item_id 1156) on tiles x:0-100, y:0-60",
    0
) {}

void VisionCommand::set_core(core::Core* core) {
    s_core = core;
}

bool VisionCommand::is_vision_enabled() {
    return s_vision_enabled;
}

std::unique_ptr<CommandBase> VisionCommand::clone() const {
    return std::make_unique<VisionCommand>(*this);
}

void VisionCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("VisionCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    auto* to_game_client = s_core->get_client();
    auto* to_game_player = (to_game_client ? to_game_client->get_player() : nullptr);
    auto* to_server_player = (server ? server->get_player() : nullptr);
    if (!to_game_player) {
        spdlog::error("VisionCommand: No game-server route player!");
        return;
    }

    if (args.size() != 1) {
        if (to_server_player) {
            send_console(to_server_player, "`4Usage: /vision");
        }
        return;
    }

    constexpr int32_t bg_item_id = 1156;
    auto& world_mgr = utils::WorldManager::get_instance();
    if (!world_mgr.has_world()) {
        if (to_server_player) {
            send_console(to_server_player, "`4Vision: no parsed world data. Re-enter world first.");
        }
        return;
    }

    const auto& tiles = world_mgr.get_tiles();
    if (tiles.empty()) {
        if (to_server_player) {
            send_console(to_server_player, "`4Vision: tile list is empty.");
        }
        return;
    }

    uint32_t width = world_mgr.get_world_width();
    if (width == 0) {
        width = 100;
    }

    auto send_raw_client = [&](uint8_t pkt_type, int32_t int_data, int32_t x, int32_t y) {
        MoriStatePacket pkt{};
        pkt.type = pkt_type;
        pkt.net_id = 0;
        pkt.value = static_cast<uint32_t>(int_data);
        pkt.vector_x = 0.0f;
        pkt.vector_y = 0.0f;
        pkt.int_x = x;
        pkt.int_y = y;
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        to_game_player->send_packet(bs.get_data(), 0);
        if (to_server_player) {
            to_server_player->send_packet(bs.get_data(), 0);
        }
    };

    auto is_target_tile = [](uint16_t item_id) {
        return item_id == 3556 || item_id == 7378 || item_id == 7458;
    };

    int total = 0;
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        if (!is_target_tile(tile.Fg) && !is_target_tile(tile.Bg)) {
            continue;
        }

        const int32_t x = static_cast<int32_t>(i % width);
        const int32_t y = static_cast<int32_t>(i / width);
        send_raw_client(static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST), bg_item_id, x, y);
        ++total;

        if ((total % 50) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    s_vision_enabled = true;
    s_core->get_config().set<bool>("player.vision_enabled", true);

    if (to_server_player) {
        send_overlay(to_server_player, fmt::format("Vision Enabled ({} replaced)", total));
    }
    spdlog::info("VisionCommand: replaced {} target tiles with item {}", total, bg_item_id);
}


core::Core* SpawnBGLsCommand::s_core = nullptr;

SpawnBGLsCommand::SpawnBGLsCommand() : CommandBase(
    {"spawnbgls"},
    {},
    "Spawn Blue Gem Locks (item 7188) as live objects across the entire world",
    0
) {}

std::unique_ptr<CommandBase> SpawnBGLsCommand::clone() const {
    return std::make_unique<SpawnBGLsCommand>(*this);
}

void SpawnBGLsCommand::set_core(core::Core* core) {
    s_core = core;
}

void SpawnBGLsCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("SpawnBGLsCommand: missing core/client");
        return;
    }
    auto* server = s_core->get_server();
    auto* to_game_client = s_core->get_client();
    auto* to_game_player = (to_game_client ? to_game_client->get_player() : nullptr);
    auto* to_server_player = (server ? server->get_player() : nullptr);
    if (!to_game_player) {
        spdlog::error("SpawnBGLsCommand: no game-server player route");
        return;
    }

    auto& world_mgr = utils::WorldManager::get_instance();
    if (!world_mgr.has_world()) {
        if (to_server_player) send_console(to_server_player, "`4SpawnBGLs: no world data");
        return;
    }

    const auto& tiles = world_mgr.get_tiles();
    if (tiles.empty()) {
        if (to_server_player) send_console(to_server_player, "`4SpawnBGLs: tile list empty");
        return;
    }

    uint32_t width = world_mgr.get_world_width();
    if (width == 0) width = 100;

    constexpr int32_t item_id = 7188;

    auto send_drop = [&](int32_t x, int32_t y) {
        packet::TankUpdatePacket t{};
        t.type = 0;
        t.net_id = -1;
        t.target_net_id = 0;
        t.flags = 0;
        t.float_var = 1.0f; 
        t.int_data = item_id;
        t.vec_x = static_cast<float>(x);
        t.vec_y = static_cast<float>(y);
        
        packet::GameUpdatePacket pkt{};
        
        pkt.type = packet::PACKET_ITEM_CHANGE_OBJECT;
        pkt.net_id = 0;
        pkt.flags.extended = 1;
        pkt.data_size = sizeof(t);

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(reinterpret_cast<std::byte*>(&t), sizeof(t));

        to_game_player->send_packet(bs.get_data(), 0);
        if (to_server_player) to_server_player->send_packet(bs.get_data(), 0);
    };

    int count = 0;
    for (size_t i = 0; i < tiles.size(); ++i) {
        const int32_t x = static_cast<int32_t>(i % width);
        const int32_t y = static_cast<int32_t>(i / width);
        send_drop(x, y);
        ++count;
        if ((count % 100) == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (to_server_player) send_overlay(to_server_player, fmt::format("Spawned {} BGLs", count));
    spdlog::info("SpawnBGLsCommand: spawned {} items", count);
}


core::Core* ChestCommand::s_core = nullptr;
core::Core* RemaCommand::s_core = nullptr;

ChestCommand::ChestCommand() : CommandBase(
    {"chest"},
    {},
    "Replace target chest tiles (596/1530/5618/14752) with Dragon Gate (598)",
    0
) {}

void ChestCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> ChestCommand::clone() const {
    return std::make_unique<ChestCommand>(*this);
}

void ChestCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("ChestCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    auto* to_game_client = s_core->get_client();
    auto* to_game_player = (to_game_client ? to_game_client->get_player() : nullptr);
    auto* to_server_player = (server ? server->get_player() : nullptr);
    if (!to_game_player) {
        spdlog::error("ChestCommand: No game-server route player!");
        return;
    }

    if (args.size() != 1) {
        if (to_server_player) {
            send_console(to_server_player, "`4Usage: /chest");
        }
        return;
    }

    auto& world_mgr = utils::WorldManager::get_instance();
    if (!world_mgr.has_world()) {
        if (to_server_player) {
            send_console(to_server_player, "`4Chest: no parsed world data. Re-enter world first.");
        }
        return;
    }

    const auto& tiles = world_mgr.get_tiles();
    if (tiles.empty()) {
        if (to_server_player) {
            send_console(to_server_player, "`4Chest: tile list is empty.");
        }
        return;
    }

    uint32_t width = world_mgr.get_world_width();
    if (width == 0) {
        width = 100;
    }

    constexpr int32_t replacement_item_id = 598;
    auto is_target_tile = [](uint16_t item_id) {
        return item_id == 596 || item_id == 1530 || item_id == 5618 || item_id == 14752;
    };

    auto send_raw_client = [&](uint8_t pkt_type, int32_t int_data, int32_t x, int32_t y) {
        MoriStatePacket pkt{};
        pkt.type = pkt_type;
        pkt.net_id = 0;
        pkt.value = static_cast<uint32_t>(int_data);
        pkt.vector_x = 0.0f;
        pkt.vector_y = 0.0f;
        pkt.int_x = x;
        pkt.int_y = y;
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        to_game_player->send_packet(bs.get_data(), 0);
        if (to_server_player) {
            to_server_player->send_packet(bs.get_data(), 0);
        }
    };

    int total = 0;
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        if (!is_target_tile(tile.Fg) && !is_target_tile(tile.Bg)) {
            continue;
        }

        const int32_t x = static_cast<int32_t>(i % width);
        const int32_t y = static_cast<int32_t>(i / width);
        send_raw_client(static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST), replacement_item_id, x, y);
        ++total;

        if ((total % 50) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    if (to_server_player) {
        send_overlay(to_server_player, fmt::format("Chest Replaced ({} tiles)", total));
    }
    spdlog::info("ChestCommand: replaced {} target tiles with item {}", total, replacement_item_id);
}


RemaCommand::RemaCommand() : CommandBase(
    {"rema"},
    {},
    "Remove your access from all locks in world (auto-confirm unaccess dialog)",
    0
) {}

void RemaCommand::set_core(core::Core* core) {
    s_core = core;
}

std::unique_ptr<CommandBase> RemaCommand::clone() const {
    return std::make_unique<RemaCommand>(*this);
}

void RemaCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("RemaCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    auto* to_game_client = s_core->get_client();
    auto* to_game_player = (to_game_client ? to_game_client->get_player() : nullptr);
    if (!server || !server->get_player() || !to_game_player) {
        spdlog::error("RemaCommand: Missing route player(s)");
        return;
    }
    s_core->get_config().set<bool>("features.rema_hide_unaccess_dialog", true);

    
    {
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
        bs.write("action|input\ntext|/unaccesss", false);
        to_game_player->send_packet(bs.get_data(), 0);
    }

    send_console(server->get_player(), "`0[ `bVinProxy `0] `9Unaccess requested, confirming...");

    
    core::Core* core = s_core;
    std::thread([to_game_player, core]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(350));

        for (int i = 0; i < 12; ++i) { 
            
            ByteStream<std::uint16_t> bs1{};
            bs1.write(packet::NET_MESSAGE_GENERIC_TEXT);
            bs1.write("action|dialog_return\ndialog_name|unaccesss", false);
            to_game_player->send_packet(bs1.get_data(), 0);

            
            ByteStream<std::uint16_t> bs2{};
            bs2.write(packet::NET_MESSAGE_GENERIC_TEXT);
            bs2.write("action|dialog_return\ndialog_name|unaccess\nbuttonClicked|yes", false);
            to_game_player->send_packet(bs2.get_data(), 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
        if (core) {
            core->get_config().set<bool>("features.rema_hide_unaccess_dialog", false);
        }
    }).detach();
}



core::Core* SkinCommand::s_core = nullptr;
SkinCommand::SkinCommand() : CommandBase({"skin"}, {"<r>","<g>","<b>"}, "Change skin color. Usage: /skin <r> <g> <b>  or  /skin <0xRRGGBB>", 1) {}
std::unique_ptr<CommandBase> SkinCommand::clone() const { return std::make_unique<SkinCommand>(*this); }
void SkinCommand::set_core(core::Core* core) { s_core = core; }
void SkinCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /skin <r> <g> <b>  or  /skin <0xRRGGBB>");
        return;
    }
    int color = 0;
    try {
        if (args[1].size() > 2 && (args[1][1]=='x'||args[1][1]=='X'))
            color = static_cast<int>(std::stoul(args[1], nullptr, 16));
        else if (args.size() >= 4) {
            int r = std::clamp(std::stoi(args[1]),0,255);
            int g = std::clamp(std::stoi(args[2]),0,255);
            int b = std::clamp(std::stoi(args[3]),0,255);
            color = (r<<16)|(g<<8)|b;
        } else color = std::stoi(args[1]);
    } catch (...) { send_console(server->get_player(), "`4Invalid color!"); return; }
    packet::Variant var{}; var.add("OnChangeSkin"); var.add(lp.netID); var.add(color);
    std::vector<std::byte> ext = var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Skin color: `b0x{:06X}", color));
    spdlog::info("OnChangeSkin: color=0x{:06X} netID={}", color, lp.netID);
}


core::Core* DeathAnimCommand::s_core = nullptr;
DeathAnimCommand::DeathAnimCommand() : CommandBase({"deathanim","da"}, {"<id>"}, "Change death animation (0-30). Usage: /deathanim <id>", 1) {}
std::unique_ptr<CommandBase> DeathAnimCommand::clone() const { return std::make_unique<DeathAnimCommand>(*this); }
void DeathAnimCommand::set_core(core::Core* core) { s_core = core; }
void DeathAnimCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /deathanim <id 0-30>"); return; }
    int id=0; try { id=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    packet::Variant var{}; var.add("OnChangeDeathAnim"); var.add(lp.netID); var.add(id);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Death anim: `b{}", id));
    spdlog::info("OnChangeDeathAnim: id={} netID={}", id, lp.netID);
}


core::Core* RespawnAnimCommand::s_core = nullptr;
RespawnAnimCommand::RespawnAnimCommand() : CommandBase({"respawnanim","ra"}, {"<id>"}, "Change respawn animation (0-30). Usage: /respawnanim <id>", 1) {}
std::unique_ptr<CommandBase> RespawnAnimCommand::clone() const { return std::make_unique<RespawnAnimCommand>(*this); }
void RespawnAnimCommand::set_core(core::Core* core) { s_core = core; }
void RespawnAnimCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /respawnanim <id 0-30>"); return; }
    int id=0; try { id=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    packet::Variant var{}; var.add("OnChangeRespawnAnim"); var.add(lp.netID); var.add(id);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Respawn anim: `b{}", id));
    spdlog::info("OnChangeRespawnAnim: id={} netID={}", id, lp.netID);
}


core::Core* ParticleCommand::s_core = nullptr;
ParticleCommand::ParticleCommand() : CommandBase({"particle","pfx"}, {"<id>","[v2]"}, "Spawn particle effect at your pos. Usage: /particle <id> [v2]", 1) {}
std::unique_ptr<CommandBase> ParticleCommand::clone() const { return std::make_unique<ParticleCommand>(*this); }
void ParticleCommand::set_core(core::Core* core) { s_core = core; }
void ParticleCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /particle <id> [v2]");
        send_console(server->get_player(), "`9Append 'v2' to use OnParticleEffectV2");
        return;
    }
    int id=0; try { id=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    bool v2 = (args.size()>=3 && args[2]=="v2");
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    float px=lp.position.x, py=lp.position.y;
    packet::Variant var{};
    if (v2) { var.add("OnParticleEffectV2"); var.add(id); var.add(px); var.add(py); var.add(0); }
    else     { var.add("OnParticleEffect");   var.add(id); var.add(px); var.add(py); }
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Particle `b{}` `9at ({:.0f},{:.0f}) [{}]", id, px, py, v2?"V2":"V1"));
    spdlog::info("Particle {} {} at ({},{})", id, v2?"V2":"V1", px, py);
}


core::Core* ItemEffectCommand::s_core = nullptr;
ItemEffectCommand::ItemEffectCommand() : CommandBase({"itemfx","ieffect"}, {"<item_id>"}, "Trigger item effect on yourself. Usage: /itemfx <item_id>", 1) {}
std::unique_ptr<CommandBase> ItemEffectCommand::clone() const { return std::make_unique<ItemEffectCommand>(*this); }
void ItemEffectCommand::set_core(core::Core* core) { s_core = core; }
void ItemEffectCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /itemfx <item_id>"); return; }
    int id=0; try { id=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    packet::Variant var{}; var.add("OnItemEffect"); var.add(lp.netID); var.add(id);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9OnItemEffect: item `b{}", id));
    spdlog::info("OnItemEffect: item={} netID={}", id, lp.netID);
}


core::Core* BannerCommand::s_core = nullptr;
BannerCommand::BannerCommand() : CommandBase({"banner","bandolier"}, {"<id>"}, "Set bandolier banner on avatar. Usage: /banner <id 0-50>", 1) {}
std::unique_ptr<CommandBase> BannerCommand::clone() const { return std::make_unique<BannerCommand>(*this); }
void BannerCommand::set_core(core::Core* core) { s_core = core; }
void BannerCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /banner <id 0-50>"); return; }
    int id=0; try { id=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    packet::Variant var{}; var.add("OnBannerBandolier"); var.add(lp.netID); var.add(id);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Banner/Bandolier: `b{}", id));
    spdlog::info("OnBannerBandolier: id={} netID={}", id, lp.netID);
}


core::Core* DisguiseCommand::s_core = nullptr;
DisguiseCommand::DisguiseCommand() : CommandBase({"disguise"}, {"<item_id>"}, "Disguise as an item (0=clear). Usage: /disguise <item_id>", 1) {}
std::unique_ptr<CommandBase> DisguiseCommand::clone() const { return std::make_unique<DisguiseCommand>(*this); }
void DisguiseCommand::set_core(core::Core* core) { s_core = core; }
void DisguiseCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /disguise <item_id>  (0 to clear)"); return; }
    int id=0; try { id=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    packet::Variant var{}; var.add("OnDisguiseChanged"); var.add(lp.netID); var.add(id);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), id==0 ? "`0[ `bVinProxy `0] `9Disguise cleared." : fmt::format("`0[ `bVinProxy `0] `9Disguised as item: `b{}", id));
    spdlog::info("OnDisguiseChanged: item={} netID={}", id, lp.netID);
}


core::Core* PaintballCommand::s_core = nullptr;
PaintballCommand::PaintballCommand() : CommandBase({"paintball","pb"}, {"<netid>","<color>"}, "Paintball a player. Usage: /paintball <netid> <0xRRGGBB>", 2) {}
std::unique_ptr<CommandBase> PaintballCommand::clone() const { return std::make_unique<PaintballCommand>(*this); }
void PaintballCommand::set_core(core::Core* core) { s_core = core; }
void PaintballCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    if (args.size() < 3) {
        send_console(server->get_player(), "`4Usage: /paintball <netid> <0xRRGGBB>");
        send_console(server->get_player(), "`9Use netid=0 for yourself. Example: /paintball 0 0xFF0000");
        return;
    }
    int tgt=0, color=0xFF0000;
    try {
        tgt = std::stoi(args[1]);
        color = (args[2].size()>2&&(args[2][1]=='x'||args[2][1]=='X')) ? static_cast<int>(std::stoul(args[2],nullptr,16)) : std::stoi(args[2]);
    } catch(...) { send_console(server->get_player(),"`4Invalid args!"); return; }
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (tgt==0) tgt=lp.netID;
    packet::Variant var{}; var.add("OnAvatarBePaintBalled"); var.add(tgt); var.add(color);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Paintballed netID `b{}` `9color `b0x{:06X}", tgt, color));
    spdlog::info("OnAvatarBePaintBalled: netID={} color=0x{:06X}", tgt, color);
}


core::Core* BroadcastCommand::s_core = nullptr;
BroadcastCommand::BroadcastCommand() : CommandBase({"broadcast","bc"}, {"<text>"}, "Inject a fake broadcast message locally. Usage: /broadcast <text>", 1) {}
std::unique_ptr<CommandBase> BroadcastCommand::clone() const { return std::make_unique<BroadcastCommand>(*this); }
void BroadcastCommand::set_core(core::Core* core) { s_core = core; }
void BroadcastCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /broadcast <text>"); return; }
    std::string text; for (size_t i=1;i<args.size();++i){if(i>1)text+=" ";text+=args[i];}
    packet::Variant var{}; var.add("OnSDBroadcast"); var.add(text);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Broadcast: `b{}", text));
    spdlog::info("OnSDBroadcast: {}", text);
}


core::Core* RoleSkinCommand::s_core = nullptr;
RoleSkinCommand::RoleSkinCommand() : CommandBase({"roleskin","rs"}, {"<skin_id>","[icon_id]"}, "Change role skin and icon. Usage: /roleskin <skin_id> [icon_id]", 1) {}
std::unique_ptr<CommandBase> RoleSkinCommand::clone() const { return std::make_unique<RoleSkinCommand>(*this); }
void RoleSkinCommand::set_core(core::Core* core) { s_core = core; }
void RoleSkinCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /roleskin <skin_id> [icon_id]"); return; }
    int skin=0,icon=0;
    try { skin=std::stoi(args[1]); if(args.size()>=3)icon=std::stoi(args[2]); } catch(...) { send_console(server->get_player(),"`4Invalid ID!"); return; }
    auto send_var=[&](const char* fn, int v){
        packet::Variant var{}; var.add(fn); var.add(lp.netID); var.add(v);
        std::vector<std::byte> ext=var.serialize();
        packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
        ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
        server->get_player()->send_packet(bs.get_data(),0);
    };
    send_var("OnChangeRoleSkin", skin);
    send_var("OnChangeRoleIcon", icon);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Role skin: `b{}` `9| Icon: `b{}", skin, icon));
    spdlog::info("OnChangeRoleSkin/Icon: skin={} icon={} netID={}", skin, icon, lp.netID);
}


core::Core* LevelUpCommand::s_core = nullptr;
LevelUpCommand::LevelUpCommand() : CommandBase({"levelup","lvlup"}, {"<level>"}, "Trigger level-up effect. Usage: /levelup <level>", 1) {}
std::unique_ptr<CommandBase> LevelUpCommand::clone() const { return std::make_unique<LevelUpCommand>(*this); }
void LevelUpCommand::set_core(core::Core* core) { s_core = core; }
void LevelUpCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    if (lp.netID == 0) { send_console(server->get_player(), "`4Not spawned yet!"); return; }
    if (args.size() < 2) { send_console(server->get_player(), "`4Usage: /levelup <level>"); return; }
    int lvl=0; try { lvl=std::stoi(args[1]); } catch(...) { send_console(server->get_player(),"`4Invalid level!"); return; }
    packet::Variant var{}; var.add("OnPlayerLeveledUp"); var.add(lp.netID); var.add(lvl);
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=-1; pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Level-up effect: `bLevel {}", lvl));
    spdlog::info("OnPlayerLeveledUp: lvl={} netID={}", lvl, lp.netID);
}


core::Core* RawVariantCommand::s_core = nullptr;
RawVariantCommand::RawVariantCommand() : CommandBase({"rawvar","rv"}, {"<OnFnName>","[args]..."}, "Send any variant call. Usage: /rawvar <FnName> [arg1] [arg2]...", 1) {}
std::unique_ptr<CommandBase> RawVariantCommand::clone() const { return std::make_unique<RawVariantCommand>(*this); }
void RawVariantCommand::set_core(core::Core* core) { s_core = core; }
void RawVariantCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    if (args.size() < 2) {
        send_console(server->get_player(), "`4Usage: /rawvar <FunctionName> [arg1] [arg2]...");
        send_console(server->get_player(), "`9Numerics auto-typed as int/float, others as string.");
        send_console(server->get_player(), "`9Example: /rawvar OnConsoleMessage Hello World");
        return;
    }
    auto& tracker = utils::PlayerTracker::get_instance(); auto lp = tracker.get_local_player();
    packet::Variant var{}; var.add(args[1]);
    for (size_t i=2; i<args.size()&&i<7; ++i) {
        try { size_t p=0; int iv=std::stoi(args[i],&p); if(p==args[i].size()){var.add(iv);continue;} } catch(...){}
        try { size_t p=0; float fv=std::stof(args[i],&p); if(p==args[i].size()){var.add(fv);continue;} } catch(...){}
        var.add(args[i]);
    }
    std::vector<std::byte> ext=var.serialize();
    packet::GameUpdatePacket pkt{}; pkt.type=packet::PACKET_CALL_FUNCTION; pkt.net_id=(lp.netID>0?lp.netID:-1); pkt.flags.extended=1; pkt.data_size=static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{}; bs.write(packet::NET_MESSAGE_GAME_PACKET); bs.write(pkt); bs.write_data(ext.data(),ext.size());
    server->get_player()->send_packet(bs.get_data(),0);
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9RawVar: `b{} `9({} args)", args[1], args.size()-2));
    spdlog::info("RawVariant fn='{}' args={}", args[1], args.size()-2);
}


core::Core* PlayersInfoCommand::s_core = nullptr;
PlayersInfoCommand::PlayersInfoCommand() : CommandBase({"players","plist"}, {}, "List all tracked players with netID and tile position", 0) {}
std::unique_ptr<CommandBase> PlayersInfoCommand::clone() const { return std::make_unique<PlayersInfoCommand>(*this); }
void PlayersInfoCommand::set_core(core::Core* core) { s_core = core; }
void PlayersInfoCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server||!server->get_player()) return;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto all = tracker.get_all_players();
    if (all.empty()) { send_console(server->get_player(), "`4No players tracked. Enter a world first."); return; }
    send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9Players in world (`b{}`9):", all.size()));
    for (const auto& [netid, info] : all) {
        auto pos = tracker.get_player_position(netid);
        send_console(server->get_player(), fmt::format(
            "`9netID=`b{:<5}`9  tile=`b({:>3},{:>3})`9  `b{}{}",
            netid, static_cast<int>(pos.x/32.0f), static_cast<int>(pos.y/32.0f),
            info.name, info.is_local?" `6[YOU]":""));
    }
    spdlog::info("PlayersInfo: listed {} players", all.size());
}

core::Core* DebugAnimCommand::s_core = nullptr;
DebugAnimCommand::DebugAnimCommand() : CommandBase({"debuganim", "copypunch", "animdebug"}, {}, "Debug and inspect captured real vs visual punch animation", 0) {}
std::unique_ptr<CommandBase> DebugAnimCommand::clone() const { return std::make_unique<DebugAnimCommand>(*this); }
void DebugAnimCommand::set_core(core::Core* core) { s_core = core; }
void DebugAnimCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server(); if (!server || !server->get_player()) return;

    auto clothing = utils::PlayerTracker::get_instance().get_clothing();
    uint32_t real_hand = static_cast<uint32_t>(clothing.hand);
    
    auto hand_it = command::g_clothing_slots.find(5);
    uint32_t visual_hand = (hand_it != command::g_clothing_slots.end()) ? static_cast<uint32_t>(hand_it->second) : 0;

    auto& anim_mgr = utils::WeaponAnimationManager::get_instance();
    const auto& last_cap = anim_mgr.get_last_captured();

    if (!args.empty() && (args[0] == "test" || args[0] == "punch")) {
        uint32_t target_hand = (visual_hand > 0) ? visual_hand : (real_hand > 0 ? real_hand : 7830);
        auto player_info = utils::PlayerTracker::get_instance().get_local_player();
        packet::TankUpdatePacket test_tank{};
        test_tank.type = packet::PACKET_STATE;
        test_tank.net_id = static_cast<int32_t>(player_info.netID);
        test_tank.flags = 0xA20;
        test_tank.vec_x = player_info.position.x;
        test_tank.vec_y = player_info.position.y;
        anim_mgr.play_weapon_effects(s_core, target_hand, player_info.netID, &test_tank);
        send_console(server->get_player(), fmt::format("`2[VIN] Tested punch animation for hand item `b{} `2(sent visual_punch to client)!``", target_hand));
        return;
    }

    send_console(server->get_player(), "`0[ `bVinProxy `0] `2=== ANIMATION DEBUG & COPIER ===");
    send_console(server->get_player(), fmt::format("`9Real Hand Item: `b{} `9| Visual Hand Item: `b{}", real_hand, visual_hand));
    if (last_cap.is_valid) {
        send_console(server->get_player(), fmt::format("`2Captured Real Punch: `9item=`b{} `9flags=`b0x{:X} `9anim=`b{} `9vec2=`b({:.1f},{:.1f})",
                     last_cap.hand_id, last_cap.flags, (int)last_cap.animation_type, last_cap.vec_x2, last_cap.vec_y2));
        send_console(server->get_player(), "`aStatus: Authentic profile actively copied and applied to visual weapons!`");
    } else {
        send_console(server->get_player(), "`4No real punch captured yet! Equip your real item (no /clothes hand) and punch once to capture.`");
    }
    send_console(server->get_player(), "`eUsage: `w/debuganim `9(info) | `w/debuganim test `9(trigger visual punch)``");
}

} 
