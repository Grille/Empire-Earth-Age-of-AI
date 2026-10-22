"""Rules of the C style the compiler doesn't check (run by dll/build.sh: a violation fails the build).

- `int* a, b;` is refused: the * reads as part of the type, but C gives it to the first name only - b is a plain
  int. The code puts the * on the name (`int *a, b;`, `uint8_t *u = it.u, *td = it.td;`), where it says what it
  does; a declaration with one name may still write it either way.
- `internal` at file level, `static` inside a function (base/lang.h): C's static means two things - a name only
  this DLL sees, and a local that keeps its value between calls - and each gets its own word. File level is told by
  the braces (outside every function body and initializer), not by the indentation.

    python scripts/style.py [files]     the check (default: every .c/.h under dll/; exit 1 on a violation)
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# a declaration's start: qualifiers, the type (struct x or a name), then the * glued to the type and a space
STAR_ON_TYPE = re.compile(r'^(?:(?:static|internal|const|volatile|unsigned|signed|extern)\s+)*(?:struct\s+)?\w+\*+\s+\w')
NOT_TYPES = {'return', 'case', 'goto', 'else', 'sizeof'}


def blank(src):
    """The source with comments, strings and character literals turned to spaces (lines kept)."""
    keep = lambda m: re.sub(r'[^\n]', ' ', m.group(0))
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', keep, src, flags=re.S)


def top_comma(s):
    depth = 0
    for ch in s:
        if ch in '([{': depth += 1
        elif ch in ')]}': depth -= 1
        elif ch == ',' and depth == 0: return True
    return False


def check(path, rel):
    bad = []
    src = blank(open(path, encoding='latin-1').read())
    words = not rel.startswith('dinput/') and rel != 'base/lang.h'   # dinput/: its own DLL
    depth, cont = 0, False
    for ln, line in enumerate(src.split('\n'), 1):
        pre, cont = cont or line.lstrip().startswith('#'), line.rstrip().endswith('\\')
        if pre: continue                        # a preprocessor line (and its continuations): a macro's body
        for st in re.split(r'[;{}]', line):
            st = st.strip()
            if st.startswith('for') and '(' in st: st = st[st.index('(') + 1:].strip()   # for (int* p = a, q ...
            if STAR_ON_TYPE.match(st) and st.split()[0] not in NOT_TYPES and top_comma(st):
                bad.append((ln, 'write the * on each name (int *a, b): a * on the type still makes only the first '
                                'name a pointer: %s' % st))
        if words:
            for m in re.finditer(r'[{}]|\b(static|internal)\b', line):
                if m.group(0) == '{': depth += 1
                elif m.group(0) == '}': depth -= 1
                elif m.group(1) == 'static' and depth == 0:
                    bad.append((ln, 'internal at file level (base/lang.h) - static only inside a function'))
                elif m.group(1) == 'internal' and depth > 0:
                    bad.append((ln, 'static inside a function (a variable that keeps its value) - internal is for '
                                    'file level'))
    return bad


def main(args):
    files = args or sorted(glob.glob(os.path.join(ROOT, 'dll', '**', '*.[ch]'), recursive=True))
    n = 0
    for f in files:
        name = os.path.relpath(f, ROOT) if f.startswith(ROOT) else f
        rel = os.path.relpath(f, os.path.join(ROOT, 'dll')).replace(os.sep, '/') if f.startswith(ROOT) else f
        for ln, msg in check(f, rel):
            n += 1
            print('%s:%d: %s' % (name, ln, msg))
    return 1 if n else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
