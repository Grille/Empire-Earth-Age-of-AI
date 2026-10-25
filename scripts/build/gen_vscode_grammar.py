"""Writes extensions/eeaimod-texts/syntaxes/eeai.tmLanguage.json - the highlighting of the aimod_ai\\*.eeai files
(personalities, mod/computer.eeai, mod/prompt.eeai, mod/system.eeai; the format: dll/script/expr.c THE TEXT FILES) and of the setup scripts
(tests/scenarios/*.eeai, tests/probes/*.eeai: docs/scripts.md - statements, functions, the commands) - with the names the DLL knows,
read from its sources: the game's values (lang/player.cs, names.cs, game.cs), the epochs and functions (expr.c), the situations,
traits and decisions (persona.c), the system and prompt keys (texts.c, texts_prompt.h), the style keys (settings.c). A key the DLL
doesn't know in its record shows as an error. Run it after changing one of them, then scripts/build/pack_vscode.py:
local/venv/Scripts/python.exe scripts/build/gen_vscode_grammar.py"""
import json
import glob
import os
import re
import lang_read

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
def src(name):
    return open(glob.glob(os.path.join(root, 'dll', '**', name), recursive=True)[0], encoding='latin-1').read()   # dll/<folder>/<name>

expr, persona, texts, tpl, settings = (src(n) for n in ('expr.c', 'persona.c', 'texts.c', 'texts_prompt.h', 'settings.c'))
expr += src('constants.c')   # the game's constants (epoch., resource. ...) in the texts
persona += src('players.c')   # the player colours (game/players.c)
lang = lang_read.read_all()   # the players' values and relations, the texts' own names, the game's members (lang/)
members = [(m.name, lang_read.xm_kind(m)) for m in lang['player'].members]
globals_ = [m.name for m in lang['names'].members]
member_names = [n for n, k in members if k not in ('XM_REL', 'XM_FILTER', 'XM_DO')]
relations = [n for n, k in members if k in ('XM_REL', 'XM_FILTER', 'XM_DO')]   # called: A.kills(B), A.count(kind.towers), p.win()
game_members = [m.name for m in lang['game'].members]   # game.time ...
# without a player: the globals (opinion, at_war, allied among them: mine with them), my epoch and traits (me.)
bare = globals_ + ['epoch'] + \
       [n for n, k in members if k == 'XM_PLAYER' and n in ('aggression', 'loyalty', 'pride', 'greed', 'forgiveness', 'caution', 'talk', 'allies', 'enemies')]
values = bare
epochs = re.findall(r'"(\w+)"', re.search(r'k_epoch_const\[15\] = \{(.*?)\};', expr, re.S).group(1))
colors = re.findall(r'\{ "(\w+)", "', re.search(r'k_color_names\[17\]\[3\] = \{(.*?)\};', persona, re.S).group(1))   # the English names
resources = re.findall(r'"(\w+)"', re.search(r'k_resource_const\[5\] = \{(.*?)\};', expr, re.S).group(1)) + ['all']
funcs = sorted(set(re.findall(r'!lstrcmpiA\(name, "(\w+)"\)', expr)) - {'true', 'false', 'null', 'return', 'and', 'or', 'not'})
script_funcs = re.findall(r'\{ "(\w+)", \d \}', re.search(r'x_script_fn\(const char \*name\) \{(.*?)\n\}', expr, re.S).group(1))
commands = re.findall(r'"(\w+)"', open(os.path.join(root, 'dll', 'test', 'cmd_verbs.h'), encoding='utf-8').read())
unit_names = open(os.path.join(root, 'dll', 'game', 'unit_names.h'), encoding='utf-8').read()
unit_ids = re.findall(r'^\| \d+ \| `(\w+)` \|', open(os.path.join(root, 'docs', 'units.md'), encoding='utf-8').read(), re.M)   # the retail data's
kind_ids = re.findall(r'"(\w+)"', re.search(r'k_kind_ident\[UNIT_KINDS\] = \{(.*?)\};', unit_names, re.S).group(1))
situations = re.findall(r'\{ "(\w+)", \d+, \d+, A_\w+ \}', persona)
traits = re.findall(r'"(\w+)"', re.search(r'k_trait_keys\[T_N\] = \{(.*?)\};', persona).group(1))
decisions = re.findall(r'"(\w+)"', re.search(r'k_decide_keys\[D_N\] = \{(.*?)\};', persona).group(1))
sys_keys = re.findall(r'\{ "(\w+)", "', texts[texts.index('k_sys[] = {'):texts.index('#define N_SYS')])
tpl_keys = re.findall(r'\{ "(\w+)", "', tpl[tpl.index('k_tpl[] = {'):])
style_keys = re.findall(r'\{ "(\w+)", offsetof', settings)
assert len(colors) == 17, colors
assert len(script_funcs) >= 6 and len(commands) > 50 and len(unit_ids) > 500, (script_funcs, len(commands), len(unit_ids))
assert len(values) > 15 and len(relations) > 12 and len(epochs) == 15 and 'chance' in funcs and len(situations) >= 40 and len(traits) == 7
assert len(sys_keys) > 20 and len(tpl_keys) > 100 and len(style_keys) > 10, (len(sys_keys), len(tpl_keys), len(style_keys))

def words(ws):
    return r'\b(?:' + '|'.join(sorted(ws, key=len, reverse=True)) + r')\b'

player = r'\b(?:me|them|enemy|lord)(?:\.lord)*'   # a player: them.lord.lord ...
expression = [
    {'include': '#string_verbatim'},   # @"a\path": before the plain one
    {'include': '#string'},   # strings in an expression, also inside {...}: their own {placeholders} nest
    {'include': '#string_single'},
    {'name': 'support.constant.epoch.eeaimod', 'match': r'\bepoch\.(?:' + '|'.join(epochs) + r')\b(?!\.)'},
    {'name': 'invalid.illegal.unknown-epoch.eeaimod', 'match': r'\bepoch\.[A-Za-z_][\w.]*'},
    {'name': 'support.constant.color.eeaimod', 'match': r'\bcolor\.(?:' + '|'.join(colors) + r')\b(?!\.)'},
    {'name': 'invalid.illegal.unknown-color.eeaimod', 'match': r'\bcolor\.[A-Za-z_][\w.]*'},
    {'name': 'support.constant.resource.eeaimod', 'match': r'\bresource\.(?:' + '|'.join(resources) + r')\b(?!\.)'},
    {'name': 'invalid.illegal.unknown-resource.eeaimod', 'match': r'\bresource\.[A-Za-z_][\w.]*'},
    {'name': 'support.constant.unit-type.eeaimod', 'match': r'\bunit\.(?:' + '|'.join(unit_ids) + r')\b(?!\.)'},   # docs/units.md
    {'name': 'invalid.illegal.unknown-unit-type.eeaimod', 'match': r'\bunit\.[A-Za-z_][\w.]*'},
    {'name': 'support.constant.unit-kind.eeaimod', 'match': r'\bkind\.(?:' + '|'.join(kind_ids) + r')\b(?!\.)'},   # docs/units.md
    {'name': 'invalid.illegal.unknown-unit-kind.eeaimod', 'match': r'\bkind\.[A-Za-z_][\w.]*'},
    {'name': 'keyword.control.eeaimod', 'match': r'\b(?:return|if|else|while|for|in|break|continue|let)\b'},
    {'name': 'storage.type.function.arrow.eeaimod', 'match': r'=>'},
    {'name': 'support.function.script.eeaimod', 'match': words(script_funcs) + r'(?=\s*\()'},   # scripts: wait, at, every ...
    {'name': 'support.function.command.eeaimod', 'match': words(commands) + r'(?=\s*\()'},   # scripts: the pipe's commands
    {'name': 'constant.language.script.eeaimod', 'match': r'\b(?:stop|time)\b(?![.\w(])'},
    {'name': 'support.function.eeaimod', 'match': words(funcs) + r'(?=\s*\()'},
    {'name': 'constant.language.boolean.eeaimod', 'match': r'\b(?:true|false)\b'},
    {'name': 'constant.language.null.eeaimod', 'match': r'\bnull\b'},
    {'name': 'keyword.control.logical.eeaimod', 'match': r'\b(?:and|or|not)\b'},
    # me.kills(them): the player, the relation - also a function's parameter that is a player: by.kills(of)
    {'match': r'(?:(' + player + r'|\b[A-Za-z_]\w*)(\.))?(' + words(relations) + r')(?=\s*\()',
     'captures': {'1': {'name': 'variable.language.game-value.eeaimod'}, '2': {'name': 'punctuation.accessor.eeaimod'},
                  '3': {'name': 'support.function.relation.eeaimod'}}},
    {'name': 'variable.language.game-value.eeaimod', 'match': r'\bgame\.' + words(game_members) + r'|\bgame\b(?![.\w(])'},   # the game object
    {'name': 'invalid.illegal.unknown-member.eeaimod', 'match': r'\bgame\.[A-Za-z_]\w*'},
    {'name': 'variable.language.game-value.eeaimod', 'match': player + r'\.' + words(member_names) + r'(?![.\w(])'},
    {'name': 'invalid.illegal.unknown-member.eeaimod', 'match': player + r'\.[A-Za-z_][\w.]*'},
    {'name': 'variable.language.game-value.eeaimod', 'match': words(values) + r'(?!\.)'},
    {'name': 'constant.numeric.eeaimod', 'match': r'(?<![\w.])(?:\d+(?:\.\d+)?|\.\d+)(?:%|s\b|min\b)?'},   # 70% = 0.7, 20s, 19.5min
    {'name': 'invalid.illegal.operator.eeaimod', 'match': r'&&?|\|\|?|!(?!=)|%'},   # and, or, not - only the words; % only right after a number
    {'name': 'keyword.operator.eeaimod', 'match': r'\?\?|\.\.|<=|>=|==|!=|[-+*/<>?:]'},   # ..: a for's range
    {'name': 'punctuation.separator.comma.eeaimod', 'match': ','},
    {'name': 'punctuation.parenthesis.eeaimod', 'match': r'[()]'},
    {'name': 'variable.other.eeaimod', 'match': r'[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*'},
]
value = [{'include': '#comment'}, {'include': '#string_verbatim'}, {'include': '#string'}, {'include': '#list'}, {'include': '#group'}, {'include': '#block'}] + expression
# a statement (a script's, a function's block): let name = value, name = value, name += value ...
assignment = {
    'begin': r'^\s*(?:(let)\s+)?([A-Za-z_]\w*)\s*([-+*/]?=)(?![=>])',
    'beginCaptures': {'1': {'name': 'keyword.control.eeaimod'}, '2': {'name': 'variable.other.assignment.eeaimod'},
                      '3': {'name': 'keyword.operator.assignment.eeaimod'}},
    'end': r'$',
    'patterns': [{'include': '#value'}],
}

def record(names, keys, key_scope):
    """names = { up to its }: its known keys, anything else an error"""
    lines = [{'include': '#comment'}]
    if keys:
        lines.append(key_line(r'(?:' + '|'.join(sorted(keys, key=len, reverse=True)) + r')', key_scope))
    lines.append(key_line(r'[\w:.-]+', 'invalid.illegal.unknown-key.eeaimod'))
    return {
        'begin': r'^\s*(' + names + r')\s*(=)\s*(\{)\s*(?=$|#)',
        'beginCaptures': {'1': {'name': 'entity.name.section.eeaimod' if keys else 'invalid.illegal.unknown-record.eeaimod'},
                          '2': {'name': 'keyword.operator.assignment.eeaimod'},
                          '3': {'name': 'punctuation.definition.record.begin.eeaimod'}},
        'end': r'^\s*(\})',
        'endCaptures': {'1': {'name': 'punctuation.definition.record.end.eeaimod'}},
        'patterns': lines,
    }

# a name of the file's own with parameters: name(a, b) = ...
function_def = {
    'begin': r'^\s*([A-Za-z_]\w*)\s*(\()([^)]*)(\))\s*(=)(?![=>])',
    'beginCaptures': {'1': {'name': 'entity.name.function.eeaimod'}, '2': {'name': 'punctuation.definition.parameters.begin.eeaimod'},
                      '3': {'name': 'variable.parameter.eeaimod'}, '4': {'name': 'punctuation.definition.parameters.end.eeaimod'},
                      '5': {'name': 'keyword.operator.assignment.eeaimod'}},
    'end': r'$',
    'patterns': [{'include': '#value'}],
}

def key_line(key, scope):
    return {
        'begin': r'^\s*(' + key + r')\s*(=)',
        'beginCaptures': {'1': {'name': scope}, '2': {'name': 'keyword.operator.assignment.eeaimod'}},
        'end': r'$',
        'patterns': [{'include': '#value'}],
    }

grammar = {
    '$schema': 'https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json',
    'name': 'eeaimod texts',
    'scopeName': 'source.eeai',
    'comment': 'generated by scripts/build/gen_vscode_grammar.py - edit that, not this',
    'patterns': [
        {'include': '#comment'},
        {'match': r'^\s*(include|register_personality)(?=\s*\()', 'captures': {'1': {'name': 'keyword.control.import.eeaimod'}}},   # include("pack_base.eeai"), register_personality()
        record('personality', ['name', 'civilization', 'character', 'color', 'colour', 'farbe'] + traits, 'support.type.property-name.eeaimod'),
        record('style', style_keys, 'support.type.property-name.eeaimod'),
        record('lines', situations, 'support.type.property-name.situation.eeaimod'),
        record('decide', decisions, 'support.type.property-name.eeaimod'),
        record('system', sys_keys, 'support.type.property-name.eeaimod'),
        record('prompt', tpl_keys, 'support.type.property-name.eeaimod'),
        record(r'[A-Za-z_]\w*', [], ''),
        {'name': 'invalid.illegal.section.eeaimod', 'match': r'^\[[^\]]*\]'},   # the old [sections]
        function_def,
        assignment,   # a name of the file's own (vars), a script's statements
        {'include': '#value'},
    ],
    'repository': {
        'comment': {'patterns': [{'name': 'comment.line.eeaimod', 'match': r'//.*$'},
                                 {'name': 'invalid.illegal.comment.eeaimod', 'match': r'[;#].*$'}]},   # comments start with // only (the game's style)
        'value': {'patterns': value},
        # "text {placeholder or expression} \n"
        'string': {
            'begin': '"',
            'beginCaptures': {'0': {'name': 'punctuation.definition.string.begin.eeaimod'}},
            'end': r'"|$',
            'endCaptures': {'0': {'name': 'punctuation.definition.string.end.eeaimod'}},
            'name': 'string.quoted.double.eeaimod',
            'patterns': [
                {'name': 'constant.character.escape.eeaimod', 'match': r'''\\["'\\n{}]'''},   # as the files' reader: \" \' \\ \n
                {'name': 'invalid.illegal.unknown-escape.eeaimod', 'match': r'\\.'},
                {'include': '#placeholder'},
            ],
        },
        'placeholder': {
            'begin': r'\{',
            'beginCaptures': {'0': {'name': 'punctuation.definition.template-expression.begin.eeaimod'}},
            'end': r'\}',
            'endCaptures': {'0': {'name': 'punctuation.definition.template-expression.end.eeaimod'}},
            'contentName': 'meta.embedded.expression.eeaimod',
            # {x:%} {x:.1%} {x:.2}: the format, after a ':' right before the }
            'patterns': [{'include': '#placeholder'},
                         {'name': 'constant.other.placeholder-format.eeaimod', 'match': r':\s*(?:\.[0-6])?%?\s*(?=\})'}] + expression,
        },
        # @"folder\file": no escapes, no placeholders, "" a quote
        'string_verbatim': {
            'begin': '@"',
            'beginCaptures': {'0': {'name': 'punctuation.definition.string.begin.eeaimod'}},
            'end': r'"(?!")|$',
            'endCaptures': {'0': {'name': 'punctuation.definition.string.end.eeaimod'}},
            'name': 'string.quoted.double.verbatim.eeaimod',
            'patterns': [{'name': 'constant.character.escape.eeaimod', 'match': '""'}],
        },
        'string_single': {
            'begin': "'",
            'beginCaptures': {'0': {'name': 'punctuation.definition.string.begin.eeaimod'}},
            'end': r"'|$",
            'endCaptures': {'0': {'name': 'punctuation.definition.string.end.eeaimod'}},
            'name': 'string.quoted.single.eeaimod',
            'patterns': [
                {'name': 'constant.character.escape.eeaimod', 'match': r'''\\["'\\n{}]'''},
                {'name': 'invalid.illegal.unknown-escape.eeaimod', 'match': r'\\.'},
                {'include': '#placeholder'},
            ],
        },
        # a function's statements: (i) => { ... }
        'block': {
            'begin': r'\{',
            'beginCaptures': {'0': {'name': 'punctuation.definition.block.begin.eeaimod'}},
            'end': r'\}',
            'endCaptures': {'0': {'name': 'punctuation.definition.block.end.eeaimod'}},
            'patterns': [assignment, {'include': '#value'}],
        },
        # [ "a", allied ? "b", ] and ( ... ) - over several lines
        'list': {
            'begin': r'\[',
            'beginCaptures': {'0': {'name': 'punctuation.definition.list.begin.eeaimod'}},
            'end': r'\]',
            'endCaptures': {'0': {'name': 'punctuation.definition.list.end.eeaimod'}},
            'patterns': [{'include': '#value'}],
        },
        'group': {
            'begin': r'\(',
            'beginCaptures': {'0': {'name': 'punctuation.parenthesis.eeaimod'}},
            'end': r'\)',
            'endCaptures': {'0': {'name': 'punctuation.parenthesis.eeaimod'}},
            'patterns': [{'include': '#value'}],
        },
    },
}
out = os.path.join(root, 'extensions', 'eeaimod-texts', 'syntaxes', 'eeai.tmLanguage.json')
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, 'w', encoding='utf-8', newline='\n') as f:
    json.dump(grammar, f, indent=2)
    f.write('\n')
print('%s: %d values, %d epochs, %d functions, %d situations, %d system keys, %d prompt keys, %d style keys' % (
    out, len(values), len(epochs), len(funcs), len(situations), len(sys_keys), len(tpl_keys), len(style_keys)))
