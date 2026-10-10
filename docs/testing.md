# Testing

The tooling runs AI-vs-AI games unattended and lets you inspect the running game.

## Autotest mode

The DLL looks for `aimod_autotest.ini` in the game folder. `scripts/run_test.ps1` copies
[`autotest.ini`](../autotest.ini) there for the duration of a run only, so normal play is never affected.
The `[autotest]` keys:

| Key | Meaning |
|---|---|
| `skip_intro=1` | skips the intro movies |
| `quit_minutes=75` | ends the game after this many game minutes |
| `start_epoch=N` | (random maps) starting epoch, e.g. 2 = Copper Age, for tests that should begin later in the game |
| `speed=7.5` | game speed (normal 2.5). Up to 7.5 the simulation step stays normal, so a run is ~7x faster than real time and behaves identically. Higher speeds enlarge the steps. |
| `random_map=Continental` | starts a random map instead of the save. Also: `map_size` (0 tiny..), `ai_count`, `seed`, `teams=ffa\|pairs`, `start_epoch`, `citizens`, `ai_cheat`, `ai_ignore`, `epoch_min`/`epoch_max` (test epoch timer). The human player (slot 0) idles, sees the whole map and is allied with every AI. |
| `scenario_mode=1` | starts `Data\Scenarios\<save>.scn` through the real scenario path (set by `-Scenario`) |
| any `aimod.ini` key | overrides that setting for all difficulties, e.g. `avoid_towers=0` |
| `watch=<hex>`, `watch_len` | hardware write-watchpoint, logs every writer of an address |
| `trace_player`, `trace_until_min` | logs the economy calls of one AI |

Same speed, same settings, same seed: the run is byte-identical. A/B tests are therefore exact, and
any difference comes from the change under test.

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

Suite: 4 land maps with fixed seeds (Continental, Plains, Highlands, Mediterranean), one run per variant
and map, then scored (survival, epoch, workers, food):

```sh
sh scripts/suite.sh <minutes> "name:key=value,key=value" ...    # names without underscores
EPOCHS="" sh scripts/suite.sh 50 "base:" "plan:copper_plan=1"     # EPOCHS="" = the game's real epoch timer
```

## Log (`aimod_econ.csv`)

- CSV rows: per AI every 10 s, with stock, resource demand, worker targets and counts, manager timers and epoch.
- `#` lines: events, for example settings, budget decisions (`# B`), epoch plan, attacks (`# attack:`),
  attack routes (`# route:` start/goal/known defences, `# route done:` waypoints, `!` = inside tower range),
  weak-spot choices (`# weak spot:`) and battle counters every 10 minutes (`# battle:`).

Scripts:
- `scripts/summarize.py <csv> [minutes]`: per AI timeline.
- `scripts/score.py <csv>...`: suite score.
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
| `watch <hex> [len]` | set the hardware watchpoint at runtime |
| `save <name>` | save the running game to `Data\Saved Games\<name>.ees`, a fixture for later runs (`run_test.ps1 -Save <name>.ees`, without `random_map`) |

`scripts/mem.py` wraps `peek` for Python (`mem.u32(addr)`, `mem.rd(addr, n)`).
`scripts/screenshot.ps1` captures the game window to `work/screen.png`, even while it's covered.
`scripts/ee_mcp.py` exposes the pipe and screenshots as an MCP server (`.mcp.json`) for Claude Code.
