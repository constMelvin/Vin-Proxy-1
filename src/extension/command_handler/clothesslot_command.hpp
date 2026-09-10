#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>
#include <vector>

namespace command {

// /save1, /save2, /save3, /save4 — Save current visual clothes to slot
class SaveSlotCommand : public CommandBase {
public:
    explicit SaveSlotCommand(int slot);
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);

private:
    int slot_;
    static core::Core* s_core;
};

// /load1, /load2, /load3, /load4, /set1, /set2, /set3, /set4 — Load visual clothes from slot
class LoadSlotCommand : public CommandBase {
public:
    explicit LoadSlotCommand(int slot);
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);

private:
    int slot_;
    static core::Core* s_core;
};

} // namespace command
