#pragma once
#include "command_base.hpp"

namespace command {

// /leme - Enable & disable leme spin tag on roulette spins (LuckyProxy)
// Saved as features.host.show_leme_spin; the tag itself is added by the parser
class LemeCommand : public CommandBase {
public:
    LemeCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

}
