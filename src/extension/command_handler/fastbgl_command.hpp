#pragma once
#include "command_base.hpp"
#include <atomic>
#include <string>

namespace command {

// /fastbgl [amount] - Fast BGL (Storage) (LuckyProxy fastbgl): opening the World Lock Storage
// converts Diamond Locks into Blue Gem Locks without the storage dialogs
class FastBglCommand : public CommandBase {
public:
    FastBglCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static bool is_enabled();
    static void set_enabled(bool enabled);
    // World Lock Storage dialog; true = converted and hidden
    static bool on_dialog_request(const std::string& dialog);

private:
    static void convert(int target_bgl);
    static std::atomic<bool> s_enabled;
    static std::atomic<int> s_count;
};

}
