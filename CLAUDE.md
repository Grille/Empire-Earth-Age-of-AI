# eeaimod - working notes for Claude sessions

An AI and tuning mod for Empire Earth 1.04 (32-bit, German retail + DSML), injected as `eeaimod.dll`. The game
folder: env `EE_GAME_DIR`, else `game_dir.txt` in the repo (git-ignored), else the repo's parent (`<game>\_aimod`,
the setup here) - scripts find it through `scripts/game_dir.sh` / `game_dir.py` / run_test.ps1; paths in the repo
(`setup=scenarios/x.eeai`, `CONFIG=...`) are relative to the repo. What the mod does: README.md; how it is tested:
docs/testing.md.
Several sessions may work in this tree at the same time - see Git below.

## Build and install

- `sh dll/build.sh` builds `dll/eeaimod.dll` and `dll/dinput.dll` (llvm-mingw in `tools/`, 32-bit; it also
  regenerates `dll/persist_layout.h` and `dll/cmd_verbs.h`). One translation unit: `dll/eeaimod.c` includes the rest.
  A build must be free of warnings.
- `sh scripts/install.sh` puts the committed build into the game: built from a clean `git archive HEAD` (never the
  working tree), copied under a reservation of the game folder (a loaded DLL can't be replaced). Commit first.
- A task that needs the game folder to itself (an install, a slot-0 run): `sh scripts/reserve.sh "<what>" <command>`
  - test runs start no game meanwhile, the running ones (and the player's) are waited for; nobody has to pause.
- Game addresses live only in `dll/addr.h`, by name with one value per exe build: `GA(G_WORLD_CLOCK)`,
  `GF(FN_CAN_BUILD)(pl, t, 0x20, 0)`, `GL(TRAIN_SITES)` - never a literal address in the code (another build, e.g.
  Art of Conquest, is one more column).
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

One language for personalities, system texts, prompts and test scripts (dll/expr.c, docs/scripts.md,
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
- Comments and docs: plain, dense sentences in the style of the surrounding code; say why, not only what.
