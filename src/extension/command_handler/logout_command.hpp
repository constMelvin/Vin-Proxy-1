#pragma once
#include "command_base.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

namespace command {

// /logout - Log out of the game and go back to the Growtopia main menu
// (the screen you see when the game first opens). Same steps as doing it by hand:
//   1. "Exit World"  -> action|quit_to_exit to the server
//   2. the server opens the world select menu (OnRequestWorldSelectMenu)
//   3. "Back"        -> Esc is pressed in the Growtopia window (utils::game_window); the game
//                       sends action|quit itself and goes to the main menu
// The server can't send the game to the main menu: any server-side ending is treated as a lost
// connection. If the game reconnects anyway, that login is rejected and Cancel is pressed.
class LogoutCommand : public CommandBase {
public:
    LogoutCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    // Step 2: called when the server sends OnRequestWorldSelectMenu
    static void on_world_select_menu();

    // Called with the game's login packet. Right after /logout it rejects that login like the
    // real server does (log message + action|logon_fail), which sends the game to the main menu.
    // Returns true when the login was rejected (don't forward it to the server).
    static bool reject_login_after_logout();

private:
    static void leave_to_main_menu();

    static std::atomic<bool> s_waiting_for_menu;
    static std::mutex s_mutex;
    static std::chrono::steady_clock::time_point s_logout_until;
};

}
