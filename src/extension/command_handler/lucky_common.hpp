#pragma once
#include "../../core/core.hpp"
#include <cstdint>
#include <string>

namespace player { class Player; }
namespace extension::item_finder { class ItemDatabase; }

// Shared helpers for the commands/features ported from LuckyProxy
// (options, showoc, me, leme, gaspull, logs, arroz, balance and the options-page features)
namespace command::lucky {

void set_core(core::Core* core);
core::Core* get_core();
void set_item_database(extension::item_finder::ItemDatabase* database);

// Local game client (build dialogs with utils::Dialog and send them to local_out())
player::Player* local_out();
void log(const std::string& msg);
void send_overlay(const std::string& text);

// Game server
void send_server_text(const std::string& text);            // NET_MESSAGE_GENERIC_TEXT ("action|respawn", ...)
void send_server_input(const std::string& text);           // chat / slash command
void send_server_game_message(const std::string& text);    // NET_MESSAGE_GAME_MESSAGE ("action|quit_to_exit", ...)
// Game client
void send_client_game_message(const std::string& text);    // NET_MESSAGE_GAME_MESSAGE ("action|logon_fail", ...)

// Items
std::string item_name(uint16_t id);
bool is_entrance(uint16_t fg);

// Config flags
bool cfg_flag(const std::string& key, bool def);
void cfg_set(const std::string& key, bool value);
void cfg_save();

// Text
std::string lower_no_codes(const std::string& text);
std::string time_hhmm();
std::string world_upper();
int extract_int(const std::string& content, const std::string& key);

}
