"""Converts .eeai text files to records with a comma after each entry (2026-10-22: records are values, their entries
separated by commas). Before, a record held one entry a line:

    lines = {
        greeting = "Hello."
        farewell = ["Bye.", "Farewell."]
    }

after: greeting = "Hello.", - the comma after the entry's last line (an entry over several lines while a bracket is
open: after the line that closes it), before a comment. Records are those of the old format: "name = {" alone at the
end of its line, outside any other record, up to a "}" alone on a line. A README's examples: inside its ``` blocks only.

usage: eeai_commas.py file ... (rewritten in place; files already converted stay as they are)
"""
import re
import sys

OPEN = re.compile(r'^\s*[A-Za-z_]\w*\s*=\s*\{\s*(#.*)?$')


def scan(line):
    """the line's bracket depth change and where its code ends (a comment outside strings: its #)"""
    depth, i, n = 0, 0, len(line)
    while i < n:
        c = line[i]
        if c in '"\'':
            i += 1
            while i < n and line[i] != c:
                i += 2 if line[i] == '\\' else 1
        elif c == '#':
            return depth, i
        elif c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
        i += 1
    return depth, n


def add_comma(line):
    _, end = scan(line)
    code = line[:end].rstrip()
    if code.endswith(','):
        return line
    rest = line[end:]
    return code + ',' + (' ' + rest if rest else '')


def convert(lines):
    out, in_rec, depth = [], False, 0
    for line in lines:
        body = line.rstrip('\r\n')
        nl = line[len(body):]
        if not in_rec:
            in_rec = bool(OPEN.match(body))
            out.append(line)
            continue
        if depth == 0:
            s = body.strip()
            if s.startswith('}'):
                in_rec = False
                out.append(line)
                continue
            if not s or s.startswith('#'):
                out.append(line)
                continue
        d, _ = scan(body)
        depth += d
        if depth <= 0:
            depth = 0
            body = add_comma(body)
        out.append(body + nl)
    return out


def convert_md(lines):   # a README: its ``` blocks
    out, block, buf = [], False, []
    for line in lines:
        if line.lstrip().startswith('```'):
            if block:
                out += convert(buf)
                buf = []
            block = not block
            out.append(line)
        elif block:
            buf.append(line)
        else:
            out.append(line)
    return out + buf


for path in sys.argv[1:]:
    text = open(path, encoding='latin-1', newline='').read()
    lines = text.splitlines(keepends=True)
    new = ''.join(convert_md(lines) if path.lower().endswith('.md') else convert(lines))
    if new != text:
        open(path, 'w', encoding='latin-1', newline='').write(new)
        print(path, 'converted')
    else:
        print(path, 'unchanged')
