
#include "autocollect_command.hpp"
#include "autofarm_common.hpp"
#include "../../utils/packet_utils.hpp"
#include "../../utils/text_parse.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../player/player.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/inventory_manager.hpp"
#include <thread>
#include <chrono>
#include <sstream>
#include <cmath>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace command {

core::Core*              AutoCollectCommand::s_core     = nullptr;
std::atomic<bool>        AutoCollectCommand::s_running  {false};
std::atomic<uint64_t>    AutoCollectCommand::s_generation{0};
std::atomic<int>         AutoCollectCommand::s_range_tiles{AutoCollectCommand::DEFAULT_RANGE_TILES};



// Same "[VinProxy Premium]" prefix as every other console message
static void send_console(player::Player* p, const std::string& msg) {
    if (!p) return;
    utils::PacketUtils::send_chat_message(p, msg);
}





#include "../../utils/player_tracker.hpp"

void AutoCollectCommand::send_collect_packet(player::Player* to_server, uint32_t uid, float x, float y) {
    if (!to_server) return;

    uint8_t buf[60] = {};
    int off = 0;
    auto w1  = [&](uint8_t  v){ buf[off++] = v; };
    auto w4u = [&](uint32_t v){ memcpy(buf+off,&v,4); off+=4; };
    auto w4i = [&](int32_t  v){ memcpy(buf+off,&v,4); off+=4; };
    auto w4f = [&](float    v){ memcpy(buf+off,&v,4); off+=4; };

    w1(11);    
    w1(0);     
    w1(0);     
    w1(0);     
    
    int netid = -1;
    if (AutoCollectCommand::s_core) {
        try {
            netid = AutoCollectCommand::s_core->get_config().get<int>("player.netid");
        } catch (...) {}
    }
    if (netid <= 0) {
        auto local = utils::PlayerTracker::get_instance().get_local_player();
        if (local.netID != 0) {
            netid = static_cast<int>(local.netID);
        } else {
            netid = -1;
        }
    }

    w4i(netid); 
    w4i(0);    
    w4u(0);    
    w4f(0.f);  
    w4u(uid);  
    w4f(x);    
    w4f(y);    
    w4f(0.f);  
    w4f(0.f);  
    w4f(0.f);  
    w4i(0);    
    w4i(0);    
    w4u(0);    

    std::vector<std::byte> wire(60);
    uint32_t msg = 4;
    memcpy(wire.data(),    &msg, 4);
    memcpy(wire.data()+4,  buf, 56);

    to_server->send_packet(wire, 0);
    spdlog::info("[AutoCollect] Network Send: Collect UID {} using NetID {}", uid, netid);
}



void AutoCollectCommand::notify_item_drop(float x, float y) {}

AutoCollectCommand::AutoCollectCommand() : CommandBase(
    {"autocollect", "ac"}, {}, "Toggle auto-collect dropped items", 0
) {}

std::unique_ptr<CommandBase> AutoCollectCommand::clone() const {
    return std::make_unique<AutoCollectCommand>(*this);
}

void AutoCollectCommand::set_core(core::Core* core) { s_core = core; }

void AutoCollectCommand::stop() {
    s_running  = false;
    s_generation.fetch_add(1);
}

// /autocollect and /ac: quick toggle
void AutoCollectCommand::execute(client::Client* , const std::vector<std::string>& ) {
    if (!s_core) return;

    auto* server = s_core->get_server();
    if (!server || !server->get_player()) return;

    set_enabled(server->get_player(), !s_running.load());
}

void AutoCollectCommand::set_enabled(player::Player* player, bool enable) {
    if (enable == s_running.load()) return;

    if (!enable) {
        s_running = false;
        s_generation.fetch_add(1);
        send_console(player, "`4AutoCollect stopped.");
        return;
    }

    s_running = true;
    const uint64_t gen = ++s_generation;
    send_console(player, fmt::format("`2AutoCollect started. `9Range: `2{} `9tiles.", s_range_tiles.load()));
    std::thread([gen]() { run_autocollect(gen); }).detach();
}

// /collect: Auto Collect page
void AutoCollectCommand::show_dialog(player::Player* player) {
    const int range = s_range_tiles.load();
    std::ostringstream d;
    d << "set_default_color|`o\n";
    d << "add_label_with_icon|big|`9Auto Collect Page|left|112|\n";
    d << "add_spacer|small|\n";
    d << "add_checkbox|enable_auto_collect|`2Enable Auto Collect|" << (s_running.load() ? 1 : 0) << "|\n";
    d << command::autofarm::desc_text("`oToggle this to start or stop collecting floating items. /ac and /autocollect do the same.");
    d << "add_text_input|auto_collect_range|`cCollect Range (tiles): |" << range << "|3|\n";
    d << command::autofarm::tiny_text("`9How far from you floating items are collected. 1 tile = 32 pixels. Allowed "
        + std::to_string(MIN_RANGE_TILES) + "-" + std::to_string(MAX_RANGE_TILES) + " (100 covers a whole 100-wide world).");
    d << command::autofarm::tiny_text("`9Current range: `2" + std::to_string(range) + " tiles `9(" + std::to_string(range * 32) + " pixels)");
    d << "add_spacer|small|\n";
    d << "end_dialog|auto_collect_page|Cancel|OK|\n";
    command::autofarm::send_dialog(player, d.str());
}

void AutoCollectCommand::handle_dialog_response(player::Player* player, const std::string& raw) {
    TextParse tp{raw};
    int v = 0;
    if (command::autofarm::parse_int(tp.get("auto_collect_range"), v)) {
        const int clamped = std::clamp(v, MIN_RANGE_TILES, MAX_RANGE_TILES);
        s_range_tiles = clamped;
        if (clamped != v)
            send_console(player, fmt::format("`9Range must be {}-{} tiles, set to `2{}`9.", MIN_RANGE_TILES, MAX_RANGE_TILES, clamped));
        else
            send_console(player, fmt::format("`9Collect range set to `2{} `9tiles.", clamped));
    }
    set_enabled(player, command::autofarm::checkbox_on(tp, "enable_auto_collect"));
}

void AutoCollectCommand::run_autocollect(uint64_t generation) {
    constexpr auto  INTERVAL = std::chrono::milliseconds(500);

    spdlog::info("[AutoCollect] started gen={}", generation);
    
    
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> collection_times;

    while (s_running.load() && generation == s_generation.load()) {

        
        if (!s_core) break;
        player::Player* to_server = s_core->get_client()  ? s_core->get_client()->get_player()  : nullptr;
        player::Player* to_local  = s_core->get_server()  ? s_core->get_server()->get_player() : nullptr;

        if (!to_server || !to_local) {
            std::this_thread::sleep_for(INTERVAL);
            continue;
        }

        
        {
            auto wname = utils::WorldManager::get_instance().get_world_name();
            if (wname.empty() || wname == "EXIT") {
                std::this_thread::sleep_for(INTERVAL);
                continue;
            }
        }

        
        // Read every pass so a range change from the /collect page applies immediately
        const float RADIUS = static_cast<float>(s_range_tiles.load()) * 32.0f;

        float bot_x = 0.f, bot_y = 0.f;
        {
            auto px = s_core->get_config().get<std::string>("player.position.x");
            auto py = s_core->get_config().get<std::string>("player.position.y");
            try { if (!px.empty()) bot_x = std::stof(px); } catch(...) {}
            try { if (!py.empty()) bot_y = std::stof(py); } catch(...) {}
        }

        
        auto& inv      = utils::InventoryManager::get_instance();
        uint32_t inv_size  = inv.get_inventory_size();
        auto     inv_snap  = inv.get_items_snapshot();
        size_t   item_cnt  = inv_snap.size();
        std::unordered_map<uint16_t,uint8_t> amounts;
        for (const auto& it : inv_snap) amounts[it.id] = it.amount;

        
        auto& wm = utils::WorldManager::get_instance();
        std::vector<world::DroppedItemInfo> snap_items = wm.get_items();
        std::vector<world::DroppedItemInfo> snap_live  = wm.get_live_objects();

        
        struct Cand { uint32_t uid; uint16_t id; float x,y,d2; };
        std::vector<Cand> cands;
        cands.reserve(64);
        std::unordered_set<uint32_t> seen;

        auto enqueue = [&](const world::DroppedItemInfo& item){
            if (seen.count(item.Uid)) return;
            float dx = bot_x - item.X, dy = bot_y - item.Y;
            float d2 = dx*dx + dy*dy;
            if (d2 <= RADIUS*RADIUS) {
                seen.insert(item.Uid);
                cands.push_back({item.Uid, item.ItemId, item.X, item.Y, d2});
            }
        };
        for (const auto& it : snap_items) enqueue(it);
        for (const auto& it : snap_live)  enqueue(it);

        if (cands.empty()) { std::this_thread::sleep_for(INTERVAL); continue; }

        
        std::sort(cands.begin(), cands.end(),
            [](const Cand& a, const Cand& b){ return a.d2 < b.d2; });

        
        int sent = 0;
        for (const auto& c : cands) {
            if (!s_running.load() || generation != s_generation.load()) break;

            
            bool can = false;
            auto it = amounts.find(c.id);
            if (it != amounts.end())
                can = (it->second < 200);
            else
                can = (inv_size == 0 || item_cnt < inv_size);

            if (!can) continue;

            
            auto now = std::chrono::steady_clock::now();
            auto last_it = collection_times.find(c.uid);
            if (last_it != collection_times.end()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_it->second).count();
                if (elapsed < 2) continue; 
            }

            
            player::Player* srv = s_core->get_client() ? s_core->get_client()->get_player() : nullptr;
            if (!srv) break;

            
            
            for (int brute = -10; brute <= 10; ++brute) {
                uint32_t b_uid = static_cast<uint32_t>(static_cast<int32_t>(c.uid) + brute);
                if (b_uid == 0) continue;
                send_collect_packet(srv, b_uid, c.x, c.y);
            }

            collection_times[c.uid] = now;
            
            int netid = s_core ? s_core->get_config().get<int>("player.netid") : 0;
            spdlog::info("[AutoCollect] BRUTE-FORCE SWEEP sent for uid={} (Range +/-10) id={} (NetID={})", 
                         c.uid, c.id, netid);

            
            

            
            player::Player* loc = s_core->get_server() ? s_core->get_server()->get_player() : nullptr;
            send_console(loc, fmt::format("`2[AC]`w Collecting item `5{}`` uid:`3{}``", c.id, c.uid));
            ++sent;
        }

        if (sent > 0)
            spdlog::info("[AutoCollect] tick: {} packets sent", sent);

        std::this_thread::sleep_for(INTERVAL);
    }

    spdlog::info("[AutoCollect] stopped gen={}", generation);
    s_running = false;
}

} 
