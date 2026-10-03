#include "banfire_command.hpp"
#include "lucky_common.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../utils/packet_utils.hpp"
#include <spdlog/spdlog.h>

namespace command {

core::Core* BanFireCommand::s_core = nullptr;
bool BanFireCommand::s_enabled = false;

BanFireCommand::BanFireCommand() : CommandBase(
    {"banfire", "bf"},
    {},
    "Toggle auto-ban fire mode (bans players who use pocket lighter)",
    0
) {}

std::unique_ptr<CommandBase> BanFireCommand::clone() const {
    return std::make_unique<BanFireCommand>(*this);
}

void BanFireCommand::set_core(core::Core* core) {
    s_core = core;
}

bool BanFireCommand::is_enabled() {
    return s_enabled;
}

void BanFireCommand::toggle() {
    s_enabled = !s_enabled;
    spdlog::info("BanFireCommand: {}", s_enabled ? "ENABLED" : "DISABLED");
}

// LuckyProxy autobanfire: world ban whoever uses a Pocket Lighter / Eldritch Flame
bool BanFireCommand::on_talk_bubble(uint32_t net_id, const std::string& text) {
    if (!s_enabled || net_id == 0) return false;
    if (text.find("`7[```4MWAHAHAHA!! FIRE FIRE FIRE") == std::string::npos &&
        text.find("`7[```4BURN, PUNY MORTALS!") == std::string::npos)
        return false;
    spdlog::info("[BanFire] Banning netID {} for using a fire item", net_id);
    const std::string id = std::to_string(net_id);
    lucky::send_server_text("action|wrench\n|netid|" + id);
    lucky::send_server_text("action|dialog_return\ndialog_name|popup\nnetID|" + id + "|\nnetID|" + id + "|\nbuttonClicked|worldban");
    return true;
}

void BanFireCommand::execute(client::Client* client, const std::vector<std::string>& ) {
    if (!client || !client->get_player()) {
        spdlog::error("BanFireCommand: No client or player!");
        return;
    }

    if (!s_core) {
        spdlog::error("BanFireCommand: No core set!");
        return;
    }

    toggle();

    
    player::Player* target_player = nullptr;
    if (auto* server = s_core->get_server(); server && server->get_player()) {
        target_player = server->get_player();
    } else {
        target_player = client->get_player();
    }

    if (target_player) {
        if (s_enabled) {
            utils::PacketUtils::send_chat_message(target_player,
                "`2BanFire enabled:`o will auto-ban players who use pocket lighter");
        } else {
            utils::PacketUtils::send_chat_message(target_player,
                "`4BanFire disabled.`o Normal behavior restored");
        }
    }
}

} 
