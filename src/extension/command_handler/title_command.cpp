#include "title_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../core/core.hpp"
#include "../../packet/packet_helper.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/display_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace command {

TitleCommand::TitleCommand() : CommandBase(
    {"title", "titles", "tag"},
    {},
    "Open title selection GUI",
    0
) {}

std::unique_ptr<CommandBase> TitleCommand::clone() const {
    return std::make_unique<TitleCommand>();
}

void TitleCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    spdlog::warn("TitleCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}

void TitleCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    spdlog::info("TitleCommand::execute_with_core called - opening title GUI");
    send_title_gui(client, core);
    spdlog::info("Title command executed - GUI should be displayed");
}

void TitleCommand::send_title_gui(client::Client* client, core::Core* core) {
    if (!client || !client->get_player()) {
        spdlog::error("send_title_gui: client or player is null!");
        return;
    }

    auto* server = core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("send_title_gui: No server available!");
        return;
    }

    spdlog::info("Building title GUI dialog...");

    try {
        // Read Super Supporter state for toggle label
        bool super_on = false;
        try { super_on = core->get_config().get<bool>("display.title.super_supporter"); }
        catch (...) {}

        std::ostringstream dialog;
        dialog << "set_default_color|`o\n";

        // ── Header (Icon: 18 matches Newbie Proxy) ──
        dialog << "add_label_with_icon|big|`pVisual Titles Page``|left|14186|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_textbox|`oClick on the `wicon`` `oto toggle a title on/off. Active titles will be displayed before your name.|left|\n";
        dialog << "add_spacer|small|\n";

        dialog << "add_label_with_icon_button|big|`wMax Level``|left|1488|maxlv|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_label_with_icon_button|big|`wDoctor``|left|7068|dr|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_label_with_icon_button|big|`wLegendary``|left|1794|legend|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_label_with_icon_button|big|`wGrow4Good``|left|11816|g4g|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_label_with_icon_button|big|`wMaster``|left|9472|mentor|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_label_with_icon_button|big|"
               << (super_on ? "`4Disable `wSuper Supporter``" : "`2Enable `wSuper Supporter``")
               << "|left|14360|super_supporter|\n";

        dialog << "add_spacer|small|\n";

        // ── Reset All Titles ──
        dialog << "add_button|cleartitle|`pReset All Titles``|\n";

        dialog << "add_spacer|small|\n";

        // ── Footer ── Cancel / Okey matching screenshot
        dialog << "add_quick_exit|\n";
        dialog << "end_dialog|title_gui|Cancel|Okey|";

        packet::Variant variant{};
        variant.add("OnDialogRequest");
        variant.add(dialog.str());
        
        std::vector<std::byte> ext_data = variant.serialize();

        packet::GameUpdatePacket game_packet{};
        game_packet.type = packet::PACKET_CALL_FUNCTION;
        game_packet.net_id = -1;
        game_packet.flags.extended = 1;
        game_packet.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> byte_stream{};
        byte_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        byte_stream.write(game_packet);
        byte_stream.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(byte_stream.get_data(), 0);
        
        spdlog::info("Title GUI sent to client successfully");

    } catch (const std::exception& e) {
        spdlog::error("Failed to send title GUI: {}", e.what());
    }
}


G4GCommand::G4GCommand() : CommandBase(
    {"g4g"},
    {},
    "Apply G4G title",
    0
) {}

std::unique_ptr<CommandBase> G4GCommand::clone() const {
    return std::make_unique<G4GCommand>();
}

void G4GCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    
    spdlog::warn("G4GCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}


MaxLevelCommand::MaxLevelCommand() : CommandBase(
    {"maxlevel", "maxlv", "ml"},
    {},
    "Apply Max Level title",
    0
) {}

std::unique_ptr<CommandBase> MaxLevelCommand::clone() const {
    return std::make_unique<MaxLevelCommand>();
}

void MaxLevelCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    
    spdlog::warn("MaxLevelCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}


DrCommand::DrCommand() : CommandBase(
    {"dr", "doctor"},
    {},
    "Apply Dr. title",
    0
) {}

std::unique_ptr<CommandBase> DrCommand::clone() const {
    return std::make_unique<DrCommand>();
}

void DrCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    
    spdlog::warn("DrCommand::execute called without core - this shouldn't happen");
    if (client && client->get_player()) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Command requires core access.");
    }
}





void G4GCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    if (!client || !client->get_player()) {
        spdlog::error("G4GCommand: No client or player!");
        return;
    }

    auto* server = core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("G4GCommand: No server available!");
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Server not available.");
        return;
    }

    auto& player_tracker = utils::PlayerTracker::get_instance();
    auto player_info = player_tracker.get_local_player();
    
    if (player_info.netID == 0) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Try again after spawning.");
        return;
    }

    spdlog::info("Applying G4G title to netid: {} (Name: {})", player_info.netID, player_info.name);
    
    
    bool current = core->get_config().get<bool>("display.title.g4g");
    core->get_config().set("display.title.g4g", !current);

    
    utils::DisplayManager::apply_display_name(core, player_info.netID, player_info.name);

    if (!current) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`2G4G title enabled!");
    } else {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4G4G title disabled.");
    }
    spdlog::info("Toggled G4G title via DisplayManager");
}

void MaxLevelCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    if (!client || !client->get_player()) {
        spdlog::error("MaxLevelCommand: No client or player!");
        return;
    }

    auto* server = core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("MaxLevelCommand: No server available!");
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Server not available.");
        return;
    }

    auto& player_tracker = utils::PlayerTracker::get_instance();
    auto player_info = player_tracker.get_local_player();
    
    if (player_info.netID == 0) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Try again after spawning.");
        return;
    }

    spdlog::info("Applying Max Level title to netid: {} (Name: {})", player_info.netID, player_info.name);
    
    
    bool current = core->get_config().get<bool>("display.title.maxlevel");
    core->get_config().set("display.title.maxlevel", !current);
    
    
    utils::DisplayManager::apply_display_name(core, player_info.netID, player_info.name);

    if (!current) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`5Max Level title enabled!");
    } else {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Max Level title disabled.");
    }
    spdlog::info("Toggled Max Level title via DisplayManager");
}

void DrCommand::execute_with_core(client::Client* client, const std::vector<std::string>& args, core::Core* core) {
    if (!client || !client->get_player()) {
        spdlog::error("DrCommand: No client or player!");
        return;
    }

    auto* server = core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("DrCommand: No server available!");
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Server not available.");
        return;
    }

    auto& player_tracker = utils::PlayerTracker::get_instance();
    auto player_info = player_tracker.get_local_player();
    
    if (player_info.netID == 0) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Try again after spawning.");
        return;
    }

    spdlog::info("Applying Dr. title to netid: {} (Name: {})", player_info.netID, player_info.name);
    
    
    bool current = core->get_config().get<bool>("display.title.dr");
    core->get_config().set("display.title.dr", !current);
    
    
    utils::DisplayManager::apply_display_name(core, player_info.netID, player_info.name);

    if (!current) {
        utils::PacketUtils::send_chat_message(client->get_player(), "`9Dr. title enabled!");
    } else {
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Dr. title disabled.");
    }
    spdlog::info("Toggled Dr. title via DisplayManager");
}

} 
