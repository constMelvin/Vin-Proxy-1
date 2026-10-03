#pragma once
#include "command_base.hpp"
#include <atomic>

namespace command {

// /arroz, /roz [amount] - Drop Arroz Con Pollo (LuckyProxy)
class ArrozCommand : public CommandBase {
public:
    ArrozCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    // Hides the drop dialog while the proxy confirms it (true = hide)
    static bool on_dialog_request(const std::string& dialog);

private:
    static std::atomic<bool> s_dropping;
};

}
