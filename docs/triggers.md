# Scenario triggers (EE 1.04)

How the game runs the scenario editor's triggers, read statically from the exe (2026-10-17); the runtime parts the
`trig` test command uses are checked in play (reloadtime.scn). "Guess" marks what is not proven.
Test command: `trig` (dll/scen.c, docs/testing.md).

## Runtime

- EEScenarioManager [0x918E24] (vtable 0x8306E8). Vectors (begin, end) of triggers +0xE8, conditions +0xF8,
  effects +0x108, objects +0x118, areas +0x128; the id of each is its index (getters 0x57ED54, 0x56D186, 0x573183,
  0x579460, 0x56DFB7, thiscall(mgr, id), NULL when out of range). +0x210 scenario time (ms).
- Update 0x5A1ED2 (from 0x546DBA): every 500 ms of game time ([0x8306DC]) each trigger is updated (0x5B93B0(now)),
  then the queued effects (+0x168, 12-byte entries {effect, time, cinematic, done}) whose time has come run.
  0x5A2659 runs the list at +0x138 (start effects, guess).
- EESMTrigger (vtable 0x8307E4, save vf5 0x5B99AE): +4 name, +0x18 description (UWideString: 8-bit text pointer at
  +4), +0x30 condition wrappers, +0x40 effect wrappers, +0x4C conditions true for (s), +0x50 since, +0x54 id,
  +0x58 on, +0x59 fired, +0x5A looping, +0x5B active in cinematics. A trigger that fires and does not loop
  switches itself off.
- Condition wrapper: +4 condition id, +8 how it joins the next (0 end, 1 and, 2 or), +0xC not. Evaluated in order
  (0x5B95AA); a failed "and" skips to the next "or" - "and" binds tighter.
- Effect wrapper: +4 kind (2 = "or" between groups: the trigger runs one group at random, 0x5531C2), +8 delay (s),
  +0xC effect id. Effects are queued with 0x59FE52(effect, time, cinematic).
- EESMEffect: 0x194 bytes, ctor 0x5AB2D3 (no arguments), new 0x683718. Runs with 0x5AD29B thiscall(effect, bool
  cinematic) on the game thread - also outside any trigger (`trig effect`).


## EE 1.04 scenario CONDITIONS - reversed (static)

All addresses are EE.exe VAs. "Guess" means not proven. mgr = EEScenarioManager [0x918E24].

### 1. Storage and ids

| What | Getter (thiscall mgr, id) | Vector (begin/end) | Notes |
|---|---|---|---|
| Conditions EESMCondition | 0x56D186 | mgr+0xF8 / +0xFC (vector object at +0xF4) | the id is the plain index; out of range -> NULL |
| Effects EESMEffect | 0x573183 | mgr+0x108 / +0x10C | same scheme |
| Objects EESMObject | 0x579460 | mgr+0x118 / +0x11C | same; id 0 = "none" is used as "no parent object" |
| Areas EESMArea | 0x56DFB7 | mgr+0x128 / +0x12C | same |
| Triggers | 0x57ED54 | mgr+0xE8 / +0xEC | same |

The ids are vector indices, so deleting an entry in the editor renumbers the rest (vf1..vf6 renumber the stored ids).
Unit ids: top byte = player index; 0x53B889 thiscall(game state 0x918300, id) -> unit or NULL (player
`[0x918300+0x74+4*(id>>24)]`, then 0x55FAFB on it).

### 2. EESMCondition (vtable 0x830750), Evaluate 0x5AA80B thiscall() -> bool

The field list comes from the copy function 0x5AA733 and the editor "apply" functions (0x5710C1 player
attribute, 0x571851 trigger, 0x56FBFF game):

| Off | Type | Meaning |
|---|---|---|
| +0x04,+0x18 | UWideString | name / text (guess: name, description) |
| +0x2C | UWideString | chat text (type 7) |
| +0x40 | int | condition type 0..7 (jump table 0x5AACA4) |
| +0x44 | int | compare operator 0 `<`, 1 `<=`, 2 `=`, 3 `>=`, 4 `>` (strings 29310..29314) |
| +0x48 | int | difficulty to match (type 6) |
| +0x4C | int | diplomacy stance (type 5): 0 Allied, 1 Enemy (29510/29511; player+0x9E4[i]) |
| +0x50 | int | player attribute, left side (type 2) |
| +0x54 | int | player attribute, right side (type 2, player-vs-player mode) |
| +0x58 | int | tech id (type 3) |
| +0x5C | int | constant to compare against (type 2) |
| +0x60 | int | EESMObject id (type 1) |
| +0x64 | int | player spec 1 (types 2,3,5) |
| +0x68 | int | player spec 2 (type 5 target, type 2 right side) |
| +0x6C / +0x70 | int | elapsed time window min / max, in seconds (type 6); the editor keeps min <= max |
| +0x74 | int | hour (type 6) |
| +0x78 | int | trigger id (type 4) |
| +0x7C | int | copied but never read by Evaluate (unknown) |
| +0x80 / +0x81 | bool | trigger attribute: test "On" / test "Fired" (two editor check boxes) |
| +0x82 / +0x83 / +0x84 | bool | game attribute: test difficulty / test elapsed time / test hour |
| +0x85 | bool | player attribute: compare against another player's attribute (instead of +0x5C) |

At the start Evaluate always fetches `obj = 0x579460(+0x60)` and `trig = 0x57ED54(+0x78)`, whatever the type.

#### Compare 0x5AA6E4 thiscall(a, op, b)
op 0 a<b, 1 a<=b, 2 a==b, 3 a>=b, 4 a>b, any other value returns false. **The compare is UNSIGNED** (setb/seta/sbb),
so a negative value counts as huge (for example "gold < 100" is false if gold is -5).

#### Player specs: 0x5A23F6 thiscall(mgr, spec, vector<int>* out, bool pickOne) -> int
The editor combo 0x586D28 maps the list position to spec as follows: bit items, then players.

| spec | Editor (string) | Vector filled with | Return |
|---|---|---|---|
| 0 | World (29050) | {0} | 0 |
| 1 | Any Human (29051) | players 1..n-1 with +0x9B8 != 0 (human) | one of them chosen at random (after random_shuffle 0x440311); -1 if there are none |
| 2 | Any Computer (29052) | players 1..n-1 with +0x9B4 != 0 (computer) | random one; -1 if none |
| 3 | All Human (29053) | humans | **-1** |
| 4 | All Computer (29054) | computers | **-1** |
| 5 | Any Player (29055) | players 0..n-1 (World included) | random 0x5531C2(0, n-1) |
| 6 | Any Exclude World (29056) | players 1..n-1 | random 0x5531C2(1, n-1) |
| >=7 | Player %d (29057) | {spec-6} | spec-6, or -1 with an empty vector if spec-6 >= player count |

With pickOne the vector is reduced to the returned player (effects use this). Conditions pass pickOne=0 and
**use the return value only as a mode flag**: `anyMode = (ret == -1)`.
- anyMode (ret -1): OR over the vector, stops at the first true.
- otherwise: AND over the vector (res = r0 && r1 && ...).

**So in conditions the names are swapped:** "Any Human/Computer/Player/Exclude World" are ANDed over every
member, and "All Human/All Computer" are ORed. A specific player or World is a 1-element AND (fine). If the
group is empty, "Any Human/Computer" returns -1 (OR over nothing) = false; "All ..." with an empty vector = false.
This is read from the code and has not been checked in a game.
Player flags: +0x9B8 != 0 human, +0x9B4 != 0 computer (+0x9B4 is a pointer; the difficulty is read through it).

#### Types

| # | Editor | Evaluation |
|---|---|---|
| 0 | (always true) | true |
| 1 | Object | `obj ? 0x5B5CAE(obj, 0) : false` (section 3). Side effect: it can lock a non-dynamic object |
| 2 | Player-Attribute (29300) | for p in spec(+0x64): `a = 0x5AACC4(p, +0x50)`. If +0x85: for q in spec(+0x68): `r = cmp(a, +0x44, 0x5AACC4(q, +0x54))`, combined with q's any/all mode; otherwise `r = cmp(a, +0x44, +0x5C)`. Then combined over p with p's mode |
| 3 | Player-Technology (29301) | tech id +0x58, player tech tree p+0x9D4. id < 14: epoch reached: `tree+0x4E4 (current epoch) >= id`. Otherwise `node = 0x4019A1(tree, id)`; if node and (node data+0x7C (an object tech = unit/building) or node+4 (researched)): clear node+4 for a moment, `r = node->vf1()` (0x5BEF72 "available now": +6 enabled, not vf2, not +0x1E, epoch ok, epoch-advance timing), put +4 back. So it means "researched and still enabled", or for unit/building techs "can be made now". Not researched -> false. No compare operator |
| 4 | Trigger-Attribute (29302) | `r = 1; if +0x81: r = trig+0x59 (fired); if +0x80: r = trig+0x58 (on)`. With both boxes ticked only "on" counts (it overwrites, no AND). "Off"/"Not Fired" (29651/29653) need the trigger's NOT flag. **No NULL check on trig** -> crash if the trigger id is invalid and a box is ticked |
| 5 | Player-Diplomacy (29303) | for p in spec(+0x64) (skips NULL players), for q in spec(+0x68): `r = p+0x9E4[q] == +0x4C`. Nested any/all as above (p's view of q, one direction only) |
| 6 | Game-Attribute (29304) | `res = false; ok = 1`. If +0x83: `t = mgr+0x210 (float ms game time) * 0.001`; `res = ok = (+0x6C <= t <= +0x70)`. If +0x84 && ok: `f = [0x918E74]` (time of day, guess 0..1); `hour = (30 - ftol(f * -24)) % 24` (unsigned); `res = ok = hour >= +0x74` (**only "at or after", no upper bound**). If +0x82 && ok: `res = [0x9193F0] (game difficulty 0..2) == +0x48`. With no box ticked: false |
| 7 | Chat Message Contains (29305) | copies +0x2C and mgr+0xC0, `_mbslwr` on both (UWideString keeps a narrow char* at +4), `res = chat.Find(text, 0) != chat+0xC` (UWideString::Find). So: **case-insensitive substring** |

**Chat buffer mgr+0xC0:** EEMInGameChat::vf10 (0x4B78C7, at 0x4B7A93) **assigns** (operator= 0x8231D0) every
incoming chat line to it, from any sender, with no player check. The trigger tick 0x5A1ED2 empties it
(0x5A217B) after every 500 ms evaluation. So only the last line received in the window counts, and two lines in
the same 500 ms lose the first one. An empty condition text probably always matches when any line arrived
(guess: Find("") = 0). It is saved with the game (Load at 0x5A4339).

#### Player attribute getter 0x5AACC4 thiscall(cond, playerIdx, attr) -> int (ret 8; `this` is only used for the recursive call)
Returns 0 for a NULL player. Editor list = 29670.. (only 0..10 are offered; 11..30 and >30 can only be set by
hand or by data).

| attr | Name | Source |
|---|---|---|
| 0 | Any Resource (29670) | **one random resource 2..6 (0x5531C2) each evaluation** - not "any of" |
| 1 | All Resources | sum food+wood+stone+gold+iron |
| 2 | Stone | +0xB00 |
| 3 | Gold | +0xB04 |
| 4 | Wood | +0xAFC |
| 5 | Food | +0xAF8 |
| 6 | Iron | +0xB08 |
| 7 | Current Pop | 0x556E11(player, 0, 0) (population) |
| 8 | Pop Cap | player vtable +0x7C (vf31, 0x556EBC: game limit [0x9183D4] + per-player +0xB1C) |
| 9 | Unit Count | +0x460 = number of objects in the player's object table (inc 0x55FB7A / dec 0x55F9F7; buildings included, guess) |
| 10 | Difficulty Level | `p+0x9B4 ? [p+0x9B4]+0xBA4 : 0` (computer players only; humans give 0 = Easy) |
| 11..20 | (hidden) | `p+0xA18+4*attr` = +0xA44..+0xA68, 10 ints. Guess: per-player scenario variables |
| 21/22 | food tribute sent / received | +0xAB8 / +0xACC |
| 23/24 | wood sent / received | +0xABC / +0xAD0 |
| 25/26 | stone sent / received | +0xAC0 / +0xAD4 |
| 27/28 | gold sent / received | +0xAC4 / +0xAD8 |
| 29/30 | iron sent / received | +0xAC8 / +0xADC |
| >30 | statistics series attr-30 | 0x55DCD3(p+0x4A8, attr-30): 31 buildings lost, 32 razed, 33 units killed, 34 units lost, 35 conversions, 36 calamities, 37..41 food..iron gathered, 42 epoch, 43 army, 44 citizens, 45 heroes, 46 wonders, 47/48 tribute sent/received, 49 research count, ... 54 tiles explored (series n = string 31200+n) |

Tribute fields: 0x4C00D8 thiscall(player, dir 0 sent/1 received, res 2..6, amount) adds to
`+0xAB0 + 4*(res + 5*dir)` and to statistics series 17/18.

The matching setter, 0x5AC7F5 thiscall(effect, player, attr, mode, value) (mode 2 = subtract, which negates the
value; mode 0 = set; otherwise add), follows the same table:
- Any Resource: one random resource. All Resources: every resource 2..6.
- Pop Cap: writes +0xB1C (relative to [0x9183D4]) and sends a message (0x59FE04).
- Unit Count (9): ignored.
- Difficulty: clamped to 0..2, written to the computer's +0xBA4.
- Tribute fields: also update stats 17/18.
- >30: set uses series attr-30 (0x55DC0D), but **add/subtract uses attr-29 (0x55DBCF) - off-by-one bug**:
  adding to "units killed" changes "units lost".

### 3. EESMObject (vtable 0x8307A4), Evaluate 0x5B5CAE thiscall(bool includeDead) -> bool

Fields (from Load vf1 0x5B7B12, in file order, and the evaluator):

| Off | Meaning |
|---|---|
| +0x04 | name (UWideString) |
| +0x18 | vector<int> unit ids = the object's current set (begin +0x1C, end +0x20). Effects act on it |
| +0x28 | owner player spec |
| +0x2C | class filter (see below), 0 = Any |
| +0x30 | specific unit type id (0 = none) |
| +0x34 / +0x38 | number of units min / max |
| +0x3C | area id (EESMArea) |
| +0x40 | relation kind: 0 LOS, 1 Range, 2 Near (29001..29003) |
| +0x44 | other EESMObject id for the relation |
| +0x48 | parent EESMObject id: filter that object's units instead of collecting from the players |
| +0x4C | unit attribute id (0..74 = strings 29100..29174; files older than version 0xCD shift ids >16 by +3) |
| +0x50 / +0x54 | attribute range min / max |
| +0x58 | loaded but not read by the evaluator (unknown) |
| +0x5C | state value (current action type) |
| +0x60 | player spec for "visible by" / "selected by" |
| +0x68 | in-area mode, +0x6C relation mode, +0x70 attribute mode, +0x74 visible-by mode, +0x78 state mode, +0x7C selected-by mode: 0 ignore, 1 must match, 2 must not match |
| +0x80 | locked (the set is final; filled in at runtime, saved) |
| +0x81 | dynamic (collect again every evaluation) |
| +0x82 | attribute as a percentage of the maximum (version >= 0x15) |

**includeDead** = the argument, or `+0x5C in 6001..6004` (guess: the dying/dead states).

#### Collection (only when the set is not locked and is (dynamic, or empty))
- If dynamic, clear the set first. If the set is non-empty, skip collecting.
- **Parent (+0x48 != 0):** evaluate the parent (`0x5B5CAE(parent, includeDead)`), copy its set (0x5B7B02), then
  keep only the units whose type passes the class test and whose owner (`unit+0x18 -> +0x45C`) is in spec(+0x28).
- **Otherwise:** for each p in spec(+0x28), walk the player's object list `u = p+0x450; u; u = u+0xC` and push
  `u+0x24` (the id) if `0x5B58A3(obj, typeId = u+0x2C -> +0x4B8)` passes. Note: it uses the spec's whole vector,
  so the any/all naming does not matter here.
- If the count is below min (+0x34), clear the set.

#### Class test 0x5B58A3 thiscall(obj, typeId) -> bool
- If +0x30 is set, the type must equal it. Otherwise the type must have `0x9418A0[type]+0x5F8` set (shown in
  triggers, guess).
- Family `+0x4B4` must not be 9, 11, 12 or 20, and must not be excluded by 0x917570 (0x4A7618(family, 0x79)).
- Class +0x2C:
  - 0: any.
  - 1..14: epoch, `type+0x508 + 1 == cls` (strings 32002..32015).
  - 15..71: family `type+0x4C4 == cls-15` (32016 NoFamily..32072).
  - 72..112: RPS class `type+0x4CC == cls-72` (32073..32113).
  - >112: unit group `cls-113` from the list at 0x917570 (0x40DD73 gets it, 0x52BFA4(type) tests membership;
    guess: the 29761.. classes such as Archers and Cavalry).

#### Filter, for each id in the set (unit = 0x53B889(id))
A unit is rejected if any of these holds. The checks run in this order and stop at the first failure:
1. The unit is NULL.
2. It is not includeDead, `unit+0x3C` (hp) <= 0, and type+0x619 is 0.
3. `unit+0x30` is set and `[unit+0x30]+0x1CC != 0` (guess: being removed or garrisoned).
4. **State (+0x78):** `unit+0x30 -> +0x1E8 (current action) -> +0x24` ==/!= +0x5C. NULL+0x30 -> reject.
5. **Attribute (+0x70):** `v = 0x5B6FA0(+0x4C, +0x82, unit, 0, 0)`. Mode 1 keeps `min <= v <= max`; mode 2 keeps
   values outside the range.
6. **Selected by (+0x7C)**, AND over spec(+0x60), whatever its mode:
   - Mode 1: `p+0x9BC (selection)->vf3(unit+0x30)` or `p+0x9D8 == unit`.
   - Mode 2: neither of these.
7. **Visible by (+0x74)**, AND over spec(+0x60): 0x631B64 cdecl(unit, player, 0). This is the fog grid: the
   player's vision count at the unit's tile is > 1, or the unit is seen by an ally; it uses [0x9183BC] and is
   skipped when player+0x466 is clear. Mode 1 needs visible, mode 2 needs not visible.
8. **In area (+0x68):** EESMArea 0x5A9E1D thiscall(area, unit) == (mode 1). If the area is missing, both modes
   reject.
9. **Relation (+0x6C):** `other = 0x579460(+0x44)`; 0x5B599A thiscall(other, unit, kind +0x40, 0) == (mode 1). If
   other is missing, both modes reject.

When the set was collected this evaluation, rejected ids are erased from it. Otherwise (locked or kept set) they
stay, and the loop returns true at the first unit that passes.

#### After a collecting pass
- Fewer than min -> clear.
- Otherwise random_shuffle (0x440311) and, if more than max (+0x38), erase down to max (a random max-sized
  subset).
- `result = size > 0`. If true and not dynamic, set **+0x80 = 1: the object is locked onto those units for good**
  (later evaluations only filter that fixed set; dead ones are not removed; min/max are not checked again).

So the "Number of units" test is `count >= min`; max only caps the selection.

#### Helpers (EESMArea vtable 0x830744, relation, attribute)
- **0x5A9E1D area contains** (unit +0x1C/+0x20 = tile x,y; -10000 = off the map):
  - Off Map area (0x5AA1AA: area +0x2C or +0x30 == -10000): true when the unit is off the map.
  - "Previous Location" area (0x5AA1BF: +0x2C or +0x30 < 0): true when the unit's
    `unit+0x30 -> +0xDC/+0xE0` (previous position floats) equals its current tile.
  - Otherwise 0x5A9EA0, by area type +0x5C:
    - 0: GERectangle at +0x34 Contains(point).
    - 1: same land/sea region: continent manager 0x917BC8, 0x412F4C(x,y) id of the unit's tile == id of the
      area's point +0x2C/+0x30.
    - 2: the unit is inside one of the player shapes at player+0x7CC (count 0x40DCCE) that contain the area's
      point (guess: a territory or region).
- **0x5B599A relation** (this = the other object, which it evaluates first; true if any of its units, other than
  the tested unit itself and not off the map, satisfies it). Distance² comes from 0x61680D, between the closest
  tiles of the two units:
  - 0 LOS: dist² <= LOS² (stats +0xA4).
  - 1 Range: `unit+0x30 -> 0x5F0CE8(other unit)` (in attack range).
  - 2 Near: dist² <= 2.0 (adjacent).
- **0x5B6FA0 unit attribute** (attr 0..0x4A, table 0x5B79D6; ret 0x14):
  - 0 HitPoints = unit+0x3C. With percent: `*100 / stats+0x144` (max hp); a max of 0 gives 100.
  - 1..6 resources carried: +0x40, filtered by kind +0x38.
  - 7.. flight time, cargo, garrison, ...: see 29107+.

### 4. Reusable for an AI mod
- **Units of class X in an area / of player P:** walk `player+0x450` (`next = +0xC`, id `+0x24`, type `+0x2C`,
  owner `+0x18`, tile `+0x1C/+0x20`, hp `+0x3C`). 0x53B889(0x918300, id) resolves an id.
- **Class predicate:** 0x5B58A3 needs only obj+0x2C/+0x30, so a fake 0x84-byte block with the class/type works.
  The class numbering (epoch / family / RPS / group) is handy for "count my anti-tank units".
- **Fog / visibility:** 0x631B64 cdecl(unit, player*, 0) "can player see unit".
- **Proximity:** 0x5B599A works but needs EESMObjects. The underlying calls are 0x61680D (closest tile pair) and
  0x5F0CE8 (in range).
- **Area:** a built EESMArea plus 0x5A9E1D. For plain rectangles GERectangle::Contains [0x823418] is simpler.
- **Player stats:** 0x5AACC4 (`this` is not needed except for attr 0) gives resources, pop, pop cap, object count,
  tribute and every statistics series. 0x55DCD3 directly reads the series (kills, losses, ...).
- **Player-group resolver:** 0x5A23F6 (human / computer lists via +0x9B8 / +0x9B4).
- **Hidden in data:** player attrs 11..30 and >30 work in conditions and effects although the editor offers
  only 0..10. A scenario (or the mod writing EESMCondition +0x50) can test kills/losses/tribute.
- **Gotchas to remember:** Any/All are swapped in conditions; "Any Resource" is random; the compare is unsigned;
  the chat buffer holds one line per 500 ms; the hour test is ">=" only; trigger attributes don't check for
  NULL; non-dynamic objects lock on their first success; stats add/subtract are off by one.

## EE 1.04 scenario effects, category 1 (Object): the 13 actions and the attribute tables

All read statically (capstone). "unit" = EEUnit returned by `0x53B889` (thiscall on 0x918300, arg unit id → unit*,
the global unit lookup). Its fields: +0x18 owner player*, +0x1C/+0x20 tile x/y (-10000 = off the map), +0x24 id,
+0x2C type data (EEUnitClass, = 0x9418A0[type]), **+0x30 = the unit itself** (EEUnit init 0x616B8A writes
`[esi+0x30]=esi` at 0x616BB9, so the "physics" fields in engine-notes are on the unit), +0x34 the player's stats
record of the type (= `[player+0x878][type id]`, i.e. 0x492969 on player+0x874), +0x3C hp, +0x38/+0x40 carried
resource type/amount. Stats record +0x60 = type data. EESMObject (`0x579460(id)` or mgr+0x114 list):
`0x5B5CAE(this,0)` refreshes its unit list, +0x1C/+0x20 vector of unit ids, +0x28 player spec.

### 1. Object action switch (effect +0x104, 0x5AEA2C, table 0x5AEB41)

All actions take the source EESMObject id at effect **+0x158**.

| +0x104 | name (29350+) | handler | fields | what it does / engine calls |
|---|---|---|---|---|
| 0 | Create | 0x5ACD49 | +0x158 | see below |
| 1 / 2 | Freeze / Unfreeze | 0x5AF0BB(bool) | +0x158 | dyn-cast to EEComplexUnit (0x683720, RTTI 0x8A1940→0x8A1958). Freeze: `0x5EAE25(unit,0)` (clears the goal queue), state `0x5EFCF0(unit, 0x1388)` (0x3F0 for family 3 = aircraft, 0x3A9B for EEUCGate family 0x19), type vf 0x74(unit) (apply), +0x334=0. Unfreeze: +0x334 = type+0x613. Both: `0x5F01CF(unit, frozen)` writes +0x33B (frozen; also updates the wall/gate lists for types 0x42/0x7E/0x7F), +0x335 = !frozen |
| 3 / 4 | Kill / Remove | 0x5AF26F(bool kill) | +0x158 | for live units: `unit->vf[0x60/4](kill, 0, 0)` (kill=1 dies, 0 vanishes); if unit+8==0 also `0x55FD76(player, unit, 0)` (drop from the player's lists) |
| 5 | Task-Object | 0x5B1F5B | +0x158 src, +0x168 target object, +0x160 task | see "Tasks" |
| 6 | Task-Area | 0x5B32CB | +0x158 src, +0x164 area (mgr+0x124), +0x160 task | see "Tasks" |
| 7 | Stance | 0x5B1826 | +0x158, +0x15C stance (29475+) | see "Stance" |
| 8 | Unit-Attribute | 0x5B1B4C | +0x148 attr 0..19, +0x14C value, +0x108 op, +0x190 percent | → 0x5B8823 |
| 12 | Class-Attribute | 0x5B1B4C (same) | +0x148 attr 20..41 | → 0x5B80FF |
| 9 | Owner | 0x5ACC11 | +0x158, +0xD8 player spec | live units, de-duplicated (0x5B411B); new owner = `0x5A23F6(mgr, spec, 0, 1)` (returns a player index); skipped if -1 **or 0 (Gaia)**; **`0x638670(unit, playerIdx, 0)` cdecl = change owner** |
| 10 | Class-Name | 0x5AC64E | +0x8C CString | writes the name into **stats record +0x4C** (`0x5B47A4` = unit+0x34, `0x5B4775` sets +0x4C): renames the whole class for that player, not one unit; sets UI dirty `[[0x9197A0]+0x12C]+0x5DF=1` |
| 11 | Face Object | 0x5AEE9A | +0x14C angle in **degrees** (× 0.0174533), +0x190 instant | only idle units (no goal or goal id 0x1388) with `0x54A4E7(unit)>0` and a difference ≥ 1°. Instant: rotate the transform (unit+0x130) and `0x5F043B(unit, &mat, 1)` + `0x5EE7D7`. Else new 0x38 **EEUGLocation** `0x614D47(pos, 1,1,1, angle, 0,0)` → `0x5F05BD(unit, goal, 0, 0)`, `0x5EFCF0(unit, 0x1F40)`, type vf 0x74 |

#### Create (0x5ACD49)
- Mode A ("specific units": EESMObject +0x80 set, or +0x81 clear and a non-empty unit list): count = list size, for
  each listed unit a copy of its **type** for its **owner**. Mode B (class object): count = random(+0x34..+0x38)
  (`0x5531C2(rng 0x919774, lo, hi)`), players from +0x28 (`0x5A23F6`), type `0x5B636A(obj)`.
- Position `0x5B65A5(obj, typeId, &xy, 1)` (free spot in the object's area; fails → skip).
- **Create a unit: `typeData->vf[0x30/4](int x, int y, int playerIdx, 0, 0)` → EEUnit*** (after vf 0x54). Types with
  +0x4E0 group id whose group kind is 6 go through 0x629DDB (formation/group spawn, guess).
- Post-spawn: unit+8 → `0x5EFF65(unit,1,0,1)` (construction complete); aircraft (type+0x5D4 &1): flight time
  +0x28C = stats+0x130, ammo +0x2E4 = type+0x5A0, turned to face the map centre (0x5F043B); power +0x2D0 =
  stats+0x148; `player->vf1(0,1)`, `0x53B8AC` (unit list), map `[0x9183BC]->vf8(unit)` (put on the map).
- If the object has an attribute filter (+0x70: 1 in range, 2 outside; +0x4C attr, +0x50/+0x54 lo/hi, +0x82 %)
  the new unit gets it: `0x5B8823(attr, %, unit, lo, hi, +0x70==2, op=2 set)`. Ids are recorded (0x5B90A3).

#### Tasks (+0x160): 0 Normal, 1 Patrol, 2 Attack-Move, 3 Load, 4 Unload, 5 Garrison (29375..), **7+ = cast power
`task-6`** (EEUGCastCalamity 0x60E2FF; needs a member able to cast, 0x5BEFCB)
Common: units skipped if dead, or their goal id is 0x1771..0x1774 (guess: inside/boarding), or neither frozen
(+0x33B) nor +0x335; **a frozen unit is unfrozen** (0x5F01CF(0), +0x335=1, +0x334=type+0x613). Per player a new
**EEActiveGroup** `new 0xB8; 0x5DB37C(player)`, added to player+0x844 (0x4C210E), members `group->vf[0x48/4](unit,1)`;
group goal `0x5DD436(group, goal, 0)`, group+0x6C = goal id, start `0x5DD463(group)`. Idle aircraft get full fuel/ammo.
- **Task-Object** (target = first.. units of +0x168, sources spread round-robin over targets):
  - a member can gather it (0x616255) → EEUGGather `0x60F6A4(target,0,1)` (new 0x44), id 0xFA4.
  - enemy target (diplomacy player+0x9E4[i]≠0), Normal only, leader `0x4039D7(group)` not family 0x16 →
    `0x5DE545(group,target,x,y)` + EEUGAttackUnit `0x60AD45(leader,target,1)` (new 0x40), id 0x7D1.
  - healable (0x630E07) → per unit EEUGHealUnit 0x612A8A(target,1), state 0x3E81.
  - own: Garrison (5) and type+0x550 (garrison capacity) > 0 → **EEUGLoadGarrison 0x614120(target,&list,1)**;
    Load (3) and transport (0x404B37) → **EEUGLoadTransport 0x61487F**; damaged building → `0x5E8A3B(unit,target)`
    (repair); drop site (0x404AEE, types 36/65) → EEUGGather drop-off id 0xFAB; type 223 → town patrol.
  - else move: Normal/Attack-Move/Load/Unload/Garrison → **EEUGLocation `0x614CE6(float xy*, 1, task==0, 0,
    0.0, 0, 0)`** (new 0x38), id 0x1F40 (aircraft individually); Patrol → **EEUGPatrol 0x615446** (new 0x2C; waypoints
    via 0x552616, 0x5545C9), id 0x1F4D.
- **Task-Area**: point = a random tile of the area (0x5AA1E1 tile list). Off-map area (x or y = -10000, 0x5AA1AA):
  units are **taken off the map** (0x4AC71B / 0x5EF8A8). Units currently **off-map are teleported into the area**
  (free spot 0x5B65A5, transform 0x5F043B or carrier 0x5EF068, state 0x1388). On-map units: 0/2 → EEUGLocation,
  1 → EEUGPatrol, 4 Unload → **EEUGUnloadTransport `0x615A1F(unit, x, y, 1)`** for transports with cargo
  (0x42E347), 7+ → power at the point; **Load(3) and Garrison(5) do nothing for areas**.

#### Stance (0x5B1826, +0x15C)
0 Explore → group + **EEUGExplore 0x60EC79(1)** (new 0x4C), id 0x2AFA. 1 Idle → `0x5E9664(unit,0)` (stop),
state 0x1388/0x3F0. 2 Locked / 3 Unlocked → +0x33C = 1/0 (gates: also both halves via 0x6030FC/0x603123).
4 Town Patrol → EEUGPatrol 0x61549E(town 0x5ECB95+0x6C, point 0x5526E3, 1), id 0x1F4D. **5..10 Scout, Stand
Ground, Guard, Harass, Cautious, Aggressive → unit+0x208 = `[0x8D2044][stance-5]`** (combat reaction; the table is
filled at run time).

### 2. Attribute effects

Dispatcher 0x5B1B4C: attr < 20 → unit function, ≥ 20 → class function.
- With an object (+0x158) and +0x154 == -1, +0x150 == 0: unit attrs for every unit of the object (dead units only
  for types with +0x619); class attrs per distinct (type, player) - from the living units, or for a pure class
  object from players(+0x28) × type 0x5B636A.
- Otherwise (game-made effects, guess): unit id +0x154 / player idx +0x11C + type id +0x150, **percent forced off**.

**Unit** `0x5B8823(cdecl: attr, bool percent, EEUnit*, int value, int hi, bool outside, int op)` - value =
`rand(value,hi)` if hi≠0 (the outside flag picks outside the range). op: **0 increase, 1 decrease, 2 (and other)
exactly** (editor 29590..29592). Percent: value = base × v / 100 (base below, the class/type value, never the
current value). Results clamp at 0. Editor list = strings 29100..29119 (dialog 0x57D97F; 0x57D81E saves combo
→ +0x148, op → +0x108, checkbox → +0x190, edit → +0x14C).

| id | name | writes (on the unit) | % base | notes |
|---|---|---|---|---|
| 0 | HitPoints | +0x3C hp via **`0x616F30(unit,hp)` (SetHP)** | stats+0x144 | alive units only; ≤0 → `unit->vf[0x60/4](0,0,0)` (removed, no death), family 1: +0x40=0, player vf 0xC4 + 0x55FD76 |
| 1 | Any Resource | +0x40 amount, +0x38 type | stats+0x14C | rolls type 2..6, then falls through and stores **4 (stone)** - looks like a bug |
| 2-6 | Stone/Gold/Wood/Food/Iron | +0x40 amount, +0x38 = 4/5/3/2/6 | stats+0x14C | works on citizens and resource sites; if the goal is 0xFA4 (gathering) only when the type matches |
| 7 | Flight Time | +0x28C (int ms) | stats+0x130 | non-% value is **seconds** (×1000) |
| 8 | Cargo Count | - | - | **not handled (no effect)** |
| 9 | Garrison Level | +0x284 via `0x5E88C6(unit, delta, +0x288)` | type+0x550 | capped at type+0x550; when full creates EEUGLoadGarrison 0x614120 + state 0x2EE8 |
| 10 | Ammunition | +0x2E4 | type+0x5A0 | |
| 11 | Animation Is Paused | `0x5EFD6D(unit, +0x260, +0x178, value≠0)` | - | |
| 12 | Reload Time | **+0x2A0** (int ms) | **stats+0x138** | see below |
| 13 | Graphic Scale | +0x174 (float) | type+0x42C | non-% value used as is; ≤0 → 0.001 |
| 14 | Power | +0x2D0 | stats+0x148 | |
| 15 | Rotting Time | +0x2A8 | type+0x548 | |
| 16 | Construction Complete | `0x5EFF65(unit, value≠0, 1, 1)` | - | |
| 17-19 | Unit Variable1-3 | `[unit+0xD0]` + 0/4/8 | - | no %; scenario-only storage |

**Class** `0x5B80FF(cdecl: attr, bool percent, player*, stats*, int value, int op)` - op **0 increase, anything
else decrease** (editor offers only 29590/29591; no "exactly"). Writes the player's stats record, so every unit of
the type (present and future) changes. Editor list = strings 29120..29141 (dialog 0x57DCBE, save 0x57DBB5 adds 20).
Results clamp at 0. Ends with `0x59FE04(mgr, record)`.

| id | name | stats field | % base (type) | notes |
|---|---|---|---|---|
| 20-25 | Shock/Arrow/Pierce/Gun/Laser/Missile Armor+ | +0xE0+4i (float) | +0x4EC+4i (int) | computes **old − base + v** (fsubr at 0x5B8240): adding drops the base armour - looks like a bug, check in game |
| 26 | ReloadTime+ | +0x138 (int ms) | +0x50C | |
| 27 | Attack+ | +0xD8 (float) | +0x510 | |
| 28 | WorkRate+ | +0x134 | +0x514 | |
| 29 | FlightTime+ | +0x130 (ms) | +0x534 | non-% in seconds |
| 30 | Leadership+ | +0xDC (float) | +0x538 | |
| 31 | MaxObjectsCarried+ | +0x13C | +0x53C | |
| 32 | Repair+ | +0x140 | +0x554 | |
| 33 | HitPoints+ | +0x144 (max hp) | +0x55C | min 1; current hp unchanged |
| 34 | CarryAmount+ | +0x14C | +0x570 | |
| 35 | ContainerHealingHP/Sec+ | +0x150 | +0x574 | |
| 36 | Range+ | +0x9C via **`0x62814D(stats, float)` SetRange** | +0x420 (float) | also sets the projectile speed +0xC0 |
| 37 | LineOfSight+ | +0xA4 (float) | +0x460 (float) | then refreshes LOS for the player's units of the type (player+0x450 list, unit vf 0x24, vf 0xC) |
| 38 | MaxVelocity+ | +0xAC via **`0x628051(stats, float)` SetMaxVelocity** | +0x430 | non-% v × 0.05; min 0.0001; scales accel +0xA8, turn +0xB0/+0xB4/+0xB8 and reciprocals +0xC4..+0xD4 |
| 39 | Power+ | +0x148 | +0x580 | |
| 40 | Animal AI Active+ | +0x160 bool | - | % → v>0; copies to every unit of the type, unit+0xF9 (list 0x556AE2) |
| 41 | CameraFocus+ | +0xF8 (float) | - | in hundredths: round(f×100) ± v, /100 |

**Reload explained.** Unit attr 12 writes unit+0x2A0. The fire code 0x61E176 does: `if (unit+0x2A0 == 0 &&
stats+0x138 != 0) unit+0x2A0 = stats+0x138` (and ×1.5 when stats+0x15C and unit+0x342 are set); 0x60ABB7 and
other states refill +0x2A0 from the class too. So **0 means "unset - use the class"**: "exactly 0%" writes 0
and the next shot refills it - no effect. "exactly 1000%" = 10 × stats+0x138 (artillery 12000 → 120000;
the 117790 seen was this value counting down) and holds until a refill. Small absolute values are overwritten the
same way. For a lasting change use the class attribute (stats+0x138), as the user found.

### 3. Not object effects
0x5B0581 (switch on effect +0xE8−1, 123 cases) is **AI effect subtype 5 "Set AI Variable"** (29695; subtypes 0..6 =
29690..29696), not unit attributes. Category 6 dispatch is in 0x5AD29B.

### Callable engine APIs (for an AI mod)
- Unit by id: `0x53B889` thiscall(0x918300, id). Create: `typeData->vf12(x, y, playerIdx, 0, 0)` then map
  `[0x9183BC]->vf8(unit)` + 0x53B8AC (copy the Create tail). Change owner: `0x638670(unit, idx, 0)` cdecl.
- Kill/remove: `unit->vf24(kill,0,0)`. SetHP `0x616F30` thiscall(unit, hp). Freeze `0x5F01CF(unit, bool)`.
- Orders: `0x5EAE25(unit,0)` clear, `0x5F05BD(unit, goal, 0, 0)` give, `0x5EFCF0(unit, stateId)`, `type->vf29(unit)`.
  Goal ctors (operator new 0x683718 + ctor): EEUGLocation 0x614CE6 (0x38), EEUGPatrol 0x615446/0x61549E (0x2C),
  EEUGAttackUnit 0x60AD45 (0x40), EEUGGather 0x60F6A4 (0x44), EEUGHealUnit 0x612A8A (0x40), EEUGLoadGarrison 0x614120
  (0x74), EEUGLoadTransport 0x61487F, EEUGUnloadTransport 0x615A1F (0x40), EEUGExplore 0x60EC79 (0x4C),
  EEUGCastCalamity 0x60E2FF (0x34). Groups: EEActiveGroup 0x5DB37C (0xB8).
- Stat changes: unit `0x5B8823`, class `0x5B80FF` (cdecl, signatures above). Note 0x5B80FF also calls 0x59FE04.
- Random: `0x5531C2` thiscall([0x919774], lo, hi). State ids seen: 0x1388 idle, 0x3F0 aircraft idle, 0x1F40 order,
  0x1F4D patrol, 0x7D1 attack, 0xFA4 gather, 0xFAB drop off, 0x3E81 heal, 0x2AFA explore.

Guesses are marked as guesses. Not run in game: the armour bug and the "Any Resource → stone" bug need checking in play.

## EE 1.04 trigger effects: categories 2..6 (Player, Trigger, Media, Game, AI)

Sources: static disassembly plus the Ghidra export `_aimod/decomp/*.c`. The editor field names come from the
`EESETSET*::vf2` (dialog -> effect) and `vf5` (layout) functions. "pl" = EEPlayer*, "cp" = EEComputerPlayer*,
"cm" = combat manager `[cp+0xB8C]`. Everything is thiscall unless marked otherwise. Guesses are marked (guess).

### Categories (effect +0x100) - confirmed
1 Object, 2 Player, 3 Trigger, 4 Media, 5 Game, 6 AI. These match strings 29321 + (cat - 1). Each category's
sub-switch matches the order of its string list. The player list is the player spec at +0xDC (also +0xD4 and
+0xE0), resolved by 0x5A23F6 into a vector of player indices.
Common id fields: +0x120 area id (0x56DFB7 on the scenario manager), +0x124 / +0x17C EESMObject id (0x579460;
its unit list +0x1C/+0x20 is filled by 0x5B5CAE), +0x178 area, +0x180 trigger index.

### 2 Player (switch +0xF8, table 0x5AEB11), strings 29452 + n
| n | name | fields / engine calls |
|---|---|---|
| 0/2 | Victory / Defeat | If pl+0x467 == 0: `pl->0x55BCEA(1 win / 2 lose)` sets pl+0xA30 (result) and shows the chat line "Player '%s' is victorious/defeated" (31815/31814). Then `0x918300->0x52E2B7(char won, idx)`: sets pl+0x465 (reveal map) unless game+0x519 is set and the player lost, sets pl+0x7A8 = now, calls 0x5A18D0, and refreshes the fog (0x5150F2) for the local player. Then, if pl+0x9B4 is set, `[pl+0x9B4]->0x43F3E7(0)`: **a beaten or winning AI is switched off**. |
| 1 | Script-Camera | Local player only: 0x5B007C(effect). Fields: +0x120/+0x178 areas, +0x124/+0x17C objects, +0x189/+0x18F "use area" flags, +0x18A/+0x18B, +0x114 zoom, +0x118 scroll speed (PlayerView dialog: Move/Face camera, Follow, Track, Snap/Fast/Slow). |
| 3 | Diplomacy | Targets: player spec +0xE0. Stance +0xCC (29510.., 0 allied, 1 enemy). For each target other than the source: `0x918300->0x52D5D7(src, dst, stance)` (-> SetDiplomacy 0x55B8E1). If stance != 1: `pl->vf22(+0x58)(targetPl, 0)`. For a computer player that is 0x43033E -> cm->0x46FD01 -> attack commander 0x4510F2: groups whose +0x48 target is that player stand down (subgroup vf9(1)). |
| 4 | Set-Attribute Level | `0x5AC7F5 cdecl(idx, attr +0x170, mode +0xC8, amount +0x174)`, mode 0 exact / 1 add / 2 subtract (29500..). attr: 0 random resource 2..6, 1 all resources, 2 stone +0xB00, 3 gold +0xB04, 4 wood +0xAFC, 5 food +0xAF8, 6 iron +0xB08, 7 current pop and 9 unit count (ignored), 8 pop cap +0xB1C (stored relative to the game limit [0x9183D4]), 10 difficulty -> cp+0xBA4 (clamped to 2; add also clamps at 0), 11..20 **player variables "Variable0..9" at pl+0xA44 + 4k**, 21..30 tribute sent/received per resource (+0xAB8.. sent, +0xACC.. received, stats series 0x11/0x12), 31+ statistics series (exact: n-30, add: n-29, **off by one**). The stock is written directly: no AddResource and no AI demand update. |
| 5 | Modify-Tech-Tree | Tech +0x10C. +0x191 = Add (1) / Remove (0). Remove: if techdata+0x7C is set and it has a unit type, 0x62F0E8(pl, type, 100000), then `[pl+0x9D4]->vf2(tech)`. Add: tech < 14 (epochs): advance with 0x5BC8F6 if the epoch at +0x4EC is lower. Otherwise the record (0x4019A1) is marked researched (+0x14 = 0, +0x18 = 14, 0x5BF3B3 / 0x5BF4C8, upgrade 0x558CB6(type, 0x7F) once), then `[pl+0x9D4]->vf4(tech)`. Afterwards 0x59FE04 (guess: applied-effect history). |
| 6 | Award-Civ-Points | `(pl+0x468)->0x521B3F(+0x16C)`: +0x34 and +0x38 += n, +0x3C = 0. Then the UI refresh (0x667A8A, dialog 0x1B7). |
| 7/8 | Fade-In / Fade-Out | Local player only: 0x4FFDA7(3000) / 0x4FFDC0(3000) ms. Fade-Out works only while the cinematic byte 0x918EA0 is set (set by 0x5A1CDA). |
| 9/10 | Visibility-On/Off | pl+0x465 = 1/0 (explore all); 0x5150F2 when it is the local player. |
| 11 | MiniMap-Flare | +0x18F use area: area +0x178 -> random tile (0x5AA061), else object +0x17C -> the first unit's tile. `(pl+0x928)->0x55EDA6(&pos, 3, pl, -1, 1)`. |

### 3 Trigger (+0x180 trigger index via 0x57ED54, state +0xFC, 29650..)
0 On: trig+0x50 = 0, +0x58 = 1 (restarts the "true for" timer). 1 Off: +0x58 = 0. 2 Fired: +0x59 = 1. 3 Not
Fired: +0x59 = 0.

### 4 Media (+0xF0, 29525..)
- 0 Play Sound: file +0xA4 under `Sounds\` (the language folder 0x913ABC for campaigns) -> 0x534D85. Plays for
  everyone.
- 1 Change Text: +0x144 0 Hints / 1 Instructions / 2 History (29550..). +0x18E "Clear" sets it to empty;
  otherwise the "Add Line" text +0xB4 is appended to the scenario manager's string (mgr+0x188/+0x1D8/+0x19C).
  The UI is refreshed (0x1B7 and 0x5D6904/0x5D6CFE).
- 2 Send Dialogue: players +0xD4, local player only. Message +0x50, sound +0xA4, duration +0x140 s, speaker
  object +0x158 (its type id +0x4B8 is the portrait, guess), +0x114 camera scroll speed - 1. Queued through
  0x59FDF2. Other players get 0x65C6A9.
- 3 Send Chat Message: players +0xD4, local player only. Duration +0x140 == 0 -> 0x65C6A9; otherwise
  0x661E91(&white, title, +0x50, ms, 1), a timed on-screen line.

### 5 Game (+0x13C, 29575..)
- 0 Daylight Hour +0x134 (0..23): 0x531292(((h + 18) % 24) / 24).
- 1 Day/Night Cycle Time: the editor stores a number at +0x138, but the code only uses it as a switch:
  byte [0x919774]+0xE0 = (+0x138 != 0). The value itself is ignored.
- 2 Calamity +0x128 (0 = "Any": random 1..22, 29600..). The calamity comes from 0x4A8FB7(id) and its kind from
  vf5 (0x4A905F). Kind 1 global: 0x4A8DF4. Kind 2 at a tile: area +0x12C (random tile) or object +0x130 (each
  unit) -> 0x4A8E46(id, -1|unit, x, y, 0). Kind 3 per unit: 0x4A8F05(id, -1, unit, 0). vf15 checks first whether
  the calamity may start at that spot. +0x192 = use area.
- 3 Graphic Effect +0x128 (0x404C83). +0x192: area -> 0x4FB842 at a random tile (0x4FBD2A for kinds
  0x2A/0x3D); object -> 0x4FAE07 on each unit, or 0x4FB4DF beams between consecutive units (kinds 0x14, 0x16,
  0x21, 0x26, 0x2D).
- 4 Start-Cinematic: 0x5A1CDA (sets 0x918EA0). Game speed float [0x9183C0] = idx +0x134 x 0.1 + 0.1.
  Then [0x918300] vtable +0x2C.
- 5 End-Cinematic: 0x5A18D0, or 0x5A437B(effect, 0) while something is queued (mgr+0x237 / 0x5B47A8).
- 6 Ambient-Weather: if [0x918E28] is set: 0x5244A9(+0x188 on/off).

### 6 AI (+0xE4 = string 29690 + n). Every handler loops over player spec +0xDC.
Some handlers skip a player that is not dynamic_cast EEComputerPlayer (0x8A1658); others skip only humans
(pl vf29 +0x74 = is human). Useful: **pl+0x9B4 = (EEComputerPlayer*)this**, set in the constructor 0x42F57A;
it is 0 for humans.

#### n=0 Modify AI Military Target - 0x5AC03F (dialog EESETSETModifyAIMilitaryTarget)
Fields: +0x18C Add (1) / Remove (0); +0x18A Attack (1) / Defend (0); +0x189 "Use Area"; area +0x120, object
+0x124. Both ids are read whatever +0x189 says.
- Location: area -> its +0x64 is matched against the global vector 0x917C78 (entry +0x78) to get entry +0x60.
  Otherwise the area centre (+0x2C/+0x30) -> `0x41C537(ecx = continent manager 0x917BC8, x, y)` -> EEAIArea
  +0x60. Result `aiAreaId` (0 = none).
- Unit: the first living EEComplexUnit (hp +0x3C > 0) of the object -> its id +0x24, else -1.
- Calls on cm (every one is thiscall, ecx = [cp+0xB8C]):
  - Add: `char 0x4706DE(aiAreaId, unitId, char attack)` checks the target. Attack needs an enemy unit/area owner
    (cp+0x9E4[i] != 0); Defend needs the AI's own or an ally's. Then:
    - Attack: `0x46F289(unitId, aiAreaId)` (ret 8) -> pushes an EECPAGTask onto the vector at **cm+0x20**.
    - Defend: `0x46F3A6(unitId, aiAreaId)` -> the same task, pushed onto **cm+0x10**.
    - Task fields: target unit (refcounted: 0x442246); +0x20 target player (the owner, or a random enemy from
      0x47D99A); +0x28 = 1 own unit or area / 7 enemy unit; +0x2C = 2. Attack stores unit+0x30 as the target,
      Defend the complex unit itself.
  - Remove: `char 0x4716F9(unitId, 1)` if a unit was found, else `char 0x4718BA(aiAreaId, 0)`. Both search
    **only cm+0x10** (records of 0x34 bytes: +0x10 area id, +0x1C target ->+0x24 id) and erase with 0x472E0B.
    The flag 1 also clears subgroup+0x12C in every attack group aimed at that unit (0x4705CB; 0x47056A for areas).
- Who reads the lists: cm+0x10 is the goal pool of the attack subgroups. Land and air subgroups use it through
  0x4769F7, sea subgroups through 0x45462A. With chance min(0.9, 0.5 + 0.05 x count) they take one of these
  targets (0x470160: a living target on their continent) instead of their own choice. **cm+0x20 is never read**,
  except by save (0x471CB4) and post-load fix-up (0x471980, cp vf8). In 1.04, then, "Attack" targets do
  nothing, and "Defend" targets turn into goals for the attack groups (high confidence on the reads; it is
  odd, so check at run time before relying on it).

#### n=1 Create AI Military Group - 0x5AB8BE (EESETSETCreateAIMilitaryGroup)
Fields: +0x189 Use Area (area +0x120) / Use Object (+0x124); +0x18A Attack (1) / Defend (0); +0x18B
"Favorites"; effect +8/+0xC = vector of EESMObject ids (the group's makeup, set by the dialog).
Per computer player (dynamic_cast):
1. Locals: `EECPCombatStrategy s` (0x44 bytes, ctor 0x473BC4, vtable 0x82795C, reset 0x473CA7, dtor 0x473C99);
   `EECPUnitRequirement r` (0x28, ctor 0x45182D, vtable 0x827538, reset 0x45185D); `EECPILocation loc` (0x18,
   ctor 0x47C4E8, vtable 0x8265F4); `EECPAGTask t` (0x34, ctor 0x441FB3, vtable 0x8265FC, dtor 0x442283);
   `vector<int> units` (0x4D4D10).
2. Target: an area gives `loc.+0xC = aiAreaId` and `loc.+0x10 = 1` (from the tile) or 8 (from the 0x917C78
   list); `loc.0x47C879()` fills x,y; `t.loc = loc` (0x47C8B7). An object gives `t.target = complex unit`
   (0x442246) and the location = the unit's tile, or its container's (+0x1D8).
3. `t+0x20` = the target owner if it is not the AI itself, else a random enemy (0x47D99A on cm+0x50), else
   player 0. `t+0x28` mission = 7 (Attack, enemy object), 2 (Attack, no object), otherwise 1. `t+0x2C` = 2,
   `t+0x30` = 1.
4. `s+0x3C` = 4 Attack / 5 Defend; `s+0x40` = Favorites. For each EESMObject:
   - Placed units (0x5B5CAE true and a unit list) are appended to `units` (0x420C38).
   - Otherwise one requirement: count = random in [obj+0x34, obj+0x38] (0x5531C2) -> r+0xC and r+0x10. The
     totals go to s+0x28/+0x2C (only while every object has a count). Kind obj+0x2C: 0 -> r+0x18 = type obj+0x30
     (the type needs typedata+0x5F8); 1..14 (editor "Epoch n") -> r+0x18 = cp+0xBA8 (**favourite land unit**);
     15..70 -> r+0x1C = kind - 14 (family); 71..112 -> r+0x20 = kind - 70 (RPS class); r+0x24 = 3. Then
     `0x4518C3(ecx = &s+4, &r)` (push_back).
   - The scenario object matcher 0x5B58A3 decodes the same kinds as 1..14 epoch (+0x508), 15..71 family
     (+0x4C4, kind - 15), 72..112 RPS (+0x4CC, kind - 72), 113+ unit sets. The AI's bounds and offsets differ by
     1 or 2 (a bug, or 1-based fields: guess). Unit sets are ignored.
5. Each listed unit is kept if `unit+0x30 != 0`, its owner is the AI, hp > 0 and `cm->0x4707D8(unit+0x30)`
   (free for military use). The unit's +0x341 is then cleared. Units that fail are dropped (0x5A5486).
6. `if (cm->0x47067A(&t, &s, &units))` (requirements or units present, and 0x4706DE accepts the target)
   `[cm+0x48]->0x45106C(&t, &s, &units)`.

**0x45106C** `EECPAttackGroup* __thiscall(EECPAttackCommander*, EECPAGTask*, EECPCombatStrategy*, vector<int>*)`
(ret 0xC). It does new(0x6C) and ctor 0x453C0D (serial id from cm+0x58). The task is copied to group+0x28, so
the target player lands at group+0x48. Launch 0x456077 forms the subgroups (0x455251) and requests the
missing units. A group without subgroups is deleted and NULL returned; otherwise it is appended to the
commander's deque (+4). **This is the entry point for an AI mod that wants a scripted attack or defence
group:** build t/s/r as above on the stack, with the same ctors and dtors.

#### n=2 Build Object - 0x5AB55D (EESETSETBuildObject: only object +0x124)
Not a building order: it creates a **group from one requirement, held at home**. Location: the player's first
town (vector pl+0x7CC/+0x7D0) -> loc.+0xC = town+0x60, kind 5. Otherwise an area from the continent manager
`0x425DB0(1)`, then `(0)` -> list 0x917C04 -> +0x60, kind 1. `t+0x20` = a random enemy, `t+0x28` = 1, `t+0x30`
= 1; `s+0x3C` = 1, `s+0x40` = 0. One requirement is decoded from the object exactly as in Create group, then
0x47067A and 0x45106C as above. So the AI trains N units of that type into a group that holds its town.
Buildings are not handled (a type without +0x5F8 is skipped).

#### n=3 Maintain Object - **no-op**
The case calls 0x5248D4 (a shared empty `ret`, named EECPASGUProphetCalamityAnalyzerBase::vf7 by Ghidra). Its
dialog (EESETSETMaintainObject) has only vf4.

#### n=4 Set AI Toggle - 0x5B16EF (EESETSETToggleAI: +0xEC which, +0x188 On/Off; 29625..)
Applies to every non-human player; there is no dynamic_cast. 0 Active -> `cp->0x43F3E7(char)` (ret 4). Turning it
off calls 0x48EE0D on the unit manager ([cp+0xB90]+0x44), which deletes every unit's AI goal. Turning it on
calls 0x48FAB5, which re-registers all the player's units (0x4905DF) and gives the citizens to the resource
manager (0x48B344). Then it writes cp+0xEF9. The others are plain byte writes: 1 Cheat +0xEFB, 2 Attack
+0xEF4, 3 Defend +0xF00, 4 Build/Repair +0xEFC, 5 Explore +0xEFD, 6 Ignore +0xF02, 7 Gather +0xEFF.

#### n=5 Set AI Variable - 0x5B0581 (EESETSETSetAIAttribute)
+0xE8 = **dbcpbehavior record index** (switch 1..123, table 0x5B133E; name = 28000 + the record's name id).
+0x114 int1, +0x118 int2 (text boxes), +0x110 float (slider / record "Scen Float Divisor"). Skips humans. Most
cases write the same cp field that the loader 0x431697 fills from that record, so the trigger replaces the
.dat value at run time. "now" stamps are `_ftol(float)` (0x53BA92), i.e. round(+0x110). "x c" = x 0.01.
| rec | name | written |
|---|---|---|
| 1 | Initial Attack Delay | +0xD2C min, +0xD28 max (int1, int2) |
| 4/5/44 | Troops to Assist Allies / Ships to Maintain / (DM) | +0xCC0/+0xCC4 (**both int1**, int2 lost) +0xCC8=round(f); +0xCB0/+0xCB4/+0xCB8; +0xCA4/+0xCA8/+0xCAC |
| 6 | Attack Frequency | +0xD00 min, +0xCFC max, then `cp->0x43DD2F(now)` reschedules the next attack (+0xED8) |
| 7, 11 | Attack Freq. Modifier, Attack Group Modifier | +0xCEC/+0xCF0; +0xD30=+0xD34=int1, +0xD38=round(f) |
| 8/9/10 | Land/Sea/Air explorers | +0xD90 / +0xE18 / +0xCD4 |
| 14, 15 | Drop-off distance, Research frequency (s) | +0xE10, +0xE04 |
| 20, 43, 45, 48 | Defensive unit cap, Military units to train, Specialized %, Max attack response range | floats +0xC38, +0xC44, +0xBC8, +0xC40 |
| 21, 19 | Attack Frequency Boundary (+0xCF8 min/+0xCF4 max), Boundaries (+0xD24/+0xD20) | |
| 25/26/27 | Attack enemy / to claim resource / for revenge (chances) | +0xCD8 / +0xCDC / +0xCE0 |
| 28, 29 | Epoch Frequency (+0xD5C min/+0xD58 max), Epoch Freq. Modifier (+0xD60/+0xD64) | |
| 30..42, 95, 108, 109 | Desired number of barracks ... towers, tank factories; **109 shows "RPS Selection Islands" but sets the anti-air buildings** | setters 0x5B4xxx(cp, int): int1 -> v1 field (e.g. +0xE5C), round(f) -> second field (e.g. +0xDA4). **Both write the same build-plan entry** `[cp+0xB90]+0x1C` map{type, 0}+0x10 (type 0x22 barracks, 0x2F towers, 0x91 AA), so the plan ends up with round(float). Logged by 0x59FE04. |
| 46, 47 | Unit / attributes selected for improvements | +0xD84, +0xD80 |
| 49/50/51/52 | Priest / Prophet / Hero / Carrier usage | int1 -> +0xDFC/+0xE00/+0xD7C/+0xCD0, int2 x c -> +0xC50/+0xC58/+0xC30/+0xBBC, f -> +0xC4C/+0xC54/+0xC34/+0xBC0 |
| 53, 54 | RPS non-islands +0xBCC; **54 writes +0xBD0** (the loader puts it in +0xBD4; +0xBD0 is rec 110 Naval Unit Weight) | |
| 67, 70..85, 90, 91 | Unit cost reduction medium +0xBB4 (x c); guard %s chokepoint +0xBF8, continent +0xBF0, high ground +0xBFC, chokepoint-air +0xBF4, town +0xC24, waterway +0xC2C (and unnamed +0xC00..+0xC28); town guard % +0xBD8; CvC aggression +0xBB8 | floats |
| 87, 89, 88, 113 | Defense group size % (+0xD54/+0xD50), replenish delay (+0xD8C/+0xD88), defensive unit update +0xEB0/+0xEB4/+0xEB8, town guard update +0xE94 | |
| 96 | Unit Cost Reduction Easy & Hard | **writes +0xD60/+0xD64 (Epoch Freq. Modifier)**, not the cheat % fields +0xD48/+0xD4C (bug) |
| 97, 98, 102 | Expansion timer +0xD70/+0xD74/+0xD78; Pop limit adjustment +0xDF0/+0xDF4/+0xDF8 (easy/med/hard); +0xD68/+0xD6C | |
| 99, 100 | Capitol upgrade timer +0xE1C, TC upgrade timer +0xE90 | |
| 103/104/106 | Min military buildings easy/hard/med | only v1: +0xDD8 / +0xDE8 / +0xDE0 |
| 110, 111, 112 | Naval weight +0xBD0, Air weight +0xC68, Flame throwers +0xC6C | floats |
| 114/115/116 | Bonzai attack level +0xE98.., Island expansion timer +0xEA4.., DM initial attack mod +0xEBC.. | int1, int2, round(f) |
| 118..120 | unnamed (EE strings missing) | +0xCBC, +0xC3C, +0xD3C |
| 121/122/123 | per difficulty (cp+0xBA4 picks the slot) | 0x43F4B4 -> +0xEC8/+0xECC/+0xED0; 0x43F442 -> +0xC74/C78/C7C; 0x43F47B -> +0xC80/C84/C88 |
| 2 | Attack Delay Modifier (hidden) | **writes the difficulty +0xBA4 = int1** (the loader puts it in +0xD08/+0xD0C) |
| 12 | hidden | +0xC64 = int1 x c, then overwritten by int2 x c (+0xC60 never set) |
| 3, 17, 24, 92..94, 101, 107 | Citizen %, Attack Wonder, Extra resources, ... | no case: nothing happens |
Other hidden cases: 13 (+0xCE8/+0xCE4), 16 (+0xD44/+0xD40), 18 (+0xD04), 22 (+0xBC4), 23 (+0xE08/+0xE0C),
55..66 (+0xE20..+0xE4C), 68 (+0xC5C), 69 (+0xC48), 86 (+0xD94), 105 (+0xBE0 = int1 x c).

#### n=6 Set Favorite Units - 0x5B152A (dialog fields +0x158 / +0x114 / +0x118, labels Land/Air/Sea 31861..)
Applies to non-human players. Each slot has its own check (typedata 0x9418A0[t]):
- +0x158 -> cp+0xBB0 if flags +0x5D4 bit0, +0x620 == 0, vf21 false, typedata+0x510 > 0 and speed +0x430 > 0.
- +0x114 -> cp+0xBA8 (the "land" favourite that Create group / Build Object use for the epoch kinds) if bit2 is
  clear (with bit0 it also needs vf23 true), +0x620 == 0, vf21 false, speed > 0 (no attack check), and family
  +0x4B4 in {5..8, 13, 15, 16, 23, 26+}.
- +0x118 -> cp+0xBAC if bit2 (air) is set, +0x620 == 0, speed > 0, attack > 0.
- The bit meanings are a guess: bit0 = naval, bit2 = air. A type that fails is ignored without any message.

### Entry points for an AI mod (all `__thiscall`)
- `cp->0x43F3E7(char active)`: turn the AI on or off properly (ret 4). Other toggles: write cp+0xEF4.. bytes.
- `cm->0x46F3A6(int unitId|-1, int aiAreaId)`: add a goal for the attack groups (cm+0x10). `cm->0x4716F9(unitId, 1)`
  and `cm->0x4718BA(aiAreaId, 0)` remove one. `cm->0x4706DE(area, unit, char attack)` checks the stance.
- `[cm+0x48]->0x45106C(task*, strategy*, vector<int>* unitIds)` -> EECPAttackGroup* (see n=1 for the layout).
- Build plan: the 0x5B4xxx setters (cp, int count), or [cp+0xB90]+0x1C map{type, 0}+0x10 directly.
- Attack timer: write +0xD00/+0xCFC, then `cp->0x43DD2F(now)` with now = [0x919774]+0xD0.
- Favourite units: cp+0xBA8 land / +0xBAC air / +0xBB0 sea (no checks when written directly).
- Resources from a trigger bypass AddResource (cp vf18 0x42F783), so the AI's demand bookkeeping is not updated.
