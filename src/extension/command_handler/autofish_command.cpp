#include "autofish_command.hpp"
#include "autofarm_common.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include <chrono>
#include <sstream>
#include <thread>
#include <spdlog/spdlog.h>

namespace command {
namespace af = command::autofarm;

core::Core* AutoFishCommand::s_core = nullptr;
std::atomic<bool> AutoFishCommand::s_enabled{false};
std::atomic<std::uint64_t> AutoFishCommand::s_generation{0};
std::atomic<int> AutoFishCommand::s_bait_id{0};
std::atomic<bool> AutoFishCommand::s_char_right{false};
std::atomic<bool> AutoFishCommand::s_char_left{false};

AutoFishCommand::AutoFishCommand()
    : CommandBase({"autofish"}, {}, "Open auto fish settings", 0) {}
std::unique_ptr<CommandBase> AutoFishCommand::clone() const {
    return std::make_unique<AutoFishCommand>(*this);
}
void AutoFishCommand::set_core(core::Core* core) { s_core = core; }

void AutoFishCommand::stop() {
    s_enabled = false;
    s_generation.fetch_add(1);
}

// LuckyProxy events.cpp /afish: "Auto Fish Options Page" dialog
void AutoFishCommand::show_dialog(player::Player* player) {
    std::ostringstream d;
    d << "set_default_color|`o\n";
    d << "add_label_with_icon|big|`9Auto Fish Options Page|left|3436|\n";
    d << "add_text_input|farmid|`cBait ID: |" << s_bait_id.load() << "|5|\n";
    d << af::tiny_text("`9How to find the bait ID? Type /find [your bait] to see its item ID.");
    d << "add_checkbox|baitkiri|`9Your Character On Right|" << (s_char_right.load() ? 1 : 0) << "|\n";
    d << af::desc_text("`oThe side your character is facing. (Enable / disable the checkbox.)");
    d << "add_checkbox|baitkanan|`9Your Character On Left|" << (s_char_left.load() ? 1 : 0) << "|\n";
    d << af::desc_text("`oThe side your character is facing. (Enable / disable the checkbox.)");
    d << "add_textbox|`9First write /fish, then press the bait on the water and stay AFK.|\n";
    d << af::tiny_text("`9Type /fish again if you want to disable it.");
    d << "add_quick_exit|\n";
    d << "end_dialog|autofish_page|Cancel|OK|\n";
    af::send_dialog(player, d.str());
}

void AutoFishCommand::execute(client::Client*, const std::vector<std::string>&) {
    if (!s_core || !s_core->get_server()) return;
    show_dialog(s_core->get_server()->get_player());
}

void AutoFishCommand::handle_dialog_response(player::Player*, const std::string& raw) {
    TextParse tp{raw};
    int v = 0;
    if (af::parse_int(tp.get("farmid"), v) && v > 0) s_bait_id = v;
    s_char_right = af::checkbox_on(tp, "baitkiri");
    s_char_left = af::checkbox_on(tp, "baitkanan");
}

void AutoFishCommand::toggle(player::Player* player) {
    if (s_enabled.load()) {
        stop();
        af::send_console(player, "`2[FISH`2]`w: `4Disabled `9Auto Fish");
        return;
    }
    if (s_bait_id.load() <= 0) {
        af::send_console(player, "`4Auto Fish: set a bait id in /autofish first.");
        return;
    }
    s_enabled = true;
    ++s_generation;
    af::send_console(player, "`2[FISH`2]`w: `2Enabled `9Auto Fish");
}

// LuckyProxy OnConsoleMessage handler: after "You caught " re-cast the bait
// one tile diagonally below the player, on the side they are facing.
void AutoFishCommand::on_console_message(const std::string& msg) {
    if (!s_enabled.load() || msg.find("You caught ") == std::string::npos) return;
    if (!af::in_world() || s_bait_id.load() <= 0) return;

    const std::uint64_t gen = s_generation.load();
    std::thread([gen] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!s_enabled.load() || gen != s_generation.load()) return;
        int px = 0, py = 0;
        if (!af::local_tile_pos(s_core, px, py)) return;
        const int dir = af::facing_left() ? -1 : 1;
        spdlog::info("[AutoFish] caught -> recast bait {} at ({},{})", s_bait_id.load(), px + dir, py + 1);
        af::place(s_core, s_bait_id.load(), px + dir, py + 1);
    }).detach();
}

// ============================================================
// /fish
// ============================================================
FishToggleCommand::FishToggleCommand()
    : CommandBase({"fish"}, {}, "Toggle auto fish", 0) {}
std::unique_ptr<CommandBase> FishToggleCommand::clone() const {
    return std::make_unique<FishToggleCommand>(*this);
}
void FishToggleCommand::execute(client::Client*, const std::vector<std::string>&) {
    auto* core = AutoFishCommand::get_core();
    if (!core || !core->get_server()) return;
    AutoFishCommand::toggle(core->get_server()->get_player());
}

}
