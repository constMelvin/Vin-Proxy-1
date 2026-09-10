#include "proxy_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/packet_utils.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>

namespace command {

static core::Core* g_core_proxy = nullptr;
static core::Core* g_core_info = nullptr;

void ProxyCommand::set_core(core::Core* core) {
    g_core_proxy = core;
}

void InfoCommand::set_core(core::Core* core) {
    g_core_info = core;
}

ProxyCommand::ProxyCommand() : CommandBase(
    {"proxy", "commands", "help"},
    {},
    "Open comprehensive commands directory with all available commands",
    0
) {}

std::unique_ptr<CommandBase> ProxyCommand::clone() const {
    return std::make_unique<ProxyCommand>();
}

struct CommandDoc {
    std::string category;     // "move", "econ", "cosmetic", "world", "mod", "auto"
    std::string cmd;          // e.g. "/pos1 - /pos4"
    std::string args;         // e.g. "[world]"
    std::string desc;         // description
    std::string color;        // "`2", "`e", "`b", "`1", "`4", "`3"
    std::string aliases;      // search keywords
};

struct CategoryInfo {
    std::string id;
    std::string title;
    std::string color;
    int icon;
};

static const std::vector<CategoryInfo>& get_categories() {
    static const std::vector<CategoryInfo> s_cats = {
        {"move",     "Position & Movement",           "`2", 242},
        {"econ",     "Economy, Drops & Vending",       "`e", 2978},
        {"cosmetic", "Clothes & Visual Customization", "`b", 1784},
        {"world",    "World, Scanner & Navigation",    "`1", 6878},
        {"mod",      "Admin, Moderation & Cheats",     "`4", 758},
        {"auto",     "Automation & Utilities",         "`3", 3410}
    };
    return s_cats;
}

static const std::vector<CommandDoc>& get_all_commands() {
    static const std::vector<CommandDoc> s_commands = {
        // --- 1. Position & Movement ---
        {"move", "/pos1 - /pos4", "", "Save checkpoint 1-4 with ring particle VFX (#88)", "`2", "pos1 pos2 pos3 pos4 pos checkpoint ring"},
        {"move", "/posback", "", "Save current position as back checkpoint", "`2", "posback checkpoint"},
        {"move", "/tp1 - /tp4", "", "Instant teleport to saved position 1-4", "`2", "tp1 tp2 tp3 tp4 teleport"},
        {"move", "/back", "", "Teleport back to saved pos (or previous world)", "`2", "back return teleport"},
        {"move", "/path", "[x] [y]", "Walk pathfinding to coordinates (or Shift+Click)", "`2", "path findpath walk astar"},
        {"move", "/findpath", "", "Open pathfinding settings dialog (click mode & delay)", "`2", "findpath settings click path"},
        {"move", "/stop", "", "Cancel current pathfinding walk immediately", "`2", "stop cancel walk"},
        {"move", "/playertp", "[name/netid]", "Teleport directly to player in world", "`2", "playertp ptp tp player"},
        {"move", "/setpos", "[x] [y]", "Send raw OnSetPos position update packet", "`2", "setpos rawpos teleport"},

        // --- 2. Economy, Drops & Vending ---
        {"econ", "/drop, /dropfast", "[amt]", "Fast drop items from hand without confirmation", "`e", "drop dropfast fastdrop"},
        {"econ", "/dropall", "", "Drop all items from inventory fast", "`e", "dropall alldrop"},
        {"econ", "/dropat", "[x] [y] [amt]", "Drop items directly at target tile coordinates", "`e", "dropat tile drop target"},
        {"econ", "/w1 - /w4", "[amt]", "Quick drop 1-4 World Locks (or /dw [amt])", "`e", "w1 w2 w3 w4 dw dropwl wl"},
        {"econ", "/d1 - /d4", "[amt]", "Quick drop 1-4 Diamond Locks (or /dd [amt])", "`e", "d1 d2 d3 d4 dd dropdl dl"},
        {"econ", "/b1 - /b4", "[amt]", "Quick drop 1-4 Blue Gem Locks (or /dbgl [amt])", "`e", "b1 b2 b3 b4 dbgl dropbgl bgl"},
        {"econ", "/balance, /bal", "", "Check total currency balance (WL / DL / BGL)", "`e", "balance bal currency locks"},
        {"econ", "/autocomp", "", "Toggle auto-compress (100 WL->DL, 100 DL->BGL)", "`e", "autocomp compress locks currency"},
        {"econ", "/autocollect", "", "Toggle auto-collect nearby dropped items", "`e", "autocollect collect magnet suck"},
        {"econ", "/bank", "", "Open World Lock bank deposit & withdraw dialog", "`e", "bank locks deposit withdraw"},
        {"econ", "/bankadd", "[amt]", "Deposit World Locks into bank", "`e", "bankadd deposit bank"},
        {"econ", "/bankwith", "[amt]", "Withdraw World Locks from bank", "`e", "bankwith withdraw bank"},
        {"econ", "/bankcheck", "", "Check bank account balance", "`e", "bankcheck balance bank"},
        {"econ", "/vendloc", "", "Scan & list all vending machines in world", "`e", "vendloc vends vending scan list"},
        {"econ", "/vendfind, /vendtp", "[item]", "Find & highlight vending machine with item", "`e", "vendfind vendtp find vend"},
        {"econ", "/vendfast", "", "Open fast vending setup configuration dialog", "`e", "vendfast quickvend setup vend"},
        {"econ", "/vendsafe", "", "Toggle safe vending auto-buyer helper (anti-scam)", "`e", "vendsafe safebuy buyer vend"},
        {"econ", "/vendlogs", "", "View transaction logs for vending machines", "`e", "vendlogs logs history vend"},
        {"econ", "/buy", "", "Open in-game item purchase dialog", "`e", "buy store purchase"},
        {"econ", "/fillgbc", "", "Auto-fill Golden Blast into wishing well", "`e", "fillgbc gbc well blast"},
        {"econ", "/dbox", "", "Toggle fast donation box drop mode", "`e", "dbox donation drop"},
        {"econ", "/setdb", "[amt]", "Set donation box drop amount", "`e", "setdb amount donation"},
        {"econ", "/trashfast", "", "Fast trash items without confirmation dialog", "`e", "trashfast trash delete recycle"},
        {"econ", "/mailclaim", "[id]", "Auto-claim mailbox message reward", "`e", "mailclaim mailbox mail claim"},
        {"econ", "/bid", "[amt]", "Place auction house bid on current auction", "`e", "bid auction wls"},

        // --- 3. Clothes, Visuals & Cosmetics ---
        {"cosmetic", "/clothes", "[ids...]", "Open visual clothes selector GUI or equip IDs", "`b", "clothes wear equip items skin"},
        {"cosmetic", "/clearclothes", "", "Remove all visual clothes items", "`b", "clearclothes unequip naked reset"},
        {"cosmetic", "/save1 - /save4", "", "Save current clothes loadout to slot 1-4", "`b", "save1 save2 save3 save4 set1 set2 set3 set4 slot"},
        {"cosmetic", "/load1 - /load4", "", "Equip clothes loadout from slot 1-4", "`b", "load1 load2 load3 load4 slot equip"},
        {"cosmetic", "/skin", "[hex]", "Set skin color (e.g. 0xFFFFFF, 0x000000)", "`b", "skin color hex rgb"},
        {"cosmetic", "/name", "[col] [name]", "Change visual display nickname and color", "`b", "name nick nickname color rename"},
        {"cosmetic", "/title", "", "Open title selection GUI (Legend, Doctor, etc.)", "`b", "title dr maxlevel g4g doctor legend"},
        {"cosmetic", "/cleartitle", "", "Reset and clear current visual title", "`b", "cleartitle reset title"},
        {"cosmetic", "/titleicon", "[id]", "Set visual title icon badge", "`b", "titleicon icon badge title"},
        {"cosmetic", "/flag", "[code]", "Change country flag (us, id, tr, lb, etc.)", "`b", "flag country nation icon"},
        {"cosmetic", "/roleskin", "[skin] [icon]", "Apply legendary role skin & icon", "`b", "roleskin role skin badge"},
        {"cosmetic", "/disguise", "[id]", "Disguise yourself as item (0 to reset)", "`b", "disguise morph item transform"},
        {"cosmetic", "/deathanim", "[id]", "Set custom death animation ID", "`b", "deathanim anim death animation"},
        {"cosmetic", "/respawnanim", "[id]", "Set custom respawn animation ID", "`b", "respawnanim anim respawn spawn"},
        {"cosmetic", "/itemfx", "[id]", "Trigger visual item effect on yourself", "`b", "itemfx effect visual vfx"},
        {"cosmetic", "/particle", "[id] [v2]", "Spawn particle visual effect at coordinates", "`b", "particle vfx fx visual"},
        {"cosmetic", "/hitvfx", "[id]", "Set custom punch hit visual effect", "`b", "hitvfx punch hit strike"},
        {"cosmetic", "/banner", "[id]", "Set custom guild banner / bandolier effect", "`b", "banner bandolier guild flag"},
        {"cosmetic", "/paintball", "[col]", "Set paintball splatter color hex", "`b", "paintball paint splat color"},
        {"cosmetic", "/levelup", "[lvl]", "Trigger visual level-up animation & fireworks", "`b", "levelup level fireworks celebration"},
        {"cosmetic", "/rainbow", "", "Toggle cycling rainbow clothes and skin colors", "`b", "rainbow cycle rgb colors"},
        {"cosmetic", "/dragon", "", "Toggle daylight dragon visual aura", "`b", "dragon aura daylight effect"},
        {"cosmetic", "/riftwings", "", "Toggle rift wings & cape visual aura", "`b", "riftwings rift cape wings aura"},
        {"cosmetic", "/infinity", "[type]", "Toggle infinity aura effect", "`b", "infinity aura effect"},
        {"cosmetic", "/ghost", "", "Toggle ghost character visual mode", "`b", "ghost transparent invis char"},

        // --- 4. World, Scanner & Navigation ---
        {"world", "/warp", "[world]", "Warp directly to specified world", "`1", "warp world goto join"},
        {"world", "/back", "", "Warp back to previous visited world", "`1", "back previous world warp"},
        {"world", "/exit", "", "Exit world to main menu", "`1", "exit leave unjoin"},
        {"world", "/growscan, /gs", "", "Scan floating items, blocks & trees in world", "`1", "growscan gs scan floating items world"},
        {"world", "/gsbeta", "", "Beta world scanner with item totals", "`1", "gsbeta scan beta totals"},
        {"world", "/find, /search", "[item]", "Search for specific item located in world", "`1", "find search locate item world"},
        {"world", "/lockefind", "", "Scan world for Locke the Traveling Salesman", "`1", "lockefind locke salesman merchant"},
        {"world", "/dat", "", "Inspect tile extra data (seeds, signs, locks)", "`1", "dat tile trees seeds sign data inspect"},
        {"world", "/chest", "", "Inspect items placed inside chests in world", "`1", "chest box container inspect"},
        {"world", "/inventory", "", "View complete inventory items list", "`1", "inventory inv items list"},
        {"world", "/players", "", "List all players currently in world with NetIDs", "`1", "players list netid player who"},
        {"world", "/door", "[door_id]", "Enter specific door by ID", "`1", "door enter id portal"},
        {"world", "/doorid", "", "Scan and display all door IDs in world", "`1", "doorid scan door ids"},
        {"world", "/doorbf", "", "Bruteforce locked door IDs", "`1", "doorbf brute bf password door"},
        {"world", "/remo, /unaccess", "", "Fast unaccess from world locks", "`1", "remo unaccess remove lock wl hl"},
        {"world", "/eventmenu", "", "Open clash event menu (even if not live)", "`1", "eventmenu clash event tournament"},
        {"world", "/weather", "[id]", "Change client-side weather machine effect", "`1", "weather setweather atmosphere sky"},
        {"world", "/initworld", "", "Send OnInitNewWorld refresh packet", "`1", "initworld refresh reload world"},

        // --- 5. Admin, Moderation & Cheats ---
        {"mod", "/admin", "", "Check online admin/mod UIDs and status", "`4", "admin mod check staff uid"},
        {"mod", "/moddetect", "", "Toggle mod spawn detection & screen alert", "`4", "moddetect mod alert sound detect warn"},
        {"mod", "/run", "", "Emergency escape: hops multiple worlds to evade mods", "`4", "run escape flee evade emergency"},
        {"mod", "/immune", "", "Toggle collision & damage immunity", "`4", "immune god godmode collision spike lava"},
        {"mod", "/antigravity", "", "Toggle zero-gravity floating mode", "`4", "antigravity gravity float fly"},
        {"mod", "/antipunch", "", "Toggle punch jammer protection effect", "`4", "antipunch punch block shield jammer"},
        {"mod", "/vision", "", "Toggle night vision (see clearly in dark worlds)", "`4", "vision nightvision see dark light"},
        {"mod", "/invis", "", "Toggle player invisibility mode", "`4", "invis invisible vanish hide"},
        {"mod", "/sm", "", "Super Supporter mode (rejoin for long punch)", "`4", "sm supersupporter longpunch mod"},
        {"mod", "/mstate", "[1/0]", "Toggle mod state long punch immediately", "`4", "mstate longpunch modstate punch"},
        {"mod", "/wrench", "", "Toggle auto-wrench mode (pull/kick/ban on tap)", "`4", "wrench autowrench pull kick ban fastwrench"},
        {"mod", "/join", "", "Open join mode GUI (auto-kick/pull/ban on enter)", "`4", "join autokick autopull autoban"},
        {"mod", "/banall", "", "Mass ban all non-access players in world", "`4", "banall massban ban all"},
        {"mod", "/pullall", "", "Mass pull all players in world to your position", "`4", "pullall masspull pull all"},
        {"mod", "/banfire", "", "Auto-ban player with special fire animation", "`4", "banfire ban fire effect"},
        {"mod", "/freeze", "[netid]", "Freeze player client (fake disconnect)", "`4", "freeze fake dc lag drop netid"},
        {"mod", "/warn", "[netid]", "Send warning popup dialog to player", "`4", "warn warning popup alert player"},
        {"mod", "/mentor", "[netid]", "Set player mentor state badge", "`4", "mentor role badge helper"},
        {"mod", "/fakemaint", "[msg]", "Display fake system maintenance popup", "`4", "fakemaint maintenance restart fake"},
        {"mod", "/host", "", "Open CSN casino hoster calculator & dialog", "`4", "host csn casino calculator roulette hoster"},
        {"mod", "/ignorecsn", "", "Block CSN casino broadcast messages", "`4", "ignorecsn csn block ignore broadcast"},
        {"mod", "/ignorecsnchat", "", "Block CSN casino chat bubbles", "`4", "ignorecsnchat csn chat ignore mute"},

        // --- 6. Automation & Utility ---
        {"auto", "/spam", "", "Toggle automated chat spammer ON/OFF", "`3", "spam autospam chat loop"},
        {"auto", "/spamtext", "[text]", "Set message text for automated chat spammer", "`3", "spamtext text message spam"},
        {"auto", "/spamdelay", "[sec]", "Set delay for automated chat spammer (min 2s)", "`3", "spamdelay delay interval spam"},
        {"auto", "/autosurg", "", "Toggle automated surgery bot", "`3", "autosurg surgery bot hospital surg"},
        {"auto", "/autocrime", "", "Toggle automated superhero crime solver", "`3", "autocrime crime bot superhero villain"},
        {"auto", "/gui", "", "Toggle desktop ImGui overlay control window", "`3", "gui imgui menu window overlay"},
        {"auto", "/ping", "", "Display real-time ping to Growtopia server", "`3", "ping latency ms server"},
        {"auto", "/devicecheck", "", "Inspect spoofed MAC, RID, and hardware details", "`3", "devicecheck mac rid hardware spoof device"},
        {"auto", "/broadcast", "[msg]", "Send visual broadcast announcement banner", "`3", "broadcast announcement banner fake"},
        {"auto", "/bubble", "[text]", "Spawn custom chat bubble over head", "`3", "bubble talk chat text speech"},
        {"auto", "/overlay", "[text]", "Display screen center text announcement", "`3", "overlay screen text display"},
        {"auto", "/zoom", "[val]", "Adjust camera zoom level", "`3", "zoom camera view distance"},
        {"auto", "/gems", "[amt]", "Set visual gem count (server-validated)", "`3", "gems bux fake visual count"},
        {"auto", "/bux", "[amt]", "Set visual Growtokens count (server-validated)", "`3", "bux tokens visual count"},
        {"auto", "/debuganim", "", "Animation packet debugger and copier", "`3", "debuganim animation debug packet copy"},
        {"auto", "/rawvar", "[args]", "Send custom raw VarList packet", "`3", "rawvar varlist packet debug"},
        {"auto", "/proxy, /help", "", "Open this comprehensive commands guide", "`3", "proxy commands help info guide menu"}
    };
    return s_commands;
}

static std::string to_lower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return res;
}

static std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

void ProxyCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) {
        spdlog::error("ProxyCommand: client or player is null!");
        return;
    }

    if (!g_core_proxy) {
        spdlog::error("ProxyCommand: No core set!");
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Core not initialized");
        return;
    }

    ProxyCommand::show_commands_gui(client->get_player(), g_core_proxy);
}

void ProxyCommand::show_commands_gui(player::Player* player, core::Core* core, const std::string& /*filter*/, const std::string& /*category*/) {
    if (!player) {
        spdlog::error("ProxyCommand: player is null!");
        return;
    }

    auto* server = core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("ProxyCommand: No server available!");
        return;
    }

    try {
        const auto& all_cmds = get_all_commands();
        const auto& all_cats = get_categories();

        std::ostringstream dialog;
        dialog << "set_default_color|`o\n";
        dialog << "add_label_with_icon|big|`wVinProxy Commands and Features``|left|5956|\n";
        dialog << "add_spacer|small|\n";
        dialog << "add_smalltext|`9Complete Command Directory • Total: `w" << all_cmds.size() << " Commands``|\n";
        dialog << "add_spacer|small|\n";

        // Display all categories and their commands compactly
        for (size_t i = 0; i < all_cats.size(); ++i) {
            const auto& cat = all_cats[i];
            if (i > 0) {
                dialog << "add_spacer|small|\n";
            }
            dialog << "add_label_with_icon|small|" << cat.color << cat.title << "``|left|" << cat.icon << "|\n";
            dialog << "add_spacer|small|\n";

            for (const auto& c : all_cmds) {
                if (c.category != cat.id) continue;

                std::string arg_str = c.args.empty() ? "" : (" `9" + c.args);
                std::string line = fmt::format("{}{}{} `o- {}``", c.color, c.cmd, arg_str, c.desc);
                dialog << "add_smalltext|" << line << "|\n";
            }
        }

        // Footer
        dialog << "add_spacer|small|\n";
        dialog << "add_smalltext|`9Auto-save enabled for positions, clothes slots & features!``|\n";
        dialog << "add_quick_exit|\n";
        dialog << "end_dialog|proxy_commands_gui|Close||";

        std::string dialog_data = dialog.str();

        packet::Variant variant{};
        variant.add("OnDialogRequest");
        variant.add(dialog_data);

        std::vector<std::byte> ext_data = variant.serialize();

        packet::GameUpdatePacket game_packet{};
        game_packet.type = packet::PACKET_CALL_FUNCTION;
        game_packet.net_id = -1;
        game_packet.flags.extended = 1;
        game_packet.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> byte_stream{};
        byte_stream.write(packet::NET_MESSAGE_GAME_PACKET);
        byte_stream.write(game_packet);
        byte_stream.write_data(ext_data.data(), ext_data.size());

        server->get_player()->send_packet(byte_stream.get_data(), 0);

        spdlog::info("ProxyCommand: GUI sent successfully");

    } catch (const std::exception& e) {
        spdlog::error("ProxyCommand: Failed to send GUI: {}", e.what());
    }
}

void ProxyCommand::handle_dialog_return(player::Player* /*player*/, const std::string& /*button_clicked*/, const std::string& /*search_query*/) {
    // No action needed since dialog shows all commands directly
}

InfoCommand::InfoCommand() : CommandBase(
    {"info", "information"},
    {},
    "Open comprehensive commands directory with all available commands",
    0
) {}

std::unique_ptr<CommandBase> InfoCommand::clone() const {
    return std::make_unique<InfoCommand>();
}

void InfoCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) {
        spdlog::error("InfoCommand: client or player is null!");
        return;
    }

    if (!g_core_info) {
        spdlog::error("InfoCommand: No core set!");
        utils::PacketUtils::send_chat_message(client->get_player(), "`4Error: Core not initialized");
        return;
    }

    std::string filter = args.empty() ? "" : args[0];
    ProxyCommand::show_commands_gui(client->get_player(), g_core_info, filter, "all");
}

} 
