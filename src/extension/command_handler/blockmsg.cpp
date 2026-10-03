#include "blockmsg.hpp"
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> BlockMsg::s_enabled{false};

bool BlockMsg::is_enabled() { return s_enabled.load(); }
void BlockMsg::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[BlockMsg] {}", enabled ? "enabled" : "disabled");
}

bool BlockMsg::should_hide(const std::string& message) {
    if (!s_enabled.load() || message.find("[MSG]") == std::string::npos) return false;
    spdlog::debug("[BlockMsg] Hidden private message: {}", message);
    return true;
}

}
