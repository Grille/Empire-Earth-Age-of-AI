import struct,collections,sys
f=open(sys.argv[1],'rb');h=f.read(16);n=struct.unpack('<I',h[12:16])[0]
idx=f.read(n)
names=[];i=0
while i<len(idx)-4:
    l=struct.unpack('<I',idx[i:i+4])[0];i+=4
    if l==0 or l>300: break
    nm=idx[i:i+l].rstrip(b'\0').decode('latin1');i+=l
    s,e,sz=struct.unpack('<III',idx[i:i+12]);i+=12
    names.append((nm,s,e,sz))
print(len(names))
print(collections.Counter(nm.split(chr(92))[0] for nm,*_ in names))
for nm,s,e,sz in names:
    if nm.startswith(('unit','db')): print(nm,sz)
import os
want=sys.argv[2:]
for nm,s,e,sz in names:
    if nm in want or any(nm.endswith(w) for w in want):
        f.seek(s); d=f.read(e-s+1)
        print('=====',nm,s,e,sz,len(d), d[:4])
        out=os.path.join('ex',nm.replace(chr(92),'_')); os.makedirs('ex',exist_ok=True); open(out,'wb').write(d)
