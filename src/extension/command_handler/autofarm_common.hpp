#pragma once
#include "../../core/core.hpp"
#include "../../player/player.hpp"
#include "../../utils/text_parse.hpp"
#include <cstdint>
#include <string>

namespace command::autofarm {

void send_dialog(player::Player* p, const std::string& body);
void send_console(player::Player* p, const std::string& msg);
bool in_world();
bool local_tile_pos(core::Core* core, int& tx, int& ty);
uint16_t tile_fg(int x, int y);
bool facing_left();
void set_facing_left(bool left);
void send_tile_action(core::Core* core, int32_t int_data, int32_t x, int32_t y);
void punch(core::Core* core, int x, int y);
// Punch the way the real client does (LuckyProxy place_tile "legit" mode): a state packet
// announcing a swing with the fist (item 18) at the tile, then the tile change carrying
// the player's own position. Works whatever item is currently selected in the hotbar.
void punch_legit(core::Core* core, int x, int y);
void place(core::Core* core, int item_id, int x, int y);
bool move_to(core::Core* core, int x, int y);
bool parse_int(const std::string& s, int& out);
bool checkbox_on(const TextParse& tp, const char* key);
std::string item_name(int id);

// Dialog text in the /spam page style: small grey description text.
// desc_text  - hugs the checkbox above it (negative margin, then a gap below)
// tiny_text  - plain small grey line, for under inputs / pickers
std::string desc_text(const std::string& text);
std::string tiny_text(const std::string& text);

} // namespace command::autofarm
