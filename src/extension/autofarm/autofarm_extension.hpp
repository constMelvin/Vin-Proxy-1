#pragma once
#include "../extension.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../packet/packet_variant.hpp"
#include "../command_handler/autofarm_common.hpp"
#include "../command_handler/autoharvest_command.hpp"
#include "../command_handler/autoplant_command.hpp"
#include "../command_handler/autofarm_command.hpp"
#include "../command_handler/autofish_command.hpp"
#include <string>

namespace extension::autofarm {

// Feeds the auto-farming commands: tracks facing direction, forwards the
// "You caught" console message (fish) and "You were pulled by" overlay (farm),
// and stops every automation when the world changes or the connection drops.
class AutoFarmExtension final : public IExtension {
    core::Core* core_;

public:
    PROVIDE_EXT_UID(0x41465258);

    explicit AutoFarmExtension(core::Core* core) : core_{ core } {}
    ~AutoFarmExtension() override = default;

    void init() override {
        core_->get_event_dispatcher().appendListener(
            core::EventType::Packet,
            [this](const core::EventPacket& event) { on_packet(event); });
        core_->get_event_dispatcher().appendListener(
            core::EventType::Disconnection,
            [](const core::EventDisconnection&) { stop_all(); });
    }

    void free() override { delete this; }

private:
    static void stop_all() {
        command::AutoHarvestCommand::stop();
        command::AutoPlantCommand::stop();
        command::AutoFarmCommand::stop();
        command::AutoFishCommand::stop();
    }

    void on_packet(const core::EventPacket& event) {
        const auto& gp = event.get_packet();

        if (event.from == core::EventFrom::FromClient && gp.type == packet::PACKET_STATE) {
            const auto& ext = event.get_ext_data();
            const packet::TankUpdatePacket* tank = ext.size() >= sizeof(packet::TankUpdatePacket)
                ? reinterpret_cast<const packet::TankUpdatePacket*>(ext.data())
                : reinterpret_cast<const packet::TankUpdatePacket*>(&gp);
            command::autofarm::set_facing_left((tank->flags & packet::PACKET_FLAG_ROTATE_LEFT) != 0);
            return;
        }

        if (event.from != core::EventFrom::FromServer) return;

        if (gp.type == packet::PACKET_SEND_MAP_DATA) {
            // entering a world: previous targets are stale
            stop_all();
            return;
        }

        if (gp.type != packet::PACKET_CALL_FUNCTION) return;
        try {
            packet::Variant v{};
            if (!v.deserialize(event.get_ext_data())) return;
            auto params = v.get_variants();
            if (params.size() < 2) return;
            const std::string fn = std::get<std::string>(params[0]);
            const std::string msg = std::get<std::string>(params[1]);
            if (fn == "OnConsoleMessage")
                command::AutoFishCommand::on_console_message(msg);
            else if (fn == "OnTextOverlay")
                command::AutoFarmCommand::on_overlay_message(msg);
        } catch (...) {}
    }
};

} // namespace extension::autofarm
