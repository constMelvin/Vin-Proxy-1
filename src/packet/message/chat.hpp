#pragma once
#include <fmt/format.h>

#include "../packet_types.hpp"
#include "../packet_helper.hpp"

namespace packet::message {
struct Log : NetMessage<NetMessageType::NET_MESSAGE_GAME_MESSAGE> {
    std::string msg;

    void write(ByteStream<std::uint16_t>& byte_stream)
    {
        std::string raw = fmt::format("action|log\nmsg|{}\n", msg);
        byte_stream.write_data(raw.c_str(), raw.size() + 1);
    }
};

struct Chat : NetMessage<NetMessageType::NET_MESSAGE_GAME_MESSAGE> {
    std::string message;

    void write(ByteStream<std::uint16_t>& byte_stream)
    {
        std::string raw = fmt::format("action|input\ntext|{}\n", message);
        byte_stream.write_data(raw.c_str(), raw.size() + 1);
    }
};
}