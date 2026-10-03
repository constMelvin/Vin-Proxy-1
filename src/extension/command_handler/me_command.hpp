#pragma once
#include "command_base.hpp"
#include <atomic>

namespace command {

// /me - Toggle /me effect: normal chat is sent as "/me <text>" (LuckyProxy)
class MeCommand : public CommandBase {
public:
    MeCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static bool is_enabled();
    // Plain chat from the client; true = sent as /me instead
    static bool on_chat_input(const std::string& text);

private:
    static std::atomic<bool> s_enabled;
};

}
