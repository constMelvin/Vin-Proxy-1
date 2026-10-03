#include "leme_command.hpp"
#include "lucky_common.hpp"

namespace command {

LemeCommand::LemeCommand() : CommandBase({"leme"}, {}, "Enable & disable leme spin", 0) {}
std::unique_ptr<CommandBase> LemeCommand::clone() const { return std::make_unique<LemeCommand>(*this); }

void LemeCommand::execute(client::Client*, const std::vector<std::string>&) {
    const bool enabled = !lucky::cfg_flag("features.host.show_leme_spin", false);
    lucky::cfg_set("features.host.show_leme_spin", enabled);
    lucky::cfg_save();
    lucky::log(enabled ? "`2[LEME`2]`w: `wLeme mode `2Enabled" : "`2[LEME`2]`w: `wLEME mode `4Disabled");
}

}
