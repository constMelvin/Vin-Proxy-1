# VinProxy Premium - Complete Commands Directory

> **VinProxy** features a high-performance in-game command system with **144 specialized command handlers** and **over 250 distinct slash triggers & aliases**. All commands can be typed directly into the Growtopia chat bar.

---

## 📌 General Command Syntax & Guidelines

- **Prefix**: All commands start with `/` (e.g. `/proxy`, `/warp`). Special shortcuts like `//` or `///` can also be typed with or without a leading slash.
- **Parameters**:
  - `<parameter>` : **Required** argument that must be provided.
  - `[parameter]` : **Optional** argument that has a sensible default if omitted.
- **Case-Insensitive**: Command triggers and world names are generally case-insensitive (`/warp START` equals `/warp start`).
- **Typo & Buffer Tolerance**: The command engine features smart prefix matching to handle client keyboard buffer lag and accidental keystrokes.
- **In-Game Directory**: Type `/proxy` or `/gui` at any time to open the visual menu inside Growtopia.

---

## 📑 Table of Contents

- [🛡️ Moderator Detection, Defense & World Protection](#moderator-detection-defense-world-protection)
- [🌐 Movement, Warping & Smart Pathfinding](#movement-warping-smart-pathfinding)
- [🎰 Casino, Auto-Hoster & Gambling Tools](#casino-auto-hoster-gambling-tools)
- [🏪 Vending Machines, Item Finder & Shop Utilities](#vending-machines-item-finder-shop-utilities)
- [💰 Economy, Storage, Banking & Drops](#economy-storage-banking-drops)
- [🌍 World Scanning, Analysis & Object Trackers](#world-scanning-analysis-object-trackers)
- [🤖 Automation Bots & Macro Features](#automation-bots-macro-features)
- [👔 Visuals, Cosmetics, Skins & Titles](#visuals-cosmetics-skins-titles)
- [🔧 Proxy, System & Interface Controls](#proxy-system-interface-controls)
- [🧪 Packets, Events & Developer Debugging](#packets-events-developer-debugging)
- [🔤 Complete Alphabetical Quick Index (A–Z)](#-complete-alphabetical-quick-index-az)

---

## 🛡️ Moderator Detection, Defense & World Protection

*Real-time safety mechanisms designed to safeguard players from game moderators, staff detection, lethal hazards, and unwanted interactions.*

### Quick Reference (14 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/antigravity` | `/ag` | *None* | Place + activate antigravity using raw packets (item_id f... |
| `/antipunch` | `/ap` | *None* | Place + activate antipunch visual jammer using raw packet... |
| `/banall` | *None* | *None* | Ban all players who have spawned |
| `/banfire` | `/bf` | *None* | Toggle auto-ban fire mode (bans players who use pocket li... |
| `/fakemaint` | *None* | `reason` | Send fake maintenance system messages |
| `/ignorecsn` | *None* | *None* | Toggle auto-ignore for CSN/REME seller spam |
| `/ignorecsnchat` | *None* | *None* | Toggle auto-ignore for CSN/REME chat spam |
| `/immune` | *None* | *None* | Toggle immunity to fire and acid / spike damage |
| `/join` | `/j` | *None* | Show join mode menu (pull/kick/ban) |
| `/moddetect` | *None* | *None* | Toggle moderator spawn detection |
| `/pullall` | *None* | *None* | Pull all players who have spawned |
| `/run` | *None* | *None* | Join 12 random worlds (9-12 alphanumeric chars) with delay |
| `/warn` | `/warning`, `/fakeban` | *None* | Send a fake ban warning notification to yourself |
| `/wrench` | `/awr` | *None* | Open auto-wrench settings (pull/kick/ban) |

### Detailed Command Specifications

#### `/antigravity`
- **Class**: `AntiGravityCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/antigravity`, `/ag`
- **Syntax**: `/antigravity`
- **Description**: Places and activates anti-gravity generator mechanics (item ID 4992) using raw packets to float without gravity.
- **Notes & Mechanics**: Allows floating and frictionless movement across the world.
- **Usage Examples**:
  ```text
  /antigravity
  /ag
  ```

#### `/antipunch`
- **Class**: `AntiPunchCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/antipunch`, `/ap`
- **Syntax**: `/antipunch`
- **Description**: Places and activates a visual anti-punch jammer (item ID 1276) using raw packets to block punching animations.
- **Notes & Mechanics**: Prevents punch disruption locally.
- **Usage Examples**:
  ```text
  /antipunch
  /ap
  ```

#### `/banall`
- **Class**: `BanallCommand` in [banall_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/banall_command.cpp)
- **Aliases**: `/banall`
- **Syntax**: `/banall`
- **Description**: Instantly mass-bans all spawned players currently present in the world who do not have world access.
- **Notes & Mechanics**: Iterates through all tracked players in the world and dispatches `/ban <name>` packets.
- **Usage Examples**:
  ```text
  /banall
  ```

#### `/banfire`
- **Class**: `BanFireCommand` in [banfire_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/banfire_command.cpp)
- **Aliases**: `/banfire`, `/bf`
- **Syntax**: `/banfire`
- **Description**: Toggles automated ban protection against players who attempt to use pocket lighters or set fire in your world.
- **Notes & Mechanics**: Monitors game action packets; immediately bans offending netIDs.
- **Usage Examples**:
  ```text
  /banfire
  /bf
  ```

#### `/fakemaint`
- **Class**: `FakeMaintCommand` in [fakemaint_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/fakemaint_command.cpp)
- **Aliases**: `/fakemaint`
- **Syntax**: `/fakemaint reason`
- **Minimum Arguments**: `2`
- **Description**: Injects a fake server maintenance announcement system message into your local client.
- **Notes & Mechanics**: Generates official-looking server maintenance popups.
- **Usage Examples**:
  ```text
  /fakemaint Maintenance in 10 minutes
  /fakemaint
  ```

#### `/ignorecsn`
- **Class**: `IgnoreCSNCommand` in [ignorecsn_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/ignorecsn_command.cpp)
- **Aliases**: `/ignorecsn`
- **Syntax**: `/ignorecsn`
- **Description**: Toggles auto-ignoring and silencing of players spamming CSN/REME casino advertisements in global broadcasts.
- **Notes & Mechanics**: Filters incoming server broadcast variant packets containing casino keywords.
- **Usage Examples**:
  ```text
  /ignorecsn
  ```

#### `/ignorecsnchat`
- **Class**: `IgnoreCSNChatCommand` in [ignorecsnchat_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/ignorecsnchat_command.cpp)
- **Aliases**: `/ignorecsnchat`
- **Syntax**: `/ignorecsnchat`
- **Description**: Toggles auto-filtering of CSN and casino gambling messages from the local world chat stream.
- **Notes & Mechanics**: Filters `OnTalkBubble` and `OnConsoleMessage` packets matching spam triggers.
- **Usage Examples**:
  ```text
  /ignorecsnchat
  ```

#### `/immune`
- **Class**: `ImmuneCommand` in [immune_command.hpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/immune_command.hpp)
- **Aliases**: `/immune`
- **Syntax**: `/immune`
- **Description**: Toggles local collision immunity against hazard blocks such as lava, spikes, and fire hazards.
- **Notes & Mechanics**: Modifies local packet simulation so hazards do not register lethal hit damage.
- **Usage Examples**:
  ```text
  /immune
  ```

#### `/join`
- **Class**: `JoinCommand` in [join_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/join_command.cpp)
- **Aliases**: `/join`, `/j`
- **Syntax**: `/join`
- **Description**: Opens the Join Mode configuration dialog to automatically pull, kick, or ban players as soon as they enter the world.
- **Notes & Mechanics**: Configures automatic welcome or security actions on player spawn.
- **Usage Examples**:
  ```text
  /join
  /j
  ```

#### `/moddetect`
- **Class**: `ModDetectCommand` in [moddetect_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/moddetect_command.cpp)
- **Aliases**: `/moddetect`
- **Syntax**: `/moddetect`
- **Description**: Toggles active moderator and staff detection. When a known moderator or developer spawns into your current world, the proxy triggers an instant fullscreen red visual alert and plays an alert sound.
- **Notes & Mechanics**: Maintains an internal list of known moderator UIDs and names. Real-time alert prevents moderation bans.
- **Usage Examples**:
  ```text
  /moddetect
  ```

#### `/pullall`
- **Class**: `PullallCommand` in [pullall_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/pullall_command.cpp)
- **Aliases**: `/pullall`
- **Syntax**: `/pullall`
- **Description**: Instantly pulls all spawned players in the world to your current position.
- **Notes & Mechanics**: Useful in casino hosting or trading worlds to bring all players to the table.
- **Usage Examples**:
  ```text
  /pullall
  ```

#### `/run`
- **Class**: `RunCommand` in [run_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/run_command.cpp)
- **Aliases**: `/run`
- **Syntax**: `/run`
- **Description**: Emergency panic escape command. Automatically connects and hops across 12 pseudo-randomly generated worlds with a 400ms delay to clear your trace from moderators.
- **Notes & Mechanics**: Each world name is 9 to 12 alphanumeric characters long.
- **Usage Examples**:
  ```text
  /run
  ```

#### `/warn`
- **Class**: `WarnCommand` in [warn_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/warn_command.cpp)
- **Aliases**: `/warn`, `/warning`, `/fakeban`
- **Syntax**: `/warn`
- **Description**: Simulates and displays a client-side fake ban/warning notification dialog for testing UI or streaming protection.
- **Notes & Mechanics**: Local visual only; does not affect actual account standing.
- **Usage Examples**:
  ```text
  /warn
  /warning
  /fakeban
  ```

#### `/wrench`
- **Class**: `WrenchCommand` in [wrench_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/wrench_command.cpp)
- **Aliases**: `/wrench`, `/awr`
- **Syntax**: `/wrench`
- **Description**: Opens the Auto-Wrench configuration menu to select automatic actions (Pull, Kick, or Ban) when wrenching/tapping other players.
- **Notes & Mechanics**: Action priority order: Ban > Kick > Pull.
- **Usage Examples**:
  ```text
  /wrench
  /awr
  ```

---

## 🌐 Movement, Warping & Smart Pathfinding

*Precision movement, coordinate tracking, automated A* tile pathfinding, and fast-travel world warping.*

### Quick Reference (11 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/back` | `/BACK` | *None* | Warp to the previously entered world |
| `/door` | *None* | `door_id` | Join a specific door ID in the current world |
| `/fastdoor` | *None* | *None* | Toggle fast open/close entrance doors |
| `/path` | `/pathf`, `/findpath` | `<x> <y>` | Walk to coordinates using A* pathfinding (fast). |
| `/pathfind` | `/pathfinding` | *None* | Open LuckyProxy-style pathfinder options dialog |
| `/pf` | *None* | *None* | Toggle pathfinder mode on/off without opening dialog |
| `/player` | `/tpplayer` | `<name>` | Teleport to player by name |
| `/pos1` | `/pos2`, `/pos3`, `/pos4`, `/posback` | *None* | Set drop/teleport positions (pos1-4 for drops, posback fo... |
| `/setpos` | *None* | *None* | Teleport to position (Usage: /setpos <x> <y>) |
| `/tp1` | `/tp2`, `/tp3`, `/tp4` | *None* | Teleport to saved position (tp1-4) |
| `/warp` | `/exit` | `world_name` | Warp into the world specified by world_name (/exit warps ... |

### Detailed Command Specifications

#### `/back`
- **Class**: `BackCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/back`, `/BACK`
- **Syntax**: `/back`
- **Description**: Warps directly back to the previously visited world.
- **Notes & Mechanics**: Automatically tracks your last world name before the current world transition.
- **Usage Examples**:
  ```text
  /back
  /BACK
  ```

#### `/door`
- **Class**: `DoorCommand` in [door_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/door_command.cpp)
- **Aliases**: `/door`
- **Syntax**: `/door door_id`
- **Minimum Arguments**: `1`
- **Description**: Directly enters a target door ID within the current world without needing to click the door.
- **Notes & Mechanics**: Sends a direct `join_request` packet with world and door ID concatenated.
- **Usage Examples**:
  ```text
  /door VIP
  /door SHOP
  /door 1234
  ```

#### `/fastdoor`
- **Class**: `FastDoorCommand` in [fastdoor_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/fastdoor_command.cpp)
- **Aliases**: `/fastdoor`
- **Syntax**: `/fastdoor`
- **Description**: Toggles instant entrance and exit through doors, skipping door entry animations.
- **Notes & Mechanics**: Speeds up world traversal and door hopping.
- **Usage Examples**:
  ```text
  /fastdoor
  ```

#### `/path`
- **Class**: `FindPathCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/path`, `/pathf`, `/findpath`
- **Syntax**: `/path <x> <y>`
- **Description**: Executes rapid A* pathfinding to calculate the shortest obstacle-free route to map coordinates (X, Y) and moves your character.
- **Notes & Mechanics**: Avoids solid blocks, locked tiles, and hazards.
- **Usage Examples**:
  ```text
  /path 50 23
  /findpath 10 5
  /pathf 80 40
  ```

#### `/pathfind`
- **Class**: `PathFindDialogCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/pathfind`, `/pathfinding`
- **Syntax**: `/pathfind`
- **Description**: Opens the comprehensive Pathfinder configuration dialog (speed, collision filters, click-walk options).
- **Notes & Mechanics**: Provides settings for walk delay, wall clipping, and step thresholds.
- **Usage Examples**:
  ```text
  /pathfind
  /pathfinding
  ```

#### `/pf`
- **Class**: `PfCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/pf`
- **Syntax**: `/pf`
- **Description**: Toggles click-to-walk pathfinder mode ON or OFF without needing to open the configuration dialog.
- **Notes & Mechanics**: When enabled, punching or clicking any tile automatically pathfinds your character there.
- **Usage Examples**:
  ```text
  /pf
  /pf on
  /pf off
  ```

#### `/player`
- **Class**: `PlayerTPCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/player`, `/tpplayer`
- **Syntax**: `/player <name>`
- **Minimum Arguments**: `1`
- **Description**: Finds a player in the world by name and automatically walks/pathfinds to their exact tile position.
- **Notes & Mechanics**: Supports partial name matching.
- **Usage Examples**:
  ```text
  /player Melvin
  /tpplayer GrowGuy
  ```

#### `/pos1`
- **Class**: `PositionCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/pos1`, `/pos2`, `/pos3`, `/pos4`, `/posback`
- **Syntax**: `/pos1`
- **Description**: Saves your current tile coordinates as a numbered drop or teleport checkpoint (positions 1 through 4, or return point posback).
- **Notes & Mechanics**: Emits particle rings and visual highlights on the selected tile block.
- **Usage Examples**:
  ```text
  /pos1
  /pos2
  /pos3
  /pos4
  /posback
  ```

#### `/setpos`
- **Class**: `SetPosCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/setpos`
- **Syntax**: `/setpos`
- **Minimum Arguments**: `2`
- **Description**: Directly updates your character coordinate position packet to specified coordinates (X, Y).
- **Notes & Mechanics**: Transmits raw position update.
- **Usage Examples**:
  ```text
  /setpos 100 200
  ```

#### `/tp1`
- **Class**: `TeleportPosCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/tp1`, `/tp2`, `/tp3`, `/tp4`
- **Syntax**: `/tp1`
- **Description**: Automatically navigates and walks you directly to a previously saved checkpoint (pos1 through pos4) using smart pathfinding.
- **Notes & Mechanics**: Calculates A* steps and alerts you on arrival.
- **Usage Examples**:
  ```text
  /tp1
  /tp2
  /tp3
  /tp4
  ```

#### `/warp`
- **Class**: `WarpCommand` in [warp_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/warp_command.cpp)
- **Aliases**: `/warp`, `/exit`
- **Syntax**: `/warp world_name`
- **Minimum Arguments**: `1`
- **Description**: Warps directly to the specified target world, or warps to the EXIT lobby.
- **Notes & Mechanics**: Bypasses standard world select menus.
- **Usage Examples**:
  ```text
  /warp START
  /warp BUYGHC
  /exit
  ```

---

## 🎰 Casino, Auto-Hoster & Gambling Tools

*Complete automation toolkit for casino hosts: automated betting checkpoints, 1.5-tile reach bet collection, wheel spin calculation, and automated winner payouts.*

### Quick Reference (15 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/cpos1` | `/cpos2` | *None* | Highlight and show saved Pos1 or Pos2 |
| `/daw` | `/dawl`, `/dropalllocks`, `/dropallwl` | *None* | Drop all World Locks, Diamond Locks, and Blue Gem Locks f... |
| `/dbgl` | *None* | `<amount>` | Drop Blue Gem Locks. /dbgl <amount> |
| `/dbglvis` | *None* | `<amount>` | Visual drop Blue Gem Locks (no inventory loss). /dbglvis ... |
| `/dd` | `/ddl` | `<amount>` | Drop Diamond Locks. /dd <amount> |
| `/dpos` | *None* | *None* | Set target drop position where you stand (same logic & an... |
| `/dropat` | `/dposdrop` | `[amt]` | Teleport to /dpos position and drop items (all or specifi... |
| `/dw` | `/dwl` | `<amount>` | Drop World Locks. /dw <amount> |
| `/fd` | *None* | *None* | Toggle fast drop mode (auto-confirms drop dialogs) |
| `/host` | *None* | *None* | Open casino hoster settings |
| `/qq` | `/showqq` | `[on/off]` | Toggle Show QQ Number in roulette spin |
| `/reme` | `/showreme` | `[on/off]` | Toggle Show REME Spin in roulette spin |
| `/spos1` | `/spos2` | *None* | Punch tile to set Pos1 or Pos2 |
| `/tp` | *None* | *None* | Teleport to drop position and collect bets with 1.5 tile ... |
| `/w1` | `/win1`, `/w2`, `/win2` | *None* | Teleport to player 1 or 2 winning spot, drop prize, and r... |

### Detailed Command Specifications

#### `/cpos1`
- **Class**: `CPosCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/cpos1`, `/cpos2`
- **Syntax**: `/cpos1`
- **Description**: Sends visual highlight packets and glowing particle rings to check the exact location of saved Pos1 or Pos2.
- **Notes & Mechanics**: Confirms tile selection visually before starting casino games.
- **Usage Examples**:
  ```text
  /cpos1
  /cpos2
  ```

#### `/daw`
- **Class**: `DropAllLocksCommand` in [drop_currency_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/drop_currency_command.cpp)
- **Aliases**: `/daw`, `/dawl`, `/dropalllocks`, `/dropallwl`
- **Syntax**: `/daw`
- **Description**: Drops all World Locks, Diamond Locks, and Blue Gem Locks currently held in your inventory in one click.
- **Notes & Mechanics**: Drops all lock tiers sequentially.
- **Usage Examples**:
  ```text
  /daw
  /dawl
  /dropalllocks
  /dropallwl
  ```

#### `/dbgl`
- **Class**: `DropBGLCommand` in [drop_currency_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/drop_currency_command.cpp)
- **Aliases**: `/dbgl`
- **Syntax**: `/dbgl <amount>`
- **Minimum Arguments**: `1`
- **Description**: Drops a specified amount of Blue Gem Locks directly onto the ground.
- **Notes & Mechanics**: Fast-drops BGLs without popup dialogs.
- **Usage Examples**:
  ```text
  /dbgl 1
  /dbgl 5
  ```

#### `/dbglvis`
- **Class**: `VisualDropCommand` in [drop_currency_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/drop_currency_command.cpp)
- **Aliases**: `/dbglvis`
- **Syntax**: `/dbglvis <amount>`
- **Minimum Arguments**: `1`
- **Description**: Simulates dropping Blue Gem Locks on your screen visually without consuming real inventory items.
- **Notes & Mechanics**: Client-side cosmetic drop effect.
- **Usage Examples**:
  ```text
  /dbglvis 50
  ```

#### `/dd`
- **Class**: `DropDLCommand` in [drop_currency_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/drop_currency_command.cpp)
- **Aliases**: `/dd`, `/ddl`
- **Syntax**: `/dd <amount>`
- **Minimum Arguments**: `1`
- **Description**: Drops a specified amount of Diamond Locks directly onto the ground.
- **Notes & Mechanics**: Automatically breaks into DLs or handles exact counts.
- **Usage Examples**:
  ```text
  /dd 5
  /ddl 10
  ```

#### `/dpos`
- **Class**: `DposCommand` in [dropat_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/dropat_command.cpp)
- **Aliases**: `/dpos`
- **Syntax**: `/dpos`
- **Description**: Saves your current tile coordinates as the target drop position for `/dropat`.
- **Notes & Mechanics**: Emits confirmation particle effects on the tile.
- **Usage Examples**:
  ```text
  /dpos
  ```

#### `/dropat`
- **Class**: `DropAtCommand` in [dropat_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/dropat_command.cpp)
- **Aliases**: `/dropat`, `/dposdrop`
- **Syntax**: `/dropat [amt]`
- **Description**: Teleports to the coordinate saved with `/dpos` and drops items from your inventory.
- **Notes & Mechanics**: Useful for designated prize deposit spots.
- **Usage Examples**:
  ```text
  /dropat
  /dropat 50
  /dposdrop 100
  ```

#### `/dw`
- **Class**: `DropWLCommand` in [drop_currency_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/drop_currency_command.cpp)
- **Aliases**: `/dw`, `/dwl`
- **Syntax**: `/dw <amount>`
- **Minimum Arguments**: `1`
- **Description**: Drops a specified amount of World Locks directly onto the ground.
- **Notes & Mechanics**: Bypasses standard item drop dialog confirmations.
- **Usage Examples**:
  ```text
  /dw 50
  /dwl 100
  ```

#### `/fd`
- **Class**: `DropFastCommand` in [dropfast_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/dropfast_command.cpp)
- **Aliases**: `/fd`
- **Syntax**: `/fd`
- **Description**: Toggles Fast Drop mode. Automatically auto-confirms all item drop dialogs instantly with zero delay.
- **Notes & Mechanics**: Allows clicking items in inventory and dropping them instantly without manual popup approval.
- **Usage Examples**:
  ```text
  /fd
  ```

#### `/host`
- **Class**: `HostCommand` in [host_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/host_command.cpp)
- **Aliases**: `/host`
- **Syntax**: `/host`
- **Description**: Opens the Casino Hoster settings dialog to toggle roulette visual settings, real spin displays, and instant wheel calculations.
- **Notes & Mechanics**: Options include Show Real Spin, Show QQ Number, Show REME Spin, and Instant Spin.
- **Usage Examples**:
  ```text
  /host
  ```

#### `/qq`
- **Class**: `QQCommand` in [host_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/host_command.cpp)
- **Aliases**: `/qq`, `/showqq`
- **Syntax**: `/qq [on/off]`
- **Description**: Quick toggle to show or hide the QQ wheel number calculation in roulette spin announcements.
- **Notes & Mechanics**: Shows the sum of roulette digits.
- **Usage Examples**:
  ```text
  /qq
  /qq on
  /qq off
  /showqq
  ```

#### `/reme`
- **Class**: `RemeCommand` in [host_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/host_command.cpp)
- **Aliases**: `/reme`, `/showreme`
- **Syntax**: `/reme [on/off]`
- **Description**: Quick toggle to show or hide the REME spin result in roulette spin announcements.
- **Notes & Mechanics**: Calculates odd/even or reverse game values.
- **Usage Examples**:
  ```text
  /reme
  /reme on
  /reme off
  /showreme
  ```

#### `/spos1`
- **Class**: `SPosCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/spos1`, `/spos2`
- **Syntax**: `/spos1`
- **Description**: Puts proxy into 'punch-to-set' mode. Your next punched tile becomes Pos1 or Pos2 for casino betting.
- **Notes & Mechanics**: Simplifies setting exact betting coordinates without having to stand on the block.
- **Usage Examples**:
  ```text
  /spos1
  /spos2
  ```

#### `/tp`
- **Class**: `CasinoTPCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/tp`
- **Syntax**: `/tp`
- **Description**: Automated bet sweep and verification: checks Pos1 and Pos2 with a 1.5-tile pickup radius, sweeps all World Locks, Diamond Locks, and BGLs, compares bets for equality, applies configured host tax %, and calculates the prize pot.
- **Notes & Mechanics**: Saves your starting spot as back position, collects bets in two rapid passes, and displays an on-screen summary overlay.
- **Usage Examples**:
  ```text
  /tp
  ```

#### `/w1`
- **Class**: `WinCommand` in [position_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/position_command.cpp)
- **Aliases**: `/w1`, `/win1`, `/w2`, `/win2`
- **Syntax**: `/w1`
- **Description**: Automated winner payout: walks to Player 1 or Player 2's betting spot, drops the calculated prize pool in locks, and automatically pathfinds back to your host position.
- **Notes & Mechanics**: Safely verifies your inventory balance before dropping and resets the active prize.
- **Usage Examples**:
  ```text
  /w1
  /win1
  /w2
  /win2
  ```

---

## 🏪 Vending Machines, Item Finder & Shop Utilities

*High-performance tools for searching, tracking, auditing, and automating vending machines across worlds.*

### Quick Reference (7 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/buy` | *None* | *None* | Purchase an item from the store (opens GUI to enter store... |
| `/vendf` | `/vendloc`, `/vendfind` | `item_name or item_id` | Search for vending machines selling a specific item |
| `/vendf` | `/vendloc` | *None* | Search for items in vending machines |
| `/vendfast` | `/vf` | *None* | Show vending fast actions menu (empty/add/buy) |
| `/vendlogs` | *None* | *None* | Show captured vending purchase logs from OnConsoleMessage |
| `/vendsafe` | `/vsafe` | *None* | Toggle safe vending buy helper |
| `/vendtp` | *None* | `item_name` | Highlight matching vends in current world with particle e... |

### Detailed Command Specifications

#### `/buy`
- **Class**: `BuyCommand` in [buy_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/buy_command.cpp)
- **Aliases**: `/buy`
- **Syntax**: `/buy`
- **Description**: Purchases items from the in-game store by store ID directly from a dialog prompt.
- **Notes & Mechanics**: Opens a prompt where you enter the store item ID and quantity to purchase.
- **Usage Examples**:
  ```text
  /buy
  ```

#### `/vendf`
- **Class**: `VendFindCommand` in [vendfind_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/vendfind_command.cpp)
- **Aliases**: `/vendf`, `/vendloc`, `/vendfind`
- **Syntax**: `/vendf item_name or item_id`
- **Minimum Arguments**: `1`
- **Description**: Direct search command for vending machine items with instant query filtering.
- **Notes & Mechanics**: Queries the local cached vending index.
- **Usage Examples**:
  ```text
  /vendfind 1796
  /vendf Laser Grid
  ```

#### `/vendf`
- **Class**: `VendLocCommand` in [vendloc_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/vendloc_command.cpp)
- **Aliases**: `/vendf`, `/vendloc`
- **Syntax**: `/vendf`
- **Description**: Searches for vending machines selling or buying a specific item by name or item ID across previously scanned worlds or current world.
- **Notes & Mechanics**: Opens an interactive dialog listing world locations, stock quantities, and prices.
- **Usage Examples**:
  ```text
  /vendloc 242
  /vendloc Angel Wings
  /vendf Magplant
  ```

#### `/vendfast`
- **Class**: `VendFastCommand` in [vendfast_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/vendfast_command.cpp)
- **Aliases**: `/vendfast`, `/vf`
- **Syntax**: `/vendfast`
- **Description**: Opens the Vending Fast Actions menu with toggles for Fast Empty, Fast Add/Stock, and Fast Buy modes.
- **Notes & Mechanics**: Automates repeated restocking and bulk buying operations on vending machines.
- **Usage Examples**:
  ```text
  /vendfast
  /vf
  ```

#### `/vendlogs`
- **Class**: `VendLogsCommand` in [vendlogs_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/vendlogs_command.cpp)
- **Aliases**: `/vendlogs`
- **Syntax**: `/vendlogs`
- **Description**: Displays an audit dialog containing captured vending machine transaction logs, purchases, and revenues.
- **Notes & Mechanics**: Parses incoming console transaction packets to track total items sold and income earned.
- **Usage Examples**:
  ```text
  /vendlogs
  ```

#### `/vendsafe`
- **Class**: `VendSafeCommand` in [vendsafe_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/vendsafe_command.cpp)
- **Aliases**: `/vendsafe`, `/vsafe`
- **Syntax**: `/vendsafe`
- **Description**: Toggles the Safe Vending verification helper to protect against price-switch scams in shops.
- **Notes & Mechanics**: Inspects purchase packets before submission to ensure price matches expected item value.
- **Usage Examples**:
  ```text
  /vendsafe
  /vsafe
  ```

#### `/vendtp`
- **Class**: `VendTPCommand` in [vendloc_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/vendloc_command.cpp)
- **Aliases**: `/vendtp`
- **Syntax**: `/vendtp item_name`
- **Minimum Arguments**: `1`
- **Description**: Highlights all vending machines in the current world that sell a specified item, emitting glowing particle VFX above them.
- **Notes & Mechanics**: Quickly locates the right vending machine in huge shopping malls.
- **Usage Examples**:
  ```text
  /vendtp Dirt
  /vendtp 242
  ```

---

## 💰 Economy, Storage, Banking & Drops

*World lock storage management, multi-tier lock splitting/compressing, item sweeps, fast inventory drops, and trash confirmation bypasses.*

### Quick Reference (15 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/autocollect` | `/ac` | *None* | Toggle auto-collect dropped items |
| `/autocomp` | `/ac` | *None* | Compress 100 WLs to 1 DL (run multiple times to compress ... |
| `/bal` | `/balance` | *None* | Show current World Lock, Diamond Lock, and BGL balance\nU... |
| `/bankadd` | *None* | *None* | Deposit all World Locks to storage. |
| `/bankadd` | *None* | *None* | Deposit all World Locks from inventory directly to bank s... |
| `/bankcheck` | *None* | *None* | Check World Lock bank balance. |
| `/bankwith` | *None* | *None* | Withdraw all World Locks from storage. |
| `/bid` | `/auctionbid` | `<amount>` | Place an auction bid with specified amount |
| `/dbox` | `/setdb` | *None* | Auto-donate items to Donation Box with max inventory count |
| `/dropall` | *None* | `[quantity]` | Drop items from inventory (all or specified quantity) |
| `/fillgbc` | *None* | *None* | Auto-fill Well of Love with Golden Booty Chests |
| `/inv` | `/inventory` | `[item_id]` | Show inventory info\nUsage: /inv [item_id] |
| `/mailclaim` | *None* | *None* | Claim mail reward by message ID - Usage: /mailclaim <mess... |
| `/trashfast` | `/tf` | *None* | Toggle fast trash mode (auto-confirms trash dialogs) |
| `/wlbank` | `/wlstorage` | `<amount>` | Modify World Lock storage amount (positive=deposit, negat... |

### Detailed Command Specifications

#### `/autocollect`
- **Class**: `AutoCollectCommand` in [autocollect_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/autocollect_command.cpp)
- **Aliases**: `/autocollect`, `/ac`
- **Syntax**: `/autocollect`
- **Description**: Toggles auto-sweep collection mode to pull nearby dropped items directly into your inventory.
- **Notes & Mechanics**: Sweeps floating items across tiles using raw collect packets.
- **Usage Examples**:
  ```text
  /autocollect
  /ac
  ```

#### `/autocomp`
- **Class**: `AutoCompCommand` in [autocomp_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/autocomp_command.cpp)
- **Aliases**: `/autocomp`, `/ac`
- **Syntax**: `/autocomp`
- **Description**: Automatically compresses 100 World Locks into 1 Diamond Lock with a single command.
- **Notes & Mechanics**: Can be executed repeatedly to compress multiple stacks.
- **Usage Examples**:
  ```text
  /autocomp
  /ac
  ```

#### `/bal`
- **Class**: `BalanceCommand` in [balance_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/balance_command.cpp)
- **Aliases**: `/bal`, `/balance`
- **Syntax**: `/bal`
- **Description**: Calculates and outputs your total net worth across inventory (WLs, DLs, BGLs) formatted cleanly in chat.
- **Notes & Mechanics**: Outputs: `Balance: [ <total_wl> WL | <dl> DL | <bgl> BGL ]`.
- **Usage Examples**:
  ```text
  /bal
  /balance
  ```

#### `/bankadd`
- **Class**: `BankCommand` in [bank_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/bank_command.cpp)
- **Aliases**: `/bankadd`
- **Syntax**: `/bankadd`
- **Description**: Deposits all World Locks, Diamond Locks, and BGLs from your inventory directly into your World Lock Storage / Bank.
- **Notes & Mechanics**: Can deposit a specific amount or your entire balance automatically.
- **Usage Examples**:
  ```text
  /bankadd
  /bankadd 500
  ```

#### `/bankadd`
- **Class**: `BankAddCommand` in [bankadd_command.hpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/bankadd_command.hpp)
- **Aliases**: `/bankadd`
- **Syntax**: `/bankadd`
- **Description**: Dedicated deposit command to store all loose World Locks from inventory into bank storage.
- **Notes & Mechanics**: Sends storage modify packets in 100ms intervals.
- **Usage Examples**:
  ```text
  /bankadd
  ```

#### `/bankcheck`
- **Class**: `BankCheckCommand` in [bank_check_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/bank_check_command.cpp)
- **Aliases**: `/bankcheck`
- **Syntax**: `/bankcheck`
- **Description**: Queries and reports your current World Lock Storage bank balance in chat.
- **Notes & Mechanics**: Pings the storage container and extracts the numerical balance.
- **Usage Examples**:
  ```text
  /bankcheck
  ```

#### `/bankwith`
- **Class**: `BankWithdrawCommand` in [bank_withdraw_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/bank_withdraw_command.cpp)
- **Aliases**: `/bankwith`
- **Syntax**: `/bankwith`
- **Description**: Withdraws a specified amount of World Locks from your World Lock Storage / Bank directly into inventory.
- **Notes & Mechanics**: Sends rapid withdraw packets directly to the storage container.
- **Usage Examples**:
  ```text
  /bankwith 100
  /bankwith 5000
  ```

#### `/bid`
- **Class**: `AuctionBidCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/bid`, `/auctionbid`
- **Syntax**: `/bid <amount>`
- **Description**: Submits a bid with the specified amount in active world auctions.
- **Notes & Mechanics**: Transmits auction bid network packet.
- **Usage Examples**:
  ```text
  /bid 50
  /auctionbid 100
  ```

#### `/dbox`
- **Class**: `DboxCommand` in [dbox_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/dbox_command.cpp)
- **Aliases**: `/dbox`, `/setdb`
- **Syntax**: `/dbox`
- **Description**: Automates donating items to a Donation Box up to the maximum inventory capacity.
- **Notes & Mechanics**: Fast donation cycle.
- **Usage Examples**:
  ```text
  /dbox
  /setdb
  ```

#### `/dropall`
- **Class**: `DropAllCommand` in [dropall_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/dropall_command.cpp)
- **Aliases**: `/dropall`
- **Syntax**: `/dropall [quantity]`
- **Description**: Drops all non-lock items from your inventory onto your current tile.
- **Notes & Mechanics**: Accepts an optional maximum quantity parameter.
- **Usage Examples**:
  ```text
  /dropall
  /dropall 50
  ```

#### `/fillgbc`
- **Class**: `FillGBCCommand` in [fillgbc_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/fillgbc_command.cpp)
- **Aliases**: `/fillgbc`
- **Syntax**: `/fillgbc`
- **Description**: Automatically deposits Golden Booty Chests into the Well of Love.
- **Notes & Mechanics**: Streamlines Well of Love event automation.
- **Usage Examples**:
  ```text
  /fillgbc
  ```

#### `/inv`
- **Class**: `InventoryCommand` in [inventory_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/inventory_command.cpp)
- **Aliases**: `/inv`, `/inventory`
- **Syntax**: `/inv [item_id]`
- **Description**: Inspects your complete inventory structure or checks the quantity of a specific item ID.
- **Notes & Mechanics**: Lists all equipped slots, inventory capacity, and total items.
- **Usage Examples**:
  ```text
  /inv
  /inventory
  /inv 242
  ```

#### `/mailclaim`
- **Class**: `MailClaimCommand` in [mailclaim_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/mailclaim_command.cpp)
- **Aliases**: `/mailclaim`
- **Syntax**: `/mailclaim`
- **Minimum Arguments**: `1`
- **Description**: Claims mail attachment rewards by message ID.
- **Notes & Mechanics**: Claims attached rewards without opening mailbox UI.
- **Usage Examples**:
  ```text
  /mailclaim 12345
  ```

#### `/trashfast`
- **Class**: `TrashFastCommand` in [trashfast_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/trashfast_command.cpp)
- **Aliases**: `/trashfast`, `/tf`
- **Syntax**: `/trashfast`
- **Description**: Toggles Fast Trash mode. Bypasses confirmation dialogs when trashing items for rapid inventory clearing.
- **Notes & Mechanics**: Automatically answers trash popup dialogs with approval.
- **Usage Examples**:
  ```text
  /trashfast
  /tf
  ```

#### `/wlbank`
- **Class**: `WorldLockStorageCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/wlbank`, `/wlstorage`
- **Syntax**: `/wlbank <amount>`
- **Description**: Low-level modifier for World Lock storage balances (positive to deposit, negative to withdraw).
- **Notes & Mechanics**: Interacts directly with the storage network interface.
- **Usage Examples**:
  ```text
  /wlbank 50
  /wlstorage -100
  ```

---

## 🌍 World Scanning, Analysis & Object Trackers

*Deep world analysis tools: tile scanning, door ID extraction, brute-force door cracking, floating item audits, and lock ownership inspect.*

### Quick Reference (15 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/admin` | *None* | *None* | Scan world for locks and show owner/admin data |
| `/cgems` | *None* | *None* | Show gem count on all tiles with gems |
| `/chest` | *None* | *None* | Replace target chest tiles (596/1530/5618/14752) with Dra... |
| `/dat` | *None* | *None* | Show tile data. Use /dat scan [radius] [delay_ms] for don... |
| `/doorbf` | *None* | *None* | Bruteforce door IDs using white-door position change dete... |
| `/doordat` | *None* | *None* | Scan world for doors and portals with their destination data |
| `/doorid` | *None* | *None* | Toggle door ID reveal - shows door IDs when you enter them |
| `/find` | *None* | `item_name or item_id` | Search for items in vending machines |
| `/growscan` | `/gs` | *None* | Scan world for blocks and items |
| `/gsbeta` | *None* | *None* | GrowScan beta (uses SEND_MAP_DATA world) |
| `/lockefind` | *None* | *None* | Show latest worlds where Locke stopped by |
| `/locketest001` | *None* | *None* | Send a test Locke OnConsoleMessage and upload a test sigh... |
| `/rema` | *None* | *None* | Remove your access from all locks in world (auto-confirm ... |
| `/save` | `/saveworld`, `/setsave` | *None* | Save and warp to designated safe storage world (/setsave ... |
| `/search` | *None* | `[item_name or item_id]` | Search all items in the database without limit |

### Detailed Command Specifications

#### `/admin`
- **Class**: `AdminCommand` in [admin_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/admin_command.cpp)
- **Aliases**: `/admin`
- **Syntax**: `/admin`
- **Description**: Scans the entire world for all locks (World Locks, Diamond Locks, BGLs, Royal Locks, etc.) and lists lock owners and access lists.
- **Notes & Mechanics**: Identifies lock positions and authorized players across all 26 lock types.
- **Usage Examples**:
  ```text
  /admin
  ```

#### `/cgems`
- **Class**: `CGemsCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/cgems`
- **Syntax**: `/cgems`
- **Description**: Calculates and prints the gem count on all tiles containing gems across the world.
- **Notes & Mechanics**: Summarizes total floating and tile-bound gems.
- **Usage Examples**:
  ```text
  /cgems
  ```

#### `/chest`
- **Class**: `ChestCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/chest`
- **Syntax**: `/chest`
- **Description**: Client-side inspection helper: replaces visual chest tiles (IDs 596, 1530, 5618, 14752) with Dragon Gates (ID 598) for easier click targeting.
- **Notes & Mechanics**: Visual aid for inspecting hidden chests.
- **Usage Examples**:
  ```text
  /chest
  ```

#### `/dat`
- **Class**: `DatCommand` in [dat_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/dat_command.cpp)
- **Aliases**: `/dat`
- **Syntax**: `/dat`
- **Description**: Scans and displays detailed tile metadata, doors, signs, donation boxes, and supports auto-scanning donation contents.
- **Notes & Mechanics**: Subcommand `/dat scan [radius] [delay_ms]` activates automated donation box scanning.
- **Usage Examples**:
  ```text
  /dat
  /dat scan 10 220
  ```

#### `/doorbf`
- **Class**: `DoorBFCommand` in [doorbf_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/doorbf_command.cpp)
- **Aliases**: `/doorbf`
- **Syntax**: `/doorbf`
- **Description**: High-speed door brute-forcer: automatically guesses door IDs using dictionary lists and numeric ranges, detecting access via white-door position shift.
- **Notes & Mechanics**: Stops automatically on discovery or when `/doorbf` is typed again.
- **Usage Examples**:
  ```text
  /doorbf
  /doorbf 1-100 vip shop
  /doorbf ?list
  /doorbf ?clearlist
  ```

#### `/doordat`
- **Class**: `DoordatCommand` in [doordat_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/doordat_command.cpp)
- **Aliases**: `/doordat`
- **Syntax**: `/doordat`
- **Description**: Scans the entire world for doors, blue portals, and path markers, reporting their destination target tags and IDs.
- **Notes & Mechanics**: Reveals hidden door paths and destination connections.
- **Usage Examples**:
  ```text
  /doordat
  ```

#### `/doorid`
- **Class**: `DoorIDCommand` in [doorid_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/doorid_command.cpp)
- **Aliases**: `/doorid`
- **Syntax**: `/doorid`
- **Description**: Toggles door ID reveal mode: whenever you enter any door, its internal door ID is printed to chat.
- **Notes & Mechanics**: Instantly exposes hidden IDs when testing worlds.
- **Usage Examples**:
  ```text
  /doorid
  ```

#### `/find`
- **Class**: `FindCommand` in [find_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/find_command.cpp)
- **Aliases**: `/find`
- **Syntax**: `/find item_name or item_id`
- **Minimum Arguments**: `1`
- **Description**: Searches for items across the entire item database and shows item properties.
- **Notes & Mechanics**: Searches names and numerical IDs.
- **Usage Examples**:
  ```text
  /find Rayman
  /find 1796
  ```

#### `/growscan`
- **Class**: `GrowScanCommand` in [growscan_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/growscan_command.cpp)
- **Aliases**: `/growscan`, `/gs`
- **Syntax**: `/growscan`
- **Description**: Opens the comprehensive GrowScan world analysis dialog, allowing you to scan all placed blocks, seeds, tree growth timers, and floating dropped items.
- **Notes & Mechanics**: Categorizes world contents into tiles, seeds, and dropped objects.
- **Usage Examples**:
  ```text
  /growscan
  /gs
  ```

#### `/gsbeta`
- **Class**: `GsBetaCommand` in [gsbeta_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/gsbeta_command.cpp)
- **Aliases**: `/gsbeta`
- **Syntax**: `/gsbeta`
- **Description**: Alternative GrowScan engine utilizing raw map data packets (`SEND_MAP_DATA`) for world scanning.
- **Notes & Mechanics**: Useful when standard world parser is processing large worlds.
- **Usage Examples**:
  ```text
  /gsbeta
  ```

#### `/lockefind`
- **Class**: `LockeFindCommand` in [lockefind_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/lockefind_command.cpp)
- **Aliases**: `/lockefind`
- **Syntax**: `/lockefind`
- **Description**: Queries and shows the latest known world locations where Locke the traveling salesman was spotted.
- **Notes & Mechanics**: Retrieves recent sightings to locate Locke's special shop.
- **Usage Examples**:
  ```text
  /lockefind
  ```

#### `/locketest001`
- **Class**: `LockeTest001Command` in [locketest001_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/locketest001_command.cpp)
- **Aliases**: `/locketest001`
- **Syntax**: `/locketest001`
- **Description**: Developer test tool: sends a simulated Locke notification message and uploads a mock sighting.
- **Notes & Mechanics**: Tests Locke notification parser pipeline.
- **Usage Examples**:
  ```text
  /locketest001
  ```

#### `/rema`
- **Class**: `RemaCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/rema`
- **Syntax**: `/rema`
- **Description**: Automatically unaccesses and removes yourself from all locks in the world, auto-confirming unaccess prompts.
- **Notes & Mechanics**: Cleans up your access rights across a world in one go.
- **Usage Examples**:
  ```text
  /rema
  ```

#### `/save`
- **Class**: `SaveWorldCommand` in [save_world_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/save_world_command.cpp)
- **Aliases**: `/save`, `/saveworld`, `/setsave`
- **Syntax**: `/save`
- **Description**: Saves and warps to your designated safe storage world, or configures the target safe world.
- **Notes & Mechanics**: Allows one-command emergency retreat to your private storehouse.
- **Usage Examples**:
  ```text
  /save
  /saveworld
  /setsave MYSAFEWORLD
  ```

#### `/search`
- **Class**: `SearchCommand` in [search_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/search_command.cpp)
- **Aliases**: `/search`
- **Syntax**: `/search [item_name or item_id]`
- **Description**: Interactive item search dialog with pagination, item icons, and action buttons.
- **Notes & Mechanics**: Displays paginated item database interface.
- **Usage Examples**:
  ```text
  /search
  /search Lock
  ```

---

## 🤖 Automation Bots & Macro Features

*Intelligent background bots for auto-spamming, automated surgery solving, and auto-crime completion.*

### Quick Reference (6 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `//` | `///` | *None* | Toggle spam ON/OFF |
| `//` | `///` | *None* | Quick toggle auto spammer ON / OFF directly in chat |
| `/autocrime` | *None* | *None* | Toggle auto-crime mode |
| `/autosurg` | `/autos` | *None* | Toggle auto-surgery mode (auto-selects correct tool) |
| `/spam` | *None* | *None* | Open Auto Spam settings dialog |
| `/spamdelay` | `/sd` | *None* | Set spam delay in milliseconds (usage: /spamdelay <ms>) |

### Detailed Command Specifications

#### `//`
- **Class**: `SpamToggleCommand` in [spam_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/spam_command.cpp)
- **Aliases**: `//`, `///`
- **Syntax**: `//`
- **Description**: Quick toggle to turn the automated chat spammer ON or OFF.
- **Notes & Mechanics**: Can also be typed as plain `//` directly into chat.
- **Usage Examples**:
  ```text
  //
  ///
  ```

#### `//`
- **Class**: `SpamToggleChat` in [command_handler_impl.hpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/command_handler_impl.hpp)
- **Aliases**: `//`, `///`
- **Syntax**: `//`
- **Description**: Plain chat trigger to instantly toggle auto-spam without slash prefix handling.
- **Notes & Mechanics**: Built directly into packet event listeners for instantaneous response.
- **Usage Examples**:
  ```text
  //
  ///
  ```

#### `/autocrime`
- **Class**: `AutoCrimeCommand` in [autocrime_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/autocrime_command.cpp)
- **Aliases**: `/autocrime`
- **Syntax**: `/autocrime`
- **Description**: Toggles automated crime solver mode: automatically answers superhero crime dialogs with winning options.
- **Notes & Mechanics**: Maximizes crime completion success rates.
- **Usage Examples**:
  ```text
  /autocrime
  ```

#### `/autosurg`
- **Class**: `AutoSurgCommand` in [autosurg_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/autosurg_command.cpp)
- **Aliases**: `/autosurg`, `/autos`
- **Syntax**: `/autosurg`
- **Description**: Toggles the automated surgery bot: analyzes patient condition and automatically selects the optimal surgical instrument.
- **Notes & Mechanics**: Solves surgical procedures automatically with zero misclicks.
- **Usage Examples**:
  ```text
  /autosurg
  /autos
  ```

#### `/spam`
- **Class**: `SpamCommand` in [spam_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/spam_command.cpp)
- **Aliases**: `/spam`
- **Syntax**: `/spam`
- **Description**: Opens the Auto-Spammer settings dialog to configure spam messages, intervals, colored text, and pull safety.
- **Notes & Mechanics**: Supports random color injection and auto-disable when pulled by another player.
- **Usage Examples**:
  ```text
  /spam
  ```

#### `/spamdelay`
- **Class**: `SpamDelayCommand` in [spam_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/spam_command.cpp)
- **Aliases**: `/spamdelay`, `/sd`
- **Syntax**: `/spamdelay`
- **Minimum Arguments**: `1`
- **Description**: Sets the chat spam delay in milliseconds.
- **Notes & Mechanics**: Default recommended delay is 3000ms to prevent server mutes.
- **Usage Examples**:
  ```text
  /spamdelay 3000
  /sd 2500
  ```

---

## 👔 Visuals, Cosmetics, Skins & Titles

*Client-side visual customization: custom skin RGB colors, spoofed names, animated titles, cosmetic equipment sets, slot saving, and visual effect overrides.*

### Quick Reference (34 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/banner` | `/bandolier` | `<id>` | Set bandolier banner on avatar. Usage: /banner <id 0-50> |
| `/bubble` | `/talk` | *None* | Show talk bubble with OnTalkBubble |
| `/clearclothes` | `/clearvisual`, `/resetclothes` | *None* | Clear all saved visual clothing items |
| `/cleartitle` | `/ct`, `/resettitle` | *None* | Remove all titles and reset name |
| `/clothes` | `/wear`, `/visual` | *None* | Visual clothes manager, slot sets, and clothing overrides |
| `/deathanim` | `/da` | `<id>` | Change death animation (0-30). Usage: /deathanim <id> |
| `/disguise` | *None* | `<item_id>` | Disguise as an item (0=clear). Usage: /disguise <item_id> |
| `/dr` | `/doctor` | *None* | Apply Dr. title |
| `/dragon` | *None* | *None* | Toggle Daylight Dragon effect |
| `/flag` | `/country` | `<flag_id>` | Change country flag display |
| `/g4g` | *None* | *None* | Apply G4G title |
| `/ghostchardddddddddddddddddd` | *None* | *None* | Toggle ghost character mode (send ghost packets and modif... |
| `/infinity` | *None* | *None* | Activate Infinity effects (crown/aura/weapon) |
| `/invis` | *None* | *None* | Toggle invisibility mode |
| `/itemfx` | `/ieffect` | `<item_id>` | Trigger item effect on yourself. Usage: /itemfx <item_id> |
| `/legend` | `/lg`, `/legendary` | *None* | Apply Legendary title (of Legend) |
| `/levelup` | `/lvlup` | `<level>` | Trigger level-up effect. Usage: /levelup <level> |
| `/load1` | `/load2`, `/load3`, `/load4`, `/set1`, `/set2`, `/set3`, `/set4` | *None* | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/maxlevel` | `/maxlv`, `/ml` | *None* | Apply Max Level title |
| `/mentor` | `/master`, `/mr` | *None* | Apply Mentor title |
| `/name` | `/nick`, `/nickname` | `new_name` | Change your display name |
| `/paintball` | `/pb` | `<netid> <color>` | Paintball a player. Usage: /paintball <netid> <0xRRGGBB> |
| `/particle` | `/pfx` | `<id> [v2]` | Spawn particle effect at your pos. Usage: /particle <id> ... |
| `/rainbow` | `/pure` | *None* | Toggle rainbow mode (OnChangePureBeingMode) |
| `/respawnanim` | `/ra` | `<id>` | Change respawn animation (0-30). Usage: /respawnanim <id> |
| `/riftwings` | `/rift` | *None* | Toggle Rift Wings/Cape effects |
| `/roleskin` | `/rs` | `<skin_id> [icon_id]` | Change role skin and icon. Usage: /roleskin <skin_id> [ic... |
| `/save1` | `/save2`, `/save3`, `/save4` | *None* | Save current visual clothes set to Slot 1-4 (e.g. /save1,... |
| `/setweather` | *None* | *None* | Set world weather (0=none, 1-30=types) |
| `/skin` | *None* | `<r> <g> <b>` | Change skin color. Usage: /skin <r> <g> <b>  or  /skin <0... |
| `/title` | `/titles`, `/tag` | *None* | Open title selection GUI |
| `/titleicon` | `/ticon` | *None* | Set title icon (Usage: /titleicon <0-200>) |
| `/vision` | *None* | *None* | Apply visual background vision (item_id 1156) on tiles x:... |
| `/weather` | *None* | `[weather_id]` | Open the Custom Weather Machine (or /weather <id>) |

### Detailed Command Specifications

#### `/banner`
- **Class**: `BannerCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/banner`, `/bandolier`
- **Syntax**: `/banner <id>`
- **Minimum Arguments**: `1`
- **Description**: Attaches a bandolier banner to your character avatar.
- **Notes & Mechanics**: Supports banner IDs from 0 to 50.
- **Usage Examples**:
  ```text
  /banner 10
  /bandolier 5
  ```

#### `/bubble`
- **Class**: `BubbleCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/bubble`, `/talk`
- **Syntax**: `/bubble`
- **Minimum Arguments**: `1`
- **Description**: Displays a custom chat talk bubble above your character using `OnTalkBubble`.
- **Notes & Mechanics**: Customizable talk bubble effect.
- **Usage Examples**:
  ```text
  /bubble
  /talk
  ```

#### `/clearclothes`
- **Class**: `ClearClothesCommand` in [clearclothes_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/clearclothes_command.cpp)
- **Aliases**: `/clearclothes`, `/clearvisual`, `/resetclothes`
- **Syntax**: `/clearclothes`
- **Description**: Resets and clears all currently applied visual clothing items back to default.
- **Notes & Mechanics**: Restores your actual equipment appearance.
- **Usage Examples**:
  ```text
  /clearclothes
  /clearvisual
  /resetclothes
  ```

#### `/cleartitle`
- **Class**: `ClearTitleCommand` in [cleartitle_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/cleartitle_command.cpp)
- **Aliases**: `/cleartitle`, `/ct`, `/resettitle`
- **Syntax**: `/cleartitle`
- **Description**: Clears all applied custom visual titles and resets your name tag to standard.
- **Notes & Mechanics**: Removes doctor, mentor, legendary, and custom prefixes.
- **Usage Examples**:
  ```text
  /cleartitle
  /ct
  /resettitle
  ```

#### `/clothes`
- **Class**: `ClothesCommand` in [clothes_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/clothes_command.cpp)
- **Aliases**: `/clothes`, `/wear`, `/visual`
- **Syntax**: `/clothes`
- **Description**: Opens the Visual Clothes Manager dialog to customize, equip, override, or inspect your client-side visual clothing.
- **Notes & Mechanics**: Changes clothes purely on your screen without owning the items.
- **Usage Examples**:
  ```text
  /clothes
  /wear
  /visual
  ```

#### `/deathanim`
- **Class**: `DeathAnimCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/deathanim`, `/da`
- **Syntax**: `/deathanim <id>`
- **Minimum Arguments**: `1`
- **Description**: Changes your character death animation style (IDs 0 to 30).
- **Notes & Mechanics**: Selects custom death particle sequences.
- **Usage Examples**:
  ```text
  /deathanim 5
  /da 12
  ```

#### `/disguise`
- **Class**: `DisguiseCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/disguise`
- **Syntax**: `/disguise <item_id>`
- **Minimum Arguments**: `1`
- **Description**: Disguises your character sprite as any item block (use ID 0 to clear).
- **Notes & Mechanics**: Transforms avatar appearance into selected item.
- **Usage Examples**:
  ```text
  /disguise 242
  /disguise 0
  ```

#### `/dr`
- **Class**: `DrCommand` in [title_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/title_command.cpp)
- **Aliases**: `/dr`, `/doctor`
- **Syntax**: `/dr`
- **Description**: Applies the Dr. (Doctor) title prefix to your character's name.
- **Notes & Mechanics**: Displays prestigious surgeon title.
- **Usage Examples**:
  ```text
  /dr
  /doctor
  ```

#### `/dragon`
- **Class**: `DragonCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/dragon`
- **Syntax**: `/dragon`
- **Description**: Toggles Daylight Dragon cosmetic flight effect.
- **Notes & Mechanics**: Dragon visual trail.
- **Usage Examples**:
  ```text
  /dragon
  ```

#### `/flag`
- **Class**: `FlagCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/flag`, `/country`
- **Syntax**: `/flag <flag_id>`
- **Minimum Arguments**: `1`
- **Description**: Changes the country flag displayed next to your player name.
- **Notes & Mechanics**: Accepts 2-letter ISO country codes.
- **Usage Examples**:
  ```text
  /flag us
  /country id
  /flag jp
  ```

#### `/g4g`
- **Class**: `G4GCommand` in [title_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/title_command.cpp)
- **Aliases**: `/g4g`
- **Syntax**: `/g4g`
- **Description**: Applies the G4G (Grow4Good) title to your character's nametag.
- **Notes & Mechanics**: Visual badge toggle.
- **Usage Examples**:
  ```text
  /g4g
  ```

#### `/ghostchardddddddddddddddddd`
- **Class**: `GhostCharCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/ghostchardddddddddddddddddd`
- **Syntax**: `/ghostchardddddddddddddddddd`
- **Description**: Toggles ghost character animation mode and ghost packet simulation.
- **Notes & Mechanics**: Modifies movement animation frames.
- **Usage Examples**:
  ```text
  /ghostchardddddddddddddddddd
  ```

#### `/infinity`
- **Class**: `InfinityCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/infinity`
- **Syntax**: `/infinity`
- **Minimum Arguments**: `1`
- **Description**: Activates Infinity visual effects (Crown, Aura, Weapon).
- **Notes & Mechanics**: Cosmetic godmode aura suite.
- **Usage Examples**:
  ```text
  /infinity
  ```

#### `/invis`
- **Class**: `InvisCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/invis`
- **Syntax**: `/invis`
- **Description**: Toggles character invisibility mode locally.
- **Notes & Mechanics**: Hides your sprite while allowing movement.
- **Usage Examples**:
  ```text
  /invis
  ```

#### `/itemfx`
- **Class**: `ItemEffectCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/itemfx`, `/ieffect`
- **Syntax**: `/itemfx <item_id>`
- **Minimum Arguments**: `1`
- **Description**: Triggers the visual aura or special effect of any item ID on your character.
- **Notes & Mechanics**: Simulates legendary weapon and aura visual effects.
- **Usage Examples**:
  ```text
  /itemfx 1796
  /ieffect 7188
  ```

#### `/legend`
- **Class**: `LegendCommand` in [legend_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/legend_command.cpp)
- **Aliases**: `/legend`, `/lg`, `/legendary`
- **Syntax**: `/legend`
- **Description**: Applies the 'of Legend' title suffix to your character's name.
- **Notes & Mechanics**: Displays legendary title suffix.
- **Usage Examples**:
  ```text
  /legend
  /lg
  /legendary
  ```

#### `/levelup`
- **Class**: `LevelUpCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/levelup`, `/lvlup`
- **Syntax**: `/levelup <level>`
- **Minimum Arguments**: `1`
- **Description**: Triggers the visual level-up fireworks animation for any level number.
- **Notes & Mechanics**: Local cosmetic level celebration.
- **Usage Examples**:
  ```text
  /levelup 125
  /lvlup 99
  ```

#### `/load1`
- **Class**: `LoadSlotCommand` in [clothesslot_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/clothesslot_command.cpp)
- **Aliases**: `/load1`, `/load2`, `/load3`, `/load4`, `/set1`, `/set2`, `/set3`, `/set4`
- **Syntax**: `/load1`
- **Description**: Loads and applies a saved visual clothing set from slots 1 through 4.
- **Notes & Mechanics**: Instantly switches full outfits with a single command.
- **Usage Examples**:
  ```text
  /load1
  /set1
  /load2
  /set2
  /load3
  /load4
  ```

#### `/maxlevel`
- **Class**: `MaxLevelCommand` in [title_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/title_command.cpp)
- **Aliases**: `/maxlevel`, `/maxlv`, `/ml`
- **Syntax**: `/maxlevel`
- **Description**: Applies the Max Level title badge to your character.
- **Notes & Mechanics**: Visual cosmetic indicator.
- **Usage Examples**:
  ```text
  /maxlevel
  /maxlv
  /ml
  ```

#### `/mentor`
- **Class**: `MentorCommand` in [mentor_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/mentor_command.cpp)
- **Aliases**: `/mentor`, `/master`, `/mr`
- **Syntax**: `/mentor`
- **Description**: Applies the Mentor / Master title prefix to your character.
- **Notes & Mechanics**: Displays mentor nametag.
- **Usage Examples**:
  ```text
  /mentor
  /master
  /mr
  ```

#### `/name`
- **Class**: `NameCommand` in [name_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/name_command.cpp)
- **Aliases**: `/name`, `/nick`, `/nickname`
- **Syntax**: `/name new_name`
- **Minimum Arguments**: `1`
- **Description**: Changes your local display name and chat nametag to any custom text.
- **Notes & Mechanics**: Client-side cosmetic name spoof.
- **Usage Examples**:
  ```text
  /name ProPlayer
  /nick LegendGuy
  /nickname Melvin
  ```

#### `/paintball`
- **Class**: `PaintballCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/paintball`, `/pb`
- **Syntax**: `/paintball <netid> <color>`
- **Minimum Arguments**: `2`
- **Description**: Simulates a paintball splat on a player or yourself.
- **Notes & Mechanics**: Use netID 0 for yourself, followed by hex color code.
- **Usage Examples**:
  ```text
  /paintball 0 0xFF0000
  /pb 12 0x00FF00
  ```

#### `/particle`
- **Class**: `ParticleCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/particle`, `/pfx`
- **Syntax**: `/particle <id> [v2]`
- **Minimum Arguments**: `1`
- **Description**: Spawns particle visual effects at your character position.
- **Notes & Mechanics**: Supports standard particle or v2 enhanced mode.
- **Usage Examples**:
  ```text
  /particle 88
  /pfx 42 v2
  ```

#### `/rainbow`
- **Class**: `RainbowCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/rainbow`, `/pure`
- **Syntax**: `/rainbow`
- **Description**: Toggles Pure Being / Rainbow cycling visual effect (`OnChangePureBeingMode`).
- **Notes & Mechanics**: Cycles rainbow hues across your avatar.
- **Usage Examples**:
  ```text
  /rainbow
  /pure
  ```

#### `/respawnanim`
- **Class**: `RespawnAnimCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/respawnanim`, `/ra`
- **Syntax**: `/respawnanim <id>`
- **Minimum Arguments**: `1`
- **Description**: Changes your character respawn animation style (IDs 0 to 30).
- **Notes & Mechanics**: Customizes world spawn entrance effects.
- **Usage Examples**:
  ```text
  /respawnanim 7
  /ra 15
  ```

#### `/riftwings`
- **Class**: `RiftWingsCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/riftwings`, `/rift`
- **Syntax**: `/riftwings`
- **Description**: Toggles Rift Wings and Cape visual trail effects.
- **Notes & Mechanics**: Cosmetic rift particle aura.
- **Usage Examples**:
  ```text
  /riftwings
  /rift
  ```

#### `/roleskin`
- **Class**: `RoleSkinCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/roleskin`, `/rs`
- **Syntax**: `/roleskin <skin_id> [icon_id]`
- **Minimum Arguments**: `1`
- **Description**: Changes role skin outfit and role icon badge.
- **Notes & Mechanics**: Sets custom role skins and icons.
- **Usage Examples**:
  ```text
  /roleskin 1 2
  /rs 3
  ```

#### `/save1`
- **Class**: `SaveSlotCommand` in [clothesslot_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/clothesslot_command.cpp)
- **Aliases**: `/save1`, `/save2`, `/save3`, `/save4`
- **Syntax**: `/save1`
- **Description**: Saves your current visual clothing set into one of four quick-load equipment slots.
- **Notes & Mechanics**: Saves sets to disk so they persist across sessions.
- **Usage Examples**:
  ```text
  /save1
  /save2
  /save3
  /save4
  ```

#### `/setweather`
- **Class**: `SetWeatherCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/setweather`
- **Syntax**: `/setweather`
- **Minimum Arguments**: `1`
- **Description**: Direct weather modifier for weather machine types (0 = None, 1-30 = types).
- **Notes & Mechanics**: Switches weather machine visual states.
- **Usage Examples**:
  ```text
  /setweather 1
  /setweather 7
  ```

#### `/skin`
- **Class**: `SkinCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/skin`
- **Syntax**: `/skin <r> <g> <b>`
- **Minimum Arguments**: `1`
- **Description**: Changes your character skin tone to any custom color using RGB values or hex format.
- **Notes & Mechanics**: Customizes avatar appearance beyond normal skin tone limits.
- **Usage Examples**:
  ```text
  /skin 255 0 0
  /skin 0x00FF00
  /skin 255 255 255
  ```

#### `/title`
- **Class**: `TitleCommand` in [title_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/title_command.cpp)
- **Aliases**: `/title`, `/titles`, `/tag`
- **Syntax**: `/title`
- **Description**: Opens the Title Selection GUI to pick from dozens of custom visual nametag titles.
- **Notes & Mechanics**: Provides interactive menu for title badges.
- **Usage Examples**:
  ```text
  /title
  /titles
  /tag
  ```

#### `/titleicon`
- **Class**: `TitleIconCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/titleicon`, `/ticon`
- **Syntax**: `/titleicon`
- **Minimum Arguments**: `1`
- **Description**: Sets a specific title icon ID next to your nametag.
- **Notes & Mechanics**: Supports icon IDs from 0 to 200.
- **Usage Examples**:
  ```text
  /titleicon 50
  /ticon 12
  ```

#### `/vision`
- **Class**: `VisionCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/vision`
- **Syntax**: `/vision`
- **Description**: Applies visual background night vision across tiles X:0-100, Y:0-60.
- **Notes & Mechanics**: Illuminates dark caves and unlit worlds.
- **Usage Examples**:
  ```text
  /vision
  ```

#### `/weather`
- **Class**: `WeatherCommand` in [weather_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/weather_command.cpp)
- **Aliases**: `/weather`
- **Syntax**: `/weather [weather_id]`
- **Description**: Opens the Custom Weather Machine dialog (ported from LuckyProxy): click any weather machine to change the visual weather. `/weather <id>` sets an ID (1-255) directly.
- **Notes & Mechanics**: Client-side only. Tick "Keep Weather Across All Worlds" to re-apply the chosen weather every time you enter a world.
- **Usage Examples**:
  ```text
  /weather
  /weather 5
  ```

---

## 🔧 Proxy, System & Interface Controls

*Core controls for VinProxy: desktop overlay GUI, latency monitoring, hardware identifier inspection, and session reconnects.*

### Quick Reference (9 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/devicecheck` | `/devices` | *None* | Show tracked spawned players and their device type |
| `/eventmenu` | *None* | *None* | Opens the event menu |
| `/gui` | `/proxygui` | *None* | Toggle the Vin Proxy Premium Desktop GUI |
| `/info` | `/information` | *None* | Open comprehensive commands directory with all available ... |
| `/ping` | *None* | *None* | Toggle ping display next to your name |
| `/players` | `/plist` | *None* | List all tracked players with netID and tile position |
| `/proxy` | `/news` | *None* | Open comprehensive commands directory with all available ... |
| `/relog` | *None* | *None* | Reconnect and relog into the current world |
| `/ubiclub` | `/ubisoft` | *None* | Opens Ubisoft Club/Connect interface |

### Detailed Command Specifications

#### `/devicecheck`
- **Class**: `DeviceCheckCommand` in [devicecheck_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/devicecheck_command.cpp)
- **Aliases**: `/devicecheck`, `/devices`
- **Syntax**: `/devicecheck`
- **Description**: Inspects all players in the world and lists their detected device platform (Android, iOS, Windows, Mac).
- **Notes & Mechanics**: Analyzes login handshake attributes to identify client platforms.
- **Usage Examples**:
  ```text
  /devicecheck
  /devices
  ```

#### `/eventmenu`
- **Class**: `EventMenuCommand` in [eventmenu_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/eventmenu_command.cpp)
- **Aliases**: `/eventmenu`
- **Syntax**: `/eventmenu`
- **Description**: Opens the active Growtopia seasonal event menu dialog.
- **Notes & Mechanics**: Direct access to event information.
- **Usage Examples**:
  ```text
  /eventmenu
  ```

#### `/gui`
- **Class**: `GuiCommand` in [gui_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/gui_command.cpp)
- **Aliases**: `/gui`, `/proxygui`
- **Syntax**: `/gui`
- **Description**: Toggles the floating Dear ImGui desktop window overlay on top of the game screen.
- **Notes & Mechanics**: Provides mouse-clickable buttons, sliders, real-time telemetry, and feature toggles.
- **Usage Examples**:
  ```text
  /gui
  /proxygui
  ```

#### `/info`
- **Class**: `InfoCommand` in [proxy_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/proxy_command.cpp)
- **Aliases**: `/info`, `/information`
- **Syntax**: `/info`
- **Description**: Opens the proxy information and quick navigation directory.
- **Notes & Mechanics**: Alias to proxy navigation system.
- **Usage Examples**:
  ```text
  /info
  /information
  ```

#### `/ping`
- **Class**: `PingCommand` in [ping_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/ping_command.cpp)
- **Aliases**: `/ping`
- **Syntax**: `/ping`
- **Description**: Toggles real-time network latency (ping in milliseconds) display next to your nametag.
- **Notes & Mechanics**: Measures true round-trip packet latency to the server.
- **Usage Examples**:
  ```text
  /ping
  ```

#### `/players`
- **Class**: `PlayersInfoCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/players`, `/plist`
- **Syntax**: `/players`
- **Description**: Prints a detailed list of all players in the current world with their netIDs, user IDs, and exact tile positions.
- **Notes & Mechanics**: Full audit log of active room occupants.
- **Usage Examples**:
  ```text
  /players
  /plist
  ```

#### `/proxy`
- **Class**: `ProxyCommand` in [proxy_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/proxy_command.cpp)
- **Aliases**: `/proxy`, `/news`
- **Syntax**: `/proxy`
- **Description**: Opens the main VinProxy directory dialog featuring custom texture tabs, categorized feature pages, and quick search.
- **Notes & Mechanics**: Supports tab shortcuts: `/proxy 1` (Overview), `/proxy 2` (Main Features), `/proxy 3` (Logs), `/proxy 4` (Mods), `/proxy 5` (Options).
- **Usage Examples**:
  ```text
  /proxy
  /news
  ```

#### `/relog`
- **Class**: `RelogCommand` in [relog_command.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/relog_command.cpp)
- **Aliases**: `/relog`
- **Syntax**: `/relog`
- **Description**: Instantly reconnects and rejoins the current world without needing to restart the game client.
- **Notes & Mechanics**: Fixes visual desyncs and ghost glitches cleanly.
- **Usage Examples**:
  ```text
  /relog
  ```

#### `/ubiclub`
- **Class**: `UbiclubCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/ubiclub`, `/ubisoft`
- **Syntax**: `/ubiclub`
- **Description**: Opens the Ubisoft Club / Ubisoft Connect dialog interface directly.
- **Notes & Mechanics**: Bypasses standard menu clicks.
- **Usage Examples**:
  ```text
  /ubiclub
  /ubisoft
  ```

---

## 🧪 Packets, Events & Developer Debugging

*Advanced network diagnostics: custom variant list injection, raw packet generation, animation debugging, and simulation triggers.*

### Quick Reference (18 Commands)

| Primary Command | Aliases | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/action` | *None* | *None* | Trigger action (Usage: /action <type>) |
| `/betatest` | `/beta` | *None* | Toggle Beta Tester mode |
| `/broadcast` | `/bc` | `<text>` | Inject a fake broadcast message locally. Usage: /broadcas... |
| `/bux` | `/growtokens` | *None* | Attempt to set Growtokens with OnSetBux |
| `/cmsg` | *None* | *None* | Send console message (Usage: /cmsg <text>) |
| `/debuganim` | `/copypunch`, `/animdebug` | *None* | Debug and inspect captured real vs visual punch animation |
| `/dialog` | *None* | *None* | Open custom dialog (Usage: /dialog <text>) |
| `/event` | *None* | `<event_name>` | Activate special events (valentines, easter, halloween, etc) |
| `/freeze` | *None* | `[netid]` | Freeze yourself (toggle) or try to freeze a player by netID |
| `/gems` | *None* | *None* | Open Gem Settings dialog |
| `/hitvfx` | `/vfx` | `<effectid>` | Trigger hit visual effects (0=default, 1-100=custom effects) |
| `/initworld` | `/initnew` | *None* | Reinitialize world state with OnInitNewWorld |
| `/mstate` | *None* | *None* | Toggle mod state (long punch, zoom, extended range) |
| `/overlay` | `/textoverlay` | *None* | Show text overlay with OnTextOverlay |
| `/rawvar` | `/rv` | `<OnFnName> [args]...` | Send any variant call. Usage: /rawvar <FnName> [arg1] [ar... |
| `/sm` | *None* | *None* | Toggle SuperMod state |
| `/spawnbgls` | *None* | *None* | Spawn Blue Gem Locks (item 7188) as live objects across t... |
| `/trade` | *None* | *None* | Trade commands (Usage: /trade <start|end> [netid]) |

### Detailed Command Specifications

#### `/action`
- **Class**: `ActionCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/action`
- **Syntax**: `/action`
- **Minimum Arguments**: `1`
- **Description**: Triggers generic game action commands via text packets.
- **Notes & Mechanics**: Sends raw action strings directly.
- **Usage Examples**:
  ```text
  /action respawn
  /action wrench
  ```

#### `/betatest`
- **Class**: `BetaCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/betatest`, `/beta`
- **Syntax**: `/betatest`
- **Description**: Toggles Beta Tester status flags on your player.
- **Notes & Mechanics**: Enables beta interface flags.
- **Usage Examples**:
  ```text
  /betatest
  /beta
  ```

#### `/broadcast`
- **Class**: `BroadcastCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/broadcast`, `/bc`
- **Syntax**: `/broadcast <text>`
- **Minimum Arguments**: `1`
- **Description**: Injects a simulated local Super-Broadcast (SB) announcement on your screen.
- **Notes & Mechanics**: Local client test only.
- **Usage Examples**:
  ```text
  /broadcast `4[ANNOUNCEMENT] `wServer update coming soon!
  /bc Test message
  ```

#### `/bux`
- **Class**: `BuxCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/bux`, `/growtokens`
- **Syntax**: `/bux`
- **Minimum Arguments**: `1`
- **Description**: Attempts to update client Growtokens display via `OnSetBux` variant list.
- **Notes & Mechanics**: Client-side token counter display.
- **Usage Examples**:
  ```text
  /bux
  /growtokens
  ```

#### `/cmsg`
- **Class**: `ConsoleMessageCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/cmsg`
- **Syntax**: `/cmsg`
- **Minimum Arguments**: `1`
- **Description**: Injects a custom text message into your game console via `OnConsoleMessage`.
- **Notes & Mechanics**: Supports all Growtopia color formatting codes.
- **Usage Examples**:
  ```text
  /cmsg `2Custom test notification
  ```

#### `/debuganim`
- **Class**: `DebugAnimCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/debuganim`, `/copypunch`, `/animdebug`
- **Syntax**: `/debuganim`
- **Description**: Inspects and compares your character's real equipped hand item versus visual weapon animations and captured punch packets.
- **Notes & Mechanics**: Logs packet discrepancies to console.
- **Usage Examples**:
  ```text
  /debuganim
  /copypunch
  /animdebug
  ```

#### `/dialog`
- **Class**: `DialogCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/dialog`
- **Syntax**: `/dialog`
- **Minimum Arguments**: `1`
- **Description**: Opens a custom raw dialog box defined by input string.
- **Notes & Mechanics**: Allows testing raw dialog syntax in-game.
- **Usage Examples**:
  ```text
  /dialog set_default_color|`o
add_label_with_icon|big|Test|left|242
end_dialog|test|OK|
  ```

#### `/event`
- **Class**: `EventCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/event`
- **Syntax**: `/event <event_name>`
- **Description**: Simulates seasonal events locally (Valentines, Easter, Halloween, Winterfest, etc.).
- **Notes & Mechanics**: Activates holiday themes.
- **Usage Examples**:
  ```text
  /event halloween
  /event valentines
  /event easter
  ```

#### `/freeze`
- **Class**: `FreezeCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/freeze`
- **Syntax**: `/freeze [netid]`
- **Description**: Toggles freeze status on yourself or attempts to freeze a target player by netID.
- **Notes & Mechanics**: Client simulation freeze.
- **Usage Examples**:
  ```text
  /freeze
  /freeze 24
  ```

#### `/gems`
- **Class**: `GemsCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/gems`
- **Syntax**: `/gems`
- **Description**: Opens the Gem Settings and drop customization dialog.
- **Notes & Mechanics**: Controls gem collection and display parameters.
- **Usage Examples**:
  ```text
  /gems
  ```

#### `/hitvfx`
- **Class**: `HitVFXCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/hitvfx`, `/vfx`
- **Syntax**: `/hitvfx <effectid>`
- **Description**: Triggers hit visual particle effects (IDs 1 through 100).
- **Notes & Mechanics**: Custom hit impact feedback.
- **Usage Examples**:
  ```text
  /hitvfx 12
  /vfx 5
  ```

#### `/initworld`
- **Class**: `InitWorldCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/initworld`, `/initnew`
- **Syntax**: `/initworld`
- **Description**: Re-triggers world initialization by sending `OnInitNewWorld` variant packet.
- **Notes & Mechanics**: Reloads world structures and tile maps.
- **Usage Examples**:
  ```text
  /initworld
  /initnew
  ```

#### `/mstate`
- **Class**: `MstateCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/mstate`
- **Syntax**: `/mstate`
- **Description**: Toggles Mod State simulation: extended punch reach, camera zoom, and boundary expansion.
- **Notes & Mechanics**: Client testing mode.
- **Usage Examples**:
  ```text
  /mstate
  ```

#### `/overlay`
- **Class**: `OverlayCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/overlay`, `/textoverlay`
- **Syntax**: `/overlay`
- **Minimum Arguments**: `1`
- **Description**: Displays an on-screen text banner overlay via `OnTextOverlay`.
- **Notes & Mechanics**: Tests HUD message banners.
- **Usage Examples**:
  ```text
  /overlay
  /textoverlay
  ```

#### `/rawvar`
- **Class**: `RawVariantCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/rawvar`, `/rv`
- **Syntax**: `/rawvar <OnFnName> [args]...`
- **Minimum Arguments**: `1`
- **Description**: Sends an arbitrary serialized VariantList packet to your client. Automatically casts numbers to int/float and strings as text.
- **Notes & Mechanics**: Indispensable tool for protocol reverse-engineering and packet testing.
- **Usage Examples**:
  ```text
  /rawvar OnConsoleMessage Hello World
  /rv OnTextOverlay Test 5000
  /rawvar OnSetBux 99999
  ```

#### `/sm`
- **Class**: `SmCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/sm`
- **Syntax**: `/sm`
- **Description**: Toggles SuperMod visual state and moderator badge attributes.
- **Notes & Mechanics**: Cosmetic testing flag.
- **Usage Examples**:
  ```text
  /sm
  ```

#### `/spawnbgls`
- **Class**: `SpawnBGLsCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/spawnbgls`
- **Syntax**: `/spawnbgls`
- **Description**: Spawns visual Blue Gem Lock items (item ID 7188) across tiles across the entire map.
- **Notes & Mechanics**: Client-side testing of object rendering limits.
- **Usage Examples**:
  ```text
  /spawnbgls
  ```

#### `/trade`
- **Class**: `TradeCommand` in [utility_commands.cpp](file:///d:/Code/Skript-Growtopia-Proxy/src/extension/command_handler/utility_commands.cpp)
- **Aliases**: `/trade`
- **Syntax**: `/trade`
- **Minimum Arguments**: `1`
- **Description**: Sends trade control packets to initiate or terminate trades by player netID.
- **Notes & Mechanics**: Automates trade session packets.
- **Usage Examples**:
  ```text
  /trade start 12
  /trade end
  ```

---

## 🔤 Complete Alphabetical Quick Index (A–Z)

A complete quick lookup reference covering all **252+** command triggers and aliases:

| Trigger | Primary Command | Category | Parameters | Summary |
| :--- | :--- | :--- | :--- | :--- |
| `//` | `//` | Automation & Bots | - | Toggle spam ON/OFF |
| `//` | `//` | Automation & Bots | - | Quick toggle auto spammer ON / OFF directly in chat |
| `///` | `//` | Automation & Bots | - | Toggle spam ON/OFF |
| `///` | `//` | Automation & Bots | - | Quick toggle auto spammer ON / OFF directly in chat |
| `/ac` | `/autocollect` | Economy & Banking | - | Toggle auto-collect dropped items |
| `/ac` | `/autocomp` | Economy & Banking | - | Compress 100 WLs to 1 DL (run multiple times to compress ... |
| `/action` | `/action` | Packet & Debugging | - | Trigger action (Usage: /action <type>) |
| `/admin` | `/admin` | World & Scanning | - | Scan world for locks and show owner/admin data |
| `/ag` | `/antigravity` | Moderation & Defense | - | Place + activate antigravity using raw packets (item_id f... |
| `/animdebug` | `/debuganim` | Packet & Debugging | - | Debug and inspect captured real vs visual punch animation |
| `/antigravity` | `/antigravity` | Moderation & Defense | - | Place + activate antigravity using raw packets (item_id f... |
| `/antipunch` | `/antipunch` | Moderation & Defense | - | Place + activate antipunch visual jammer using raw packet... |
| `/ap` | `/antipunch` | Moderation & Defense | - | Place + activate antipunch visual jammer using raw packet... |
| `/auctionbid` | `/bid` | Economy & Banking | `<amount>` | Place an auction bid with specified amount |
| `/autocollect` | `/autocollect` | Economy & Banking | - | Toggle auto-collect dropped items |
| `/autocomp` | `/autocomp` | Economy & Banking | - | Compress 100 WLs to 1 DL (run multiple times to compress ... |
| `/autocrime` | `/autocrime` | Automation & Bots | - | Toggle auto-crime mode |
| `/autos` | `/autosurg` | Automation & Bots | - | Toggle auto-surgery mode (auto-selects correct tool) |
| `/autosurg` | `/autosurg` | Automation & Bots | - | Toggle auto-surgery mode (auto-selects correct tool) |
| `/awr` | `/wrench` | Moderation & Defense | - | Open auto-wrench settings (pull/kick/ban) |
| `/back` | `/back` | Movement & Pathfinding | - | Warp to the previously entered world |
| `/BACK` | `/back` | Movement & Pathfinding | - | Warp to the previously entered world |
| `/bal` | `/bal` | Economy & Banking | - | Show current World Lock, Diamond Lock, and BGL balance\nU... |
| `/balance` | `/bal` | Economy & Banking | - | Show current World Lock, Diamond Lock, and BGL balance\nU... |
| `/banall` | `/banall` | Moderation & Defense | - | Ban all players who have spawned |
| `/bandolier` | `/banner` | Visuals & Cosmetics | `<id>` | Set bandolier banner on avatar. Usage: /banner <id 0-50> |
| `/banfire` | `/banfire` | Moderation & Defense | - | Toggle auto-ban fire mode (bans players who use pocket li... |
| `/bankadd` | `/bankadd` | Economy & Banking | - | Deposit all World Locks to storage. |
| `/bankadd` | `/bankadd` | Economy & Banking | - | Deposit all World Locks from inventory directly to bank s... |
| `/bankcheck` | `/bankcheck` | Economy & Banking | - | Check World Lock bank balance. |
| `/bankwith` | `/bankwith` | Economy & Banking | - | Withdraw all World Locks from storage. |
| `/banner` | `/banner` | Visuals & Cosmetics | `<id>` | Set bandolier banner on avatar. Usage: /banner <id 0-50> |
| `/bc` | `/broadcast` | Packet & Debugging | `<text>` | Inject a fake broadcast message locally. Usage: /broadcas... |
| `/beta` | `/betatest` | Packet & Debugging | - | Toggle Beta Tester mode |
| `/betatest` | `/betatest` | Packet & Debugging | - | Toggle Beta Tester mode |
| `/bf` | `/banfire` | Moderation & Defense | - | Toggle auto-ban fire mode (bans players who use pocket li... |
| `/bid` | `/bid` | Economy & Banking | `<amount>` | Place an auction bid with specified amount |
| `/broadcast` | `/broadcast` | Packet & Debugging | `<text>` | Inject a fake broadcast message locally. Usage: /broadcas... |
| `/bubble` | `/bubble` | Visuals & Cosmetics | - | Show talk bubble with OnTalkBubble |
| `/bux` | `/bux` | Packet & Debugging | - | Attempt to set Growtokens with OnSetBux |
| `/buy` | `/buy` | Vending & Shops | - | Purchase an item from the store (opens GUI to enter store... |
| `/cgems` | `/cgems` | World & Scanning | - | Show gem count on all tiles with gems |
| `/chest` | `/chest` | World & Scanning | - | Replace target chest tiles (596/1530/5618/14752) with Dra... |
| `/clearclothes` | `/clearclothes` | Visuals & Cosmetics | - | Clear all saved visual clothing items |
| `/cleartitle` | `/cleartitle` | Visuals & Cosmetics | - | Remove all titles and reset name |
| `/clearvisual` | `/clearclothes` | Visuals & Cosmetics | - | Clear all saved visual clothing items |
| `/clothes` | `/clothes` | Visuals & Cosmetics | - | Visual clothes manager, slot sets, and clothing overrides |
| `/cmsg` | `/cmsg` | Packet & Debugging | - | Send console message (Usage: /cmsg <text>) |
| `/copypunch` | `/debuganim` | Packet & Debugging | - | Debug and inspect captured real vs visual punch animation |
| `/country` | `/flag` | Visuals & Cosmetics | `<flag_id>` | Change country flag display |
| `/cpos1` | `/cpos1` | Casino & Auto-Hoster | - | Highlight and show saved Pos1 or Pos2 |
| `/cpos2` | `/cpos1` | Casino & Auto-Hoster | - | Highlight and show saved Pos1 or Pos2 |
| `/ct` | `/cleartitle` | Visuals & Cosmetics | - | Remove all titles and reset name |
| `/da` | `/deathanim` | Visuals & Cosmetics | `<id>` | Change death animation (0-30). Usage: /deathanim <id> |
| `/dat` | `/dat` | World & Scanning | - | Show tile data. Use /dat scan [radius] [delay_ms] for don... |
| `/daw` | `/daw` | Casino & Auto-Hoster | - | Drop all World Locks, Diamond Locks, and Blue Gem Locks f... |
| `/dawl` | `/daw` | Casino & Auto-Hoster | - | Drop all World Locks, Diamond Locks, and Blue Gem Locks f... |
| `/dbgl` | `/dbgl` | Casino & Auto-Hoster | `<amount>` | Drop Blue Gem Locks. /dbgl <amount> |
| `/dbglvis` | `/dbglvis` | Casino & Auto-Hoster | `<amount>` | Visual drop Blue Gem Locks (no inventory loss). /dbglvis ... |
| `/dbox` | `/dbox` | Economy & Banking | - | Auto-donate items to Donation Box with max inventory count |
| `/dd` | `/dd` | Casino & Auto-Hoster | `<amount>` | Drop Diamond Locks. /dd <amount> |
| `/ddl` | `/dd` | Casino & Auto-Hoster | `<amount>` | Drop Diamond Locks. /dd <amount> |
| `/deathanim` | `/deathanim` | Visuals & Cosmetics | `<id>` | Change death animation (0-30). Usage: /deathanim <id> |
| `/debuganim` | `/debuganim` | Packet & Debugging | - | Debug and inspect captured real vs visual punch animation |
| `/devicecheck` | `/devicecheck` | Proxy & Interface | - | Show tracked spawned players and their device type |
| `/devices` | `/devicecheck` | Proxy & Interface | - | Show tracked spawned players and their device type |
| `/dialog` | `/dialog` | Packet & Debugging | - | Open custom dialog (Usage: /dialog <text>) |
| `/disguise` | `/disguise` | Visuals & Cosmetics | `<item_id>` | Disguise as an item (0=clear). Usage: /disguise <item_id> |
| `/doctor` | `/dr` | Visuals & Cosmetics | - | Apply Dr. title |
| `/door` | `/door` | Movement & Pathfinding | `door_id` | Join a specific door ID in the current world |
| `/doorbf` | `/doorbf` | World & Scanning | - | Bruteforce door IDs using white-door position change dete... |
| `/doordat` | `/doordat` | World & Scanning | - | Scan world for doors and portals with their destination data |
| `/doorid` | `/doorid` | World & Scanning | - | Toggle door ID reveal - shows door IDs when you enter them |
| `/dpos` | `/dpos` | Casino & Auto-Hoster | - | Set target drop position where you stand (same logic & an... |
| `/dposdrop` | `/dropat` | Casino & Auto-Hoster | `[amt]` | Teleport to /dpos position and drop items (all or specifi... |
| `/dr` | `/dr` | Visuals & Cosmetics | - | Apply Dr. title |
| `/dragon` | `/dragon` | Visuals & Cosmetics | - | Toggle Daylight Dragon effect |
| `/dropall` | `/dropall` | Economy & Banking | `[quantity]` | Drop items from inventory (all or specified quantity) |
| `/dropalllocks` | `/daw` | Casino & Auto-Hoster | - | Drop all World Locks, Diamond Locks, and Blue Gem Locks f... |
| `/dropallwl` | `/daw` | Casino & Auto-Hoster | - | Drop all World Locks, Diamond Locks, and Blue Gem Locks f... |
| `/dropat` | `/dropat` | Casino & Auto-Hoster | `[amt]` | Teleport to /dpos position and drop items (all or specifi... |
| `/dw` | `/dw` | Casino & Auto-Hoster | `<amount>` | Drop World Locks. /dw <amount> |
| `/dwl` | `/dw` | Casino & Auto-Hoster | `<amount>` | Drop World Locks. /dw <amount> |
| `/event` | `/event` | Packet & Debugging | `<event_name>` | Activate special events (valentines, easter, halloween, etc) |
| `/eventmenu` | `/eventmenu` | Proxy & Interface | - | Opens the event menu |
| `/exit` | `/warp` | Movement & Pathfinding | `world_name` | Warp into the world specified by world_name (/exit warps ... |
| `/fakeban` | `/warn` | Moderation & Defense | - | Send a fake ban warning notification to yourself |
| `/fakemaint` | `/fakemaint` | Moderation & Defense | `reason` | Send fake maintenance system messages |
| `/fastdoor` | `/fastdoor` | Movement & Pathfinding | - | Toggle fast open/close entrance doors |
| `/fd` | `/fd` | Casino & Auto-Hoster | - | Toggle fast drop mode (auto-confirms drop dialogs) |
| `/fillgbc` | `/fillgbc` | Economy & Banking | - | Auto-fill Well of Love with Golden Booty Chests |
| `/find` | `/find` | World & Scanning | `item_name or item_id` | Search for items in vending machines |
| `/findpath` | `/path` | Movement & Pathfinding | `<x> <y>` | Walk to coordinates using A* pathfinding (fast). |
| `/flag` | `/flag` | Visuals & Cosmetics | `<flag_id>` | Change country flag display |
| `/freeze` | `/freeze` | Packet & Debugging | `[netid]` | Freeze yourself (toggle) or try to freeze a player by netID |
| `/g4g` | `/g4g` | Visuals & Cosmetics | - | Apply G4G title |
| `/gems` | `/gems` | Packet & Debugging | - | Open Gem Settings dialog |
| `/ghostchardddddddddddddddddd` | `/ghostchardddddddddddddddddd` | Visuals & Cosmetics | - | Toggle ghost character mode (send ghost packets and modif... |
| `/growscan` | `/growscan` | World & Scanning | - | Scan world for blocks and items |
| `/growtokens` | `/bux` | Packet & Debugging | - | Attempt to set Growtokens with OnSetBux |
| `/gs` | `/growscan` | World & Scanning | - | Scan world for blocks and items |
| `/gsbeta` | `/gsbeta` | World & Scanning | - | GrowScan beta (uses SEND_MAP_DATA world) |
| `/gui` | `/gui` | Proxy & Interface | - | Toggle the Vin Proxy Premium Desktop GUI |
| `/hitvfx` | `/hitvfx` | Packet & Debugging | `<effectid>` | Trigger hit visual effects (0=default, 1-100=custom effects) |
| `/host` | `/host` | Casino & Auto-Hoster | - | Open casino hoster settings |
| `/ieffect` | `/itemfx` | Visuals & Cosmetics | `<item_id>` | Trigger item effect on yourself. Usage: /itemfx <item_id> |
| `/ignorecsn` | `/ignorecsn` | Moderation & Defense | - | Toggle auto-ignore for CSN/REME seller spam |
| `/ignorecsnchat` | `/ignorecsnchat` | Moderation & Defense | - | Toggle auto-ignore for CSN/REME chat spam |
| `/immune` | `/immune` | Moderation & Defense | - | Toggle immunity to fire and acid / spike damage |
| `/infinity` | `/infinity` | Visuals & Cosmetics | - | Activate Infinity effects (crown/aura/weapon) |
| `/info` | `/info` | Proxy & Interface | - | Open comprehensive commands directory with all available ... |
| `/information` | `/info` | Proxy & Interface | - | Open comprehensive commands directory with all available ... |
| `/initnew` | `/initworld` | Packet & Debugging | - | Reinitialize world state with OnInitNewWorld |
| `/initworld` | `/initworld` | Packet & Debugging | - | Reinitialize world state with OnInitNewWorld |
| `/inv` | `/inv` | Economy & Banking | `[item_id]` | Show inventory info\nUsage: /inv [item_id] |
| `/inventory` | `/inv` | Economy & Banking | `[item_id]` | Show inventory info\nUsage: /inv [item_id] |
| `/invis` | `/invis` | Visuals & Cosmetics | - | Toggle invisibility mode |
| `/itemfx` | `/itemfx` | Visuals & Cosmetics | `<item_id>` | Trigger item effect on yourself. Usage: /itemfx <item_id> |
| `/j` | `/join` | Moderation & Defense | - | Show join mode menu (pull/kick/ban) |
| `/join` | `/join` | Moderation & Defense | - | Show join mode menu (pull/kick/ban) |
| `/legend` | `/legend` | Visuals & Cosmetics | - | Apply Legendary title (of Legend) |
| `/legendary` | `/legend` | Visuals & Cosmetics | - | Apply Legendary title (of Legend) |
| `/levelup` | `/levelup` | Visuals & Cosmetics | `<level>` | Trigger level-up effect. Usage: /levelup <level> |
| `/lg` | `/legend` | Visuals & Cosmetics | - | Apply Legendary title (of Legend) |
| `/load1` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/load2` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/load3` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/load4` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/lockefind` | `/lockefind` | World & Scanning | - | Show latest worlds where Locke stopped by |
| `/locketest001` | `/locketest001` | World & Scanning | - | Send a test Locke OnConsoleMessage and upload a test sigh... |
| `/lvlup` | `/levelup` | Visuals & Cosmetics | `<level>` | Trigger level-up effect. Usage: /levelup <level> |
| `/mailclaim` | `/mailclaim` | Economy & Banking | - | Claim mail reward by message ID - Usage: /mailclaim <mess... |
| `/master` | `/mentor` | Visuals & Cosmetics | - | Apply Mentor title |
| `/maxlevel` | `/maxlevel` | Visuals & Cosmetics | - | Apply Max Level title |
| `/maxlv` | `/maxlevel` | Visuals & Cosmetics | - | Apply Max Level title |
| `/mentor` | `/mentor` | Visuals & Cosmetics | - | Apply Mentor title |
| `/ml` | `/maxlevel` | Visuals & Cosmetics | - | Apply Max Level title |
| `/moddetect` | `/moddetect` | Moderation & Defense | - | Toggle moderator spawn detection |
| `/mr` | `/mentor` | Visuals & Cosmetics | - | Apply Mentor title |
| `/mstate` | `/mstate` | Packet & Debugging | - | Toggle mod state (long punch, zoom, extended range) |
| `/name` | `/name` | Visuals & Cosmetics | `new_name` | Change your display name |
| `/news` | `/proxy` | Proxy & Interface | - | Open comprehensive commands directory with all available ... |
| `/nick` | `/name` | Visuals & Cosmetics | `new_name` | Change your display name |
| `/nickname` | `/name` | Visuals & Cosmetics | `new_name` | Change your display name |
| `/overlay` | `/overlay` | Packet & Debugging | - | Show text overlay with OnTextOverlay |
| `/paintball` | `/paintball` | Visuals & Cosmetics | `<netid> <color>` | Paintball a player. Usage: /paintball <netid> <0xRRGGBB> |
| `/particle` | `/particle` | Visuals & Cosmetics | `<id> [v2]` | Spawn particle effect at your pos. Usage: /particle <id> ... |
| `/path` | `/path` | Movement & Pathfinding | `<x> <y>` | Walk to coordinates using A* pathfinding (fast). |
| `/pathf` | `/path` | Movement & Pathfinding | `<x> <y>` | Walk to coordinates using A* pathfinding (fast). |
| `/pathfind` | `/pathfind` | Movement & Pathfinding | - | Open LuckyProxy-style pathfinder options dialog |
| `/pathfinding` | `/pathfind` | Movement & Pathfinding | - | Open LuckyProxy-style pathfinder options dialog |
| `/pb` | `/paintball` | Visuals & Cosmetics | `<netid> <color>` | Paintball a player. Usage: /paintball <netid> <0xRRGGBB> |
| `/pf` | `/pf` | Movement & Pathfinding | - | Toggle pathfinder mode on/off without opening dialog |
| `/pfx` | `/particle` | Visuals & Cosmetics | `<id> [v2]` | Spawn particle effect at your pos. Usage: /particle <id> ... |
| `/ping` | `/ping` | Proxy & Interface | - | Toggle ping display next to your name |
| `/player` | `/player` | Movement & Pathfinding | `<name>` | Teleport to player by name |
| `/players` | `/players` | Proxy & Interface | - | List all tracked players with netID and tile position |
| `/plist` | `/players` | Proxy & Interface | - | List all tracked players with netID and tile position |
| `/pos1` | `/pos1` | Movement & Pathfinding | - | Set drop/teleport positions (pos1-4 for drops, posback fo... |
| `/pos2` | `/pos1` | Movement & Pathfinding | - | Set drop/teleport positions (pos1-4 for drops, posback fo... |
| `/pos3` | `/pos1` | Movement & Pathfinding | - | Set drop/teleport positions (pos1-4 for drops, posback fo... |
| `/pos4` | `/pos1` | Movement & Pathfinding | - | Set drop/teleport positions (pos1-4 for drops, posback fo... |
| `/posback` | `/pos1` | Movement & Pathfinding | - | Set drop/teleport positions (pos1-4 for drops, posback fo... |
| `/proxy` | `/proxy` | Proxy & Interface | - | Open comprehensive commands directory with all available ... |
| `/proxygui` | `/gui` | Proxy & Interface | - | Toggle the Vin Proxy Premium Desktop GUI |
| `/pullall` | `/pullall` | Moderation & Defense | - | Pull all players who have spawned |
| `/pure` | `/rainbow` | Visuals & Cosmetics | - | Toggle rainbow mode (OnChangePureBeingMode) |
| `/qq` | `/qq` | Casino & Auto-Hoster | `[on/off]` | Toggle Show QQ Number in roulette spin |
| `/ra` | `/respawnanim` | Visuals & Cosmetics | `<id>` | Change respawn animation (0-30). Usage: /respawnanim <id> |
| `/rainbow` | `/rainbow` | Visuals & Cosmetics | - | Toggle rainbow mode (OnChangePureBeingMode) |
| `/rawvar` | `/rawvar` | Packet & Debugging | `<OnFnName> [args]...` | Send any variant call. Usage: /rawvar <FnName> [arg1] [ar... |
| `/relog` | `/relog` | Proxy & Interface | - | Reconnect and relog into the current world |
| `/rema` | `/rema` | World & Scanning | - | Remove your access from all locks in world (auto-confirm ... |
| `/reme` | `/reme` | Casino & Auto-Hoster | `[on/off]` | Toggle Show REME Spin in roulette spin |
| `/resetclothes` | `/clearclothes` | Visuals & Cosmetics | - | Clear all saved visual clothing items |
| `/resettitle` | `/cleartitle` | Visuals & Cosmetics | - | Remove all titles and reset name |
| `/respawnanim` | `/respawnanim` | Visuals & Cosmetics | `<id>` | Change respawn animation (0-30). Usage: /respawnanim <id> |
| `/rift` | `/riftwings` | Visuals & Cosmetics | - | Toggle Rift Wings/Cape effects |
| `/riftwings` | `/riftwings` | Visuals & Cosmetics | - | Toggle Rift Wings/Cape effects |
| `/roleskin` | `/roleskin` | Visuals & Cosmetics | `<skin_id> [icon_id]` | Change role skin and icon. Usage: /roleskin <skin_id> [ic... |
| `/rs` | `/roleskin` | Visuals & Cosmetics | `<skin_id> [icon_id]` | Change role skin and icon. Usage: /roleskin <skin_id> [ic... |
| `/run` | `/run` | Moderation & Defense | - | Join 12 random worlds (9-12 alphanumeric chars) with delay |
| `/rv` | `/rawvar` | Packet & Debugging | `<OnFnName> [args]...` | Send any variant call. Usage: /rawvar <FnName> [arg1] [ar... |
| `/save` | `/save` | World & Scanning | - | Save and warp to designated safe storage world (/setsave ... |
| `/save1` | `/save1` | Visuals & Cosmetics | - | Save current visual clothes set to Slot 1-4 (e.g. /save1,... |
| `/save2` | `/save1` | Visuals & Cosmetics | - | Save current visual clothes set to Slot 1-4 (e.g. /save1,... |
| `/save3` | `/save1` | Visuals & Cosmetics | - | Save current visual clothes set to Slot 1-4 (e.g. /save1,... |
| `/save4` | `/save1` | Visuals & Cosmetics | - | Save current visual clothes set to Slot 1-4 (e.g. /save1,... |
| `/saveworld` | `/save` | World & Scanning | - | Save and warp to designated safe storage world (/setsave ... |
| `/sd` | `/spamdelay` | Automation & Bots | - | Set spam delay in milliseconds (usage: /spamdelay <ms>) |
| `/search` | `/search` | World & Scanning | `[item_name or item_id]` | Search all items in the database without limit |
| `/set1` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/set2` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/set3` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/set4` | `/load1` | Visuals & Cosmetics | - | Load saved visual clothes set from Slot 1-4 (e.g. /load1,... |
| `/setdb` | `/dbox` | Economy & Banking | - | Auto-donate items to Donation Box with max inventory count |
| `/setpos` | `/setpos` | Movement & Pathfinding | - | Teleport to position (Usage: /setpos <x> <y>) |
| `/setsave` | `/save` | World & Scanning | - | Save and warp to designated safe storage world (/setsave ... |
| `/setweather` | `/setweather` | Visuals & Cosmetics | - | Set world weather (0=none, 1-30=types) |
| `/showqq` | `/qq` | Casino & Auto-Hoster | `[on/off]` | Toggle Show QQ Number in roulette spin |
| `/showreme` | `/reme` | Casino & Auto-Hoster | `[on/off]` | Toggle Show REME Spin in roulette spin |
| `/skin` | `/skin` | Visuals & Cosmetics | `<r> <g> <b>` | Change skin color. Usage: /skin <r> <g> <b>  or  /skin <0... |
| `/sm` | `/sm` | Packet & Debugging | - | Toggle SuperMod state |
| `/spam` | `/spam` | Automation & Bots | - | Open Auto Spam settings dialog |
| `/spamdelay` | `/spamdelay` | Automation & Bots | - | Set spam delay in milliseconds (usage: /spamdelay <ms>) |
| `/spawnbgls` | `/spawnbgls` | Packet & Debugging | - | Spawn Blue Gem Locks (item 7188) as live objects across t... |
| `/spos1` | `/spos1` | Casino & Auto-Hoster | - | Punch tile to set Pos1 or Pos2 |
| `/spos2` | `/spos1` | Casino & Auto-Hoster | - | Punch tile to set Pos1 or Pos2 |
| `/tag` | `/title` | Visuals & Cosmetics | - | Open title selection GUI |
| `/talk` | `/bubble` | Visuals & Cosmetics | - | Show talk bubble with OnTalkBubble |
| `/textoverlay` | `/overlay` | Packet & Debugging | - | Show text overlay with OnTextOverlay |
| `/tf` | `/trashfast` | Economy & Banking | - | Toggle fast trash mode (auto-confirms trash dialogs) |
| `/ticon` | `/titleicon` | Visuals & Cosmetics | - | Set title icon (Usage: /titleicon <0-200>) |
| `/title` | `/title` | Visuals & Cosmetics | - | Open title selection GUI |
| `/titleicon` | `/titleicon` | Visuals & Cosmetics | - | Set title icon (Usage: /titleicon <0-200>) |
| `/titles` | `/title` | Visuals & Cosmetics | - | Open title selection GUI |
| `/tp` | `/tp` | Casino & Auto-Hoster | - | Teleport to drop position and collect bets with 1.5 tile ... |
| `/tp1` | `/tp1` | Movement & Pathfinding | - | Teleport to saved position (tp1-4) |
| `/tp2` | `/tp1` | Movement & Pathfinding | - | Teleport to saved position (tp1-4) |
| `/tp3` | `/tp1` | Movement & Pathfinding | - | Teleport to saved position (tp1-4) |
| `/tp4` | `/tp1` | Movement & Pathfinding | - | Teleport to saved position (tp1-4) |
| `/tpplayer` | `/player` | Movement & Pathfinding | `<name>` | Teleport to player by name |
| `/trade` | `/trade` | Packet & Debugging | - | Trade commands (Usage: /trade <start|end> [netid]) |
| `/trashfast` | `/trashfast` | Economy & Banking | - | Toggle fast trash mode (auto-confirms trash dialogs) |
| `/ubiclub` | `/ubiclub` | Proxy & Interface | - | Opens Ubisoft Club/Connect interface |
| `/ubisoft` | `/ubiclub` | Proxy & Interface | - | Opens Ubisoft Club/Connect interface |
| `/vendf` | `/vendf` | Vending & Shops | `item_name or item_id` | Search for vending machines selling a specific item |
| `/vendf` | `/vendf` | Vending & Shops | - | Search for items in vending machines |
| `/vendfast` | `/vendfast` | Vending & Shops | - | Show vending fast actions menu (empty/add/buy) |
| `/vendfind` | `/vendf` | Vending & Shops | `item_name or item_id` | Search for vending machines selling a specific item |
| `/vendloc` | `/vendf` | Vending & Shops | `item_name or item_id` | Search for vending machines selling a specific item |
| `/vendloc` | `/vendf` | Vending & Shops | - | Search for items in vending machines |
| `/vendlogs` | `/vendlogs` | Vending & Shops | - | Show captured vending purchase logs from OnConsoleMessage |
| `/vendsafe` | `/vendsafe` | Vending & Shops | - | Toggle safe vending buy helper |
| `/vendtp` | `/vendtp` | Vending & Shops | `item_name` | Highlight matching vends in current world with particle e... |
| `/vf` | `/vendfast` | Vending & Shops | - | Show vending fast actions menu (empty/add/buy) |
| `/vfx` | `/hitvfx` | Packet & Debugging | `<effectid>` | Trigger hit visual effects (0=default, 1-100=custom effects) |
| `/vision` | `/vision` | Visuals & Cosmetics | - | Apply visual background vision (item_id 1156) on tiles x:... |
| `/visual` | `/clothes` | Visuals & Cosmetics | - | Visual clothes manager, slot sets, and clothing overrides |
| `/vsafe` | `/vendsafe` | Vending & Shops | - | Toggle safe vending buy helper |
| `/w1` | `/w1` | Casino & Auto-Hoster | - | Teleport to player 1 or 2 winning spot, drop prize, and r... |
| `/w2` | `/w1` | Casino & Auto-Hoster | - | Teleport to player 1 or 2 winning spot, drop prize, and r... |
| `/warn` | `/warn` | Moderation & Defense | - | Send a fake ban warning notification to yourself |
| `/warning` | `/warn` | Moderation & Defense | - | Send a fake ban warning notification to yourself |
| `/warp` | `/warp` | Movement & Pathfinding | `world_name` | Warp into the world specified by world_name (/exit warps ... |
| `/wear` | `/clothes` | Visuals & Cosmetics | - | Visual clothes manager, slot sets, and clothing overrides |
| `/weather` | `/weather` | Visuals & Cosmetics | `[weather_id]` | Open the Custom Weather Machine (or /weather <id>) |
| `/win1` | `/w1` | Casino & Auto-Hoster | - | Teleport to player 1 or 2 winning spot, drop prize, and r... |
| `/win2` | `/w1` | Casino & Auto-Hoster | - | Teleport to player 1 or 2 winning spot, drop prize, and r... |
| `/wlbank` | `/wlbank` | Economy & Banking | `<amount>` | Modify World Lock storage amount (positive=deposit, negat... |
| `/wlstorage` | `/wlbank` | Economy & Banking | `<amount>` | Modify World Lock storage amount (positive=deposit, negat... |
| `/wrench` | `/wrench` | Moderation & Defense | - | Open auto-wrench settings (pull/kick/ban) |

---

### 💡 Tips for Using VinProxy Commands
- **Desktop GUI**: Run `/gui` at any time to open the floating visual Dear ImGui window for mouse control.
- **Command Directory**: Run `/proxy` in chat to open the custom in-game dialog tabs.
- **Spam Hotkey**: Type plain `//` into chat to instantly toggle chat spam without opening menus.
- **Panic Escape**: If a moderator joins, `/moddetect` alerts you immediately, and `/run` warps through 12 random worlds to protect your account.
