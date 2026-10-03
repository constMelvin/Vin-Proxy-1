#include "onspawn_commands.hpp"
#include "position_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/player_tracker.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/dialog.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

namespace command {

namespace {

constexpr size_t kAutoPullTileMin = 3;
constexpr size_t kAutoPullTileMax = 15;

struct TilePos {
    int x = 0;
    int y = 0;
    bool operator==(const TilePos& o) const { return x == o.x && y == o.y; }
    bool operator!=(const TilePos& o) const { return !(*this == o); }
};

struct SpawnedPlayer {
    std::string name;      // colour codes stripped
    bool has_access = false;
};

core::Core* g_core = nullptr;
std::mutex g_mutex;

// /autopull, /pullauto
bool g_autopull = false;
bool g_autopull_include_admin = true;
bool g_autopull_auto_disable = false;

// /autopulltile, /apt, /sapt, /pulltile
std::vector<TilePos> g_tile_positions;
std::unordered_map<uint32_t, TilePos> g_tile_last;
bool g_tile_enabled = false;
bool g_tile_selecting = false;
bool g_tile_include_admin = true;
bool g_tile_auto_disable = false;

// /auto, /aban
bool g_enter_pull = false;
bool g_enter_ban = false;
bool g_pull_by_name = false;
bool g_ban_by_name = false;
std::vector<std::string> g_pull_names;   // lowercase
std::vector<std::string> g_ban_names;    // lowercase

std::unordered_map<uint32_t, SpawnedPlayer> g_players;

player::Player* local_out() {
    if (!g_core) return nullptr;
    auto* srv = g_core->get_server();
    return srv ? srv->get_player() : nullptr;
}

// Game console message, mirrored to the terminal without colour codes
void log(const std::string& msg) {
    std::string plain;
    for (size_t i = 0; i < msg.size(); ++i) {
        if (msg[i] == '`' && i + 1 < msg.size()) { ++i; continue; }
        plain.push_back(msg[i]);
    }
    spdlog::info("[OnSpawn] {}", plain);
    if (auto* out = local_out())
        utils::PacketUtils::send_chat_message(out, msg);
}

// Sends chat commands to the game server, 50ms apart (LuckyProxy sleeps 50ms between them)
void send_server_inputs(std::vector<std::string> texts) {
    if (texts.empty() || !g_core) return;
    core::Core* core = g_core;
    std::thread([core, texts = std::move(texts)]() {
        for (const auto& text : texts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto* client = core->get_client();
            if (!client || !client->get_player()) return;
            ByteStream<std::uint16_t> bs{};
            bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
            bs.write("action|input\ntext|" + text, false);
            bs.write(std::uint8_t{0});   // null-terminated, or the server drops the last letter
            (void)client->get_player()->send_packet(bs.get_data(), 0);
        }
    }).detach();
}

std::string strip_name(std::string name) {
    name.erase(std::remove(name.begin(), name.end(), '\''), name.end());
    name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
    size_t pos = 0;
    while ((pos = name.find('`')) != std::string::npos)
        name.erase(pos, pos + 1 < name.size() ? 2 : 1);
    name.erase(0, name.find_first_not_of(" \t\r\n"));
    name.erase(name.find_last_not_of(" \t\r\n") + 1);
    return name;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// LuckyProxy world.h hasAccessName: owner (`2) or world access (`^) name colour
bool has_access_name(const std::string& raw_name) {
    return raw_name.find("``") != std::string::npos &&
           (raw_name.find("`^") != std::string::npos || raw_name.find("`2") != std::string::npos);
}

bool has_tile(const TilePos& tile) {
    return std::find(g_tile_positions.begin(), g_tile_positions.end(), tile) != g_tile_positions.end();
}

bool contains(const std::vector<std::string>& list, const std::string& lower_name) {
    return std::find(list.begin(), list.end(), lower_name) != list.end();
}

std::string join_names(const std::vector<std::string>& list) {
    std::string out;
    for (const auto& n : list) {
        if (!out.empty()) out += ", ";
        out += n;
    }
    return out;
}

bool checkbox_on(const TextParse& tp, const char* key) {
    std::string v = tp.get(key);
    v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char c) { return std::isspace(c); }), v.end());
    return !v.empty() && v != "0";
}

std::string trimmed(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

void show_autopull_dialog() {
    std::string status;
    std::string auto_disable_status;
    std::string players_only_btn, players_admin_btn, disable_btn, auto_disable_btn;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_autopull) status = "`4Status:`w Auto Pull is disabled.";
        else if (g_autopull_include_admin) status = "`2Status:`w Pulling players and admins.";
        else status = "`2Status:`w Pulling players only.";

        auto_disable_status = g_autopull_auto_disable ? "`2Auto-disable:`w Enabled (turns off after one pull)."
                                                      : "`4Auto-disable:`w Disabled (stays on).";
        players_only_btn = g_autopull && !g_autopull_include_admin ? "`2Players Only (Selected)" : "`9Players Only";
        players_admin_btn = g_autopull && g_autopull_include_admin ? "`2Players & Admins (Selected)" : "`9Players & Admins";
        disable_btn = !g_autopull ? "`2Auto Pull Already Disabled" : "`4Disable Auto Pull";
        auto_disable_btn = g_autopull_auto_disable ? "`2Auto Disable (Selected)" : "`9Auto Disable After Pull";
    }

    utils::Dialog dlg;
    dlg.label_with_icon("`2Auto Pull Settings``", 2246)
       .smalltext(status)
       .smalltext(auto_disable_status)
       .spacer()
       .button("autopull_players", players_only_btn)
       .button("autopull_all", players_admin_btn)
       .button("autopull_disable", disable_btn)
       .button("autopull_autodisable", auto_disable_btn)
       .end_dialog("autopull_settings", "", "Close");
    dlg.send(local_out());
}

void show_autopulltile_dialog() {
    utils::Dialog dlg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        dlg.label_with_icon("`2Auto Pull Tile Settings``", 2246)
           .smalltext(std::string("`9Status:`w ") + (g_tile_enabled ? "`2Enabled" : "`4Disabled"))
           .smalltext("`9Tiles:`w " + std::to_string(g_tile_positions.size()) + " `8(min 3, max 15)")
           .smalltext(g_tile_selecting ? "`2Selection:`w ON" : "`4Selection:`w OFF")
           .smalltext(g_tile_include_admin ? "`2Pulling players & admins." : "`2Pulling players only.")
           .smalltext(g_tile_auto_disable ? "`2Auto-disable:`w Enabled (turns off after one pull)."
                                          : "`4Auto-disable:`w Disabled (stays on).")
           .smalltext("`9Use `0/apt`9 to select tiles, `0/sapt`9 to preview.")
           .spacer()
           .button("autopulltile_toggle", g_tile_enabled ? "`4Disable Auto Pull Tile" : "`2Enable Auto Pull Tile")
           .button("autopulltile_admin", g_tile_include_admin ? "`9Don't Pull Admin/Owner" : "`2Don't Pull Admin/Owner (Selected)")
           .button("autopulltile_autodisable", g_tile_auto_disable ? "`2Auto Disable After Pull (Selected)" : "`9Auto Disable After Pull")
           .button("autopulltile_reset", "`4Reset Tiles")
           .end_dialog("autopulltile_settings", "", "Close");
    }
    dlg.send(local_out());
}

void show_auto_dialog() {
    bool pull, ban;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        pull = g_enter_pull;
        ban = g_enter_ban;
    }
    utils::Dialog dlg;
    dlg.label_with_icon("Auto options", 2250)
       .spacer()
       .button("autobani", "`4Auto ban `9(Specific people)")
       .button("autopuli", "`#Auto pull `9(Specific people)")
       .textbox("`9This below will pull/ban `2everyone `9who joins the world")
       .checkbox("pullnam_4", "`#Enable Auto Pull", pull)
       .checkbox("pullnam_3", "`4Enable Auto Ban", ban)
       .quick_exit()
       .end_dialog("auto_dialog", "Cancel", "Okay");
    dlg.send(local_out());
}

void show_by_name_dialog(bool is_ban) {
    bool enabled;
    std::string names;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        enabled = is_ban ? g_ban_by_name : g_pull_by_name;
        names = join_names(is_ban ? g_ban_names : g_pull_names);
    }
    const std::string word = is_ban ? "autoban" : "autopull";
    const std::string prefix = is_ban ? "pullban" : "pullnam";
    utils::Dialog dlg;
    if (is_ban) dlg.label_with_icon("`2AutoBan Page", 2242);
    else dlg.label_with_icon("`2AutoPull Page", 2246);
    dlg.textbox("`9Name has to be fully written, for " + word + " to work.")
       .textbox("`9Name has be to atleast 3 letters long & without spaces")
       .textbox("`cNames: " + (names.empty() ? std::string("`4Empty``") : names))
       .checkbox(prefix + "_1", is_ban ? "`4Enable Auto ban" : "`2Enable Auto Pull", enabled)
       .text_input(prefix + "_2", "Name", "", 20)
       .button(prefix + "_clear", "`4Clear Names")
       .end_dialog(is_ban ? "banby_name" : "pullby_name", "Cancel", "Okay");
    dlg.send(local_out());
}

void handle_by_name_dialog(bool is_ban, const std::string& raw) {
    TextParse tp{raw};
    const char* prefix = is_ban ? "pullban" : "pullnam";
    const std::string button = trimmed(tp.get("buttonClicked"));
    const bool enabled = checkbox_on(tp, (std::string(prefix) + "_1").c_str());
    const std::string name = trimmed(tp.get(std::string(prefix) + "_2"));

    std::string msg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        (is_ban ? g_ban_by_name : g_pull_by_name) = enabled;
        auto& list = is_ban ? g_ban_names : g_pull_names;

        if (button == std::string(prefix) + "_clear") {
            list.clear();
            msg = is_ban ? "`9Autoban List cleared." : "`9AutoPull List cleared.";
        } else if (!name.empty()) {
            if (name.size() < 3 || name.find(' ') != std::string::npos) {
                msg = "`4Name has to be at least 3 letters long & without spaces.";
            } else {
                std::string lower = to_lower(name);
                if (!contains(list, lower)) list.push_back(lower);
                msg = is_ban ? "`9Successfully added to Autoban List" : "`9Successfully added to AutoPull List";
            }
        } else {
            msg = fmt::format("`9Auto {} by name is now {}", is_ban ? "Ban" : "Pull", enabled ? "`2enabled" : "`4disabled");
        }
    }
    log(msg);
}

} // namespace

// ============================================================
// OnSpawnManager
// ============================================================
void OnSpawnManager::set_core(core::Core* core) { g_core = core; }

bool OnSpawnManager::is_autopull_enabled() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_autopull;
}

void OnSpawnManager::on_player_spawn(uint32_t net_id, const std::string& raw_name) {
    const std::string name = strip_name(raw_name);
    if (name.empty()) return;
    const bool access = has_access_name(raw_name);
    const std::string lower = to_lower(name);

    std::vector<std::string> inputs;
    std::vector<std::string> logs;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_players[net_id] = SpawnedPlayer{name, access};

        bool pull = false;
        if (g_enter_pull) pull = true;
        if (g_pull_by_name && contains(g_pull_names, lower)) pull = true;
        if (g_autopull && (g_autopull_include_admin || !access)) {
            pull = true;
            if (g_autopull_auto_disable) {
                g_autopull = false;
                logs.push_back("`2Auto Pull triggered for `w" + name + "``. Auto Pull disabled.");
            } else {
                logs.push_back("`2Auto Pull triggered for `w" + name + "``.");
            }
        }
        const bool ban = g_enter_ban || (g_ban_by_name && contains(g_ban_names, lower));

        if (pull) inputs.push_back("/pull " + name);
        if (ban) inputs.push_back("/ban " + name);
    }
    for (const auto& m : logs) log(m);
    if (!inputs.empty()) {
        std::string sent;
        for (const auto& in : inputs) sent += (sent.empty() ? "" : ", ") + in;
        spdlog::info("[OnSpawn] {} joined the world -> {}", name, sent);
        send_server_inputs(std::move(inputs));
    }
}

void OnSpawnManager::on_player_moved(uint32_t net_id, float x, float y) {
    std::string pull_name;
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_tile_enabled || g_tile_positions.size() < kAutoPullTileMin) return;
        if (net_id == utils::PlayerTracker::get_instance().get_local_netid()) return;

        TilePos tile{static_cast<int>(x) / 32, static_cast<int>(y) / 32};
        auto last_it = g_tile_last.find(net_id);
        if (last_it != g_tile_last.end() && last_it->second == tile) return;
        g_tile_last[net_id] = tile;
        if (!has_tile(tile)) return;

        auto pit = g_players.find(net_id);
        if (pit == g_players.end()) return;
        if (!g_tile_include_admin && pit->second.has_access) return;

        pull_name = pit->second.name;
        if (g_tile_auto_disable) {
            g_tile_enabled = false;
            g_tile_last.clear();
            msg = "`2Auto Pull Tile triggered for `6" + pull_name + "`2. Auto Pull Tile disabled.";
        } else {
            msg = "`2Auto Pull Tile triggered for `6" + pull_name + "`2.";
        }
    }
    send_server_inputs({"/pull " + pull_name});
    log(msg);
}

void OnSpawnManager::on_tile_punched(int tile_x, int tile_y) {
    std::string msg;
    bool highlight = false;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_tile_selecting) return;
        TilePos tile{tile_x, tile_y};
        if (g_tile_positions.size() >= kAutoPullTileMax) {
            msg = "`4Auto Pull Tile limit reached (15). Use /autopulltile to reset.";
        } else if (has_tile(tile)) {
            msg = fmt::format("`6Auto Pull Tile already selected: `2{}, {}", tile_x, tile_y);
        } else {
            g_tile_positions.push_back(tile);
            highlight = true;
            msg = fmt::format("`2Auto Pull Tile added: `6{}, {} `2({}/15)", tile_x, tile_y, g_tile_positions.size());
        }
    }
    if (highlight) {
        if (auto* out = local_out())
            PositionCommand::highlight_tile(out, tile_x, tile_y);
    }
    log(msg);
}

void OnSpawnManager::on_world_exit() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_tile_last.clear();
    g_players.clear();
}

void OnSpawnManager::handle_autopull_dialog(const std::string& button_clicked) {
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (button_clicked == "autopull_players") {
            g_autopull = true;
            g_autopull_include_admin = false;
            msg = "`aAuto Pull is now enabled for players only.";
        } else if (button_clicked == "autopull_all") {
            g_autopull = true;
            g_autopull_include_admin = true;
            msg = "`aAuto Pull is now enabled for players and admins.";
        } else if (button_clicked == "autopull_disable") {
            g_autopull = false;
            msg = "`aAuto Pull is now `4disabled.";
        } else if (button_clicked == "autopull_autodisable") {
            g_autopull_auto_disable = !g_autopull_auto_disable;
            msg = g_autopull_auto_disable ? "`aAuto Pull will now `2turn off`w after a successful pull."
                                          : "`aAuto Pull will now `2stay enabled`w after each pull.";
        }
    }
    if (!msg.empty()) log(msg);
}

void OnSpawnManager::handle_autopulltile_dialog(const std::string& button_clicked) {
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (button_clicked == "autopulltile_toggle") {
            if (!g_tile_enabled) {
                if (g_tile_positions.size() < kAutoPullTileMin) {
                    msg = "`4Auto Pull Tile needs at least 3 tiles. Use /apt to select.";
                } else {
                    g_tile_enabled = true;
                    g_tile_last.clear();
                    msg = "`2Auto Pull Tile enabled.";
                }
            } else {
                g_tile_enabled = false;
                msg = "`4Auto Pull Tile disabled.";
            }
        } else if (button_clicked == "autopulltile_admin") {
            g_tile_include_admin = !g_tile_include_admin;
            msg = g_tile_include_admin ? "`6Auto Pull Tile will now pull admins/owners."
                                       : "`6Auto Pull Tile will now ignore admins/owners.";
        } else if (button_clicked == "autopulltile_autodisable") {
            g_tile_auto_disable = !g_tile_auto_disable;
            msg = g_tile_auto_disable ? "`6Auto Pull Tile will now `2turn off`6 after one pull."
                                      : "`6Auto Pull Tile will now `2stay enabled`6 after each pull.";
        } else if (button_clicked == "autopulltile_reset") {
            g_tile_enabled = false;
            g_tile_selecting = false;
            g_tile_positions.clear();
            g_tile_last.clear();
            msg = "`4Auto Pull Tile list has been reset.";
        }
    }
    if (!msg.empty()) log(msg);
}

void OnSpawnManager::handle_auto_dialog(const std::string& button_clicked, const std::string& raw) {
    TextParse tp{raw};
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!tp.get("pullnam_3").empty() && g_enter_ban != checkbox_on(tp, "pullnam_3")) {
            g_enter_ban = checkbox_on(tp, "pullnam_3");
            spdlog::info("[OnSpawn] Ban everyone who joins: {}", g_enter_ban ? "on" : "off");
        }
        if (!tp.get("pullnam_4").empty() && g_enter_pull != checkbox_on(tp, "pullnam_4")) {
            g_enter_pull = checkbox_on(tp, "pullnam_4");
            spdlog::info("[OnSpawn] Pull everyone who joins: {}", g_enter_pull ? "on" : "off");
        }
    }
    if (button_clicked == "autopuli") {
        show_by_name_dialog(false);
    } else if (button_clicked == "autobani") {
        show_by_name_dialog(true);
    }
}

void OnSpawnManager::handle_pullby_name_dialog(const std::string& raw) { handle_by_name_dialog(false, raw); }
void OnSpawnManager::handle_banby_name_dialog(const std::string& raw) { handle_by_name_dialog(true, raw); }

// ============================================================
// /autopull - Auto pull menu for new visitors
// ============================================================
AutoPullCommand::AutoPullCommand() : CommandBase({"autopull"}, {}, "Auto pull menu for new visitors", 0) {}
std::unique_ptr<CommandBase> AutoPullCommand::clone() const { return std::make_unique<AutoPullCommand>(*this); }
void AutoPullCommand::execute(client::Client*, const std::vector<std::string>&) { show_autopull_dialog(); }

// ============================================================
// /pullauto - Toggle auto pull
// ============================================================
PullAutoCommand::PullAutoCommand() : CommandBase({"pullauto"}, {}, "Toggle auto pull", 0) {}
std::unique_ptr<CommandBase> PullAutoCommand::clone() const { return std::make_unique<PullAutoCommand>(*this); }
void PullAutoCommand::execute(client::Client*, const std::vector<std::string>&) {
    bool enabled;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_autopull = !g_autopull;
        enabled = g_autopull;
    }
    log(enabled ? "`aPull Auto is now enabled." : "`aPull Auto is now `4disabled.");
}

// ============================================================
// /autopulltile - Auto pull by selected tiles
// ============================================================
AutoPullTileCommand::AutoPullTileCommand() : CommandBase({"autopulltile"}, {}, "Auto pull by selected tiles", 0) {}
std::unique_ptr<CommandBase> AutoPullTileCommand::clone() const { return std::make_unique<AutoPullTileCommand>(*this); }
void AutoPullTileCommand::execute(client::Client*, const std::vector<std::string>&) { show_autopulltile_dialog(); }

// ============================================================
// /apt - Select auto pull tiles (punch to add, /apt again to save)
// ============================================================
AptCommand::AptCommand() : CommandBase({"apt"}, {}, "Select auto pull tiles", 0) {}
std::unique_ptr<CommandBase> AptCommand::clone() const { return std::make_unique<AptCommand>(*this); }
void AptCommand::execute(client::Client*, const std::vector<std::string>&) {
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_tile_selecting) {
            if (g_tile_positions.size() >= kAutoPullTileMax) {
                msg = "`4Auto Pull Tile list is full (15). Use /autopulltile to reset.";
            } else {
                g_tile_selecting = true;
                msg = "`6Punch tiles to add Auto Pull Tile (`23-15`6). Use /apt again to save.";
            }
        } else {
            g_tile_selecting = false;
            size_t count = g_tile_positions.size();
            if (count < kAutoPullTileMin) {
                // LuckyProxy leaves selection off here; /apt resumes it
                msg = fmt::format("`4Select at least 3 tiles. Current: `6{}`4. Use /apt to continue.", count);
            } else {
                msg = fmt::format("`2Saved your autopull tile {}.", count);
            }
        }
    }
    log(msg);
}

// ============================================================
// /sapt - Show auto pull tile highlights
// ============================================================
SaptCommand::SaptCommand() : CommandBase({"sapt"}, {}, "Show auto pull tile highlights", 0) {}
std::unique_ptr<CommandBase> SaptCommand::clone() const { return std::make_unique<SaptCommand>(*this); }
void SaptCommand::execute(client::Client*, const std::vector<std::string>&) {
    std::vector<TilePos> tiles;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        tiles = g_tile_positions;
    }
    if (tiles.empty()) {
        log("`4No Auto Pull Tile set. Use /apt to select.");
        return;
    }
    std::thread([tiles]() {
        for (const auto& tile : tiles) {
            if (auto* out = local_out())
                PositionCommand::highlight_tile(out, tile.x, tile.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }).detach();
    log("`6Showing Auto Pull Tile highlights.");
}

// ============================================================
// /pulltile, /pt - Enable auto pull tiles
// ============================================================
PullTileCommand::PullTileCommand() : CommandBase({"pulltile", "pt"}, {}, "Enable auto pull tiles", 0) {}
std::unique_ptr<CommandBase> PullTileCommand::clone() const { return std::make_unique<PullTileCommand>(*this); }
void PullTileCommand::execute(client::Client*, const std::vector<std::string>&) {
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_tile_enabled) {
            g_tile_enabled = false;
            msg = "`4Auto Pull Tile disabled.";
        } else if (g_tile_positions.size() < kAutoPullTileMin) {
            msg = "`4Auto Pull Tile needs at least 3 tiles. Use /apt to select.";
        } else {
            g_tile_enabled = true;
            g_tile_last.clear();
            msg = "`2Auto Pull Tile enabled.";
        }
    }
    log(msg);
}

// ============================================================
// /aban, /autoban - Auto ban player when joining world
// ============================================================
AutoBanCommand::AutoBanCommand() : CommandBase({"aban", "autoban"}, {}, "Auto ban player when joining world", 0) {}
std::unique_ptr<CommandBase> AutoBanCommand::clone() const { return std::make_unique<AutoBanCommand>(*this); }
void AutoBanCommand::execute(client::Client*, const std::vector<std::string>&) {
    bool enabled;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_enter_ban = !g_enter_ban;
        enabled = g_enter_ban;
    }
    log(enabled ? "`aAuto Ban is now enabled. `9Everyone who joins the world will be banned."
                : "`aAuto Ban is now `4disabled.");
}

// ============================================================
// /auto - Auto pull/ban options
// ============================================================
AutoOptionsCommand::AutoOptionsCommand() : CommandBase({"auto"}, {}, "Auto pull/ban options", 0) {}
std::unique_ptr<CommandBase> AutoOptionsCommand::clone() const { return std::make_unique<AutoOptionsCommand>(*this); }
void AutoOptionsCommand::execute(client::Client*, const std::vector<std::string>&) { show_auto_dialog(); }

}
