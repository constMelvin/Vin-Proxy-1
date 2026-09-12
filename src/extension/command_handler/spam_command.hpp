#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include "../../utils/text_parse.hpp"
#include <string>
#include <atomic>
#include <chrono>

namespace command {

class SpamCommand : public CommandBase {
public:
    SpamCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);
    static void toggle_spam();
    static bool is_spamming();
    static void stop_spam();
    static void tick();

    static void send_spam_dialog(player::Player* player = nullptr);
    static void handle_dialog_return(player::Player* player, const std::string& button_clicked, const TextParse& tp);

    static bool is_auto_disable_on_pull();
    static void on_player_pulled();

    static std::string s_spam_text;
    static int s_spam_delay_ms;
    static bool s_colored_text;
    static bool s_auto_disable_on_pull;
    static std::atomic<bool> s_spamming;
    static core::Core* s_core;

private:
    static std::chrono::steady_clock::time_point s_last_spam_time;
};

class SpamToggleCommand : public CommandBase {
public:
    SpamToggleCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
};

class SpamDelayCommand : public CommandBase {
public:
    SpamDelayCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
};

} // namespace command
