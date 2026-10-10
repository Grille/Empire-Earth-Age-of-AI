"""Emit names.tsv: funcVA \t Class \t vfN  from MSVC RTTI vtables (base-most class wins)."""
import re,sys,os
sys.path.insert(0,os.path.dirname(__file__))
import ee
out=sys.argv[1]
cands={}  # fva -> list of (nbases, cls, slot)
classes=0
for m in re.finditer(rb'\.\?AV([\w@]+?)@@\0',ee.img):
    td=ee.IB+m.start()-8; name=m.group(1).decode().replace('@','_')
    for ref in ee.xrefs(td):
        col=ref-12
        if ee.u32(col)!=0: continue   # only primary vtable (offset 0)
        if ee.u32(col+4)!=0: continue
        chd=ee.u32(col+16)
        try: nb=ee.u32(chd+8)
        except Exception: continue
        for vref in ee.xrefs(col):
            vt=vref+4; classes+=1
            for i in range(400):
                f=ee.u32(vt+4*i)
                if not (ee.text_va<=f<ee.text_va+ee.text_sz): break
                if i>0 and ee.xrefs(vt+4*i,(0x823000,0x7b000)) : pass
                # follow jmp thunk
                b=ee.rd(f,5)
                if b[0]==0xe9:
                    import struct; f=f+5+struct.unpack('<i',b[1:5])[0]
                cands.setdefault(f,[]).append((nb,name,i))
with open(out,'w') as fo:
    for f,l in sorted(cands.items()):
        nb,name,i=min(l)
        fo.write(f'{f:08x}\t{name}\tvf{i}\n')
print(classes,'vtables',len(cands),'functions')
