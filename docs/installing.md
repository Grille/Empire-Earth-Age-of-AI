# Installing, the game versions, NeoEE and multiplayer

The details behind the README's short install: which exes the mod runs on, every step and option of the install,
NeoEE, multiplayer, and the status of each part.

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
([game-fixes.md](game-fixes.md#beside-other-dll-mods)).

## Installing

The mod keeps the game folder tidy: next to `Empire Earth.exe` go only `ageofai.dll` and `ageofai_config.exe`
(Windows and the game load DLLs from there only); everything else of the mod is in one folder `ageofai` beside them
- its settings, the personalities, the game's settings file, the log. Writing personalities? `ageofai_check.exe`, a
download of its own, goes there too: it checks them without starting the game
([config/personalities/README.md](../config/personalities/README.md)).

A release zip is laid out that way: unpacked into the game folder, it is installed. By hand:

1. Copy `ageofai.dll` into the Empire Earth folder (next to `Empire Earth.exe`), and make a folder `ageofai` there.
2. Copy [`config/ageofai.ini`](../config/ageofai.ini) into `ageofai` and adjust it. Every option is explained in the
   file. `[ai_base]` is every AI difficulty's (the medium AI plays it as it is); `[ai_easy]` and `[ai_hard]`
   override it for those two.
3. Optional: copy the folder [`config/personalities`](../config/personalities) into `ageofai` too, for the AI
   personalities and the mod's texts (without them the AIs say nothing and have no diplomacy of their own). Add or
   edit `.eeai` files there to make your own; they are read when Empire Earth starts.
4. Start the game normally. Changes to `ageofai.ini` take effect when a game starts or a save is loaded.

### The settings window

Instead of editing the file you can copy `ageofai_config.exe` next to `Empire Earth.exe` and start it: every
setting in one tree - the sections (General, Display, ... the AI), their topics (Movement, Navy, ...), a master
setting with the ones it switches - with its explanation, its value and its default, an AI setting's easy and hard
values where they differ; a search finds any of them; a value is checked as you type, as the game reads it, and Save
writes only what you changed, keeping the file's comments. It also lists keys in your file that this version no
longer knows (old experiments, typing mistakes) and can remove them. Without an `ageofai\ageofai.ini` it offers to
make one with the default settings. Help (F1) explains its colours and the rest.

Its Game exe button shows which exe is next to it and can give the game 4 GB of memory instead of 2 (the Large
Address Aware flag - big armies and maps need it; one bit in the exe's header, the game closed, the exe kept as
`Empire Earth.exe.original` and put back by Restore).

### The game's settings in a file

Optional, on the same Game page, "Settings in a file": the game then keeps its own settings (graphics, sound, scroll
speed, the last game's options - what the options screen sets) in `ageofai\eesettings.ini` instead of the Windows
registry, so they travel with the folder; the first start copies the registry's values into it. The game reads them
before it loads the mod, so this makes Windows load `ageofai.dll` earlier: it adds it to the imports of
`Low-Level Engine.dll` (the engine's code stays as it is; the file kept as `Low-Level Engine.dll.original`). The same
button takes it out again, and so does the game's "verify files" (Steam, GOG) - the settings are then in the registry
again. While it is on, the game needs `ageofai.dll`.

### Switching it off, uninstalling, the log

`ageofai\ageofai.ini` is required: without it Empire Earth shows an error and closes. To play the original game,
set `enabled=0` at the top of `ageofai.ini` (the mod then installs nothing), or delete `ageofai.dll` to
uninstall (with the settings in a file: switch that off first - the engine won't start without the DLL). A setting
missing from an older `ageofai.ini` takes its built-in default. The DLL writes a log to
`ageofai\ageofai_log.csv`.

The game starts borderless (`[display] display_mode`; Alt+Enter for a window, `native` for the game's fullscreen),
and the mod draws the picture itself (`[display] renderer = own`). A `ddraw.dll` wrapper in the game folder - GOG and
NeoEE ship dgVoodoo - needn't be removed: the mod doesn't load it unless `renderer = wrapper`
([game-fixes.md](game-fixes.md#window-and-picture)).

## NeoEE

The same steps, in NeoEE's game folder (`%LOCALAPPDATA%\Programs\Neo Empire Earth\Empire Earth` for its current
installer). The mod knows NeoEE's `Empire Earth.exe`: the start's log says `# build: NeoEE (ee's code)`. What
differs from the GOG version:

- NeoEE's exe has no 4 GB flag; `ageofai_config.exe` sets it as on the GOG exe (checked: the game starts and plays).
- NeoEE ships dreXmod 3.4, dgVoodoo as `ddraw.dll` and an English `Language.dll`. dreXmod's FixTnLVertexBuffer does
  what `[display] vertex_sysmem` does (the mod finds it done and leaves it). The mod's `[camera]` settings do what
  dreXmod's camera does and more (a zoom further out): set `<Camera><Enabled>` to false in `dreXmod.config`. Its menu
  resolution and the mod's `[display]` settings haven't been tried together by hand yet.
- NeoEE's data changes a few rules (its `Data\db` files: a population limit of 4000, more starting-resource levels
  and game variants). The AI adapts to the data, but it was measured on the original rules.
- NeoEE's Art of Conquest (`EE-AOC.exe`) is recognized too, but off in a normal start until test games have run on
  it.
- Not on NeoEE's online servers: see [Multiplayer](#multiplayer).

## Multiplayer

Not tested yet. Every player needs the same `ageofai.dll`, the same `ageofai.ini` and the same personalities. The
game is lockstep - every PC simulates everything - so different settings make the games drift apart; the mod's
decisions use only the game's own state, math and random numbers for this. Nothing checks yet that every PC has the
same; until the mod does (the host's settings for every PC), don't use it in online games - and not in NeoEE's
ranked games at all: its rank system and cheat detector expect the original game, and how they treat the mod is a
question for NeoEE's people, not something to try.

## Status

- **The computer player is the mod's own** since 2026-11-14 (`[ai_base] game_ai = 0`): the game's computer player
  no longer runs for the mod's AIs, and what they do is written anew - the citizens' work, building, research and the
  epochs, production and the soldiers by territory, priests and prophets, a scenario's targets. Ships and aircraft by
  role are still being written. What is done and what is open: [game-ai-routines.md](game-ai-routines.md#the-games-computer-player-off-2026-11-14-ai_base-game_ai--0-the-default).
- **Confirmed in play:** the scenario fix, the early economy (fair prices, economy first, budget), windowed and
  borderless mode, editor resolution, camera scrolling.
- **Tested in automated AI games and test scenarios, needs playtesting against a human:** most of the AI. The
  numbers are in [ai-analysis.md](ai-analysis.md#test-results-so-far) and the commits.
- **NeoEE:** recognized and tested in automated games (the same checks and hooks as on the GOG exe); not played by
  hand yet.
- **Experimental, off by default:** what hasn't shown a gain yet ([experiments.md](experiments.md)), the
  personalities' play styles (`ai_style`).
- **Art of Conquest, on since 2026-11-05:** every address the mod uses is known for it too (its load checks hold).
  Automated test games: 14 maps and seeds of 45 minutes each, AoC and the base game the same games, no crash and the
  AI as strong in both (alive, epoch, workers, soldiers within a few percent). The base game's online screens, which
  AoC removed, and two of the off-map fix's ten calls are the base game's only. Not played by hand much yet.
