#include "antigravity.hpp"
#include "lucky_common.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../utils/byte_stream.hpp"
#include <spdlog/spdlog.h>

namespace command {

namespace {
constexpr uint32_t kAntigravityGenerator = 4992;
constexpr int32_t kTileX = 98;   // LuckyProxy anti_gravity() tile
constexpr int32_t kTileY = 55;

void send_local(uint8_t type, uint32_t int_data) {
    auto* out = lucky::local_out();
    if (!out) {
        spdlog::warn("[AntiGravity] No game client connected");
        return;
    }
    packet::TankUpdatePacket pkt{};
    pkt.type = type;
    pkt.int_data = int_data;
    pkt.int_x = kTileX;
    pkt.int_y = kTileY;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    (void)out->send_packet(bs.get_data(), 0);
}
}

std::atomic<bool> AntiGravity::s_enabled{false};

bool AntiGravity::is_enabled() { return s_enabled.load(); }

// LuckyProxy anti_gravity(): place a generator on the client, then "punch" it to toggle it
void AntiGravity::apply() {
    send_local(static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST), kAntigravityGenerator);
    send_local(static_cast<uint8_t>(packet::PACKET_TILE_APPLY_DAMAGE), 0);
}

void AntiGravity::set_enabled(bool enabled) {
    if (enabled == s_enabled.load()) return;
    s_enabled.store(enabled);
    spdlog::info("[AntiGravity] {}", enabled ? "enabled" : "disabled");
    apply();
    lucky::send_overlay(enabled ? "`2Antigravity has been activated``" : "`2Antigravity has been`` `4deactivated``");
}

void AntiGravity::on_local_spawn() {
    if (!s_enabled.load()) return;
    spdlog::debug("[AntiGravity] Re-applied after entering a world");
    apply();
}

}
