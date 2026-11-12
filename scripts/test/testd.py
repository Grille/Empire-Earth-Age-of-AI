# The test server: one process that queues the test games of every session and plays them (the player, 2026-11-08:
# "make it a proper py server that just runs continuously and manages the jobs" - a PowerShell runner per game took 81 MB
# of private memory, compiling its C# at the start, a game 446 MB). run_test.py hands it a game and waits; the first
# client starts it, and it runs on. When this file changes it hands over: it takes no new client (the next one starts
# the new server) and ends with its last game - the two share the slots through the named objects below, so nothing
# collides meanwhile.
# Log: local/work/testd.log.
#
# THE QUEUE. The next game is the one whose session has the fewest games playing, the oldest of those: a session with
# nothing playing goes first, a batch of 300 can't keep another session's tests waiting behind it, and a session alone
# gets every place. A game starts once slots held < EE_TEST_GAMES (the client's, default half the logical processors)
# and enough memory is free - never less than EE_TEST_MIN_FREE_GB (default MIN_FREE_GB), the games started in the last minute reckoned at their size
# already (they grow while loading); with no test game playing that doesn't hold, or it could wait for ever. One start
# a second: not all loading their data at once. A running game is never stopped for another.
#
# A SLOT n of 1..30 is where a game plays, none of the callers' business: test ini, log and dumps in local\work\slot<n>\ (the
# DLL learns it from EEAIMOD_SLOT / EEAIMOD_SLOT_DIR), control pipe \\.\pipe\eeaimod<n>, network ports moved by 2n, the
# game's single-instance check patched at the start (docs/testing.md, Parallel runs). Who holds what is told by named
# objects of this Windows session, not by files: creating one is a single step that either makes it or finds it there,
# and it is gone once the last process holding a handle ends, however it ended.
#   eeaimod_slot<n>      the slot: the server and (a handle given to it at the start) the game - a game that outlives a
#                        killed server keeps its slot, so no second game plays into its test ini, log and pipe
#   eeaimod_game_<pid>   a test game (held by the game only): how a test game is told from the player's
#   eeaimod_reserved_<game folder>  scripts/test/reserve.sh holds the game folder: no game starts meanwhile
import ctypes, filecmp, gzip, json, os, re, shutil, socket, subprocess, sys, threading, time, traceback, winreg
from ctypes import wintypes as W

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from testd_client import PORT

MIN_FREE_GB = 3
GAME_GB = 0.5   # what a game takes once loaded (measured 2026-11-08: 446 MB private, in mid game)
LOG = os.path.join(HERE, '..', '..', 'local', 'work', 'testd.log')

# the build's own: its registry key, and the three patches of a suspended start - the loading splash's jne
# (spec/game/hooks.cs code_splash_start_jne), the single-instance check's je after its CreateMutexA (both builds'
# 0x535CA7 / 0x540D73) and the mutex's name (in .data, read only by that CreateMutexA: 0x538840 / 0x54398E)
BUILDS = {
    True: {'key': r'Software\Mad Doc Software\EE-AOC', 'splash': 0x53F2FE, 'multi': 0x540DA6, 'mutex': 0x8C10DC,
           'mutex_name': b'MadDocSoftwarePresentsEmpireEarthExpansion', 'multi_bytes': bytes((0x0F, 0x84, 0xC8, 3, 0, 0))},
    False: {'key': r'Software\SSSI\Empire Earth', 'splash': 0x534400, 'multi': 0x535CDA, 'mutex': 0x8AA964,
            'mutex_name': b'StainlessSteelStudiosPresentsEmpireEarth', 'multi_bytes': bytes((0x0F, 0x84, 0xBE, 2, 0, 0))},
}
SOUND = 'Sound Volume', 'Music Volume', 'Music Enabled'

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
HANDLE = W.HANDLE


class STARTUPINFO(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('r', W.LPWSTR), ('desktop', W.LPWSTR), ('title', W.LPWSTR), ('x', W.DWORD), ('y', W.DWORD),
                ('w', W.DWORD), ('h', W.DWORD), ('xc', W.DWORD), ('yc', W.DWORD), ('fill', W.DWORD), ('flags', W.DWORD),
                ('show', W.WORD), ('r2', W.WORD), ('r3', W.LPVOID), ('i', HANDLE), ('o', HANDLE), ('e', HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [('hp', HANDLE), ('ht', HANDLE), ('pid', W.DWORD), ('tid', W.DWORD)]


class MEMORYSTATUSEX(ctypes.Structure):
    _fields_ = [('len', W.DWORD), ('load', W.DWORD), ('total', ctypes.c_ulonglong), ('avail', ctypes.c_ulonglong),
                ('rest', ctypes.c_ulonglong * 5)]


for fn, res, args in (
    ('CreateEventW', HANDLE, (W.LPVOID, W.BOOL, W.BOOL, W.LPCWSTR)),
    ('OpenEventW', HANDLE, (W.DWORD, W.BOOL, W.LPCWSTR)),
    ('CloseHandle', W.BOOL, (HANDLE,)),
    ('GetCurrentProcess', HANDLE, ()),
    ('DuplicateHandle', W.BOOL, (HANDLE, HANDLE, HANDLE, ctypes.POINTER(HANDLE), W.DWORD, W.BOOL, W.DWORD)),
    ('CreateProcessW', W.BOOL, (W.LPCWSTR, W.LPWSTR, W.LPVOID, W.LPVOID, W.BOOL, W.DWORD, W.LPCWSTR, W.LPCWSTR,
                                ctypes.POINTER(STARTUPINFO), ctypes.POINTER(PROCESS_INFORMATION))),
    ('ReadProcessMemory', W.BOOL, (HANDLE, W.LPCVOID, W.LPVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t))),
    ('WriteProcessMemory', W.BOOL, (HANDLE, W.LPVOID, W.LPCVOID, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t))),
    ('VirtualProtectEx', W.BOOL, (HANDLE, W.LPVOID, ctypes.c_size_t, W.DWORD, ctypes.POINTER(W.DWORD))),
    ('ResumeThread', W.DWORD, (HANDLE,)),
    ('WaitForSingleObject', W.DWORD, (HANDLE, W.DWORD)),
    ('GetExitCodeProcess', W.BOOL, (HANDLE, ctypes.POINTER(W.DWORD))),
    ('TerminateProcess', W.BOOL, (HANDLE, W.UINT)),
    ('SetThreadExecutionState', W.DWORD, (W.DWORD,)),
    ('GlobalMemoryStatusEx', W.BOOL, (ctypes.POINTER(MEMORYSTATUSEX),)),
):
    f = getattr(k32, fn)
    f.restype, f.argtypes = res, args


def new_named(name):   # a handle to the new object, or None when it exists already
    h = k32.CreateEventW(None, True, False, 'Local\\' + name)
    if h and ctypes.get_last_error() == 183:   # ERROR_ALREADY_EXISTS
        k32.CloseHandle(h)
        return None
    return h


def test_named(name):
    h = k32.OpenEventW(0x100000, False, 'Local\\' + name)   # SYNCHRONIZE
    if h:
        k32.CloseHandle(h)
    return bool(h)


def log(*a):
    try:
        with open(LOG, 'a', encoding='utf-8') as f:
            print(time.strftime('%F %T'), *a, file=f)
    except OSError:
        pass


lock = threading.Lock()         # the queue, the playing games, the sound
start_lock = threading.Lock()   # one game being started: a game without its marker is never seen by another's check
waiting, playing = [], []       # the jobs
seq = 0


class Job:
    def __init__(self, req, conn):
        self.__dict__.update(req)
        self.conn, self.slot, self.since = conn, None, 0
        self.limit = int(self.env.get('EE_TEST_GAMES') or 0) or (os.cpu_count() or 2) // 2
        self.limit = max(1, min(30, self.limit))
        self.min_free = float(self.env.get('EE_TEST_MIN_FREE_GB') or MIN_FREE_GB)

    def tell(self, **m):
        try:
            self.conn.sendall(json.dumps(m).encode() + b'\n')
        except OSError:
            pass   # its client is gone: the game plays on, the log is archived all the same

    def say(self, text):
        self.tell(say=text)

    def path(self, p):   # relative to the repository
        return p if os.path.isabs(p) else os.path.join(self.repo, p)


def prepare(j):   # what can be wrong with a run, said before it queues
    g = j.game
    j.aoc = (j.exe or ('EE-AOC.exe' if os.path.isfile(os.path.join(g, 'EE-AOC.exe')) else 'Empire Earth.exe')) == 'EE-AOC.exe'
    j.exe = j.exe or ('EE-AOC.exe' if j.aoc else 'Empire Earth.exe')
    if not os.path.isfile(os.path.join(g, j.exe)):
        return 'no %s in %s - name the game folder in EE_GAME_DIR or local/game_dir.txt' % (j.exe, g)
    if j.name and not re.fullmatch(r'[\w-]+', j.name):
        return '-Name / RUN: letters, digits, _ and - only'
    if j.scenario:
        # beside other games the file may be open in another: copied only when it differs
        src, dst = (os.path.join(g, 'Data', d, j.scenario + e) for d, e in (('Scenarios', '.scn'), ('Saved Games', '.ees')))
        if not os.path.isfile(src):
            return 'no scenario %s' % src
        with start_lock:
            if not os.path.exists(dst) or not filecmp.cmp(src, dst, shallow=False):
                shutil.copy(src, dst)
        j.save = j.scenario + '.ees'
    if re.search('[-/]', j.save):
        return 'save name must not contain - or /'
    # the settings the DLL plays with. Default: the shipped ones - what players get, never the player's own ageofai.ini
    # in the game folder's ageofai\ (its experiments would bias every A/B); CONFIG=<game>\ageofai\ageofai.ini tests it.
    j.config = os.path.abspath(j.path(j.config or r'config\ageofai.ini'))
    j.ai_dir = os.path.abspath(j.path(j.ai_dir or r'config\personalities'))
    if not os.path.isfile(j.config):
        return 'config not found: ' + j.config
    if not os.path.isdir(j.ai_dir):
        return 'personality folder not found: ' + j.ai_dir
    j.settings = j.path(j.settings or r'tests\config\eesettings.ini')
    if j.dll:
        j.dll = j.path(j.dll)
        if not os.path.isfile(j.dll):
            return '-Dll not found: ' + j.dll
        with open(os.path.join(g, 'ageofai.dll'), 'rb') as f:
            if b'EEAIMOD_DLL' not in f.read():
                return 'the installed ageofai.dll predates -Dll (install a current build once)'
    # the test ini = tests/config/autotest.ini + -IniExtra; an extra key replaces the same key from the file (the DLL
    # reads the first occurrence)
    extra = [x for x in j.ini.split(',') if x]
    for i, x in enumerate(extra):
        m = re.match(r'\s*setup\s*=(.*)$', x)
        if not m:
            continue
        files = []
        for f in m.group(1).split(';'):   # relative to the repository (tests/scenarios/x.eeai), made absolute for the game
            f = f.strip()
            if f:
                f = os.path.normpath(j.path(f))
                if not os.path.exists(f):
                    return 'setup: no file ' + f
                files.append(f)
        extra[i] = 'setup=' + ';'.join(files)
    if j.scenario:
        extra.append('scenario_mode=1')
    # the game's command line: the save, or (multiplayer) none - the main menu, where the DLL hosts or joins
    j.args = '-loadgame "%s"' % j.save
    mp = next((x.split('=', 1)[1].strip() for x in extra if re.match(r'\s*mp\s*=', x)), None)
    if mp in ('host', 'join'):
        j.args = ''
    elif mp:
        return 'mp must be host or join'
    extra += ['config=' + j.config, 'ai_dir=' + j.ai_dir]
    j.extra = extra
    return None


# -- the player's sound. With the game's settings in the registry a run mutes the sound there, once for all games that
# run at a time, and the last one restores it; a backup file covers an interrupted run. The user copy keeps the last
# settings that weren't muted, so a backup taken in a muted state can't lose the player's real volumes.
def sound_files(j):
    w = os.path.join(j.repo, 'local', 'work')
    return os.path.join(w, 'sound_settings_backup.json'), os.path.join(w, 'sound_settings_user.json')


def restore_sound(j):
    backup, user = sound_files(j)
    if not os.path.exists(backup):
        return
    b = json.load(open(backup))
    if int(b[SOUND[0]]) + int(b[SOUND[1]]) == 0 and os.path.exists(user):
        b = json.load(open(user))
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, BUILDS[j.aoc]['key'], 0, winreg.KEY_SET_VALUE) as k:
        for n in SOUND:
            winreg.SetValueEx(k, n, 0, winreg.REG_DWORD, int(b[n]))
    os.remove(backup)


def mute_sound(j):
    backup, user = sound_files(j)
    if os.path.exists(backup):
        return
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, BUILDS[j.aoc]['key'], 0, winreg.KEY_READ | winreg.KEY_SET_VALUE) as k:
        cur = {n: winreg.QueryValueEx(k, n)[0] for n in SOUND}
        json.dump(cur, open(backup, 'w'))
        if int(cur[SOUND[0]]) + int(cur[SOUND[1]]) > 0:
            json.dump(cur, open(user, 'w'))
        for n in SOUND:
            winreg.SetValueEx(k, n, 0, winreg.REG_DWORD, 0)


def file_has(path, what):
    try:
        with open(path, 'rb') as f:
            return what in f.read()
    except OSError:
        return False


def uses_settings_file(g):   # ageofai.dll an import of the engine (ageofai_config.exe --settings-file on), or the old dinput.dll
    return file_has(os.path.join(g, 'Low-Level Engine.dll'), b'ageofai_settings') or file_has(os.path.join(g, 'dinput.dll'), b'EEAimodSettings')


def foreign_game(j):   # a game running that is no test's - the player's own
    r = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq ' + j.exe, '/FO', 'CSV', '/NH'], capture_output=True, text=True,
                       creationflags=0x08000000)
    return any(not test_named('eeaimod_game_' + m) for m in re.findall(r'^"[^"]*","(\d+)"', r.stdout, re.M))


def wait_reservation(j):
    if j.env.get('EEAIMOD_RESERVED'):   # the reserving task's own run
        return
    name = 'eeaimod_reserved_' + os.path.abspath(j.game).rstrip('\\').lower().replace('\\', '/')
    told = False
    while test_named(name):
        if not told:
            j.say('waiting: the game folder is reserved (scripts/test/reserve.sh)')
            told = True
        time.sleep(5)


def patch(hp, at, want, put):   # only while the bytes are the expected ones
    cur = ctypes.create_string_buffer(len(want))
    n = ctypes.c_size_t()
    if not k32.ReadProcessMemory(hp, at, cur, len(want), ctypes.byref(n)) or cur.raw != want:
        return False
    old = W.DWORD()
    return bool(k32.VirtualProtectEx(hp, at, len(put), 0x40, ctypes.byref(old))
                and k32.WriteProcessMemory(hp, at, put, len(put), ctypes.byref(n)))


def give(hp, h):   # a copy of the handle h in process hp (DUPLICATE_SAME_ACCESS)
    th = HANDLE()
    return bool(k32.DuplicateHandle(k32.GetCurrentProcess(), h, hp, ctypes.byref(th), 0, False, 2))


def start_game(j, env, headless, slot_h):
    # started suspended and patched before the game runs: the single-instance check nopped (it plays beside others; the
    # mutex is created before the engine loads our DLL) and the mutex renamed, its first letter T (a test game no longer
    # holds the player's mutex, so the player's game, started by hand while tests run, doesn't find it taken and quit
    # without a word), and when headless the loading splash off (its jne -> jmp, see display.c): it comes up before the
    # engine loads our DLL, centred on screen, and takes the focus
    b = BUILDS[j.aoc]
    exe = os.path.join(j.game, j.exe)
    si = STARTUPINFO()
    si.cb = ctypes.sizeof(si)
    if headless:
        si.flags = 0x80   # STARTF_FORCEOFFFEEDBACK - no busy cursor while it starts
    pi = PROCESS_INFORMATION()
    cmd = ctypes.create_unicode_buffer('"%s" %s' % (exe, j.args))
    block = '\0'.join('%s=%s' % kv for kv in sorted(env.items(), key=lambda kv: kv[0].upper())) + '\0\0'
    if not k32.CreateProcessW(exe, cmd, None, None, False, 0x4 | 0x400, block, j.game, ctypes.byref(si), ctypes.byref(pi)):   # CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT
        raise OSError('CreateProcess failed: %d' % ctypes.get_last_error())
    if headless and not patch(pi.hp, b['splash'], bytes((0x75, 0x2F)), b'\xEB'):
        j.say('warning: loading splash not switched off (unexpected bytes at %X)' % b['splash'])
    if not patch(pi.hp, b['multi'], b['multi_bytes'], b'\x90' * 6):
        j.say('warning: single-instance check not switched off (unexpected bytes at %X)' % b['multi'])
    if not patch(pi.hp, b['mutex'], b['mutex_name'] + b'\0', b'T'):
        j.say("warning: mutex not renamed (unexpected bytes at %X) - the player's game won't start while this one runs" % b['mutex'])
    # the game gets handles of its own to the slot and to eeaimod_game_<pid>, before it runs: they live as long as it
    # does, whatever happens to this server
    marker = k32.CreateEventW(None, True, False, 'Local\\eeaimod_game_%d' % pi.pid)
    if not (give(pi.hp, slot_h) and give(pi.hp, marker)):
        j.say("warning: the game didn't get its slot handles - a killed server would free its slot")
    k32.CloseHandle(marker)
    k32.ResumeThread(pi.ht)
    k32.CloseHandle(pi.ht)
    return pi.hp


def log_has_autotest(path):
    with open(path, 'rb') as f:
        return re.search(rb'^# autotest:', f.read(), re.M) is not None


def archive(j, sdir):   # the log into local\work\, gzipped: ~7 times smaller (scripts/results/logs.py reads both)
    log_file = os.path.join(sdir, 'ageofai_log.csv')
    dest = os.path.join(j.repo, 'local', 'work', 'autotest_%s_s%d%s.csv' % (time.strftime('%Y%m%d_%H%M%S'), j.slot, j.tag))   # the slot: two runs may end in the same second
    if not os.path.exists(log_file):
        return dest
    shutil.move(log_file, dest)
    try:
        with open(dest, 'rb') as src, gzip.open(dest + '.gz', 'wb') as z:
            shutil.copyfileobj(src, z, 1 << 20)
        os.remove(dest)
        return dest + '.gz'
    except OSError as e:   # should it fail, the plain log stays
        j.say('warning: the log not gzipped: %s' % e)
        return dest


last_prune = 0


def prune(j):   # old logs and dumps go (work_prune.ps1: older than 7 days, or over 10 GB), at most once an hour, on its own
    global last_prune
    with lock:
        if time.time() - last_prune < 3600:
            return
        last_prune = time.time()
    subprocess.Popen(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                      os.path.join(HERE, 'work_prune.ps1'), '-Hourly', '-Quiet'], creationflags=0x08000000,
                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


HANG_S = int(os.environ.get('EE_TEST_HANG_S', '240'))


def clock_place(log_file):   # the DLL's "# autotest: game clock at [ptr]+off" (eeaimod.c), None before it / without it
    try:
        with open(log_file, 'rb') as f:
            m = re.search(rb'^# autotest: game clock at \[([0-9a-f]+)\]\+([0-9a-f]+)', f.read(1 << 20), re.M)
    except OSError:
        return None
    return (int(m.group(1), 16), int(m.group(2), 16)) if m else None


def log_game_ms(log_file):   # without the clock's place (a DLL older than it): the game time of the log's last data row
    try:
        with open(log_file, 'rb') as f:
            f.seek(0, 2)
            f.seek(max(0, f.tell() - 65536))
            lines = f.read().split(b'\n')[1:]
    except OSError:
        return None
    for line in reversed(lines):
        head = line.split(b',', 1)[0]
        if head.isdigit():
            return int(head)
    return None


def game_clock(hp, place):   # the game time read out of the game's memory (no thread of the game needed), None: unreadable
    def rd(a):
        v, got = ctypes.c_uint32(), ctypes.c_size_t()
        ok = k32.ReadProcessMemory(hp, ctypes.c_void_p(a), ctypes.byref(v), 4, ctypes.byref(got))
        return v.value if ok and got.value == 4 else None
    phys = rd(place[0]) if place else None
    return rd(phys + place[1]) if phys else None


def watch(j, hp, log_file, t0, sdir):
    """waits for the game to end: '' when it did, else why it was stopped - its time up, or HUNG: once its clock ran (read
    out of its memory every 10 s where its log's "# autotest: game clock at" says), it stood still HANG_S seconds
    (EE_TEST_HANG_S, default 240; a DLL without that line: the game time of its log's last data row instead; a game whose
    clock never starts - a menu, the editor, a probe without AI rows on an old DLL - only by its time). Not through the pipe: in a hang the simulation's thread spun holding the heap, and the pipe's thread
    gave no answer that told it apart. A hung game's threads (where each is, hangdump.py) go into its log as "# hang:"
    lines and a minidump beside it (local/work/hang_s<slot>_<time>.dmp) before it is killed: 2026-11-12 one sat 100
    minutes spinning in the heap under the unit list's re-sort until its time was up, and nothing said where."""
    end = t0 + int(j.timeout)
    last_ms, last_t, ran, place = None, time.time(), False, None
    while k32.WaitForSingleObject(hp, 10000) != 0:
        now = time.time()
        if now >= end:
            return 'TIMEOUT after %s s - killed' % j.timeout
        place = place or clock_place(log_file)
        # an older DLL (a probe built on an old commit: 2026-11-12 two such hung 20 minutes unseen): its log's data rows
        ms = game_clock(hp, place) if place else log_game_ms(log_file)
        if ms is not None and ms != last_ms:
            ran = ran or last_ms is not None
            last_ms, last_t = ms, now
            continue
        if not ran or now - last_t < HANG_S:
            continue
        pid = k32.GetProcessId(hp)
        lines = []
        try:
            import hangdump
            lines = hangdump.describe(pid, 1024)
            dump = os.path.join(j.repo, 'local', 'work', 'hang_s%d_%s.dmp' % (j.slot, time.strftime('%Y%m%d_%H%M%S')))
            lines.append('minidump %s %s' % ('written' if hangdump.minidump(pid, dump) else 'FAILED', dump))
        except Exception as e:
            lines.append('hangdump failed: %s' % e)
        game = 'at %.1f min' % (last_ms / 60000.0) if last_ms is not None else 'before its first row'
        try:
            with open(log_file, 'a', encoding='mbcs', errors='replace') as f:
                f.write('# hang: the game time stood still %d s %s - killed by the test server\n' % (now - last_t, game))
                for l in lines:
                    f.write('# hang: %s\n' % l)
        except OSError:
            pass
        return 'HUNG: the game time stood still %d s %s - killed (its threads: "# hang:" lines in its log)' % (now - last_t, game)
    return ''


def play(j, slot_h):
    # ES_CONTINUOUS | ES_SYSTEM_REQUIRED for this thread, as long as it runs: unattended suites were stopped by the PC
    # going to sleep (the screen may still turn off)
    k32.SetThreadExecutionState(0x80000001)
    sdir = os.path.join(j.repo, 'local', 'work', 'slot%d' % j.slot)
    os.makedirs(sdir, exist_ok=True)
    g = j.game
    env = dict(j.env)
    for k in 'EEAIMOD_SETTINGS', 'EEAIMOD_DLL', 'EEAIMOD_HEADLESS':
        env.pop(k, None)
    env.update(EEAIMOD_SLOT=str(j.slot), EEAIMOD_SLOT_DIR=sdir, EEAIMOD_REPO=j.repo)
    env['EEAIMOD_UNREADY'] = '1'   # a known build that isn't ready yet plays here (game/api/exe.c) - never in normal play
    work = os.path.join(j.repo, 'local', 'work')
    proxy = uses_settings_file(g)
    # The game's settings. With them in a file (dll/engine/game_settings.c) every run reads a copy of -Settings (default
    # tests\config\eesettings.ini: fixed settings, sound off) - not the player's, not the registry - and nothing needs muting.
    if proxy and j.mute == '1':
        if not os.path.exists(j.settings):
            return 'settings file not found: ' + j.settings, 1
        env['EEAIMOD_SETTINGS'] = os.path.join(work, 'settings_slot%d.ini' % j.slot)
        shutil.copy(j.settings, env['EEAIMOD_SETTINGS'])
        # Art of Conquest checks for the 2017 update's install ("needs the original game") by its "Installed From"
        # values - the player's own, not in the shared file: the registry's (both keys; doubled backslashes: the file's escape)
        if j.aoc:
            add = []
            for path in BUILDS[False]['key'], BUILDS[True]['key']:
                try:
                    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, path) as k:
                        add += ['', '[HKCU\\%s]' % path]
                        for n in 'Installed From Volume', 'Installed From Directory':
                            try:
                                v = winreg.QueryValueEx(k, n)[0]
                            except OSError:
                                continue
                            if v:
                                add.append('%s="%s"' % (n, str(v).replace('\\', '\\\\').replace('"', '\\"')))
                except OSError:
                    pass
            with open(env['EEAIMOD_SETTINGS'], 'a', encoding='mbcs', newline='\r\n') as f:
                f.write('\n'.join(add) + '\n')
    # this run's own DLL: the installed one (which needs to know EEAIMOD_DLL) loads the copy and stays out of the way
    if j.dll:
        env['EEAIMOD_DLL'] = os.path.join(work, 'eeaimod_slot%d.dll' % j.slot)
        try:
            shutil.copy(j.dll, env['EEAIMOD_DLL'])
        except OSError as e:
            return "can't copy -Dll to %s: %s" % (env['EEAIMOD_DLL'], e), 1
    keys = [x.split('=', 1)[0].strip() for x in j.extra]
    with open(os.path.join(j.repo, 'tests', 'config', 'autotest.ini'), encoding='mbcs') as f:
        lines = [x.rstrip('\r\n') for x in f if x.split('=', 1)[0].strip() not in keys]
    ini = os.path.join(sdir, 'ageofai_autotest.ini')
    with open(ini, 'w', encoding='mbcs', newline='\r\n') as f:
        f.write('\n'.join(lines + j.extra) + '\n')
    # headless: no window of the game takes the focus or a taskbar button - from the process start on when ageofai.dll is
    # loaded with the engine (dll/base/headless_win.c; else from the engine's DLL scan on)
    headless = 'headless=1' in j.extra
    if headless:
        env['EEAIMOD_HEADLESS'] = '1'
    log_file = os.path.join(sdir, 'ageofai_log.csv')
    code, why = W.DWORD(0), ''
    hp = None
    try:
        # a log still in the slot's folder is a crashed run's (tests archive theirs below): keep it, never delete it
        if os.path.exists(log_file):
            kept = os.path.join(work, 'slot%d_left_%s.csv' % (j.slot, time.strftime('%Y%m%d_%H%M%S')))
            shutil.move(log_file, kept)
            j.say('kept the previous log: ' + kept)
        wait_reservation(j)
        t0 = time.time()
        with start_lock:
            # a game that is no test's - the player's own - is never disturbed: with the settings in the registry this run
            # would mute it
            mute = j.mute == '1' and not proxy
            if mute and foreign_game(j):
                return "game already running - aborting (its settings in the registry: the run would mute it - ageofai_config.exe --settings-file on)", 1
            if mute:
                with lock:
                    mute_sound(j)
            hp = start_game(j, env, headless, slot_h)
        # safety: the DLL logs "# autotest:" at load when it found the test ini; without it the game would run at normal
        # speed with intros and (if the ini vanished) sound - kill it instead. The line comes with the log's first lines,
        # so once the log is there it follows within moments; until then the start may take long - several games
        # loading at once took more than 10 s (2026-10-22: one of 8 starting together killed at 10 s, its log not begun)
        ok, log_at = False, None
        while k32.WaitForSingleObject(hp, 250) != 0:
            waited = time.time() - t0
            if os.path.exists(log_file):
                if log_has_autotest(log_file):
                    ok = True
                    break
                log_at = log_at or waited
                if waited - log_at > 5:
                    why = 'the DLL logs, but not in autotest mode (test ini missing?)'
                    break
            elif waited > 60:
                why = 'the DLL did not log within 60 s'
                break
        started = int(time.time() - t0)
        running = k32.WaitForSingleObject(hp, 0) != 0
        if not ok and running:
            k32.TerminateProcess(hp, 0xFFFFFFFF)
            k32.WaitForSingleObject(hp, 0xFFFFFFFF)
            why = 'ABORTED: ' + why
            j.say(why)
        else:
            why = watch(j, hp, log_file, t0, sdir)
            if why:
                k32.TerminateProcess(hp, 0xFFFFFFFF)
                k32.WaitForSingleObject(hp, 0xFFFFFFFF)
                j.say(why)
        k32.GetExitCodeProcess(hp, ctypes.byref(code))
        secs = int(time.time() - t0)
    finally:
        if hp:
            k32.CloseHandle(hp)
        try:
            os.remove(ini)   # the slot is this run's until it ends: its test ini can go (the game read it at its start)
        except OSError:
            pass
    # the game's DDrawCompat log (DDrawCompat-<exe>[n].log in the game folder, its name in the DLL's log: test/autotest.c)
    # into the slot's folder - the game folder keeps the player's
    if os.path.exists(log_file):
        with open(log_file, 'rb') as f:
            m = re.search(rb'^# slot: ddrawcompat log (.+?)\r?$', f.read(), re.M)
        if m:
            try:
                shutil.move(m.group(1).decode('mbcs'), os.path.join(sdir, 'ddrawcompat.log'))
            except OSError:
                pass
    # a crash: the DLL's exception line (dll/base: "# exception c0000005 at ...") or an exit code but 0 - said first on
    # the line, with where, so that a batch's output shows it (jobs.sh counts them)
    crash = ''
    if os.path.exists(log_file):
        with open(log_file, 'rb') as f:
            m = re.search(rb'^# exception (.{0,120})', f.read(), re.M)
        if m:
            crash = m.group(1).decode('mbcs', errors='replace').split(';')[0]
    rc = ctypes.c_int32(code.value).value
    dest = archive(j, sdir)
    prune(j)
    head = ('HUNG: ' if why.startswith('HUNG') else 'TIMEOUT: ' if why.startswith('TIMEOUT') else
            'CRASHED (%s): ' % crash if crash else 'CRASHED (exit code %d): ' % rc if rc and not why else 'ABORTED: ' if why else '')
    return '%sfinished in %d s real time (started in %d s), exit code %d%s, log: %s' % (
        head, secs, started, rc, ' (%s)' % why if why else '', dest), 0


def play_and_finish(j, slot_h):
    try:
        line, rc = play(j, slot_h)
    except Exception:
        log('game', j.name, 'failed:', traceback.format_exc())
        line, rc = 'the test server failed on this run: %s (local/work/testd.log)' % sys.exc_info()[1], 1
    k32.CloseHandle(slot_h)   # the game keeps its own handle until it has ended
    with lock:
        playing.remove(j)
        last = not playing
        if last:
            try:
                restore_sound(j)   # the last game running restores the player's sound
            except OSError as e:
                log('sound not restored:', e)
    log('done', j.name, 'slot', j.slot, line)
    j.tell(done=line, rc=rc)


def memory_low(j):
    m = MEMORYSTATUSEX()
    m.len = ctypes.sizeof(m)
    k32.GlobalMemoryStatusEx(ctypes.byref(m))
    young = sum(1 for p in playing if time.time() - p.since < 60)
    return m.avail / 2 ** 30 - young * GAME_GB < j.min_free


def next_up():   # the waiting jobs in the order they will play
    count = {}
    for j in playing:
        count[j.session] = count.get(j.session, 0) + 1
    return sorted(waiting, key=lambda j: (count.get(j.session, 0), j.seq))


def tick():   # at most one game started: the first in line whose own limits allow it (EE_TEST_GAMES, the memory floor)
    with lock:
        held = sum(test_named('eeaimod_slot%d' % s) for s in range(1, 31))
        j = next((j for j in next_up() if held < j.limit and not (playing and memory_low(j))), None)
        if not j:
            return
        for s in range(1, 31):
            h = new_named('eeaimod_slot%d' % s)
            if h:
                j.slot, j.since = s, time.time()
                waiting.remove(j)
                playing.append(j)
                break
        else:
            return
    log('start', j.name, 'session', j.session[:8], 'slot', j.slot)
    threading.Thread(target=play_and_finish, args=(j, h), daemon=True).start()


def find(name):
    return next((j for j in waiting + playing if j.name.lower() == name.lower()), None)


def status():
    with lock:
        out = ['%d playing, %d waiting' % (len(playing), len(waiting))]
        for j in sorted(playing, key=lambda j: j.slot):
            out.append('  playing  %-40s session %-8s slot %2d, %d min' % (j.name, j.session[:8], j.slot, (time.time() - j.since) / 60))
        for i, j in enumerate(next_up()):
            out.append('  %3d next %-40s session %-8s %d min in the queue' % (i + 1, j.name, j.session[:8], (time.time() - j.queued) / 60))
    return '\n'.join(out)


def serve(conn):
    global seq
    with conn:
        reader = conn.makefile('rb')
        line = reader.readline()
        if not line:
            return
        req = json.loads(line)
        op = req.pop('op')
        if op == 'status':
            conn.sendall(json.dumps({'done': status(), 'rc': 0}).encode() + b'\n')
        elif op == 'where':
            while True:
                with lock:
                    j = find(req['name'])
                    slot = j.slot if j else None
                if slot is not None or j is None:
                    conn.sendall(json.dumps({'slot': slot}).encode() + b'\n')
                    return
                time.sleep(0.25)
        elif op == 'run':
            j = Job(req, conn)
            err = prepare(j)
            if err is None:
                with lock:
                    seq += 1
                    j.seq, j.queued = seq, time.time()
                    j.name = j.name or 'run%d' % seq
                    if find(j.name):
                        err = 'a run named %s is queued or playing already' % j.name
                    else:
                        waiting.append(j)
            if err:
                j.tell(done=err, rc=1)
                return
            log('queued', j.name, 'session', j.session[:8])
            time.sleep(1.2)   # one that starts at once has nothing to say
            with lock:
                ahead = next_up().index(j) if j in waiting else None
            if ahead is not None:
                j.say('queued: %d ahead (at most %d games at a time)' % (ahead, j.limit))
            # until the client goes - after its game's last line, or before: then a queued game goes with it
            try:
                while reader.readline():
                    pass
            except OSError:
                pass
            with lock:
                if j in waiting:
                    waiting.remove(j)
                    log('cancelled', j.name)


def main():
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
    try:
        s.bind(('127.0.0.1', PORT))
    except OSError:
        return   # one runs already
    s.listen(64)
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    if os.path.exists(LOG) and os.path.getsize(LOG) > 1 << 20:
        os.replace(LOG, LOG + '.old')
    log('server started, pid', os.getpid())
    me = os.path.abspath(__file__)
    code = os.path.getmtime(me)

    s.settimeout(1)
    open_for_clients = threading.Event()
    open_for_clients.set()

    def accept():
        while open_for_clients.is_set():
            try:
                c, _ = s.accept()
            except OSError:   # the timeout: a look at the flag
                continue
            threading.Thread(target=serve, args=(c,), daemon=True).start()
        s.close()
    threading.Thread(target=accept, daemon=True).start()
    while True:
        try:
            tick()
        except Exception:
            log('tick failed:', traceback.format_exc())
        time.sleep(1)
        if open_for_clients.is_set() and os.path.getmtime(me) != code:
            log('this file changed: no new clients, the next one starts the new server; this one ends with its last game')
            open_for_clients.clear()
        if not open_for_clients.is_set() and not waiting and not playing:
            return


if __name__ == '__main__':
    main()
