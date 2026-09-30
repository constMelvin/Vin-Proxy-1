# Auto Farming Commands Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `/autoharvest`, `/autoplant`, `/autofarm`, `/farm`, `/autofish`, `/fish` to Vin-Proxy, ported from LuckyProxy with identical dialogs and behavior.

**Architecture:** Four `CommandBase` subclasses (one header/source pair each) using the existing `AutoCollectCommand` pattern (static state, detached worker thread, atomic generation counter). A shared header holds low-level helpers (send dialog/console, punch, place, move, world/tile access, facing state). One small extension observes server/client packets to feed the fish and "pulled" triggers and to track facing direction. Dialog returns route through `command_handler_impl.hpp`.

**Tech Stack:** C++20, CMake (`file(GLOB)` in `src/CMakeLists.txt`), spdlog, fmt, existing `packet::` / `utils::` classes. No test framework exists in the repo — verification is a successful build plus in-game smoke tests (see each task).

**Spec:** `docs/superpowers/specs/2026-09-30-auto-farming-commands-design.md`

## Global Constraints

- Command names exactly: `autoharvest`, `autoplant`, `autofarm`, `farm`, `autofish`, `fish`.
- Dialog names exactly: `auto_harvest_page`, `auto_plant_page`, `farmpage`, `autofish_page`.
- Dialog field names/labels/icons/colors copied verbatim from LuckyProxy `events.cpp` (harvest ~5949, plant ~6002, afish ~7013, afarm ~7031, farm validation ~4415).
- Defaults: harvest path 800 / punch 500; plant path 800 / place 500; farm interval 300, minimum 150; fish re-cast delay 100 ms.
- Tile coordinates = pixel position / 32.
- Punch = `PACKET_TILE_CHANGE_REQUEST` with `int_data = 18`; place = same packet type with `int_data = item_id`.
- New sources go in `src/extension/command_handler/` (and `src/extension/autofarm/` for the extension). CMake globs, so re-run CMake configure after adding files.
- Follow surrounding style: 4-space indent, `namespace command { }`, `clone()` via `make_unique<T>(*this)`, backtick color codes.
- Do not touch existing modified-but-uncommitted files except the listed wiring edits (`command_handler_impl.hpp`, `main.cpp`).

## Review Focus

- Toggling a command while not in a world (`get_world_name()` empty or `"EXIT"`) must not crash or spin; it should report and idle/stop.
- Enabling harvest/plant/farm/fish with no item selected must refuse with a console message rather than run.
- Non-numeric or empty dialog inputs (delays, bait id) must keep the previous value; farm interval < 150 must re-show the dialog with the error.
- Starting the same automation twice must not spawn two workers (generation counter / running flag).
- Leaving the world or disconnecting must stop all workers and not leave the toggle showing "enabled".

---

### Task 1: Shared helpers header

**Files:**
- Create: `src/extension/command_handler/autofarm_common.hpp`
- Create: `src/extension/command_handler/autofarm_common.cpp`

**Interfaces:**
- Consumes: `core::Core`, `player::Player`, `utils::WorldManager`, `utils::PlayerTracker`, `packet::` types.
- Produces (namespace `command::autofarm`):
  - `void send_dialog(player::Player* p, const std::string& body);`
  - `void send_console(player::Player* p, const std::string& msg);`
  - `bool in_world();` — false when world name is empty or `"EXIT"`.
  - `bool local_tile_pos(core::Core* core, int& tx, int& ty);` — from config `player.position.x/y` / 32.
  - `uint16_t tile_fg(int x, int y);` — `WorldManager::get_tile_fg`, 0 if out of bounds.
  - `bool facing_left();` / `void set_facing_left(bool);` — atomic.
  - `void send_tile_action(core::Core* core, int32_t int_data, int32_t x, int32_t y);` — sends `PACKET_TILE_CHANGE_REQUEST` to the server.
  - `void punch(core::Core*, int x, int y);` = `send_tile_action(core, 18, x, y)`
  - `void place(core::Core*, int item_id, int x, int y);` = `send_tile_action(core, item_id, x, y)`
  - `void move_to(core::Core*, int x, int y);` — sends `PACKET_STATE` at (x*32, y*32+2) to the server.
  - `bool parse_int(const std::string&, int& out);` — strict integer parse, false on empty/non-numeric.
  - `bool checkbox_on(const TextParse&, const char* key);`

- [ ] **Step 1: Write `autofarm_common.hpp`**

```cpp
#pragma once
#include "../../core/core.hpp"
#include "../../player/player.hpp"
#include "../../utils/text_parse.hpp"
#include <cstdint>
#include <string>

namespace command::autofarm {

void send_dialog(player::Player* p, const std::string& body);
void send_console(player::Player* p, const std::string& msg);
bool in_world();
bool local_tile_pos(core::Core* core, int& tx, int& ty);
uint16_t tile_fg(int x, int y);
bool facing_left();
void set_facing_left(bool left);
void send_tile_action(core::Core* core, int32_t int_data, int32_t x, int32_t y);
void punch(core::Core* core, int x, int y);
void place(core::Core* core, int item_id, int x, int y);
void move_to(core::Core* core, int x, int y);
bool parse_int(const std::string& s, int& out);
bool checkbox_on(const TextParse& tp, const char* key);

} // namespace command::autofarm
```

- [ ] **Step 2: Write `autofarm_common.cpp`**

Copy the `MoriStatePacket` definition location: `grep -n "struct MoriStatePacket" -r src` and include that header (it is used in `utility_commands.cpp:3766`). Then:

```cpp
#include "autofarm_common.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../packet/packet_variant.hpp"
#include "../../packet/packet_types.hpp"
#include "../../utils/byte_stream.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/player_tracker.hpp"
#include <atomic>
#include <cmath>
#include <vector>
// + the header that defines MoriStatePacket (see grep above)

namespace command::autofarm {

static std::atomic<bool> s_facing_left{false};

static void send_function(player::Player* p, const char* fn, const std::string& arg) {
    if (!p) return;
    packet::Variant var{};
    var.add(fn);
    var.add(arg);
    std::vector<std::byte> ext = var.serialize();
    packet::GameUpdatePacket pkt{};
    pkt.type = packet::PACKET_CALL_FUNCTION;
    pkt.net_id = static_cast<uint32_t>(-1);
    pkt.flags.extended = 1;
    pkt.data_size = static_cast<uint32_t>(ext.size());
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    bs.write_data(ext.data(), ext.size());
    (void)p->send_packet(bs.get_data(), 0);
}

void send_dialog(player::Player* p, const std::string& body) { send_function(p, "OnDialogRequest", body); }
void send_console(player::Player* p, const std::string& msg)  { send_function(p, "OnConsoleMessage", msg); }

bool in_world() {
    auto name = utils::WorldManager::get_instance().get_world_name();
    return !name.empty() && name != "EXIT";
}

bool local_tile_pos(core::Core* core, int& tx, int& ty) {
    if (!core) return false;
    try {
        auto px = core->get_config().get<std::string>("player.position.x");
        auto py = core->get_config().get<std::string>("player.position.y");
        if (px.empty() || py.empty()) return false;
        tx = static_cast<int>(std::stof(px) / 32.0f);
        ty = static_cast<int>(std::stof(py) / 32.0f);
        return true;
    } catch (...) { return false; }
}

uint16_t tile_fg(int x, int y) {
    if (x < 0 || y < 0) return 0;
    return utils::WorldManager::get_instance().get_tile_fg(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

bool facing_left() { return s_facing_left.load(); }
void set_facing_left(bool left) { s_facing_left.store(left); }

void send_tile_action(core::Core* core, int32_t int_data, int32_t x, int32_t y) {
    if (!core || !core->get_client() || !core->get_client()->get_player()) return;
    MoriStatePacket pkt{};
    pkt.type = static_cast<uint8_t>(packet::PACKET_TILE_CHANGE_REQUEST);
    pkt.net_id = utils::PlayerTracker::get_instance().get_local_netid();
    pkt.value = static_cast<uint32_t>(int_data);
    pkt.vector_x = static_cast<float>(x * 32);
    pkt.vector_y = static_cast<float>(y * 32);
    pkt.int_x = x;
    pkt.int_y = y;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    core->get_client()->get_player()->send_packet(bs.get_data(), 0);
}

void punch(core::Core* core, int x, int y) { send_tile_action(core, 18, x, y); }
void place(core::Core* core, int item_id, int x, int y) { send_tile_action(core, item_id, x, y); }

void move_to(core::Core* core, int x, int y) {
    if (!core || !core->get_client() || !core->get_client()->get_player()) return;
    MoriStatePacket pkt{};
    pkt.type = static_cast<uint8_t>(packet::PACKET_STATE);
    pkt.net_id = utils::PlayerTracker::get_instance().get_local_netid();
    pkt.vector_x = static_cast<float>(x * 32);
    pkt.vector_y = static_cast<float>(y * 32) + 2.0f;
    pkt.int_x = -1;
    pkt.int_y = -1;
    pkt.flags = 0x1u | packet::PACKET_FLAG_ON_SOLID;
    if (s_facing_left.load()) pkt.flags |= packet::PACKET_FLAG_ROTATE_LEFT;
    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(pkt);
    core->get_client()->get_player()->send_packet_unreliable(bs.get_data(), 0);
    core->get_config().set<std::string>("player.position.x", std::to_string(x * 32));
    core->get_config().set<std::string>("player.position.y", std::to_string(y * 32));
}

bool parse_int(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) { return false; }
}

bool checkbox_on(const TextParse& tp, const char* key) {
    int v = 0;
    return parse_int(tp.get(key), v) && v != 0;
}

} // namespace command::autofarm
```

> Field names on `MoriStatePacket` (`type`, `net_id`, `value`, `vector_x/y`, `int_x/y`, `flags`) are taken from `utility_commands.cpp:986-1002` and `3766-3773`. If `flags` or the include differs, fix to match that file before building.

- [ ] **Step 3: Re-run CMake configure and build**

Run: `cmake -S . -B build_test && cmake --build build_test --config Release 2>&1 | tail -30`
Expected: builds; new `.cpp` picked up by glob. Fix any compile errors (include path for `MoriStatePacket`, `TextParse::get` const-ness).

- [ ] **Step 4: Commit**

```bash
git add src/extension/command_handler/autofarm_common.hpp src/extension/command_handler/autofarm_common.cpp
git commit -m "feat: add shared helpers for auto farming commands"
```

---

### Task 2: Event extension (facing, fish trigger, pulled trigger, world-leave stop)

**Files:**
- Create: `src/extension/autofarm/autofarm_extension.hpp`
- Modify: `src/main.cpp:590` (include + `core.add_extension`)

**Interfaces:**
- Consumes: `command::autofarm::set_facing_left`, `AutoFishCommand::on_console_message(const std::string&)`, `AutoFarmCommand::on_overlay_message(const std::string&)`, `AutoHarvestCommand::stop()`, `AutoPlantCommand::stop()`, `AutoFarmCommand::stop()`, `AutoFishCommand::stop()` (all defined in Tasks 3–6; until then this task will not compile — build it last in Task 7, or stub calls behind Task 3–6).
- Produces: `extension::autofarm::AutoFarmExtension`.

> Order note: implement Tasks 3–6 before building this task. Write the file now, build in Task 7.

- [ ] **Step 1: Write the extension**

```cpp
#pragma once
#include "../extension.hpp"
#include "../../packet/packet_types.hpp"
#include "../../packet/tank_packet.hpp"
#include "../../packet/packet_variant.hpp"
#include "../command_handler/autofarm_common.hpp"
#include "../command_handler/autoharvest_command.hpp"
#include "../command_handler/autoplant_command.hpp"
#include "../command_handler/autofarm_command.hpp"
#include "../command_handler/autofish_command.hpp"
#include <string>

namespace extension::autofarm {

class AutoFarmExtension final : public IExtension {
    core::Core* core_;

public:
    PROVIDE_EXT_UID(0x41465258);

    explicit AutoFarmExtension(core::Core* core) : core_{ core } {}
    ~AutoFarmExtension() override = default;

    void init() override {
        core_->get_event_dispatcher().appendListener(
            core::EventType::Packet,
            [this](const core::EventPacket& event) { on_packet(event); });
        core_->get_event_dispatcher().appendListener(
            core::EventType::Disconnection,
            [](const core::EventDisconnection&) { stop_all(); });
    }

    void free() override {}

private:
    static void stop_all() {
        command::AutoHarvestCommand::stop();
        command::AutoPlantCommand::stop();
        command::AutoFarmCommand::stop();
        command::AutoFishCommand::stop();
    }

    void on_packet(const core::EventPacket& event) {
        const auto& gp = event.get_packet();

        if (event.from == core::EventFrom::FromClient && gp.type == packet::PACKET_STATE) {
            const auto& ext = event.get_ext_data();
            const packet::TankUpdatePacket* tank = ext.size() >= sizeof(packet::TankUpdatePacket)
                ? reinterpret_cast<const packet::TankUpdatePacket*>(ext.data())
                : reinterpret_cast<const packet::TankUpdatePacket*>(&gp);
            command::autofarm::set_facing_left((tank->flags & packet::PACKET_FLAG_ROTATE_LEFT) != 0);
            return;
        }

        if (event.from != core::EventFrom::FromServer) return;

        if (gp.type == packet::PACKET_SEND_MAP_DATA) {
            // entering a new world: previous automation targets are stale
            stop_all();
            return;
        }

        if (gp.type != packet::PACKET_CALL_FUNCTION) return;
        try {
            packet::Variant v{};
            if (!v.deserialize(event.get_ext_data())) return;
            auto params = v.get_variants();
            if (params.size() < 2) return;
            const std::string fn = std::get<std::string>(params[0]);
            const std::string msg = std::get<std::string>(params[1]);
            if (fn == "OnConsoleMessage")
                command::AutoFishCommand::on_console_message(msg);
            else if (fn == "OnTextOverlay")
                command::AutoFarmCommand::on_overlay_message(msg);
        } catch (...) {}
    }
};

} // namespace extension::autofarm
```

> Verify against `world_logger_extension.hpp`: the `IExtension` virtuals (`init`, `free`), `EventType::Disconnection` listener signature, and `PROVIDE_EXT_UID` usage. Match them exactly; the UID value must be unique (grep `PROVIDE_EXT_UID` and pick an unused one).

- [ ] **Step 2: Register in `main.cpp`**

Add near line 27: `#include "extension/autofarm/autofarm_extension.hpp"`, and after line 590:

```cpp
        core.add_extension(new extension::autofarm::AutoFarmExtension{ &core });
```

- [ ] **Step 3: Commit (after Task 7 builds)**

```bash
git add src/extension/autofarm/autofarm_extension.hpp src/main.cpp
git commit -m "feat: add event extension for auto farming triggers"
```

---

### Task 3: AutoHarvest

**Files:**
- Create: `src/extension/command_handler/autoharvest_command.hpp`
- Create: `src/extension/command_handler/autoharvest_command.cpp`

**Interfaces:**
- Consumes: `command::autofarm::*` (Task 1).
- Produces: `class command::AutoHarvestCommand : CommandBase` with `set_core`, `static void handle_dialog_response(player::Player*, const std::string& raw)`, `static void stop()`, `static bool is_enabled()`.

- [ ] **Step 1: Header**

```cpp
#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <atomic>
#include <cstdint>

namespace command {

class AutoHarvestCommand : public CommandBase {
public:
    AutoHarvestCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    static void handle_dialog_response(player::Player* player, const std::string& raw);
    static void stop();
    static bool is_enabled() { return s_enabled.load(); }

private:
    static void show_dialog(player::Player* player);
    static void start();
    static void run(std::uint64_t generation);

    static core::Core* s_core;
    static std::atomic<bool> s_enabled;
    static std::atomic<std::uint64_t> s_generation;
    static std::atomic<int> s_seed_id;
    static std::atomic<int> s_path_delay;
    static std::atomic<int> s_punch_delay;
};

}
```

- [ ] **Step 2: Source**

Dialog text is LuckyProxy `events.cpp:5949-5974` translated with the table in the spec. Logic is `gt::do_auto_harvest` (`gt.cpp:74-119`) as a loop.

```cpp
#include "autoharvest_command.hpp"
#include "autofarm_common.hpp"
#include "../../client/client.hpp"
#include "../../server/server.hpp"
#include "../../utils/text_parse.hpp"
#include "../../utils/world_manager.hpp"
#include "../../utils/item_database.hpp"   // replace with the real header providing item names (see note)
#include <chrono>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <spdlog/spdlog.h>

namespace command {
namespace af = command::autofarm;

core::Core* AutoHarvestCommand::s_core = nullptr;
std::atomic<bool> AutoHarvestCommand::s_enabled{false};
std::atomic<std::uint64_t> AutoHarvestCommand::s_generation{0};
std::atomic<int> AutoHarvestCommand::s_seed_id{0};
std::atomic<int> AutoHarvestCommand::s_path_delay{800};
std::atomic<int> AutoHarvestCommand::s_punch_delay{500};

AutoHarvestCommand::AutoHarvestCommand()
    : CommandBase({"autoharvest"}, {}, "Open auto harvest settings", 0) {}
std::unique_ptr<CommandBase> AutoHarvestCommand::clone() const {
    return std::make_unique<AutoHarvestCommand>(*this);
}
void AutoHarvestCommand::set_core(core::Core* core) { s_core = core; }

void AutoHarvestCommand::stop() {
    s_enabled = false;
    s_generation.fetch_add(1);
}

void AutoHarvestCommand::show_dialog(player::Player* player) {
    std::ostringstream d;
    d << "add_label_with_icon|big|`9Auto Harvest Page|left|5638|\n";
    d << "add_spacer|small|\n";
    d << "add_checkbox|enable_auto_harvest|`2Enable Auto Harvest|" << (s_enabled.load() ? 1 : 0) << "|\n";
    d << "add_smalltext|Toggle this to start or stop automatic harvesting of items.|\n";
    d << "add_spacer|small|\n";
    const int seed = s_seed_id.load();
    if (seed > 0) {
        d << "add_label_with_icon|small|Current Harvest Item: " << item_name(seed) << "|left|" << seed << "|\n";
        d << "add_item_picker|auto_harvest_seed|`2Change Harvest Item|Select Harvest Item From Your Inventory|\n";
    } else {
        d << "add_item_picker|auto_harvest_seed|`2Current Harvest Item: |Select Harvest Item From Your Inventory|\n";
    }
    d << "add_smalltext|Choose which item you want the bot to automatically harvest.|\n";
    d << "add_spacer|small|\n";
    d << "add_text_input|auto_harvest_path_delay|Pathfinding Delay (ms)|" << s_path_delay.load() << "|5|\n";
    d << "add_smalltext|Delay between each pathfinding step in milliseconds.|\n";
    d << "add_text_input|auto_harvest_punch_delay|Punch Delay (ms)|" << s_punch_delay.load() << "|5|\n";
    d << "add_smalltext|Delay between each harvest punch in milliseconds.|\n";
    d << "end_dialog|auto_harvest_page|Cancel|OK|\n";
    af::send_dialog(player, d.str());
}

void AutoHarvestCommand::execute(client::Client*, const std::vector<std::string>&) {
    if (!s_core || !s_core->get_server()) return;
    show_dialog(s_core->get_server()->get_player());
}

void AutoHarvestCommand::handle_dialog_response(player::Player* player, const std::string& raw) {
    TextParse tp{raw};
    int v = 0;
    if (af::parse_int(tp.get("auto_harvest_seed"), v) && v > 0) s_seed_id = v;
    if (af::parse_int(tp.get("auto_harvest_path_delay"), v) && v > 0) s_path_delay = v;
    if (af::parse_int(tp.get("auto_harvest_punch_delay"), v) && v > 0) s_punch_delay = v;

    const bool want = af::checkbox_on(tp, "enable_auto_harvest");
    if (want && !s_enabled.load()) {
        if (s_seed_id.load() <= 0) {
            af::send_console(player, "`0[ `bVinProxy `0] `4Auto Harvest: select an item first.");
            return;
        }
        start();
        af::send_console(player, "`0[ `bVinProxy `0] `9Auto Harvest is now `2enabled.");
    } else if (!want && s_enabled.load()) {
        stop();
        af::send_console(player, "`0[ `bVinProxy `0] `9Auto Harvest is now `4disabled.");
    }
}

void AutoHarvestCommand::start() {
    s_enabled = true;
    const std::uint64_t gen = ++s_generation;
    std::thread([gen] { run(gen); }).detach();
}

void AutoHarvestCommand::run(std::uint64_t gen) {
    using clock = std::chrono::steady_clock;
    auto last_path = clock::now();
    auto last_punch = clock::now();
    auto due = [](clock::time_point& last, int ms) {
        auto now = clock::now();
        if (now - last < std::chrono::milliseconds(ms)) return false;
        last = now;
        return true;
    };

    while (s_enabled.load() && gen == s_generation.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (!af::in_world()) continue;

        int px = 0, py = 0;
        if (!af::local_tile_pos(s_core, px, py)) continue;

        const int seed = s_seed_id.load();
        auto& wm = utils::WorldManager::get_instance();
        const int w = static_cast<int>(wm.get_world_width());
        const int h = static_cast<int>(wm.get_world_height());

        int best = INT_MAX, tx = -1, ty = -1;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (af::tile_fg(x, y) != seed) continue;
                int d = std::abs(px - x) + std::abs(py - y);
                if (d < best) { best = d; tx = x; ty = y; }
            }
        }
        if (tx < 0) continue;

        if (px != tx || py != ty) {
            if (due(last_path, s_path_delay.load())) af::move_to(s_core, tx, ty);
        } else if (due(last_punch, s_punch_delay.load())) {
            af::punch(s_core, tx, ty);
        }
    }
    spdlog::info("[AutoHarvest] worker stopped gen={}", gen);
}

}
```

> **Item names:** find how existing commands resolve an item name from an ID (`grep -rn "get_item_name\|item_name(" src`) and use that in a local `static std::string item_name(int id)` helper instead of the placeholder include above. Put the helper in `autofarm_common` if it is needed by more than one command (harvest, plant, farm all use it) — add `std::string item_name(int id);` to the header and implement it there in this step.
>
> **Scan cost:** the tile scan is O(width×height) each 20 ms tick. That is acceptable for GT worlds (100×60); if profiling shows otherwise, scan only when `s_enabled` and rate-limit to every 100 ms.

- [ ] **Step 3: Build check** (after Task 7 wiring) — commit:

```bash
git add src/extension/command_handler/autoharvest_command.* src/extension/command_handler/autofarm_common.*
git commit -m "feat: add /autoharvest command with dialog"
```

---

### Task 4: AutoPlant

**Files:**
- Create: `src/extension/command_handler/autoplant_command.hpp`
- Create: `src/extension/command_handler/autoplant_command.cpp`

**Interfaces:**
- Consumes: `command::autofarm::*`, `item_name`.
- Produces: `class command::AutoPlantCommand : CommandBase` with `set_core`, `handle_dialog_response(player::Player*, const std::string&)`, `stop()`, `is_enabled()`.

- [ ] **Step 1: Header** — same shape as Task 3 with class `AutoPlantCommand`, statics `s_seed_id`, `s_target_block`, `s_path_delay`(800), `s_place_delay`(500).

- [ ] **Step 2: Source** — identical skeleton to Task 3 with these differences:

Dialog (from `events.cpp:6003-6050`):

```cpp
d << "add_label_with_icon|big|`#Auto Plant Page|left|5638|\n";
d << "add_spacer|small|\n";
d << "add_checkbox|enable_auto_plant|`2Enable Auto Plant|" << (s_enabled.load() ? 1 : 0) << "|\n";
d << "add_smalltext|Toggle this to start or stop automatic planting.|\n";
d << "add_spacer|small|\n";
// seed picker: field "plantSeedID"
if (seed > 0) {
    d << "add_label_with_icon|small|Current Seed : " << item_name(seed) << "|left|" << seed << "|\n";
    d << "add_item_picker|plantSeedID|`2Change Seed|Select a Different Seed From Your Inventory|\n";
} else {
    d << "add_item_picker|plantSeedID|`2Select Seed to Plant|Select a Seed From Your Inventory|\n";
}
d << "add_smalltext|Choose which seed will be planted.|\n";
d << "add_spacer|small|\n";
// target block picker: field "plantTargetBlock"
if (block > 0) {
    d << "add_label_with_icon|small|Target Block : " << item_name(block) << "|left|" << block << "|\n";
    d << "add_item_picker|plantTargetBlock|`2Change Target Block|Select a Different Block|\n";
} else {
    d << "add_item_picker|plantTargetBlock|`2Select Target Block|Select Block to Plant On|\n";
}
```

Then read `events.cpp:6040-6050` for the remaining smalltext strings and the two delay inputs (`auto_plant_path_delay`, `auto_plant_place_delay`, length 5), and end with `end_dialog|auto_plant_page|Cancel|OK|`. Copy those strings verbatim.

`handle_dialog_response`: parse `plantSeedID`, `plantTargetBlock`, the two delays, checkbox `enable_auto_plant`. Refuse with a console message if enabled while seed or target block is unset.

Worker loop (from `gt.cpp:121-175`): for each tile (x, y) with `tile_fg(x,y) == 0` and `tile_fg(x, y + 1) == target_block` (**verify orientation at first in-game test:** GT tile Y grows downward, so "tile below the empty one" is `y + 1` — if seeds appear planted in the wrong row, swap to `y - 1`), track the nearest by Manhattan distance. If the player is not on it, `move_to` every path delay; else `place(s_core, seed, tx, ty)` every place delay while the tile is still empty.

- [ ] **Step 3: Build and commit** (after Task 7):

```bash
git add src/extension/command_handler/autoplant_command.*
git commit -m "feat: add /autoplant command with dialog"
```

---

### Task 5: AutoFarm and /farm

**Files:**
- Create: `src/extension/command_handler/autofarm_command.hpp`
- Create: `src/extension/command_handler/autofarm_command.cpp`

**Interfaces:**
- Consumes: `command::autofarm::*`, `item_name`.
- Produces:
  - `class AutoFarmCommand : CommandBase` (`autofarm`): `set_core`, `handle_dialog_response(player::Player*, const std::string&)`, `stop()`, `is_enabled()`, `static void toggle(player::Player*)`, `static void on_overlay_message(const std::string&)`.
  - `class FarmToggleCommand : CommandBase` (`farm`): `execute` calls `AutoFarmCommand::toggle`.

- [ ] **Step 1: Header** with statics `s_farm_item` (int, 0), `s_interval` (int, 300), `s_disable_on_pull` (bool), `s_enabled`, `s_generation`.

- [ ] **Step 2: Source**

Dialog builder `build_dialog(const std::string& error)` (from `events.cpp:4415-4447` and `7031-7053`): if `error` is non-empty add `add_textbox|`4AutoFarm Delay Can't be less than 150ms!|` after the title. Fields: title `add_label_with_icon|big|`5Auto Farm Page|left|898|`; `add_spacer|small|`; item picker `farmID` (label `` `#Change Farmable Item`` when set with a `Current `5Farmable Item: ` label with the item name, else `` `2Select Farmable Item``); `add_checkbox|autofrm|`2Enable `#Auto Farming|<0/1>|` + desc "You Can Also Type /farm to Enable /Disable AutoFarming."; `add_checkbox|disablekao|`2Auto `#Disabled Auto Farm When Pulled|<0/1>|` + desc "Automatically Disabld Auto Farming When Someone Pull You."; `add_text_input|farminterval|`5AutoFarm Interval `#[ms] |<interval>|5|` + smalltext "`5(1000ms = 1 second)"; `end_dialog|farmpage|Cancel|Okey|`.

`handle_dialog_response`:

```cpp
TextParse tp{raw};
int v = 0;
if (af::parse_int(tp.get("farmID"), v) && v > 0) s_farm_item = v;
s_disable_on_pull = af::checkbox_on(tp, "disablekao");
int interval = s_interval.load();
if (af::parse_int(tp.get("farminterval"), v)) interval = v;
if (interval < 150) { af::send_dialog(player, build_dialog(true)); return; }
s_interval = interval;
want = af::checkbox_on(tp, "autofrm");
// start/stop with console messages "`9Auto Farm is now `2enabled." / "`4disabled."
// refuse to start (console "select a farmable item first") if s_farm_item <= 0
```

`toggle(player)`: same start/stop, refusing when no item or not in a world.

Worker (from `AutoFarmPlace`/`AutoFarmPunch`, `events.cpp:2649-2770`), single loop replacing the two:

```cpp
while (s_enabled && gen == s_generation && af::in_world()) {
    int px, py;
    if (!af::local_tile_pos(s_core, px, py)) { sleep(50); continue; }
    const int dir = af::facing_left() ? -1 : 1;
    for (int step = 1; step <= 2; ++step) {
        if (!(s_enabled && gen == s_generation)) break;
        const int tx = px + dir * step;
        if (af::tile_fg(tx, py) == 0) af::place(s_core, s_farm_item, tx, py);
        else                          af::punch(s_core, tx, py);
        std::this_thread::sleep_for(std::chrono::milliseconds(s_interval.load()));
    }
}
s_enabled = false;
```

(LuckyProxy runs place and punch as two concurrent threads sharing the interval; the single loop preserves the observable behavior — place on empty, punch on filled, ±1 and ±2, one action per interval — without racing packets.)

`on_overlay_message(msg)`: if `s_disable_on_pull && msg.find("You were pulled by") != npos` → `stop()`, `s_disable_on_pull = false`, console "`9Auto Farm is now `4disabled (pulled)."

`FarmToggleCommand` registers as `{"farm"}`, description "Toggle auto farm".

- [ ] **Step 3: Build and commit** (after Task 7):

```bash
git add src/extension/command_handler/autofarm_command.*
git commit -m "feat: add /autofarm dialog and /farm toggle"
```

---

### Task 6: AutoFish and /fish

**Files:**
- Create: `src/extension/command_handler/autofish_command.hpp`
- Create: `src/extension/command_handler/autofish_command.cpp`

**Interfaces:**
- Consumes: `command::autofarm::*`.
- Produces:
  - `class AutoFishCommand : CommandBase` (`autofish`): `set_core`, `handle_dialog_response`, `stop()`, `is_enabled()`, `static void toggle(player::Player*)`, `static void on_console_message(const std::string&)`.
  - `class FishToggleCommand : CommandBase` (`fish`).

- [ ] **Step 1: Header** with statics `s_bait_id` (int, 0), `s_char_right` (bool), `s_char_left` (bool), `s_enabled` (bool), `s_generation`.

- [ ] **Step 2: Source**

Dialog (from `events.cpp:7014-7026`):

```cpp
d << "add_label_with_icon|big|`9Auto Fish Options Page|left|3436|\n";
d << "add_text_input|farmid|`9Bait id:|" << s_bait_id.load() << "|5|\n";
d << "add_smalltext|`#How to Find Bait id? type /find [your bait] There Is Item id.|\n";
d << "add_checkbox|baitkiri|`9Your Character On Right|" << (s_char_right ? 1 : 0) << "|\n";
d << "add_smalltext|`9What is Your Character On Left? It's like Where Your Character is Facing. (Enable & Disable Checkbox The Button)|\n";
d << "add_checkbox|baitkanan|`9Your Character On Left|" << (s_char_left ? 1 : 0) << "|\n";
d << "add_smalltext|`9What is Your Character On  Right? It's like Where Your Character is Facing. (Enable & Disable Checkbox The Button)|\n";
d << "add_textbox|`9First Write /fish then press Bait on Water and stay AFK|\n";
d << "add_smalltext|`#Type /fish Again if you want to disable it.|\n";
d << "add_quick_exit|\n";
d << "end_dialog|autofish_page|Cancel|OK|\n";
```

`handle_dialog_response`: parse `farmid` (positive int else keep), `baitkiri`, `baitkanan` checkboxes. Does not start fishing (LuckyProxy starts it with `/fish`).

`toggle(player)`: flip `s_enabled`; refuse with console "`4Auto Fish: set a bait id in /autofish first." when enabling with `s_bait_id <= 0`; else console "`2[FISH`2]`w: `2Enabled `9Auto Fish" / "`2[FISH`2]`w: `4Disabled `9Auto Fish".

`on_console_message(msg)` (from `events.cpp:9166-9189`):

```cpp
void AutoFishCommand::on_console_message(const std::string& msg) {
    if (!s_enabled.load() || msg.find("You caught ") == std::string::npos) return;
    if (!af::in_world() || s_bait_id.load() <= 0) return;
    const std::uint64_t gen = s_generation.load();
    std::thread([gen] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!s_enabled.load() || gen != s_generation.load()) return;
        int px, py;
        if (!af::local_tile_pos(s_core, px, py)) return;
        const bool left = af::facing_left();
        // LuckyProxy: facing right (32) -> x+1, facing left (48) -> x-1, always y+1
        af::place(s_core, s_bait_id.load(), px + (left ? -1 : 1), py + 1);
    }).detach();
}
```

The two direction checkboxes are stored and shown, matching LuckyProxy, whose gating on them is commented out in the source (`//if (baitkanan)`); direction comes from the facing flag.

`FishToggleCommand`: `{"fish"}`, "Toggle auto fish", calls `AutoFishCommand::toggle`.

`stop()` sets `s_enabled = false` and bumps the generation.

- [ ] **Step 3: Build and commit** (after Task 7):

```bash
git add src/extension/command_handler/autofish_command.*
git commit -m "feat: add /autofish dialog and /fish toggle"
```

---

### Task 7: Wiring, build, smoke test

**Files:**
- Modify: `src/extension/command_handler/command_handler_impl.hpp` (includes ~line 28; `set_core` ~line 129; `register_command` ~line 248; dialog routing ~line 763)
- Modify: help list wherever command names are listed (`grep -n "autocollect" src/extension/command_handler/*.cpp src/proxy_imgui_gui.cpp` to find the list/dialog).

- [ ] **Step 1: Include, set_core, register**

```cpp
#include "autoharvest_command.hpp"
#include "autoplant_command.hpp"
#include "autofarm_command.hpp"
#include "autofish_command.hpp"
```

After `command::AutoCollectCommand::set_core(core_);`:

```cpp
        command::AutoHarvestCommand::set_core(core_);
        command::AutoPlantCommand::set_core(core_);
        command::AutoFarmCommand::set_core(core_);
        command::AutoFishCommand::set_core(core_);
```

After `register_command(std::make_unique<command::AutoCollectCommand>());`:

```cpp
        register_command(std::make_unique<command::AutoHarvestCommand>());
        register_command(std::make_unique<command::AutoPlantCommand>());
        register_command(std::make_unique<command::AutoFarmCommand>());
        register_command(std::make_unique<command::FarmToggleCommand>());
        register_command(std::make_unique<command::AutoFishCommand>());
        register_command(std::make_unique<command::FishToggleCommand>());
```

- [ ] **Step 2: Dialog routing** — after the `speed_page` branch (~line 763-768) add, in the same style:

```cpp
            else if (dialog_name == "auto_harvest_page") {
                if (button_clicked != "Cancel")
                    command::AutoHarvestCommand::handle_dialog_response(const_cast<player::Player*>(&event.get_player()), event.get_message().get_raw());
                event.canceled = true;
                return;
            }
            else if (dialog_name == "auto_plant_page") {
                if (button_clicked != "Cancel")
                    command::AutoPlantCommand::handle_dialog_response(const_cast<player::Player*>(&event.get_player()), event.get_message().get_raw());
                event.canceled = true;
                return;
            }
            else if (dialog_name == "farmpage") {
                if (button_clicked != "Cancel")
                    command::AutoFarmCommand::handle_dialog_response(const_cast<player::Player*>(&event.get_player()), event.get_message().get_raw());
                event.canceled = true;
                return;
            }
            else if (dialog_name == "autofish_page") {
                if (button_clicked != "Cancel")
                    command::AutoFishCommand::handle_dialog_response(const_cast<player::Player*>(&event.get_player()), event.get_message().get_raw());
                event.canceled = true;
                return;
            }
```

There is a second dialog-routing site near line 1145 (`handle_dialog_response(const core::EventPacket&)`); read lines 1130-1160 and add the same four branches there if that path handles dialogs the first path does not.

- [ ] **Step 3: Add to help/command list** in the same style as the `autocollect` entry.

- [ ] **Step 4: Configure and build**

Run: `cmake -S . -B build_test && cmake --build build_test --config Release 2>&1 | tail -40`
Expected: build succeeds with no errors. Fix compile errors in place (missing includes, `MoriStatePacket` header, `IExtension` overrides).

- [ ] **Step 5: Smoke test in game (record results in the commit message)**
  1. `/autofarm` opens; pick an item, interval 100 → dialog re-opens with the 150 ms error. Interval 300 + enable → console "Auto Farm is now enabled"; blocks are placed and punched at ±1/±2 in the facing direction. `/farm` stops it, `/farm` restarts it.
  2. `/autofish` opens; set a bait id; `/fish` → "Enabled"; on "You caught " the bait is re-cast below-front. `/fish` disables.
  3. `/autoharvest`: pick a tree seed item, enable → walks to the nearest match and punches; disable stops it.
  4. `/autoplant`: pick seed + target block, enable → walks to empty tiles above the block and plants.
  5. Enter another world while any automation runs → it stops.
  6. Toggle any automation on with no item selected → refuses with a console message.

- [ ] **Step 6: Commit**

```bash
git add src/extension/command_handler/command_handler_impl.hpp src/extension/command_handler/*.cpp src/proxy_imgui_gui.cpp
git commit -m "feat: wire auto farming commands and dialog routing"
```

---

## Self-Review (spec coverage)

| Spec requirement | Task |
|---|---|
| Six command names | 3, 4, 5, 6, 7 |
| Four dialogs with exact fields | 3, 4, 5, 6 |
| Dialog routing | 7 |
| Harvest / plant / farm / fish logic | 3, 4, 5, 6 |
| Farm interval minimum 150 | 5 |
| Disable when pulled (`OnTextOverlay`) | 2, 5 |
| Fish re-cast on `"You caught "` | 2, 6 |
| Punch / place / move / facing primitives | 1, 2 |
| Stop on world change / disconnect | 2 |
| Missing-item refusal, non-numeric input | 3–6 (handlers), Review Focus |
| Plant orientation and facing field verification | 1 (facing via extension), 4 (orientation note + smoke test) |

Known gaps handled by notes, not placeholders: the `item_name` helper (Task 3) and the `MoriStatePacket` include path (Task 1) must be resolved by grep when implementing, since the exact existing header names were not read.
