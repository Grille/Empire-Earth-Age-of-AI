"""duels.py: the duel logs of scripts/duels.sh - the estimate at the start, both sides over time, when it ended.
usage: python scripts/duels.py work/autotest_*_duel_*.csv"""
import re,sys,glob
for f in sys.argv[1:]:
    L=open(f,encoding='latin1').read().split('\n')
    rows=[]
    k=0
    for l in L:
        m=re.match(r'# setup: stats fight 60 31 \d+ \d+ \d+ -> ?(.*)',l)
        if not m: continue
        t=(0.14+0.05*k)*60-6; k+=1
        x=m.group(1)
        mm=re.match(r'P2 \((\d+) units(?: in \d+ types)?, (\d+) hp, ([\d.]+) dmg/s.*?\) vs P3 \((\d+)(?: in \d+)?, (\d+) hp, ([\d.]+)/s.*?\): P2 needs (\d+) s, P3 (\d+) s',x)
        rows.append((t,mm.groups() if mm else None, x[:60]))
    print('==',f.split('duel_')[-1])
    first=None; last=None
    for t,g,x in rows:
        if g and first is None: first=(t,g)
        if g: last=(t,g)
    if first:
        t,g=first; print(f'  start t={t:.0f}s: P2 {g[0]} units {g[1]} hp {g[2]}/s, P3 {g[3]} units {g[4]} hp {g[5]}/s -> predicted P2 needs {g[6]} s, P3 needs {g[7]} s')
        t,g=last; print(f'  last  t={t:.0f}s: P2 {g[0]} units {g[1]} hp, P3 {g[3]} units {g[4]} hp')
        end=[t for t,g,x in rows if g is None and t>first[0]]
        print(f'  fight over at about {end[0] if end else None} s; {(end[0]-first[0]) if end else None} s after start')
    for t,g,x in rows[:40:3]:
        if g: print(f'    t={t:3.0f} P2 {g[0]:>2}/{g[1]:>5}  P3 {g[3]:>2}/{g[4]:>5}')
