# The experiments that are off: why they failed, what was missing

A review of every AI-behaviour switch that is off in `config/aimod.ini` (2026-10-21, after `fall_back=7`, 7b39bcc).
Sources: the commits that added and measured each one, docs/economy.md, docs/navy.md, docs/testing.md,
docs/ai-analysis.md and the code in dll/ai/. Nothing was re-run for this review: every number is a number already in
the docs, and the "why" is the mechanism the logs showed. Where they don't show it, it says so.

Numbers: d = with - without, paired by seed and seat, standard error in brackets; kl = kills - losses per game; "cheat" =
the base AI at hard with its 50% prices. Map names as in `scripts/isl_ablate.sh`: `cont` Continental e0 Small, `coL`
Continental Large (Middle Ages, unit limit 1500), `si5` Small Islands Small, `siL` Small Islands Large, `tiH`
Tournament Islands Huge, `liH` Large Islands Huge.

## What changed under the old results

Every experiment below was measured **before** `fall_back` was on. Several were also measured under conditions that
have since changed. Before reading an old "off" as "doesn't work", check which of these applied:

| What changed | Commit | Which experiments it touches |
|---|---|---|
| **fall_back 7**: before it, 24-40% of the value the AI lost died in fights that were already lost, and on the island maps warships sank faster than they were replaced (two thirds of the value lost was warships). With it, value lost in lost fights fell by 15-65% on every map, warships +3 to +9.5 on the large island maps, kl +12 to +56 | 7b39bcc | all of them - most of all those that added soldiers or ships (army_plan, workforce_plan 4, metal_plan 8, navy_mix, early_army) |
| Every test map was Small (`map_size` wrote the wrong field); "Large Islands e5" and the head-to-heads with `map_size=1` were Small maps | e03574a | food_plan, mine_fill, citizen_plan, army_plan, army_mass, expand_islands 1, navy_mix, early_army |
| Tests read the user's own aimod.ini (with `hold_town=1`, `attack_retreat=1`) instead of the shipped one | 9941f9b | fight_estimate, militia_estimate, landing_bombard (first version), hold_town / attack_retreat baselines |
| No base-AI benchmark yet: AI-vs-AI at medium, scenarios with 4-10 seeds | f0a079a | hold_town, attack_retreat, assault, fight_estimate, militia_estimate, navy_mix, landing_bombard |
| On the large island maps gold and iron limit the army (the land maps have 900-1000 lying in stock) | 9b25420, 30b67e5 | workforce_plan, army_plan, metal_plan, expand_islands |
| The base AI settles the free islands first (minute 30-40, 22-29 with the cheat) | d474fc1 | expand_islands |
| With 8 games at a time two runs of the same DLL can part after minute ~55; 20-game screens looked better than 40 (expand 185 tiH cheat +45.6 at 20 games, +13.4 at 40) | d474fc1, 11c1793 | every late-game kl number |

## Overview

| Key (bits) | Meant to | Best / worst measured | Why it failed | Verdict |
|---|---|---|---|---|
| `metal_plan` 8 FLEET (+16 CALM) | move the gold + iron the never-full fleet target swallowed (two thirds of it) to the land army | liH no cheat kl +23 to +37, 60-100 more soldiers trained; cheat siL -26.6 (13.6), tiH -48.6 (14.2); h2h ships lost +8.5 (2.7) | the bigger army fought lost fights and died; fewer warships while ships sank fast | **RETEST** with fall_back (1st) |
| `army_plan` 1+2+4 | turn unspent stock into soldiers, more production buildings | cont soldiers +4.5 / +5.3 at 60; h2h losses +14.4 (5.8) | the extra soldiers came as a trickle and were fed in one by one; only measured on Small maps | **RETEST** on coL with fall_back (2nd) |
| `workforce_plan` 4 ARMY | army_plan's FILL + BUILD on every map at cap 250+ | coL/liH/tiH no cheat soldiers +10 to +19, kl within the noise | as army_plan; on the island maps the metal is the limit | **RETEST** with army_plan on coL (2nd) |
| `army_mass` 1 GATHER, 65, 144 | reinforcements gather and join in groups; the pack stays together | GATHER no cheat kl +11.0 (3.4); cheat kills -17.0 (8.1) | hit with less at once; against twice the army the pack that waits trades less | **RETEST** with fall_back on cont + coL (3rd) |
| `navy_mix` | warships by role (frigates, battleships, galleys/subs) and against the enemy fleet seen | h2h kl -8.6 (2.3) / -7.5 (2.9) | the fleets were 1-1.5 ships: no mix could form; battleships sent by the game's sea groups against coasts | **RETEST** with navy_fleet=2 + fall_back (4th) |
| `early_army`, `early_soldiers=0` | ranged, food-free soldiers from epoch 1 (B) | A+B cheat kl +39 (17.7); no cheat wins 3 of 20 vs 8 | measured before food_plan=3 and fall_back; not clearly better than A | **RETEST** on cont (5th) |
| `militia_estimate` | citizens fight by the fight estimate in every age, as a fallback | 10 seeds, within the noise | too few games, too weak a raid, the user's ini | **RETEST** (low) |
| `landing_bombard` | shell a strong beach before landing | ashore 2.30 vs 2.15, ships lost 40.7 vs 37.1, noise | tiny fleet economy, ships died at the beach | **RETEST** (low) |
| `copper_plan=2` | only the next epoch saved for, citizens keep training | no cheat, 60%: kl +22.6 (8.8); cheat 40%: +5.3 (16.8), fewer workers at 60 | slower Stone Age delays the army; measured before food_plan=3 | **RETEST** (low) |
| `expand_islands` 185 / 249 | outposts on other islands for metal | og30 +2 to +4, foe on home island -2 to -5; by 60 min gone; kl both ways | the base AI settles the same islands with 2-3x the army and takes them; `sea_held` passes when the enemy fleet was simply never seen | **FIX FIRST** |
| `fight_estimate` | defence decisions by the real fight estimate | overrun alive 4 vs 3 of 10, kills 49 vs 31; muster fires 15/17 vs 7/6 | the estimate is an open-field fight; at home the defender has towers, buildings, reinforcements | **FIX FIRST** |
| `workforce_plan` 1 CAP, 2 WOOD | citizens only as many as the economy uses; surplus woodcutters to the mines | CAP -30 to -43 citizens, soldiers unchanged; 3 siL cheat kl -11 (4) | room and food were not what limited the army; the mines' slots were full - nowhere to put the woodcutters | **FIX FIRST** (needs outposts' slots) |
| `food_plan` 16 WILD | no fields while wild food (fish) is free | islands workers -7 to -10 at 30 min | the AI has no fishing boat before minute ~25 | **FIX FIRST** (early boats) |
| `assault` (+ `assault_outmatched`) | the mod leads the land attack as one army | worse in every test (pathtest kills 20-33 vs 50) | straight lines into the defences (no route planner), left the town to the enemy camp | **REMOVE** |
| `hold_town` | home soldiers pull back and guard instead of being drawn out one by one | camp2: kept 4 vs 17 citizens, Town Center 2 vs 5 times | the soldiers pulled back *behind* the town and left citizens and buildings to the pushes | **REMOVE** (fall_back 2 does it right) |
| `attack_retreat` = `army_mass` 4 | call a losing attack home | cheat kl -19.8 (8.3), kills -28.7 (9.4) | called back attacks that were trading well; whole group, not the units that lose | **REMOVE** (fall_back 2) |
| `army_mass` 2 WAIT, 8, 16/32 on units | wait for 1.3x before attacking; tether | WAIT hardly acted; 16 cheat kills -13.9 (5.6) | never triggered; holding back costs kills against the cheat | **REMOVE** (2, 8); 16/48 only with #3 |
| `metal_plan` 1 VALUE, 2 NOWAIT, 4 SPARE | the soldier with the most per gold + iron | cont wins 11 -> 4 (in 27); coL screen kills -73 (20) | the game's own choice was better; cheap-per-metal units die | **REMOVE** |
| `workforce_plan` 8 THRIFT | refused gold units replaced at once by the cheapest per metal | cont soldiers +3.5 to +4; siL kl -37 (16) | same idea as VALUE, same failure | **REMOVE** |
| `mine_fill` | every mine slot near a drop site worked | 7: mines +5-8, kl +8 to +10; workers -4 to -12, si5 base AI wins 3 -> 7 | took the woodcutters; since then citizen_plan 1 and navy_mines do it with new citizens | **REMOVE** (superseded) |
| `safe_sites` | citizens only near the base, safe routes | never measured in the docs ("slows mining on open maps") | switching it on bypasses mine_choice, food NEAR and the tower/threat refusals; a 20-tile limit | **REMOVE** (superseded) |
| `early_soldiers` | 4 soldiers in epoch 0 instead of towers | the worst variant: base AI wins 9 of 20, 30 citizens die | the soldiers' food starves the economy | **REMOVE** |
| `idle_citizen_slack` | no new citizens while many stand idle | stalled growth | the wrong lever; citizen_plan / CAP handle the goal | **REMOVE** |
| `food_plan` 8 HUNT | hunt every animal near a drop site | kl -6.5 / -12.6 | hunting is the longest walk; twice the hunters gathered 5% more | **REMOVE** |
| `overseas_mines` | the game's overseas mine ferry only to mines on another island (Art of Conquest's fix) | si5 e5 / Large Islands e9, 36 seeds: citizens +4.2 (2.5) / +2.3 (1.1), home miners +3.1 (1.7) / +1.7 (1.3), income within noise | (not failed: a small gain, within 2 standard errors) | **RETEST** with expand_islands |
| `citizen_attacks=0` | (not an experiment: 0 is the fix - attack groups of half citizens are called off) | | | keep 0 |

## Patterns across the failures

1. **More soldiers, more losses.** army_plan, workforce_plan 4, metal_plan 8 and early_army all did what they were
   built for - 5 to 100 more soldiers trained - and the fights did not get better. The extra soldiers went into the
   same fights the AI was already losing (a quarter to two fifths of its lost value died in fights that were lost
   when they began). That is exactly what `fall_back` fixed, and why these are the first to re-run.
2. **Whole-group decisions failed, the per-unit one worked.** hold_town, attack_retreat, army_mass 4 and assault all
   decided for a whole army (pull back, call off, wait). Against the cheating base AI with twice the army they cost
   kills (attack_retreat -28.7 (9.4)): an attack trading well was called home because the sum looked bad.
   `fall_back` 2 pulls back only the units whose own fight is lost, to the nearest own tower, and lets the attack go
   on - +12 to +56 kl. The same estimate (stats.c) that failed in `fight_estimate` for whole defence decisions works
   there, per unit.
3. **Holding back costs kills against the cheat.** GATHER, TETHER, WAIT: fewer stragglers and fewer losses, but against
   twice the army the chasers' kills count and the waiting pack trades less (GATHER cheat kills -17.0 (8.1)). The
   no-cheat and head-to-head results were positive. The docs' own conclusion was "the lever is which fights the pack
   takes and when it breaks one off" - fall_back 2 now does that, which may make holding together pay.
4. **The wrong limit.** workforce_plan CAP freed population room and food, but room and food did not limit the army -
   gold and iron did. army_plan raised the budget, but the budget was not the limit either (the stock was spent;
   orders failed for wood, gold, iron). mine_fill took workers from wood, which then went short. Before building
   the next lever, the logs (`# mil`, `# metal`, `# wf`) should show it is the binding one - they did for metal.
5. **Never acted, or hardly.** army_mass WAIT (under a minute of waiting a game), the one-front goal switch (0 times
   in 80 games), navy_mix (1-1.5 warships to mix), mine_fill 3, expand_islands on maps without a free island, with
   the cheat (0.3 trips a game), metal_plan in the Continental head-to-head (under metal_cap). Such a result says
   nothing about the idea - only that the test didn't exercise it.
6. **Measured on the wrong maps or config.** Until e03574a everything was Small; the playtester plays Large / Huge.
   Until 9941f9b the tests ran the user's ini (hold_town and attack_retreat on in the base). The scenario tests
   (hold_town 5-6 pairs, fight_estimate / militia_estimate 10 seeds) are too few to call; the docs themselves found
   4 seeds to be noise.
7. **Superseded.** safe_sites (mine_choice, avoid_towers_citizens, citizen_threats, food NEAR), mine_fill
   (citizen_plan 1, navy_mines), hold_town / attack_retreat / army_mass 4 (fall_back 2, muster), THRIFT (metal_plan
   VALUE / NOWAIT), assault (the game's attacks with attack routing).
8. **A missing companion.** Outposts need the army to hold them and the sea to reach them; the army on islands needs
   the metal the outposts would give; the warships that would hold the sea were sinking. These were measured one at
   a time, each without the others' precondition. With fall_back the ships now survive (warships +3 to +9.5), which
   breaks one link of the circle.

## Dependencies

```
fall_back 7 (on) ──> metal_plan 24 (FLEET + CALM): land army from the warships' metal, the fleet now survives
                      └──> expand_islands 185 (needs army 1.5x and the sea) + workforce_plan 1+2 (citizens for its slots)
                           └──> needs: sea_held from a fleet that has actually looked (FIX FIRST)
fall_back 7 ──> army_plan 7 / workforce_plan 4 on coL (metal lies in stock on land maps)
            └──> army_mass 1 / 65 / 144: holding together, now that lost fights are broken off per unit
navy_fleet 2 + fall_back 4 ──> navy_mix (a fleet big enough to mix) ──> landing_bombard
food_plan 16 WILD ──> needs early fishing boats
fight_estimate ──> needs a home advantage in the estimate
```

Not yet combined and worth knowing: army_mass GATHER and fall_back both hold units and refuse their subgroup's orders
(landing.c `hook_asg_give_goal`: `mass_refuses || cover_refuses`); both give walk orders. Together they have never run.
metal_plan VALUE and early_army both replace the game's unit choice.

## Each experiment

### metal_plan (dll/ai/metal.c; 30b67e5)

**Meant to:** spend gold and iron where they win the most. On the island maps the fleet's target (15% of the cap, 76
warships at 505) was never reached, so the navy kept 50% of the income all game and two thirds of the gold + iron went
into warships - yet the mod had no more warships afloat at 60 minutes than the base AI: they sank.

**Measured (40 games a cell):** 24 (FLEET + CALM; 250% / 20): liH / siL / tiH no cheat kl +23.0 (10.7) / +7.3 (15.1) /
+5.8 (14.0), soldiers at 75 +12 to +18, G+I income +950 to +1250; cheat -3.3 (9.6) / -26.6 (13.6) / -19.3 (16.9), though
the base AI won fewer (siL 29 -> 20, liH 12 -> 7). 150% / 12: tiH cheat -48.6 (14.2). Head-to-head siL: soldiers +10.4
(4.9), razed +6.4 (2.9), kills +27, losses +22, warships -7.1 (1.6), ships lost +8.5 (2.7). 27 (with VALUE): cont wins
11 -> 4.

**Why it failed:** the larger army fought - and lost - more: "its larger army fights - and loses - more" (economy.md).
That is the lost-fights waste fall_back now cuts. On the sea, FLEET capped the fleet while ships sank faster than they
were replaced, so the sea got weaker (h2h ships lost +8.5). fall_back 4 keeps ships alive (liH no cheat warships +9.5
(2.0), h2h +8.1 (2.1)), so the same cap now costs less. VALUE failed on its own: the game's choice was better on small
maps, and cheap-per-metal units die.

**Missing:** units that break off lost fights (now there), a fleet that survives (now there).

**Verdict:** 8 + 16 **RETEST first**. VALUE / NOWAIT / SPARE **REMOVE** (VALUE: cont 11 -> 4 wins, coL screen kills
-73 (20); NOWAIT within the noise alone; SPARE only with VALUE).

### army_plan (dll/ai/army.c; b3b58fe) and workforce_plan 4 ARMY (9b25420)

**Meant to:** turn everything the economy has into army: unspent stock tops up the account (FLOAT), soldiers into the
room the citizens leave (FILL), more Barracks / Ranges / Stables when they are busy (BUILD), food workers to the metal
the army lacks (MIX). workforce_plan 4 runs FILL + BUILD on every map from cap 250.

**Measured:** army_plan 7 on cont: soldiers at 60 +4.5 (1.6) / +5.3 (2.2), kl -0.8 (3.3) / +7.0 (4.6); head-to-head
soldiers +4.2 (2.1), losses +14.4 (5.8); lowpop kills +22.6 (11.1), losses -22.4 (10.2). On islands it never acts
(needs an enemy on its continent). workforce_plan 4: coL / liH / tiH no cheat soldiers at 75 +19 / +16 / +14, kl -1
(10) / +11 (13) / +14 (19), coL wins 9/2 -> 11/4; with the cheat nothing.

**Why it failed:** "the extra soldiers come in a trickle and the game's attacks and home guard feed them in piecemeal:
more soldiers, as many kills, more losses". army_plan was only ever measured on the Small Continental. On the land maps
the metal is *not* the limit (900-1000 gold + iron lie in stock on coL and cont), so a land map at high population is
exactly where turning stock into soldiers can work - and coL is where fall_back gained most (kl +56.0 (10.2), soldiers at
75 +19.4 (6.1)).

**Missing:** soldiers that survive the fights they're fed into (fall_back), and a test on a large land map.

**Verdict:** **RETEST** (2nd): army_plan 7 and workforce_plan 4 on coL + cont with fall_back. MIX (8) moved a few
workers rarely, UPGRADE (16) was noise: leave out.

### army_mass (dll/ai/mass.c; 007044a, 993ca48)

**Meant to:** the army as one - replacements gather at a rally and join in groups (1 GATHER, 64 JOIN), attacks wait
until 1.3x the enemy (2 WAIT), attacks retreat (4), members don't chase far from the pack (16 TETHER, 128 buildings
only, 32 REGROUP).

**Measured (cont only, 40 games):** GATHER no cheat kl +11.0 (3.4), losses -4.0, soldiers alone and away 20% -> 14%;
cheat kills -17.0 (8.1), kl -9.7 (8.0), base AI wins 3 -> 4, mod wins 20 -> 14 without the cheat; head-to-head kills +9.0
(3.6). TETHER 16: no cheat losses -7.4 (2.8), cheat kills -13.9 (5.6). 144: neutral everywhere (80 games). 65
GATHER+JOIN: no-cheat wins kept (20 -> 19) but cheat kl -11.8 (7.5). WAIT +-1, "the game launches with what it has, under
a minute of waiting a game". 4 RETREAT: cheat kl -19.8 (8.3).

**Why it failed:** GATHER and TETHER work as intended (fewer stragglers, fewer losses) but "the attacks hit with less at
once" - razed buildings fall everywhere, and against the cheating base AI with twice the army the pack that waits trades
less. The measurement itself found the deaths are in the pack's fights (61-67%), not on the way: so how the army walks
was never the big lever; which fights it takes and when it breaks off is. That is fall_back 2 now. WAIT never got to act.

**Missing:** per-unit break-off (now there); a test on coL (army_mass needs an enemy on the same continent; never run
on a large land map).

**Verdict:** 1, 65 and 144 **RETEST** (3rd) with fall_back on cont + coL; 16 / 48 only if 1 gains. 2 WAIT, 8 and 4
**REMOVE** (4 is attack_retreat, below). If WAIT is wanted later it needs a trigger that knows the enemy army - the
enemy's strength it compares with is only what intel has seen.

### navy_mix (dll/ai/navy.c `navy_pick`; 245355d)

**Meant to:** warships by role from the game's hit table - frigates, a battleship per two frigates, galleys / subs - and
against what the enemy fleet has.

**Measured (2026-10-19, before the big fleet, Small maps, medium-era tests):** head-to-head kl -8.6 (2.3) (160 games),
-7.5 (2.9) (next payable role); the sea exchange even (+0.5). Both AIs with the mix: noise. With navy_fleet 1 (`fm`):
kl -6.3 (3.8).

**Why it failed:** "the fleets are tiny: 1-1.5 warships on average, 3-5 at most, so a mix hardly forms". The game's sea
groups decided where ships went, and the battleships the mix bought went against coasts (Large e5 lost 2.4 vs 0.8).
Battleships' gold was missed elsewhere.

**Missing:** a fleet big enough to mix (navy_fleet 2 now trains up to 15% of the cap, 26 at 176), the mod leading it
(navy_fleet 2), ships that leave a lost fight (fall_back 4) - all there now.

**Verdict:** **RETEST** (4th). Watch the gold: a battleship-heavy mix competes with metal_plan's land army.

### early_army and early_soldiers (dll/ai/early.c; 63af991, 9aa8aac)

**Meant to:** the playtester's opening, part B: no army in epoch 0, then food-free ranged soldiers (Slinger, Bowman,
Javelin ...) paid from the army's own account. early_soldiers=N: N soldiers at home in epoch 0 instead of towers.

**Measured (cont, 20 games a cell):** A+B vs shipped, cheat kl +39.3 (17.7), dead -11.6 (4.4) (A alone +32.4 (10.1));
without the cheat wins 3 of 20 vs 8. Islands neutral. The alternative (4 soldiers + B + C): base AI wins 9 of 20, 30
citizens die, kl -10.7 (7.2).

**Why it failed:** not a failure so much as no clear gain over A - the cheat cell is +7 over A within a large se, the
no-cheat wins fell. B was measured before food_plan=3 (first epoch 8 minutes earlier, +7-9 workers) and before
fall_back (BREAK applies to cheap units too; HEAL needs a value of 100). early_soldiers: soldiers in epoch 0 eat the
food the citizens need - clearly worse.

**Verdict:** early_army **RETEST** (5th, cont both cheats with A + food_plan 3 + fall_back). early_soldiers **REMOVE**.

### copper_plan=2 (other off-by-default)

Only the next epoch's cost held; citizens keep training above min_workforce. Cheat: workers +5 to +13 at 20-30 min but
fewer at 60 and kl +5.3 (16.8) (40%) / +23.1 (13.1) (60%); no cheat with 60%: 6/0/14, kl +22.6 (8.8). The slower Stone
Age delays the army. food_plan PLAN has since fixed part of the same problem (food workers for the citizens trained).
**RETEST** (low): `copper_plan=2,epoch_share=60%` on cont both cheats, 40 games.

### militia_estimate (dll/ai/defence.c; 3f6261d)

**Meant to:** citizens fight by the fight estimate in every age - only as a fallback when no own soldiers near would win,
builders when they win, soldiers only when they win clearly. (WW1 citizens have 264 hp: 3 beat a machine gunner.)

**Measured:** 10 seeds on scenarios, the user's ini: assault alive 8 vs 10, workers 31.5 vs 29.2, losses 53 vs 61;
raid_ww1 "too weak to tell".

**Why it failed:** it didn't fail - it was never measured with enough games or a raid strong enough. It fits the
playtester's rule (militia only as a fallback, citizens stay safe) better than an age limit (a Copper limit was worse:
alive 7, losses 82).

**Verdict:** **RETEST** (low): against the base AI on cont and coL both cheats; count citizens killed and kl.

### landing_bombard (dll/ai/landing.c; 0485acb, 27672e8)

Shell a too-strong beach with a fleet first. 20 seeds, Small maps, given 3000 of each: ashore 2.30 / 2.15, razed 8.55 /
6.75, ships lost 40.7 / 37.1 - noise. Without given resources the fleet never came. Since then: navy_fleet 2, fall_back 4
(the shelling ships go to the docks below half). **RETEST** (low): siL tiH liH, after navy_mix.

### expand_islands (dll/ai/expand.c; 27617d2, d474fc1, 11c1793)

**Meant to:** outposts on free islands for metal - Settlement, towers, more citizens; on large maps early (8), additional
citizens (16), more islands (32), a forward base towards the enemy (64), only with the sea and 1.5x the army (128).

**Measured:** 1 on Small maps: one free island at most, nothing changed beyond the noise. 57 / 121 / 185 / 249 on the
large maps: outposts from minute 25-30, og30 +2 to +4, enemy soldiers on the home island -2 to -5 without the cheat; by
60 the other islands' miners are back where they were (siL og60 -7.2 (2.9) with 185); liH no cheat kl +35 to +41 (57 / 121),
siL cheat base AI wins 29 -> 21; with the cheat 0.5-0.75 trips a game. 185 + metal_plan 24: the most metal (+2000-3000
G+I by 60) and soldiers (+11 to +20), kl within the noise.

**Why it failed:** the outposts don't hold - the base AI settles the same islands and its 2-3x larger army takes them;
with the cheat the AI rarely gets to plan (army not 1.5x, sea not held). FORWARD placed the forward base where the base
AI was already settling, and `sea_held` passed "on the mod's own warships because the enemy's had rarely been seen" -
the test passes on ignorance. The mod sees the enemy's new islands 10-25 minutes late or never.

**Missing:** (1) a sea test that needs a recent look at the enemy fleet (or the fleet scouting), not the absence of
sightings; (2) an army on the island that beats the base AI's landing - the land army on islands is the metal again
(metal_plan 24) and units that survive (fall_back); (3) an island the base AI does not contest, or taking it first
(minute 15-20).

**Verdict:** **FIX FIRST** (`sea_held` from what the fleet has seen recently), then retest 185 + `metal_plan=24` with
fall_back - after metal_plan's retest. Bit 4 (ally islands) is a test artifact, never measured.

### workforce_plan 1 CAP, 2 WOOD, 8 THRIFT (dll/ai/citizens.c; 9b25420)

**Meant to:** stop the oversized workforce (170-227 citizens, 31-63 idle, 70-150 on wood, 8400-14900 wood unspent) and
move it into the army.

**Measured:** CAP: citizens -30 to -43, soldiers unchanged, kl +6 (18) / +20 (14) no cheat, siL cheat -12 (5). 3 (+WOOD)
siL cheat -11 (4). THRIFT: cont soldiers +3.5 to +4, siL kl -37 (16).

**Why it failed:** "the soldiers do not rise, because neither population room (180-250 free) nor food is what limits
them" - gold and iron do. WOOD moves woodcutters to the metals up to the slots near drop sites, and those slots were
already full. THRIFT buys the cheapest hit points x attack per metal: the weak units die (as VALUE).

**Verdict:** CAP + WOOD **FIX FIRST**: they need mine slots to send people to - the outposts (expand_islands) - and are a
companion for that retest (185 + CAP already gave tiH / liH no cheat +21.8 / +20.3, siL cheat -20.4 (14.6), 20 games). THRIFT **REMOVE**.

### food_plan 4 / 8 / 16 / 32 (dll/ai/food.c; f9246a5)

8 HUNT: kl -6.5 / -12.6 - twice the food gatherers, 5% more food: the walk decides, and hunting is the longest -
**REMOVE**. 16 WILD: on islands the fields waited until minute 25-30 but the boats came no sooner (workers -7 to -10) -
**FIX FIRST**: the mod has no fishing boat before minute ~25 (the base AI 3-4 at minute 10, economy.md "Large island
maps" lead 4); with early boats WILD is the playtester's own rule. 4 NEAR (+0.2 / -10.1) and 32 CAMP (+8.2 / -2.3):
neutral, nothing to retest.

### mine_fill (dll/ai/mines.c; f9246a5)

7: 5-8 more on the mines, kl +8 / +10 (the "Large Islands e5" that was a Small map), but workers -4 to -12 (wood went
short) and si5 no cheat base AI wins 3 -> 7. It took workers from wood. Since then citizen_plan 1 trains *new* citizens
for every mine slot near a drop site and navy_mines fills the home island's slots on water maps from minute 15.
**REMOVE** (superseded).

### fight_estimate (dll/ai/defence.c, stats.c; eea5e28, ba55f42)

**Meant to:** the defence decisions (muster, hold_town, attack_retreat, militia) by the real fight estimate (range,
area damage, the damage tables) instead of hit points x attack.

**Measured (10 seeds, scenarios, user's ini):** overrun alive 4 vs 3, kills 49 vs 31; assault alive 8 vs 9, workers 31.5
vs 22.8. Rooted to the old scale: militia normal again, but muster still fired 15/17 vs 7/6.

**Why it failed:** "the estimate is an open-field fight, and a defender at home has more (buildings draw fire, towers,
attackers drawn into range, reinforcements)" - it rated the defenders too low and mustered (gave up the base) too often.
cover.c found one more gap: a static tower out of reach of anything that outranges it is counted wrong (ships dying to
towers show "won" fights). The estimate itself is good per unit - fall_back 2 runs on it.

**Verdict:** **FIX FIRST**: a home advantage in the estimate (own towers, buildings as targets, reinforcements in
reach) and the tower-reach case; then 20+ seeds on overrun / assault and against the base AI on cont.

### hold_town (dll/ai/defence.c; e30a929, c8e9c7f)

**Meant to:** when a stronger army camps by the town, the home soldiers stop being drawn out one by one (the user's
game: P2 lost 320, killed 18), pull back and Guard, and sortie together at 1.3x.

**Measured:** camp (static), 6 pairs: lost 22 vs 31, killed 21 vs 26; camp2 (pushes and pulls back like a player), 5
pairs: killed 27 vs 36, lost 51 vs 59, kept 4 vs 17 citizens and its Town Center 2 vs 5 times.

**Why it failed:** the pull-back point was 6 tiles *beyond* the base, away from the enemy - the soldiers left the
citizens and buildings to the pushes. Strength by hit points x attack without the towers. Evidence thin (5-6 pairs),
but the mechanism is clear.

**Verdict:** **REMOVE**. fall_back 2 does the right version: a unit whose fight is lost falls back under the nearest own
tower on the base's side and fights there with the tower, and muster handles a hopeless fight at home.

### attack_retreat (dll/ai/defence.c; c8e9c7f) - also army_mass 4

Call an attack home at half its peak or facing 1.5x twice. As army_mass 4 against the base AI: cheat kl -19.8 (8.3),
kills -28.7 (9.4); no cheat +2.7 (2.8). It turned back attacks that were trading well against the cheating AI, and
it decides for the whole group. **REMOVE**: fall_back 2 breaks off only the units that are losing, and the attack goes
on.

### assault (dll/ai/assault.c; c11aac0) + assault_outmatched

The mod leads the land attack: gather, go at 1.5x, march as one, fight together. Reference save with a playing camp: P3
killed 6-10 soldiers and 9-16 aircraft of the player instead of 28-34 and 32-34 - the army left the town to the camp;
pathtest: straight into the lasers, kills 20-33 instead of 50. It drops the route planner's tower avoidance and the
weak-spot search, and pulls the army away from home. Its good ideas were taken up separately (army_mass, attack
routing, fall_back). **REMOVE.**

### safe_sites (dll/ai/sites.c; 3f81455)

Citizens only within 20 tiles of the base, routes clear of known enemies. No measurement in the docs; the ini notes "slows
mining (gold/iron) on open maps". Turned on, it bypasses mine_choice, food_plan NEAR and `refuse_danger` (the
tower / threat refusals) - all of which do its job better now - and 20 tiles is far short of where the mines are on
large maps. **REMOVE** (superseded).

### overseas_mines (dll/ai/sites.c; 2026-10-21)

Art of Conquest's fix (docs/engine-notes.md, "Rewritten"). On a water map the citizen assignment asks at its last mine
search (MINE_SITES[5]) for any free mine, whenever a mine type is short or the overseas timer is due, and puts every
idle citizen of the home island on the transport manager's crossing to it. Any mine does, the home island's too: in
the 2017 update 60-75% of these searches found a home mine (`# overseas mines:`, 110-175 a game per AI). The boats
mostly never sail (no citizen aboard in any 5-minute sample of 144 games), but the block takes every idle citizen out
of that pass, and the leftover pass after it is skipped: they stand idle until the next one (10 s). With the switch
the search gets only a mine on another continent than the base (140 a game instead of 62-93), none otherwise.
`scenarios/islands.eeai`, 60 min, 36 seeds x 2 AIs, on - off: Small Islands e5 - citizens +4.2 (2.5), home miners
+3.1 (1.7), miners on other islands +2.2 (1.8), income +659 (1017); Large Islands e9 - citizens +2.3 (1.1), home
miners +1.7 (1.3), income +25 (536); kills, losses, epochs within noise, no crash. A small gain on every count, none
beyond 2 standard errors. **RETEST** with expand_islands (an outpost gives the overseas mines a drop site).

### idle_citizen_slack, citizen_attacks, ai_style

idle_citizen_slack (no new citizens while many idle and food short) stalled growth; citizen_plan's goal and CAP handle
the number of citizens - **REMOVE**. citizen_attacks=0 is the fix itself (attack groups of half citizens are called off),
not an experiment: keep 0. ai_style (personalities change attack timing / army share) was never measured; it is a
personality feature, not a failed experiment.

## What to run next

All against the shipped build (fall_back 7) as the base, `scripts/isl_ablate.sh`, both cheats, 20 seeds x both seats
(40 games a cell), score after `ISL_ABLATE DONE` with the scorer named. A variants file has one `name|keys` line per
variant; the base line is `fb7base|` (empty keys = shipped). New names each run.

| # | Variant lines | MAPS | Scorer | Look for |
|---|---|---|---|---|
| 1 | `fbm24|metal_plan=24,metal_fleet_pct=250,metal_fleet_min=20` | `siL tiH liH si5` | `scripts/metal.py -base fb7base`, `cover.py` | kl with the cheat (was -27 / -19 on siL / tiH), warships and ships lost; then h2h `h2h.sh 75 late "random_map=Small Islands,seed=101,map_scale=large,unit_limit=2500,ai_count=4,start_epoch=5,difficulty=hard" "v_metal_plan=24"` (was ships lost +8.5) |
| 2 | `fbap7|army_plan=7`, `fbwf4|workforce_plan=4` | `coL cont` | `scripts/milspend.py`, `metal.py`, `h2h.py` | soldiers and losses together (army_plan h2h losses was +14.4); then h2h Continental `map_size=1` with `v_army_plan=7` |
| 3 | `fbms1|army_mass=1`, `fbms65|army_mass=65`, `fbms144|army_mass=144` | `cont coL` | `scripts/mass.py`, `cover.py` | cheat kills (was -17); whether GATHER and fall_back hold the same units at cross purposes (`# cover` held / refused) |
| 4 | `fbnm|navy_mix=1` | `siL tiH liH si5` | `scripts/navy_ab.py -mix`, `cover.py` | warships by role, sunk - lost, kl; then the `seaop` scenario |
| 5 | `fbea|early_army=1` | `cont` | `scripts/eco_food.py` / economy log | citizens killed, workers 20/30/60, wins without the cheat (was 3 vs 8) |
| 6 | `fbme|militia_estimate=1`, `fbcp2|copper_plan=2,epoch_share=60%` | `cont coL` / `cont` | `cover.py`, economy log | citizens killed, kl |
| 7 | after 1 and the sea_held fix: `fbx185|expand_islands=185,metal_plan=24,workforce_plan=3` | `siL tiH liH si5` | `scripts/islands.py`, `metal.py` | og60 (outposts still standing at 60), G+I, kl |
| 8 | after 4: `fblb|landing_bombard=1` | `siL tiH liH` | `scripts/landings.py` | ashore, razed, ships lost |

Run 1 and 2 first: they test the clearest claim of this review - that the "more soldiers, more losses" failures were
the waste fall_back removed. If both still lose, the problem is elsewhere (how the army picks fights before they are
lost, not after), and 3-5 drop in priority.
