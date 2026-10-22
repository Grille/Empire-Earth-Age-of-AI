# eeaimod - working notes for Claude sessions

An AI and tuning mod for Empire Earth 1.04 (32-bit, German retail + DSML), injected as `eeaimod.dll`. The game
folder: env `EE_GAME_DIR`, else `game_dir.txt` in the repo (git-ignored), else the repo's parent (`<game>\_aimod`,
the setup here) - scripts find it through `scripts/game_dir.sh` / `game_dir.py` / run_test.ps1; paths in the repo
(`setup=scenarios/x.eeai`, `CONFIG=...`) are relative to the repo. What the mod does: README.md; how it is tested:
docs/testing.md.
Several sessions may work in this tree at the same time - see Git below.

## Build and install

- `sh dll/build.sh` builds `dll/eeaimod.dll` and `dll/dinput.dll` (llvm-mingw in `tools/`, 32-bit; it also
  regenerates `dll/persist_layout.h` and `dll/test/cmd_verbs.h`). One translation unit: `dll/eeaimod.c` includes the rest.
  A build must be free of warnings.
- `dll/` has a folder per part, and which part may use which is a rule the build checks (`scripts/layers.py`):
  base (nothing) <- game <- engine, ai (and voice/say.h: what the AI says goes through it); script (base only: the
  language knows no game - script/host.h asks the program); voice (base, game, ai, script); game_script (the game in
  the language: all of those);
  test and the wiring (`eeaimod.c`, `persist.c`) everything. A new crossing fails the build - put the code where it
  belongs (a shared helper down into base/ or game/) instead of adding an exception. `--why <folder>` lists the uses.
- A move that should change no code (files, functions between files): `scripts/code_same.py HEAD` must say "0 differ".
- `sh scripts/install.sh` puts the committed build into the game: built from a clean `git archive HEAD` (never the
  working tree), copied under a reservation of the game folder (a loaded DLL can't be replaced). Commit first.
- A task that needs the game folder to itself (an install, a slot-0 run): `sh scripts/reserve.sh "<what>" <command>`
  - test runs start no game meanwhile, the running ones (and the player's) are waited for; nobody has to pause.
- What the mod knows of the game's code and objects lives in `dll/game/api/`. Game addresses: `addr.h` (the builds,
  data and the rest), `functions_addr.h` (the functions the mod calls), `hooks_addr.h` (what it patches to hook) - by
  name with one value per exe build: `GA(G_WORLD_CLOCK)`,
  `GF(FN_CAN_BUILD)(pl, t, 0x20, 0)`, `GL(TRAIN_SITES)`, a vtable slot `GA(SLOT_ASG_UPDATE)`, an object's size
  `GA(SIZE_GOAL_BUILD)` - never a literal address, slot or size in the code. The game objects' fields likewise live
  in `fields.h` there, by kind and the 2017 update's offset: `FIELD(pl, OF(PL, 0x9D4), void *)` (a field not in
  the table doesn't build; the runtime library's layouts - deque, vector, string - stay plain numbers). Two columns:
  the 2017 update and Art of Conquest (not ready: tested in AoC only since 2026-10-21 - test runs play it,
  `EE_GAME_DIR=<its folder>`, docs/testing.md). A new row needs both: `scripts/port_addrs.py` /
  `scripts/port_fields.py Class[,Class] offset` find the AoC value, `scripts/port_check.py` checks it in the object's
  own code (and a called function's stack arguments: `rets`), `scripts/port_dll.py` for the engine DLLs (AoC ships
  its own builds: an RVA_ row, or better the export by name). Code that differs by more than an address:
  `build_is("aoc")` (AoC rewrote a few functions - docs/engine-notes.md).
- `dll/game/api/functions.h`: the objects typed (`ee_player *`, `ee_resource_mgr *`, ... one per fields.h kind) and the
  game's functions as typed wrappers (`explored_by(site, pl)`, `rm_assign(rm, u, site)`); `dll/game/api/hooks.h`: per hooked
  function its hook's type and installer (`patch_place(hook_place)` - a hook with other parameters doesn't build).
  Being moved over folder by folder: a folder in scripts/layers.py TYPED (ai/ so far) uses no GF, GA(FN_/CALL_/SLOT_)
  or GL (add a wrapper or an installer); a kind not typed yet is `void *` there. A move must keep the machine code
  (`scripts/code_same.py`) or, where it can't, play the same game (the same game_seed, the logs compared).
- The engine loads every DLL in the game folder: never leave copies of DLLs there (a stray dinput copy crashed it).
- The game reads the personality files from `config/aimod_ai/` (`ai_dir` in the user's aimod.ini): a change to
  their format needs the matching installed DLL.

## Testing

- `scripts/run_test.ps1` (and scen.sh, suite.sh, h2h.sh ... on top of it): always `headless=1`, always in a slot of
  its own (`-Slot n` / `SLOT=n`, `JOBS=n` for the scripts) - slots 1..30 run beside the player's game and each
  other; slot 0 shares the game folder and refuses while the player's game runs. `DLL=<path>` plays your own build
  without installing it. Tests use the shipped `config/aimod.ini` and `config/aimod_ai/`, never the user's own.
- Setup scripts (`scenarios/*.eeai`, `scripts/*.eeai`): docs/scripts.md. The control pipe: `scripts/eectl.py`.
- Without `game_seed` a random map is a different game every run. A/B results need many seeds (docs/testing.md).
- A change to the text engine or to which line is chosen: run `texts corpus` before and after in the same game
  (docs/testing.md "Text engine comparison") - the results must be identical unless the change is meant to differ.

## Generators - rerun after changing their sources

| Change | Run |
|---|---|
| a pipe command, a saved variable | `sh dll/build.sh` (does both) |
| names the text files know (values, situations, keys) | `scripts/gen_vscode_grammar.py`, then `scripts/pack_vscode.py` |
| situations, the format help | `scripts/gen_persona.py` (`_template.eeai`), `scripts/gen_eeai_readme.py` (aimod_ai README tables) |
| the game data's unit types | `scripts/gen_units.py` (docs/units.md) |

Python: `venv/Scripts/python.exe`.

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
  function (a local that keeps its value); `int *a, b` with the * on the name. scripts/style.py checks both.
- Comments and docs: plain, dense sentences in the style of the surrounding code; say why, not only what.
