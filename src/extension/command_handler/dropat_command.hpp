#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <memory>
#include <atomic>
#include <cstdint>

namespace command {

struct DropTargetPos {
    int x = -1;
    int y = -1;
    bool is_set() const { return x >= 0 && y >= 0; }
    void clear() { x = -1; y = -1; }
};

class DposCommand : public CommandBase {
public:
    DposCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static DropTargetPos get_pos();
    static void set_pos(int x, int y);
    static void clear_pos();

private:
    static core::Core* s_core;
    static DropTargetPos s_target_pos;
};

class DropAtCommand : public CommandBase {
public:
    DropAtCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static bool is_running();

    static void set_local_facing_left(bool left);
    static bool get_local_facing_left();

private:
    static core::Core* s_core;
    static std::atomic<bool> s_running;
    static std::atomic<std::uint64_t> s_generation;
    static std::atomic<bool> s_local_facing_left;

    static void run_drop_at(client::Client* client_ptr, int target_x, int target_y, int max_quantity, std::uint64_t generation, bool face_left);
};

} // namespace command
