#include "dropall_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/inventory_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <chrono>

namespace command {

core::Core* DropAllCommand::s_core = nullptr;
std::atomic<bool> DropAllCommand::s_running{false};
std::atomic<std::uint64_t> DropAllCommand::s_generation{0};

bool DropAllCommand::is_running() {
    return s_running.load();
}

DropAllCommand::DropAllCommand() : CommandBase(
    {"dropall"},
    {"[quantity]"},
    "Drop items from inventory (all or specified quantity)",
    0
) {}

std::unique_ptr<CommandBase> DropAllCommand::clone() const {
    return std::make_unique<DropAllCommand>(*this);
}

void DropAllCommand::set_core(core::Core* core) {
    s_core = core;
}

static void send_console(player::Player* local_player, const std::string& msg) {
    if (!local_player) return;
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

    local_player->send_packet(bs.get_data(), 0);
}

static void send_generic_text(player::Player* to_server_player, const std::string& raw) {
    if (!to_server_player) return;
    ByteStream<std::uint16_t> bs{};
    
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(raw, false);
    to_server_player->send_packet(bs.get_data(), 0);
}

void DropAllCommand::execute(client::Client* , const std::vector<std::string>& args) {
    spdlog::info("DropAllCommand::execute called; s_core={}, server={}", (void*)s_core, (void*)(s_core ? s_core->get_server() : nullptr));

    if (!s_core) {
        spdlog::error("DropAllCommand: No core set!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("DropAllCommand: No local (server) player!");
        return;
    }

    int max_quantity = 0;
    if (args.size() > 1) {
        try {
            max_quantity = std::stoi(args[1]);
            if (max_quantity <= 0) {
                send_console(server->get_player(), "`4[ `bVinProxy `4] `9Quantity must be greater than 0");
                return;
            }
        } catch (...) {
            send_console(server->get_player(), fmt::format("`4[ `bVinProxy `4] `9Invalid quantity: {}", args[1]));
            return;
        }
    }

    
    if (s_running.load()) {
        spdlog::info("DropAllCommand::execute - cancelling existing run");
        
        ++s_generation;
        send_console(server->get_player(), "`0[ `bVinProxy `0] `9dropall cancelled");
        s_running = false;
        return;
    }

    
    spdlog::info("DropAllCommand::execute - starting new run (max_quantity={})", max_quantity);
    s_running = true;
    const std::uint64_t gen = ++s_generation;
    if (max_quantity > 0) {
        send_console(server->get_player(), fmt::format("`0[ `bVinProxy `0] `9dropping up to `w{}`9 of each item..", max_quantity));
    } else {
        send_console(server->get_player(), "`0[ `bVinProxy `0] `9dropping all items..");
    }

    std::thread([gen, max_quantity]() {
        spdlog::info("DropAllCommand thread launched (gen={}, max_quantity={})", gen, max_quantity);
        run_dropall(gen, max_quantity);
    }).detach();
}

void DropAllCommand::run_dropall(std::uint64_t generation, int max_quantity) {
    auto finish = []() {
        DropAllCommand::s_running = false;
    };

    if (!s_core) {
        finish();
        return;
    }

    auto* server = s_core->get_server();
    auto* client = s_core->get_client();
    if (!server || !server->get_player() || !client || !client->get_player()) {
        finish();
        return;
    }

    
    auto& inv_mgr = utils::InventoryManager::get_instance();
    const auto items = inv_mgr.get_items_snapshot();
    spdlog::info("DropAll: inventory snapshot contains {} items", items.size());

    
    
    

    std::size_t dropped_stacks = 0;
    if (items.empty()) {
        spdlog::warn("DropAll: no items to drop");
    }
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

        // Skip empty or 0 amount items (do not skip seeds or any inventory items)
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

            // Step 2: Confirm count in drop dialog
            std::string confirm = fmt::format(
                "action|dialog_return\ndialog_name|drop_item\nitemID|{}|\ncount|{}\n",
                item.id, chunk
            );
            send_generic_text(to_server_player, confirm);

            // Step 3: If dropping the full stack or a 1-quantity item, confirm the last-item warning dialog
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
                    fmt::format("`0[ `bVinProxy `0] `9dropped items (`w{}`9 stacks, up to `w{}`9 each)", dropped_stacks, max_quantity));
            } else {
                send_console(local_player,
                    fmt::format("`0[ `bVinProxy `0] `9dropped all items (`w{}`9 stacks)", dropped_stacks));
            }
        }
    }

    finish();
}

} 

