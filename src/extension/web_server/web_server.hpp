#pragma once
#include "../extension.hpp"

struct IWebServerExtension : extension::IExtension {
    PROVIDE_EXT_UID(0x153bd697);
    virtual std::string get_address() const = 0;
    virtual uint16_t get_port() const = 0;
};
