#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
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
    uint8_t amount;
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
            
            InventoryItem item;
            item.id = *reinterpret_cast<const uint16_t*>(&data[offset]);
            offset += 2;
            item.amount = static_cast<uint8_t>(data[offset++]);
            item.flags = static_cast<uint8_t>(data[offset++]);
            
            items_[item.id] = item;
            
            if (i < 5) {  
                spdlog::info("  Item {}: id={}, amount={}, flags={}", i, item.id, item.amount, item.flags);
            }
        }
        
        spdlog::info("✓ Parsed {} inventory items", items_.size());
    }

    void update_inventory_item(uint32_t item_id, float amount, uint32_t flags) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint16_t id = static_cast<uint16_t>(item_id);
        uint8_t count = static_cast<uint8_t>(amount);

        if (count == 0) {
            items_.erase(id);
        } else {
            items_[id] = InventoryItem{ id, count, static_cast<uint8_t>(flags) };
        }

        if (raw_server_ext_data_.size() >= 7) {
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
    }
    
    bool has_item(uint16_t item_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.find(item_id) != items_.end();
    }
    
    uint8_t get_item_count(uint16_t item_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = items_.find(item_id);
        return (it != items_.end()) ? it->second.amount : 0;
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
    InventoryManager() : inventory_size_(0), last_update_{} {}
    ~InventoryManager() = default;
    InventoryManager(const InventoryManager&) = delete;
    InventoryManager& operator=(const InventoryManager&) = delete;
    
    mutable std::mutex mutex_;
    uint32_t inventory_size_;
    std::unordered_map<uint16_t, InventoryItem> items_;
    std::chrono::steady_clock::time_point last_update_;

    packet::GameUpdatePacket raw_server_game_packet_{};
    std::vector<std::byte> raw_server_ext_data_{};
    bool has_raw_data_{ false };
};

} 
