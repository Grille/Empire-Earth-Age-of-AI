"""battle.py: the stats fight lines of a battle200.eeai log, one per snapshot (det: each type's share).
usage: python scripts/battle.py work/autotest_*_big*.csv [det]"""
import re,sys
k=0; L=open(sys.argv[1],encoding='latin1').read().split('\n')
for i,l in enumerate(L):
    m=re.match(r'# setup: stats fight 60 42 32 detail -> (.*)',l)
    if not m: continue
    t=0.2+0.1*k; k+=1
    mm=re.match(r'P2 \((\d+) units in (\d+) types, (\d+) hp, ([\d.]+) dmg/s, at contact ([\d.]+), (\d+) s closing\) vs P3 \((\d+) in (\d+), (\d+) hp, ([\d.]+)/s, ([\d.]+), (\d+) s\): P2 needs (\d+) s, P3 (\d+) s -> (P\d) \(([\d.]+)\)',m.group(1))
    if mm:
        g=mm.groups()
        print(f'{t:4.1f} min  P2 {g[0]:>3} units {g[2]:>6} hp {float(g[4]):7.1f}/s | P3 {g[6]:>3} units {g[8]:>6} hp {float(g[10]):7.1f}/s | P2 needs {g[12]:>4} s, P3 {g[13]:>4} s -> {g[14]} {g[15]}')
        if 'det' in sys.argv: print('      ',L[i+1][2:].strip(),'|',L[i+2][2:].strip())
