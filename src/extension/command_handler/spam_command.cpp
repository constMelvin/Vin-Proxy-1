#include "spam_command.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/text_parse.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <chrono>
#include "../../utils/packet_utils.hpp"
#include <sstream>
#include <random>

namespace command {

std::string SpamCommand::s_spam_text = "HELLO";
int SpamCommand::s_spam_delay_ms = 4000;
bool SpamCommand::s_colored_text = false;
bool SpamCommand::s_auto_disable_on_pull = false;
std::atomic<bool> SpamCommand::s_spamming{false};
core::Core* SpamCommand::s_core = nullptr;
std::chrono::steady_clock::time_point SpamCommand::s_last_spam_time{};

SpamCommand::SpamCommand() : CommandBase(
    {"spam"},
    {},
    "Open Auto Spam settings dialog",
    0
) {}

std::unique_ptr<CommandBase> SpamCommand::clone() const {
    return std::make_unique<SpamCommand>();
}

void SpamCommand::set_core(core::Core* core) {
    s_core = core;
    if (s_core) {
        try { s_spam_text = s_core->get_config().get<std::string>("spam.text", "HELLO"); } catch (...) {}
        try {
            int d = s_core->get_config().get<int>("spam.delay_ms", 4000);
            s_spam_delay_ms = (d >= 1000) ? d : 4000;
        } catch (...) {
            s_spam_delay_ms = 4000;
        }
        try { s_colored_text = s_core->get_config().get<bool>("spam.colored_text", false); } catch (...) {}
        try { s_auto_disable_on_pull = s_core->get_config().get<bool>("spam.auto_disable_pull", false); } catch (...) {}
    }
}

void SpamCommand::toggle_spam() {
    if (s_spamming) {
        stop_spam();
    } else {
        s_spamming = true;
        int delay = s_spam_delay_ms;
        if (delay < 1000) delay = 4000;
        s_last_spam_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(delay);
        spdlog::info("Spam started with delay {}ms", delay);
    }
}

bool SpamCommand::is_spamming() {
    return s_spamming;
}

void SpamCommand::stop_spam() {
    s_spamming = false;
    spdlog::info("Spam stopped");
}

bool SpamCommand::is_auto_disable_on_pull() {
    return s_auto_disable_on_pull;
}

void SpamCommand::on_player_pulled() {
    if (s_auto_disable_on_pull && s_spamming) {
        stop_spam();
        if (s_core && s_core->get_server() && s_core->get_server()->get_player()) {
            utils::PacketUtils::send_chat_message(
                s_core->get_server()->get_player(),
                "  `4Auto-disabled spam because you pulled someone!"
            );
        }
        spdlog::info("SpamCommand: Auto-disabled spam due to pull action");
    }
}

void SpamCommand::send_spam_dialog(player::Player* player) {
    if (!s_core) {
        spdlog::error("SpamCommand::send_spam_dialog: No core set!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("SpamCommand::send_spam_dialog: Local player not connected!");
        return;
    }

    player = server->get_player();

    std::ostringstream dialog;
    dialog << "set_default_color|`o\n";
    dialog << "add_label_with_icon|big|Auto Spam Page|left|2480|\n";
    dialog << fmt::format("add_checkbox|auto_pull|`2Auto Disable When Pulled|{}|\n", s_auto_disable_on_pull ? 1 : 0);
    dialog << "add_textbox|`oAutomatically Disables Spam When you Pull someone.|left|\n";
    dialog << fmt::format("add_checkbox|c_text|`2Enable `ccolored text|{}|\n", s_colored_text ? 1 : 0);
    dialog << "add_textbox|`oIf Colored Text is enabled, leave the text without ` colors.|left|\n";
    dialog << fmt::format("add_text_input|spam_msg|`2Spam Text: |{}|50|\n", s_spam_text);
    dialog << fmt::format("add_text_input|delay_msg|`2Delay (ms): |{}|5|\n", s_spam_delay_ms);
    dialog << "add_textbox|`o1000ms = 1 Second (Default: 4000ms)|left|\n";
    dialog << "add_textbox|`oWrite // to Enable/Disable Spam.|left|\n";
    dialog << "end_dialog|spam_dialog|Cancel|Apply|\n";

    std::string dialog_str = dialog.str();
    packet::Variant var{};
    var.add("OnDialogRequest");
    var.add(dialog_str);

    std::vector<std::byte> ext = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = -1;
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext.data(), ext.size());
    player->send_packet(bs.get_data(), 0);

    spdlog::info("SpamCommand: Sent Auto Spam Page dialog to player");
}

void SpamCommand::handle_dialog_return(player::Player* player, const std::string& button_clicked, const TextParse& tp) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (server && server->get_player()) {
        player = server->get_player();
    }
    if (!player) return;

    auto clean_str = [](std::string s) -> std::string {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        if (!s.empty()) {
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        }
        return s;
    };

    std::string btn = clean_str(button_clicked);
    spdlog::info("SpamCommand: Dialog response button: '{}' (cleaned: '{}')", button_clicked, btn);

    if (btn == "Cancel" || (!btn.empty() && btn != "Apply" && btn != "Set")) {
        spdlog::info("SpamCommand: Dialog cancelled");
        return;
    }

    std::string auto_pull = clean_str(tp.get("auto_pull"));
    s_auto_disable_on_pull = (auto_pull == "1" || auto_pull == "true" || auto_pull == "on");

    std::string c_text = clean_str(tp.get("c_text"));
    s_colored_text = (c_text == "1" || c_text == "true" || c_text == "on");

    std::string msg = clean_str(tp.get("spam_msg"));
    if (!msg.empty()) {
        s_spam_text = msg;
    }

    std::string delay_str = clean_str(tp.get("delay_msg"));
    if (!delay_str.empty()) {
        try {
            int d = std::stoi(delay_str);
            if (d < 1000) d = 1000;
            s_spam_delay_ms = d;
        } catch (...) {
            // keep current delay
        }
    }

    // Persist to configuration
    try {
        s_core->get_config().set("spam.text", s_spam_text);
        s_core->get_config().set("spam.delay_ms", s_spam_delay_ms);
        s_core->get_config().set("spam.colored_text", s_colored_text);
        s_core->get_config().set("spam.auto_disable_pull", s_auto_disable_on_pull);
    } catch (const std::exception& e) {
        spdlog::warn("SpamCommand: Failed to save config: {}", e.what());
    }

    utils::PacketUtils::send_chat_message(
        player,
        fmt::format(" `9Spam settings updated! Text: '`w{}' `9Delay: `2{}ms `9Colored: {} `9Auto-Pull: {}",
            s_spam_text, s_spam_delay_ms,
            s_colored_text ? "`2ON``" : "`4OFF``",
            s_auto_disable_on_pull ? "`2ON``" : "`4OFF``"
        )
    );
}

void SpamCommand::tick() {
    if (!s_spamming.load()) return;
    if (!s_core) return;

    auto* client = s_core->get_client();
    if (!client || !client->get_player() || !client->get_player()->is_connected()) {
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player() || !server->get_player()->is_connected()) {
        return;
    }

    int delay = s_spam_delay_ms;
    if (delay < 1000) delay = 4000;

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_spam_time).count() < delay) {
        return;
    }
    s_last_spam_time = now;

    std::string text_to_send = s_spam_text;
    if (s_colored_text) {
        // Strip any leading color codes so they don't override the random color
        while (text_to_send.size() >= 2 && text_to_send[0] == '`') {
            text_to_send.erase(0, 2);
        }

        // Curated bright and light colors that stand out cleanly in chat
        static const std::string colored_text_array[] = {
            "`1", // Light Cyan
            "`2", // Green
            "`3", // Light Blue
            "`8", // Orange
            "`9", // Yellow
            "`^", // Light Green
            "`p", // Pink
            "`o", // Dreamsicle
            "`$", // Pale Yellow
            "`#", // Bright Purple
            "`@", // Peach / Light Pink
            "`!", // Bright Cyan
            "`r"  // Pale Green
        };
        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_int_distribution<size_t> color_dist(0, std::size(colored_text_array) - 1);

        size_t color_idx = color_dist(rng);
        text_to_send = colored_text_array[color_idx] + text_to_send;
    }

    std::string spam_packet = fmt::format("action|input\ntext|{}\n", text_to_send);

    ByteStream<std::uint16_t> byte_stream{};
    byte_stream.write(packet::NET_MESSAGE_GENERIC_TEXT);
    byte_stream.write(spam_packet, false);

    client->get_player()->send_packet(byte_stream.get_data(), 0);
    spdlog::debug("Spam sent: {}", text_to_send);
}

void SpamCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) {
        return;
    }

    if (!s_core) {
        spdlog::error("SpamCommand: No core set!");
        return;
    }

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("SpamCommand: No server player!");
        return;
    }

    send_spam_dialog(server->get_player());
}

// -------------------------------------------------------------------------
// SpamToggleCommand: handles // and /
// -------------------------------------------------------------------------
SpamToggleCommand::SpamToggleCommand() : CommandBase(
    {"/", "//"},
    {},
    "Toggle spam ON/OFF",
    0
) {}

std::unique_ptr<CommandBase> SpamToggleCommand::clone() const {
    return std::make_unique<SpamToggleCommand>();
}

void SpamToggleCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) {
        return;
    }

    if (!SpamCommand::s_core) {
        spdlog::error("SpamToggleCommand: No core set!");
        return;
    }

    auto* server = SpamCommand::s_core->get_server();
    if (!server || !server->get_player()) {
        spdlog::error("SpamToggleCommand: No server player!");
        return;
    }

    SpamCommand::toggle_spam();

    if (SpamCommand::is_spamming()) {
        utils::PacketUtils::send_chat_message(server->get_player(), " `9Spam is `2ON");
    } else {
        utils::PacketUtils::send_chat_message(server->get_player(), " `9Spam is `4OFF");
    }
}

// -------------------------------------------------------------------------
// SpamDelayCommand
// -------------------------------------------------------------------------
SpamDelayCommand::SpamDelayCommand() : CommandBase(
    {"spamdelay", "sd"},
    {},
    "Set spam delay in milliseconds (usage: /spamdelay <ms>)",
    1
) {}

std::unique_ptr<CommandBase> SpamDelayCommand::clone() const {
    return std::make_unique<SpamDelayCommand>();
}

void SpamDelayCommand::execute(client::Client* client, const std::vector<std::string>& args) {
    if (!client || !client->get_player()) {
        return;
    }

    if (!SpamCommand::s_core) {
        return;
    }

    auto* server = SpamCommand::s_core->get_server();
    if (!server || !server->get_player()) {
        return;
    }

    int delay = 4000;
    try {
        delay = std::stoi(args[1]);
        if (delay < 1000) delay = 1000;
    } catch (...) {
        delay = 4000;
    }

    SpamCommand::s_spam_delay_ms = delay;
    try {
        SpamCommand::s_core->get_config().set("spam.delay_ms", delay);
    } catch (...) {}

    utils::PacketUtils::send_chat_message(
        server->get_player(),
        fmt::format(" `9Spam delay set to `2{} `9ms", delay)
    );
    
    spdlog::info("Spam delay set to: {} ms", delay);
}

} 
