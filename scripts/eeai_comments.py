"""Turn a .eeai file's # comments into // (the game's own scripts' style: .rmv, .tai). Strings are left alone - read as
dll/script/expr.c x_string reads them: one line, "..." or '...', \\ escapes, {placeholders} with strings of their own.

usage: eeai_comments.py [--check] file.eeai ...   (--check: only count, write nothing; files are kept byte for byte
otherwise - encoding and line ends)
"""
import sys


def string_end(line, i):
    """the index after the string starting at line[i], or -1 (not closed on its line)"""
    q0, q, depth = line[i], None, 0
    i += 1
    while i < len(line):
        c = line[i]
        if c in b'\r\n':
            return -1
        if depth:
            if q:
                if c == q:
                    q = None
                elif c == ord('\\'):
                    i += 1
            elif c in b'"\'':
                q = c
            elif c == ord('{'):
                depth += 1
            elif c == ord('}'):
                depth -= 1
        elif c == q0:
            return i + 1
        elif c == ord('\\'):
            i += 1
        elif c == ord('{'):
            depth = 1
        i += 1
    return -1


def convert_line(line):
    """the line with its comment's # made // - and whether it had one"""
    i = 0
    while i < len(line):
        c = line[i]
        if c in b'"\'':
            e = string_end(line, i)
            if e < 0:
                return line, False   # (a mistake the reader tells: left as it is)
            i = e
            continue
        if c == ord('#'):
            return line[:i] + b'//' + line[i + 1:], True
        if line[i:i + 2] == b'//':
            return line, False   # already one
        i += 1
    return line, False


def main():
    args = sys.argv[1:]
    check = '--check' in args
    total = 0
    for path in [a for a in args if a != '--check']:
        data = open(path, 'rb').read()
        out, n = [], 0
        for line in data.splitlines(keepends=True):
            new, hit = convert_line(line)
            out.append(new)
            n += hit
        total += n
        if n and not check:
            open(path, 'wb').write(b''.join(out))
        if n:
            print('%s: %d comment%s' % (path, n, '' if n == 1 else 's'))
    print('%d comments%s' % (total, ' (nothing written)' if check else ''))


if __name__ == '__main__':
    main()
