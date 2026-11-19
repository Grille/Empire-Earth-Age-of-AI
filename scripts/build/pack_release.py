"""Packs a release: local/work/release/ageofai-<date>-<commit>.zip - unpacked into the game folder (next to
Empire Earth.exe) it is the mod as docs/installing.md "Installing" lays it out:
    ageofai.dll, ageofai_config.exe            beside the game
    ageofai/ageofai.ini, ageofai/personalities/ the mod's folder: the shipped settings and personalities
    ageofai/README.txt, ageofai/LICENSE.txt     for the player (config/README.txt, the repository's LICENSE: the GPL
                                                asks its text to go with the binaries); not in the game folder's root
                                                (the player 2026-10-27: only the DLL and the config exe there). A
                                                changelog goes beside them from the second release on (the player
                                                2026-11-18: the first binary release needs none)
and beside the zip ageofai_check.exe, the .eeai checker - a download of its own on the release page, never in the zip
(the player 2026-11-02: for modders) - and SHA256SUMS.txt, a SHA-256 for the zip, the checker and each file in the zip
as it lies unpacked (the first request of the release page's issue 1, 2026-11-19), to paste into the release notes with
the commit it prints. The same commit builds the same bytes (scripts/build/build.sh REPRO; every entry of the zip
dated the commit's time), so anyone can check a release by building its commit. The game's settings in a file are ageofai_config.exe's to switch on (Game).
Built from the committed state - a clean `git archive HEAD` built anew (as scripts/build/install.sh), never the
working tree, which may hold another session's unfinished changes: commit first.
    python scripts/build/pack_release.py"""
import datetime
import hashlib
import os
import shutil
import subprocess
import sys
import zipfile
import zlib

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
git = lambda *a: subprocess.run(['git', *a], cwd=root, check=True, capture_output=True).stdout
rev = git('rev-parse', '--short', 'HEAD').decode().strip()
full = git('rev-parse', 'HEAD').decode().strip()
when = datetime.datetime.fromtimestamp(int(git('log', '-1', '--format=%ct', full)), datetime.timezone.utc)
if git('status', '--porcelain', '--untracked-files=no').strip():
    print('note: the working tree has changes - they are not in the release (HEAD %s is)' % rev)
build = os.path.join(root, 'local', 'work', 'release_build')
shutil.rmtree(build, ignore_errors=True)
os.makedirs(build)
subprocess.run(['tar', '-x', '-C', build.replace('\\', '/')], input=git('archive', rev), check=True)
env = dict(os.environ, PY=os.path.join(root, 'local', 'venv', 'Scripts', 'python.exe').replace('\\', '/'),
           CC=os.path.join(root, 'local', 'tools', 'llvm-mingw', 'bin', 'i686-w64-mingw32-clang').replace('\\', '/'))
b = subprocess.run(['sh', 'build.sh'], cwd=build, env=env, capture_output=True, text=True)
if b.returncode:
    sys.exit(b.stdout + b.stderr + '\nthe build failed - nothing packed')
out_dir = os.path.join(root, 'local', 'work', 'release')
os.makedirs(out_dir, exist_ok=True)
out = os.path.join(out_dir, 'ageofai-%s-%s.zip' % (datetime.date.today().isoformat(), rev))
stamp = (max(when.year, 1980),) + when.timetuple()[1:6]   # every entry the commit's time: the same zip from the same commit


class Zip(zipfile.ZipFile):
    def write(self, path, name):
        info = zipfile.ZipInfo(name, stamp)
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o644 << 16
        with open(path, 'rb') as f:
            self.writestr(info, f.read())


with Zip(out, 'w') as z:
    for f in ('ageofai.dll', 'ageofai_config.exe'):
        z.write(os.path.join(build, 'dll', f), f)
    z.write(os.path.join(build, 'config', 'ageofai.ini'), 'ageofai/ageofai.ini')
    z.write(os.path.join(build, 'config', 'texfix.eeai'), 'ageofai/texfix.eeai')
    z.write(os.path.join(build, 'config', 'language.eeai'), 'ageofai/language.eeai')
    z.write(os.path.join(build, 'config', 'setup.eeai'), 'ageofai/setup.eeai')
    z.write(os.path.join(build, 'config', 'README.txt'), 'ageofai/README.txt')
    z.write(os.path.join(build, 'LICENSE'), 'ageofai/LICENSE.txt')
    pers = os.path.join(build, 'config', 'personalities')
    for d, _, files in os.walk(pers):
        for f in sorted(files):
            p = os.path.join(d, f)
            z.write(p, 'ageofai/personalities/' + os.path.relpath(p, pers).replace(os.sep, '/'))
check = os.path.join(out_dir, 'ageofai_check.exe')
shutil.copy(os.path.join(build, 'dll', 'ageofai_check.exe'), check)
names = zipfile.ZipFile(out).namelist()
sha = lambda data: hashlib.sha256(data).hexdigest()
lines = ['%s  %s' % (sha(open(p, 'rb').read()), os.path.basename(p)) for p in (out, check)]
with zipfile.ZipFile(out) as z:   # as they lie in the game folder once unpacked
    lines += ['%s  %s' % (sha(z.read(n)), n) for n in sorted(names)]
sums = os.path.join(out_dir, 'SHA256SUMS.txt')
with open(sums, 'w', newline='\n') as f:
    f.write('\n'.join(lines) + '\n')
print('%s: %d files (%s ...)' % (out, len(names), ', '.join(names[:4])))
print('%s: the checker, a download of its own' % check)
print('%s: SHA-256 of the zip, the checker and the files in the zip - for the release notes, with the commit:' % sums)
print('commit %s (%s UTC)' % (full, when.strftime('%Y-%m-%d %H:%M')))
# what the same bytes need besides the commit: the same compiler (llvm-mingw's clang) and zlib (the zip's deflate)
cc = subprocess.run([env['CC'], '--version'], capture_output=True, text=True).stdout.splitlines()[0]
print('built with %s; zip by Python %s, zlib %s' % (cc, sys.version.split()[0], zlib.ZLIB_RUNTIME_VERSION))
print('\n'.join(lines[:2]))
