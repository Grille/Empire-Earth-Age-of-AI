// AI PERSONALITIES: every AI player gets a personality from <game>\aimod_ai\*.ini - a name, traits (0-100) that
// steer its diplomacy, and the lines it says in situations of the game. Included by eeaimod.c after defence.c,
// before diplo.c (which makes the decisions and raises most of the situations).
//
// File format (UTF-8 or ANSI; lines starting with ; or # are comments):
//   [personality]
//   name=Khan Temur            the player's name in the game (replaces "Computer"; empty = keep)
//   aggression=80 loyalty=30 pride=70 greed=40 forgiveness=20 caution=40 talk=60   (one per line)
//   [lines]
//   war_declared=Variant one|Variant two      variants separated by |, or the key repeated
// Placeholders: {name} the player the line is about, {me} the speaker, {enemy} a third player (alliances),
// {epoch} the speaker's age. A situation without lines stays silent (the game's own lines for wonders, flares and
// war remain where the personality has none).
//
// Assignment at the AI's first update: aimod.ini [general] personality_P<n>=<file name without .ini>, otherwise
// random, every personality once before any repeats (autotest: from the map seed, so runs repeat).
// aimod.ini [general] ai_talk = 1 (default) lines on, 0 off; ai_names = 1 (default) the personalities' names.
//
// Strings: UWideString takes only char * (the game's ANSI code page), so UTF-8 files are converted on load.
// Randomness: an own generator - the game's rand() is part of the simulation and must not be touched.

enum trait { T_AGGRESSION, T_LOYALTY, T_PRIDE, T_GREED, T_FORGIVENESS, T_CAUTION, T_TALK, T_N };
static const char *const k_trait_keys[T_N] = { "aggression", "loyalty", "pride", "greed", "forgiveness", "caution", "talk" };

// situations: key, chance in % at talk=50 (100 = always, scaled by talk otherwise), cooldown s per AI
enum line_kind {
    L_GREETING, L_WAR_DECLARED, L_BETRAYED, L_PEACE_ACCEPT, L_PEACE_REFUSE, L_ALLIANCE_OFFER, L_OFFER_ACCEPTED,
    L_OFFER_WITHDRAWN, L_BETRAY, L_TRIBUTE_THANKS, L_TRIBUTE_SMALL, L_ATTACK, L_UNDER_ATTACK, L_BUILDING_LOST,
    L_EPOCH_UP, L_ENEMY_AHEAD, L_WINNING, L_LOSING, L_DESPERATE, L_ENEMY_DEFEATED, L_ALLY_DEFEATED, L_DEFEATED,
    L_WONDER_THREAT, L_WONDER_HELP, L_WONDER_NOHELP, L_HELP_LAND, L_HELP_AIR, L_HELP_NAVAL, L_HELP_NONE, L_N
};
static const struct { const char *key; int chance, cooldown; } k_lines[L_N] = {
    { "greeting", 60, 0 },          { "war_declared", 100, 20 },     { "betrayed", 100, 20 },
    { "peace_accept", 100, 0 },     { "peace_refuse", 100, 60 },     { "alliance_offer", 100, 0 },
    { "offer_accepted", 100, 0 },   { "offer_withdrawn", 100, 0 },   { "betray", 100, 0 },
    { "tribute_thanks", 100, 20 },  { "tribute_small", 100, 60 },    { "attack", 40, 300 },
    { "under_attack", 60, 240 },    { "building_lost", 25, 180 },    { "epoch_up", 50, 60 },
    { "enemy_ahead", 40, 600 },     { "winning", 40, 600 },          { "losing", 50, 600 },
    { "desperate", 100, 0 },        { "enemy_defeated", 80, 30 },    { "ally_defeated", 80, 30 },
    { "defeated", 100, 0 },         { "wonder_threat", 100, 60 },    { "wonder_help", 100, 60 },
    { "wonder_nohelp", 100, 60 },   { "help_land", 100, 30 },        { "help_air", 100, 30 },
    { "help_naval", 100, 30 },      { "help_none", 100, 30 },
};

#define MAX_PERSONAS 32
#define MAX_VARIANTS 12
struct persona {
    char file[40], name[48];
    int trait[T_N];
    char *line[L_N][MAX_VARIANTS];
    uint8_t nline[L_N];
};
static struct persona g_personas[MAX_PERSONAS];
static int g_npersonas;
static int g_persona_of[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static int g_persona_assigned[16];
static int g_ai_talk = 1, g_ai_names = 1;
static char g_persona_force[16][40];
static uint32_t g_talk_last[16][L_N], g_talk_any[16];
static uint8_t g_talk_variant[16][L_N];
static unsigned g_talk_lines[16];
static uint32_t g_rnd = 0x9E3779B9u;

static uint32_t rnd32(void) { g_rnd ^= g_rnd << 13; g_rnd ^= g_rnd >> 17; g_rnd ^= g_rnd << 5; return g_rnd; }
static int rnd(int n) { return n > 0 ? (int)(rnd32() % (unsigned)n) : 0; }

// whole file as text in the ANSI code page (UTF-8 converted), malloc'd, or NULL
static char *read_text_acp(const char *path) {
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    DWORD size = GetFileSize(h, NULL), got = 0;
    char *raw = size < (1u << 20) ? malloc(size + 1) : NULL;
    if (raw && (!ReadFile(h, raw, size, &got, NULL) || got != size)) { free(raw); raw = NULL; }
    CloseHandle(h);
    if (!raw) return NULL;
    raw[size] = 0;
    char *src = raw;
    if (size >= 3 && (uint8_t)raw[0] == 0xEF && (uint8_t)raw[1] == 0xBB && (uint8_t)raw[2] == 0xBF) src += 3;
    int wn = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, NULL, 0);
    if (wn <= 0) return raw;   // not UTF-8: already ANSI
    wchar_t *w = malloc((size_t)wn * sizeof *w);
    int an = w ? (MultiByteToWideChar(CP_UTF8, 0, src, -1, w, wn), WideCharToMultiByte(CP_ACP, 0, w, -1, NULL, 0, NULL, NULL)) : 0;
    char *a = an > 0 ? malloc((size_t)an) : NULL;
    if (a) WideCharToMultiByte(CP_ACP, 0, w, -1, a, an, NULL, NULL);
    free(w);
    if (!a) return raw;
    free(raw);
    return a;
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static void persona_load(const char *path, const char *file) {
    if (g_npersonas >= MAX_PERSONAS) return;
    char *text = read_text_acp(path);
    if (!text) return;
    struct persona *p = &g_personas[g_npersonas];
    memset(p, 0, sizeof *p);
    lstrcpynA(p->file, file, sizeof p->file);
    char *dot = strrchr(p->file, '.');
    if (dot) *dot = 0;
    for (int t = 0; t < T_N; t++) p->trait[t] = 50;
    int section = 0;   // 1 personality, 2 lines
    for (char *ln = strtok(text, "\n"); ln; ln = strtok(NULL, "\n")) {
        char *s = trim(ln);
        if (!*s || *s == ';' || *s == '#') continue;
        if (*s == '[') { section = !_strnicmp(s, "[personality]", 13) ? 1 : !_strnicmp(s, "[lines]", 7) ? 2 : 0; continue; }
        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = trim(s), *val = trim(eq + 1);
        if (section == 1) {
            if (!lstrcmpiA(key, "name")) lstrcpynA(p->name, val, sizeof p->name);
            for (int t = 0; t < T_N; t++)
                if (!lstrcmpiA(key, k_trait_keys[t])) { int v = atoi(val); p->trait[t] = v < 0 ? 0 : v > 100 ? 100 : v; }
        } else if (section == 2) {
            for (int k = 0; k < L_N; k++) {
                if (lstrcmpiA(key, k_lines[k].key)) continue;
                for (char *v = val; v && *v && p->nline[k] < MAX_VARIANTS;) {
                    char *bar = strchr(v, '|');
                    if (bar) *bar = 0;
                    char *t = trim(v);
                    if (*t) p->line[k][p->nline[k]++] = _strdup(t);
                    v = bar ? bar + 1 : NULL;
                }
            }
        }
    }
    free(text);
    g_npersonas++;
}

static void personas_load(void) {
    char dir[MAX_PATH], pat[MAX_PATH], path[MAX_PATH];
    GetModuleFileNameA(NULL, dir, sizeof dir);
    char *bs = strrchr(dir, '\\');
    if (bs) bs[1] = 0;
    lstrcatA(dir, "aimod_ai\\");
    wsprintfA(pat, "%s*.ini", dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        wsprintfA(path, "%s%s", dir, fd.cFileName);
        persona_load(path, fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static struct persona *persona_of(void *pl) {
    if (!pl) return NULL;
    int i = g_persona_of[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    return i >= 0 ? &g_personas[i] : NULL;
}
static int trait(void *pl, enum trait t) { struct persona *p = persona_of(pl); return p ? p->trait[t] : 50; }
static int persona_has(void *pl, enum line_kind k) { struct persona *p = persona_of(pl); return p && g_ai_talk && p->nline[k]; }

// once per AI (its first update): pick and apply the personality
static void persona_assign(void *pl) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_persona_assigned[idx]) return;
    g_persona_assigned[idx] = 1;
    if (!g_npersonas) return;
    int pick = -1;
    for (int i = 0; i < g_npersonas && g_persona_force[idx][0]; i++)
        if (!lstrcmpiA(g_personas[i].file, g_persona_force[idx])) pick = i;
    if (pick < 0) {
        int used[MAX_PERSONAS] = { 0 }, minuse = 1 << 30;
        for (int j = 0; j < 16; j++) if (g_persona_of[j] >= 0) used[g_persona_of[j]]++;
        for (int i = 0; i < g_npersonas; i++) if (used[i] < minuse) minuse = used[i];
        int n = 0;
        for (int i = 0; i < g_npersonas; i++) n += used[i] == minuse;
        int r = rnd(n);
        for (int i = 0; i < g_npersonas && pick < 0; i++) if (used[i] == minuse && r-- == 0) pick = i;
    }
    g_persona_of[idx] = pick;
    struct persona *p = &g_personas[pick];
    if (g_ai_names && p->name[0])
        ((void *(__thiscall *)(void *, const char *))*(void **)0x00823224)((uint8_t *)pl + 0x24, p->name);
    if (g_log) {
        fprintf(g_log, "# persona: P%u is %s (%s):", idx, p->file, p->name[0] ? p->name : "-");
        for (int t = 0; t < T_N; t++) fprintf(g_log, " %s %d", k_trait_keys[t], p->trait[t]);
        fprintf(g_log, "\n");
    }
}

static const char *player_name(void *pl) {
    const char *s = pl ? FIELD(pl, 0x28, const char *) : NULL;
    return s ? s : "?";
}

// the speaker's age, localized (strings 30145 + epoch: Vorzeit, Steinzeit, ...)
static void epoch_text(void *pl, char *out, int cap) {
    void *res = *(void **)0x00919770;
    int e = current_epoch(pl);
    lstrcpynA(out, epoch_name(e), cap);
    if (!res || e < 0 || e > 14) return;
    __attribute__((aligned(8))) uint8_t s[32];
    ((void (__thiscall *)(void *))*(void **)0x008231A4)(s);
    ((int (__thiscall *)(void *, int, void *))*(void **)0x0082319C)(res, 30145 + e, s);
    if (FIELD(s, 4, const char *)) lstrcpynA(out, FIELD(s, 4, const char *), cap);
    ((void (__thiscall *)(void *))*(void **)0x00823AEC)(s);
}

// say a line of situation k (other = {name}, third = {enemy}); returns 1 if said
static int persona_say(void *pl, enum line_kind k, void *other, void *third) {
    struct persona *p = persona_of(pl);
    if (!p || !g_ai_talk || !p->nline[k] || (FIELD(pl, 0x467, uint8_t) && k != L_DEFEATED)) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t now = game_time();
    if (g_talk_any[idx] && now - g_talk_any[idx] < 3000 && k_lines[k].chance < 100) return 0;
    if (g_talk_last[idx][k] && now - g_talk_last[idx][k] < (uint32_t)k_lines[k].cooldown * 1000u) return 0;
    if (k_lines[k].chance < 100 && rnd(100) >= k_lines[k].chance * p->trait[T_TALK] / 50) return 0;
    int n = p->nline[k], v = rnd(n);
    if (n > 1 && v == g_talk_variant[idx][k]) v = (v + 1) % n;
    g_talk_variant[idx][k] = (uint8_t)v;
    char buf[256], ep[48] = "";
    int o = 0;
    for (const char *s = p->line[k][v]; *s && o < (int)sizeof buf - 1;) {
        const char *rep = NULL;
        if (*s == '{') {
            if (!strncmp(s, "{name}", 6)) { rep = player_name(other); s += 6; }
            else if (!strncmp(s, "{me}", 4)) { rep = player_name(pl); s += 4; }
            else if (!strncmp(s, "{enemy}", 7)) { rep = player_name(third); s += 7; }
            else if (!strncmp(s, "{epoch}", 7)) { epoch_text(pl, ep, sizeof ep); rep = ep; s += 7; }
        }
        if (rep) while (*rep && o < (int)sizeof buf - 1) buf[o++] = *rep++;
        else buf[o++] = *s++;
    }
    buf[o] = 0;
    g_talk_last[idx][k] = g_talk_any[idx] = now ? now : 1;
    chat_line(pl, buf, g_chat_ms);
    g_talk_lines[idx]++;
    if (g_log) fprintf(g_log, "# talk: P%u at %.1f min (%s): %s\n", idx, now / 60000.0, k_lines[k].key, buf);
    return 1;
}

// an attack group sets out against target's base (the route hook, attack.c)
static void persona_attack(void *pl, void *target) { persona_say(pl, L_ATTACK, target, NULL); }

// The game's own AI lines (EECPPlayerInteractionManager 0x484EB3 thiscall(pim, base id, ms), pim +0x20 = player),
// called at 0x470040 (12500 war), 0x4715F8/0x47166E/0x47280E/0x472B9D (wonders, flares): the personality's line
// where it has one. 12500: diplo.c says the war line itself (with the declaring player's name), so it is dropped
// here when the personality has one.
#define FN_NATIVE_CHAT 0x00484EB3
static char __fastcall hook_native_chat(uint8_t *pim, void *edx, int base, int ms) {
    (void)edx;
    void *pl = pim ? FIELD(pim, 0x20, void *) : NULL;
    int k = base == 12000 ? L_WONDER_THREAT : base == 12100 ? L_WONDER_HELP : base == 12200 ? L_WONDER_NOHELP :
            base == 12300 ? L_HELP_LAND : base == 12330 ? L_HELP_AIR : base == 12360 ? L_HELP_NAVAL :
            base == 12400 ? L_HELP_NONE : base == 12500 ? L_WAR_DECLARED : -1;
    if (k >= 0 && persona_has(pl, (enum line_kind)k)) {
        if (k != L_WAR_DECLARED) persona_say(pl, (enum line_kind)k, NULL, NULL);
        return 1;
    }
    return ((char (__thiscall *)(void *, int, int))FN_NATIVE_CHAT)(pim, base, ms);
}

// cmd: persona [P name]: who is who; with arguments AI P gets personality <name> (file name) from now on
static void cmd_persona(char *out, const char *args) {
    int p = -1;
    char name[40] = "";
    sscanf(args, "%d %39s", &p, name);
    if (p >= 0 && p < 16 && name[0]) {
        for (int i = 0; i < g_npersonas; i++)
            if (!lstrcmpiA(g_personas[i].file, name)) {
                g_persona_of[p] = i;
                if (g_players[p] && g_ai_names && g_personas[i].name[0])
                    ((void *(__thiscall *)(void *, const char *))*(void **)0x00823224)((uint8_t *)g_players[p] + 0x24, g_personas[i].name);
                REPLY("P%d is now %s\n", p, name);
                return;
            }
        REPLY("error: no personality '%s'\n", name);
        return;
    }
    REPLY("%d personalities loaded, talk %d, names %d\n", g_npersonas, g_ai_talk, g_ai_names);
    for (int i = 0; i < 16; i++) {
        struct persona *q = g_persona_of[i] >= 0 ? &g_personas[g_persona_of[i]] : NULL;
        if (!q) continue;
        REPLY("P%d %-12s %-24s", i, q->file, q->name);
        for (int t = 0; t < T_N; t++) REPLY(" %c%d", k_trait_keys[t][0], q->trait[t]);
        REPLY("  lines %u\n", g_talk_lines[i]);
    }
}

static void persona_install(const char *ini, const char *test_ini) {
    g_ai_talk = GetPrivateProfileIntA("general", "ai_talk", 1, ini);
    g_ai_names = GetPrivateProfileIntA("general", "ai_names", 1, ini);
    for (int i = 0; i < 16; i++) {
        char key[24];
        wsprintfA(key, "personality_P%d", i);
        GetPrivateProfileStringA("general", key, "", g_persona_force[i], sizeof g_persona_force[i], ini);
        if (test_ini) GetPrivateProfileStringA("autotest", key, g_persona_force[i], g_persona_force[i], sizeof g_persona_force[i], test_ini);
    }
    if (test_ini) {
        g_ai_talk = GetPrivateProfileIntA("autotest", "ai_talk", g_ai_talk, test_ini);
        g_ai_names = GetPrivateProfileIntA("autotest", "ai_names", g_ai_names, test_ini);
    }
    g_rnd ^= test_ini ? 0x5EED : GetTickCount();
    if (!g_rnd) g_rnd = 1;
    personas_load();
    int ok = 0;
    static const uintptr_t sites[] = { 0x470040, 0x4715F8, 0x47166E, 0x47280E, 0x472B9D };
    for (size_t i = 0; i < sizeof sites / sizeof *sites; i++) ok += patch_call(sites[i], FN_NATIVE_CHAT, (void *)hook_native_chat);
    if (g_log) fprintf(g_log, "# persona: %d personalities, talk %d, names %d, native line hooks %d/5\n", g_npersonas, g_ai_talk, g_ai_names, ok);
}
