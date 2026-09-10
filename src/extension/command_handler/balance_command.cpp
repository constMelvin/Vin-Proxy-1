#include "balance_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/inventory_manager.hpp"
#include <fmt/format.h>

namespace command {

BalanceCommand::BalanceCommand() : CommandBase(
    {"bal", "balance"},
    {},
    "Show current World Lock, Diamond Lock, and BGL balance\nUsage: /bal or /balance",
    0
) {}

std::unique_ptr<CommandBase> BalanceCommand::clone() const {
    return std::make_unique<BalanceCommand>();
}

void BalanceCommand::execute(client::Client* client, const std::vector<std::string>& /*args*/) {
    if (!client || !client->get_player()) return;

    auto& inv_mgr = utils::InventoryManager::get_instance();
    int wl = 0, dl = 0, bgl = 0, total_wl = 0;
    inv_mgr.get_balance(wl, dl, bgl, total_wl);

    std::string balance_msg = fmt::format("Balance:`w [ `#{} ā `w| `#{} `!DL `w| `#{} `eBGL`w ]", total_wl, dl, bgl);
    utils::PacketUtils::send_chat_message(client->get_player(), balance_msg, false);
}

} // namespace command
