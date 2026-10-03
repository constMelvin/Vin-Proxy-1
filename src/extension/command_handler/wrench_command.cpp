#include "wrench_command.hpp"

#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../server/server.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/packet_utils.hpp"
#include "fastdoor_command.hpp"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>

namespace command {

// Item id for the red square "Disable" icon (170 = Red Block)
static constexpr int kDisableIcon = 170;

core::Core* WrenchCommand::s_core = nullptr;
std::string WrenchCommand::s_saved_mode = "pull";   // LuckyProxy default wrench mode

WrenchCommand::WrenchCommand() : CommandBase(
    {"wrench", "awr"},
    {},
    "Open auto-wrench settings (pull/kick/ban)",
    0
) {}

std::unique_ptr<CommandBase> WrenchCommand::clone() const {
    return std::make_unique<WrenchCommand>(*this);
}

void WrenchCommand::set_core(core::Core* core) {
    s_core = core;
}

void WrenchCommand::execute(client::Client* client, const std::vector<std::string>&) {
    if (!client || !client->get_player() || !s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;
    send_settings_dialog(server->get_player());
}

void WrenchCommand::send_settings_dialog(player::Player* out) {
    if (!out || !s_core) return;

    auto getb = [&](const std::string& key, bool defv = false) -> bool {
        try { return s_core->get_config().get<bool>(key); }
        catch (...) { s_core->get_config().set<bool>(key, defv); return defv; }
    };

    // Active mode uses the same Ban > Kick > Pull priority the parser applies
    const bool is_ban = getb("features.wrench.auto_ban", false);
    const bool is_kick = getb("features.wrench.auto_kick", false);
    const bool is_pull = getb("features.wrench.auto_pull", false);
    const char* mode = is_ban ? "Ban" : is_kick ? "Kick" : is_pull ? "Pull" : "Disable";

    const int right_click_kick = getb("features.wrench.right_click_kick", false) ? 1 : 0;
    const int fast_entrance = FastDoorCommand::is_enabled() ? 1 : 0;

    // Mode icon button: selected mode gets the yellow frame + purple label, others red
    auto mode_button = [&](const char* id, const char* label, int icon) {
        const bool selected = std::string(mode) == label;

        // Growtopia uses large font for short text and downscales longer text (like "Disable").
        // Padding shorter labels with symmetrical spaces forces Growtopia to use the same small font.
        std::string display_label = label;
        if (display_label == "Pull") display_label = "   Pull   ";
        else if (display_label == "Kick") display_label = "   Kick   ";
        else if (display_label == "Ban") display_label = "    Ban    ";

        std::ostringstream b;
        b << "add_button_with_icon|" << id << "|" << (selected ? "`5" : "`4") << display_label << "``|"
          << (selected ? "staticYellowFrame" : "staticBlueFrame") << "|" << icon << "|\n";
        return b.str();
    };

    std::ostringstream dialog;
    dialog << "set_default_color|`o\n";
    dialog << "add_label_with_icon|big|`5Choose Wrench Mode``|left|32|\n";
    dialog << "add_textbox|`wCurrent Wrench Mode: `5" << mode << "``|left|\n";
    dialog << "add_spacer|small|\n";
    dialog << mode_button("wm_pull", "Pull", 32);
    dialog << mode_button("wm_kick", "Kick", 32);
    dialog << mode_button("wm_ban", "Ban", 32);
    dialog << mode_button("wm_disable", "Disable", kDisableIcon);
    dialog << "add_button_with_icon||END_LIST|noflags|0|\n";
    dialog << "add_spacer|small|\n";
    dialog << "add_checkbox|wrench_right_kick|`5Enable `wRight Click Kick|" << right_click_kick << "|\n";
    dialog << "add_custom_margin|x:0;y:-32|\n";
    dialog << "add_custom_textbox|`oGrants The Ability To `4Kick `9A Person With Right Click.|size:tiny;color:200,200,200,200|\n";
    dialog << "add_custom_margin|x:0;y:10|\n";
    dialog << "add_checkbox|wrench_fast_entrance|`5Enable `wFast Right Click Open/Close Entrances|" << fast_entrance << "|\n";
        dialog << "add_custom_margin|x:0;y:-32|\n";
    dialog << "add_custom_textbox|`oGrants The Ability To Open/Close Entrances By Right Mouse Click Without Additional Dialogs.|size:tiny;color:200,200,200,200|\n";
    dialog << "add_custom_margin|x:0;y:10|\n";
    dialog << "end_dialog|wrench_settings|Cancel|Okay|\n";

    packet::Variant var{};
    var.add("OnDialogRequest");
    var.add(dialog.str());
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
    out->send_packet(bs.get_data(), 0);
}

void WrenchCommand::apply_dialog_settings(const TextParse& tp) {
    if (!s_core) return;

    auto to_bool = [](const std::string& v) -> bool {
        return v == "1" || v == "true" || v == "on";
    };

    const std::string button = tp.get("buttonClicked");
    if (button == "Cancel") return;

    auto& cfg = s_core->get_config();

    // Checkboxes are sent with every click (mode button or Okay)
    cfg.set<bool>("features.wrench.right_click_kick", to_bool(tp.get("wrench_right_kick")));
    FastDoorCommand::set_enabled(to_bool(tp.get("wrench_fast_entrance")));

    // Mode buttons select exactly one mode; Okay leaves the mode as is
    const bool mode_click = button.rfind("wm_", 0) == 0;
    if (mode_click) {
        cfg.set<bool>("features.wrench.auto_pull", button == "wm_pull");
        cfg.set<bool>("features.wrench.auto_kick", button == "wm_kick");
        cfg.set<bool>("features.wrench.auto_ban", button == "wm_ban");
        if (button != "wm_disable")
            remember_mode(button.substr(3));
    }
    cfg.save();

    auto* out = s_core->get_server() ? s_core->get_server()->get_player() : nullptr;
    if (!out) return;

    if (mode_click) {
        send_settings_dialog(out);   // re-open with the new mode highlighted
    } else {
        utils::PacketUtils::send_chat_message(out, "`2Wrench settings updated``.", false);
    }
}


std::string WrenchCommand::current_mode() {
    if (!s_core) return {};
    auto getb = [](const std::string& key) -> bool {
        try { return s_core->get_config().get<bool>(key); } catch (...) { return false; }
    };
    // Same Ban > Kick > Pull priority the parser applies
    if (getb("features.wrench.auto_ban")) return "ban";
    if (getb("features.wrench.auto_kick")) return "kick";
    if (getb("features.wrench.auto_pull")) return "pull";
    return {};
}

void WrenchCommand::remember_mode(const std::string& mode) {
    if (!mode.empty()) s_saved_mode = mode;
}

const std::string& WrenchCommand::saved_mode() {
    return s_saved_mode;
}

void WrenchCommand::apply_mode(const std::string& mode) {
    if (!s_core) return;
    spdlog::info("[Wrench] Wrench mode {}", mode.empty() ? std::string("off") : mode);
    auto& cfg = s_core->get_config();
    cfg.set<bool>("features.wrench.auto_pull", mode == "pull");
    cfg.set<bool>("features.wrench.auto_kick", mode == "kick");
    cfg.set<bool>("features.wrench.auto_ban", mode == "ban");
    cfg.save();
}

player::Player* WrenchCommand::local_out() {
    if (!s_core || !s_core->get_server()) return nullptr;
    return s_core->get_server()->get_player();
}

bool WrenchCommand::get_flag(const std::string& key) {
    if (!s_core) return false;
    try { return s_core->get_config().get<bool>(key); } catch (...) { return false; }
}

void WrenchCommand::set_flag(const std::string& key, bool value) {
    if (!s_core) return;
    s_core->get_config().set<bool>(key, value);
    s_core->get_config().save();
}

// ============================================================
// /wm - Enable & Disable Wrench Mode
// ============================================================
WrenchModeToggleCommand::WrenchModeToggleCommand() : CommandBase(
    {"wm"},
    {},
    "Enable & Disable Wrench Mode",
    0
) {}

std::unique_ptr<CommandBase> WrenchModeToggleCommand::clone() const {
    return std::make_unique<WrenchModeToggleCommand>(*this);
}

void WrenchModeToggleCommand::execute(client::Client*, const std::vector<std::string>&) {
    auto* out = WrenchCommand::local_out();
    const std::string active = WrenchCommand::current_mode();
    if (!active.empty()) {
        WrenchCommand::remember_mode(active);
        WrenchCommand::apply_mode("");
        if (out) utils::PacketUtils::send_chat_message(out, "`aWrench mode is `4off.");
    } else {
        WrenchCommand::apply_mode(WrenchCommand::saved_mode());
        if (out) utils::PacketUtils::send_chat_message(out, "`aWrench mode is on. `9Mode: `a" + WrenchCommand::saved_mode());
    }
}

// ============================================================
// /ws [mode] - Set wrench mode
// ============================================================
WrenchSetModeCommand::WrenchSetModeCommand() : CommandBase(
    {"ws"},
    {"[pull|kick|ban]"},
    "Set wrench mode",
    0
) {}

std::unique_ptr<CommandBase> WrenchSetModeCommand::clone() const {
    return std::make_unique<WrenchSetModeCommand>(*this);
}

void WrenchSetModeCommand::execute(client::Client*, const std::vector<std::string>& args) {
    auto* out = WrenchCommand::local_out();
    if (args.size() < 2) {
        if (out) utils::PacketUtils::send_chat_message(out, "`4Usage: /ws [pull|kick|ban] `9(current: `a" + WrenchCommand::saved_mode() + "`9)");
        return;
    }
    std::string mode = args[1];
    std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (mode != "pull" && mode != "kick" && mode != "ban") {
        if (out) utils::PacketUtils::send_chat_message(out, "`4Unknown wrench mode. Use: pull, kick or ban");
        return;
    }

    WrenchCommand::remember_mode(mode);
    // LuckyProxy /ws only changes the mode; /wm turns wrench mode on/off
    const bool active = !WrenchCommand::current_mode().empty();
    if (active) WrenchCommand::apply_mode(mode);
    if (out) {
        utils::PacketUtils::send_chat_message(out, "`6Wrench mode set to `a" + mode +
            (active ? "" : " `9(wrench mode is off, use `0/wm`9 to enable)"));
    }
}

// ============================================================
// /rkick - Enable & Disable Right Click Kick Mode
// ============================================================
RightClickKickCommand::RightClickKickCommand() : CommandBase(
    {"rkick"},
    {},
    "Enable & Disable Right Click Kick Mode",
    0
) {}

std::unique_ptr<CommandBase> RightClickKickCommand::clone() const {
    return std::make_unique<RightClickKickCommand>(*this);
}

bool RightClickKickCommand::is_enabled() {
    return WrenchCommand::get_flag("features.wrench.right_click_kick");
}

void RightClickKickCommand::execute(client::Client*, const std::vector<std::string>&) {
    const bool enabled = !is_enabled();
    WrenchCommand::set_flag("features.wrench.right_click_kick", enabled);
    spdlog::info("[Wrench] Right click kick {}", enabled ? "enabled" : "disabled");
    if (auto* out = WrenchCommand::local_out()) {
        utils::PacketUtils::send_chat_message(out, enabled ? "`9Right Click Kick Mode Now `2Enabled"
                                                           : "`9Right Click Kick Mode Now `4Disabled");
    }
}

// ============================================================
// /setmsg <text> - Message for /wrenchmsg and /wrenchspam
// ============================================================
static std::mutex g_message_mutex;
static std::string g_message;

SetMsgCommand::SetMsgCommand() : CommandBase(
    {"setmsg"},
    {"<text>"},
    "Set the message used by /wrenchmsg and /wrenchspam",
    0
) {}

std::unique_ptr<CommandBase> SetMsgCommand::clone() const {
    return std::make_unique<SetMsgCommand>(*this);
}

std::string SetMsgCommand::get_message() {
    std::lock_guard<std::mutex> lock(g_message_mutex);
    return g_message;
}

void SetMsgCommand::execute(client::Client*, const std::vector<std::string>& args) {
    std::string text;
    for (size_t i = 1; i < args.size(); ++i) {
        if (!text.empty()) text += ' ';
        text += args[i];
    }
    {
        std::lock_guard<std::mutex> lock(g_message_mutex);
        g_message = text;
    }
    spdlog::info("[Wrench] Wrench message set to \"{}\"", text);
    if (auto* out = WrenchCommand::local_out()) {
        if (text.empty())
            utils::PacketUtils::send_chat_message(out, "`4Usage: /setmsg <text>");
        else
            utils::PacketUtils::send_chat_message(out, "`9Set Message to `w" + text);
    }
}

} 
