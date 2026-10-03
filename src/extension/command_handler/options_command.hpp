#pragma once
#include "command_base.hpp"

namespace command {

// /options - LuckyProxy options page; every checkbox drives the matching VinProxy feature
class OptionsCommand : public CommandBase {
public:
    OptionsCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static void show_dialog();
    // Checkboxes + Mod Detect Settings button, shared by /options and the /proxy Options tab
    static std::string options_content();
    // "options_page" dialog_return (raw = full dialog_return text)
    static void handle_dialog(const std::string& raw);
};

}
