#include "drop_currency_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../player/player.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/inventory_manager.hpp"
#include "../../utils/player_tracker.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <regex>
#include <algorithm>
#include <vector>
#include <string>

namespace command {

std::atomic<bool> DropCurrencyState::s_dropping{false};

core::Core* DropWLCommand::s_core  = nullptr;
core::Core* DropDLCommand::s_core  = nullptr;
core::Core* DropBGLCommand::s_core = nullptr;
core::Core* VisualDropCommand::s_core = nullptr;

static void send_generic(player::Player* p, const std::string& raw) {
    if (!p) return;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(raw, false);
    p->send_packet(bs.get_data(), 0);
}

static void send_overlay(core::Core* core, const std::string& text) {
    player::Player* p = nullptr;
    if (core && core->get_server()) {
        p = core->get_server()->get_player();
    }
    if (!p) {
        spdlog::warn("[OVERLAY] Cannot send OnTextOverlay: server player is null");
        return;
    }

    packet::Variant var{};
    var.add("OnTextOverlay");
    var.add(text);
    std::vector<std::byte> ext_data = var.serialize();

    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = static_cast<uint32_t>(-1);
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());
    p->send_packet(bs.get_data(), 0);
    spdlog::info("[OVERLAY] Sent OnTextOverlay to client: {}", text);
}

static void send_console(core::Core* core, const std::string& text, bool add_prefix = true) {
    if (!core || !core->get_server() || !core->get_server()->get_player()) return;
    utils::PacketUtils::send_chat_message(core->get_server()->get_player(), text, add_prefix);
}

static std::string get_local_player_name() {
    auto lp = utils::PlayerTracker::get_instance().get_local_player();
    std::string name = lp.name;
    if (name.empty() && lp.netID > 0) {
        name = utils::PlayerTracker::get_instance().get_player_by_netid(lp.netID).name;
    }
    std::string clean;
    for (size_t i = 0; i < name.size(); ++i) {
        if (name[i] == '`') {
            if (i + 1 < name.size()) ++i;
            continue;
        }
        clean.push_back(name[i]);
    }
    return clean;
}

static void send_activate_packet(player::Player* to_server_player, uint16_t item_id) {
    if (!to_server_player) return;
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_ITEM_ACTIVATE_REQUEST;
    pkt.net_id = static_cast<uint32_t>(-1);
    pkt.flags.value = packet::PACKET_FLAG_NONE;
    pkt.data_size = 0;
    pkt.decompressed_data_size = 0;

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write(static_cast<int32_t>(item_id));

    to_server_player->send_packet(bs.get_data(), 0);
    spdlog::info("DropCurrency: Sent PACKET_ITEM_ACTIVATE_REQUEST for item ID {}", item_id);
}

struct ParsedDropInput {
    int bgl = 0;
    int dl  = 0;
    int wl  = 0;
    int raw_val = 0;
    bool has_explicit_unit = false;
    bool is_valid = false;
};

static ParsedDropInput parse_drop_args(const std::vector<std::string>& args) {
    ParsedDropInput res;
    if (args.size() < 2) return res;

    std::string text;
    for (size_t i = 1; i < args.size(); ++i) {
        if (!text.empty()) text += " ";
        text += args[i];
    }
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    static const std::regex unit_regex(R"((\d+)\s*(bgl|dl|wl)s?)");
    auto it = std::sregex_iterator(text.begin(), text.end(), unit_regex);
    auto end = std::sregex_iterator();
    bool found_unit = false;

    for (; it != end; ++it) {
        found_unit = true;
        int val = 0;
        try { val = std::stoi((*it)[1].str()); } catch (...) {}
        std::string unit = (*it)[2].str();
        if (unit == "bgl") res.bgl += val;
        else if (unit == "dl") res.dl += val;
        else if (unit == "wl") res.wl += val;
    }

    if (found_unit) {
        res.has_explicit_unit = true;
        res.is_valid = (res.bgl > 0 || res.dl > 0 || res.wl > 0);
        return res;
    }

    static const std::regex num_regex(R"((\d+))");
    std::smatch m;
    if (std::regex_search(text, m, num_regex)) {
        try {
            res.raw_val = std::stoi(m[1].str());
            if (res.raw_val > 0) res.is_valid = true;
        } catch (...) {}
    }

    return res;
}

static void execute_drop_single(core::Core* core,
                                const std::vector<std::string>& args,
                                uint16_t item_id,
                                const std::string& item_name,
                                const std::string& display_name)
{
    if (!core || !core->get_client() || !core->get_client()->get_player()) {
        send_overlay(core, fmt::format("`4Drop{}: not connected!", display_name));
        send_console(core, fmt::format("`4Drop{}: not connected!", display_name));
        return;
    }

    if (DropCurrencyState::s_dropping.load()) {
        send_overlay(core, "`4Drop already in progress!");
        send_console(core, "`4Drop already in progress!");
        return;
    }

    if (args.size() < 2) {
        std::string usage = fmt::format("`4Usage: /{} <amount>", (item_id == 242 ? "dw" : "dbgl"));
        send_overlay(core, usage);
        send_console(core, usage);
        return;
    }

    int amount = 0;
    try { amount = std::stoi(args[1]); } catch (...) {}
    if (amount <= 0) {
        std::string msg = "`4Invalid amount!";
        send_overlay(core, msg);
        send_console(core, msg);
        return;
    }

    auto& inv = utils::InventoryManager::get_instance();
    int have = inv.get_item_count(item_id);

    if (have < amount || have <= 0) {
        std::string msg;
        if (have <= 0) {
            msg = fmt::format("`4You don't have any {}!", item_name);
        } else {
            msg = fmt::format("`4You don't have enough {}! (Have: {}, Need: {})", item_name, have, amount);
        }
        send_overlay(core, msg);
        send_console(core, msg);
        spdlog::warn("[DROP-CHECK] Blocked /{} have={} need={}", display_name, have, amount);
        return;
    }

    DropCurrencyState::s_dropping = true;

    std::thread([core, item_id, amount, display_name]() {
        auto& inv = utils::InventoryManager::get_instance();
        player::Player* client_player = core->get_client()->get_player();
        if (!client_player) {
            DropCurrencyState::s_dropping = false;
            return;
        }

        int remaining = amount;
        while (remaining > 0) {
            int chunk = std::min(remaining, 100);

            send_generic(client_player, fmt::format("action|drop\nitemID|{}|", item_id));
            std::this_thread::sleep_for(std::chrono::milliseconds(130));

            send_generic(client_player,
                fmt::format("action|dialog_return\ndialog_name|drop_item\nitemID|{}|\ncount|{}|\nbuttonClicked|yes",
                    item_id, chunk));

            inv.remove_item(item_id, static_cast<uint8_t>(chunk));
            inv.record_pending_drop(item_id, static_cast<uint8_t>(chunk));

            remaining -= chunk;
            if (remaining > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(130));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        DropCurrencyState::s_dropping = false;

        std::string tag = utils::PlayerTracker::get_instance().get_player_tag(0, get_local_player_name());
        std::string col = (item_id == 7188) ? "`e" : "`#";
        send_console(core, fmt::format("{} `9Dropped {}{} {}``", tag, col, amount, display_name), false);
        spdlog::info("DropCurrency: Successfully dropped {} {}", amount, display_name);
    }).detach();
}

struct DropPlanItem {
    uint16_t item_id;
    int count;
};

static void execute_drop_dd(core::Core* core, const std::vector<std::string>& args) {
    if (!core || !core->get_client() || !core->get_client()->get_player()) {
        send_overlay(core, "`4Drop: not connected!");
        send_console(core, "`4Drop: not connected!");
        return;
    }

    if (DropCurrencyState::s_dropping.load()) {
        send_overlay(core, "`4Drop already in progress!");
        send_console(core, "`4Drop already in progress!");
        return;
    }

    ParsedDropInput parsed = parse_drop_args(args);
    if (!parsed.is_valid) {
        std::string usage = "`4Usage: /dd <amount> (e.g. /dd 150 = 1dl 50wl, /dd 100 = 1dl)";
        send_overlay(core, usage);
        send_console(core, usage);
        return;
    }

    auto& inv = utils::InventoryManager::get_instance();
    int have_bgl = inv.get_item_count(7188);
    int have_dl  = inv.get_item_count(1796);
    int have_wl  = inv.get_item_count(242);

    int total_wl_owned = have_wl + have_dl * 100 + have_bgl * 10000;

    int target_bgl = 0;
    int target_dl  = 0;
    int target_wl  = 0;

    if (parsed.has_explicit_unit) {
        int total_needed_wl = parsed.wl + parsed.dl * 100 + parsed.bgl * 10000;
        target_bgl = total_needed_wl / 10000;
        target_dl  = (total_needed_wl % 10000) / 100;
        target_wl  = total_needed_wl % 100;
    } else {
        const int N = parsed.raw_val;
        // In /dd, amounts >= 100 are interpreted in WL: 100wl = 1dl, 100dl = 1bgl
        // Example: /dd 150 = 150wl -> 1dl and 50wl; /dd 100 = 1dl; /dd 250 = 2dl and 50wl
        if (N >= 100) {
            target_bgl = N / 10000;
            target_dl  = (N % 10000) / 100;
            target_wl  = N % 100;
        } else {
            // N < 100: /dd 1 -> 1 DL; /dd 5 -> 5 DL
            if (have_dl >= N) {
                target_bgl = 0;
                target_dl  = N;
                target_wl  = 0;
            } else if (have_wl >= N) {
                target_bgl = 0;
                target_dl  = 0;
                target_wl  = N;
            } else {
                target_bgl = 0;
                target_dl  = N;
                target_wl  = 0;
            }
        }
    }

    int total_wl_needed = target_wl + target_dl * 100 + target_bgl * 10000;

    // Check if player has enough total lock balance
    if (total_wl_owned < total_wl_needed || total_wl_needed <= 0) {
        std::string msg;
        if (total_wl_owned <= 0) {
            msg = "`4You don't have any locks!";
        } else {
            if (target_bgl > 0 && target_dl > 0) {
                msg = fmt::format("`4You don't have enough locks! (Need: {} BGL {} DL, Have: {} BGL, {} DL, {} WL)",
                    target_bgl, target_dl, have_bgl, have_dl, have_wl);
            } else if (target_dl > 0 && target_wl > 0) {
                msg = fmt::format("`4You don't have enough locks! (Need: {} DL {} WL, Have: {} DL, {} WL)",
                    target_dl, target_wl, have_dl, have_wl);
            } else if (target_bgl > 0) {
                msg = fmt::format("`4You don't have enough locks! (Need: {} BGL, Have: {} BGL, {} DL, {} WL)",
                    target_bgl, have_bgl, have_dl, have_wl);
            } else if (target_dl > 0) {
                msg = fmt::format("`4You don't have enough locks! (Need: {} DL, Have: {} DL, {} WL)",
                    target_dl, have_dl, have_wl);
            } else {
                msg = fmt::format("`4You don't have enough locks! (Need: {} WL, Have: {} WL)",
                    target_wl, have_wl);
            }
        }
        send_overlay(core, msg);
        send_console(core, msg);
        spdlog::warn("[DROP-CHECK] Blocked /dd: total_wl_owned={} < total_wl_needed={}", total_wl_owned, total_wl_needed);
        return;
    }

    DropCurrencyState::s_dropping = true;

    std::thread([core, target_bgl, target_dl, target_wl]() mutable {
        auto& inv = utils::InventoryManager::get_instance();
        player::Player* client_player = core->get_client()->get_player();
        if (!client_player) {
            DropCurrencyState::s_dropping = false;
            return;
        }

        int cur_bgl = inv.get_item_count(7188);
        int cur_dl  = inv.get_item_count(1796);
        int cur_wl  = inv.get_item_count(242);

        int drop_bgl = target_bgl;
        int drop_dl  = target_dl;
        int drop_wl  = target_wl;

        // 1. If we need more BGL than we have, convert shortfall to DL (1 BGL = 100 DL)
        if (cur_bgl < drop_bgl) {
            int missing_bgl = drop_bgl - cur_bgl;
            drop_bgl = cur_bgl;
            drop_dl += missing_bgl * 100;
        }

        // 2. If we need more DL than we have:
        // Try breaking extra BGL into 100 DL first
        if (cur_dl < drop_dl) {
            int missing_dl = drop_dl - cur_dl;
            int bgl_to_break = (missing_dl + 99) / 100;
            if (cur_bgl - drop_bgl >= bgl_to_break) {
                for (int b = 0; b < bgl_to_break; ++b) {
                    send_activate_packet(client_player, 7188);
                    inv.remove_item(7188, 1);
                    inv.add_item(1796, 100);
                    cur_bgl--;
                    cur_dl += 100;
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
            } else if (cur_wl >= (drop_wl + missing_dl * 100)) {
                drop_dl = cur_dl;
                drop_wl += missing_dl * 100;
            }
        }

        // 3. If we need more WL than we have:
        // Try breaking extra DL into 100 WL
        if (cur_wl < drop_wl) {
            int missing_wl = drop_wl - cur_wl;
            int dl_to_break = (missing_wl + 99) / 100;
            if (cur_dl - drop_dl >= dl_to_break) {
                for (int d = 0; d < dl_to_break; ++d) {
                    send_activate_packet(client_player, 1796);
                    inv.remove_item(1796, 1);
                    inv.add_item(242, 100);
                    cur_dl--;
                    cur_wl += 100;
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
            }
        }

        std::vector<DropPlanItem> plans;
        if (drop_bgl > 0) plans.push_back({ 7188, drop_bgl });
        if (drop_dl > 0)  plans.push_back({ 1796, drop_dl });
        if (drop_wl > 0)  plans.push_back({ 242, drop_wl });

        for (size_t i = 0; i < plans.size(); ++i) {
            const auto& plan = plans[i];
            int remaining = plan.count;
            while (remaining > 0) {
                int chunk = std::min(remaining, 100);

                send_generic(client_player, fmt::format("action|drop\nitemID|{}|", plan.item_id));
                std::this_thread::sleep_for(std::chrono::milliseconds(130));

                send_generic(client_player,
                    fmt::format("action|dialog_return\ndialog_name|drop_item\nitemID|{}|\ncount|{}|\nbuttonClicked|yes",
                        plan.item_id, chunk));

                inv.remove_item(plan.item_id, static_cast<uint8_t>(chunk));
                inv.record_pending_drop(plan.item_id, static_cast<uint8_t>(chunk));

                remaining -= chunk;
                if (remaining > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(130));
                }
            }
            if (i + 1 < plans.size()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(130));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        DropCurrencyState::s_dropping = false;

        std::vector<std::string> parts;
        if (drop_bgl > 0) parts.push_back(fmt::format("`e{} BGL``", drop_bgl));
        if (drop_dl > 0)  parts.push_back(fmt::format("`!{} DL``", drop_dl));
        if (drop_wl > 0)  parts.push_back(fmt::format("`#{} WL``", drop_wl));

        std::string summary;
        for (size_t idx = 0; idx < parts.size(); ++idx) {
            if (idx > 0) summary += " `9and ";
            summary += parts[idx];
        }
        std::string tag = utils::PlayerTracker::get_instance().get_player_tag(0, get_local_player_name());
        send_console(core, fmt::format("{} `9Dropped {}", tag, summary), false);
        spdlog::info("DropCurrency: Successfully dropped {}", summary);
    }).detach();
}

DropWLCommand::DropWLCommand() : CommandBase(
    {"dw", "dwl"}, {"<amount>"}, "Drop World Locks. /dw <amount>", 1) {}

std::unique_ptr<CommandBase> DropWLCommand::clone() const {
    return std::make_unique<DropWLCommand>(*this);
}

void DropWLCommand::set_core(core::Core* core) { s_core = core; }

void DropWLCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    s_core = core;
    execute(client, args);
}

void DropWLCommand::execute(client::Client* , const std::vector<std::string>& args) {
    execute_drop_single(s_core, args, 242, "World Locks", "WL");
}

DropDLCommand::DropDLCommand() : CommandBase(
    {"dd", "ddl"}, {"<amount>"}, "Drop Diamond Locks. /dd <amount>", 1) {}

std::unique_ptr<CommandBase> DropDLCommand::clone() const {
    return std::make_unique<DropDLCommand>(*this);
}

void DropDLCommand::set_core(core::Core* core) { s_core = core; }

void DropDLCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    s_core = core;
    execute(client, args);
}

void DropDLCommand::execute(client::Client* , const std::vector<std::string>& args) {
    execute_drop_dd(s_core, args);
}

DropBGLCommand::DropBGLCommand() : CommandBase(
    {"dbgl"}, {"<amount>"}, "Drop Blue Gem Locks. /dbgl <amount>", 1) {}

std::unique_ptr<CommandBase> DropBGLCommand::clone() const {
    return std::make_unique<DropBGLCommand>(*this);
}

void DropBGLCommand::set_core(core::Core* core) { s_core = core; }

void DropBGLCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    s_core = core;
    execute(client, args);
}

void DropBGLCommand::execute(client::Client* , const std::vector<std::string>& args) {
    execute_drop_single(s_core, args, 7188, "Blue Gem Locks", "BGL");
}

VisualDropCommand::VisualDropCommand() : CommandBase(
    {"dbglvis"}, {"<amount>"}, "Visual drop Blue Gem Locks (no inventory loss). /dbglvis <amount>", 1) {}

std::unique_ptr<CommandBase> VisualDropCommand::clone() const {
    return std::make_unique<VisualDropCommand>(*this);
}

void VisualDropCommand::set_core(core::Core* core) { s_core = core; }

void VisualDropCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) return;
    auto *player = client->get_player();
    if (args.size() < 2) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /dbglvis <amount>");
        return;
    }
    int amount = 0;
    try { amount = std::stoi(args[1]); } catch (...) {}
    if (amount <= 0) {
        utils::PacketUtils::send_chat_message(player, "`4Invalid amount!");
        return;
    }

    if (!s_core || !s_core->get_client() || !s_core->get_client()->get_player()) {
        utils::PacketUtils::send_chat_message(player, "`4VisualDrop: not connected!");
        return;
    }
    player::Player* client_player = s_core->get_client()->get_player();

    DropCurrencyState::s_dropping = true;
    std::thread([client_player, amount]() {
        for (int i = 0; i < amount; ++i) {
            send_generic(client_player,
                fmt::format("action|drop\n|itemID|{}|", 7188));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        DropCurrencyState::s_dropping = false;
    }).detach();
    spdlog::info("VisualDrop: sent {} visual drops of itemID=7188", amount);
}

} 
 
