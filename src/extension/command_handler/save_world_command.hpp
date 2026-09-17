#pragma once

#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>

namespace command {

class SaveWorldCommand : public CommandBase {
public:
    SaveWorldCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static std::string get_save_world();
    static void set_save_world(const std::string& world_name);

private:
    static core::Core* s_core;
    static std::string s_save_world;
};

} // namespace command
