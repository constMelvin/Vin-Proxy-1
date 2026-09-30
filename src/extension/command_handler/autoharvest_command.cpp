#include "autoharvest_command.hpp"
#include "autofarm_common.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/world_manager.hpp"
#include <chrono>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <spdlog/spdlog.h>

namespace command {
namespace af = command::autofarm;

core::Core* AutoHarvestCommand::s_core = nullptr;
std::atomic<bool> AutoHarvestCommand::s_enabled{false};
std::atomic<std::uint64_t> AutoHarvestCommand::s_generation{0};
std::atomic<int> AutoHarvestCommand::s_seed_id{0};
std::atomic<int> AutoHarvestCommand::s_path_delay{800};
std::atomic<int> AutoHarvestCommand::s_punch_delay{500};

AutoHarvestCommand::AutoHarvestCommand()
    : CommandBase({"autoharvest"}, {}, "Open auto harvest settings", 0) {}
std::unique_ptr<CommandBase> AutoHarvestCommand::clone() const {
    return std::make_unique<AutoHarvestCommand>(*this);
}
void AutoHarvestCommand::set_core(core::Core* core) { s_core = core; }

void AutoHarvestCommand::stop() {
    s_enabled = false;
    s_generation.fetch_add(1);
}

// LuckyProxy events.cpp /harvest: "Auto Harvest Page" dialog
void AutoHarvestCommand::show_dialog(player::Player* player) {
    std::ostringstream d;
    d << "set_default_color|`o\n";
    d << "add_label_with_icon|big|`9Auto Harvest Page|left|5638|\n";
    d << "add_spacer|small|\n";
    d << "add_checkbox|enable_auto_harvest|`2Enable Auto Harvest|" << (s_enabled.load() ? 1 : 0) << "|\n";
    d << af::desc_text("`oToggle this to start or stop automatic harvesting. You can also type /harvest.");
    d << "add_spacer|small|\n";
    const int seed = s_seed_id.load();
    if (seed > 0) {
        d << "add_label_with_icon|small|Current Harvest Item: " << af::item_name(seed) << "|left|" << seed << "|\n";
        d << "add_item_picker|auto_harvest_seed|`2Change Harvest Item|Select Harvest Item From Your Inventory|\n";
    } else {
        d << "add_item_picker|auto_harvest_seed|`2Current Harvest Item: |Select Harvest Item From Your Inventory|\n";
    }
    d << af::tiny_text("`oChoose which item you want the bot to automatically harvest.");
    d << "add_spacer|small|\n";
    d << "add_text_input|auto_harvest_path_delay|`cPathfinding Delay (ms): |" << s_path_delay.load() << "|5|\n";
    d << af::tiny_text("`9Delay between each pathfinding step.");
    d << "add_text_input|auto_harvest_punch_delay|`cPunch Delay (ms): |" << s_punch_delay.load() << "|5|\n";
    d << af::tiny_text("`9Delay between each harvest punch. 1000ms = 1 Second");
    d << "add_spacer|small|\n";
    d << "end_dialog|auto_harvest_page|Cancel|OK|\n";
    af::send_dialog(player, d.str());
}

void AutoHarvestCommand::execute(client::Client*, const std::vector<std::string>&) {
    if (!s_core || !s_core->get_server()) return;
    show_dialog(s_core->get_server()->get_player());
}

void AutoHarvestCommand::handle_dialog_response(player::Player* player, const std::string& raw) {
    TextParse tp{raw};
    int v = 0;
    if (af::parse_int(tp.get("auto_harvest_seed"), v) && v > 0) s_seed_id = v;
    if (af::parse_int(tp.get("auto_harvest_path_delay"), v) && v > 0) s_path_delay = v;
    if (af::parse_int(tp.get("auto_harvest_punch_delay"), v) && v > 0) s_punch_delay = v;

    const bool want = af::checkbox_on(tp, "enable_auto_harvest");
    if (want && !s_enabled.load()) {
        if (s_seed_id.load() <= 0) {
            af::send_console(player, "`4Auto Harvest: select an item first.");
            return;
        }
        start();
        af::send_console(player, "`9Auto Harvest is now `2enabled.");
    } else if (!want && s_enabled.load()) {
        stop();
        af::send_console(player, "`9Auto Harvest is now `4disabled.");
    }
}

void AutoHarvestCommand::toggle(player::Player* player) {
    if (s_enabled.load()) {
        stop();
        af::send_console(player, "`9Auto Harvest is now `4disabled.");
        return;
    }
    if (s_seed_id.load() <= 0) {
        af::send_console(player, "`4Auto Harvest: select an item in /autoharvest first.");
        return;
    }
    if (!af::in_world()) {
        af::send_console(player, "`4Auto Harvest: join a world first.");
        return;
    }
    start();
    af::send_console(player, "`9Auto Harvest is now `2enabled.");
}

void AutoHarvestCommand::start() {
    s_enabled = true;
    const std::uint64_t gen = ++s_generation;
    std::thread([gen] { run(gen); }).detach();
}

// LuckyProxy gt::do_auto_harvest as a worker loop
void AutoHarvestCommand::run(std::uint64_t gen) {
    using clock = std::chrono::steady_clock;
    auto last_path = clock::now();
    auto last_punch = clock::now();
    auto due = [](clock::time_point& last, int ms) {
        auto now = clock::now();
        if (now - last < std::chrono::milliseconds(ms)) return false;
        last = now;
        return true;
    };

    while (s_enabled.load() && gen == s_generation.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (!af::in_world()) continue;

        int px = 0, py = 0;
        if (!af::local_tile_pos(s_core, px, py)) continue;

        const int seed = s_seed_id.load();
        auto& wm = utils::WorldManager::get_instance();
        const int w = static_cast<int>(wm.get_world_width());
        const int h = static_cast<int>(wm.get_world_height());

        int best = INT_MAX, tx = -1, ty = -1;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (af::tile_fg(x, y) != seed) continue;
                const int d = std::abs(px - x) + std::abs(py - y);
                if (d < best) { best = d; tx = x; ty = y; }
            }
        }
        if (tx < 0) continue;

        if (px != tx || py != ty) {
            if (due(last_path, s_path_delay.load())) {
                const bool ok = af::move_to(s_core, tx, ty);
                spdlog::info("[AutoHarvest] walk ({},{}) -> ({},{}) {}", px, py, tx, ty, ok ? "ok" : "FAILED");
            }
        } else if (due(last_punch, s_punch_delay.load())) {
            spdlog::info("[AutoHarvest] punch ({},{})", tx, ty);
            af::punch(s_core, tx, ty);
        }
    }
    spdlog::info("[AutoHarvest] worker stopped gen={}", gen);
}

// ============================================================
// /harvest
// ============================================================
HarvestToggleCommand::HarvestToggleCommand()
    : CommandBase({"harvest"}, {}, "Toggle auto harvest", 0) {}
std::unique_ptr<CommandBase> HarvestToggleCommand::clone() const {
    return std::make_unique<HarvestToggleCommand>(*this);
}
void HarvestToggleCommand::execute(client::Client*, const std::vector<std::string>&) {
    auto* core = AutoHarvestCommand::get_core();
    if (!core || !core->get_server()) return;
    AutoHarvestCommand::toggle(core->get_server()->get_player());
}

}
