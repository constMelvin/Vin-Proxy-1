#pragma once
#include "command_base.hpp"

namespace command {

class BalanceCommand : public CommandBase {
public:
    BalanceCommand();
    ~BalanceCommand() override = default;

    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

} // namespace command
