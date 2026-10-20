# sourced by the shell scripts: GAME = the game folder (as run_test.ps1 finds it) - env EE_GAME_DIR, else the line
# in the repository's game_dir.txt (git-ignored), else the folder the repository sits in (<game>\_aimod).
# REPO = the repository. Empire Earth.exe must be in GAME.
REPO="${REPO:-$(cd "$(dirname "$0")/.." && pwd)}"   # a caller that changed its folder sets REPO first
GAME="$EE_GAME_DIR"
[ -z "$GAME" ] && [ -f "$REPO/game_dir.txt" ] && GAME="$(grep -v '^[[:space:]]*$' "$REPO/game_dir.txt" | head -1 | tr -d '\r')"
[ -z "$GAME" ] && GAME="$(cd "$REPO/.." && pwd)"
GAME="$(cd "$GAME" 2>/dev/null && pwd)" || { echo "game folder not found: $GAME"; exit 1; }
[ -f "$GAME/Empire Earth.exe" ] || { echo "no Empire Earth.exe in $GAME - name the game folder in EE_GAME_DIR or $REPO/game_dir.txt"; exit 1; }
