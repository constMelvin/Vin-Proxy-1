#include "lucky_common.hpp"
#include "../item_finder/item_finder.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/world_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace command::lucky {

namespace {

constexpr uint16_t kEntranceCollision = 3;   // items.dat collision type of entrances/gateways

core::Core* g_core = nullptr;
extension::item_finder::ItemDatabase* g_items = nullptr;

void send_call(packet::Variant& variant) {
    auto* out = local_out();
    if (!out) return;
    std::vector<std::byte> ext_data = variant.serialize();
    packet::GameUpdatePacket game_packet{};
    game_packet.type = packet::PACKET_CALL_FUNCTION;
    game_packet.net_id = static_cast<uint32_t>(-1);
    game_packet.flags.extended = 1;
    game_packet.data_size = static_cast<uint32_t>(ext_data.size());
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(game_packet);
    bs.write_data(ext_data.data(), ext_data.size());
    (void)out->send_packet(bs.get_data(), 0);
}

} // namespace

void set_core(core::Core* core) { g_core = core; }
core::Core* get_core() { return g_core; }
void set_item_database(extension::item_finder::ItemDatabase* database) { g_items = database; }

player::Player* local_out() {
    if (!g_core || !g_core->get_server()) return nullptr;
    return g_core->get_server()->get_player();
}

void log(const std::string& msg) {
    if (auto* out = local_out())
        utils::PacketUtils::send_chat_message(out, msg, false);
}

void send_overlay(const std::string& text) {
    packet::Variant v{};
    v.add("OnTextOverlay");
    v.add(text);
    send_call(v);
}

void send_server_text(const std::string& text) {
    if (!g_core || !g_core->get_client() || !g_core->get_client()->get_player()) {
        spdlog::warn("[Proxy] Not connected to the game server, dropped: {}", text.substr(0, text.find('\n')));
        return;
    }
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
    bs.write(text, false);
    bs.write(std::uint8_t{0});   // Growtopia reads text messages as null-terminated
    (void)g_core->get_client()->get_player()->send_packet(bs.get_data(), 0);
}

void send_server_input(const std::string& text) {
    send_server_text("action|input\ntext|" + text);
}

void send_server_game_message(const std::string& text) {
    if (!g_core || !g_core->get_client() || !g_core->get_client()->get_player()) {
        spdlog::warn("[Proxy] Not connected to the game server, dropped: {}", text.substr(0, text.find('\n')));
        return;
    }
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_MESSAGE);
    bs.write(text, false);
    bs.write(std::uint8_t{0});   // Growtopia reads text messages as null-terminated
    (void)g_core->get_client()->get_player()->send_packet(bs.get_data(), 0);
}

void send_client_game_message(const std::string& text) {
    auto* out = local_out();
    if (!out) {
        spdlog::warn("[Proxy] No game client connected, dropped: {}", text.substr(0, text.find('\n')));
        return;
    }
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_MESSAGE);
    bs.write(text, false);
    bs.write(std::uint8_t{0});   // Growtopia reads text messages as null-terminated
    (void)out->send_packet(bs.get_data(), 0);
}

std::string item_name(uint16_t id) {
    if (g_items) {
        if (const auto* info = g_items->get_item_by_id(id))
            return info->name;
    }
    return fmt::format("Item {}", id);
}

bool is_entrance(uint16_t fg) {
    if (!g_items || fg == 0) return false;
    const auto* info = g_items->get_item_by_id(fg);
    return info && info->collision_type == kEntranceCollision;
}

bool cfg_flag(const std::string& key, bool def) {
    if (!g_core) return def;
    try { return g_core->get_config().get<bool>(key); } catch (...) { return def; }
}

void cfg_set(const std::string& key, bool value) {
    if (!g_core) return;
    if (cfg_flag(key, !value) != value)
        spdlog::info("[Config] {} = {}", key, value ? "on" : "off");
    g_core->get_config().set<bool>(key, value);
}

void cfg_save() {
    if (g_core) g_core->get_config().save();
}

std::string lower_no_codes(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '`' && i + 1 < text.size()) { ++i; continue; }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[i]))));
    }
    return out;
}

std::string time_hhmm() {
    std::time_t now = std::time(nullptr);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%H:%M");
    return oss.str();
}

std::string world_upper() {
    std::string w = utils::WorldManager::get_instance().get_world_name();
    std::transform(w.begin(), w.end(), w.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return w;
}

int extract_int(const std::string& content, const std::string& key) {
    size_t pos = content.find(key);
    if (pos == std::string::npos) return -1;
    try { return std::stoi(content.substr(pos + key.size())); } catch (...) { return -1; }
}

}
