#include "hidelevel.hpp"
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> HideLevel::s_enabled{false};

bool HideLevel::is_enabled() { return s_enabled.load(); }
void HideLevel::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[HideLevel] {}", enabled ? "enabled" : "disabled");
}

bool HideLevel::should_hide(const std::string& text) {
    if (!s_enabled.load() || text.find("You need to be Level") == std::string::npos) return false;
    spdlog::debug("[HideLevel] Hidden: {}", text);
    return true;
}

}
