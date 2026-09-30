#include "autofarm_command.hpp"
#include "autofarm_common.hpp"
#include "position_command.hpp"
#include "autocollect_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include <chrono>
#include <sstream>
#include <thread>
#include <spdlog/spdlog.h>

namespace command {
namespace af = command::autofarm;

core::Core* AutoFarmCommand::s_core = nullptr;
std::atomic<bool> AutoFarmCommand::s_enabled{false};
std::atomic<std::uint64_t> AutoFarmCommand::s_generation{0};
std::atomic<int> AutoFarmCommand::s_farm_item{0};
std::atomic<int> AutoFarmCommand::s_interval{300};
std::atomic<bool> AutoFarmCommand::s_disable_on_pull{false};
std::atomic<int> AutoFarmCommand::s_collect_wait{500};

static constexpr int MAX_COLLECT_WAIT_MS = 10000;

static constexpr int MIN_INTERVAL_MS = 150;

AutoFarmCommand::AutoFarmCommand()
    : CommandBase({"autofarm"}, {}, "Open auto farm settings", 0) {}
std::unique_ptr<CommandBase> AutoFarmCommand::clone() const {
    return std::make_unique<AutoFarmCommand>(*this);
}
void AutoFarmCommand::set_core(core::Core* core) { s_core = core; }

void AutoFarmCommand::stop() {
    s_enabled = false;
    s_generation.fetch_add(1);
}

// LuckyProxy events.cpp /afarm: "Auto Farm Page" dialog
std::string AutoFarmCommand::build_dialog(bool interval_error) {
    std::ostringstream d;
    d << "set_default_color|`o\n";
    d << "add_label_with_icon|big|`5Auto Farm Page|left|898|\n";
    if (interval_error)
        d << "add_textbox|`4AutoFarm Delay Can't be less than 150ms!|\n";
    d << "add_spacer|small|\n";
    const int item = s_farm_item.load();
    if (item > 0) {
        d << "add_label_with_icon|small|`#Current `5Farmable Item: `5" << af::item_name(item) << "|left|" << item << "|\n";
        d << "add_item_picker|farmID|`#Change Farmable Item|Select Item From Your Inventory|\n";
    } else {
        d << "add_item_picker|farmID|`2Select Farmable Item|Select Item From Your Inventory|\n";
    }
    d << "add_spacer|small|\n";
    d << "add_checkbox|autofrm|`2Enable `#Auto Farming|" << (s_enabled.load() ? 1 : 0) << "|\n";
    d << af::desc_text("`oYou can also type /farm to enable / disable auto farming.");
    d << "add_checkbox|disablekao|`2Auto `#Disable Auto Farm When Pulled|" << (s_disable_on_pull.load() ? 1 : 0) << "|\n";
    d << af::desc_text("`oAutomatically disables auto farming when someone pulls you.");
    d << "add_checkbox|farm_collect|`2Enable `#Auto Collect|" << (AutoCollectCommand::is_enabled() ? 1 : 0) << "|\n";
    d << af::desc_text("`oPicks up the floating items your farming drops. Set its range with /collect.");
    d << "add_text_input|farm_collect_wait|`cCollect Wait (ms): |" << s_collect_wait.load() << "|5|\n";
    d << af::tiny_text("`9Pause after both blocks break so Auto Collect can pick up the drops. "
                       "Only used while Auto Collect is on. 0 = no pause, max " + std::to_string(MAX_COLLECT_WAIT_MS) + ".");
    d << "add_text_input|farminterval|`cAutoFarm Interval (ms): |" << s_interval.load() << "|5|\n";
    d << af::tiny_text("`91000ms = 1 Second");
    d << "end_dialog|farmpage|Cancel|Okey|\n";
    return d.str();
}

void AutoFarmCommand::execute(client::Client*, const std::vector<std::string>&) {
    if (!s_core || !s_core->get_server()) return;
    af::send_dialog(s_core->get_server()->get_player(), build_dialog(false));
}

bool AutoFarmCommand::start(player::Player* player) {
    if (s_farm_item.load() <= 0) {
        af::send_console(player, "`4Auto Farm: select a farmable item in /autofarm first.");
        return false;
    }
    if (!af::in_world()) {
        af::send_console(player, "`4Auto Farm: join a world first.");
        return false;
    }
    if (s_enabled.exchange(true)) return true;
    const std::uint64_t gen = ++s_generation;
    std::thread([gen] { run(gen); }).detach();
    af::send_console(player, "`9Auto Farm is now `2enabled.");
    return true;
}

void AutoFarmCommand::handle_dialog_response(player::Player* player, const std::string& raw) {
    TextParse tp{raw};
    int v = 0;
    if (af::parse_int(tp.get("farmID"), v) && v > 0) s_farm_item = v;
    s_disable_on_pull = af::checkbox_on(tp, "disablekao");
    // Auto collect shares the /collect page's collector and range
    AutoCollectCommand::set_enabled(player, af::checkbox_on(tp, "farm_collect"));
    if (af::parse_int(tp.get("farm_collect_wait"), v) && v >= 0)
        s_collect_wait = std::min(v, MAX_COLLECT_WAIT_MS);

    int interval = s_interval.load();
    if (af::parse_int(tp.get("farminterval"), v)) interval = v;
    if (interval < MIN_INTERVAL_MS) {
        af::send_dialog(player, build_dialog(true));
        return;
    }
    s_interval = interval;

    const bool want = af::checkbox_on(tp, "autofrm");
    if (want) {
        start(player);
    } else if (s_enabled.load()) {
        stop();
        af::send_console(player, "`9Auto Farm is now `4disabled.");
    }
}

void AutoFarmCommand::toggle(player::Player* player) {
    if (s_enabled.load()) {
        stop();
        af::send_console(player, "`9Auto Farm is now `4disabled.");
    } else {
        start(player);
    }
}

void AutoFarmCommand::on_overlay_message(const std::string& msg) {
    if (!s_enabled.load() || !s_disable_on_pull.load()) return;
    if (msg.find("You were pulled by") == std::string::npos) return;
    stop();
    s_disable_on_pull = false;
    if (s_core && s_core->get_server())
        af::send_console(s_core->get_server()->get_player(),
                         "`9Auto Farm is now `4disabled `9(pulled).");
}

static bool farm_running(std::atomic<bool>& enabled, std::atomic<std::uint64_t>& generation, std::uint64_t gen) {
    return enabled.load() && gen == generation.load() && af::in_world();
}

// Farms the two tiles in front of the player (1 = near, 2 = far, facing direction)
// as one ordered cycle. The server will not let you place or punch a tile that sits
// behind a filled tile, so the order matters:
//   Place phase: far tile first (while the near tile is still empty), then the near tile.
//   Break phase: near tile first, then the far tile once the near tile is clear.
// Two independent place/punch loops kept hitting the far tile while the near one was
// filled, so it was rejected and the far block was never placed or broken.
void AutoFarmCommand::run(std::uint64_t gen) {
    enum class Phase { Place, Break };
    constexpr int STALL_LIMIT = 12;   // actions in a row with no change in either tile

    Phase phase = Phase::Place;
    int stall = 0;
    int last_near = -1, last_far = -1;

    auto switch_phase = [&](Phase next, const char* why) {
        phase = next;
        stall = 0;
        spdlog::info("[AutoFarm] -> {} phase ({})", next == Phase::Place ? "PLACE" : "BREAK", why);
    };

    while (farm_running(s_enabled, s_generation, gen)) {
        int px = 0, py = 0;
        if (!af::local_tile_pos(s_core, px, py)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        const int dir = af::facing_left() ? -1 : 1;
        const int near_x = px + dir;
        const int far_x = px + dir * 2;
        const int near_fg = af::tile_fg(near_x, py);
        const int far_fg = af::tile_fg(far_x, py);
        const bool near_full = near_fg != 0;
        const bool far_full = far_fg != 0;

        // Nothing changed since the last action: the server ignored it (or is still answering)
        if (near_fg == last_near && far_fg == last_far) ++stall; else stall = 0;
        last_near = near_fg;
        last_far = far_fg;
        if (stall >= STALL_LIMIT) {
            switch_phase(phase == Phase::Place ? Phase::Break : Phase::Place, "stalled, no response");
            continue;
        }

        if (phase == Phase::Place) {
            if (near_full && far_full)  { switch_phase(Phase::Break, "both placed"); continue; }
            if (near_full && !far_full) { switch_phase(Phase::Break, "far blocked by near"); continue; }
            const int tx = far_full ? near_x : far_x;     // far first, then near
            af::place(s_core, s_farm_item.load(), tx, py);
            // Same tile animation as /pos1 (ring VFX + tile highlight) at the put location
            if (s_core && s_core->get_server())
                PositionCommand::highlight_tile(s_core->get_server()->get_player(), tx, py);
            spdlog::info("[AutoFarm] place item {} at ({},{}) near={} far={}", s_farm_item.load(), tx, py, near_fg, far_fg);
        } else {
            if (!near_full && !far_full) {
                // Let Auto Collect pick up the drops before the next place/break round
                const int wait_ms = s_collect_wait.load();
                if (AutoCollectCommand::is_enabled() && wait_ms > 0) {
                    spdlog::info("[AutoFarm] waiting {} ms for Auto Collect", wait_ms);
                    for (int waited = 0; waited < wait_ms && farm_running(s_enabled, s_generation, gen); waited += 50)
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                switch_phase(Phase::Place, "both broken");
                continue;
            }
            const int tx = near_full ? near_x : far_x;    // near first, then far
            af::punch_legit(s_core, tx, py);   // fist swing regardless of the selected hotbar item
            // Same /pos1 tile animation at the breaking spot as at the put spot
            if (s_core && s_core->get_server())
                PositionCommand::highlight_tile(s_core->get_server()->get_player(), tx, py);
            spdlog::info("[AutoFarm] punch ({},{}) near={} far={}", tx, py, near_fg, far_fg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(s_interval.load()));
    }
    if (gen == s_generation.load()) s_enabled = false;
    spdlog::info("[AutoFarm] worker stopped gen={}", gen);
}

// ============================================================
// /farm
// ============================================================
FarmToggleCommand::FarmToggleCommand()
    : CommandBase({"farm"}, {}, "Toggle auto farm", 0) {}
std::unique_ptr<CommandBase> FarmToggleCommand::clone() const {
    return std::make_unique<FarmToggleCommand>(*this);
}
void FarmToggleCommand::execute(client::Client*, const std::vector<std::string>&) {
    auto* core = AutoFarmCommand::get_core();
    if (!core || !core->get_server()) return;
    AutoFarmCommand::toggle(core->get_server()->get_player());
}

}
