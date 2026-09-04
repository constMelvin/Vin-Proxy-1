#include "gui_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_helper.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

GuiCommand::GuiCommand() : CommandBase(
    {"gui", "menu", "interface"},
    {},
    "Open the Vin Proxy Premium GUI",
    0
) {}

std::unique_ptr<CommandBase> GuiCommand::clone() const {
    return std::make_unique<GuiCommand>();
}

void GuiCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    
    
    
    
    packet::message::Log success_msg{};
    success_msg.msg = "`2Opening Vin Proxy Premium GUI...";
    if (client->get_player()) {
        packet::PacketHelper::send(success_msg, *client->get_player());
    }
    
    spdlog::info("GUI command executed");
}

} 
