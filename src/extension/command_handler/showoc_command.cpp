#include "showoc_command.hpp"
#include "lucky_common.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/world_manager.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace command {

namespace {
constexpr uint16_t kPaintRed = 0x2000;
constexpr uint16_t kPaintGreen = 0x4000;
constexpr uint16_t kPaintMask = 0xE000;      // red | green | blue
constexpr uint16_t kDoorOpenMask = 0x00C0;   // TILEFLAG_OPEN | TILEFLAG_PUBLIC
}

std::atomic<bool> ShowOcCommand::s_enabled{false};

ShowOcCommand::ShowOcCommand() : CommandBase({"showoc"}, {}, "Show open/closed doors", 0) {}
std::unique_ptr<CommandBase> ShowOcCommand::clone() const { return std::make_unique<ShowOcCommand>(*this); }

bool ShowOcCommand::is_enabled() { return s_enabled.load(); }

void ShowOcCommand::execute(client::Client*, const std::vector<std::string>&) {
    set_enabled(!s_enabled.load());
}

void ShowOcCommand::set_enabled(bool enabled) {
    if (enabled == s_enabled.load()) return;
    s_enabled.store(enabled);
    spdlog::info("[ShowOC] {}", enabled ? "enabled" : "disabled");
    if (enabled) {
        lucky::log("`9Show `2Open`9/`4Closed`9 Doors is now `2enabled.");
        paint_all_entrances();
    } else {
        lucky::log("`9Show `2Open`9/`4Closed`9 Doors is now `4disabled. `9Re-enter the world to clear the colours.");
    }
}

// Sends the tile back to the client with a red (closed) or green (open) paint flag.
// ext_data is the tile data (fg, bg, parent, flags, ...); any extra data is kept as is.
void ShowOcCommand::send_painted_tile(int32_t x, int32_t y, std::vector<std::byte> ext_data, bool open) {
    auto* out = lucky::local_out();
    if (!out || ext_data.size() < 8) return;
    uint16_t flags = 0;
    std::memcpy(&flags, ext_data.data() + 6, 2);
    flags = static_cast<uint16_t>((flags & ~kPaintMask) | (open ? kPaintGreen : kPaintRed));
    std::memcpy(ext_data.data() + 6, &flags, 2);

    packet::TankUpdatePacket pkt{};
    pkt.type = static_cast<uint8_t>(packet::PACKET_SEND_TILE_UPDATE_DATA);
    pkt.net_id = -1;
    pkt.flags = 1u << 3;   // extended
    pkt.int_x = x;
    pkt.int_y = y;
    pkt.extra_data_size = static_cast<uint32_t>(ext_data.size());
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());
    (void)out->send_packet(bs.get_data(), 0);
}

// LuckyProxy server::makeworld
void ShowOcCommand::paint_all_entrances() {
    const world_v2::World world = utils::WorldManager::get_instance().get_world_v2();
    int painted = 0;
    for (const auto& tile : world.tiles) {
        if (!lucky::is_entrance(tile.fg)) continue;
        // No extra data is sent, so drop the "has extra data" flag
        const uint16_t flags = static_cast<uint16_t>(tile.flags & ~0x0001);
        std::vector<std::byte> ext(8);
        std::memcpy(ext.data() + 0, &tile.fg, 2);
        std::memcpy(ext.data() + 2, &tile.bg, 2);
        std::memcpy(ext.data() + 4, &tile.parent_index, 2);
        std::memcpy(ext.data() + 6, &flags, 2);
        send_painted_tile(static_cast<int32_t>(tile.x), static_cast<int32_t>(tile.y), std::move(ext),
                          (tile.flags & kDoorOpenMask) != 0);
        ++painted;
    }
    if (painted == 0 && !world.tiles.empty())
        spdlog::warn("[ShowOC] No entrances found (is the item database loaded?)");
    spdlog::info("[ShowOC] Painted {} entrances", painted);
}

void ShowOcCommand::on_local_spawn() {
    if (s_enabled.load()) paint_all_entrances();
}

bool ShowOcCommand::on_tile_update(const packet::TankUpdatePacket& tank, const std::vector<std::byte>& ext_data) {
    if (!s_enabled.load() || ext_data.size() < 8) return false;
    uint16_t fg = 0, flags = 0;
    std::memcpy(&fg, ext_data.data(), 2);
    std::memcpy(&flags, ext_data.data() + 6, 2);
    if (!lucky::is_entrance(fg)) return false;
    send_painted_tile(tank.int_x, tank.int_y, ext_data, (flags & kDoorOpenMask) != 0);
    return true;
}

}
