# Read game memory through the control pipe (peek). import mem; mem.rd(addr, n) -> bytes
import struct, sys, os
sys.path.insert(0, os.path.dirname(__file__))
import eectl

def rd(addr, n):
    out = b''
    while n > 0:
        k = min(n, 1024)
        r = eectl.send('peek %x %d' % (addr, k))
        for line in r.splitlines():
            if ':' in line and not line.startswith('error'):
                out += bytes.fromhex(line.split(':', 1)[1].strip())
        if len(out) == 0 or r.startswith('error'):
            raise IOError(r)
        addr += k; n -= k
    return out

def u32(a): return struct.unpack('<I', rd(a, 4))[0]
def i32(a): return struct.unpack('<i', rd(a, 4))[0]
def f32(a): return struct.unpack('<f', rd(a, 4))[0]
