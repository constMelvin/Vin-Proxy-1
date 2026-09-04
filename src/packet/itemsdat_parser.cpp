#include "itemsdat_parser.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace packet {

static bool can_read(const uint8_t* ptr, const uint8_t* end, size_t n) {
    return ptr + n <= end;
}

static uint8_t  r8 (const uint8_t*& p) { return *p++; }
static int8_t   r8s(const uint8_t*& p) { return static_cast<int8_t>(*p++); }
static uint16_t r16(const uint8_t*& p) { uint16_t v; memcpy(&v,p,2); p+=2; return v; }
static uint32_t r32(const uint8_t*& p) { uint32_t v; memcpy(&v,p,4); p+=4; return v; }

static void skip(const uint8_t*& p, size_t n) { p += n; }
static void skip_str(const uint8_t*& p) { uint16_t len = r16(p); p += len; }

static bool parse_item_fast(
    const uint8_t*  base,
    const uint8_t*& ptr,
    const uint8_t*  end,
    uint16_t        version,
    uint8_t&        out_body_part,
    size_t&         out_fx_flags_offset)
{
    out_body_part       = 0;
    out_fx_flags_offset = 0;

#define CHK(n) if (!can_read(ptr, end, (n))) return false;

    CHK(4); r32(ptr);               // id  uint32
    CHK(2); r16(ptr);               // flags  uint16
    CHK(1); r8(ptr);                // type  uint8
    CHK(1); r8(ptr);                // material  uint8

    // name  uint16-len + bytes (XOR-encrypted, skip)
    CHK(2); { uint16_t len = r16(ptr); CHK(len); skip(ptr, len); }

    CHK(2); skip_str(ptr);          // textureFile  string
    CHK(4); r32(ptr);               // textureHash  uint32
    CHK(1); r8(ptr);                // visualEffect  uint8
    CHK(4); r32(ptr);               // cookingTime  int32
    CHK(1); r8(ptr);                // textureX  uint8
    CHK(1); r8(ptr);                // textureY  uint8
    CHK(1); r8(ptr);                // spreadType  uint8
    CHK(1); r8s(ptr);               // layer  int8
    CHK(1); r8(ptr);                // collisionType  uint8
    CHK(1); r8(ptr);                // hp  uint8
    CHK(4); r32(ptr);               // restoreTime  int32
    CHK(1); out_body_part = r8(ptr);// bodyPart  uint8  <-- clothing slot
    CHK(2); r16(ptr);               // rarity  int16
    CHK(1); r8(ptr);                // maxCanHold  uint8
    CHK(2); skip_str(ptr);          // extraFile  string
    CHK(4); r32(ptr);               // extraFileHash  uint32
    CHK(4); r32(ptr);               // animMS  int32

    // v4+: pet strings
    if (version > 3) {
        CHK(2); skip_str(ptr);      // petName
        CHK(2); skip_str(ptr);      // petSubName
        CHK(2); skip_str(ptr);      // petEndName
        CHK(2); skip_str(ptr);      // petPowerName
    }

    CHK(4); skip(ptr, 4);           // seedBg/Fg, treeBg/Fg  uint8 x4
    CHK(8); skip(ptr, 8);           // seedBgColor, seedFgColor  uint32 x2
    CHK(4); skip(ptr, 4);           // seed1, seed2  uint16 x2
    CHK(4); r32(ptr);               // growTime  uint32

    // v7+: fxFlags(uint32) + multiAnim1(string)  <-- fxFlags = anim_type
    if (version > 6) {
        CHK(4);
        out_fx_flags_offset = static_cast<size_t>(ptr - base);
        r32(ptr);                   // fxFlags - remember offset for patching
        CHK(2); skip_str(ptr);      // multiAnim1
    }

    // v8+: overlayTexture(str), multiAnim2(str), dualAnimLayer(int32 x2)
    if (version > 7) {
        CHK(2); skip_str(ptr);
        CHK(2); skip_str(ptr);
        CHK(8); skip(ptr, 8);
    }

    // v9+: flags2(uint32) + clientData[15](int32) = 64 bytes
    if (version > 8) { CHK(64); skip(ptr, 64); }

    // v10+: tileRange + pileSize  uint32 x2
    if (version > 9) { CHK(8); skip(ptr, 8); }

    // v11+: punchParameters  string
    if (version > 10) { CHK(2); skip_str(ptr); }

    // v12+: extraSlotCounter(uint32) + extraSlotBodyParts[9](uint8) = 13 bytes
    if (version > 11) { CHK(13); skip(ptr, 13); }

    // v13+: lightSourceRange  uint32
    if (version > 12) { CHK(4); skip(ptr, 4); }

    // v14+: variantVersionItem  uint32
    if (version > 13) { CHK(4); skip(ptr, 4); }

    // v15+: chairEnabled(uint8) + chairPlayerOffset(int32 x2) + chairArmPos(int32 x2)
    //       + chairArmOffset(int32 x2) + chairArmTexture(string)  = 25 + str
    if (version > 14) { CHK(25); skip(ptr, 25); CHK(2); skip_str(ptr); }

    // v16+: configName  string
    if (version > 15) { CHK(2); skip_str(ptr); }

    // v17+: otherPlayerHitParticle  int32
    if (version > 16) { CHK(4); skip(ptr, 4); }

    // v18+: configNameHash  uint32
    if (version > 17) { CHK(4); skip(ptr, 4); }

    // v19+: randomSpriteEnabled(uint8) + randomSpriteOffsetMod(int32) + randomSpriteChance(float)
    if (version > 18) { CHK(9); skip(ptr, 9); }

    // v20+: hiddenPartsFlags  uint8
    if (version > 19) { CHK(1); skip(ptr, 1); }

    // v21+: canTransform  uint8
    if (version > 20) { CHK(1); skip(ptr, 1); }

    // v22+: description  string
    if (version > 21) { CHK(2); skip_str(ptr); }

    // v23+: seed1, seed2  uint16 x2
    if (version > 22) { CHK(4); skip(ptr, 4); }

    // v24+: slipperyType  uint8
    if (version > 23) { CHK(1); skip(ptr, 1); }

    // v25+: unknownString(str) + unknownUint32
    if (version > 24) { CHK(2); skip_str(ptr); CHK(4); skip(ptr, 4); }

    // v26+: unknownByte
    if (version > 25) { CHK(1); skip(ptr, 1); }

#undef CHK
    return true;
}

std::vector<std::byte> patch_items_dat(const std::vector<std::byte>& data) {
    if (data.size() < 6) return data;

    std::vector<std::byte> patched = data;
    uint8_t* base = reinterpret_cast<uint8_t*>(patched.data());
    const uint8_t* end = base + patched.size();

    const uint8_t* rptr = base;
    uint16_t version  = r16(rptr);
    uint32_t item_cnt = r32(rptr);

    spdlog::info("[ItemsDat] Patching v{} ({} items, {} bytes)", version, item_cnt, data.size());

    if (version < 7) {
        spdlog::info("[ItemsDat] Version {} has no fxFlags, nothing to patch", version);
        return patched;
    }

    uint32_t patched_count = 0;

    for (uint32_t i = 0; i < item_cnt && rptr < end; ++i) {
        uint8_t body_part      = 0;
        size_t  fx_offset      = 0;

        if (!parse_item_fast(base, rptr, end, version, body_part, fx_offset)) {
            spdlog::warn("[ItemsDat] Failed at item index {}, stopping", i);
            break;
        }

        // Patch hand items (bodyPart==5) that have fxFlags==0 → set to 33 (spin-arm swing)
        if (body_part == 5 && fx_offset != 0) {
            uint32_t fx = 0;
            memcpy(&fx, base + fx_offset, 4);
            if (fx == 0) {
                uint32_t new_fx = 33;
                memcpy(base + fx_offset, &new_fx, 4);
                patched_count++;
            }
        }
    }

    spdlog::info("[ItemsDat] Patched {} hand items with fxFlags=33", patched_count);
    return patched;
}

} // namespace packet
