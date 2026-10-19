"""Writes vscode/eeaimod-texts/syntaxes/eeai.tmLanguage.json - the highlighting of the aimod_ai\\*.eeai files
(personalities, _system.eeai, _template.eeai; the format: dll/expr.c THE TEXT FILES) - with the names the DLL knows,
read from its sources: the game's values (diplo.c k_xmembers, k_xglobals), the epochs and functions (expr.c), the situations,
traits and decisions (persona.c), the system and prompt keys (texts.c, texts_prompt.h), the style keys (settings.c). A key the DLL
doesn't know in its section shows as an error. Run it after changing one of them, then scripts/pack_vscode.py:
venv/Scripts/python.exe scripts/gen_vscode_grammar.py"""
import json
import os
import re

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def src(name):
    return open(os.path.join(root, 'dll', name), encoding='latin-1').read()

diplo, expr, persona, texts, tpl, settings = (src(n) for n in ('diplo.c', 'expr.c', 'persona.c', 'texts.c', 'texts_prompt.h', 'settings.c'))
def xtable(name):   # diplo.c k_xmembers / k_xglobals: (name, kind)
    body = re.search(r'static const struct xmember %s\[\] = \{(.*?)\n\};' % name, diplo, re.S).group(1)
    return re.findall(r'\{ "(\w+)", (XM_\w+),', body)
members = xtable('k_xmembers')
globals_ = [n for n, k in xtable('k_xglobals')]
member_names = [n for n, k in members if k != 'XM_REL']
relations = [n for n, k in members if k == 'XM_REL']
# without a player: the globals (opinion, at_war, allied among them: mine with them), my epoch and traits (me.)
bare = globals_ + ['epoch'] + \
       [n for n, k in members if k == 'XM_PLAYER' and n in ('aggression', 'loyalty', 'pride', 'greed', 'forgiveness', 'caution', 'talk', 'allies', 'enemies')]
values = bare
epochs = re.findall(r'"(\w+)"', re.search(r'k_epoch_const\[15\] = \{(.*?)\};', expr, re.S).group(1))
colors = re.findall(r'\{ "(\w+)", "', re.search(r'k_color_names\[17\]\[3\] = \{(.*?)\};', persona, re.S).group(1))   # the English names
units = re.findall(r'"(\w+)"', re.search(r'k_units_const\[6\] = \{(.*?)\};', expr, re.S).group(1))
funcs = sorted(set(re.findall(r'!lstrcmpiA\(name, "(\w+)"\)', expr)) - {'true', 'false'})
situations = re.findall(r'\{ "(\w+)", \d+, \d+, A_\w+ \}', persona)
traits = re.findall(r'"(\w+)"', re.search(r'k_trait_keys\[T_N\] = \{(.*?)\};', persona).group(1))
decisions = re.findall(r'"(\w+)"', re.search(r'k_decide_keys\[D_N\] = \{(.*?)\};', persona).group(1))
sys_keys = re.findall(r'^    \{ "(\w+)", "', texts[texts.index('k_sys[] = {'):texts.index('#define N_SYS')], re.M)
tpl_keys = re.findall(r'^    \{ "(\w+)", ', tpl[tpl.index('k_tpl[] = {'):], re.M)   # [prompt] only, not the built-in [vars]
style_keys = re.findall(r'\{ "(\w+)", offsetof', settings)
assert len(colors) == 17, colors
assert len(values) > 15 and len(relations) > 12 and len(epochs) == 15 and 'chance' in funcs and len(situations) >= 40 and len(traits) == 7
assert len(sys_keys) > 20 and len(tpl_keys) > 100 and len(style_keys) > 10, (len(sys_keys), len(tpl_keys), len(style_keys))

def words(ws):
    return r'\b(?:' + '|'.join(sorted(ws, key=len, reverse=True)) + r')\b'

player = r'\b(?:me|them|enemy|lord)(?:\.lord)*'   # a player: them.lord.lord ...
expression = [
    {'include': '#string'},   # strings in an expression, also inside {...}: their own {placeholders} nest
    {'include': '#string_single'},
    {'name': 'support.constant.epoch.eeaimod', 'match': r'\bepoch\.(?:' + '|'.join(epochs) + r')\b(?!\.)'},
    {'name': 'invalid.illegal.unknown-epoch.eeaimod', 'match': r'\bepoch\.[A-Za-z_][\w.]*'},
    {'name': 'support.constant.color.eeaimod', 'match': r'\bcolor\.(?:' + '|'.join(colors) + r')\b(?!\.)'},
    {'name': 'invalid.illegal.unknown-color.eeaimod', 'match': r'\bcolor\.[A-Za-z_][\w.]*'},
    {'name': 'support.constant.units.eeaimod', 'match': r'\bunits\.(?:' + '|'.join(units) + r')\b(?!\.)'},
    {'name': 'invalid.illegal.unknown-units.eeaimod', 'match': r'\bunits\.[A-Za-z_][\w.]*'},
    {'name': 'support.function.eeaimod', 'match': words(funcs) + r'(?=\s*\()'},
    {'name': 'constant.language.boolean.eeaimod', 'match': r'\b(?:true|false)\b'},
    {'name': 'keyword.control.logical.eeaimod', 'match': r'\b(?:and|or|not)\b'},
    # me.kills(them): the player, the relation - also a function's parameter that is a player: by.kills(of)
    {'match': r'(?:(' + player + r'|\b[A-Za-z_]\w*)(\.))?(' + words(relations) + r')(?=\s*\()',
     'captures': {'1': {'name': 'variable.language.game-value.eeaimod'}, '2': {'name': 'punctuation.accessor.eeaimod'},
                  '3': {'name': 'support.function.relation.eeaimod'}}},
    {'name': 'variable.language.game-value.eeaimod', 'match': player + r'\.' + words(member_names) + r'(?![.\w(])'},
    {'name': 'invalid.illegal.unknown-member.eeaimod', 'match': player + r'\.[A-Za-z_][\w.]*'},
    {'name': 'variable.language.game-value.eeaimod', 'match': words(values) + r'(?!\.)'},
    {'name': 'constant.numeric.eeaimod', 'match': r'(?<![\w.])(?:\d+(?:\.\d+)?|\.\d+)%?'},   # 70% = 0.7
    {'name': 'invalid.illegal.operator.eeaimod', 'match': r'&&?|\|\|?|!(?!=)|%'},   # and, or, not - only the words; % only right after a number
    {'name': 'keyword.operator.eeaimod', 'match': r'\?\?|<=|>=|==|!=|[-+*/<>?:]'},
    {'name': 'punctuation.separator.comma.eeaimod', 'match': ','},
    {'name': 'punctuation.parenthesis.eeaimod', 'match': r'[()]'},
    {'name': 'variable.other.eeaimod', 'match': r'[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*'},
]
value = [{'include': '#comment'}, {'include': '#string'}, {'include': '#list'}, {'include': '#group'}] + expression

def section(names, keys, key_scope, functions=False):
    """[names] / [names:<language>] up to the next section: its known keys, anything else an error; functions: also
    name(a, b) = ... (a function of [vars])"""
    lines = [{'include': '#comment'}]
    if functions:
        lines.append({
            'begin': r'^\s*([A-Za-z_]\w*)\s*(\()([^)]*)(\))\s*(=)',
            'beginCaptures': {'1': {'name': 'entity.name.function.eeaimod'}, '2': {'name': 'punctuation.definition.parameters.begin.eeaimod'},
                              '3': {'name': 'variable.parameter.eeaimod'}, '4': {'name': 'punctuation.definition.parameters.end.eeaimod'},
                              '5': {'name': 'keyword.operator.assignment.eeaimod'}},
            'end': r'$',
            'patterns': [{'include': '#value'}],
        })
    if keys is not None:
        lines.append(key_line(r'(?:' + '|'.join(sorted(keys, key=len, reverse=True)) + r')(?::[\w-]+)?', key_scope))
    lines.append(key_line(r'[\w:.-]+', 'invalid.illegal.unknown-key.eeaimod' if keys is not None else key_scope))
    return {
        'begin': r'^\s*(\[)(' + names + r')(?:(:)([^\]]*))?(\])',
        'beginCaptures': {'1': {'name': 'punctuation.definition.section.eeaimod'}, '2': {'name': 'entity.name.section.eeaimod'},
                          '3': {'name': 'punctuation.separator.eeaimod'}, '4': {'name': 'constant.language.language-code.eeaimod'},
                          '5': {'name': 'punctuation.definition.section.eeaimod'}},
        'end': r'(?=^\s*\[)',
        'patterns': lines,
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
    'comment': 'generated by scripts/gen_vscode_grammar.py - edit that, not this',
    'patterns': [
        {'include': '#comment'},
        key_line('include', 'keyword.control.import.eeaimod'),   # before the first section: include = "_shared.eeai"
        section('personality', ['name', 'civilization', 'character', 'color', 'colour', 'farbe'] + traits, 'support.type.property-name.eeaimod'),
        section('style', style_keys, 'support.type.property-name.eeaimod'),
        section('vars', None, 'entity.name.variable.eeaimod', functions=True),
        section('lines', situations, 'support.type.property-name.situation.eeaimod'),
        section('decide', decisions, 'support.type.property-name.eeaimod'),
        section('system', sys_keys, 'support.type.property-name.eeaimod'),
        section('prompt', tpl_keys, 'support.type.property-name.eeaimod'),
        section(r'[^\]:]+', [], ''),
    ],
    'repository': {
        'comment': {'name': 'comment.line.eeaimod', 'match': r'[;#].*$'},
        'value': {'patterns': value},
        # "text {placeholder or expression} \n"
        'string': {
            'begin': '"',
            'beginCaptures': {'0': {'name': 'punctuation.definition.string.begin.eeaimod'}},
            'end': r'"|$',
            'endCaptures': {'0': {'name': 'punctuation.definition.string.end.eeaimod'}},
            'name': 'string.quoted.double.eeaimod',
            'patterns': [
                {'name': 'constant.character.escape.eeaimod', 'match': r'''\\["'\\n]'''},   # as the files' reader: \" \' \\ \n
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
        'string_single': {
            'begin': "'",
            'beginCaptures': {'0': {'name': 'punctuation.definition.string.begin.eeaimod'}},
            'end': r"'|$",
            'endCaptures': {'0': {'name': 'punctuation.definition.string.end.eeaimod'}},
            'name': 'string.quoted.single.eeaimod',
            'patterns': [
                {'name': 'constant.character.escape.eeaimod', 'match': r'''\\["'\\n]'''},
                {'name': 'invalid.illegal.unknown-escape.eeaimod', 'match': r'\\.'},
                {'include': '#placeholder'},
            ],
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
out = os.path.join(root, 'vscode', 'eeaimod-texts', 'syntaxes', 'eeai.tmLanguage.json')
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, 'w', encoding='utf-8', newline='\n') as f:
    json.dump(grammar, f, indent=2)
    f.write('\n')
print('%s: %d values, %d epochs, %d functions, %d situations, %d system keys, %d prompt keys, %d style keys' % (
    out, len(values), len(epochs), len(funcs), len(situations), len(sys_keys), len(tpl_keys), len(style_keys)))
