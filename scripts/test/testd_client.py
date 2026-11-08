# The way to the test server (scripts/test/testd.py) for run_test.py and eectl.py: one line of JSON each way over
# localhost. Kept apart from the server so that a client - one per waiting run - loads nothing but sockets. The first
# client starts the server, on its own: it outlives the session that happened to start it.
import json, os, socket, subprocess, sys, time

PORT = int(os.environ.get('EE_TESTD_PORT') or 33320)   # below the games' own, 33334 + 2 n; another: a server of its own, to try changes to it
HERE = os.path.dirname(os.path.abspath(__file__))


def _start_server():
    exe = getattr(sys, '_base_executable', sys.executable)   # the real python: a venv's python.exe is one more process
    detached = 0x00000008 | 0x00000200   # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
    for flags in (detached | 0x01000000, detached):   # CREATE_BREAKAWAY_FROM_JOB, where the caller's job allows it
        try:
            subprocess.Popen([exe, os.path.join(HERE, 'testd.py')], creationflags=flags, close_fds=True, cwd=HERE,
                             stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return
        except OSError:
            pass


def connect(start=True):   # the connection, or None; the server started when none runs (two starting: one gets the port)
    end = time.time() + 20
    started = False
    while True:
        try:
            s = socket.create_connection(('127.0.0.1', PORT), timeout=2)
            s.settimeout(None)
            return s
        except OSError:
            if not start or time.time() > end:
                return None
        if not started:
            _start_server()
            started = True
        time.sleep(0.2)


def send(s, obj):
    s.sendall(json.dumps(obj).encode() + b'\n')


def where(name):   # the slot the run of that name plays in, waiting while it is queued; None: no such run
    s = connect(start=False)
    if not s:
        return None
    with s:
        send(s, {'op': 'where', 'name': name})
        line = s.makefile('rb').readline()
    return json.loads(line).get('slot') if line else None
