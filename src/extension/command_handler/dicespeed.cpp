#include "dicespeed.hpp"
#include "lucky_common.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> DiceSpeed::s_enabled{false};

bool DiceSpeed::is_enabled() { return s_enabled.load(); }
void DiceSpeed::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[DiceSpeed] {}", enabled ? "enabled" : "disabled");
}

// LuckyProxy: a damage packet with no item is a dice roll, its count byte is roll - 1
void DiceSpeed::on_tile_damage(const packet::TankUpdatePacket& tank) {
    if (!s_enabled.load() || tank.int_data != 0) return;
    const int roll = static_cast<int>(tank.animation_type) + 1;
    if (roll < 1 || roll > 6) return;
    spdlog::info("[DiceSpeed] Dice will roll {}", roll);
    lucky::log(fmt::format("`bThe dice `bwill roll a `#{}", roll));
    lucky::send_overlay(fmt::format("`9The dice will roll a `#{}", roll));
}

}
