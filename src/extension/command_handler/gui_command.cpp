#include "gui_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_helper.hpp"
#include "../../proxy_imgui_gui.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

GuiCommand::GuiCommand() : CommandBase(
    {"gui", "proxygui"},
    {},
    "Toggle the Vin Proxy Premium Desktop GUI",
    0
) {}

std::unique_ptr<CommandBase> GuiCommand::clone() const {
    return std::make_unique<GuiCommand>();
}

void GuiCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    ToggleGui();
    bool visible = IsGuiVisible();
    std::string state_str = visible ? "`2OPENED" : "`4CLOSED";
    std::string text = fmt::format("`oVin Proxy GUI: {} `o(Press `bCtrl + G`o or `bINSERT`o to toggle)``", state_str);
    
    packet::message::Log success_msg{};
    success_msg.msg = text;
    if (client && client->get_player()) {
        packet::PacketHelper::send(success_msg, *client->get_player());
    }
    
    spdlog::info("GUI command executed - GUI is now {}", visible ? "visible" : "hidden");
}

} 
