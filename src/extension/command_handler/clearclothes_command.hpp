#pragma once
#include "command_base.hpp"
#include <string>
#include <vector>

namespace player {
class Player;
}

namespace command {

class ClearClothesCommand : public CommandBase {
public:
    ClearClothesCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void clear_all_items();
    static void execute_clear(client::Client* client = nullptr, player::Player* client_player = nullptr);
};

} 
