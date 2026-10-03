#include "logout_command.hpp"
#include "lucky_common.hpp"
#include "../../player/player.hpp"
#include "../../utils/game_window.hpp"
#include <spdlog/spdlog.h>
#include <thread>

namespace command {

namespace {
// How long after /logout an automatic re-login of the game is rejected
constexpr auto kLogoutWindow = std::chrono::seconds(30);
// If no world select menu shows up (e.g. already on it), go on to step 3 after this
constexpr auto kMenuTimeout = std::chrono::milliseconds(2500);
// Give the world select menu a moment to open before pressing Back
constexpr auto kMenuDelay = std::chrono::milliseconds(500);
// Time for the game to leave after Back before trying the fallback
constexpr auto kLeaveTimeout = std::chrono::milliseconds(2000);
constexpr const char* kLogoutMessage = "`wYou have been logged out.`` Press `2Play Online`` to log in again.";

bool game_connected() {
    auto* out = lucky::local_out();
    return out && out->is_connected();
}
}

std::atomic<bool> LogoutCommand::s_waiting_for_menu{false};
std::mutex LogoutCommand::s_mutex;
std::chrono::steady_clock::time_point LogoutCommand::s_logout_until{};

LogoutCommand::LogoutCommand() : CommandBase({"logout"}, {}, "Log out and go back to the Growtopia main menu", 0) {}
std::unique_ptr<CommandBase> LogoutCommand::clone() const { return std::make_unique<LogoutCommand>(*this); }

void LogoutCommand::execute(client::Client*, const std::vector<std::string>&) {
    spdlog::info("[Logout] Step 1/3: exiting the world");
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_logout_until = std::chrono::steady_clock::now() + kLogoutWindow;
    }
    s_waiting_for_menu.store(true);
    lucky::send_server_game_message("action|quit_to_exit");

    // Not in a world (already on the world select menu): no menu packet will come
    std::thread([] {
        std::this_thread::sleep_for(kMenuTimeout);
        if (s_waiting_for_menu.exchange(false)) {
            spdlog::info("[Logout] No world select menu after exiting, continuing");
            leave_to_main_menu();
        }
    }).detach();
}

void LogoutCommand::on_world_select_menu() {
    if (!s_waiting_for_menu.exchange(false)) return;
    spdlog::info("[Logout] Step 2/3: world select menu is open");
    std::thread([] {
        std::this_thread::sleep_for(kMenuDelay);
        leave_to_main_menu();
    }).detach();
}

// Step 3: press Back on the world select menu, like doing it by hand. The game sends action|quit
// itself and goes to the main menu.
void LogoutCommand::leave_to_main_menu() {
    spdlog::info("[Logout] Step 3/3: pressing Back on the world select menu");
    utils::game_window::press_back();

    std::thread([] {
        std::this_thread::sleep_for(kLeaveTimeout);
        if (!game_connected()) {
            spdlog::info("[Logout] Done, the game is on the main menu");
            return;
        }
        // Back didn't reach the game (window not found / key ignored): log out on the server side.
        // The game then reconnects and reject_login_after_logout sends it to the main menu.
        spdlog::warn("[Logout] The game is still connected after Back, logging out on the server side");
        lucky::send_server_game_message("action|quit");
    }).detach();
}

bool LogoutCommand::reject_login_after_logout() {
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (std::chrono::steady_clock::now() >= s_logout_until) return false;
        s_logout_until = {};
    }
    spdlog::info("[Logout] The game tried to log in again, rejecting it and pressing Cancel");
    lucky::send_client_game_message(std::string("action|log\nmsg|") + kLogoutMessage);
    lucky::send_client_game_message("action|logon_fail");
    std::thread([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (auto* out = lucky::local_out()) out->disconnect_later();
        // The game stays on the connecting screen after a rejected login; Cancel takes it to the menu
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        utils::game_window::press_back();
    }).detach();
    return true;
}

}
