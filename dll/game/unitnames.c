// OBJECT TYPE IDENTIFIERS (docs/units.md): every unit and building type has its internal name from the game data,
// the same in every language; lowercase with '_' for the rest it is the identifier - b_guard_tower_bronze for 462. Read
// from the data the game has loaded (its type table G_TYPES: the name behind +0x1C, the family at +0x4C4), so a data
// mod's own types and families count too - once the game has loaded it (before its menu; the text files are read
// earlier: unit.<identifier> names in them are checked then - unit_name_later). The kinds are the engine's families,
// by name (unit_names.h k_kind_ident, from the game data's converter log); each kind's types are listed here once.
// The pipe and setup commands that take a type take its identifier as well (type_args, control.c execute); the
// replies name the type next to its number.
#include "unit_names.h"
#include "unit_ids.h"   // the types the AI code names: UT(B_FARM)
internal int unit_type_named(const char *name, int n);

internal char g_unit_ident[G_TYPES_N][64];      // "" no such type
internal unsigned short g_kind_first[UNIT_KINDS + 1], g_kind_types[G_TYPES_N];   // a kind's types: g_kind_types[first[k]..first[k + 1])
internal int g_unit_types_n;                    // the types read; 0: the game hasn't loaded its data yet
internal struct { char name[48], file[48]; int line; } g_unit_later[32];   // unit.<identifier> in the files, before
internal int g_unit_nlater;                                                 // the data was loaded: checked then
internal void unit_types_check_later(void);
// the type table into the identifiers and kinds - once the game has loaded its data (0 before: fewer than 100 types)
internal int unit_types_load(void) {
    if (g_unit_types_n) return 1;
    int n = 0;
    for (int t = 0; t < G_TYPES_N; t++) n += type_data(t) != NULL;
    if (n < 100) return 0;
    unsigned short cnt[UNIT_KINDS] = { 0 };
    static unsigned char kind[G_TYPES_N];
    for (int t = 0; t < G_TYPES_N; t++) {
        uint8_t *td = type_data(t);
        const char *s = td ? FIELD(td, OF(TD, 0x1C), const char *) : NULL;
        int o = 0, gap = 0;
        kind[t] = 255;
        if (!s) continue;
        for (; *s && o < 62; s++) {   // lowercase, every run of other characters one '_', none at either end
            char c = *s >= 'A' && *s <= 'Z' ? (char)(*s + 32) : *s;
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) { if (gap && o) g_unit_ident[t][o++] = '_'; g_unit_ident[t][o++] = c; gap = 0; }
            else gap = 1;
        }
        g_unit_ident[t][o] = 0;
        for (int u = 0; u < t && o; u++)
            if (!strcmp(g_unit_ident[u], g_unit_ident[t])) {   // a data mod's two names alike: the first type keeps it
                x_error("unit.%s names the types %d and %d in the game's data - it stands for %d", g_unit_ident[t], u, t, u);
                g_unit_ident[t][0] = 0;
                break;
            }
        int f = FIELD(td, OF(TD, 0x4C4), int);
        if (f >= 0 && f < UNIT_KINDS && k_kind_ident[f]) { kind[t] = (unsigned char)f; cnt[f]++; }
        else x_error("type %d (unit.%s): its family %d has no kind - in no kind.<kind>", t, g_unit_ident[t], f);
    }
    g_kind_first[0] = 0;
    for (int k = 0; k < UNIT_KINDS; k++) g_kind_first[k + 1] = (unsigned short)(g_kind_first[k] + cnt[k]);
    unsigned short at[UNIT_KINDS];
    memcpy(at, g_kind_first, sizeof at);
    for (int t = 0; t < G_TYPES_N; t++) if (kind[t] != 255) g_kind_types[at[kind[t]]++] = (unsigned short)t;
    g_unit_types_n = n;
    if (g_log) fprintf(g_log, "# units: %d types from the game's data, %d of them in kinds\n", n, g_kind_first[UNIT_KINDS]);
    static const char *const ut_ident[UT_N] = {
#define UT_IDENT(n, s) s,
        UT_TYPES(UT_IDENT)
#undef UT_IDENT
    };
    for (int i = 0; i < UT_N; i++) {   // the types the AI code names (unit_ids.h)
        int t = unit_type_named(ut_ident[i], (int)strlen(ut_ident[i]));
        g_ut[i] = t >= 0 ? t : UT_NONE;
        if (t < 0 && g_log) fprintf(g_log, "# units: %s, a type the AI knows by name, isn't in the game's data - left out\n", ut_ident[i]);
    }
    for (int t = 0; t < G_TYPES_N; t++) {   // what each type is (unit_ids.h, the roles)
        uint8_t *td = type_data(t);
        if (!td || FIELD(td, OF(TD, 0x4B8), int) != t) continue;
        int f = FIELD(td, OF(TD, 0x4C4), int), c = FIELD(td, OF(TD, 0x4B4), int);
        g_ut_role[t] = (unsigned char)((f == KIND_TOWERS ? UTR_TOWER : 0) | (f == KIND_WALLS ? UTR_WALL : 0) |
            (c == CLS_GATE ? UTR_GATE : 0) |
            (c == CLS_LAND && (f == KIND_SIEGE || f == KIND_MACHINES) && FIELD(td, OF(TD, 0x420), float) >= 3.0f ? UTR_SIEGE : 0) |
            (c == CLS_LAND && f == KIND_RAM ? UTR_RAM : 0) | (c == CLS_CITIZEN && f == KIND_CITIZEN ? UTR_CITIZEN : 0) |
            (c == CLS_TRANSPORT ? UTR_TRANSPORT : 0) | (c == CLS_FISHING_BOAT ? UTR_FISHING_BOAT : 0));
    }
    if (g_log) {   // the roles' types by number: a data mod's new ones show here
        static const char *const rn[8] = { "towers", "walls", "gates", "siege", "rams", "citizens", "transports", "fishing boats" };
        for (int r = 0; r < 8; r++) {
            fprintf(g_log, "# units: %s:", rn[r]);
            for (int t = 0; t < G_TYPES_N; t++) if (g_ut_role[t] & (1 << r)) fprintf(g_log, " %d", t);
            fprintf(g_log, "\n");
        }
    }
    unit_types_check_later();
    return 1;
}
internal const char *unit_ident(int t) {   // "?" for no such type
    unit_types_load();
    return t >= 0 && t < G_TYPES_N && g_unit_ident[t][0] ? g_unit_ident[t] : "?";
}
internal int unit_type_named(const char *name, int n) {   // the type of an identifier (n characters), -1: none
    if (!unit_types_load()) return -1;
    for (int t = 0; t < G_TYPES_N; t++)
        if (g_unit_ident[t][0] && !strncmp(g_unit_ident[t], name, n) && !g_unit_ident[t][n]) return t;
    return -1;
}
// a unit.<identifier> in a file read before the game loaded its data: checked when it has (unit_types_load)
internal void unit_name_later(const char *name, const char *file, int line) {
    if (g_unit_nlater == (int)(sizeof g_unit_later / sizeof *g_unit_later)) return;
    lstrcpynA(g_unit_later[g_unit_nlater].name, name, 48);
    lstrcpynA(g_unit_later[g_unit_nlater].file, file, 48);
    g_unit_later[g_unit_nlater++].line = line;
}
internal void unit_types_check_later(void) {
    for (int i = 0; i < g_unit_nlater; i++)
        if (unit_type_named(g_unit_later[i].name, (int)strlen(g_unit_later[i].name)) < 0)
            x_error("%s:%d: an unknown name: unit.%s (no such type in the game's data - docs/units.md)", g_unit_later[i].file,
                    g_unit_later[i].line, g_unit_later[i].name);
    g_unit_nlater = 0;
}

// The commands with a type among their arguments (sub: the first argument, for stats), and where: bit k = argument k
// (counted from 0, sub included); ~1u: every one after the first.
internal const struct { const char *verb, *sub; uint32_t at; } k_type_args[] = {
    { "spawn", NULL, 1 << 1 }, { "spawnline", NULL, 1 << 1 }, { "train", NULL, 1 << 1 }, { "objs", NULL, 1 << 1 },
    { "found", NULL, 1 << 1 }, { "hurt", NULL, 1 << 1 }, { "watchtype", NULL, 1 << 1 }, { "army", NULL, 1 << 1 },
    { "seltest", NULL, 1 << 0 }, { "plan", NULL, 1 << 2 }, { "tinfo", NULL, ~1u }, { "costs", NULL, ~1u },
    { "stats", "vs", 1 << 2 | 1 << 4 }, { "stats", "fight", 1 << 4 | 1 << 5 }, { "stats", "engage", 1 << 5 },
};
// args with every identifier in a type's place as its number (out); 0: not such a command (args as they are),
// 1: out, -1: a word in a type's place that is no identifier (*bad, *bad_n)
internal int type_args(const char *verb, const char *args, char *out, int cap, const char **bad, int *bad_n) {
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
