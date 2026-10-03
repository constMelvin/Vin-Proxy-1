#include "options_command.hpp"
#include "lucky_common.hpp"
#include "autoaccess.hpp"
#include "fastbox.hpp"
#include "hidelevel.hpp"
#include "blockmsg.hpp"
#include "dicespeed.hpp"
#include "antigravity.hpp"
#include "showoc_command.hpp"
#include "autobgl_command.hpp"
#include "fastbgl_command.hpp"
#include "autosurg_command.hpp"
#include "banfire_command.hpp"
#include "dropfast_command.hpp"
#include "trashfast_command.hpp"
#include "moddetect_command.hpp"
#include "fastdoor_command.hpp"
#include "utility_commands.hpp"
#include "wrench_command.hpp"
#include "extended_commands.hpp"
#include "../../server/server.hpp"
#include "../../utils/gems_manager.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/dialog.hpp"
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace command {

OptionsCommand::OptionsCommand() : CommandBase({"options"}, {}, "Open the options page", 0) {}
std::unique_ptr<CommandBase> OptionsCommand::clone() const { return std::make_unique<OptionsCommand>(*this); }
void OptionsCommand::execute(client::Client*, const std::vector<std::string>&) { show_dialog(); }

void OptionsCommand::show_dialog() {
    utils::Dialog dlg;
    dlg.label_with_icon("`5Options Page", 262)
       .spacer()
       .raw(options_content())
       .end_dialog("options_page", "Cancel", "Okay");
    dlg.send(lucky::local_out());
}

// Same layout and order as the LuckyProxy /options page (+ Show Open / Closed Doors)
std::string OptionsCommand::options_content() {
    const std::string wrench_mode = WrenchCommand::current_mode();
    const std::string mode_label = wrench_mode.empty() ? WrenchCommand::saved_mode() : wrench_mode;
    const bool ping = lucky::cfg_flag("display.show_ping", true) || lucky::cfg_flag("features.host.show_ping", true);

    utils::Dialog dlg;
    dlg.checkbox("autosurg", "`^Enable `5Auto Surg ", AutoSurgCommand::is_enabled())
       .description("Grants you the ability to Auto Surg")
       .checkbox("fastdrbp", "`^Enable `5Auto Access", AutoAccess::is_enabled())
       .description("it Allows You To Automaticaly Accept Any Access From Any Given Lock.")
       .checkbox("ciacia", "`^Enable `5Fast Change BGL", AutoBglCommand::is_enabled())
       .description("Enables You To Fast Change BGL With Wrench Only.")
       .checkbox("fastbgl", "`^Enable `5Fast BGL (Storage)", FastBglCommand::is_enabled())
       .description("Convert DL->BGL via World Lock Storage without phone dialogs.")
       .checkbox("fastbox", "`^Enable `5Fast Retrieve Donation Box", FastBox::is_enabled())
       .description("Enables Fast Retrieve Donation Box Without Additional Dialogs.")
       .checkbox("sadboy", "`^Enable `5Fast Open / Close Entrances", FastDoorCommand::is_enabled())
       .description("Grants You ability to Fast Closes / Open Entrances With Right Click Mouse.")
       .checkbox("showoc", "`^Show `5Open / Closed Doors", ShowOcCommand::is_enabled())
       .description("Paints Open Entrances `2Green`o And Closed Entrances `4Red`o. Same As /showoc.")
       .checkbox("fasttrqp", "`^Enable `5Pathfinding Toggle", FindPathCommand::is_click_mode_enabled())
       .description("You Can Teleport To Any Position With `2Shift + Click.")
       .checkbox("fastdrop", "`^Enable `5Fast Drop", DropFastCommand::is_enabled())
       .description("Enables Fast Drop Without Additional Dialogs /dcount [ amount ] For Specific Amount.")
       .checkbox("fasttrap", "`^Enable `5Fast Trash", TrashFastCommand::is_enabled())
       .description("Enables Fast Trash Without Additional Dialogs /tcount [ amount ] For Specific Amount..")
       .checkbox("fastdrjp", "`^Enable `5Ghost Mode", GhostCommand::is_enabled())
       .description("Enables To Moderator Mode Can Go Throught Blocks.")
       .checkbox("hidelevel", "`^Enable `5Hide You need to be Level x Message", HideLevel::is_enabled())
       .description("Will Allow You To block You need to be level x message.")
       .checkbox("blockmsg", "`^Enable `5Hide Private Messages", BlockMsg::is_enabled())
       .description("Enables You To Ignoring All Private Messages.")
       .checkbox("graciacia", "`^Enable `5Anti Gravity", AntiGravity::is_enabled())
       .description("Grants You Ability To Unlimited Jumps")
       .checkbox("wrenchmode", "`^Enable `5Wrench " + mode_label, !wrench_mode.empty())
       .description("Enables Fast Wrenching. You Can Select Modes in /wrench.")
       .checkbox("enablemod", "`^Enable `5Auto Mod Detect", ModDetectCommand::is_enabled())
       .description("Enables Automatic `#@Mod`` Detector & Acts Upon Mod Detect Settings")
       .button("kadaryt", "`5Mod Detect Settings")
       .checkbox("firekontol", "`^Enable `5Pocket Lighter Ban", BanFireCommand::is_enabled())
       .description("Will Automatically Ban Someone Put Pocket Lighter.")
       .checkbox("apiyatim", "`^Enable `5Eldritch Flame Ban", BanFireCommand::is_enabled())
       .description("Will Automatically Ban Someone Put Eldritch Flame.")
       .checkbox("tilegems", "`^Show `5Instant Gem Drop Count", utils::GemsManager::get_instance().instant_gems)
       .description("Will Display Gem Count On A Tile Instantly After Spawning.")
       .checkbox("dicespeed", "`^Show `5Instant Dice Roll", DiceSpeed::is_enabled())
       .description("Grants You Instant Dice Roll Number.")
       .checkbox("namenumber", "`^Show `5Last Roulette Spin", lucky::cfg_flag("features.host.show_last_spin", true))
       .description("Will Display Every Player's last Roulette Wheel Spin Next To Their Name.")
       .checkbox("pingcheck", "`^Display `5Ping Latency", ping)
       .description("Will Display Your Ping Latency Next To Your Name.")
       .checkbox("realfake", "`^Show `5Real / Fake Spin", lucky::cfg_flag("features.host.show_real_spin", true))
       .description("Will Show You `w[`2REAL`w] and `w[`4FAKE`w] Roulette Wheel Spins.")
       .checkbox("remespin", "`^Show `5Reme Spin", lucky::cfg_flag("features.host.show_reme_spin", true))
       .description("Will Show You Reme Numbers Next To Original Spin.")
       .checkbox("qemekontol", "`^Show `5Qeme Spin", lucky::cfg_flag("features.host.show_qq_number", false))
       .description("Will Show You Qeme Numbers Next To Original Spin.")
       .checkbox("evelyn", "`^Show `5Leme Spin", lucky::cfg_flag("features.host.show_leme_spin", false))
       .description("Will Show You Leme Numbers Next To Original Spin.");
    return dlg.str();
}

void OptionsCommand::handle_dialog(const std::string& raw) {
    TextParse tp{raw};
    // LuckyProxy: the Mod Detect Settings button only opens its page
    if (tp.get("buttonClicked").find("kadaryt") == 0) {
        spdlog::debug("[Options] Opening Mod Detect Settings");
        ModDetectCommand::show_settings_dialog();
        return;
    }

    auto has = [&](const char* key) { return !tp.get(key).empty(); };
    auto on = [&](const char* key) {
        std::string v = tp.get(key);
        v.erase(std::remove_if(v.begin(), v.end(), [](unsigned char c) { return std::isspace(c); }), v.end());
        return !v.empty() && v != "0";
    };
    auto sync_toggle = [&](const char* key, bool current, auto toggle) {
        if (has(key) && on(key) != current) toggle();
    };

    sync_toggle("autosurg", AutoSurgCommand::is_enabled(), [] { AutoSurgCommand::toggle(); });
    sync_toggle("fastdrop", DropFastCommand::is_enabled(), [] { DropFastCommand::toggle(); });
    sync_toggle("fasttrap", TrashFastCommand::is_enabled(), [] { TrashFastCommand::toggle(); });
    sync_toggle("fasttrqp", FindPathCommand::is_click_mode_enabled(), [] { FindPathCommand::toggle_click_mode(); });
    sync_toggle("enablemod", ModDetectCommand::is_enabled(), [] { ModDetectCommand::toggle(); });

    // Pocket Lighter and Eldritch Flame share one switch (as in LuckyProxy); a change on either applies
    if (has("firekontol") || has("apiyatim")) {
        const bool current = BanFireCommand::is_enabled();
        const bool fire = has("firekontol") ? on("firekontol") : current;
        const bool flame = has("apiyatim") ? on("apiyatim") : current;
        const bool want = (fire != current) ? fire : flame;
        if (want != current) BanFireCommand::toggle();
    }

    if (has("sadboy")) FastDoorCommand::set_enabled(on("sadboy"));
    if (has("showoc")) ShowOcCommand::set_enabled(on("showoc"));
    if (has("fastdrjp") && on("fastdrjp") != GhostCommand::is_enabled()) GhostCommand::set_enabled(on("fastdrjp"));
    if (has("graciacia")) AntiGravity::set_enabled(on("graciacia"));
    if (has("tilegems")) utils::GemsManager::get_instance().instant_gems = on("tilegems");

    if (has("wrenchmode")) {
        const bool want = on("wrenchmode");
        const std::string current = WrenchCommand::current_mode();
        if (want && current.empty()) {
            WrenchCommand::apply_mode(WrenchCommand::saved_mode());
        } else if (!want && !current.empty()) {
            WrenchCommand::remember_mode(current);
            WrenchCommand::apply_mode("");
        }
    }

    if (has("fastdrbp")) AutoAccess::set_enabled(on("fastdrbp"));
    if (has("ciacia")) AutoBglCommand::set_enabled(on("ciacia"));
    if (has("fastbgl")) FastBglCommand::set_enabled(on("fastbgl"));
    if (has("fastbox")) FastBox::set_enabled(on("fastbox"));
    if (has("hidelevel")) HideLevel::set_enabled(on("hidelevel"));
    if (has("blockmsg")) BlockMsg::set_enabled(on("blockmsg"));
    if (has("dicespeed")) DiceSpeed::set_enabled(on("dicespeed"));

    if (has("namenumber")) lucky::cfg_set("features.host.show_last_spin", on("namenumber"));
    if (has("pingcheck")) {
        lucky::cfg_set("display.show_ping", on("pingcheck"));
        lucky::cfg_set("features.host.show_ping", on("pingcheck"));
    }
    if (has("realfake")) lucky::cfg_set("features.host.show_real_spin", on("realfake"));
    if (has("remespin")) lucky::cfg_set("features.host.show_reme_spin", on("remespin"));
    if (has("qemekontol")) lucky::cfg_set("features.host.show_qq_number", on("qemekontol"));
    if (has("evelyn")) lucky::cfg_set("features.host.show_leme_spin", on("evelyn"));
    lucky::cfg_save();

    spdlog::info("[Options] Saved");
    lucky::log("`2Options saved``.");
}

}
