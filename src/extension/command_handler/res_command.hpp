#pragma once
#include "command_base.hpp"

namespace command {

// /res, /respawn - Respawn right away (LuckyProxy /res)
class ResCommand : public CommandBase {
public:
    ResCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

}
