// OBJECT TYPE IDENTIFIERS (docs/units.md, generated with dll/unit_names.h by scripts/gen_units.py): every unit and
// building type has its internal name from the game data, the same in every language; lowercase with '_' for the
// rest it is the identifier - b_guard_tower_bronze for 462. The pipe and setup commands that take a type take its
// identifier as well (type_args, control.c execute); the replies name the type next to its number.
#include "unit_names.h"

static const char *unit_ident(int t) {   // "?" for no such type
    return t >= 0 && t < UNIT_TYPES && k_unit_ident[t] ? k_unit_ident[t] : "?";
}
static int unit_type_named(const char *name, int n) {   // the type of an identifier (n characters), -1: none
    for (int t = 0; t < UNIT_TYPES; t++)
        if (k_unit_ident[t] && !strncmp(k_unit_ident[t], name, n) && !k_unit_ident[t][n]) return t;
    return -1;
}

// The commands with a type among their arguments (sub: the first argument, for stats), and where: bit k = argument k
// (counted from 0, sub included); ~1u: every one after the first.
static const struct { const char *verb, *sub; uint32_t at; } k_type_args[] = {
    { "spawn", NULL, 1 << 1 }, { "spawnline", NULL, 1 << 1 }, { "train", NULL, 1 << 1 }, { "objs", NULL, 1 << 1 },
    { "found", NULL, 1 << 1 }, { "hurt", NULL, 1 << 1 }, { "watchtype", NULL, 1 << 1 }, { "army", NULL, 1 << 1 },
    { "seltest", NULL, 1 << 0 }, { "plan", NULL, 1 << 2 }, { "tinfo", NULL, ~1u }, { "costs", NULL, ~1u },
    { "stats", "vs", 1 << 2 | 1 << 4 }, { "stats", "fight", 1 << 4 | 1 << 5 }, { "stats", "engage", 1 << 5 },
};
// args with every identifier in a type's place as its number (out); 0: not such a command (args as they are),
// 1: out, -1: a word in a type's place that is no identifier (*bad, *bad_n)
static int type_args(const char *verb, const char *args, char *out, int cap, const char **bad, int *bad_n) {
    uint32_t at = 0;
    for (size_t i = 0; i < sizeof k_type_args / sizeof *k_type_args; i++) {
        const char *sub = k_type_args[i].sub;
        size_t sn = sub ? strlen(sub) : 0;
        if (!strcmp(verb, k_type_args[i].verb) && (!sub || (!strncmp(args, sub, sn) && (!args[sn] || args[sn] == ' '))))
            at = k_type_args[i].at;
    }
    if (!at) return 0;
    int o = 0;
    out[0] = 0;
    for (int k = 0; *args; k++) {
        while (*args == ' ' || *args == '\t') { if (o < cap - 1) out[o++] = *args; args++; }
        const char *w = args;
        while (*args && *args != ' ' && *args != '\t') args++;
        int n = (int)(args - w);
        if (!n) break;
        int here = k < 32 && (at >> k & 1);
        char num[16];
        if (here && ((*w >= 'a' && *w <= 'z') || *w == '_')) {
            int t = unit_type_named(w, n);
            if (t < 0) { *bad = w; *bad_n = n; return -1; }
            snprintf(num, sizeof num, "%d", t);
            w = num;
            n = (int)strlen(num);
        }
        if (o + n >= cap) n = cap - 1 - o;
        memcpy(out + o, w, n);
        o += n;
    }
    out[o] = 0;
    return 1;
}
