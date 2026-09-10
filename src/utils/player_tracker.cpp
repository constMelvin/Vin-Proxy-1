#include "player_tracker.hpp"
#include "world_manager.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <cctype>

namespace utils {

PlayerTracker& PlayerTracker::get_instance() {
    static PlayerTracker instance;
    return instance;
}

void PlayerTracker::update_player_info(uint32_t netID, uint32_t userID, const std::string& name, 
                                     const std::string& country, bool is_local) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    PlayerInfo info{};
    auto it = players_.find(netID);
    if (it != players_.end()) {
        info = it->second; 
    }

    info.netID = netID;
    if (userID != 0) {
        info.userID = userID;
    }
    if (!name.empty()) {
        std::string raw = name;
        raw.erase(std::remove(raw.begin(), raw.end(), '\''), raw.end());
        raw.erase(std::remove(raw.begin(), raw.end(), '"'), raw.end());
        if (raw.size() >= 2 && raw[0] == '`') {
            info.name_color = raw.substr(0, 2);
        }
        std::string clean;
        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '`') {
                if (i + 1 < raw.size()) ++i;
                continue;
            }
            clean.push_back(raw[i]);
        }
        clean.erase(0, clean.find_first_not_of(" \t\r\n"));
        clean.erase(clean.find_last_not_of(" \t\r\n") + 1);
        info.name = clean.empty() ? raw : clean;
    }
    if (!country.empty()) {
        info.country = country;
    }
    info.is_local = is_local;
    
    players_[netID] = info;
    
    if (is_local) {
        local_player_netid_ = netID;
        if (userID != 0) {
            local_player_userid_ = userID;
        }
        spdlog::info("Local player tracked - NetID: {}, UserID: {}, Name: {}, Country: {}", 
                    netID, userID, info.name, country);
    } else {
        spdlog::debug("Player tracked - NetID: {}, UserID: {}, Name: {}", 
                     netID, userID, info.name);
    }
}

void PlayerTracker::update_player_position(uint32_t netID, float x, float y) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = players_.find(netID);
    if (it != players_.end()) {
        PlayerPosition new_position(x, y);
        
        
        if (it->second.position != new_position) {
            it->second.position = new_position;
            spdlog::debug("Player {} position updated: X={}, Y={}", netID, x, y);
        }
    } else {
        
        PlayerInfo info;
        info.netID = netID;
        info.name = fmt::format("Player_{}", netID); 
        info.position = PlayerPosition(x, y);
        players_[netID] = info;
        spdlog::debug("Auto-tracked player {} at position X={}, Y={}", netID, x, y);
    }
}

void PlayerTracker::update_connection_info(uint32_t netID, const std::string& mac, const std::string& country) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = players_.find(netID);
    if (it != players_.end()) {
        it->second.mac_address = mac;
        if (!country.empty()) {
            it->second.country = country;
        }
        spdlog::debug("Updated connection info for netID {}: MAC={}, Country={}", 
                     netID, mac, country);
    } else {
        spdlog::warn("Cannot update connection info: netID {} not found in tracker", netID);
    }
}

void PlayerTracker::update_platform_info(uint32_t netID, const std::string& platform_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = players_.find(netID);
    if (it != players_.end()) {
        it->second.platform_id = platform_id;
        it->second.device_type = get_device_from_platform_id(platform_id);
        spdlog::debug("Updated platform info for netID {}: {} -> {}",
                     netID, platform_id, it->second.device_type);
    } else {
        PlayerInfo info{};
        info.netID = netID;
        info.platform_id = platform_id;
        info.device_type = get_device_from_platform_id(platform_id);
        players_[netID] = info;
        spdlog::debug("Auto-tracked player {} with platform {} ({})",
                     netID, platform_id, info.device_type);
    }
}

PlayerTracker::PlayerInfo PlayerTracker::get_local_player() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (local_player_netid_ == 0) {
        return PlayerInfo{};
    }
    
    auto it = players_.find(local_player_netid_);
    if (it != players_.end()) {
        auto info = it->second;
        if (info.cloth_hand == 0 && clothing_.hand > 0) {
            info.cloth_hand = static_cast<uint32_t>(clothing_.hand);
        }
        return info;
    }
    
    return PlayerInfo{};
}

PlayerTracker::PlayerInfo PlayerTracker::get_player_by_netid(uint32_t netID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = players_.find(netID);
    if (it != players_.end()) {
        return it->second;
    }
    
    return PlayerInfo{};
}

PlayerTracker::PlayerPosition PlayerTracker::get_player_position(uint32_t netID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = players_.find(netID);
    if (it != players_.end()) {
        return it->second.position;
    }
    
    return PlayerPosition{};
}

bool PlayerTracker::has_local_player() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return local_player_netid_ != 0;
}

void PlayerTracker::update_player_name(uint32_t netID, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string raw = name;
    raw.erase(std::remove(raw.begin(), raw.end(), '\''), raw.end());
    raw.erase(std::remove(raw.begin(), raw.end(), '"'), raw.end());
    std::string detected_color;
    if (raw.size() >= 2 && raw[0] == '`') {
        detected_color = raw.substr(0, 2);
    }
    std::string clean;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '`') {
            if (i + 1 < raw.size()) ++i;
            continue;
        }
        clean.push_back(raw[i]);
    }
    clean.erase(0, clean.find_first_not_of(" \t\r\n"));
    clean.erase(clean.find_last_not_of(" \t\r\n") + 1);
    std::string final_name = clean.empty() ? raw : clean;

    auto it = players_.find(netID);
    if (it != players_.end()) {
        it->second.name = final_name;
        if (!detected_color.empty()) {
            it->second.name_color = detected_color;
        }
        spdlog::debug("Updated player {} name to '{}'", netID, final_name);
    } else {
        PlayerInfo info;
        info.netID = netID;
        info.name = final_name;
        if (!detected_color.empty()) {
            info.name_color = detected_color;
        }
        players_[netID] = info;
        spdlog::debug("Auto-tracked player {} with name '{}'", netID, final_name);
    }
}

void PlayerTracker::update_player_color(uint32_t netID, const std::string& color) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = players_.find(netID);
    if (it != players_.end()) {
        it->second.name_color = color;
    }
}

std::string PlayerTracker::get_player_color(uint32_t netID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t target_netid = (netID == 0) ? local_player_netid_ : netID;
    auto it = (target_netid != 0) ? players_.find(target_netid) : players_.end();
    
    // 1. Role color from server (e.g. Mod `4 / `@, Developer `b, Legend `6)
    if (it != players_.end()) {
        const auto& pinfo = it->second;
        if (!pinfo.name_color.empty() && 
            pinfo.name_color != "`w" && 
            pinfo.name_color != "`0" && 
            pinfo.name_color != "`2" &&
            pinfo.name_color != "`^") {
            return pinfo.name_color;
        }
    }
    
    // 2. Main lock access check from server world data
    auto& wm = WorldManager::get_instance();
    if (wm.is_server_reported_no_access()) {
        return "`w";
    }

    uint32_t uid = 0;
    if (it != players_.end()) {
        uid = it->second.userID;
        if (it->second.is_local && uid == 0) {
            uid = local_player_userid_;
        }
    } else if (target_netid == local_player_netid_ || target_netid == 0) {
        uid = local_player_userid_;
    }

    if (uid > 0) {
        if (wm.is_world_owner(uid)) {
            return "`2"; // World owner is `2 (Green)
        }
        if (wm.has_world_admin_access(uid)) {
            return "`^"; // Access on main lock is `^ (Light Green)
        }
    }

    if (it != players_.end()) {
        if (it->second.name_color == "`2") return "`2";
        if (it->second.name_color == "`^") return "`^";
    }

    return "`w"; // Rest is white
}

std::string PlayerTracker::get_player_tag(uint32_t netID, const std::string& fallback_name) const {
    std::string name;
    uint32_t target_netid = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        target_netid = (netID == 0) ? local_player_netid_ : netID;
        if (target_netid != 0) {
            auto it = players_.find(target_netid);
            if (it != players_.end() && !it->second.name.empty()) {
                name = it->second.name;
            }
        }
    }

    if (name.empty()) {
        name = fallback_name;
    }
    if (name.empty()) {
        name = (netID == 0 || (local_player_netid_ != 0 && netID == local_player_netid_)) ? "You" : "Someone";
    }

    std::string clean_name;
    for (size_t i = 0; i < name.size(); ++i) {
        if (name[i] == '`') {
            if (i + 1 < name.size()) ++i;
            continue;
        }
        if (name[i] != '\'' && name[i] != '"') {
            clean_name.push_back(name[i]);
        }
    }
    clean_name.erase(0, clean_name.find_first_not_of(" \t\r\n"));
    clean_name.erase(clean_name.find_last_not_of(" \t\r\n") + 1);
    if (clean_name.empty()) {
        clean_name = fallback_name.empty() ? "You" : fallback_name;
    }

    // Color from server data: `2 (green) if owner/access on main lock, `w (white) if no access
    std::string color = get_player_color(target_netid);
    return fmt::format("<{}{}``>", color, clean_name);
}

std::unordered_map<uint32_t, PlayerTracker::PlayerInfo> PlayerTracker::get_all_players() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return players_;
}

std::string PlayerTracker::get_device_from_platform_id(const std::string& platform_id) {
    std::string normalized = platform_id;
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), [](unsigned char c) {
        return std::isspace(c);
    }), normalized.end());

    if (normalized == "0,1,1") {
        return "Windows";
    }
    if (normalized == "1,0,0" || normalized == "1,1,0") {
        return "Android";
    }
    if (normalized == "2,0,0" || normalized == "2,1,0") {
        return "iOS";
    }
    return "Unknown";
}

void PlayerTracker::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    players_.clear();
    local_player_netid_ = 0;
}

} 
