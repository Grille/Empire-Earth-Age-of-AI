# The early game against the base game's AI

The benchmark (docs/testing.md "Against the base-game AI"): the base game's AI at hard with its Cheat (every price
50%). The mod's AI won 0 of 158 games. The economy log (`# eco:`, `# death:`, dll/ecolog.c) and the ablations
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

## The features (dll/early.c, econ.c, defence.c; `[all]` keys)

| Key | What it does |
|---|---|
| `early_towers` (A) | In epochs 0-1 the town's first 2 towers stand by the Town Center, 4 tiles out: one towards the front, one on the side where the citizens work. The game's own tower requests take these spots first, and the mod queues the missing ones while stone allows. Every unfinished tower within 20 tiles of the base gets 2 builders (none when enemy soldiers are within 8). The worker plan asks for their stone. |
| `tower_bait` (A) | Up to epoch 3: a citizen with an enemy soldier within 5 tiles (or one attacking it), and outside its towers' reach, walks to 1.5 tiles behind its nearest finished tower or other shooting building within 25 tiles. Raiders that follow come under fire. The citizen goes back to work through the normal assignment (raided sites are refused). |
| `early_militia` (A) | Citizens fight soldiers only when own soldiers are within 12 tiles or an own tower reaches the spot, and only when the estimate (dll/stats.c) says they win before losing one citizen. |
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

## Food and mines (2026-10-20, dll/food.c, dll/mines.c)

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

## Citizen production (2026-10-20, dll/citizens.c)

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
games; dll/attack.c range sort guard, 29914e2).

## Economy into army (2026-10-20, dll/army.c)

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

## The army as one (2026-10-20, dll/mass.c)

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

## Other islands (2026-10-20, dll/expand.c)

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

`expand_islands` ([all], bits, `v_expand_islands` for head-to-head; header of dll/expand.c): 1 SETTLE - from the Copper
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
