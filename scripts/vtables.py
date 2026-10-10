# Primary vtable of RTTI classes (MSVC): address, slot count, and selected slots.
# usage: [EE_EXE=<exe>] python vtables.py Class[:slot,slot..] ...
import re, struct, sys, os
sys.path.insert(0, os.path.dirname(__file__))
import ee

TEXT_LO, TEXT_HI = ee.text_va, ee.text_va + ee.text_sz

def vtable(cls):
    m = re.search(rb'\.\?AV' + re.escape(cls.encode()) + rb'@@\0', ee.img)
    if not m:
        return None
    td = ee.IB + m.start() - 8                       # type descriptor
    for col_field in ee.xrefs(td):
        col = col_field - 12                         # complete object locator: sig, offset, cdOffset, td
        if ee.u32(col) != 0 or ee.u32(col + 4) != 0:
            continue
        for ref in ee.xrefs(col):                    # vtable[-1] = &COL
            vt = ref + 4
            n = 0
            while TEXT_LO <= ee.u32(vt + 4 * n) < TEXT_HI:
                n += 1
            return vt, n
    return None

for arg in sys.argv[1:]:
    cls, _, slots = arg.partition(':')
    r = vtable(cls)
    if not r:
        print('%-46s not found' % cls); continue
    vt, n = r
    extra = ' '.join('[%s]=%06x' % (s, ee.u32(vt + 4 * int(s))) for s in slots.split(',') if s)
    print('%-46s vtable %06x  %3d slots  %s' % (cls, vt, n, extra))
