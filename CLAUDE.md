# eeaimod - working notes for Claude sessions

An AI and tuning mod for Empire Earth 1.04 (32-bit, German retail + DSML), injected as `eeaimod.dll`. The game
folder: env `EE_GAME_DIR`, else `local/game_dir.txt` in the repo (git-ignored), else the repo's parent (`<game>\_aimod`,
the setup here) - scripts find it through `scripts/game_dir.sh` / `game_dir.py` / run_test.ps1; paths in the repo
(`setup=tests/scenarios/x.eeai`, `CONFIG=...`) are relative to the repo. What the mod does: README.md; how it is tested:
docs/testing.md.
Several sessions may work in this tree at the same time - see Git below.

The tree (2026-10-25): `dll/` the mod, `spec/` the game as the mod knows it (below), `config/` what ships, `tests/` what the tests run (`scenarios/`, `probes/`,
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
- `spec/` is the source of truth about the game (2026-10-25): every class, field, function, call site, number and place
  in its code the mod knows, build by build, as C# declarations that compile (`dotnet build` in spec/;
  `scripts/build/spec_check.py`, run by the build, holds the rules the compiler leaves open). It describes the game only
  - the C comes from `scripts/build/gen_api.py` (run by the build: plain C in `dll/game/api/gen/`, never edited by
  hand; what each generated name is: its header) and the hand-written code on top; the format: spec/attributes.cs's
  header. A class per object of the game (spec/game/<its api file>.cs: EEPlayer, EECPCivilizationManager ...) with its
  fields' offsets; a static class per file for its data, numbers and places in the code (`[address] code
  call_rm_update`, `[value] uint32 slot_asg_update`) and one `<file>_functions` (`[c_prefix("")]`) for the game's
  functions by the names the C side gives them. Never a literal address, slot or size in the code: the generated
  value (`data_ai_managers_call_rm_update`, `value_attack_slot_asg_update`, `fn_unit_distance`, `of_unit_hp`) or its
  accessor / wrapper (`unit_get_hp(u)`, `unit_distance(a, b)`); the values only inside game/api/ (layers.py), the
  accessors for everyone. Names: a class's C prefix is its C type without ee_ (civ_mgr_get_timer on ee_civ_mgr,
  unit_type_get_hp on ee_unit_type - one name for type and functions, the player 2026-10-25; a class of a C type of
  its own has its base's fields under its name too: load_msg_get_from). At load, before any patch, the spec is
  checked against the running exe (gen/'s spec_check, exe.c's helpers: code in .text, data in the image, a [vtable]
  its class's RTTI name, a class's own function in its slot, a [call_site] a call of its function) - the start's log
  says `# spec: N checks against the exe, all hold` or names what fails: read it after a spec change or a port. A call site
  the mod patches is a `[call_site("x", ...)]` of its function: gen/ makes call_x (several: calls_x[]) and
  patch_x(hook), the hook the function's own type (`<function>_fn` - a hook with other parameters doesn't build:
  wrong types are the spec's to fix). Still places (`code`, a hand-written installer): a call hooked by an asm thunk
  (no C type), an installer over two functions or by build. A function of the game on an object is a method of its class in the
  spec (`extern void update(uint32 now)` in EECPResourceManager: C resource_mgr_update); still in `<file>_functions` with
  [thiscall]: those whose class isn't known yet (their object a void*; 12 since 2026-10-25, the runtime library's vector
  ops among them). An object's class: its vtable's RTTI - in a running game (`peek` its first dword, then vtable-4 ->
  locator +0xC -> type descriptor +8: `.?AVName@@`), a constructor's by the vtable it writes, scripts/re/vtables.py a
  class's in both exes; a class's [vtable] is checked by that name at load. The game's enums are the spec's too (`[c_prefix("SM_CAT")] enum sm_category : int32 { @object = 1, ... }`: gen/enums.c's
  SM_CAT_OBJECT ..., as the spec's integer type: enum sm_category : int32_t; the scenario editor's lists, the unit type's class_id: CLS_NAVAL ... - its family is still the field category, the data's Family ID). Ghidra knows the spec too: scripts/re/spec_ghidra.py + ApplySpec.java (README "Setup") name
  the functions Class::member with their types, the structures and enums under /spec - run them after a spec change. Two builds: the 2017 update and Art of Conquest (not ready: tested in AoC only since 2026-10-21 -
  test runs play it, `EE_GAME_DIR=<its folder>`, docs/testing.md); a new value needs both. The port scripts read the
  spec (`scripts/re/port_addrs.py`: every address's AoC value found again and compared with the spec's - same,
  DIFFERS, NEW; `port_check.py` a class's fields by its code; `port_fields.py`, `port_dll.py`): a value they find goes
  into the spec by hand, one found by hand also into port_addrs.py's EXE_HAND. AoC's function
  list for the pairing is local/games/aoc/decomp/'s from before its Ghidra project split 2787 functions (the newer list
  pairs 10 addresses fewer; decomp.named/ has the decompilation with the spec's names). Code that differs by more
  than an address: `build_is("aoc")` (AoC rewrote a few functions - docs/engine-notes.md).
- `dll/game/api/`: a file per object of the game (`unit.c`, `player.c`, `attack.c`, `tech.c` ...): the hand-written
  code over the generated - accessors that read more than one field (`unit_get_pos(u)`), wrappers that do more than
  pass their arguments on (`explored_by(site, pl)`), its hooks' types and installers (`patch_place(hook_place)` - a
  hook with other parameters doesn't build); `game.c` the game's global state and its getters (`game_map()`,
  `game_players()`, `end_epoch()`, `game_new(n)`), `hooks.c` the patches of the game's code, `types.h` the types'
  names, `raw/editor_layout.h` the scenario editor's layout (generated, the 2017 update). A kind not typed yet is
  `void *`. A change there must keep the machine code (`scripts/build/code_same.py`) or, where it can't, play the same
  game (the same game_seed, the logs compared - with the run-to-run heap addresses masked; AoC's player stock 0 and 1
  are memory it never sets, and an AoC game parts ways now and then with any DLL: seed 9 on Small Islands, about one
  run in 15 when many run at once, at 36 s - a control pair of one DLL first).
- The engine loads every DLL in the game folder: never leave copies of DLLs there (a stray dinput copy crashed it).
- A setting of aimod.ini is a row in `dll/settings/rows.h` (kind, section, key, default, limits) and its key in
  `config/aimod.ini` with that default; the code reads `g_set.key` ([general]), `g_set.section_key` (the other
  sections: `g_set.display_present`) or `g_set_ai[d].key` ([all], a value per difficulty from [easy] / [medium] /
  [hard], else [all]; code that doesn't tell difficulties apart reads [1]), and the test ini's [autotest] overrides a
  row by that same name (`scripts/build/settings_check.py`, run by the build, holds rows and file together). Nothing
  reads aimod.ini itself any more; a test-only knob is a row of section autotest. A new section is a row of
  `SETTING_SECTIONS` there too (its order and title make aimod_config.exe's page). A CHILD row is a flag under a
  master CHOICE `default|enabled|disabled` (improved_unit_movement over the seven movement keys): not set, it follows
  the master; the shipped file has it commented out.
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
- `dll/game/api/gen/` is made from all of spec/, another session's unfinished changes too: never `git add` it from
  the working tree - `sh scripts/build/stage_gen.sh <your spec files, gen_api.py ...>` stages it as HEAD's spec plus
  yours make it (twice a commit took another session's half of a rename, 2026-10-25).

## Pitfalls

- `dll/` mixes CRLF and LF files and some are latin-1: match a file's line endings; Edit handles both.
- Backslashes get mangled in shell heredocs (`\n`, `\b`, `\a` ...): write edit scripts as files or use Edit/Write.
- File-level functions and variables are `internal` (base/lang.h: C's static there), `static` only inside a
  function (a local that keeps its value); `int *a, b` with the * on the name. scripts/build/style.py checks both.
- Comments and docs: plain, dense sentences in the style of the surrounding code; say why, not only what.
