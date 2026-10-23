"""Diff two eeaimod snapshots region by region (int32 granularity), skipping pointer-like pairs."""
import sys,struct,dumpread as D
def ptr(v): return 0x00400000<=v<0x7fff0000 and (v&3)==0 and v>0x01000000
A=D.load(sys.argv[1]); B=D.load(sys.argv[2])
for tag in A:
    if tag not in B: continue
    a=A[tag][1]; b=B[tag][1]; n=min(len(a),len(b),A[tag][2] or len(a),B[tag][2] or len(b))
    rows=[]
    for o in range(0,n-3,4):
        va,vb=struct.unpack_from('<I',a,o)[0],struct.unpack_from('<I',b,o)[0]
        if va==vb or (ptr(va) and ptr(vb)): continue
        fa,fb=struct.unpack_from('<f',a,o)[0],struct.unpack_from('<f',b,o)[0]
        rows.append(f'  +{o:04x}: working={va:<11} broken={vb:<11} (f {fa:.3g}/{fb:.3g})')
    print(f'== {tag}: {len(rows)} diffs'); print('\n'.join(rows[:60]))
