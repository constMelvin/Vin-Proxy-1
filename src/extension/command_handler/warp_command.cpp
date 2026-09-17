#include "warp_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include <fmt/format.h>

namespace command {

WarpCommand::WarpCommand() : CommandBase(
    {"warp", "exit"},
    {"world_name"},
    "Warp into the world specified by world_name (/exit warps to EXIT)",
    1
) {}

std::unique_ptr<CommandBase> WarpCommand::clone() const {
    return std::make_unique<WarpCommand>(*this);
}

void WarpCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    
    std::string command_name;
    if (!args.empty()) {
        command_name = args[0];
    }

    if (command_name == "exit") {
        const std::string world_name = "EXIT";
        send_warp_packet(client, world_name);

        packet::message::Log success_msg{};
        success_msg.msg = "`2Warping to world: `5EXIT";
        if (client->get_player()) {
            packet::PacketHelper::send(success_msg, *client->get_player());
        }
        return;
    }

    if (args.size() < 2) {
        packet::message::Log error_msg{};
        error_msg.msg = "`4Usage: /warp <world>|<door> or /warp <world> <door>";
        if (client->get_player()) {
            packet::PacketHelper::send(error_msg, *client->get_player());
        }
        return;
    }

    std::string world;
    std::string door;

    std::string raw = args[1];
    if (args.size() >= 3) {
        raw += " " + args[2];
    }

    size_t pipe_pos = raw.find('|');
    if (pipe_pos != std::string::npos) {
        world = raw.substr(0, pipe_pos);
        door = raw.substr(pipe_pos + 1);
    } else {
        size_t space_pos = raw.find_first_of(" \t");
        if (space_pos != std::string::npos) {
            world = raw.substr(0, space_pos);
            door = raw.substr(space_pos + 1);
        } else {
            world = raw;
        }
    }

    // Trim whitespace
    auto trim = [](std::string& s) {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    };
    trim(world);
    trim(door);

    for (char& c : world) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (char& c : door) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    std::string target = world;
    if (!door.empty()) {
        target += "|" + door;
    }

    send_warp_packet(client, target);
    
    packet::message::Log success_msg{};
    success_msg.msg = fmt::format("`2Warping to `1{}", target);
    if (client->get_player()) {
        packet::PacketHelper::send(success_msg, *client->get_player());
    }
}

void WarpCommand::send_warp_packet(client::Client* client, const std::string& world_name) {
    
    TextParse text_parse{};
    text_parse.add("action", {"join_request"});
    text_parse.add("name", {world_name});
    text_parse.add("invitedWorld", {"0"});

    
    ByteStream<std::uint16_t> byte_stream{};
    byte_stream.write(packet::NET_MESSAGE_GAME_MESSAGE);
    byte_stream.write(text_parse.get_raw(), false);

    
    if (client->get_player()) {
        client->get_player()->send_packet(byte_stream.get_data(), 0);
    }
}

} 
