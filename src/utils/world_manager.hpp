#pragma once
#include "world_info.h"
#include "world_tile.h"
#include "world_parser_v2.h"
#include <mutex>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>

namespace utils {

inline bool is_world_lock_item(uint16_t fg) {
    switch (fg) {
        case 242:   // World Lock
        case 1796:  // Diamond Lock
        case 2408:  // Emerald Lock
        case 2950:  // Robotic Lock
        case 4428:  // Ruby Lock
        case 4802:  // Royal Lock
        case 4994:  // Builder's Lock
        case 5260:  // Harmonic Lock
        case 5814:  // Guild Lock
        case 5980:  // Bunny Lock
        case 7188:  // Blue Gem Lock
        case 8470:  // Ecto Lock
        case 9640:  // My First World Lock
        case 10410: // Legendary Lock
        case 11550: // Blood Dragon Lock
        case 11586: // Prince of Persia Lock
        case 11902: // Radical City Lock
        case 12654: // Assassin's Creed Lock
        case 13200: // Rayman Lock
        case 13636: // Steampunk Lock
        case 14296: // Immortals Fenyx Rising Lock
        case 14536: // Enchanted Lock
        case 14538: // Royal Enchanted Lock
            return true;
        default:
            return false;
    }
}

inline bool is_any_lock_item(uint16_t fg) {
    if (fg == 202 || fg == 204 || fg == 206) return true; // Small Lock, Big Lock, Huge Lock
    if (fg == 4992 || fg == 8468) return true;
    return is_world_lock_item(fg);
}

class WorldManager {
public:
    
    struct TileData {
        uint16_t Fg = 0;
        uint16_t Bg = 0;
        uint16_t Flags = 0;
        uint16_t ParentIndex = 0;
        uint16_t LockIndex = 0;
    };

    static WorldManager& get_instance() {
        static WorldManager instance;
        return instance;
    }

    
    void set_current_world_v2(const world_v2::World& world) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        
        tiles_.clear();
        items_.clear();
        has_world_ = false;
        server_reported_no_access_ = false;
        
        
        width_ = world.width;
        height_ = world.height;
        world_name_ = world.name;  
        world_v2_copy_ = world;    
        
        spdlog::info("WorldManager: Storing V2 world data - '{}' {}x{}, {} tiles", 
                     world.name, world.width, world.height, world.tiles.size());
        
        
        if (!world.tiles.empty()) {
            tiles_.reserve(world.tiles.size());
            for (const auto& tile : world.tiles) {
                TileData td;
                td.Fg = tile.fg;
                td.Bg = tile.bg;
                td.Flags = tile.flags;
                td.ParentIndex = tile.parent_index;
                td.LockIndex = tile.lock_index;
                tiles_.push_back(td);
            }
            spdlog::info("WorldManager: Stored {} tiles", tiles_.size());
            has_world_ = true;
        } else {
            spdlog::warn("WorldManager: No tiles in V2 world data!");
        }
        
        
        if (!world.dropped_items.empty()) {
            items_.reserve(world.dropped_items.size());
            for (const auto& item : world.dropped_items) {
                world::DroppedItemInfo di;
                di.ItemId = item.id;
                di.X = item.x;
                di.Y = item.y;
                di.Amount = item.count;  
                di.Flag = item.flags;    
                di.Uid = item.uid;
                items_.push_back(di);
            }
            spdlog::info("WorldManager: Stored {} dropped items", items_.size());
        } else {
            spdlog::info("WorldManager: No dropped items in V2 world");
        }

        last_dropped_item_uid_ = world.last_dropped_item_uid;
        for (const auto& it : items_) {
            if (it.Uid > last_dropped_item_uid_) last_dropped_item_uid_ = it.Uid;
        }
    }

    
    void set_current_world(const world::WorldInfo& world_info) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        
        tiles_.clear();
        items_.clear();
        has_world_ = false;  
        
        
        width_ = world_info.Width;
        height_ = world_info.Height;
        
        spdlog::info("WorldManager: Storing world data - {}x{}, {} tiles, {} items", 
                     world_info.Width, world_info.Height, 
                     world_info.TilesCount, world_info.ItemDropsCount);
        
        
        if (world_info.Tiles && world_info.TilesCount > 0) {
            tiles_.reserve(world_info.TilesCount);
            for (size_t i = 0; i < world_info.TilesCount; ++i) {
                TileData tile;
                tile.Fg = world_info.Tiles[i].Fg;
                tile.Bg = world_info.Tiles[i].Bg;
                tile.Flags = world_info.Tiles[i].Flags.Value;
                tile.ParentIndex = world_info.Tiles[i].ParentTileIndex;
                tile.LockIndex = world_info.Tiles[i].LockIndex;
                tiles_.push_back(tile);
            }
            spdlog::info("WorldManager: Stored {} tiles", tiles_.size());
            has_world_ = true;  
        } else {
            spdlog::warn("WorldManager: No tiles in world data!");
        }
        
        
        if (world_info.ItemDrops && world_info.ItemDropsCount > 0) {
            items_.reserve(world_info.ItemDropsCount);
            for (size_t i = 0; i < world_info.ItemDropsCount; ++i) {
                items_.push_back(world_info.ItemDrops[i]);
            }
            spdlog::info("WorldManager: Stored {} dropped items", items_.size());
        } else {
            spdlog::info("WorldManager: No dropped items in world");
        }
    }

    
    bool has_world() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return has_world_;
    }

    
    std::string get_world_name() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return world_name_;
    }

    
    uint32_t get_world_width() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return width_;
    }

    uint32_t get_world_height() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return height_;
    }

    world_v2::World get_world_v2() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return world_v2_copy_;
    }

    
    const std::vector<TileData>& get_tiles() const { 
        return tiles_; 
    }

    uint16_t get_tile_fg(uint32_t tile_x, uint32_t tile_y) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (tile_x >= width_ || tile_y >= height_) {
            return 0;
        }
        size_t idx = static_cast<size_t>(tile_y) * width_ + tile_x;
        if (idx < tiles_.size()) {
            if (tiles_[idx].Fg != 0) {
                return tiles_[idx].Fg;
            }
        }
        // Fallback to parsed world tiles
        if (idx < world_v2_copy_.tiles.size()) {
            return world_v2_copy_.tiles[idx].fg;
        }
        return 0;
    }

    uint16_t get_tile_flags(uint32_t tile_x, uint32_t tile_y) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (tile_x >= width_ || tile_y >= height_) {
            return 0;
        }
        size_t idx = static_cast<size_t>(tile_y) * width_ + tile_x;
        if (idx < tiles_.size()) {
            if (tiles_[idx].Flags != 0) {
                return tiles_[idx].Flags;
            }
        }
        if (idx < world_v2_copy_.tiles.size()) {
            return world_v2_copy_.tiles[idx].flags;
        }
        return 0;
    }

    bool is_tile_gate_public_or_open_unlocked(uint32_t tile_x, uint32_t tile_y) const {
        if (tile_x >= width_ || tile_y >= height_) {
            return false;
        }
        size_t idx = static_cast<size_t>(tile_y) * width_ + tile_x;
        if (idx < tiles_.size()) {
            uint16_t flags = tiles_[idx].Flags;
            // TILEFLAG_OPEN = 1 << 6 (0x0040), TILEFLAG_PUBLIC = 1 << 7 (0x0080)
            if ((flags & 0x0040) != 0 || (flags & 0x0080) != 0) {
                return true;
            }
        }
        if (idx < world_v2_copy_.tiles.size()) {
            const auto& t = world_v2_copy_.tiles[idx];
            if ((t.flags & 0x0040) != 0 || (t.flags & 0x0080) != 0 || t.lock_data.is_public) {
                return true;
            }
        }
        return false;
    }

    bool is_tile_gate_public_or_open(uint32_t tile_x, uint32_t tile_y) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_tile_gate_public_or_open_unlocked(tile_x, tile_y);
    }

    bool is_world_owner_unlocked(uint32_t user_id) const {
        if (server_reported_no_access_) return false;
        if (user_id == 0) return false;
        for (const auto& t : world_v2_copy_.tiles) {
            if (is_world_lock_item(t.fg) && t.lock_data.has_data()) {
                if (t.lock_data.owner_uid == user_id) {
                    return true;
                }
            }
        }
        return false;
    }

    bool is_world_owner(uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_world_owner_unlocked(user_id);
    }

    bool has_world_admin_access_unlocked(uint32_t user_id) const {
        if (server_reported_no_access_) return false;
        if (user_id == 0) return false;
        for (const auto& t : world_v2_copy_.tiles) {
            if (is_world_lock_item(t.fg) && t.lock_data.has_data()) {
                for (uint32_t admin : t.lock_data.access_list) {
                    if (admin == user_id) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool has_world_admin_access(uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return has_world_admin_access_unlocked(user_id);
    }

    bool has_world_access_unlocked(uint32_t user_id) const {
        if (server_reported_no_access_) return false;
        if (user_id == 0) return false;
        for (const auto& t : world_v2_copy_.tiles) {
            if (is_world_lock_item(t.fg) && t.lock_data.has_data()) {
                if (t.lock_data.owner_uid == user_id) {
                    return true;
                }
                for (uint32_t admin : t.lock_data.access_list) {
                    if (admin == user_id) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool has_world_access(uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return has_world_access_unlocked(user_id);
    }

    void set_server_reported_no_access(bool no_access) {
        std::lock_guard<std::mutex> lock(mutex_);
        server_reported_no_access_ = no_access;
        spdlog::info("WorldManager: Server reported no_access={}", no_access);
    }

    bool is_server_reported_no_access() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return server_reported_no_access_;
    }

    bool has_tile_lock_access_unlocked(uint32_t tile_x, uint32_t tile_y, uint32_t user_id) const {
        if (user_id == 0) return false;
        if (tile_x >= width_ || tile_y >= height_) {
            return false;
        }
        size_t idx = static_cast<size_t>(tile_y) * width_ + tile_x;

        // 1. Check if the tile itself has lock/entrance access data (e.g. VIP Entrance, Friends Entrance)
        if (idx < world_v2_copy_.tiles.size()) {
            const auto& t = world_v2_copy_.tiles[idx];
            if (t.lock_data.has_data()) {
                if (t.lock_data.owner_uid == user_id) return true;
                for (uint32_t admin : t.lock_data.access_list) {
                    if (admin == user_id) return true;
                }
            }
        }

        // 2. Check candidate parent lock indices
        std::vector<uint32_t> candidate_indices;
        if (idx < tiles_.size()) {
            if (tiles_[idx].LockIndex > 0) candidate_indices.push_back(tiles_[idx].LockIndex);
            if (tiles_[idx].ParentIndex > 0) candidate_indices.push_back(tiles_[idx].ParentIndex);
        }
        if (idx < world_v2_copy_.tiles.size()) {
            if (world_v2_copy_.tiles[idx].lock_index > 0) candidate_indices.push_back(world_v2_copy_.tiles[idx].lock_index);
            if (world_v2_copy_.tiles[idx].parent_index > 0) candidate_indices.push_back(world_v2_copy_.tiles[idx].parent_index);
        }

        for (uint32_t p_idx : candidate_indices) {
            if (p_idx < world_v2_copy_.tiles.size()) {
                const auto& parent_t = world_v2_copy_.tiles[p_idx];
                if (parent_t.lock_data.has_data()) {
                    if (parent_t.lock_data.owner_uid == user_id) return true;
                    for (uint32_t admin : parent_t.lock_data.access_list) {
                        if (admin == user_id) return true;
                    }
                }
            }
        }

        // 3. Check area locks (Small Lock 202, Big Lock 204, Huge Lock 206) covering this tile geometrically
        for (const auto& t : world_v2_copy_.tiles) {
            if (t.lock_data.has_data()) {
                int radius = 0;
                if (t.fg == 202) radius = 1;       // Small Lock: 3x3
                else if (t.fg == 204) radius = 2;  // Big Lock: 5x5
                else if (t.fg == 206) radius = 3;  // Huge Lock: 7x7
                if (radius > 0) {
                    int lx = static_cast<int>(t.x);
                    int ly = static_cast<int>(t.y);
                    int tx = static_cast<int>(tile_x);
                    int ty = static_cast<int>(tile_y);
                    if (tx >= lx - radius && tx <= lx + radius &&
                        ty >= ly - radius && ty <= ly + radius) {
                        if (t.lock_data.owner_uid == user_id) return true;
                        for (uint32_t admin : t.lock_data.access_list) {
                            if (admin == user_id) return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool has_tile_lock_access(uint32_t tile_x, uint32_t tile_y, uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return has_tile_lock_access_unlocked(tile_x, tile_y, user_id);
    }

    bool can_player_teleport_on_gate(uint32_t tile_x, uint32_t tile_y, uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_tile_gate_public_or_open_unlocked(tile_x, tile_y)) {
            return true;
        }
        if (user_id != 0) {
            if (has_world_access_unlocked(user_id)) {
                return true;
            }
            if (has_tile_lock_access_unlocked(tile_x, tile_y, user_id)) {
                return true;
            }
        }
        return false;
    }

    bool is_tile_lock_unlocked(uint32_t tile_x, uint32_t tile_y) const {
        if (tile_x >= width_ || tile_y >= height_) return false;
        size_t idx = static_cast<size_t>(tile_y) * width_ + tile_x;
        uint16_t fg = 0;
        if (idx < tiles_.size()) fg = tiles_[idx].Fg;
        if (fg == 0 && idx < world_v2_copy_.tiles.size()) fg = world_v2_copy_.tiles[idx].fg;
        if (is_any_lock_item(fg)) return true;
        if (idx < world_v2_copy_.tiles.size() && world_v2_copy_.tiles[idx].is_lock()) return true;
        return false;
    }

    bool is_tile_lock(uint32_t tile_x, uint32_t tile_y) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_tile_lock_unlocked(tile_x, tile_y);
    }

    const world_v2::Tile* get_area_lock_unlocked(uint32_t tile_x, uint32_t tile_y) const {
        if (tile_x >= width_ || tile_y >= height_) return nullptr;
        for (const auto& t : world_v2_copy_.tiles) {
            if (t.lock_data.has_data()) {
                int radius = 0;
                if (t.fg == 202) radius = 1;       // Small Lock: 3x3
                else if (t.fg == 204) radius = 2;  // Big Lock: 5x5
                else if (t.fg == 206) radius = 3;  // Huge Lock: 7x7
                if (radius > 0) {
                    int lx = static_cast<int>(t.x);
                    int ly = static_cast<int>(t.y);
                    int tx = static_cast<int>(tile_x);
                    int ty = static_cast<int>(tile_y);
                    if (tx >= lx - radius && tx <= lx + radius &&
                        ty >= ly - radius && ty <= ly + radius) {
                        return &t;
                    }
                }
            }
        }
        return nullptr;
    }

    const world_v2::Tile* get_area_lock(uint32_t tile_x, uint32_t tile_y) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return get_area_lock_unlocked(tile_x, tile_y);
    }

    bool can_player_access_area_lock_unlocked(uint32_t target_x, uint32_t target_y, uint32_t cur_x, uint32_t cur_y, uint32_t user_id) const {
        const auto* target_lock = get_area_lock_unlocked(target_x, target_y);
        if (!target_lock) {
            return true; // No area lock covers the target spot
        }

        if (target_lock->lock_data.is_public) {
            return true;
        }
        if (user_id != 0) {
            if (target_lock->lock_data.owner_uid == user_id) {
                return true;
            }
            for (uint32_t admin : target_lock->lock_data.access_list) {
                if (admin == user_id) {
                    return true;
                }
            }
        }

        // If player is already inside the same area lock, allow moving inside it
        const auto* cur_lock = get_area_lock_unlocked(cur_x, cur_y);
        if (cur_lock != nullptr && cur_lock == target_lock) {
            return true;
        }

        return false;
    }

    bool can_player_access_area_lock(uint32_t target_x, uint32_t target_y, uint32_t cur_x, uint32_t cur_y, uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return can_player_access_area_lock_unlocked(target_x, target_y, cur_x, cur_y, user_id);
    }

    const world_v2::Tile* get_covering_lock_unlocked(uint32_t tile_x, uint32_t tile_y) const {
        if (tile_x >= width_ || tile_y >= height_) return nullptr;
        size_t idx = static_cast<size_t>(tile_y) * width_ + tile_x;

        // 1. Check area locks (Small Lock 202: 3x3, Big Lock 204: 5x5, Huge Lock 206: 7x7)
        const auto* area_lock = get_area_lock_unlocked(tile_x, tile_y);
        if (area_lock) return area_lock;

        // 2. Check if tile is marked locked via TILEFLAG_LOCKED (0x0002)
        uint16_t flags = 0;
        uint16_t lock_idx = 0;
        if (idx < tiles_.size()) {
            flags = tiles_[idx].Flags;
            lock_idx = tiles_[idx].LockIndex;
            if (lock_idx == 0) lock_idx = tiles_[idx].ParentIndex;
        }
        if (idx < world_v2_copy_.tiles.size()) {
            if (flags == 0) flags = world_v2_copy_.tiles[idx].flags;
            if (lock_idx == 0) lock_idx = world_v2_copy_.tiles[idx].lock_index;
            if (lock_idx == 0) lock_idx = world_v2_copy_.tiles[idx].parent_index;
        }

        if ((flags & 0x0002) != 0 || lock_idx > 0) {
            if (lock_idx < world_v2_copy_.tiles.size()) {
                const auto& parent_t = world_v2_copy_.tiles[lock_idx];
                if (parent_t.lock_data.has_data() || parent_t.is_lock() || is_any_lock_item(parent_t.fg)) {
                    return &parent_t;
                }
            }
        }

        // 3. Check World Lock covering the world
        for (const auto& t : world_v2_copy_.tiles) {
            if (is_world_lock_item(t.fg) && t.lock_data.has_data()) {
                return &t;
            }
        }

        return nullptr;
    }

    bool can_player_access_tile_lock_unlocked(uint32_t target_x, uint32_t target_y, uint32_t cur_x, uint32_t cur_y, uint32_t user_id) const {
        return can_player_access_area_lock_unlocked(target_x, target_y, cur_x, cur_y, user_id);
    }

    bool can_player_access_tile_lock(uint32_t target_x, uint32_t target_y, uint32_t cur_x, uint32_t cur_y, uint32_t user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return can_player_access_tile_lock_unlocked(target_x, target_y, cur_x, cur_y, user_id);
    }

    bool is_world_locked_unlocked() const {
        if (server_reported_no_access_) return true;
        for (const auto& t : world_v2_copy_.tiles) {
            if (is_world_lock_item(t.fg)) {
                return true;
            }
        }
        for (const auto& t : tiles_) {
            if (is_world_lock_item(t.Fg)) {
                return true;
            }
        }
        return false;
    }

    bool is_world_locked() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_world_locked_unlocked();
    }

    void update_tile(int32_t x, int32_t y, uint16_t item_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (x >= 0 && y >= 0 && static_cast<uint32_t>(x) < width_ && static_cast<uint32_t>(y) < height_) {
            size_t idx = static_cast<size_t>(y) * width_ + static_cast<size_t>(x);
            if (idx < tiles_.size()) {
                // Item 18 is punch/fist action - NEVER overwrite tile with 0 for punch!
                if (item_id == 18) {
                    return;
                }
                tiles_[idx].Fg = item_id;
                spdlog::debug("WorldManager: Updated tile ({},{}) Fg -> {}", x, y, item_id);
            }
        }
    }
    
    
    const std::vector<world::DroppedItemInfo>& get_items() const { 
        return items_; 
    }
    
    
    const std::vector<world::DroppedItemInfo>& get_live_objects() const {
        return live_objects_;
    }
    
    
    void clear_live_objects() {
        std::lock_guard<std::mutex> lock(mutex_);
        live_objects_.clear();
        spdlog::info("WorldManager: Cleared live objects");
    }
    
    
    uint32_t allocate_next_dropped_uid() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& it : items_)        if (it.Uid > last_dropped_item_uid_) last_dropped_item_uid_ = it.Uid;
        for (const auto& it : live_objects_) if (it.Uid > last_dropped_item_uid_) last_dropped_item_uid_ = it.Uid;
        return ++last_dropped_item_uid_;
    }

    void record_server_uid(uint32_t uid) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (uid > last_dropped_item_uid_) last_dropped_item_uid_ = uid;
    }

    void add_live_object(const world::DroppedItemInfo& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        live_objects_.push_back(item);
        if (item.Uid > last_dropped_item_uid_) last_dropped_item_uid_ = item.Uid;
        spdlog::info("WorldManager: Added live object {} x{} at ({:.1f}, {:.1f}) [total: {}]", 
                    item.ItemId, item.Amount, item.X, item.Y, live_objects_.size());
    }
    
    void remove_dropped_item_by_uid(uint32_t uid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it1 = std::remove_if(items_.begin(), items_.end(),
            [uid](const world::DroppedItemInfo& item) { return item.Uid == uid; });
        if (it1 != items_.end()) {
            items_.erase(it1, items_.end());
            spdlog::debug("WorldManager: Removed dropped item by UID {} from items_ (remaining: {})", uid, items_.size());
        }

        auto it2 = std::remove_if(live_objects_.begin(), live_objects_.end(),
            [uid](const world::DroppedItemInfo& item) { return item.Uid == uid; });
        if (it2 != live_objects_.end()) {
            live_objects_.erase(it2, live_objects_.end());
            spdlog::debug("WorldManager: Removed live object UID {} (remaining: {})", uid, live_objects_.size());
        }
    }

    void remove_live_object(uint32_t uid) {
        remove_dropped_item_by_uid(uid);
    }
    
    void add_dropped_item(const world::DroppedItemInfo& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push_back(item);
        if (item.Uid > last_dropped_item_uid_) last_dropped_item_uid_ = item.Uid;
        spdlog::debug("WorldManager: Added dropped item {} (total: {})", item.ItemId, items_.size());
    }
    
    void remove_dropped_item(uint16_t item_id, float x, float y) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto pred = [item_id, x, y](const world::DroppedItemInfo& item) {
            return (item_id == 0 || item.ItemId == item_id) && 
                   std::abs(item.X - x) < 48.0f && 
                   std::abs(item.Y - y) < 48.0f;
        };
        auto it1 = std::remove_if(items_.begin(), items_.end(), pred);
        if (it1 != items_.end()) {
            items_.erase(it1, items_.end());
            spdlog::debug("WorldManager: Removed dropped item {} near ({:.0f}, {:.0f}) from items_", item_id, x, y);
        }

        auto it2 = std::remove_if(live_objects_.begin(), live_objects_.end(), pred);
        if (it2 != live_objects_.end()) {
            live_objects_.erase(it2, live_objects_.end());
            spdlog::debug("WorldManager: Removed dropped item {} near ({:.0f}, {:.0f}) from live_objects_", item_id, x, y);
        }
    }

    std::optional<world::DroppedItemInfo> get_dropped_item(uint32_t uid) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : live_objects_) {
            if (item.Uid == uid) return item;
        }
        for (const auto& item : items_) {
            if (item.Uid == uid) return item;
        }
        return std::nullopt;
    }

    std::optional<world::DroppedItemInfo> find_nearest_dropped_item(float x, float y, float max_dist = 96.0f) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::optional<world::DroppedItemInfo> best = std::nullopt;
        float best_dist_sq = max_dist * max_dist;

        auto check_list = [&](const std::vector<world::DroppedItemInfo>& list) {
            for (const auto& item : list) {
                float dx = item.X - x;
                float dy = item.Y - y;
                float d2 = dx * dx + dy * dy;
                if (d2 < best_dist_sq || (std::abs(d2 - best_dist_sq) < 16.0f && (!best || item.Uid > best->Uid))) {
                    best_dist_sq = d2;
                    best = item;
                }
            }
        };

        check_list(live_objects_);
        check_list(items_);
        return best;
    }

    std::optional<world::DroppedItemInfo> find_nearest_dropped_lock(float x, float y, float max_dist = 600.0f) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::optional<world::DroppedItemInfo> best = std::nullopt;
        float best_dist_sq = max_dist * max_dist;

        auto check_list = [&](const std::vector<world::DroppedItemInfo>& list) {
            for (const auto& item : list) {
                if (item.ItemId != 242 && item.ItemId != 1796 && item.ItemId != 7188) continue;
                float dx = item.X - x;
                float dy = item.Y - y;
                float d2 = dx * dx + dy * dy;
                if (d2 < best_dist_sq || (std::abs(d2 - best_dist_sq) < 16.0f && (!best || item.Uid > best->Uid))) {
                    best_dist_sq = d2;
                    best = item;
                }
            }
        };

        check_list(live_objects_);
        check_list(items_);
        return best;
    }

    bool has_dropped_item_at(uint32_t tile_x, uint32_t tile_y) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : live_objects_) {
            uint32_t ix = static_cast<uint32_t>(item.X / 32.0f);
            uint32_t iy = static_cast<uint32_t>(item.Y / 32.0f);
            if (ix == tile_x && iy == tile_y) return true;
        }
        for (const auto& item : items_) {
            uint32_t ix = static_cast<uint32_t>(item.X / 32.0f);
            uint32_t iy = static_cast<uint32_t>(item.Y / 32.0f);
            if (ix == tile_x && iy == tile_y) return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        has_world_ = false;
        server_reported_no_access_ = false;
        tiles_.clear();
        items_.clear();
        live_objects_.clear();
        last_dropped_item_uid_ = 0;
        world_v2_copy_.reset();
        spdlog::info("WorldManager: Cleared all data");
    }

private:
    WorldManager() : has_world_(false), width_(0), height_(0), server_reported_no_access_(false), last_dropped_item_uid_(0) {}
    ~WorldManager() = default;
    WorldManager(const WorldManager&) = delete;
    WorldManager& operator=(const WorldManager&) = delete;

    mutable std::mutex mutex_;
    bool has_world_;
    bool server_reported_no_access_;
    uint32_t width_, height_;
    std::string world_name_;
    std::vector<TileData> tiles_;
    std::vector<world::DroppedItemInfo> items_;      
    std::vector<world::DroppedItemInfo> live_objects_; 
    world_v2::World world_v2_copy_;
    uint32_t last_dropped_item_uid_ = 0;
};

} 
