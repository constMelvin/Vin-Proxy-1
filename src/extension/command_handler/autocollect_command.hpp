#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <atomic>
#include <cstdint>

namespace command {

class AutoCollectCommand : public CommandBase {
public:
    AutoCollectCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);

    // /collect dialog page: enable/disable + collect range in tiles
    static void show_dialog(player::Player* player);
    static void handle_dialog_response(player::Player* player, const std::string& raw);
    static void set_enabled(player::Player* player, bool enable);
    static int  get_range_tiles() { return s_range_tiles.load(); }
    static constexpr int MIN_RANGE_TILES = 1;
    static constexpr int MAX_RANGE_TILES = 200;
    static constexpr int DEFAULT_RANGE_TILES = 100;   // 3200 px, the previous fixed radius

    static void run_autocollect(std::uint64_t generation);
    static void send_collect_packet(player::Player* to_server, uint32_t uid, float x, float y);
    static void stop();
    static bool is_enabled() { return s_running.load(); }

    
    
    
    static void notify_item_drop(float x, float y);
    static core::Core* s_core;

private:
    static std::atomic<bool> s_running;
    static std::atomic<std::uint64_t> s_generation;
    static std::atomic<int> s_range_tiles;
};

} 
