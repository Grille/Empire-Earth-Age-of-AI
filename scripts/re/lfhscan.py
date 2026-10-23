# LFH user blocks in one heap segment of the running game (read-only, from outside): per user-blocks group the
# block count (busy bitmap size) and the stride, then the strides that hold most memory, with sample block contents
# and the pointers they contain resolved to modules. usage: lfhscan.py <segment hex>
import ctypes, ctypes.wintypes as wt, subprocess, struct, sys
from collections import Counter, defaultdict
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi')
k32.OpenProcess.restype = wt.HANDLE
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
class MBI(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', wt.DWORD),
                ('PartitionId', wt.WORD), ('RegionSize', ctypes.c_size_t), ('State', wt.DWORD), ('Protect', wt.DWORD),
                ('Type', wt.DWORD)]
k32.VirtualQueryEx.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.POINTER(MBI), ctypes.c_size_t]
k32.VirtualQueryEx.restype = ctypes.c_size_t
out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq Empire Earth.exe', '/FO', 'CSV', '/NH'], capture_output=True, text=True).stdout
pid = int(out.split('","')[1])
h = k32.OpenProcess(0x0400 | 0x0010, False, pid)
def rd(a, n):
    buf = ctypes.create_string_buffer(n); got = ctypes.c_size_t()
    k32.ReadProcessMemory(h, ctypes.c_void_p(a), buf, n, ctypes.byref(got))
    return buf.raw[:got.value]
mods = (ctypes.c_void_p * 512)(); need = wt.DWORD()
psapi.EnumProcessModulesEx(wt.HANDLE(h), mods, ctypes.sizeof(mods), ctypes.byref(need), 1)
modr = []
for m in mods[:need.value // ctypes.sizeof(ctypes.c_void_p)]:
    if not m: continue
    name = ctypes.create_unicode_buffer(260); psapi.GetModuleBaseNameW(wt.HANDLE(h), ctypes.c_void_p(m), name, 260)
    info = (ctypes.c_void_p * 3)(); psapi.GetModuleInformation(wt.HANDLE(h), ctypes.c_void_p(m), info, ctypes.sizeof(info))
    modr.append((m, m + (info[1] or 0), name.value))
def owner(v):
    for lo, hi, n in modr:
        if lo <= v < hi: return '%s+%x' % (n, v - lo)
    return None
seg = int(sys.argv[1], 16)
mem = bytearray()
addr, mbi = seg, MBI()
while k32.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)) and mbi.AllocationBase == seg:
    chunk = rd(mbi.BaseAddress, mbi.RegionSize) if mbi.State == 0x1000 else b''
    mem += chunk + bytes(mbi.RegionSize - len(chunk))
    addr = mbi.BaseAddress + mbi.RegionSize
print('segment %08x: %d KB' % (seg, len(mem) >> 10))
groups = []
for off in range(0, len(mem) - 0x40, 8):
    if mem[off + 0xC: off + 0x10] != b'\xc0\xd0\xe0\xf0': continue
    nbits, bmp = struct.unpack_from('<II', mem, off + 0x14)
    if not (0 < nbits < 4096) or bmp != seg + off + 0x1C: continue
    groups.append((off, nbits))
# the user blocks of a group end where the next group (or the segment's committed data) begins: stride from that
by_stride = defaultdict(lambda: [0, 0, []])
for i, (off, n) in enumerate(groups):
    end = groups[i + 1][0] - 8 if i + 1 < len(groups) else len(mem)
    first = (0x1C + (n + 31) // 32 * 4 + 7) & ~7
    stride = (end - off - first) // n
    stride &= ~7
    g = by_stride[stride]; g[0] += 1; g[1] += n; g[2].append((off, first, n))
print('%d LFH user-block groups' % len(groups))
print('stride: groups, blocks, MB')
for s, (ng, nb, l) in sorted(by_stride.items(), key=lambda x: -x[0] * x[1][1])[:10]:
    print('  %6d: %4d %7d %7.1f' % (s, ng, nb, s * nb / 2**20))
for s, (ng, nb, l) in sorted(by_stride.items(), key=lambda x: -x[0] * x[1][1])[:2]:
    off, first, n = l[len(l) // 2]
    ptrs = Counter()
    for gi in range(min(len(l), 20)):
        go, gf, gn = l[gi]
        for k in range(gn):
            b = go + gf + k * s
            for (v,) in struct.iter_unpack('<I', mem[b: b + min(s, 256)][: min(s, 256) // 4 * 4]):
                o = owner(v) if v > 0x10000 else None
                if o: ptrs[o] += 1
    print('-- stride %d: module pointers in the first 256 bytes of its blocks (20 groups):' % s)
    for o, c in ptrs.most_common(12): print('     %6d  %s' % (c, o))
    for k in range(2):
        b = off + first + k * s
        d = mem[b: b + 96]
        print('   block %08x: %s' % (seg + b, ' '.join('%08x' % v for (v,) in struct.iter_unpack('<I', d))))
        print('      %r' % bytes(d))
