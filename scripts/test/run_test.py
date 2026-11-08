# Plays one test game: hands it to the test server (scripts/test/testd.py, started if none runs), waits, prints the
# server's lines; the last one is "finished in .. s real time (started in .. s), exit code .., log: <path>". The game is
# queued and plays when its turn comes, beside the others and the player's game. A client killed while its game is
# queued takes it out of the queue; once it plays, the game plays to its end and its log is archived all the same.
# usage: python scripts/test/run_test.py [-Save aitest.ees] [-TimeoutSec 900] [-Tag _x] [-IniExtra "a=1,b=2"] ...
#   -Scenario <name> (without .scn): Data\Scenarios\<name>.scn through the game's real "play scenario" path
#   -Name <letters, digits, _ and -> (env RUN): the name to reach the running game by (RUN=<name> eectl.py ...)
#   -Config <ageofai.ini> (env CONFIG), -AiDir <folder> (env AI_DIR), -Settings <eesettings.ini> (env SETTINGS), -Dll <path>
#   (env DLL): this run's own, relative to the repository; -Mute 0: the player's own sound settings; -Exe <name>: a
#   renamed copy of the exe; -Quiet: only the last line. The game folder: scripts/game_dir.py (env EE_GAME_DIR).
#   EE_TEST_GAMES: at most that many games at a time, every session's together (default half the logical processors).
#        python scripts/test/run_test.py --status       what plays and what waits
import json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [HERE, os.path.dirname(HERE)]
import testd_client
from game_dir import REPO, game_dir

FLAGS = {'-Save': 'save', '-TimeoutSec': 'timeout', '-Tag': 'tag', '-Scenario': 'scenario', '-IniExtra': 'ini',
         '-Exe': 'exe', '-Config': 'config', '-AiDir': 'ai_dir', '-Settings': 'settings', '-Dll': 'dll', '-Name': 'name',
         '-Mute': 'mute'}


def main():
    e = os.environ
    if e.get('SLOT') or e.get('SLOTS'):
        sys.exit('SLOT / SLOTS are gone: runs are queued, RUN=<name> names one to reach it by (eectl.py too)')
    req = {'save': 'aitest.ees', 'timeout': '900', 'tag': '', 'scenario': '', 'ini': '', 'exe': '', 'mute': '1',
           'config': e.get('CONFIG', ''), 'ai_dir': e.get('AI_DIR', ''), 'settings': e.get('SETTINGS', ''),
           'dll': e.get('DLL', ''), 'name': e.get('RUN', '')}
    quiet = status = False
    args = sys.argv[1:]
    while args:
        a = args.pop(0)
        if a == '-Quiet':
            quiet = True
        elif a == '--status':
            status = True
        elif a in FLAGS and args:
            req[FLAGS[a]] = args.pop(0)
        else:
            sys.exit('run_test.py: what is %s? (the head of scripts/test/run_test.py lists the arguments)' % a)
    s = testd_client.connect(start=not status)
    if not s:
        if status:
            print('the test server is not running: nothing plays, nothing waits')
            return 0
        sys.exit('the test server did not start (local/work/testd.log)')
    if status:
        testd_client.send(s, {'op': 'status'})
    else:
        req.update(op='run', repo=REPO, game=game_dir(), env=dict(e), session=e.get('CLAUDE_CODE_SESSION_ID', 'by hand'))
        testd_client.send(s, req)
    rc = None
    for line in s.makefile('rb'):
        m = json.loads(line)
        if 'done' in m:
            print(m['done'])
            rc = m['rc']
            break
        if not quiet:
            print(m['say'], flush=True)
    if rc is None:
        sys.exit('the test server went away (local/work/testd.log)')
    return rc


if __name__ == '__main__':
    sys.stdout.reconfigure(errors='replace')
    sys.exit(main())
