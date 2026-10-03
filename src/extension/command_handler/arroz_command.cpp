#include "arroz_command.hpp"
#include "lucky_common.hpp"
#include "../../utils/inventory_manager.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

namespace command {

namespace {
constexpr uint16_t kArrozId = 4604;   // Arroz Con Pollo
}

std::atomic<bool> ArrozCommand::s_dropping{false};

ArrozCommand::ArrozCommand() : CommandBase({"arroz", "roz"}, {"[amount]"}, "Drop Arroz Con Pollo", 0) {}
std::unique_ptr<CommandBase> ArrozCommand::clone() const { return std::make_unique<ArrozCommand>(*this); }

void ArrozCommand::execute(client::Client*, const std::vector<std::string>& args) {
    int count = 1;
    if (args.size() > 1) {
        try { count = std::stoi(args[1]); } catch (...) {
            lucky::log("`4Usage: /arroz [amount]");
            return;
        }
    }
    if (count <= 0) count = 1;

    const int have = utils::InventoryManager::get_instance().get_item_count(kArrozId);
    if (have < count) {
        spdlog::warn("[Arroz] Not enough Arroz Con Pollo: have {}, asked {}", have, count);
        lucky::log("`9Dont have `#Arroz Con Pollo`9.");
        return;
    }
    if (s_dropping.exchange(true)) {
        spdlog::warn("[Arroz] A drop is already in progress, ignored");
        return;
    }
    spdlog::info("[Arroz] Dropping {} Arroz Con Pollo", count);

    std::thread([count]() {
        lucky::send_server_text(fmt::format("action|drop\n|itemID|{}", kArrozId));
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        lucky::send_server_text(fmt::format("action|dialog_return\ndialog_name|drop_item\nitemID|{}|\ncount|{}", kArrozId, count));
        lucky::log(fmt::format("`9Dropping `2{}`9 of Arroz Con Pollo.", count));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        s_dropping.store(false);
    }).detach();
}

bool ArrozCommand::on_dialog_request(const std::string& dialog) {
    return s_dropping.load() &&
           dialog.find("end_dialog|drop_item") != std::string::npos &&
           dialog.find(std::to_string(kArrozId)) != std::string::npos;
}

}
