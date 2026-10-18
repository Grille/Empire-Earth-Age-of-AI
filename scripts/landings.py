"""Landings per run: planned, sailed, ashore (soldiers), called off (beach too strong / boarding failed / other),
told to unload by the game (refused by the transport manager, kept off the shore while boarding, retries refused), and the score line (kills, losses, razed). usage: python scripts/landings.py logs..."""
import re, sys, subprocess, os

here = os.path.dirname(os.path.abspath(__file__))
for f in sys.argv[1:]:
    txt = open(f, encoding='utf-8', errors='replace').read()
    c = lambda pat: len(re.findall(pat, txt))
    ashore = sum(int(m) for m in re.findall(r'# landing: P\d at [\d.]+ min: all (\d+) ashore', txt))
    ashore += sum(int(m) for m in re.findall(r'# landing: P\d at [\d.]+ min: ashore near [\d,]+ - (\d+) landed', txt)) and 0
    out = subprocess.run([sys.executable, os.path.join(here, 'score.py'), f], capture_output=True, text=True).stdout
    sc = ' | '.join(re.sub(r'\s+', ' ', l.strip()) for l in out.splitlines() if re.match(r'\s+P\d ', l))
    sc = re.sub(r'epoch \d+ +workers end +\d+ +max +\d+ +food end +\d+ +last seen +[\d.]+ +', '', sc)
    print(f'{os.path.basename(f)[-40:]:40} planned {c(r"plans a landing on"):2} sailed {c(r"min: sails with"):2} '
          f'ashore {c(r"min: all \d+ ashore"):2} ({ashore:3} soldiers) beach-off {c(r"too strong a beach"):2} '
          f'board-fail {c(r"boarding failed"):2} game-unload {c(r"told to unload at"):3} mgr-refused {c(r"transport manager wanted it"):3} '
          f'kept-off {c(r"kept off the shore"):2} retry-refused {c(r"unload retry at"):2} | {sc}')
