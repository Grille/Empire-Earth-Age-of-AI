# Empire Earth tuning mod

A runtime mod for **Empire Earth 1.04 with the Difficulty Setting & Multiplayer Lobby add-on** (the
last official base-game exe; not Art of Conquest). It fixes and tunes the game from inside: a DLL in the
game folder patches the running exe, and the game's files stay unchanged. The engine loads every DLL
in its folder, so dropping the file there is enough.

The current focus is the **computer player**. The tooling here works for any part of the game: hooks,
control pipe, automated tests and the engine notes. Further fixes and tuning will live in the same DLL,
each as its own module with its own settings.

Retail/Gold and the **Steam Gold Edition** (`steamapps\common\Empire Earth Gold Edition\Empire Earth`)
both work: the Steam exe has the same code, only a signature and a DPI manifest are added, and there's no
DRM wrapper. Art of Conquest (`EE-AOC.exe`) isn't supported yet.

The mod coexists with community DLL patches such as *Empire Earth Tweaks* (widescreen, map sizes),
which patch different places.

## Computer player (current focus)

The original AI cheats: by default it pays about 1% of the real prices. With cheating switched off, it
can't run its economy. It spends its food on soldiers before it has farms, sends citizens through enemy
towers to distant mines, and gets stuck in the early epochs. This mod changes that:

| Feature | What it does |
|---|---|
| **Fair prices** | The AI pays what you pay (configurable per difficulty, e.g. a discount only on Hard). |
| **Economy first** | Before farms (Copper Age), the AI grows its workforce, counts the wild food it has found as its budget, saves for the next epochs and keeps only a token army. |
| **Military budget** | Soldiers are paid only from a share of the starting stock and of the income, not from the epoch reserve. |
| **Epoch pacing** | Timer or "as soon as affordable"; optionally waits for the human player. |
| **Attack tuning** | Fewer, bigger waves instead of a stream of small, wasted attacks. |
| **Attack routing** | Armies march around the enemy towers they know about, and look for undefended targets instead of running into the tower line. *(in development)* |
| **Scenario fix** | AIs in scenarios started from the menu play like in skirmish games (the original turns most of their planning off). |
| **Safe gathering** | Optional: citizens only gather near their base, on routes away from known enemies. |

The AI never gets information a human couldn't have: route planning only uses enemy buildings that
the AI has seen.

## Installing (players)

1. Copy `eeaimod.dll` into the Empire Earth folder (next to `Empire Earth.exe`).
2. Copy [`config/aimod.ini`](config/aimod.ini) into the same folder and adjust it. Every option is
   explained in the file. Settings can differ per difficulty (`[easy]`, `[medium]`, `[hard]` override
   `[all]`).
3. Optional: copy the folder [`config/aimod_ai`](config/aimod_ai) into the same folder. Every AI then gets
   one of its personalities: a name, a favoured colour (random maps), character traits that decide its diplomacy (peace, alliances,
   betrayal, what tribute is worth to it) and its own lines in the chat. Add or edit `.ini` files there to
   make your own (the format is described in `eroberer.ini`); they are read when Empire Earth starts.
   Lines and names are English by default; `[lines:de]` / `name:de=` replace them in a German game
   (the shipped personalities have both).
4. Start the game normally. Changes to `aimod.ini` take effect when a game starts or a save is loaded.

To uninstall, delete `eeaimod.dll`. If you delete only `aimod.ini`, the original AI comes back and
only the scenario fix stays on. The DLL writes a log to `aimod_econ.csv` in the game folder.

## Building

Requirements: Windows, any 32-bit MinGW C compiler. The project uses
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases) (`llvm-mingw-*-ucrt-x86_64.zip`) and a
POSIX shell (Git Bash).

```sh
# the repository is expected at <Empire Earth folder>/_aimod
git clone <this repo> "X:/Games/Empire Earth Gold/Empire Earth/_aimod"
cd _aimod
# unpack llvm-mingw to tools/llvm-mingw (or set CC=/path/to/i686-w64-mingw32-clang)
sh dll/build.sh            # -> dll/eeaimod.dll
sh dll/build.sh install    # build and copy into the game folder
```

The DLL is a single translation unit: `dll/eeaimod.c` includes the other `.c` files.

## Development setup

Everything below is only needed for reverse engineering and automated tests.

| Tool | Where | Used for |
|---|---|---|
| llvm-mingw | `tools/llvm-mingw` | building the DLL |
| Python 3.12+ | `venv/` (`python -m venv venv`, `venv/Scripts/pip install -r requirements.txt`) | analysis and test scripts |
| Ghidra 12 + JDK 21 | `tools/ghidra12`, `tools/jdk21` | decompiling the game |

None of these, and nothing derived from the game's files, are in the repository (see `.gitignore`).
Generate them locally:

```sh
# function names from the game's RTTI (vtables) -> work/names.tsv
mkdir -p work && venv/Scripts/python scripts/rtti_names.py work/names.tsv
# full decompilation to decomp/<address>.c + decomp/functions.tsv (takes a while)
cp "../Empire Earth.exe" work/EE.exe
tools/ghidra12/support/analyzeHeadless ghidra_proj EE -import work/EE.exe \
    -scriptPath ghidra_scripts -postScript ExportDecomp.java "$(pwd)/decomp"
# string table of the German Language.dll -> strings_de.json
venv/Scripts/python scripts/strtab.py "../Language.dll"
```

Helpers working on the dump and the exe:
- `scripts/fn.sh <hex>`: print a decompiled function.
- `scripts/refs.py <hex>`: list calls, jumps and pointers to an address.
- `scripts/disp.py <hex>`: find instructions using a displacement or offset.
- `scripts/pdis.py <hex>`: disassemble at an address.

[docs/engine-notes.md](docs/engine-notes.md) collects the verified addresses and data layouts.
[docs/ai-analysis.md](docs/ai-analysis.md) explains how the original AI works, where it fails, and the test
results so far.

## Testing

The DLL has an **autotest mode** and a **control pipe**, so AI games run unattended and fully
reproducibly. The same speed gives byte-identical runs, which makes A/B tests exact. See
[docs/testing.md](docs/testing.md).

```sh
# one 75-minute AI-vs-AI game on a random map, logs to work/autotest_<stamp>_<tag>.csv
powershell -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees -Tag _try \
    -IniExtra "random_map=Continental,ai_count=3,seed=101,avoid_towers=1"
# the 4-map suite for several variants, scored per variant
sh scripts/suite.sh 50 "base:" "smart:avoid_towers=1"
# talk to the running game
venv/Scripts/python scripts/eectl.py status | threat 2 | camera 80 70 | speed 7.5 | quit
```

## Project layout

```
dll/            the mod (C): eeaimod.c core hooks + logging, settings.c aimod.ini, econ.c budget and
                epoch plan, sites.c safe gathering, attack.c attack routing, randmap.c test-game setup,
                control.c control pipe, watch.c hardware watchpoints, trace.c call tracing
config/         aimod.ini template shipped to players
scripts/        test runner, suite, log analysis, control client, MCP server, RE helpers
ghidra_scripts/ Ghidra headless script that names and decompiles the exe
mod/            generator for a tuned dbcpbehavior.dat (AI settings table; superseded by aimod.ini)
docs/           engine notes, AI analysis, testing guide
```

## Display and interface

Settings in `aimod.ini`:

- `[display] mode=windowed`: native windowed mode. The engine still contains SSSI's own windowed path; the
  mod switches it on and fixes what broke on current Windows (presentation, crashes, terrain, editor).
  The window has the game resolution from the game settings (scaled down if it doesn't fit the screen).
  A DirectDraw wrapper (`ddraw.dll`) is still needed for the game to start on current Windows; which one is up
  to you, the mod works with both it was tested with:
  - [DDrawCompat](https://github.com/narzoul/DDrawCompat): fastest by far (2560x1440: ~4.5 ms per frame vs ~10).
    Put `CpuAffinity = all` into `DDrawCompat-Empire Earth.ini` next to the exe (its default runs the whole
    game on one CPU core). The window gets a classic (Windows 98 style) frame, which DDrawCompat draws itself.
  - dgVoodoo (DirectX pass-through is enough): modern window frame, slower.

  `[display] present=auto` picks the matching way to show the picture (DDrawCompat: its primary surface;
  otherwise Direct3D 11).
  The mouse follows the Windows cursor (your pointer speed), the window can be moved and switched like
  any other; `lock_mouse=1` keeps the cursor inside while the game is in front.
- `[display] mode=borderless`: a frameless window over the whole monitor, the game image scaled to it
  (aspect kept). Alt+Enter switches between window and borderless. The window is DPI-aware (`dpi_aware`),
  so Windows display scaling doesn't blur it; `smooth=1` filters the scaled image.
- `[editor] resolution=aoc`: scenario editor at 1024x768 with The Art of Conquest's layout; `game` or any
  `WIDTHxHEIGHT` extends that layout to the resolution (map view fills the screen, panels at the bottom,
  minimap and file buttons at the bottom right).
- `[camera] scroll=aoc`: AoC's smooth camera scrolling.
- `[general] skip_intro=1`: skip the intro movies.

## Status

- Scenario fix and economy: done and confirmed in play.
- Windowed and borderless mode (Alt+Enter toggle), editor resolution, camera scrolling: working.
- Attack tuning: settings are in `aimod.ini`.
- Attack routing: in development.
- An Art of Conquest port is planned (the AI classes are the same, the addresses differ).

## License

Copyright (C) 2026 Paul Hirch.

This mod is free software under the [GNU General Public License v3.0](LICENSE): you may use, share and change
it, and anything you distribute that is built from it must be published under the same license, with its
source code.

Empire Earth and its files belong to their rights holders and are not part of this repository; files derived
from them (decompilation, extracted strings and tables) are kept out of git (see `.gitignore`). Addresses,
offsets and similar facts about the game, including those cross-checked with the community's GPL-3.0 projects
at [EE-modders](https://github.com/EE-modders), are documented in `docs/`.
