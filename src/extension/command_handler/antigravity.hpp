#pragma once
#include <atomic>

namespace command {

// Anti Gravity toggle (LuckyProxy antigravity, /options): client-side Antigravity Generator
// effect, re-applied every time you enter a world. Nothing is sent to the game server.
class AntiGravity {
public:
    static bool is_enabled();
    static void set_enabled(bool enabled);
    static void on_local_spawn();

private:
    static void apply();
    static std::atomic<bool> s_enabled;
};

}
