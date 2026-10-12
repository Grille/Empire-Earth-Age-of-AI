# Which heap owns the growing 16 MB regions, and which module made that heap (read-only, from outside).
import ctypes, ctypes.wintypes as wt, subprocess, struct, sys
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.OpenProcess.restype = wt.HANDLE
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq Empire Earth.exe', '/FO', 'CSV', '/NH'], capture_output=True, text=True).stdout
pid = int(out.split('","')[1])
h = k32.OpenProcess(0x0400 | 0x0010, False, pid)
def rd(a, n):
    buf = ctypes.create_string_buffer(n); got = ctypes.c_size_t()
    if not k32.ReadProcessMemory(h, ctypes.c_void_p(a), buf, n, ctypes.byref(got)): return None
    return buf.raw[:got.value]
def u32(a):
    b = rd(a, 4); return struct.unpack('<I', b)[0] if b else None

# 32-bit PEB of a WOW64 process: NtQueryInformationProcess(ProcessWow64Information = 26)
ntdll = ctypes.WinDLL('ntdll')
peb32 = ctypes.c_void_p()
ntdll.NtQueryInformationProcess(wt.HANDLE(h), 26, ctypes.byref(peb32), ctypes.sizeof(peb32), None)
peb = peb32.value
proc_heap = u32(peb + 0x18)
nheaps = u32(peb + 0x88); heaps_arr = u32(peb + 0x90)
heaps = [u32(heaps_arr + 4 * i) for i in range(nheaps)]
print('process heap %08x; %d heaps: %s' % (proc_heap, nheaps, ' '.join('%08x' % x for x in heaps)))
for base in [int(x, 16) for x in sys.argv[1:]]:
    sig = u32(base + 8)
    heap = u32(base + 0x18)
    print('region %08x: segment signature %08x, heap %08x%s' % (base, sig or 0, heap or 0,
          ' (process heap)' if heap == proc_heap else ' (heap #%d)' % heaps.index(heap) if heap in heaps else ''))
# per heap: total committed in its segments (HEAP +0xA4 SegmentList? use VirtualQuery of each region instead)
