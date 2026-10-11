# Send one command to the running game through eeaimod's control pipe and print the reply.
# usage: python eectl.py status | camera [x y] | speed [s] | pause | resume | quit | peek <hex> [len] | poke <hex> <bytes> | ping
import sys, threading, time

PIPE = r'\\.\pipe\eeaimod'


def _exchange(cmd, result):
    with open(PIPE, 'r+b', buffering=0) as p:
        p.write(cmd.encode() + b'\n')
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
    end = time.time() + connect_timeout
    while True:
        result, err = [], []

        def run():
            try:
                _exchange(cmd, result)
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
    print(send(' '.join(sys.argv[1:]) or 'status'), end='')
