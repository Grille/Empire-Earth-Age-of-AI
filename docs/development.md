# Development

How to build the mod, set up the reverse-engineering tools and find your way in the tree. What a change has to look
like to be merged: [CONTRIBUTING.md](../CONTRIBUTING.md). The tests: [testing.md](testing.md).

## Building

Requirements: Windows, any 32-bit MinGW C compiler. The project uses
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases) (`llvm-mingw-*-ucrt-x86_64.zip`) and a
POSIX shell (Git Bash).

```sh
git clone <this repo> eeaimod     # anywhere; inside the game folder (<game>/_aimod) needs no setting
cd eeaimod
# elsewhere: name the game folder (or set EE_GAME_DIR) - the tests and the install use it
echo "X:/Games/Empire Earth Gold/Empire Earth" > local/game_dir.txt
# unpack llvm-mingw to local/tools/llvm-mingw (or set CC=/path/to/i686-w64-mingw32-clang)
sh build.sh                      # -> dll/ageofai.dll, dll/ageofai_config.exe, dll/ageofai_check.exe
sh scripts/build/install.sh      # the committed build (a clean git archive of HEAD) into the game folder
```

The DLL is a single translation unit: `dll/eeaimod.c` includes the other `.c` files, one folder per part (Project
layout); which part may use which is checked by the build (`scripts/build/layers.py`). A build must be free of
warnings. It also generates C from the C# declarations in `spec/` (the game: `dll/game/api/gen/`) and `dll_cs/`
(the settings: `dll/settings/rows.h` and `config/ageofai.ini`; the language's objects: `dll/game_script/lang.h`),
and runs `ageofai_check.exe` over the shipped personalities and the test scripts.

`install.sh` moves files in use aside instead of waiting for the game to close: a running game keeps its build until it is restarted. A
release zip: `local/venv/Scripts/python.exe scripts/build/pack_release.py` (into `local/work/release/`).

## Setup

Everything below is only needed for reverse engineering and automated tests.

| Tool | Where | Used for |
|---|---|---|
| llvm-mingw | `local/tools/llvm-mingw` | building the DLL |
| Python 3.12+ | `local/venv` (`python -m venv local/venv`, `local/venv/Scripts/pip install -r requirements.txt`) | analysis and test scripts |
| Ghidra 12 + JDK 21 | `local/tools/ghidra12`, `local/tools/jdk21` | decompiling the game |

None of these, and nothing derived from the game's files, are in the repository: they live in `local/` (git-ignored -
`work/` the test logs and builds, `tools/`, `venv/`, `games/ee/` and `games/aoc/` what is taken from each game).
Generate them locally:

```sh
# function names from the game's RTTI (vtables) -> local/work/names.tsv
mkdir -p local/work && local/venv/Scripts/python scripts/re/rtti_names.py local/work/names.tsv
# full decompilation to local/games/ee/decomp/<address>.c + local/games/ee/decomp/functions.tsv (takes a while)
cp "../Empire Earth.exe" local/work/EE.exe
local/tools/ghidra12/support/analyzeHeadless local/games/ee/ghidra_proj EE -import local/work/EE.exe \
    -scriptPath scripts/re/ghidra -postScript ExportDecomp.java "$(pwd)/local/games/ee/decomp"
# the spec's names, types, structures and enums into the project (before the decompilation above, so it reads with
# them; again after a change of spec/ - the spec's names win): local/work/spec_ee.json, then ApplySpec (aoc: AOC's project)
local/venv/Scripts/python scripts/re/spec_ghidra.py ee
local/tools/ghidra12/support/analyzeHeadless local/games/ee/ghidra_proj EE -process -noanalysis     -scriptPath scripts/re/ghidra -postScript ApplySpec.java "$(pwd)/local/work/spec_ee.json"
# the same for each engine DLL (scripts/re/port_dll.py; Art of Conquest's into local/games/aoc/decomp/dll/ from its folder)
local/tools/ghidra12/support/analyzeHeadless local/games/ee/ghidra_proj DLL_LowLevelEngine -import "../Low-Level Engine.dll" \
    -scriptPath scripts/re/ghidra -postScript ExportDecomp.java "$(pwd)/local/games/ee/decomp/dll/LowLevelEngine"
# string table of the German Language.dll -> local/games/ee/strings_de.json
local/venv/Scripts/python scripts/re/strtab.py "../Language.dll"
```

Helpers working on the dump and the exe:
- `scripts/re/fn.sh <hex>`: print a decompiled function.
- `scripts/re/refs.py <hex>`: list calls, jumps and pointers to an address.
- `scripts/re/disp.py <hex>`: find instructions using a displacement or offset.
- `scripts/re/pdis.py <hex>`: disassemble at an address.

[engine-notes.md](engine-notes.md) collects the verified addresses and data layouts.
The community's projects at [EE-modders](https://github.com/EE-modders) are a good second source (the game's
text in all languages, the DB file formats, other DLL mods).

## Testing

The DLL has an **autotest mode** and a **control pipe**, so AI games run unattended. Scenario and save
runs at the same speed are byte-identical, which makes A/B tests exact; random maps vary between runs, so
those are compared over several runs (`game_seed` makes one the same game every run). See [testing.md](testing.md).

```sh
# one 75-minute AI-vs-AI game on a random map, logs to local/work/autotest_<stamp>_<tag>.csv.gz
powershell -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 -Slot 1 -Save newgame.ees -Tag _try \
    -IniExtra "random_map=Continental,ai_count=3,seed=101,avoid_towers=1"
# the 4-map suite for several variants, scored per variant (4 games at a time, each in a test slot)
JOBS=4 sh scripts/test/suite.sh 50 "base:" "smart:avoid_towers=1"
# talk to the running game (SLOT=n for a test slot's)
SLOT=1 local/venv/Scripts/python scripts/test/eectl.py status | army 2 | diplo | persona | say 2 Hello | quit
```

## Project layout

```
build.sh          the build (scripts/build/build.sh)
spec/             the game as the mod knows it, as C# declarations that compile: every class, field, function, call
                  site and number, per build (2017 update, Art of Conquest); gen_api.py makes dll/game/api/gen/ of it
dll_cs/           the C# the build makes some of dll/'s C from: settings/ every ageofai.ini key, lang/ the .eeai
                  language's objects
dll/              the mod (C), one translation unit: eeaimod.c includes every part - the game start, the AI's update
                  hooks that call the parts, install, DllMain; persist.c the mod's state in saved games
  base/             the DLL's own helpers: log, patching, the pipe commands' REPLY, paths and the test slot, mistakes.c
                    the report of a mistake in the mod's files, math.c (the same bits on every PC), ageofai.ini
                    (ini.c), watchpoints, headless_win.c test windows without focus or taskbar button
  game/             what the mod knows of the game: api/ the game's own code and objects, one module (api.c) - exe.c
                    the exe build, its addresses per build (gen/: the spec's C, generated; builds.h;
                    editor_layout.h generated), its objects typed (types.h), a file per object (unit.c, player.c,
                    attack.c ...: its fields by name, the game's functions on it, its hooks; per build:
                    spec/game/), game.c the globals, hooks.c the code patches; world.c
                    type data, game time, RTTI names, players.c the players (and the vanilla
                    ones, the colours), random.c the decisions' generator (alike on every PC), the unit types
                    (unit_ids.h, unitnames.c, unit_names.h generated) and roles.c what a type is,
                    grid.c the unit grid (iterator)
  engine/           fixes and features of the game itself: display.c / present_d3d.c windowed mode (display_text.c,
                    display_diag.c), camera.c scrolling, aim.c leading targets, sound.c, select.c, drawlist.c,
                    playername.c the menu name fix, scenario_ai.c the scenario AI fix
  ai/               the computer players: econ.c budget, epoch plan, build queue, workers, research; sites.c gathering
                    sites; attack.c attack routing, siege, unit choice; stats.c fight estimates; defence.c towers, walls, home guard; diplo.c
                    diplomacy; landing.c landings; navy.c / fleet.c the navy; settings.c ageofai.ini's AI settings ...
  script/           expr.c the text language (parsed once into a tree; statements, functions) - it knows no game:
                    what it asks the program it runs in is script/host.h (eeaimod.c x_host_install sets the game's)
  game_script/      the game in the language: constants.c (epoch., resource., color., kind., unit.), relations.c
                    the players' values and relations (them.soldiers, them.kills(me) - from the AI's state)
  voice/            what the AI says: say.h the AI's one door to it (its lines, its personality's traits and
                    decisions, the language model), persona.c personalities, texts.c the text files, chat.c chat lines, llm.c the
                    language model
  test/             control.c the control pipe, testcmd.c / testopp.c test commands and the sea opponent, randmap.c
                    test-game setup, autotest.c, scripts.c the setup scripts (cmd_verbs.h, generated: the commands
                    they call), trace.c call tracing, ecolog.c the economy log
  settings/         rows.h (generated from dll_cs/settings/*.cs) every ageofai.ini key as a row (kind, section, default, limits), every section (order, kind,
                    title) and the topics in them (ageofai_config.exe's tree), settings.c reads them once into
                    g_set / g_set_ai (a value per AI difficulty) - the parts read those, never the file
  tool/             ageofai_config.c: ageofai_config.exe, the settings in a window (from base/, script/ and settings/ only:
                    the DLL's own rows and checks; the shipped ageofai.ini built in as the help)
  check/            ageofai_check.c: ageofai_check.exe, the .eeai files checked without the game (the DLL's code
                    compiled into a program: the checks the game runs at its start; the build runs it too)
config/           ageofai.ini and the personalities, shipped to players
tests/            what the tests run: scenarios/ the setup scripts (docs/scripts.md) that turn a save into a test
                  scenario (attacks, sieges, raids), probes/ the one-off setups of single measurements, config/
                  autotest.ini and the test configs, text_engine_cases.txt the text engine comparison's cases
scripts/          game_dir.py / .sh: where the game is (every script's)
  build/          the build's checks and generators (layers.py, style.py, spec_check.py, gen_api.py, persist_layout.py,
                  gen_cmd_verbs.py, gen_settings.py, gen_lang.py, gen_*.py for the docs and the grammar),
                  code_same.py, install.sh / install_latest.sh / install_wait.sh, pack_release.py
  test/           run_test.ps1 and the runners on top of it (suite.sh, scen.sh, h2h.sh, vanilla.sh, jobs.sh),
                  reserve.sh, eectl.py the control client, ee_mcp.py the MCP server, work_prune.ps1
  results/        the log readers and scorers (score.py, h2h.py, vanilla.py ...; logs.py opens the gzipped logs)
  re/             reverse engineering: the exe and dump helpers, port_*.py for Art of Conquest, gen_units.py +
                  dat_read.py (docs/units.md and dll/game/unit_names.h from the game data: the community DB editor's
                  field layouts and English strings, downloaded into local/games/ee/dats/), make_cpbehavior.py (a
                  tuned dbcpbehavior.dat; superseded by ageofai.ini), ghidra/ the headless scripts that name and
                  decompile the exe (ExportDecomp) and put the spec into a project (ApplySpec, from spec_ghidra.py's file)
extensions/       eeaimod-texts: the VS Code extension for the .eeai files
local/            git-ignored: this machine's tools, test output and what is derived from the games (Setup)
docs/             what the mod does (ai-features, diplomacy, game-fixes), the AI's research (ai-analysis, economy,
                  navy, experiments), the engine (engine-notes, triggers, renderer, units: every type), testing.md,
                  scripts.md (the setup scripts), this file
```
