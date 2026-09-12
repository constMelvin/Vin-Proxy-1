#include "cleartitle_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../core/core.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/display_manager.hpp"
#include <spdlog/spdlog.h>

namespace command {

ClearTitleCommand::ClearTitleCommand() : CommandBase(
    {"cleartitle", "ct", "resettitle"},
    {},
    "Remove all titles and reset name",
    0
) {}

std::unique_ptr<CommandBase> ClearTitleCommand::clone() const {
    return std::make_unique<ClearTitleCommand>();
}

void ClearTitleCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    spdlog::warn("ClearTitleCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}

void ClearTitleCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    if (!core) return;

    auto* server = core->get_server();
    auto* target_player = (server && server->get_player()) ? server->get_player() : (client ? client->get_player() : nullptr);
    if (!target_player || !target_player->is_connected()) {
        spdlog::error("ClearTitleCommand: No target player connected!");
        return;
    }

    auto& player_tracker = utils::PlayerTracker::get_instance();
    auto player_info = player_tracker.get_local_player();
    uint32_t netid = player_info.netID;

    if (netid == 0) {
        auto all = player_tracker.get_all_players();
        if (!all.empty()) {
            netid = all.begin()->first;
        }
    }

    spdlog::info("Clearing all titles for netid: {} (Name: {})", netid, player_info.name);

    // 1. Reset all title configs
    core->get_config().set("display.title.g4g", false);
    core->get_config().set("display.title.maxlevel", false);
    core->get_config().set("display.title.dr", false);
    core->get_config().set("display.title.mentor", false);
    core->get_config().set("display.title.legend", false);
    core->get_config().set("display.title.super_supporter", false);
    core->get_config().set("display.visual_name", "");

    // 2. Send OnCountryState to reset title badges in GT client
    if (netid != 0 && server && server->get_player()) {
        std::string country = player_info.country.empty() ? "us" : player_info.country;
        packet::Variant var{};
        var.add("OnCountryState");
        var.add(country);

        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = static_cast<int>(netid);
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(bs.get_data(), 0);
    }

    // 3. Re-apply base display name without titles
    std::string base_name = player_info.name.empty() ? "" : player_info.name;
    if (!base_name.empty() && netid != 0) {
        utils::DisplayManager::apply_display_name(core, netid, base_name);
    }

    utils::PacketUtils::send_chat_message(target_player, "`2All titles cleared!");
    spdlog::info("Cleared all titles successfully");
}

} 
