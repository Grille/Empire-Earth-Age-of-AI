# Empire Earth tuning mod

A runtime mod for **Empire Earth 1.04 with the Difficulty Setting & Multiplayer Lobby add-on** (the
last official base-game exe; not Art of Conquest). It fixes and tunes the game from inside: a DLL in the
game folder patches the running exe, and the game's files stay unchanged. The engine loads every DLL
in its folder, so dropping the file there is enough.

The current focus is the **computer player**: it should play fair, build a real economy and army, defend
itself, and feel like an opponent with a character. The tooling here works for any part of the game: hooks,
control pipe, automated tests and the engine notes.

The mod needs the 1.04 exe of the 2017 update: the **GOG** version, the **Steam Gold Edition**
(`steamapps\common\Empire Earth Gold Edition\Empire Earth` - the same code, only a signature and a DPI manifest are
added) or the **Vanilla** setup from empireearth.eu. Old CD versions, no-CD exes and NeoEE have their code in other
places: the mod recognizes the exe when the game starts and stays switched off with another one, with a message
(docs/engine-notes.md, Exe builds). Art of Conquest (`EE-AOC.exe`) isn't supported yet. The mod coexists with community DLL
patches such as *Empire Earth Tweaks* or *dreXmod*, which patch different places.

**Documentation**

| Document | Contents |
|---|---|
| [docs/ai-analysis.md](docs/ai-analysis.md) | How the original AI works, where it fails, what the mod changes, and all test results |
| [docs/engine-notes.md](docs/engine-notes.md) | Verified addresses and data layouts of the engine |
| [docs/testing.md](docs/testing.md) | Autotest mode, test scenarios, log format, control pipe commands |
| [config/aimod.ini](config/aimod.ini) | Every setting, explained |
| [config/aimod_ai/README.md](config/aimod_ai/README.md) | The personalities and texts: format, expressions, values, situations |
| [config/aimod_ai/_persona.eeai](config/aimod_ai/_persona.eeai) | An empty personality with every key explained, to copy |
| [config/aimod_ai/_system.eeai](config/aimod_ai/_system.eeai) | The system lines the mod shows (English, German) |
| [config/aimod_ai/_template.eeai](config/aimod_ai/_template.eeai) | What the mod tells the language model (the AI voice) |

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
| Farm fields | The original AI left the fields around its granary as bare foundations (always on maps with fish, often on land), put nobody on finished ones and never replaced destroyed ones. It now builds them as far as its wood pays (keeping 50 wood per field for that), farms every one and lays out lost fields again. |
| More granaries | The original AI built a single granary (8 fields) all game; its food stayed short and spare citizens went to gold. It now adds granaries while it wants more food workers than its fields hold. |
| Citizens, towers and raids | Citizens no longer gather inside or walk past the range of enemy towers the AI has seen, nor go one by one to a field or mine where enemy soldiers stand; those working there are called back. The AI no longer plans its own towers and walls where the builders would walk into enemy fire. |
| Citizens fight back | Enemy citizens putting up a tower next to the AI's gatherers get attacked - the builders first, by its soldiers or its citizens - and in the early ages citizens beat off small raids when clearly stronger. |
| Paid upgrades | The original AI got all unit and tower upgrades of an epoch for free the moment it reached it. It now researches them at their buildings and pays, like a player. |
| Working build queue | The original build list started with a building that can't be placed, which blocked everything after it (granary, archery range, siege factory, university...). |
| Short ways to work | Citizens mine the free mine nearest to a drop site, move from oversupplied resources to short ones, and get the gathering upgrades the original AI rarely researched. |
| Epoch pacing | Timer or "as soon as affordable"; optionally waits for the human player. |

**Defence and army**

| Feature | What it does |
|---|---|
| Towers and walls | Towers at the front facing the enemy and at outlying mines instead of scattered around; a wall arc in front of them. |
| Home guard | The AI keeps an army at home that grows with its population cap (the original trained only for its next attack). |
| Muster | When an enemy army in its base is too strong to fight (twice its soldiers and towers), the AI stops sending every new soldier in alone: they gather beyond the base and strike together once they can win; citizens next to the enemy flee there too. |
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
- **Offers to you are messages:** an AI offering you an alliance (or peace inside your block) only says so; it
  stays at war until you set it to allied (2 minutes), and only then turns allied itself.
- **Stance lines** (`stance_lines`): only the AIs' stance changes towards you are shown, naming the AI ("Khan
  Temur is now your enemy"). The game's line for every change - also between AIs, without saying towards whom - is
  `stance_lines=2`; `0` shows none (the personalities still speak).
- **Alliance blocks:** everyone linked by alliances is one block, with peace inside it: two AIs allied with you
  (or with the same AI) don't fight each other. An AI at war with you that joins your block offers you peace.
  Whoever turns on a member breaks the block, and every other AI picks a side (`alliance_blocks`). Joining
  means peace with the whole block, so an AI refuses an alliance when it - or one of its allies - holds a grudge
  against one of yours (and says who stands in the way). Vassals have no say in their lord's alliances - they
  make peace as told, grumbling if they hold a grudge.
- **Capitulation:** when your army is in its base and the fight is lost, an AI offers peace and tribute
  ("set us allied"); accept within 2 minutes and it pays and stays loyal. Proud personalities hold out longer.
  A vassal has no diplomacy of its own: no offers, no betrayal, and whoever asks it for peace is sent to its lord.
  It still defends itself, and a lord who attacks his vassal sets it free.
- **Reputation:** breaking an alliance is seen by every AI - each thinks worse of the traitor, the more loyal it
  is (twice as much if the victim is its own ally). Attacking your own vassal is three times as bad, and right after
  accepting its capitulation (taking the tribute, then attacking) five times.
- **Victory over vassals:** vassals at war with each other would keep the game running for ever. Once every
  other player has capitulated to you, you are told so; play on as long as you like, and type `sieg` (or
  `victory`) in the chat to end the game as the winner. Resigning or exiting from the menu then counts as
  a victory too.
- **Ending the game yourself** (`auto_victory=0`): the game no longer ends on its own when one alliance is left;
  once nobody stands against you, a hint says so, and `sieg`/`victory`, Resign or Exit ends it as a victory.
- **Saved games:** the AIs remember across save and load - grudges, alliances, capitulations, pending offers
  (a small `.aimod` file next to each save; `save_state`).
- **Surrender:** an AI that has lost all its citizens and can't train new ones resigns (with a last line),
  instead of leaving its last buildings for you to hunt down (`surrender`, not in scenarios). It gives up to
  whoever destroyed the most of it lately, and doesn't capitulate or take sides in a war once it is beyond saving.
- **Personalities** (optional folder [`config/aimod_ai`](config/aimod_ai)): every AI gets a name, a favoured
  colour on random maps, character traits (aggression, loyalty, pride, greed, forgiveness, caution, talk) and
  its own lines for situations of the game: greetings, attacks, being attacked, new ages, winning and losing,
  diplomacy. Six personalities are included, in English and German; the game's language is detected
  (`[lines:de]`, `[lines:fr]`... replace the English `[lines]`). A saved game finds its personalities again.
  `computer.eeai` in the same folder is no personality but the fallback: neutral lines for what must not go
  unsaid (war, peace, offers waiting for your answer, capitulation, victory). A personality without a line for
  such a situation uses it, and AIs without any personality speak only these lines. `_persona.eeai` is an empty
  personality with every key explained: copy it to a new name to make your own.
- **The mod's own texts** are in the same folder too (files starting with `_` are no personalities):
  `_system.eeai` holds the system lines it shows you (offers waiting for your answer, chat hints, victory) in
  English and German - `[system:fr]` etc. adds a language -, `_template.eeai` everything it tells the language
  model (the AI voice: who the AI is, the rules, the game as it stands, the situations, the tools). Placeholders in
  braces are filled in; a key or file left out keeps the built-in text.
- **The format** of all these files: `key = value`, where a value is an expression - a `"string"` with
  `{placeholders}` and `{expressions}` in it, a `["list", "of", "strings"]` (one at random), conditions
  (`allied ? "Welcome back."`), fallbacks (`a ?? b`) - over the game as it stands: what the AI thinks of you, how
  strong each side is, how long the war has lasted, whether you betrayed it before, what you paid it, its epoch.
  Everything - the format, the expressions, every value, every situation - is in
  [`config/aimod_ai/README.md`](config/aimod_ai/README.md), which goes into the folder with the files. A mistake is
  logged with the file and line and only that key is skipped; `strict_parsing=1` in aimod.ini shows every mistake
  at once (for whoever writes texts).
- **Editing them in VS Code:** [`vscode/eeaimod-texts`](vscode/eeaimod-texts) highlights the `.eeai`
  files - sections, keys, strings, placeholders, expressions; an unknown key, situation or epoch name shows as an
  error. `python scripts/pack_vscode.py`, then `code --install-extension work/eeaimod-texts.vsix`
  and reload the window. The grammar takes the names from the DLL's sources (`scripts/gen_vscode_grammar.py`).
- **Play style** (experimental, `ai_style=1`, off by default): a personality can also change how its AI plays
  (attack timing, army share, home guard). Off, every AI plays the same, tested way.

- **AI voice** (optional, `[llm] enabled=1`): a language model on your PC (LM Studio, Ollama, llama.cpp - any
  server with the OpenAI chat API) speaks for the AIs. Their lines come in their own words, knowing the game's
  state (who is at war with whom, who is stronger, what they think of you, how many soldiers, citizens and buildings each of you has destroyed of the other's - in the whole game and in the last 20 minutes), and they answer when you write to them
  in the chat - the game's chat filter decides who gets your line (everyone, allies, enemies or the players you
  chose); of those, the one you name answers ("Temur, ..." - a typo or the first letters will do), else the last one
  you heard, else the only one that got it. In single player a line nobody will answer gets a system line saying
  why (no AI named, the filter reaches none or not the one you named, the model can't be reached; with the AI voice
  off, once a game: the AIs don't read the chat). Words only: the model decides
  nothing, and an AI can't be talked into anything - it knows its real terms for peace (the tribute that would buy
  it, by the same rules that decide, or why no price would: its pride after your declaration, an ally's veto, a
  grudge too deep), so it can name its price and its words match what it will do. Tribute amounts and names must survive the rewording, else the fixed line is shown;
  without a server (or too slow, `timeout`) the fixed lines show as before. Only on your PC - in multiplayer each
  player's own model (or none). `character=` in a personality file adds a few words about it for the model.
  - **The model decides** (`decide=1`, single player, a model with tool calls): your peace offers are answered by
    the model, and in the chat it can make peace, declare war on you or send you a tribute - real deals ("pay me
    1000 and we have peace" holds if it says so). The rules only limit it: what isn't possible right now (its
    pride after your declaration, an ally's veto, being a vassal, war within 5 minutes of an alliance, more than a
    quarter of its stock as tribute) is never offered to it, and without an answer in time the rules decide. Its
    nature and the rules' verdict are its advice. Everything else (answering war, capitulations, alliances between
    the AIs, betrayal) stays with the rules.
  - **The AIs talk to each other** (`ai_talk=1`, single player): when one AI says something to another (a
    declaration of war, a betrayal, an alliance offer, a capitulation), they talk it over - a few messages, each
    pair with its own memory of what they told each other (and what one tells another, you don't learn - it may tell
    you something else). Words only, their decisions stay with the rules. One exchange at a time, at most one a minute
    (`ai_talk_gap`), your own conversations go to the model first; `ai_talk_show=1` lets you read along.
  - **The model decides between the AIs too** (`ai_decide=1`, single player): an AI's alliance offer or capitulation
    to another AI is answered by the other's model (accept or refuse - and haggle: "an alliance against Gunther?
    250 gold first"), and in their talks an AI can offer peace, declare war on an ally or send a tribute. The rules
    still limit what is possible (an ally's veto, a vassal, war only after 5 minutes of alliance) and decide when
    the model doesn't answer; answering war, betrayal and taking sides in a split alliance stay with the rules.
  - **Nobody is special** (`equal=1`): the model isn't told who is human - you are another leader like the AIs, and
    no AI is told it is played by the computer.

The AIs can also explain themselves in the chat (`chat=1`, a debug aid): attacks, new ages, towers, walls,
siege, home guard.

## Installing (players)

1. Copy `eeaimod.dll` into the Empire Earth folder (next to `Empire Earth.exe`).
2. Copy [`config/aimod.ini`](config/aimod.ini) into the same folder and adjust it. Every option is
   explained in the file. Settings can differ per difficulty (`[easy]`, `[medium]`, `[hard]` override
   `[all]`).
3. Optional: copy the folder [`config/aimod_ai`](config/aimod_ai) into the same folder for the AI
   personalities. Add or edit `.eeai` files there to make your own; they are read when Empire Earth starts.
4. Start the game normally. Changes to `aimod.ini` take effect when a game starts or a save is loaded.
5. Optional: copy `dinput.dll` into the same folder too. The game then keeps its own settings (graphics,
   sound, scroll speed, the last game's options - what the options screen sets) in `eesettings.ini` in the
   game folder instead of the Windows registry, so they travel with the folder; the first start copies the
   registry's values into it. It is a stand-in Windows loads at the start: the game's one DirectInput call
   goes on to the system's `dinput.dll`. If another program in the folder already has a `dinput.dll`, keep
   that one - everything works without ours, the settings then simply stay in the registry. Delete it to go
   back to the registry (the file is then ignored).

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
- `[general] lead_targets=1` (default): ranged units and towers aim where a moving target will be when the
  projectile comes down, not where it stood when it was fired. Before, slow projectiles (arrows, catapults,
  cannon balls) missed nearly everything walking across their line of fire.
- `[general] spread_missiles=1` (default): AA towers and other missile launchers stop wasting missiles on an
  aircraft the missiles already on their way will destroy; they take another target in range or hold the shot.
  `lead_missiles=1` (off by default) makes homing missiles intercept instead of chase.
- Multiplayer (not tested yet): every player needs the same `eeaimod.dll`, the same `aimod.ini` and the same
  `aimod_ai` personalities. The game is lockstep - every PC simulates everything - so different settings make
  the games drift apart. The mod's decisions use only the game's own state, math and random numbers for this.
- `[general] skip_intro=1`: skip the intro movies.
- `[display] draw_limit=4096` (default): the game drew at most 512 units, buildings and missiles per frame - arrows
  in flight or stuck in the ground included (a 360-archer fight reached 445) - so in big fights, and the more so at
  high resolutions, the farthest objects blinked in and out. `0` keeps the 512.
- `[general] big_selection=1` (default): double-clicking a unit (all of its kind on screen) or dragging a box selected
  at most 63 units - with a large army only a part. Now up to about 240, as many as the game's selection message
  can carry; the message keeps its format, so it should stay compatible in multiplayer.
- `[sound] clamp_channels=1` (default): the "Max Number Of Sounds" slider goes to 200%, which asks for 32 sound
  channels, but the sound mixer has only 16 on current Windows. Sounds put on the 16 missing channels were silent,
  and a looping one (unit movement such as the U-boats', ambience) stayed stuck there and never played again -
  especially after changing the slider in a running game. The channels are now held to what the mixer has, so
  anything from 100% up gives the same working channels.
- `[sound] channels=32` (default): the mixer's 16 channels are only a fallback because Windows no longer reports
  hardware voices; Miles mixes in software and allows 63. The mod raises the maximum to this value, so the slider
  gives 8 (0%) to 32 (100% and up) channels: fewer sounds cut in big battles. `0` keeps the 16.

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
                    grid.c the unit grid (iterator), testcmd.c control-pipe commands only tests use,
                    testopp.c the sea opponent (tests: a human-like navy for P1)
                    econ.c budget, epoch plan, build queue, workers, research; sites.c gathering sites
                    attack.c attack routing, siege, unit choice; defence.c towers, walls, home guard
                    diplo.c diplomacy; persona.c personalities; chat.c chat lines
                    landing.c landings; navy.c the navy log (observation only)
                    display.c / present_d3d.c windowed mode (display_text.c text DC and bitmap fonts, display_diag.c
                    frame watch and diagnostics, playername.c the menu name fix), camera.c scrolling, editor_layout.h editor
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
