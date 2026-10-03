#pragma once
#include "command_base.hpp"
#include "../../packet/tank_packet.hpp"
#include <atomic>
#include <cstddef>
#include <vector>

namespace command {

// /showoc - Show open (green) / closed (red) doors (LuckyProxy)
class ShowOcCommand : public CommandBase {
public:
    ShowOcCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static bool is_enabled();
    static void set_enabled(bool enabled);
    // Local player spawned: paint every entrance of the world
    static void on_local_spawn();
    // Server PACKET_SEND_TILE_UPDATE_DATA; true = replaced by a painted copy
    static bool on_tile_update(const packet::TankUpdatePacket& tank, const std::vector<std::byte>& ext_data);

private:
    static void paint_all_entrances();
    static void send_painted_tile(int32_t x, int32_t y, std::vector<std::byte> ext_data, bool open);
    static std::atomic<bool> s_enabled;
};

}
