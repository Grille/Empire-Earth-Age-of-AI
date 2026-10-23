"""Extract (and DCL-decompress) files from data.ssa whose names start with given prefixes."""
import struct,sys,os
sys.path.insert(0,os.path.dirname(__file__))
from blast import explode
ssa,outdir=sys.argv[1],sys.argv[2]; prefixes=tuple(sys.argv[3:])
f=open(ssa,'rb');h=f.read(16);n=struct.unpack('<I',h[12:16])[0];idx=f.read(n);i=0;cnt=0
while i<len(idx)-4:
    l=struct.unpack('<I',idx[i:i+4])[0];i+=4
    if l==0 or l>300: break
    nm=idx[i:i+l].rstrip(b'\0').decode('latin1');i+=l
    s,e,sz=struct.unpack('<III',idx[i:i+12]);i+=12
    if not nm.startswith(prefixes): continue
    f.seek(s); d=f.read(e-s+1)
    if d[:4]==b'PK01': d=explode(d[12:])
    p=os.path.join(outdir,*nm.split(chr(92))); os.makedirs(os.path.dirname(p),exist_ok=True)
    open(p,'wb').write(d); cnt+=1
print(cnt,'files')
