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
#include <thread>
#include <chrono>
#include <atomic>
#include <filesystem>
#include <fstream>
#include "vin_tabs_data.hpp"
#include "logs_command.hpp"
#include "options_command.hpp"

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
    {"proxy","news"},
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
    std::string args = "";    // e.g. "[world]"
    std::string desc = "";    // description
    std::string color = "`2"; // default cmd color: "`2"
    std::string aliases = ""; // search keywords
};

struct CategoryInfo {
    std::string id;
    std::string title;
    std::string color;
    int icon;
};

static const std::vector<CategoryInfo>& get_categories() {
    static const std::vector<CategoryInfo> s_cats = {
        {"info",   "Info:",             "`e", 2920},
        {"customize",     "Customize:",           "`e", 12436},
        {"mf",     "Main Features:",       "`e", 14544},
        {"casino",    "Auto Hoster:",    "`e", 758},
        {"econ",   "Economy & Drops:",   "`e", 242},
        {"visual", "Clothes & Visual Customization:", "`e", 1784},
        {"information",      "Information:",     "`e", 3524},
        {"sc",     "Shortcuts:",         "`e", 1794},
        {"of", "Other Features:", "`e", 7612}
    };
    return s_cats;
}

static const std::vector<CommandDoc>& get_all_commands() {
    static const std::vector<CommandDoc> s_commands = {
        // --- 0. Info ---
        {"info", "/proxy", "", "(Shows Commands & Help Directory)"},
        {"info", "/news", "", "(Shows Proxy News and Commands)"},
        {"info", "/gui", "", "(Open Floating ImGui Desktop Window)"},
        {"info", "/discord", "", "(Join Proxy Discord Community)"},
        {"info", "/scan", "", "(Toggle World Item Scan / Extract Mode)"},
        {"info", "/track", "", "(Toggle Drop/Collect Logs - See Them In /logs)"},
        {"info", "/logs", "", "(World Logs: Roulette, Drop & Collect)"},

        // --- 1. Customize ---
        {"customize", "/flag", "[itemID]", "(Sets Country Flag To Item ID)"},
        {"customize", "/countrylist", "", "(Shows All Country Flag IDs List)"},
        {"customize", "/name", "[name]", "(Set Visual Display Name)"},
        {"customize", "/title", "", "(Open Title Selection GUI)"},
        {"customize", "/world", "", "(Toggle World Options: Chat, PVP, Nametags)"},
        {"customize", "/pathfind, /pf", "", "(Pathfinder Settings Page)"},
        {"customize", "/hotkeys", "", "(Configure Chat Command Hotkey Shortcuts)"},
        {"customize", "/options", "", "(Open All Selectable Feature Options Page)"},
        {"customize", "/showoc", "", "(Show Open/Closed Doors)"},
        {"customize", "/me", "", "(Toggle /me Effect)"},

        // --- 2. Main Features ---
        {"mf", "/speed", "[speed] [gravity]", "(Open Speed & Gravity Settings)"},
        {"mf", "/wrench", "", "(Select Wrench Mode: Pull/Kick/Ban)"},
        {"mf", "/wrenchmsg", "", "(Wrench A Player To /msg Them Your /setmsg Text)"},
        {"mf", "/wrenchspam", "", "(Wrench A Player To Say Your /setmsg Text)"},
        {"mf", "/wm", "", "(Enable & Disable Wrench Mode)"},
        {"mf", "/ws", "[mode]", "(Set Wrench Mode: pull/kick/ban)"},
        {"mf", "/rkick", "", "(Enable & Disable Right Click Kick Mode)"},
        {"mf", "/setmsg", "<text>", "(Set Message For /wrenchmsg & /wrenchspam)"},
        {"mf", "/spam", "", "(Toggle AutoSpam Options)"},
        {"mf", "//", "", "(Shortcut To Enable/Disable Chat Auto-Spam)"},
        {"mf", "/spamdelay", "<ms>", "(Set Auto-Spam Delay In Milliseconds)"},
        {"mf", "/automsg", "", "(Toggle Auto-Message Broadcast Bot)"},
        {"mf", "/autopull", "", "(Auto Pull Menu For New Visitors)"},
        {"mf", "/pullauto", "", "(Toggle Auto Pull)"},
        {"mf", "/autopulltile", "", "(Auto Pull By Selected Tiles)"},
        {"mf", "/apt", "", "(Select Auto Pull Tiles)"},
        {"mf", "/sapt", "", "(Show Auto Pull Tile Highlights)"},
        {"mf", "/pulltile, /pt", "", "(Enable Auto Pull Tiles)"},
        {"mf", "/aban, /autoban", "", "(Auto Ban Player When Joining World)"},
        {"mf", "/auto", "", "(Auto Pull/Ban Options)"},
        {"mf", "/autosurg", "", "(Toggle AutoSurgery Options)"},
        {"mf", "/surg", "", "(Enables AutoSurgery)"},
        {"mf", "/autocrime", "", "(Toggle AutoCrime Options)"},
        {"mf", "/crime", "", "(Enables AutoCrime)"},
        {"mf", "/join", "", "(Auto Ban/Pull/Kick When Player Joins)"},
        {"mf", "/vendfast", "", "(Fast Vend Page: Empty, Stock, Buy)"},
        {"mf", "/fastvend", "", "(Enable/Disable Fast Vend Auto-Stock)"},
        {"mf", "/buy", "", "(Fast Growtopia Shop Purchase Page)"},
        {"mf", "/host", "", "(Casino Host Settings & Custom Tax Amount)"},
        {"mf", "/growscan", "", "(Free Fully Functional Growscan On Punch/Scan)"},
        {"mf", "/collect", "", "(Auto Collect Page - Enable/Disable & Set Collect Range)"},
        {"mf", "/count", "", "(Edit Drop/Trash/Recycle/Vend Count Settings)"},
        {"mf", "/fd", "", "(Enable/Disable Fast Drop Auto-Confirm)"},
        {"mf", "/ft", "", "(Enable/Disable Fast Trash Auto-Confirm)"},
        {"mf", "/fr", "", "(Enable/Disable Fast Recycle Auto-Confirm)"},
        {"mf", "/autofish", "", "(Auto Fish Bot - Automatically Catches Fish)"},
        {"mf", "/autofarm", "", "(Auto Farm Bot - Automatically Punches & Farms)"},
        {"mf", "/farm", "", "(Toggle Auto Farm On/Off)"},
        {"mf", "/fish", "", "(Toggle Auto Fish On/Off)"},
        {"mf", "/autoharvest", "", "(Auto Harvest Page - Walks To & Punches Ready Trees)"},
        {"mf", "/harvest", "", "(Toggle Auto Harvest On/Off)"},
        {"mf", "/autoplant", "", "(Auto Plant Page - Plants Seeds On A Target Block)"},
        {"mf", "/plant", "", "(Toggle Auto Plant On/Off)"},
        {"mf", "/autocollect", "", "(Toggle Auto Collect Floating Dropped Items)"},
        {"mf", "/ac", "", "(Shortcut To Enable Autocollect)"},
        {"mf", "/autocomp", "", "(Compress 100 WLs to 1 DL Automatically)"},
        {"mf", "/banall", "", "(Bans Everyone In The World Without Access)"},
        {"mf", "/pullall", "", "(Pulls Everyone In The World)"},
        {"mf", "/banfire", "", "(Toggle Auto-Ban Fire Mode For Pocket Lighter Grief)"},
        {"mf", "/ghost", "", "(Toggle Moderator Ghost Mode - Walk Through Blocks)"},

        // --- 3. Auto Hoster ---
        {"casino", "/tp", "", "(Starts Autohoster Bet Sweep [Don't Move])"},
        {"casino", "/win1, /win2", "", "(Drops Prize To Pos1-2 & Returns To Host Spot)"},
        {"casino", "/game", "[bet]", "(Set Game Bet Count & Calculate Tax)"},
        {"casino", "/gdrop", "", "(Drop Prize To Winner With Tax Deduction)"},
        {"casino", "/settax", "[%]", "(Set Tax Percentage e.g. 5 = 5%)"},
        {"casino", "/pos1, /pos2, /pos3, /pos4", "", "(Set Pos1-4 For Autohost To Teleport)"},
        {"casino", "/posback", "", "(Set Return Position After Drop/Host)"},
        {"casino", "/spos1, /spos2", "", "(Punch Tile To Set Pos1-2 For Autohost)"},
        {"casino", "/cpos1, /cpos2", "", "(Highlight & Check Saved Pos1-2)"},
        {"casino", "/tp1, /tp2, /tp3, /tp4", "", "(Teleport/Pathfind To Saved Pos1-4)"},
        {"casino", "/qq, /qeme", "[on/off]", "(Toggle Show QQ Number in Roulette Spin)"},
        {"casino", "/leme", "", "(Enable & Disable Leme Spin)"},
        {"casino", "/gp, /gaspull", "", "(Auto Pull When Someone Says Gas/Play)"},
        {"casino", "/reme", "[on/off]", "(Toggle Show REME Spin in Roulette Spin)"},

        // --- 5. Economy & Drops ---
        {"econ", "/dw", "<amount>", "(Drop World Locks Without Confirmation Dialog)"},
        {"econ", "/cd, /cdrop", "<amount>", "(Custom Drop World Locks Exact Amount)"},
        {"econ", "/dd, /ddrop", "<amount>", "(Drop Diamond Locks Amount)"},
        {"econ", "/dbgl", "<amount>", "(Drop Blue Gem Locks Without Confirmation)"},
        {"econ", "/dropbgl", "<amount>", "(Drop Specified Amount Of BGLs)"},
        {"econ", "/daw", "", "(Drop All Locks: WLs, DLs & BGLs)"},
        {"econ", "/dbglvis", "<amount>", "(Visual Drop Blue Gem Locks [No Loss])"},
        {"econ", "/dropall", "[qty]", "(Drop Items From Inventory [All Or Quantity])"},
        {"econ", "/dpos", "", "(Set Target Drop Position Where You Stand)"},
        {"econ", "/dropat", "[amt]", "(Teleport To Saved Drop Spot & Drop Prize)"},
        {"econ", "/balance, /bal", "", "(Shows Your WL / DL / BGL Balance In A Dialog)"},
        {"econ", "/arroz, /roz", "[amount]", "(Drop Arroz Con Pollo)"},
        {"econ", "/autobgl, /bgl", "", "(Auto Change BGL When Wrench Phone)"},
        {"econ", "/fastbgl", "[amount]", "(Fast BGL via WL Storage)"},

        // --- 6. Clothes & Visual Customization ---
        {"visual", "/find", "[item name]", "(Find An Item So You Can Visually Add To Inventory)"},
        {"visual", "/clothes", "", "(Visual Clothes Options & Wardrobe)"},
        {"visual", "/flag", "[itemID]", "(Sets Flag To Item ID)"},
        {"visual", "/titles", "", "(You Can Select Visual g4g, maxlevel, dr, mentor...)"},
        {"visual", "/legend", "", "(Sets Your Name To Legendary Name)"},
        {"visual", "/dr", "", "(Apply Dr. Title)"},
        {"visual", "/mentor", "", "(Apply Mentor Title)"},
        {"visual", "/maxlevel", "", "(Apply Max Level Title)"},
        {"visual", "/g4g", "", "(Apply G4G Title)"},
        {"visual", "/cleartitle", "", "(Remove All Titles And Reset Name)"},
        {"visual", "/name", "[name]", "(Set Visual Name To Your Name)"},
        {"visual", "/skin", "<r> <g> <b>", "(Set Visual Skin Color To Yourself)"},
        {"visual", "/weather", "[id]", "(Custom Weather Machine dialog)"},
        {"visual", "/vision", "", "(Visually Replace Background Blocks To Glass Pane)"},
        {"visual", "/invis", "", "(Visual Moderator Invis Mode)"},
        {"visual", "/fakeban", "", "(Visually Get Perma-Ban Notification)"},
        {"visual", "/warn", "", "(Warn Yourself With Any Text Notification)"},
        {"visual", "/save1, /save2, /save3, /save4", "", "(Save Current Visual Clothes Set To Slot 1-4)"},
        {"visual", "/load1, /load2, /load3, /load4", "", "(Load Saved Visual Clothes Set From Slot 1-4)"},
        {"visual", "/set1, /set2, /set3, /set4", "", "(Shortcut To Equip Saved Set Slot 1-4)"},
        {"visual", "/clearclothes", "", "(Clear All Saved Visual Clothing Items)"},
        {"visual", "/rainbow", "", "(Toggle Rainbow Pure Being Mode)"},
        {"visual", "/ghostchar", "", "(Toggle Ghost Character Mode)"},
        {"visual", "/blink", "", "(Enable Rainbow Color-Cycle Blink Mode)"},

        // --- 7. Information ---
        {"information", "/growscan", "", "(Free Fully Functional Growscan For Items & Blocks)"},
        {"information", "/chest", "", "(Shows Hidden Items In Chests)"},
        {"information", "/gems", "", "(Shows Gems Amount On Tiles & Settings)"},
        {"information", "/cgems", "", "(Show Gem Count On All Tiles With Gems)"},
        {"information", "/dat", "[scan]", "(Show Tile Data, Floating Drops & Tile Scanner)"},
        {"information", "/doordat", "", "(Scan World For Doors & Portal Destinations)"},
        {"information", "/doorid", "", "(Toggle Door ID Reveal When Entering Doors)"},
        {"information", "/admin", "", "(Scan World For Locks And Show Owner/Admin Data)"},
        {"information", "/lockefind", "", "(Show Latest Worlds Where Locke Stopped By)"},
        {"information", "/devicecheck", "", "(Show Tracked Spawned Players And Their Device Type)"},
        {"information", "/players", "", "(List All Tracked Players With NetID And Tile Position)"},
        {"information", "/inv", "[item_id]", "(Show Inventory Info & Item Counts)"},
        {"information", "/showxy", "", "(Show Current X,Y Tile Coordinate Position)"},
        {"information", "/uid", "", "(Display Current Player User ID)"},
        {"information", "/scan", "", "(Toggle World Item Scan & Extract Mode)"},
        {"information", "/ping", "", "(Toggle Real-Time Ping Latency Display)"},
        {"information", "/track", "", "(Toggle Drop/Collect Logs - See Them In /logs)"},
        {"information", "/balance, /bal", "", "(Shows Your WL / DL / BGL Balance In A Dialog)"},

        // --- 8. Shortcuts ---
        {"sc", "/antigravity", "", "(Toggle Unlimited Jumping & Zero Gravity)"},
        {"sc", "/antipunch", "", "(Activate Anti-Punch Visual Jammer)"},
        {"sc", "/immune", "", "(Toggle Immunity To Fire, Lava & Spike Hazards)"},
        {"sc", "/moddetect", "", "(Toggle Moderator Spawn Detection & Alert)"},
        {"sc", "/back", "", "(Warps You To A Previously Visited World)"},
        {"sc", "/save", "", "(Warps You To A Save World)"},
        {"sc", "/setsave", "[world]", "(Set Save World)"},
        {"sc", "/relog", "", "(Fast Exit & Join Back To The World)"},
        {"sc", "/exit", "", "(Leave The Current World)"},
        {"sc", "/res, /respawn", "", "(Respawn Right Away)"},
        {"sc", "/logout", "", "(Log Out & Go Back To The Growtopia Main Menu)"},
        {"sc", "/run", "", "(Panic Moderator Escape: Hops 12 Random Worlds)"},
        {"sc", "/warp", "<world>", "(Warp Directly Into Specified World)"},
        {"sc", "/vendlogs", "", "(Opens Up A Page With Proxy Saved Logs)"},
        {"sc", "/vendf", "[item]", "(Search For Items In Vending Machines Across Worlds)"},
        {"sc", "/vendtp", "<item>", "(Highlight Matching Vends In Current World)"},
        {"sc", "/vendsafe", "", "(Toggle Safe Vending Buy Anti-Scam Protection)"},
        {"sc", "/door", "<door_id>", "(Join Specific Door ID In Current World)"},
        {"sc", "/fastdoor", "", "(Toggle Fast Open/Close Entrance Doors)"},
        {"sc", "/path", "<x> <y>", "(Walk To Coordinates Using Smart A* Pathfinding)"},
        {"sc", "/player", "<name>", "(Teleport And Pathfind To Player By Name)"},
        {"sc", "/setpos", "<x> <y>", "(Teleport Character To Position Coordinates)"},
        {"sc", "/pf", "", "(Toggle Click-To-Walk Pathfinder Mode ON/OFF)"},
        {"sc", "/rema", "", "(Remove Your Access From All Locks In World)"},
        {"sc", "/tf", "", "(Enables Fast Trash/Recycle Auto-Confirm)"},

        // --- 9. Other Features ---
        {"of", "/bankadd", "[amt]", "(Deposit World Locks From Inventory To Storage)"},
        {"of", "/bankwith", "[amt]", "(Withdraw World Locks From Bank Storage)"},
        {"of", "/bankcheck", "", "(Check Stored World Lock Bank Balance)"},
        {"of", "/wlbank", "<amount>", "(Modify World Lock Storage Amount)"},
        {"of", "/dbox", "", "(Auto-Donate Items To Donation Box In World)"},
        {"of", "/fillgbc", "", "(Auto-Fill Well of Love With Golden Booty Chests)"},
        {"of", "/doorbf", "", "(Bruteforce Hidden Door Passwords & IDs)"},
        {"of", "/bid", "<amount>", "(Place World Auction Bid With Specified Amount)"},
        {"of", "/mailclaim", "", "(Claim Mail Reward By Message ID)"},
        {"of", "/ignorecsn", "", "(Toggle Auto-Ignore For CSN/REME Seller Spam)"},
        {"of", "/ignorecsnchat", "", "(Toggle Auto-Ignore For CSN Casino Chat Spam)"},
        {"of", "/fakemaint", "<reason>", "(Send Fake Maintenance System Message Dialog)"},
        {"of", "/broadcast", "<text>", "(Inject Simulated Local Super-Broadcast SB)"},
        {"of", "/rawvar", "<name> [args]", "(Send Arbitrary Serialized VarList Packet)"},
        {"of", "/cmsg", "<text>", "(Send Console Message With Color Code Support)"},
        {"of", "/dialog", "<text>", "(Test Custom Raw Dialog Box Syntax)"},
        {"of", "/overlay", "<text>", "(Display On-Screen Text Overlay Banner)"},
        {"of", "/mstate", "", "(Toggle Mod State: Long Punch, Zoom & Reach)"},
        {"of", "/sm", "", "(Toggle SuperMod Visual State & Badge)"},
        {"of", "/betatest", "", "(Toggle Beta Tester Mode Interface Flags)"},
        {"of", "/freeze", "[netid]", "(Simulate Freeze Status On Yourself Or Player)"},
        {"of", "/hitvfx", "<id>", "(Trigger Hit Visual Impact Particle Effects 1-100)"},
        {"of", "/trade", "<start|end>", "(Send Trade Session Control Packets)"},
        {"of", "/action", "<type>", "(Trigger Generic Game Action Packets)"},
        {"of", "/spawnbgls", "", "(Spawn Visual Blue Gem Locks Across World)"},
        {"of", "/initworld", "", "(Reinitialize World State With OnInitNewWorld)"},
        {"of", "/bux", "<tokens>", "(Update Client Growtokens Display With OnSetBux)"},
        {"of", "/debuganim", "", "(Debug Real vs Visual Equipped Punch Animations)"},
        {"of", "/event", "<name>", "(Simulate Holiday Event: Valentines, Easter, Halloween)"}
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

// Extra space after the last row so it can be scrolled fully into view (see show_commands_gui)
static constexpr int kBottomPadding = 40;

static int s_current_proxy_tab = 0;
static std::atomic<bool> s_switching_tab = false;

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

    int tab = 0;
    std::string filter = "";
    if (args.size() > 1) {
        if (args[1] == "1" || args[1] == "vin" || args[1] == "vinproxy" || args[1] == "news" || args[1] == "default") tab = 0;
        else if (args[1] == "2" || args[1] == "main" || args[1] == "feat" || args[1] == "features") tab = 1;
        else if (args[1] == "3" || args[1] == "log" || args[1] == "logs") tab = 2;
        else if (args[1] == "4" || args[1] == "mod" || args[1] == "mods" || args[1] == "cheat") tab = 3;
        else if (args[1] == "5" || args[1] == "opt" || args[1] == "option" || args[1] == "options" || args[1] == "settings") tab = 4;
        else filter = args[1];
    }
    s_current_proxy_tab = tab;
    ProxyCommand::show_commands_gui(client->get_player(), g_core_proxy, filter, tab);
}

static std::vector<std::string> get_categories_for_tab(int tab) {
    switch (tab) {
        case 0: return {"info", "customize", "mf", "casino", "econ", "visual", "information", "sc", "of"}; // Tab 0: All Commands
        case 1: return {}; // Tab 1: Main Features (reserved)
        case 2: return {}; // Tab 2: Proxy Logs (reserved)
        case 3: return {}; // Tab 3: Hidden Mods (reserved)
        case 4: return {}; // Tab 4: Options (reserved)
        default: return {};
    }
}

static void ensure_vin_tabs_texture_installed() {
    try {
        const char* local_app = std::getenv("LOCALAPPDATA");
        if (!local_app) return;
        std::filesystem::path gt_dir = std::filesystem::path(local_app) / "Growtopia" / "interface" / "large";
        std::filesystem::path target = gt_dir / "vin_tabs.rttex";
        std::filesystem::path src = "resources/interface/large/vin_tabs.rttex";

        bool needs_install = false;
        if (!std::filesystem::exists(target)) {
            needs_install = true;
        } else if (std::filesystem::file_size(target) != g_vin_tabs_rttex_size) {
            needs_install = true;
        }

        if (needs_install) {
            std::filesystem::create_directories(gt_dir);
            // 1. If resources folder exists alongside VinProxy, copy from there
            if (std::filesystem::exists(src)) {
                std::filesystem::copy_file(src, target, std::filesystem::copy_options::overwrite_existing);
                spdlog::info("ProxyCommand: Installed vin_tabs.rttex from resources into Growtopia folder");
            } else {
                // 2. Standalone embedded fallback: unpack directly from binary memory!
                std::ofstream out(target, std::ios::binary);
                if (out.is_open()) {
                    out.write(reinterpret_cast<const char*>(g_vin_tabs_rttex_data), g_vin_tabs_rttex_size);
                    out.close();
                    spdlog::info("ProxyCommand: Unpacked embedded vin_tabs.rttex ({} bytes) into Growtopia folder", g_vin_tabs_rttex_size);
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::debug("ProxyCommand: ensure_vin_tabs error: {}", e.what());
    }
}

void ProxyCommand::show_commands_gui(player::Player* player, core::Core* core, const std::string& filter, int active_tab) {
    if (!player) {
        spdlog::error("ProxyCommand: player is null!");
        return;
    }

    auto* server = core ? core->get_server() : nullptr;
    auto* target_player = (server && server->get_player()) ? server->get_player() : player;
    if (!target_player) {
        spdlog::error("ProxyCommand: No player available!");
        return;
    }

    s_current_proxy_tab = active_tab;
    ensure_vin_tabs_texture_installed();

    try {
        const auto& all_cmds = get_all_commands();
        const auto& all_cats = get_categories();
        auto tab_cats = get_categories_for_tab(active_tab);

        std::string filter_lower = to_lower(trim(filter));

        std::ostringstream dialog;
        dialog << "set_default_color|`o\n";

        // 5 Custom Tabs docked directly on top of dialog header
        dialog << "start_custom_tabs|\n";
        dialog << fmt::format("add_custom_button|proxy_tab_0|image:interface/large/vin_tabs.rttex;image_size:228,92;frame:{},0;width:0.16;|\n", active_tab == 0 ? 1 : 0);
        dialog << fmt::format("add_custom_button|proxy_tab_1|image:interface/large/vin_tabs.rttex;image_size:228,92;frame:{},1;width:0.16;|\n", active_tab == 1 ? 1 : 0);
        dialog << fmt::format("add_custom_button|proxy_tab_2|image:interface/large/vin_tabs.rttex;image_size:228,92;frame:{},2;width:0.16;|\n", active_tab == 2 ? 1 : 0);
        dialog << fmt::format("add_custom_button|proxy_tab_3|image:interface/large/vin_tabs.rttex;image_size:228,92;frame:{},3;width:0.16;|\n", active_tab == 3 ? 1 : 0);
        dialog << fmt::format("add_custom_button|proxy_tab_4|image:interface/large/vin_tabs.rttex;image_size:228,92;frame:{},4;width:0.16;|\n", active_tab == 4 ? 1 : 0);
        dialog << "end_custom_tabs|\n";
        dialog << "add_spacer|small|\n";

        // Tab Header Title
        if (active_tab == 0) {
            dialog << "add_label_with_icon|big|`2VinProxy Premium Commands``|left|7188|\n";
        } else if (active_tab == 1) {
            dialog << "add_label_with_icon|big|`wVinProxy: `3Main Features & Gameplay``|left|5956|\n";
        } else if (active_tab == 2) {
            dialog << "add_label_with_icon|big|`wVinProxy: `eProxy Logs & Activity History``|left|3524|\n";
        } else if (active_tab == 3) {
            dialog << "add_label_with_icon|big|`wVinProxy: `4Hidden Mods & Cheats``|left|4758|\n";
        } else {
            dialog << "add_label_with_icon|big|`wVinProxy: `6Options & Configuration``|left|32|\n";
        }
        dialog << "add_spacer|small|\n";

        // Tab Subheader & Description
        if (active_tab == 0) {
            // dialog << "add_label_with_icon|big|`2All Commands:``|left|5956|\n";
        } else if (active_tab == 1) {
            // dialog << "add_smalltext|`3[MAIN FEATURES] `wMovement, Pathfinding, World Scanner, Casino & Economy``|\n";
        } else if (active_tab == 2) {
            // dialog << "add_smalltext|`e[PROXY LOGS] `wReal-time Packet Streams, Activity Records & Transaction Logs``|\n";
        } else if (active_tab == 3) {
            // dialog << "add_smalltext|`4[HIDDEN MODS] `wStaff Detection, Godmode Immunity & World Moderation Cheats``|\n";
        } else {
            // dialog << "add_smalltext|`6[OPTIONS] `wProxy Settings, Auto-Spam, Automation Timers & Client Toggles``|\n";
        }

        // dialog << "add_spacer|small|\n";

        // Tab 2: Proxy Logs (same logs as /logs, with enable/disable buttons)
        if (active_tab == 2 && filter_lower.empty()) {
            dialog << LogsCommand::proxy_logs_tab();
        }

        // Tab 4: Options (same page as /options; checkboxes are saved with the Save Options button)
        if (active_tab == 4 && filter_lower.empty()) {
            dialog << OptionsCommand::options_content();
            dialog << "add_spacer|small|\n";
            dialog << "add_button|options_save|`2Save Options|noflags|0|0|\n";
        }

        // Reserved / Testing area for Tabs 1 and 3 (user will customize these later)
        if ((active_tab == 1 || active_tab == 3) && filter_lower.empty()) {
            dialog << "add_spacer|small|\n";
            dialog << "add_textbox|`o[RESERVED / TESTING AREA]``|left|\n";
            dialog << "add_smalltext|`7This page is reserved for testing and custom buttons.``|\n";
            dialog << "add_smalltext|`7All proxy commands are listed completely under the `wVIN PROXY`7 tab above.``|\n";
            dialog << "add_spacer|small|\n";
        }

        // Render matching categories and commands (active for Tab 0 or active search filter)
        for (const auto& cat : all_cats) {
            bool is_cat_in_tab = std::find(tab_cats.begin(), tab_cats.end(), cat.id) != tab_cats.end();
            if (filter_lower.empty() && !is_cat_in_tab) continue;

            std::vector<const CommandDoc*> cat_cmds;
            for (const auto& c : all_cmds) {
                if (c.category != cat.id) continue;
                if (!filter_lower.empty()) {
                    std::string search_target = to_lower(c.cmd + " " + c.args + " " + c.desc + " " + c.aliases + " " + c.category);
                    if (search_target.find(filter_lower) == std::string::npos) continue;
                }
                cat_cmds.push_back(&c);
            }

            if (cat_cmds.empty()) continue;

            dialog << "add_spacer|small|\n";
            dialog << "add_label_with_icon|small|" << cat.color << cat.title << " (" << cat_cmds.size() << ")``|left|" << cat.icon << "|\n";
            dialog << "add_spacer|small|\n";

            for (const auto* c : cat_cmds) {
                std::string cmd_color = c->color.empty() ? "`2" : c->color;
                std::string arg_str = c->args.empty() ? "" : (" `9" + c->args);
                std::string line = fmt::format("{}{}{} `9{}``", cmd_color, c->cmd, arg_str, c->desc);
                // '|' separates dialog fields; "<start|end>" would cut the line short
                std::replace(line.begin(), line.end(), '|', '/');
                dialog << "add_smalltext|" << line << "|\n";
            }
        }
        // Bottom padding: the custom tab strip pushes the content down, but Growtopia sizes the
        // scroll area without it, so the last ~100px stayed hidden. Trailing spacers are ignored,
        // so reserve the space with a margin followed by an invisible text line.
        dialog << fmt::format("add_custom_margin|x:0;y:{}|\n", kBottomPadding);
        dialog << "add_custom_textbox|.|size:tiny;color:0,0,0,0|\n";
        dialog << "end_dialog|proxy_commands_gui|||\n";
        dialog << "add_quick_exit|\n";

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

        target_player->send_packet(byte_stream.get_data(), 0);

        spdlog::debug("ProxyCommand: Tabbed GUI sent (tab={}, dialog_name=proxy_commands_gui)", active_tab);

    } catch (const std::exception& e) {
        spdlog::error("ProxyCommand: Failed to send GUI: {}", e.what());
    }
}

void ProxyCommand::handle_dialog_return(player::Player* player, const std::string& button_clicked, const std::string& search_query,
                                        const std::string& raw) {
    if (!player || !g_core_proxy) return;

    int new_tab = -1;
    if (button_clicked == "proxy_tab_0") new_tab = 0;
    else if (button_clicked == "proxy_tab_1") new_tab = 1;
    else if (button_clicked == "proxy_tab_2") new_tab = 2;
    else if (button_clicked == "proxy_tab_3") new_tab = 3;
    else if (button_clicked == "proxy_tab_4") new_tab = 4;

    if (new_tab != -1) {
        if (s_switching_tab.exchange(true)) {
            spdlog::debug("ProxyCommand: Tab switch in progress, ignoring click on tab {}", new_tab);
            return;
        }

        s_current_proxy_tab = new_tab;

        spdlog::debug("ProxyCommand: Tab {} clicked. Dialog closed, re-triggering /proxy in 500ms...", new_tab);

        // Allow the Growtopia client to cleanly finish closing all dialogs on screen
        // before re-triggering the /proxy dialog with the newly selected tab.
        std::thread([core = g_core_proxy, player, new_tab]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            try {
                auto* server = core ? core->get_server() : nullptr;
                auto* send_to = (server && server->get_player()) ? server->get_player() : player;
                if (send_to && send_to->is_connected()) {
                    ProxyCommand::show_commands_gui(send_to, core, "", new_tab);
                    spdlog::debug("ProxyCommand: Automatically re-triggered /proxy for tab {}", new_tab);
                }
            } catch (const std::exception& e) {
                spdlog::error("ProxyCommand: Failed to re-trigger /proxy: {}", e.what());
            }
            s_switching_tab = false;
        }).detach();
        return;
    }

    // Options tab: "Save Options" applies the checkboxes and re-opens the tab,
    // "Mod Detect Settings" opens its page (the client closes the dialog on click, so wait first)
    if (button_clicked == "options_save" || button_clicked == "kadaryt") {
        if (s_switching_tab.exchange(true)) return;
        std::thread([core = g_core_proxy, player, button_clicked, raw]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            try {
                OptionsCommand::handle_dialog(raw);
                if (button_clicked == "options_save") {
                    auto* server = core ? core->get_server() : nullptr;
                    auto* send_to = (server && server->get_player()) ? server->get_player() : player;
                    if (send_to && send_to->is_connected()) {
                        s_current_proxy_tab = 4;
                        ProxyCommand::show_commands_gui(send_to, core, "", 4);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("ProxyCommand: Options tab button failed: {}", e.what());
            }
            s_switching_tab = false;
        }).detach();
        return;
    }

    // Proxy Logs tab buttons: toggles re-open the tab, others open the full /logs pages.
    // The client closes the dialog on click, so wait like the tab switch does.
    if (button_clicked.rfind("logs_", 0) == 0) {
        if (s_switching_tab.exchange(true)) return;
        std::thread([core = g_core_proxy, player, button_clicked]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            try {
                if (LogsCommand::apply_toggle(button_clicked)) {
                    auto* server = core ? core->get_server() : nullptr;
                    auto* send_to = (server && server->get_player()) ? server->get_player() : player;
                    if (send_to && send_to->is_connected()) {
                        s_current_proxy_tab = 2;
                        ProxyCommand::show_commands_gui(send_to, core, "", 2);
                    }
                } else {
                    LogsCommand::handle_dialog("logs_menu", button_clicked, "");
                }
            } catch (const std::exception& e) {
                spdlog::error("ProxyCommand: Proxy Logs button failed: {}", e.what());
            }
            s_switching_tab = false;
        }).detach();
        return;
    }

    bool is_close = (button_clicked.empty() || button_clicked == "close" || button_clicked == "Close" || button_clicked == "cancel");
    if (is_close) {
        if (s_current_proxy_tab != 0) {
            if (s_switching_tab.exchange(true)) {
                return;
            }
            int prev_tab = s_current_proxy_tab;
            s_current_proxy_tab = 0;
            spdlog::debug("ProxyCommand: X/ESC clicked on tab {}. Backing to VinProxy tab 0...", prev_tab);

            std::thread([core = g_core_proxy, player]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                try {
                    auto* server = core ? core->get_server() : nullptr;
                    auto* send_to = (server && server->get_player()) ? server->get_player() : player;
                    if (send_to && send_to->is_connected()) {
                        ProxyCommand::show_commands_gui(send_to, core, "", 0);
                        spdlog::debug("ProxyCommand: Successfully backed to VinProxy tab 0 via X/ESC");
                    }
                } catch (const std::exception& e) {
                    spdlog::error("ProxyCommand: Failed to back to tab 0: {}", e.what());
                }
                s_switching_tab = false;
            }).detach();
            return;
        }

        s_current_proxy_tab = 0;
        spdlog::debug("ProxyCommand: Dialog closed via X/ESC on VinProxy tab 0");
        return;
    }

    if (button_clicked == "search_btn" || (!search_query.empty() && !is_close)) {
        ProxyCommand::show_commands_gui(player, g_core_proxy, search_query, 0);
        return;
    }
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
    ProxyCommand::show_commands_gui(client->get_player(), g_core_info, filter, 0);
}

} 
