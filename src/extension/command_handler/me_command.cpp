#include "me_command.hpp"
#include "lucky_common.hpp"
#include <spdlog/spdlog.h>

namespace command {

std::atomic<bool> MeCommand::s_enabled{false};

MeCommand::MeCommand() : CommandBase({"me"}, {}, "Toggle /me effect on your chat", 0) {}
std::unique_ptr<CommandBase> MeCommand::clone() const { return std::make_unique<MeCommand>(*this); }

bool MeCommand::is_enabled() { return s_enabled.load(); }

void MeCommand::execute(client::Client*, const std::vector<std::string>& args) {
    // "/me <text>" is the normal game emote; only a bare "/me" toggles the mode
    if (args.size() > 1) {
        std::string text;
        for (size_t i = 1; i < args.size(); ++i) {
            if (!text.empty()) text += ' ';
            text += args[i];
        }
        lucky::send_server_input("/me " + text);
        return;
    }
    const bool enabled = !s_enabled.load();
    s_enabled.store(enabled);
    spdlog::info("[Me] /me mode {}", enabled ? "enabled" : "disabled");
    lucky::send_overlay(enabled ? "`2/me -> Enabled" : "`4/me -> Disabled");
}

bool MeCommand::on_chat_input(const std::string& text) {
    if (!s_enabled.load() || text.empty() || text[0] == '/') return false;
    lucky::send_server_input("/me " + text);
    return true;
}

}
