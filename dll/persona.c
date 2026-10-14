// AI PERSONALITIES: every AI player gets a personality from <game>\aimod_ai\*.ini - a name, traits (0-100) that
// steer its diplomacy, and the lines it says in situations of the game. Included by eeaimod.c after defence.c,
// before diplo.c (which makes the decisions and raises most of the situations).
//
// File format (UTF-8 or ANSI; lines starting with ; or # are comments):
//   [personality]
//   name=Merchant Benedict     the player's name in the game (replaces "Computer"; empty = keep)
//   name:de=Kaufherr Benedikt  ... in a game of that language (see lang_detect)
//   color=yellow, lightyellow  favoured colours on random maps
//   aggression=80 loyalty=30 pride=70 greed=40 forgiveness=20 caution=40 talk=60   (one per line)
//   [lines]                    the default (English)
//   war_declared=Variant one|Variant two      variants separated by |, or the key repeated
//   [lines:de]                 in a game of that language these replace [lines] situation by situation
//   [style]                    EXPERIMENTAL, only with [general] ai_style=1 (default 0): aimod.ini keys
//   first_attack_min=15        (settings.c k_style_keys) on top of the difficulty's settings for this AI
// Placeholders: {name} the player the line is about, {me} the speaker, {enemy} a third player (alliances),
// {epoch} the speaker's age, {tribute} the resources offered / paid (capitulation). A situation without lines stays
// silent (the game's own lines for wonders, flares and war remain where the personality has none).
//
// Assignment at the AI's first update: aimod.ini [general] personality_P<n>=<file name without .ini>, otherwise
// random, every personality once before any repeats (autotest: from the map seed, so runs repeat).
// aimod.ini [general] ai_talk = 1 (default) lines on, 0 off; ai_names = 1 (default) the personalities' names.
//
// Strings: UWideString takes only char * (the game's ANSI code page), so UTF-8 files are converted on load.
// Randomness: an own generator - the game's rand() is part of the simulation and must not be touched.

enum trait { T_AGGRESSION, T_LOYALTY, T_PRIDE, T_GREED, T_FORGIVENESS, T_CAUTION, T_TALK, T_N };
static const char *const k_trait_keys[T_N] = { "aggression", "loyalty", "pride", "greed", "forgiveness", "caution", "talk" };

// situations: key, chance in % at talk=50 (100 = always, scaled by talk otherwise), cooldown s per AI, audience:
// who is meant to hear it - everyone, the AI's allies (plans, worries, calls for help) or only {name}, the player
// it is said to (taunts, diplomacy). The local player sees a line only when they are in its audience - an enemy AI
// doesn't tell them that it attacks someone else. With [general] chat=1+ (debug) every line shows, marked.
enum line_kind {
    L_GREETING, L_WAR_DECLARED, L_BETRAYED, L_PEACE_ACCEPT, L_PEACE_REFUSE, L_ALLIANCE_OFFER, L_OFFER_ACCEPTED,
    L_OFFER_WITHDRAWN, L_BETRAY, L_TRIBUTE_THANKS, L_TRIBUTE_SMALL, L_ATTACK, L_UNDER_ATTACK, L_BUILDING_LOST,
    L_EPOCH_UP, L_ENEMY_AHEAD, L_WINNING, L_LOSING, L_DESPERATE, L_ENEMY_DEFEATED, L_ALLY_DEFEATED, L_DEFEATED,
    L_WONDER_THREAT, L_WONDER_HELP, L_WONDER_NOHELP, L_HELP_LAND, L_HELP_AIR, L_HELP_NAVAL, L_HELP_NONE, L_SURRENDER,
    L_CAPITULATE, L_CAPITULATE_ACCEPTED, L_CAPITULATE_REFUSED, L_ATTACK_THREAT, L_ALL_SUBMITTED, L_LORD_VICTORY,
    L_BLOCK_REFUSE, L_BLOCK_REFUSE_OWN, L_VASSAL_GRUMBLE, L_ASK_LORD, L_N
};
enum audience { A_ALL, A_ALLIES, A_NAME };
static const struct { const char *key; int chance, cooldown; enum audience aud; } k_lines[L_N] = {
    { "greeting", 60, 0, A_ALL },             { "war_declared", 100, 20, A_NAME },     { "betrayed", 100, 20, A_NAME },
    { "peace_accept", 100, 0, A_NAME },       { "peace_refuse", 100, 60, A_NAME },     { "alliance_offer", 100, 0, A_NAME },
    { "offer_accepted", 100, 0, A_NAME },     { "offer_withdrawn", 100, 0, A_NAME },   { "betray", 100, 0, A_NAME },
    { "tribute_thanks", 100, 20, A_NAME },    { "tribute_small", 100, 60, A_NAME },    { "attack", 40, 300, A_ALLIES },
    { "under_attack", 60, 240, A_ALLIES },    { "building_lost", 25, 180, A_ALLIES },  { "epoch_up", 50, 60, A_ALL },
    { "enemy_ahead", 40, 600, A_ALLIES },     { "winning", 40, 600, A_NAME },          { "losing", 50, 600, A_ALLIES },
    { "desperate", 100, 0, A_ALLIES },        { "enemy_defeated", 80, 30, A_ALL },     { "ally_defeated", 80, 30, A_ALLIES },
    { "defeated", 100, 0, A_ALL },            { "wonder_threat", 100, 60, A_ALL },     { "wonder_help", 100, 60, A_ALLIES },
    { "wonder_nohelp", 100, 60, A_ALLIES },   { "help_land", 100, 30, A_ALLIES },      { "help_air", 100, 30, A_ALLIES },
    { "help_naval", 100, 30, A_ALLIES },      { "help_none", 100, 30, A_ALLIES },      { "surrender", 100, 0, A_ALL },
    { "capitulate", 100, 0, A_NAME },         { "capitulate_accepted", 100, 0, A_NAME }, { "capitulate_refused", 100, 0, A_NAME },
    { "attack_threat", 60, 300, A_NAME },     { "all_submitted", 100, 0, A_NAME },     { "lord_victory", 100, 0, A_ALL },
    { "block_refuse", 100, 60, A_NAME },      { "block_refuse_own", 100, 60, A_NAME },  { "vassal_grumble", 100, 60, A_NAME },
    { "ask_lord", 100, 60, A_NAME },
};

#define MAX_PERSONAS 32
#define MAX_VARIANTS 12
struct persona {
    char file[40], name[48];
    char character[300];   // character=: a few words about it for the language model (llm.c)
    int trait[T_N];
    int color[4], ncolor;   // favoured player colours, in order of preference
    char *style_key[16], *style_val[16];   // [style]: aimod.ini settings for this personality (ai_style=1)
    int nstyle;
    char *line[L_N][MAX_VARIANTS];
    uint8_t nline[L_N];
};
static struct persona g_personas[MAX_PERSONAS];
static int g_npersonas;
static int g_persona_of[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static int g_persona_assigned[16];
static int g_ai_talk = 1, g_ai_names = 1;
static int g_ai_style;   // [general] ai_style: 1 = the personalities' [style] settings apply (default 0: off)
static char g_persona_force[16][40];
static uint32_t g_talk_last[16][L_N], g_talk_any[16];
static uint8_t g_talk_variant[16][L_N];
static unsigned g_talk_lines[16];
static uint32_t g_attack_sent[16][16];   // [ai][target]: game time its last attack wave set out (the AI voice)
static uint32_t g_rnd = 0x9E3779B9u;

static uint32_t rnd32(void) { g_rnd ^= g_rnd << 13; g_rnd ^= g_rnd >> 17; g_rnd ^= g_rnd << 5; return g_rnd; }
static int rnd(int n) { return n > 0 ? (int)(rnd32() % (unsigned)n) : 0; }

// Two generators: rnd() for what is only shown (chat lines; ai_talk and the audience differ per PC), rnd_game() for
// decisions that change the game (the personality, a betrayal). Multiplayer is lockstep - every PC simulates every
// AI and must decide the same - so in a game with more than one human the game generator is seeded from the game's
// own rand() (msvcrt, import 0x824648) the first time the simulation needs it: every PC calls it at the same step.
// Single player: seeded like rnd() (the clock; tests a constant).
static uint32_t g_rnd_game = 0x2545F491u;
static int g_mp, g_mp_seeded;   // g_mp: more than one human in the setup (persona_new_game)
static uint32_t rnd_game32(void) {
    if (g_mp && !g_mp_seeded) {
        int (__cdecl *game_rand)(void) = *(int (__cdecl **)(void))0x00824648;
        int a = game_rand(), b = game_rand();
        g_rnd_game = ((uint32_t)a << 15 ^ (uint32_t)b) * 2654435761u ^ 0x2545F491u;
        if (!g_rnd_game) g_rnd_game = 1;
        g_mp_seeded = 1;
        if (g_log) fprintf(g_log, "# persona: multiplayer, game generator seeded from the game's rand (%d %d)\n", a, b);
    }
    g_rnd_game ^= g_rnd_game << 13; g_rnd_game ^= g_rnd_game >> 17; g_rnd_game ^= g_rnd_game << 5;
    return g_rnd_game;
}
static int rnd_game(int n) { return n > 0 ? (int)(rnd_game32() % (unsigned)n) : 0; }

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

// Player colours: index into the game's colour table (0x8CFA74: pointers to {?, float r, g, b}); entries 0-16 are
// player colours (17+ other engine colours). Names in English or German (cp1252 umlauts as \xFC).
static const char *const k_color_names[17][3] = {
    { "blue", "blau", "" }, { "red", "rot", "" }, { "yellow", "gelb", "" }, { "magenta", "lila", "" },
    { "orange", "orange", "" }, { "green", "gruen", "gr\xFCn" }, { "cyan", "tuerkis", "t\xFCrkis" },
    { "darkred", "dunkelrot", "" }, { "darkgrey", "dunkelgrau", "" }, { "purple", "violett", "" },
    { "grey", "grau", "" }, { "silver", "silber", "" }, { "lightyellow", "hellgelb", "" },
    { "lime", "hellgruen", "hellgr\xFCn" }, { "lightblue", "hellblau", "" }, { "pink", "rosa", "" },
    { "darkgrey2", "dunkelgrau2", "" },
};
static int color_index(const char *s) {
    if (*s >= '0' && *s <= '9') { int v = atoi(s); return v <= 16 ? v : -1; }
    for (int c = 0; c < 17; c++)
        for (int k = 0; k < 3; k++)
            if (k_color_names[c][k][0] && !lstrcmpiA(s, k_color_names[c][k])) return c;
    return -1;
}
static const float *color_rgb(int c) {
    uint8_t **b = *(uint8_t ***)0x008CFA74, **e = *(uint8_t ***)0x008CFA78;
    return b && c >= 0 && c < e - b && b[c] ? (const float *)(b[c] + 4) : NULL;
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

// The game's language, for [lines:<language>] and name:<language>: aimod.ini [general] language (a code such as
// de, or a name such as german), else from the game's text - the first age's name (string 30145 in Language.dll;
// its resources are all marked 0x409 whatever the language, so the language id says nothing). Unknown: [lines] only.
static char g_lang[16];
static const char *const k_lang_alias[][4] = {
    { "de", "german", "deutsch", "" }, { "en", "english", "", "" }, { "fr", "french", "francais", "" },
    { "es", "spanish", "espanol", "" }, { "it", "italian", "italiano", "" }, { "pl", "polish", "polski", "" },
    { "pt-br", "pt", "portuguese", "portugues" }, { "ru", "russian", "", "" }, { "ko", "korean", "", "" },
    { "zh-cn", "zh", "chinese", "" }, { "zh-tw", "", "", "" },
};
static int lang_matches(const char *tag) {
    if (!g_lang[0]) return 0;
    if (!lstrcmpiA(tag, g_lang)) return 1;
    for (size_t r = 0; r < sizeof k_lang_alias / sizeof *k_lang_alias; r++) {
        int row = 0;
        for (int k = 0; k < 4 && k_lang_alias[r][k][0]; k++) row |= !lstrcmpiA(g_lang, k_lang_alias[r][k]);
        for (int k = 0; k < 4 && row && k_lang_alias[r][k][0]; k++) if (!lstrcmpiA(tag, k_lang_alias[r][k])) return 1;
    }
    return 0;
}
// The strings of all 12 languages from the community's translation sources (github.com/EE-modders/localized-text,
// Game/<lang>/EE/Language.rc; de and en also checked in the retail German and the Steam English Language.dll):
// the first age's name identifies each language exactly; "Hostile" (29514) only as a fallback where no other
// language shares it (en/fr "Hostile" and es/pt-BR "Hostil" do).
static void lang_detect(void) {
    static const struct { const wchar_t *first_age, *hostile; const char *code; } k_known[] = {
        { L"Vorzeit", L"Feindselig", "de" },
        { L"Prehistoric Age", NULL, "en" },
        { L"I - Pr\u00e9histoire", NULL, "fr" },
        { L"Edad Prehist\u00f3rica", NULL, "es" },
        { L"I - Preistoria", L"Ostile", "it" },
        { L"Prehistoria", L"Wrogi", "pl" },
        { L"Pr\u00e9-Hist\u00f3ria", NULL, "pt-br" },
        { L"\u0414\u043e\u0438\u0441\u0442\u043e\u0440\u0438\u0447\u0435\u0441\u043a\u0430\u044f \u044d\u043f\u043e\u0445\u0430 (I)",
          L"\u0412\u0440\u0430\u0433", "ru" },
        { L"\uc120\uc0ac \uc2dc\ub300", L"\uc801\ub300", "ko" },
        { L"\u53f2\u524d\u65f6\u4ee3", L"\u654c\u5bf9", "zh-cn" },
        { L"\u53f2\u524d\u6642\u4ee3", L"\u6575\u610f", "zh-tw" },
    };
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, sizeof path);
    char *bs = strrchr(path, '\\');
    if (bs) lstrcpyA(bs + 1, "Language.dll");
    HMODULE h = LoadLibraryExA(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    wchar_t age[64] = L"", hostile[64] = L"";
    if (h) { LoadStringW(h, 30145, age, 64); LoadStringW(h, 29514, hostile, 64); FreeLibrary(h); }
    for (size_t i = 0; i < sizeof k_known / sizeof *k_known && !g_lang[0]; i++)
        if (!wcscmp(age, k_known[i].first_age)) lstrcpyA(g_lang, k_known[i].code);
    for (size_t i = 0; i < sizeof k_known / sizeof *k_known && !g_lang[0]; i++)
        if (k_known[i].hostile && !wcscmp(hostile, k_known[i].hostile)) lstrcpyA(g_lang, k_known[i].code);
    if (g_log) fprintf(g_log, "# persona: Language.dll %s: first age \"%ls\", hostile \"%ls\" -> language '%s'\n",
                       h ? "read" : "NOT FOUND", age, hostile, g_lang);
}

// computer.ini: no personality - never assigned - but the fallback for every AI: a situation a personality has no
// line for (and none of the situation's fallbacks either) is said with computer.ini's line; AIs without a
// personality use it alone. It holds neutral lines for what must not go unsaid (diplomacy, capitulation, victory).
static struct persona g_computer;
static int g_has_computer;
static int g_personas_only_computer;   // autotest personas=0: only computer.ini (AIs without a personality)
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
    static char *ll[L_N][MAX_VARIANTS];   // [lines:<game language>]: replace [lines] situation by situation
    uint8_t nll[L_N] = { 0 };
    int lang_name = 0;   // name:<game language> seen (wins over name=)
    int section = 0;   // 1 personality, 2 lines, 3 style, 4 lines in the game's language
    for (char *ln = strtok(text, "\n"); ln; ln = strtok(NULL, "\n")) {
        char *s = trim(ln);
        if (!*s || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            char *close = strchr(s, ']');
            if (close) *close = 0;
            section = !lstrcmpiA(s, "[personality") ? 1 : !lstrcmpiA(s, "[lines") ? 2 : !lstrcmpiA(s, "[style") ? 3 :
                      !_strnicmp(s, "[lines:", 7) && lang_matches(s + 7) ? 4 : 0;
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = trim(s), *val = trim(eq + 1);
        if (section == 1) {
            if (!lstrcmpiA(key, "name") && !lang_name) lstrcpynA(p->name, val, sizeof p->name);
            if (!lstrcmpiA(key, "character")) lstrcpynA(p->character, val, sizeof p->character);
            if (!_strnicmp(key, "name:", 5) && lang_matches(key + 5)) { lstrcpynA(p->name, val, sizeof p->name); lang_name = 1; }
            if (!lstrcmpiA(key, "color") || !lstrcmpiA(key, "colour") || !lstrcmpiA(key, "farbe"))
                for (char *t = val; t && *t && p->ncolor < 4;) {   // not strtok: the line loop uses it
                    char *sep = strpbrk(t, ",|");
                    if (sep) *sep = 0;
                    int c = color_index(trim(t));
                    if (c >= 0) p->color[p->ncolor++] = c;
                    t = sep ? sep + 1 : NULL;
                }
            for (int t = 0; t < T_N; t++)
                if (!lstrcmpiA(key, k_trait_keys[t])) { int v = atoi(val); p->trait[t] = v < 0 ? 0 : v > 100 ? 100 : v; }
        } else if (section == 3) {
            if (p->nstyle < 16) { p->style_key[p->nstyle] = _strdup(key); p->style_val[p->nstyle++] = _strdup(val); }
        } else if (section == 2 || section == 4) {
            for (int k = 0; k < L_N; k++) {
                if (lstrcmpiA(key, k_lines[k].key)) continue;
                char **dst = section == 2 ? p->line[k] : ll[k];
                uint8_t *n = section == 2 ? &p->nline[k] : &nll[k];
                for (char *v = val; v && *v && *n < MAX_VARIANTS;) {
                    char *bar = strchr(v, '|');
                    if (bar) *bar = 0;
                    char *t = trim(v);
                    if (*t) dst[(*n)++] = _strdup(t);
                    v = bar ? bar + 1 : NULL;
                }
            }
        }
    }
    free(text);
    for (int k = 0; k < L_N; k++)   // the language's lines where it has them, [lines] for the rest
        if (nll[k]) {
            for (int v = 0; v < p->nline[k]; v++) free(p->line[k][v]);
            memcpy(p->line[k], ll[k], nll[k] * sizeof *ll[k]);
            p->nline[k] = nll[k];
        }
    if (!lstrcmpiA(p->file, "computer")) {   // the neutral fallback lines, no personality of its own
        g_computer = *p;
        g_has_computer = 1;
        return;
    }
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
        if (fd.cFileName[0] == '_') continue;   // _system.ini, _template.ini, _persona.ini: no personalities (texts.c)
        if (!g_personas_only_computer || !lstrcmpiA(fd.cFileName, "computer.ini")) persona_load(path, fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static struct persona *persona_of(void *pl) {
    if (!pl) return NULL;
    int i = g_persona_of[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    return i >= 0 ? &g_personas[i] : NULL;
}
static int trait(void *pl, enum trait t) { struct persona *p = persona_of(pl); return p ? p->trait[t] : 50; }
// A situation without a line falls back to a more general one; then computer.ini.
static int line_fallback(int k) {
    switch (k) {
    case L_BETRAYED: return L_WAR_DECLARED;
    case L_ASK_LORD: return L_PEACE_REFUSE;
    case L_BLOCK_REFUSE_OWN: return L_BLOCK_REFUSE;
    case L_BLOCK_REFUSE: return L_PEACE_REFUSE;
    case L_SURRENDER: return L_DEFEATED;
    default: return -1;
    }
}
// where situation k's line comes from for this AI: its personality (k or a fallback of it), else computer.ini
static struct persona *line_source(void *pl, int k, int *kk) {
    struct persona *p = persona_of(pl);
    for (int t = k; t >= 0 && p; t = line_fallback(t)) if (p->nline[t]) { *kk = t; return p; }
    for (int t = k; t >= 0 && g_has_computer; t = line_fallback(t)) if (g_computer.nline[t]) { *kk = t; return &g_computer; }
    return NULL;
}
static int persona_has(void *pl, enum line_kind k) { int kk; return pl && g_ai_talk && line_source(pl, k, &kk); }

// Personalities in use this game (each once before any repeats)
static int g_persona_used[MAX_PERSONAS];
static const char *player_name(void *pl);
static int g_plan_persona[16], g_plan_color[16];   // by setup slot, planned at the game start (-1 none)
static int g_color_human[17], g_ai_colors[16], g_ai_color_n, g_ai_color_k;   // for hook_player_color
static int persona_forced(int player) {
    for (int i = 0; i < g_npersonas && player >= 0 && player < 16 && g_persona_force[player][0]; i++)
        if (!lstrcmpiA(g_personas[i].file, g_persona_force[player])) return i;
    return -1;
}
static int persona_random(void) {
    int minuse = 1 << 30, n = 0, pick = -1;
    for (int i = 0; i < g_npersonas; i++) if (g_persona_used[i] < minuse) minuse = g_persona_used[i];
    for (int i = 0; i < g_npersonas; i++) n += g_persona_used[i] == minuse;
    int r = rnd_game(n);
    for (int i = 0; i < g_npersonas && pick < 0; i++) if (g_persona_used[i] == minuse && r-- == 0) pick = i;
    return pick;
}

// A new game (the game start hook, before the game creates the players from the setup): forget the last game;
// on a random map from the menu (setup +0x3D8 mode 0) pick every computer slot's personality now and give the
// slot (+0x24) the personality's favoured colour - the first one no human and no other AI has - otherwise its own
// colour if still free, otherwise any free one. Slots: 16 x 0x38 bytes, +0 type (1 human, 0 computer, 4 closed).
// Scenarios and saved games keep their colours (a saved game finds its personalities again by name).
static void persona_new_game(uint8_t *setup) {
    for (int i = 0; i < 16; i++) { g_persona_of[i] = g_plan_persona[i] = g_plan_color[i] = -1; g_persona_assigned[i] = 0; }
    memset(g_persona_used, 0, sizeof g_persona_used);
    g_ai_color_n = g_ai_color_k = 0;
    memset(g_talk_last, 0, sizeof g_talk_last);
    memset(g_talk_any, 0, sizeof g_talk_any);
    memset(g_talk_lines, 0, sizeof g_talk_lines);
    memset(g_attack_sent, 0, sizeof g_attack_sent);
    memset(g_cfg_pl_on, 0, sizeof g_cfg_pl_on);
    int humans = 0;
    for (int s = 0; s < 16; s++) humans += FIELD(setup + s * 0x38, 0, int) == 1;
    g_mp = humans > 1; g_mp_seeded = 0;
    // multiplayer: no planning here (each PC would pick on its own, and the colours are the lobby's) - the
    // personalities are picked at the AIs' first updates from the game generator, the same on every PC
    if (!g_npersonas || *(int *)(setup + 0x3D8) != 0 || g_mp) return;
    int used[17] = { 0 }, player = 0;
    for (int s = 0; s < 16; s++) {
        uint8_t *slot = setup + s * 0x38;
        int type = FIELD(slot, 0, int), c = FIELD(slot, 0x24, int);
        if (type != 0 && type != 1) continue;
        player++;
        if (type == 1) { if (c >= 0 && c < 17) used[c] = 1; continue; }
        int pick = persona_forced(player);
        if (pick < 0) pick = persona_random();
        if (pick < 0) continue;
        g_persona_used[pick]++;
        g_plan_persona[s] = pick;
    }
    for (int pass = 0; pass < 2; pass++)   // favoured colours first, then the AIs without (or with taken ones)
        for (int s = 0; s < 16; s++) {
            if (g_plan_persona[s] < 0 || g_plan_color[s] >= 0) continue;
            struct persona *p = &g_personas[g_plan_persona[s]];
            int c = -1, own = FIELD(setup + s * 0x38, 0x24, int);
            for (int k = 0; k < p->ncolor && c < 0; k++) if (!used[p->color[k]]) c = p->color[k];
            if (c < 0 && pass == 0) continue;
            if (c < 0 && own >= 0 && own < 17 && !used[own]) c = own;
            for (int k = 0; k < 16 && c < 0; k++) if (!used[k]) c = k;
            if (c < 0) continue;
            used[c] = 1;
            g_plan_color[s] = c;
            FIELD(setup + s * 0x38, 0x24, int) = c;
            if (g_log) fprintf(g_log, "# persona: setup slot %d -> %s, colour %d (%s, was %d)\n", s, p->file, c, k_color_names[c][0], own);
        }
    for (int c = 0; c < 17; c++) g_color_human[c] = 0;
    g_ai_color_n = g_ai_color_k = 0;
    for (int s = 0; s < 16; s++) {
        uint8_t *slot = setup + s * 0x38;
        int type = FIELD(slot, 0, int), c = FIELD(slot, 0x24, int);
        if (type == 1 && c >= 0 && c < 17) g_color_human[c] = 1;
        if (type == 0) g_ai_colors[g_ai_color_n++] = g_plan_color[s];
    }
}

// The game ignores a computer slot's colour: the game start (0x52599B, random map / test play) gives humans their
// slot's colour and every other player the next colour not taken by a human, in slot order (0x5262E4: the colour
// table lookup 0x526B85(index) cdecl -> rgb, then 0x526B3A sets player +0x438). Here an AI's lookup returns its
// planned colour instead. In the game everything shows the player's rgb (diplomacy screen, chat), not an index.
#define CALL_PLAYER_COLOR 0x005262E4
#define FN_COLOR_RGB      0x00526B85
static void *__cdecl hook_player_color(unsigned idx) {
    if (!(idx < 17 && g_color_human[idx]) && g_ai_color_k < g_ai_color_n) {
        int c = g_ai_colors[g_ai_color_k++];
        if (c >= 0) idx = (unsigned)c;
    }
    return ((void *(__cdecl *)(unsigned))FN_COLOR_RGB)(idx);
}

// once per AI (its first update): pick and apply the personality - pinned by aimod.ini, planned at the game start
// (found by the player's colour, +0x438 = the colour table's rgb), the one with the player's name (a loaded game),
// or a random one
static void persona_assign(void *pl) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_persona_assigned[idx]) return;
    g_persona_assigned[idx] = 1;
    if (!g_npersonas) return;
    int pick = persona_forced((int)idx);
    const float *rgb = (const float *)((uint8_t *)pl + 0x438);
    for (int s = 0; s < 16 && pick < 0; s++) {
        const float *c = g_plan_persona[s] >= 0 ? color_rgb(g_plan_color[s]) : NULL;
        if (c && c[0] == rgb[0] && c[1] == rgb[1] && c[2] == rgb[2]) { pick = g_plan_persona[s]; g_plan_persona[s] = -1; }
    }
    for (int i = 0; i < g_npersonas && pick < 0; i++)
        if (g_personas[i].name[0] && !lstrcmpA(g_personas[i].name, player_name(pl))) pick = i;
    if (pick < 0) { pick = persona_random(); if (pick >= 0) g_persona_used[pick]++; }
    if (pick < 0) return;
    g_persona_of[idx] = pick;
    struct persona *p = &g_personas[pick];
    if (g_ai_names && p->name[0])
        ((void *(__thiscall *)(void *, const char *))*(void **)0x00823224)((uint8_t *)pl + 0x24, p->name);
    if (g_log) {
        fprintf(g_log, "# persona: P%u is %s (%s):", idx, p->file, p->name[0] ? p->name : "-");
        for (int t = 0; t < T_N; t++) fprintf(g_log, " %s %d", k_trait_keys[t], p->trait[t]);
        fprintf(g_log, "\n");
    }
    // PLAY STYLE (experimental, ai_style=1): the personality's [style] keys on top of the difficulty's settings,
    // from now on what cfg_of() returns for this player; settings_apply runs right after (hook_rm_update).
    if (g_ai_style && p->nstyle) {
        struct ai_cfg c = *cfg_of(pl);
        for (int k = 0; k < p->nstyle; k++)
            if (!cfg_style_set(&c, p->style_key[k], p->style_val[k]) && g_log)
                fprintf(g_log, "# persona: %s: [style] key '%s' unknown, ignored\n", p->file, p->style_key[k]);
        cfg_style_fix(&c);
        g_cfg_pl[idx] = c;
        g_cfg_pl_on[idx] = 1;
        if (g_log) {
            fprintf(g_log, "# persona: P%u plays with %s's style:", idx, p->file);
            for (int k = 0; k < p->nstyle; k++) fprintf(g_log, " %s=%s", p->style_key[k], p->style_val[k]);
            fprintf(g_log, "\n");
        }
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

// is the local player (0x918E84) in the audience of AI pl's line k said to `other`? Allies: both stances allied.
static int line_heard(void *pl, enum line_kind k, void *other) {
    unsigned h = *(unsigned *)0x00918E84;
    void *me = h < 16 ? ((void **)0x00918374)[h] : NULL;
    if (!me || me == pl || k_lines[k].aud == A_ALL) return 1;
    if (k_lines[k].aud == A_NAME) return other == me;
    return !FIELD(pl, 0x9E4 + h * 4, int) && !FIELD(me, 0x9E4 + (FIELD(pl, PL_INDEX, int) & 15) * 4, int);
}

// say a line of situation k (other = {name}, third = {enemy}); returns 1 if said (heard or not: the situation is
// answered, callers don't fall back to another line)
static int g_say_tribute[7];   // {tribute}: amounts by resource (stock index 2..6), set by the caller (diplo.c)
// {tribute}: the amounts by resource in the game's language, e.g. "200 Gold, 800 Eisen" (only what is paid)
static const char *sys_text(const char *key);   // texts.c: a system text in the game's language
static void tribute_text(const int *amount, char *buf, int cap) {
    static const char *const key[7] = { "", "", "food", "wood", "stone", "gold", "iron" };
    int o = 0;
    buf[0] = 0;
    for (int r = 2; r < 7; r++) {
        if (amount[r] <= 0 || o > cap - 48) continue;
        o += wsprintfA(buf + o, "%s%d %.24s", o ? ", " : "", amount[r], sys_text(key[r]));
    }
    if (!o) lstrcpynA(buf, sys_text("nothing"), cap);
}
static int llm_line(void *pl, enum line_kind k, void *other, void *third, const char *fixed);   // llm.c
static void llm_heard(void *pl, void *other, const char *text);
static void llm_talk_event(void *pl, enum line_kind k, void *other, void *third, const char *fixed);   // llm.c: AI TALK
static int persona_say(void *pl, enum line_kind k, void *other, void *third) {
    int kk = k;
    struct persona *p = pl && g_ai_talk ? line_source(pl, k, &kk) : NULL, *own = persona_of(pl);
    if (!p || (FIELD(pl, 0x467, uint8_t) && k != L_DEFEATED && k != L_SURRENDER)) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t now = game_time();
    if (g_talk_any[idx] && now - g_talk_any[idx] < 3000 && k_lines[k].chance < 100) return 0;
    if (g_talk_last[idx][k] && now - g_talk_last[idx][k] < (uint32_t)k_lines[k].cooldown * 1000u) return 0;
    if (k_lines[k].chance < 100 && rnd(100) >= k_lines[k].chance * (own ? own->trait[T_TALK] : 50) / 50) return 0;
    int n = p->nline[kk], v = rnd(n);
    if (n > 1 && v == g_talk_variant[idx][k]) v = (v + 1) % n;
    g_talk_variant[idx][k] = (uint8_t)v;
    char buf[256], ep[96] = "";
    int o = 0;
    for (const char *s = p->line[kk][v]; *s && o < (int)sizeof buf - 1;) {
        const char *rep = NULL;
        if (*s == '{') {
            if (!strncmp(s, "{name}", 6)) { rep = player_name(other); s += 6; }
            else if (!strncmp(s, "{me}", 4)) { rep = player_name(pl); s += 4; }
            else if (!strncmp(s, "{enemy}", 7)) { rep = player_name(third); s += 7; }
            else if (!strncmp(s, "{epoch}", 7)) { epoch_text(pl, ep, sizeof ep); rep = ep; s += 7; }
            else if (!strncmp(s, "{tribute}", 9)) { tribute_text(g_say_tribute, ep, sizeof ep); rep = ep; s += 9; }
        }
        if (rep) while (*rep && o < (int)sizeof buf - 1) buf[o++] = *rep++;
        else buf[o++] = *s++;
    }
    buf[o] = 0;
    int heard = line_heard(pl, k, other);
    g_talk_last[idx][k] = now ? now : 1;
    if (heard) {
        g_talk_any[idx] = now ? now : 1;
        if (!llm_line(pl, k, other, third, buf)) {   // the language model says it in its own words (later), else now
            chat_line(pl, buf, g_chat_ms);
            llm_heard(pl, other, buf);
        }
        g_talk_lines[idx]++;
    } else llm_talk_event(pl, k, other, third, buf);   // to another AI: they may talk it over (llm.c AI TALK)
    if (!heard && g_chat) {   // debug: every line, with its audience
        char dbg[300];
        wsprintfA(dbg, "[%s] %s", k_lines[k].aud == A_ALLIES ? "allies" : player_name(other), buf);
        chat_line(pl, dbg, g_chat_ms);
    }
    if (g_log) fprintf(g_log, "# talk: P%u at %.1f min (%s%s%s%s%s): %s\n", idx, now / 60000.0, k_lines[k].key,
                       kk != (int)k ? " as " : "", kk != (int)k ? k_lines[kk].key : "", p == &g_computer ? ", computer.ini" : "",
                       heard ? "" : ", not for the player", buf);
    return 1;
}

// an attack group sets out against target's base (the route hook, attack.c): a threat to the local player when
// they are the target (attack_threat), otherwise news for the AI's allies (attack). Once per attack wave: the route
// hook runs for every leg of every subgroup, so the line waits for a launch (the next-attack time +0xED8 redrawn,
// econ_tick) and goes with the first route after it.
static uint8_t g_attack_news[16];
static void persona_attack_launched(unsigned ai) { g_attack_news[ai & 15] = 1; }
static void persona_attack(void *pl, void *target) {
    unsigned ai = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_attack_news[ai]) return;
    g_attack_news[ai] = 0;
    if (target) g_attack_sent[ai][FIELD(target, PL_INDEX, int) & 15] = game_time() ? game_time() : 1;
    unsigned h = *(unsigned *)0x00918E84;
    void *me = h < 16 ? ((void **)0x00918374)[h] : NULL;
    persona_say(pl, target && target == me ? L_ATTACK_THREAT : L_ATTACK, target, NULL);
}

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
    REPLY("%d personalities loaded, language '%s', talk %d, names %d, style %d\n", g_npersonas, g_lang, g_ai_talk, g_ai_names, g_ai_style);
    for (int i = 0; i < 16; i++) {
        struct persona *q = g_persona_of[i] >= 0 ? &g_personas[g_persona_of[i]] : NULL;
        if (!q) continue;
        REPLY("P%d %-12s %-24s", i, q->file, q->name);
        for (int t = 0; t < T_N; t++) REPLY(" %c%d", k_trait_keys[t][0], q->trait[t]);
        REPLY("  lines %u%s\n", g_talk_lines[i], g_cfg_pl_on[i] ? "  (own style)" : "");
    }
}

static void persona_install(const char *ini, const char *test_ini) {
    g_ai_talk = GetPrivateProfileIntA("general", "ai_talk", 1, ini);
    g_ai_names = GetPrivateProfileIntA("general", "ai_names", 1, ini);
    g_ai_style = GetPrivateProfileIntA("general", "ai_style", 0, ini);
    for (int i = 0; i < 16; i++) {
        char key[24];
        wsprintfA(key, "personality_P%d", i);
        GetPrivateProfileStringA("general", key, "", g_persona_force[i], sizeof g_persona_force[i], ini);
        if (test_ini) GetPrivateProfileStringA("autotest", key, g_persona_force[i], g_persona_force[i], sizeof g_persona_force[i], test_ini);
    }
    if (test_ini) {
        g_ai_talk = GetPrivateProfileIntA("autotest", "ai_talk", g_ai_talk, test_ini);
        g_ai_names = GetPrivateProfileIntA("autotest", "ai_names", g_ai_names, test_ini);
        g_ai_style = GetPrivateProfileIntA("autotest", "ai_style", g_ai_style, test_ini);
        g_personas_only_computer = !GetPrivateProfileIntA("autotest", "personas", 1, test_ini);
    }
    g_rnd ^= test_ini ? 0x5EED : GetTickCount();
    g_rnd_game ^= test_ini ? 0x5EED : GetTickCount() * 2654435761u;
    if (!g_rnd) g_rnd = 1;
    GetPrivateProfileStringA("general", "language", "", g_lang, sizeof g_lang, ini);
    if (test_ini) GetPrivateProfileStringA("autotest", "language", g_lang, g_lang, sizeof g_lang, test_ini);
    if (!g_lang[0]) lang_detect();
    personas_load();
    int okc = patch_call(CALL_PLAYER_COLOR, FN_COLOR_RGB, (void *)hook_player_color), ok = 0;
    static const uintptr_t sites[] = { 0x470040, 0x4715F8, 0x47166E, 0x47280E, 0x472B9D };
    for (size_t i = 0; i < sizeof sites / sizeof *sites; i++) ok += patch_call(sites[i], FN_NATIVE_CHAT, (void *)hook_native_chat);
    if (g_log) fprintf(g_log, "# persona: %d personalities, computer.ini %d, language '%s', talk %d, names %d, native line hooks %d/5, colour hook %d\n", g_npersonas, g_has_computer, g_lang, g_ai_talk, g_ai_names, ok, okc);
}
