# Empire Earth: Age of AI

*An unofficial fan mod. Not affiliated with or endorsed by the rights holders of Empire Earth.*

The name says it twice: the computer players are the heart of the mod, and the mod itself exists because of AI -
the game's code was reverse-engineered and the mod written largely with AI coding tools. Short: **EE: Age of AI**.

A runtime mod for **Empire Earth 1.04 with the Difficulty Setting & Multiplayer Lobby add-on** (the last official
base-game exe) and **The Art of Conquest**. It fixes and tunes the game from inside: a DLL in the game folder
patches the running exe, and the game's files stay unchanged. The engine loads every DLL in its folder, so dropping
the file there is enough.

## What it changes

- **The computer player** (the main focus): the original AI pays about 1% of the real prices and still can't run
  an economy without that cheat. The mod's AI pays what you pay and never sees what a human couldn't: it builds a
  real economy, defends its base, attacks as one army round the defences it has seen, and goes to sea on water
  maps. Tuned for 200-400 population per player. [docs/ai-features.md](docs/ai-features.md)
- **Diplomacy and personalities**: the AIs answer war, make peace and alliances, capitulate and betray, remember
  across save and load, and speak with a character of their own - optionally through a language model on your PC
  that you can talk to in the chat. [docs/diplomacy.md](docs/diplomacy.md)
- **Fixes of the game itself**: windowed and borderless mode, the scenario editor at higher resolutions, smoother
  camera, no blinking objects in big fights, walls that follow the ground, selections of more than 63 units,
  transports that share out the units, straight routes, turning, collision and formations that hold together,
  more sound channels. [docs/game-fixes.md](docs/game-fixes.md)

Every feature has its own setting in `ageofai.ini`, explained in the file and in `ageofai_config.exe`.

## Which game

The mod needs the 1.04 exe of the 2017 update: the **GOG** version, the **Steam Gold Edition**
(`steamapps\common\Empire Earth Gold Edition\Empire Earth` - the same code, only a signature and a DPI manifest
added), the **Vanilla** setup from empireearth.eu or **NeoEE** (the community's online version: see [NeoEE](#neoee)).
Another copy of 1.04 - the retail CD game's exe without its copy protection, say - plays too when its code is the
same: an exe the mod doesn't know by its file is checked against every place in its code the mod uses when the game
starts, and the mod switches on only when every one holds (the log's `# build:` and `# spec:` lines). With the copy
protection still on (SafeDisc, which Windows 10 and 11 no longer run anyway) and with versions before 1.04 the mod
stays switched off, with a message.

Art of Conquest (`EE-AOC.exe`, GOG and Steam) plays with the mod too, since 2026-11-05 (see [Status](#status)).

The mod coexists with community DLL patches such as *Empire Earth Tweaks* or *dreXmod* (3.4 too: none of its 116
patches touches a place the mod patches); a few of their options overlap with the mod's
([docs/game-fixes.md](docs/game-fixes.md#beside-other-dll-mods)).

## Installing

The mod keeps the game folder tidy: next to `Empire Earth.exe` go only `ageofai.dll` and `ageofai_config.exe`
(Windows and the game load DLLs from there only); everything else of the mod is in one folder `ageofai` beside them
- its settings, the personalities, the game's settings file, the log. Writing personalities? `ageofai_check.exe`, a
download of its own, goes there too: it checks them without starting the game (config/personalities/README.md).

1. Copy `ageofai.dll` into the Empire Earth folder (next to `Empire Earth.exe`), and make a folder `ageofai` there.
2. Copy [`config/ageofai.ini`](config/ageofai.ini) into `ageofai` and adjust it. Every option is explained in the
   file. `[ai_base]` is every AI difficulty's (the medium AI plays it as it is); `[ai_easy]` and `[ai_hard]`
   override it for those two.
3. Optional: copy the folder [`config/personalities`](config/personalities) into `ageofai` too, for the AI
   personalities and the mod's texts (without them the AIs say nothing and have no diplomacy of their own). Add or
   edit `.eeai` files there to make your own; they are read when Empire Earth starts.
4. Start the game normally. Changes to `ageofai.ini` take effect when a game starts or a save is loaded.
   Instead of editing the file you can copy `ageofai_config.exe` next to `Empire Earth.exe` and start it: every
   setting in one tree - the sections (General, Display, ... the AI), their topics (Movement, Navy, ...), a
   master setting with the ones it switches - with its explanation, its value and its default, an AI setting's
   easy and hard values where they differ; a search finds any of them; a value is checked as you type, as the
   game reads it, and Save writes only what you changed, keeping the file's comments. It also lists keys in your
   file that this version no longer knows (old experiments, typing mistakes) and can remove them. Without an
   `ageofai\ageofai.ini` it offers to make one with the default settings. Its Game exe button shows which exe is next
   to it and can give the game 4 GB of memory instead of 2 (the Large Address Aware flag - big armies and maps
   need it; one bit in the exe's header, the game closed, the exe kept as `Empire Earth.exe.original` and put
   back by Restore). Help (F1) explains its colours and the rest.
5. Optional: on the same Game page, "Settings in a file": the game then keeps its own settings (graphics, sound,
   scroll speed, the last game's options - what the options screen sets) in `ageofai\eesettings.ini` instead of
   the Windows registry, so they travel with the folder; the first start copies the registry's values into it.
   The game reads them before it loads the mod, so this makes Windows load `ageofai.dll` earlier: it adds it to
   the imports of `Low-Level Engine.dll` (the engine's code stays as it is; the file kept as
   `Low-Level Engine.dll.original`). The same button takes it out again, and so does the game's "verify files"
   (Steam, GOG) - the settings are then in the registry again. While it is on, the game needs `ageofai.dll`.

`ageofai\ageofai.ini` is required: without it Empire Earth shows an error and closes. To play the original game,
set `enabled=0` at the top of `ageofai.ini` (the mod then installs nothing), or delete `ageofai.dll` to
uninstall (with the settings in a file: switch that off first - the engine won't start without the DLL). A setting
missing from an older `ageofai.ini` takes its built-in default. The DLL writes a log to
`ageofai\ageofai_log.csv`.

For windowed or borderless play set `[display] display_mode`; the mod draws the picture itself or through a
DirectDraw wrapper such as DDrawCompat ([docs/game-fixes.md](docs/game-fixes.md#window-and-picture)).

### NeoEE

The same steps, in NeoEE's game folder (`%LOCALAPPDATA%\Programs\Neo Empire Earth\Empire Earth` for its current
installer). The mod knows NeoEE's `Empire Earth.exe`: the start's log says `# build: NeoEE (ee's code)`. What
differs from the GOG version:

- NeoEE's exe has no 4 GB flag; `ageofai_config.exe` sets it as on the GOG exe (checked: the game starts and plays).
- NeoEE ships dreXmod 3.4, dgVoodoo as `ddraw.dll` and an English `Language.dll`. dreXmod's FixTnLVertexBuffer does
  what `[display] vertex_sysmem` does (the mod finds it done and leaves it). dreXmod's camera and menu resolution
  and the mod's `[camera]` and `[display]` settings haven't been tried together by hand yet.
- NeoEE's data changes a few rules (its `Data\db` files: a population limit of 4000, more starting-resource levels
  and game variants). The AI adapts to the data, but it was measured on the original rules.
- NeoEE's Art of Conquest (`EE-AOC.exe`) is recognized too, but off in a normal start until test games have run on
  it.
- Not on NeoEE's online servers: see [Multiplayer](#multiplayer).

### Multiplayer

Not tested yet. Every player needs the same `ageofai.dll`, the same `ageofai.ini` and the same personalities. The
game is lockstep - every PC simulates everything - so different settings make the games drift apart; the mod's
decisions use only the game's own state, math and random numbers for this. Nothing checks yet that every PC has the
same; until the mod does (the host's settings for every PC), don't use it in online games - and not in NeoEE's
ranked games at all: its rank system and cheat detector expect the original game, and how they treat the mod is a
question for NeoEE's people, not something to try.

## Status

- **Confirmed in play:** the scenario fix, the early economy (fair prices, economy first, budget), windowed and
  borderless mode, editor resolution, camera scrolling.
- **Tested in automated AI games and test scenarios, needs playtesting against a human:** most of the AI - build
  queue, mines, defences, home guard, the army, attack routing, siege, the navy, diplomacy and personalities. The
  numbers are in [docs/ai-analysis.md](docs/ai-analysis.md#test-results-so-far) and the commits.
- **NeoEE:** recognized and tested in automated games (the same checks and hooks as on the GOG exe); not played by
  hand yet.
- **Experimental, off by default:** what hasn't shown a gain yet ([docs/experiments.md](docs/experiments.md)), the
  personalities' play styles (`ai_style`).
- **Art of Conquest, on since 2026-11-05:** every address the mod uses is known for it too (its load checks hold).
  Automated test games: 14 maps and seeds of 45 minutes each, AoC and the base game the same games, no crash and the
  AI as strong in both (alive, epoch, workers, soldiers within a few percent). The base game's online screens, which
  AoC removed, and two of the off-map fix's ten calls are the base game's only. Not played by hand much yet.

## Documentation

| Document | Contents |
|---|---|
| [docs/ai-features.md](docs/ai-features.md) | What the mod's computer player does differently, feature by feature, with the measurements |
| [docs/diplomacy.md](docs/diplomacy.md) | Diplomacy, the personalities and their texts, the AI voice (language model) |
| [docs/game-fixes.md](docs/game-fixes.md) | Display, editor, camera, walls, selection, transports, movement and sound fixes |
| [config/ageofai.ini](config/ageofai.ini) | Every setting, explained |
| [config/personalities/README.md](config/personalities/README.md) | The personalities and texts: format, expressions, values, situations |
| [config/personalities/_template.eeai](config/personalities/_template.eeai) | An empty personality with every key explained, to copy |
| [docs/ai-analysis.md](docs/ai-analysis.md) | How the original AI works, where it fails, and the test results |
| [docs/economy.md](docs/economy.md), [docs/navy.md](docs/navy.md) | The AI's economy against the base game's AI; ship roles and the navy |
| [docs/experiments.md](docs/experiments.md) | The AI experiments that are off: why they failed |
| [docs/engine-notes.md](docs/engine-notes.md) | Verified addresses and data layouts of the engine |
| [docs/triggers.md](docs/triggers.md), [docs/renderer.md](docs/renderer.md) | The scenario triggers; the mod's own renderer |
| [docs/units.md](docs/units.md) | Every unit and building type: id, identifier, name (generated) |
| [docs/development.md](docs/development.md) | Building, the reverse-engineering setup, the project layout |
| [docs/testing.md](docs/testing.md), [docs/scripts.md](docs/scripts.md) | Automated tests, the control pipe, the test scripts |
| [CONTRIBUTING.md](CONTRIBUTING.md) | What the project takes, how to report, the rules a change has to follow |

## License

Copyright (C) 2026 Paul Hirch.

This mod is free software under the [GNU General Public License v3.0](LICENSE): you may use, share and change
it, and anything you distribute that is built from it must be published under the same license, with its
source code.

Empire Earth: Age of AI is an unofficial fan project, not affiliated with or endorsed by the rights holders of
Empire Earth; the name Empire Earth is theirs and is used only to say which game the mod is for.
Empire Earth and its files belong to their rights holders and are not part of this repository; files derived
from them (decompilation, extracted strings and tables) are kept out of git (see `.gitignore`). Addresses,
offsets and similar facts about the game, including those cross-checked with the community's GPL-3.0 projects
at [EE-modders](https://github.com/EE-modders), are documented in `docs/`.
