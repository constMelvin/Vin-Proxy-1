#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace player { class Player; }

// OnSpawn commands ported from LuckyProxy events.cpp:
// /autopull, /pullauto, /autopulltile, /apt, /sapt, /pulltile (/pt), /aban (/autoban), /auto
namespace command {

// Shared state + packet hooks for all OnSpawn commands
class OnSpawnManager {
public:
    static void set_core(core::Core* core);

    // OnSpawn of a non-local player (raw_name = spawn "name" field, colour codes included)
    static void on_player_spawn(uint32_t net_id, const std::string& raw_name);
    // Server PACKET_STATE for any player (pixel position)
    static void on_player_moved(uint32_t net_id, float x, float y);
    // Client punched a tile (used by /apt selection)
    static void on_tile_punched(int tile_x, int tile_y);
    // Left the world (OnRequestWorldSelectMenu)
    static void on_world_exit();

    // dialog_return handlers
    static void handle_autopull_dialog(const std::string& button_clicked);
    static void handle_autopulltile_dialog(const std::string& button_clicked);
    static void handle_auto_dialog(const std::string& button_clicked, const std::string& raw);
    static void handle_pullby_name_dialog(const std::string& raw);
    static void handle_banby_name_dialog(const std::string& raw);

    static bool is_autopull_enabled();
};

class AutoPullCommand : public CommandBase {
public:
    AutoPullCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static bool is_enabled() { return OnSpawnManager::is_autopull_enabled(); }
};

class PullAutoCommand : public CommandBase {
public:
    PullAutoCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

class AutoPullTileCommand : public CommandBase {
public:
    AutoPullTileCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

class AptCommand : public CommandBase {
public:
    AptCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

class SaptCommand : public CommandBase {
public:
    SaptCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

class PullTileCommand : public CommandBase {
public:
    PullTileCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

class AutoBanCommand : public CommandBase {
public:
    AutoBanCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

class AutoOptionsCommand : public CommandBase {
public:
    AutoOptionsCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
};

}
