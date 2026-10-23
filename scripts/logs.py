"""The test logs: run_test.ps1 stores them gzipped (work/autotest_*.csv.gz, since 2026-10-23: a log is ~7 times
smaller so), the player's own (play_*.csv) and a game's running log stay plain - the scripts read both through these."""
import glob, gzip, os


def open_log(path, encoding=None, errors='replace', newline=None):
    """a log as text, gzipped or not (the arguments as open()'s)"""
    if path.endswith('.gz'):
        return gzip.open(path, 'rt', encoding=encoding, errors=errors, newline=newline)
    return open(path, encoding=encoding, errors=errors, newline=newline)


def read_log(path, **kw):
    with open_log(path, **kw) as f:
        return f.read()


def log_paths(args):
    """the logs args name: a file as it is (x.csv: x.csv.gz when only that is there), a pattern (* ?) its matches in
    name order - one ending in .csv its .csv.gz too, so the patterns of the usage lines find both"""
    out = []
    for a in args:
        if not any(c in a for c in '*?'):
            out.append(a + '.gz' if not os.path.exists(a) and os.path.exists(a + '.gz') else a)
            continue
        out += sorted(set(glob.glob(a) + (glob.glob(a + '.gz') if a.endswith('.csv') else [])))
    return out
