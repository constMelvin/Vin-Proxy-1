#pragma once
#include <atomic>
#include <string>

namespace command {

// Hide "You need to be Level x" message (LuckyProxy levelmsg, /options)
class HideLevel {
public:
    static bool is_enabled();
    static void set_enabled(bool enabled);

    // Console message or talk bubble text; true = hide it
    static bool should_hide(const std::string& text);

private:
    static std::atomic<bool> s_enabled;
};

}
