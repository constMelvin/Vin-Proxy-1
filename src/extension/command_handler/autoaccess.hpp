#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

namespace command {

// Auto Access (LuckyProxy autoacc, /options): accepts lock access as soon as the server asks for it
class AutoAccess {
public:
    static bool is_enabled();
    static void set_enabled(bool enabled);

    // "Wrench yourself to accept." console message -> accept the access
    static void on_console_message(const std::string& message);
    // Hides the wrench popup / accept dialogs answered by the proxy (true = hide)
    static bool on_dialog_request(const std::string& dialog);

private:
    static std::atomic<bool> s_enabled;
    static std::mutex s_mutex;
    static std::chrono::steady_clock::time_point s_hide_until;
};

}
