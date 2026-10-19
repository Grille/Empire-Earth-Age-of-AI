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
