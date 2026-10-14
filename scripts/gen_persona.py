"""Writes config/aimod_ai/_persona.ini - an empty personality with every key explained - from the DLL's sources:
the situations (persona.c k_lines: who hears them, how often, their fallbacks), their descriptions (texts_prompt.h
sit_<key>), the format and the expressions (texts.c k_format_help, k_expr_help). Run it after changing one of them:
venv/Scripts/python.exe scripts/gen_persona.py"""
import os
import re

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def src(name):
    return open(os.path.join(root, 'dll', name), encoding='latin-1').read()

pers, tpl, texts = src('persona.c'), src('texts_prompt.h'), src('texts.c')
lines = re.findall(r'\{ "([a-z_]+)", (\d+), (\d+), (A_ALL|A_ALLIES|A_NAME) \}', pers)
assert len(lines) >= 40

def c_string(code):   # the text of the C string literals in code, escapes undone
    raw = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', code))
    return re.sub(r'\\(.)', lambda m: '\n' if m.group(1) == 'n' else m.group(1), raw)

def sit(key):
    m = re.search(r'\{ "sit_%s", ((?:\s*"(?:[^"\\]|\\.)*")+)' % key, tpl)
    return c_string(m.group(1)) if m else key

def help_text(name):
    a = texts.index('static const char %s[] =' % name)
    return [ln for ln in c_string(texts[a:texts.index(';\n', a)]).split('\n') if ln]

aud = {'A_ALL': 'everyone', 'A_ALLIES': 'its allies', 'A_NAME': '{name} only'}
fb = re.search(r'static int line_fallback\(int k\) \{(.*?)\n\}', pers, re.S).group(1)
fallback = {a.lower(): b.lower() for a, b in re.findall(r'case L_(\w+): return L_(\w+);', fb)}

out = []
w = out.append
w('; _persona.ini - a template for a new AI personality (eeaimod). Not a personality itself: files starting with "_"')
w('; are skipped. Copy it to a new name in this folder (e.g. seefahrer.ini), fill it in, and the AIs of the next game')
w('; may get it (picked at random, a matching colour first).')
w(';')
w('; Lines: one string, or a list of them - one is picked at random. Placeholders: {name} the player the line is about,')
w('; {me} this AI, {enemy} a third player, {epoch} this AI\'s age, {tribute} what a capitulation pays ("200 gold, 800')
w('; iron"). A situation without a line stays silent - unless it must be said (war, peace, offers, capitulation,')
w('; victory): then the line comes from a more general situation (noted below) or from computer.ini.')
w('; [lines] is English (the default); [lines:de] replaces it situation by situation in a German game, [lines:fr],')
w('; [lines:es] ... in other languages. The same for name:de.')
w('; With the AI voice ([llm] in aimod.ini) the model says the lines in its own words, from character and the traits;')
w('; the lines are its examples of how this AI talks.')
w(';')
w('; A line may carry a condition: "Go away, {name}." if opinion < -50 - the lines whose condition holds are picked')
w('; from, those without one only when none does:')
w(';   greeting = [')
w(';       "Welcome back, {name}." if allied,')
w(';       "Go away, {name}." if opinion < -50,')
w(';       "Greetings, {name}.",')
w(';   ]')
for ln in help_text('k_format_help') + help_text('k_expr_help'):
    w(ln)
w('')
w('[personality]')
w('; the name in the game (the player name in the scores and the chat); name:de for a German game')
w('name = ""')
w(';name:de = ""')
w('; favoured player colours on random maps, the first free one wins: blue, red, yellow, magenta, orange, green, cyan,')
w('; darkred, darkgrey, purple, grey, silver, lightyellow, lime, lightblue, pink (or German names, or "0".."16")')
w(';color = ["blue", "cyan"]')
w('; traits 0-100 (50 when left out). aggression: betrays, refuses peace; loyalty: keeps its alliances;')
w('; pride: how long it holds a grudge and refuses peace after a declaration of war (pride x 3 s), how long it fights')
w('; on before it capitulates; greed: what a tribute is worth to it; forgiveness: how fast a grudge fades;')
w('; caution: capitulates earlier, pays more; talk: how often it says the lines that are not always said')
for t in ['aggression', 'loyalty', 'pride', 'greed', 'forgiveness', 'caution', 'talk']:
    w('%s = 50' % t)
w('; for the AI voice: who this is, in a few words (the model plays it)')
w('character = ""')
w('')
w('[style]')
w('; EXPERIMENTAL - only with ai_style=1 in aimod.ini (default off: every AI plays the same, tested way). The same')
w('; keys as in aimod.ini; they replace the difficulty\'s values for this AI. Uncomment what it should change.')
for k in ['military_start_share', 'military_income_share', 'military_income_share_farms', 'epoch_min', 'epoch_max',
          'first_attack_min', 'first_attack_max', 'attack_interval_min', 'attack_interval_max', 'attack_group_percent',
          'attack_on_contact', 'attack_for_resources', 'attack_revenge', 'citizen_share', 'home_guard_share']:
    w(';%s = ' % k)
w('')
w('[vars]')
w('; named strings and expressions for its lines, before those of _system.ini; [vars:de] (or :fr ...) replaces them in')
w('; a game of that language. For example:')
w(';address = opinion > 30 ? "my friend" : opinion < -30 ? "you dog" : "neighbour"')
w('')
w('[lines]')
for key, chance, cooldown, a in lines:
    note = '; %s - heard by %s' % (sit(key), aud[a])
    extra = []
    if int(chance) < 100: extra.append('%s%% of the time at talk=50' % chance)
    if int(cooldown): extra.append('at most every %s s' % cooldown)
    if key in fallback: extra.append('without a line: %s' % fallback[key])
    if extra: note += ' (' + ', '.join(extra) + ')'
    w(note)
    w('%s = []' % key)
w('')
w('; the same keys in German (or [lines:fr] ...): only those that should differ from [lines]')
w(';[lines:de]')
w(';greeting = ["..."]')
path = os.path.join(root, 'config', 'aimod_ai', '_persona.ini')
open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
print(path, len(lines), 'situations')
