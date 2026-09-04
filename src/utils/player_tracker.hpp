#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace utils {

class PlayerTracker {
public:
    struct PlayerPosition {
        float x = 0.0f;
        float y = 0.0f;
        
        PlayerPosition() = default;
        PlayerPosition(float pos_x, float pos_y) : x(pos_x), y(pos_y) {}
        
        bool operator==(const PlayerPosition& other) const {
            return x == other.x && y == other.y;
        }
        
        bool operator!=(const PlayerPosition& other) const {
            return !(*this == other);
        }
    };

    struct PlayerInfo {
        uint32_t netID = 0;
        uint32_t userID = 0;
        std::string name;
        std::string country;
        std::string mac_address;
        std::string platform_id;
        std::string device_type;
        PlayerPosition position;
        bool is_local = false;
        uint32_t cloth_hand = 0;
        
        PlayerInfo() = default;
        
        
        PlayerInfo(uint32_t nid, uint32_t uid, const std::string& n, const std::string& c, bool local = false)
            : netID(nid), userID(uid), name(n), country(c), is_local(local) {}
    };

    static PlayerTracker& get_instance();

    void update_player_info(uint32_t netID, uint32_t userID, const std::string& name, 
                          const std::string& country, bool is_local);
    
    void update_player_position(uint32_t netID, float x, float y);
    void update_connection_info(uint32_t netID, const std::string& mac, const std::string& country);
    void update_platform_info(uint32_t netID, const std::string& platform_id);
    
    
    void update_player_name(uint32_t netID, const std::string& name);
    
    PlayerInfo get_local_player() const;
    PlayerInfo get_player_by_netid(uint32_t netID) const;
    PlayerPosition get_player_position(uint32_t netID) const;
    bool has_local_player() const;
    
    
    std::unordered_map<uint32_t, PlayerInfo> get_all_players() const;
    static std::string get_device_from_platform_id(const std::string& platform_id);
    
    struct ClothingInfo {
        int hat = 0, shirt = 0, pants = 0;
        int shoes = 0, face = 0, hand = 0;
        int back = 0, hair = 0, neck = 0;
        uint32_t skin_color = 2190853119;
        int ances = 0;
    };

    void update_clothing(int hat, int shirt, int pants, int shoes, int face, int hand, int back, int hair, int neck, uint32_t skin, int ances) {
        std::lock_guard<std::mutex> lock(mutex_);
        clothing_.hat = hat; clothing_.shirt = shirt; clothing_.pants = pants;
        clothing_.shoes = shoes; clothing_.face = face; clothing_.hand = hand;
        clothing_.back = back; clothing_.hair = hair; clothing_.neck = neck;
        clothing_.skin_color = skin; clothing_.ances = ances;
        if (local_player_netid_ > 0 && players_.count(local_player_netid_)) {
            players_[local_player_netid_].cloth_hand = (hand > 0) ? static_cast<uint32_t>(hand) : 0;
        }
    }

    void update_clothing_slot(int slot, int item_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (slot) {
            case 0: clothing_.hat = item_id; break;
            case 1: clothing_.shirt = item_id; break;
            case 2: clothing_.pants = item_id; break;
            case 3: clothing_.shoes = item_id; break;
            case 4: clothing_.face = item_id; break;
            case 5: clothing_.hand = item_id; break;
            case 6: clothing_.back = item_id; break;
            case 7: clothing_.hair = item_id; break;
            case 8: clothing_.neck = item_id; break;
            case 9: clothing_.ances = item_id; break;
            default: break;
        }
        if (slot == 5) {
            if (local_player_netid_ > 0 && players_.count(local_player_netid_)) {
                players_[local_player_netid_].cloth_hand = (item_id > 0) ? static_cast<uint32_t>(item_id) : 0;
            }
        }
    }

    ClothingInfo get_clothing() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return clothing_;
    }

    void clear();

private:
    PlayerTracker() = default;
    ~PlayerTracker() = default;

    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, PlayerInfo> players_; 
    uint32_t local_player_netid_ = 0;
    ClothingInfo clothing_;
};

} 
