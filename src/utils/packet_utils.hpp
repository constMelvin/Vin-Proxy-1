#pragma once
#include "../packet/message/chat.hpp"
#include "../packet/packet_helper.hpp"
#include "../packet/packet_variant.hpp"
#include "../packet/packet_types.hpp"
#include "../utils/byte_stream.hpp"
#include "../player/player.hpp"
#include "../core/logger.hpp"
#include <thread>
#include <chrono>

namespace utils {
class PacketUtils {
public:
    static void send_chat_message(player::Player* player, const std::string& message, bool add_prefix = true) {
        if (!player || !player->is_connected()) {
            spdlog::error("Cannot send message: player is null or not connected.");
            return;
        }

        packet::message::Log message_packet{};
        if (add_prefix) {
            message_packet.msg = "`^[VinProxy Premium]`o " + message;
        } else {
            message_packet.msg = message;
        }

        if (!packet::PacketHelper::send(message_packet, *player)) {
            spdlog::error("Failed to send chat message packet to player.");
        } else {
            spdlog::debug("Chat message sent: {}", message);
        }
    }

    static void send_delayed_chat_message(player::Player* player, const std::string& message, int delay_ms = 100) {
        std::thread([player, message, delay_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            send_chat_message(player, message, false);
        }).detach();
    }

    static void dismiss_active_dialog(player::Player* player) {
        if (!player || !player->is_connected()) return;
        try {
            packet::Variant variant{};
            variant.add("OnDialogRequest");
            variant.add("");
            std::vector<std::byte> ext_data = variant.serialize();
            packet::GameUpdatePacket game_packet{};
            game_packet.type = packet::PACKET_CALL_FUNCTION;
            game_packet.net_id = -1;
            game_packet.flags.extended = 1;
            game_packet.data_size = static_cast<uint32_t>(ext_data.size());
            ByteStream<std::uint16_t> byte_stream{};
            byte_stream.write(packet::NET_MESSAGE_GAME_PACKET);
            byte_stream.write(game_packet);
            byte_stream.write_data(ext_data.data(), ext_data.size());
            player->send_packet(byte_stream.get_data(), 0);
            spdlog::debug("PacketUtils: Sent dismiss_active_dialog packet");
        } catch (const std::exception& e) {
            spdlog::error("PacketUtils: Failed to dismiss active dialog: {}", e.what());
        }
    }
};
}