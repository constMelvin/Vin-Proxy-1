#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <atomic>
#include <cstdint>

namespace command {

class AutoFarmCommand : public CommandBase {
public:
    AutoFarmCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static core::Core* get_core() { return s_core; }
    static void handle_dialog_response(player::Player* player, const std::string& raw);
    static void toggle(player::Player* player);
    static void on_overlay_message(const std::string& msg);
    static void stop();
    static bool is_enabled() { return s_enabled.load(); }

private:
    static std::string build_dialog(bool interval_error);
    static bool start(player::Player* player);
    static void run(std::uint64_t generation);

    static core::Core* s_core;
    static std::atomic<bool> s_enabled;
    static std::atomic<std::uint64_t> s_generation;
    static std::atomic<int> s_farm_item;
    static std::atomic<int> s_interval;
    static std::atomic<bool> s_disable_on_pull;
    static std::atomic<int> s_collect_wait;   // ms to pause after breaking so Auto Collect can pick up drops
};

class FarmToggleCommand : public CommandBase {
public:
    FarmToggleCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
};

}
