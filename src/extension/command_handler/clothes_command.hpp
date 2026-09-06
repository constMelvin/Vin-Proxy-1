#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <map>

namespace player {
class Player;
}

class TextParse;

namespace command {

extern std::unordered_map<int, int> g_clothing_slots;
extern std::unordered_set<uint32_t> g_all_visual_items;

class ClothesCommand : public CommandBase {
public:
    ClothesCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static core::Core* get_core();
    static void set_pending_item(int item_id, int clothing_type, int anim_type = -1);
    static bool toggle_item(int item_id, int clothing_type, int anim_type = -1);
    static bool is_equipped(int item_id);
    static void send_clothing_change(client::Client* client = nullptr);

    // Dialog and Visual Set Management
    static void send_clothes_dialog(player::Player* player = nullptr);
    static void handle_dialog_response(player::Player* player, const std::string& button_clicked, const TextParse& tp);
    static bool save_visual_set(const std::string& name);
    static bool load_visual_set(const std::string& name, client::Client* client = nullptr, player::Player* player = nullptr);
    static bool delete_visual_set(const std::string& name);
    static std::map<std::string, std::unordered_map<int, int>> get_saved_sets();
};

} 
