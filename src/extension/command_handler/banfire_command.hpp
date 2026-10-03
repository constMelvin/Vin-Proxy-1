#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <cstdint>
#include <string>

namespace command {

class BanFireCommand : public CommandBase {
public:
    BanFireCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static void set_core(core::Core* core);
    static bool is_enabled();
    static void toggle();
    // Talk bubble from the server; bans Pocket Lighter / Eldritch Flame users (true = hide bubble)
    static bool on_talk_bubble(uint32_t net_id, const std::string& text);

private:
    static core::Core* s_core;
    static bool s_enabled;
};

} 
