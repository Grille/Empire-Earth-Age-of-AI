"""ageofai.ini's keys (settings/*.cs, settings_read.py) -> dll/settings/rows.h's tables and the shipped
config/ageofai.ini's sections. The trial (2026-10-31): settings/ holds only some sections, so nothing in the tree is
written yet - the output goes to local/work/settings_gen/, and --compare holds it against HEAD's rows.h (row by row,
spaces aside) and HEAD's config/ageofai.ini (the sections' text, byte by byte).
usage: gen_settings.py [--compare]"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import settings_read   # noqa: E402

ROOT = settings_read.ROOT
OUT = os.path.join(ROOT, 'local', 'work', 'settings_gen')


def c_str(s):
    return '"%s"' % s.replace('\\', '\\\\').replace('"', '\\"')


def default_of(sec, k):   # the default as the code's row has it and as the file writes it: (row text, file text)
    kind = settings_read.kind_of(sec, k)
    d = k.default
    if kind in ('FLAG', 'CHILD'):
        v = '1' if d == 'true' else '0'
        return v, v
    if kind == 'CHOICE':
        name = sec.enums[k.type][d.split('.')[-1].lstrip('@')]   # scroll_choice.game -> game, lock_mouse_choice.on -> 1
        return c_str(name), name
    if kind == 'FLAGS':
        return '""', ''
    if kind in ('STR', 'STR_P'):
        s = eval(d) if d else ''   # a C# string literal: the same escapes
        return c_str(s), s
    v = d or '0'
    return v, v + ('%' if kind in ('PCT', 'SHARE') else '')


def row_of(sec, k):   # X(FLAG, general, enabled, 1, 0, 0) - X / S / D by the section
    kind = settings_read.kind_of(sec, k)
    a = k.attrs
    d, _ = default_of(sec, k)
    lo_hi = {
        'FLAG': ('0', '0'),
        'INT': tuple(settings_read.arg_text(x) for x in a.get('range', [])) or ('0', '0'),
        'NUM': tuple(settings_read.arg_text(x) for x in a.get('range', [])) or ('0', '0'),
        'PCT': (settings_read.arg_text(a.get('pct', [('num', 0)])[0]), '0'),
        'SHARE': tuple(settings_read.arg_text(x) for x in a.get('share', [])),
        'CHOICE': (c_str('|'.join(sec.enums.get(k.type, {}).values())), '0'),
        'STR': (settings_read.arg_text(a['length'][0]) if 'length' in a else '0', '0'),
        'STR_P': (settings_read.arg_text(a['length'][0]) if 'length' in a else '0', '0'),
        'CHILD': (settings_read.arg_text(a['child'][0]) if 'child' in a else '', '0'),
        'FLAGS': ('%s_FIELDS' % k.name.upper(), '0'),
    }[kind]
    if 'all' in sec.attrs:
        return 'D(%s, %s, %s, %s, %s)' % (kind, k.name, d, lo_hi[0], lo_hi[1])
    macro = 'X' if sec.name in ('general', 'autotest') else 'S'
    return '%s(%s, %s, %s, %s, %s, %s)' % (macro, kind, sec.name, k.name, d, lo_hi[0], lo_hi[1])


def field_name(sec, k):   # g_set's: general's (and [ai_base]'s, [autotest]'s) by the key, the others section_key
    return k.name if sec.name in ('general', 'ai_base', 'autotest') else '%s_%s' % (sec.name, k.name)


def tables(secs):   # rows.h's tables as lists of lines
    rows, fields, groups, game, under = [], [], [], [], []
    for sec in secs:
        for sname, (flds, doc) in sec.structs.items():
            macro = sname.replace('_fields', '').upper() + '_FIELDS'
            parts = []
            for t, f in flds:
                if t == 'bool':
                    parts.append('field_bool(%s)' % f)
                else:
                    parts.append('field_enum(%s, %s)' % (f, c_str('/'.join(sec.enums[t].values()))))
            doc = ["FLAGS rows' fields: " + doc[0]] + doc[1:] if doc and not fields else doc   # the block's heading
            fields += ['// ' + d for d in doc] + ['#define %s(field_bool, field_enum) %s' % (macro, ' '.join(parts))]
        for k in sec.keys:
            rows.append(row_of(sec, k))
            if 'group' in k.attrs:
                groups.append('G(%s, %s)' % (field_name(sec, k), c_str(k.attrs['group'][0])))
            if 'game' in k.attrs:
                g = k.attrs['game']
                game.append('V(%s, %s, %s, %s)' % (field_name(sec, k), c_str(g[0]), c_str(g[1]), settings_read.arg_text(g[2])))
            if 'under' in k.attrs:
                under.append('U(%s, %s)' % (k.name, settings_read.arg_text(k.attrs['under'][0])))
    return rows, fields, groups, game, under


def ini_of(sec):   # the section's text in the shipped file
    out = ['[%s]' % sec.name]
    for k in sec.keys:
        g = k.attrs.get('group')
        if g and len(g) > 1 and settings_read.arg_text(g[1]) == 'true':
            out += ['', '; --- %s ---' % g[0]]
        out += [('; ' + d) if d else ';' for d in k.doc]
        _, v = default_of(sec, k)
        if 'commented' in k.attrs:
            c = k.attrs['commented']
            out.append(';%s=%s' % (k.name, c[0] if c and c[0] else v))
        else:
            out.append('%s=%s' % (k.name, v))
    return '\n'.join(out) + '\n'


def head(path):
    return subprocess.run(['git', 'show', 'HEAD:' + path], cwd=ROOT, capture_output=True, text=True, check=True).stdout


def norm(row):
    return re.sub(r'\s+', '', row.rstrip('\\').strip())


def compare(secs, rows, fields, groups, game, under):
    rows_h = head('dll/settings/rows.h')
    ini = head('config/ageofai.ini')
    bad = 0
    names = {s.name for s in secs}
    have = {}
    for line in rows_h.splitlines():
        m = re.match(r'^\s*([XSD])\((\w+),\s*(\w+),\s*(\w+)', line)
        if m:
            sec = 'ai_base' if m.group(1) == 'D' else m.group(3)
            key = m.group(3) if m.group(1) == 'D' else m.group(4)
            have[(sec, key)] = norm(line)
    mine = {}
    for r in rows:
        m = re.match(r'^([XSD])\((\w+), (\w+), (\w+)', r)
        mine[(m.group(3) if m.group(1) != 'D' else 'ai_base', m.group(4) if m.group(1) != 'D' else m.group(3))] = norm(r)
    for key in [k for k in have if k[0] in names]:
        if key not in mine:
            print('rows.h: %s.%s not in settings/' % key); bad += 1
        elif mine[key] != have[key]:
            print('rows.h: %s.%s differs:\n  HEAD %s\n  spec %s' % (key + (have[key], mine[key]))); bad += 1
    for key in mine:
        if key not in have:
            print('rows.h: %s.%s only in settings/' % key); bad += 1
    flat = {norm(x) for x in rows_h.splitlines()}
    for what, lines in (('fields', [f for f in fields if f.startswith('#define')]), ('groups', groups), ('game', game), ('under', under)):
        for x in lines:
            if norm(x) not in flat and norm(x) + '\\' not in flat:
                print('rows.h %s: not in HEAD: %s' % (what, x)); bad += 1
    for f in fields:
        if f.startswith('// ') and norm(f) not in flat:
            print('rows.h fields comment differs: %s' % f); bad += 1
    print('rows.h: %d rows of %d sections compared' % (len(mine), len(secs)))
    for sec in secs:
        m = re.search(r'^\[%s\]\n.*?(?=^\[|\Z)' % re.escape(sec.name), ini, re.S | re.M)
        want = m.group(0).rstrip('\n') + '\n' if m else ''
        got = ini_of(sec)
        banner = re.search(r'\n\n\n; -{20,}\n;  AI behaviour.*?; -{20,}\n$', want, re.S)
        if banner and sec.name != 'ai_base':   # known: HEAD has [ai_base]'s banner before [sound]
            print('ini [%s]: (known) the AI banner after it - its place is before [ai_base]' % sec.name)
            want = want[:banner.start()] + '\n'
        if want == got:
            print('ini [%s]: same (%d lines)' % (sec.name, got.count('\n')))
            continue
        bad += 1
        w, g = want.splitlines(), got.splitlines()
        i = next((i for i in range(min(len(w), len(g))) if w[i] != g[i]), min(len(w), len(g)))
        print('ini [%s]: differs from line %d:\n  HEAD %r\n  spec %r' % (sec.name, i + 1, w[i] if i < len(w) else None, g[i] if i < len(g) else None))
    return bad


def main():
    secs = settings_read.read_all()
    rows, fields, groups, game, under = tables(secs)
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, 'rows.h'), 'w', newline='\n') as f:
        f.write('\n'.join(fields) + '\n\n#define SETTINGS(X, S, D) \\\n' + ' \\\n'.join('    ' + r for r in rows) + '\n\n'
                + '#define SETTING_GROUPS(G) \\\n' + ' \\\n'.join('    ' + g for g in groups) + '\n\n'
                + '#define SETTING_GAME(V) \\\n' + ' \\\n'.join('    ' + g for g in game) + '\n')
    with open(os.path.join(OUT, 'ageofai.ini'), 'w', newline='\n') as f:
        f.write('\n'.join(ini_of(s) for s in secs))
    if '--compare' in sys.argv:
        bad = compare(secs, rows, fields, groups, game, under)
        print('%d difference(s)' % bad)
        sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
