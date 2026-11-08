# Send one command to the running game through eeaimod's control pipe and print the reply.
# usage: python eectl.py status | camera [x y] | speed [s] | pause | resume | quit | peek <hex> [len] | poke <hex> <bytes> | ping
#        RUN=<name>: the test game of that name (run_test.ps1 -Name, env RUN), waiting while it is queued; without it the
#        player's game; ee_mcp.py goes through here too
#        python eectl.py --folder (with RUN): the folder of its files (log, dumps) from the repository, while it plays
import os, sys, threading, time

if os.environ.get('SLOT'):
    sys.exit('SLOT is gone: runs are queued, RUN=<name> names one (run_test.ps1 -Name)')
PIPE = r'\\.\pipe\eeaimod'
QUEUE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'local', 'work', 'queue')


def _in_queue(kind, name):   # run_test.ps1's files: wait.<time>.<name>, run.<slot>.<name> - the part after the kind
    try:
        files = os.listdir(QUEUE)
    except OSError:
        return []
    return [f.split('.')[1] for f in files if f.split('.')[0] == kind and f.split('.')[-1].lower() == name.lower()]


def run_slot(name):   # the slot the run of that name plays in, waiting while it is queued; None: no such run
    while True:
        queued = _in_queue('wait', name)   # first: a run's file is there before its ticket goes
        playing = _in_queue('run', name)
        if playing:
            return playing[0]
        if not queued:
            return None
        time.sleep(0.5)


def _exchange(pipe, cmd, result):
    with open(pipe, 'r+b', buffering=0) as p:
        p.write(cmd.encode('cp1252', errors='replace') + b'\n')   # the game's code page, like the replies
        chunks = []
        while True:
            try:
                b = p.read(65536)
            except OSError:          # server disconnected after the reply
                break
            if not b:
                break
            chunks.append(b)
    result.append(b''.join(chunks).decode('cp1252', errors='replace'))   # the game's code page (personality names)


def send(cmd, connect_timeout=10.0, reply_timeout=20.0):
    pipe = PIPE
    name = os.environ.get('RUN')
    if name:
        slot = run_slot(name)
        if slot is None:
            return 'error: no run named %s\n' % name
        pipe += slot
    end = time.time() + connect_timeout
    while True:
        result, err = [], []

        def run():
            try:
                _exchange(pipe, cmd, result)
            except OSError as e:
                err.append(e)
        t = threading.Thread(target=run, daemon=True)
        t.start()
        t.join(reply_timeout)
        if t.is_alive():
            return 'error: no reply within %.0f s\n' % reply_timeout
        if result:
            return result[0]
        if time.time() > end:       # pipe busy (another client) or game not running
            return 'error: control pipe not available (game not running?) %s\n' % (err[0] if err else '')
        time.sleep(0.2)


if __name__ == '__main__':
    sys.stdout.reconfigure(errors='replace')
    if sys.argv[1:] == ['--folder']:
        slot = run_slot(os.environ.get('RUN', ''))
        if slot is None:
            sys.exit('no run named %s' % os.environ.get('RUN', ''))
        print('local/work/slot%s' % slot)
        sys.exit(0)
    print(send(' '.join(sys.argv[1:]) or 'status'), end='')
