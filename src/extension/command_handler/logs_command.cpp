#include "logs_command.hpp"
#include "lucky_common.hpp"
#include "extended_commands.hpp"
#include "../../utils/dialog.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/text_parse.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace command {

namespace {

constexpr size_t kMaxLogs = 300;
constexpr size_t kProxyTabShow = 10;

void append_filtered(utils::Dialog& dlg, const std::vector<std::string>& logs, const std::string& query) {
    const std::string q = lucky::lower_no_codes(query);
    bool any = false;
    for (const auto& text : logs) {
        if (!q.empty() && lucky::lower_no_codes(text).find(q) == std::string::npos) continue;
        dlg.smalltext(text);
        any = true;
    }
    if (!any) dlg.smalltext("`oNo logs found.");
}

} // namespace

std::mutex LogsCommand::s_mutex;
bool LogsCommand::s_roulette_enabled = true;
std::vector<std::string> LogsCommand::s_drop_logs;
std::vector<std::string> LogsCommand::s_collect_logs;
std::vector<std::string> LogsCommand::s_roulette_logs;
std::string LogsCommand::s_search;

LogsCommand::LogsCommand() : CommandBase({"logs"}, {}, "World logs information", 0) {}
std::unique_ptr<CommandBase> LogsCommand::clone() const { return std::make_unique<LogsCommand>(*this); }
void LogsCommand::execute(client::Client*, const std::vector<std::string>&) { show_menu(); }

// ------------------------------------------------------------
// Recording
// ------------------------------------------------------------
void LogsCommand::push(std::vector<std::string>& logs, std::string line) {
    logs.push_back(std::move(line));
    if (logs.size() > kMaxLogs) logs.erase(logs.begin());
}

void LogsCommand::on_world_load() {
    spdlog::debug("[Logs] Cleared world logs");
    std::lock_guard<std::mutex> lock(s_mutex);
    s_drop_logs.clear();
    s_collect_logs.clear();
    s_roulette_logs.clear();
}

void LogsCommand::on_drop(int32_t net_id, uint16_t item_id, uint32_t count) {
    if (!TrackCommand::is_enabled() || net_id <= 0) return;
    auto& tracker = utils::PlayerTracker::get_instance();
    std::string who;
    if (static_cast<uint32_t>(net_id) == tracker.get_local_netid()) {
        who = "`9You ";
    } else {
        auto info = tracker.get_player_by_netid(static_cast<uint32_t>(net_id));
        if (info.name.empty()) return;
        who = "`w" + info.name + "`2 `9";
    }
    std::string line = fmt::format("`9[`2{}`9]`` {}Dropped `2{} `2{}`9 in World: `2{}",
                                   lucky::time_hhmm(), who, count, lucky::item_name(item_id), lucky::world_upper());
    std::lock_guard<std::mutex> lock(s_mutex);
    push(s_drop_logs, std::move(line));
}

void LogsCommand::on_collect(uint32_t net_id, uint16_t item_id, uint32_t count) {
    if (!TrackCommand::is_enabled() || net_id == 0 || item_id == 0) return;
    auto info = utils::PlayerTracker::get_instance().get_player_by_netid(net_id);
    if (info.name.empty()) return;
    std::string line = fmt::format("`9[`2{}`9]`` `w{}`9 Collected `2{} `2{}`9 in World: `2{}",
                                   lucky::time_hhmm(), info.name, count, lucky::item_name(item_id), lucky::world_upper());
    std::lock_guard<std::mutex> lock(s_mutex);
    push(s_collect_logs, std::move(line));
}

void LogsCommand::on_roulette(const std::string& message) {
    std::string line = "`9[`2" + lucky::time_hhmm() + "``]`` " + message;
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_roulette_enabled) return;
    push(s_roulette_logs, std::move(line));
}

// ------------------------------------------------------------
// Dialogs
// ------------------------------------------------------------
// Enable/disable buttons shared by /logs and the /proxy Proxy Logs tab
std::string LogsCommand::toggle_buttons() {
    const bool track = TrackCommand::is_enabled();
    bool roulette;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        roulette = s_roulette_enabled;
    }
    utils::Dialog dlg;
    dlg.smalltext(std::string("`9Drop/Collect Logs: ") + (track ? "`2Enabled" : "`4Disabled") +
                  " `9/ Roulette Logs: " + (roulette ? "`2Enabled" : "`4Disabled"))
       .button("logs_toggle_track", track ? "`4Disable Drop/Collect Logs" : "`2Enable Drop/Collect Logs")
       .button("logs_toggle_roulette", roulette ? "`4Disable Roulette Logs" : "`2Enable Roulette Logs")
       .button("logs_clear", "`4Clear All Logs");
    return dlg.str();
}

void LogsCommand::show_menu() {
    utils::Dialog dlg;
    dlg.label_with_icon("`2World Logs Page``", 1436)
       .spacer()
       .raw(toggle_buttons())
       .spacer()
       .button("logs_roulette", "`9World Roulette Logs")
       .button("logs_both", "`9World Collect/Drop Logs")
       .button("logs_drop", "`9World Drop Logs")
       .button("logs_collect", "`9World Collect Logs")
       .quick_exit()
       .end_dialog("logs_menu", "Cancel", "Okay");
    dlg.send(lucky::local_out());
}

void LogsCommand::show_world_logs(View view) {
    utils::Dialog dlg;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        dlg.label_with_icon("`2World Logs", 242)
           .spacer()
           .text_input("logs_search", "Search name", utils::Dialog::sanitize(s_search), 24)
           .spacer();
        if (view != View::Collect) {
            dlg.label_with_icon("`2Drop Logs", 242);
            append_filtered(dlg, s_drop_logs, s_search);
        }
        if (view == View::Both) dlg.spacer();
        if (view != View::Drop) {
            dlg.label_with_icon("`2Collect Logs", 242);
            append_filtered(dlg, s_collect_logs, s_search);
        }
    }
    if (!TrackCommand::is_enabled())
        dlg.smalltext("`4Drop/Collect Logs are OFF. Enable them on the World Logs page or with /track.");
    const char* name = view == View::Drop ? "world_logs_drop" : view == View::Collect ? "world_logs_collect" : "world_logs_both";
    dlg.spacer()
       .button("backlogs", "Back")
       .quick_exit()
       .end_dialog(name, "Close", "Search");
    dlg.send(lucky::local_out());
}

void LogsCommand::show_roulette_logs() {
    utils::Dialog dlg;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        dlg.label_with_icon("`2Roulette Logs", 758);
        for (const auto& text : s_roulette_logs)
            dlg.smalltext(text);
        if (s_roulette_logs.empty())
            dlg.smalltext("`oNo roulette spins in this world yet.");
    }
    dlg.spacer()
       .button("backlogs", "Back")
       .quick_exit()
       .end_dialog("world_logs_roulette", "", "Close");
    dlg.send(lucky::local_out());
}

bool LogsCommand::apply_toggle(const std::string& button_clicked) {
    if (button_clicked == "logs_toggle_track") {
        const bool enabled = !TrackCommand::is_enabled();
        TrackCommand::set_enabled(enabled);
        spdlog::info("[Logs] Drop/collect logs {}", enabled ? "enabled" : "disabled");
        lucky::log(enabled ? "`2Drop/Collect Logs `9are now `2ON" : "`2Drop/Collect Logs `9are now `4OFF");
        return true;
    }
    if (button_clicked == "logs_toggle_roulette") {
        bool enabled;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_roulette_enabled = !s_roulette_enabled;
            enabled = s_roulette_enabled;
        }
        spdlog::info("[Logs] Roulette logs {}", enabled ? "enabled" : "disabled");
        lucky::log(enabled ? "`2Roulette Logs `9are now `2ON" : "`2Roulette Logs `9are now `4OFF");
        return true;
    }
    if (button_clicked == "logs_clear") {
        on_world_load();
        lucky::log("`4All world logs cleared.");
        return true;
    }
    return false;
}

// Proxy Logs tab of /proxy: toggles + the latest entries of every log
std::string LogsCommand::proxy_logs_tab() {
    utils::Dialog dlg;
    dlg.raw(toggle_buttons()).spacer();

    auto section = [&](const char* title, int icon, const std::vector<std::string>& logs, const char* open_btn) {
        dlg.label_with_icon(fmt::format("`2{} `9({})``", title, logs.size()), icon, utils::Dialog::Size::Small);
        if (logs.empty()) {
            dlg.smalltext("`oNo logs yet.");
        } else {
            const size_t start = logs.size() > kProxyTabShow ? logs.size() - kProxyTabShow : 0;
            for (size_t i = logs.size(); i-- > start;)   // newest first
                dlg.smalltext(logs[i]);
            if (logs.size() > kProxyTabShow)
                dlg.smalltext(fmt::format("`7... showing latest {}, open the full list below.", kProxyTabShow));
        }
        dlg.button(open_btn, std::string("`9Open ") + title).spacer();
    };

    std::lock_guard<std::mutex> lock(s_mutex);
    section("Roulette Logs", 758, s_roulette_logs, "logs_roulette");
    section("Drop Logs", 242, s_drop_logs, "logs_drop");
    section("Collect Logs", 242, s_collect_logs, "logs_collect");
    return dlg.str();
}

void LogsCommand::handle_dialog(const std::string& dialog_name, const std::string& button_clicked, const std::string& raw) {
    if (apply_toggle(button_clicked)) { show_menu(); return; }
    if (button_clicked == "backlogs") { show_menu(); return; }
    if (button_clicked == "logs_roulette") { show_roulette_logs(); return; }
    if (button_clicked == "logs_both") { show_world_logs(View::Both); return; }
    if (button_clicked == "logs_drop") { show_world_logs(View::Drop); return; }
    if (button_clicked == "logs_collect") { show_world_logs(View::Collect); return; }

    // "Search" on a world logs page re-opens it filtered by the search box
    if (dialog_name.rfind("world_logs_", 0) == 0 && dialog_name != "world_logs_roulette" &&
        button_clicked != "Close" && button_clicked != "Cancel") {
        TextParse tp{raw};
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_search = tp.get("logs_search");
        }
        if (dialog_name == "world_logs_drop") show_world_logs(View::Drop);
        else if (dialog_name == "world_logs_collect") show_world_logs(View::Collect);
        else show_world_logs(View::Both);
    }
}

}
