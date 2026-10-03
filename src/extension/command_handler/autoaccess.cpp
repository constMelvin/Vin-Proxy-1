#include "autoaccess.hpp"
#include "lucky_common.hpp"
#include "../../utils/player_tracker.hpp"
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> AutoAccess::s_enabled{false};
std::mutex AutoAccess::s_mutex;
std::chrono::steady_clock::time_point AutoAccess::s_hide_until{};

bool AutoAccess::is_enabled() { return s_enabled.load(); }
void AutoAccess::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[AutoAccess] {}", enabled ? "enabled" : "disabled");
}

void AutoAccess::on_console_message(const std::string& message) {
    if (!s_enabled.load() || message.find("Wrench yourself to accept.") == std::string::npos) return;

    const uint32_t netid = utils::PlayerTracker::get_instance().get_local_netid();
    if (netid == 0) return;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_hide_until = std::chrono::steady_clock::now() + std::chrono::milliseconds(1500);
    }
    spdlog::info("[AutoAccess] Server asked to accept lock access, accepting");
    const std::string id = std::to_string(netid);
    lucky::send_server_text("action|wrench\n|netid|" + id);
    lucky::send_server_text("action|dialog_return\ndialog_name|popup\nnetID|" + id + "|\nbuttonClicked|acceptlock");
    lucky::send_server_text("action|dialog_return\ndialog_name|acceptaccess");
    lucky::send_overlay("`2ACCESS GRANTED!");
}

bool AutoAccess::on_dialog_request(const std::string& dialog) {
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (std::chrono::steady_clock::now() >= s_hide_until) return false;
    }
    return dialog.find("acceptaccess") != std::string::npos ||
           dialog.find("end_dialog|popup|") != std::string::npos;
}

}
