#pragma once

#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>

namespace command {

class ModDetectCommand : public CommandBase {
public:
    ModDetectCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static void handle_spawn_packet(const std::string& spawn_data);

    
    static bool is_enabled();
    static void toggle();

    // LuckyProxy "Mod Detect Settings" page (opened from /options) and the actions it enables
    static void show_settings_dialog();
    static void handle_settings_dialog(const std::string& raw);

private:
    static void run_mod_actions();
    static core::Core* s_core;
};

} 

