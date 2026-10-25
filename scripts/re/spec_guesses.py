"""The spec's guesses ([guess]: a name and type from what the code does with a field, not confirmed) and where the mod
uses them: the generated accessors under a guess's comment (dll/game/api/gen/, made by the build) and their uses in
dll/ outside gen/ - a guess the mod reads or writes is the first to confirm (a read of the code, a peek in a running
game). usage: python scripts/re/spec_guesses.py   (after sh dll/build.sh)"""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GEN = os.path.join(ROOT, 'dll', 'game', 'api', 'gen')
FN = re.compile(r'^internal inline [^(]*?\b(\w+)\(')

guesses = []   # (its doc's first line, [accessor names])
for f in sorted(os.listdir(GEN)):
    state, cur = None, None
    for line in open(os.path.join(GEN, f), encoding='utf-8', errors='replace'):
        if line.startswith('// A GUESS'):
            cur, state = ['', []], 'doc'
            guesses.append(cur)
        elif state == 'doc' and line.startswith('//'):
            cur[0] = cur[0] or line[2:].strip()
        elif state and FN.match(line) and (state == 'doc' or of in line):   # the guess's own accessors only
            of = state == 'doc' and re.search(r'\bof_\w+', line).group(0) or of
            cur[1].append(FN.match(line).group(1))
            state = 'fn'
        elif state:
            state = None

uses = {}
for top, dirs, files in os.walk(os.path.join(ROOT, 'dll')):
    if os.path.abspath(top).startswith(GEN):
        continue
    for f in files:
        if f.endswith(('.c', '.h')):
            text = open(os.path.join(top, f), encoding='utf-8', errors='replace').read()
            for n in set(re.findall(r'\b\w+\b', text)):
                uses.setdefault(n, []).append(os.path.relpath(os.path.join(top, f), ROOT).replace(os.sep, '/'))
used = 0
for doc, fns in guesses:
    where = sorted(set(w for n in fns for w in uses.get(n, [])))
    used += bool(where)
    print('%-44s %s' % (fns[0] if fns else '?', ('USED: ' + ', '.join(where)) if where else '-'))
    print('    %s' % doc[:110])
print('%d guesses, %d used by the mod' % (len(guesses), used))
