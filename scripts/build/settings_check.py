"""The settings' rows (dll/settings/rows.h) against the shipped config/aimod.ini, run by dll/build.sh: every row of
aimod.ini's sections is in the file - set (key=value, then with the row's default: the built-in default is what the
shipped file says) or shown commented out (;key=example) - every key set in a section is a row, a key of [ai_easy] /
[ai_hard] a row of [ai_base] (D), and the shipped [ai_easy] sets what SETTINGS_EASY says. The sections
(SETTING_SECTIONS): every row's section is one, every section of the file is one and every one of them is in the file
(but the test ini's, kind TEST: its rows aren't looked for), each named once, each order its own, the DIFF ones
easy and hard. A CHILD's master is a CHOICE default|enabled|disabled (default "default") before it in its
section, and the child is only commented out in the file (set, it would never follow). Exit 1 with the differences."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROW = re.compile(r'\b(?:[XS]\((\w+),\s*(\w+),|D\((\w+),)\s*(\w+),\s*("[^"]*"|-?\w+),\s*("[^"]*"|-?\w+),')
MASTER = '"default|enabled|disabled"'
SECTION = re.compile(r'\bT\((\w+),\s*(-?\d+),\s*(\w+),\s*"[^"]*"\)')
EASY = re.compile(r'\bE\((\w+),\s*"([^"]*)"\)')
DIFF = ('ai_easy', 'ai_hard')   # the medium AI plays [ai_base]: no section of its own


def rows():
    src = open(os.path.join(ROOT, 'dll', 'settings', 'rows.h'), encoding='utf-8').read()
    sections = [(sec, int(order), kind) for sec, order, kind in SECTION.findall(src)]
    table = []
    for m in ROW.finditer(src):
        kind, sec = (m.group(1), m.group(2)) if m.group(1) else (m.group(3), 'ai_base')
        table.append((kind, sec, m.group(4), m.group(5), m.group(6)))
    return table, sections, dict(EASY.findall(src))


def ini():
    """{section: {key: (value or None when commented out)}} - a commented-out key is a line ;key=... (no space)"""
    out, sec = {}, None
    for line in open(os.path.join(ROOT, 'config', 'aimod.ini'), encoding='utf-8', errors='replace'):
        line = line.strip()
        m = re.match(r'\[(\w+)\]$', line)
        if m:
            sec = out.setdefault(m.group(1).lower(), {})
            continue
        m = re.match(r'(;?)(\w+)=(.*)$', line)
        if m and sec is not None:
            value = None if m.group(1) else m.group(3).split(';')[0].strip()
            if m.group(2) not in sec or value is not None: sec[m.group(2)] = value
    return out


def same(kind, value, default):
    """the file's value is the row's default (a share either way: 35% or 0.35)"""
    want = default.strip('"')
    if kind == 'PCT': return value == want + '%'
    if kind == 'SHARE':
        try: x = float(value[:-1]) / 100 if value.endswith('%') else float(value)
        except ValueError: return False
        return abs(x - int(want) / 100) < 1e-9
    if kind in ('FLAG', 'CHILD', 'INT', 'NUM'):
        try: return int(value, 0) == int(want, 0)
        except ValueError: return False
    return value == want


def main():
    table, sections, easy = rows()
    keys = ini()
    bad = []
    secs = [sec for sec, _, _ in sections]
    for sec in sorted({sec for sec in secs if secs.count(sec) > 1}):
        bad.append('[%s]: two section rows in dll/settings/rows.h' % sec)
    orders = [order for _, order, _ in sections]
    for sec, order, _ in sections:
        if orders.count(order) > 1: bad.append('[%s]: its order %d is another section\'s too' % (sec, order))
    diff = [sec for sec, _, kind in sections if kind == 'DIFF']
    if sorted(diff) != sorted(DIFF): bad.append('the DIFF sections are %s, settings.c k_set_diff %s' % (diff, list(DIFF)))
    test = {sec for sec, _, kind in sections if kind == 'TEST'}
    whole = [sec for sec, _, kind in sections if kind != 'TEST']
    for sec in sorted({row[1] for row in table} - set(secs)):
        bad.append('[%s]: a row\'s section, but not in SETTING_SECTIONS' % sec)
    for sec in keys:
        if sec not in secs or sec in test: bad.append('[%s] in config/aimod.ini, but no section of SETTING_SECTIONS' % sec)
    for sec in whole:
        if sec not in keys: bad.append('[%s]: in SETTING_SECTIONS, but not in config/aimod.ini' % sec)
    for i, (kind, sec, key, default, lo) in enumerate(table):   # a CHILD's master: a CHOICE before it, in its section
        if kind != 'CHILD': continue
        master = [r for r in table[:i] if r[2] == lo and r[1] == sec]
        if not master or master[0][0] != 'CHOICE' or master[0][4] != MASTER or master[0][3] != '"default"':
            bad.append('[%s] %s: its master %s is no CHOICE row before it in its section, %s, default "default"'
                       % (sec, key, lo, MASTER))
        if keys.get(sec, {}).get(key) is not None:
            bad.append('[%s] %s: set in config/aimod.ini - a child follows its master only commented out (;%s=...)'
                       % (sec, key, key))
    known = {}
    for kind, sec, key, default, _ in table:
        if sec in test: continue
        known.setdefault(sec, []).append((kind, key))
        have = keys.get(sec, {})
        names = [k for k in have if k == key or (kind == 'STR_P' and re.fullmatch(re.escape(key) + r'\d+', k))]
        if not names:
            bad.append('[%s] %s: a row, but not in config/aimod.ini (set to its default, or ;%s=example)' % (sec, key, key))
        for name in names:
            value = have[name]
            if value is not None and not same(kind, value, default):
                bad.append('[%s] %s=%s in config/aimod.ini, the row\'s default %s' % (sec, name, value, default))
    for sec in whole:
        for name in keys.get(sec, {}):
            rows_of = known.get('ai_base' if sec in DIFF else sec, [])
            if not any(k == name or (kind == 'STR_P' and re.fullmatch(re.escape(k) + r'\d+', name)) for kind, k in rows_of):
                bad.append('[%s] %s: in config/aimod.ini, but no row in dll/settings/rows.h' % (sec, name))
    shipped_easy = {k: v for k, v in keys.get('ai_easy', {}).items() if v is not None}
    if shipped_easy != easy:
        bad.append('[ai_easy] in config/aimod.ini sets %s, SETTINGS_EASY says %s' % (shipped_easy, easy))
    for b in bad: print('settings: ' + b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
