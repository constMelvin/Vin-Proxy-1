#include "autobgl_command.hpp"
#include "lucky_common.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

namespace {

constexpr const char* kPhoneNumber = "53785";

bool tile_coords(const std::string& dialog, int& x, int& y) {
    x = lucky::extract_int(dialog, "embed_data|tilex|");
    y = lucky::extract_int(dialog, "embed_data|tiley|");
    return x >= 0 && y >= 0;
}

// Name of the add_button whose label contains `label` (case-insensitive)
std::string find_button_name(const std::string& dialog, const std::string& label) {
    const std::string wanted = lucky::lower_no_codes(label);
    const std::string needle = "add_button|";
    size_t pos = 0;
    while ((pos = dialog.find(needle, pos)) != std::string::npos) {
        const size_t line_end = dialog.find('\n', pos);
        const size_t name_start = pos + needle.size();
        const size_t name_end = dialog.find('|', name_start);
        if (name_end != std::string::npos && (line_end == std::string::npos || name_end < line_end)) {
            size_t label_end = dialog.find('|', name_end + 1);
            if (label_end == std::string::npos || (line_end != std::string::npos && label_end > line_end))
                label_end = line_end == std::string::npos ? dialog.size() : line_end;
            if (lucky::lower_no_codes(dialog.substr(name_end + 1, label_end - name_end - 1)).find(wanted) != std::string::npos)
                return dialog.substr(name_start, name_end - name_start);
        }
        pos = line_end == std::string::npos ? dialog.size() : line_end + 1;
    }
    return {};
}

// "Hang Up" given as a button label on the end_dialog|phonecall| line
std::string find_end_dialog_label(const std::string& dialog, const std::string& label) {
    const std::string needle = "end_dialog|phonecall|";
    const size_t pos = dialog.find(needle);
    if (pos == std::string::npos) return {};
    const size_t line_end = dialog.find('\n', pos);
    const std::string line = dialog.substr(pos, (line_end == std::string::npos ? dialog.size() : line_end) - pos);
    const std::string wanted = lucky::lower_no_codes(label);
    size_t start = 0;
    int index = 0;
    while (start <= line.size()) {
        size_t sep = line.find('|', start);
        if (sep == std::string::npos) sep = line.size();
        const std::string part = line.substr(start, sep - start);
        if (index >= 2 && lucky::lower_no_codes(part).find(wanted) != std::string::npos) return part;
        start = sep + 1;
        ++index;
    }
    return {};
}

} // namespace

std::atomic<bool> AutoBglCommand::s_enabled{false};
std::atomic<bool> AutoBglCommand::s_hangup_pending{false};

AutoBglCommand::AutoBglCommand() : CommandBase({"autobgl", "bgl"}, {}, "Auto change BGL when wrenching a phone", 0) {}
std::unique_ptr<CommandBase> AutoBglCommand::clone() const { return std::make_unique<AutoBglCommand>(*this); }

bool AutoBglCommand::is_enabled() { return s_enabled.load(); }
void AutoBglCommand::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[AutoBGL] {}", enabled ? "enabled" : "disabled");
}

void AutoBglCommand::execute(client::Client*, const std::vector<std::string>&) {
    const bool enabled = !s_enabled.load();
    set_enabled(enabled);
    lucky::log(enabled ? "`2[AUTOBGL`2]`w: `wAuto Change BGL is now `2ON" : "`2[AUTOBGL`2]`w: `wAuto Change BGL is now `4OFF");
}

bool AutoBglCommand::try_hang_up(const std::string& dialog) {
    if (dialog.find("phonecall") == std::string::npos) return false;
    if (lucky::lower_no_codes(dialog).find("hang up") == std::string::npos) return false;
    int x = 0, y = 0;
    if (!tile_coords(dialog, x, y)) return false;
    std::string button = find_button_name(dialog, "Hang Up");
    if (button.empty()) button = find_end_dialog_label(dialog, "Hang Up");
    if (button.empty()) {
        spdlog::warn("[AutoBGL] Phone dialog has no Hang Up button, leaving it open");
        return false;
    }
    spdlog::info("[AutoBGL] Hanging up the phone at ({}, {})", x, y);
    lucky::send_server_text(fmt::format(
        "action|dialog_return\ndialog_name|phonecall\ntilex|{}|\ntiley|{}|\nbuttonClicked|{}", x, y, button));
    return true;
}

bool AutoBglCommand::on_dialog_request(const std::string& dialog) {
    if (s_hangup_pending.load() && try_hang_up(dialog)) {
        s_hangup_pending.store(false);
        return true;
    }
    if (!s_enabled.load() || s_hangup_pending.load()) return false;

    int x = 0, y = 0;
    if (dialog.find("Dial a number to call somebody in Growtopia.") != std::string::npos && tile_coords(dialog, x, y)) {
        spdlog::info("[AutoBGL] Dialing {} on the phone at ({}, {})", kPhoneNumber, x, y);
        lucky::send_server_text(fmt::format(
            "action|dialog_return\ndialog_name|phonecall\ntilex|{}|\ntiley|{}|\nnum|-2|\ndial|{}", x, y, kPhoneNumber));
        return true;
    }
    if (dialog.find(std::string("embed_data|num|") + kPhoneNumber) != std::string::npos && tile_coords(dialog, x, y)) {
        lucky::send_server_text(fmt::format(
            "action|dialog_return\ndialog_name|phonecall\ntilex|{}|\ntiley|{}|\nnum|{}|\nbuttonClicked|chc5", x, y, kPhoneNumber));
        return true;
    }
    if (dialog.find("Excellent! I'm happy to sell you a Blue Gem Lock in exchange for 100 Diamond Lock") != std::string::npos &&
        tile_coords(dialog, x, y)) {
        lucky::send_server_text(fmt::format(
            "action|dialog_return\ndialog_name|phonecall\ntilex|{}|\ntiley|{}|\nnum|-34|\nbuttonClicked|chc0", x, y));
        spdlog::info("[AutoBGL] Buying a Blue Gem Lock for 100 Diamond Locks");
        s_hangup_pending.store(true);
        return true;
    }
    return false;
}

}
