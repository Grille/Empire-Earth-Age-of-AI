"""Generate a tuned dbcpbehavior.dat (AI settings table) from the original.

Record layout (14 x int32, 56 bytes), loaded to 0x8cfa80 + idx*56:
  0 nameStr  1 idx  2 min1 3 max1 4 val1  5 min2 6 max2 7 val2
  8 ?  9 minF 10 maxF 11 scaleF 12 fval(float) 13 flag
"""
import struct,sys
FIELDS={'val1':4,'val2':7,'fval':12}

# idx -> {field: value}
EDITS={
    # Unit cost the cheating AI pays, in percent (game multiplies by 0.01).
    # Original: 1 / 1 / 1.0  -> AI pays 1% of every cost.
    96: {'val1':100,     # Easy: full price (no discount)
         'val2':50},     # Hard: half price
    67: {'fval':75.0},   # Medium: 75% of price
    # "Epochen-Frequenz": min/max game minutes before the AI starts its next epoch advance
    # (x difficulty modifier, record 29). Original 50/70 -> AI sat in Prehistoric for ~an hour.
    # 28: {'val1':8, 'val2':12},   # tried; reverted - user prefers slow epochs, fix economy instead
}

def build(src,dst):
    d=bytearray(open(src,'rb').read())
    n=struct.unpack_from('<I',d,0)[0]
    found=set()
    for r in range(n):
        off=4+r*56
        idx=struct.unpack_from('<i',d,off+4)[0]
        if idx in EDITS:
            for f,v in EDITS[idx].items():
                k=FIELDS[f]
                old=struct.unpack_from('<f' if f=='fval' else '<i',d,off+k*4)[0]
                struct.pack_into('<f' if f=='fval' else '<i',d,off+k*4,v)
                print(f'record {idx} {f}: {old} -> {v}')
            found.add(idx)
    assert found==set(EDITS),set(EDITS)-found
    open(dst,'wb').write(d)

if __name__=='__main__':
    build(sys.argv[1],sys.argv[2])
