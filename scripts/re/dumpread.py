"""Parse eeaimod memory snapshots: records of [16-byte tag][addr,len,got][len bytes]."""
import struct,sys
def load(path):
    d=open(path,'rb').read(); i=0; out={}
    while i<len(d):
        tag=d[i:i+16].rstrip(b'\0').decode(); addr,ln,got=struct.unpack_from('<III',d,i+16); i+=28
        out[tag]=(addr,d[i:i+ln],got); i+=ln
    return out
def u8(r,o): return r[1][o]
def u32(r,o): return struct.unpack_from('<I',r[1],o)[0]
