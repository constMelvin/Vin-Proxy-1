#include "fastbox.hpp"
#include "lucky_common.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> FastBox::s_enabled{false};

bool FastBox::is_enabled() { return s_enabled.load(); }
void FastBox::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[FastBox] {}", enabled ? "enabled" : "disabled");
}

bool FastBox::on_dialog_request(const std::string& dialog) {
    if (!s_enabled.load() || dialog.find("You have ") == std::string::npos) return false;
    if (dialog.find("add_label_with_icon|big|`wDonation Box``|left|1452") == std::string::npos &&
        dialog.find("add_label_with_icon|big|`wHoliday Gift Box``|left|1390") == std::string::npos)
        return false;

    const int x = lucky::extract_int(dialog, "embed_data|tilex|");
    const int y = lucky::extract_int(dialog, "embed_data|tiley|");
    if (x < 0 || y < 0) {
        spdlog::warn("[FastBox] Box dialog without tile coordinates, showing it normally");
        return false;
    }
    spdlog::info("[FastBox] Emptying box at ({}, {})", x, y);
    lucky::send_server_text(fmt::format(
        "action|dialog_return\ndialog_name|donation_box_edit\ntilex|{}|\ntiley|{}|\nbuttonClicked|clear", x, y));
    return true;
}

}
