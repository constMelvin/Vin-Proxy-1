#include "balance_command.hpp"
#include "lucky_common.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../utils/dialog.hpp"
#include "../../utils/inventory_manager.hpp"

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

// LuckyProxy /balance (WL / DL / BGL counts), shown in a dialog instead of the console
void BalanceCommand::execute(client::Client*, const std::vector<std::string>& /*args*/) {
    int wl = 0, dl = 0, bgl = 0, total_wl = 0;
    utils::InventoryManager::get_instance().get_balance(wl, dl, bgl, total_wl);

    using Size = utils::Dialog::Size;
    utils::Dialog dlg;
    dlg.label_with_icon("`2Your Balance``", 7188)
       .spacer()
       .label_with_icon("`2WL : `w" + std::to_string(wl), 242, Size::Small)
       .label_with_icon("`2DL : `w" + std::to_string(dl), 1796, Size::Small)
       .label_with_icon("`2BGL : `w" + std::to_string(bgl), 7188, Size::Small)
       .spacer()
       .textbox("`9Total: `w" + std::to_string(total_wl) + " `9World Locks")
       .quick_exit()
       .end_dialog("balance_dialog", "", "Close");
    dlg.send(lucky::local_out());
}

} // namespace command
