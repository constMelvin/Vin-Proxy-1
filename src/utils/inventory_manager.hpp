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

namespace command {
extern std::unordered_map<int, int> g_clothing_slots;
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

    void send_inventory(core::Core* core, uint32_t net_id, uint16_t visual_hand_id = 0) {
        if (!core || !core->get_server() || !core->get_server()->get_player()) return;

        std::lock_guard<std::mutex> lock(mutex_);

        // If inventory is empty, initialize with basic defaults (Fist and Wrench)
        if (items_.empty()) {
            items_[18] = InventoryItem{ 18, 1, 0 };
            items_[32] = InventoryItem{ 32, 1, 0 };
            if (inventory_size_ == 0) inventory_size_ = 16;
        }

        std::vector<InventoryItem> item_list;
        item_list.reserve(items_.size() + 12);

        // Always put Fist (18) and Wrench (32) first in hotbar
        item_list.push_back(InventoryItem{ 18, 1, 0 });
        if (items_.find(32) != items_.end()) {
            item_list.push_back(items_[32]);
        } else {
            item_list.push_back(InventoryItem{ 32, 1, 0 });
        }

        std::unordered_set<uint16_t> added_ids;
        added_ids.insert(18);
        added_ids.insert(32);

        // Add all real inventory items
        for (const auto& [id, itm] : items_) {
            if (added_ids.count(id)) continue;
            item_list.push_back(itm);
            added_ids.insert(id);
        }

        // Add visual hand weapon as EQUIPPED (flags = 1)
        if (visual_hand_id > 0) {
            if (!added_ids.count(visual_hand_id)) {
                item_list.push_back(InventoryItem{ visual_hand_id, 1, 1 });
                added_ids.insert(visual_hand_id);
            } else {
                for (auto& itm : item_list) {
                    if (itm.id == visual_hand_id) {
                        itm.flags = 1;
                        break;
                    }
                }
            }
        }

        // Add all active visual clothing items as EQUIPPED (flags = 1)
        for (const auto& [slot, item_id] : command::g_clothing_slots) {
            if (item_id <= 0) continue;
            uint16_t uid = static_cast<uint16_t>(item_id);
            if (!added_ids.count(uid)) {
                item_list.push_back(InventoryItem{ uid, 1, 1 });
                added_ids.insert(uid);
            } else {
                for (auto& itm : item_list) {
                    if (itm.id == uid) {
                        itm.flags = 1;
                        break;
                    }
                }
            }
        }

        std::vector<std::byte> ext_data;
        ext_data.reserve(7 + item_list.size() * 4);
        ext_data.push_back(std::byte{ 0x01 });

        uint32_t inv_size = inventory_size_ > 0 ? inventory_size_ : 16;
        if (item_list.size() > inv_size) {
            inv_size = static_cast<uint32_t>(item_list.size());
        }
        const std::byte* size_ptr = reinterpret_cast<const std::byte*>(&inv_size);
        ext_data.insert(ext_data.end(), size_ptr, size_ptr + 4);

        uint16_t count = static_cast<uint16_t>(item_list.size());
        const std::byte* count_ptr = reinterpret_cast<const std::byte*>(&count);
        ext_data.insert(ext_data.end(), count_ptr, count_ptr + 2);

        for (const auto& item : item_list) {
            const std::byte* id_ptr = reinterpret_cast<const std::byte*>(&item.id);
            ext_data.insert(ext_data.end(), id_ptr, id_ptr + 2);
            ext_data.push_back(static_cast<std::byte>(item.amount));
            ext_data.push_back(static_cast<std::byte>(item.flags));
        }

        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_SEND_INVENTORY_STATE;
        pkt.net_id = 0xFFFFFFFF; // ALWAYS -1 for local inventory!
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());

        core->get_server()->get_player()->send_packet(bs.get_data(), 0);
        spdlog::info("InventoryManager: Forwarded inventory ({} items, visual hand {}) to client (net_id=-1)",
                     count, visual_hand_id);
    }

    void parse_inventory(const std::vector<std::byte>& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        spdlog::info("=== PARSING INVENTORY ===");
        spdlog::info("Data size: {} bytes", data.size());
        
        if (data.size() < 7) {
            spdlog::warn("Inventory data too small: {} bytes", data.size());
            
            items_.clear();
            inventory_size_ = 0;
            return;
        }
        
        
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
};

} 
