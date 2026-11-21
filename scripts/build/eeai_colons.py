"""Converts .eeai files to records written with colons (2026-11-21, the player: "most languages use : for dict instead
of ="): an entry of a { ... } is name: value or [key]: value, as in a struct's fields and the type annotations - the
file's top keeps name = value (a name of the file), a block's statements keep theirs (x = 5).

    personality = {                 personality = {
        name = "Khan",          ->      name: "Khan",
        //color = ["blue"],             //color: ["blue"],
    }                               }
    let d: dict<text, int> = { a = 1, [k] = 2 }   ->   { a: 1, [k]: 2 }

A { is a record's where a value goes - after = ( [ , : ? or return (=> { is a block's, as are if x {, f(a) {, struct
s {, else {); its entries are where it opens and after each of its commas. A comment line there that is an entry
commented out (//name = ...) is converted too. Strings are left alone (their {placeholders} too: a dict or struct is
never made in one). A .md file: its ``` blocks and its `code` spans.

usage: eeai_colons.py [--check] file ...   (rewritten in place, encoding and line ends kept; --check: only count)
"""
import re
import sys

IDENT = re.compile(r'[A-Za-z_]\w*')
COMMENTED = re.compile(r'(\s*)([A-Za-z_]\w*)([ \t]*)=(?![=>])([ \t]*)')


def string_end(t, i):
    """the index after the string starting at t[i] (as dll/script/expr.c x_string reads it); at a line's end if open"""
    n = len(t)
    if t[i] == '@':
        i += 2
        while i < n and t[i] not in '\r\n':
            if t[i] == '"':
                if t[i + 1:i + 2] != '"':
                    return i + 1
                i += 1
            i += 1
        return i
    q0, q, raw, depth = t[i], None, False, 0
    i += 1
    while i < n:
        c = t[i]
        if c in '\r\n':
            return i
        if depth:
            if raw:
                if c == '"' and t[i + 1:i + 2] == '"':
                    i += 1
                elif c == '"':
                    q, raw = None, False
            elif q:
                if c == q:
                    q = None
                elif c == '\\' and i + 1 < n and t[i + 1] != '\n':
                    i += 1
            elif c == '@' and t[i + 1:i + 2] == '"':
                i += 1
                q, raw = '"', True
            elif c in '"\'':
                q = c
            elif c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
        elif c == q0:
            return i + 1
        elif c == '\\':
            i += 1
        elif c == '{':
            depth = 1
        i += 1
    return i


def bracket_end(t, i):
    """the index after the ] matching t[i] == '['; -1: none"""
    depth, n = 0, len(t)
    while i < n:
        c = t[i]
        if c == '"' or c == "'" or (c == '@' and t[i + 1:i + 2] == '"'):
            i = string_end(t, i)
            continue
        if c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return -1


def convert(t, last=''):
    """t converted; and how many entries were. last: the code before t's start (a doc's `code` span: '=', a { a
    record's)"""
    out, stack, i, n, count = [], [], 0, len(t), 0
    entry = False   # at a record's entry: where it opens, after its commas
    word = ''   # the code before: its last character (last), its last word
    while i < n:
        c = t[i]
        if c == '/' and t[i + 1:i + 2] == '/':
            e = t.find('\n', i)
            e = n if e < 0 else e
            text = t[i:e]
            if entry:
                m = COMMENTED.match(text, 2)
                line_start = t.rfind('\n', 0, i) + 1
                if m and not t[line_start:i].strip():
                    text = text[:2] + m.group(1) + m.group(2) + ': ' + text[m.end():]
                    count += 1
            out.append(text)
            i = e
            continue
        if c in ' \t\r\n':
            out.append(c)
            i += 1
            continue
        if c == '"' or c == "'" or (c == '@' and t[i + 1:i + 2] == '"'):
            e = string_end(t, i)
            out.append(t[i:e])
            i, last, word, entry = e, '"', '', False
            continue
        if entry and (c == '[' or IDENT.match(t, i)):
            if c == '[':
                e = bracket_end(t, i)
                key = t[i:e] if e > 0 else None
            else:
                key = IDENT.match(t, i).group(0)
                e = i + len(key)
            if key is not None:
                m = re.compile(r'([ \t]*)=(?![=>])([ \t]*)').match(t, e)
                if m:
                    out.append(key + ':' + (' ' if t[m.end():m.end() + 1] not in '\r\n' else ''))
                    count += 1
                    i, last, word, entry = m.end(), ':', '', False
                    continue
        entry = False
        if c == '{':
            rec = (last != '' and last in '=([,:?') or word == 'return'
            stack.append('R' if rec else 'B')
            entry = rec
        elif c in '([':
            stack.append(c)
        elif c in ')]}':
            if stack:
                stack.pop()
        elif c == ',' and stack and stack[-1] == 'R':
            entry = True
        m = IDENT.match(t, i)
        if m:
            word = m.group(0)
            out.append(word)
            i += len(word)
            last = word[-1]
            continue
        out.append(c)
        last, word = c, ''
        i += 1
    return ''.join(out), count


SPAN = re.compile(r'`([^`\n]+)`')


def convert_md(t):
    lines, out, block, buf, count = t.splitlines(keepends=True), [], False, [], 0
    for line in lines:
        if line.lstrip().startswith('```'):
            if block:
                s, k = convert(''.join(buf))
                out.append(s)
                count += k
                buf = []
            block = not block
            out.append(line)
        elif block:
            buf.append(line)
        else:
            def span(m):
                nonlocal count
                s, k = convert(m.group(1), '=')
                count += k
                return '`' + s + '`'
            out.append(SPAN.sub(span, line))
    if buf:
        out.append(''.join(buf))
    return ''.join(out), count


check = '--check' in sys.argv
total = 0
for path in [a for a in sys.argv[1:] if a != '--check']:
    raw = open(path, 'rb').read()
    enc = 'utf-8'
    try:
        text = raw.decode('utf-8')
    except UnicodeDecodeError:
        enc = 'latin-1'
        text = raw.decode('latin-1')
    new, k = convert_md(text) if path.lower().endswith('.md') else convert(text)
    total += k
    if k:
        print('%5d  %s' % (k, path))
        if not check:
            open(path, 'wb').write(new.encode(enc))
print('%d entries%s' % (total, ' (nothing written)' if check else ''))
