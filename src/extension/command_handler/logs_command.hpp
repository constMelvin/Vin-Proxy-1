#pragma once
#include "command_base.hpp"
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace command {

// /logs - World Logs: roulette, drop & collect logs (LuckyProxy)
// Drop/collect recording follows /track (TrackCommand); roulette recording has its own switch.
class LogsCommand : public CommandBase {
public:
    LogsCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    // Recording hooks
    static void on_world_load();
    static void on_drop(int32_t net_id, uint16_t item_id, uint32_t count);
    static void on_collect(uint32_t net_id, uint16_t item_id, uint32_t count);
    static void on_roulette(const std::string& message);

    // Dialogs
    static void show_menu();
    static void handle_dialog(const std::string& dialog_name, const std::string& button_clicked, const std::string& raw);
    // Enable/disable/clear buttons (logs_toggle_track, logs_toggle_roulette, logs_clear); true if handled
    static bool apply_toggle(const std::string& button_clicked);
    // Dialog lines for the /proxy Proxy Logs tab
    static std::string proxy_logs_tab();

private:
    enum class View { Drop, Collect, Both };

    static std::string toggle_buttons();
    static void show_world_logs(View view);
    static void show_roulette_logs();
    static void push(std::vector<std::string>& logs, std::string line);

    static std::mutex s_mutex;
    static bool s_roulette_enabled;
    static std::vector<std::string> s_drop_logs;
    static std::vector<std::string> s_collect_logs;
    static std::vector<std::string> s_roulette_logs;
    static std::string s_search;
};

}
