"""Writes config/aimod_ai/_template.eeai - an empty personality with every key explained - from the DLL's sources:
the situations (persona.c k_lines: who hears them, how often, their fallbacks), their descriptions (mod/prompt.eeai
sit_<key>). Run it after changing one of them:
venv/Scripts/python.exe scripts/gen_persona.py"""
import glob
import os
import re
from eeai_prompt import prompt_texts

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def src(name):
    return open(glob.glob(os.path.join(root, 'dll', '**', name), recursive=True)[0], encoding='latin-1').read()   # dll/<folder>/<name>

pers = src('persona.c')
prompt = prompt_texts(root)
lines = re.findall(r'\{ "([a-z_]+)", (\d+), (\d+), (A_ALL|A_ALLIES|A_NAME) \}', pers)
assert len(lines) >= 40

def sit(key):
    # an expression in it ({lord == them ? ...}) left out: the plain words describe the situation
    return re.sub(r'\{[^{}]*[ ?][^{}]*\}', '', prompt.get('sit_' + key, key))

FORMAT_HELP = [   # the rest is in README.md
    '#',
    '# key = value, a value is an expression: "a string" ({placeholders} and {expressions} in it), ["a list", "of',
    '# strings"] (one at random), allied ? "a condition", a ?? b, numbers ... The keys the mod reads stand in records',
    '# (lines = { ... }, a comma after each entry); a name outside them is the file\'s own, for its values - a record of',
    '# your own too (colors = { dark = ..., light = ... }: colors.dark). The format, the expressions and every',
    '# value of the game (opinion, them.soldiers, epoch.middle, ...): README.md in this folder.',
]

aud = {'A_ALL': 'everyone', 'A_ALLIES': 'its allies', 'A_NAME': '{them} only'}
fb = re.search(r'internal int line_fallback\(int k\) \{(.*?)\n\}', pers, re.S).group(1)
fallback = {a.lower(): b.lower() for a, b in re.findall(r'case L_(\w+): return L_(\w+);', fb)}

out = []
w = out.append
w('# _template.eeai - a template for a new AI personality (eeaimod). Not a personality itself: its')
w('# register_personality() below is commented out. Copy it to a new name in this folder (e.g. seefahrer.eeai), take')
w('# the # from register_personality(), fill it in, and the AIs of the next game may get it (picked at random, a')
w('# matching colour first).')
w('#')
w('# Each situation below gets a line: a string, or a list of them (one at random), with conditions if you like:')
w('#   greeting = [allied ? "Welcome back, {them}.", opinion < -50% ? ["Go away.", "You again?"]] ?? "Greetings, {them}."')
w('# A situation without a line stays silent - unless it must be said (war, peace, offers, capitulation, victory): then')
w('# a more general situation speaks (noted below) or a layer - mod/computer.eeai. The lines are English.')
for ln in FORMAT_HELP:
    w(ln)
w('')
w('# eeaimod\'s base, mod/computer.eeai, and the language model\'s texts, mod/prompt.eeai, are layers of every')
w('# personality without being named: the lines that must not go unsaid, the names this_war, many_dead, big_tribute,')
w('# the decide rules, the prompts - each counts where this file has none of its own. A pack\'s own base comes before')
w('# them: include("pack_base.eeai").')
w('# this file is a personality (an AI may get it) - without it, a file is only included by others')
w('#register_personality()')
w('')
w('personality = {')
w('    # the name in the game (the player name in the scores and the chat) - a list: the first no other player has')
w('    # (all taken: the first with a number, "Khan Temur II"): ["Khan Temur", "Khan Batu"], pick([...]) a random one')
w('    name = "",')
w('    # its civilization\'s name as the game shows it, instead of the one it plays (empty: keep that)')
w('    #civilization = "",')
w('    # favoured player colours on random maps, the first free one wins: blue, red, yellow, magenta, orange, green,')
w('    # cyan, darkred, darkgrey, purple, grey, silver, lightyellow, lime, lightblue, pink (or German names, or a number')
w('    # 1..17: color.red or 2), or one of its own: rgb(128, 64, 0) - 0 to 255 each')
w('    #color = ["blue", "cyan"],')
w('    # traits 0% to 100% (50% when left out). aggression: betrays, refuses peace; loyalty: keeps its alliances (a vassal')
w('    # under 50% rebels against a clearly weaker lord, the sooner the prouder); pride: how long it holds a grudge and')
w('    # refuses peace after a declaration of war (pride x 5 minutes), how long it fights on before it capitulates, how')
w('    # weak a lord it serves; greed: what a tribute is worth to it; forgiveness: how fast a grudge fades;')
w('    # caution: capitulates earlier, pays more; talk: how often it says the lines that are not always said')
for t in ['aggression', 'loyalty', 'pride', 'greed', 'forgiveness', 'caution', 'talk']:
    w('    %s = 50%%,' % t)
w('    # for the AI voice: who this is, in a few words (the model plays it)')
w('    character = "",')
w('}')
w('')
w('style = {')
w('    # EXPERIMENTAL - only with ai_style=1 in aimod.ini (default off: every AI plays the same, tested way). The same')
w('    # keys as in aimod.ini; they replace the difficulty\'s values for this AI. Uncomment what it should change. Each')
w('    # an expression worked out at the game start (like color =): shares and chances with % (home_guard_share = 35%,')
w('    # attack_on_contact = 20%, attack_group_percent = 150%), minutes as numbers - chance(50%) ? 15 : 25 works too.')
for k in ['military_start_share', 'military_income_share', 'military_income_share_farms', 'epoch_min', 'epoch_max',
          'first_attack_min', 'first_attack_max', 'attack_interval_min', 'attack_interval_max', 'attack_group_percent',
          'attack_on_contact', 'attack_for_resources', 'attack_revenge', 'citizen_share', 'home_guard_share']:
    w('    #%s = ,' % k)
w('}')
w('')
w('# names of your own (vars) for its lines, before those of mod/system.eeai - anywhere outside the records. For example:')
w('#address = opinion > 30% ? "my friend" : opinion < -30% ? "you dog" : "neighbour"')
w('')
w('lines = {')
w('    # [] or a list with nothing left: no line (a more general situation or mod/computer.eeai speaks, or nobody)')
for key, chance, cooldown, a in lines:
    note = '    # %s - heard by %s' % (sit(key), aud[a])
    extra = []
    if int(chance) < 100: extra.append('%s%% of the time at talk=50' % chance)
    if int(cooldown): extra.append('at most every %s s' % cooldown)
    if key in fallback: extra.append('without a line: %s' % fallback[key])
    if extra: note += ' (' + ', '.join(extra) + ')'
    w(note)
    w('    %s = [],' % key)
w('}')
w('')
w('# what the language model is told for this AI (aimod.ini [llm]), instead of mod/prompt.eeai\'s text - every key and')
w('# its placeholders are there. For example:')
w('#prompt = {')
w('#    nature = "You speak in short, proud sentences.",')
w('#}')
path = os.path.join(root, 'config', 'aimod_ai', '_template.eeai')
open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
print(path, len(lines), 'situations')
