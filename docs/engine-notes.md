# Engine notes: Empire Earth 1.04 (`Empire Earth.exe`, image base 0x400000)

Verified addresses and layouts that the mod relies on. RTTI class names and source paths are intact in
the exe. Ghidra's function names (`Class::vfN`) come from the vtables. *thiscall* = `this` in ECX; the DLL
hooks them as `__fastcall` with a dummy EDX.

## Loading, threads, globals

- `Low-Level Engine.dll` `LoadLibrary()`s every `*.dll` in the game folder (random-map plugin scan), which is
  how `eeaimod.dll` gets in. It pins itself, because the engine may `FreeLibrary()` non-plugins.
- The simulation and AI run on the `RTSPhysics::Main` thread (0x5532FB), not the window thread. The control
  pipe executes commands from the hook on `call [0x8236B0]` (TSThread::Receive) at 0x553348.

| Address | Meaning |
|---|---|
| 0x918374 | player pointers [16], count at 0x9183CC, local player index 0x918E84 |
| 0x9183BC | map: +0x1816FC size (tiles), +0x181724 unit grid (3 tiles), +0x181720 grid (10 tiles) |
| 0x9183C0 | game speed (float, normal 2.5); notify vtable slot 11 of 0x918300 |
| [0x91977C]+0xD0 | game time (ms) |
| 0x918410 | game settings: +0x3E0/+0x3E4 start/end epoch, +0x3F0 unit limit, +0x409 scenario flag (= byte 0x918819) |
| 0x918BF4 | game type: 0 random map, 1 scenario, 2 saved game |
| 0x918EA2 | paused byte; pause action EEAUserPauseGame vtable 0x830E94, queued via 0x66192F on [0x91977C] |
| 0x918CD0 | EECameraControl: look-at +0x78/+0x7C; center via EEACenterCamera ctor 0x560179 + 0x401F3E(x, y) |
| 0x9418A0 | object type data [type] (see Units) |
| 0x918E20 | EEInfluenceTable (see Attacks) |

Game start: 0x52599B(state, setup). The setup has 16 slots of 0x38 bytes (+0 type 1 human/0 computer/4 closed,
+0x20 index, +0x24 color, +0x28 team, +0x2C citizens, +0x34 flags), +0x3A8 map name (UString), +0x3D8 mode
(0 random map, 1 scenario, 2 saved game, 3 editor test, 4 campaign), +0x3E0/+0x3E4 epochs, +0x3E8 map size,
+0x3F8 difficulty. The command line only knows `-loadgame "name.ees"` (parser 0x522A36).

## Players

| Offset | |
|---|---|
| +0x45C | index |
| +0x465 | explore all (no fog) |
| +0x4A8 | EEPlayerStatistics: 31 series at +0x18 + n*0x18 {?, begin, end} of 8-byte samples, latest = 0x55DCD3(n). Series n = Language.dll string 31201+n: 1 buildings lost, 2 buildings destroyed, 3 units killed, 4 units lost, 5 conversions, 6 calamities, 7..11 food, wood, gold, stone, iron, 12 epoch, 13 army, 14 citizens. Add 0x55DBCF(n, d). |
| +0x9D4 → +0x4E4 | current epoch (0 = Prehistoric) |
| +0x9E4 + i*4 | diplomacy towards player i: 0 allied, 1 enemy (setter EEPlayer::SetDiplomacy 0x55B8E1(i, stance, 0)) |
| +0xAF0 | int stock[7] (2 food, 3 wood, 4..6 stone, gold, iron) |
| +0xB46 | civilization chosen (civ dialog opens at the civ timer if 0) |
| +0xB50 | vector of resource zones (map-wide, shared by all players) |
| +0xB90 | EECPCivilizationManager (computer players) |
| +0xBA4 | difficulty 0 easy, 1 medium, 2 hard |
| +0xEF4.. | toggles: Angreifen EF4, Aktiv EF9, Cheat EFB, Bauen EFC, Erkunden EFD, Sammeln EFF, Verteidigen F00, Ignorieren F02 |

EEComputerPlayer vtable 0x8264C0: slot 18 AddResource (0x42F783), slot 27 AdjustCost (0x430FBD: price × pct/100 if
Cheat; pct easy +0xD48 int, medium +0xBB4 float, hard +0xD4C int). Update 0x43F91A (called at 0x42F3CD).

AI settings table `dbcpbehavior.dat`: 56-byte records at 0x8CFA80 + r*0x38 (+0x10 val1, +0x1C val2, +0x30 fval),
copied into player fields by 0x431697:

| Field | Meaning |
|---|---|
| +0xD2C/+0xD28 | first attack min/max (minutes) |
| +0xD00/+0xCFC | interval between attacks |
| +0xD30/+0xD34/+0xD38 | attack group % easy/medium/hard |
| +0xCD8/+0xCDC/+0xCE0 | instant attack chance on contact / for resources / revenge |
| +0xED8 | next attack time, scheduled by 0x43DD2F |
| +0xEF5 | first attack done |
| +0xD58/+0xD5C | epoch timer min/max (SetNextEpochAdvanceTime 0x46DE8C) |

**Scenario bug:** the scenario branch of the game start sets 0x918819. Several AI routines (attack timing 0x431141,
0x472427, 0x49400D, ...) then skip their planning. The mod clears the byte only while EEComputerPlayer::Update runs.

## Computer player managers

EECPCivilizationManager `cm` = player+0xB90 (Update 0x46E2C7):
- +0x38 resource manager, +0x3C build manager (Update 0x468540), +0x40 research manager (0x486A74),
  +0x44 unit manager (training 0x49014F(req{type, count}), 5 call sites).
- +0x58 next epoch attempt time, +0x76 epoch requirement pending; AttemptEpochAdvance 0x46C966.
- Epoch techs: tech tree +8 + epoch*0x54 → +8 data: costs +0x24.. (food..iron), +0x40 needed / +0x48 researched
  requirement techs. Stone Age 850 food; Copper Age 750 food, 400 gold, 400 iron. Farms and granaries arrive in
  the Copper Age (epoch 2).

EECPResourceManager `rm` (cm+0x38): +0xA4 7 unit lists (stride 0x14, count +0x10), +0x134 demand[7],
+0x150 targets[7]. Update 0x48D263: ComputeTargets 0x48D0A2 every 5 s, AssignCitizens 0x48A554 every 10 s.
- Food search 0x48BBA8.
- Mines 0x48C204.
- Leftover citizens 0x48A1AE: any free mine anywhere, which is the cause of citizens walking into enemy towers.
- Resource site (EEUnit): +0x38 type, +0x40 amount, +0x1C/+0x20 tile, +0x2C type data (+0x4B8 type id; 0x37 = forage).
- Gatherers per site: 0x52E5F7 on 0x918300, cap = table 0x917BB0 entry 0x16 (6).
- Enemy near: 0x4660B9 on the build manager. Region: 0x425F9F(0x917BC8, pos, 2).

## Units

Unit grid [[0x9183BC]+0x181724]: +4 cells of 16 bytes {?, begin, end, cap} holding `EEUnit*`, +8 cell size (3),
+0x10/+0x14 cells per axis. Big buildings appear in several cells.

| EEUnit | |
|---|---|
| +0x14 → +0xC | uint16 per player, low byte != 0 = seen by that player (0x404A8A) |
| +0x18 | owner player |
| +0x1C/+0x20 | tile x/y (int) |
| +0x2C | type data |
| +0x34 | live stats: +0x9C range, +0xD8 attack (floats) |
| +0x60 | unit id (lookup 0x4111F6) |

Type data (0x9418A0[type]): +0x4B4 family (2 building, 29 fortress, 5/6 units), +0x4B8 type id, +0x420 range,
+0x460 line of sight, +0x510 attack, +0x55C hit points, +0x540 influence. Type names: `data/db/converteroutput.log`.
Guard towers 47, 461..466; AA 145/147; town center 35; house 37; citizen 32.

## Attacks

- **Launch:** 0x46F4FF, when 0x43121E says the attack time has come. It picks an enemy player via the
  intelligence files (EECPIntelligencePlayerFile: +0x44 player, +0x34..+0x40 known area).
- **Subgroup:** EECPASGLand (vtable 0x826CA8).
  - Slot 80 (0x446CA1) starts a march: the subgroup's position (slot 53) and goal (target struct asg+0x100:
    +4/+8 tile, +0xC unit id, +0x10 kind, 1 = unit, +0x14 player).
  - Slot 94 (0x44BC57) requests the route (0x4A15E7, queued).
  - Slot 97 (0x44BDBF) receives it: waypoints in the deque at asg+0x60 ({x, y, ?}, 0x155 per block, +0x70 count),
    about 10 tiles apart.
  - asg+0x190 own player, asg+0x234/+0x238 current waypoint.
- **Step cost:** EEAIGPGModifierLandAttackSubGroupRoutePlanner (vtable 0x825CC4) slot 1 (0x4141E8) =
  10/14 per step + 750/3750 for owned territory + 1.85 × the target player's influence in the step's cell.
  - Influence table [0x918E20]: 9-tile cells, per player the sum of type data +0x540 of its units.
  - A tower counts 175, a house 90. Defences are hardly distinguished from any other base, so routes cross tower lines.
