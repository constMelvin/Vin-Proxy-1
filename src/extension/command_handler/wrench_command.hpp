#pragma once

#include "command_base.hpp"
#include "../../core/core.hpp"
#include "../../utils/text_parse.hpp"

namespace command {

class WrenchCommand : public CommandBase {
public:
    WrenchCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static void apply_dialog_settings(const TextParse& tp);

    // Active wrench mode ("pull"/"kick"/"ban"), empty when wrench mode is off
    static std::string current_mode();
    // Mode restored by /wm and changed by /ws while wrench mode is off
    static void remember_mode(const std::string& mode);
    static void apply_mode(const std::string& mode);
    static const std::string& saved_mode();
    static player::Player* local_out();
    static bool get_flag(const std::string& key);
    static void set_flag(const std::string& key, bool value);

private:
    static core::Core* s_core;
    static std::string s_saved_mode;
    static void send_settings_dialog(player::Player* out);
};

// /wm - Enable & Disable Wrench Mode (LuckyProxy)
class WrenchModeToggleCommand : public CommandBase {
public:
    WrenchModeToggleCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
};

// /ws [mode] - Set wrench mode (LuckyProxy)
class WrenchSetModeCommand : public CommandBase {
public:
    WrenchSetModeCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
};

// /rkick - Enable & Disable Right Click Kick Mode
class RightClickKickCommand : public CommandBase {
public:
    RightClickKickCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    static bool is_enabled();
};

// /setmsg <text> - Message used by /wrenchmsg and /wrenchspam (LuckyProxy)
class SetMsgCommand : public CommandBase {
public:
    SetMsgCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    static std::string get_message();
};

}

