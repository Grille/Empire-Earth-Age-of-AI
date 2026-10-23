"""The game folder, as run_test.ps1 and scripts/game_dir.sh find it: env EE_GAME_DIR, else the line in the
repository's local/game_dir.txt (git-ignored), else the folder the repository sits in (<game>/_aimod)."""
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def game_dir():
    g = os.environ.get('EE_GAME_DIR')
    f = os.path.join(REPO, 'local/game_dir.txt')
    if not g and os.path.exists(f):
        g = next((ln.strip() for ln in open(f, encoding='utf-8') if ln.strip()), None)
    g = os.path.abspath(g or os.path.dirname(REPO))
    if not any(os.path.exists(os.path.join(g, x)) for x in ('Empire Earth.exe', 'EE-AOC.exe')):   # (or Art of Conquest's)
        raise SystemExit('no Empire Earth.exe (or EE-AOC.exe) in %s - name the game folder in EE_GAME_DIR or %s' % (g, f))
    return g
