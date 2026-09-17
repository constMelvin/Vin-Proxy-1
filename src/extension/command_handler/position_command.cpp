#include "position_command.hpp"
#include "utility_commands.hpp"
#include "autocollect_command.hpp"
#include "drop_currency_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/inventory_manager.hpp"
#include "../../utils/text_parse.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <unordered_set>

namespace command {

core::Core* PositionCommand::s_core = nullptr;
std::array<Position, 5> PositionCommand::s_positions = {};
std::atomic<bool> PositionCommand::s_spos1_wait{false};
std::atomic<bool> PositionCommand::s_spos2_wait{false};
glm::vec2 PositionCommand::s_back_pos{0.0f, 0.0f};
std::atomic<int64_t> PositionCommand::s_prize{0};

core::Core* SPosCommand::s_core = nullptr;
core::Core* CPosCommand::s_core = nullptr;
core::Core* CasinoTPCommand::s_core = nullptr;
core::Core* WinCommand::s_core = nullptr;
core::Core* TeleportPosCommand::s_core = nullptr;
core::Core* BackCommand::s_core = nullptr;
std::string BackCommand::s_current_world{};
std::string BackCommand::s_previous_world{};

static void send_console(player::Player* player, const std::string& msg) {
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

static void send_overlay(player::Player* player, const std::string& text) {
    if (!player) return;
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

    player->send_packet(bs.get_data(), 0);
}

static void send_particle(player::Player* player, int effect_id, float x, float y) {
    if (!player) return;
    packet::Variant var{};
    var.add("OnParticleEffect");
    var.add((uint32_t)effect_id);
    var.add(glm::vec2(x, y));
    var.add((uint32_t)0);
    var.add((uint32_t)0);

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

void PositionCommand::highlight_tile(player::Player* player, int32_t tile_x, int32_t tile_y) {
    if (!player) return;

    // 1. Emit glowing ring particle VFX (particle 88) at tile center
    float px = static_cast<float>(tile_x * 32 + 16);
    float py = static_cast<float>(tile_y * 32 + 16);
    send_particle(player, 88, px, py);

    // 2. Lucky Proxy exact highlighttile packet (GAME_SELECT_TILE_INDEX = 38)
    int32_t player_flags = 0;
    try {
        player_flags = std::stoi(std::to_string(static_cast<int>(tile_y)) + std::to_string(static_cast<int>(tile_x)));
    } catch (...) {
        player_flags = tile_x + tile_y * 100;
    }

    // Lucky Proxy sends 60-byte gameupdatepacket_t with m_type=38 and m_player_flags at offset 4
    uint8_t raw_pkt[60] = {0};
    raw_pkt[0] = static_cast<uint8_t>(packet::PACKET_SELECT_TILE_INDEX); // 38
    std::memcpy(raw_pkt + 4, &player_flags, sizeof(int32_t));            // offset 4: m_player_flags

    ByteStream<std::uint16_t> bs_lucky{};
    bs_lucky.write(packet::NET_MESSAGE_GAME_PACKET);
    bs_lucky.write_data(raw_pkt, sizeof(raw_pkt));
    player->send_packet(bs_lucky.get_data(), 0);

    // 3. Also send standard TankUpdatePacket for modern client compatibility
    packet::TankUpdatePacket tank_pkt{};
    tank_pkt.type = static_cast<uint8_t>(packet::PACKET_SELECT_TILE_INDEX);
    tank_pkt.net_id = player_flags;
    tank_pkt.flags = static_cast<uint32_t>(player_flags);
    tank_pkt.int_data = static_cast<uint32_t>(player_flags);
    tank_pkt.int_x = tile_x;
    tank_pkt.int_y = tile_y;

    ByteStream<std::uint16_t> bs_tank{};
    bs_tank.write(packet::NET_MESSAGE_GAME_PACKET);
    bs_tank.write(tank_pkt);
    player->send_packet(bs_tank.get_data(), 0);
}

void PositionCommand::handle_punched_tile(int32_t tile_x, int32_t tile_y) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (s_spos1_wait.load()) {
        s_spos1_wait.store(false);
        set_position(0, tile_x, tile_y, static_cast<float>(tile_x * 32), static_cast<float>(tile_y * 32));
        highlight_tile(server->get_player(), tile_x, tile_y);
        send_console(server->get_player(), fmt::format("`91st position: `#{:.0f}, {:.0f}", (float)(tile_x * 32), (float)(tile_y * 32)));
        spdlog::info("SPosCommand: Pos1 set by punch to ({}, {})", tile_x, tile_y);
    } else if (s_spos2_wait.load()) {
        s_spos2_wait.store(false);
        set_position(1, tile_x, tile_y, static_cast<float>(tile_x * 32), static_cast<float>(tile_y * 32));
        highlight_tile(server->get_player(), tile_x, tile_y);
        send_console(server->get_player(), fmt::format("`92nd position: `#{:.0f}, {:.0f}", (float)(tile_x * 32), (float)(tile_y * 32)));
        spdlog::info("SPosCommand: Pos2 set by punch to ({}, {})", tile_x, tile_y);
    }
}

PositionCommand::PositionCommand() : CommandBase(
    {"pos1", "pos2", "pos3", "pos4", "posback"},
    {},
    "Set drop/teleport positions (pos1-4 for drops, posback for return)",
    0
) {}

std::unique_ptr<CommandBase> PositionCommand::clone() const {
    return std::make_unique<PositionCommand>(*this);
}

void PositionCommand::set_core(core::Core* core) {
    s_core = core;
}

Position PositionCommand::get_position(int index) {
    if (index >= 0 && index < 5) {
        return s_positions[index];
    }
    return Position{};
}

void PositionCommand::set_position(int index, int x, int y, float px, float py) {
    if (index >= 0 && index < 5) {
        float real_px = (px >= 0.0f) ? px : static_cast<float>(x * 32);
        float real_py = (py >= 0.0f) ? py : static_cast<float>(y * 32);
        s_positions[index] = Position{x, y, real_px, real_py};
    }
}

void PositionCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.empty()) return;
    const std::string& cmd = args[0];

    int pos_index = -1;
    std::string pos_name;
    
    if (cmd == "pos1") { pos_index = 0; pos_name = "1"; }
    else if (cmd == "pos2") { pos_index = 1; pos_name = "2"; }
    else if (cmd == "pos3") { pos_index = 2; pos_name = "3"; }
    else if (cmd == "pos4") { pos_index = 3; pos_name = "4"; }
    else if (cmd == "posback") { pos_index = 4; pos_name = "back"; }
    
    if (pos_index == -1) return;

    // Reset punch wait flags if user runs manual /pos1 or /pos2
    s_spos1_wait.store(false);
    s_spos2_wait.store(false);

    float pos_x = 0.0f;
    float pos_y = 0.0f;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local = tracker.get_local_player();
    if (local.netID != 0) {
        pos_x = local.position.x;
        pos_y = local.position.y;
        s_core->get_config().set<std::string>("player.position.x", std::to_string(pos_x));
        s_core->get_config().set<std::string>("player.position.y", std::to_string(pos_y));
        s_core->get_config().save();
    } else {
        try {
            pos_x = std::stof(s_core->get_config().get<std::string>("player.position.x"));
            pos_y = std::stof(s_core->get_config().get<std::string>("player.position.y"));
        } catch (...) {
            send_console(server->get_player(), "`4Failed to get position - not tracked yet");
            return;
        }
    }
    
    int tile_x = static_cast<int>(pos_x / 32.0f);
    int tile_y = static_cast<int>(pos_y / 32.0f);
    
    set_position(pos_index, tile_x, tile_y, pos_x, pos_y);
    highlight_tile(server->get_player(), tile_x, tile_y);

    if (pos_index == 0) {
        send_console(server->get_player(), fmt::format("`91st position: `#{:.0f}, {:.0f}", pos_x, pos_y));
    } else if (pos_index == 1) {
        send_console(server->get_player(), fmt::format("`92nd position: `#{:.0f}, {:.0f}", pos_x, pos_y));
    } else {
        send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9pos {} set to `b{} `9,`b{}", pos_name, tile_x, tile_y));
    }
}

// =========================================================================
// SPosCommand: /spos1, /spos2
// =========================================================================
SPosCommand::SPosCommand() : CommandBase(
    {"spos1", "spos2"},
    {},
    "Punch tile to set Pos1 or Pos2",
    0
) {}

std::unique_ptr<CommandBase> SPosCommand::clone() const {
    return std::make_unique<SPosCommand>(*this);
}

void SPosCommand::set_core(core::Core* core) {
    s_core = core;
}

void SPosCommand::execute(client::Client*, const std::vector<std::string>& args) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.empty()) return;
    const std::string& cmd = args[0];

    if (cmd == "spos1") {
        PositionCommand::set_spos1_waiting(true);
        PositionCommand::set_spos2_waiting(false);
        send_console(server->get_player(), "`9Punch tile to set Pos1.");
    } else if (cmd == "spos2") {
        PositionCommand::set_spos2_waiting(true);
        PositionCommand::set_spos1_waiting(false);
        send_console(server->get_player(), "`9Punch tile to set Pos2.");
    }
}

// =========================================================================
// CPosCommand: /cpos1, /cpos2
// =========================================================================
CPosCommand::CPosCommand() : CommandBase(
    {"cpos1", "cpos2"},
    {},
    "Highlight and show saved Pos1 or Pos2",
    0
) {}

std::unique_ptr<CommandBase> CPosCommand::clone() const {
    return std::make_unique<CPosCommand>(*this);
}

void CPosCommand::set_core(core::Core* core) {
    s_core = core;
}

void CPosCommand::execute(client::Client*, const std::vector<std::string>& args) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.empty()) return;
    const std::string& cmd = args[0];

    if (cmd == "cpos1") {
        Position pos = PositionCommand::get_position(0);
        if (pos.is_set()) {
            PositionCommand::highlight_tile(server->get_player(), pos.x, pos.y);
        }
        send_console(server->get_player(), "`9Showing Pos1.");
    } else if (cmd == "cpos2") {
        Position pos = PositionCommand::get_position(1);
        if (pos.is_set()) {
            PositionCommand::highlight_tile(server->get_player(), pos.x, pos.y);
        }
        send_console(server->get_player(), "`9Showing Pos2.");
    }
}

// =========================================================================
// CasinoTPCommand: /tp (Expanded 1.5 tile reach bet collection & verification)
// =========================================================================
CasinoTPCommand::CasinoTPCommand() : CommandBase(
    {"tp"},
    {},
    "Teleport to drop position and collect bets with 1.5 tile reach",
    0
) {}

std::unique_ptr<CommandBase> CasinoTPCommand::clone() const {
    return std::make_unique<CasinoTPCommand>(*this);
}

void CasinoTPCommand::set_core(core::Core* core) {
    s_core = core;
}

void CasinoTPCommand::execute(client::Client* client, const std::vector<std::string>&) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    auto p1 = PositionCommand::get_position(0);
    auto p2 = PositionCommand::get_position(1);

    if (!p1.is_set() || !p2.is_set()) {
        send_console(server->get_player(), "`9Please setpos first (/pos1, /pos2)");
        return;
    }

    // 1. Determine host starting position & store as back_pos
    glm::vec2 start_pos{0.0f, 0.0f};
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local = tracker.get_local_player();
    if (local.position.x > 0.0f || local.position.y > 0.0f) {
        start_pos = glm::vec2(local.position.x, local.position.y);
    } else if (s_core) {
        try {
            float cx = std::stof(s_core->get_config().get<std::string>("player.position.x"));
            float cy = std::stof(s_core->get_config().get<std::string>("player.position.y"));
            if (cx > 0.0f || cy > 0.0f) {
                start_pos = glm::vec2(cx, cy);
            }
        } catch (...) {}
    }
    // If still (0,0), check saved posback (index 4)
    if (start_pos.x == 0.0f && start_pos.y == 0.0f) {
        Position pback = PositionCommand::get_position(4);
        if (pback.is_set()) {
            start_pos = glm::vec2(pback.x * 32.0f, pback.y * 32.0f);
        }
    }
    // If host is standing directly on pos1 or pos2, use posback if available to avoid treating pos1/pos2 as host home
    int start_tx = static_cast<int>(start_pos.x / 32.0f);
    int start_ty = static_cast<int>(start_pos.y / 32.0f);
    if ((start_tx == p1.x && start_ty == p1.y) || (start_tx == p2.x && start_ty == p2.y)) {
        Position pback = PositionCommand::get_position(4);
        if (pback.is_set()) {
            start_pos = glm::vec2(pback.x * 32.0f, pback.y * 32.0f);
        }
    }
    PositionCommand::set_back_pos(start_pos);

    // 2. Query all dropped items deduplicated across live_objects and items
    auto& wm = utils::WorldManager::get_instance();
    std::vector<world::DroppedItemInfo> dropped;
    std::unordered_set<uint32_t> seen_uids;
    for (const auto& obj : wm.get_live_objects()) {
        if (seen_uids.insert(obj.Uid).second) dropped.push_back(obj);
    }
    for (const auto& obj : wm.get_items()) {
        if (seen_uids.insert(obj.Uid).second) dropped.push_back(obj);
    }

    float pos1_px = (p1.px >= 0.0f) ? p1.px : static_cast<float>(p1.x * 32);
    float pos1_py = (p1.py >= 0.0f) ? p1.py : static_cast<float>(p1.y * 32);
    float pos2_px = (p2.px >= 0.0f) ? p2.px : static_cast<float>(p2.x * 32);
    float pos2_py = (p2.py >= 0.0f) ? p2.py : static_cast<float>(p2.y * 32);

    float c1_x = pos1_px + 16.0f;
    float c1_y = pos1_py + 16.0f;
    float c2_x = pos2_px + 16.0f;
    float c2_y = pos2_py + 16.0f;

    std::vector<world::DroppedItemInfo> p1_objs;
    std::vector<world::DroppedItemInfo> p2_objs;
    int count1 = 0;
    int count2 = 0;

    auto is_near_tile = [](float obj_x, float obj_y, int tile_x, int tile_y, float center_x, float center_y) {
        int tx = static_cast<int>((obj_x + 8.0f) / 32.0f);
        int ty = static_cast<int>((obj_y + 8.0f) / 32.0f);
        if (std::abs(tx - tile_x) <= 1 && std::abs(ty - tile_y) <= 1) return true;
        float dx = center_x - (obj_x + 8.0f);
        float dy = center_y - (obj_y + 8.0f);
        return (dx * dx + dy * dy) <= (64.0f * 64.0f);
    };

    for (const auto& obj : dropped) {
        if (obj.ItemId != 242 && obj.ItemId != 1796 && obj.ItemId != 7188) continue;

        if (is_near_tile(obj.X, obj.Y, p1.x, p1.y, c1_x, c1_y)) {
            p1_objs.push_back(obj);
            if (obj.ItemId == 242) count1 += obj.Amount;
            else if (obj.ItemId == 1796) count1 += obj.Amount * 100;
            else if (obj.ItemId == 7188) count1 += obj.Amount * 10000;
        } else if (is_near_tile(obj.X, obj.Y, p2.x, p2.y, c2_x, c2_y)) {
            p2_objs.push_back(obj);
            if (obj.ItemId == 242) count2 += obj.Amount;
            else if (obj.ItemId == 1796) count2 += obj.Amount * 100;
            else if (obj.ItemId == 7188) count2 += obj.Amount * 10000;
        }
    }

    core::Core* core = s_core;
    std::thread([core, client, p1_objs, p2_objs, count1, count2]() {
        if (!core || !client) return;
        auto* server = core->get_server();
        if (!server) return;

        auto* to_server = client->get_player();
        auto* to_client = server->get_player();

        // Send collection packets directly to server without moving character
        for (const auto& obj : p1_objs) {
            AutoCollectCommand::send_collect_packet(to_server, obj.Uid, obj.X, obj.Y);
        }
        for (const auto& obj : p2_objs) {
            AutoCollectCommand::send_collect_packet(to_server, obj.Uid, obj.X, obj.Y);
        }

        // Second pass after 50ms to guarantee pickup
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        for (const auto& obj : p1_objs) {
            AutoCollectCommand::send_collect_packet(to_server, obj.Uid, obj.X, obj.Y);
        }
        for (const auto& obj : p2_objs) {
            AutoCollectCommand::send_collect_packet(to_server, obj.Uid, obj.X, obj.Y);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (count1 == count2 && count1 > 0) {
            int tax_p = 10;
            try {
                tax_p = core->get_config().get<int>("features.host.tax_percent");
            } catch (...) {
                core->get_config().set<int>("features.host.tax_percent", 10);
            }

            int64_t total_bet = static_cast<int64_t>(count1) * 2;
            int64_t tax_amount = (total_bet * tax_p) / 100;
            int64_t prize = total_bet - tax_amount;

            PositionCommand::set_prize(prize);

            std::string notif = fmt::format(
                "`2Both Bets Are Equal\n`2{}% Tax\n`wPos1:`# {} `9Wls\n`wPos2: `#{}`9Wls\n`wAmount To Drop:`# {} `9Wls",
                tax_p, count1, count2, prize
            );
            send_overlay(to_client, notif);
            send_console(to_client, notif);
            spdlog::info("CasinoTP: Bets equal ({} vs {}). Prize to drop: {} WLs ({}% tax)", count1, count2, prize, tax_p);
        } else {
            PositionCommand::set_prize(0);
            std::string notif = fmt::format(
                "`4Bets Are Not Equal\n`wPos1:`# {} `9Wls\n`wPos2: `#{}`9Wls",
                count1, count2
            );
            send_overlay(to_client, notif);
            send_console(to_client, notif);
            spdlog::info("CasinoTP: Bets not equal ({} vs {}). No prize set.", count1, count2);
        }
    }).detach();
}

// =========================================================================
// WinCommand: /w1, /win1, /w2, /win2
// =========================================================================
WinCommand::WinCommand() : CommandBase(
    {"w1", "win1", "w2", "win2"},
    {},
    "Teleport to player 1 or 2 winning spot, drop prize, and return",
    0
) {}

std::unique_ptr<CommandBase> WinCommand::clone() const {
    return std::make_unique<WinCommand>(*this);
}

void WinCommand::set_core(core::Core* core) {
    s_core = core;
}

void WinCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    int64_t prize = PositionCommand::get_prize();
    if (prize <= 0) {
        send_console(server->get_player(), "`9Please start the game first!");
        return;
    }

    auto& inv = utils::InventoryManager::get_instance();
    int64_t my_wls = inv.get_item_count(242) +
                     static_cast<int64_t>(inv.get_item_count(1796)) * 100 +
                     static_cast<int64_t>(inv.get_item_count(7188)) * 10000;

    if (my_wls < prize) {
        send_console(server->get_player(), "`4Not enough balance to drop reward. Run /tp again to collect the bets.");
        return;
    }

    auto p1 = PositionCommand::get_position(0);
    auto p2 = PositionCommand::get_position(1);
    if (!p1.is_set() || !p2.is_set()) {
        send_console(server->get_player(), "`4Positions not set!");
        return;
    }

    const std::string& cmd = args[0];
    bool is_p1 = (cmd == "w1" || cmd == "win1");

    int target_x = is_p1 ? p1.x : p2.x;
    int target_y = is_p1 ? p1.y : p2.y;
    int win_tile_x = target_x;
    int win_tile_y = target_y;

    core::Core* core = s_core;
    glm::vec2 back_pos = PositionCommand::get_back_pos();
    if (back_pos.x == 0.0f && back_pos.y == 0.0f) {
        auto local = utils::PlayerTracker::get_instance().get_local_player();
        if (local.position.x > 0.0f || local.position.y > 0.0f) {
            back_pos = glm::vec2(local.position.x, local.position.y);
        } else {
            try {
                float cx = std::stof(core->get_config().get<std::string>("player.position.x"));
                float cy = std::stof(core->get_config().get<std::string>("player.position.y"));
                if (cx > 0.0f || cy > 0.0f) back_pos = glm::vec2(cx, cy);
            } catch (...) {}
        }
        if (back_pos.x == 0.0f && back_pos.y == 0.0f) {
            Position pback = PositionCommand::get_position(4);
            if (pback.is_set()) back_pos = glm::vec2(pback.x * 32.0f, pback.y * 32.0f);
        }
    }

    std::thread([core, client, target_x, target_y, win_tile_x, win_tile_y, back_pos, prize]() {
        // Path directly to winner drop spot
        command::FindPathCommand::run_path(client, target_x, target_y, true, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Execute drop
        DropDLCommand drop_cmd;
        drop_cmd.execute_with_core(client, {"dd", std::to_string(prize)}, core);

        // Wait for drop to complete before moving away
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        int timeout_ms = 4000;
        while (DropCurrencyState::s_dropping.load() && timeout_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            timeout_ms -= 50;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        if (core && core->get_server() && core->get_server()->get_player()) {
            PositionCommand::highlight_tile(core->get_server()->get_player(), win_tile_x, win_tile_y);
        }

        // Return to saved back position reliably
        if (back_pos.x > 0.0f || back_pos.y > 0.0f) {
            int bx = static_cast<int>(back_pos.x / 32.0f);
            int by = static_cast<int>(back_pos.y / 32.0f);
            command::FindPathCommand::run_path(client, bx, by, true, 1);

            if (core && core->get_server() && core->get_server()->get_player()) {
                auto* srv_player = core->get_server()->get_player();
                uint32_t active_netid = utils::PlayerTracker::get_instance().get_local_netid();
                SetPosCommand::set_position(srv_player, static_cast<uint32_t>(-1), back_pos.x, back_pos.y);
                if (active_netid > 0) {
                    SetPosCommand::set_position(srv_player, active_netid, back_pos.x, back_pos.y);
                    utils::PlayerTracker::get_instance().update_player_position(active_netid, back_pos.x, back_pos.y);
                }
            }
        }

        PositionCommand::set_prize(0);
    }).detach();
}

// =========================================================================
// TeleportPosCommand: /tp1, /tp2, /tp3, /tp4
// =========================================================================
TeleportPosCommand::TeleportPosCommand() : CommandBase(
    {"tp1", "tp2", "tp3", "tp4"},
    {},
    "Teleport to saved position (tp1-4)",
    0
) {}

std::unique_ptr<CommandBase> TeleportPosCommand::clone() const {
    return std::make_unique<TeleportPosCommand>(*this);
}

void TeleportPosCommand::set_core(core::Core* core) {
    s_core = core;
}

void TeleportPosCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.empty()) return;
    int pos_index = -1;
    const std::string& cmd = args[0];
    if (cmd == "tp1") pos_index = 0;
    else if (cmd == "tp2") pos_index = 1;
    else if (cmd == "tp3") pos_index = 2;
    else if (cmd == "tp4") pos_index = 3;
    
    if (pos_index == -1) return;

    Position pos = PositionCommand::get_position(pos_index);
    if (!pos.is_set()) {
        send_console(server->get_player(), "`4Pos Not Set");
        return;
    }

    uint32_t cur_tx = 0, cur_ty = 0;
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local = tracker.get_local_player();
    if (local.netID != 0) {
        cur_tx = static_cast<uint32_t>(local.position.x / 32.0f);
        cur_ty = static_cast<uint32_t>(local.position.y / 32.0f);
    }

    if (cur_tx == static_cast<uint32_t>(pos.x) && cur_ty == static_cast<uint32_t>(pos.y)) {
        send_console(server->get_player(), "`6Already at that saved position");
        return;
    }

    int steps = command::FindPathCommand::run_path(client, pos.x, pos.y, true, 1);
    if (steps > 0) {
        send_overlay(server->get_player(),
            fmt::format("`2Pathfind:`w {} steps to (`9{},{} `w) `o(fast)", steps, pos.x, pos.y));
        send_console(server->get_player(),
            fmt::format("`0[ `bVinProxy `0] `9Pathing to pos{} at `b{} `9,`b{} (`9{} steps`9)",
                pos_index + 1, pos.x, pos.y, steps));
    } else {
        send_console(server->get_player(), "`4Failed to path to position");
    }
}

// =========================================================================
// BackCommand: /back, /BACK
// =========================================================================
BackCommand::BackCommand() : CommandBase(
    {"back", "BACK"},
    {},
    "Warp to the previously entered world",
    0
) {}

std::unique_ptr<CommandBase> BackCommand::clone() const {
    return std::make_unique<BackCommand>(*this);
}

void BackCommand::set_core(core::Core* core) {
    s_core = core;
}

void BackCommand::note_join_request_target(const std::string& raw_name) {
    if (raw_name.empty()) return;

    std::string world = raw_name;
    const size_t pipe_pos = world.find('|');
    if (pipe_pos != std::string::npos) {
        world = world.substr(0, pipe_pos);
    }

    const size_t space_pos = world.find(' ');
    if (space_pos != std::string::npos) {
        world = world.substr(0, space_pos);
    }

    if (world.empty()) return;
    if (!s_current_world.empty() && s_current_world == world) return;

    s_previous_world = s_current_world;
    s_current_world = world;
}

void BackCommand::execute(client::Client* client, const std::vector<std::string>&) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (s_previous_world.empty()) {
        send_console(server->get_player(), "`4No previous world tracked yet.");
        return;
    }

    TextParse text_parse{};
    text_parse.add("action", {"join_request"});
    text_parse.add("name", {s_previous_world});
    text_parse.add("invitedWorld", {"0"});

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_MESSAGE);
    bs.write(text_parse.get_raw(), false);
    client->get_player()->send_packet(bs.get_data(), 0);
    send_console(server->get_player(), fmt::format("`0[`2BACK```0]``: `wGoing to previous world: `3{}", s_previous_world));
    spdlog::info("BackCommand: warping to previous world '{}'", s_previous_world);
}

} // namespace command
