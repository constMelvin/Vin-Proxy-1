#include "moddetect_command.hpp"
#include "lucky_common.hpp"
#include "autocollect_command.hpp"
#include "save_world_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/dialog.hpp"
#include <thread>

#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../utils/byte_stream.hpp"

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <unordered_map>
#include <algorithm>
#include <chrono>

namespace command {

core::Core* ModDetectCommand::s_core = nullptr;

namespace {
const std::unordered_map<std::string, std::string> kModerators = {
    {"30835728", "NekoRei"}, {"36310143", "Pangloss"}, {"41539504", "Hufflewitz"},
    {"41538133", "Pharaohboi"}, {"36709249", "vvCephei"}, {"36559671", "GadnokBOW"},
    {"25374", "Anulot"}, {"553625", "Jenuine"}, {"73346", "Aimster"},
    {"629331", "BlueDwarf"}, {"536707", "Misthios"}, {"35869182", "Ottowo"},
    {"29160268", "Sabaei"}, {"15006163", "Bleulabel"}, {"16966321", "Fournos"},
    {"32036726", "Kailyx"}, {"22353525", "Ubidev"}, {"22242821", "VectorCat"},
    {"25181947", "ThePsyborg"}, {"24233063", "nPlus1"}, {"24969470", "Qadevone"},
    {"36713808", "Gatello"}, {"308143", "Tharapita"}, {"41553179", "Phainomai"},
    {"41552957", "Santackles"}, {"3804202", "Explorate"}, {"41208310", "Akrius"},
    {"7275489", "Elbanna"}, {"43432233", "Serenite"}, {"38753466", "Lunatary"},
    {"46919291", "Cynced"}, {"42705852", "WindyPlay"}, {"41263973", "Trinculo"},
    {"47093010", "HollowDragon"}, {"47094621", "Sadilus"}, {"47091700", "Kintsugin"},
    {"47119248", "MrKeter"}, {"47120399", "Circulatum"}, {"44250099", "Caitriona"}
};

std::unordered_map<std::string, std::chrono::steady_clock::time_point> g_last_alert;

std::string clean_name(std::string name) {
    name.erase(std::remove(name.begin(), name.end(), '\''), name.end());
    name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
    size_t pos = 0;
    while ((pos = name.find('`')) != std::string::npos) {
        if (pos + 1 < name.length()) name.erase(pos, 2);
        else {
            name.erase(pos, 1);
            break;
        }
    }
    if (!name.empty()) {
        const auto first = name.find_first_not_of(" \t\r\n");
        const auto last = name.find_last_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        name = name.substr(first, last - first + 1);
    }
    return name;
}

void send_mod_notification(core::Core* core, const std::string& mod_name, const std::string& uid) {
    if (!core || !core->get_server() || !core->get_server()->get_player()) return;
    auto* player = core->get_server()->get_player();

    packet::Variant var{};
    var.add("OnAddNotification");
    var.add("interface/atomic_button.rttex");
    var.add(fmt::format("`4Moderator Spawned``: `w{} `` (/run to escape)", mod_name));
    var.add("audio/hub_open.wav");
    var.add(0);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = -1;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());
    player->send_packet(bs.get_data(), 0);

    utils::PacketUtils::send_chat_message(
        player,
        fmt::format("`4[MODDETECT]`` Moderator Spawned: `w{} ``(UID: {})", mod_name, uid),
        false
    );
}

void send_console_message(core::Core* core, const std::string& message) {
    if (!core || !core->get_server() || !core->get_server()->get_player()) return;
    auto* player = core->get_server()->get_player();

    packet::Variant var{};
    var.add("OnConsoleMessage");
    var.add(message);

    std::vector<std::byte> ext_data = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = -1;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext_data.data(), ext_data.size());
    player->send_packet(bs.get_data(), 0);
}
} 

ModDetectCommand::ModDetectCommand() : CommandBase(
    {"moddetect"},
    {},
    "Toggle moderator spawn detection",
    0
) {}

std::unique_ptr<CommandBase> ModDetectCommand::clone() const {
    return std::make_unique<ModDetectCommand>(*this);
}

void ModDetectCommand::set_core(core::Core* core) {
    s_core = core;
}

void ModDetectCommand::execute(client::Client* client, const std::vector<std::string>&) {
    if (!client || !client->get_player() || !s_core) return;

    
    bool enabled = true;
    try {
        enabled = s_core->get_config().get<bool>("features.moddetect");
    } catch (...) {
        enabled = true;
    }

    enabled = !enabled;
    s_core->get_config().set<bool>("features.moddetect", enabled);

    send_console_message(
        s_core,
        enabled ? "`2ModDetect enabled`` - watching moderator spawns."
                : "`4ModDetect disabled``."
    );
}

bool ModDetectCommand::is_enabled() {
    if (!s_core) return false;
    try {
        return s_core->get_config().get<bool>("features.moddetect");
    } catch (...) {
        return false;
    }
}

void ModDetectCommand::toggle() {
    if (!s_core) return;
    s_core->get_config().set<bool>("features.moddetect", !is_enabled());
}

// ------------------------------------------------------------
// Mod Detect Settings (LuckyProxy "kadaryt" page)
// ------------------------------------------------------------
namespace {
struct ModAction {
    const char* id;          // checkbox id (LuckyProxy names)
    const char* config_key;
    const char* label;
    const char* desc;
};
const ModAction kModActions[] = {
    {"leaveworld", "features.moddetect.exit_world", "`2Auto `9Exit World", "Automatically Exit World When Moderator Come."},
    {"isjunkvisusnx", "features.moddetect.ban_all", "`2Ban `9Everyone in World", "Automatically Ban Everyone in World."},
    {"unaccesslucky", "features.moddetect.unaccess", "`2Auto `9Unaccess", "Automatically Unaccess Yourself in The World."},
    {"masuksave", "features.moddetect.warp_save", "`2Auto `9Warp To Save World", "Automatically Warp To Save World /setsave"},
    {"tamaknibos", "features.moddetect.collect", "`2Auto `9Collect Items 10 Far", "Automatically Auto Collect 10 Far."},
};

bool mod_action_on(const char* config_key) { return lucky::cfg_flag(config_key, false); }

} // namespace

void ModDetectCommand::show_settings_dialog() {
    utils::Dialog dlg;
    dlg.label_with_icon("`2Auto Mod Detect Settings", 278)
       .textbox("`2When `#@Moderator `2 Joins The World You will:");
    for (const auto& a : kModActions)
        dlg.checkbox(a.id, a.label, mod_action_on(a.config_key)).description(a.desc);
    dlg.end_dialog("mod_settings_spare", "Cancel", "Okay");
    dlg.send(lucky::local_out());
}

void ModDetectCommand::handle_settings_dialog(const std::string& raw) {
    TextParse tp{raw};
    for (const auto& a : kModActions) {
        std::string v = tp.get(a.id);
        if (v.empty()) continue;
        lucky::cfg_set(a.config_key, v[0] == '1');
    }
    lucky::cfg_save();
    spdlog::info("[ModDetect] Settings saved");
    lucky::log("`2Mod Detect settings saved``.");
}

// LuckyProxy itsmod(): actions when a moderator joins the world
void ModDetectCommand::run_mod_actions() {
    if (mod_action_on("features.moddetect.collect")) {
        auto* core = lucky::get_core();
        auto* to_server = core && core->get_client() ? core->get_client()->get_player() : nullptr;
        const auto local = utils::PlayerTracker::get_instance().get_local_player();
        const auto pos = utils::PlayerTracker::get_instance().get_player_position(local.netID);
        constexpr float kRange = 10.0f * 32.0f;
        int collected = 0;
        for (const auto& item : utils::WorldManager::get_instance().get_all_dropped_items()) {
            const float dx = item.X - pos.x, dy = item.Y - pos.y;
            if (dx * dx + dy * dy > kRange * kRange) continue;
            AutoCollectCommand::send_collect_packet(to_server, item.Uid, item.X, item.Y);
            ++collected;
        }
        spdlog::info("[ModDetect] Auto collecting {} item(s) within 10 tiles", collected);
        if (collected > 0) lucky::send_overlay("`9Auto Collecting....");
    }
    if (mod_action_on("features.moddetect.ban_all")) {
        lucky::log("`9You have `4(Ban All)`9 When Mod joins option `2Enabled");
        spdlog::info("[ModDetect] Banning everyone in the world");
        lucky::log("`4Banning `9Everyone in the world...");
        const uint32_t local_netid = utils::PlayerTracker::get_instance().get_local_netid();
        for (const auto& [netid, info] : utils::PlayerTracker::get_instance().get_all_players()) {
            if (netid == local_netid || info.name.empty()) continue;
            lucky::send_server_input("/ban " + info.name);
        }
    }
    if (mod_action_on("features.moddetect.unaccess")) {
        spdlog::info("[ModDetect] Removing own access (/unaccess)");
        lucky::send_server_input("/unaccess");
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            lucky::send_server_text("action|dialog_return\ndialog_name|unaccess\nbuttonClicked|Yes");
            lucky::log("`2Done Unaccess.");
        }).detach();
    }

    const bool exit_world = mod_action_on("features.moddetect.exit_world");
    if (mod_action_on("features.moddetect.warp_save")) {
        const std::string save = SaveWorldCommand::get_save_world();
        if (!save.empty()) {
            spdlog::info("[ModDetect] Warping to save world {}", save);
            lucky::send_server_game_message("action|join_request\nname|" + save + "\ninvitedWorld|0");
            return;
        }
        spdlog::warn("[ModDetect] Warp to save world is on but no save world is set (/setsave)");
        lucky::log(exit_world ? "`4Save world is empty. Use /setsave, falling back to exit."
                              : "`4Save world is empty. Use /setsave.");
    }
    if (exit_world) {
        lucky::log("`9You have `4(Exit World)`9 When Mod joins option `2Enabled");
        spdlog::info("[ModDetect] Leaving the world");
        lucky::log("`bLeaving The World Now");
        lucky::send_server_game_message("action|quit_to_exit");
    }
}

void ModDetectCommand::handle_spawn_packet(const std::string& spawn_data) {
    if (!s_core) return;

    bool enabled = true;
    try {
        enabled = s_core->get_config().get<bool>("features.moddetect");
    } catch (...) {
        
        enabled = true;
        s_core->get_config().set<bool>("features.moddetect", true);
    }
    if (!enabled) return;

    TextParse text_parse{ spawn_data };
    const std::string spawn_type = text_parse.get("type");
    if (spawn_type == "local") return;

    std::string uid = text_parse.get("userID");
    if (uid.empty()) uid = text_parse.get("userid");
    if (uid.empty()) return;

    auto it = kModerators.find(uid);
    if (it == kModerators.end()) return;

    const auto now = std::chrono::steady_clock::now();
    auto last_it = g_last_alert.find(uid);
    if (last_it != g_last_alert.end()) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_it->second).count();
        if (elapsed < 15) return;
    }
    g_last_alert[uid] = now;

    std::string spawn_name = clean_name(text_parse.get("name"));
    const std::string& known_name = it->second;
    const std::string final_name = spawn_name.empty() ? known_name : spawn_name;

    spdlog::warn("[MODDETECT] Moderator spawn detected: {} (uid={})", final_name, uid);
    send_mod_notification(s_core, final_name, uid);
    run_mod_actions();
}

} 
