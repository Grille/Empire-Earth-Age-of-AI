"""ageofai.ini's keys made C and text (run by build.sh): dll_cs/settings/*.cs (settings_read.py) -> dll/settings/rows.h
(the rows, the FLAGS rows' fields, the sections, the groups, the easy AI's values, the game's values, the folded rows -
with the comments around them, settings_text.py) and the shipped config/ageofai.ini (every section's text, each key
under its lines). Both are written whole and only when they change. They are generated, not edited: rows.h's header
keeps a hash of itself and of the ini, and a file whose content no longer matches its hash was changed by hand - then
nothing is written (exit 1, the file named: the change belongs into dll_cs/settings/; --force overwrites it).
usage: gen_settings.py [--force] [--print]   (--print: the two files to stdout, nothing written)"""
import hashlib
import os
import sys
import textwrap

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import settings_read   # noqa: E402
from settings_text import TEXT   # noqa: E402

ROOT = settings_read.ROOT
ROWS_H = os.path.join(ROOT, 'dll', 'settings', 'rows.h')
INI = os.path.join(ROOT, 'config', 'ageofai.ini')
HASH_LINE = '// generated: '


def c_str(s):
    return '"%s"' % s.replace('\\', '\\\\').replace('"', '\\"')


def cs_str(d):   # a C# string literal's text (the same escapes as C's)
    return d[1:-1].replace('\\"', '"').replace('\\\\', '\\') if d else ''


def default_of(sec, k):   # the default as the code's row has it and as the file writes it: (row text, file text)
    kind = settings_read.kind_of(sec, k)
    d = k.default
    if kind == 'FLAG':
        v = '1' if d == 'true' else '0'
        return v, v
    if kind == 'CHOICE':
        name = sec.enums[k.type][d.split('.')[-1].lstrip('@')]   # scroll_choice.game -> game, sound_sample_rate_choice.hz_48000 -> 48000
        return c_str(name), name
    if kind == 'FLAGS':   # [on("a, b")]: those fields on
        on = settings_read.arg_text(k.attrs['on'][0]) if k.has('on') else ''
        return c_str(on), on
    if kind in ('STR', 'STR_P'):
        s = d if k.type == 'vec2' else cs_str(d)   # a vec2 key's: the expression as written (a name of the section's)
        return c_str(s), s
    if kind in ('SHARE', 'FACTOR'):
        v = settings_read.percent(d, k.where)
        return v, v + '%'
    v = d or '0'
    return v, v


def diff_value(base, k):   # a difficulty's own value of an [ai_base] key, as the file writes it
    row = next((b for b in base.keys if b.name == k.name), None)
    if not row:
        raise SystemExit('%s: %s is no key of [ai_base]' % (k.where, k.name))
    kind = settings_read.kind_of(base, row)
    if k.type == 'bool':
        return '1' if k.default == 'true' else '0'
    if k.type in ('share', 'factor') or kind in ('SHARE', 'FACTOR'):
        return settings_read.percent(k.default, k.where) + '%'
    if k.type == 'int':
        return k.default
    v = cs_str(k.default)
    if kind == 'CHOICE' and v not in base.enums[row.type].values():
        raise SystemExit('%s: %s is none of %s' % (k.where, v, ', '.join(base.enums[row.type].values())))
    return v


def row_of(sec, k):   # X(FLAG, general, enabled, 1, 0, 0) - D for a row of [ai_base], else X
    kind = settings_read.kind_of(sec, k)
    a = k.attrs
    d, _ = default_of(sec, k)
    arg = settings_read.arg_text
    lo_hi = {
        'FLAG': ('0', '0'),
        'INT': tuple(arg(x) for x in a.get('range', [])[:2]) or ('0', '0'),
        'NUM': tuple(arg(x) for x in a.get('range', [])[:2]) or ('0', '0'),
        'LIMIT': tuple(arg(x) for x in a.get('range', [])[:2]),
        'SHARE': tuple(settings_read.percent(arg(x), k.where) for x in a.get('range', [])[:2]) or ('-1000000', '1000000'),   # no bound
        'CHOICE': (c_str(','.join(sec.enums.get(k.type, {}).values())), '0'),   # a name may hold a | (default | file)
        'STR': ('48', '1') if k.type == 'vec2' else (arg(a['length'][0]) if 'length' in a else '0', '0'),   # a vec2 key: b 1
        'STR_P': (arg(a['length'][0]) if 'length' in a else '0', '0'),
        'FACTOR': tuple(settings_read.percent(arg(x), k.where) for x in a.get('range', [])[:2]) or ('-1000000', '1000000'),
        'FLAGS': ('%s_FIELDS' % k.name.upper(), '0'),
    }[kind]
    if sec.kind == 'ALL':
        return 'D(%s %s %s, %s, %s)' % ((kind + ',').ljust(6), (k.name + ',').ljust(30), d, lo_hi[0], lo_hi[1])
    return 'X(%s %s %s %s, %s, %s)' % ((kind + ',').ljust(7), (sec.name + ',').ljust(9), (k.name + ',').ljust(25),
                                     d, lo_hi[0], lo_hi[1])


def field_name(sec, k):   # g_set's (and g_set_ai's): the key - one name across the file, whatever its section
    return k.name


def comment(lines):
    return ['// ' + x if x else '//' for x in lines]


def table(name, args, lines):
    return comment(TEXT[name]) + ['#define %s(%s) \\' % (name, args)] + ['    %s \\' % x for x in lines[:-1]] + ['    ' + lines[-1], '']


def rows_h(secs):   # the file below its header
    fields, rows, groups, game, members, also, titles, easy, target, named = [], [], [], [], [], [], [], [], [], []
    arg = settings_read.arg_text
    seen = {}
    for sec in secs:
        for cname, text in sec.consts.items():
            if text == 'game_input<vec2>()':   # the code's own value goes over it; this one only for the check when the file is read (1: dividing by it is no mistake)
                text = 'vec(1, 1)'
            named.append('N(%s %s %s)' % ((sec.name + ',').ljust(10), (cname + ',').ljust(10), c_str(text)))
        for k in sec.keys if sec.kind != 'DIFF' else []:
            if k.name in seen:
                raise SystemExit("%s: key %s is [%s]'s already (%s) - a key's name is its own across the file" % (k.where, k.name, seen[k.name][0], seen[k.name][1]))
            seen[k.name] = (sec.name, k.where)
        for sname, (flds, doc) in sec.structs.items():
            parts = []
            for t, f in flds:
                parts.append('field_bool(%s)' % f if t == 'bool' else 'field_enum(%s, %s)' % (f, c_str('/'.join(sec.enums[t].values()))))
            fields += comment(doc) + ['#define %s_FIELDS(field_bool, field_enum) %s' % (sname[:-len('_fields')].upper(), ' '.join(parts))]
        if sec.kind == 'DIFF':
            if sec.name == 'ai_easy':
                base = next(s for s in secs if s.kind == 'ALL')
                easy += ['E(%s %s)' % ((k.name + ',').ljust(17), c_str(diff_value(base, k).rstrip('%'))) for k in sec.keys]
            continue
        for k in sec.keys:
            rows.append(row_of(sec, k))
            fn = field_name(sec, k)
            if 'group' in k.attrs:
                groups.append('G(%s %s)' % ((fn + ',').ljust(23), c_str(arg(k.attrs['group'][0]))))
            if 'game' in k.attrs:
                g = k.attrs['game']
                game.append('V(%s %s, %s, %s)' % ((fn + ',').ljust(21), c_str(arg(g[0])), c_str(arg(g[1])), arg(g[2])))
            if 'title' in k.attrs:
                titles.append('K(%s %s)' % ((k.name + ',').ljust(20), c_str(arg(k.attrs['title'][0]))))
            if 'also' in k.attrs:
                to = settings_read.arg_text(k.attrs['also'][0]).strip('"')
                if to not in [s.name for s in secs] or to == sec.name:
                    raise SystemExit('%s: [also("%s")]: no other section of that name' % (k.where, to))
                also.append('A(%s %s)' % ((k.name + ',').ljust(20), to))
            if 'target' in k.attrs:
                to = settings_read.arg_text(k.attrs['target'][0])
                if to not in ('ee', 'aoc'):
                    raise SystemExit('%s: [target(%s)]: ee or aoc' % (k.where, to))
                target.append('B(%s %s)' % ((k.name + ',').ljust(20), to))
            if k.master:
                members.append('M(%s %s)' % ((k.name + ',').ljust(20), k.master))
    sections = ['T(%s %s %s %s)' % ((s.name + ',').ljust(10), ('%d,' % s.place).rjust(4), (s.kind + ',').ljust(6), c_str(s.title))
                for s in secs]
    out = comment(TEXT['top']) + ['', "// FLAGS rows' fields:"] + fields + ['']
    out += table('SETTINGS', 'X, D', rows)
    out += table('SETTING_SECTIONS', 'T', sections)
    out += table('SETTING_GROUPS', 'G', groups)
    out += table('SETTINGS_EASY', 'E', easy)
    out += table('SETTING_GAME', 'V', game)
    out += table('SETTING_MEMBERS', 'M', members)
    out += table('SETTING_ALSO', 'A', also)
    out += table('SETTING_TARGET', 'B', target)
    out += table('SETTING_TITLES', 'K', titles)
    out += table('SETTING_NAMED', 'N', named)
    return '\n'.join(out).rstrip('\n') + '\n'


def values_doc(sec, k):   # a CHOICE's or FLAGS key's list under its doc, of its members' own /// lines:
    # "  name = text", the names padded to the longest, the text flowed to the file's width (120) under itself
    docs = sec.docs.get(k.type)
    if not docs or not any(docs.values()):
        return []
    names = sec.enums[k.type] if k.type in sec.enums else {f: f for _, f in sec.structs[k.type][0]}
    w = max(len(names[m]) for m in docs)
    out = []
    for m, d in docs.items():
        if not d:
            raise SystemExit('%s: %s.%s has no /// line - every member of a documented %s has one' % (k.where, k.type, m, k.type))
        text = textwrap.wrap(' '.join(x.strip() for x in d), 113 - w, break_long_words=False, break_on_hyphens=False)
        out.append('  %s = %s' % (names[m].ljust(w), text[0]))
        out += ['  %s   %s' % (' ' * w, x) for x in text[1:]]
    return out


def ini(secs):
    out = []
    base = next(s for s in secs if s.kind == 'ALL')

    def text(lines):
        return ['; ' + x if x else '' for x in lines]
    for sec in secs:
        if sec.kind == 'TEST':
            continue
        out += text(sec.before) + ['[%s]' % sec.name]
        for k in sec.keys:
            out += text(k.doc + values_doc(sec, k))
            if sec.kind == 'DIFF':
                out.append('%s=%s' % (k.name, diff_value(base, k)))
                continue
            _, v = default_of(sec, k)
            c = k.attrs.get('commented')
            if c is None and k.master:   # a master's key: commented out (only file and the combinations read it), its default to try
                c = []
            if c is None:
                out.append('%s=%s' % (k.name, v))
            else:   # [commented], [commented("x")], [commented("x", "key_P2")]
                out.append(';%s=%s' % (settings_read.arg_text(c[1]) if len(c) > 1 else k.name, settings_read.arg_text(c[0]) if c and c[0] else v))
        out += text(sec.trailer)
    return '\n'.join(out) + '\n'


def digest(s):
    return hashlib.sha1(s.replace('\r\n', '\n').encode('utf-8')).hexdigest()[:16]


def read(path):
    try:
        return open(path, encoding='utf-8', newline='').read().replace('\r\n', '\n')
    except FileNotFoundError:
        return None


def main():
    secs = settings_read.read_all()
    body, ini_text = rows_h(secs), ini(secs)
    head = ['// GENERATED by scripts/build/gen_settings.py from dll_cs/settings/*.cs (ageofai.ini\'s keys) - not edited by hand:',
            '// a setting is a field there (dll_cs/settings/settings.cs: the format), config/ageofai.ini is made of them too.',
            '%srows.h %s, ageofai.ini %s' % (HASH_LINE, digest(body), digest(ini_text))]
    rows_text = '\n'.join(head) + '\n' + body
    if '--print' in sys.argv:
        sys.stdout.write(rows_text + '\n' + ini_text)
        return
    cur_rows, cur_ini = read(ROWS_H), read(INI)
    if '--force' not in sys.argv:   # a file changed by hand since it was made: told, nothing written
        lines = (cur_rows or '').split('\n')
        rec = lines[2][len(HASH_LINE):].split(', ') if len(lines) > 2 and lines[2].startswith(HASH_LINE) else None
        was = {'rows.h': rec[0].split()[-1], 'ageofai.ini': rec[1].split()[-1]} if rec and len(rec) == 2 else {}
        bad = []
        if cur_rows is not None and cur_rows != rows_text and digest('\n'.join(lines[3:])) != was.get('rows.h'):
            bad.append('dll/settings/rows.h')
        if cur_ini is not None and cur_ini != ini_text and digest(cur_ini) != was.get('ageofai.ini'):
            bad.append('config/ageofai.ini')
        if bad:
            raise SystemExit('gen_settings.py: %s changed by hand - a setting is a field of dll_cs/settings/*.cs (the '
                             'files are made of them); put the change there, then build (--force overwrites them)' % ' and '.join(bad))
    for path, text, cur in ((ROWS_H, rows_text, cur_rows), (INI, ini_text, cur_ini)):
        if cur != text:
            open(path, 'w', encoding='utf-8', newline='\n').write(text)
            print('gen_settings.py: wrote %s' % os.path.relpath(path, ROOT).replace(os.sep, '/'))


if __name__ == '__main__':
    main()
