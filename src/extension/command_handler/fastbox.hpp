#pragma once
#include <atomic>
#include <string>

namespace command {

// Fast Retrieve Donation Box (LuckyProxy fastbox, /options): empties Donation / Holiday Gift Boxes
// without showing their dialog
class FastBox {
public:
    static bool is_enabled();
    static void set_enabled(bool enabled);

    // true = dialog answered and hidden
    static bool on_dialog_request(const std::string& dialog);

private:
    static std::atomic<bool> s_enabled;
};

}
