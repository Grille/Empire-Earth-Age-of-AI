"""Packs a release: local/work/release/ageofai-<date>-<commit>.zip - unpacked into the game folder (next to
Empire Earth.exe) it is the mod as README "Installing" lays it out:
    ageofai.dll, ageofai_config.exe            beside the game
    ageofai/ageofai.ini, ageofai/personalities/ the mod's folder: the shipped settings and personalities
and beside the zip ageofai_check.exe, the .eeai checker - a download of its own on the release page, never in the zip
(the player 2026-11-02: for modders). The game's settings in a file are ageofai_config.exe's to switch on (Game).
Built from the committed state - a clean `git archive HEAD` built anew (as scripts/build/install.sh), never the
working tree, which may hold another session's unfinished changes: commit first.
    python scripts/build/pack_release.py"""
import datetime
import os
import shutil
import subprocess
import sys
import zipfile

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
git = lambda *a: subprocess.run(['git', *a], cwd=root, check=True, capture_output=True).stdout
rev = git('rev-parse', '--short', 'HEAD').decode().strip()
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
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
    for f in ('ageofai.dll', 'ageofai_config.exe'):
        z.write(os.path.join(build, 'dll', f), f)
    z.write(os.path.join(build, 'config', 'ageofai.ini'), 'ageofai/ageofai.ini')
    z.write(os.path.join(build, 'config', 'texfix.eeai'), 'ageofai/texfix.eeai')
    z.write(os.path.join(build, 'config', 'language.eeai'), 'ageofai/language.eeai')
    z.write(os.path.join(build, 'config', 'setup.eeai'), 'ageofai/setup.eeai')
    pers = os.path.join(build, 'config', 'personalities')
    for d, _, files in os.walk(pers):
        for f in sorted(files):
            p = os.path.join(d, f)
            z.write(p, 'ageofai/personalities/' + os.path.relpath(p, pers).replace(os.sep, '/'))
check = os.path.join(out_dir, 'ageofai_check.exe')
shutil.copy(os.path.join(build, 'dll', 'ageofai_check.exe'), check)
names = zipfile.ZipFile(out).namelist()
print('%s: %d files (%s ...)' % (out, len(names), ', '.join(names[:4])))
print('%s: the checker, a download of its own' % check)
