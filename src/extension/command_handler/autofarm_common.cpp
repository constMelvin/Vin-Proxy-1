#include "autofarm_common.hpp"
#include "utility_commands.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../proxy_imgui_gui.hpp"
#include "../item_finder/item_finder.hpp"
#include <atomic>
#include <vector>

namespace command::autofarm {

static std::atomic<bool> s_facing_left{false};

static void send_function(player::Player* p, const char* fn, const std::string& arg) {
    if (!p) return;
    packet::Variant var{};
    var.add(fn);
    var.add(arg);
    std::vector<std::byte> ext = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = static_cast<uint32_t>(-1);
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext.data(), ext.size());
    (void)p->send_packet(bs.get_data(), 0);
}

void send_dialog(player::Player* p, const std::string& body) { send_function(p, "OnDialogRequest", body); }
// Same "[VinProxy Premium]" prefix the parser puts on every other console message.
void send_console(player::Player* p, const std::string& msg)  { utils::PacketUtils::send_chat_message(p, msg); }

bool in_world() {
    auto name = utils::WorldManager::get_instance().get_world_name();
    return !name.empty() && name != "EXIT";
}

bool local_tile_pos(core::Core* core, int& tx, int& ty) {
    if (!core) return false;
    try {
        auto px = core->get_config().get<std::string>("player.position.x");
        auto py = core->get_config().get<std::string>("player.position.y");
        if (px.empty() || py.empty()) return false;
        tx = static_cast<int>(std::stof(px) / 32.0f);
        ty = static_cast<int>(std::stof(py) / 32.0f);
        return true;
    } catch (...) { return false; }
}

uint16_t tile_fg(int x, int y) {
    if (x < 0 || y < 0) return 0;
    return utils::WorldManager::get_instance().get_tile_fg(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

bool facing_left() { return s_facing_left.load(); }
void set_facing_left(bool left) { s_facing_left.store(left); }

void send_tile_action(core::Core* core, int32_t int_data, int32_t x, int32_t y) {
    if (!core || !core->get_client() || !core->get_client()->get_player()) return;
    MoriStatePacket pkt{};
    pkt.type = static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST);
    pkt.net_id = utils::PlayerTracker::get_instance().get_local_netid();
    pkt.value = static_cast<uint32_t>(int_data);
    pkt.vector_x = static_cast<float>(x * 32);
    pkt.vector_y = static_cast<float>(y * 32);
    pkt.int_x = x;
    pkt.int_y = y;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    (void)core->get_client()->get_player()->send_packet(bs.get_data(), 0);
}

void punch(core::Core* core, int x, int y) { send_tile_action(core, 18, x, y); }

void punch_legit(core::Core* core, int x, int y) {
    if (!core || !core->get_client() || !core->get_client()->get_player()) return;

    // The player's own pixel position, as a real client puts in both packets
    float pos_x = static_cast<float>(x * 32);
    float pos_y = static_cast<float>(y * 32);
    try {
        auto sx = core->get_config().get<std::string>("player.position.x");
        auto sy = core->get_config().get<std::string>("player.position.y");
        if (!sx.empty() && !sy.empty()) { pos_x = std::stof(sx); pos_y = std::stof(sy); }
    } catch (...) {}

    const uint32_t netid = utils::PlayerTracker::get_instance().get_local_netid();
    auto* to_server = core->get_client()->get_player();

    // 1. state packet: swing the fist (item 18) at the target tile.
    //    2592 = ON_TILE_ACTION | ON_PUNCHED | ON_SOLID, the flags LuckyProxy sends.
    MoriStatePacket state{};
    state.type = static_cast<uint8_t>(packet::PACKET_STATE);
    state.net_id = netid;
    state.flags = packet::PACKET_FLAG_ON_TILE_ACTION | packet::PACKET_FLAG_ON_PUNCHED | packet::PACKET_FLAG_ON_SOLID;
    if (s_facing_left.load()) state.flags |= packet::PACKET_FLAG_ROTATE_LEFT;
    state.value = 18;
    state.vector_x = pos_x;
    state.vector_y = pos_y;
    state.int_x = x;
    state.int_y = y;
    ByteStream<std::uint16_t> bs_state{};
    bs_state.write(packet::NET_MESSAGE_GAME_PACKET);
    bs_state.write(state);
    (void)to_server->send_packet(bs_state.get_data(), 0);

    // 2. the tile change itself, with the player position rather than the tile position
    MoriStatePacket tile{};
    tile.type = static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST);
    tile.net_id = netid;
    tile.value = 18;
    tile.vector_x = pos_x;
    tile.vector_y = pos_y;
    tile.int_x = x;
    tile.int_y = y;
    ByteStream<std::uint16_t> bs_tile{};
    bs_tile.write(packet::NET_MESSAGE_GAME_PACKET);
    bs_tile.write(tile);
    (void)to_server->send_packet(bs_tile.get_data(), 0);
}
void place(core::Core* core, int item_id, int x, int y) { send_tile_action(core, item_id, x, y); }

// LuckyProxy MoveXY: walk a real A* path (state packets per step, local client
// kept in sync). A single teleport packet is rejected/rubberbanded by the server.
// Blocks for the duration of the walk, so call it from a worker thread only.
bool move_to(core::Core* core, int x, int y) {
    if (!core || !core->get_client() || x < 0 || y < 0) return false;
    const int steps = FindPathCommand::run_path(core->get_client(),
                                                static_cast<uint32_t>(x), static_cast<uint32_t>(y),
                                                /*show_console=*/false);
    return steps >= 0;
}

bool parse_int(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) { return false; }
}

bool checkbox_on(const TextParse& tp, const char* key) {
    int v = 0;
    return parse_int(tp.get(key), v) && v != 0;
}

std::string desc_text(const std::string& text) {
    return "add_custom_margin|x:0;y:-32|\n"
           "add_custom_textbox|" + text + "|size:tiny;color:200,200,200,200|\n"
           "add_custom_margin|x:0;y:10|\n";
}

std::string tiny_text(const std::string& text) {
    return "add_custom_textbox|" + text + "|size:tiny;color:200,200,200,200|\n";
}

std::string item_name(int id) {
    if (auto* db = GetItemDatabase()) {
        if (const auto* info = db->get_item_by_id(id)) return info->name;
    }
    return "Item #" + std::to_string(id);
}

} // namespace command::autofarm
