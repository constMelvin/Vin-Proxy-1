#include "dropat_command.hpp"
#include "position_command.hpp"
#include "utility_commands.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/inventory_manager.hpp"
#include "../../utils/player_tracker.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <algorithm>

namespace command {

// --- DposCommand static state ---
core::Core* DposCommand::s_core = nullptr;
DropTargetPos DposCommand::s_target_pos{};

// --- DropAtCommand static state ---
core::Core* DropAtCommand::s_core = nullptr;
std::atomic<bool> DropAtCommand::s_running{false};
std::atomic<std::uint64_t> DropAtCommand::s_generation{0};
std::atomic<bool> DropAtCommand::s_local_facing_left{false};

void DropAtCommand::set_local_facing_left(bool left) {
    s_local_facing_left.store(left);
}

bool DropAtCommand::get_local_facing_left() {
    return s_local_facing_left.load();
}

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

static void send_generic_text(player::Player* player, const std::string& text) {
    if (!player) return;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(text, false);
    player->send_packet(bs.get_data(), 0);
}

// ==========================================
// DposCommand Implementation
// ==========================================

DposCommand::DposCommand() : CommandBase(
    {"dpos"},
    {},
    "Set target drop position where you stand (same logic & animation as pos1-4)",
    0
) {}

std::unique_ptr<CommandBase> DposCommand::clone() const {
    return std::make_unique<DposCommand>(*this);
}

void DposCommand::set_core(core::Core* core) {
    s_core = core;
}

DropTargetPos DposCommand::get_pos() {
    if (s_target_pos.is_set()) {
        return s_target_pos;
    }
    if (s_core) {
        try {
            int x = s_core->get_config().get<int>("player.drop_pos.x", -1);
            int y = s_core->get_config().get<int>("player.drop_pos.y", -1);
            if (x >= 0 && y >= 0) {
                s_target_pos = {x, y};
                return s_target_pos;
            }
        } catch (...) {}
    }
    return s_target_pos;
}

void DposCommand::set_pos(int x, int y) {
    s_target_pos = {x, y};
    if (s_core) {
        try {
            s_core->get_config().set<int>("player.drop_pos.x", x);
            s_core->get_config().set<int>("player.drop_pos.y", y);
            s_core->get_config().save();
        } catch (...) {}
    }
}

void DposCommand::clear_pos() {
    s_target_pos.clear();
    if (s_core) {
        try {
            s_core->get_config().set<int>("player.drop_pos.x", -1);
            s_core->get_config().set<int>("player.drop_pos.y", -1);
            s_core->get_config().save();
        } catch (...) {}
    }
}

void DposCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) {
        spdlog::error("DposCommand: No core or client player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("DposCommand: No server player!");
        return;
    }

    auto* srv_player = server->get_player();

    if (args.size() >= 2) {
        std::string sub = args[1];
        if (sub == "clear" || sub == "reset") {
            clear_pos();
            send_console(srv_player, "`0[ `bVinProxy `0] `9Drop position cleared.");
            return;
        }

        if (sub == "status" || sub == "check" || sub == "info") {
            auto p = get_pos();
            bool face_left = DropAtCommand::get_local_facing_left();
            if (p.is_set()) {
                int stand_x = face_left ? (p.x + 1) : (p.x - 1);
                send_console(srv_player, fmt::format(
                    "`0[ `bVinProxy `0] `9Drop Target: (`b{},{}`9) | Facing: `b{} `9| Stand At: (`b{},{}`9)",
                    p.x, p.y, face_left ? "LEFT (+1)" : "RIGHT (-1)", stand_x, p.y
                ));
            } else {
                send_console(srv_player, "`0[ `bVinProxy `0] `9No drop position set. Use `w/dpos`9 where you want items to drop.");
            }
            return;
        }
    }

    // Default: capture current character position (identical to pos1-4)
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
        std::string pos_x_str = s_core->get_config().get<std::string>("player.position.x", "");
        std::string pos_y_str = s_core->get_config().get<std::string>("player.position.y", "");
        try {
            if (!pos_x_str.empty() && !pos_y_str.empty()) {
                pos_x = std::stof(pos_x_str);
                pos_y = std::stof(pos_y_str);
            }
        } catch (const std::exception& e) {
            send_console(srv_player, "`4Failed to get position - not tracked yet");
            spdlog::warn("DposCommand: Failed to parse position from config: {}", e.what());
            return;
        }
    }

    if (pos_x <= 0.0f && pos_y <= 0.0f) {
        send_console(srv_player, "`4Failed to get position - not tracked yet");
        return;
    }

    int tile_x = static_cast<int>(pos_x / 32.0f);
    int tile_y = static_cast<int>(pos_y / 32.0f);

    set_pos(tile_x, tile_y);

    std::string msg = fmt::format("`0[ `bVinProxy `0] `9dpos set to `b{} `9,`b{}", 
                                   tile_x, tile_y);
    send_console(srv_player, msg);

    // Play visual target reticle effect at set position (particle 88 identical to pos1-4)
    int particle_id = 88;
    float px = static_cast<float>(tile_x * 32 + 16);
    float py = static_cast<float>(tile_y * 32 + 16);

    std::thread([srv_player, particle_id, px, py]() {
        for (int i = 0; i < 3; ++i) {
            send_particle(srv_player, particle_id, px, py);
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }
    }).detach();

    spdlog::info("Set dpos to ({}, {}) with visual effect {}", tile_x, tile_y, particle_id);
}

// ==========================================
// DropAtCommand Implementation
// ==========================================

bool DropAtCommand::is_running() {
    return s_running.load();
}

DropAtCommand::DropAtCommand() : CommandBase(
    {"dropat", "dposdrop"},
    {"[amt]"},
    "Teleport to /dpos position and drop items (all or specified amount)",
    0
) {}

std::unique_ptr<CommandBase> DropAtCommand::clone() const {
    return std::make_unique<DropAtCommand>(*this);
}

void DropAtCommand::set_core(core::Core* core) {
    s_core = core;
}

void DropAtCommand::execute(client::Client* client_ptr, const std::vector<std::string>& args) {
    if (!s_core || !client_ptr || !client_ptr->get_player()) {
        spdlog::error("DropAtCommand: No core or player!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("DropAtCommand: No server player!");
        return;
    }

    auto* srv_player = server->get_player();
    // Toggle cancel if already running
    if (s_running.load()) {
        ++s_generation;
        s_running = false;
        send_console(srv_player, "`0[ `bVinProxy `0] `9dropat cancelled");
        return;
    }

    auto p = DposCommand::get_pos();
    if (!p.is_set()) {
        send_console(srv_player, "`4[ `bVinProxy `4] `9Pos Not Set! Use `w/dpos`9 first where you want items dropped.");
        return;
    }

    int target_x = p.x;
    int target_y = p.y;
    int max_quantity = 0;

    if (args.size() > 1) {
        try {
            max_quantity = std::stoi(args[1]);
            if (max_quantity <= 0) {
                send_console(srv_player, "`4[ `bVinProxy `4] `9Amount must be greater than 0");
                return;
            }
        } catch (...) {
            send_console(srv_player, fmt::format("`4[ `bVinProxy `4] `9Invalid amount: {}", args[1]));
            return;
        }
    }

    // 1. Check facing BEFORE teleporting
    bool face_left = get_local_facing_left();

    // If facing left: standing at (target_x + 1) will drop onto target_x.
    // If facing right: standing at (target_x - 1) will drop onto target_x.
    int stand_x = face_left ? (target_x + 1) : (target_x - 1);
    int stand_y = target_y;

    s_running = true;
    const std::uint64_t gen = ++s_generation;

    send_console(srv_player, fmt::format(
        "`0[ `bVinProxy `0] `9Drop target: (`b{},{}`9) | Facing: `b{} `9| Stand at: (`b{},{}`9)",
        target_x, target_y, face_left ? "LEFT (+1)" : "RIGHT (-1)", stand_x, stand_y
    ));

    if (max_quantity > 0) {
        send_console(srv_player, fmt::format("`0[ `bVinProxy `0] `9Dropping up to `w{}`9 of each item..", max_quantity));
    } else {
        send_console(srv_player, "`0[ `bVinProxy `0] `9Dropping all items..");
    }

    std::thread([client_ptr, target_x, target_y, max_quantity, gen, face_left]() {
        run_drop_at(client_ptr, target_x, target_y, max_quantity, gen, face_left);
    }).detach();
}

void DropAtCommand::run_drop_at(client::Client* client_ptr, int target_x, int target_y, int max_quantity, std::uint64_t generation, bool face_left) {
    auto finish = []() {
        DropAtCommand::s_running = false;
    };

    if (!s_core || !client_ptr || !client_ptr->get_player()) {
        finish();
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        finish();
        return;
    }

    auto* srv_player = server->get_player();
    auto* cl_player = client_ptr->get_player();

    // Stand tile calculation based on facing
    int stand_x = face_left ? (target_x + 1) : (target_x - 1);
    int stand_y = target_y;

    // 1. Move / Teleport to stand position
    auto& tracker = utils::PlayerTracker::get_instance();
    auto local = tracker.get_local_player();
    int cur_tx = -1;
    int cur_ty = -1;
    if (local.netID != 0) {
        cur_tx = static_cast<int>(local.position.x / 32.0f);
        cur_ty = static_cast<int>(local.position.y / 32.0f);
    }

    if (cur_tx != stand_x || cur_ty != stand_y) {
        // Fast pathfind
        command::FindPathCommand::run_path(client_ptr, static_cast<uint32_t>(stand_x), static_cast<uint32_t>(stand_y), false, 1);

        // Instant position and facing lock
        const float target_px = static_cast<float>(stand_x * 32);
        const float target_py = static_cast<float>(stand_y * 32);
        uint32_t active_netid = (local.netID != 0) ? local.netID : 0;

        SetPosCommand::set_position(srv_player, static_cast<uint32_t>(-1), target_px, target_py);
        if (active_netid > 0) {
            SetPosCommand::set_position(srv_player, active_netid, target_px, target_py);
            tracker.update_player_position(active_netid, target_px, target_py);
        }

        s_core->get_config().set<std::string>("player.position.x", std::to_string(target_px));
        s_core->get_config().set<std::string>("player.position.y", std::to_string(target_py));
        s_core->get_config().save();

        MoriStatePacket final_pkt{};
        final_pkt.type = static_cast<uint8_t>(packet::PACKET_STATE);
        final_pkt.net_id = active_netid;
        final_pkt.vector_x = target_px;
        final_pkt.vector_y = target_py + 2.0f;
        final_pkt.flags = 0x1u | packet::PACKET_FLAG_ON_SOLID;
        if (face_left) {
            final_pkt.flags |= packet::PACKET_FLAG_ROTATE_LEFT; // 0x10
        }

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(final_pkt);
        // Send to remote server so server knows player's position & facing
        cl_player->send_packet(bs.get_data(), 0);
        // Send to local client so game screen sprite faces the correct direction
        srv_player->send_packet(bs.get_data(), 0);

        std::string move_packet = fmt::format("action|move\nx|{}\ny|{}", stand_x, stand_y);
        ByteStream<std::uint16_t> move_bs{};
        move_bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
        move_bs.write(move_packet, false);
        cl_player->send_packet(move_bs.get_data(), 0);

        // Visual reticle (particle 88) directly at the TARGET DROP tile where items land
        for (int i = 0; i < 3; ++i) {
            send_particle(srv_player, 88, static_cast<float>(target_x * 32 + 16), static_cast<float>(target_y * 32 + 16));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    } else {
        // Already at stand position - ensure facing packet is firmly applied
        const float target_px = static_cast<float>(stand_x * 32);
        const float target_py = static_cast<float>(stand_y * 32);
        uint32_t active_netid = (local.netID != 0) ? local.netID : 0;

        MoriStatePacket final_pkt{};
        final_pkt.type = static_cast<uint8_t>(packet::PACKET_STATE);
        final_pkt.net_id = active_netid;
        final_pkt.vector_x = target_px;
        final_pkt.vector_y = target_py + 2.0f;
        final_pkt.flags = 0x1u | packet::PACKET_FLAG_ON_SOLID;
        if (face_left) {
            final_pkt.flags |= packet::PACKET_FLAG_ROTATE_LEFT;
        }

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(final_pkt);
        cl_player->send_packet(bs.get_data(), 0);
        srv_player->send_packet(bs.get_data(), 0);

        for (int i = 0; i < 3; ++i) {
            send_particle(srv_player, 88, static_cast<float>(target_x * 32 + 16), static_cast<float>(target_y * 32 + 16));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (generation != s_generation.load()) {
        finish();
        return;
    }

    // 2. Drop inventory items - dropped items will land directly on target_x, target_y!
    auto& inv_mgr = utils::InventoryManager::get_instance();
    const auto items = inv_mgr.get_items_snapshot();
    std::size_t dropped_stacks = 0;

    for (const auto& item : items) {
        if (!s_core || generation != s_generation.load()) {
            break;
        }

        player::Player* to_server_player = nullptr;
        if (s_core && s_core->get_client())
            to_server_player = s_core->get_client()->get_player();
        if (!to_server_player) {
            break;
        }

        if (item.id == 0 || item.amount == 0) {
            continue;
        }

        int remaining = (max_quantity > 0)
            ? std::min(static_cast<int>(item.amount), max_quantity)
            : static_cast<int>(item.amount);

        if (remaining <= 0) {
            continue;
        }

        while (remaining > 0 && generation == s_generation.load()) {
            const int chunk = (remaining > 200) ? 200 : remaining;
            remaining -= chunk;

            // Step 1: Send drop request
            send_generic_text(to_server_player, fmt::format("action|drop\n|itemID|{}|\n", item.id));
            std::this_thread::sleep_for(std::chrono::milliseconds(150));

            // Step 2: Confirm count
            std::string confirm = fmt::format(
                "action|dialog_return\ndialog_name|drop_item\nitemID|{}|\ncount|{}\n",
                item.id, chunk
            );
            send_generic_text(to_server_player, confirm);

            // Step 3: Last item warning confirmation
            if (chunk >= static_cast<int>(item.amount) || remaining == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
                std::string warn_confirm = fmt::format(
                    "action|dialog_return\ndialog_name|drop_item\nitemID|{}|\nbuttonClicked|yes\n",
                    item.id
                );
                send_generic_text(to_server_player, warn_confirm);
            }

            dropped_stacks++;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

    if (s_core) {
        auto* local_player = s_core->get_server() ? s_core->get_server()->get_player() : nullptr;
        if (local_player && generation == s_generation.load()) {
            if (max_quantity > 0) {
                send_console(local_player,
                    fmt::format("`0[ `bVinProxy `0] `9Finished dropping items at (`w{},{}`9) - `w{}`9 stacks (up to `w{}`9 each)",
                                target_x, target_y, dropped_stacks, max_quantity));
            } else {
                send_console(local_player,
                    fmt::format("`0[ `bVinProxy `0] `9Finished dropping all items at (`w{},{}`9) - `w{}`9 stacks",
                                target_x, target_y, dropped_stacks));
            }
        }
    }

    finish();
}

} // namespace command
