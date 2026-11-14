# eeaimod - working notes for Claude sessions

An AI and tuning mod for Empire Earth 1.04 (32-bit, German retail + DSML), injected as `ageofai.dll`. The game
folder: env `EE_GAME_DIR`, else `local/game_dir.txt` in the repo (git-ignored), else the repo's parent (`<game>\_aimod`,
the setup here) - scripts find it through `scripts/game_dir.sh` / `game_dir.py`; paths in the repo
(`setup=tests/scenarios/x.eeai`, `CONFIG=...`) are relative to the repo. What the mod does: README.md; how it is tested:
docs/testing.md.
Several sessions may work in this tree at the same time - see Git below.

The tree (2026-10-25): `build.sh` the build (scripts/build/build.sh, called from the root to be found first, the player 2026-11-02), `dll/` the mod, `spec/` the game as the mod knows it (below), `dll_cs/` the C# the build makes some of dll/'s C from
(beside it on purpose, the player 2026-10-31) - `lang/` the .eeai language's objects (below), `settings/` ageofai.ini's
keys (below), `config/` what ships, `tests/` what the tests run (`scenarios/`, `probes/`,
`config/` with autotest.ini), `scripts/` by job - `build/` (the build's checks, generators, install), `test/` (run_test
and the runners, eectl, the MCP server), `results/` (log readers, scorers), `re/` (reverse engineering, the AoC port) -,
`extensions/` the VS Code extension, `docs/`. Everything git-ignored is in `local/`: `work/` (logs, slots, builds),
`tools/`, `venv/`, `games/ee/` and `games/aoc/` (decompilations, Ghidra projects, extracted data), `scratch/`.

## Build and install

- `sh build.sh` builds `dll/ageofai.dll` and `dll/ageofai_config.exe` (llvm-mingw in `local/tools/`, 32-bit; it also
  regenerates `dll/persist_layout.h` and `dll/test/cmd_verbs.h`). One translation unit: `dll/eeaimod.c` includes the rest.
  A build must be free of warnings. It also writes `.editor/compile_commands.json` (git-ignored, read by VS Code's
  C/C++ through `.vscode/c_cpp_properties.json`): each part with the text eeaimod.c has before it, so the editor
  knows what the compiler knows there.
- The build also makes `dll/ageofai_check.exe` (`dll/check/`: eeaimod.c compiled into a program, no game - the .eeai
  files checked as the game reads them, `path:line: what`) and runs it over config/personalities and the test scripts:
  a mistake there fails the build. Code the personalities' or scripts' loading calls must not read the game's memory
  without a game (`g_exe_build < 0`: none known) - the checker faults on such a read. VS Code: `.vscode/tasks.json`
  "eeai: check this file" / "eeai: check all" into the Problems panel.
- `dll/` has a folder per part, and which part may use which is a rule the build checks (`scripts/build/layers.py`):
  base (nothing) <- game/api (the game's code and objects, one module: nothing of its gen/ values or editor_layout.h outside it) <- game <- engine, ai (and voice/say.h: what the AI says goes through it); script (base only: the
  language knows no game - script/host.h asks the program); voice (base, game, ai, script); game_script (the game in
  the language: all of those); settings (base only: ageofai.ini's keys as rows, read once into `g_set` - used by
  engine, ai, voice, game_script, test and the wiring); ui (base only: Win32 controls in the game's look, from the
  player's Data\data.ssa - ageofai_config.exe's window and the DLL's dialogs); tool (base, settings, ui, script: ageofai_config.exe, a program of
  its own - a name there must not be one of the DLL's too: layers.py reads the tree as one namespace);
  test, check/ and the wiring (`eeaimod.c`, `persist.c`) everything. A new crossing fails the build - put the code where it
  belongs (a shared helper down into base/ or game/) instead of adding an exception. `--why <folder>` lists the uses.
- A move that should change no code (files, functions between files): `scripts/build/code_same.py HEAD` must say "0 differ".
- `sh scripts/build/install.sh` puts the committed build into the game: built from a clean `git archive HEAD` (never the
  working tree), copied under a reservation of the game folder (a loaded DLL can't be replaced). Commit first. Art of
  Conquest's folder (EE_AOC_DIR, local/aoc_dir.txt, else beside the game's) gets the same build; its ageofai\ is a
  junction to the game folder's on the player's PC (2026-11-13: one ini, log and personalities for both).
- A release: `local/venv/Scripts/python.exe scripts/build/pack_release.py` - HEAD built from a clean archive, zipped
  as the game folder lays it out (ageofai.dll, ageofai_config.exe, ageofai/ with ageofai.ini and personalities/) into
  `local/work/release/`, ageofai_check.exe beside the zip (a download of its own).
- Every commit installs itself (the user's rule: the install is for testing the newest changes, a crash is welcome
  news): `.git/hooks/post-commit` (local) starts `scripts/build/install_latest.sh` in the background -
  install.sh at once (a file in use - a running game's DLL - is moved aside, not waited for: the running game keeps its
  build until restarted), a failure tried again; log `local/work/install.log`. No manual install after a commit. Before
  telling the player a change is done: `sh scripts/build/install_wait.sh` (waits until HEAD is in the game folder, about
  a minute - he kept testing builds from before the fix, 2026-11-04), and say so if his game runs (restart it).
- A task that needs the game folder to itself: `sh scripts/test/reserve.sh "<what>" <command>`
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
  [thiscall]: those whose class isn't known yet (their object a void*; 7 since 2026-10-25, the runtime library's vector
  ops among them). A virtual function's class: the vtables that hold it, their RTTI and bases. An object's class: its vtable's RTTI - in a running game (`peek` its first dword, then vtable-4 ->
  locator +0xC -> type descriptor +8: `.?AVName@@`), a constructor's by the vtable it writes, scripts/re/vtables.py a
  class's in both exes; a class's [vtable] is checked by that name at load. The game's enums are the spec's too (`[c_prefix("SM_CAT")] enum sm_category : int32 { @object = 1, ... }`: gen/enums.c's
  SM_CAT_OBJECT ..., as the spec's integer type: enum sm_category : int32_t; the scenario editor's lists, the unit type's class_id: CLS_NAVAL ... - its family the field family, the data's Family ID). A name or type read from what the code does with a field, not confirmed, is `[guess]` with its evidence in the /// comment (the build insists; gen/ and Ghidra say "A GUESS"; `scripts/re/spec_guesses.py` lists them with the mod's uses - confirm one before the mod relies on it). Ghidra knows the spec too: scripts/re/spec_ghidra.py + ApplySpec.java (docs/development.md "Setup") name
  the functions Class::member with their types, the structures and enums under /spec - run them after a spec change. Two builds: the 2017 update and Art of Conquest (ready since 2026-11-05; NeoEE's AoC
  not yet - test runs play it too, `EE_GAME_DIR=<its folder>`, docs/testing.md); a new value needs both. The port scripts read the
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
  names, `editor_layout.h` the scenario editor's layout (generated, the 2017 update). A kind not typed yet is
  `void *`. A change there must keep the machine code (`scripts/build/code_same.py`) or, where it can't, play the same
  game (the same game_seed, the logs compared - with the run-to-run heap addresses masked; AoC's player stock 0 and 1
  are memory it never sets, and an AoC game parts ways now and then with any DLL: seed 9 on Small Islands, about one
  run in 15 when many run at once, at 36 s - a control pair of one DLL first).
- The engine loads every DLL in the game folder: never leave copies of DLLs there (a stray copy of the mod's
  old dinput.dll crashed it).
- The game folder holds only ageofai.dll and ageofai_config.exe (the player 2026-10-27: keep the root clean) - and ageofai_check.exe, the .eeai checker (the player 2026-11-02: special enough; installed, but not in the
  release zip - a download of its own on the release page). The game's own settings in a file (eesettings.ini) are
  ageofai.dll's when it is loaded with the engine: an import of Low-Level Engine.dll that ageofai_config.exe adds and
  removes (the player 2026-11-02: no stand-in for another program's DLL - the mod's dinput.dll of before is gone;
  engine/game_settings.c, eeaimod.c early_start, tool/engine_import.c); test runs need it on in the game folder for
  their own settings (else they mute the registry's sound); everything else of the mod is in `<game>\ageofai\` - ageofai.ini, eesettings.ini, personalities\ (when no
  ai_dir), the log, the dumps, slot 0's test ini (base/paths.c slot_dir; a test slot has its own folder instead). A
  new file the mod writes goes there too. install.sh moves a folder set up before (scripts/build/mod_dir.sh) - also
  the names before Empire Earth: Age of AI (2026-10-27: aimod\, aimod.ini, aimod_econ.csv, *.aimod beside the saves),
  and removes an old eeaimod.dll / aimod_config.exe (both DLLs would load). Players see ageofai; eeaimod stays inside
  the code (eeaimod.c, the pipe, the runner's names, EEAIMOD_* variables).
- A setting of ageofai.ini is a field in `dll_cs/settings/<section>.cs` (2026-10-31): its type and attributes the
  kind, default and limits, its /// lines the file's lines above it (dll_cs/settings/settings.cs: the format).
  `scripts/build/gen_settings.py`, run by the build, makes `dll/settings/rows.h` (the rows: kind, section, key, default,
  limits) and the shipped `config/ageofai.ini` whole of them - both generated, never edited: one changed by hand
  stops the build (its change belongs into dll_cs/settings/). A key's name is its own across the file (the build stops at a second one): a section is only its place in the
  file and its page in ageofai_config.exe, a key moved keeps its name. The code reads `g_set.key` or `g_set_ai[d].key` ([ai_base], a value per difficulty from [ai_easy] / [ai_hard], else
  [ai_base] - medium plays [ai_base], it has no section; code that doesn't tell difficulties apart reads [1]), and the test ini's [autotest] overrides a
  row by that same name (`scripts/build/settings_check.py`, run by the build, holds rows and file together). Nothing
  reads ageofai.ini itself any more; a test-only knob is a field of autotest.cs. A new section is a class with
  [section(place)] (its order makes ageofai_config.exe's section; its topics there: a group, a nested class). A
  [master] group is one key over its keys (bool and factor only; ImprovedUnitMovement: improved_unit_movement):
  default = their defaults, custom = the file's, enabled / disabled = all on / off (a factor its default / 1) -
  settings.cs "A MASTER". A row's range is a sanity bound: outside it a warning, the value
  used (the code clamps what it can't take); only what can't be used at all is a mistake, the default instead -
  a range the code can't go beyond is `[range(lo, hi, error: true)]` (LIMIT: outside it a mistake too). A key
  that only sets one of the game's own values has [game(...)] (rows.h SETTING_GAME: ageofai_config.exe shows its key blue).
- The game reads the personality files from `config/personalities/` (`ai_dir` in the user's ageofai.ini): a change to
  their format needs the matching installed DLL.

## Testing

- `scripts/test/run_test.py` (and scen.sh, suite.sh, h2h.sh ... on top of it): always `headless=1`. Runs go to the test
  server (`scripts/test/testd.py`, started by the first client, runs on, one process for all games; docs/testing.md) and
  play beside the player's game and each other; when and in which slot is the server's business. Sessions take turns
  (the fewest games playing goes first, then the oldest), at most `EE_TEST_GAMES` games at a time (default half the
  logical processors) and memory left free; `run_test.py --status` shows what plays and waits. `JOBS=n` only bounds a
  batch's waiting clients (default 12). `-Name x` / `RUN=x` names a run to reach it: `RUN=x python scripts/test/eectl.py ...` (waits while it is queued; without RUN eectl is the player's game),
  `eectl.py --folder` the folder of its files while it plays. `DLL=<path>` plays your own build
  without installing it. Tests use the shipped `config/ageofai.ini` and `config/personalities/`, never the user's own.
- Setup scripts (`tests/scenarios/*.eeai`, `tests/probes/*.eeai`): docs/scripts.md. The control pipe: `scripts/test/eectl.py`.
- A run's log is `local/work/autotest_*.csv.gz` (gzipped: `zgrep`, `zcat`; a script reads it through `scripts/results/logs.py`);
  logs older than 7 days, or over 10 GB, are deleted (`scripts/test/work_prune.ps1`) - one to keep goes into `local/work/keep/`.
- Without `game_seed` a random map is a different game every run. A/B results need many seeds (docs/testing.md).
- A change to the text engine or to which line is chosen: run `texts corpus` before and after in the same game
  (docs/testing.md "Text engine comparison") - the results must be identical unless the change is meant to differ.

## Generators - rerun after changing their sources

| Change | Run |
|---|---|
| a pipe command, a saved variable | `sh build.sh` (does both) |
| a setting (dll_cs/settings/*.cs) | `sh build.sh` (gen_settings.py: dll/settings/rows.h and config/ageofai.ini) |
| a member of the language's objects (dll_cs/lang/*.cs: player, game, slot, a text's names) | `sh build.sh` (gen_lang.py: dll/game_script/lang.h), then `scripts/build/gen_eeai_readme.py` and the grammar (next row) |
| names the text files know (values, situations, keys) | `scripts/build/gen_vscode_grammar.py`, then `scripts/build/pack_vscode.py` |
| situations, the format help | `scripts/build/gen_persona.py` (`_template.eeai`), `scripts/build/gen_eeai_readme.py` (personalities README tables) |
| the game data's unit types | `scripts/re/gen_units.py` (docs/units.md) |

Python: `local/venv/Scripts/python.exe`.

## The text language (.eeai)

One language for personalities, system texts, prompts and test scripts (dll/script/expr.c, docs/scripts.md,
config/personalities/README.md). Strict and explicit: a mistake is told with file and line, nothing is guessed, no
compatibility shims before the release. English only until the release, but for the words the mod writes on the game's screens: `config/language.eeai` (2026-11-09: a record per word, `vassal = { en = "Vassal", de = "Vasall" }`; a new one also goes into dll/base/words.c k_words, the code asks `word("vassal")`, `word_w` for Win32, the system lines `{word.vassal}`). Its own language, not C# (the player
2026-10-25: C#'s quirks would put modders off) - but its objects (player, game, slot, a text's names: members, types,
arguments, docs) are declared once in `dll_cs/lang/` as C# (dll_cs/lang/language.cs: the format; `dotnet build` in dll_cs/ checks them and the settings, one project: dll_cs/language.cs the types and attributes both share - an ini value is the language's too),
and the C tables, the README's tables and the editor's names are made of them (gen_lang.py, lang_read.py).

## Decisions that aren't visible in the code

- The game's own computer player is OFF for the mod's AIs since 2026-11-14 (`[ai_base] game_ai = 0`, the player: "i wnat a
  clean slate ... we have neough systems in place to rebuild now"): until the mod's own economy is written they stand
  still (citizens idle, nothing built). A test or measurement that needs a playing AI passes `game_ai=1` (the AI as it
  was); what is rebuilt is measured with it off. docs/game-ai-routines.md "The game's computer player off".
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
