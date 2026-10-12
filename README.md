# Empire Earth tuning mod

A runtime mod for **Empire Earth 1.04 with the Difficulty Setting & Multiplayer Lobby add-on** (the
last official base-game exe; not Art of Conquest). It fixes and tunes the game from inside: a DLL in the
game folder patches the running exe, and the game's files stay unchanged. The engine loads every DLL
in its folder, so dropping the file there is enough.

The current focus is the **computer player**: it should play fair, build a real economy and army, defend
itself, and feel like an opponent with a character. The tooling here works for any part of the game: hooks,
control pipe, automated tests and the engine notes.

Retail/Gold and the **Steam Gold Edition** (`steamapps\common\Empire Earth Gold Edition\Empire Earth`)
both work: the Steam exe has the same code, only a signature and a DPI manifest are added, and there's no
DRM wrapper. Art of Conquest (`EE-AOC.exe`) isn't supported yet. The mod coexists with community DLL
patches such as *Empire Earth Tweaks* or *dreXmod*, which patch different places.

**Documentation**

| Document | Contents |
|---|---|
| [docs/ai-analysis.md](docs/ai-analysis.md) | How the original AI works, where it fails, what the mod changes, and all test results |
| [docs/engine-notes.md](docs/engine-notes.md) | Verified addresses and data layouts of the engine |
| [docs/testing.md](docs/testing.md) | Autotest mode, test scenarios, log format, control pipe commands |
| [config/aimod.ini](config/aimod.ini) | Every setting, explained |
| [config/aimod_ai/eroberer.ini](config/aimod_ai/eroberer.ini) | The personality file format |

## Computer player

The original AI cheats: by default it pays about 1% of the real prices. With cheating switched off, it
can't run its economy, gets stuck in the early epochs, only trains soldiers for its next attack and leaves its
base undefended. This mod changes that. Every feature has its own setting in `aimod.ini`; the AI never gets
information a human couldn't have (it only uses enemy buildings it has seen).

**Economy**

| Feature | What it does |
|---|---|
| Fair prices | The AI pays what you pay (configurable per difficulty, e.g. a discount only on Hard). |
| Economy first | Before farms (Copper Age), the AI grows its workforce, counts the wild food it has found as its budget, saves for the next epochs and keeps only a token army. |
| Military budget | Soldiers are paid from a share of the starting stock and of the income, never from the epoch reserve; from the Copper Age on most of the income goes to the army. |
| Working build queue | The original build list started with a building that can't be placed, which blocked everything after it (granary, archery range, siege factory, university...). |
| Short ways to work | Citizens mine the free mine nearest to a drop site, move from oversupplied resources to short ones, and get the gathering upgrades the original AI rarely researched. |
| Epoch pacing | Timer or "as soon as affordable"; optionally waits for the human player. |

**Defence and army**

| Feature | What it does |
|---|---|
| Towers and walls | Towers at the front facing the enemy and at outlying mines instead of scattered around; a wall arc in front of them. |
| Home guard | The AI keeps an army at home that grows with its population cap (the original trained only for its next attack). |
| Unit choice | When the AI asks for a unit it can't pay for or train, it trains the strongest soldier it can afford. |
| Counter-siege | Soldiers near the base attack enemy siege weapons shelling it, when clearly stronger than the escort. |

**Attacks**

| Feature | What it does |
|---|---|
| Attack tuning | Fewer, bigger waves instead of a stream of small, wasted attacks. |
| Routing | Armies march around the enemy towers they know about and look for undefended targets instead of running into the tower line. |
| Siege | The AI builds siege weapons when it knows enemy defences, keeps them at firing range and breaks through walls. |

**Scenarios and gathering**

| Feature | What it does |
|---|---|
| Scenario fix | AIs in scenarios started from the menu play like in skirmish games (the original turns most of their planning off). |
| Safe gathering | Optional: citizens only gather near their base, on routes away from known enemies. |

## Diplomacy and personalities

The original AI has no diplomacy at all: declaring war on it changes nothing on its side, and it never makes
peace or alliances. With the mod:

- **War is answered:** whoever declares war on an AI gets war back, with a revenge attack and a taunt.
- **Peace and alliances:** an AI judges peace offers by its opinion of you, the strength of both sides, how
  recently you attacked it and its character. Tribute buys goodwill. AIs ally against a much stronger enemy,
  and a disloyal one may betray a weak ally. Nothing changes in scenarios or with locked teams.
- **Capitulation:** when your army is in its base and the fight is lost, an AI offers peace and tribute
  ("set us allied"); accept within 2 minutes and it pays and stays loyal. Proud personalities hold out longer.
- **Victory over vassals:** vassals at war with each other would keep the game running for ever. Once every
  other player has capitulated to you, you are told so; play on as long as you like, and type `sieg` (or
  `victory`) in the chat to end the game as the winner. Resigning or exiting from the menu then counts as
  a victory too.
- **Saved games:** the AIs remember across save and load - grudges, alliances, capitulations, pending offers
  (a small `.aimod` file next to each save; `save_state`).
- **Surrender:** an AI that has lost all its citizens and can't train new ones resigns (with a last line),
  instead of leaving its last buildings for you to hunt down (`surrender`, not in scenarios).
- **Personalities** (optional folder [`config/aimod_ai`](config/aimod_ai)): every AI gets a name, a favoured
  colour on random maps, character traits (aggression, loyalty, pride, greed, forgiveness, caution, talk) and
  its own lines for situations of the game: greetings, attacks, being attacked, new ages, winning and losing,
  diplomacy. Six personalities are included, in English and German; the game's language is detected
  (`[lines:de]`, `[lines:fr]`... replace the English `[lines]`). A saved game finds its personalities again.
- **Play style** (experimental, `ai_style=1`, off by default): a personality can also change how its AI plays
  (attack timing, army share, home guard). Off, every AI plays the same, tested way.

The AIs can also explain themselves in the chat (`chat=1`, a debug aid): attacks, new ages, towers, walls,
siege, home guard.

## Installing (players)

1. Copy `eeaimod.dll` into the Empire Earth folder (next to `Empire Earth.exe`).
2. Copy [`config/aimod.ini`](config/aimod.ini) into the same folder and adjust it. Every option is
   explained in the file. Settings can differ per difficulty (`[easy]`, `[medium]`, `[hard]` override
   `[all]`).
3. Optional: copy the folder [`config/aimod_ai`](config/aimod_ai) into the same folder for the AI
   personalities. Add or edit `.ini` files there to make your own; they are read when Empire Earth starts.
4. Start the game normally. Changes to `aimod.ini` take effect when a game starts or a save is loaded.

`aimod.ini` is required: without it Empire Earth shows an error and closes. To play the original game,
set `enabled=0` at the top of `aimod.ini` (the mod then installs nothing), or delete `eeaimod.dll` to
uninstall. A setting missing from an older `aimod.ini` takes its built-in default. The DLL writes a log to
`aimod_econ.csv` in the game folder.

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
  any other. `lock_mouse` keeps the cursor inside while the game is in front: `auto` (default) only in
  borderless, so edge scrolling doesn't slide onto a second monitor, while a window stays free; `1` always, `0` never.
- `[display] mode=borderless`: a frameless window over the whole monitor, the game image scaled to it
  (aspect kept). Alt+Enter switches between window and borderless. The window is DPI-aware (`dpi_aware`),
  so Windows display scaling doesn't blur it; `smooth=1` filters the scaled image.
- `[editor] resolution=aoc`: scenario editor at 1024x768 with The Art of Conquest's layout; `game` or any
  `WIDTHxHEIGHT` extends that layout to the resolution (map view fills the screen, panels at the bottom,
  minimap and file buttons at the bottom right).
- `[camera] scroll=aoc`: AoC's smooth camera scrolling.
- `[general] skip_intro=1`: skip the intro movies.

## Status

- **Confirmed in play:** scenario fix, the early economy (fair prices, economy first, budget), windowed and
  borderless mode, editor resolution, camera scrolling.
- **Tested in automated AI games and test scenarios, needs playtesting against a human:** build queue,
  mines, gathering upgrades, defences, home guard and army size, attack routing, siege, diplomacy and
  personalities. The numbers are in [docs/ai-analysis.md](docs/ai-analysis.md#test-results-so-far).
- **Experimental, off by default:** personality play styles (`ai_style`).
- **Planned:** an Art of Conquest port (the AI classes are the same, the addresses differ).

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
The community's projects at [EE-modders](https://github.com/EE-modders) are a good second source (the game's
text in all languages, the DB file formats, other DLL mods).

## Testing

The DLL has an **autotest mode** and a **control pipe**, so AI games run unattended. Scenario and save
runs at the same speed are byte-identical, which makes A/B tests exact; random maps vary between runs, so
those are compared over several runs. See [docs/testing.md](docs/testing.md).

```sh
# one 75-minute AI-vs-AI game on a random map, logs to work/autotest_<stamp>_<tag>.csv
powershell -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees -Tag _try \
    -IniExtra "random_map=Continental,ai_count=3,seed=101,avoid_towers=1"
# the 4-map suite for several variants, scored per variant
sh scripts/suite.sh 50 "base:" "smart:avoid_towers=1"
# talk to the running game
venv/Scripts/python scripts/eectl.py status | army 2 | diplo | persona | say 2 Hello | quit
```

## Project layout

```
dll/              the mod (C), one module per file:
                    eeaimod.c core hooks + logging, settings.c aimod.ini, control.c control pipe
                    econ.c budget, epoch plan, build queue, workers, research; sites.c gathering sites
                    attack.c attack routing, siege, unit choice; defence.c towers, walls, home guard
                    diplo.c diplomacy; persona.c personalities; chat.c chat lines
                    display.c / present_d3d.c windowed mode, camera.c scrolling, editor_layout.h editor
                    randmap.c test-game setup, watch.c watchpoints, trace.c call tracing
config/           aimod.ini and the aimod_ai personalities, shipped to players
scenarios/        setup scripts that turn a save into a test scenario (attacks, sieges, raids)
scripts/          test runner, suite, log analysis, control client, MCP server, RE helpers
ghidra_scripts/   Ghidra headless script that names and decompiles the exe
mod/              generator for a tuned dbcpbehavior.dat (AI settings table; superseded by aimod.ini)
docs/             engine notes, AI analysis, testing guide
```

## License

Copyright (C) 2026 Paul Hirch.

This mod is free software under the [GNU General Public License v3.0](LICENSE): you may use, share and change
it, and anything you distribute that is built from it must be published under the same license, with its
source code.

Empire Earth and its files belong to their rights holders and are not part of this repository; files derived
from them (decompilation, extracted strings and tables) are kept out of git (see `.gitignore`). Addresses,
offsets and similar facts about the game, including those cross-checked with the community's GPL-3.0 projects
at [EE-modders](https://github.com/EE-modders), are documented in `docs/`.
