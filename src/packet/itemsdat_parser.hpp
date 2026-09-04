#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace packet {

// Patches items.dat binary from SEND_ITEM_DATABASE_DATA:
// - Sets fxFlags (anim_type) = 33 (spin arm) for all hand items (bodyPart==5) with fxFlags==0
// - Returns patched copy of data, ready to forward to client
// - Size is unchanged (uint32 fxFlags already in binary, just changed value)
std::vector<std::byte> patch_items_dat(const std::vector<std::byte>& data);

} // namespace packet
