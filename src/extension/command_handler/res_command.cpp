#include "res_command.hpp"
#include "lucky_common.hpp"
#include <spdlog/spdlog.h>

namespace command {

ResCommand::ResCommand() : CommandBase({"res", "respawn"}, {}, "Respawn right away", 0) {}
std::unique_ptr<CommandBase> ResCommand::clone() const { return std::make_unique<ResCommand>(*this); }

void ResCommand::execute(client::Client*, const std::vector<std::string>&) {
    spdlog::info("[Res] Respawning");
    lucky::log("`4Respawning..");
    lucky::send_server_text("action|respawn");
}

}
