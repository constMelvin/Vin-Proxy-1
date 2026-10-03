#pragma once
#include "../../packet/tank_packet.hpp"
#include <atomic>

namespace command {

// Instant Dice Roll (LuckyProxy dicespeed, /options): shows the dice result before the animation ends
class DiceSpeed {
public:
    static bool is_enabled();
    static void set_enabled(bool enabled);

    // Server PACKET_TILE_APPLY_DAMAGE
    static void on_tile_damage(const packet::TankUpdatePacket& tank);

private:
    static std::atomic<bool> s_enabled;
};

}
