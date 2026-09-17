#include "fastdoor_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include <spdlog/spdlog.h>

namespace command {

core::Core* FastDoorCommand::s_core = nullptr;
std::atomic<bool> FastDoorCommand::s_enabled{false};

static void send_console(player::Player* p, const std::string& msg) {
    if (!p) return;
    packet::Variant var{};
    var.add("OnConsoleMessage");
    var.add(msg);
    std::vector<std::byte> ext = var.serialize();

    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = -1;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext.data(), ext.size());
    p->send_packet(bs.get_data(), 0);
}

FastDoorCommand::FastDoorCommand() : CommandBase(
    {"fastdoor"},
    {},
    "Toggle fast open/close entrance doors",
    0
) {}

std::unique_ptr<CommandBase> FastDoorCommand::clone() const {
    return std::make_unique<FastDoorCommand>(*this);
}

void FastDoorCommand::set_core(core::Core* core) {
    s_core = core;
}

bool FastDoorCommand::is_enabled() {
    return s_enabled.load();
}

void FastDoorCommand::set_enabled(bool enabled) {
    s_enabled.store(enabled);
}

void FastDoorCommand::execute(client::Client*, const std::vector<std::string>&) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    bool new_state = !s_enabled.load();
    s_enabled.store(new_state);

    if (new_state) {
        send_console(server->get_player(), "`9Fast Open / Close Entrances Mode Now `2Enabled");
    } else {
        send_console(server->get_player(), "`9Fast Open / Close Entrances Mode Now `4Disabled");
    }
}

} // namespace command
