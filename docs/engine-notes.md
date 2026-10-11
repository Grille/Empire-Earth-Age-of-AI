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
  +0x44 unit manager (Update 0x490AD6, training 0x49014F(req{type, count}, &trained), 5 call sites).
- Unit manager: 14 request lists (deques at +0xC + i*0x14, 16-byte entries {type, count, ..}), +0x13C units in
  production. Room for new units = cap (EEComputerPlayer vf31: own cap +0xCCC unless Ignorieren, else game
  unit limit [0x9183D4] + player +0xB1C/+0xB20) - in production - population (0x556E11). 0x49014F adds the
  requested count to +0x13C even when the building queue (0x5E8908, which re-checks affordability) refuses
  the unit; finished units (0x4905DF) and cancelled requests (0x48EB80) subtract. Anything that makes 0x5E8908
  refuse after the unit manager's own check (vf 0x80) leaks room until the AI trains nothing.
- AI requests include untrainable campaign heroes (e.g. 611 Golden Hind, 196 Court Jester, 33 Grigor II); they
  just fail (no producing building), and cost nothing.
- Unit price: object data +0x4E0 = tech id -> tech 0x4019A1(tech tree player+0x9D4, id) -> +8 data, costs +0x24..
  (food..iron); the object data itself has no prices at +0x24.
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

## Orders, game end, splash

- Give a unit a goal (as EECPAttackSubGroup::vf57 0x45CBEA): alive (+0x3C > 0, +0x1C != -10000, +0x33B == 0),
  then unit 0x5F05BD(goal, append, 0), 0x5EFCF0(priority), typedata slot 29 (unit). EEUGAttackUnit: new 0x40,
  0x60AD45(attacker, target, 1). EEUGLocation: new 0x38, 0x614CE6(float xy[2], 1, 1, 0, 0.0, 0, 0).
- Game decided: player +0xA30 = 1 won, 2 lost. The end screen pauses the lockstep server (EEServer +0x2D), the
  physics gets ticks without steps.
- Build manager queues (cm+0x3C, +4 + epoch*0x14): 12-byte entries {type, x, y} (0,0 = it picks the place);
  Update 0x468540 every 3 s takes the first buildable entry of epochs 0..current (0x555710(type, 0x20, 0) = tech
  available and affordable), places it with 0x46580A or re-queues it with 0x468C37 (append).
- Training request for the unit manager: {type, count, building (0 = any), area (0)}.
- Loading splash: thread 0x5344FB (class "Loading Game Window"), started at 0x5343F8 (jne 0x534400) before the
  engine loads the DLLs; the stop 0x534433 waits for the thread without a time limit.

## Creating units

- In a running game: EEMHackANewUnit, the game's debug spawner (0x64C561): operator new 0x683718 (0x20 bytes),
  ctor 0x650B3C thiscall(int *type, float xy[2], char player), then lockstep server [0x91984C] slot 6
  (msg, 0xFF). Executed in a coming tick (vf10 0x4B7044): ground height via 0x4C0201, buildings checked against
  the map, then typedata slot 13.
- Typedata ([0x9418A0][type]) slot 13 (+0x34): thiscall(GETransformation *, player, 0, 0) -> object, +8 != 0 on
  success, +0x30 the EEUnit. GETransformation: 4x4 floats, position at +0xC/+0x1C/+0x2C. Called directly from
  outside the editor (z = 0, no further setup) the unit crashes the renderer (unit +0x130).
- Type ids and names: `data/db/converteroutput.log`. Walls 467-472, gates 294/442-444/454/473, guard towers
  47/461-466.

## Simulation pacing

- RTSPhysics::Main 0x5532FB (simulation thread): each pass waits for a TMTimer event (TSEvent::Wait at
  0x5533C5, period thread+0x54 = 22 ms), takes the write lock and runs up to 2 steps (EEPhysics vf8 0x546C63).
- A step needs a grant: EEPhysics ([0x919774]) +0xB8 steps left. Grants come as "turn" messages (type 0x28,
  +0x10 steps) that EEServer::vf27 0x4C3839 makes from the lockstep server's communications ticks.
- EEServer ([0x91984C]) is a NESynchronousServer (low-level engine.dll): its Send thread (module+0x55D2E) sends a
  tick every SendInterval ms (+0x198, 350) with PTs-per-CT steps (+0x1F9, 15) = ~43 steps a second, the real
  limit even in single player. +0x190 ticks sent, +0x10 ticks processed in physics.
- Step length: EEPhysics vf3 0x5476C1: +0x98 = 1000 * speed / +0x58 (44) ms game time. The game speed only
  makes the steps longer.

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
  - Slot 7 (0x446C80 -> EECPAttackSubGroup::vf7 0x459CA3): order one unit to attack a target: new EEUGAttackUnit
    (priority 0x7D4), then a move back to a position (8000).
- **Subgroup unit record:** EECPAttackSubGroupUnit (vtable 0x827844; heroes, priests, prophets, air and sea have
  their own vtables with the same slot 2). +0x44 unit, +0x48 current target, +0x4C subgroup.
  - vf5 0x46286F: pick a target (vf4); if it changed, store it in +0x48 and call the subgroup's slot 7. Returns
    "engaged" (+0x48 != 0); while units are engaged, the march waits.
  - vf4 0x4620DF: scores every enemy in the subgroup's lists (subgroup +0x30/+0x40) with vf2, keeps the best.
  - vf2 0x461854: `float thiscall(info)`, info+4 = the enemy unit; 0.0 = not a target.
  - Units fight enemies on the way this way (enemies waiting next to their towers draw the army in).
- **EEUGAttackUnit** (vtable 0x831968, ctor 0x60AD45(attacker, target, ?), 0x40 bytes): +0x2C attacker,
  +0x30 target unit, +0x34/+0x38 target tile.
- Unit goal: unit+0x1EC (EEUGLocation = move, EEUGAttackUnit, EEUGExplore, ...; null = idle).
- **Step cost:** EEAIGPGModifierLandAttackSubGroupRoutePlanner (vtable 0x825CC4) slot 1 (0x4141E8) =
  10/14 per step + 750/3750 for owned territory + 1.85 × the target player's influence in the step's cell.
  - Influence table [0x918E20]: 9-tile cells, per player the sum of type data +0x540 of its units.
  - A tower counts 175, a house 90. Defences are hardly distinguished from any other base, so routes cross tower lines.

## Data files

- `Data\data.ssa`: "rass" archive. Each file is "PK01" + u32 size + 4 bytes, then a PKWARE DCL stream (decoder:
  `scripts/blast.py`, extractor: `scripts/extract_all.py`). Loose files under `Data\` override the archive.
- `.scn`/`.ees`: PK01 (DCL) blocks (`scripts/unpack_save.py`).
- `dbcpbehavior.dat` (AI settings): u32 count + 56-byte records [name string id, index, min1, max1, val1, min2,
  max2, val2, ?, minF, maxF, scaleF, fval, flag] -> global 0x8CFA80 + index*0x38, copied into player fields by
  0x431697. Record names = Language.dll string 28000 + name id. Cheat percentage: record 96 val1 (easy) / val2
  (hard), record 67 fval (medium); the default is 1%.
- Object/unit type names: `data/db/converteroutput.log` (from the data files).
- Language.dll string table: `scripts/strtab.py`. The statistics names are 31201.., the AI settings names 28000..

## Resource manager details

- Update 0x48D263: ComputeTargets 0x48D0A2 every 5 s (call site 0x48D347), AssignCitizens 0x48A554 every 10 s
  (call site 0x48D36A). Timers at +0x16C..+0x17C, serializer 0x48C5B2.
- ComputeTargets: targets proportional to each resource's share of the demand, times all citizens. A negative
  minimum shift permanently rewrites the demand, and the mine cap counter isn't reset per type (bugs).
- AssignCitizens: fills deficits per type. Mines go via 0x48C204 (free slot, no enemy near, same continent),
  leftovers via 0x48A1AE (mines, then wood 0x48C060). Surplus workers are never taken back.
- Demand: added by 0x48B9E9 (costs via AdjustCost), subtracted by 0x48D050 / 0x48B6E8 and by AddResource.
- Unit type table 0x9418A0 has 724 entries (type ids 0..723); typedata +0x4B4 family: 5 land units (siege
  included), 4 ships, 2 buildings. 0x555710(type, 0x20, 0) thiscall on the player: can train/build one now
  (available, affordable, building/builder, pop room) - true for campaign heroes too.
- Damage effect EEEDamagedEffect::vf19 0x4F8048: model = unit vtable +0x54; 0x4D291F(index, state) looks up
  model+0x14C[state] -> map [model+0x234 + index*4]. A model with an inner map without head crashed (0x4D3BD3)
  twice; guarded at 0x4F80A4.
- Building plan: cm 0x46CD18 (attention flag 4) works through a list of {type, wanted count}; 0x2F = "the tower
  of the epoch". Spot finder 0x467B1E: towers (47, 461-466, 328) 0x466ED4 (samples along paths from the base);
  walls have no spot. Placement test 0x62E940 cdecl char(player, type, x, y, w, h, 0), w = h = typedata +0x58C.
  A build queue entry with a position skips the spot finder.
- Wall manager cm+0x48 (EECPWallManager, main 0x499E20, from the Copper Age): plans a ring around the base and
  builds it segment by segment with 0x497C3F thiscall(wm, int from[2], int to[2], type, char register) - picks
  citizens near `from` (0x48F463) and sends them an EEMBuildWall command like a player's; `register` = 1 puts
  the new pieces into the ring plan (0x499C34).
- Research manager (cm+0x40, Update 0x486A74): +8 vector = pool of candidate techs; +0x18 + e*0x10 = scheduled
  lists per epoch (vector begin/end), researched by 0x4865D3 one per tick; +0x114 epoch. Random planner 0x4858D9
  (every player+0xE04 s x random) only while +0x11A is set; schedules with 0x485F57(tech) (requests the building,
  adds demand). Tech record (tech tree lookup 0x4019A1): +4 researched, +6 available, +8 data (+0x24.. cost,
  +0x50 effect list index: 0x93B1B8 + i*0x10, +4 ids, +8 count; effect 0x919E70 + id*0x44: +0x14 unit type
  (-1 all), +0x20 kind (2 = attribute), +0x2C attribute (0x1A.. gathering rates)).
- Attention 0x42F912: per period only player+0xE20 + 1 (4+1) activities are enabled, ordered by strategy +0xA28:
  0 cm+0x7A (request epoch requirement techs), 1 rm+0x188, 2 rs+0x11A (random research), 3 um+0x140, 4 cm+0x7B
  (build manager pass in 0x46CD18), 5 cm+0x7C, 6 [b8c+0x4C]+0x48, 7 b98+0x2D, 8 b8c+0x62.
- Citizens wanted = citizen share (player+0xBDC, 0.45 by default) x population cap (0x48EDDB); trained in
  batches of up to 10. Military slots = (1 - share) x cap (0x47049E); set by 0x43F42E (scenario trigger).
- Mine search 0x48C204 thiscall(rm, type, unit, reverse): first site in zone order (reverse = from the last zone)
  with a free slot (gatherers 0x52E5F7 < table 0x48E817(0x16) = 6), amount > 0, explored, no enemy near, the unit's
  continent - distance plays no part. Mines hold 300000.
- Assign to site 0x48C81C thiscall(rm, unit, site, 0): new gather target (EEUGGather), moves the unit between the
  resource lists (0x48B662 remove, 0x48A4EC add). AssignCitizens only hands out idle citizens (no goal, or a
  finished gather goal).
- Spending: EEComputerPlayer::vf45 0x43F53D (pay a unit/building/tech; skips record +0x4C epoch 0..13) -> vf46
  0x43F526 (stock[type] -= amount). Units and research are paid from the production queue 0x5E8908, buildings
  from 0x62EF8A. AddResource (vf18) is income only.
- Resource zones (player+0xB50, map-wide, shared by all players, 0x28 bytes, +0x18 active for this AI, +8 vector
  of sites). 0x4886F2(zone, 2) returns a site.

## Civilization manager details

- cm (player+0xB90, Update 0x46E2C7): +0x4C player, +0x50 current build project, timers +0x58 next epoch attempt,
  +0x5C, +0x60 military buildings (31 s), +0x64 build, +0x68, +0x6C (59 s), +0x75 one-time setup done,
  +0x76 epoch requirement pending.
- Epoch: SetNextEpochAdvanceTime 0x46DE8C = random(record 28 min..max) minutes x difficulty factor, x2 when
  ahead of / /2 when behind the highest human epoch; AttemptEpochAdvance 0x46C966 asks the research manager
  once for the required techs (0x4860D6).
- Build manager (cm+0x3C): init 0x465E4F (placement area at +0x11C, per-epoch build lists at +4 + i*0x14);
  the lists are 14 deques of {type, x, y} (0x155 per block, front 0x44CD5E, pop 0x44CAAF, append 0x468C37);
  Update 0x468540 every 3 s: the first list whose front 0x555710 (available, affordable, a builder) passes is
  placed with 0x46580A(type, x, y) (one call site, 0x468629) and the round ends - a failed placement goes back
  into its list, so an unplaceable front blocks every later list (type 395 "Single Player Settlement" on random
  maps). 0x465A8E re-plans missing buildings every 120 s.
  base position +0x12C/+0x130; enemy near 0x4660B9(x, y).
- Research manager (cm+0x40): +0x110 next research, +0x118 initialised.

## Misc

- Civ selection: the civ timer (EEMCivTimerExpired 0x4B4F9C) opens the civ dialog when the local player
  has +0xB46 == 0 and +0x467 == 0. The dialog freezes game time (not the pause flag).
- Command line: only `hostgame`, `joinbyip`, `update_saved_games` and `-loadgame "name.ees"` (no space after
  the name, no '-' or '/' in it; parser 0x522A36, loader 0x521F94).
- Intro movies: AddMovie calls at 0x538909/0x53892A/0x538945.
- Saving: EEASaveGame (vtable 0x825084, 0x30 bytes, ctor 0x404FA1(const UString &file, bool, UString folder
  by value)), queued like the pause action; the folder is relative to the game folder ("data\Saved Games\").

## Art of Conquest (EE-AOC.exe) - first overview

Not supported yet; notes for the port. Steam and GOG `EE-AOC.exe` have identical code. AoC's `Low-Level Engine.dll`
also loads every `*.dll` in its folder, so the mod gets in the same way.

- RTTI: 1741 classes (base 1717), 1696 shared. New: the Space Age (EECPASGSpace*, EEUCSpaceship..., carriers),
  civ powers (EECivPowersManager, EECCCloaking/MindControl/MeteorStorm...), an A* continent graph. Gone: WON lobby.
- Vtables: same slot counts for all classes we hook (EEComputerPlayer 56, EEPlayer 55, route planner 4, ...);
  EECPASGLand has 148 slots instead of 147 (the extra one lies behind slot 97).
- Code of the hooked functions is identical, only addresses and struct offsets move. Player fields shift by
  region: +0x34 around 0xBA4 (difficulty 0xBA4 -> 0xBD8), +0x38 around 0xCCC (AI cap -> 0xD04), +0x50 around the
  toggles (0xEF9/0xEFB/0xF02 -> 0xF49/0xF4B/0xF52). An offset table can be generated by pairing matched functions.

| | base | AoC |
|---|---|---|
| EEComputerPlayer vtable / AddResource / AdjustCost / cap | 0x8264C0 / 0x42F783 / 0x430FBD / 0x4311B9 | 0x83A57C / 0x42D1A5 / 0x42EACE / 0x42ECCA |
| EECPASGLand vtable / route / route done | 0x826CA8 / 0x44BC57 / 0x44BDBF | 0x83AD7C / 0x44B3C7 / 0x44B53D |
| land route planner vtable / step cost | 0x825CC4 / 0x4141E8 | 0x839C70 / 0x410074 |
| EEASaveGame / EEAUserPauseGame vtable | 0x825084 / 0x830E94 | 0x839060 / 0x846790 |

`scripts/vtables.py` (with `EE_EXE=<exe>`) prints vtables and slots for any class.

### What AoC changed in the code (scripts/diff_exes.py)

Of the 6030 functions both exes name via RTTI (`Class::vfN`), 5654 (94%) have the same size; 376 changed.
By subsystem (same/changed): computer player EECP 408/70, AI framework/pathfinding EEAI 127/13, units EEU 336/73,
physics/movement models EEP 107/49, interface EEUI 819/31, scenario EES 611/12, effects EEE 389/20.
- EEComputerPlayer::vf6 is the AI's save/load serializer: +7 KB = more AI state (matches the field shifts).
- Attack subgroups (EECPASGLand 19, EECPASGAir 10, EECPAttackSubGroup 10 changed): the samples read so far add
  checks for new unit types (e.g. types 0x2D6/0x2D9/0x2DD) and continent-chain routing (the new A* continent graph,
  EEAIAStarGraphContinentalChainGoal) - mostly support for the new content and multi-continent paths.
- Movement models (EEPMNaval/Helicopter/Land/Airplane/SubmergedNaval) and unit classes changed most outside the AI.
  Caution: where a base class gained virtual functions the vfN numbering shifts, so EEUnitClass/EEComplexUnit
  "changes" are partly numbering artifacts and need matching by code, not by name.

## Scenario / save file format (.scn / .ees)

From the author's earlier C# project EEScnEdit (GitHub, `EEScnEdit/Headers`, `EEScnEdit/Data`), which reads and
writes the header and the lump container and parses terrain. Strings are length-prefixed.

- Header: u32 size, name, description, u64 **version** (candidate for telling EE and AoC files apart), player
  names, date, u8, 16 x PlayerHeader, Settings, campaign name, scenario name, u32, u8, forced name, u32,
  u32 n + n x u32 lump types, scenario texts, u32 system uptime.
- PlayerHeader: u8, u8, bool shared line of sight, u32 flags, name, float, u32 id, u32 color, u32 team,
  u32 starting citizens - the same order as the in-memory game setup slots (+0x20 index, +0x24 color,
  +0x28 team, +0x2C citizens, +0x34 flags).
- Settings: u8 x3, u32 game speed, u32, u32 game variant, u32 map size, u32 start epoch, u32 end epoch,
  u32 resources, u32 unit limit, u32 wonders for victory, u32 AI difficulty, bools victory allowed, lock teams,
  lock speed, reveal map, cheats, u8, custom civs, u8 - matches setup +0x3D8.. (mode, epochs, map size, limit,
  difficulty).
- Lumps: u64 version, u32 type (1 unknown, 2 seed, 3 camera, 4 terrain, 5 player data/entities, 6 triggers,
  7 misc, 8 unknown, 10 gfx effects, 11 calamities, 12 unknown), u32 size, then a compression header (bool
  compressed, u32 size, u32) and the (PKWARE DCL) data.
- Player data lump (started): a list of "PlayerEnabler" records {u32 id, bool, u32, u32 per player x16, u32} -
  probably the per-player enable state of units/techs; the lever for locking AoC-only content in EE scenarios.
- Triggers: name, description, bools active/looping/fired/active in cinematic, u32 conditions-true-for,
  u32 seconds true, u32 id, then condition and effect bindings.

## Display

- SetResolution 0x65F314(width, height, force, flag): stdcall, ECX unused. Menus use 1024x768; games use
  [0x9193FC] x [0x9193F8].
- The scenario editor is fixed at 800x600 in both entries: main menu 0x6601D9 (call 0x6601F1) and return
  from test play 0x65FD21 (call 0x65FD6D). AoC uses 1024x768 in both. The mod redirects both calls
  (aimod.ini [editor] resolution).
- Camera scrolling: see dll/camera.c (EEAScroll, AoC smooth scrolling).
- Editor at 1024x768 (done, dll/editor_layout.h generated from the AoC diff): AoC also moved the whole editor layout (constructor 0x58D3E1 ->
  AoC 0x59D615: bounds 599/799 -> 767/1023, minimap 587-795 x 454-662 -> 854-1023 x 598-767, buttons
  625-644 x 580-599 -> 849-868 x 718-737; layout 0x58E400 -> 0x59E80F: map view bottom 438 -> 606) and the
  panel code (~0x58E000-0x5B1000, many hard-coded positions), and ships a 1024x768 background
  (textures\scenfull_R_C.tga, 4x3 tiles of 256x256 32-bit; base: .sst 800x600). The editor background is a
  UIFullBackground("textures\Scenfull", 4, 3, parent, 800, 600) from Low-Level Engine.dll.
- .sst textures = 15-byte header (00 01 01 00 | 00 00 00 01 | 00 00 00 01 | 00 00 00) + a plain 24-bit TGA.
  The base Low-Level Engine.dll only loads .sst; AoC's also loads .tga.

### Windowed mode (dll/display.c)

The engine keeps the windowed path SSSI used during development; the byte 0x91945A (1 = fullscreen)
selects it. It is already 1 when the DLLs load and nothing writes it later. Windowed (0), the game:

- creates a titled, resizable window: DX7Screen::CreateAWindow mode 1 (WS_OVERLAPPEDWINDOW), the window is
  resized to each video mode (GEWindow::Resize);
- takes DirectDraw with DDSCL_NORMAL (DX7Screen::Capture flags 0x408 instead of 0x413) and never changes the
  display mode (DX7Screen::SetMode only acts when the screen's fullscreen byte +0x3D is set);
- renders into an offscreen back buffer of window size with an attached z-buffer (ActivateScreen) and
  presents it with a Blt to the primary at the window's client origin (DX7Rasterizer::ShowScene, vtable
  slot 66; fullscreen: Flip);
- hides/restores the window on deactivation instead of releasing surfaces (0x67DE3E / 0x67E542).

What breaks on current Windows and what the mod does (all hooks are installed by replacing every pointer
to the function in the read-only data of DX7HRDisplay.dll / DX7HRTnLDisplay.dll, because the live objects
are subclasses with their own vtables):

| Problem | Cause | Fix |
|---|---|---|
| black window | the DDSCL_NORMAL primary is an emulated 1024x768 16-bit surface (Windows and dgVoodoo pass-through alike); the Blt to it fails with DDERR_INVALIDRECT. GetDC on the 3D back buffer also reads black | present by Lock + StretchDIBits into the window; the original ShowScene is not called windowed |
| editor crash | at 1024x768 that Blt only overhangs by the title bar and copies past the end of the surface (DDRAW.dll) | as above: original ShowScene skipped |
| intermittent crashes at start | SetMode skips recording the mode index (+0x38) when windowed: GetCurrentMode reads garbage (0x535958), ActivateScreen builds the primary with a garbage pixel format (CreateSurface) | SetMode only records the index; GetCurrentMode/Index clamp it |
| land terrain invisible (water, models fine) | the z-buffer of a new windowed back buffer starts out unusable for the terrain | one Lock/Unlock of the attached z-buffer per new back buffer |
| editor: scanlines where the mouse moved | the UI's copy-dirty rectangles restore regions from the front surface (page-flip logic); windowed that is the emulated primary | copies from/to the front surface skipped (the back buffer is never flipped) |
| loading splash stays | started (0x5343F8) while the flag is still 1, its stop (0x534433) checks the flag again | jne at 0x53443D removed when windowed |
| no Windows cursor, title bar not clickable | the mouse is a DirectInput device in exclusive mode (ISMouse::Resume, LLE +0x4C837 `push 5` = EXCLUSIVE\|FOREGROUND): no cursor, no mouse messages. The window procedure 0x6622DD also answers every WM_SETCURSOR with SetCursor(NULL), even over the title bar and in the background | non-exclusive (`push 6`, also in ISInitialize +0x4A76E); the window is subclassed: game cursor only in the client area while in front, Windows' cursor elsewhere |
| mouse speed feels wrong | the game cursor (RTSMouse [0x9197A8], +4/+8 position, x range +0x18..+0x1C, y +0x20..+0x14) moves by raw DirectInput deltas scaled by the desktop size and divided by the smallest delta seen so far (0x510E5D, "MinXValue"): no Windows pointer speed/acceleration | windowed it follows the real cursor (client position mapped to the game resolution, GEWindow +0xC/+0x10, then clamped to the range - which the game narrows to the map area while dragging in the 3D view; the 4 calls of 0x510E5D hooked, the original still reads the wheel). On the title bar it holds; outside the window it clamps to the edge (edge scrolling) |
| clicks on the title bar / other windows reach the game | non-exclusive, the device sees every click while the window is in front | unacquired while the cursor is outside the client area and no button is held; Acquire + RecomputeCurrentButtonState on re-entry |
| window minimizes on focus loss | WM_ACTIVATEAPP(0) calls CloseWindow (0x662670) | the je at 0x662666 made a jmp |
| clicking into the window doesn't give it input | WM_ACTIVATEAPP/WM_ACTIVATE resume the devices, but Acquire (FOREGROUND) fails before the window is foreground; the intro sequence (0x538xxx) also ends with Suspend. WM_ACTIVATE returns without DefWindowProc (no keyboard focus) | in front, a suspended device (+4) is resumed (vtable +0x10, every 250 ms at most); SetFocus after WM_ACTIVATE |
| crash in DDRAW.dll+2B133 (dgVoodoo) at a resolution as big as the screen | the window (client + frame) is larger than the desktop; Windows cuts it and dgVoodoo then reads past a surface in the UI's CopyTo | LLE's SetWindowPos (GEWindow::Resize) wrapped: the window always fits the work area, the client scaled down with the aspect kept |

Editor minimap: EEUIMiniMap (0x65DF30..) sizes its picture as screen width * 1/800 and height * 1/600
(0x8358A8 / 0x82CBCC, also for mapping clicks) - right for the in-game frame, which scales with the screen,
too big for the editor's fixed AoC frame at resolutions above 1024x768. The 6 operands read the mod's factors:
1.28 / width and 1.28 / height while the editor is active (set by its SetResolution hooks), the originals after
every other SetResolution call (7 sites).

Borderless (`mode=borderless`) and Alt+Enter: the window is subclassed; borderless swaps
WS_OVERLAPPEDWINDOW for WS_POPUP and covers the monitor. The game sizes the window itself (GEWindow::Resize,
LLE +0x49E0D: AdjustWindowRect(0xCF0000) + SetWindowPos with SWP_NOMOVE, stores the game size at GEWindow
+0xC/+0x10), so WM_WINDOWPOSCHANGING sets the size on every change: monitor rect when borderless, client =
game size otherwise (not while the user drags the frame or when maximized). The display DLLs never read the
client size - the back buffer follows the game resolution - so present scales it into the client area
(aspect kept, bars filled black) and the mouse maps through the same rectangle. DPI: CreateWindowExA in both
display DLLs' import tables runs in a per-monitor-aware thread context, so the window isn't bitmap-stretched
by Windows; present and the mouse code switch their (unaware) threads to the window's context for geometry calls.

Presenting (dll/present_d3d.c): by default the locked frame is uploaded to a Direct3D 11 texture and drawn
by a flip-model swap chain on the game window (GPU scaling, tearing allowed when vsync=0); `present=gdi`
keeps the CPU path (StretchDIBits, or for `smooth=1` a multithreaded bilinear scaler - GDI's HALFTONE took
~35 ms per frame at 2560x1440). The GDI path into a visible window held 60 fps on a 120 Hz screen; headless
measurements: game ~10 ms per frame at 2560x1440, Direct3D present ~1.3 ms, GDI ~2.4 ms (ddinfo "present
timing", ddpoke bench).

dreXmod.dll briefly hooks 0x534433 while the DLLs load (a JMP at the function start, restored later).
Plain Windows ddraw.dll crashed EE at startup here; with dgVoodoo's ddraw.dll in the folder (even in
pass-through) it starts.
