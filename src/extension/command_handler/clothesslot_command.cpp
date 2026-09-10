#include "clothesslot_command.hpp"
#include "clothes_command.hpp"
#include "../../client/client.hpp"
#include "../../player/player.hpp"
#include "../../server/server.hpp"
#include "../../utils/packet_utils.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace command {

// ── SaveSlotCommand ─────────────────────────────────────────────────────────

core::Core* SaveSlotCommand::s_core = nullptr;

SaveSlotCommand::SaveSlotCommand(int slot)
    : CommandBase(
        { fmt::format("save{}", slot) },
        {},
        fmt::format("Save current visual clothes to Slot {}", slot),
        0
    )
    , slot_(slot)
{}

std::unique_ptr<CommandBase> SaveSlotCommand::clone() const {
    return std::make_unique<SaveSlotCommand>(*this);
}

void SaveSlotCommand::set_core(core::Core* core) {
    s_core = core;
}

void SaveSlotCommand::execute(client::Client* /*client*/, const std::vector<std::string>& /*args*/) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    player::Player* player = server->get_player();
    std::string slot_name = fmt::format("Slot {}", slot_);

    bool ok = ClothesCommand::save_visual_set(slot_name);
    if (ok) {
        auto sets = ClothesCommand::get_saved_sets();
        size_t count = sets[slot_name].size();
        utils::PacketUtils::send_chat_message(player,
            fmt::format("Successfully saved current visual set to `pSlot {} `2({} items)!``", slot_, count));
    } else {
        utils::PacketUtils::send_chat_message(player,
            fmt::format("No visual items equipped! Equip visual items first to save to Slot {}.``", slot_));
    }
}

// ── LoadSlotCommand ─────────────────────────────────────────────────────────

core::Core* LoadSlotCommand::s_core = nullptr;

LoadSlotCommand::LoadSlotCommand(int slot)
    : CommandBase(
        { fmt::format("load{}", slot), fmt::format("set{}", slot) },
        {},
        fmt::format("Load visual clothes from Slot {}", slot),
        0
    )
    , slot_(slot)
{}

std::unique_ptr<CommandBase> LoadSlotCommand::clone() const {
    return std::make_unique<LoadSlotCommand>(*this);
}

void LoadSlotCommand::set_core(core::Core* core) {
    s_core = core;
}

void LoadSlotCommand::execute(client::Client* client, const std::vector<std::string>& /*args*/) {
    if (!s_core) return;
    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    player::Player* player = server->get_player();
    std::string slot_name = fmt::format("Slot {}", slot_);

    bool ok = ClothesCommand::load_visual_set(slot_name, client, player);
    if (ok) {
        auto sets = ClothesCommand::get_saved_sets();
        size_t count = sets[slot_name].size();
        utils::PacketUtils::send_chat_message(player,
            fmt::format("Loaded `eSlot {} Set `2({} items)! Visual clothes enabled!``", slot_, count));
    } else {
        utils::PacketUtils::send_chat_message(player,
            fmt::format("Slot {} is empty! Equip visual clothes and save to Slot {} first.``", slot_, slot_));
    }
}

} // namespace command
