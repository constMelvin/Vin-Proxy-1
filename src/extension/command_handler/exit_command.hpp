#pragma once
#include "command_base.hpp"

namespace command {

// /exit - Leave the current world, same as the game's Exit button (action|quit_to_exit)
class ExitCommand : public CommandBase {
public:
    ExitCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

}
