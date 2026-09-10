#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>
#include "../core/core.hpp"
#include "../server/server.hpp"
#include "../player/player.hpp"
#include "../packet/packet_types.hpp"
#include "../packet/tank_packet.hpp"
#include "byte_stream.hpp"
#include "player_tracker.hpp"
#include "visual_items_manager.hpp"

namespace command {
extern std::unordered_map<int, int> g_clothing_slots;
extern std::unordered_set<uint32_t> g_all_visual_items;
}

namespace utils {

struct InventoryItem {
    uint16_t id;
    uint32_t amount;
    uint8_t flags;
};

class InventoryManager {
public:
    static InventoryManager& get_instance() {
        static InventoryManager instance;
        return instance;
    }

    std::vector<std::byte> get_patched_inventory_data_unlocked(const std::unordered_map<int, int>& clothing_slots) const {
        if (raw_server_ext_data_.size() < 7) {
            return raw_server_ext_data_;
        }

        std::vector<std::byte> patched = raw_server_ext_data_;

        uint32_t inv_size = *reinterpret_cast<const uint32_t*>(&patched[1]);
        uint16_t item_count = *reinterpret_cast<const uint16_t*>(&patched[5]);

        size_t items_start = 7;
        size_t items_end = items_start + static_cast<size_t>(item_count) * 4;
        if (items_end > patched.size()) {
            return patched;
        }

        std::unordered_set<uint16_t> all_ids;
        for (uint32_t id : command::g_all_visual_items) {
            if (id > 0) all_ids.insert(static_cast<uint16_t>(id));
        }
        for (uint32_t id : utils::VisualItemsManager::get_instance().get_all_visual_items()) {
            if (id > 0) all_ids.insert(static_cast<uint16_t>(id));
        }
        for (const auto& [slot, item_id] : clothing_slots) {
            if (item_id > 0) all_ids.insert(static_cast<uint16_t>(item_id));
        }

        if (all_ids.empty()) {
            return patched;
        }

        // Uncheck real server clothing items whose slots are overridden by visual clothing
        auto base = PlayerTracker::get_instance().get_server_clothing();
        std::unordered_set<uint16_t> overridden_server_items;
        for (const auto& [slot, item_id] : clothing_slots) {
            if (item_id <= 0) continue;
            int s_item = 0;
            switch (slot) {
                case 0: s_item = base.hat; break;
                case 1: s_item = base.shirt; break;
                case 2: s_item = base.pants; break;
                case 3: s_item = base.shoes; break;
                case 4: s_item = base.face; break;
                case 5: s_item = base.hand; break;
                case 6: s_item = base.back; break;
                case 7: s_item = base.hair; break;
                case 8: s_item = base.neck; break;
                case 9: s_item = base.ances; break;
                default: break;
            }
            if (s_item > 0 && s_item != item_id) {
                overridden_server_items.insert(static_cast<uint16_t>(s_item));
            }
        }

        for (uint16_t i = 0; i < item_count; ++i) {
            size_t offset = items_start + static_cast<size_t>(i) * 4;
            uint16_t id = *reinterpret_cast<const uint16_t*>(&patched[offset]);
            if (overridden_server_items.count(id)) {
                patched[offset + 3] = std::byte{ 0 }; // Remove checkmark from real item!
            }
        }

        for (uint16_t vid : all_ids) {
            bool is_equipped = false;
            for (const auto& [slot, item_id] : clothing_slots) {
                if (item_id == static_cast<int>(vid)) {
                    is_equipped = true;
                    break;
                }
            }
            uint8_t target_flags = is_equipped ? 1 : 0;

            bool found = false;
            for (uint16_t i = 0; i < item_count; ++i) {
                size_t offset = items_start + static_cast<size_t>(i) * 4;
                uint16_t id = *reinterpret_cast<const uint16_t*>(&patched[offset]);
                if (id == vid) {
                    // Update flags: 1 if equipped, 0 if in backpack unequipped
                    patched[offset + 3] = static_cast<std::byte>(target_flags);
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Insert 4 bytes at items_end: [id: uint16, amount: uint8 (1), flags: uint8 (target_flags)]
                std::byte entry[4];
                *reinterpret_cast<uint16_t*>(&entry[0]) = vid;
                entry[2] = std::byte{ 1 }; // amount = 1 (stays in backpack)
                entry[3] = static_cast<std::byte>(target_flags); // 1 = equipped, 0 = in backpack

                patched.insert(patched.begin() + items_end, entry, entry + 4);
                item_count++;
                items_end += 4;

                // Update item_count in header
                *reinterpret_cast<uint16_t*>(&patched[5]) = item_count;
                if (item_count > inv_size) {
                    inv_size = item_count;
                    *reinterpret_cast<uint32_t*>(&patched[1]) = inv_size;
                }
            }
        }

        return patched;
    }

    void send_inventory(core::Core* core) {
        if (!core || !core->get_server() || !core->get_server()->get_player()) return;

        std::lock_guard<std::mutex> lock(mutex_);
        if (!has_raw_data_ || raw_server_ext_data_.size() < 7) {
            spdlog::warn("InventoryManager: No raw server inventory data available to patch!");
            return;
        }

        std::vector<std::byte> patched = get_patched_inventory_data_unlocked(command::g_clothing_slots);

        packet::GameUpdatePacket pkt = raw_server_game_packet_;
        pkt.data_size = static_cast<uint32_t>(patched.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(patched.data(), patched.size());

        core->get_server()->get_player()->send_packet(bs.get_data(), 0);
        spdlog::info("InventoryManager: Forwarded authentic patched inventory ({} bytes, original {} bytes) with visual items EQUIPPED",
                     patched.size(), raw_server_ext_data_.size());
    }

    void send_inventory(core::Core* core, uint32_t /*net_id*/, uint16_t /*visual_hand_id*/ = 0) {
        send_inventory(core);
    }

    void parse_inventory(const std::vector<std::byte>& data, const packet::GameUpdatePacket* pkt = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        spdlog::info("=== PARSING INVENTORY ===");
        spdlog::info("Data size: {} bytes", data.size());
        
        if (data.size() < 7) {
            spdlog::warn("Inventory data too small: {} bytes", data.size());
            items_.clear();
            inventory_size_ = 0;
            return;
        }

        // Save raw server data for authentic patching
        raw_server_ext_data_ = data;
        if (pkt) {
            raw_server_game_packet_ = *pkt;
        }
        has_raw_data_ = true;
        
        
        std::ostringstream hex;
        for (size_t i = 0; i < std::min(size_t(20), data.size()); i++) {
            hex << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
        }
        spdlog::info("First 20 bytes: {}", hex.str());
        
        size_t offset = 1; 
        
        
        inventory_size_ = *reinterpret_cast<const uint32_t*>(&data[offset]);
        offset += 4;
        
        
        uint16_t item_count = *reinterpret_cast<const uint16_t*>(&data[offset]);
        offset += 2;
        
        spdlog::info("Inventory: size={}, items={}", inventory_size_, item_count);
        
        
        items_.clear();
        
        
        last_update_ = std::chrono::steady_clock::now();
        
        
        if (item_count > 500) {
            spdlog::error("Item count {} is unreasonable! Skipping parse.", item_count);
            inventory_size_ = 0;
            return;
        }
        
        
        for (uint16_t i = 0; i < item_count; i++) {
            if (offset + 4 > data.size()) {
                spdlog::warn("Not enough data for item {} (offset={}, size={})", i, offset, data.size());
                break;
            }
            
            uint16_t id = *reinterpret_cast<const uint16_t*>(&data[offset]);
            offset += 2;
            uint8_t amount = static_cast<uint8_t>(data[offset++]);
            uint8_t flags = static_cast<uint8_t>(data[offset++]);
            
            auto it = items_.find(id);
            if (it != items_.end()) {
                it->second.amount += amount;
            } else {
                items_[id] = InventoryItem{ id, amount, flags };
            }
            
            if (i < 5) {  
                spdlog::info("  Item {}: id={}, amount={}, flags={}", i, id, amount, flags);
            }
        }
        
        spdlog::info("✓ Parsed {} inventory items", items_.size());
        save_balance_cache_unlocked();
    }

    void record_pending_drop(uint16_t item_id, uint8_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_drop_lost_[item_id] += count;
    }

    bool record_collected_uid(uint32_t uid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_uid_cleanup_).count() > 15) {
            processed_uids_.clear();
            last_uid_cleanup_ = now;
        }
        if (processed_uids_.find(uid) != processed_uids_.end()) {
            return false;
        }
        processed_uids_.insert(uid);
        return true;
    }

    void add_item(uint16_t item_id, uint8_t count) {
        if (count == 0) return;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = items_.find(item_id);
        uint8_t flags = 0;
        if (it != items_.end()) {
            it->second.amount += count;
            flags = it->second.flags;
        } else {
            items_[item_id] = InventoryItem{ item_id, static_cast<uint32_t>(count), flags };
        }
        sync_raw_ext_data_unlocked(item_id, static_cast<uint8_t>(std::min(it != items_.end() ? it->second.amount : count, 200u)), flags);
        if (item_id == 242 || item_id == 1796 || item_id == 7188) {
            save_balance_cache_unlocked();
            spdlog::info("[BALANCE-UPDATE] Added {}x item {} -> New balance: {} WL, {} DL, {} BGL",
                         count, item_id, get_item_count_unlocked(242), get_item_count_unlocked(1796), get_item_count_unlocked(7188));
        }
    }

    void remove_item(uint16_t item_id, uint8_t count) {
        if (count == 0) return;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = items_.find(item_id);
        if (it == items_.end()) return;

        if (count >= it->second.amount || count == 255) {
            items_.erase(it);
            sync_raw_ext_data_unlocked(item_id, 0, 0);
        } else {
            it->second.amount -= count;
            sync_raw_ext_data_unlocked(item_id, static_cast<uint8_t>(std::min(it->second.amount, 200u)), it->second.flags);
        }
        if (item_id == 242 || item_id == 1796 || item_id == 7188) {
            save_balance_cache_unlocked();
            spdlog::info("[BALANCE-UPDATE] Removed {}x item {} -> New balance: {} WL, {} DL, {} BGL",
                         count, item_id, get_item_count_unlocked(242), get_item_count_unlocked(1796), get_item_count_unlocked(7188));
        }
    }

    void apply_modify_inventory(uint16_t item_id, uint8_t lost_count, uint8_t gained_count, uint32_t flags, float float_var) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = items_.find(item_id);

        if (lost_count > 0) {
            uint32_t already_deducted = 0;
            auto pd_it = pending_drop_lost_.find(item_id);
            if (pd_it != pending_drop_lost_.end() && pd_it->second > 0) {
                already_deducted = std::min(static_cast<uint32_t>(lost_count), pd_it->second);
                pd_it->second -= already_deducted;
                if (pd_it->second == 0) {
                    pending_drop_lost_.erase(pd_it);
                }
            }
            uint32_t effective_lost = lost_count - already_deducted;
            if (effective_lost > 0) {
                if (it != items_.end()) {
                    if (effective_lost >= it->second.amount || lost_count == 255) {
                        items_.erase(it);
                        sync_raw_ext_data_unlocked(item_id, 0, 0);
                    } else {
                        it->second.amount -= effective_lost;
                        it->second.flags = static_cast<uint8_t>(flags);
                        sync_raw_ext_data_unlocked(item_id, static_cast<uint8_t>(std::min(it->second.amount, 200u)), it->second.flags);
                    }
                }
            }
        } else if (gained_count > 0) {
            if (it != items_.end()) {
                it->second.amount += gained_count;
                it->second.flags = static_cast<uint8_t>(flags);
                sync_raw_ext_data_unlocked(item_id, static_cast<uint8_t>(std::min(it->second.amount, 200u)), static_cast<uint8_t>(flags));
            } else {
                items_[item_id] = InventoryItem{ item_id, static_cast<uint32_t>(gained_count), static_cast<uint8_t>(flags) };
                sync_raw_ext_data_unlocked(item_id, gained_count, static_cast<uint8_t>(flags));
            }
        } else if (float_var > 0.0f) {
            uint32_t cnt = static_cast<uint32_t>(float_var);
            items_[item_id] = InventoryItem{ item_id, cnt, static_cast<uint8_t>(flags) };
            sync_raw_ext_data_unlocked(item_id, static_cast<uint8_t>(std::min(cnt, 200u)), static_cast<uint8_t>(flags));
        } else {
            // When both counts and float_var are 0, this is an equip flag change - DO NOT erase!
            if (it != items_.end()) {
                it->second.flags = static_cast<uint8_t>(flags);
                sync_raw_ext_data_unlocked(item_id, static_cast<uint8_t>(std::min(it->second.amount, 200u)), static_cast<uint8_t>(flags));
            }
        }

        if (item_id == 242 || item_id == 1796 || item_id == 7188) {
            save_balance_cache_unlocked();
            spdlog::info("[BALANCE-UPDATE] Modified item {} (lost={}, gained={}) -> New balance: {} WL, {} DL, {} BGL",
                         item_id, lost_count, gained_count, get_item_count_unlocked(242), get_item_count_unlocked(1796), get_item_count_unlocked(7188));
        }
    }

    void update_inventory_item(uint32_t item_id, float amount, uint32_t flags) {
        apply_modify_inventory(static_cast<uint16_t>(item_id), 0, 0, flags, amount);
    }
    
    bool has_item(uint16_t item_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.find(item_id) != items_.end();
    }
    
    int get_item_count(uint16_t item_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return get_item_count_unlocked(item_id);
    }

    void get_balance(int& out_wl, int& out_dl, int& out_bgl, int& out_total_wl) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!items_.empty() || !has_cached_balance_) {
            out_wl = get_item_count_unlocked(242);
            out_dl = get_item_count_unlocked(1796);
            out_bgl = get_item_count_unlocked(7188);
        } else {
            out_wl = cached_wl_;
            out_dl = cached_dl_;
            out_bgl = cached_bgl_;
        }
        out_total_wl = out_wl + out_dl * 100 + out_bgl * 10000;
    }

    int get_total_wl() const {
        int wl = 0, dl = 0, bgl = 0, total_wl = 0;
        get_balance(wl, dl, bgl, total_wl);
        return total_wl;
    }

    bool has_cached_balance() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return has_cached_balance_ || !items_.empty();
    }
    
    const std::unordered_map<uint16_t, InventoryItem>& get_items() const {
        return items_;
    }

    [[nodiscard]] std::vector<InventoryItem> get_items_snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<InventoryItem> out;
        out.reserve(items_.size());
        for (const auto& [id, item] : items_) {
            out.push_back(item);
        }
        return out;
    }
    
    uint32_t get_inventory_size() const {
        return inventory_size_;
    }
    
    bool is_fresh() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_).count();
        return elapsed <= 5;
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
        inventory_size_ = 0;
        last_update_ = std::chrono::steady_clock::time_point{};
    }

private:
    InventoryManager() : inventory_size_(0), last_update_{}, cached_wl_(0), cached_dl_(0), cached_bgl_(0), has_cached_balance_(false) {
        load_balance_cache();
    }
    ~InventoryManager() = default;
    InventoryManager(const InventoryManager&) = delete;
    InventoryManager& operator=(const InventoryManager&) = delete;

    int get_item_count_unlocked(uint16_t item_id) const {
        auto it = items_.find(item_id);
        return (it != items_.end()) ? static_cast<int>(it->second.amount) : 0;
    }

    void sync_raw_ext_data_unlocked(uint16_t id, uint8_t count, uint8_t flags) {
        if (raw_server_ext_data_.size() < 7) return;
        uint16_t item_count = *reinterpret_cast<const uint16_t*>(&raw_server_ext_data_[5]);
        size_t items_start = 7;
        for (uint16_t i = 0; i < item_count; ++i) {
            size_t offset = items_start + static_cast<size_t>(i) * 4;
            if (offset + 4 <= raw_server_ext_data_.size()) {
                uint16_t entry_id = *reinterpret_cast<const uint16_t*>(&raw_server_ext_data_[offset]);
                if (entry_id == id) {
                    if (count == 0) {
                        raw_server_ext_data_.erase(raw_server_ext_data_.begin() + offset, raw_server_ext_data_.begin() + offset + 4);
                        item_count--;
                        *reinterpret_cast<uint16_t*>(&raw_server_ext_data_[5]) = item_count;
                    } else {
                        raw_server_ext_data_[offset + 2] = static_cast<std::byte>(count);
                        raw_server_ext_data_[offset + 3] = static_cast<std::byte>(flags);
                    }
                    return;
                }
            }
        }
        if (count > 0) {
            std::byte entry[4];
            *reinterpret_cast<uint16_t*>(&entry[0]) = id;
            entry[2] = static_cast<std::byte>(count);
            entry[3] = static_cast<std::byte>(flags);
            raw_server_ext_data_.insert(raw_server_ext_data_.begin() + items_start + item_count * 4, entry, entry + 4);
            item_count++;
            *reinterpret_cast<uint16_t*>(&raw_server_ext_data_[5]) = item_count;
        }
    }

    void save_balance_cache_unlocked() {
        try {
            int wl = get_item_count_unlocked(242);
            int dl = get_item_count_unlocked(1796);
            int bgl = get_item_count_unlocked(7188);
            cached_wl_ = wl;
            cached_dl_ = dl;
            cached_bgl_ = bgl;
            has_cached_balance_ = true;

            std::string json_data = fmt::format("{{\"wl\":{},\"dl\":{},\"bgl\":{}}}", wl, dl, bgl);
            std::ofstream f("balance_cache.json");
            if (f.is_open()) {
                f << json_data;
            }
            if (std::filesystem::exists("build/src/Release")) {
                std::ofstream f2("build/src/Release/balance_cache.json");
                if (f2.is_open()) {
                    f2 << json_data;
                }
            }
        } catch (...) {}
    }

    void load_balance_cache() {
        try {
            std::ifstream f("balance_cache.json");
            if (!f.is_open()) {
                f.open("build/src/Release/balance_cache.json");
            }
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                auto parse_field = [&](const std::string& key) -> int {
                    size_t pos = content.find("\"" + key + "\":");
                    if (pos == std::string::npos) pos = content.find("\"" + key + "\" :");
                    if (pos == std::string::npos) return 0;
                    size_t val_start = content.find_first_of("0123456789", pos);
                    if (val_start == std::string::npos) return 0;
                    size_t val_end = content.find_first_not_of("0123456789", val_start);
                    if (val_end == std::string::npos) val_end = content.size();
                    return std::stoi(content.substr(val_start, val_end - val_start));
                };
                cached_wl_ = parse_field("wl");
                cached_dl_ = parse_field("dl");
                cached_bgl_ = parse_field("bgl");
                has_cached_balance_ = true;
                spdlog::info("InventoryManager: Loaded cached balance: {} WL, {} DL, {} BGL", cached_wl_, cached_dl_, cached_bgl_);
            }
        } catch (...) {}
    }
    
    mutable std::mutex mutex_;
    uint32_t inventory_size_;
    std::unordered_map<uint16_t, InventoryItem> items_;
    std::chrono::steady_clock::time_point last_update_;

    int cached_wl_;
    int cached_dl_;
    int cached_bgl_;
    bool has_cached_balance_;

    std::unordered_set<uint32_t> processed_uids_{};
    std::chrono::steady_clock::time_point last_uid_cleanup_{};

    std::unordered_map<uint16_t, uint32_t> pending_drop_lost_{};

    packet::GameUpdatePacket raw_server_game_packet_{};
    std::vector<std::byte> raw_server_ext_data_{};
    bool has_raw_data_{ false };
};

} 
