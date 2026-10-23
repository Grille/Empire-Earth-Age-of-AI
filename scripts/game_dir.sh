# sourced by the shell scripts: GAME = the game folder (as run_test.ps1 finds it) - env EE_GAME_DIR, else the line
# in the repository's local/game_dir.txt (git-ignored), else the folder the repository sits in (<game>\_aimod).
# REPO = the repository. GAME_EXE = the game in GAME: Empire Earth.exe, or Art of Conquest's EE-AOC.exe (its folder in
# EE_GAME_DIR: the test runs there).
REPO="${REPO:-$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")/.." && pwd)}"   # this file's folder's parent; a caller may set REPO
GAME="$EE_GAME_DIR"
[ -z "$GAME" ] && [ -f "$REPO/local/game_dir.txt" ] && GAME="$(grep -v '^[[:space:]]*$' "$REPO/local/game_dir.txt" | head -1 | tr -d '\r')"
[ -z "$GAME" ] && GAME="$(cd "$REPO/.." && pwd)"
GAME="$(cd "$GAME" 2>/dev/null && pwd)" || { echo "game folder not found: $GAME"; exit 1; }
if [ -f "$GAME/Empire Earth.exe" ]; then GAME_EXE="Empire Earth.exe"
elif [ -f "$GAME/EE-AOC.exe" ]; then GAME_EXE="EE-AOC.exe"
else echo "no Empire Earth.exe (or EE-AOC.exe) in $GAME - name the game folder in EE_GAME_DIR or $REPO/local/game_dir.txt"; exit 1; fi
