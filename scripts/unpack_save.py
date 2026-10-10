"""Decompress all PK01 (PKWARE DCL) blocks in an EE .scn/.ees file; writes <out>_<n>.bin."""
import sys,struct,re,os
sys.path.insert(0,os.path.dirname(__file__))
from blast import explode
d=open(sys.argv[1],'rb').read(); out=sys.argv[2]
for n,m in enumerate(re.finditer(rb'PK01',d)):
    o=m.start(); usz=struct.unpack_from('<I',d,o+4)[0]
    try:
        b=explode(d[o+12:])
    except Exception as e:
        print(n,hex(o),'fail',e); continue
    open(f'{out}_{n}.bin','wb').write(b)
    print(n,hex(o),'declared',usz,'got',len(b))
