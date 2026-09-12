#pragma once
#include "../core/core.hpp"
#include "../server/server.hpp"
#include "../player/player.hpp"
#include "../packet/packet_helper.hpp"
#include "../packet/packet_variant.hpp"
#include "../packet/packet_types.hpp"
#include "player_tracker.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <unordered_set>

namespace utils {

class DisplayManager {
public:
    
    static std::string build_display_name(core::Core* core, const std::string& base_name, uint32_t netID = 0) {
        if (!core) return base_name;
        
        std::string display_name = base_name;
        
        std::string saved_visual_name = core->get_config().get<std::string>("display.visual_name");
        bool show_ping = core->get_config().get<bool>("display.show_ping");
        
        bool has_legend = false;
        try { has_legend = core->get_config().get<bool>("display.title.legend"); } catch (...) {}

        bool has_super_supporter = false;
        try { has_super_supporter = core->get_config().get<bool>("display.title.super_supporter"); } catch (...) {}

        // Strip existing Dr. prefix and of Legend suffix if present
        if (display_name.size() >= 2 && display_name.substr(display_name.size() - 2) == "``") {
            display_name = display_name.substr(0, display_name.size() - 2);
        }
        size_t legend_pos = display_name.find(" of Legend");
        if (legend_pos != std::string::npos) {
            size_t len = 10;
            if (legend_pos + 12 <= display_name.size() && display_name.substr(legend_pos + 10, 2) == "``") {
                len = 12;
            }
            display_name.erase(legend_pos, len);
        }
        if (display_name.rfind("``", 0) == 0) {
            display_name = display_name.substr(2);
        }
        static const std::string kDrPrefix = "`9Dr.``";
        if (display_name.rfind(kDrPrefix, 0) == 0) {
            display_name = display_name.substr(kDrPrefix.size());
        } else if (display_name.rfind("Dr.", 0) == 0) {
            display_name = display_name.substr(3);
        }

        if (!saved_visual_name.empty()) {
            display_name = saved_visual_name;
        } else {
            std::string player_color = utils::PlayerTracker::get_instance().get_player_color(netID);
            if (has_super_supporter && (player_color == "`w" || player_color.empty())) {
                player_color = "`1"; 
            }
            if (display_name.size() >= 2 && display_name[0] == '`') {
                display_name = display_name.substr(2);
            }
            if (has_legend) {
                display_name = "``" + display_name;
            } else {
                display_name = player_color + display_name;
            }
        }
        
        bool has_dr = core->get_config().get<bool>("display.title.dr");
        if (has_dr) {
            display_name = "`9Dr.``" + display_name;
        }

        if (show_ping) {
            auto* server = core->get_server();
            if (server && server->get_player() && server->get_player()->get_peer()) {
                int ping_ms = static_cast<int>(server->get_player()->get_peer()->roundTripTime);
                
                std::string ping_color = "`2"; 
                if (ping_ms > 150) {
                    ping_color = "`6"; 
                }
                if (ping_ms > 300) {
                    ping_color = "`4"; 
                }
                display_name = fmt::format("`0[{}{}``] {}", ping_color, ping_ms, display_name);
            }
        }

        bool show_last_spin = true;
        try {
            show_last_spin = core->get_config().get<bool>("features.host.show_last_spin");
        } catch (...) {
            try {
                show_last_spin = core->get_config().get<bool>("display.show_last_spin");
            } catch (...) {}
        }

        if (!has_legend && show_last_spin && utils::PlayerTracker::get_instance().has_last_spin(netID)) {
            int val = utils::PlayerTracker::get_instance().get_last_spin(netID);
            auto get_roulette_color_code = [](int spin) -> std::string {
                if (spin == 0) return "`2";
                static const std::unordered_set<int> red_numbers = {
                    1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36
                };
                if (red_numbers.count(spin) > 0) return "`4";
                return "`b";
            };
            std::string color = get_roulette_color_code(val);
            display_name += " `w[" + color + std::to_string(val) + "`w]``";
        }

        if (has_legend) {
            display_name += " of Legend``";
        }
        
        return display_name;
    }
    
    static void apply_display_name(core::Core* core, uint32_t netID, const std::string& base_name, bool apply_titles = true) {
        if (!core) return;
        if (netID == 0 || base_name.empty() || base_name == "Unknown") return;
        
        auto* server = core->get_server();
        if (!server || !server->get_player() || !server->get_player()->is_connected()) {
            spdlog::warn("DisplayManager: Server not available");
            return;
        }
        
        std::string display_name = build_display_name(core, base_name, netID);
        
        try {
            
            packet::Variant variant{};
            variant.add("OnNameChanged");
            variant.add(display_name);
            
            std::vector<std::byte> ext_data = variant.serialize();
            
            packet::GameUpdatePacket game_packet{};
            game_packet.type = packet::PACKET_CALL_FUNCTION;
            game_packet.net_id = netID;
            game_packet.flags.extended = 1;
            game_packet.data_size = static_cast<uint32_t>(ext_data.size());
            
            ByteStream<std::uint16_t> byte_stream{};
            byte_stream.write(packet::NET_MESSAGE_GAME_PACKET);
            byte_stream.write(game_packet);
            byte_stream.write_data(ext_data.data(), ext_data.size());
            
            server->get_player()->send_packet(byte_stream.get_data(), 0);
            
            spdlog::debug("DisplayManager: Applied display name '{}' for netID {}", display_name, netID);
            
            if (apply_titles) {
                apply_title_effects(core, netID);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("DisplayManager: Failed to apply display name: {}", e.what());
        }
    }
    
    
    static void apply_title_effects(core::Core* core, uint32_t netID) {
        if (!core) return;
        
        auto* server = core->get_server();
        if (!server || !server->get_player() || !server->get_player()->is_connected()) {
            return;
        }
        
        bool has_g4g = core->get_config().get<bool>("display.title.g4g");
        bool has_maxlevel = core->get_config().get<bool>("display.title.maxlevel");
        bool has_dr = core->get_config().get<bool>("display.title.dr");
        bool has_mentor = core->get_config().get<bool>("display.title.mentor");
        
        bool has_legend = false;
        bool has_super_supporter = false;
        try { has_legend = core->get_config().get<bool>("display.title.legend"); } catch (...) {}
        try { has_super_supporter = core->get_config().get<bool>("display.title.super_supporter"); } catch (...) {}
        
        // When Legendary Title is active, sending OnCountryState resets the country flag
        // and destroys the native Growtopia Legendary Orb!
        if (has_legend) {
            spdlog::debug("DisplayManager: Skipping OnCountryState because Legendary Title is active");
            return;
        }

        bool has_any_title = has_g4g || has_maxlevel || has_dr || has_mentor || has_super_supporter;
        if (!has_any_title) {
            return;
        }
        
        std::string country = PlayerTracker::get_instance().get_local_player().country;
        if (country.empty()) country = "us";
        
        std::string country_state = country;
        if (has_g4g) country_state += "|donor";
        if (has_maxlevel) country_state += "|maxLevel";
        if (has_dr) country_state += "|doctor";
        if (has_mentor) country_state += "|master";
        if (has_super_supporter) country_state += "|superSupporter";
        
        try {
            send_title_packet(server, netID, country_state);
            spdlog::debug("DisplayManager: Applied country state '{}' for netID {}", country_state, netID);
        } catch (const std::exception& e) {
            spdlog::error("DisplayManager: Failed to apply title effects: {}", e.what());
        }
    }
    
    static void send_title_packet(server::Server* server, uint32_t netID, const std::string& country_state) {
        packet::Variant var{};
        var.add("OnCountryState");
        var.add(country_state);
        
        std::vector<std::byte> ext_data = var.serialize();
        packet::GameUpdatePacket pkt{};
        pkt.type = packet::PACKET_CALL_FUNCTION;
        pkt.net_id = netID;
        pkt.flags.extended = 1;
        pkt.data_size = static_cast<uint32_t>(ext_data.size());
        
        ByteStream<std::uint16_t> bs{};
        bs.write(packet::NET_MESSAGE_GAME_PACKET);
        bs.write(pkt);
        bs.write_data(ext_data.data(), ext_data.size());
        
        server->get_player()->send_packet(bs.get_data(), 0);
    }
    
    
    static void update_display(core::Core* core) {
        if (!core) return;
        
        auto& player_tracker = PlayerTracker::get_instance();
        auto player_info = player_tracker.get_local_player();
        
        if (player_info.netID == 0 || player_info.name.empty() || player_info.name == "Unknown") return;
        
        apply_display_name(core, player_info.netID, player_info.name, false);
    }
};

} 
