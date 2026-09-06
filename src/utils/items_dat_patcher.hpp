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

        patched_hash_ = compute_proton_hash(current_data.data(), current_data.size());
        server_hash_ = patched_hash_;
        spdlog::info("[ItemsDatPatcher] Clean authentic items.dat active! Hash: {}", patched_hash_);
        return true;
    }

private:
    inline static uint32_t patched_hash_ = 0;
    inline static uint32_t server_hash_ = 822489052; // Default fallback from GT 5.55
};

} // namespace utils
