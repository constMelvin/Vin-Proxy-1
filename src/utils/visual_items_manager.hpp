#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <glm/glm.hpp>
#include "../packet/tank_packet.hpp"
#include "../packet/packet_variant.hpp"

namespace player {
class Player;
}

namespace core {
class Core;
}

namespace utils {

class VisualItemsManager {
public:
    static VisualItemsManager& get_instance();

    // Master toggle
    bool is_enabled() const;
    void set_enabled(bool enabled);
    bool toggle_enabled();

    // Visual slots management (0=hat, 1=shirt, 2=pants, 3=shoes, 4=face, 5=hand, 6=back, 7=hair, 8=neck, 9=ances)
    void set_visual_item(int clothing_type, int item_id);
    bool toggle_visual_item(int clothing_type, int item_id);
    void clear_visual_items();
    bool is_item_equipped(int item_id) const;
    int get_equipped_item(int slot) const;
    const std::unordered_map<int, int>& get_visual_slots() const;
    const std::unordered_set<uint32_t>& get_all_visual_items() const;
    bool has_any_visual_equipped() const;

    // Punch effect mapping and status
    uint8_t get_punch_id(uint32_t item_id) const;
    uint8_t get_current_punch_effect() const;
    bool has_double_jump() const;

    // Native Growtopia Character State Transmission
    void send_character_state(player::Player* client_player, uint32_t net_id);
    void restore_character_state(player::Player* client_player, uint32_t net_id);
    void send_visual_clothing(player::Player* client_player, uint32_t net_id, core::Core* core = nullptr);

    // In-flight Packet Overrides
    void inject_spawn_clothing(std::string& spawn_data);
    bool patch_server_clothing(packet::Variant& variant, uint32_t net_id);
    void patch_server_character_state(packet::TankUpdatePacket* tank);

private:
    VisualItemsManager();
    ~VisualItemsManager() = default;
    VisualItemsManager(const VisualItemsManager&) = delete;
    VisualItemsManager& operator=(const VisualItemsManager&) = delete;

    void update_state_flags_locked();

    mutable std::mutex mutex_;
    bool is_enabled_{ true };
    std::unordered_map<int, int> visual_slots_;
    std::unordered_set<uint32_t> all_visual_items_;
    uint8_t current_punch_effect_{ 0 };
    bool double_jump_{ false };
};

} // namespace utils
