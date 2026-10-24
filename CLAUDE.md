# eeaimod - working notes for Claude sessions

An AI and tuning mod for Empire Earth 1.04 (32-bit, German retail + DSML), injected as `eeaimod.dll`. The game
folder: env `EE_GAME_DIR`, else `local/game_dir.txt` in the repo (git-ignored), else the repo's parent (`<game>\_aimod`,
the setup here) - scripts find it through `scripts/game_dir.sh` / `game_dir.py` / run_test.ps1; paths in the repo
(`setup=tests/scenarios/x.eeai`, `CONFIG=...`) are relative to the repo. What the mod does: README.md; how it is tested:
docs/testing.md.
Several sessions may work in this tree at the same time - see Git below.

The tree (2026-10-23): `dll/` the mod, `config/` what ships, `tests/` what the tests run (`scenarios/`, `probes/`,
`config/` with autotest.ini), `scripts/` by job - `build/` (the build's checks, generators, install), `test/` (run_test
and the runners, eectl, the MCP server), `results/` (log readers, scorers), `re/` (reverse engineering, the AoC port) -,
`extensions/` the VS Code extension, `docs/`. Everything git-ignored is in `local/`: `work/` (logs, slots, builds),
`tools/`, `venv/`, `games/ee/` and `games/aoc/` (decompilations, Ghidra projects, extracted data), `scratch/`.

## Build and install

- `sh dll/build.sh` builds `dll/eeaimod.dll`, `dll/dinput.dll` and `dll/aimod_config.exe` (llvm-mingw in `local/tools/`, 32-bit; it also
  regenerates `dll/persist_layout.h` and `dll/test/cmd_verbs.h`). One translation unit: `dll/eeaimod.c` includes the rest.
  A build must be free of warnings. It also writes `.editor/compile_commands.json` (git-ignored, read by VS Code's
  C/C++ through `.vscode/c_cpp_properties.json`): each part with the text eeaimod.c has before it, so the editor
  knows what the compiler knows there.
- `dll/` has a folder per part, and which part may use which is a rule the build checks (`scripts/build/layers.py`):
  base (nothing) <- game/api (the game's code and objects, one module: nothing of its raw/ outside it) <- game <- engine, ai (and voice/say.h: what the AI says goes through it); script (base only: the
  language knows no game - script/host.h asks the program); voice (base, game, ai, script); game_script (the game in
  the language: all of those); settings (base only: aimod.ini's keys as rows, read once into `g_set` - used by
  engine, ai, voice, game_script, test and the wiring); tool (base, settings, script: aimod_config.exe, a program of
  its own - a name there must not be one of the DLL's too: layers.py reads the tree as one namespace);
  test and the wiring (`eeaimod.c`, `persist.c`) everything. A new crossing fails the build - put the code where it
  belongs (a shared helper down into base/ or game/) instead of adding an exception. `--why <folder>` lists the uses.
- A move that should change no code (files, functions between files): `scripts/build/code_same.py HEAD` must say "0 differ".
- `sh scripts/build/install.sh` puts the committed build into the game: built from a clean `git archive HEAD` (never the
  working tree), copied under a reservation of the game folder (a loaded DLL can't be replaced). Commit first.
- Every commit installs itself (the user's rule: the install is for testing the newest changes, a crash is welcome
  news): `.git/hooks/post-commit` (local) starts `scripts/build/install_latest.sh` in the background - it waits for the
  user's own game to end, then install.sh; log `local/work/install.log`. No manual install after a commit.
- A task that needs the game folder to itself (an install, a slot-0 run): `sh scripts/test/reserve.sh "<what>" <command>`
  - test runs start no game meanwhile, the running ones (and the player's) are waited for; nobody has to pause.
- What the mod knows of the game's code and objects lives in `dll/game/api/`. The raw rows, api/'s own, in `raw/`: `functions.h`
  (the functions the mod calls), `hooks.h` (what it patches to hook), `data.h` (the game's data), made a table by
  `tables.c` (with `fields.h`'s; the builds `builds.h`'s) - `api.c` the module's one entry - by name with one value per exe build: `GA(G_WORLD_CLOCK)`,
  `GF(FN_CAN_BUILD)(pl, t, 0x20, 0)`, `GL(TRAIN_SITES)`, a vtable slot `GA(SLOT_ASG_UPDATE)`, an object's size
  `GA(SIZE_GOAL_BUILD)` - never a literal address, slot or size in the code. The game objects' fields likewise live
  in `raw/fields.h`, one row each, named by kind and what it is (`O(UNIT_X, 0x1C, 0x1C)`, `UNIT_0x50` while not
  understood, a structure inside the object one row; the name is a variable holding the running build's offset:
  `FIELD(u, UNIT_X, int)`; a field not in the table doesn't build; the runtime library's layouts - deque, vector,
  string - stay plain numbers), read by name through the object's file: `unit_get_x(u)`, `unit_set_behaviour(u, b)` (a setter only where the mod writes), typed by kind
  (types.h); a field's name only in game/api/ (it is raw/'s: scripts/build/layers.py) - a new field: its row, then its
  accessor. FIELD with a plain number stays for the runtime library's layouts and raw dumps. Two columns:
  the 2017 update and Art of Conquest (not ready: tested in AoC only since 2026-10-21 - test runs play it,
  `EE_GAME_DIR=<its folder>`, docs/testing.md). A new row needs both: `scripts/re/port_addrs.py` /
  `scripts/re/port_fields.py Class[,Class] offset` find the AoC value, `scripts/re/port_check.py` checks it in the object's
  own code (and a called function's stack arguments: `rets`), `scripts/re/port_dll.py` for the engine DLLs (AoC ships
  its own builds: an RVA_ row, or better the export by name). Code that differs by more than an address:
  `build_is("aoc")` (AoC rewrote a few functions - docs/engine-notes.md).
- `dll/game/api/`: a file per object of the game (`unit.c`, `player.c`, `attack.c`, `tech.c` ... - by the first
  argument, the C++ object of a thiscall): its fields, the game's functions on it as typed wrappers (`explored_by(site,
  pl)`, `rm_assign(rm, u, site)`, `goal_is_attack(g)`, `goal_build_alloc()`), its hooks' types and installers
  (`patch_place(hook_place)` - a hook with other parameters doesn't build); `game.c` the game's global state and its getters
  (`game_map()`, `game_players()`, `end_epoch()`, `game_new(n)`), `hooks.c` the patches of the game's code, `types.h` the types' names. GA, GF
  and GL only in game/api/ (scripts/build/layers.py; elsewhere add a wrapper, an accessor or an installer); a kind not typed
  yet is `void *`. `scripts/build/addr_sort.py` moves the rows into the file their use calls for (the build checks it). A
  change there must keep the machine code (`scripts/build/code_same.py`, which compares the address and field tables by name) or, where it
  can't, play the same game (the same game_seed, the logs compared).
- The engine loads every DLL in the game folder: never leave copies of DLLs there (a stray dinput copy crashed it).
- A setting of aimod.ini is a row in `dll/settings/rows.h` (kind, section, key, default, limits) and its key in
  `config/aimod.ini` with that default; the code reads `g_set.key` ([general]), `g_set.section_key` (the other
  sections: `g_set.display_present`) or `g_set_ai[d].key` ([all], a value per difficulty from [easy] / [medium] /
  [hard], else [all]; code that doesn't tell difficulties apart reads [1]), and the test ini's [autotest] overrides a
  row by that same name (`scripts/build/settings_check.py`, run by the build, holds rows and file together). Nothing
  reads aimod.ini itself any more; a test-only knob is a row of section autotest.
- The game reads the personality files from `config/aimod_ai/` (`ai_dir` in the user's aimod.ini): a change to
  their format needs the matching installed DLL.

## Testing

- `scripts/test/run_test.ps1` (and scen.sh, suite.sh, h2h.sh ... on top of it): always `headless=1`, always in a slot of
  its own (`-Slot n` / `SLOT=n`, `JOBS=n` for the scripts) - slots 1..30 run beside the player's game and each
  other; slot 0 shares the game folder and refuses while the player's game runs. `DLL=<path>` plays your own build
  without installing it. Tests use the shipped `config/aimod.ini` and `config/aimod_ai/`, never the user's own.
- Setup scripts (`tests/scenarios/*.eeai`, `tests/probes/*.eeai`): docs/scripts.md. The control pipe: `scripts/test/eectl.py`.
- A run's log is `local/work/autotest_*.csv.gz` (gzipped: `zgrep`, `zcat`; a script reads it through `scripts/results/logs.py`);
  logs older than 7 days, or over 10 GB, are deleted (`scripts/test/work_prune.ps1`) - one to keep goes into `local/work/keep/`.
- Without `game_seed` a random map is a different game every run. A/B results need many seeds (docs/testing.md).
- A change to the text engine or to which line is chosen: run `texts corpus` before and after in the same game
  (docs/testing.md "Text engine comparison") - the results must be identical unless the change is meant to differ.

## Generators - rerun after changing their sources

| Change | Run |
|---|---|
| a pipe command, a saved variable | `sh dll/build.sh` (does both) |
| names the text files know (values, situations, keys) | `scripts/build/gen_vscode_grammar.py`, then `scripts/build/pack_vscode.py` |
| situations, the format help | `scripts/build/gen_persona.py` (`_template.eeai`), `scripts/build/gen_eeai_readme.py` (aimod_ai README tables) |
| the game data's unit types | `scripts/re/gen_units.py` (docs/units.md) |

Python: `local/venv/Scripts/python.exe`.

## The text language (.eeai)

One language for personalities, system texts, prompts and test scripts (dll/script/expr.c, docs/scripts.md,
config/aimod_ai/README.md). Strict and explicit: a mistake is told with file and line, nothing is guessed, no
compatibility shims before the release. English only until the release.

## Decisions that aren't visible in the code

- The shipped defaults are the AI at its strongest without cheating; easing off only for the easy difficulty.
- Behaviour changes are measured before they are switched on (default off until a test shows the gain).
- The user plays the game and reports; explain engine and reverse-engineering findings plainly.

## Git

- Never push. Commit each finished step (no need to ask) with a message that says what changed and how it was
  checked.
- Never commit files another session is changing (check `git status` and `git diff` before `git add`; add files by
  name, never `git add -A`).

## Pitfalls

- `dll/` mixes CRLF and LF files and some are latin-1: match a file's line endings; Edit handles both.
- Backslashes get mangled in shell heredocs (`\n`, `\b`, `\a` ...): write edit scripts as files or use Edit/Write.
- File-level functions and variables are `internal` (base/lang.h: C's static there), `static` only inside a
  function (a local that keeps its value); `int *a, b` with the * on the name. scripts/build/style.py checks both.
- Comments and docs: plain, dense sentences in the style of the surrounding code; say why, not only what.
