#include "extended_commands.hpp"
#include "autocollect_command.hpp"
#include "onspawn_commands.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/text_parse.hpp"
#include <cstring>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <sstream>
#include <thread>
#include <chrono>

namespace command {

// Helper: resolve player to send to (server player preferred)
static player::Player* resolve_player(core::Core* core, client::Client* client) {
    if (core) {
        if (auto* srv = core->get_server(); srv && srv->get_player())
            return srv->get_player();
    }
    return client ? client->get_player() : nullptr;
}

// Helper: send a dialog packet to player
static void send_dialog(player::Player* player, const std::string& dialog_data) {
    if (!player) return;
    packet::Variant variant{};
    variant.add("OnDialogRequest");
    variant.add(dialog_data);
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

// ============================================================
// ShowXYCommand
// ============================================================
core::Core* ShowXYCommand::s_core = nullptr;
bool ShowXYCommand::s_enabled = false;

ShowXYCommand::ShowXYCommand() : CommandBase({"showxy"}, {}, "Toggle show X,Y tile position", 0) {}
std::unique_ptr<CommandBase> ShowXYCommand::clone() const { return std::make_unique<ShowXYCommand>(*this); }
void ShowXYCommand::set_core(core::Core* core) { s_core = core; }
bool ShowXYCommand::is_enabled() { return s_enabled; }

void ShowXYCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`2ShowXY `9is now `2ON `9- X,Y position shown above your head");
    else
        utils::PacketUtils::send_chat_message(player, "`4ShowXY `9is now `4OFF");
    spdlog::info("ShowXYCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// UidCommand
// ============================================================
core::Core* UidCommand::s_core = nullptr;

UidCommand::UidCommand() : CommandBase({"uid"}, {}, "Show current player user ID", 0) {}
std::unique_ptr<CommandBase> UidCommand::clone() const { return std::make_unique<UidCommand>(*this); }
void UidCommand::set_core(core::Core* core) { s_core = core; }

void UidCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    auto local = utils::PlayerTracker::get_instance().get_local_player();
    utils::PacketUtils::send_chat_message(player,
        fmt::format("`9[`2VinProxy`9] Your NetID: `2{} `9| UserID (UID): `2{}", local.netID, local.userID));
    spdlog::info("UidCommand: NetID={}, UserID={}", local.netID, local.userID);
}

// ============================================================
// ScanCommand
// ============================================================
core::Core* ScanCommand::s_core = nullptr;
bool ScanCommand::s_enabled = false;

ScanCommand::ScanCommand() : CommandBase({"scan", "extract"}, {}, "Toggle world item scan/extract mode", 0) {}
std::unique_ptr<CommandBase> ScanCommand::clone() const { return std::make_unique<ScanCommand>(*this); }
void ScanCommand::set_core(core::Core* core) { s_core = core; }
bool ScanCommand::is_enabled() { return s_enabled; }

void ScanCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`2Scan Mode `9enabled - world items will be scanned/extracted");
    else
        utils::PacketUtils::send_chat_message(player, "`4Scan Mode `9disabled");
    spdlog::info("ScanCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// TrackCommand
// ============================================================
core::Core* TrackCommand::s_core = nullptr;
bool TrackCommand::s_enabled = false;

TrackCommand::TrackCommand() : CommandBase({"track"}, {}, "Toggle drop tracker log", 0) {}
std::unique_ptr<CommandBase> TrackCommand::clone() const { return std::make_unique<TrackCommand>(*this); }
void TrackCommand::set_core(core::Core* core) { s_core = core; }
bool TrackCommand::is_enabled() { return s_enabled; }
void TrackCommand::set_enabled(bool enabled) { s_enabled = enabled; }

void TrackCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`2Drop Tracker `9is now `2ON");
    else
        utils::PacketUtils::send_chat_message(player, "`2Drop Tracker `9is now `4OFF");
    spdlog::info("TrackCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// GhostCommand
// ============================================================
core::Core* GhostCommand::s_core = nullptr;
bool GhostCommand::s_enabled = false;

GhostCommand::GhostCommand() : CommandBase({"ghost"}, {}, "Toggle moderator ghost/invis mode", 0) {}
std::unique_ptr<CommandBase> GhostCommand::clone() const { return std::make_unique<GhostCommand>(*this); }
void GhostCommand::set_core(core::Core* core) { s_core = core; }
bool GhostCommand::is_enabled() { return s_enabled; }

// LuckyProxy /ghost: noclip bit in the character state; client movement is held back from the server
void GhostCommand::set_enabled(bool enabled) {
    s_enabled = enabled;
    if (s_core && s_core->get_server() && s_core->get_server()->get_player())
        SpeedCommand::send_state(s_core->get_server()->get_player());
}

void GhostCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    set_enabled(!s_enabled);
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`#@Moderator `9Mode is now `2ON `9- Ghost mode active");
    else
        utils::PacketUtils::send_chat_message(player, "`#@Moderator `9Mode is now `4OFF");
    spdlog::info("GhostCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// AutoMsgCommand
// ============================================================
core::Core* AutoMsgCommand::s_core = nullptr;
bool AutoMsgCommand::s_enabled = false;

AutoMsgCommand::AutoMsgCommand() : CommandBase({"automsg"}, {}, "Toggle auto-message broadcast bot", 0) {}
std::unique_ptr<CommandBase> AutoMsgCommand::clone() const { return std::make_unique<AutoMsgCommand>(*this); }
void AutoMsgCommand::set_core(core::Core* core) { s_core = core; }
bool AutoMsgCommand::is_enabled() { return s_enabled; }

void AutoMsgCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`6AutoMsg `9is now `2enabled");
    else
        utils::PacketUtils::send_chat_message(player, "`6AutoMsg `9is now `4disabled");
    spdlog::info("AutoMsgCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// FastRecycleCommand
// ============================================================
core::Core* FastRecycleCommand::s_core = nullptr;
bool FastRecycleCommand::s_enabled = false;

FastRecycleCommand::FastRecycleCommand() : CommandBase({"fr", "fastrecycle"}, {}, "Toggle fast recycle (auto-confirm)", 0) {}
std::unique_ptr<CommandBase> FastRecycleCommand::clone() const { return std::make_unique<FastRecycleCommand>(*this); }
void FastRecycleCommand::set_core(core::Core* core) { s_core = core; }
bool FastRecycleCommand::is_enabled() { return s_enabled; }

void FastRecycleCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`aFast Recycle `9is now `2enabled");
    else
        utils::PacketUtils::send_chat_message(player, "`aFast Recycle `9is now `4disabled");
    spdlog::info("FastRecycleCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// WrenchMsgCommand
// ============================================================
core::Core* WrenchMsgCommand::s_core = nullptr;
bool WrenchMsgCommand::s_enabled = false;

WrenchMsgCommand::WrenchMsgCommand() : CommandBase({"wrenchmsg"}, {}, "Toggle show wrench chat messages", 0) {}
std::unique_ptr<CommandBase> WrenchMsgCommand::clone() const { return std::make_unique<WrenchMsgCommand>(*this); }
void WrenchMsgCommand::set_core(core::Core* core) { s_core = core; }
bool WrenchMsgCommand::is_enabled() { return s_enabled; }

void WrenchMsgCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`aWrenchMsg `9is now `2enabled");
    else
        utils::PacketUtils::send_chat_message(player, "`aWrenchMsg `9is now `4disabled");
    spdlog::info("WrenchMsgCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// WrenchSpamCommand
// ============================================================
core::Core* WrenchSpamCommand::s_core = nullptr;
bool WrenchSpamCommand::s_enabled = false;

WrenchSpamCommand::WrenchSpamCommand() : CommandBase({"wrenchspam"}, {}, "Toggle wrench spam auto mode", 0) {}
std::unique_ptr<CommandBase> WrenchSpamCommand::clone() const { return std::make_unique<WrenchSpamCommand>(*this); }
void WrenchSpamCommand::set_core(core::Core* core) { s_core = core; }
bool WrenchSpamCommand::is_enabled() { return s_enabled; }

void WrenchSpamCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`aWrenchSpam `9is now `2enabled");
    else
        utils::PacketUtils::send_chat_message(player, "`aWrenchSpam `9is now `4disabled");
    spdlog::info("WrenchSpamCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// BlinkCommand
// ============================================================
core::Core* BlinkCommand::s_core = nullptr;
bool BlinkCommand::s_enabled = false;

BlinkCommand::BlinkCommand() : CommandBase({"blink"}, {}, "Toggle rainbow blink color-cycle mode", 0) {}
std::unique_ptr<CommandBase> BlinkCommand::clone() const { return std::make_unique<BlinkCommand>(*this); }
void BlinkCommand::set_core(core::Core* core) { s_core = core; }
bool BlinkCommand::is_enabled() { return s_enabled; }

void BlinkCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`2Blink Mode `9enabled - rainbow color cycle active");
    else
        utils::PacketUtils::send_chat_message(player, "`4Blink Mode `9disabled");
    spdlog::info("BlinkCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// FastVendToggleCommand
// ============================================================
core::Core* FastVendToggleCommand::s_core = nullptr;
bool FastVendToggleCommand::s_enabled = false;

FastVendToggleCommand::FastVendToggleCommand() : CommandBase({"fastvend"}, {}, "Toggle fast vend auto-stock mode", 0) {}
std::unique_ptr<CommandBase> FastVendToggleCommand::clone() const { return std::make_unique<FastVendToggleCommand>(*this); }
void FastVendToggleCommand::set_core(core::Core* core) { s_core = core; }
bool FastVendToggleCommand::is_enabled() { return s_enabled; }

void FastVendToggleCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`aFast Vend `9is now `2enabled `9- auto-confirm vend dialogs");
    else
        utils::PacketUtils::send_chat_message(player, "`aFast Vend `9is now `4disabled");
    spdlog::info("FastVendToggleCommand: {}", s_enabled ? "ON" : "OFF");
}

// ============================================================
// SpeedCommand
// ============================================================
core::Core* SpeedCommand::s_core = nullptr;
float SpeedCommand::s_speed = SpeedCommand::DEFAULT_SPEED;
float SpeedCommand::s_gravity = SpeedCommand::DEFAULT_GRAVITY;

SpeedCommand::SpeedCommand() : CommandBase({"speed"}, {"[speed] [gravity]"}, "Open speed/gravity settings", 0) {}
std::unique_ptr<CommandBase> SpeedCommand::clone() const { return std::make_unique<SpeedCommand>(*this); }
void SpeedCommand::set_core(core::Core* core) { s_core = core; }
float SpeedCommand::get_speed() { return s_speed; }
float SpeedCommand::get_gravity() { return s_gravity; }
bool SpeedCommand::is_custom() { return s_speed != DEFAULT_SPEED || s_gravity != DEFAULT_GRAVITY; }

// LuckyProxy events.cpp /speed: "Speed Settings" dialog with Speed + Gravity inputs
void SpeedCommand::show_dialog(player::Player* player) {
    std::ostringstream dlg;
    dlg << "add_label_with_icon|big|Speed Settings|left|2324|\n";
    dlg << "add_text_input|speed_x|`9Speed:|" << s_speed << "|7|\n";
    dlg << "add_text_input|speed_y|`9Gravity:|" << s_gravity << "|7|\n";
    dlg << "end_dialog|speed_page|Cancel|Okay|\n";
    send_dialog(player, dlg.str());
}

// LuckyProxy server::sendState: PACKET_SET_CHARACTER_STATE to the local client
void SpeedCommand::send_state(player::Player* player) {
    if (!player) return;
    auto local = utils::PlayerTracker::get_instance().get_local_player();
    if (local.netID <= 0) return;

    uint8_t data[56];
    memset(data, 0, sizeof(data));

    int type = static_cast<int>(packet::PACKET_SET_CHARACTER_STATE);
    int32_t nid = static_cast<int32_t>(local.netID);
    int state = (1 << 1) | (1 << 24);   // double jump + super supporter (as LuckyProxy)
    if (GhostCommand::is_enabled()) state |= 1 << 0;   // ghost / noclip (LuckyProxy sendState)
    float x = 1000.0f, y = 400.0f;
    float waterspeed = 200.0f;
    memcpy(data + 0, &type, 4);
    memcpy(data + 4, &nid, 4);
    memcpy(data + 16, &waterspeed, 4);
    memcpy(data + 20, &state, 4);
    memcpy(data + 24, &x, 4);
    memcpy(data + 28, &y, 4);
    memcpy(data + 32, &s_speed, 4);     // vec_x2 = move speed
    memcpy(data + 36, &s_gravity, 4);   // vec_y2 = gravity
    data[2] = 128;                      // build range
    data[3] = 128;                      // punch range

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write_data(reinterpret_cast<const std::byte*>(data), sizeof(data));
    (void)player->send_packet(bs.get_data(), 0);
}

// LuckyProxy apply_speed_value: read speed_x / speed_y from dialog_return, then sendState
void SpeedCommand::handle_dialog_response(player::Player* player, const std::string& raw) {
    TextParse tp{raw};
    auto apply = [&](const char* key, float& target) {
        std::string val = tp.get(key);
        if (val.empty()) return;
        try {
            float f = std::stof(val);
            if (f > 0.0f) target = f;   // 0 / negative would freeze or break movement
        } catch (...) {}
    };
    apply("speed_x", s_speed);
    apply("speed_y", s_gravity);
    send_state(player);
    if (player)
        utils::PacketUtils::send_chat_message(player, fmt::format("`9Speed: `2{} `9Gravity: `2{}", s_speed, s_gravity));
    spdlog::info("SpeedCommand: speed={} gravity={}", s_speed, s_gravity);
}

void SpeedCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    if (args.size() < 2) {
        show_dialog(player);
        return;
    }

    // Optional shortcut: /speed <speed> [gravity]
    try {
        float spd = std::stof(args[1]);
        float grav = args.size() > 2 ? std::stof(args[2]) : s_gravity;
        if (spd <= 0.0f || grav <= 0.0f) {
            utils::PacketUtils::send_chat_message(player, "`4Speed and gravity must be greater than 0.");
            return;
        }
        s_speed = spd;
        s_gravity = grav;
        send_state(player);
        utils::PacketUtils::send_chat_message(player, fmt::format("`9Speed: `2{} `9Gravity: `2{}", s_speed, s_gravity));
    } catch (...) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /speed [speed] [gravity]");
    }
}

// ============================================================
// SetTaxCommand
// ============================================================
core::Core* SetTaxCommand::s_core = nullptr;
int SetTaxCommand::s_tax = 5;

SetTaxCommand::SetTaxCommand() : CommandBase({"settax"}, {"[%]"}, "Set game tax percentage (e.g. 5 = 5%)", 0) {}
std::unique_ptr<CommandBase> SetTaxCommand::clone() const { return std::make_unique<SetTaxCommand>(*this); }
void SetTaxCommand::set_core(core::Core* core) { s_core = core; }
int SetTaxCommand::get_tax() { return s_tax; }

void SetTaxCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    if (args.size() < 2) {
        utils::PacketUtils::send_chat_message(player,
            fmt::format("`9Current tax: `2{}%`9. Usage: /settax [percentage]", s_tax));
        return;
    }

    try {
        int tax = std::stoi(args[1]);
        if (tax < 0 || tax > 100) {
            utils::PacketUtils::send_chat_message(player, "`4Tax must be between 0-100%.");
            return;
        }
        s_tax = tax;
        utils::PacketUtils::send_chat_message(player, fmt::format("`9Tax set to `2{}%", s_tax));
        spdlog::info("SetTaxCommand: tax={}%", s_tax);
    } catch (...) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /settax [percentage]");
    }
}

// ============================================================
// GameBetCommand
// ============================================================
core::Core* GameBetCommand::s_core = nullptr;
int GameBetCommand::s_last_drop_amount = 0;

GameBetCommand::GameBetCommand() : CommandBase({"game"}, {"[bet]"}, "Calculate game bet tax and set drop amount", 0) {}
std::unique_ptr<CommandBase> GameBetCommand::clone() const { return std::make_unique<GameBetCommand>(*this); }
void GameBetCommand::set_core(core::Core* core) { s_core = core; }
int GameBetCommand::get_last_drop_amount() { return s_last_drop_amount; }

void GameBetCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    if (args.size() < 2) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /game [bet amount]");
        return;
    }

    try {
        int bet = std::stoi(args[1]);
        int tax_pct = SetTaxCommand::get_tax();
        int total = bet * 2;
        int tax_taken = (total * tax_pct) / 100;
        int to_drop = total - tax_taken;
        s_last_drop_amount = to_drop;

        utils::PacketUtils::send_chat_message(player,
            fmt::format("`9Bet: `b{}WLs `9vs `b{}WLs", bet, bet));
        utils::PacketUtils::send_chat_message(player,
            fmt::format("`9Amount to drop (after {}% tax): `2{}WLs", tax_pct, to_drop));
        utils::PacketUtils::send_chat_message(player,
            fmt::format("`9Tax taken: `4{}WLs `9| Use `2/gdrop `9to drop now", tax_taken));
        spdlog::info("GameBetCommand: bet={} tax={}% drop={}", bet, tax_pct, to_drop);
    } catch (...) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /game [bet amount]");
    }
}

// ============================================================
// GDropCommand
// ============================================================
core::Core* GDropCommand::s_core = nullptr;

GDropCommand::GDropCommand() : CommandBase({"gdrop", "gd"}, {}, "Drop prize to winner with tax deduction", 0) {}
std::unique_ptr<CommandBase> GDropCommand::clone() const { return std::make_unique<GDropCommand>(*this); }
void GDropCommand::set_core(core::Core* core) { s_core = core; }

void GDropCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    int amount = GameBetCommand::get_last_drop_amount();
    if (amount <= 0) {
        utils::PacketUtils::send_chat_message(player,
            "`4No game bet set. Use `2/game [bet]`4 first to calculate drop amount.");
        return;
    }

    utils::PacketUtils::send_chat_message(player,
        fmt::format("`9Dropping `2{}WLs `9to winner position...", amount));
    utils::PacketUtils::send_chat_message(player,
        fmt::format("`9Use `2/dw {} `9to execute the drop at winner position.", amount));
    spdlog::info("GDropCommand: requested drop of {}WLs", amount);
}

// ============================================================
// CollectCommand
// ============================================================
core::Core* CollectCommand::s_core = nullptr;

CollectCommand::CollectCommand() : CommandBase({"collect"}, {}, "Open the auto collect page (enable/disable + range)", 0) {}
std::unique_ptr<CommandBase> CollectCommand::clone() const { return std::make_unique<CollectCommand>(*this); }
void CollectCommand::set_core(core::Core* core) { s_core = core; }

// /collect opens the Auto Collect page (enable/disable + editable range)
void CollectCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    AutoCollectCommand::show_dialog(player);
    spdlog::info("CollectCommand: auto collect dialog sent");
}

// ============================================================
// CountryListCommand
// ============================================================
core::Core* CountryListCommand::s_core = nullptr;

CountryListCommand::CountryListCommand() : CommandBase({"countrylist", "clist"}, {}, "Show all country flag IDs", 0) {}
std::unique_ptr<CommandBase> CountryListCommand::clone() const { return std::make_unique<CountryListCommand>(*this); }
void CountryListCommand::set_core(core::Core* core) { s_core = core; }

void CountryListCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    std::ostringstream dlg;
    dlg << "set_default_color|`o\n";
    dlg << "add_label_with_icon|big|`2Country Flag List``|left|3394|\n";
    dlg << "add_spacer|small|\n";
    dlg << "add_textbox|`9Use `/flag [code]` to set your flag|left|\n";
    dlg << "add_spacer|small|\n";
    const std::vector<std::pair<std::string,std::string>> countries = {
        {"af","Afghanistan"}, {"al","Albania"}, {"dz","Algeria"}, {"ar","Argentina"},
        {"am","Armenia"}, {"au","Australia"}, {"at","Austria"}, {"az","Azerbaijan"},
        {"bs","Bahamas"}, {"bd","Bangladesh"}, {"be","Belgium"}, {"br","Brazil"},
        {"bg","Bulgaria"}, {"ca","Canada"}, {"cl","Chile"}, {"cn","China"},
        {"co","Colombia"}, {"hr","Croatia"}, {"cz","Czech Republic"}, {"dk","Denmark"},
        {"eg","Egypt"}, {"fi","Finland"}, {"fr","France"}, {"de","Germany"},
        {"gh","Ghana"}, {"gr","Greece"}, {"gt","Guatemala"}, {"hk","Hong Kong"},
        {"hu","Hungary"}, {"in","India"}, {"id","Indonesia"}, {"ir","Iran"},
        {"iq","Iraq"}, {"ie","Ireland"}, {"il","Israel"}, {"it","Italy"},
        {"jm","Jamaica"}, {"jp","Japan"}, {"jo","Jordan"}, {"kz","Kazakhstan"},
        {"ke","Kenya"}, {"kr","South Korea"}, {"kw","Kuwait"}, {"lb","Lebanon"},
        {"lt","Lithuania"}, {"my","Malaysia"}, {"mx","Mexico"}, {"ma","Morocco"},
        {"nl","Netherlands"}, {"nz","New Zealand"}, {"ng","Nigeria"}, {"no","Norway"},
        {"pk","Pakistan"}, {"pa","Panama"}, {"ph","Philippines"}, {"pl","Poland"},
        {"pt","Portugal"}, {"qa","Qatar"}, {"ro","Romania"}, {"ru","Russia"},
        {"sa","Saudi Arabia"}, {"rs","Serbia"}, {"sg","Singapore"}, {"za","South Africa"},
        {"es","Spain"}, {"se","Sweden"}, {"ch","Switzerland"}, {"sy","Syria"},
        {"tw","Taiwan"}, {"th","Thailand"}, {"tn","Tunisia"}, {"tr","Turkey"},
        {"ua","Ukraine"}, {"ae","UAE"}, {"gb","United Kingdom"}, {"us","USA"},
        {"uy","Uruguay"}, {"uz","Uzbekistan"}, {"ve","Venezuela"}, {"vn","Vietnam"},
        {"ye","Yemen"},
    };
    for (const auto& [code, name] : countries) {
        dlg << fmt::format("add_textbox|`c{}:`# {}|left|2480|\n", code, name);
    }
    dlg << "add_spacer|small|\n";
    dlg << "end_dialog|country_list_dlg|Close||\n";
    dlg << "add_quick_exit|\n";
    send_dialog(player, dlg.str());
    spdlog::info("CountryListCommand: dialog sent");
}

// ============================================================
// WorldOptionsCommand
// ============================================================
core::Core* WorldOptionsCommand::s_core = nullptr;

WorldOptionsCommand::WorldOptionsCommand() : CommandBase({"world"}, {}, "Open world management options dialog", 0) {}
std::unique_ptr<CommandBase> WorldOptionsCommand::clone() const { return std::make_unique<WorldOptionsCommand>(*this); }
void WorldOptionsCommand::set_core(core::Core* core) { s_core = core; }

void WorldOptionsCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    std::ostringstream dlg;
    dlg << "set_default_color|`o\n";
    dlg << "add_label_with_icon|big|`2World Commands``|left|32|\n";
    dlg << "add_spacer|small|\n";
    dlg << "add_button|pullall|`5Pull All Players``|noflags|0|0|\n";
    dlg << "add_button|banall|`4Ban All Players``|noflags|0|0|\n";
    dlg << "add_button|killall|`4Kick All Players``|noflags|0|0|\n";
    dlg << "add_spacer|small|\n";
    dlg << "add_quick_exit|\n";
    dlg << "end_dialog|world_opts_dlg|Cancel||\n";
    send_dialog(player, dlg.str());
    spdlog::info("WorldOptionsCommand: dialog sent");
}

// ============================================================
// HotkeysCommand
// ============================================================
core::Core* HotkeysCommand::s_core = nullptr;

HotkeysCommand::HotkeysCommand() : CommandBase({"hotkeys", "hotkey"}, {}, "Configure chat command hotkey shortcuts", 0) {}
std::unique_ptr<CommandBase> HotkeysCommand::clone() const { return std::make_unique<HotkeysCommand>(*this); }
void HotkeysCommand::set_core(core::Core* core) { s_core = core; }

void HotkeysCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    std::ostringstream dlg;
    dlg << "set_default_color|`o\n";
    dlg << "add_label_with_icon|big|`2Hotkey Configuration``|left|4428|\n";
    dlg << "add_spacer|small|\n";
    dlg << "add_textbox|`9Assign chat commands to quick-access hotkeys.|left|\n";
    dlg << "add_spacer|small|\n";
    dlg << "add_input_box|hky_1|`9Hotkey 1 Command `2(with /)`9:|/proxy|20|\n";
    dlg << "add_input_box|hky_2|`9Hotkey 2 Command `2(with /)`9:|/pullall|20|\n";
    dlg << "add_input_box|hky_3|`9Hotkey 3 Command `2(with /)`9:|/banall|20|\n";
    dlg << "add_input_box|hky_4|`9Hotkey 4 Command `2(with /)`9:|/relog|20|\n";
    dlg << "add_spacer|small|\n";
    dlg << "add_smalltext|`7Type any /command to bind it as a hotkey|left|\n";
    dlg << "end_dialog|hotkeys_dlg|Cancel|Save|\n";
    dlg << "add_quick_exit|\n";
    send_dialog(player, dlg.str());
    spdlog::info("HotkeysCommand: dialog sent");
}

// ============================================================
// DropBGLAliasCommand
// ============================================================
core::Core* DropBGLAliasCommand::s_core = nullptr;

DropBGLAliasCommand::DropBGLAliasCommand() : CommandBase({"dropbgl"}, {"<amount>"}, "Drop specified amount of BGLs", 0) {}
std::unique_ptr<CommandBase> DropBGLAliasCommand::clone() const { return std::make_unique<DropBGLAliasCommand>(*this); }
void DropBGLAliasCommand::set_core(core::Core* core) { s_core = core; }

void DropBGLAliasCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    if (args.size() < 2) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /dropbgl [amount]");
        return;
    }

    try {
        int amount = std::stoi(args[1]);
        if (amount <= 0) {
            utils::PacketUtils::send_chat_message(player, "`4Amount must be positive.");
            return;
        }
        utils::PacketUtils::send_chat_message(player,
            fmt::format("`9Dropping `2{} `9Blue Gem Locks...", amount));
        utils::PacketUtils::send_chat_message(player,
            fmt::format("`7Tip: Use `/dbgl {}` for the same result.", amount));
    } catch (...) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /dropbgl [amount]");
    }
}

} // namespace command
