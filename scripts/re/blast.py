# Port of zlib contrib/blast.c (PKWARE DCL explode)
import sys,struct
class S:
    def __init__(s,d): s.d=d;s.p=0;s.bb=0;s.bc=0
    def bits(s,n):
        v=s.bb
        while s.bc<n:
            v|=s.d[s.p]<<s.bc; s.p+=1; s.bc+=8
        s.bb=v>>n; s.bc-=n
        return v&((1<<n)-1)
def construct(rep):
    length=[]
    for b in rep:
        l=(b&15)+2; length+= [b>>4 if False else (b&15)]*0
        cnt=(b>>4)+1; length+=[b&15]*cnt
    count=[0]*14
    for l in length: count[l]+=1
    offs=[0]*14
    for i in range(1,13): offs[i+1]=offs[i]+count[i]
    sym=[0]*len(length)
    for i,l in enumerate(length):
        if l: sym[offs[l]]=i; offs[l]+=1
    return count,sym
def decode(s,h):
    count,sym=h; code=first=index=0
    for l in range(1,14):
        code|=s.bits(1)^1
        c=count[l]
        if code<first+c: return sym[index+(code-first)]
        index+=c; first+=c; first<<=1; code<<=1
    raise ValueError
LITLEN=[11,124,8,7,28,7,188,13,76,4,10,8,12,10,12,10,8,23,8,9,7,6,7,8,7,6,55,8,23,24,12,11,7,9,11,12,6,7,22,5,7,24,6,11,9,6,7,22,7,11,38,7,9,8,25,11,8,11,9,12,8,12,5,38,5,38,5,11,7,5,6,21,6,10,53,8,7,24,10,27,44,253,253,253,252,252,252,13,12,45,12,45,12,61,12,45,44,173]
LENLEN=[2,35,36,53,38,23]
DISTLEN=[2,20,53,230,247,151,248]
BASE=[3,2,4,5,6,7,8,9,10,12,16,24,40,72,136,264]
EXTRA=[0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8]
LIT=construct(LITLEN);LEN=construct(LENLEN);DIST=construct(DISTLEN)
def explode(d):
    s=S(d); lit=s.bits(8); dict_=s.bits(8); out=bytearray()
    while True:
        if s.bits(1):
            sym=decode(s,LEN); ln=BASE[sym]+s.bits(EXTRA[sym])
            if ln==519: break
            sym=2 if ln==2 else dict_
            dist=decode(s,DIST)<<sym; dist+=s.bits(sym); dist+=1
            for _ in range(ln): out.append(out[-dist])
        else:
            out.append(decode(s,LIT) if lit else s.bits(8))
    return bytes(out)
if __name__=='__main__':
    d=open(sys.argv[1],'rb').read()
    usz=struct.unpack('<I',d[4:8])[0]
    o=explode(d[12:]) if d[:4]==b'PK01' else d
    sys.stdout.buffer.write(o); sys.stderr.write(f'{usz} {len(o)}\n')
