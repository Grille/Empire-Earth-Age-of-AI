"""Transports stuck in their .tai script, per run (`transport_log=1`: dll/ai/transports.c's "# tw:" lines): each stall (a
transport with a goal or passengers kept one state on one tile for 20 s) by kind, as it stood at its last report -
  lost      WaitForPassengers, no pending passenger coming (idle, without a goal, another order, gone): hang 1
  partly    WaitForPassengers, some coming, some not
  coming    WaitForPassengers, all pending passengers on their way
  unload    Idle with its unload goal and passengers aboard: hang 2
  other     any other state - its name
with how many, their seconds together and the longest (a stall still open at the log's end: until then), who owned the
transport (landing / ferry / game); the watchdog's steps (transport_watchdog=1); the landings sailed and ashore.
usage: python scripts/transports.py [-v] logs...      (-v: each stall of the kinds lost / partly / unload)"""
import re, sys, collections

STALL = re.compile(r'# tw: P(\d+) at ([\d.]+) min: (\w+) transport (\w+) (stalls|still) in (\w+) (\d+) s at (\d+),(\d+) goal (\S+) aboard (\d+)'
                   r'(?: pending (\d+) \(board (\d+), idle (\d+), no goal (\d+), other (\d+), gone (\d+)\))?')
END = re.compile(r'# tw: P(\d+) at ([\d.]+) min: (?:\w+ )?transport (\w+) (?:moves on after|gone after) (\d+) s')
DOG = re.compile(r'# tw: P\d+ at [\d.]+ min: \w+ transport \w+ watchdog: (?:after \d+ s in WaitForPassengers (\d+) of|Idle)')
MIN = re.compile(r'at ([\d.]+) min')


def kind(m):
    st, goal, aboard = m.group(6), m.group(10), int(m.group(11))
    if st == 'WaitForPassengers' and m.group(12):
        board, lost = int(m.group(13)), sum(int(m.group(i)) for i in range(14, 18))
        return 'lost' if not board and lost else 'partly' if lost else 'coming'
    if st == 'Idle' and goal.startswith('UnloadTransport') and aboard > 0: return 'unload'
    return st


def run(path, verbose):
    open_, done = {}, []
    end_min = 0.0
    dog_drop = dog_kick = 0
    sailed = ashore = 0
    for ln in open(path, encoding='utf-8', errors='replace'):
        if not ln.startswith('#'): continue
        m = MIN.search(ln)
        if m: end_min = max(end_min, float(m.group(1)))
        if 'min: sails with' in ln: sailed += 1
        if re.search(r'min: all \d+ ashore', ln): ashore += 1
        m = DOG.match(ln)
        if m:
            if m.group(1): dog_drop += int(m.group(1))
            else: dog_kick += 1
            continue
        m = STALL.match(ln)
        if m:
            t0 = float(m.group(2)) - int(m.group(7)) / 60.0
            s = open_.setdefault(m.group(4), {'t0': t0, 'who': m.group(3), 'pl': m.group(1)})
            s['kind'], s['line'] = kind(m), ln.strip()
            continue
        m = END.match(ln)
        if m and m.group(3) in open_:
            s = open_.pop(m.group(3)); s['sec'] = int(m.group(4)); done.append(s)
    for s in open_.values():   # still stalled at the end
        s['sec'] = int((end_min - s['t0']) * 60); s['open'] = 1; done.append(s)
    return done, dog_drop, dog_kick, sailed, ashore


def main():
    verbose = '-v' in sys.argv
    files = [a for a in sys.argv[1:] if a != '-v']
    tot = collections.defaultdict(lambda: [0, 0, 0])
    for f in files:
        done, dd, dk, sailed, ashore = run(f, verbose)
        by = collections.defaultdict(lambda: [0, 0, 0])
        for s in done:
            b = by[s['kind']]; b[0] += 1; b[1] += s['sec']; b[2] = max(b[2], s['sec'])
            t = tot[s['kind']]; t[0] += 1; t[1] += s['sec']; t[2] = max(t[2], s['sec'])
        main_kinds = ('lost', 'partly', 'coming', 'unload')
        cols = ' '.join('%s %d/%ds/max %ds' % (k, by[k][0], by[k][1], by[k][2]) for k in main_kinds)
        other = sum(v[0] for k, v in by.items() if k not in main_kinds)
        print('%-28s %s other %d | open at end %d | watchdog let go %d, kicked %d | landings sailed %d ashore %d'
              % (f.replace('\\', '/').split('/')[-1][-28:], cols, other, sum(1 for s in done if s.get('open')), dd, dk, sailed, ashore))
        if verbose:
            for s in done:
                if s['kind'] in ('lost', 'partly', 'unload'):
                    print('    %s %s %ds%s: %s' % (s['who'], s['kind'], s['sec'], ' (open)' if s.get('open') else '', s['line'][:300]))
    if len(files) > 1:
        print('all %d runs: %s' % (len(files), ' '.join('%s %d/%ds/max %ds' % (k, v[0], v[1], v[2]) for k, v in sorted(tot.items()))))


if __name__ == '__main__':
    main()
