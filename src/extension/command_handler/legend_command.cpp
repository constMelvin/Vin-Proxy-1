#include "legend_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../core/core.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/display_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

LegendCommand::LegendCommand() : CommandBase(
    {"legend", "lg", "legendary"},
    {},
    "Apply Legendary title (of Legend)",
    0
) {}

std::unique_ptr<CommandBase> LegendCommand::clone() const {
    return std::make_unique<LegendCommand>();
}

void LegendCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    spdlog::warn("LegendCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}

void LegendCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    if (!client || !client->get_player()) {
        spdlog::error("LegendCommand: No client or player!");
        return;
    }

    auto* server = core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("LegendCommand: No server available!");
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Server not available.");
        return;
    }

    auto& player_tracker = utils::PlayerTracker::get_instance();
    auto player_info = player_tracker.get_local_player();
    
    if (player_info.netID == 0) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Try again after spawning.");
        return;
    }

    spdlog::info("Applying Legend title to netid: {} (Name: {})", player_info.netID, player_info.name);
    
    bool current = false;
    try { current = core->get_config().get<bool>("display.title.legend"); } catch (...) {}
    core->get_config().set("display.title.legend", !current);
    
    utils::DisplayManager::apply_display_name(core, player_info.netID, player_info.name);

    if (!current) {
        utils::PacketUtils::send_chat_message(client->get_player(), fmt::format("`6Legendary title enabled! ({} of Legend)", player_info.name));
    } else {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Legendary title disabled.");
    }
    spdlog::info("Toggled Legend title via DisplayManager");
}

}
