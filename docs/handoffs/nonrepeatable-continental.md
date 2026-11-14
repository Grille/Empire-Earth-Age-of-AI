# Handoff: a same-seed game that is not repeatable (Continental huge, from epoch 5)

Found 2026-11-14 while checking a change with "option off = the same game". Not looked into further. Nobody is
working on it.

## What happens

The same build, the same ini and the same `game_seed` should play the same game, step for step (docs/testing.md
`game_seed`; CLAUDE.md "play the same game"). For this setup they do not: three runs of one unchanged build
(HEAD ac6632bd) gave three different games.

| Compared | First data row that differs (heap addresses masked) |
|---|---|
| run 1 and run 2 | minute 31.1, player 5: wood stock 27 against 31, and its demands |
| run 1 and run 3 | minute 19.1, player 4: stone stock 15 against 30 |
| run 2 and run 3 | minute 19.1, the same row |

Before that every data row is identical, so the games start the same and part ways somewhere between minute 10 and
19 (the rows are periodic; the real first difference is earlier than the row that shows it). By minute 25 the runs
found towns at the same spot with a different count of mines seen there, and they end with 119, 134 and 175 death
and town lines.

## The setup

```
ai_count=4,random_map=Continental,map_scale=huge,start_epoch=5,difficulty=hard,headless=1,turbo=1,quit_minutes=45,game_seed=2
```

Two games were started at the same time each time (two slots), all with the shipped config.

## What is known to be repeatable

`random_map=Small Islands,map_scale=large,start_epoch=9,game_seed=6` (otherwise the same keys, 40 minutes) gave
identical death and outpost lines on three pairs of runs the same day. Only those lines were compared there, not the
data rows - worth redoing with the row comparison before trusting it fully.

Later the same day the island test drifted too, in a telling way. It has exactly two games: the usual one (157 death
and outpost lines) and a second one (142 lines, apart from minute 19.8 on: one AI's transport sails 6 seconds later).
Four pairs were played, two games started together each time (the first in slot 1, the second in slot 2):

| Pair | First game | Second game |
|---|---|---|
| 1 | unchanged build: usual | a build with read-only changes: second |
| 2 | unchanged build: usual | unchanged build: second |
| 3 | unchanged build: usual | the changed build: usual |
| 4 | unchanged build: second | the changed build: second |

The second game is byte-identical every time it appears, whichever build plays it. So this is not a random race
with many outcomes; something with two states decides, and it is not the build (pair 2). In pairs 3 and 4 both games
of a pair agreed, in pairs 1 and 2 they did not.

Two more pairs an hour later (another read-only change): second / usual, then usual / second - so which game is
played does not go by the slot or by the pair either.

## How to redo it

- `NEW=<a dll> sh local/scratch/pt_same.sh` plays the setup above once on `local/work/tvbase.dll` and once on `NEW`
  (build both from HEAD with `sh local/scratch/mybuild.sh tvbase`, then `NEW=local/work/tvbase.dll`).
- `local/venv/Scripts/python.exe local/scratch/first_diff2.py <log a> <log b>` prints the first data row that differs
  and which columns. The logs are `local/work/autotest_*_ptsame_*.csv.gz` (pruned after 7 days).
- A line-by-line diff of the whole log is useless: the `#` lines of different threads interleave differently from run
  to run. Compare the data rows, or one kind of `#` line at a time.

## Not checked - where I would look first

- Whether it needs two games at once, turbo, the huge map, or epoch 5: run one game alone, then vary one key.
- Whether it is new: play the same pair on a build from a week ago.
- Simulation code that reads the real-time clock. These files of the mod call QueryPerformanceCounter, GetTickCount
  or timeGetTime and are not pure drawing: `dll/engine/shiproute.c`, `dll/engine/walls.c`, `dll/engine/collect.c`,
  `dll/game/reachmap.c`, `dll/ai/stats.c`. A work budget counted in real time that changes a route or an order would
  give exactly this picture: identical for minutes, then a small difference in one player's stock.
- The first differing quantity is a few units of one resource for one AI, which points at a citizen's walk or drop
  being a step early or late, not at a different decision.

## Findings, 2026-11-14 (second session)

Scripts in `local/scratch/nonrepeat/` (git-ignored): `nr.sh <tag> <n> [,keys]` n same-seed games at once (35 min) on
`local/work/nr_head.dll` (HEAD add34873, a clean build); `nrt2.sh` the same for 1 minute with the slot's `synctrace.txt`
kept; `nrw.sh` a game at the normal pace with a hardware watch on the synchronised generator's seed (EEPhysics +0x18)
from 0.3 s to 17 s; `seqcmp.py` / `dumpcmp.py` compare those.

- **Reproduced, and not the conditions the note suspected.** The 35-minute game parts in about one game of four or
  five: 4 at once (1 parted, 30.6 min), 8 at once (1 at 29.1 min; the others at 19.1 min, player 4's stock), 8 with
  `draw_during_steps=0` (the same branches), 6 one after another with nothing else of mine running (4 parted). Not two
  games at once, not the picture drawn during the steps, not turbo (normal pace parts the same way: 1 of 6).
- **The branches are few and the same each time**: a few distinct first differences (19.1 min stone 30 / 15; 27.0,
  29.1, 30.6 min a list count), not noise.
- **The mod's AI is not needed.** `vanilla_players=2345` (the game's own AI for all four, the mod's engine changes on)
  parts at 0.7 min (player 5's demands), with `improved_unit_movement=disabled` as well. `enabled=0` can't be compared:
  it switches the test harness off too, no rows.
- **The first difference** (vanilla, `sync_trace`): at 14.86 s of game time the synchronised random generator's seed
  differs (1768322413 / 1533751932, two outcomes only) while the stock and the units' hash are the same; at 16.2 s the
  seeds are the same again and one unit differs: P3's citizen (type 480) at 130,69 / 131,69. So the draws were not
  more or fewer - they came in another order: one unit did something a step earlier or later, or on another tile.
- **Who draws** (the watch, 262 writes in 4 s, all on the simulation thread): the generator's function in
  Low-Level Engine.dll (via [0x8233B0]); callers the physics step itself (0x546CE9, 0x547C09, once a step each), citizens'
  behaviour (0x5531A8 random range <- EEUCBasicCitizen::vf39 0x5FFB75 <- 0x618CCB <- EEUnitAction::ask 0x619FF5, the bulk)
  and EEUnitClass::vf47 0x62462B <- EEUCLand::vf45. The mod draws nothing (reload=game; no rand() in the mod). Nine
  watched games at the normal pace all took the common branch - chance (~13%) or the watch's exceptions change the
  timing; not settled.
- **Ruled out by reading**: the clock reads in the mod's simulation files (measurements, the scenario editor's own
  timers; route_budget counts tiles); the game's own clock calls (none in its AI, path or unit code); the path manager's
  queue of unfinished searches (EEPathManager 0x917D00: request 0x4A1AFF with sync=0 gives a search 15 iterations, the
  rest goes onto a list worked off by 0x4A3F1A - 32 iterations a step, counted in work, and game time - deterministic).

Where I would look next: something whose result depends on **heap addresses or uninitialised memory** - both differ
from run to run (other threads allocate from the same heap). The citizen a tile apart fits a path search breaking a
tie between equal routes by a node's address (the search: vtable 0x827C68, its run slot 8 = 0x4A4FE1, the open list
behind +0x6C). A check that would settle it without reading the search: two games whose heap layout is made the same
(e.g. no other thread allocating - not found how yet), or a pointer-free tie-break hooked in and the branches gone.
Also seen: player stock slot 1 (not a resource) holds 0 or 0x827820 (an address in the game's data) in different runs
at 0.1 min - memory never set, logged, not used; harmless, but a sign the game reads memory it hasn't set.

## Why it matters

Every "with the option off the game is the same" proof and every same-seed comparison is
unreliable until this is understood - on the island test a pair that differs has to be played again and each game
named as the usual or the second one. Head-to-head measurements are not affected: they average over seeds.
