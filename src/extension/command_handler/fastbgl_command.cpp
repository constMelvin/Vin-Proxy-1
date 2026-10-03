#include "fastbgl_command.hpp"
#include "lucky_common.hpp"
#include "../../utils/inventory_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <thread>

namespace command {

namespace {
constexpr uint16_t kDiamondLock = 1796;
constexpr int kDlPerBgl = 100;
}

std::atomic<bool> FastBglCommand::s_enabled{false};
std::atomic<int> FastBglCommand::s_count{1};

FastBglCommand::FastBglCommand() : CommandBase({"fastbgl"}, {"[amount]"}, "Fast BGL via World Lock Storage", 0) {}
std::unique_ptr<CommandBase> FastBglCommand::clone() const { return std::make_unique<FastBglCommand>(*this); }

bool FastBglCommand::is_enabled() { return s_enabled.load(); }
void FastBglCommand::set_enabled(bool enabled) {
    if (s_enabled.exchange(enabled) != enabled)
        spdlog::info("[FastBGL] {}", enabled ? "enabled" : "disabled");
}

void FastBglCommand::execute(client::Client*, const std::vector<std::string>& args) {
    // "/fastbgl <amount>" sets the BGLs per run and turns it on; bare "/fastbgl" toggles
    if (args.size() > 1) {
        try { s_count.store(std::max(1, std::stoi(args[1]))); } catch (...) {}
        set_enabled(true);
    } else {
        set_enabled(!s_enabled.load());
    }
    if (s_enabled.load())
        spdlog::info("[FastBGL] {} BGL per run", s_count.load());
    if (s_enabled.load())
        lucky::log(fmt::format("`2[FASTBGL]`w: Storage converter `2ON`w ({} BGL/run).", s_count.load()));
    else
        lucky::log("`4[FASTBGL]`w: Storage converter `4OFF`w.");
}

// LuckyProxy perform_fastbgl_storage (it divided by 10000 DL; 1 BGL is 100 DL)
void FastBglCommand::convert(int target_bgl) {
    const int dl = utils::InventoryManager::get_instance().get_item_count(kDiamondLock);
    const int possible = dl / kDlPerBgl;
    if (possible <= 0) {
        spdlog::warn("[FastBGL] Not enough Diamond Locks: have {}, need {}", dl, kDlPerBgl);
        lucky::log("`4[FASTBGL] Not enough DL to craft a BGL.");
        return;
    }
    const int make = std::min(std::max(1, target_bgl), possible);

    lucky::send_server_text("action|worldlock_storage_getpage\npageIndex|-1");
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    for (int i = 0; i < make; ++i) {
        lucky::send_server_text("action|worldlock_storage_modify_amount\namount|10000\ntype|2");
        lucky::send_server_text("action|worldlock_storage_getpage\npageIndex|0");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    spdlog::info("[FastBGL] Converted {} DL into {} BGL", make * kDlPerBgl, make);
    lucky::log(fmt::format("`2[FASTBGL] Crafted {} BGL from DL.", make));
}

bool FastBglCommand::on_dialog_request(const std::string& dialog) {
    if (!s_enabled.load()) return false;
    if (dialog.find("open_worldlock_storage") == std::string::npos &&
        dialog.find("show_world_lock_storage") == std::string::npos)
        return false;
    const int count = s_count.load();
    std::thread([count]() { convert(count); }).detach();
    return true;
}

}
