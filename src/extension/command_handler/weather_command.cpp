#include "weather_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/dialog.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace command {

core::Core* WeatherCommand::s_core = nullptr;
bool WeatherCommand::s_keep_weather = false;
int WeatherCommand::s_current_weather = 0;

namespace {

struct WeatherEntry {
    const char* label;
    int icon;
    int weather_id;
};

// LuckyProxy events.cpp /weather dialog + weather_page button -> weather id table
const WeatherEntry k_weathers[] = {
    {"Beach Blast", 830, 1},
    {"Weather Machine - Night", 934, 2},
    {"Weather Machine - Arid", 946, 3},
    {"Weather Machine - Rainy City", 984, 5},
    {"Harvest Moon Blast", 1060, 6},
    {"Mars Blast", 1136, 7},
    {"Weather Machine - Spooky", 1210, 8},
    {"Imperial Starship Blast", 6420, 9},
    {"Weather Machine - Nothingness", 1490, 10},
    {"Weather Machine - Snowy", 1364, 11},
    {"Growchimilco Boat", 1364, 13},
    {"Undersea Blast", 1532, 14},
    {"Weather Machine - Warp Speed", 1750, 15},
    {"Weather Machine - Comet", 2046, 16},
    {"Weather Machine - Howling Sky", 3534, 17},
    {"Weather Machine - Party", 2284, 18},
    {"Weather Machine - Pineapples", 2744, 19},
    {"Weather Machine - Snowy Night", 3252, 20},
    {"Bountiful Blast", 8738, 21},
    {"Weather Machine - Heatwave", 3694, 24},
    {"Weather Machine - Stuff", 3832, 29},
    {"Weather Machine - Pagoda", 4242, 30},
    {"Weather Machine - Apocalypse", 4486, 31},
    {"Weather Machine - Jungle", 4776, 32},
    {"Weather Machine - Balloon Warz", 4892, 33},
    {"Weather Machine - Background", 5000, 34},
    {"Weather Machine - Autumn", 5112, 35},
    {"Weather Machine - Valentine's", 5654, 36},
    {"Weather Machine - St. Paddy's Day", 5716, 37},
    {"Weather Machine - Epoch Ice", 5958, 38},
    {"Weather Machine - Epoch Volcano", 5958, 39},
    {"Weather Machine - Epoch Island", 5958, 40},
    {"Weather Machine - Digital Rain", 6854, 42},
    {"Monochrome Blast", 7380, 43},
    {"Weather Machine - Frozen Cliffs", 7644, 44},
    {"SurgWorld Blast", 8556, 45},
    {"Stellarix Starship Blast", 6422, 49},
    {"HyperTech Starship Blast", 1750, 50},
    {"Weather Machine - Celebrity Hills", 6488, 51},
    {"Pet Dragon Lock", 11562, 53},
    {"Blood Dragon Lock", 11550, 54},
    {"Prince Of Persia Lock", 11596, 55},
    {"Weather Machine - Radical City Lock", 11902, 58},
    {"Weather Machine - Plaza", 11880, 59},
    {"Weather Machine - Nebula", 12054, 60},
    {"Weather Machine - Protostar Landing", 12056, 61},
    {"Weather Machine - Dark Mountains", 12408, 62},
    {"Weather Machine - Assassin's Creed Lock", 12654, 63},
    {"Weather Machine - Mt. Growmore", 12844, 64},
    {"Weather Machine - Crack In Reality", 13004, 65},
    {"Weather Machine - Nian's Mountains", 13070, 66},
    {"Weather Machine - Rayman Lock", 13200, 67},
    {"Weather Machine - Steampunk Lock", 13636, 68},
    {"Weather Machine - Realm of Spirits", 13640, 69},
    {"Weather Machine - Black Hole", 13690, 70},
    {"Weather Machine - Rainin' Gems", 14032, 71},
};

constexpr const char* k_button_prefix = "weather_";

player::Player* resolve_player(core::Core* core, client::Client* client) {
    if (core) {
        if (auto* srv = core->get_server(); srv && srv->get_player())
            return srv->get_player();
    }
    return client ? client->get_player() : nullptr;
}

void send_variant(player::Player* player, packet::Variant& variant) {
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
    (void)player->send_packet(bs.get_data(), 0);
}

} // namespace

WeatherCommand::WeatherCommand() : CommandBase(
    {"weather"},
    {"[weather_id]"},
    "Open the Custom Weather Machine (or /weather <id>)",
    0
) {}

std::unique_ptr<CommandBase> WeatherCommand::clone() const {
    return std::make_unique<WeatherCommand>(*this);
}

void WeatherCommand::set_core(core::Core* core) {
    s_core = core;
}

void WeatherCommand::show_dialog(player::Player* player) {
    utils::Dialog dlg;
    dlg.label_with_icon("Custom Weather Machine", 2284)
       .spacer()
       .checkbox("keep_weather", "Keep Weather Across All Worlds", s_keep_weather)
       .smalltext("Click On Any Weather Machine To Change The Weather");
    for (const auto& w : k_weathers)
        dlg.label_with_icon_button(w.label, w.icon, k_button_prefix + std::to_string(w.weather_id));
    dlg.quick_exit()
       .end_dialog("weather_page", "Cancel", "Okay");
    dlg.send(player);
}

void WeatherCommand::send_weather_packet(player::Player* player, int weather_id) {
    if (!player) return;
    packet::Variant variant{};
    variant.add("OnSetCurrentWeather");
    variant.add(weather_id);
    send_variant(player, variant);
    spdlog::info("[Weather] Weather set to {}", weather_id);
}

void WeatherCommand::handle_dialog_response(player::Player* player, const std::string& button_clicked, const std::string& raw) {
    TextParse tp{raw};
    std::string keep = tp.get("keep_weather");
    if (!keep.empty() && s_keep_weather != (keep[0] == '1')) {
        s_keep_weather = keep[0] == '1';
        spdlog::info("[Weather] Keep weather across worlds {}", s_keep_weather ? "on" : "off");
    }

    if (button_clicked.rfind(k_button_prefix, 0) != 0)
        return;

    try {
        int weather_id = std::stoi(button_clicked.substr(std::char_traits<char>::length(k_button_prefix)));
        s_current_weather = weather_id;
        send_weather_packet(player, weather_id);
    } catch (...) {
        spdlog::warn("[Weather] Unknown weather button '{}'", button_clicked);
    }
}

void WeatherCommand::on_local_spawn(player::Player* player) {
    if (!s_keep_weather || s_current_weather <= 0) return;
    send_weather_packet(player, s_current_weather);
}

void WeatherCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    auto* player = resolve_player(s_core, client);
    if (!player) {
        spdlog::warn("[Weather] No game client connected");
        return;
    }

    if (args.size() < 2) {
        show_dialog(player);
        return;
    }

    // Optional shortcut: /weather <id>
    try {
        int weather_id = std::stoi(args[1]);
        if (weather_id < 1 || weather_id > 255) {
            utils::PacketUtils::send_chat_message(player, "`4Weather ID must be between 1-255.");
            return;
        }
        s_current_weather = weather_id;
        send_weather_packet(player, weather_id);
    } catch (...) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /weather [weather_id]");
    }
}

}
