#pragma once
#include "command_base.hpp"
#include <atomic>
#include <string>

namespace command {

// /autobgl, /bgl - Fast Change BGL (LuckyProxy autobgl): wrench a phone and the proxy dials
// 53785, buys a Blue Gem Lock for 100 Diamond Locks and hangs up, without showing the dialogs
class AutoBglCommand : public CommandBase {
public:
    AutoBglCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static bool is_enabled();
    static void set_enabled(bool enabled);
    // Phone dialogs; true = answered by the proxy and hidden
    static bool on_dialog_request(const std::string& dialog);

private:
    static bool try_hang_up(const std::string& dialog);
    static std::atomic<bool> s_enabled;
    static std::atomic<bool> s_hangup_pending;
};

}
