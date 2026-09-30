# Auto Farming Commands — Design

Date: 2026-09-30

## Goal

Port LuckyProxy's auto-farming automation into Vin-Proxy with the same behavior and dialog layouts, rewritten for Vin-Proxy's `CommandBase` framework, `WorldManager`, `InventoryManager` and `PlayerTracker`.

Source: `D:\Code\luckyproxy-source-main\proxy\` (`events.cpp`, `gt.cpp`, `gt.hpp`, `proxy.cpp`).

## Command mapping

| LuckyProxy | Vin-Proxy | Kind |
|---|---|---|
| `/harvest` | `/autoharvest` | opens dialog |
| `/plant` | `/autoplant` | opens dialog |
| `/afarm` | `/autofarm` | opens dialog |
| `/farm` | `/farm` | toggles auto farm |
| `/afish` | `/autofish` | opens dialog |
| `/fish` | `/fish` | toggles auto fish |

## Files (all in `src/extension/command_handler/`)

Each is a `CommandBase` subclass following `AutoCollectCommand` / `SpeedCommand`: static `s_core`, static settings, `show_dialog`, `handle_dialog_response`, a detached worker thread guarded by an atomic generation counter (`stop()` bumps it).

- `autoharvest_command.{hpp,cpp}` — `AutoHarvestCommand`
- `autoplant_command.{hpp,cpp}` — `AutoPlantCommand`
- `autofarm_command.{hpp,cpp}` — `AutoFarmCommand` (`autofarm`) and `FarmToggleCommand` (`farm`)
- `autofish_command.{hpp,cpp}` — `AutoFishCommand` (`autofish`) and `FishToggleCommand` (`fish`)

Toggle commands share state with their dialog command via public static accessors.

## Wiring

- Register the commands where `AutoCollectCommand` is registered; call `set_core` the same way.
- Route dialog returns in `command_handler_impl.hpp` next to the `speed_page` branch (~line 763): `auto_harvest_page`, `auto_plant_page`, `farmpage`, `autofish_page`. Cancel does nothing; each branch sets `event.canceled = true`.
- Add the commands to the help / command list dialog.
- CMake: add the new `.cpp` files if sources are listed explicitly.

## Dialogs

Raw GT dialog strings via `send_dialog` (as `SpeedCommand::show_dialog`). LuckyProxy's `Dialog` builder maps to:

| LuckyProxy | GT string |
|---|---|
| `addLabelWithIcon(t, id, LABEL_BIG/SMALL)` | `add_label_with_icon|big/small|t|left|id|` |
| `addCheckbox(n, t, v)` | `add_checkbox|n|t|v|` |
| `addInputBox(n, t, v, len)` | `add_text_input|n|t|v|len|` |
| `addPicker(n, t, hint)` | `add_item_picker|n|t|hint|` |
| `addSmallText` / `addDescText` | `add_smalltext|..|` |
| `addTextBox` | `add_textbox|..|` |
| `addSpacer(SMALL)` | `add_spacer|small|` |
| `addQuickExit` | `add_quick_exit|` |
| `endDialog(name, ok, cancel)` | `end_dialog|name|cancel|ok|` |

Layouts, texts, icons and color codes are copied verbatim from LuckyProxy (`events.cpp` ~5949 harvest, ~6002 plant, ~7013 afish, ~7031 afarm; farm validation dialog ~4415). "Current item" labels resolve the item name through the loaded items data.

### Auto Harvest (`auto_harvest_page`, icon 5638)
Checkbox `enable_auto_harvest`; picker `auto_harvest_seed`; inputs `auto_harvest_path_delay` (default 800), `auto_harvest_punch_delay` (default 500).

### Auto Plant (`auto_plant_page`, icon 5638)
Checkbox `enable_auto_plant`; pickers `plantSeedID`, `plantTargetBlock`; inputs `auto_plant_path_delay` (800), `auto_plant_place_delay` (500).

### Auto Farm (`farmpage`, icon 898)
Picker `farmID`; checkboxes `autofrm`, `disablekao` ("disable when pulled"); input `farminterval` (default 300). Interval below 150 ms re-shows the dialog with the error "AutoFarm Delay Can't be less than 150ms!".

### Auto Fish (`autofish_page`, icon 3436)
Input `farmid` (bait id); checkboxes `baitkiri` ("character on right"), `baitkanan` ("character on left"). Text says: type `/fish` to start, `/fish` again to stop.

## Logic

Coordinates: tile = position / 32.

- **Harvest** (from `gt::do_auto_harvest`): worker loop; if enabled and seed set, find the nearest tile (Manhattan distance) whose foreground equals the selected item; if the player is not on it, move there every `path_delay` ms; if on it, punch every `punch_delay` ms.
- **Plant** (from `gt::do_auto_plant`): find the nearest empty tile (foreground 0) whose tile directly below (y+1 in LuckyProxy's convention — verify orientation against Vin-Proxy's tile Y axis during implementation) is the target block; move there every `path_delay`; place the seed every `place_delay`.
- **Farm** (from `AutoFarmPlace` / `AutoFarmPunch`): two loops while enabled and in-world. Facing right (`right_left == 32`) uses tiles x+1, x+2; facing left (`48`) uses x-1, x-2. Place the farm item on empty tiles and punch occupied tiles, sleeping `farmDelay` between steps. If "disable when pulled" is set, a `"You were pulled by"` text overlay stops farming.
- **Fish** (from the `OnConsoleMessage` handler, `events.cpp` ~9166): on a console message containing `"You caught "`, wait 100 ms and place the bait item at (x±1, y+1) depending on facing.

## Primitives

- **Punch:** `PACKET_TILE_CHANGE_REQUEST` with `int_data = 18` and tile coords (matches the existing check in `player_state_tracker_impl.hpp:160`), sent to the server.
- **Place:** `PACKET_TILE_CHANGE_REQUEST` with `int_data = item_id` (existing pattern at `utility_commands.cpp:3765`).
- **Move:** `PACKET_STATE` with the target position (existing pattern at `utility_commands.cpp:987`). Reuse `FindPathCommand`'s walk if it can be called cleanly; otherwise a simple direct state update.
- **Facing:** read from the tracked local player's state flags (the value LuckyProxy calls `right_left`); confirm the field name in `PlayerTracker`.
- **World data:** tiles from `WorldManager::get_tiles()`; player position from config `player.position.x/y` or `PlayerTracker`.
- **Console/overlay hooks** (fish, pulled): subscribe via the existing packet/event mechanism used by other extensions, not new server hooks.

## Error handling

- Workers exit when the generation changes, the world is empty/`EXIT`, or the server player is gone.
- Missing required setting (no seed/item selected): send a console message and do not start.
- Non-numeric dialog input is ignored (previous value kept); delays clamp to the same minimums LuckyProxy uses (farm 150 ms).
- Leaving the world stops all four automations.

## Testing

No test harness for command logic exists in the repo, so verification is: project builds; each dialog opens and its returned values are read back correctly (log the parsed state); each toggle starts/stops its worker (log lines); in-game smoke test of harvest, plant, farm, fish on a test world.

## Out of scope

Other LuckyProxy commands; refactoring existing Vin-Proxy commands; persisting settings across sessions (LuckyProxy does not either).
