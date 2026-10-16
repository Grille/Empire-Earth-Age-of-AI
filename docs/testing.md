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
| `setup=<file>` | control-pipe commands (one per line, `#` comments) run once at 2 s game time; `@<min> <command>` waits until that game minute, path relative to the game folder - e.g. `spawn` lines that turn a map into a test scenario (see Scenarios); replies of more than one line go into the log as `#   ` lines |
| `scenario_mode=1` | starts `Data\Scenarios\<save>.scn` through the real scenario path (set by `-Scenario`) |
| any `aimod.ini` key | overrides that setting for all difficulties, e.g. `avoid_towers=0` |
| `watch=<hex>`, `watch_len` | hardware write-watchpoint, logs every writer of an address |
| `tech_log=1` | logs every AI's newly researched techs once a second (`# techlog:`) and every free research completion with its call stack (`# techgrant:`, hook on EETechTreeNode::vf6) |
| `sndstate`, `sndchan N` (pipe/setup) | the sound manager's channels with their sound ids and Miles status, the mixer's channels, categories in use, the paused queue / SetMaxChannels(N) as the options screen does |
| `sound_channels=N`, `clamp_channels=0/1` | aimod.ini `[sound] channels` / `clamp_channels` for the run |
| `seltest T [x0 y0 x1 y1]` (pipe/setup) | builds the double-click selection message for unit type T (screen rectangle 0x8D2778 or the given one) and carries it out: units found, packed, unpacked, selected |
| `texts`, `texts dump [force]` (pipe/setup) | the text files read (`aimod_ai\_system.eeai`, `_template.eeai`); dump writes both with every key and its built-in text (existing files kept without force) - regenerate them after changing a built-in text (dll/texts.c, dll/texts_prompt.h) |
| `strict_parsing=1` | every mistake in the texts also queued and shown: in a test no message box (the files' mistakes come as system lines at the first AI update), the log says `# texts: strict parsing: N mistake(s) shown in the game`. Test of 2026-10-14 on `llmtest.setup` with the mock: no mistake in the built-in texts (no placeholder left unfilled) |
| `texts eval [P [Q]] <expression>`, `texts line P <situation> [Q [R]]` (pipe/setup) | an expression's value with P speaking to Q (default the local player), with _system.eeai's and P's personality's `[vars]` (dll/expr.c); the variant P would say now in that situation, its condition checked and filled. The system lines the player is shown are logged (`# system line:`). Test of 2026-10-14: a temporary personality with `[vars]`, `[vars:de]` and conditioned variants, `[vars]` in both files, a prompt piece and two system lines with expressions - all as written; old/new DLL on `llmtest.setup` with the mock: the same prompts but for the stock amounts |
| `diplo offer A B C`, `diplo capit A B` (pipe/setup) | AI A offers B an alliance against C / capitulates to B, as its initiative would (with `llm_ai_decide`: B's model answers) |
| `diplo propose A alliance\|betray\|capitulate\|rebel [C]` (pipe/setup) | put a move of the rules towards the local player to AI A's model, as the rules do (`llm_decide=1`; C: the third player of an alliance). An empty answer that ended by itself is its no (the proposal says "write nothing"); only one cut off (max_tokens) or an error lets the rules go ahead |
| `diplo rebel A` (pipe/setup) | AI A's rebellion against its lord now, without the dice: it rebels, or why not (loyal, served under 10 minutes, thinks well of its lord, too weak against its lord's side) |
| `offer_minutes=N` (test ini) | `[all] offer_minutes`: how long an offer to a player stands (`diplo offer 3 1 2` at 1.5 min with 5: withdrawn at 6.5). With `llm_decide=1` the rules' moves towards the local player go to the model first (`# llm: P2 decides on a move towards P1 (asked): rebel`, then `does it` / `declines` - 10 minutes before that move is proposed again - or `no answer on a move - the rules decide`) |
| `diplo terms P H` (pipe/setup) | AI P's terms for peace with H as the AI voice gets them (price or why none), with the peace score and opinion; `scripts/terms.setup` declares war, pays and makes peace |
| `diplo kills` (pipe/setup) | army, airforce, marine, civilian units and buildings each player destroyed of each other player, in the whole game and in the last 20 minutes (kills by pair; the sums match the game's statistics). `scripts/kills.setup` (`random_map=Continental,map_size=0,ai_count=3,seed=101,start_epoch=12`): spawns P1's Stingers, Dragoons and battleships (on the water) against P3's Clubmen, citizens, Apaches, fishing boats and transports, then `texts eval 3 1 them.kills(me, units.marine)` ... |
| `scripts/repair.setup` | `random_map=Continental,map_size=0,ai_count=1,seed=101,start_epoch=9,ai_diplomacy=0`: P2's Town Center damaged with 3 of P1's soldiers beside it - the citizens log line counts `repairs refused`; the soldiers removed at 2.5 min, the repair goes ahead |
| `scripts/repair2.setup` | the same with 3 of P1's Bronze Cannons (range 8) 10-12 tiles away - beyond 8 tiles, inside range + 3: repairs refused too |
| `llm`, `llm say P TEXT`, `llm filter M [P...]`, `llm prompt P Q` (pipe/setup) | the language model's state (server up/down, answers, failures, fixed lines, pending, last error) / the system message and every tool AI P would get towards Q, filled, into the log (`# llm prompt`; no model needed) / TEXT as the local player's chat line meant for AI P (not sent) / the local player's chat filter (player +0x9E0: 0 everyone, 1 allies, 2 enemies, 3 the players listed, +0xB2C). `diplo chat TEXT` sends a real chat line; `scripts/chatfilter.setup` tests every mode |
| `llm=1`, `llm_url`, `llm_model`, `llm_lines`, `llm_chat`, `llm_timeout`, `llm_decide`, `llm_ai_talk`, `llm_ai_talk_show`, `llm_ai_talk_gap`, `llm_ai_talk_turns`, `llm_equal`, `llm_ai_decide`, `llm_constrained`, `llm_fallback`, `llm_memory`, `llm_memory_mode` (`llm_ai_decide` needs `ai_diplomacy=1`; with the model the run must not be turbo - it outruns the answers) | aimod.ini `[llm]` for the run. Without a model: `scripts/mockllm.py PORT LOG` is a fake server answering every request with something checkable (the line itself, markup to clean, every other line with numbers without them) and logging the prompts; `setup=_aimod/scripts/llmtest.setup` (personalities eroberer, haendler, ritter as P2-P4) chats and makes two AIs capitulate. `llm_decide=1`: the mock calls the tools it is offered by keyword (peace / war / gold), rotating the answer's form (a call with words, without words, written into the text); `setup=_aimod/scripts/decide.setup` (random map, `personality_P2=haendler`) goes through every tool and its limits. `llm_constrained=0`: `llm prompt P Q` lists the steps open (a vassal: war on its lord - the log says it rebels). `llm_fallback=pause`: start the server only after the first request (`llm say 2 Hello`) - the log shows `# llm: paused ... (cannot connect)` and `answered after N s - the game runs on` |
| `textlog N` (pipe) | the next N text draws into the log (`# textlog:`): surface, font, HFONT, the bitmap font used, the colour (GEText +0x14, floats r g b and the size) and the string - for a screen whose text is missing |
| `textstate N` (pipe) | device state around the mod's texture text: 0 as the scene left it, 1 fog off + normal alpha blending (default), 2 also texture stage 0 = texture x diffuse. With `textlog` the states at each draw are logged (`# textlog state:`). Repro: the player's 70-minute save, save screen opened 4 times: state 0 blank labels in 2 of 4, state 1 in none |
| `wtype TEXT` (pipe) | the characters as WM_CHAR to the game window (edit boxes, e.g. the single-player name field) |
| `drawcount` (pipe) | objects collected for the last frame: units/buildings/missiles of the draw limit, small objects of 1536 |
| `draw_limit=N` | aimod.ini `[display] draw_limit` for the run |
| `sound_log=1` / `2` | (also aimod.ini `[sound] log=`, for normal play) 2 adds every play (id, volume, pan, the node's channel, the caller) and the mixer's channel/start/volume calls. 1: once a game minute (`# sound:`): sounds the game asked for, the ones the sound manager dropped and why (all copies of that sound busy / category full / no free channel), the Miles mixer's refusals, the most dropped sound ids. Calls at volume 0 (out of earshot) are counted apart: the manager skips them on purpose |
| games without AI | `setup`, `quit_minutes` and speed run from the simulation loop when no AI resource manager ticks (e.g. `-Scenario` with only the human player) |
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
| `overrun` (Continental 101, 2 AIs, Bronze) | as `assault`, but the attacker gets about 90 units: a hopeless fight at home (muster). 4 seeds, 40 min: muster off - the defender lost 3 of 4, 35 workers at the end; on - lost 2, 42 workers (assault: no change, it never musters there) |
| `assault` (Continental 101, 2 AIs, Bronze) | the defender builds for 15 min, then the attacker (no defence layout) gets 43 units incl. 3 catapults; base ini with `first_attack_min=16,first_attack_max=18,attack_interval_min=5,attack_interval_max=6` |
| `siege_raid` (Continental 101, 2 AIs, Bronze) | the defender builds for 12 min and gets 8 spearmen, then the attacker gets 4 catapults with 4 spearmen outside the tower range; `count` at minute 24 shows what is left |
| `raid` (Continental 101, 2 AIs, Prehistoric start, base ini `epoch_min=15,epoch_max=20`, 55 min) | a grown base raided at minute 46 by 15 spearmen and 10 bowmen; `count 3` at 54 |
| `late` (empty setup) | a plain late start (base `start_epoch=8`) for economy changes |
| `waves` (Continental 101, 1 AI, 120 min) | stress: P2 builds up for 60 min, then every 5 min the idle P1 gets 50, 75, 100, 150 ... 1000% of P2's army (`wave`, at P1's Town Center) and charges P2's town (`charge`, every 10 s); the minute P2 falls (game over) is the score. First run: army broken at the 150% wave (75 min), fallen at 98 |

Head-to-head: suites with one variant for all AIs measure how decisive the wars are, not which variant is
stronger (alive 70-78% with a standard error of ~6 points per variant). `scripts/h2h.sh` lets some AIs of the
same game play variant B (`variant_players=24` + `v_late_targets`, `v_hoard_cap_mineral`, `v_substitute_wait`),
the others the settings; the B seats swap every run (P2+P4, P3+P5 with 4 AIs). `scripts/h2h.py` scores the
per-game difference B - A (survival, soldiers, kills, losses, razed, epoch, workers):

```sh
REPEAT=8 NAME=hoard600 sh scripts/h2h.sh 50 late \
    "random_map=Continental,seed=101,map_size=1,ai_count=4,start_epoch=8" "v_hoard_cap_mineral=600"
```

2026-10-14, `late`, Continental 101 / Mediterranean 404 / Highlands 303, map_size 1, 4 AIs, 50 min, B - A per game:
hoard_cap_mineral 600 vs 200 (24 games): soldiers -5.2 (se 2.6), razed -6.7 (se 3.6), kills -21.5 (se 15), alive
-8 points - 200 stays. late_targets 15 vs 7 (food loop, 48 games): kills +10 (se 11), losses -6.5 (se 5.6), razed
+4.6 (se 2.9), alive +6 points (se 7) - leaning better, not proven; in `waves` (6 seeds each) it fell earlier
(mean 101 vs 107 min, survivors counted as 120, about 6 min of noise) - off by default.

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
| `wmclose` | WM_CLOSE to the game's windows: the game's own way out with its teardown - `quit` and the end of a run (`quit_minutes`) terminate the process and never run it, so exit crashes need this. `scripts/exit.setup` (on a copy of a save: `@77 save zz_exit2`, `@77.5 wmclose`) |
| `attacks P` | AI P's attack groups: target, next attack time, per subgroup its class (EECPASGLand, Sea, Air, SeaTransport ...), members alive and their centre (about -1000 or -10000: aboard a transport), how many march, fight, do something else or nothing. `scripts/waves_ref*.setup` (copies of `aimod_ref_ww1.ees`): P3's main force stays in its own town for 9+ minutes, re-routed to its docks again and again, only stragglers leave; 3 more transports change nothing |
| `attacks P raw` | as `attacks P`, plus per subgroup its state (+0x1D0), mission, goal, waypoints, engaged members, and per member its type, tile, hit points, goal (with an attack goal: the target, its owner, type and family), then the subgroup's fields +0x100..+0x260 |
| `asgtrace P [off]` | logs every state or goal change of AI P's attack subgroups (`# asgtrace:`): class, state old -> new, mission, goal, members alive, centre, waypoints, engaged members, march/fight/other/idle, the group's fields +0x28..+0x6C. `scripts/asgtrace.setup` |
| `train P type [count]` | AI P orders units through its own unit manager (a priority order: the stock above the epoch reserve); unlike `spawn`, they come from its buildings and join its unit pool |
| `give P amount` | player P's food, wood, stone, gold and iron each grow by amount. `scripts/transports.setup` / `transports_ref.setup`: the reference save, P3 given 3000 each, with and without 3 more transports |
| `continent x y ...` | per tile its land continent (EEAIContinentManager 0x425F9F kind 2) and ocean (kind 1) ids and the area object. Reference save: both AIs' towns, the player's base and outposts all on land continent 2 |
| `assault P` | AI P's assault (`assault=1`): phase, target, rally point, objective, waypoint, wave, subgroups led, launches, call-offs, orders |
| `landing P [now]` | AI P's landing (`landing=1`): phase, target, site and the water in front, ships, warships, passengers (aboard), subgroups led, counters; the last site search (points, land, at a shore, at its sea, the seas seen); the beach's strength when planned and now (soldiers, defences, over the water); the warships and their targets; objective, each passenger (tile, hit points, goal) and ship. `now`: plan at once. Test keys: `landing_knows_map=1` (every building of the target counts, seen or not), `landing_flag` (the unload goal's last argument), `landing_citizens=N` (0-10, default 10: the most citizens along for the forward base - by the landing's size: one transport none, two 3 (two towers), three 6 (a barracks too), four and more 10 (towers, barracks, town center, hospital, factory)), `landing_transports=N` (1-10, default 8: the most transports a landing takes - the size follows the beach), `landing_waves=N` (0-10, default 3: how often the transports bring more soldiers over), `landing_trace=1` (`# ldtrace:` every transport with soldiers aboard, every 0.5 s while sailing: tile, hit points, aboard, goal; an unload goal's water points, unload point, reach) |
| `intel P` | what AI P knows (`honest_vision=1`): floors per owner, records per owner and kind (seen now, remembered where they were, somewhere), cells looked at (ever, within 3 minutes), the army it knows of |
| `calloff P T` | AI P calls off its attack on T at the next safe point (the game's call-off, as attack_retreat). On the reference save `calloff 2 1` at 86.6 min crashed the game every time before the call-off guard |
| `vis P [target] [allies] [far] [all]` | what AI or player P sees of its enemies: per kind (building, soldier, ship, aircraft, other) and distance to its nearest unit (with `allies`: or an ally's) the units whose visibility count for P is set; `far`: the land tiles farthest from every unit; the explore-all flag and vision slot (+0x45C) per player |
| `vistrack P x y r` / `vistrack P` | remember the enemy units within r tiles of x, y (adds); then each one's tile, hit points, P's count and the raw counts of all slots |
| `terrain x y [r]` | ASCII map around a tile (r up to 40): `~` ocean, `.` the tile's continent, digit another continent, `?` neither (outside the map), `T` gaia objects (trees, resources), `B`/`C`/`D` buildings of P1/P2/P3, `b`/`c`/`d` their units |
| `scripts/landing_ref.setup` (reference save) | P3 given 3000 of each, attack and landing at once, `landing 3` every minute, `diplo kills` at the end |
| `scripts/landing_big.setup` (reference save) | P2 given 4000 of each, 40 machine gunners at home and 3 transports: a landing of 36 on 3 transports (the size from the beach and those behind it), the transports gather at sea and run in together (all 33 ashore within 36 s), then the waves |
| `scripts/landing_base.setup` (reference save) | P2 given 4000 of each, 60 machine gunners and 5 transports: a landing of 53 with 10 citizens - the forward base (tower, barracks, town center, hospital, factory, tower), soldiers trained there join the troops |
| `scripts/landing_sea.setup` (reference save) | the player's 7 U-boats and 6 strafing Fokkers before its coast, P2 with 4 frigates and 2 cruisers: it waits while its escorts clear the way (subs 4 -> 0, aircraft 10 -> 4), then lands 36 of 36; without the frigates and cruisers it waits and calls the landing off |
| `board P x y [r]` | the transports of AI P within r tiles (10) of x,y: how many each holds; an empty one loads every soldier and siege unit of P within r (the game's load goal) - a transport's capacity. `board reload P type`: the first unit of P of that type - type +0x50C, its stats record (= the player's record) and the unit object, every non-zero dword (for diffs). `board hits`: the game's hit factor (0x4D5F2C) of some attackers (P2) against a transport, U-boat, fighter, battleship, machine gunner and tower (P1). `board types`: the type fields (ints) small for every transport type and 0 for a soldier - +0x54C is the capacity (313/110: 10, 111: 12, 112-114: 14; 110 held 10 when loaded with 53) |
| `scripts/refcamp.setup` (reference save) | the player's camp near 108,75 plays: 4 machine gunners and a tank every 2 minutes (artillery every 4), pushes into P3's town at 78, 82, 86 (`charge`), pulls back a minute later (`move`); both AIs attack at once; `diplo kills`, `army 1` at the end. The runs are deterministic (two runs, same numbers) |
| `scripts/pathtest_as.setup` (`-Scenario pathtest`) | `diplo kills`, `army 2/3`, `assault 2` at 10 and 20 minutes |
| `attacknow P` | AI P's next attack is due now (player +0xED8 = game time); the game launches it at its next look if it has a target. `scripts/retreat_ref.setup` (on a copy of `aimod_ref_ww1.ees`: P3 attacks at 76 min) |
| `nullmember P [off]` | the first member record of one of AI P's attack subgroups (2+ members) loses its unit; with `off` the attack is called off in the same step, as the diplomacy does - the state of the crash at 0x45AD70 (engine guard `asg_purge`). `scripts/nullmember.setup` (`random_map=Continental,map_size=1,ai_count=4,seed=303,start_epoch=9,ai_diplomacy=1,turbo=1`, 40 min): taken out, called off, no crash |
| `threat P` | enemy defences AI player P knows about |
| `targets P` | all enemy buildings of AI player P: owner, tile, seen by P or not, known tower threat on them |
| `army P [type]` | soldiers of player P (optionally only one unit type) grouped by current goal (EEUGLocation = marching, EEUGAttackUnit = fighting; goal +0x30 = target unit), with centre and the goal's first dwords |
| `wave A D pct [x y]` | player A gets pct% of player D's army, type by type, the most of each type D had at a wave so far (ships and aircraft left out), in groups of 16 around A's Town Center (the spot is kept for later waves); A and D become enemies |
| `charge A D` / `charge off` | from now on every 10 s (at most 80 orders a pass, logged every minute): each soldier of A without a fight attacks the nearest soldier of D within 15 tiles, else the nearest citizen within 20, else D's nearest building, wall or tower (farther than 20 tiles: walks there first) |
| `objs P T` | every object of type T of player P: tile, hit points, goal (e.g. `objs 2 42`: farm fields, 1 hp = not built) |
| `build P bx by x y r` | player P's citizens within r tiles of x,y build P's building at bx,by (EEUGBuildRepair, the game's order) - with `found`: enemy builders at a tower |
| `techs P` | techs player P has researched, and which are new since the last call (with tech epoch and cost) |
| `watchtech P id`, `watchtype P T [off]` | hardware write-watch on a tech's researched byte / on a unit's field (default +0x2C, its type); the writer goes into the log |
| `found P T x y` | lay out an unfinished building of type T for player P (0x62EF8A, as the build manager does; pays its cost) - e.g. an enemy tower foundation next to an AI's gatherers |
| `hurt P T x y r dmg` | damage every object of type T of player P within r tiles of x,y as an attack does (target vf 0x70) - e.g. `hurt 2 42 128 128 256 1000` destroys P2's fields |
| `plan P`, `plan P retry T` | the AI's building plan (type:wanted) / type T counts as a dropped build-queue entry and is tried again now (the retry of dropped buildings) |
| `count P` | all units and buildings of player P by type (type:count), e.g. whether an AI has a Siege Factory (63) |
| `bqueue P` | the build manager's queues per epoch (building types; `$` = can be built now) |
| `drops P` | drop sites of player P (Town Center, Capitol, Settlement; `g` = granary) |
| `tech P id...` | tech records of player P: state bytes (+4 researched, +6 available), cost, effects |
| `rlist P` | research manager: pool (list -1) and scheduled techs per epoch (`*` = researched) |
| `basemap P [r]` | map of AI player P's base, 2 characters per tile: T tower, W wall, G gate, C town center/settlement, b building, c citizen, s soldier, m mine, o planned tower slot, digits = other players |
| `defence P [0\|1]` | defence layout off/on for one player (A/B scenarios) |
| `tinfo P type...` | type data (family, hit points, attack, range) and whether player P can train/build it now |
| `chat [0\|1\|2]` | AI chat level (0 off, 1 debug lines, 2 also the 5-minute status); lines shown/dropped per player |
| `chatme text` | the local player types text into the chat box, as EEUIECESendMessage::vf1 (0x5C28D6) sends it: cheat code first (when cheats are on), then the chat line to all - scenario triggers on chat messages see it as typed (reloadtime.scn: `class+1` raised the class reload, the trigger's "OK" came back) |
| `trig [list\|desc]`, `trig on\|off\|reset\|fire T`, `trig effect N` | the scenario's triggers (docs/triggers.md): each with its state, conditions (type and key values: chat text, attribute and compare, tech, trigger, time) and effects (category and action); T = index or name (the editor's, e.g. `<Trigger0>`). `fire` runs all of T's effects at once (delays and random groups ignored) and marks it fired; `effect N` runs one effect. Test of 2026-10-17 on reloadtime.scn: all 8 triggers listed with their chat texts, `chatme class+1` fired trigger 0 (then off, fired), `reset` put it back - a scenario's triggers as test levers, switched on or fired at a set minute from a setup file |
| `say P text` | AI player P writes text in the game's chat (no rate limit) |
| `diplo [A B S]`, `diplo observer 0\|1` | stance matrix, AI opinions, power and wars answered; with arguments player A takes stance S (0 allied, 1 enemy) towards B, as the stance message does |
| `diplo tribute A B N` | player A's tribute of N reaches AI B (the tribute hook's logic) |
| `persona [P name]` | personalities loaded and assigned (traits, lines said); with arguments AI P gets personality `name` |
| `spawn P type x y [n]` | create n units of a type for player P around tile x, y (2-tile grid), through the game's own debug spawner (EEMHackANewUnit, executed in the next tick like a player command). Type ids: `data/db/converteroutput.log` |
| `spawnline P type x0 y0 x1 y1 [step]` | one unit every step tiles along a line, e.g. a wall |
| `watch <hex> [len]` | set the hardware watchpoint at runtime |
| `save <name>` | save the running game to `Data\Saved Games\<name>.ees`, a fixture for later runs (`run_test.ps1 -Save <name>.ees`, without `random_map`). Known quirk: such a save loaded with `turbo=1` never starts simulating (the AI makes no step; without turbo it runs, and autosaves run with turbo) |
| `ddinfo [dump\|blit\|notex1]` | DirectDraw state (screen, surfaces, z-buffer, display mode, counters); `dump` writes front/back/window to `aimod_dumps/dd_*.bmp`. Note: it locks the z-buffer, which changes the windowed rendering state |
| `ddpoke z\|back\|front\|mode` | one of ddinfo's side effects alone (lock a surface, query the display mode); `ddpoke cursor x y` puts the Windows cursor at a client position in the game's own (DPI-virtualised) coordinates - `ddinfo` shows the game cursor and the mouse device state |
| `aim [lead 0\|1] [missiles 0\|1] [spread 0\|1] [reset] [types]` | settings at runtime; missiles launched, spread switches/held shots, per missile flight time and end (hit, target died first, missed); `types` = unit types firing missiles. Also ranged shots at units and\|1] [reset]` | ranged shots at units and, after `reset` (always in autotest), the measurement: where the target really is when each projectile comes down, against the original and the led aim ("on tile" = the hit condition), split by moved/stood and in/out of the arc's reach; `down` = projectiles that came down beside their target. Autotest `lead_targets`, `aim_log=N` (first N shots as `# aim:` lines) |
| `uinfo x y [r]` | units within r tiles: owner, hit points, position, speed, facing, goal |
| `move P x y r tx ty` | units of player P around x, y walk to tx, ty keeping their spread (tests) |
| `uipost <hex msg> [hex p1]` | post a message to the UI thread's queue; runs on the pipe thread, so it also works in the menus |

### Performance

- `ddinfo` reports the windowed present timing since the last call (frame interval, readback, copy to the
  window, rest = the game) and the game's own frame statistics (FPS display).
- `ddpoke bench [w h]`: CPU cost of the copy/scaling methods for a window of w x h.
- `profile [ms]`: sampling profiler over all game threads (busy threads, modules, hot addresses);
  `profile <ms> render`: only the render thread, waits included, with the game function each sample is in or
  called from (only real return addresses: the bytes before them are a CALL), and for the time in GDI which
  non-Windows code called it.
- `topbar [0|1|2]`: what the game's top bar shows - the age, the game clock, or "FPS: n".

Findings 2026-10-12 (user report: 25 fps in a long game, window mode): in the user's game the render thread was
64% in GDI (gdi32full); in test games 11-18%, all GDI calls from DDrawCompat itself (its ddraw.dll +0x46CB6,
+0x47544). Not the cause: scaling into a smaller window, the FPS display, Alt+Enter switches. Test frame times
vary 7-33 ms between and within runs in the same save - the slow state is not reproduced yet.
- Autotest `present_every=N` presents only every Nth frame (to separate present cost from game cost).
- `camtrace N`: the next N presented frames - frame time, camera movement, scroll steps in between (`# camtrace:`
  lines). Edge scrolling headless: `vmouse 2559 700`. 2026-10-12, AoC scrolling: 0 or 1 steps per frame of 0.285
  tiles each (every fifth frame none); with `[camera] smooth` one step per frame, 0.0285 tiles per ms of frame time.
- Frame watch (`[display] frame_watch`, default 35 fps; autotest `frame_watch=N` also runs it headless): in the
  player's own games, every minute `# frames: at <min>: <fps>, worst second, seconds under the threshold; GDI
  objects, USER objects`, and after 10 s under the threshold the render-thread profile (`# |` lines, at most 5 a
  session, 5 min apart). The profile now also lists the game code that led into GDI and reads 8 KB of stack.
  Test game (headless, 2026-10-12): 155 fps, worst second 113, 124 GDI objects; 30% of the render thread in GDI,
  ~60% of that from DDrawCompat with no game code on the stack (its own work), the rest under
  DX7HRTnLDisplay +0x3815 and Low-Level Engine +0x447E6.
- Findings 2026-10-12, a 109-minute game of the user (DDrawCompat, borderless 2560x1440): 111 fps at minute 1,
  49 at 10, 35 at 20, 17-24 from 50, 12-13 at the end; GDI/USER objects flat (140/67). Profiles: 73% of the render
  thread in gdi32full - ~28% under DX7Rasterizer::Draw(GEText) (GetTextExtentPoint32A, text on surfaces), ~28%
  under DX7Surface::Lock (gdi32's D3DKMT calls), ~25% under the exe's WONAPI code (0x6E0020). Its last autosave
  (minute 101) in a fresh session: 148 fps on the user's screen, 165 headless - **the slowdown builds up over a
  session, not with the game state**. The process had 620-690 MB in the slow session, 350 MB after the restart.
  Text costs at full speed: text into a rectangle ~0.4-0.7 ms per call (1-6 per frame, all from Low-Level Engine
  +0x70B68), text locks (GetDC on a surface) ~2 ms each. The minute line now has `# frame costs:` (those calls per
  frame with their callers) and `# session:` (private memory, handles, surfaces created/deleted, textures
  created/destroyed since the start) - the next long session shows what grows.
- The next session (2026-10-12): memory +13 MB a minute, surfaces/textures flat. Heap analysis from outside
  (`scripts/memgrow.py` growing regions, `heapid.py` which heap, `lfhscan.py` LFH block sizes and contents;
  all VirtualQueryEx/ReadProcessMemory, read-only, safe on the player's running game): the process heap grew by 16 MB segments full of 1504-byte LFH
  blocks (~4 a frame) holding the same DC and font handle, linked in a list. Autotest `alloc_trace=<bytes>`
  (hot-patched RtlAllocateHeap, raw stack copies, chains in the minute line) showed the source: the game's
  ExtTextOutA on the surface DC -> DDrawCompat -> gdi32full LpkUseGDIWidthCache / GdiRealizationInfo: a font
  realization cache entry per text output, never freed. Headless A/B (6 min): DDrawCompat 232 -> 263 MB,
  dgVoodoo flat (but ~half the frame rate); DDrawCompat `GdiInterops = none` or `FontAntialiasing = off`: still
  growing. Fix `[display] text_dc` (default 1): the game's text goes into a memory DC of the mod (TextLock only
  locks the surface; ExtTextOutA/TextOutW load the 32x32 tiles of the text box, TextUnlock writes them back):
  233 -> 240 MB over the same run, text areas of a screenshot pixel-identical. The text surface is the
  2560x1440 back buffer in video memory: each text lock still costs ~4-5 ms (the readback, as with GetDC).
- The user's next game with the fix: 93-119 fps over 110 minutes (before: 111 -> 13), memory levels off at ~295 MB.
  The text lock (~4 ms a frame) is DDrawCompat waiting for the GPU at any lock of the back buffer - locking only
  the text rectangles changed nothing. `[display] text_bitmap` (default 1): the engine's GEFontBitmap path for
  every font while its text goes to the back buffer (characters prerendered into 256x256 alpha atlases, drawn as
  sprites). Headless: text 0.02-0.05 ms a frame instead of ~4.5, 0 text locks, 149-153 fps vs 134-141; age label
  and resource bar as before (1 px lower), umlauts/ß correct after the movsx -> movzx patch. Two engine bugs
  fixed on the way: characters with a negative left bearing ("j") spilled into the neighbouring atlas cell (a
  stroke under every "i"; clipped to the cell while the mod builds an atlas), and without text_dc the atlas
  stays empty under DDrawCompat (no text at all).
- User report (menu at 800 fps): white dots left of letters (the sprites sample a texel of the neighbouring atlas
  cell: the arm of the "k" left of every "l") and no smoothing (the engine draws into ARGB4444, green -> alpha;
  Windows doesn't smooth there, and Tahoma's gasp table forbids it for small sizes). Now the mod writes each atlas
  character itself from GetGlyphOutline GGO_GRAY8_BITMAP (65 levels, every size), only into its cell, the last
  column left empty. Autotest `atlas_dump=1` saves each new font's atlas. Checked: menu buttons without dots,
  the small chat font with all intermediate levels (before: the lowest bands empty).

Findings 2026-10-11 (2560x1440, RTX 3080, dgVoodoo DirectX pass-through): ~80 fps; the render thread is CPU
bound in Windows' DirectX 7 layer and the driver (DDRAW.dll 35%, nvd3dum 26%, kernel calls 24%), i.e. per
draw-call overhead, which grows with the visible map. The present costs ~15%. dgVoodoo's own DirectX
translation (pass-through off) was twice as slow (20 ms per frame).
DDrawCompat v0.7.1 (narzoul) instead of dgVoodoo's ddraw.dll: 7.9 ms per frame with its default
`CpuAffinity = 1` (whole game on one core), **5.6 ms (~177 fps) with `CpuAffinity = all`**
(`DDrawCompat-Empire Earth.ini`); game, editor and windowed/borderless present work.

Autotest `personas=0`: only `aimod_ai/computer.eeai` is loaded - the AIs have no personality and say its neutral
lines (the `# talk:` log marks them `computer.eeai`; a fallback situation as `X as Y`).

### Alliance blocks

`diplo A B S` sets stances, `diplo observer 1` lets the test observer (P1, allied with every AI) count as a human
member. Needs `ai_diplomacy=1` and a player outside the blocks (one alliance of everyone ends the game).
2026-10-12, Continental 101, 4 AIs (P2-P5), speed 2.5:
- P2-P3 and P3-P4 allied, P2-P4 at war: P2 and P4 make peace within 2 s. P2 turns on P3: P4 sides with P3 (the
  attacked +10) and declares war on P2; P3 answers; {P3, P4} stay a block.
- Observer counts, P5 outside: P2-P4 all make peace. P1 turns on P2: P3 and P4 side with P1 (opinion 21 vs 0+10).
  P2 allied with P3 again: P2-P4 peace at once, P2 offers P1 peace; ignored for 2 min: block split, P3/P4 with P1.
- Veto: P3+P4 a block, P2 pays P3 3000 (opinion 35) and offers peace (score 47). P2 had just declared war on P4:
  refused, block_refuse_own names P4. Without that: accepted, P2-P4 peace through the block at once.
- Vassal: P3 capitulates to P4, P2 declares war on P3, then pays P4 and offers peace: P4 accepts (the vassal has no
  say), P3 makes peace with P2 and says vassal_grumble. P2 offers the vassal P3 peace itself: ask_lord ("talk to
  P4"); P4 declares war on P3: no longer a vassal, P3 answers the war.
- Reputation: P2 breaks its alliance with P5: P3 (loyalty 70) -10.5, P4 (loyalty 95) -14 towards P2. P3 capitulates
  to P4, P4 attacks it at once: P2 (loyalty 35) -26, P5 (loyalty 60) -45 towards P4 (x5), P3 itself -78.

### Population limit

2026-10-13, Continental 101, 2 AIs, Prehistoric start, Ignorieren off (`ai_ignore=0`; the suites run with it on,
i.e. with the game limit), 45 min: `pop_follow_human=1` cap 33 (the idle observer has ~5 units, game limit 166),
soldiers at 40 min 6 / 0; `pop_follow_human=0` cap 166, soldiers 41 / 25.

### Citizens and enemy towers

2026-10-13, `scenarios/citizen_towers.txt` (Continental 101, 2 AIs, Bronze start; P3 guard towers from the start
next to P2's iron, hunting ground, stone and gold). Towers spawned at minute 2 onto working citizens killed nearly
all of them whatever the AI did - not a fair test. With towers from the start every run lost P2's economy: its
defence front pointed at the enemy tower near its base and the builders of its own towers walked past it. After
the slot fix, P2's citizens at minute 20 over 12 seeds, avoidance + militia off / on: average 21.9 / 27.8
(2 collapses each; std. error ~4). Suite (4 maps, 2 seeds, 75 min): no change (workers 40.0 / 39.5, epoch 3.67 /
3.67); after the farm passes skip dangerous fields 9 citizens sent back in 8 games (one AI had 180 before: its
farmer pass and the retreat took turns). Militia: `found 3 462 74 61` next to P2's berries at minute 8 - destroyed
within 30 s; 3 Bronze soldiers among P2's woodcutters: a count-based militia lost 10 citizens, the strength check
joins only when it wins (both soldiers it fought died, P2 kept 32 citizens) and keeps out in the Prehistoric.

### Raids and tower builders

2026-10-13, Continental 101, 2 AIs, Bronze start. Builders: at minute 8 P3 gets 3 citizens and a guard tower
foundation next to P2's berries (`spawn`, `found`, `build`); 5 seeds, response off / on: the foundation stood at
minute 12 in all 5 (finished in 1) / was gone in all 5; P2 citizens minute 8 -> 12 +5.6 / +3.2. Raid: 6 Bronze
archers (type 10) at P2's fields at minute 12; 6 seeds, call-back off / on: P2 citizens minute 12 -> 16 +6.0 /
+6.2. The first raid check (the game's 0x4660B9, enemy influence) sent builders at the front back 250-300 times a
game, then wild animals counted as raiders; with enemy soldiers only and the searches refusing raided sites,
10-28. Suite (4 maps, 2 seeds, 75 min), off / on: alive 75 / 79%, epoch 3.62 / 3.83, workers at the end
37.3 / 43.2 (std. error ~5), call-backs ~66 per AI and game (hunting, mines, fields where armies move), no crash.

### More granaries

2026-10-13, suite (4 maps, Prehistoric start, 75 min, 2 seeds), `more_granaries` 0 -> 1: alive 75 -> 88%, epoch
3.71 -> 3.96 (std. error 0.11/0.07), workers at the end 36.5 -> 41.5, kills 174 -> 166 (18-21), no crash.
Mediterranean 404 seed 1: P2 1 -> 4 granaries (32 fields), food stock at 70 min 320 -> 4201.

### Farm fields

2026-10-13, 2 AIs, start epoch 13, 31 min, game seeds 1-3 (`objs P 42`: 1 hp = foundation). Without the fix the 8
fields stayed foundations on Large Islands (fishing map) and on Continental (still all 8 at minute 14). Income per
AI at minute 25 (6 AIs each), `build_fields` 0 -> 1: Large Islands food 2262 -> 4327, total 10123 -> 12093;
Continental food 2295 -> 4539, total 8169 -> 11711. `hurt 2 42 128 128 256 1000` at minute 15 (Continental): laid
out again at 15.2, all 8 rebuilt and farmed by 16.5. Suite (4 maps, Prehistoric start, 60 min, 2 seeds): food income
at 55 min 6618 -> 7563, epoch 3.42 -> 3.08 (std. error 0.12/0.10; same delay after the planned attempt, the
difference is the epoch timer and one AI beaten in the Copper Age), workers 44.5 -> 41.4 (5), kills 91 -> 103.
The first suite run crashed once (0x60BB19): the build list's re-queue asked for a lost field without foundation -
guarded in hook_place since (2 hits in the next suite, no crash).

### Automatic victory

2026-10-12, 2 AIs + observer (allied with both): P2 and P3 make peace = one alliance. `auto_victory=1`: game over at
once (P1=1 P2=1 P3=1). `auto_victory=0`: goes on (hint at once - the observer is allied with all), `diplo chat sieg`:
game over P1=1 P2=1 P3=1 (allies win with him).

### Ranged aim

Test 2026-10-12 (Continental 101, 2 AIs, start epoch 3, speed 1): 3 P2 guard towers (462) at 57..65,45, 20 P3
spearmen (24) spawned at 54,50 and walked between x 54 and 66 every 6 s real time (`move`), 10 rounds, `aim` at the end:

| | moving targets: aim on their tile at landing | came down beside the target | spearmen left of 20 |
|---|---|---|---|
| lead off, seed 1 | 44-52% (led aim would be 88%) | 35 of 65 shots | 14 |
| lead on, seed 1 | 89% | 3 of 43 | 7 |
| lead off, seed 2 | | 20 of 32 | 20 |
| lead on, seed 2 | | 4 of 33 | 10 |

Standing targets stay on tile 100% with the lead (speed 0, no change). Spearmen walk at 0.6-0.72 tiles/s, arrow
flights take 0.3-2 s.

Missiles (2026-10-12, same map, AA missile towers 147 vs P3 Apache helicopters (6) flying x 54 <-> 66 every 6 s;
`aim` shows launches, flight time and the end of each missile: hit within 1.5, target died first, missed):

| | moving targets: missiles hit / done (missed) | wasted on a dead target |
|---|---|---|
| intercept off, seeds 1-3 | 26/28 (2), 27/29 (0), 28/31 (3) | |
| intercept on, seeds 1-3 | 28/29 (0), 31/33 (2), 41/43 (0) | |
| 6 towers vs 8 helicopters, spread off, seeds 1-2 | 55/59, 53/56 | 4, 3 |
| spread on (switched 5/4, held 46/84) | 53/53, 53/53 | 0, 0 |

Intercept: flight times the same (1.9-2.2 s). Pursuit already hits: missiles 2.5 tiles/s vs 1.05.

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
