#include "extended_commands.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
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

void GhostCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
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
// AutoPullCommand
// ============================================================
core::Core* AutoPullCommand::s_core = nullptr;
bool AutoPullCommand::s_enabled = false;

AutoPullCommand::AutoPullCommand() : CommandBase({"autopull", "pullauto"}, {}, "Toggle auto pull players on join", 0) {}
std::unique_ptr<CommandBase> AutoPullCommand::clone() const { return std::make_unique<AutoPullCommand>(*this); }
void AutoPullCommand::set_core(core::Core* core) { s_core = core; }
bool AutoPullCommand::is_enabled() { return s_enabled; }

void AutoPullCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;
    s_enabled = !s_enabled;
    if (s_enabled)
        utils::PacketUtils::send_chat_message(player, "`2Auto Pull `9is now `2ON `9- players will be pulled when they join");
    else
        utils::PacketUtils::send_chat_message(player, "`2Auto Pull `9is now `4OFF");
    spdlog::info("AutoPullCommand: {}", s_enabled ? "ON" : "OFF");
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
int SpeedCommand::s_speed = 1;

SpeedCommand::SpeedCommand() : CommandBase({"speed"}, {"[1-5]"}, "Set movement speed multiplier", 0) {}
std::unique_ptr<CommandBase> SpeedCommand::clone() const { return std::make_unique<SpeedCommand>(*this); }
void SpeedCommand::set_core(core::Core* core) { s_core = core; }
int SpeedCommand::get_speed() { return s_speed; }

void SpeedCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    if (args.size() < 2) {
        // Show speed selection dialog
        std::ostringstream dlg;
        dlg << "set_default_color|`o\n";
        dlg << "add_label_with_icon|big|`2Speed Selection``|left|2246|\n";
        dlg << "add_spacer|small|\n";
        dlg << fmt::format("add_textbox|`9Current Speed: `2{}x``|left|\n", s_speed);
        dlg << "add_spacer|small|\n";
        dlg << "add_button|speed_1|`91x Speed (Normal)``|noflags|0|0|\n";
        dlg << "add_button|speed_2|`92x Speed``|noflags|0|0|\n";
        dlg << "add_button|speed_3|`93x Speed``|noflags|0|0|\n";
        dlg << "add_button|speed_4|`94x Speed``|noflags|0|0|\n";
        dlg << "add_button|speed_5|`25x Speed (Max)``|noflags|0|0|\n";
        dlg << "end_dialog|speed_dialog|Cancel||\n";
        dlg << "add_quick_exit|\n";
        send_dialog(player, dlg.str());
        return;
    }

    try {
        int spd = std::stoi(args[1]);
        if (spd < 1 || spd > 5) {
            utils::PacketUtils::send_chat_message(player, "`4Invalid speed. Use 1-5.");
            return;
        }
        s_speed = spd;
        utils::PacketUtils::send_chat_message(player, fmt::format("`9Speed set to `2{}x", s_speed));
        spdlog::info("SpeedCommand: speed set to {}x", s_speed);
    } catch (...) {
        utils::PacketUtils::send_chat_message(player, "`4Usage: /speed [1-5]");
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

CollectCommand::CollectCommand() : CommandBase({"collect"}, {}, "Collect floating items within 10 tiles", 0) {}
std::unique_ptr<CommandBase> CollectCommand::clone() const { return std::make_unique<CollectCommand>(*this); }
void CollectCommand::set_core(core::Core* core) { s_core = core; }

void CollectCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    utils::PacketUtils::send_chat_message(player,
        "`2Collect `9triggered - picking up floating items within 10 tiles...");
    spdlog::info("CollectCommand: triggered collect");
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
// AutoFishCommand
// ============================================================
core::Core* AutoFishCommand::s_core = nullptr;
bool AutoFishCommand::s_enabled = false;

AutoFishCommand::AutoFishCommand() : CommandBase({"afish", "autofish"}, {}, "Auto fish bot settings", 0) {}
std::unique_ptr<CommandBase> AutoFishCommand::clone() const { return std::make_unique<AutoFishCommand>(*this); }
void AutoFishCommand::set_core(core::Core* core) { s_core = core; }
bool AutoFishCommand::is_enabled() { return s_enabled; }

void AutoFishCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    std::ostringstream dlg;
    dlg << "set_default_color|`o\n";
    dlg << "add_label_with_icon|big|`9Auto Fish Options``|left|3436|\n";
    dlg << "add_spacer|small|\n";
    dlg << fmt::format("add_textbox|`9Status: {}`9|left|\n", s_enabled ? "`2ACTIVE" : "`4INACTIVE");
    dlg << "add_spacer|small|\n";
    dlg << "add_textbox|`9To start fishing: Type `2/fish `9then press bait on water|left|\n";
    dlg << "add_spacer|small|\n";
    dlg << fmt::format("add_button|afish_toggle|{}``|noflags|0|0|\n",
        s_enabled ? "`4Disable Auto Fish" : "`2Enable Auto Fish");
    dlg << "add_spacer|small|\n";
    dlg << "add_smalltext|`7Type /fish again to disable while active|left|\n";
    dlg << "add_quick_exit|\n";
    dlg << "end_dialog|afish_dlg|Close||\n";
    send_dialog(player, dlg.str());
    spdlog::info("AutoFishCommand: dialog sent (enabled={})", s_enabled);
}

// ============================================================
// AutoFarmCommand
// ============================================================
core::Core* AutoFarmCommand::s_core = nullptr;
bool AutoFarmCommand::s_enabled = false;

AutoFarmCommand::AutoFarmCommand() : CommandBase({"afarm", "autofarm"}, {}, "Auto farm bot settings", 0) {}
std::unique_ptr<CommandBase> AutoFarmCommand::clone() const { return std::make_unique<AutoFarmCommand>(*this); }
void AutoFarmCommand::set_core(core::Core* core) { s_core = core; }
bool AutoFarmCommand::is_enabled() { return s_enabled; }

void AutoFarmCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    std::ostringstream dlg;
    dlg << "set_default_color|`o\n";
    dlg << "add_label_with_icon|big|`5Auto Farm``|left|898|\n";
    dlg << "add_spacer|small|\n";
    dlg << fmt::format("add_textbox|`9Status: {}`9|left|\n", s_enabled ? "`2ACTIVE" : "`4INACTIVE");
    dlg << "add_spacer|small|\n";
    dlg << "add_label_with_icon|small|`9Select Farmable Item:``|left|242|\n";
    dlg << "add_button_with_icon|afarm_pick|`2Select From Inventory``|staticBlueFrame|242|\n";
    dlg << "add_spacer|small|\n";
    dlg << fmt::format("add_button|afarm_toggle|{}``|noflags|0|0|\n",
        s_enabled ? "`4Stop Auto Farm" : "`2Start Auto Farm");
    dlg << "add_spacer|small|\n";
    dlg << "add_textbox|`9Tip: Auto disables if you get pulled|left|\n";
    dlg << "add_quick_exit|\n";
    dlg << "end_dialog|afarm_dlg|Close||\n";
    send_dialog(player, dlg.str());
    spdlog::info("AutoFarmCommand: dialog sent (enabled={})", s_enabled);
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
// OptionsPageCommand
// ============================================================
core::Core* OptionsPageCommand::s_core = nullptr;

OptionsPageCommand::OptionsPageCommand() : CommandBase({"options"}, {}, "Open all features options page", 0) {}
std::unique_ptr<CommandBase> OptionsPageCommand::clone() const { return std::make_unique<OptionsPageCommand>(*this); }
void OptionsPageCommand::set_core(core::Core* core) { s_core = core; }

void OptionsPageCommand::execute(client::Client* client, const std::vector<std::string>&) {
    auto* player = resolve_player(s_core, client);
    if (!player) return;

    std::ostringstream dlg;
    dlg << "set_default_color|`o\n";
    dlg << "add_label_with_icon|big|`2VinProxy Options``|left|32|\n";
    dlg << "add_spacer|small|\n";

    // Toggle features as checkboxes
    dlg << fmt::format("add_checkbox|opt_showxy|`^Show X,Y Position|{}\n", ShowXYCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Display your tile coordinates above your head\n";
    dlg << fmt::format("add_checkbox|opt_track|`^Drop Tracker|{}\n", TrackCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Log all item drops in this world\n";
    dlg << fmt::format("add_checkbox|opt_scan|`^Scan Mode|{}\n", ScanCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Scan and extract world items\n";
    dlg << fmt::format("add_checkbox|opt_ghost|`^Ghost Mode|{}\n", GhostCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Moderator ghost/invisibility mode\n";
    dlg << fmt::format("add_checkbox|opt_autopull|`^Auto Pull|{}\n", AutoPullCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Auto pull players when they join\n";
    dlg << fmt::format("add_checkbox|opt_automsg|`^Auto Message|{}\n", AutoMsgCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Broadcast automated messages\n";
    dlg << fmt::format("add_checkbox|opt_fr|`^Fast Recycle|{}\n", FastRecycleCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Auto-confirm recycle dialogs\n";
    dlg << fmt::format("add_checkbox|opt_wrenchmsg|`^Wrench Messages|{}\n", WrenchMsgCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Show wrench action chat messages\n";
    dlg << fmt::format("add_checkbox|opt_wrenchspam|`^Wrench Spam|{}\n", WrenchSpamCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Auto wrench spam mode\n";
    dlg << fmt::format("add_checkbox|opt_blink|`^Blink Mode|{}\n", BlinkCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Rainbow color cycle blink visual\n";
    dlg << fmt::format("add_checkbox|opt_fastvend|`^Fast Vend|{}\n", FastVendToggleCommand::is_enabled() ? 1 : 0);
    dlg << "add_desc_text|`9Auto-confirm vend dialogs\n";

    dlg << "add_spacer|small|\n";
    dlg << "end_dialog|options_page_dlg|Cancel|Apply|\n";
    dlg << "add_quick_exit|\n";
    send_dialog(player, dlg.str());
    spdlog::info("OptionsPageCommand: dialog sent");
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
