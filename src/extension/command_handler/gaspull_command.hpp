#pragma once
#include "command_base.hpp"
#include <atomic>

namespace command {

// /gp, /gaspull - Auto pull when someone says gas/play in world chat (LuckyProxy)
class GasPullCommand : public CommandBase {
public:
    GasPullCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static bool is_enabled();
    static void on_console_message(const std::string& message);

private:
    static std::atomic<bool> s_enabled;
};

}
