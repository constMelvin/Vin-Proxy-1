#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <atomic>
#include <cstdint>

namespace command {

class AutoPlantCommand : public CommandBase {
public:
    AutoPlantCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static core::Core* get_core() { return s_core; }
    static void handle_dialog_response(player::Player* player, const std::string& raw);
    static void toggle(player::Player* player);
    static void stop();
    static bool is_enabled() { return s_enabled.load(); }

private:
    static void show_dialog(player::Player* player);
    static void start();
    static void run(std::uint64_t generation);

    static core::Core* s_core;
    static std::atomic<bool> s_enabled;
    static std::atomic<std::uint64_t> s_generation;
    static std::atomic<int> s_seed_id;
    static std::atomic<int> s_target_block;
    static std::atomic<int> s_path_delay;
    static std::atomic<int> s_place_delay;
};

class PlantToggleCommand : public CommandBase {
public:
    PlantToggleCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
};

}
