"""Fills the generated tables of config/aimod_ai/README.md from the DLL's sources - the situations (persona.c k_lines,
line_fallback; their descriptions: mod/prompt.eeai sit_<key>), the game's values (diplo.c k_xmembers, k_xglobals), the epochs (expr.c
k_epoch_const), the style keys (settings.c k_style_keys) - between its <!-- name: ... --> and <!-- /name --> markers.
Run it after changing one of them: local/venv/Scripts/python.exe scripts/build/gen_eeai_readme.py"""
import glob
import os
import re
from eeai_prompt import prompt_texts

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
def src(name):
    return open(glob.glob(os.path.join(root, 'dll', '**', name), recursive=True)[0], encoding='latin-1').read()   # dll/<folder>/<name>
def c_string(code):   # the text of the C string literals in code, escapes undone
    raw = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', code))
    return re.sub(r'\\(.)', lambda m: '\n' if m.group(1) == 'n' else m.group(1), raw)
def cell(t):
    return t.replace('|', '\\|').replace('<', '&lt;').replace('>', '&gt;')

pers, diplo, expr, settings = (src(n) for n in ('persona.c', 'diplo.c', 'expr.c', 'settings.c'))
diplo += src('relations.c')   # the players' values and relations in the texts
expr += src('constants.c')   # the game's constants (epoch., resource. ...) in the texts
prompt = prompt_texts(root)

# situations
lines = re.findall(r'\{ "([a-z_]+)", (\d+), (\d+), (A_ALL|A_ALLIES|A_NAME) \}', pers)
fb = re.search(r'internal int line_fallback\(int k\) \{(.*?)\n\}', pers, re.S).group(1)
fallback = {a.lower(): b.lower() for a, b in re.findall(r'case L_(\w+): return L_(\w+);', fb)}
aud = {'A_ALL': 'everyone', 'A_ALLIES': 'its allies', 'A_NAME': '{them}'}
def sit(key):
    # an expression in it ({lord == them ? ...}) left out: the plain words describe the situation
    return re.sub(r'\{[^{}]*[ ?][^{}]*\}', '', prompt.get('sit_' + key, ''))
t = ['| Situation | When - "you" is the AI | Heard by | Chance | Cooldown | Without a line |', '|---|---|---|---|---|---|']
for key, chance, cooldown, a in lines:
    t.append('| `%s` | %s | %s | %s | %s | %s |' % (key, cell(sit(key)), aud[a], 'always' if chance == '100' else chance + '%',
                                                   cooldown + ' s' if cooldown != '0' else '', '`%s`' % fallback[key] if key in fallback else ''))
situations = '\n'.join(t)

# values
def members(table):
    body = re.search(r'internal const struct xmember %s\[\] = \{(.*?)\n\};' % table, diplo, re.S).group(1)
    return re.findall(r'\{ "(\w+)", (XM_\w+), ((?:"(?:[^"\\]|\\.)*"\s*)+)\}', body)
kinds = {'XM_PLAYER': [], 'XM_LINK': [], 'XM_REL': [], 'XM_FILTER': [], 'XM_DO': []}   # XM_DO: a script's effects (docs/scripts.md), not a text's
for name, kind, doc in members('k_xmembers'):
    kinds[kind].append((name, c_string(doc)))
def table(rows, prefix, head, suffix=''):
    out = ['| %s | |' % head, '|---|---|']
    pending = []
    def row():
        out.append('| %s | %s |' % (', '.join('`%s%s%s`' % (prefix, n, suffix) for n in pending[0]), cell(pending[1])))
    for name, doc in rows:   # rows without a text share the previous one's
        if doc:
            if pending: row()
            pending = [[name], doc]
        else: pending[0].append(name)
    if pending: row()
    return '\n'.join(out)
values = '\n\n'.join([
    '**Of each player** - `me.`, `them.`, `enemy.`, `lord.`:\n\n' + table(kinds['XM_PLAYER'] + kinds['XM_LINK'] + [(n + '(filter)', d) for n, d in kinds['XM_FILTER']], 'X.', 'Member'),
    '**Relations** of player A with player B - `me.opinion(them)`, `them.opinion(me)`, `enemy.at_war(them)`; '
    'without `A.` mine (`opinion(enemy)`). Those that count events - `tribute`, `refusals`, `betrayals`, `kills` - '
    'take a time window as a second argument: `them.tribute(me, 15)` counts the last 15 minutes only '
    '(`16.5` too), without it (or with 0) the whole game. Older events are the whole game minus the window - '
    '`them.betrayals(me) - them.betrayals(me, 20)` counts those more than 20 minutes ago, `me.kills(them, 60, buildings) - '
    'me.kills(them, 20, buildings)` those 20 to 60 minutes ago; a negative window is a mistake:\n\n' + table(kinds['XM_REL'], 'A.', 'Relation', '(B)'),
    '**The game:**\n\n' + table([(n, c_string(d)) for n, k, d in members('k_xglobals')], '', 'Name'),
])

# epochs, style keys
epochs = re.findall(r'"(\w+)"', re.search(r'k_epoch_const\[15\] = \{(.*?)\};', expr, re.S).group(1))
names = ['Prehistoric', 'Stone', 'Copper', 'Bronze', 'Dark', 'Middle', 'Renaissance', 'Imperial', 'Industrial',
         'Atomic - WWI', 'Atomic - WWII', 'Atomic - Modern', 'Digital', 'Nano', 'Space']
ep = ['| Name | Number | Age |', '|---|---|---|'] + ['| `epoch.%s` | %d | %s |' % (e, i + 1, names[i]) for i, e in enumerate(epochs)]
style = ', '.join('`%s`' % k for k in re.findall(r'\{ "(\w+)", offsetof', settings))

path = os.path.join(root, 'config', 'aimod_ai', 'README.md')
text = open(path, encoding='utf-8').read()
for name, body in (('situations', situations), ('values', values), ('epochs', '\n'.join(ep)), ('style', style)):
    text, n = re.subn(r'(<!-- %s: generated by scripts/build/gen_eeai_readme.py -->\n).*?(<!-- /%s -->)' % (name, name),
                      lambda m: m.group(1) + body + '\n' + m.group(2), text, flags=re.S)
    assert n == 1, name
open(path, 'w', encoding='utf-8', newline='\n').write(text)
print(path, len(lines), 'situations,', sum(len(v) for v in kinds.values()), 'members')
