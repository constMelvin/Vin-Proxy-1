#pragma once
#include <atomic>
#include <string>

namespace command {

// Hide Private Messages (LuckyProxy blockmsg, /options)
class BlockMsg {
public:
    static bool is_enabled();
    static void set_enabled(bool enabled);

    // true = console message is a private message and should be hidden
    static bool should_hide(const std::string& message);

private:
    static std::atomic<bool> s_enabled;
};

}
