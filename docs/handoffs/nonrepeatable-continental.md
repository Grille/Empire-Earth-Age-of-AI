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

## Why it matters

Every "with the option off the game is the same" proof and every same-seed comparison on a continental map is
unreliable until this is understood. Head-to-head measurements are not affected: they average over seeds.
