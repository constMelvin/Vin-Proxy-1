#pragma once
#include "../core/core.hpp"
#include "../packet/packet_variant.hpp"
#include "../packet/packet_types.hpp"
#include "../utils/byte_stream.hpp"
#include "../utils/world_manager.hpp"
#include "../utils/player_tracker.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <mutex>
#include <set>
#include <string>
#include <atomic>
#include <cmath>

namespace utils {

class GemsManager {
public:
    static GemsManager& get_instance() {
        static GemsManager instance;
        return instance;
    }

    // ─── Setting Flags (toggled by /gems dialog) ────────────────────────
    bool collected_gems   = false;   // Show collected gems count (local bubble)
    bool show_gems_to_others = false; // Broadcast collected gems to others via chat
    bool instant_gems     = false;   // Show gem count on tiles when gems are dropped
    bool punch_tile_gems  = false;   // Show gem count when punching a tile
    
    // ─── Runtime State ──────────────────────────────────────────────────
    bool gems_drop        = false;   // Flag: a gem drop event occurred, needs display
    int  instant_drop_x   = 0;       // Tile X of the last gem drop
    int  instant_drop_y   = 0;       // Tile Y of the last gem drop

    // ─── Color Scheme (matching Lucky Proxy) ────────────────────────────
    std::string prim_color = "`2";
    std::string seco_color = "`9";

    // ─── Gem Collection Tracking (Instant Bubble & Broadcast) ───────────
    void on_gems_collected(core::Core* core, int amount, uint32_t net_id = 0) {
        if (amount <= 0 || !core) return;

        const bool should_display_local = collected_gems;
        const bool should_broadcast     = show_gems_to_others;
        if (!should_display_local && !should_broadcast) return;

        auto* server = core->get_server();
        if (!server || !server->get_player()) return;

        if (net_id == 0) {
            auto local = PlayerTracker::get_instance().get_local_player();
            net_id = (local.netID > 0) ? local.netID : 0;
        }

        auto now = std::chrono::steady_clock::now();
        int display_amount = 0;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto ms_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_gain_time_).count();
            if (ms_since_last > 800) {
                accumulated_gems_ = 0;
            }
            accumulated_gems_ += amount;
            last_gain_time_ = now;
            display_amount = accumulated_gems_;
        }

        std::string message = seco_color + "Collected " + prim_color + std::to_string(display_amount) + (display_amount == 1 ? "`` Gem" : "`` Gems");

        // 1. Local speech bubble (only if NOT broadcasting to avoid double bubbles, matching Lucky Proxy)
        if (should_display_local && !should_broadcast) {
            bool should_send_bubble = false;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                const auto ms_since_last_bubble = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_bubble_time_).count();
                if (message != last_bubble_msg_ || ms_since_last_bubble > 300) {
                    last_bubble_msg_ = message;
                    last_bubble_time_ = now;
                    should_send_bubble = true;
                }
            }

            if (should_send_bubble) {
                packet::Variant bubble_var{};
                bubble_var.add("OnTalkBubble");
                bubble_var.add(static_cast<int>(net_id));
                bubble_var.add(message);
                bubble_var.add(0);
                bubble_var.add(0);

                std::vector<std::byte> ext_data = bubble_var.serialize();
                packet::GameUpdatePacket pkt{};
                pkt.type = packet::PACKET_CALL_FUNCTION;
                pkt.net_id = -1;
                pkt.flags.extended = 1;
                pkt.data_size = static_cast<uint32_t>(ext_data.size());

                ByteStream<std::uint16_t> bs{};
                bs.write(packet::NET_MESSAGE_GAME_PACKET);
                bs.write(pkt);
                bs.write_data(ext_data.data(), ext_data.size());
                server->get_player()->send_packet(bs.get_data(), 0);
            }
        }

        // 2. Instant broadcast to other players in the world via chat
        if (should_broadcast) {
            auto* client = core->get_client();
            if (client && client->get_player()) {
                auto ms_since_broadcast = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_broadcast_time_).count();
                if (ms_since_broadcast > 1000 && !broadcast_timer_running_) {
                    // Over 1 second since last chat: send INSTANTLY!
                    last_broadcast_time_ = now;
                    std::string chat_msg = "action|input\ntext|" + message + "\n";
                    ByteStream<std::uint16_t> bs{};
                    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
                    bs.write_data(chat_msg.c_str(), chat_msg.size() + 1);
                    client->get_player()->send_packet(bs.get_data(), 0);
                } else {
                    // Rapid pickup: debounce follow-up chat by 350ms so GT server does not mute for spam
                    queued_broadcast_amount_ = display_amount;
                    if (!broadcast_timer_running_) {
                        broadcast_timer_running_ = true;
                        std::thread([this, core]() {
                            std::this_thread::sleep_for(std::chrono::milliseconds(350));
                            int total = queued_broadcast_amount_;
                            queued_broadcast_amount_ = 0;
                            broadcast_timer_running_ = false;
                            if (total > 0 && show_gems_to_others) {
                                auto* cl = core->get_client();
                                if (cl && cl->get_player()) {
                                    std::string follow_msg = "action|input\ntext|" + seco_color + "Collected " + prim_color + std::to_string(total) + (total == 1 ? "`` Gem" : "`` Gems") + "\n";
                                    ByteStream<std::uint16_t> bs{};
                                    bs.write(packet::NET_MESSAGE_GENERIC_TEXT);
                                    bs.write_data(follow_msg.c_str(), follow_msg.size() + 1);
                                    cl->get_player()->send_packet(bs.get_data(), 0);
                                    last_broadcast_time_ = std::chrono::steady_clock::now();
                                }
                            }
                        }).detach();
                    }
                }
            }
        }
    }

    void register_gem_gain(int amount) {
        if (amount <= 0) return;
        std::lock_guard<std::mutex> lk(mtx_);
        tracked_gem_total_ += amount;
    }

    void update_collected_gems_display(core::Core* core) {
        // Handled directly and instantly by on_gems_collected
    }

    bool was_recently_collected(int ms_window = 300) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_gain_time_).count();
        return ms < ms_window && accumulated_gems_ > 0;
    }

    // ─── Display gem count on a tile using OnParticleEffect ─────────────
    void display_gems_in_tile(core::Core* core, int tile_x, int tile_y) {
        if (!core) return;
        if (tile_x < 0 || tile_y < 0) return;

        auto& world_mgr = WorldManager::get_instance();
        int total_gems_found = 0;

        auto all_items = world_mgr.get_all_dropped_items();
        for (const auto& obj : all_items) {
            if (obj.ItemId != 112)
                continue;
            int obj_tile_x = static_cast<int>((obj.X + 10.0f) / 32.0f);
            int obj_tile_y = static_cast<int>((obj.Y + 10.0f) / 32.0f);
            int obj_tile_x_raw = static_cast<int>(obj.X / 32.0f);
            int obj_tile_y_raw = static_cast<int>(obj.Y / 32.0f);
            if ((obj_tile_x == tile_x && obj_tile_y == tile_y) ||
                (obj_tile_x_raw == tile_x && obj_tile_y_raw == tile_y)) {
                total_gems_found += obj.Amount;
            }
        }

        if (total_gems_found <= 0)
            return;

        const int center_x = tile_x * 32 + 16;
        const int center_y = tile_y * 32 + 16;
        send_gem_particle_effect(core, center_x, center_y, total_gems_found);
    }

    // ─── Instant gem drop update (call periodically or triggered directly) ─
    void queue_instant_gem_drop(core::Core* core, int tile_x, int tile_y) {
        if (!instant_gems || !core || tile_x < 0 || tile_y < 0) return;

        // Display immediately on the first drop so visual response is instant
        display_gems_in_tile(core, tile_x, tile_y);

        // Schedule background follow-up after 120ms to catch all gems from bursts (tree harvests / block breaking)
        uint64_t key = (static_cast<uint64_t>(tile_x) << 32) | static_cast<uint32_t>(tile_y);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto now = std::chrono::steady_clock::now();
            auto it = pending_drop_tiles_.find(key);
            if (it != pending_drop_tiles_.end()) {
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
                if (ms < 120) {
                    return; // Already debounced for this tile
                }
            }
            pending_drop_tiles_[key] = now;
        }

        std::thread([this, core, tile_x, tile_y, key]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            display_gems_in_tile(core, tile_x, tile_y);
            std::lock_guard<std::mutex> lk(mtx_);
            pending_drop_tiles_.erase(key);
        }).detach();
    }

    void update_instant_gem_drop(core::Core* core) {
        if (!instant_gems) {
            gems_drop = false;
            return;
        }
        if (!core) return;

        auto now = std::chrono::steady_clock::now();
        auto ms_since = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_instant_gem_check_time_).count();
        if (ms_since < 325) return;
        last_instant_gem_check_time_ = now;

        if (!gems_drop) return;

        display_gems_in_tile(core, instant_drop_x, instant_drop_y);
        gems_drop = false;
    }

    // ─── World change reset ─────────────────────────────────────────────
    void on_world_change() {
        std::lock_guard<std::mutex> lk(mtx_);
        tracked_gem_total_  = 0;
        queued_gem_gain_    = 0;
        previous_gem_count_ = 0;
        gems_drop           = false;
        pending_drop_tiles_.clear();
    }

    // ─── Send particle effect to client ─────────────────────────────────
    void send_gem_particle_effect(core::Core* core, int pixel_x, int pixel_y, int gem_amount) {
        if (!core || gem_amount <= 0) return;

        auto* server = core->get_server();
        if (!server || !server->get_player()) return;

        packet::Variant effect_var{};
        effect_var.add("OnParticleEffect");
        effect_var.add(static_cast<uint32_t>(181));  // Particle ID 181 (floating count/number effect)
        effect_var.add(glm::vec2(static_cast<float>(pixel_x), static_cast<float>(pixel_y)));
        effect_var.add(static_cast<float>(gem_amount)); // The number rendered by Growtopia (e.g. 14)
        effect_var.add(0.0f);

        std::vector<std::byte> ext_data = effect_var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = -1;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());

        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());
        server->get_player()->send_packet(bs.get_data(), 0);

        spdlog::debug("[GemsManager] Particle effect at ({},{}) gems={}", pixel_x, pixel_y, gem_amount);
    }

private:
    GemsManager() = default;
    ~GemsManager() = default;
    GemsManager(const GemsManager&) = delete;
    GemsManager& operator=(const GemsManager&) = delete;

    std::mutex mtx_;
    int  tracked_gem_total_  = 0;
    int  queued_gem_gain_    = 0;
    int  previous_gem_count_ = 0;

    std::chrono::steady_clock::time_point last_gem_update_time_       = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_gem_gain_timestamp_    = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_instant_gem_check_time_= std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_bubble_time_           = std::chrono::steady_clock::now();
    std::string last_bubble_msg_;
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> pending_drop_tiles_;

    std::chrono::steady_clock::time_point last_gain_time_             = std::chrono::steady_clock::now();
    int accumulated_gems_                                             = 0;
    std::chrono::steady_clock::time_point last_broadcast_time_        = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    int queued_broadcast_amount_                                      = 0;
    std::atomic<bool> broadcast_timer_running_{false};
};

} // namespace utils
