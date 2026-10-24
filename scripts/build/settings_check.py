"""The settings' rows (dll/settings/rows.h) against the shipped config/aimod.ini, run by dll/build.sh: every row of
aimod.ini's sections is in the file - set (key=value, then with the row's default: the built-in default is what the
shipped file says) or shown commented out (;key=example) - every key set in a whole section (k_settings_whole) is a
row, a key of [easy] / [medium] / [hard] a row of [all] (D), and the shipped [easy] sets what SETTINGS_EASY says. A row
of section autotest is the test ini's own and not looked for. Exit 1 with the differences."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROW = re.compile(r'\b(?:[XS]\((\w+),\s*(\w+),|D\((\w+),)\s*(\w+),\s*("[^"]*"|-?\w+),')
EASY = re.compile(r'\bE\((\w+),\s*"([^"]*)"\)')
DIFF = ('easy', 'medium', 'hard')


def rows():
    src = open(os.path.join(ROOT, 'dll', 'settings', 'rows.h'), encoding='utf-8').read()
    whole = re.search(r'k_settings_whole\[\]\s*=\s*\{([^}]*)\}', src).group(1)
    table = []
    for m in ROW.finditer(src):
        kind, sec = (m.group(1), m.group(2)) if m.group(1) else (m.group(3), 'all')
        table.append((kind, sec, m.group(4), m.group(5)))
    return table, re.findall(r'"(\w+)"', whole), dict(EASY.findall(src))


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
    if kind in ('FLAG', 'INT', 'NUM'):
        try: return int(value, 0) == int(want, 0)
        except ValueError: return False
    return value == want


def main():
    table, whole, easy = rows()
    keys = ini()
    bad = []
    known = {}
    for kind, sec, key, default in table:
        if sec == 'autotest': continue
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
            rows_of = known.get('all' if sec in DIFF else sec, [])
            if not any(k == name or (kind == 'STR_P' and re.fullmatch(re.escape(k) + r'\d+', name)) for kind, k in rows_of):
                bad.append('[%s] %s: in config/aimod.ini, but no row in dll/settings/rows.h' % (sec, name))
    shipped_easy = {k: v for k, v in keys.get('easy', {}).items() if v is not None}
    if shipped_easy != easy:
        bad.append('[easy] in config/aimod.ini sets %s, SETTINGS_EASY says %s' % (shipped_easy, easy))
    for b in bad: print('settings: ' + b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
