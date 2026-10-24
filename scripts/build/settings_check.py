"""The settings' rows (dll/settings/rows.h) against the shipped config/aimod.ini, run by dll/build.sh: every row of
aimod.ini's sections is in the file - set (key=value, then with the row's default: the built-in default is what the
shipped file says) or shown commented out (;key=example) - and every key set in a whole section (k_settings_whole) is
a row. A row of section autotest is the test ini's own and not looked for. Exit 1 with the differences."""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ROW = re.compile(r'\b[XS]\((\w+),\s*(\w+),\s*(\w+),\s*("[^"]*"|-?\d+),')


def rows():
    src = open(os.path.join(ROOT, 'dll', 'settings', 'rows.h'), encoding='utf-8').read()
    whole = re.search(r'k_settings_whole\[\]\s*=\s*\{([^}]*)\}', src).group(1)
    return [m.groups() for m in ROW.finditer(src)], re.findall(r'"(\w+)"', whole)


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


def main():
    table, whole = rows()
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
            if value is None: continue
            want = default.strip('"') + ('%' if kind == 'PCT' else '')
            if value != want:
                bad.append('[%s] %s=%s in config/aimod.ini, the row\'s default %s' % (sec, name, value, want))
    for sec in whole:
        for name in keys.get(sec, {}):
            if not any(k == name or (kind == 'STR_P' and re.fullmatch(re.escape(k) + r'\d+', name)) for kind, k in known.get(sec, [])):
                bad.append('[%s] %s: in config/aimod.ini, but no row in dll/settings/rows.h' % (sec, name))
    for b in bad: print('settings: ' + b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
