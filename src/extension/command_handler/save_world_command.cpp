#include "save_world_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/text_parse.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>

namespace command {

core::Core* SaveWorldCommand::s_core = nullptr;
std::string SaveWorldCommand::s_save_world{};

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

static void send_overlay(player::Player* p, const std::string& msg) {
    if (!p) return;
    packet::Variant var{};
    var.add("OnTextOverlay");
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

SaveWorldCommand::SaveWorldCommand() : CommandBase(
    {"save", "saveworld", "setsave"},
    {},
    "Save and warp to designated safe storage world (/setsave <world> to set)",
    0
) {}

std::unique_ptr<CommandBase> SaveWorldCommand::clone() const {
    return std::make_unique<SaveWorldCommand>(*this);
}

void SaveWorldCommand::set_core(core::Core* core) {
    s_core = core;
    if (s_core) {
        try {
            s_save_world = s_core->get_config().get<std::string>("player.save_world");
        } catch (...) {}
    }
}

std::string SaveWorldCommand::get_save_world() {
    return s_save_world;
}

void SaveWorldCommand::set_save_world(const std::string& world_name) {
    s_save_world = world_name;
    if (s_core) {
        s_core->get_config().set<std::string>("player.save_world", world_name);
        s_core->get_config().save();
    }
}

void SaveWorldCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!s_core || !client || !client->get_player()) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    if (args.empty()) return;
    const std::string& cmd = args[0];

    if (cmd == "setsave") {
        if (args.size() < 2 || args[1].empty()) {
            send_console(server->get_player(), "`4Usage: /setsave <world_name>");
            return;
        }

        std::string target_world = args[1];
        set_save_world(target_world);
        send_console(server->get_player(), fmt::format("`2[SAVE`2]`w: `wSave World successfully set to `2{}", target_world));
        return;
    }

    // /save or /saveworld
    if (s_save_world.empty()) {
        try {
            s_save_world = s_core->get_config().get<std::string>("player.save_world");
        } catch (...) {}
    }

    if (s_save_world.empty()) {
        send_overlay(server->get_player(), "`wSet Your save world in `2/setsave");
        send_console(server->get_player(), "`wSet Your save world in `2/setsave");
        return;
    }

    send_console(server->get_player(), fmt::format("`wGoing to `2Save World (`3{}`w)", s_save_world));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    TextParse tp{};
    tp.add("action", {"join_request"});
    tp.add("name", {s_save_world});
    tp.add("invitedWorld", {"0"});

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_MESSAGE);
    bs.write(tp.get_raw(), false);
    client->get_player()->send_packet(bs.get_data(), 0);
}

} // namespace command
