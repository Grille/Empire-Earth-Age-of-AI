# Contributing

Thank you for wanting to help. This file says what the project is for, what it takes, and how a change has to look
to be merged. Read it once before you start; the details it points to are in the other docs.

The mod is made largely with AI coding tools, and the project says so openly. Contributions made with AI help are
welcome on the same terms as any other: you answer for what you send - you have read it, built it and tested it.

## Scope

**In scope**

- **The computer player** (the main focus): an AI that plays fair, without the original's resource cheat, and still
  gives a good game - economy, defence, attacks, navy, diplomacy, personalities.
- **Fixes of the engine's own bugs and limits**: crashes, overflows, caps from 1999 (the 512 drawn objects, the
  63-unit selection ...), each behind its own setting.
- **Display and interface**: windowed and borderless mode, the editor's resolution, camera, sound.
- **Movement and pathing**: routes, turning, collision, formations - the player's units and the AI's alike.
- **The tooling**: the spec of the game, the reverse-engineering scripts, the tests, the `.eeai` language and its VS
  Code extension, `aimod_config.exe`.
- **Personalities and texts** (`config/personalities/`): new characters, better lines.
- **NeoEE**, the community's online version, as much as the GOG and Steam versions: its `Empire Earth.exe` is the
  2017 update's code in another layout, and every change has to work on both (README "NeoEE").
- **The Art of Conquest port** (in progress, see the spec's two builds; NeoEE's AoC goes with it).

**Out of scope**

- **Other exes**: versions before 1.04, the CD game with its copy protection on. The mod supports the 1.04 exe of
  the 2017 update (GOG, Steam Gold Edition, empireearth.eu's Vanilla setup), NeoEE's and any other 1.04 exe whose
  code passes the start's checks (a no-CD exe of the retail game: a report names its `# build:` line), and is being
  ported to Art of Conquest; another build
  with code of its own is a large job and needs talking about first.
- **An AI that cheats or sees what a human can't**: no free resources, no map knowledge it hasn't explored, no
  enemy units it doesn't see. An easier AI for the easy difficulty is fine; a stronger one through cheating is not.
- **Changes to the game's files**: the mod patches the running exe and leaves the game's files alone.
- **The game's files or anything taken from them in the repository**: no exe, DLL, `.dat`, decompiled code, extracted
  strings or tables. They live in `local/` (git-ignored) on your machine. Facts about the game - addresses, offsets,
  field layouts, numbers - go into `spec/` and `docs/`.
- **Online games before the settings sync**: multiplayer is being built (two-PC test games stay in sync,
  `scripts/test/mp.sh`; the mod has a network message of its own), but nothing yet makes sure every PC runs the
  same DLL and settings. Until it does, the mod is not for online games, NeoEE's servers included. Changes to the
  simulation must keep multiplayer possible (see Determinism).
- **NeoEE's ranked games**: its rank system and cheat detector expect the original game. Whether the mod may ever
  play there is for NeoEE's people to decide - don't try it, and don't send code that hides the mod from them.
- **Languages other than English**, until the release: the texts and the docs are English for now, a file per
  language comes then.

Not sure whether something fits? Open an issue and ask before you write the code.

## Ways to help

- **Play and report.** The most useful thing anyone can do. A good report says which version you play (GOG,
  Steam, NeoEE ...) and which exe the mod found (the `# build:` line near the top of `aimod_econ.csv` in the game
  folder's `aimod`), the other mods in the folder (NeoEE ships dreXmod and dgVoodoo), your `aimod.ini` (or what you changed in it), the map and the
  difficulty, what the AI did and what you expected it to do, and - when you can - a saved game from shortly before.
  For a crash, also the log and the last thing you did.
- **Write a personality.** Copy `config/personalities/_template.eeai`, which explains every key; the format is in
  [config/personalities/README.md](config/personalities/README.md). `strict_parsing=1` in `aimod.ini` shows all mistakes at
  once.
- **Reverse engineering.** A function, field or number of the game found and checked goes into `spec/` (below).
  Say how you know it: what the code does with it, what you measured in a running game.
- **Code.** Below.

## Before you write code

- **A larger change starts as an issue**: what it changes for the player, how you will show that it works. Small
  fixes can go straight to a pull request.
- **Set up the build** as [README.md](README.md) says under Building and Development setup: llvm-mingw, Git Bash,
  Python in `local/venv`. Tell the scripts where the game is (`local/game_dir.txt` or `EE_GAME_DIR`).
- **Read** [README.md](README.md)'s Project layout and the doc for your area: [docs/engine-notes.md](docs/engine-notes.md)
  (the engine), [docs/ai-analysis.md](docs/ai-analysis.md), [docs/economy.md](docs/economy.md) and
  [docs/navy.md](docs/navy.md) (the AI), [docs/testing.md](docs/testing.md) (the tests), [docs/scripts.md](docs/scripts.md)
  (the `.eeai` language).
- [CLAUDE.md](CLAUDE.md) holds the project's working rules in short form, written for AI coding sessions. If you use
  one, it reads that file by itself. It is also a good summary for a human, though some of it (the post-commit hook,
  the test slots beside a running game) is about the maintainer's machine.

## How the code is built

The mod is C, built 32-bit by llvm-mingw into `dll/eeaimod.dll` (plus `dinput.dll` and `aimod_config.exe`):

```sh
sh dll/build.sh
```

**The build must stay free of warnings and pass its checks.** It runs them itself:

| Check | What it holds |
|---|---|
| `scripts/build/layers.py` | which part of `dll/` may use which (below); `--why <folder>` lists the uses |
| `scripts/build/style.py` | the C style rules the compiler doesn't check (below) |
| `scripts/build/spec_check.py` | the rules of `spec/` the C# compiler leaves open |
| `scripts/build/settings_check.py` | `dll/settings/rows.h` and `config/aimod.ini` say the same |

**One translation unit**: `dll/eeaimod.c` includes every other `.c` file. A new file is included there, in the place
where what it uses is already defined.

**The layers.** Each folder of `dll/` is a part, and a part may use only the parts below it:

- `base/` - the DLL's own helpers, nothing of the game.
- `game/api/` - the game's code and objects, one module: the only place that knows addresses and offsets.
- `game/` - the game as the rest of the mod sees it.
- `engine/`, `ai/` - fixes of the game and the computer players, on top of `game/`. What the AI says goes through
  `voice/say.h`.
- `script/` - the `.eeai` language; it knows no game, only `base/`. What it needs from the program is
  `script/host.h`.
- `voice/`, `game_script/`, `settings/`, `tool/`, `test/` - see [CLAUDE.md](CLAUDE.md) for what each may use.

A use across the layers the build refuses is a sign the code belongs elsewhere: move the shared helper down into
`base/` or `game/` instead of adding an exception.

## The game's code: spec/ and no literal addresses

`spec/` is the project's knowledge of the game: every class, field, function, call site and number the mod uses,
for each build, written as C# declarations that compile (`dotnet build` in `spec/`). `scripts/build/gen_api.py`
turns them into C in `dll/game/api/gen/`; the format is in `spec/attributes.cs`'s header.

- **Never write an address, offset, slot or size in the code.** Use the generated name or its accessor:
  `unit_get_hp(u)`, `unit_distance(a, b)`, `data_ai_managers_call_rm_update`. The raw values are visible only inside
  `game/api/`.
- **Never edit `dll/game/api/gen/` by hand**: change the spec and rebuild.
- **A new value needs both builds** (the 2017 update and Art of Conquest). If you can only find one, say so in the
  pull request; `scripts/re/port_addrs.py` helps find the other. NeoEE needs no column of its own: it plays the
  2017 update's.
- **NeoEE's exe has the same code in another layout**: its import table is elsewhere, its game code is in a section
  called `.data`, and it has two `.text` sections. So an import-table slot is an `[import_slot("dll", "name")]`, found
  by name at load (`spec_check.py` insists), and code that scans the exe goes by the image's ranges
  (`base/patch.c`), never by a section's name.
- **What you inferred but haven't confirmed is `[guess]`**, with the evidence in its `///` comment. The mod should
  not rely on a guess; `scripts/re/spec_guesses.py` lists them.
- **At every start the spec is checked against the running exe.** After a spec change, read the start of the log:
  it says `# spec: N checks against the exe, all hold` or names what fails.
- A hook on a call the mod patches is a `[call_site]` of its function: the generated `patch_x(hook)` takes a hook
  of the function's own type, so a wrong signature doesn't build - fix the spec, not the cast.

## Settings

Every behaviour the mod adds has a setting in `aimod.ini`, so a player can turn it off and a test can compare it.

- A setting is a row in `dll/settings/rows.h` (kind, section, key, default, limits) and its key, with that default
  and an explanation, in `config/aimod.ini`. The code reads `g_set.key` (or `g_set_ai[difficulty].key` for the AI's
  per-difficulty settings), never the file.
- **New behaviour of the AI is off by default until a test shows it helps** (below). The shipped defaults are the AI
  at its strongest without cheating; only the easy difficulty eases off.
- An engine fix that only removes a bug can be on by default once it is shown to cause no harm.

## Determinism

Empire Earth's multiplayer is lockstep: every PC simulates the whole game and must come to the same result. So
anything that changes the game - an AI's decision, a unit's movement - must give the same bits on every PC:

- Random decisions use `rnd_game()` (`dll/game/random.c`), never `rand()` or the clock. `rnd()` is only for what is
  shown and doesn't change the game (which chat line is picked).
- Math whose result decides something: the helpers in `dll/base/math.c` (`det_log2f` ...), not library functions
  whose last bit can differ between CPUs.
- Simulation decisions don't depend on anything local: the wall clock, the frame rate, the window, the language
  model, files only one PC has.

This also makes tests repeatable: with a fixed seed a scenario plays the same game every run.

## Testing

What a change needs depends on what it touches. The tools are in [docs/testing.md](docs/testing.md); tests always
run headless and with the shipped `config/aimod.ini`, never your own.

| Change | Show |
|---|---|
| A move of code that should change nothing (files, functions between files) | `scripts/build/code_same.py HEAD` says "0 differ" |
| `dll/game/api/` | the same machine code (`code_same.py`), or - where that can't be - the same game: the same `game_seed`, the logs compared |
| A bug fix | a test or probe that showed the bug, before and after |
| New or changed AI behaviour | an A/B over many seeds and maps (`scripts/test/suite.sh`, `h2h.sh`), the numbers in the pull request; one game says nothing. Measured on the original game's data (NeoEE's changes a few rules, a population limit of 4000 among them) |
| The text engine, or which line is chosen | `texts corpus` before and after in the same game - identical unless meant to differ |
| The spec | the start log's spec line, in both builds if you can |
| Code that reads or patches the exe itself (imports, sections, scans) | a test game on GOG and on NeoEE side by side (`EE_GAME_DIR=<NeoEE's game folder>`): the `# build:` and `# spec:` lines, the same hooks |
| A setting | `settings_check.py` (the build) and `aimod_config.exe` shows it |

Some generated files have to be made again when their sources change (the table "Generators" in
[CLAUDE.md](CLAUDE.md)): the build does most of them, the docs' tables and the VS Code grammar have their own
scripts.

## Code style

Read the code around yours and write like it. Beyond that:

- **`internal` at file level, `static` only inside a function** (a local that keeps its value). `internal` is
  `base/lang.h`'s name for C's file-level `static`. `style.py` checks it.
- **The `*` on the name**: `int *a, b;`, never `int* a, b;` (which makes only `a` a pointer). `style.py` checks it.
- **Names**: lower case with underscores. A part's functions share a short prefix (`fl_` the fleet, `cv_` cover,
  `ms_` mass ...). An accessor of a game object is named by its C type without `ee_`: `unit_get_hp`,
  `civ_mgr_get_timer`, `unit_type_get_hp`.
- **Comments say why**: what the game does, what was measured, why this way and not the obvious one. Plain, dense
  sentences, as in the files around. A file starts with what it is for and where it is described in `docs/`.
- **Line endings and encoding**: `dll/` mixes CRLF and LF files, and a few are latin-1. Keep each file as it is
  (`.c`, `.sh`, `.py` are LF by `.gitattributes`). Don't reformat a file you only change a few lines in.
- **No new dependencies** in the DLL: it links only Windows' own libraries. Python scripts may use what
  `requirements.txt` lists; ask before adding to it.
- **The `.eeai` language stays strict**: a mistake is reported with its file and line, nothing is guessed, and
  there are no compatibility shims before the release.

## Commits and pull requests

- **One step per commit**, built and checked. The message says what changed and how you checked it - the test, the
  numbers, what you looked at in the game. The project's history is the place to see the style.
- **A pull request says**: what it changes for the player, the setting (and its default), how it was tested, and
  for an AI change the A/B results.
- Keep unrelated changes out: a fix and a cleanup of the code around it are two pull requests.
- Don't commit anything from `local/`, the game's files, built DLLs or logs.

## License

The mod is under the [GNU General Public License v3.0](LICENSE). By sending a contribution you agree that it is
published under the same license. Only send what you have the right to: your own work, or code under a license
compatible with the GPL-3.0, with its source and author named. Code copied from the game - decompiled or not - can
never be part of it.
