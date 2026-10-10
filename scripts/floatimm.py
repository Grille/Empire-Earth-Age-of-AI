# List the float immediates (and pushed ints) in a function: instruction address, operand address, value.
# usage: [EE_EXE=..] python floatimm.py <func hex> <length hex> [min] [max]
import sys, os, struct
sys.path.insert(0, os.path.dirname(__file__))
import ee

start, length = int(sys.argv[1], 16), int(sys.argv[2], 16)
lo = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
hi = float(sys.argv[4]) if len(sys.argv) > 4 else 5000.0
for i in ee.md.disasm(ee.rd(start, length), start):
    raw = bytes(i.bytes)
    # the immediate is the last 4 bytes for mov [mem], imm32 / push imm32
    if i.mnemonic in ('mov', 'push') and len(raw) >= 5:
        v = struct.unpack('<I', raw[-4:])[0]
        f = struct.unpack('<f', raw[-4:])[0]
        if lo <= abs(f) <= hi or (i.mnemonic == "push" and lo <= v <= hi):
            what = ("%.2f" % f) if lo <= abs(f) <= hi else ("int %d" % v)
            print('%08x: %-40s imm@%08x = %s' % (i.address, i.mnemonic + ' ' + i.op_str, i.address + len(raw) - 4, what))
