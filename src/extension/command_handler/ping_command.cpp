#include "ping_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../core/core.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_helper.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/display_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

PingCommand::PingCommand() : CommandBase(
    {"ping"},
    {},
    "Toggle ping display next to your name",
    0
) {}

std::unique_ptr<CommandBase> PingCommand::clone() const {
    return std::make_unique<PingCommand>(*this);
}

void PingCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    spdlog::warn("PingCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}

void PingCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    if (!client || !client->get_player()) return;

    int ping_ms = 0;
    auto* srv = core ? core->get_server() : nullptr;
    if (srv && srv->get_player() && srv->get_player()->get_peer()) {
        ping_ms = static_cast<int>(srv->get_player()->get_peer()->roundTripTime);
    }

    std::string ping_color = "`2";
    if (ping_ms > 150) ping_color = "`6";
    if (ping_ms > 300) ping_color = "`4";

    std::string msg = fmt::format("`0[ `bVinProxy `0] `wYour current ping: {}{}`w ms", ping_color, ping_ms);
    utils::PacketUtils::send_chat_message(client->get_player(), msg);
}

} 
