#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>
#include <vector>

namespace command {

class ProxyCommand : public CommandBase {
public:
    ProxyCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);
    static void show_commands_gui(player::Player* player, core::Core* core, const std::string& filter = "", int active_tab = 0);
    static void handle_dialog_return(player::Player* player, const std::string& button_clicked, const std::string& search_query);
    static bool is_active();
    static void set_active(bool active);
};

class InfoCommand : public CommandBase {
public:
    InfoCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);
};

} 
