#include "exit_command.hpp"
#include "lucky_common.hpp"
#include "../../utils/world_manager.hpp"
#include <spdlog/spdlog.h>

namespace command {

ExitCommand::ExitCommand() : CommandBase({"exit"}, {}, "Leave the current world", 0) {}
std::unique_ptr<CommandBase> ExitCommand::clone() const { return std::make_unique<ExitCommand>(*this); }

void ExitCommand::execute(client::Client*, const std::vector<std::string>&) {
    const std::string world = utils::WorldManager::get_instance().get_world_name();
    spdlog::info("[Exit] Leaving world {}", world.empty() ? std::string("(unknown)") : world);
    lucky::log("`9Leaving The World Now");
    lucky::send_server_game_message("action|quit_to_exit");
}

}
