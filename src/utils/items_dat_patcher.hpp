#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <spdlog/spdlog.h>

namespace utils {

class ItemsDatPatcher {
public:
    static uint32_t compute_proton_hash(const uint8_t* data, size_t length) {
        uint32_t hash = 0x55555555;
        for (size_t i = 0; i < length; ++i) {
            hash = ((hash >> 27) + (hash << 5) + data[i]) & 0xFFFFFFFF;
        }
        return hash;
    }

    static std::string get_cache_dir() {
        char* localappdata = nullptr;
        size_t len = 0;
        if (_dupenv_s(&localappdata, &len, "LOCALAPPDATA") == 0 && localappdata != nullptr) {
            std::string dir = std::string(localappdata) + "\\Growtopia\\cache";
            free(localappdata);
            return dir;
        }
        return "";
    }

    static uint32_t get_patched_hash() {
        return patched_hash_;
    }

    static uint32_t get_server_hash() {
        return server_hash_;
    }

    static void set_server_hash(uint32_t hash) {
        if (hash != 0) {
            server_hash_ = hash;
        }
    }

    static bool ensure_patched() {
        std::string cache_dir = get_cache_dir();
        if (cache_dir.empty()) {
            spdlog::warn("[ItemsDatPatcher] Could not find LOCALAPPDATA directory");
            return false;
        }

        std::string items_path = cache_dir + "\\items.dat";
        std::string backup_path = cache_dir + "\\items.dat.original";

        if (!std::filesystem::exists(items_path)) {
            spdlog::warn("[ItemsDatPatcher] items.dat not found at {}", items_path);
            return false;
        }

        std::vector<uint8_t> current_data;
        {
            std::ifstream file(items_path, std::ios::binary);
            if (!file.is_open()) {
                spdlog::warn("[ItemsDatPatcher] Could not open items.dat for reading");
                return false;
            }
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            current_data.resize(size);
            file.read(reinterpret_cast<char*>(current_data.data()), size);
        }

        // Check if original backup exists, if not create it
        if (!std::filesystem::exists(backup_path)) {
            std::ofstream bkp(backup_path, std::ios::binary);
            if (bkp.is_open()) {
                bkp.write(reinterpret_cast<const char*>(current_data.data()), current_data.size());
                spdlog::info("[ItemsDatPatcher] Created backup at {}", backup_path);
            }
        }

        std::vector<uint8_t> base_data = current_data;
        if (std::filesystem::exists(backup_path)) {
            std::ifstream bkp_file(backup_path, std::ios::binary);
            if (bkp_file.is_open()) {
                bkp_file.seekg(0, std::ios::end);
                size_t b_size = bkp_file.tellg();
                bkp_file.seekg(0, std::ios::beg);
                base_data.resize(b_size);
                bkp_file.read(reinterpret_cast<char*>(base_data.data()), b_size);
                server_hash_ = compute_proton_hash(base_data.data(), base_data.size());
                spdlog::info("[ItemsDatPatcher] Server items.dat hash: {}", server_hash_);
            }
        }

        // Check if current items.dat is already patched with Fist UP_SPINARM2 animation
        bool already_patched = is_already_patched(current_data);
        if (already_patched) {
            patched_hash_ = compute_proton_hash(current_data.data(), current_data.size());
            spdlog::info("[ItemsDatPatcher] items.dat is already patched! Hash: {}", patched_hash_);
            return true;
        }

        // Patch the data from clean base_data
        std::vector<uint8_t> patched_data;
        if (!patch_data(base_data, patched_data)) {
            spdlog::error("[ItemsDatPatcher] Failed to patch items.dat");
            return false;
        }

        // Write patched file
        {
            std::ofstream out(items_path, std::ios::binary);
            if (!out.is_open()) {
                spdlog::error("[ItemsDatPatcher] Could not open items.dat for writing");
                return false;
            }
            out.write(reinterpret_cast<const char*>(patched_data.data()), patched_data.size());
        }

        patched_hash_ = compute_proton_hash(patched_data.data(), patched_data.size());
        spdlog::info("[ItemsDatPatcher] Successfully patched items.dat! Patched hash: {}", patched_hash_);
        return true;
    }

private:
    inline static uint32_t patched_hash_ = 0;
    inline static uint32_t server_hash_ = 822489052; // Default fallback from GT 5.55

    static std::string decrypt_item_name(const std::string& enc_name, uint32_t item_id) {
        static const std::string key = "PBG892FXX982ABC*";
        std::string dec = enc_name;
        for (size_t i = 0; i < enc_name.size(); ++i) {
            uint8_t k = static_cast<uint8_t>(key[(i + item_id) % key.size()]);
            dec[i] = static_cast<char>(static_cast<uint8_t>(enc_name[i]) ^ k);
        }
        return dec;
    }

    static bool is_already_patched(const std::vector<uint8_t>& data) {
        // Check for our V2 patch signature in items.dat
        const std::string needle = "VIN_WEAPON_PUNCH_V2";
        auto it = std::search(data.begin(), data.end(), needle.begin(), needle.end());
        return it != data.end();
    }

    static bool patch_data(const std::vector<uint8_t>& orig, std::vector<uint8_t>& out) {
        if (orig.size() < 6) return false;

        size_t offset = 0;
        uint16_t version = *reinterpret_cast<const uint16_t*>(&orig[offset]); offset += 2;
        uint32_t item_count = *reinterpret_cast<const uint32_t*>(&orig[offset]); offset += 4;

        out.clear();
        out.reserve(orig.size() + 200000);

        // Header
        const uint8_t* v_ptr = reinterpret_cast<const uint8_t*>(&version);
        out.insert(out.end(), v_ptr, v_ptr + 2);
        const uint8_t* ic_ptr = reinterpret_cast<const uint8_t*>(&item_count);
        out.insert(out.end(), ic_ptr, ic_ptr + 4);

        auto copy_bytes = [&](size_t n) {
            if (offset + n > orig.size()) return false;
            out.insert(out.end(), orig.begin() + offset, orig.begin() + offset + n);
            offset += n;
            return true;
        };

        auto copy_str = [&]() {
            if (offset + 2 > orig.size()) return false;
            uint16_t l = *reinterpret_cast<const uint16_t*>(&orig[offset]);
            if (offset + 2 + l > orig.size()) return false;
            out.insert(out.end(), orig.begin() + offset, orig.begin() + offset + 2 + l);
            offset += 2 + l;
            return true;
        };

        auto read_str_val = [&]() -> std::string {
            if (offset + 2 > orig.size()) return "";
            uint16_t l = *reinterpret_cast<const uint16_t*>(&orig[offset]);
            offset += 2;
            if (offset + l > orig.size()) return "";
            std::string s(reinterpret_cast<const char*>(&orig[offset]), l);
            offset += l;
            return s;
        };

        auto write_str_val = [&](const std::string& s) {
            uint16_t l = static_cast<uint16_t>(s.size());
            const uint8_t* lp = reinterpret_cast<const uint8_t*>(&l);
            out.insert(out.end(), lp, lp + 2);
            out.insert(out.end(), s.begin(), s.end());
        };

        int patched_count = 0;

        for (uint32_t i = 0; i < item_count; ++i) {
            if (offset + 4 > orig.size()) break;
            uint32_t item_id = *reinterpret_cast<const uint32_t*>(&orig[offset]);
            uint8_t item_type = (offset + 6 < orig.size()) ? orig[offset + 6] : 0;
            
            if (!copy_bytes(4 + 2 + 1 + 1)) return false; // id, flags, type, material

            if (offset + 2 > orig.size()) return false;
            uint16_t name_len = *reinterpret_cast<const uint16_t*>(&orig[offset]);
            std::string name_str = "";
            if (offset + 2 + name_len <= orig.size()) {
                name_str = std::string(reinterpret_cast<const char*>(&orig[offset + 2]), name_len);
            }
            if (!copy_bytes(2 + name_len)) return false;

            if (!copy_str()) return false; // textureFile
            if (!copy_bytes(4 + 1 + 4 + 1 + 1 + 1 + 1 + 1 + 1 + 4)) return false;

            if (offset >= orig.size()) return false;
            uint8_t body_part = orig[offset];
            if (!copy_bytes(1 + 2 + 1)) return false; // bodyPart, rarity, maxCanHold

            if (!copy_str()) return false; // extraFile
            if (!copy_bytes(4 + 4)) return false; // extraFileHash, animMS

            if (version > 3) {
                for (int s = 0; s < 4; ++s) {
                    if (!copy_str()) return false;
                }
            }

            if (!copy_bytes(4 + 8 + 4 + 4)) return false; // seed/tree, growTime

            if (version > 6) {
                if (!copy_bytes(4)) return false;
                if (!copy_str()) return false; // multiAnim1
            }

            if (version > 7) {
                if (!copy_str()) return false; // overlayTexture
                if (!copy_str()) return false; // multiAnim2
                if (!copy_bytes(8)) return false; // dualAnimLayer
            }

            if (version > 8) { if (!copy_bytes(64)) return false; }
            if (version > 9) { if (!copy_bytes(8)) return false; }

            // v11: punchParameters
            std::string punch_str = read_str_val();
            if (item_id == 18) {
                // Item 18 is Fist! By default, Growtopia plays the bare fist punch arm (UP_ARM1).
                // We patch Item 18 to UP_SPINARM2 with slash audio & heart particles and V2 signature:
                std::string fist_punch = "ONPUNCHSTART;VIN_WEAPON_PUNCH_V2;op_particle2:190;op_params:0,20;op_audio:audio/slash.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2";
                write_str_val(fist_punch);
                patched_count++;
            } else if (item_type == 20 && body_part == 5 && punch_str.empty()) {
                std::string dec_name = decrypt_item_name(name_str, item_id);
                std::string nl = dec_name;
                std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);

                std::string new_punch = "";
                if (nl.find("heatbow") != std::string::npos || item_id == 4136) {
                    new_punch = "ONPUNCHSTART;op_particle2:217;op_audio:audio/arrow_thwip.wav;UPDATEPUNCH;up_face:4";
                } else if (nl.find("golden heartbow") != std::string::npos || item_id == 1464) {
                    new_punch = "ONPUNCHSTART;op_particle2:191;op_audio:audio/arrow_thwip.wav;UPDATEPUNCH;up_face:4";
                } else if (nl.find("heartbow") != std::string::npos || item_id == 366) {
                    new_punch = "ONPUNCHSTART;op_particle2:190;op_audio:audio/arrow_thwip.wav;UPDATEPUNCH;up_face:4";
                } else if (nl.find("bow") != std::string::npos || nl.find("crossbow") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/arrow_thwip.wav;UPDATEPUNCH;up_face:4";
                } else if (nl.find("gun") != std::string::npos || nl.find("blaster") != std::string::npos || 
                           nl.find("rifle") != std::string::npos || nl.find("pistol") != std::string::npos ||
                           nl.find("shotgun") != std::string::npos || nl.find("cannon") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_particle1:285;op_punch_pattern:1;op_params:5,-29;op_audio:audio/laser.wav;UPDATEPUNCH;up_face:4;UP_ARM1Angle:-60;UP_ARM2Angle:60";
                } else if (nl.find("sword") != std::string::npos || nl.find("saber") != std::string::npos || 
                           nl.find("katana") != std::string::npos || nl.find("blade") != std::string::npos || 
                           nl.find("scimitar") != std::string::npos || nl.find("dagger") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/slash.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2";
                } else if (nl.find("pickaxe") != std::string::npos || nl.find("axe") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/slash.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2";
                } else if (nl.find("hammer") != std::string::npos || nl.find("mallet") != std::string::npos || nl.find("mace") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/hammer_rumble.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2;RFA_HIDEITEM";
                } else if (nl.find("spear") != std::string::npos || nl.find("lance") != std::string::npos || 
                           nl.find("trident") != std::string::npos || nl.find("javelin") != std::string::npos ||
                           nl.find("scythe") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/buster.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2;RFA_HIDEITEM";
                } else if (nl.find("staff") != std::string::npos || nl.find("wand") != std::string::npos || nl.find("rod") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/swoosh.wav;UPDATEPUNCH;up_face:4;up_arm2:330";
                } else if (nl.find("glove") != std::string::npos || nl.find("fist") != std::string::npos || nl.find("claw") != std::string::npos) {
                    new_punch = "ONPUNCHSTART;op_audio:audio/fireball.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2;RFA_HIDEITEM";
                } else {
                    new_punch = "ONPUNCHSTART;op_audio:audio/slash.wav;UPDATEPUNCH;up_face:4;UP_SPINARM2";
                }

                write_str_val(new_punch);
                patched_count++;
            } else {
                write_str_val(punch_str);
            }

            if (version > 11) { if (!copy_bytes(13)) return false; }
            if (version > 12) { if (!copy_bytes(4)) return false; }
            if (version > 13) { if (!copy_bytes(4)) return false; }
            if (version > 14) {
                if (!copy_bytes(25)) return false;
                if (!copy_str()) return false;
            }
            if (version > 15) { if (!copy_str()) return false; }
            if (version > 16) { if (!copy_bytes(4)) return false; }
            if (version > 17) { if (!copy_bytes(4)) return false; }
            if (version > 18) { if (!copy_bytes(9)) return false; }
            if (version > 19) { if (!copy_bytes(1)) return false; }
            if (version > 20) { if (!copy_bytes(1)) return false; }
            if (version > 21) { if (!copy_str()) return false; }
            if (version > 22) { if (!copy_bytes(4)) return false; }
            if (version > 23) { if (!copy_bytes(1)) return false; }
            if (version > 24) {
                if (!copy_str()) return false;
                if (!copy_bytes(4)) return false;
            }
            if (version > 25) { if (!copy_bytes(1)) return false; }
        }

        // Copy any remaining trailing bytes
        if (offset < orig.size()) {
            out.insert(out.end(), orig.begin() + offset, orig.end());
        }

        spdlog::info("[ItemsDatPatcher] Injected custom punch animations for {} hand items", patched_count);
        return true;
    }
};

} // namespace utils
