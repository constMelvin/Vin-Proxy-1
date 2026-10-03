#include "gaspull_command.hpp"
#include "lucky_common.hpp"
#include "../../utils/player_tracker.hpp"
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> GasPullCommand::s_enabled{false};

GasPullCommand::GasPullCommand() : CommandBase({"gp", "gaspull"}, {}, "Auto pull when someone says gas/play", 0) {}
std::unique_ptr<CommandBase> GasPullCommand::clone() const { return std::make_unique<GasPullCommand>(*this); }

bool GasPullCommand::is_enabled() { return s_enabled.load(); }

void GasPullCommand::execute(client::Client*, const std::vector<std::string>&) {
    const bool enabled = !s_enabled.load();
    s_enabled.store(enabled);
    spdlog::info("[GasPull] {}", enabled ? "enabled" : "disabled");
    lucky::log(enabled ? "`2[GP`2]`w: `wGas Pull is `2ON" : "`2[GP`2]`w: `wGas Pull is `4OFF");
}

// LuckyProxy: world chat looks like "[W]_ `6<`wName``>`` text"
void GasPullCommand::on_console_message(const std::string& message) {
    if (!s_enabled.load()) return;
    static const std::string kChatHead = "[W]_ `6<`w";
    const size_t head = message.find(kChatHead);
    if (head == std::string::npos) return;
    const size_t name_start = head + kChatHead.size();
    const size_t name_end = message.find("``>``", name_start);
    if (name_end == std::string::npos) return;

    const std::string name = message.substr(name_start, name_end - name_start);
    const std::string said = lucky::lower_no_codes(message.substr(name_end));
    const auto local = utils::PlayerTracker::get_instance().get_local_player();
    if (name.empty() || name == local.name) return;
    if (said.find("gas") != std::string::npos || said.find("play") != std::string::npos) {
        spdlog::info("[GasPull] Pulling {} (said gas/play)", name);
        lucky::send_server_input("/pull " + name);
    }
}

}
