"""The texts of config/aimod_ai/mod/prompt.eeai's prompt = { ... } record, by key - for the generators that describe
the situations (gen_persona.py, gen_eeai_readme.py): the file is the only place the texts are."""
import os

def _string(t, i):   # the string starting at t[i] == '"': its text and the index after it - quotes in {braces} are an expression's
    out, depth, i = [], 0, i + 1
    while True:
        c = t[i]
        if depth == 0 and c == '"':
            return ''.join(out), i + 1
        if depth == 0 and c == '\\':
            out.append('\n' if t[i + 1] == 'n' else t[i + 1]); i += 2; continue
        if c == '{': depth += 1
        elif c == '}': depth -= 1
        out.append(c); i += 1

def prompt_texts(root):
    t = open(os.path.join(root, 'config', 'aimod_ai', 'mod', 'prompt.eeai'), encoding='utf-8').read().replace('\r\n', '\n')
    i = t.index('\nprompt = {\n') + len('\nprompt = {\n')
    texts = {}
    while not t.startswith('}', i):
        line_end = t.index('\n', i)
        s = t[i:line_end].strip()
        if not s or s.startswith('//'):
            i = line_end + 1; continue
        key, rest = s.split(' = ', 1)
        j = t.index(' = ', i) + 3
        parts = []
        if t[j] == '(':   # pieces side by side, one a line
            j += 1
            while True:
                while t[j] in ' \n': j += 1
                if t[j] == ')': j += 1; break
                text, j = _string(t, j); parts.append(text)
        else:
            text, j = _string(t, j); parts.append(text)
        texts[key] = ''.join(parts)
        i = t.index('\n', j) + 1
    return texts
