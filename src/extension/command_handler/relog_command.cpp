#include "relog_command.hpp"
#include "position_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/world_manager.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

namespace command {

core::Core* RelogCommand::s_core = nullptr;

static void send_console(player::Player* p, const std::string& msg) {
    if (!p) return;
    packet::Variant var{};
    var.add("OnConsoleMessage");
    var.add(msg);
    std::vector<std::byte> ext = var.serialize();

    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = 0xFFFFFFFF;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext.data(), ext.size());
    p->send_packet(bs.get_data(), 0);
}

RelogCommand::RelogCommand() : CommandBase(
    {"relog"},
    {},
    "Reconnect and relog into the current world",
    0
) {}

std::unique_ptr<CommandBase> RelogCommand::clone() const {
    return std::make_unique<RelogCommand>(*this);
}

void RelogCommand::set_core(core::Core* core) {
    s_core = core;
}

void RelogCommand::execute(client::Client* client, const std::vector<std::string>&) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    std::string world_name = BackCommand::get_current_world();
    if (world_name.empty()) {
        world_name = utils::WorldManager::get_instance().get_world_name();
    }

    if (world_name.empty()) {
        send_console(server->get_player(), "`4Cannot relog: current world name unknown.");
        return;
    }

    send_console(server->get_player(), "`9Relog World!");

    // Send quit_to_exit
    TextParse tp_exit{};
    tp_exit.add("action", {"quit_to_exit"});
    ByteStream<std::uint16_t> bs_exit{};
    bs_exit.write(packet::NET_MESSAGE_GAME_MESSAGE);
    bs_exit.write(tp_exit.get_raw(), false);
    client->get_player()->send_packet(bs_exit.get_data(), 0);

    // After 300ms, send join_request
    core::Core* core = s_core;
    std::thread([core, world_name]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (!core || !core->get_client() || !core->get_client()->get_player()) return;

        TextParse tp_join{};
        tp_join.add("action", {"join_request"});
        tp_join.add("name", {world_name});
        tp_join.add("invitedWorld", {"0"});

        ByteStream<std::uint16_t> bs_join{};
        bs_join.write(packet::NET_MESSAGE_GAME_MESSAGE);
        bs_join.write(tp_join.get_raw(), false);
        core->get_client()->get_player()->send_packet(bs_join.get_data(), 0);
    }).detach();
}

} // namespace command
