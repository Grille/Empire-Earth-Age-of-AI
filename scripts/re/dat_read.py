"""Read Empire Earth's database files (local/games/ee/data/db/*.dat) with the field layouts of the community DB editor
(github.com/EE-modders/Empire-Earth---DB-Editor, GPL-3.0: EEEditorData/Vanilla/*.dats and its English language.txt,
downloaded into scripts/dats/ when missing - not kept in this repository).

A file is a list of groups: a record count (the tech tree's one less than the records) and the records, each the
layout's fields in order - numbers of 1 or 4 bytes, floats, fixed-size strings; a tech tree record ends with as
many object ids as its last field says. Used by gen_units.py.
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
DATS = os.path.join(HERE, '..', '..', 'local', 'games', 'ee', 'dats')
SOURCE = 'https://raw.githubusercontent.com/EE-modders/Empire-Earth---DB-Editor/HEAD/EEEditorData/Vanilla/'
FILES = ('common.dats', 'dbobjects.dats', 'dbtechtree.dats', 'language.txt')


def fetch():
    """the editor's files into scripts/dats/ (once)"""
    import urllib.request
    os.makedirs(DATS, exist_ok=True)
    with open(os.path.join(DATS, '.gitignore'), 'w') as f:
        f.write('*\n')
    for name in FILES:
        p = os.path.join(DATS, name)
        if not os.path.exists(p):
            print('downloading', SOURCE + name)
            urllib.request.urlretrieve(SOURCE + name, p)


def _common():
    out = {}
    for ln in open(os.path.join(DATS, 'common.dats'), encoding='utf-8'):
        if ln.startswith('#') or not ln.strip():
            continue
        f = ln.rstrip('\n').split('\t')
        out[f[0]] = f[1:]
    return out


def layout(name):
    """[(field name, size, type)] of <name>.dats"""
    common = _common()
    fields = []
    for ln in open(os.path.join(DATS, name + '.dats'), encoding='utf-8'):
        if ln.startswith('#') or not ln.strip():
            continue
        f = ln.rstrip('\n').split('\t')
        if f[1].startswith('@'):
            c = common[f[1][1:]]
            size, typ, label = int(c[0]), c[1], c[2]
        else:
            size, typ = int(f[1]), f[2]
            label = f[4] if typ in ('LINK', 'ENUM', 'RANGE') else f[3]
        fields.append((label, size, typ))
    return fields


def read(path, name, adjust=0, extra_count_field=None):
    """the records of a .dat file: a list of {field name: value}; a name used twice keeps the first, the others get
    #2, #3 ...; extra_count_field: that many ints follow the record (list 'extra')"""
    fields = layout(name)
    labels, seen = [], {}
    for label, _, _ in fields:
        seen[label] = seen.get(label, 0) + 1
        labels.append(label if seen[label] == 1 else '%s #%d' % (label, seen[label]))
    data = open(path, 'rb').read()
    pos, recs = 0, []
    while pos < len(data):
        n = struct.unpack_from('<i', data, pos)[0] + adjust
        pos += 4
        if n < 0 or n > 100000:
            raise ValueError('%s: bad record count %d at %d' % (path, n, pos - 4))
        for _ in range(n):
            r = {}
            for (label, size, typ), key in zip(fields, labels):
                if typ == 'STRING':
                    r[key] = data[pos:pos + size].split(b'\0', 1)[0].decode('latin-1')
                elif typ == 'FLOAT':
                    r[key] = struct.unpack_from('<f', data, pos)[0]
                elif size == 1:
                    r[key] = struct.unpack_from('<b', data, pos)[0]
                else:
                    r[key] = struct.unpack_from('<i', data, pos)[0]
                pos += size
            if extra_count_field:
                k = r[extra_count_field]
                r['extra'] = list(struct.unpack_from('<%di' % k, data, pos))
                pos += 4 * k
            recs.append(r)
    return recs


def language(path):
    """{string id: text} of a language.txt (the editor's dump of Language.dll: 123, "text")"""
    out = {}
    for ln in open(path, encoding='utf-8', errors='replace'):
        i = ln.find(', "')
        if i <= 0 or not ln[:i].strip().isdigit():
            continue
        out[int(ln[:i])] = ln[i + 3:].rstrip().rstrip('"')
    return out
