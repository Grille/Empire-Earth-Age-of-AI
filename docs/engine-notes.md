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
| +0x4A8 | EEPlayerStatistics: 31 series at +0x18 + n*0x18 {?, begin, end} of 8-byte samples, latest = 0x55DCD3(n). Series n = Language.dll string 31201+n: 1 buildings lost, 2 buildings destroyed, 3 units killed, 4 units lost, 5 conversions, 6 calamities, 7..11 food, wood, gold, stone, iron, 12 epoch, 13 army, 14 citizens. Add 0x55DBCF(n, d) thiscall(stats). Kills: EEUnit::vf28 0x6172BC (damage) at 0 hit points calls it at 0x617780 on the attacker's player +0x4A8 with 3 (unit) / 2 (building), the dead unit in ESI - not for nature's units or one's own (diplo.c counts kills by pair there; matches the totals exactly). |
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
- **Epoch requirements** (2 per epoch in the suites): "new things" of the current epoch - EETechTree::vf4 (0x5BC4B6,
  a tech finished) calls 0x5BD9B8, which counts +0x48 of the next epoch's record for techs whose data +0x7D is set.
  Objects are techs too: data +0x7C set = a unit or building (type +0x6C), counted the first time it is made.
  - The AI's request 0x4860D6 thiscall(research manager cm+0x40, count): collects the candidates of all epochs up
    to the current one (0x5BBAF5 thiscall(tech tree, epoch, vector*); vector {?, begin, end, cap}, freed by
    0x61ABEC) with +0x7D/+0x7C set, +0x48 not 4/6, type not 0x23/0x29-0x2B/0x34 (Town Center, Capitol, Settlement,
    Granary), first those it can make now (0x485C8F), sorted (0x486C7F); then techs -> 0x485F57, units -> unit
    manager 0x48FB70(type, 1, 0, 0), buildings -> 0x46802D(type, ?, 0). Nothing makeable: falls back to every
    candidate; no candidate at all: credits the count itself (0x5BD9B8 x count).
  - The fallback picked the editor's decoration ("x Statue - Olmec" 400, "x Statue - Moai" 402; object name at
    object data +0x1C, "x " = decoration, "b  " = building): unplaceable, the request stayed pending for minutes.

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

### Ranged attacks and projectiles (dll/aim.c)

- Firing: 0x61E11C (attacker class, thiscall(unit)). Aim point from 0x633413 (cdecl: attacker, &target out,
  float aim[3], 0): the target's model hit offset (type vf21 +0x26C) or its tile centre and tile height. With a
  projectile class (attacker class +0x3FC) 0x62028B / 0x623A53 create the projectile, turn it to the aim point
  and call projectile class vf42 (EEUCProjectile 0x607E2B). Without one the damage is applied directly
  (0x61DE61). The projectile itself also runs 0x61E11C when it arrives (half the calls).
- Arc: 0x62CBB3 (cdecl: attacker, attacker type, from[3], to[3], &pitch) - speed type +0xC0, gravity
  [[type+0x60]+0x3FC]+0x438; low arc unless class +0x5E9 (or type +0x15C, unit +0x342, unit +0x260 == 8); returns 0
  out of reach, then 45 degrees. EEPMProjectile::vf1 0x550695 flies that arc from the launch point +0x10C..
  (heading +0x188, pitch +0x18C, elapsed ms +0x28C) with no steering.
- Hit: on the target's footprint (tiles +0x1C/+0x20, size class +0x58C); EEUCProjectile vf55 0x6078F9 is
  mostly the miss path (it came down beside the target). Missiles (EEUCMissile 0x832E90, torpedoes 0x832CC0)
  steer on their own.
- Missiles: EEPMMissile::vf1 0x54E67D climbs while +0x28C (ms, counted down by vf45) > 0, then 0x54E790 steers
  every step towards 0x633413's point (call 0x54E850): turn rate type +0xB4, speed +0x238 -> top +0xAC (accel
  +0xA8), slower the more it turns. Hit: EEUCMissile vf55 0x605C01, squared distance <= 2.25 (0x831860); no fuel,
  it flies until it hits or meets the ground. Launch: EEUCMissile vf42 0x606109 thiscall(class, missile, aim)
  ret 8 (slot 42 always gets the aim; the missile ignores it). AA missile tower 147: missiles ~2.5 tiles/s;
  aircraft ~1.05.
- Damage (0x61DE61): attack = per-player type (thiscall 0x492969 on player +0x874, type id) +0xD8; x table
  0x8D2050[attacker class +0x4C0 + target class +0x4C0 x 6] x bonus 0x4D5F2C (thiscall on 0x917DE0: class +0x4C8,
  target class, both player indices +0x45C; ret 0x10); minus armour (target stats +0xE0 + attack class x 4); x the
  height factor 0x61E500 (land/building attackers only); rounded, at least 1; hit points -> target vf 0x70.
- Fire calls (0x61E11C thiscall(class, unit)): 0x606067, 0x607D79 (attack actions), 0x60ABB0 (EEUGAttackGround),
  0x60B33C (EEUGAttackUnit).
- Math imports of the game's own DLL: UMCos 0x823304, UMArcTan2 0x82325C (cdecl). msvcrt rand 0x824648, srand
  0x824644.
- Unit movement: +0x238 current speed (tiles/s, 0 standing, set by 0x54A55B), +0x21C the same clamped to
  the type's minimum; facing = matrix +0x130 local y axis (+0x134, +0x144). +0x118/+0x11C is a waypoint, not
  the previous position.

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

## Text on surfaces

- DX7Surface (display DLLs, 0x60 bytes): +0x48 IDirectDrawSurface7, +0x54 text HDC, +0x5D text locked. TextLock
  = GetDC (vtable +0x44) into +0x54, TextUnlock = ReleaseDC (+0x68); GetTextDC returns +0x54. Vtable exported
  (??_7DX7Surface@@6B@), slot 0 = the (vector) deleting destructor.
- DX7Rasterizer::Draw(GEText*, GESurface*) / Draw(GEText*, const GERectangle&, GESurface*): SetBkColor,
  SelectObject(font), GetTextExtentPoint32A, ExtTextOutA on that DC; the only GDI text calls of the display DLLs.
  Fonts: CreateFontA in Low-Level Engine and the exe. The game's text goes onto the 3D back buffer (video memory).
- Bitmap fonts: GEFont +0x18 HFONT, +0x24 GEFontBitmap (SetFontBitmap adds a reference). DX7Rasterizer::Draw
  (GEText*, rect, GESurface*) uses GEFontBitmap::Draw (LLE +0x27C12) when the font has one - into the current
  render target, the surface argument is ignored; the variant without a rectangle has no such branch. The
  constructor (LLE +0x2724E, 0x24 bytes; GERasterizer*, GEFont*, screen) draws tmFirstChar..tmLastChar-1 with GDI
  into 256x256 surfaces (CreateNewFontTexture), green channel -> alpha (CreateSurfaceAlpha), one atlas cell per
  character cut out by its advance width; Draw reads characters with movsx (+0x27A46, +0x27C9A). The game makes
  bitmap fonts only for a few UI controls (0x6529C1, 0x652B26).
- With DDrawCompat v0.7.1 every ExtTextOutA on a surface DC leaves a 1472-byte font realization entry in
  gdi32full's width cache (LpkUseGDIWidthCache -> GdiRealizationInfo), never freed (see testing.md).

## Victory and chat

- 0x531913 (called every step from 0x546DE2) decides the game: players without units are resigned, and once only
  one alliance group is left (list at game state +0x9B4, 0x18 bytes per group) its members win (+0xA30 = 1, the
  others 2). Allied to everyone is not enough: two vassals allied to the player but at war with each other keep it
  running. 0x545D2A then sets +0xE6 and the simulation stops for good - there is no playing on after the end screen.
  EEPlayer vtable slot 29 (+0x74) = is human (EEHumanPlayer 0x4B129C returns 1).
- Standard victory: game state +0xB96 (from the settings +0x408 at the game start - the editor's flag). Only with it
  does 0x531913 end the game when one alliance group is left and take out players whose +0xB14 (object count) is
  below 1; without it only scenario triggers or no humans left end a game. The mod's auto_victory=0 clears it and
  sets it back when the player ends the game.
- Chat line from a player: EEMInGameChat::vf10 (0x4B78C7, vtable 0x82936C slot 10), sender = the player whose
  +0x464 equals msg+0xA (0x52E6EF on the game state), text = UWideString at msg+0x10. Every UWideString here keeps a
  narrow char * at +4 despite its name. The chat box (EEUIECESendMessage::vf1 0x5C28D6) checks cheat codes first,
  then sends 0x5C2A2F thiscall(new(0x24), UWideString *) through [0x91984C] vtable +0x18 (message, 0xFF = all).
- Leaving: the menu's Resign (EEUIEMGOEResign::vf1 0x5D0578, confirmed: 0x5D05CC) sends EEMResign (vtable 0x828820
  slot 10, 0x4BB906: the sender resigns, result 2); Exit Game (EEUIEMGOEExitGame::vf1 0x5D0AFB, confirmed: 0x5D0B4F,
  pushed at 0x5D0B2B) disconnects without a result. Both confirmation callbacks are plain cdecl void(void).
- System lines like "Spieler X wurde besiegt!" (0x55BCEA): 0x661E91 thiscall(ui [0x91977C], float colour[3] (white),
  sender UWideString = string 0x7C45 "- SPIEL -", text, ms, 0).

## Saved games

- Every save goes through 0x540ACA cdecl(UString *file, int, int, UString folder by value) -> 0 on success (else the
  error dialog 0x65C6A9); 4 call sites (save dialog EEASaveGame::vf1 0x405000 with file +0xC / folder +0x1C,
  autosave, quicksave, the network save message). The folder is "data\Saved Games\", relative to the game folder.
- The game saves its AI managers itself (EECP*Manager persist functions: build, civilization, research, resource,
  unit, wall). dll/persist.c keeps the mod's own state in <save>.aimod next to it (only used while the .ees still
  has the size and write time recorded in it).
- The exe flags all its sections as data (0xC0000040, copy protection): find code by the section name `.text`.

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
- **Attack groups:** combat manager (player +0xB8C) +0x48 = EECPAttackCommander (vtable 0x827530): a deque of
  EECPAttackGroup* at +4 (+8 block map, +0xC map size, +0x10 start, +0x14 count; 0x400 pointers per block), +0x18
  player, +0x1C unit pool. Update 0x4516C6 (round robin, from the combat manager update 0x4720D8).
  - EECPAttackGroup (vtable 0x8275A8, 0x6C bytes): +8/+0xC vector of subgroups, goal struct at +0x28 (target unit
    +0x1C, player +0x20), **+0x48 target player** (chosen once at the launch), +0x6A finished (deleted by 0x45113F).
  - Subgroup goal struct at sg+0xFC: +0x100 location (the target struct above), +0x118 target unit (refcounted,
    0x442246), +0x11C target player, +0x124 mission (7/12 attack the goal unit, 8 go home); +0x18C group, +0x1D0 state
    (0x1776 attack the goal, 0x1777 going home, 3000 combat, 0xBBC retreat).
  - Nothing re-checks the stance of the target player: units only fight enemies (0x461854, vf112), but the goal
    stays until the target unit is dead - after peace the army keeps marching on the new ally. New launches
    (0x46F4FF) only pick enemies.
  - **Calling attacks off:** EEComputerPlayer::vf22 (0x43033E) thiscall(player, target EEPlayer*, 1) -> 0x46FD01
    (combat manager) -> 0x4510F2 (commander): every group aimed at that player orders its units home, releases them
    to the pool and is deleted (0x454B90). Flag 0 (the only use in the game: the scenario stance trigger 0x5AD29B
    after 0x52D5D7) only freezes the units. Deletes subgroups - never call it from a subgroup/commander callback.
    dll/diplo.c queues it for the safe point between simulation steps.
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
  The community DB editor (github.com/EE-modders/Empire-Earth---DB-Editor, `EE.yml`) has the same 56-byte layout
  and names the two unknowns: the `?` is "Object", the flag is "Use In Scen Editor"; min/max are the scenario
  editor's limits ("Scen Min/Max"), scaleF the "Scen Float Divisor". Difficulties are not rows but values inside
  records, mostly easy/medium/hard triples (e.g. 28002/28007/28029 initial attack delay, attack frequency, epoch
  frequency; 28103/28104/28106 min military buildings per difficulty) - which explains why the attack timing
  measured on medium (every 20-25 min) differs from the 14-17 min noted in ai-analysis.md. Help texts: string
  28500 + id.
- Object/unit type names: `data/db/converteroutput.log` (from the data files).
- Language.dll string table: `scripts/strtab.py`. The statistics names are 31201.., the AI settings names 28000..
  All 12 languages' sources: github.com/EE-modders/localized-text (`Game/<lang>/EE/Language.rc`).
- Globals used by community DLLs (EE-modders Reborn-DLL / Stats / Discord, same 1.04 build "2002.09.12.v2.00" at
  0x8A9030; AoC has its version string at 0x8BF570): game data loaded [0x917BB8] (unit limit at +0x2EC), map /
  game running [0x9183BC], in the lobby 0x944254, user name [0x91930C], resolution x/y/bpp 0x9193FC/F8/F4,
  camera pitch 0x9183C8, FOV 0x91328C. EE vs AoC: byte at 0x401000 is 0xE8 in EE, 0x55 in AoC
  (Empire-Earth-Library). Their hooks (version string 0x5D16F2, FPS 0x64E29E, game start entry 0x5360CA,
  resolution immediates 0x537523.., 0x538B38.., 0x65FAC9.., 0x6601E6..) don't overlap ours.

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
- AI population cap 0x42F7A5 (every minute; callers 0x4311E5, 0x43FA9B, 0x474AF0) -> player +0xCCC: (2.4 x the
  largest human population (0x556E11, capped at the limit) + 0.6 x the game limit (EEPlayer vf31 0x556EBC)) / 3
  (dbcpbehavior records 92/93 fval) x difficulty % (record 98: easy 70, medium 90, hard 140 - +0xDF0/+0xDF4/+0xDF8,
  x 0.01) - not clamped to the limit. The game limit instead with Ignorieren (+0xF02) or 0x431141 (setting 0x9187FC
  = 4, not a scenario). Mod: pop_follow_human=0 sets +0xCCC to the game limit after it.
- Citizens wanted = citizen share (player+0xBDC, 0.45 by default) x population cap (0x48EDDB); trained in
  batches of up to 10. Military slots = (1 - share) x cap (0x47049E); set by 0x43F42E (scenario trigger).
- Mine search 0x48C204 thiscall(rm, type, unit, reverse): first site in zone order (reverse = from the last zone)
  with a free slot (gatherers 0x52E5F7 < table 0x48E817(0x16) = 6), amount > 0, explored, no enemy near, the unit's
  continent - distance plays no part. Mines hold 300000.
- Assign to site 0x48C81C thiscall(rm, unit, site, 0): new gather target (EEUGGather), moves the unit between the
  resource lists (0x48B662 remove, 0x48A4EC add). AssignCitizens only hands out idle citizens (no goal, or a
  finished gather goal).
- Farms (type 0x2A, 8 around a granary): unit manager (cm+0x44) per-type list 0x48F8AF thiscall(um, type) -> deque
  (+0xC first, +0x10 count). [field+0x30]+0x340 = finished. AssignCitizens: food shortfall left after wild food
  -> finished fields without gatherer 0x48BFD0 -> unfinished ones 0x48BF4F, builders only while player +0xEFA = 0
  (order at 0x48B118: new(0x5C), EEUGBuildRepair ctor 0x60BAD0(goal, player, building, 1) - target at goal+0x3C,
  resolved via +0x30/+0x1CC -, 0x5F05BD(unit, goal, 0, 0), 0x5EFCF0(unit, 0x1F51), type vf 0x74(unit)); with
  +0xEFA set it trains/sends fishing boats instead (0x48FB05, 0x48C424). +0xEFA is set by 0x4307A7: water map
  (world +0x181730 bit 0) and >= 10 fish (type 0x13B, 0x4312E3). A field takes 50 wood when construction starts
  (not through vf45); without it the builder stops. 0x52E5F7 counts only gatherers at the field. New
  foundation: 0x62EF8A cdecl(player, type, x, y) (the build manager's building creation; pays the cost except
  for fields). The placement 0x46580A with type 0x2A only orders builders to an existing foundation - without
  one it constructs EEUGBuildRepair for a null building and crashes (0x60BB19). The building plan (cm+8: list,
  node +0xC type, +0x10 wanted; console `plan P`) holds only houses on random maps.
- Research: EETechTreeNode::vf6 0x5BF335 (vtable slot 0x830874) marks a tech researched (+4) and applies its effects
  (available +6, ids 1000..14999, data +0x48 not 3/5) - every research ends there; the cost is paid when it is
  queued (production queue 0x5E8908, also EEMResearchTechnology::vf10). 0x5BDDBD(cat) thiscall(node): vf6 on every
  child (node +0x24 vector, 0x20-byte nodes) of that category - EEComputerPlayer::vf41 (epoch reached, 0x43D330, call
  0x43D365) does it with 7 on the epoch's node: the line upgrades (effect kind 0x13 = upgrade unit type +0x14) free.
- Line upgrades (effect kind 0x13, 0x5BA59D): the player's finished objects of the old type (0x6363CD) are converted
  by 0x62F192 cdecl(unit, player index, 0x9418A0[new type]) - hit points in proportion - and the old type can no
  longer be built (0x555710). Nothing converts what is finished later: an AI build queue entry naming the old type
  (its queue holds {type, x, y}) is laid out as the old type. defence.c: upgraded_type (placement) and
  upgrade_stragglers (every 30 s). Tower line 47, 461-466; walls 467-472; gates 473, 454, 444, 443, 442, 294.
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

- Attack subgroup stats: 0x4504FD sums the member records (subgroup +0x50 vector, record +0x44 = unit) through
  0x450554 without a null check. One crash (giga_tst.ees, 289.8 min, `read 00000034` at 0x450559): in the AI step
  right after P4 and P5 surrendered and P2 called off its attacks on P5 (0x43033E with 1 -> 0x454B90: each subgroup
  hands its units back, 0x455C90, and is released). Stack 0x483ADE -> 0x4824BF -> 0x450E07 (unit into an attack
  group) -> 0x453E10 -> subgroup vf96 -> vf 0x14C 0x45AA2F -> 0x4504FD. Not reproduced (the replay's diplomacy
  differed); the call at 0x450546 now skips a record without its unit and logs it ("# engine guard: attack subgroup").

## Selection

- Double-click (EEUICSelectUnits::vf15 0x643583: all units of the clicked type on screen) and the box (vf14) call
  0x40337D(rect, ..., type or 0x2FF, ...), which builds EEMSelectUnits (0x4BD072, 0x138 bytes, ids 0x4E/0x4F/0x50):
  units found by 0x50F589 (thiscall 0x9125D8), packed by 0x4BD15A (cdecl: list, player +0x45C, out +0x34) into
  256 bytes, length byte at +0x134; 0x4BD2C4 (cdecl: buf, len, vector) unpacks (constructor, deserialize 0x4BD436).
  vf10 0x4BD35E -> 0x55B3FB on the player adds them to the selected group (player +0x9BC, EEGroup: vf 0x18 count) -
  no limit there. The packer's budget was 63 units (0x3F). Format: byte 0 the ids' bits 24..31, then runs: header
  0x00|n, 0x40|n + bits 8..15, or 0x80|n + bits 8..23 (n <= 63), n bytes of bits 0..7; empty = 0xFF.
  `[general] big_selection` packs the same format up to the 255 bytes (about 240 units; 236 tested).

## Drawing

- EEROUnits::vf0 0x503B83 (each frame) collects what is on screen into the global block 0x8D2CB8 (static ctor
  0x506333, dtor 0x5000CA): draw lists +0 (0xC each), 512 records of 0x148 at +0x78 = 0x8D2D30 (ctor 0x506442) for
  units, buildings and missiles, 1536 records of 0x3C at +0x29078 = 0x8FBD30 for small objects (type +0x619: trees,
  decorations), counts 0x912538 / 0x91253C. Room checks: 0x5008B8 (`cmp [0x912538], 0x200` at 0x500996, `< 0x600`),
  0x5035AD gets the cap as an argument (push 0x200 at 0x504157). The render thread draws them in 0x50BD9D(records,
  count). Every reference to the unit records (disassembly scan of .text): 0x5007B1/0x5007CA (game-start init loop,
  start and end), 0x503C68, 0x503ECF, 0x503FCB (+0xFC), 0x5043B4, 0x50DC59. Measured (360 longbows fighting): up to 445
  objects; 600: the original list stays at 512 (full), `[display] draw_limit` 4096: up to 727.

## Sound

- gSoundManager 0x919848 (Low-Level Engine.dll SESoundManager, constructed 0x530000 area with (path, mixer, 50, 0)),
  mixer +0x70 (SEMixer: the Miles mixer plugin's SPIM* functions as fields, e.g. +0xB0 GetMaximumChannelCount,
  +0xD8 SetChannelSample, +0xD0 PlayChannel - cdecl), channels +0x88, channel slots +0x20 vector.
- "Max Number Of Sounds (percent)" (registry HKCU\Software\SSSI\Empire Earth): channels = round(percent x mixer
  maximum), at least 8 (SetMaxChannels, at start and in 0x5C0000 area). The Miles Sound System Mixer.dll maximum is
  DirectSound's dwFreeHwMixingAllBuffers when 1..64, else 16 - Windows since Vista reports 0, so 16: 0-53% = 8 channels,
  100% = 16. Miles itself allows 64 samples (DIG_MIXER_CHANNELS set to 64), 22 kHz 16-bit stereo, DirectSound.
- Play (0x1005B575, relative to the DLL's base 0x10000000): volume 0 without the queue flag returns at once (sounds out
  of earshot). Each sound has a fixed number of copies (0x1005ABDD); with all copies on a channel (0x1005AC64) the play
  is dropped silently unless one copy is under 1% from its end (0x1005BA08). Then categories (all 50 at 100% of the
  channels), then a free channel (0x1005AA1F) or a finished one (0x1005A992, never a looping one) or a lower-priority
  one (0x1005A7E9 - looping sounds go first, to the paused queue). The paused queue resumes without checking results.
- Measured (`sound_log`, aitest.ees 25 -> 33 min, same game): 1251 audible plays; 16 channels dropped 557 (all for
  busy copies), 8 channels 665 (421 copies + 244 no channel). Ten subs moving on Large Islands: 6 of 10 "move"
  loops dropped for busy copies at 8 and 16 channels alike. The mixer never refused a sample.
- The options slider goes to 200% (registry value 0..200): 200% asks SetMaxChannels for 32, the mixer creates 16
  (CreateChannels clamps to its maximum) but the manager keeps 32 slots. On slots 16..31 SetChannelSample returns 10
  (channel out of range), nothing plays, and the node stays there (IsChannelBusyPlaying is never 6 for it, so
  DumpNonPlayingChannels never frees it): a looping sound keeps its copy for good. SetMaxChannels in a running game
  pauses every looping sound, and the paused queue resumes from the highest channel down - straight onto the dead
  ones (seen: ambience id 56 on channel 31 after 8 -> 32). `[sound] clamp_channels` holds SetMaxChannels to
  GetMaximumChannelCount (IAT 0x823AD4).
- `[sound] channels` writes the mixer maximum (the global behind SPIMGetMaximumChannelCount `mov eax, [addr]; ret`)
  before the game's first SetMaxChannels and recomputes the count from 0x9193B8 (slider, 1.0 = 100%). 63 Miles
  handles allocate fine (DIG_MIXER_CHANNELS 64, one kept by the plugin). aitest.ees 25 -> 33 min at 100%: 16 channels
  lost 3 audible sounds for want of a channel, 32 none; ~955 more were dropped for busy copies either way.

## Misc

- Civ selection: the civ timer (EEMCivTimerExpired 0x4B4F9C) opens the civ dialog when the local player
  has +0xB46 == 0 and +0x467 == 0. The dialog freezes game time (not the pause flag).
- Command line: only `hostgame`, `joinbyip`, `update_saved_games` and `-loadgame "name.ees"` (no space after
  the name, no '-' or '/' in it; parser 0x522A36, loader 0x521F94).
- Intro movies: AddMovie calls at 0x538909/0x53892A/0x538945.
- Saving: EEASaveGame (vtable 0x825084, 0x30 bytes, ctor 0x404FA1(const UString &file, bool, UString folder
  by value)), queued like the pause action; the folder is relative to the game folder ("data\Saved Games\").
- Chat line: 0x661E91 thiscall(ui [0x91977C], const int colour[3] (player +0x438), UWideString *name (player
  +0x24), UWideString *text, display ms, 0; ret 0x14) posts message 0x418 to the UI thread - callable from any
  thread. Shows "name text" top left in the player's colour. UWideString through EE.exe's imports: ctor
  [0x8231A4], operator=(const char *) [0x823224], dtor [0x823AEC].
- The AI's own chat: EECPPlayerInteractionManager 0x484EB3 thiscall(pim, base string id, ms) shows string
  base + 0..9 (not the variant used last). Bases: 12000 wonder threatened, 12100/12200 help for an ally's wonder
  / none, 12300/12330/12360 land/air/naval help on a flare, 12400 no help, 12500 war declared on the AI
  (0x46FFB7). Nothing else in the AI talks; dll/chat.c adds the mod's own lines.
- AI diplomacy does not exist: EECPDiplomacyManager (ctor 0x47AB80, vtable 0x8279E0) has only its destructor -
  no update, no reaction. Stances are one-way (+0x9E4[i] = this player's stance towards i): a human declaring
  war on an AI (EEMSetDiplomaticStance::vf10 0x4BDDCC -> 0x52D5D7(from, to, stance) -> SetDiplomacy 0x55B8E1)
  changes only the human's side; the AI stays allied. 0x46FFB7 (revenge attack + 12500 taunt) runs only from
  EEMAttackPlayers (0x4B3851 -> 0x5346E4) and only if the AI already counts the player as enemy.
  0x52D5D7 is thiscall(game state 0x918300 - players at +0x74, count +0xCC, teams +0x9B4); 0x46FFB7 is
  thiscall(combat manager = player +0xB8C, declaring player): target = his most attractive object (0x470FF5, none ->
  nothing), attack on it (0x472427), taunt through the interaction manager (player +0xB9C). dll/diplo.c answers war.
- Locked teams: byte 0x918E9F (the diplomacy screen disables its stance boxes, its OK button sends no stance).
- The native AI lines are called at 0x470040 (12500), 0x4715F8, 0x47166E, 0x47280E, 0x472B9D (rel32 calls).
- Tribute: EEMPayTribute::vf10 0x4BA6BE (vtable 0x8289C0 slot 10): sender id +0xA (player by id 0x52E6EF on the
  game state, compares +0x464), receiver index +0x24, amounts +0x10 food, +0x14 gold, +0x18 iron, +0x1C stone,
  +0x20 wood; paid through player vtable +0x48(resource, amount).
- Player name: +0x24 UWideString (char data at +0x28); assigning it renames the player everywhere (chat, scores).
  Localized epoch names: strings 30145 + epoch (Vorzeit ... Nano-Zeitalter).
- Player colours: table at 0x8CFA74/0x8CFA78 (vector of pointers to {?, float r, g, b}); 0x526B85(index) cdecl ->
  rgb (fallback 0x8CFA60), 0x526B3A thiscall(player, rgb) sets +0x438 rgb and +0x444 a darker copy. Entries 0-16
  are player colours (0 blue, 1 red, 2 yellow, 3 magenta, 4 orange, 5 green, 6 cyan, 7 dark red, 8 dark grey,
  9 purple, 10 grey, 11 silver, 12 light yellow, 13 lime, 14 light blue, 15 pink, 16 grey). The game start
  (random map / test play) gives humans their slot's colour (+0x24) and every other player the next colour no
  human has, in slot order - a computer slot's colour is ignored (call 0x5262E4, player in edi). In the game the
  UI shows the player's rgb (diplomacy screen, chat), no index is kept.

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
  The scroll action runs on the game thread, the frames are presented on another thread; with a 1 ms
  timer the game thread makes exactly one scroll step per frame (camtrace).
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
