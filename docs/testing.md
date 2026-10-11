# Testing

The tooling runs AI-vs-AI games unattended and lets you inspect the running game.

## Autotest mode

The DLL looks for `aimod_autotest.ini` in the game folder. `scripts/run_test.ps1` copies
[`autotest.ini`](../autotest.ini) there for the duration of a run only, so normal play is never affected.
The `[autotest]` keys:

| Key | Meaning |
|---|---|
| `skip_intro=1` | skips the intro movies |
| `quit_minutes=75` | ends the game after this many game minutes; a decided game (victory/defeat screen) ends the run at once and logs `# autotest: game over at <min>: P1=1 P2=1 P3=2` (1 won, 2 lost) |
| `start_epoch=N` | (random maps) starting epoch, e.g. 2 = Copper Age, for tests that should begin later in the game |
| `speed=7.5` | game speed (normal 2.5). The simulation runs a fixed number of steps per real second (the lockstep server grants 15 steps every 350 ms); the speed only sets the game time per step, `1000 * speed / 44` ms (57 ms at 2.5, 170 ms at 7.5). Higher speeds make the steps coarser (movement artifacts). |
| `turbo=1` | the steps follow each other as fast as the CPU allows instead of 43 per second: the server sends the next tick as soon as the simulation has used up the last one, the simulation loop skips its 22 ms timer, and a headless window renders only a few frames a second. Same steps, same game: a 75-minute game takes ~15 s instead of ~10 min. Pipe: `turbo 0\|1` |
| `random_map=Continental` | starts a random map instead of the save. Also: `map_size` (0 tiny..), `ai_count`, `seed`, `teams=ffa\|pairs`, `start_epoch`, `citizens`, `unit_limit` (500, the game's, shared by all players incl. the idle human), `ai_cheat`, `ai_ignore`, `epoch_min`/`epoch_max` (test epoch timer). The human player (slot 0) idles, sees the whole map and is allied with every AI. |
| `game_seed=N` | the game's random seed at game start (otherwise the clock: `srand(TMGetTime())`, then `rand()` goes into the start message). Removes one source of run-to-run differences on random maps, not all yet (the AI still diverges after ~10 s) |
| `setup=<file>` | control-pipe commands (one per line, `#` comments) run once at 2 s game time; `@<min> <command>` waits until that game minute, path relative to the game folder - e.g. `spawn` lines that turn a map into a test scenario (see Scenarios) |
| `scenario_mode=1` | starts `Data\Scenarios\<save>.scn` through the real scenario path (set by `-Scenario`) |
| any `aimod.ini` key | overrides that setting for all difficulties, e.g. `avoid_towers=0` |
| `watch=<hex>`, `watch_len` | hardware write-watchpoint, logs every writer of an address |
| `trace_player`, `trace_until_min` | logs the economy calls of one AI |

Scenarios and saves: same speed, same settings - the run is byte-identical, with or without turbo, so
A/B tests are exact. Random maps are not: the map is fixed by `seed`, but two runs of the same map
still differ from the first minutes on (some random generator is seeded from the clock). Compare
random-map variants by averages over several runs (cheap with turbo), not by single runs.

## Running

```powershell
# one game; -Save names a file in Data\Saved Games (newgame.ees is only the start template for random maps)
powershell -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees -TimeoutSec 2400 -Tag _x `
    -IniExtra "random_map=Continental,map_size=0,ai_count=3,seed=101,teams=ffa,ai_cheat=0,ai_ignore=1"
```

- Sound and music are muted for the run and restored afterwards (a backup in `work/` covers interrupted runs).
- The log is moved to `work/autotest_<stamp><tag>.csv`.
- `-Scenario <name>` plays `Data\Scenarios\<name>.scn` instead.
- `-IniExtra` keys replace the same keys from `autotest.ini`.
- Only one run at a time: `work/run_test.lock` holds the owner's PID, and a second run refuses to start. Only the owner restores sound and removes the test ini.
- Safety: if the DLL doesn't report autotest mode within 10 s, the game is killed (no unmuted game with intros at normal speed).
- Fixtures: start a random map with the wanted settings (`start_epoch`, `citizens`, ...), let it run, `eectl.py save copper40` at the interesting moment, then run tests from that save.

Scenarios: a base map plus a setup file in `scenarios/` (spawned towers, armies, ...), built for one question.
`scripts/scen.sh` runs variants on it, REPEAT (default 5) runs each with game_seed 1..n, scored like the suite:

```sh
sh scripts/scen.sh 30 towers_on_path "random_map=Continental,map_size=0,ai_count=2,seed=101,start_epoch=3" \
    "old:attack_leash=0,attack_weak_spot_radius=25,avoid_towers_margin=3" "new:"
```

| Scenario | Question |
|---|---|
| `pathtest` (`Data\Scenarios\pathtest.scn`, made in the editor, `-Scenario pathtest`) | towers, walls and lasers on the direct way, the long way round to the main base free |
| `siege_towers` (Continental 101, 2 AIs) | same-era defences: 5 bronze guard towers in front of the defender, the attacker has 6 catapults (range 7 > 5) |
| `towers_on_path` (Continental 101, 2 AIs) | a focused tower cluster in front of the defender, open ground around it, armies for both |
| `wall_gap` (Continental 101, 2 AIs) | no way around: a wall across the continent, one gap guarded by towers and lasers; armies for both |
| `assault` (Continental 101, 2 AIs, Bronze) | the defender builds for 15 min, then the attacker (no defence layout) gets 43 units incl. 3 catapults; base ini with `first_attack_min=16,first_attack_max=18,attack_interval_min=5,attack_interval_max=6` |
| `siege_raid` (Continental 101, 2 AIs, Bronze) | the defender builds for 12 min and gets 8 spearmen, then the attacker gets 4 catapults with 4 spearmen outside the tower range; `count` at minute 24 shows what is left |
| `raid` (Continental 101, 2 AIs, Prehistoric start, base ini `epoch_min=15,epoch_max=20`, 55 min) | a grown base raided at minute 46 by 15 spearmen and 10 bowmen; `count 3` at 54 |

Suite: 4 land maps with fixed seeds (Continental, Plains, Highlands, Mediterranean), headless with turbo,
one or more runs per variant and map, then scored (survival, epoch, workers, food; with the standard error
per AI):

```sh
sh scripts/suite.sh <minutes> "name:key=value,key=value" ...    # names without underscores
REPEAT=5 QUIET=1 sh scripts/suite.sh 75 "a:" "b:key=1"       # 5 runs per map (game_seed 1..5), averages only
EPOCHS="" sh scripts/suite.sh 50 "base:" "plan:copper_plan=1"     # EPOCHS="" = the game's real epoch timer
```

## Log (`aimod_econ.csv`)

- CSV rows: per AI every 10 s, with stock, resource demand, worker targets and counts, manager timers and epoch.
- `#` lines: events, for example settings, budget decisions (`# B`), epoch plan, attacks (`# attack:`),
  attack routes (`# route:` start/goal/known defences, `# route done:` waypoints, `!` = inside tower range),
  weak-spot choices (`# weak spot:`) and battle counters every 10 minutes (`# battle:`).
- Economy lines every 5 minutes: `# gather:` workers per resource, mean distance of their sites to the nearest
  drop site and income per worker-minute; `# spend:` what was paid, by caller (5e8a11 = production queue: units,
  research; 62f076 = buildings), with the income so far; `# place:` the build manager's placements per type
  (tries/failed/dropped); `# research:` gathering upgrades scheduled; `# attention:` share of ticks each AI
  activity was enabled (every 10 min).

Scripts:
- `scripts/summarize.py <csv> [minutes]`: per AI timeline.
- `scripts/score.py <csv>...`: suite score.
- `scripts/econ.py [-s min] <csv>`: per AI economy timeline (stock, demand, targets, workers, army lines).
- `scripts/econsum.py [-m min] <csv>...`: economy per suite variant (income, soldiers trained, mine distances, rates).
- `scripts/churn.py <csv>...`: citizens changing resource and target volatility per variant.
- `scripts/epochs.py <csv>...`: epoch advances per AI and their delay after the planned attempt.
- `scripts/watch_deltas.py`: watchpoint hits grouped by code path.

## Control pipe

While the game runs, the DLL serves `\\.\pipe\eeaimod`, one command per connection. Commands that touch
game state run on the simulation thread.

```sh
venv/Scripts/python scripts/eectl.py <command>
```

| Command | |
|---|---|
| `status` | game time, speed, per AI: epoch, stock, workers, targets, demand |
| `camera [x y]` | read or center the camera (tiles) |
| `speed [s]`, `pause`, `resume`, `quit` | |
| `peek <hex> [len]`, `poke <hex> <bytes>` | read/write memory (safe: bad addresses return an error) |
| `sites P`, `foodcheck P`, `units P` | resource sites, food search, citizens by job for AI player P |
| `costs P [types]` | next epoch and unit costs |
| `threat P` | enemy defences AI player P knows about |
| `targets P` | all enemy buildings of AI player P: owner, tile, seen by P or not, known tower threat on them |
| `army P [type]` | soldiers of player P (optionally only one unit type) grouped by current goal (EEUGLocation = marching, EEUGAttackUnit = fighting; goal +0x30 = target unit), with centre and the goal's first dwords |
| `count P` | all units and buildings of player P by type (type:count), e.g. whether an AI has a Siege Factory (63) |
| `bqueue P` | the build manager's queues per epoch (building types; `$` = can be built now) |
| `drops P` | drop sites of player P (Town Center, Capitol, Settlement; `g` = granary) |
| `tech P id...` | tech records of player P: state bytes (+4 researched, +6 available), cost, effects |
| `rlist P` | research manager: pool (list -1) and scheduled techs per epoch (`*` = researched) |
| `basemap P [r]` | map of AI player P's base, 2 characters per tile: T tower, W wall, G gate, C town center/settlement, b building, c citizen, s soldier, m mine, o planned tower slot, digits = other players |
| `defence P [0\|1]` | defence layout off/on for one player (A/B scenarios) |
| `tinfo P type...` | type data (family, hit points, attack, range) and whether player P can train/build it now |
| `spawn P type x y [n]` | create n units of a type for player P around tile x, y (2-tile grid), through the game's own debug spawner (EEMHackANewUnit, executed in the next tick like a player command). Type ids: `data/db/converteroutput.log` |
| `spawnline P type x0 y0 x1 y1 [step]` | one unit every step tiles along a line, e.g. a wall |
| `watch <hex> [len]` | set the hardware watchpoint at runtime |
| `save <name>` | save the running game to `Data\Saved Games\<name>.ees`, a fixture for later runs (`run_test.ps1 -Save <name>.ees`, without `random_map`) |
| `ddinfo [dump\|blit\|notex1]` | DirectDraw state (screen, surfaces, z-buffer, display mode, counters); `dump` writes front/back/window to `aimod_dumps/dd_*.bmp`. Note: it locks the z-buffer, which changes the windowed rendering state |
| `ddpoke z\|back\|front\|mode` | one of ddinfo's side effects alone (lock a surface, query the display mode); `ddpoke cursor x y` puts the Windows cursor at a client position in the game's own (DPI-virtualised) coordinates - `ddinfo` shows the game cursor and the mouse device state |
| `uipost <hex msg> [hex p1]` | post a message to the UI thread's queue; runs on the pipe thread, so it also works in the menus |

### Performance

- `ddinfo` reports the windowed present timing since the last call (frame interval, readback, copy to the
  window, rest = the game) and the game's own frame statistics (FPS display).
- `ddpoke bench [w h]`: CPU cost of the copy/scaling methods for a window of w x h.
- `profile [ms]`: sampling profiler over all game threads (busy threads, modules, hot addresses);
  `profile <ms> render`: only the render thread, waits included, with the game function each sample is in or
  called from.
- Autotest `present_every=N` presents only every Nth frame (to separate present cost from game cost).

Findings 2026-10-11 (2560x1440, RTX 3080, dgVoodoo DirectX pass-through): ~80 fps; the render thread is CPU
bound in Windows' DirectX 7 layer and the driver (DDRAW.dll 35%, nvd3dum 26%, kernel calls 24%), i.e. per
draw-call overhead, which grows with the visible map. The present costs ~15%. dgVoodoo's own DirectX
translation (pass-through off) was twice as slow (20 ms per frame).
DDrawCompat v0.7.1 (narzoul) instead of dgVoodoo's ddraw.dll: 7.9 ms per frame with its default
`CpuAffinity = 1` (whole game on one core), **5.6 ms (~177 fps) with `CpuAffinity = all`**
(`DDrawCompat-Empire Earth.ini`); game, editor and windowed/borderless present work.

### Headless UI tests (menus, editor)

Autotest key `headless=1`: the game window is created off-screen, never activates and ignores focus
changes, so nothing on the desktop, the mouse or the keyboard is touched. Drive it through the pipe:
`run_test.ps1` starts headless runs suspended and switches off the loading splash before the game runs (it
appears before the engine loads the DLL, centred on screen, and takes the focus); the display DLL's probe
window is moved off-screen too. Nothing of the game should become visible or take the focus.

| Command | |
|---|---|
| `vmouse x y` | game cursor to x, y (game pixels); `vmouse off` = real mouse again |
| `vclick x y [right\|mid]` | move there and click (button held ~150 ms). Move there with `vmouse` first and wait a moment; screens ignore clicks while they load |
| `wclick x y` | click by posted window messages, for screens on the WONAPI UI (civ builder) that read Windows messages instead of the game cursor |
| `shot <name>` | the next presented frame to `aimod_dumps/<name>.bmp` (game resolution) |

`sh scripts/editor_headless.sh [game|aoc|WxH]` starts headless and opens the scenario editor (main menu →
game tools → scenario editor, retried until the log says `# editor: building`). `scripts/bmp2png.ps1` and
`scripts/crop.ps1` turn shots into PNGs. Menu buttons (1024x768): game tools 512,472; scenario editor 512,370.
Editor icons at 2560x1440: y 1420, x 14 map, 60 elevation, 103 terrain, 147 units, 193 triggers, 237 players,
280 texts, 322 civilizations.

`run_test.ps1 -Exe <name>` starts a renamed copy of the exe (the DLL accepts any copy of the 1.04 exe).
Autotest keys for the display work: `mode` (native|windowed|borderless, or the older `windowed`), `diag_hooks=1` (counters in `ddinfo`, also fullscreen),
`diag_skip` (1 terrain, 2 water, 3 fog off). In normal play `[display] diag=1` enables the counters.

Screenshots of DirectDraw output: PrintWindow (`screenshot.ps1`) returns black. Put the game window on top
(SetWindowPos HWND_TOPMOST without activating) and use a screen capture; find the window by title
"Empire Earth" with a caption style - `MainWindowHandle` is the loading splash. Don't run `ddinfo` before
judging the windowed image (see above).

`scripts/mem.py` wraps `peek` for Python (`mem.u32(addr)`, `mem.rd(addr, n)`).
`scripts/screenshot.ps1` captures the game window to `work/screen.png`, even while it's covered.
`scripts/ee_mcp.py` exposes the pipe and screenshots as an MCP server (`.mcp.json`) for Claude Code.
