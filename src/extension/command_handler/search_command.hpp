#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace player {
class Player;
}

namespace extension::item_finder {
struct ItemInfo;
}

class TextParse;

namespace command {

class SearchCommand : public CommandBase {
public:
    SearchCommand();
    ~SearchCommand() override = default;

    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;

    static void set_core(core::Core* core);
    static core::Core* get_core();

    static void show_search_dialog(player::Player* player, const std::string& query, int page = 0);
    static void show_item_details_dialog(player::Player* player, const extension::item_finder::ItemInfo* item, const std::string& return_query, int return_page);
    static void handle_dialog_return(player::Player* player, const std::string& button_clicked, const TextParse& tp);
};

} // namespace command
