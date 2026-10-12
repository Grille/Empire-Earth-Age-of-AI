# Which memory of the running game grows: two snapshots of the committed private regions (VirtualQueryEx, from
# outside, read-only), grouped by allocation base. usage: python memgrow.py [seconds]
import ctypes, ctypes.wintypes as wt, subprocess, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
class MBI(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', wt.DWORD),
                ('PartitionId', wt.WORD), ('RegionSize', ctypes.c_size_t), ('State', wt.DWORD), ('Protect', wt.DWORD),
                ('Type', wt.DWORD)]
k32.VirtualQueryEx.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.POINTER(MBI), ctypes.c_size_t]
k32.VirtualQueryEx.restype = ctypes.c_size_t
k32.OpenProcess.restype = wt.HANDLE

out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq Empire Earth.exe', '/FO', 'CSV', '/NH'], capture_output=True, text=True).stdout
pid = int(out.split('","')[1])
h = k32.OpenProcess(0x0400 | 0x0010, False, pid)   # QUERY_INFORMATION | VM_READ

def snap():
    regs, addr, m = {}, 0, MBI()
    while addr < 0x100000000:
        if not k32.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(m), ctypes.sizeof(m)): break
        size = m.RegionSize
        if m.State == 0x1000:   # MEM_COMMIT
            base = m.AllocationBase or 0
            t = {0x20000: 'private', 0x40000: 'mapped', 0x1000000: 'image'}.get(m.Type, hex(m.Type))
            key = (base, t)
            regs[key] = regs.get(key, 0) + size
        addr = (m.BaseAddress or 0) + size
    return regs

def total(r, t): return sum(v for (b, tt), v in r.items() if tt == t) >> 20
a = snap()
secs = int(sys.argv[1]) if len(sys.argv) > 1 else 60
time.sleep(secs)
b = snap()
print('committed MB  private %d -> %d, mapped %d -> %d, image %d -> %d' % (total(a, 'private'), total(b, 'private'),
      total(a, 'mapped'), total(b, 'mapped'), total(a, 'image'), total(b, 'image')))
new = [(k, v) for k, v in b.items() if k not in a]
grown = [(k, b[k] - a[k]) for k in b if k in a and b[k] != a[k]]
gone = [(k, v) for k, v in a.items() if k not in b]
print('new regions: %d, %.1f MB (sizes: %s)' % (len(new), sum(v for k, v in new) / 2**20,
      ', '.join('%dK' % (v >> 10) for k, v in sorted(new, key=lambda x: -x[1])[:12])))
print('freed regions: %d, %.1f MB' % (len(gone), sum(v for k, v in gone) / 2**20))
print('grown regions:')
for (base, t), d in sorted(grown, key=lambda x: -abs(x[1]))[:12]:
    print('  %08x %-8s %+8d KB (now %d KB)' % (base, t, d >> 10, b[(base, t)] >> 10))
from collections import Counter
c = Counter(v >> 10 for k, v in new)
print('new region size histogram (KB: count):', dict(c.most_common(8)))
