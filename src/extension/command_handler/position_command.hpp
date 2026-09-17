#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <memory>
#include <array>
#include <atomic>
#include <string>
#include <glm/glm.hpp>

namespace command {

struct Position {
    int x = -1;
    int y = -1;
    float px = -1.0f;
    float py = -1.0f;
    bool is_set() const { return x != -1 && y != -1; }
};

class PositionCommand : public CommandBase {
public:
    PositionCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);
    static Position get_position(int index); 
    static void set_position(int index, int x, int y, float px = -1.0f, float py = -1.0f);
    
    static bool is_spos1_waiting() { return s_spos1_wait.load(); }
    static bool is_spos2_waiting() { return s_spos2_wait.load(); }
    static void set_spos1_waiting(bool w) { s_spos1_wait.store(w); }
    static void set_spos2_waiting(bool w) { s_spos2_wait.store(w); }
    static void handle_punched_tile(int32_t tile_x, int32_t tile_y);

    static void highlight_tile(player::Player* player, int32_t tile_x, int32_t tile_y);
    static glm::vec2 get_back_pos() { return s_back_pos; }
    static void set_back_pos(const glm::vec2& p) { s_back_pos = p; }
    static int64_t get_prize() { return s_prize.load(); }
    static void set_prize(int64_t p) { s_prize.store(p); }

private:
    static core::Core* s_core;
    static std::array<Position, 5> s_positions; 
    static std::atomic<bool> s_spos1_wait;
    static std::atomic<bool> s_spos2_wait;
    static glm::vec2 s_back_pos;
    static std::atomic<int64_t> s_prize;
};

class SPosCommand : public CommandBase {
public:
    SPosCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

class CPosCommand : public CommandBase {
public:
    CPosCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

class CasinoTPCommand : public CommandBase {
public:
    CasinoTPCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

class WinCommand : public CommandBase {
public:
    WinCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

class TeleportPosCommand : public CommandBase {
public:
    TeleportPosCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);
    
private:
    static core::Core* s_core;
};

class BackCommand : public CommandBase {
public:
    BackCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;
    
    static void set_core(core::Core* core);
    static void note_join_request_target(const std::string& raw_name);
    static std::string get_current_world() { return s_current_world; }
    static std::string get_previous_world() { return s_previous_world; }
    
private:
    static core::Core* s_core;
    static std::string s_current_world;
    static std::string s_previous_world;
};

} // namespace command
