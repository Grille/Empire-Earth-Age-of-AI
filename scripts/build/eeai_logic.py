"""Converts .eeai files to the logical operators of C#, TypeScript and C (2026-11-21, the player: "the difference
between & and && starts to matter"): a and b -> a && b, a or b -> a || b, not x -> !x. The same precedence as
before (! binds as - does, && before ||), so nothing changes but the spelling.

Code only: comments are left alone (prose - an expression in one is converted by hand), a string's text too, but
its {placeholders} are expressions and converted (a string in one as well). A member named so (x.not) stays. A .md
file: its ``` blocks and its `code` spans. A .txt: one expression a line (tests/text_engine_cases.txt).

usage: eeai_logic.py [--check] file ...   (rewritten in place, encoding and line ends kept; --check: only count)
"""
import re
import sys

IDENT = re.compile(r'[A-Za-z_]\w*')
WORDS = {'and': '&&', 'or': '||', 'not': '!'}


def quote_end(t, i):
    """the index after the plain string at t[i] (a string inside a placeholder: its braces aren't counted, as
    dll/script/expr.c x_string reads it); at a line's end if open"""
    q, i, n = t[i], i + 1, len(t)
    while i < n and t[i] not in '\r\n':
        if t[i] == '\\':
            i += 2
            continue
        if t[i] == q:
            return i + 1
        i += 1
    return i


def placeholder_end(t, i):
    """the index after the placeholder starting at t[i] == '{'; at a line's end if open"""
    depth, n = 0, len(t)
    while i < n and t[i] not in '\r\n':
        c = t[i]
        if c in '"\'':
            i = quote_end(t, i)
            continue
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return i


def convert_string(t, i):
    """the string at t[i] converted (its placeholders' expressions), the index after it, the count"""
    n = len(t)
    if t[i] == '@':   # verbatim: no placeholders
        j = i + 2
        while j < n and t[j] not in '\r\n':
            if t[j] == '"':
                if t[j + 1:j + 2] != '"':
                    j += 1
                    break
                j += 1
            j += 1
        return t[i:j], j, 0
    q, out, j, count = t[i], [t[i]], i + 1, 0
    while j < n and t[j] not in '\r\n':
        c = t[j]
        if c == '\\':
            out.append(t[j:j + 2])
            j += 2
            continue
        if c == q:
            out.append(c)
            return ''.join(out), j + 1, count
        if c == '{':
            e = placeholder_end(t, j)
            s, k = convert(t[j + 1:e - 1] if t[e - 1:e] == '}' else t[j + 1:e])
            out.append('{' + s + ('}' if t[e - 1:e] == '}' else ''))
            count += k
            j = e
            continue
        out.append(c)
        j += 1
    return ''.join(out), j, count


def convert(t):
    out, i, n, count, last = [], 0, len(t), 0, ''
    while i < n:
        c = t[i]
        if c == '/' and t[i + 1:i + 2] == '/':
            e = t.find('\n', i)
            e = n if e < 0 else e
            out.append(t[i:e])
            i = e
            continue
        if c == '"' or c == "'" or (c == '@' and t[i + 1:i + 2] == '"'):
            s, i, k = convert_string(t, i)
            out.append(s)
            count += k
            last = '"'
            continue
        m = IDENT.match(t, i)
        if m and (i == 0 or not (t[i - 1].isalnum() or t[i - 1] == '_')):
            w = m.group(0)
            i += len(w)
            if w in WORDS and last != '.':
                out.append(WORDS[w])
                count += 1
                if w == 'not':
                    while i < n and t[i] in ' \t':
                        i += 1
            else:
                out.append(w)
            last = w[-1]
            continue
        out.append(c)
        if c not in ' \t\r\n':
            last = c
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
                s, k = convert(m.group(1))
                count += k
                return '`' + s + '`'
            out.append(SPAN.sub(span, line))
    if buf:
        out.append(''.join(buf))
    return ''.join(out), count


def convert_lines(t):
    out, count = [], 0
    for line in t.splitlines(keepends=True):
        s, k = convert(line)
        out.append(s)
        count += k
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
    low = path.lower()
    new, k = convert_md(text) if low.endswith('.md') else convert_lines(text) if low.endswith('.txt') else convert(text)
    total += k
    if k:
        print('%5d  %s' % (k, path))
        if not check:
            open(path, 'wb').write(new.encode(enc))
print('%d words%s' % (total, ' (nothing written)' if check else ''))
