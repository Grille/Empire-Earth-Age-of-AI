# The early game against the base game's AI

The benchmark (docs/testing.md "Against the base-game AI"): the base game's AI at hard with its Cheat (every price
50%). The mod's AI won 0 of 158 games. The economy log (`# eco:`, `# death:`, dll/test/ecolog.c) and the ablations
(scripts/eco_ablate.sh) gave the diagnosis on Continental (Prehistoric start):

- no soldier until minute ~22, so 1-6 Clubmen and Rock Throwers killed 39-48 citizens a game at the town;
- the citizens were sent against them as militia 40-63 times a game, and they lost;
- the towers were queued, but they stood unfinished;
- before the Copper Age the citizens stopped at 40 workers (`min_workforce`) to hold 1600 food for two epochs, and
  9-10 citizens gathered food (the base AI: 18-29);
- the military account held 500-780 food that was never spent, because the citizens emptied the stock first.

With `copper_plan=0` alone, Continental with the cheat went from 0 won / 12 lost to 6 won / 4 lost.

The playtester's own way, implemented first: "I don't train military in the first epoch and rely on towers and
citizens, but I go heavy on bowmen/rock slingers from the second epoch on. My first-age tactic is getting towers up
and baiting enemy troops under the towers with my citizens."

## The features (dll/ai/early.c, econ.c, defence.c; `[all]` keys)

| Key | What it does |
|---|---|
| `early_towers` (A) | In epochs 0-1 the town's first 2 towers stand by the Town Center, 4 tiles out: one towards the front, one on the side where the citizens work. The game's own tower requests take these spots first, and the mod queues the missing ones while stone allows. Every unfinished tower within 20 tiles of the base gets 2 builders (none when enemy soldiers are within 8). The worker plan asks for their stone. |
| `tower_bait` (A) | Up to epoch 3: a citizen with an enemy soldier within 5 tiles (or one attacking it), and outside its towers' reach, walks to 1.5 tiles behind its nearest finished tower or other shooting building within 25 tiles. Raiders that follow come under fire. The citizen goes back to work through the normal assignment (raided sites are refused). |
| `early_militia` (A) | Citizens fight soldiers only when own soldiers are within 12 tiles or an own tower reaches the spot, and only when the estimate (dll/ai/stats.c) says they win before losing one citizen. |
| `early_army` (B) | No army in epoch 0. Before farms the military account belongs to the army: it pays out whatever the epoch reserve is, and citizens never spend its food. The army gets 10% of food income (`early_army_food_share`, at most 300 held). From epoch 1, while the economy grows (before farms, or under `early_army_workers`=60 workers), a soldier with a food price is replaced by the best food-free one the account can pay for (score x2 for a ranged unit, x0.25 for one that costs food: Slinger, Simple Bowman, Javelin, Composite Bow, Long Bow ...). In epochs 1-3 an Archery Range is requested when there is none. |
| `early_soldiers=N` (alternative to A) | With `early_army`: the home guard keeps N soldiers in epoch 0, from minute 2. |
| `copper_plan=2` (C) | Only the next epoch's cost is held, not the cost of both epochs. Below `min_workforce` the citizens get the whole stock, as before. Above it they are no longer stopped: `epoch_share` (40%) of the income goes into an epoch account until it holds the cost, and the citizens spend the rest. When the map's wild food can no longer pay for the epoch, the epoch gets the food first. Before farms food gets at least half the workers, up to `food_floor` (18). |

Head-to-head keys: `v_early_towers`, `v_tower_bait`, `v_early_militia`, `v_early_army`, `v_early_soldiers`, `v_copper_plan`.
Log: `# early:` once a minute (autotest).

## Why copper_plan was the most harmful feature, and the rework

The plan's idea is right: before farms food is finite, so the AI must count the food it can find and advance as
soon as it can pay. Its brake did the harm, in three ways:

- 1600 food for both epochs was held from minute 0, and the stock almost never got there (30-400 food while the
  citizens trained). So every military request saw `stock - reserve < 0`, and the army got nothing until the Stone
  Age (~20-25 min).
- From 40 workers the citizens stopped completely.
- The worker plan sent food workers only by the food missing from that reserve.

`early_army` handles the first point: the account is the army's own money. `copper_plan=2` handles the other two.
The first try (40% of the income into the epoch account from minute 0) kept the Stone Age from coming at all: in one
start every berry bush and animal was gone at minute 12, which left 26 citizens and 252 food in the account for 45
minutes. Hence the wild-food fallback, and the account only from `min_workforce` on.

## Results (vs the base AI at hard, 60 min, map_size 0 / seed 101, game_seed 1-20 x both seats, build 63af991)

Each cell reads mod won / vanilla won / both standing. `kl` is the mod's kills - losses. The d columns are paired by
seed and seat against the shipped config, with the standard error in parentheses. Workers are counted at 20 / 30 /
60 minutes. Dead = the mod's citizens killed.

| Map | cheat | variant | games | M/V/D | kl | workers | dead | d kl | d dead |
|---|---|---|---|---|---|---|---|---|---|
| Continental e0 | yes | shipped | 40 | 0/12/28 | 33 | 34/39/44 | 23.8 | | |
| Continental e0 | yes | A | 40 | 1/4/35 | 66 | 39/45/51 | 16.1 | +32.4 (10.1) | -7.7 (3.7) |
| Continental e0 | no | shipped | 40 | 16/6/18 | 52 | 41/50/55 | 17.7 | | |
| Continental e0 | no | A | 40 | 13/0/27 | 63 | 39/47/63 | 3.7 | +11.2 (8.5) | -14.0 (3.3) |
| Small / Large Islands e5 | both | A | 40 each | identical to shipped | | | | 0 | 0 |

On the islands A plays the very same games: towers and bait act only in epochs 0-1 and up to 3, and the militia rule
only against soldiers in the early epochs.

Continental, 10 seeds x both seats. The pre-rebase builds are 845e519 and before; A was identical to its
re-measurement above.

| cheat | variant | M/V/D | kl | workers | dead | d kl | d dead |
|---|---|---|---|---|---|---|---|
| yes | A+B | 0/1/19 | 66 | 39/46/55 | 14.9 | +39.3 (17.7) | -11.6 (4.4) |
| yes | A+B+C (epoch_share 40%) | 0/2/18 | 32 | 44/50/46 | 20.6 | +5.3 (16.8) | -5.9 (7.3) |
| yes | A+B+C (epoch_share 60%) | 0/3/17 | 50 | 42/47/43 | 17.8 | +23.1 (13.1) | -8.7 (5.9) |
| yes | A+B, copper_plan=0 | 0/0/20 | 25 | 36/47/55 | 10.7 | -1.6 (11.6) | -15.8 (4.7) |
| yes | alternative: 4 soldiers in epoch 0 + B + C | 0/9/11 | 16 | 29/34/27 | 29.7 | -10.7 (7.2) | +3.2 (5.4) |
| no | A+B | 3/0/17 | 69 | 40/49/68 | 3.5 | +12.4 (10.0) | -11.3 (5.0) |
| no | A+B+C (60%) | 6/0/14 | 80 | 46/61/70 | 4.0 | +22.6 (8.8) | -10.8 (5.4) |
| no | alternative | 3/3/14 | 51 | 35/49/52 | 19.4 | -6.2 (10.9) | +4.6 (6.6) |

## Status (2026-10-19)

- **A** (towers, bait, careful militia) is **on by default** (code and config/aimod.ini). Against the base AI on
  Continental it clearly wins: with the cheat the base AI's wins drop from 12 to 4 of 40, kills - losses goes +32, and
  8 fewer citizens die; without the cheat the base AI never wins (6 -> 0) and 14 fewer citizens die. On the islands
  (epoch 5 starts) the games are identical. Head-to-head against the control AI (build e120a6c,
  `REPEAT=24 sh scripts/h2h.sh 60 late "random_map=Continental,seed=101,map_size=1,ai_count=4,start_epoch=0"
  "v_early_towers=1,v_tower_bait=1,v_early_militia=1"`, 24 games, 48 AIs a side), B - A per game: alive +0.08
  (0.04), kills +6.9 (5.1), losses -9.3 (5.6), workers +4.8 (2.1), soldiers +1.4 (2.1), epoch -0.08 (0.06). No
  regression.
- **B** (early army) on top of A gets the same or a little better with the cheat (kills - losses +38, 10-11 fewer
  dead, 24 and 20 games). Without the cheat it wins fewer (3 of 20 vs 8). It is not clearly better than A, and it
  also acts on the islands (substitution while under 60 workers). The islands with the cheat (A+B against shipped,
  40 games each) came out neutral: Small Islands e5 kills - losses -3.2 (4.8), citizens killed +1.4 (4.2); Large
  Islands e5 +1.1 (7.5), -2.1 (4.6). Off.
- **C** (`copper_plan=2`) brings more workers at 20/30 minutes (+5 to +13). With the cheat it costs at 60 minutes
  (fewer workers, kills - losses), because the slower Stone Age delays the army. With epoch_share 60% and no cheat it
  does best (6/0/14, kills - losses +23). Mixed result. Off: `copper_plan=1` stays, its brake now partly offset by B.
- **The alternative** (4 soldiers by minute 8 instead of A) is the worst variant. With the cheat the base AI wins 9
  of 20 and 30 citizens die. The playtester's tower-and-bait opening is the better one for this AI.

## Food and mines (2026-10-20, dll/ai/food.c, dll/ai/mines.c)

The playtester: hunting is the first step and herds should be taken fast; farms only when wild food runs low (in
late starts too: herds, forage, fish first); every mine the AI can get hold of filled, the home island's always.
Measured first with the economy log (`# eco:` fo / fh / ff / fb, the hunt list, animals, forage, fields, mines
without a drop site; `scripts/eco_food.py`), base AI at hard, 60 min, game_seed 1-20 x both seats:

- **Food sources.** Continental e0: the mod's AI had 9-11 food gatherers (the base AI 10-21), 7-9 of them at bushes,
  1-2 hunters (the base AI 3-5). Forage is the main wild food (bushes of 2500, ~10000-16000 food within 30 tiles of
  the drop sites all game - it does not run out); animals are few (22 on the whole map, ~4200 food). The islands
  (e5): one bush and ~4 animals at home, 24000 fish; the base AI fished with 6-8 boats and built no field, the mod
  built 8 fields by minute 10 and had no fishing boat before minute 20.
- **How the game feeds its AI.** The assignment's food passes (0x48A554): forage near the base (the *first* free bush
  in zone order, not the nearest), finished fields, hunting (one hunter an animal a pass, from the hunt list rm +0x88 -
  built once at the AI's start and only pruned, so animals born or come later are never hunted), forage and hunting
  farther out, then field foundations (land) or fishing boats (+0xEFA). All of it only up to the food target, and no
  gatherer is ever taken back: a raid sent 20 food gatherers home, the leftover pass put them on gold, and they stayed
  there (1 on food, target 30, 1400 gold unused).
- **The real limit before farms** was the worker plan: food got workers only for the epoch reserve's shortfall, not
  for the citizens the Town Center trains, and the game's wood demand (refused requests, 4500 in epoch 0) took half
  the workers.

`food_plan` ([all], bits; `v_food_plan` for head-to-head): 1 PLAN - before farms the plan counts the citizens' food
(50 a citizen for the next 10 while short) and the wood only of real buildings; 2 PULL - every 10 s up to 4 gatherers
back to food from the list farthest above its target; 4 NEAR - forage at the bush nearest a drop site; 8 HUNT - idle
citizens hunt every animal within `food_hunt_range` (15) tiles of a drop site (newborns too), 3 to an animal; 16 WILD -
from the Copper Age no field builders while wild food is free; 32 CAMP - a Settlement (100 wood) by the richest wild
food far from the drop sites, before farms. **Default 3 (PLAN + PULL).**

Paired against the shipped build (40 games a cell; d = variant - shipped, standard error; epoch-up = the minute of the
first epoch advance):

| Continental e0 | cheat | M/V/D | d kills-losses | d workers 30 / 60 | food gatherers 20 min | d epoch-up |
|---|---|---|---|---|---|---|
| shipped | no | 13/0/27 | | 47 / 64 | 10.1 | (32.3 min) |
| 3 PLAN+PULL | no | 22/1/17 | +9.6 (6.3) | +8.7 (1.7) / +7.3 (2.7) | 17.7 | -7.7 (1.5) |
| shipped | yes | 2/3/35 | | 45 / 53 | 10.4 | (33.5 min) |
| 3 PLAN+PULL | yes | 2/4/34 | +13.0 (13.9) | +6.7 (2.4) / +3.4 (4.9) | 16.1 | -8.7 (2.6) |

Other variants on Continental (no cheat / cheat, d kills-losses, d workers 30): 1 PLAN +4.5 / +20.2, +5.3 / +4.8;
4 NEAR +0.2 / -10.1, +7.0 / +1.2; 8 HUNT -6.5 / -12.6, -3.5 / -3.7 (23 of 28 citizens chased 15 animals; with twice
the food gatherers the food gathered by minute 20 rose only 5% - the walk decides wild food's rate, and hunting is
the longest walk); 32 CAMP +8.2 / -2.3; 7 and 35 more workers than 3, worse fights. Islands e5 (only PULL acts there):
3 kills-losses +11.4 / +4.0 (Large, no cheat / cheat), +0.0 / -5.8 (Small), workers within +-2 at 30 min. 19 (+ WILD)
on the islands: the fields waited until minute 25-30 but the game's boats came no sooner - workers -7 to -10 at 30
min, the base AI's wins on Small Islands 3 -> 5 / 25 -> 28. Head-to-head against the control AI (Continental e0,
map_size 1, 4 AIs, 60 min, 24 games, `v_food_plan=3`): B - A alive +0.06 (0.03), epoch +0.10 (0.05), workers +2.8
(2.1), kills -0.4 (4.1), losses +7.0 (7.2) - no regression.

**Mines.** Small Islands e5: the home island's mines have 30 slots near the Town Center; the mod's AI worked 12-17,
the base AI 15 at minute 20 and then built Settlements (76 slots by the end). `mine_fill` ([all], bits, `v_mine_fill`):
1 FILL - from the Copper Age the metals' targets are at least their slots near drop sites (the home continent's only
under a population cap of 120), unless 1500+ already lie in stock; workers from piled-up wood, spare food and idle
citizens; 2 DROP - a Settlement at the home continent's unworked mine nearest to a drop site; 4 FIRST - the slots
before wood and food beyond a fifth / a third of the workers. 3 changed next to nothing (the givers are rarely
there: wood is spent as it comes; mine gatherers +-0). 7 put 5-8 more on the mines and raised kills - losses by
+8 / +10 (Large, no cheat / cheat) and +9 (Small with the cheat), but the workers fell by 4-12 (wood went short) and
Small Islands without the cheat lost more (base AI wins 3 -> 7). **Off (0)**: filling the mines pays only with
wood to spare - the next step is more woodcutters' income, not fewer.

## Citizen production (2026-10-20, dll/ai/citizens.c)

The playtester: "Filling the mines is a task after ensuring enough food income that citizens can continuously be
trained; not enough citizens should never be a real limit to filling a mine after the early game or in very low pop
games."

**How the game trains citizens** (unit manager 0x490AD6, at 0x490B60): a request only while there is room under the
cap, the attention flag um +0x140 is on, no citizen request waits and no producer has a citizen queued; for the
citizens wanted (0x48EDDB: player +0xBDC x cap, 45% - 75 of a cap of 166) less those there, at most 10 (5 on water
maps). 0x49014F hands them round-robin to every finished Town Center / Capitol. A citizen takes ~28 s of a producer.

**Measured** (`# cit:` once a minute, autotest: per producer and second training a citizen / something else / nothing,
and why nothing; `scripts/cit.py`). Base AI at hard, 60 min, 20 seeds x both seats. Producer time idle and the main
reason (mod; minutes 10-30 / 30-60):

| Map | cheat | idle 10-30 | why | idle 30-60 | why |
|---|---|---|---|---|---|
| Continental e0 | no | 49% | 37% the mod's budget (the epoch reserve) | 81% | 58% the citizens wanted are there |
| Small Islands e5 | no | 34% | 22% under 50 food in stock | 83% | 56% wanted are there |
| Large Islands e5 | no | 27% | 18% food | 85% | 56% wanted are there |
| Continental e0 | yes | 52% | 31% budget, 14% food | 77% | 42% wanted, 20% food |
| Small Islands e5 | yes | 48% | 39% food | 85% | 62% food |
| Large Islands e5 | yes | 36% | 30% food | 81% | 42% food, 24% wanted |

So the game's 45% share stopped the citizens at ~75 while 40-60 mine slots near the drop sites had 12-24 miners, and
food ran short on the islands. Houses play no part: the cap is the game's (+0xCCC), not built.

`citizen_plan` ([all], bits, `v_citizen_plan`): 1 GOAL - from the Copper Age the citizens wanted are the goal when more:
every mine slot within 12 tiles of a drop site (home and wherever the AI holds one), the food and wood gatherers, the
idle and 4 builders, within the cap less `citizen_army_share` (40%) for the army (the home mines always fit, at most
80% of the cap); the metals' targets go up to those slots from food beyond its floor (FEED, fields + 2) and wood
beyond its late target, never from a mine. 2 FEED - food gets the gatherers that pay the producers the missing
citizens need (one producer per 10 missing). 4 DROP - mines.c's Settlement at the home continent's unworked mine.
8 FIRST - citizens before the epoch reserve while the food income pays both. **Default 3.**

Paired against the same build without (build 29914e2 + citizens.c, 40 games a cell; d = with - without, standard
error; metal = the mod's stone + gold + iron income in 60 min; wins = mod / base AI):

| Map | cheat | d workers 60 | d miners 60 | d metal | d soldiers 60 | d kills-losses | wins without -> with |
|---|---|---|---|---|---|---|---|
| Continental e0 | no | +16.1 (3.2) | +6.4 (2.2) | +955 (303) | -8.5 (3.3) | -5.8 (3.6) | 21/1 -> 20/1 |
| Small Islands e5 | no | +29.9 (4.4) | +4.5 (1.9) | +1175 (611) | -4.5 (2.9) | -9.0 (8.0) | 1/2 -> 0/2 |
| Large Islands e5 | no | +32.1 (3.3) | +8.0 (1.8) | +2040 (426) | -3.9 (2.7) | +1.3 (6.3) | 0/1 -> 1/1 |
| Continental e0 | yes | +15.2 (4.8) | +7.3 (2.5) | +680 (281) | -0.5 (3.7) | +0.4 (9.4) | 2/5 -> 3/3 |
| Small Islands e5 | yes | +7.6 (7.7) | +1.7 (1.8) | -102 (599) | +0.2 (2.3) | -4.8 (5.7) | 0/26 -> 0/19 |
| Large Islands e5 | yes | +34.9 (8.0) | +7.3 (3.1) | +1720 (825) | +2.8 (3.8) | -16.0 (6.6) | 0/14 -> 0/9 |

Producer time idle from minute 30 fell 5-12 points. Small Islands' home mines have 30 slots near the drop sites and
they were full: there the extra citizens cut wood and gather food - more mines need drop sites on other islands.
The first FEED fed every producer nonstop and took from the largest target (the mines): +19 workers on Continental,
all on food, no fuller mines, 11-13 soldiers fewer. 7 (+ DROP) put 6-9 more on Continental's mines (16 home mines had
no drop site) but cost 13 soldiers; FIRST changed nothing. Head-to-head against the control AI (Continental e0,
map_size 1, 4 AIs, 60 min, 24 games, `v_citizen_plan=3`): B - A alive +0.02 (0.02), workers +24.5 (2.4), razed +6.0
(1.7), soldiers +1.7 (1.7), kills -1.8 (3.7), losses +10.7 (6.1). Low population (`lowpop`, cap 30, 20 games):
workers +10.2 (0.3), soldiers -3.8 (0.7), kills +10.2 (3.0), losses -12.6 (3.1), razed -4.2 (2.2).

The runs also found an engine crash that more citizens set off: 0x630466 sorts the enemies in a unit's range with
std::sort, whose partition compares an 80-bit key with the pivot's float and runs off the array (0x62D5AC, 6 of 360
games; dll/ai/attack.c range sort guard, 29914e2).

## Economy into army (2026-10-20, dll/ai/army.c)

The playtester: "later everything it can get hold of should be exploited and converted into better attack and
defence capability". citizen_plan brought more workers and +700..+2000 stone/gold/iron an hour, but no bigger army.

**Measured** (`# mil:` once a minute, autotest, both sides: stock, military account, reserve, army and its value,
production buildings and their busy share, towers, walls, techs, the game's unit wishes, the military orders and why
they were refused; `scripts/milspend.py`). Base AI at hard, 60 min, 20 seeds x both seats, mod (M) / base AI (V) at 60
min; refused = the mod's military orders the budget refused from minute 20 (wood / gold / iron short):

| Map | cheat | army M / V | value M / V | production bld M / V | busy M / V | towers M / V | stock M / V | refused (W/G/I short) |
|---|---|---|---|---|---|---|---|---|
| Continental e0 | no | 39 / 4 | 3870 / 430 | 5.7 / 1.2 | 34 / 31% | 6.2 / 3.1 | 4240 / 7600 | 2600 (580/760/680) |
| Small Islands e5 | no | 22 / 34 | 3730 / 4150 | 11.1 / 12.7 | 21 / 21% | 6.3 / 3.5 | 5740 / 8100 | 4590 (2210/1970/1470) |
| Large Islands e5 | no | 32 / 25 | 6090 / 2880 | 13.7 / 10.2 | 28 / 20% | 10.4 / 2.9 | 7560 / 6060 | 3760 (1980/1560/1130) |
| Continental e0 | yes | 24 / 10 | 2390 / 1020 | 5.8 / 3.0 | 28 / 33% | 4.5 / 7.6 | 2630 / 18960 | 2480 (580/820/770) |
| Small Islands e5 | yes | 6 / 32 | 820 / 4340 | 6.7 / 14.3 | 12 / 20% | 3.6 / 2.8 | 1150 / 16950 | 2880 (1470/850/1100) |
| Large Islands e5 | yes | 13 / 43 | 2020 / 6290 | 8.8 / 17.1 | 24 / 17% | 5.4 / 3.5 | 2680 / 23140 | 5070 (2390/2000/1590) |

The money is spent: from minute 20 the mod's stock averaged 950-3000 (the base AI 2900-12500, it trains little of what
it asks for), the reserves held 0-560, the military account 2000-11000 more than the stock ever had; the budget refused
the army 2600-5000 times, each time short of wood, gold or iron (food rarely); the production buildings trained 12-34%
of the time, room refused nothing. Food and wood pile up only in the last 15 minutes (the 60-minute stock). The mod
researches more than the base AI (Continental 31 techs to 13) but pays its line upgrades (category 7, ~40 on the islands,
about 3700 an hour), which the base AI gets free. Towers it has more (4-10 to 3-8); stone is spent (150-390 at 60 min).
So the economy's size and mix, not the budget, buildings or room, caps the army.

`army_plan` ([all], bits, `v_army_plan`; from the Copper Age, only with an enemy on the AI's own continent): 1 FLOAT - the
stock above every reserve that stayed for 2 minutes (less `army_float_keep` 300 food/wood, `army_float_keep_mineral` 100)
tops up the military account; 2 FILL - every 10 s, while the account and stock pay one, up to 5 soldiers into the room
the citizens' goal leaves (the game's own requested type when it can train it, else the strongest); 4 BUILD - one more
Barracks / Archery Range / Stable (the kind of that soldier) when they trained 70%+ of the time and the account holds
600, up to 2 + workers / 15; 8 MIX - with food 500 above its reserve for 2 minutes, up to 8 food workers to the wood,
gold or iron the army's orders lacked; 16 UPGRADE - line upgrades wait until the line is in use. **Default 0.**

Paired against the same build without (40 games a cell; d = with - without, standard error):

| Variant | Map | cheat | d soldiers 45 / 60 | d trained | d bld | d kills-losses | base AI wins |
|---|---|---|---|---|---|---|---|
| 7 | Continental e0 | no | +2.0 (1.8) / +4.5 (1.6) | +22 | +2.3 | -0.8 (3.3) | 1 -> 1 |
| 7 | Continental e0 | yes | +0.8 (1.4) / +5.3 (2.2) | +11 | +0.8 | +7.0 (4.6) | 3 -> 3 |
| 7 (first, on islands too) | Small Islands e5 | no | -4.7 (2.6) / +0.1 (3.3) | -6 | +0.4 | -6.6 (9.8) | 2 -> 3 |
| 7 (first) | Small Islands e5 | yes | +2.2 (1.9) / +2.9 (2.1) | +5 | +0.5 | +12.8 (5.6) | 19 -> 17 |
| 7 (first) | Large Islands e5 | no / yes | +1.4 / +1.5, +1.1 / +0.2 | +8 / -3 | +1.5 / +0.7 | -7.2 (8.7) / +7.2 (7.0) | 1 -> 1, 9 -> 10 |
| 15 (+ MIX) | all | | as 7 (MIX moved a few workers, rarely) | | | | |
| 16 UPGRADE | all | | -1.4 .. +6.7 at 45 | +-3 | | -0.6 .. +5.0 | 19 -> 21 (Small, cheat) |

Head-to-head against the control AI (Continental e0, map_size 1, 4 AIs, 60 min, 24 games, `v_army_plan=7`): B - A
soldiers +4.2 (2.1), kills +1.1 (4.6), **losses +14.4 (5.8)**, razed +2.6 (2.0), alive +0.02. Low population (`lowpop`,
cap 30, 20 games): soldiers +1.9 (1.0), kills +22.6 (11.1), losses -22.4 (10.2), razed +8.1 (3.0). On the islands the
games with 7 are now the same as without (16 checked). The extra soldiers come in a trickle and the game's attacks and
home guard feed them in piecemeal: more soldiers, as many kills, more losses - against the base AI they changed nothing
that counts. Off by default; the next lever is the economy's mix (wood, gold, iron income) and how the army fights, not
its budget.

## The army as one (2026-10-20, dll/ai/mass.c)

The playtester: against the base game's nonstop waves a proper AI gathers under cover (towers, Town Center,
production), lets the waves break on the towers and the massed army, strikes only when the mass is strong enough
against the force it knows, and never feeds single soldiers into a fight - reinforcements gather and join as groups.

**Measured** (`# mass:` once a minute, autotest, both sides, sampled every 5 s: land soldiers alone - 2 or fewer own
within 6 tiles -, away from the base - 25+ tiles -, in a fight; the land attack subgroups out and their stragglers -
12+ tiles from their subgroup's middle -; the land soldiers killed - alone, away, outnumbered 2:1 within 8 tiles;
`scripts/mass.py`). Base AI at hard, Continental e0, 60 min, 20 seeds x both seats, the mod's AI as shipped:

| | cheat | soldiers | alone | alone and away | stragglers | killed | killed alone | killed away | alone and away | outnumbered |
|---|---|---|---|---|---|---|---|---|---|---|
| mod | no | 12.6 | 36% | 20% | 22% | 32.6 | 50% | 84% | 47% | 17% |
| mod | yes | 10.0 | 39% | 20% | 25% | 45.9 | 48% | 70% | 44% | 23% |
| base AI (20 games) | no | 3.3 | 78% | 27% | 11% | 44.4 | 84% | 60% | 51% | 45% |

Head-to-head (Continental e0, 4 AIs) the control AI's soldiers died alone in 65% of the cases, 45% of them away from
the base; lowpop 84%. So half the mod's soldiers die alone, nearly all of them out on an attack: the game's
attack subgroups of ~10 walk their own legs, a quarter of their members string out, and the attack commander hands
every newly trained soldier to a subgroup that is out - it walks there alone (the trickle).

`army_mass` ([all], bits, `v_army_mass`; only with an enemy on the AI's own continent, navy_land - island games are
the same as without, 8 checked): 1 GATHER - a member of a land attack subgroup that is out (past forming, its members'
middle 30+ tiles from the base) standing at home (a replacement) is held: the subgroup's orders to it are refused (vf57,
as for the fleet and the landing) and it walks to the rally point (by the own tower nearest the point 6 tiles out
toward the subgroup); the held members of a subgroup go together once 5 wait (`army_mass_group`), 3 for 2 minutes
(`army_mass_hold`), any after 4; a released one isn't held again for 3 minutes. 8 - GATHER only while that subgroup
fights (state 3000/3003). 2 WAIT - the attack time (+0xED8) moves on by 20 s while the land soldiers' hit points x
attack are under `army_mass_ratio` (130%) of the strongest enemy's army known (intel.c), `army_mass_wait` (10)
minutes at most. 4 RETREAT - attack_retreat (defence.c) for this AI. **Default 0.**

Paired against the build without (d = with - without, standard error; Continental e0, base AI at hard, 20 seeds x
both seats = 40 games a cell unless noted):

| Variant | cheat | d kills - losses | d kills | d losses | d razed | alone and away | mod wins / base AI wins |
|---|---|---|---|---|---|---|---|
| 1 GATHER | no | +11.0 (3.4) | +7.0 (3.4) | -4.0 (2.7) | -0.6 (1.1) | 20% -> 14% | 20/1 -> 14/1 |
| 1 GATHER | yes | -9.7 (8.0) | -17.0 (8.1) | -7.3 (3.5) | -2.3 (1.0) | 20% -> 14% | 3/3 -> 0/4 |
| 9 (only while fighting) | no | +3.8 (2.9) | -1.2 (2.9) | -5.0 (2.8) | -2.0 (0.9) | 20% -> 17% | 20/1 -> 17/1 |
| 9 | yes | -5.9 (5.6) | -9.0 (6.0) | -3.1 (2.8) | -2.3 (1.1) | 20% -> 13% | 3/3 -> 1/3 |
| 1, group 3, hold 60 s (20 games) | no / yes | +4.8 (4.7) / +5.5 (7.9) | +7.0 / -4.2 | +2.2 / -9.6 (4.6) | -1.0 / -0.3 | | 12/0 -> 9/0, 0/2 -> 0/2 |
| 2 WAIT (20 games) | no / yes | -1.0 (4.6) / -0.2 (6.7) | | +5.1 / +3.7 | | | 12/0 -> 13/0, 0/2 -> 0/4 |
| 4 RETREAT (20 games) | no / yes | +2.7 (2.8) / -19.8 (8.3) | +1.5 / -28.7 (9.4) | -1.2 / -8.9 (3.9) | | | 12/0 -> 9/0, 0/2 -> 0/3 |
| 7 (20 games) | no / yes | -6.6 (6.7) / -19.5 (12.2) | -7.2 / -26.8 | -0.6 / -7.3 | | | 12/0 -> 5/0, 0/2 -> 0/2 |
| army_plan 7 + 1 against army_plan 7 (20 games) | no / yes | +3.4 (7.3) / +6.9 (12.1) | | -2.8 / -5.1 | | | 13/0 -> 10/0, 0/2 -> 0/3 |

Head-to-head against the control AI (`v_army_mass=1`): Continental e0, map_size 1, 4 AIs, 24 games: B - A kills
+9.0 (3.6), losses +1.3 (5.8), soldiers +1.1 (1.6), razed -0.5 (2.1), alive +0.02; its soldiers alone 49% -> 30%,
outnumbered at death 49% -> 36%. lowpop (cap 30, 20 games): kills +9.6 (5.4), losses -6.9 (5.0), razed -3.5 (1.8),
the soldiers killed in the `# mass` count 19.1 -> 9.7.

GATHER does what it should - fewer soldiers alone, fewer lost, and against the control AI and the base AI without
the cheat more kills - but the attacks hit with less at once: razed buildings fall everywhere and against the
cheating base AI (twice the army) it kills 17 fewer and loses 1 game more of 40. WAIT hardly acts (the attack time
seldom comes while weaker - the game launches with what it has, under a minute of waiting a game); RETREAT turns
attacks back that were trading well against the cheating AI. Off by default. Next: the attack itself (subgroups
walking separate legs, a quarter of their members strung out) - one front instead of several, without assault's
failures (docs/ai-analysis.md); forward bases / rally points on islands once the landing is ashore.

### The attack as one front (2026-10-20)

**Measured first** (`# mass2:` once a minute, autotest, both sides; `scripts/mass.py -states`): the land subgroups'
members by state (forming, marching 2000s, fighting 3000s, other), their stragglers (12+ tiles from the middle) at
home - the replacements -, ahead of the middle (away from the base) and behind it; the fronts (out subgroups 20+
tiles from each other); the subgroups' orders (vf57, moves / attacks); and where the land soldiers that die stood -
in an out subgroup's pack (within 12 tiles of its middle), ahead of it, behind it, on the way from home, in no
subgroup out. Base AI at hard, Continental e0, 20 seeds x both seats, the mod as shipped:

| cheat | members fighting / marching / forming | stragglers fighting: home / ahead / behind | marching | killed in the pack / ahead / behind / from home / none out |
|---|---|---|---|---|
| no | 8.3 / 1.2 / 0.7 | 10% / 15% / 7% | 8% | 67% / 13% / 7% / 1% / 13% |
| yes | 7.8 / 0.6 / 0.7 | 11% / 18% / 9% | 15% | 61% / 16% / 8% / 1% / 13% |

So the attack is not strung out on the march - a marching subgroup holds together (its few stragglers are mostly
replacements still at home), and the march waits while any
member is engaged, so the subgroups spend most of an attack in the fight state. The quarter of stragglers are
fighters chasing targets 12+ tiles ahead of their pack or left behind, and the replacements; two thirds of the
soldiers killed die in their pack (the 6-tile "alone" count of the `# mass:` line is too tight for a pack fighting
over 10-20 tiles), one in eight chasing ahead of it, hardly any on the way from home. With 2+ subgroups out (a
third of the minutes) 4.9 subgroups stand on 2.1 fronts - but a land subgroup asks for a route only ~17 times a game:
the fronts come from the fights (each subgroup's members pick targets among the enemies it sees), not the marches.

Built (`army_mass` bits, land and mixed maps as before):

- 16 TETHER - a member of a land subgroup out picks no target more than `army_mass_tether` (12) tiles from the
  middle of its members away from the base, unless the target is within its own range + 2 (attack.c
  hook_asgu_score: the scorer returns 0, as the leash does for tower range). Nothing left in reach: the subgroup is
  not engaged, so its march goes on - the pack moves together. 128 (with 16): only buildings - enemy units are
  fought wherever they are.
- 32 REGROUP - a member out, 16+ tiles from that middle and attacking nothing, walks back to it (acted under once a
  minute: the strays are fighting).
- 64 JOIN (with 1) - GATHER's released reinforcements walk to their subgroup's pack instead of waiting for its
  next order.
- A goal switch for one front (a subgroup's route request to an enemy building turned to the goal of the biggest
  other subgroup out on the same player, as the weak spot switches goals, the route planner still planning the
  way) never acted in 80 games - dropped.

Paired against the build without (d = with - without, standard error; Continental e0, base AI at hard, 20 seeds x
both seats = 40 games a cell unless noted):

| Variant | cheat | d kills - losses | d kills | d losses | d razed | stragglers | mod wins / base AI wins |
|---|---|---|---|---|---|---|---|
| 16 TETHER | no | +6.3 (3.9) | -1.1 (3.8) | -7.4 (2.8) | +2.2 (1.3) | 22% -> 17% | 20/1 -> 19/1 |
| 16 TETHER | yes | -9.1 (5.7) | -13.9 (5.6) | -4.8 (4.0) | -0.4 (1.2) | 25% -> 17% | 3/3 -> 1/2 |
| 16, tether 20 | no / yes | +2.5 (3.0) / -2.6 (4.4) | -1.0 / -7.7 (4.9) | -3.5 / -5.2 (3.0) | +0.1 / -1.2 | | 20/1 -> 19/1, 3/3 -> 1/3 |
| 48 TETHER + REGROUP | no / yes | +6.2 (4.0) / -5.8 (5.2) | +2.2 / -12.7 (5.3) | -4.0 / -6.9 (3.0) | +1.6 / +0.2 | 22% -> 16%, 25% -> 14% | 20/1 -> 18/1, 3/3 -> 1/2 |
| 144 TETHER buildings (80 games) | no | +2.6 (1.5) | +0.7 (1.2) | -1.8 (1.3) | -0.3 (0.6) | | 40/2 -> 42/2 |
| 144 (80 games) | yes | -2.5 (3.5) | -1.9 (2.6) | +0.6 (1.8) | -0.2 (0.4) | | 4/5 -> 3/5 |
| 17 GATHER + TETHER | no / yes | +9.4 (3.7) / -16.4 (6.0) | +2.2 / -24.4 (7.1) | -7.1 / -8.0 (4.0) | -2.2 / -1.9 | | 20/1 -> 12/1, 3/3 -> 0/2 |
| 65 GATHER + JOIN | no / yes | +6.7 (3.5) / -11.8 (7.5) | +2.7 / -20.4 (7.5) | -4.0 / -8.7 (3.9) | -0.6 / -2.5 (1.2) | | 20/1 -> 19/1, 3/3 -> 0/4 |
| 145 GATHER + TETHER buildings | no / yes | +7.9 (3.2) / -10.3 (7.8) | +3.2 / -15.3 (8.6) | -4.6 / -4.9 (3.5) | -0.1 / -2.6 (1.1) | | 20/1 -> 13/1, 3/3 -> 2/3 |

Head-to-head against the control AI (`v_army_mass=144`): Continental e0, map_size 1, 4 AIs, 48 games (game_seed
1-48): B - A kills +1.1 (3.2), losses +6.0 (4.0), razed +2.5 (1.6), soldiers +1.8 (1.5), alive 0; lowpop (20
games): kills +1.4 (3.3), losses -0.9 (2.7), razed +2.8 (1.7), soldiers -1.2 (0.8).

Holding the army together works as measured - TETHER cut the stragglers by a quarter to a third and the losses
without the cheat by 7 - but every way of holding it back (GATHER, TETHER on units, both) costs kills against the
cheating base AI (-14 to -24): against twice the army the chasers' kills count, and the pack that waits trades
less. TETHER on buildings only (144) is neutral everywhere (80 games a cell within 1.5 standard errors, the
head-to-head losses +6 (4)). JOIN made GATHER's released groups keep the pressure (no-cheat wins 20 -> 19 instead of
14) but not its cost against the cheat. Nothing gains clearly without a loss elsewhere: all **off** (`army_mass=0`).
What the numbers point to instead: the deaths are in the pack's fights, so the lever is which fights the pack takes
and when it breaks one off (the fight estimate, the retreat's thresholds against what the pack really faces), not
how it walks.

## Other islands (2026-10-20, dll/ai/expand.c)

The playtester: "work every resource node on the home island fast, then expand to other islands - early it must
prioritise, later everything it can get hold of"; "a lone mining camp with a few citizens on an empty island won't
hold: a few towers and even barracks for reinforcement are needed".

**Measured** (`# isl:` every 5 minutes, autotest, both sides; `scripts/islands.py`). Base AI at hard, 60 min, map seed
101, 20 seeds x both seats. The land continents by what they are to each player (home, own, contest, enemy, ally - the
tests' idle human -, empty), their mines (stone + gold + iron sites of the zones; trees are no zone sites) and the
player's miners there:

- **The maps.** Small Islands e5: home 5 mines (30 slots), one free island in the middle with 7 (S3 G3 I1, 42 slots,
  40-50 tiles off), the idle human's island 5. Large Islands e5: home 8 mines (48 slots), no free island (the idle
  human's has 8). Tournament Islands e5: home 5, no free island (the human's 5). The same on map seeds 102-106 and
  with map_size 1-4 (the size setting changes nothing on these starts).
- **Who settles.** The base AI puts drop sites on another island in 90-100% of the games, first at minute 31-35
  without the cheat, 22-25 with it; by minute 45 it has 9-10 mines and 9-12 miners there on Small Islands (2-4
  Settlements, no tower) - on Large and Tournament Islands the idle human's island. The mod's AI never settled one by
  itself: the drop sites it had off its home island (47% of the Small and Large Islands games without the cheat, from
  minute ~47; 0-7% with the cheat) were its landings' forward bases on the enemy's island.
- **Its home mines** were not the limit: 24-38 miners at 60 minutes without the cheat (Small Islands 24 of 30 slots,
  Large 38 of 48), the base AI's 21-33; with the cheat the mod's collapse (8-24 at 60 minutes) decides.

| Map | cheat | home miners M / V at 30 | at 60 | other islands' miners M / V at 60 | V settled (first min) | mod won / base AI won |
|---|---|---|---|---|---|---|
| Small Islands e5 | no | 19.8 / 19.2 | 24.0 / 21.6 | 4.3 / 10.8 | 100% (31) | 0 / 2 |
| Large Islands e5 | no | 26.2 / 17.7 | 38.2 / 21.1 | 1.8 / 5.3 | 90% (35) | 1 / 1 |
| Tournament Islands e5 | no | 26.4 / 18.7 | 25.5 / 23.0 | 1.7 / 7.5 | 95% (32) | 0 / 4 |
| Small Islands e5 | yes | 11.0 / 22.6 | 7.6 / 23.3 | 0.0 / 13.9 | 97% (23) | 0 / 19 |
| Large Islands e5 | yes | 21.3 / 26.7 | 23.5 / 33.5 | 0.1 / 10.2 | 100% (25) | 0 / 9 |
| Tournament Islands e5 | yes | 9.7 / 21.6 | 3.2 / 22.2 | 0.0 / 15.6 | 100% (22) | 0 / 31 |

So on these maps only Small Islands has a free island, and it is the base AI's from minute 23-31.

`expand_islands` ([all], bits, `v_expand_islands` for head-to-head; header of dll/ai/expand.c): 1 SETTLE - from the Copper
Age, minute 8 (`expand_minute`), once the home island's mine slots are half worked (or from minute 15), with 25+ workers
and a land army not weaker than the enemy army it knows: the island with the most mines it has explored, no ally's
building, no enemy town (Town Center, Capitol), no known enemy defence, at most 3 known enemy buildings and no more
known enemy soldiers than its guard. A transport (its own idle one, else trained as a landing's) takes 8 citizens
(`expand_citizens`; never miners) and 3 land soldiers (`expand_guard`) from the home shore nearest the base and the
island to the island's shore nearest its mines (the sea on the way checked as the landing does - held: unloaded at home
at once). Ashore: a Settlement by the mines, a tower between the mines and the sea, one by the Settlement; once the
Settlement stands the citizens gather at the island's free mines (the game's assignment), the guard stays by it; more
citizens every 2 minutes up to the mine slots (`expand_trips` 4 in all); a transport, a Settlement and a tower are held
back from unit orders until laid. Lost (nothing of it left there): again after 8 minutes, 3 losses: never again. 2 =
a barracks there too; 4 = islands with an ally's buildings count as free.

What the first versions ran into (fixed): the AI's stock is spent as it comes - "can't pay a Settlement and a tower"
held it up 5-10 minutes (now the reserve); a transport waiting loaded at the shore while the sea was held was sunk with
everyone aboard in 9 of 10 cases (now unloaded at once); a Town Center as the drop site took the towers' stone; a
transport counted as a guard soldier (`is_soldier`) got a part of its own load goal - the load goal was deleted and the
next part called it (`_purecall`, 2 of 80 games; guards are land soldiers now, and a load goal is only split into parts
while it is the transport's goal); with honest_vision an unscouted enemy home island looked free (Tournament Islands
kills - losses -10, now no island with an enemy town).

**Paired** against the same build without (`expand_islands=0`; it plays the same games as the measurement build, 78
of 80 on Small Islands; Continental e0 the same games as main, 40 of 40). d = with - without, standard error; og =
miners on other islands, mg = all miners, metal = the mod's stone + gold + iron income in 60 min:

| Map | cheat | same games | d og60 | d mg60 | d workers 60 | d soldiers 60 | d metal | d kills - losses | mod / base AI won |
|---|---|---|---|---|---|---|---|---|---|
| Small Islands e5 | no | 5 of 40 | +5.4 (2.0) | +2.7 (3.3) | -4.5 (3.5) | +5.2 (2.0) | +578 (599) | -0.1 (7.3) | 0/2 -> 0/2 |
| Small Islands e5 | yes | 16 of 40 | +0.2 (0.2) | +3.8 (2.4) | -5.6 (3.5) | -0.7 (0.9) | -126 (229) | -3.3 (3.5) | 0/19 -> 0/21 |
| Large Islands e5 | no | 37 of 40 | +0.1 | -0.3 | -0.4 | +0.3 | +77 | +0.7 (0.4) | 1/1 -> 0/1 |
| Large Islands e5 | yes | 39 of 40 | 0 | 0 | -0.1 | 0 | 0 | 0 | 0/9 -> 0/9 |
| Tournament Islands e5 | no / yes | 40 / 40 of 40 | 0 | 0 | 0 | 0 | 0 | 0 | unchanged |

Without the cheat an outpost stood in 24 of 40 Small Islands games (1.2 trips ashore a game, 0.12 lost), 5-6 more
miners on the middle island at 60 minutes - but the home island lost about as many (the citizens shipped off; the
citizen plan trains them again only slowly), so total miners, income and fights moved within the noise. With the cheat
it rarely got going (0.3 trips a game: the army outmatched, the sea held, the base AI on the island by minute 23) and
the base AI's waves took what went (0.25 outposts lost a game). Head-to-head against the control AI (Small Islands
e5, map_size 0, 4 AIs, 60 min, 24 games, `v_expand_islands=1`): B - A per game alive +0.00, soldiers +1.3 (1.4), kills
+2.2 (2.9), losses -5.3 (4.7), razed -0.2 (0.2), workers -1.0 (0.7) (59 plans, 10 trips ashore, 2 outposts lost).
Nothing clearly better: **off** (`expand_islands=0`).

What it would take: the free island must be taken before the base AI's (minute 15-20, i.e. a transport ready before
the plan), the citizens it ships must be additional (more citizens trained for the island's slots, not taken from
wood and food at home), and the maps tested here have one free island at most - a test map with several free islands
(larger maps, more players) would show more. The idle human's island (`4`) is a test artifact, not measured.

## Large island maps (2026-10-21, dll/ai/expand.c, randmap.c)

The playtester: most players play Large or Huge maps, even 1v1, with a high population limit; there are several free
islands, and taking islands towards the enemy is part of the game; overexpanding is dangerous ("a few towers on every
island won't hold"); he takes an island near the enemy and moves the military there, the far home island becomes the
economic hub - but a forward base with open flanks lets the enemy sneak into the undefended heart, so only with sea
control. Every test map so far was Small (`map_size` wrote the wrong setup field; docs/testing.md "Large island maps"):
`map_scale` and `scripts/isl_ablate.sh` now play Small Islands Large (`siL`, 4 free islands), Tournament Islands Huge
(`tiH`, 5), Large Islands Huge (`liH`, 3) at unit limit 1500 (500 a player), 75 minutes, and Small Islands Large 2v2
(`siL2`, 2500), next to the old Small maps `si5` / `ti5` (60 minutes).

**Baseline** (the mod against the base AI at hard, 20 seeds x both seats, mod - vanilla per game; the build before
these changes, `lgbase`):

| Map | cheat | mod / base AI won | soldiers | warships | workers | kills - losses | base AI's miners off home at 60 | mod's |
|---|---|---|---|---|---|---|---|---|
| siL | no | 0 / 2 | -67.5 (11.0) | -7.3 | +54 | -197 | 27.9 | 15.5 |
| tiH | no | 0 / 0 | -31.3 (10.7) | -1.7 | +103 | -140 | 16.2 | 17.6 |
| liH | no | 0 / 0 | -31.1 (9.5) | -6.0 | +109 | -142 | 12.1 | 15.8 |
| siL2 (2v2) | no | 0 / 1 | -94.9 (4.9) | -20.6 | +17 | -198 | 49.0 | 6.9 |
| siL | yes | 0 / 29 | -145.8 (6.9) | -27.7 | -161 | -130 | 69.4 | 0.7 |
| tiH | yes | 0 / 5 | -142.3 (5.8) | -45.1 | -86 | -317 | 57.5 | 5.3 |
| liH | yes | 0 / 12 | -146.0 (5.3) | -47.4 | -93 | -274 | 53.6 | 1.0 |
| siL2 (2v2) | yes | 0 / 35 | -163.7 (4.6) | -32.6 | -191 | -128 | 111.5 | 0.1 |
| si5 | yes | 0 / 19 | -32.9 (2.6) | -3.0 | -43 | -59 | 13.9 | 0.0 |
| ti5 | yes | 0 / 31 | -45.5 (2.5) | -5.6 | -61 | -67 | 15.6 | 0.0 |

Without the cheat the games stay undecided, but the mod's AI has 2-3 times fewer soldiers with more workers: at high
population it trains up to ~300 citizens (citizen_army_share keeps 40% of the cap for the army, the base AI stays near
180-200) and puts 70-150 of them on wood, while its home island's 5-8 mines (30-48 slots) are full by minute ~30. The
base AI takes the free islands from minute 30-40 (22-29 with the cheat) and has 30-50% more metal gatherers by minute
60-75 (siL: 60.6 against 38.6 at 75 minutes). The mod's own buildings off home (from minute ~45) are its landings' forward
bases. It sees little of the map: the base AI's first building off its home island was known to the mod 12-21 minutes
after it went up without the cheat (in 6-9 of 40 games never), 9-25 minutes with it (16-27 of 40 never; `# isl
seen`, the shipped build); the sites' "explored"
flag the old pick read is set for every AI from the start (`# isl ... known mines` 100% at minute 1) - an honest count
(intel.c's looked-at cells) shows 5 of 35 mines known at minute 20 on Small Islands Large, 9 at 30.

**What fails first** (`scripts/isl_timeline.py`, the first of four events per game the mod did not win: the enemy has
a drop site on another island and the mod none, the enemy army 3x the mod's (20+), 5+ enemy soldiers on the mod's home
island, 3+ enemy warships in its waters and fewer of its own; the shipped build `lgA`, 40 games a cell):

| Map | cheat | enemy settled first | army gap | landing on home | sea lost | none | (ever: landing / sea lost, mean minute) |
|---|---|---|---|---|---|---|---|
| siL | no | 25 | 7 | 5 | 2 | 1 | 18 (44) / 22 (56) |
| tiH | no | 25 | 8 | 3 | 1 | 3 | 27 (48) / 12 (62) |
| liH | no | 17 | 7 | 12 | 3 | 1 | 27 (45) / 13 (63) |
| siL2 | no | 20 | 4 | 11 | 5 | 0 | 39 (38) / 32 (54) |
| siL | yes | 14 | 22 | 2 | 2 | 0 | 37 (40) / 38 (41) |
| tiH | yes | 9 | 28 | 0 | 3 | 0 | 36 (42) / 36 (44) |
| liH | yes | 8 | 28 | 2 | 2 | 0 | 39 (40) / 39 (44) |
| siL2 | yes | 1 | 35 | 1 | 3 | 0 | 40 (30) / 40 (26) |

(siL2 from the `lgbase` build.) So without the cheat the islands are lost first: the base AI settles them at minute
30-40 and doubles its metal; with the cheat the army gap comes first (minute 14-25: the half price buys the base AI 3x
the soldiers by minute 20-25, 8 against 24 on siL), then the sea (minute 26-45) and the landings on the mod's home
island (minute 30-43), after which the mod's miners collapse (siL with the cheat: home miners 18 at 30 minutes, 6 at
60). The fleet rarely loses the sea first on the large maps (1-5 of 40 games; Tournament Islands Small with the cheat 14), and the mod sinks more
ships than it loses on the Small maps. The bigger lever with the cheat is the army's size in the first 25 minutes - an
economy question beyond the islands (left for its own step).

**Fixed on the way**: intel.c's looked-at grid covered 150 tiles - on Large / Huge maps (up to ~330 tiles) everything
beyond counted as never looked at and landing.c assumed every beach there held (now 600 tiles). Paired with the
build before (`lgA - lgbase`): the Small maps 157 of 160 games identical, the large ones within the noise both ways
(liH all miners at 60 -5.4 (1.9), tiH with the cheat kills - losses +20.0 (6.7) but base AI wins 5 -> 8).

**The no-cheat fix in the island pick** (step 3): the pick excluded islands with an enemy Town Center whether the AI had
seen it or not (the game's knowledge); now only towns it has seen (honest_vision), and since the mines' explored flag
says nothing, an island with fewer than half its mines looked at (intel.c) is scouted first: the empty transport sails
to its shore water (the sea checked first), and an enemy town, defence or 4+ known buildings there means never again;
an enemy town seen once the ferry is on its way or ashore ends the outpost (unloaded at home). Where islands and mines
are comes from the game's resource zones, as for the base AI.

**New bits** (`expand_islands`, header of dll/ai/expand.c; all off): 8 EARLY (the plan from minute 8 without waiting for
the home mines, from 20 workers; its transport before every reserve and retried every 2 s - the 250 wood are spent on
buildings within seconds of coming in: transports waited 6 minutes at minute 12-20; 12 minutes before giving up),
16 EXTRA (citizen_plan's goal + the outpost's citizens until its Settlement stands; wood cutters board first),
32 MORE (the next island once the last holds - Settlement and towers standing, no enemy there for 3 minutes, 3+ citizens,
its trips made or slots filled -, at most `expand_max` (3), the nearest to what it holds; a held island with nothing
left is lost), 64 FORWARD (an outpost nearer the enemy's known buildings than home gets a barracks and two more
towers while `sea_held`: no enemy warship seen in its waters for 5 minutes, its warships at least the enemy's known;
home's towers, guard and militia untouched; only the outpost's own guard is held there, the rest is the game's army).

**Measured** (paired with `lgA` = the same build, `expand_islands=0`; 40 games a cell; d = with - without, standard
error; og = miners on other islands, mg = all miners, foe = the most enemy soldiers seen on the home island at a
5-minute sample; the B logs were rerun with the worktree's personality files after a format change on main):

| Map | cheat | bits | trips ashore | og30 | og60 | mg60 | foe | kills - losses | mod / base AI won |
|---|---|---|---|---|---|---|---|---|---|
| siL | no | 57 | 1.82 | +2.1 (0.6) | -4.5 (3.0) | -3.7 (4.0) | -3.4 (2.3) | -5.7 (12.9) | 0/2 -> 0/0 |
| siL | no | 121 | 1.73 | +2.1 (0.6) | -1.0 (3.2) | -0.9 (4.1) | -2.4 (2.0) | -6.9 (11.4) | 0/2 -> 0/1 |
| tiH | no | 57 | 2.52 | +2.5 (0.6) | +1.9 (3.2) | +5.5 (4.7) | -5.4 (1.6) | +1.4 (15.9) | 0/0 -> 0/0 |
| tiH | no | 121 | 2.67 | +2.3 (0.6) | +3.6 (3.0) | +4.7 (4.4) | -4.5 (1.6) | +8.3 (13.3) | 0/0 -> 0/0 |
| liH | no | 57 | 2.83 | +2.6 (0.7) | -2.2 (3.2) | -1.5 (5.1) | -4.9 (2.6) | +37.1 (13.8) | 0/0 -> 0/0 |
| liH | no | 121 | 2.85 | +2.6 (0.7) | -3.5 (3.1) | -3.6 (4.8) | -5.3 (3.0) | +34.8 (12.8) | 0/0 -> 0/0 |
| si5 | no | 57 | 1.68 | +2.8 (0.7) | +7.3 (2.5) | +2.4 (3.8) | -1.3 (1.0) | -5.2 (6.8) | 0/2 -> 0/2 |
| si5 | no | 121 | 1.75 | +2.7 (0.7) | +4.8 (1.9) | +0.1 (3.2) | -0.8 (1.0) | -10.6 (6.6) | 0/2 -> 0/2 |
| siL | yes | 57 | 0.50 | +0.8 (0.3) | +0.2 (0.5) | +1.6 (2.1) | +0.8 (3.7) | -9.6 (8.5) | 0/29 -> 0/22 |
| siL | yes | 121 | 0.55 | +0.8 (0.3) | -0.3 (0.3) | +1.4 (2.2) | -2.4 (3.0) | -7.2 (9.2) | 0/29 -> 0/21 |
| tiH | yes | 57 | 0.72 | +1.5 (0.6) | -1.8 (1.3) | -2.5 (3.7) | -2.6 (3.0) | -9.6 (13.3) | 0/8 -> 0/8 |
| tiH | yes | 121 | 0.75 | +1.6 (0.7) | +1.2 (1.8) | +0.9 (2.8) | -4.6 (3.2) | -8.1 (11.9) | 0/8 -> 0/9 |
| liH | yes | 57 | 0.72 | +1.3 (0.5) | -0.2 (0.9) | -5.1 (3.2) | -3.7 (3.4) | +12.2 (10.8) | 0/11 -> 0/14 |
| liH | yes | 121 | 0.75 | +1.3 (0.5) | -0.2 (0.9) | -6.6 (3.7) | -0.5 (3.3) | +19.9 (12.6) | 0/11 -> 0/15 |
| si5 | yes | 57 | 0.47 | +0.5 (0.4) | +1.3 (0.7) | +7.9 (2.4) | -0.2 (1.5) | +4.6 (5.8) | 0/19 -> 0/16 |
| si5 | yes | 121 | 0.50 | +0.4 (0.4) | +0.7 (0.3) | +5.9 (2.2) | +0.1 (1.6) | +4.7 (6.1) | 0/19 -> 0/16 |

Tournament Islands Small (`ti5`, no free island): 38-40 of 40 games the same. The outposts now go up from minute 25-30
(2-3 per 75-minute game without the cheat, 0.5-0.75 with it), 2-3 miners more on other islands at minute 30, and the
enemy's soldiers on the home island fall on the large maps (foe -2.4 to -5.4; the samples with any foe -0.5 to -1.0
of ~2) - the outposts meet the base AI's landings away from home. But they do not hold: by minute 60 the other islands'
miners are back where they were (og60 -4.5 to +3.6, all within the noise on the large maps), the outposts lost or
emptied (0.2-0.4 losses a game; the base AI settles the same islands and its 2-3x larger army takes them), and the
results go both ways: Large Islands Huge kills - losses +35 (13) without the cheat, but with the cheat the base AI's
wins 11 -> 14 / 15; Small Islands Large with the cheat 29 -> 21 / 22 of 40, Small Islands Small 19 -> 16. FORWARD (121
against 57) changed little: the forward base came where the base AI was already settling (the "nearer the enemy"
test found its outpost 8-82 tiles from the enemy's buildings), and `sea_held` passed on the mod's own warships because
the enemy's had rarely been seen. 
Head-to-head against the control AI (Small Islands Large, 4 AIs, unit limit 2500, 75 minutes, 24 games, `v_expand_islands`, B - A per game): 57 soldiers +9.4 (5.4), kills +5.8 (14.7), losses -9.1 (9.9), razed -3.5 (2.6), workers +1.6 (6.5); 121 soldiers +1.8 (7.9), kills +0.3 (14.8), losses -2.1 (11.7), razed -3.0 (2.9), workers +5.9 (9.6) - nothing beyond the noise, the razed count leaning against it.

No clear gain without losses elsewhere: **off** (`expand_islands=0`). The build with it off against main's build without
these bits: Continental e0 39 of 40 games the same, Small Islands Large 1 of 4 seeds checked identical, and a rerun of
one that parted (minute 70) identical to main's - with 8 games at a time, 3 of 4 Small Islands Large games parted after
minute 56-70 also between two runs of the same DLL (the heavy load, not the code; a run with 2 at a time was exact).

**What the diagnosis points to next** (not done here): (1) the army's size at high population - the mod's AI keeps up
to 60% of the cap in citizens and over-staffs wood (70-150 woodcutters, 5000-8000 wood unspent at 60 minutes) while the
base AI fields 2-3x the soldiers with fewer workers; with the cheat this gap at minute 15-25 decides the game; (2) the
outposts must hold: the island where the base AI also lands needs the army (and the forward base the sea) first, i.e.
an outpost should follow the army, not lead it; (3) map control: the mod sees the enemy's new islands 10-25 minutes
late or never - scouting with the fleet and hitting a fresh enemy outpost early (a small landing on a Settlement
without towers) are open levers; (4) fishing: the mod's AI has no fishing boat before minute ~25 on these maps (the
base AI 3-4 at minute 10), though fish is the cheapest food on islands. For land maps the same forward-base idea needs
the home guard and flank checks without the sea test (expand.c's `sea_held` / `ex_forward` are island-only; the
holding test, `ex_holds`, and the forward test against the enemy's known buildings are not).

## Workforce into army (2026-10-21, dll/ai/citizens.c workforce_plan)

The playtester: citizens are needed to fill every mine, a lack of them should not be the limit - nor should an
oversized, wood-heavy workforce starve the army; workers on wood beyond what is spent are waste.

**Measured** (the shipped build `lgA` against the base AI at hard, unit limit 1500 = cap 505 each, 75 min, 40 games a
cell; `scripts/workforce.py`, the `# wf:` line). At minute 60 without the cheat (siL / tiH / liH), mod against base AI:

| | mod | base AI |
|---|---|---|
| citizens | 170-227 | 122 |
| idle citizens | 31-63 | 3-14 |
| on wood | 68-107 | 61-68 |
| wood stock | 8400-14900 | 3400-7600 |
| soldiers (all kinds) | 35-62 | 70-81 |
| soldiers trained by 60 | 116-148 | 238-279 |
| land production buildings | 3.5-4.5 | ~10 |
| gold + iron stock | ~200 all game | 700-1500 |

Wood came in at 22000-29000 over minutes 30-60 and 14000-17000 was spent; from minute 20 the army's orders were refused
10000-12000 times, always for gold and iron.

**Why so many citizens:** citizen_plan's goal counts the citizens already there, the woodcutters and the idle
included. So whoever is trained and finds no mine raises the goal, up to the cap less citizen_army_share (303 of 505).
On top of that, the game's own 45% of the cap (227) is a floor. **Why so many on wood:** LATE WOOD's target closes the
game's wood "demand" (refused requests stay on its books, ~80000), and the 2000 it counts at most ask for 40% of the
workers. The island's mines near drop sites were full (the free slots that remain are on other islands, out of a
walking citizen's reach) and its forest was cut back, so the extra citizens stood idle (up to 150-200 in single games)
or cut wood nobody spent.

**Why so few soldiers:** every soldier needs gold or iron (Middle Ages: Long Sword / Pikeman 30 iron, Long Bow 30
gold, Knight 60 gold, warships 125-250; data/db/dbtechtree.dat). The gold and iron are spent the moment they come in.
On the islands the navy account holds part of them. Techs took little: the researched techs' gold + iron from the
start epoch on is 1000-1500 a game (`# wf ... rsch`). The game's requests are for gold-heavy units (Knights, Cavalry
Archers, Cuirassiers), and the budget saves 90 s for them (substitute_wait) while the gold goes elsewhere. So the
idle citizens and the wood are a symptom: on island maps the army is bounded by the metal the home island gives;
more needs the other islands (expand.c, transports).

**`workforce_plan`** ([all], bits; `v_workforce_plan`; **default 0**):
- 1 CAP: from the Copper Age, the citizens wanted (the game's share and citizen_plan's goal alike) are capped at what
  the economy uses: the mines' slots near drop sites (or the miners, if more), the food gatherers (at least the
  fields + 2 and FEED's), the woodcutters the wood actually spent needs (5-minute spending at the measured rate per
  worker, + 25% and 3), the outpost's, 4 builders and a margin of cap / 40.
- 2 WOOD: while the wood stock holds 3 minutes of spending + 1000, wood's target is at most that need. The rest goes
  to the metals up to their slots near drop sites, then to food up to its fields + 2.
- 4 ARMY: army_plan's FILL and BUILD on every map (islands too) at a cap of `workforce_army_cap` (250) and above.
- 8 THRIFT: while food or wood lie at 800+, a soldier refused for gold or iron is replaced at once (no saving) by the
  one with the most hit points x attack per gold + iron the budget pays.

**Paired results** (the same build with 0; d = with - without, standard error; screening 20 games a cell (`wB`, bits
1 / 3 / 8 / 15), then 40 games a cell for 4 and 12 (`wC`, seeds 1-20 x both seats)):

| Bits | Map | cheat | d citizens 75 | d soldiers 60 | d soldiers 75 | d kills - losses | mod / base AI won |
|---|---|---|---|---|---|---|---|
| 1 CAP | siL / liH | no | -30 (12) / -43 (12) | -5 (4) / 0 (5) | | +6 (18) / +20 (14) | unchanged |
| 1 CAP | siL / liH | yes | -3 (10) / -32 (11) | | | -12 (5) / +7 (9) | 15 -> 15, 6 -> 8 |
| 3 CAP+WOOD | siL | yes | | | | -11 (4) | |
| 8 THRIFT | cont | no / yes | | +4.1 (1.8) / +3.5 (1.5) | | +3.8 (2.1) / +1.6 (2.3) | 4/0 -> 3/0 |
| 8 THRIFT | siL | no | | -4 (4) | | -37 (16) | |
| 4 ARMY | coL / liH / tiH / siL | no | -3 .. +11 | +14 (7) / +10 (5) / +12 (6) / +3 (4) | +19 (8) / +16 (7) / +14 (7) / +1 (7) | -1 (10) / +11 (13) / +14 (19) / -9 (15) | coL 9/2 -> 11/4 |
| 4 ARMY | coL / liH / tiH / siL | yes | | -1 / -1 / +4 / -1 | | +2 (8) / -1 (14) / -11 (16) / -10 (10) | 37 -> 37, 11 -> 12, 7 -> 5, 29 -> 24 |
| 12 ARMY+THRIFT | coL / liH / tiH / siL | no | | +16 (7) / +9 (5) / +6 (6) / -3 (4) | +23 (7) / +18 (7) / +8 (8) / +2 (7) | -1 (13) / +14 (13) / +6 (18) / -18 (15) | coL 9/1 -> 11/5 |
| 12 ARMY+THRIFT | cont / si5 | no | | +3.1 (1.3) / +1.8 (1.6) | | +1.1 (1.3) / -1.1 (6.3) | cont 12/1 -> 9/1 |
| 12 ARMY+THRIFT | coL / liH / tiH / siL | yes | | -1 / -1 / +3 / -1 | | +3 (8) / -13 (12) / -13 (15) / -19 (12) | 37 -> 38, 12 -> 9, 7 -> 6, 27 -> 23 |

CAP does what it says: 30-60 fewer citizens on the large maps, and wood near its need. But the soldiers do not rise,
because neither population room (180-250 free) nor food is what limits them. ARMY puts 10-20 more soldiers on the
large maps without the cheat. With the cheat it does nothing, and against the base AI with the cheat it trained
20 (8) fewer on Continental Large. THRIFT adds 3-4 soldiers on small Continental but lost on Small Islands Large
(-37 (16)). Kills - losses stay within the noise or go against it, and the mod's wins on Continental e0 went
12 -> 9 of 40 with 12. No clear gain without a loss elsewhere: **off**. Not measured: head-to-head against the control
AI (normal and lowpop), Small Islands Large 2v2.

**Fixed on the way:** an engine crash that ARMY set off in one Tournament Islands Huge game, 3 of 3 runs at minute 31:
0x5ED24B dynamic_casts a unit's goal object (+0x1F0) for goal 0xFBA and calls 0x60FE7E on a NULL result (read at
+0x38). attack.c `hook_goal_cast` answers "no" for a NULL goal (`# engine guard: a unit's goal 0xFBA without its
goal object`). The guarded game played to its end (a citizen, type 32).

**Leads:** the metal. On islands that means other islands' mines (transports, outposts that hold). Everywhere it
means what the gold and iron buy (the game's gold-heavy requests, the 90-s saving, the navy account's share), and
production buildings when the army has the metal.

## Gold and iron (2026-10-21, dll/ai/metal.c)

The playtester: convert the economy into attack and defence; early food is for citizens, units that need no food are a
good alternative; the AI never cheats. "Workforce into army" found the army on the large maps limited by gold and iron.

**Measured** (`# metal:` every 5 minutes, both sides: income since the start, every payment by what it bought - from the
tech paid, so units, research and buildings apart -, the army alive by line, the enemy soldiers the mod knows by line,
the time the budget saved for a refused unit, the gold + iron in stock and in the navy account; `scripts/metal.py`).
The build without changes (`mtB0`) against the base AI at hard, 40 games a cell, gold + iron by minute 60, mod / base AI:

| Map | cheat | G+I income | share on warships | land soldiers' share | saving (G+I in stock then) | G+I stock | soldiers 60 |
|---|---|---|---|---|---|---|---|
| siL | no | 9100 / 14600 | 65% / 52% | 33% / 46% | 62% of the time (227) | 234 / 629 | 35 / 81 |
| tiH | no | 11200 / 12800 | 67% / 53% | 31% / 46% | 65% (218) | 226 / 517 | 48 / 72 |
| liH | no | 12400 / 14700 | 65% / 55% | 33% / 43% | 69% (213) | 220 / 815 | 62 / 79 |
| coL | no | 13000 / 8500 | 9% / 17% | 87% / 81% | 45% (956) | 1012 / 703 | 44 / 41 |
| cont | no | 5500 / 3000 | 9% / 4% | 85% / 94% | 31% (885) | 931 / 950 | 40 / 4 |
| si5 | no | 7200 / 10600 | 59% / 52% | 38% / 47% | 54% (283) | 300 / - | 22 / - |
| siL | yes | 4700 / 18600 | 61% / 51% | 34% / 50% | 41% (282) | 281 / - | 3 / - |

Research took 2-5% of the gold + iron (line upgrades ~1%, the Middle Ages start has no epoch left to pay much), towers
and walls none (stone). On the island maps two thirds of it went into warships: the fleet's target (navy.c stage 4,
`navy_fleet_pop` 15% of the population cap) is 76 warships at a cap of 505 - never reached, so the navy's share of the
income stayed at its most (50%) all game, and still the mod had no more warships afloat at 60 minutes (13-19) than the
base AI (15-18): they sank. The land army got a third of a smaller income (the base AI settles the free islands: 1.2-1.6x
the mod's gold + iron). The budget was saving for a refused unit 60-70% of the time with ~220 gold + iron in stock - the
saving holds little, the metal is simply not there. On the land maps the metal is not the limit: 900-1000 lay in stock.
Also seen: in one Small Islands Large game a medieval tower was laid and given up 127 times in 13 minutes (each time 175
stone paid and refunded - `# place: ... 463 130/3/1`), not followed further.

`metal_plan` ([all], bits, `v_metal_plan`; only from a population cap of `metal_cap` (250); **default 0**):
- 1 VALUE: a land soldier the game asks for is trained as the one that does most per gold + iron against the enemy army
  it knows (intel.c: soldiers seen within 10 minutes; none known: its own types as the mix) when that is 25% better:
  the enemies one kills in its life (stats.c damage - the game's class table and hit factors, armour, upgrades - against
  each enemy type's hit points, times its own hit points over the mix's damage at it; ranged behind a melee mix take
  60%), a little less for a type already half the army, over gold + iron + a quarter of food and wood + half the stone.
  The substitute (unit_substitution) is picked the same way.
- 2 NOWAIT: no saving (substitute_wait) for a unit whose missing gold or iron won't come within the wait.
- 4 SPARE: VALUE's price counts food, wood and stone lying 1500+ unused at 5%, short ones fully.
- 8 FLEET: the fleet's target at most `metal_fleet_pct` (250%) of the enemy warships it knows + 6, at least
  `metal_fleet_min` (20) x the map strength.
- 16 CALM: the sea calm for 3 minutes (no enemy warship in its waters for 5, its warships at least the enemy's known):
  the navy account keeps 100 gold and 100 iron at most.

**Paired** with `mtB0` (same seeds and seats, d = with - without, standard error; soldiers = land + siege + ships;
trained = land soldiers; G+I = income by 60; refused = the army's orders refused short of gold / iron from minute 20).
Screening (20 games a cell, siL liH coL cont): VALUE, NOWAIT and CALM alone within the noise (VALUE: Continental Large
mod wins 5 -> 10 of 20 without the cheat, but kills -73 (20); Continental e0 3 -> 1); FLEET (150%, 12) the only one that
moved the economy. Then 40 games a cell (before `metal_cap`; the small maps' cells are identical with it):

| Variant | Map | cheat | d kills - losses | d soldiers 60 | d soldiers 75 | d trained | d G+I | d warships | mod / base AI won |
|---|---|---|---|---|---|---|---|---|---|
| 27 (1+2+8+16) | coL | no | +10.7 (11.5) | +8.4 (8.0) | +14.8 (8.3) | -22 (27) | +176 | -0.8 | 9/2 -> 15/3 |
| 27 | cont | no | -2.1 (3.6) | -1.0 (2.1) | -1.0 (2.1) | +20 (5) | +112 | -0.3 | **11/1 -> 4/1** |
| 27 | liH | no | +36.7 (14.0) | +37.9 (6.2) | +40.6 (8.1) | +101 (12) | +1434 | -5.4 | 0/0 |
| 27 | siL | no | +6.5 (15.2) | +14.5 (4.7) | +25.7 (8.0) | +82 (16) | +1385 | -6.1 | 0/2 -> 0/1 |
| 27 | tiH | no | -13.6 (15.0) | +21.3 (5.4) | +18.1 (5.4) | +83 (11) | +1640 | -11.7 | 0/0 |
| 27 | si5 | no | **-26.4 (8.0)** | -1.9 (3.2) | | -3 (6) | -1187 | -3.1 | 0/2 -> 0/3 |
| 27 | siL / liH / tiH | yes | -23.0 (12.7) / -3.6 (15.2) / **-51.6 (19.8)** | +2.8 / +7.2 / +3.8 | | +34 / +50 / +51 | +657 / +336 / +1034 | | 29 -> 20, 12 -> 11, 7 -> 8 |
| 27 | coL / cont / si5 | yes | +8.1 / +4.6 / +3.0 | | | | | | 37 -> 36, 3 -> 3, 19 -> 20 |
| 24 (8+16) | liH / siL / tiH | no | +24.3 (14.2) / +16.8 (15.5) / -18.1 (14.5) | +10.4 / +7.4 / +9.2 | +16.8 / +18.2 / +7.0 | +87 / +66 / +70 | +1676 / +1030 / +1196 | -6.8 / -5.5 / -11.6 | 0/0, 0/2 -> 0/0, 0/0 |
| 24 | si5 | no | -11.4 (8.2) | +2.9 | | +12 | -117 | -1.6 | 0/2 -> 0/1 |
| 24 | liH / siL / tiH | yes | +6.0 (9.5) / -13.0 (9.8) / **-48.6 (14.2)** | +6.0 / +2.4 / +1.2 | | +56 / +31 / +49 | +1117 / +661 / +1030 | | 12 -> 8, 29 -> 24, 7 -> 6 |
| 24 (FLEET 250%, 20) | liH / siL / tiH | no | +23.0 (10.7) / +7.3 (15.1) / +5.8 (14.0) | +7.8 / +7.5 / +7.6 | +18.4 / +17.9 / +12.5 | +62 / +65 / +58 | +950 / +1128 / +1247 | -3.8 / -3.7 / -10.9 | 0/0, 0/2 -> 0/1, 0/0 |
| 24 (250%, 20) | liH / siL / tiH | yes | -3.3 (9.6) / **-26.6 (13.6)** / -19.3 (16.9) | +2.1 / +2.2 / +1.5 | +4.1 / +4.7 / +1.6 | +36 / +33 / +26 | +378 / +969 / +124 | | 12 -> 7, 29 -> 20, 7 -> 8 |

What it does: with FLEET the warships' share of the gold + iron falls from 65% to ~50% and the land army gets it - 60-100
more soldiers trained, 7-40 more standing at 60-75 minutes on the large maps; the G+I income itself rose by 1000-1700
(why was not followed further). Without
the cheat that is mostly kills - losses up (liH +23 to +37). With the cheat the base AI won fewer games on Small and
Large Islands (29 -> 20, 12 -> 7) but the mod's kills - losses fell (Small Islands Large -27 (14), Tournament Islands
Huge -19 to -52): its larger army fights - and loses - more. VALUE (in 27) turned Continental e0 from 11 to 4 wins: on
small maps the game's own choice is better (hence `metal_cap`). Head-to-head against the control AI (Small Islands Large,
4 AIs, unit limit 2500, 75 minutes, 24 games, `v_metal_plan=24` with 250% / 20; `metal.py -h2h`): B - A per game soldiers
60 +10.4 (4.9), land soldiers trained +71 (13), gold + iron +1380 (580), razed +6.4 (2.9), buildings lost -5.2 (3.3),
kills +27 (13), losses +22 (12), kills - losses +5 (21), but warships -7.1 (1.6) and ships lost +8.5 (2.7) - the sea
weaker. Head-to-head on Continental (normal: map_size 1, 4 AIs; lowpop; 8 games each, with `v_expand_islands=185` too):
the variant never acted (below `metal_cap`, no water: 0 picks, no outpost) - B - A only the seats' noise (kills - losses
-22 (18) / +13 (8)). No clear gain without a loss elsewhere: **off**.
The next lever is the metal itself: other islands (below) and the warships that sink.
