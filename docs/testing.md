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
| `random_map=Continental` | starts a random map instead of the save. Also: `map_size` (0 tiny..), `ai_count`, `seed`, `teams=ffa\|pairs`, `start_epoch`, `citizens`, `unit_limit` (500, the game's, shared by all players incl. the idle human), `ai_cheat`, `ai_ignore`, `epoch_min`/`epoch_max` (test epoch timer, not for vanilla players), `difficulty=easy\|medium\|hard` (the AIs' difficulty, default medium - the setup and the global 0x918C14 the AIs copy). The human player (slot 0) idles, sees the whole map and is allied with every AI. |
| `vanilla_players=3` (digits), `vanilla_difficulty`, `vanilla_cheat=0\|1` | those players play the game's own AI, unmodified, with their own difficulty and the game's Cheat (see "Against the base-game AI") |
| `game_seed=N` | the game's random seed at game start: the same game_seed (with the same map, `seed`, and config) plays the same game, step for step. The simulation draws from one generator (EEPhysics +0x14, seed +0x18), seeded by the game start message (type 0x5D, executed by 0x4BEE57, which also writes world +0x54). The menu start puts rand() after srand(TMGetTime()) in it (`hook_srand`), the load-game and scenario starts TMGetTime() itself (0x522DFC's callers) - a test run's random map is a load-game start, so until 2026-10-19 every run had its own seed and runs parted from ~5 s on; now `hook_start_exec` (trace.c) writes game_seed into the message whichever start built it. Checked: two runs of Continental 101 / 3 AIs / game_seed=1, 10 game minutes: the per-step trace identical (3479 steps), the AI log identical but for a column of heap pointers; turbo=0 and turbo=1 identical (the turbo run only logs idle steps before the first tick). Then one more hole: of three runs of Continental 101 / 5 AIs / 60 min, one parted at 28.7 min - the trace showed an AI message (type 129, handed to the lockstep server in a tick's last step) landing a tick late: turbo let the server send the next tick while that step still ran (EEPhysics +0xB8 is 0 already then); now turbo waits for the step's end too (control.c hook_phys_step, g_in_step). Checked: 6 runs of that game identical for 60 min, 6 more maps twice each (ai_diplomacy=1) identical, and turbo=0 identical to turbo=1 for 30 min. The map comes from `seed` (randmap.c writes world +0x54 after the start message). Without game_seed the start stays the clock's. |
| `sync_trace=<steps>` | writes `synctrace.txt` next to the test log: the start message's seed, then one line per simulation step for the first <steps> steps - step counter, game ms, the generator's seed, user inputs processed, a hash of the AI players' stock. Also the units (count and a hash), the messages each step hands to the lockstep server (`  m` lines) and with `sync_dump=<from ms>-<to ms>` every unit of the steps in that window. Diff two runs' files to find the first step they part (trace.c) |
| `setup=<file>` | control-pipe commands (one per line, `#` comments) run once at 2 s game time; `@<min> <command>` waits until that game minute, path relative to the game folder - e.g. `spawn` lines that turn a map into a test scenario (see Scenarios); replies of more than one line go into the log as `#   ` lines |
| `scenario_mode=1` | starts `Data\Scenarios\<save>.scn` through the real scenario path (set by `-Scenario`) |
| any `aimod.ini` key | overrides that setting for all difficulties, e.g. `avoid_towers=0` - read like aimod.ini (dll/ini.c): a share with its `%` (`navy_share=30%`, `home_guard_share=50%`; a bare `navy_share=30` is 3000%: logged as `# ini:` and the default used), small expressions (`epoch_min=2*25`) - no commas in a value here (-IniExtra splits on them) |
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

- **The game's own settings.** With the mod's `dinput.dll` in the game folder (dll/eesettings.c - the game's registry settings in a file) every run reads a copy of `-Settings` (env `SETTINGS`, default `config/test/eesettings.ini`: a player's settings of 2026-10-19 without the personal values, sound and music off) as `work/settings_slot<n>.ini` - not the player's `eesettings.ini`, not the registry; the log says `# game settings: <file>`. So tests no longer depend on the player's options (animal herds, scroll speed, the engine's tick rates ...), and nothing is muted. Checked 2026-10-19: the registry byte for byte unchanged after a run and after 2 parallel slots. Without the dinput.dll (`# game settings: registry ...`): sound and music are muted in the registry for the run and restored afterwards (a backup in `work/` covers interrupted runs). `-Mute:$false`: the player's own settings.
- The log is moved to `work/autotest_<stamp><tag>.csv`.
- `-Scenario <name>` plays `Data\Scenarios\<name>.scn` instead.
- `-IniExtra` keys replace the same keys from `autotest.ini`.
- **Choosing the config.** Tests play with the shipped `config/aimod.ini` and `config/aimod_ai/` (what players get), never the player's own `../aimod.ini` / `../aimod_ai` (before 2026-10-18 every test read `../aimod.ini` - with the user's experiments on: hold_town=1, attack_retreat=1, frame_watch=35 - so older A/B results measured that file). Another file: `-Config <path>` / `-AiDir <dir>` on run_test.ps1, or the env vars `CONFIG` / `AI_DIR` for every script that calls it (scen.sh, suite.sh, h2h.sh, duels.sh ...), paths relative to `_aimod` or absolute: `CONFIG=config/test/hold_retreat.ini REPEAT=5 sh scripts/scen.sh ...`; `CONFIG=../aimod.ini` tests the player's own. A test ini needs only the keys it changes (a missing key gets the built-in default = the shipped value); put it under `config/test/` and commit it. run_test.ps1 writes them as `[autotest] config=` / `ai_dir=` into the test ini; the DLL reads every section ([general], [all], the difficulties, [display], [llm], [camera], [sound], [editor] ...) from that file and the personalities from that folder (saves/.aimod stay in the game folder), and logs `# config: <path> (personalities: <dir>)` at the start. `-IniExtra` / scenario variant keys still override single keys on top (cfg_int's `[autotest]` lookup). Normal play is unchanged: `aimod.ini` and `aimod_ai\` next to the game.
- One run per slot: `work/run_test.lock` (slot 0) or `work/run_test_slot<n>.lock` holds the owner's PID and, once started, its game's; a second run in the same slot refuses to start, and so does any run while a game runs that is no test slot's (the player's) - a game whose parent is a runner with a lock counts as that runner's (it has just started; its PID is not in the lock yet - before 2026-10-19 parallel starts aborted each other now and then with "game already running"). Only the owner removes its test ini; the sound is muted once for all slots running and restored by the last one.
- **Parallel runs (test slots).** `-Slot n` (1..30, or env `SLOT`) runs a game beside others: its test ini, log and dumps are in `work/slot<n>/` (the DLL gets `EEAIMOD_SLOT` / `EEAIMOD_SLOT_DIR` and logs `# slot: n (pipe eeaimod<n>, ports ...)`), its control pipe is `\\.\pipe\eeaimod<n>` (`SLOT=n python scripts/eectl.py ...`, ee_mcp.py too), its log ends up as `work/autotest_<stamp>_s<n><tag>.csv`. Two things kept a second game from running: the game's single-instance mutex (checked before our DLL loads - run_test patches the check in the suspended start whenever a slot or another test game runs; docs/engine-notes.md, Exe builds) and the engine's network ports, UDP 33334 / TCP 33335, which even a single-player game binds - the second game's `NEProtocol::Publish` failed and `NESynchronousServer::ProcessLocalUserInput` crashed on the missing record object (LLE +0x51D37, read 0). The DLL moves slot n's ports by 2n (`slot_ports`, eeaimod.c: the 8 immediates in `TCP Protocol.dll`, `# slot: network ports 33336/33337 (8 of 8 patched)`). `scripts/jobs.sh`: `JOBS=n sh scripts/suite.sh ...` (also scen.sh, h2h.sh) runs n games at a time in slots 1..n; without JOBS they run one after the other in slot 0 as before. 2026-10-19: 4 slots at once, 60 min each, all finished; a suite of 8 games (4 maps x 2, 20 min) took 23 s with JOBS=4 instead of ~70 s. A game in a slot plays as it would alone (the same `game_seed` gives the same game).
- **A run's own DLL.** `-Dll <path>` (env `DLL`, also through scen.sh / suite.sh / h2h.sh) plays that eeaimod.dll build instead of the installed one, so sessions can test their own builds side by side without touching the game folder: run_test copies it to `work/eeaimod_slot<n>.dll` (the source can be rebuilt while the game runs) and names it in `EEAIMOD_DLL`; the installed eeaimod.dll loads that file instead of installing itself (dll/eeaimod.c `run_other_dll`), the copy takes the game folder from the exe and logs `# dll: <path> (the test run's own, built <date time>)`. It can't sit in the game folder - the engine loads every DLL in there. The installed DLL must be one that knows EEAIMOD_DLL (from 2026-10-19; run_test refuses otherwise). A copy that doesn't load: `# eeaimod: EEAIMOD_DLL ... didn't load (error n) - nothing installed`, and the runner stops the game after 10 s - never the installed build in its place. Typical: build into a scratch file (`cd dll && $CC ... -o /tmp/x/eeaimod.dll eeaimod.c ...`, the flags of build.sh) and `DLL=/tmp/x/eeaimod.dll JOBS=4 sh scripts/suite.sh ...`. Checked 2026-10-19: a slot with -Dll beside a slot with the installed DLL, same game_seed: identical games.
- Safety: if the DLL doesn't report autotest mode within 10 s, the game is killed (no unmuted game with intros at normal speed).
- Fixtures: start a random map with the wanted settings (`start_epoch`, `citizens`, ...), let it run, `eectl.py save copper40` at the interesting moment, then run tests from that save.
- Saved state (.aimod, dll/persist.c): a setup line `@1.5 save zz_name` on a random map writes zz_name.ees + .aimod; `run_test.ps1 -Save zz_name.ees` then logs `# persist: state of ... restored: N blocks, 0 skipped` (2026-10-17: 37 blocks; a block with a flipped layout hash: `skipped (another layout)`; a format-1 file (saves made before 2026-10-17, e.g. a copy of the reference save with its .ees write time set to the one in the .aimod header - the original copy has another): `has format 1 (blocks matched by tag, size - no layout check)`, 36 blocks). Note: a save made this way stalls at its start time under `turbo=1` (also with the DLL before, also with save_state=0) - load it with `turbo=0`.
- Continental stalls (2026-10-18): 60-minute Continental runs (`islands` setup; 2 AIs from epoch 5, or 3 AIs from 0) stopped logging near the end and were killed at the timeout, or crashed at 0x628D6F / 0x53110D / 0x531131 / 0x62D5AC. The cause: the unit list sorted by x (docs/engine-notes.md, Units) broken by units the game gives back to its pool twice or while still linked - set off by the attack call-off at the safe point that `attack_retreat=1` makes (the user's own aimod.ini sets it; the tests read it). Then the moves (0x5310B3) or range searches (0x62FE01) loop for good. 24 runs each (12 seeds x both setups, `game_seed` 1-12, 300 s timeout): the DLL before the review (9425949) 2 failed, HEAD 865865e 6; HEAD with `attack_retreat=0`, 48 runs: 0; with the guard (attack.c unit_node_free, hook_unit_release), 48 runs (seeds 1-24): 0, the guard took ~1 double release every few games. Seed 8 of the 3-AI setup also ended with exit code 3 (no exception logged) on both DLLs without the guard.

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
| `seaop` (Small/Large Islands 101, 1 AI, e5/e9, 60 min) | the AI's navy against a human-like navy: the idle P1 builds up a fleet and uses it as the playtester does (raids on fishing boats and the coast, hunts warships, patrols for transports, lands soldiers) - see "Sea opponent" below |

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
- `# navy:` (autotest only, every 2 game minutes per AI, dll/navy.c): docks, Navy Yards, fishing boats, the food
  its boats took from the fish (fish left on the map), warships by type, submarines, carriers, transports, its
  warships in its own / an enemy's waters (the waters of the nearest building within 20 tiles), the enemy's warships
  in its waters, navy lost / sunk and fishing boats lost / sunk, income and stock. See docs/navy.md.
- `# fishing:` (dll/navy.c, `fishing_boat`): the fishing boat the AI trains when it differs from its last pick, with
  the one the game asked for (e9-e10: "the game asks for boat 211, trains 365").
- `# navy: P<n> map` (every game, once per AI): the water-map detection - islands / mixed / land, the strength (share
  of enemies across the water), the map's water share, the game's own water flag and +0xEFA. `# navy: P<n> budget`
  (every 5 minutes, water maps): the navy account, what it paid, the ship orders paid/refused by type, warships,
  docks (built, asked), fishing boats, the room held; `# navy: ... docks N of M - one more requested`.
- Head-to-head on the navy: `[autotest] v_navy=1` with `variant_players=<p>` and `navy=0`: only those players have
  the navy account (econ.c PV). The fleet mix the same: `navy_mix=0` with `v_navy_mix=1`.
- `# navy: P<n> mix at N min:` (every 5 minutes with `navy_mix`, water maps): the wanted shares by role (F frigates,
  L battleships, A galleys/catapult ships, S attack subs, AA cruisers), own fleet by role, the type per role, the
  enemy seen (by role, aircraft, carriers, coastal units that hit battleships x3+), what the game asked for and what
  was trained, and the changed / skipped / no building / next role payable counts. The 2-minute `# navy:` line has
  `roles`, `lost-roles` and `sunk-roles` (T = transports) for every AI. `# navy: ... mix type N (for M) not trained
  though payable - skipped 5 minutes`.

Scripts:
- `scripts/summarize.py <csv> [minutes]`: per AI timeline.
- `scripts/score.py <csv>...`: suite score.
- `scripts/econ.py [-s min] <csv>`: per AI economy timeline (stock, demand, targets, workers, army lines).
- `scripts/econsum.py [-m min] <csv>...`: economy per suite variant (income, soldiers trained, mine distances, rates).
- `scripts/churn.py <csv>...`: citizens changing resource and target volatility per variant.
- `scripts/navy.py [-v] <csv>...`: the `# navy:` lines per map group (docks, fishing, fish food, warships, losses,
  who holds the waters, landings, failed ship requests); `scripts/navy.py types <csv>` the ship roles from a run with
  `setup=_aimod/scripts/navy_types.setup`.
- `scripts/navy_ab.py [-split|-mix] <csv>...`: navy A/B per suite variant (warships, waters, losses, fishing, food,
  landings, kills); `-split` the AIs with/without the navy account, `-mix` with/without the fleet mix; a second line
  with the warships by role and lost/sunk by role.
- `scripts/epochs.py <csv>...`: epoch advances per AI and their delay after the planned attempt.
- `scripts/watch_deltas.py`: watchpoint hits grouped by code path.

## Control pipe

While the game runs, the DLL serves `\\.\pipe\eeaimod`, one command per connection. Commands that touch
game state run on the simulation thread. The pipe is open in autotest runs; in a normal game only with
aimod.ini `[general] control_pipe=1` (default 0 since 2026-10-17 - a command on one PC desyncs multiplayer).
Setup files (`setup=`) run their commands directly, not through the pipe.

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
| `seaop start P [level]` / `seaop` / `seaop ships` / `seaop off` | the sea opponent (dll/testopp.c, "Sea opponent" below): P1 at war with AI P and playing a navy against it from now on; the state; each of P1's warships (group, type, tile, hit points, goal, the AI's units within 10 tiles, in danger) and the danger zones |
| `objs P T` | every object of type T of player P: tile, hit points, goal (e.g. `objs 2 42`: farm fields, 1 hp = not built) |
| `build P bx by x y r` | player P's citizens within r tiles of x,y build P's building at bx,by (EEUGBuildRepair, the game's order) - with `found`: enemy builders at a tower |
| `techs P` | techs player P has researched, and which are new since the last call (with tech epoch and cost) |
| `watchtech P id`, `watchtype P T [off]` | hardware write-watch on a tech's researched byte / on a unit's field (default +0x2C, its type); the writer goes into the log |
| `found P T x y` | lay out an unfinished building of type T for player P (0x62EF8A, as the build manager does; pays its cost) - e.g. an enemy tower foundation next to an AI's gatherers |
| `hurt P T x y r dmg` | damage every object of type T of player P within r tiles of x,y as an attack does (target vf 0x70) - e.g. `hurt 2 42 128 128 256 1000` destroys P2's fields |
| `plan P`, `plan P retry T` | the AI's building plan (type:wanted) / type T counts as a dropped build-queue entry and is tried again now (the retry of dropped buildings) |
| `count P` | all units and buildings of player P by type (type:count), e.g. whether an AI has a Siege Factory (63) |
| `gridcheck` | how the unit grid holds the units, per family: units, those in 2+ cells, the most cells, those not in the cell of their own position. 2026-10-18 (Continental 3 AIs from epoch 5, Small Islands from epoch 9, at 10/25/40/55 min): only buildings (family 2, up to 4 cells, ~30% of them) and family 29 (up to 2) stand in several cells, every unit is in its own cell. So the grid iterator (dll/grid.c) visits each unit once in every loop now (once=1: its own cell, or the area's cell nearest to it - the same units as every entry, without the repeats); the G5 conversion had kept the double visits of 31 loops. Where it mattered: the summed defence strength of a beach (landing.c ld_defence: towers counted 2-4 times) and of an assault objective (assault.c as_enemy_power, assault=0 by default); elsewhere existence / nearest searches, single-cell units (soldiers, siege, citizens), threat_refresh's own per-tile dedupe, and test-command listings (targets, uinfo, move, stats engage/danger - a building listed or ordered twice). The siege escort loop (attack.c) on the iterator too. A/B HEAD 9941f9b vs the change, 20 seeds each, P2 / P3 averages: Small Islands e5 60 min - workers 85.2/86.2 vs 83.0/82.2, kills 68.3/91.2 vs 69.2/99.7, losses 82.9/89.0 vs 84.3/103.5, landings planned 10.9 vs 10.7, sailed 7.9 vs 7.1, ashore 7.3 vs 6.7, beach too strong 0.9 vs 1.0; Large Islands e9 - workers 38.6/42.4 vs 37.2/42.9, losses 39.8/60.3 vs 37.5/54.6, landings planned 5.9 vs 6.1, sailed 2.7 vs 2.9, ashore 2.1 vs 2.5, beach-off 0.3 vs 0.1; assault (assault=1) - P3 alive 19 vs 17 of 20, workers 69.6/53.7 vs 66.3/48.0, P2 razed 5.2 vs 5.0, objective strength seen 186 vs 185 on average; raid (unchanged code path, the noise) - workers 64.0/69.6 vs 66.2/72.4, P3 razed 12.5 vs 10.6. Within noise; no crash in 160 runs |
| `bqueue P` | the build manager's queues per epoch (building types; `$` = can be built now) |
| `drops P` | drop sites of player P (Town Center, Capitol, Settlement; `g` = granary) |
| `tech P id...` | tech records of player P: state bytes (+4 researched, +6 available), cost, effects |
| `rlist P` | research manager: pool (list -1) and scheduled techs per epoch (`*` = researched) |
| `basemap P [r]` | map of AI player P's base, 2 characters per tile: T tower, W wall, G gate, C town center/settlement, b building, c citizen, s soldier, m mine, o planned tower slot, digits = other players |
| `defence P [0\|1]` | defence layout off/on for one player (A/B scenarios) |
| `navy types A B [all]`, `navy table` | per type A..B (naval families, or all): family, hit-table target category +0x4C4, weapon kind +0x4C8, damage class, hit points, attack, range, area, reload, tech, and for naval types the cost and P2's stats record (speed, armour); the game's hit factor table 0x8D200C [category][kind] and the 6x6 class table. `scripts/navy_types.setup` dumps everything into the log, `scripts/navy.py types` makes the role table (docs/navy.md) |
| `tinfo P type...` | type data (family, hit points, attack, range) and whether player P can train/build it now |
| `chat [0\|1\|2]` | AI chat level (0 off, 1 debug lines, 2 also the 5-minute status); lines shown/dropped per player |
| `chatme text` | the local player types text into the chat box, as EEUIECESendMessage::vf1 (0x5C28D6) sends it: cheat code first (when cheats are on), then the chat line to all - scenario triggers on chat messages see it as typed (reloadtime.scn: `class+1` raised the class reload, the trigger's "OK" came back) |
| `trig [list\|desc]`, `trig on\|off\|reset\|fire T`, `trig effect N` | the scenario's triggers (docs/triggers.md): each with its state, conditions (type and key values: chat text, attribute and compare, tech, trigger, time) and effects (category and action); T = index or name (the editor's, e.g. `<Trigger0>`). `fire` runs all of T's effects at once (delays and random groups ignored) and marks it fired; `effect N` runs one effect. Test of 2026-10-17 on reloadtime.scn: all 8 triggers listed with their chat texts, `chatme class+1` fired trigger 0 (then off, fired), `reset` put it back - a scenario's triggers as test levers, switched on or fired at a set minute from a setup file |
| `stats table`, `stats types P [from to]`, `stats vs P T Q U`, `stats fight x y [r [T U]]` | dll/stats.c, the game's own damage: the class table; P's stats of every type (class, hit category, weapon kind, attack, hp, reload, range, area, the 6 armours); one T of P against one U of Q both ways (hit factor, damage a shot, per second, time to kill); the sides' soldiers and shooting buildings within r of x,y (only types T and U if given): units, hp, damage per second at each other (free fire, and at contact without what is inside a minimum range), seconds the outranged side walks under fire, who needs how long; `engage P x y r`: P's units there attack the nearest enemy they can hurt. Note: setup lines before the save's start minute run while it loads (no players yet) |
| `fight_estimate=1` | the defence decisions (muster, hold the town, attack retreat, citizen militia) compare sides with the stats estimate instead of summed hit points x attack. 2026-10-17, overrun and assault, 10 seeds each, the defender old vs estimate: overrun alive 4 vs 3 of 10, kills 49 vs 31; assault alive 8 vs 9, workers 31.5 vs 22.8, losses 53 vs 62.5 - no gain (fights of numbers where both agree; the estimate rates the defenders lower and musters more). A first batch of 4 seeds looked better (alive 1 vs 3, 3 vs 4) - these runs differ between builds even with the same seed: 4 seeds are noise. That estimate was not on the old scale: its time ratio follows the square law (twice the units: 4, hit points x attack: 2), so the thresholds moved (militia from 3 meant 1.73 x the numbers: it fired 460/1312 times against 260/249). Rooted (st_ratio, same seeds): overrun alive 6 of 10, kills 66.6, losses 66.9; assault alive 6, workers 18.0, kills 30.3, losses 51.4 - militia back to 284/187, but muster still fires 15/17 times against 7/6: the estimate is an open-field fight, and a defender at home has more (buildings draw fire, towers, attackers drawn into range, reinforcements). Next: measure that home advantage. Off |
| `bunkers` (default 1), `bunker P x y [r [load\|unload]]`, `stats garrison`, `stats typedump T [from]`, `scenarios/bombing_late.txt` | the Fortress (672, Copper Age, 350 wood) holds 40 like a transport (type +0x54C; the load goal EEUGLoadTransport on it, unload EEUGUnloadTransport) - a garrison is something else: citizens put into a building for good to upgrade it (type +0x550, its most: Town Center 15, Capitol 50, Settlement 5, Granary 8), no shelter. Citizens under fire (hurt within 6 tiles in 30 s, or raiders within 8) go into a fortress within 15 tiles, out after 45 s quiet; one fortress at the main drop site once aircraft are possible or 3 citizens were hurt, a second from 40 citizens - the foundation laid directly (0x62EF8A): the build manager's placement failed every fortress, and 0x555710 also wants a free builder; its wood held back from unit training. 2026-10-17, P3 off vs on, bombing at 35 min: 20 seeds - workers 57.4 vs 66.2, buildings lost 2.4 vs 1.4, games down to 4-28 workers 4 vs 2 (10-seed batches gave 72.6, 61.7, 52.2 against 53.1 - noise; a fix between: citizens still walking in when the fortress read empty lost the quiet release and stayed inside); bombing at 20 min, shelling, raid_ww1 - about even (the fortress up at 18-33 min: little wood at a WW1 start). The `# doing:` line (once a minute): what every citizen does | 
| `landing` (default 1 now), `landing_bombard` (default 0), `scenarios/islands.txt`, `islands_rich.txt`, `scripts/landings.py logs` | island maps (the playtester: islands are popular; on Small Islands the main island holds huge concentrated forces - bomb the way free with a big fleet, land a large force with a forward base, the ships near; quick, before catapults and cannons come). Small/Large Islands, Middle Ages / WW1 start, 3 seeds, 60 min: landings on - 4-12 planned, 1-8 ashore (9-120 soldiers), 0-7 buildings razed a side; off - 0-3 razed, fewer kills. Planned against weak beaches; at the meeting point the beach often needed 3-4x (the defender's army at home) - called off. landing_bombard: a beach too strong is shelled first - a fleet (half the beach's need in warship strength, 4-16, the type by the game's table against a tower; transports built meanwhile, their cost held back), 4 minutes of shelling at most, called off on new anti-ship units or half the fleet lost. Without money the fleet never comes (gold/iron income ~100 a minute, the army's orders failing already); given 3000 of each at minutes 10 and 25: fleets of 4-8 sail, the soldiers on the beach melt (1234 -> 72), one beach shelled free and landed, but the beach's tower survives (725 stays) and the landing is called off. Found on the way: the type table ends at 723 (garbage beyond); campaign ships (Golden Hind 611, tech 14xx) pass the trainable check - ordering one crashed the game; a wall arc into the water crashed the game's wall placement (now checked: ends and middle on land). 2026-10-17 later: `landing_keep_transports` (default 1) - the game's transport manager (0x48CC5D) took landing transports for its own crossings, half of the game's unload orders to them (393 of 794 in 30 runs): refused. Island games crashed 1-6 of 12 (exception 0xC0000002, _purecall - the mod refused a land subgroup's order to a unit it led, and the game used the deleted goal, 0x44AF96/0x44AFB5: that caller's goal is now taken, not given) and about 1 in 24 more through units left without a goal set (0x619FF9, 0x5F063A, 0x5FC4ED: guarded, the default set 5000). 48 games (12 seeds x Small/Large x Middle Ages/WW1) after: no crash; landings as before (planned, sailed, soldiers ashore within noise). The crash log now catches every 0xC... exception and lists the game's own return addresses in its stack scan. 2026-10-18, the game's other unload orders to landing transports, followed to the end (`# ldforeign:` lines; 48 runs, Small Islands e5 / Large Islands e9, 12 seeds each, twice): 0x445AAD (the transport subgroup's event handler) 216, every one refused at the subgroup's vf57 or deleted by its caller (empty transport) - log noise; 0x48D022 (the game's unload retry, EEComputerPlayer vf50) 5 - three the landing's own unload at its own point (phase 3/4, kept), two while boarding: a transport still holding an unload of the transport manager's crossing (given before the landing took it, resumed after its load goal) carried 8 and 12 soldiers 40-70 tiles off and put them ashore. Now while boarding (phase 2, wave boarding) a transport with an unload is moved off the shore like an idle one (`kept off the shore`), and the retry is given only while the landing unloads, at its own point (else deleted before it is given - its caller does not touch it afterwards). 48 runs after (24 seeds each): kept off the shore 22 times in 16 runs (4-12 aboard), no retry left to refuse, 0x445AAD 100 (all refused), no crash; per run Small Islands - landings ashore 7.67 vs 6.46, soldiers ashore 86 vs 73, planned 11.4 vs 10.4; Large Islands - ashore 2.50 vs 2.54, soldiers 24.7 vs 26.7 (within noise). 2026-10-18, landing_bombard rechecked (config/test/bombard.ini, islands_rich, Small Islands e5, `landing_trace=1` adds `# ldbomb:` lines - each warship's target, each tower's hit points and reach): the towers still survived - in 57 runs of 13+8 shellings of a towered beach, 5 halved, 2 destroyed. Why: the fleet was planned on type 251 (tech +4 set, but replaced by its upgrade 263 - never trainable again) and built of what it could train now: 255 / 254 (factor 0.4 against the tower 463 of 3125-4688 hit points) and 185 (factor 2, attack 11); the ships went for the nearest defence or warship to the water; the warship type stood at the population cap (67 of 89 tries); a fleet sailing past outer towers to one behind lost 3 of 5. Now: available = the tech record's own test (vf1), the type picked against the beach's strongest tower, factor 1+ only; the fleet = its warships at least half as good (others stay home), the missing ones wait for that type; the population room for the fleet held from the game's soldier orders; the towers within 16 tiles of the beach shelled nearest to the fleet first, all on one; 4 minutes from the fleet's arrival (7 from sailing). Fleets are 263s now; 40 runs: 17 towered beaches, 8 halved, 5 destroyed. A/B 20 seeds (islands_rich): bombard on (new) / on (old, 4952a48) / off - landings ashore 2.30 / 1.95 / 2.15 a run, buildings razed 8.55 / 7.90 / 6.75 (both AIs), ships lost 40.7 / 38.8 / 37.1 (marine units incl. transports), fleets 1.0 / 0.9, shelled free 0.30 / 0.25, kills 130 / 142 / 130; no crash. Within noise: still not worth turning on (about one fleet a game, most shellings end with the beach's soldiers still too many) - landing_bombard stays 0 |
| `train_room` (player mask, default all), the `# popcheck:` line (once a minute: cap, population, the AI's units in production, units ordered with no room by call site) | the mod's own training orders (AA units, siege, home guard, transports) skipped the room check the game's callers make (cap - in production - population; the buildings' start check 0x624DF8 the same): at the cap they were queued and paid for and could not come out - lowpop P3's "in production" ran to 56 once it had resources, 63 AA tanks (146) and siege units (105) ordered with no room, none from the game's call sites. Now cut to the room. 2026-10-17, lowpop, 20 seeds, P3 without vs with (P2 without): P3 kills 23.4 vs 28.2, losses 32.2 vs 28.9; P2's losses 22.3 vs 28.6. On |
| `bunker_reserve` (player mask, default all), `scenarios/lowpop.txt` | a reserve outside the population: units in a fortress don't count (0x556E11 adds them only with its second flag; verified: 7 citizens inside, population 3 instead of 10). Map full (population at the cap - 1): up to 10 free soldiers at home go in; population 4 below the cap with nothing loaded for a minute, or enemies within 20 tiles: everyone out. lowpop: Continental 101, WW1 start, unit_limit 60 (the game splits it: 20 a player, the AI's cap 30), both AIs given 3000 of each at minutes 1, 15, 30. 2026-10-17, 10 seeds, P3 off vs on (`bunker_reserve=8`): kills 21.3 vs 21.3, losses 23.1 vs 19.0, buildings lost 9.4 vs 9.5. A first version keyed on the training's room (cap - in production - population) hoarded: in production ran to 16-30 while nothing came out, 39 sat inside to the end (losses 14.4, kills 20.1). Of 153 load orders about 30 were inside at once. What held training back was the mod's own orders queued with no room (`train_room`); with that fixed, 20 seeds, P3 off vs on: kills 22.7 vs 28.8, losses 27.2 vs 23.2, P2's losses 21.7 vs 27.4. On (all players) |
| `citizen_threats` (default 1), `scenarios/shelling.txt`, `stats danger P` | citizens keep away from what hurts them: raiders_near reaches 27 tiles (artillery 12-14, ships 24), and where a citizen lost hit points (every 4 s, not hunters) the spot counts 2 minutes - food and wood sites within 6 tiles from 1 such spot, stone, gold and iron from 3; the searches refuse them too. `stats danger 3` on the reference save: 53-75 of P3's citizens under the player's aircraft, raiders_near saw 0-5. Shelling: 4 field artillery + 6 machine gunners at P3's base edge from minute 15 (stats engage). 2026-10-17, 10 seeds, P3 off vs on: shelling alive 5 vs 7, workers 7.1 vs 17.6, losses 14.3 vs 10.8, buildings lost 6.1 vs 3.9; bombing workers 42.9 vs 54.7, losses 25.5 vs 21.0. Idle citizens after minute 15: shelling 6.9 vs 8.6, bombing 2.9 vs 6.3 - sent back with no safe site of their resource left. On |
| `aa_ahead` (default 1), `scenarios/bombing.txt`, `stats engage P x y r T tx ty` | AA before the aircraft come: from World War I, once an enemy is there, an AA building at each of the 3 drop sites nearest the base point without one, 2 AA units; aircraft seen anywhere count (half), kept 10 minutes. The scenario gives P2 an airport and aircraft (type 71, the player's on the reference save; 72 is a fighter that hits aircraft only) that attack whatever of P3's is nearest within 15 tiles of its base every 20 s and refuel on their own (without an airport they crash in about 3 minutes). 2026-10-17, 10 seeds, P3 reactive vs ahead: 25 aircraft + 4 bombers (79) - workers 36.1 vs 52.2, buildings lost 4.3 vs 1.2, losses 24.7 vs 18.8, kills 8.8 vs 18.0; 12 aircraft (7 losses more than none) - workers 65.1 vs 56.7, buildings 0.7 vs 0.8. After the AA count fix (each unit once - grid_each; the cells were counted by hand, a building in several cells counted again), same 10 seeds: workers 46.1 vs 48.7, buildings lost 1.7 vs 0.5, losses 20.1 vs 19.0, kills 13.3 vs 12.0 (other changes of the day in between too; workers about +-4 noise). Rechecked 2026-10-18 (f73cb77), 20 seeds each, P3 reactive vs ahead: bombing - workers 44.5 vs 44.1, buildings lost 1.7 vs 0.8, losses 23.0 vs 22.1, kills 11.2 vs 15.2, aircraft left after 10 min 14.7 vs 14.2; bombing_late (the aircraft at 35 min, 50 min) - workers 63.0 vs 58.5, buildings lost 1.6 vs 1.5, losses 36.8 vs 41.3, kills 31.5 vs 33.8. What holds: about half the buildings lost and more kills when the aircraft come early; the workers' gain of the first measurement is gone, and with time to build (late) it makes no difference. On |
| `militia_estimate=1`, `scenarios/raid_ww1.txt` | the citizen militia by the stats estimate in every epoch, as a fallback: not when own soldiers within 12 tiles win on their own; enemies without soldiers (builders, a foundation) when the citizens win, soldiers when they win clearly (1.5), or when caught (the soldiers attack one of ours and are faster) and they still win (`militia_caught_share` 1; 0.33 had 3 citizens attack a Mk V tank 42 times: 0.2 of their damage gets through). 2026-10-17, 10 seeds, P3 old vs estimate: assault (Bronze) alive 8 vs 10 of 10, workers 31.5 vs 29.2, kills 56 vs 61, losses 53 vs 61; raid_ww1 (6 doughboys, 2 Mk V at P3's base edge, `stats engage`) alive 10 vs 10, workers 41.9 vs 39.9, losses 3.9 vs 2.3 - the raid too weak to tell. Old fights nothing after the Bronze Age but builders; the estimate fought single machine gunners and A7Vs in WW1 - right: WW1 citizens have 264 hp and 15 a hit, and `EPOCH=9 sh scripts/duels.sh "121 1 32 3 4 3" "91 1 32 3 4 3" "204 1 32 3 4 3"`: 3 citizens beat a machine gunner, an A7V and a doughboy each, losing at most one. A hard epoch limit (Copper) left the defender worse on assault (alive 7, workers 18.5, losses 82). Off |
| `stats bench N x y [r]`, `scripts/battle200.setup`, `scripts/battle.py log [det]` | the estimate at scale (dll/stats.c works by unit type: types x types, area damage from a count per tile). Timing 2026-10-17: 1000 v 1000 0.026 ms, 2000 v 2000 0.048 ms, the unit scan ~0.03 ms. battle200 (200 v 200 mixed, both sides engaging; 5 runs): P3 won 4 (29-101 left), P2 1 (108 left); the estimate at contact P2 1.2-1.9, about even after a minute - close, as it was, but leaning to the side that mostly lost. With every neighbour of a target counted, P2's 20 artillery made 60% of its estimate (P2 realised 11% of its estimate, P3 25%): area damage now counts at most 2 units beside the target (the artillery duel still right). An AI's own orders spoil such tests: P2's AI sent the spawned army off on an attack until both sides got `stats engage` |
| `scripts/duels.sh ["A na B nb [gap [C]]" ...]`, `scripts/duels.py logs` | duels on Continental 101 (P2's A against P3's B, ~25 s a run), the estimate against the fight. 2026-10-17, six WW1 duels: the winner right in 5 (doughboys v machine gunners, Mk V v machine gunners, doughboys v A7V, mortars v doughboys, the mirror); hp x attack had the doughboys beating the machine gunners 3.3:1 (they lost 10:0) and doughboys v A7V even (lost 10:2, the tanks unhurt). With 2 s on every shot the winner's time is 1.0-1.7x the real one (the walk into range). Missed: 5 Mk V v 5 57mm AT guns, estimated 1.2 for the guns, the tanks won 5:1 - the guns did 40% of their estimate (the tanks 100%). The mirror (10 v 10 machine gunners) ended 1:10 - position matters as much. Range (the user: an outranged unit can be killed before it fires; artillery is done once something gets close): the estimate adds the walk of the outranged side under fire, drops a unit that cannot move against one that outranges it and, at contact, a unit inside its minimum range; area damage counts the neighbours of each target. Duels with a gap: 3 artillery against 8 machine gunners attacking from 20 tiles - the artillery won with 643 of 675 hp (estimated: without area damage the machine gunners, with it the artillery); 6 machine gunners (range 6 there) against a WW1 tower (7) - the tower won, its estimate too; on the reference save the player's machine gunners reach 7.2-8 and outrange it (in a test they stood at their range and took nothing while the tower lost 16 hp/s) |
| `aigroup P [x y] [nodef]` | AI P forms an attack group as the scenario effect "Create AI Military Group" does (docs/triggers.md): the game's own EECPAttackGroup with a target - the enemy building nearest x,y (default: the centre of P's soldiers; nodef: not a defence), mission 7 (attack the goal unit), P's free soldiers (in no group). Test of 2026-10-17, refcamp.setup with `aigroup` instead of `attacknow` at 75.9 (the reference save, deterministic: 3 runs each the same): the subgroups took the goal (mission 7) and marched straight - into the camp's push, 13 engaged at 93,107, the land subgroup gone by 79. P3 killed of the player 5-10 soldiers and 21-24 aircraft (the game's own attack: 20 and 31), lost 77-81 soldiers, 57-60 citizens, 42-54 buildings (66, 49, 26). A target alone makes the game's groups worse: they lack the strength check and the way around the camp, not the goal |
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
- Frame watch (`[display] frame_watch`, default 0 = off since 2026-10-17, e.g. 35 fps; autotest `frame_watch=N` also runs it headless): in the
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
  blocks (~4 a frame) holding the same DC and font handle, linked in a list. Autotest `alloc_trace=<bytes>` (removed 2026-10-17 - in git before that)
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

### Food on water maps (navy stage 0)

2026-10-18, `fishing_boat` (newest available fishing boat), the first granary without fields (`more_granaries`)
and no land soldier for a refused ship (`unit_substitution`), HEAD 325e4fb vs these, shipped config, 12 seeds,
60 min (`REPEAT=12 sh scripts/scen.sh 60 islands "random_map=<map>,map_size=0,ai_count=2,seed=101,start_epoch=<e>"`,
`scripts/navy.py` and score.py on the logs). Per AI, food gathered in the hour / workers at the end / soldiers:
Small Islands e9 2662 / 41.5 / 12.2 -> 8625 / 79.7 / 16.6; Large Islands e9 2604 / 40.8 / 12.4 -> 9587 / 82.5 /
19.6 (fish 3708 / 2398 of it, 1.5-1.7 granaries, 13 citizens on food; before 0 boats, 0 granaries). e5: Small
11137 -> 12137, Large 12373 -> 12852, workers, epoch and kills within noise. Continental e5 (6 seeds): food 10441 ->
11620, workers 67.3 -> 70.0, epoch 5.42 -> 5.33 (std. error 0.15). 54 runs each side, no crash. Table: docs/navy.md
stage 0.

### Navy account (navy stage 1)

2026-10-19, `navy` 0 vs 1 (same DLL), shipped config, 2 AIs, seed 101 map, 60 min, 20 seeds
(`REPEAT=20 sh scripts/scen.sh 60 islands "random_map=<map>,map_size=0,ai_count=2,seed=101,start_epoch=<e>" "b:navy=0" "a:navy=1"`),
plus head-to-head (`"h3:navy=0,variant_players=3,v_navy=1" "h2:navy=0,variant_players=2,v_navy=1"`, 10 seeds) and
Continental e5 (6 seeds); scored with `python scripts/navy_ab.py [-split] <logs>`. Both AIs on: warships at once 2.6-3.8 -> 3.6-4.6, docks 1.4-2.1 -> 2.9-3.2, navy lost
7-11 -> 9-15 (std. error ~0.7), landings sailed 1.8-3.5 -> 1.3-1.9 (0.2-0.3), kills Small e5 76 -> 67 (4), Small
e9 55 -> 43 (3), Large within noise; soldiers Large e5 28.2 -> 23.9 (1.0), Small e5/e9 -1.9/-2.6 (1.3). Head-to-head
the AI with the navy has the better kill/loss ratio in all 8 slot pairings, more fish food in 8, fewer boats lost
in 7. Continental: detection "land" in 12 of 12, no account, results within noise. 172 + 80 + 40 (share 15) runs,
no crash. Tables: docs/navy.md stage 1.

### Fleet mix (navy stage 2)

2026-10-19, head-to-head (`navy_mix=0` + `v_navy_mix=1`, both with the navy account; Small/Large Islands e5/e9, seed
101 map, 60 min; `scen.sh` "h3:...variant_players=3" / "h2:...variant_players=2" 10 seeds, then `h2h.sh` seeds 11-30,
and for the second version `h2h.sh` seeds 1-30). Scored per game: the mix AI minus the other.
- First version, 160 games: kills - losses -8.6 (std. error 2.3), warships sunk - lost +0.5 (0.5), ship orders paid
  -1.1 an hour.
- Next payable role, 120 games: -7.5 (2.9), sea exchange +0.5 (0.5).
- Both AIs (Small Islands e5/e9, 20 seeds, `JOBS=8`, navy_mix 0 vs 1): all within noise.
- No crash in 360 runs. `navy_mix` stays 0. Tables: docs/navy.md stage 2.

### Fleet (navy stage 3)

2026-10-19, `navy_fleet` (dll/fleet.c), off by default.

**Setup:**
- Head-to-head, one AI leads its fleet: `h2h.sh` with `"v_navy_fleet=1"`; with the fleet mix too: `"v_navy_fleet=1,v_navy_mix=1"`.
- The control games: `"v_navy_fleet=0"`, the same seeds.
- Maps: Small/Large Islands e5/e9 (`"random_map=<map>,map_size=0,ai_count=2,seed=101,start_epoch=<e>"`), 60 min.
- Seeds 1-32 (`SEED0=16` for the second half), `JOBS=8`, the run's own DLL (`DLL=`).
- Both AIs with the fleet: `scen.sh ... "<name>:navy_fleet=1"`, seeds 1-16.

**Scoring:** `python scripts/fleet_ab.py [-strip n] -ctl <ctl> -h2h <variants> -both <variants> <logs>`.
- Run names are `<variant><map>`, e.g. `flSe5` (h2h.sh `NAME`, scen.sh variant names); `-strip 1` drops a one-letter run prefix, so rounds with other seeds pool.
- It pairs each game with the control game of its seed:
  - *seat*: the variant seat minus the same seat in the control game;
  - *DiD*: (seat - other) minus that difference in the control game;
  - *both*: per AI, the variant game minus the control game.
- Control games played identical with 3 different builds (the fleet off).

**Log:** `# navy: P<n> fleet at N min: <task> (<why>) | at x,y -> target guard x,y | ships, hp | ratio vs known, near | danger <zones> | lost/sunk warships, boats | pulled flees hunts retreats defends escorts | repaired | orders refused sea-updates-skipped maint-skipped | attack groups`.

**Test keys:**
- `navy_fleet_boats=0`: no fishing boats pulled back;
- `navy_fleet_coast=0`: no shelling of the coast in a hunt;
- `navy_fleet_raid=<percent>`: the strength ratio for raids on boats, the coast and the fishing grounds (130).

**Results** (128 games per variant):
- Fleet AI, seat: warships sunk - lost +1.9 (std. error 0.45; DiD +3.9 (0.9)), positive on all 4 maps. Own warships lost -1.4 (0.4).
- Kills - losses -4.5 (3.4), DiD -10.2 (6.2). Large e9 alone: -14.9 (6.1).
- Fish food -47 (143), fishing boats lost -0.6 (0.7), soldiers +0.2 (0.5).
- With the mix: sea +2.1, kills - losses -6.3 (3.8).
- Raids at ratio 1.0 instead of 1.3: no better.
- Both AIs: kills/losses -4.7/-4.5, soldiers +1.8 (0.6), fish +288 (173), boats lost -1.2 (0.8).
- About 1000 runs, no crash.
- `navy_fleet` and `navy_mix` stay 0. Tables: docs/navy.md stage 3.

### Fleet in task forces (navy stage 4)

2026-10-19, `navy_fleet=2` (dll/fleet.c, navy.c); on by default since stage 5 (below). Details and tables: docs/navy.md stage 4.

**Setup:**
- Head-to-head: `h2h.sh` with `"v_navy_fleet=2"`, the control `"v_navy_fleet=0"`, the same seeds; Small/Large Islands
  e5/e9, seed 101 map, 60 min, seeds 1-32, `JOBS=6 SLOT0=20` (scripts/jobs.sh: `SLOT0=k` plays in slots k+1..k+JOBS, so
  two sessions don't share slots), the run's own DLL. Score: `fleet_ab.py -strip 1 -ctl ctl -h2h tf <logs>`.
- Sea opponent: `scen.sh 60 seaop "random_map=<map>,map_size=0,ai_count=1,seed=101,start_epoch=<e>" "fzSe5:navy_fleet=0"
  "fhSe5:navy_fleet=2"` (and `seaop_level=1|3`), score `seacontrol.py -ctl fz`.
- Keys: `navy_fleet_pop` (15: the target, % of the population cap x the map strength), `navy_share_max` (50).

**Log:** `# navy: P<n> tf <force> at N min: ...` per task force; the fleet line's `target built refused | haven ... |
forces ... | lost by force ... | by task ... | kills W B T C Bd O`; `# navy: P<n> at N min: the fleet trains a warship`.

**Results:**
- Head-to-head, 128 games: kills - losses +18.3 (4.2), DiD +38.8 (7.8); losses -15.8 (2.6); warships afloat +5.5,
  most +12.0 (fleet AI: 5-9 on average, 12-21 at most, 18-31 trained by the mod an hour); sea exchange -0.4 (0.65);
  own fishing boats lost -3.4 (0.9), enemy's sunk +1.9; fish food +1110 (181); enemy transports sunk +1.8; soldiers
  +3.7 (0.7); buildings lost -1.6 (0.5). Large e9 alone -10 (8).
- Sea opponent (paired, fleet - control): medium (10 seeds) kills - losses +9 to +27 per map, sea exchange -4 to -10,
  fishing boats lost -3 to -6; weak (6) sea exchange -9 to -16 and kills - losses -17 to -39 on 3 maps; strong:
  sea exchange -1 to -9.
- Both AIs with it (8 seeds, Small e5 / Large e9): fewer kills and losses, workers +4.5/+8.5. Continental: identical.
- About 1000 runs, no crash.

### Against the base-game AI

The playtester's benchmark: the base game's AI (cheating, brain-dead nonstop waves) is easy for a human - so it should
be for the mod's AI. `vanilla_players=<digits>` makes those players the game's own AI in the same game as the mod's
(dll/eeaimod.c `is_vanilla`): every per-player tick of the mod skips them (only its log lines stay - battle, army,
gather, spend, attack times), every hook that acts for or on an AI player passes straight through to the game's
original, no setting, personality, cost_percent or test epoch timer touches them, and the mod's diplomacy treats them
as a human it never decides for (no alliance offer, no capitulation to them). The engine guards (pure crash fixes)
stay on for everyone. Their difficulty and cheat: `difficulty=hard` (every AI; tests ran at medium before - a
load-game start never sent the game options, randmap.c now writes 0x918C14), `vanilla_difficulty`, `vanilla_cheat=1`.
The game's Cheat is a price cut (AdjustCost, vf27): **hard 50%, medium 75%, easy 100%** of every price - "hardest" is
hard with the cheat. Each AI's first update logs it: `# cheat: P3 (vanilla) difficulty 2, cheat 1: a price of 1000
costs 500` / `# cheat: P2 (mod) difficulty 2, cheat 0: ... costs 1000`.

```sh
JOBS=6 REPEAT=20 NAME=cheat-si5 sh scripts/vanilla.sh 60 \
    "random_map=Small Islands,map_size=0,seed=101,start_epoch=5,difficulty=hard" "vanilla_cheat=1"
```

1v1, each seed twice (vanilla P3 / mod P2, then swapped); `scripts/vanilla.py` scores wins (a decided game, else who
still has workers), kills, losses, razed / lost buildings, workers, epoch, soldiers, warships, ships lost / sunk, fish
food, the mod's landings ashore. Check that nothing of the mod reaches a vanilla player: 4 all-vanilla games
(`vanilla_players=23`, Continental e0 and Small Islands e5, game_seed 1-2, 40 min) with the shipped config and with
`config/test/vanilla_flip.ini` (34 of the mod's keys flipped): every log row and battle line identical. (`[general]
enabled=0` can't be compared: it installs nothing, not even the test start.)

2026-10-19, shipped config, hard, 60 min, map_size 0 / seed 101, game_seed 1-20 x both seats (2 islands games lost to
a start collision with another session's game). Wins: a decided game - the mod's AI resigned (surrender, no citizens
left) or the vanilla one was wiped out; "standing": both alive at 60 min. Mod - vanilla per game (standard error):

| Map | vanilla | mod won / vanilla won / standing | kills | losses | workers | epoch | ships lost | landings |
|---|---|---|---|---|---|---|---|---|
| Continental e0 | cheat | 0 / 12 / 28 | +40.6 (10.4) | -47.3 (11.8) | -21.8 (7.9) | +0.47 | +0.6 | 0.8 |
| Small Islands e5 | cheat | 0 / 20 / 20 | -5.1 (4.4) | +16.7 (6.0) | -42.7 (4.4) | -0.20 | -11.9 (1.6) | 0.6 |
| Small Islands e9 | cheat | 0 / 16 / 23 | -5.0 (2.9) | +5.6 (4.3) | -35.8 (5.9) | -0.26 | -8.8 (1.5) | 0.2 |
| Large Islands e5 | cheat | 0 / 15 / 24 | -25.3 (3.9) | +27.8 (4.9) | -42.8 (4.8) | -0.41 | -1.6 (1.2) | 0.2 |
| Continental e0 | no cheat | 22 / 6 / 12 | +55.0 (6.7) | -55.1 (5.7) | +33.5 (9.8) | +0.82 | +0.3 | 0.8 |
| Small Islands e5 | no cheat | 1 / 3 / 36 | -1.7 (5.0) | +6.1 (7.6) | -14.1 (5.7) | -0.33 | -4.7 (1.3) | 1.2 |
| Small Islands e9 | no cheat | 0 / 2 / 38 | -0.5 (4.9) | +18.9 (4.7) | +8.1 (5.9) | -0.40 | -9.0 (1.1) | 0.4 |
| Large Islands e5 | no cheat | 1 / 1 / 38 | -10.8 (7.8) | +8.7 (6.2) | +3.8 (5.9) | -0.47 | +0.9 (0.9) | 1.2 |

Ships lost: the mod's minus the vanilla's (negative: the mod lost fewer); landings: the mod's landings ashore per game.
The mod's AI never beat the cheating AI: with half prices the vanilla one has twice the workers (80-88 against 42-45 on
the islands) and 4-6 times the soldiers on the islands, and its early waves kill the mod's citizens - every lost
Continental game ended with the mod resigning at 23-50 min after 27-43 citizens were killed and none of its soldiers
(`# surrender: ... destroyed 1 buildings, 43 citizens, 0 soldiers`). Without the cheat the mod's AI wins Continental
(22 of 40, kills - losses +110 a game) but on the islands it only holds: more losses, an epoch behind, though it sinks
more ships. Where to work: the economy's size (workers) and keeping the citizens alive against early waves on land;
on the islands the land war after the crossings.

### The early game against the base AI (docs/economy.md)

2026-10-19: towers first, the bait, the careful militia (`early_towers`, `tower_bait`, `early_militia`: A), the early
army (`early_army`: B), `copper_plan=2` (C), the alternative `early_soldiers=4`. Each one is an `[all]` key. They
were run through the scratchpad runner (scripts/vanilla.sh per variant x cheat x map, tags `ab-<variant>-c<cheat>-<map>`)
and scored with `scripts/eco_ablate.py -base <base variant>`. `# early:` once a minute (autotest): towers by the Town
Center finished/all, all towers, builders sent, bait orders, militia refused, soldiers, the army's and the epoch's
accounts. The new DLL without the keys played the very same games as the build before (20 games, Continental with
the cheat). Continental with the cheat, 40 games: the base AI's wins went 12 -> 4 with A, kills - losses +32 (10),
citizens killed -7.7 (3.7). Without the cheat: 6 -> 0, citizens killed -14 (3.3). On Small / Large Islands e5 the
games were identical. A is on by default. Head-to-head against the control (24 games, Continental e0, 4 AIs): B - A alive
+0.08 (0.04), kills - losses +16, workers +4.8 (2.1). B, C and the alternative, and why they are off: docs/economy.md.

### Food and mines against the base AI (docs/economy.md "Food and mines")

2026-10-20: `scripts/eco_ablate.sh` with `MAPS="cont si5 li5"` (Large Islands e5 added) and `CHEATS`, scored with
`scripts/eco_ablate.py -base <name>` and `scripts/eco_food.py -base <name>` (food gatherers by source, hunt list and
animals, forage, fields, mine slots and gatherers, epoch-up minutes, paired differences). Give each run's base
variant its own name - older `ab-base-*` logs from other builds match the same glob (pass the files explicitly).
`food_plan=3` (PLAN + PULL) is on: Continental e0 the first epoch 7.7 / 8.7 minutes earlier (no cheat / cheat),
workers +8.7 / +6.7 at minute 30, food gatherers ~17 (the base AI's level), mod wins 13 -> 22 without the cheat;
islands within the noise; head-to-head (24 games) no regression. HUNT, NEAR, WILD, CAMP and `mine_fill` stay off
(tables in docs/economy.md). # food: / # mines: lines every 5 minutes (autotest) count what the features did. The
fix in militia_answer (53f8ced) came out of these runs: 2 of 240 games crashed there.

### Citizen production against the base AI (docs/economy.md "Citizen production")

2026-10-20: `# cit:` once a minute (autotest, dll/citizens.c): per producer (Town Center, Capitol) and second of the
minute `cit` / `other` / `idle`, the idle seconds by reason (`cap` no room, `target` the citizens wanted there,
`pfood` under 50 food - the game's own check, `pbud` the mod's budget refused, `batch`, `att`), the budget's
citizen requests (`req`, `blocked` by food / epoch reserve / other), the game's `wanted` / `have` / `room` / `cap`,
food stock / gatherers / target, workers and the plan's `goal feed slots home extra`. `scripts/cit.py [-base NAME]
"work/autotest_*_ab-<name>-*.csv"` scores it per side and phase with the paired differences (workers, miners, metal
income, soldiers, kills - losses, alive). Runs: `scripts/eco_ablate.sh` with `MAPS="cont si5 li5"`, 20 seeds
(`SEED0=10 REPEAT=10` adds seeds 11-20 to a run of 10). `citizen_plan=3` is on: workers +8 to +35 at 60 min,
miners +2 to +8, the base AI with the cheat won 31 of 120 games instead of 45; head-to-head (24 games) workers +24,
razed +6, losses +10.7 (6.1); lowpop (`h2h.sh 60 lowpop "...,unit_limit=60" "v_citizen_plan=3"`, 20 games) kills -
losses +23. Tables: docs/economy.md.

### Economy into army against the base AI (docs/economy.md "Economy into army")

2026-10-20: `# mil:` once a minute (autotest, dll/army.c, both sides): `stock` F W S G I, `acc` the military account,
`sold siege ships air` and `value` (prices without the Cheat), `bld` the finished Barracks, Archery Range, Stable, Siege
Factory, Dock, Navy Yard, Airport, Tank Factory, Cyber Factory, Fortress and `busy` (5-s samples training / all),
`towers walls` (finished), `tech up7 cost` (researched techs, line upgrades and their price), `wish` (soldiers in the
unit manager's request lists), `pop cap room`, this minute's military orders (`calls asked trained`, refused by
`budget` / `poor` (the stock can't pay) / `room` / `game` (no building, not researched), `subs`), the budget's short
resources (`short`), the reserve a land soldier sees (`res`) and what army_plan did (`plan float fill bld mix`).
`# pool:` at minutes 30 and 50: the research manager's candidate techs. `scripts/milspend.py [-base NAME]
"work/autotest_*_ab-*.csv"` scores them per side, at 20/30/45/60 min, with paired differences. Runs:
`scripts/eco_ablate.sh` with `MAPS="cont si5 li5"`, 20 seeds. `army_plan=7` (and 15, 16, 23) measured, **off**:
Continental soldiers +4.5 / +5.3 at 60 min, kills - losses -0.8 / +7.0, head-to-head (`h2h.sh 60 late
"random_map=Continental,seed=101,map_size=1,ai_count=4,start_epoch=0" "v_army_plan=7"`, 24 games) losses +14.4 (5.8),
kills +1.1; lowpop (`h2h.sh 60 lowpop "random_map=Continental,map_size=0,ai_count=2,seed=101,start_epoch=9,unit_limit=60"
"v_army_plan=7"`, 20 games) kills - losses +45.

### The army as one against the base AI (docs/economy.md "The army as one")

2026-10-20: `# mass:` once a minute (autotest, dll/mass.c, both sides; the scorer reads it from the vanilla and the
head-to-head logs): `sold iso away isoaway eng engiso` (land soldiers, alone - 2 or fewer own within 6 tiles -, away -
25+ tiles from the base -, both, in a fight, alone in it; means over 5-s samples), `sg out asg strag` (land attack
subgroups, those out, their members, stragglers 12+ tiles from their subgroup's middle), `died iso away isoaway outn`
(land soldiers killed this minute - alone, away, both, outnumbered 2:1 within 8 tiles), `held released refused
holding` (GATHER), `waits waited goes` (WAIT; seconds). `# mass: P<n> at M min: <k> reinforcements go together` and
`the attack goes after ...`. `scripts/mass.py [-base NAME] "work/autotest_*_ab-*.csv"` (vanilla logs: both sides,
paired kills - losses, kills, losses, razed, lost, workers, soldiers, deaths alone / outnumbered, alive, wins),
`scripts/mass.py -h2h "work/*_h2h<stamp>_<name>_*.csv"` (B - A per game). Runs: `scripts/eco_ablate.sh` with
`MAPS=cont`, 20 seeds (`SEED0=10` for 11-20); head-to-head `h2h.sh 60 late "random_map=Continental,seed=101,
map_size=1,ai_count=4,start_epoch=0" "v_army_mass=1"` (24 games) and `h2h.sh 60 lowpop "random_map=Continental,
map_size=0,ai_count=2,seed=101,start_epoch=9,unit_limit=60" "v_army_mass=1"` (20). The build with the measurement and
`army_mass=0` played the same games as the one before (20 checked). `army_mass` 1, 2, 4, 7, 9 measured, **off**:
GATHER (1) kills - losses +11.0 (3.4) without the cheat, -9.7 (8.0) with it (kills -17), losses -4 / -7; head-to-head
kills +9.0 (3.6), losses +1.3; lowpop kills +9.6, losses -6.9. Tables: docs/economy.md.

The attack as one front (2026-10-20, docs/economy.md "The attack as one front"): `# mass2:` once a minute
(autotest, both sides): `form march fight other` - per subgroup state class the members and their stragglers at
home / ahead of / behind the middle (means over the 5-s samples), `fronts multi` (out subgroups 20+ tiles from each
other; share of samples with 2+ out), `orders` moves/attacks per class (the subgroups' vf57 orders), `tethered
regrouped joined` (army_mass 16 / 32 / 64), `died pack ahead behind home none` (where this minute's land soldiers
killed stood). `scripts/mass.py -states "work/autotest_*_ab-<name>-*.csv"`. Runs as above (`eco_ablate.sh`, `MAPS=cont`,
20 seeds, the 144 cells again with `SEED0=20`; `h2h.sh` with `v_army_mass=144`, `SEED0=24` for games 25-48). Give
each variant a name of its own: `eco_ablate.py` / `mass.py` pair by name, seed and seat, and an older run's logs with
the same name (e.g. `fbase` of 2026-10-19) are taken in as well. TETHER (16, 128), REGROUP (32), JOIN (64) and their
mixes with GATHER measured, **off**: 16 losses -7.4 (2.8) / -4.8 (4.0) but kills -13.9 (5.6) against the cheat; 144
(80 games a cell) kills - losses +2.6 (1.5) / -2.5 (3.5), head-to-head (48 games) losses +6.0 (4.0), lowpop neutral;
GATHER + any of them like GATHER. The builds with `army_mass=0` played the same games as 007044a (the final build 80 of 80, 007044a's own DLL 40 of 40); a rerun
of 144 played all 80 games the same; Small Islands e5 games with 144 identical to without (8 checked). Seed 2 / seat 3 of
Continental e0 without the cheat parted at minute 43 between two runs of the same build: such a late split happens
now and then (it shows as 39 of 40 "same").

### Mass before striking, the mines (navy stage 5)

2026-10-19, `navy_fleet=2` with `navy_fleet_mass=1` and `navy_mines=1` (dll/fleet.c fl_rally, navy.c navy_nodes /
navy_mines), on by default. Details and tables: docs/navy.md stage 5.

**Setup:**
- Sea opponent: `scen.sh 60 seaop "random_map=<map>,map_size=0,ai_count=1,seed=101,start_epoch=<e>[,seaop_level=1|3]"
  "fz<map>:navy_fleet=0,navy_mines=0" "fm<map>:navy_fleet=2" "fi<map>:navy_fleet=0"`, `JOBS=6 SLOT0=20`, the run's own
  DLL; score `seacontrol.py -ctl fz`.
- Head-to-head: `h2h.sh` with `"v_navy_fleet=2"` against `"v_navy_fleet=0"` (NAME `ntf<map>` / `nctl<map>`), seeds 1-32;
  `fleet_ab.py -strip 1 -ctl ctl -h2h tf`. The mines: base `navy_mines=0`, `"v_navy_mines=1"` against `"v_navy_mines=0"`.
- Keys: `navy_fleet_mass` (0: stage 4 with the shelter), `navy_mines`, `v_navy_mines`.
- A test script's `tasklist | grep empire` check also sees other sessions' test games (slots with a lock in work/) -
  check the command line / locks before calling it the user's game.

**Log:** the fleet line's `| rally x,y towers n massing|striking (massed n, struck n, towers asked n)` and the task
`mass`; `# navy: P<n> at N min: no tower covers the fleet's sea - tower T queued at x,y by the dock x,y`;
`# nodes: P<n> at N min: home island S/G/I worked/known (gatherers of slots, near, far) | elsewhere S G I | drop sites |
mines pull n (wanted G I)` every 5 minutes with the navy budget line.

**Results:**
- Sea opponent, medium (10 seeds a map): sea exchange +24.9/+14.6/+2.8/+6.5 (Small e5/e9, Large e5/e9; control
  +1.1/+6.7/-7.9/-3.2), kills - losses +31 to +48 better (paired), warships afloat 3-8 on average, 10-19 at most. Weak and
  strong: kills - losses better in all 8 cells; sea exchange better in 6, weak Small e9 -3.9 (4.1), weak Large e5
  -0.3 (7.6). Strong Large e5: defeated 0.5 of the games against 0.3.
- Head-to-head, 123 games: kills - losses +23.7 (3.7) (stage 4: +18.3), every map positive (Large e9 +24.7, was -10);
  losses -22; warships afloat +6.3, most +14; sea exchange +0.15 (0.75); fish food +1139; boats lost -4.3.
- Mines (128 games, navy_fleet=0): kills - losses +1.1 (1.7), gold +200 / iron +150 an hour; against the sea opponent
  the pull stays 0 (no spare food).
- Continental: the same game with and without (2 seeds). About 1000 runs, no crash.

### Sea opponent (navy measurement)

2026-10-19, dll/testopp.c, `scenarios/seaop.txt`, `scripts/seacontrol.py`. Measuring an AI's navy against another AI's
says little - AIs are weak at sea (1-3 warships). The playtester on an island map builds 20+ ships early and uses them:
hunts fishing boats, shells coastal citizens, hunts warships before they mass, patrols for transports, lands. The sea
opponent plays that with the idle human P1 against one AI. Test code only: nothing runs unless the setup line
`seaop start <ai> [level]` starts it.

**What it does** (details in the file's header comment):
- At war with the AI from 2 s (before the AI's navy detection at 10 s). Two docks on P1's island, a beach to embark from.
- Warships spawned at the docks (P1 has no economy) on a human-like schedule, one per dock every 30 s at most, losses
  replaced up to an hour's budget; the newest type of each role P1 or the AI could build (keeps the AI's epoch):
  frigates 40%, battleships 30%, galleys / catapult ships 30% (attack subs where there are no galleys).
- Three groups every 2 s: *raid* (fishing boats, transports, citizens / docks / buildings at the coast; else the
  fishing grounds, docks, coast ring), *main* (defend, escort a landing, hunt warships at ratio 1.2+, shell docks, hold
  the sea lane), *patrol* (the lane between the islands; transports within 25 tiles, boats within 12). Retreat at
  ratio < 0.8 (patrol 1.0) or 45% of the hit points; ships under 35% go home; danger zones (defences, siege, ranged
  soldiers that hit ships) are neither entered nor crossed (a waypoint round them), a ship inside sails out.
- Honest sight: P1 knows an AI unit only within 9 tiles of its own units (warships remembered 3 min, buildings until
  the spot is seen empty). Shortcut: the AI's home position is known from the start.
- Landings: 2-3 transports full of soldiers of the AI's own land types, loaded with the game's load goal, to a beach
  out of danger (else the one under the fewest zones), the main fleet escorting, then they attack.
- Home: a guard of the AI's soldier types and 2 towers of its tower type on P1's island (an idle base ended the
  first games at the AI's first landing).

**Levels** (`[autotest] seaop_level`, default 2; `seaop_start=<min>`; `seaop_landings=0`; `seaop_trace=1` logs
each ship lost with what was near):

| | weak (1) | medium (2) | strong (3) |
|---|---|---|---|
| first ships | min 8, 2 | min 5, 3 | min 3, 4 |
| warships by min 20 / at most | 8 / 12 | 16 / 22 | 22 / 30 |
| ships an hour (budget) | 24 | 44 | 60 |
| landings (minute: transports) | 35: 2 | 25, 45: 3 | 20, 32, 44: 4 |
| home guard | 6 | 10 | 14 |

**Run:**
```sh
JOBS=6 REPEAT=10 sh scripts/scen.sh 60 seaop "random_map=Small Islands,map_size=0,ai_count=1,seed=101,start_epoch=5" \
    "n0:navy=0" "f0:" "f1:navy_fleet=1"
python scripts/seacontrol.py [-v] [-ctl f0] "work/autotest_*_suite<stamp>_*_seaop-r*.csv"
```
Name the variants `<variant><map>` (e.g. `f0Se5`) to pool maps per variant as fleet_ab.py does; `-ctl` pairs each run
with the control variant's run of the same map and seed. The game_seed holds: reruns of the same seed gave identical
`# sea:` lines (4 of 4).

**Log** (every minute): `# sea: P1 vs P2 at <m> min (<level>): opp_war= raid= main= patrol= F= L= A= S= opp_hp= spawned=
opp_lost= opp_lost_by_ai= ai_war= ai_war_most= ai_war_lost= ai_tr= ai_tr_lost= ai_boats= ai_boats_lost= ai_fish=
ai_cit_killed= ai_cit_by_ships= waves= sailed= ashore= stopped= landed= opp_tr_lost= ai_held_s= opp_held_s= total_s=
ai_isl_bld= ai_isl_towns= ai_on_opp= seen_war= mem_war= dz= guard= retreats= flees= detours= orders= | <group tasks>`;
also `# sea: wave N ...` (spawned, sails, ashore / stopped). Held: no enemy warship within 12 tiles of the side's
buildings. The AI's losses come from diplo.c's kills by pair (with one AI every loss at sea is P1's doing); citizens
killed count as by ships while P1 has no soldier on other land and the AI none on P1's island. `flees` counts ticks.

**Baseline** (HEAD 5868f55 + the opponent, medium, 1 AI, seed 101 maps, 60 min, game seeds 1-10 per map; tables in
docs/navy.md "Against a human-like navy"): 163 runs (120 + 39 levels + 4 reruns), no crash, one run lost to a start race ("game already running").
The opponent holds the AI's waters (AI home waters held 12-22% of the time on Small Islands), the AI's fish food drops
to 160-780 an hour (AI against AI: 2000-4500), the AI keeps 0.2-1.0 warships afloat against 16; navy=0 against navy=1
(the AI that builds a navy sinks 1-6 more of the opponent's ships and loses 3-9 more) separates in the warship counts;
navy_fleet=1 changes nothing beyond noise but Small e9 (sea exchange -8.1, std. error 2.1). The levels scale: weak
lets the AI fish (850-2650) and win 3 of 19 games, strong defeats it in 10 of 20.

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
appears before the engine loads the DLL, centred on screen, and takes the focus) and the busy cursor.
Every window of the game gets no taskbar button and is never activated (dll/headless_win.c, a CBT hook):
the display DLLs' 16x16 probe windows activated ~21 times per start and the game window once, each with a
taskbar button - with several slots starting, the taskbar flickered and the desktop kept losing the focus.
The mod's dinput.dll installs it at the process start (`EEAIMOD_HEADLESS=1`, set by run_test.ps1), before
the display DLLs probe; eeaimod.dll only from its own load on (without the mod's dinput.dll, 6 probes still
activate). Nothing of the game should become visible or take the focus. Backups of a DLL never go into the game
folder (`work/dll_backup/`): the engine loads every `*.dll` there - a second dinput.dll (`dinput.old.dll`) crashed
every start on 2026-10-19 from 21:42 on (now a copy does nothing).

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
