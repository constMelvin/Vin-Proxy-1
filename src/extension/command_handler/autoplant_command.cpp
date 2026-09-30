#include "autoplant_command.hpp"
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

core::Core* AutoPlantCommand::s_core = nullptr;
std::atomic<bool> AutoPlantCommand::s_enabled{false};
std::atomic<std::uint64_t> AutoPlantCommand::s_generation{0};
std::atomic<int> AutoPlantCommand::s_seed_id{0};
std::atomic<int> AutoPlantCommand::s_target_block{0};
std::atomic<int> AutoPlantCommand::s_path_delay{800};
std::atomic<int> AutoPlantCommand::s_place_delay{500};

AutoPlantCommand::AutoPlantCommand()
    : CommandBase({"autoplant"}, {}, "Open auto plant settings", 0) {}
std::unique_ptr<CommandBase> AutoPlantCommand::clone() const {
    return std::make_unique<AutoPlantCommand>(*this);
}
void AutoPlantCommand::set_core(core::Core* core) { s_core = core; }

void AutoPlantCommand::stop() {
    s_enabled = false;
    s_generation.fetch_add(1);
}

// LuckyProxy events.cpp /plant: "Auto Plant Page" dialog
void AutoPlantCommand::show_dialog(player::Player* player) {
    std::ostringstream d;
    d << "set_default_color|`o\n";
    d << "add_label_with_icon|big|`#Auto Plant Page|left|5638|\n";
    d << "add_spacer|small|\n";
    d << "add_checkbox|enable_auto_plant|`2Enable Auto Plant|" << (s_enabled.load() ? 1 : 0) << "|\n";
    d << af::desc_text("`oToggle this to start or stop automatic planting. You can also type /plant.");
    d << "add_spacer|small|\n";

    const int seed = s_seed_id.load();
    if (seed > 0) {
        d << "add_label_with_icon|small|Current Seed : " << af::item_name(seed) << "|left|" << seed << "|\n";
        d << "add_item_picker|plantSeedID|`2Change Seed|Select a Different Seed From Your Inventory|\n";
    } else {
        d << "add_item_picker|plantSeedID|`2Select Seed to Plant|Select a Seed From Your Inventory|\n";
    }
    d << af::tiny_text("`oChoose which seed will be planted.");
    d << "add_spacer|small|\n";

    const int block = s_target_block.load();
    if (block > 0) {
        d << "add_label_with_icon|small|Target Block : " << af::item_name(block) << "|left|" << block << "|\n";
        d << "add_item_picker|plantTargetBlock|`2Change Target Block|Select a Different Block|\n";
    } else {
        d << "add_item_picker|plantTargetBlock|`2Select Target Block|Select Block to Plant On|\n";
    }
    d << af::tiny_text("`oThe block on which seeds will be planted.");
    d << "add_spacer|small|\n";

    d << "add_text_input|auto_plant_path_delay|`cPathfinding Delay (ms): |" << s_path_delay.load() << "|5|\n";
    d << af::tiny_text("`9Delay between each pathfinding step.");
    d << "add_text_input|auto_plant_place_delay|`cPlant Delay (ms): |" << s_place_delay.load() << "|5|\n";
    d << af::tiny_text("`9Delay between each planting action. 1000ms = 1 Second");
    d << "add_spacer|small|\n";
    d << "end_dialog|auto_plant_page|Cancel|OK|\n";
    af::send_dialog(player, d.str());
}

void AutoPlantCommand::execute(client::Client*, const std::vector<std::string>&) {
    if (!s_core || !s_core->get_server()) return;
    show_dialog(s_core->get_server()->get_player());
}

void AutoPlantCommand::handle_dialog_response(player::Player* player, const std::string& raw) {
    TextParse tp{raw};
    int v = 0;
    if (af::parse_int(tp.get("plantSeedID"), v) && v > 0) s_seed_id = v;
    if (af::parse_int(tp.get("plantTargetBlock"), v) && v > 0) s_target_block = v;
    if (af::parse_int(tp.get("auto_plant_path_delay"), v) && v > 0) s_path_delay = v;
    if (af::parse_int(tp.get("auto_plant_place_delay"), v) && v > 0) s_place_delay = v;

    const bool want = af::checkbox_on(tp, "enable_auto_plant");
    if (want && !s_enabled.load()) {
        if (s_seed_id.load() <= 0 || s_target_block.load() <= 0) {
            af::send_console(player, "`4Auto Plant: select a seed and a target block first.");
            return;
        }
        start();
        af::send_console(player, "`9Auto Plant is now `2enabled.");
    } else if (!want && s_enabled.load()) {
        stop();
        af::send_console(player, "`9Auto Plant is now `4disabled.");
    }
}

void AutoPlantCommand::toggle(player::Player* player) {
    if (s_enabled.load()) {
        stop();
        af::send_console(player, "`9Auto Plant is now `4disabled.");
        return;
    }
    if (s_seed_id.load() <= 0 || s_target_block.load() <= 0) {
        af::send_console(player, "`4Auto Plant: select a seed and a target block in /autoplant first.");
        return;
    }
    if (!af::in_world()) {
        af::send_console(player, "`4Auto Plant: join a world first.");
        return;
    }
    start();
    af::send_console(player, "`9Auto Plant is now `2enabled.");
}

void AutoPlantCommand::start() {
    s_enabled = true;
    const std::uint64_t gen = ++s_generation;
    std::thread([gen] { run(gen); }).detach();
}

// LuckyProxy gt::do_auto_plant as a worker loop
void AutoPlantCommand::run(std::uint64_t gen) {
    using clock = std::chrono::steady_clock;
    auto last_path = clock::now();
    auto last_place = clock::now();
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

        const int block = s_target_block.load();
        auto& wm = utils::WorldManager::get_instance();
        const int w = static_cast<int>(wm.get_world_width());
        const int h = static_cast<int>(wm.get_world_height());

        // nearest empty tile that sits directly on the target block
        int best = INT_MAX, tx = -1, ty = -1;
        for (int y = 0; y + 1 < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (af::tile_fg(x, y) != 0) continue;
                if (af::tile_fg(x, y + 1) != block) continue;
                const int d = std::abs(px - x) + std::abs(py - y);
                if (d < best) { best = d; tx = x; ty = y; }
            }
        }
        if (tx < 0) continue;

        if (px != tx || py != ty) {
            if (due(last_path, s_path_delay.load())) {
                const bool ok = af::move_to(s_core, tx, ty);
                spdlog::info("[AutoPlant] walk ({},{}) -> ({},{}) {}", px, py, tx, ty, ok ? "ok" : "FAILED");
            }
        } else if (due(last_place, s_place_delay.load())) {
            spdlog::info("[AutoPlant] plant seed {} at ({},{})", s_seed_id.load(), tx, ty);
            af::place(s_core, s_seed_id.load(), tx, ty);
        }
    }
    spdlog::info("[AutoPlant] worker stopped gen={}", gen);
}

// ============================================================
// /plant
// ============================================================
PlantToggleCommand::PlantToggleCommand()
    : CommandBase({"plant"}, {}, "Toggle auto plant", 0) {}
std::unique_ptr<CommandBase> PlantToggleCommand::clone() const {
    return std::make_unique<PlantToggleCommand>(*this);
}
void PlantToggleCommand::execute(client::Client*, const std::vector<std::string>&) {
    auto* core = AutoPlantCommand::get_core();
    if (!core || !core->get_server()) return;
    AutoPlantCommand::toggle(core->get_server()->get_player());
}

}
