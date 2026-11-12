"""Where a 32-bit game's threads are: each thread stopped a moment, its eip, esp and the code addresses on its stack (the
DLL's crash line's form: [i]addr(module+offset)), and a minidump of the whole process - for a game that hangs without a
crash, where the DLL's own exception handler says nothing. testd.py calls it before it kills a hung game; by hand:
  python scripts/test/hangdump.py <pid> [dump file]   (prints the threads, writes the dump when a file is given)
Runs from 64-bit Python: the game's threads through Wow64GetThreadContext, its modules by EnumProcessModulesEx(32-bit)."""
import ctypes, ctypes.wintypes as W, sys

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenThread.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateFileW.restype = W.HANDLE
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.SuspendThread.argtypes = [W.HANDLE]
k32.ResumeThread.argtypes = [W.HANDLE]
k32.Wow64GetThreadContext.argtypes = [W.HANDLE, ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), W.DWORD]
psapi.GetModuleBaseNameW.argtypes = [W.HANDLE, W.HANDLE, W.LPWSTR, W.DWORD]
psapi.GetModuleInformation.argtypes = [W.HANDLE, W.HANDLE, ctypes.c_void_p, W.DWORD]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class MODULEINFO(ctypes.Structure):
    _fields_ = [('lpBaseOfDll', ctypes.c_void_p), ('SizeOfImage', W.DWORD), ('EntryPoint', ctypes.c_void_p)]


WOW64_CONTEXT_SIZE = 716        # WOW64_CONTEXT: ContextFlags at 0, Ebp 0xB4, Eip 0xB8, Esp 0xC4
WOW64_CONTEXT_FULL = 0x10007    # control, integer, segments


def modules(hp):
    arr = (ctypes.c_void_p * 1024)()
    need = W.DWORD()
    if not psapi.EnumProcessModulesEx(hp, arr, ctypes.sizeof(arr), ctypes.byref(need), 1):   # LIST_MODULES_32BIT
        return []
    out = []
    for h in arr[:need.value // ctypes.sizeof(ctypes.c_void_p)]:
        name = ctypes.create_unicode_buffer(260)
        psapi.GetModuleBaseNameW(hp, h, name, 260)
        mi = MODULEINFO()
        psapi.GetModuleInformation(hp, h, ctypes.byref(mi), ctypes.sizeof(mi))
        out.append((mi.lpBaseOfDll or 0, mi.SizeOfImage, name.value))
    return out


def where(mods, a):
    for base, size, name in mods:
        if base <= a < base + size:
            return '%s+%x' % (name, a - base)
    return None


def threads(pid):
    snap = k32.CreateToolhelp32Snapshot(4, 0)   # TH32CS_SNAPTHREAD
    te = THREADENTRY32()
    te.dwSize = ctypes.sizeof(te)
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == pid:
            out.append(te.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(te))
    k32.CloseHandle(snap)
    return out


def describe(pid, stack_words=160):
    """the lines: one a thread - its id, eip (module+offset), esp, and the stack's code addresses"""
    hp = k32.OpenProcess(0x0410 | 0x0010, False, pid)   # PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
    if not hp:
        return ['hangdump: cannot open process %d (%d)' % (pid, ctypes.get_last_error())]
    mods = modules(hp)
    lines = []
    for tid in threads(pid):
        th = k32.OpenThread(0x0002 | 0x0008 | 0x0040, False, tid)   # SUSPEND_RESUME | GET_CONTEXT | QUERY_INFORMATION
        if not th:
            continue
        ctx = ctypes.create_string_buffer(WOW64_CONTEXT_SIZE)
        ctypes.c_uint32.from_buffer(ctx, 0).value = WOW64_CONTEXT_FULL
        k32.SuspendThread(th)
        try:
            if not k32.Wow64GetThreadContext(th, ctx):
                lines.append('thread %d: no context (%d)' % (tid, ctypes.get_last_error()))
                continue
            eip = ctypes.c_uint32.from_buffer(ctx, 0xB8).value
            esp = ctypes.c_uint32.from_buffer(ctx, 0xC4).value
            buf = (ctypes.c_uint32 * stack_words)()
            got = ctypes.c_size_t()
            k32.ReadProcessMemory(hp, esp, buf, ctypes.sizeof(buf), ctypes.byref(got))
            scan = []
            for i in range(got.value // 4):
                w = where(mods, buf[i])
                if w:
                    scan.append('[%d]%08x(%s)' % (i, buf[i], w))
            lines.append('thread %d: eip %08x(%s) esp %08x; stack: %s' % (tid, eip, where(mods, eip) or '?', esp, ' '.join(scan[:24])))
        finally:
            k32.ResumeThread(th)
            k32.CloseHandle(th)
    k32.CloseHandle(hp)
    return lines


def minidump(pid, path):
    dbg = ctypes.WinDLL('dbghelp', use_last_error=True)
    dbg.MiniDumpWriteDump.argtypes = [W.HANDLE, W.DWORD, W.HANDLE, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
    hp = k32.OpenProcess(0x1F0FFF, False, pid)
    if not hp:
        return False
    f = k32.CreateFileW(path, 0x40000000, 0, None, 2, 0x80, None)   # GENERIC_WRITE, CREATE_ALWAYS
    ok = False
    if f and f != W.HANDLE(-1).value:
        ok = bool(dbg.MiniDumpWriteDump(hp, pid, f, 0x1 | 0x4, None, None, None))   # WithDataSegs | WithHandleData
        k32.CloseHandle(f)
    k32.CloseHandle(hp)
    return ok


if __name__ == '__main__':
    pid = int(sys.argv[1])
    for l in describe(pid):
        print(l)
    if len(sys.argv) > 2:
        print('minidump:', 'written' if minidump(pid, sys.argv[2]) else 'failed', sys.argv[2])
