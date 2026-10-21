// AI PERSONALITIES: every AI player gets a personality from <game>\aimod_ai\*.eeai - a name, traits (0-100) that
// steer its diplomacy, and the lines it says in situations of the game. Included by eeaimod.c after defence.c,
// before diplo.c (which makes the decisions and raises most of the situations).
//
// File format (UTF-8 or ANSI; the syntax: expr.c THE TEXT FILES):
//   include("pack_base.eeai")          a layer: its lines, names, keys after its own; mod/computer.eeai and
//                                      mod/prompt.eeai are layers of every personality unnamed (FILES below)
//   register_personality()             this file is a personality (else a library, for others to include)
//   personality = {
//       name = "Merchant Benedict"     the player's name in the game (replaces "Computer"; empty = keep)
//       color = ["yellow", "lightyellow"]  favoured colours on random maps
//       aggression = 80%               the traits 0%-100%: aggression loyalty pride greed forgiveness caution talk
//       character = "..."              for the language model (llm.c)
//   }
//   rich = me.stock > 5000             a name outside the records (vars): an expression for its lines, before
//                                      mod/system.eeai's
//   lines = {                          English (other languages: a file of their own, at the release)
//       war_declared = ["Variant one", opinion < -50 ? "Variant two {them}"]   any expression: one string, a list ...
//   }
//   decide = {                         its decisions as expressions (persona_decide; mod/computer.eeai's the base)
//       peace_score = opinion + ...    a number; a key it has none of: its includes', else the built-in rule
//   }
//   style = {                          EXPERIMENTAL, only with [general] ai_style=1 (default 0): aimod.ini keys
//       first_attack_min = 15          (settings.c k_style_keys) on top of the difficulty's settings for this AI
//   }
//   prompt = {                         what its language model is told (texts.c; mod/prompt.eeai has every key)
//       intro = "..."
//   }
// In braces: {them} the player the line is about, {me} the speaker, {enemy} a third player (alliances),
// {epoch} the speaker's age, {tribute} the resources offered / paid (capitulation), or an expression (expr.c). A
// situation without a line (or one that is nothing now) falls back (line_fallback, the layers: mod/computer.eeai) or stays silent
// (the game's own lines for wonders, flares and war remain where the personality has none).
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
// who is meant to hear it - everyone, the AI's allies (plans, worries, calls for help) or only {them}, the player
// it is said to (taunts, diplomacy). The local player sees a line only when they are in its audience - an enemy AI
// doesn't tell them that it attacks someone else. With [general] chat=1+ (debug) every line shows, marked.
enum line_kind {
    L_GREETING, L_WAR_DECLARED, L_BETRAYED, L_PEACE_ACCEPT, L_PEACE_REFUSE, L_ALLIANCE_OFFER, L_OFFER_ACCEPTED,
    L_OFFER_WITHDRAWN, L_BETRAY, L_TRIBUTE_THANKS, L_TRIBUTE_SMALL, L_ATTACK, L_UNDER_ATTACK, L_BUILDING_LOST,
    L_EPOCH_UP, L_ENEMY_AHEAD, L_WINNING, L_LOSING, L_DESPERATE, L_ENEMY_DEFEATED, L_ALLY_DEFEATED, L_DEFEATED,
    L_WONDER_THREAT, L_WONDER_HELP, L_WONDER_NOHELP, L_HELP_LAND, L_HELP_AIR, L_HELP_NAVAL, L_HELP_NONE, L_SURRENDER,
    L_CAPITULATE, L_CAPITULATE_ACCEPTED, L_CAPITULATE_REFUSED, L_ATTACK_THREAT, L_ALL_SUBMITTED, L_OTHER_VICTORY,
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
    { "attack_threat", 60, 300, A_NAME },     { "all_submitted", 100, 0, A_NAME },     { "other_victory", 100, 0, A_ALL },
    { "block_refuse", 100, 60, A_NAME },      { "block_refuse_own", 100, 60, A_NAME },  { "vassal_grumble", 100, 60, A_NAME },
    { "ask_lord", 100, 60, A_NAME },
};

// DECISIONS (decide): what an AI wants, worked out by diplo.c with me = the AI, them = the other player, enemy = the
// AI's strongest enemy (none: a missing player, to be checked) - a number each, from the personality or the first of
// its layers that has the key (mod/computer.eeai the last); a mistake (or no number) passes it to the next layer, and
// with none left the answer is no (told once). The rules around them (who may be asked, timers, the offers) stay
// diplo.c's.
enum decide { D_PEACE_SCORE, D_SEEK_ALLY, D_ALLY_SCORE, D_N };
static const char *const k_decide_keys[D_N] = { "peace_score", "seek_ally", "ally_score" };
static int decide_key(const char *key) { int k = 0; while (k < D_N && lstrcmpiA(key, k_decide_keys[k])) k++; return k; }

#define MAX_PERSONAS 32
#define MAX_VARIANTS 12
#define INCLUDES_MAX 8
#define LAYERS_MAX 8
struct includes { int n; char name[INCLUDES_MAX][64]; };
struct persona {
    char file[40], name[48], civ[48];   // civ: civilization= - the civilization's name the game shows for it
    char character[300];   // character=: a few words about it for the language model (llm.c)
    int trait[T_N];
    char *color;   // color=: the favoured player colours, an expression - a list in order of preference (persona_colors)
    char *style_key[16], *style_val[16];   // style: aimod.ini settings for this personality (ai_style=1) - expressions
    int nstyle;
    char *line[L_N];   // a situation's line: an expression (expr.c) - a string, a list of them, conditions
    struct xvars vars;   // its names outside the records (vars), its includes' after them
    char *decide[D_N];   // decide: a decision's expression (its own, else its includes'); NULL: the built-in rule
    char **prompt;   // prompt: the language model's texts by k_tpl index, its own only (texts.c tpl_find); NULL: none
    struct includes inc;   // include(...)
    struct persona *layer[LAYERS_MAX];   // its includes, each one's own right after it (persona_layers)
    int nlayer, registered, resolving;   // registered: register_personality() - a personality, else a library
    unsigned trait_set;   // the traits it sets itself (bit per trait): the others from its layers, else 50%
};
static struct persona g_personas[MAX_PERSONAS];
static int g_npersonas;
static int g_persona_of[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static int g_persona_assigned[16];
static int g_ai_talk = 1, g_ai_names = 1;
static int g_ai_style;   // [general] ai_style: 1 = the personalities' style settings apply (default 0: off)
static char g_persona_force[16][40];
static uint32_t g_talk_last[16][L_N], g_talk_any[16];
static uint32_t g_talk_said[16][L_N];   // the last line's hash (not the same twice when there is a choice)
static unsigned g_talk_lines[16];
static uint32_t g_attack_sent[16][16];   // [ai][target]: game time its last attack wave set out (the AI voice)
static uint8_t g_attack_news[16];        // [ai]: an attack launched, not told yet (persona_attack)
static uint32_t g_rnd = 0x9E3779B9u;

static uint32_t rnd32(void) { g_rnd ^= g_rnd << 13; g_rnd ^= g_rnd >> 17; g_rnd ^= g_rnd << 5; return g_rnd; }
static unsigned g_rnd_draws;   // how many draws so far (texts bench)
static int rnd(int n) { g_rnd_draws++; return n > 0 ? (int)(rnd32() % (unsigned)n) : 0; }

// Two generators: rnd() for what is only shown (chat lines; ai_talk and the audience differ per PC), rnd_game() for
// decisions that change the game (the personality, a betrayal). Multiplayer is lockstep - every PC simulates every
// AI and must decide the same - so in a game with more than one human the game generator is seeded from the game's
// own rand() (msvcrt, import 0x824648) the first time the simulation needs it: every PC calls it at the same step.
// Single player: seeded like rnd() (the clock; tests a constant).
static uint32_t g_rnd_game = 0x2545F491u;
static int g_mp, g_mp_seeded;   // g_mp: more than one human in the setup (persona_new_game)
static uint32_t rnd_game32(void) {
    if (g_mp && !g_mp_seeded) {
        int (__cdecl *game_rand)(void) = *(int (__cdecl **)(void))GA(IAT_RAND);
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
static int color_index(const char *s) {   // by its name; -1: none
    for (int c = 0; c < 17; c++)
        for (int k = 0; k < 3; k++)
            if (k_color_names[c][k][0] && !lstrcmpiA(s, k_color_names[c][k])) return c;
    return -1;
}
static const float *color_rgb(int c) {
    uint8_t **b = *(uint8_t ***)GA(G_COLORS_BEGIN), **e = *(uint8_t ***)GA(G_COLORS_END);
    return b && c >= 0 && c < e - b && b[c] ? (const float *)(b[c] + 4) : NULL;
}

// The game's language, for the built-in German system lines (texts.c) and the colours' names: aimod.ini [general]
// language (a code such as de, or a name such as german), else from the game's text - the first age's name (string
// 30145 in Language.dll; its resources are all marked 0x409 whatever the language, so the language id says nothing).
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

// FILES: every .eeai of the folder (not its subfolders) is read the same way (the format: expr.c): personality = {
// name, civilization, character (strings), color (an expression: a list of colours, worked out at the game start -
// persona_colors), the traits (shares 0%-100%) }, style = { numbers }, decide = { ... }, lines = { each situation a
// string or a list of them, conditions }, prompt = { the language model's texts (texts.c) } - and its names outside
// them (vars). A file that declares register_personality() is a personality (an AI may get it); any other is a
// library, read when a file includes it (_template.eeai, the one to copy, too).
// INCLUDES: include("pack_base.eeai", ...) - a plain file name of this folder, so a pack of files dropped into aimod_ai
// brings its own without touching anyone else's - or one of the mod's own, "mod/computer.eeai", "mod/prompt.eeai", "mod/units.eeai". The
// files a file includes are its LAYERS, in order, each one's own includes right after it (4 deep, no circles): their
// names, decide keys, personality and style keys count where the file has none of its own; their lines and prompt
// texts come after the file's - a situation's line from the first layer whose chain (the situation, then its
// fallbacks: line_fallback) has something to say now, worked out with that layer's names first (line_source).
// THE MOD'S OWN (aimod_ai\mod\): computer.eeai and prompt.eeai are the last layers of every personality, unnamed -
// unless it names them itself (or a layer does), which puts them where it names them; prompt.eeai is computer.eeai's
// last layer too; units.eeai (the groups the texts count with: units, army, buildings ...) a layer of both.
// computer.eeai: the neutral lines for what must not go unsaid (war, peace, offers, capitulation,
// victory: k_must); an AI without a personality speaks with it alone (autotest personas=0). A personality without a
// line for one of k_must (its own or a layer's, the situation's or a fallback's) is a mistake. prompt.eeai: what the
// language model is told (texts.c). system.eeai (texts.c): the system lines, no layer of anyone. The mod has no texts
// of its own: without these files it says nothing (g_texts_missing).
static int line_fallback(int k);
static struct persona g_computer, g_prompt;
static int g_has_computer;
static int g_texts_missing;   // the mod's files aren't there (mod/system.eeai, prompt.eeai, computer.eeai, units.eeai): it says nothing
static int g_personas_only_computer;   // autotest personas=0: only computer.eeai (AIs without a personality)
static const enum line_kind k_must[] = {
    L_WAR_DECLARED, L_BETRAYED, L_PEACE_ACCEPT, L_PEACE_REFUSE, L_ALLIANCE_OFFER, L_OFFER_ACCEPTED, L_OFFER_WITHDRAWN,
    L_BETRAY, L_CAPITULATE, L_CAPITULATE_ACCEPTED, L_CAPITULATE_REFUSED, L_BLOCK_REFUSE, L_BLOCK_REFUSE_OWN, L_ASK_LORD,
    L_ALL_SUBMITTED, L_OTHER_VICTORY, L_SURRENDER, L_DEFEATED,
};
struct inc_add { struct includes *inc; struct tlex *l; };
static void include_add(const char *name, void *arg) {
    struct inc_add *a = arg;
    size_t n = strlen(name);
    int mod = !lstrcmpiA(name, "mod/computer.eeai") || !lstrcmpiA(name, "mod/prompt.eeai") || !lstrcmpiA(name, "mod/units.eeai");
    if (!mod && (n < 6 || n >= 64 || lstrcmpiA(name + n - 5, ".eeai") || strpbrk(name, "/\\:") || strstr(name, "..")))
        x_error("%s:%d: include \"%s\": a file name in this folder expected (\"pack_base.eeai\"), or \"mod/computer.eeai\", "
                "\"mod/prompt.eeai\", \"mod/units.eeai\"", a->l->file, a->l->line, name);
    else if (a->inc->n < INCLUDES_MAX) lstrcpynA(a->inc->name[a->inc->n++], name, 64);
    else x_error("%s:%d: include \"%s\": more than %d included files", a->l->file, a->l->line, name, INCLUDES_MAX);
}
// the mod's files every personality has as layers, after the ones it names (persona_layers leaves out one it has)
static void include_unnamed(struct persona *p, const char *name) {
    for (int i = 0; i < p->inc.n; i++) if (!lstrcmpiA(p->inc.name[i], name)) return;
    if (p->inc.n < INCLUDES_MAX) lstrcpynA(p->inc.name[p->inc.n++], name, 64);
    else x_error("%s.eeai: no room for %s after its %d includes", p->file, name, INCLUDES_MAX);
}
static int tpl_key(const char *key);   // texts.c: a prompt key's index, -1 none
static void persona_prompt_key(struct persona *p, const char *key, struct tval *v, struct tlex *l);
// the record a key of the files belongs in (a situation: lines, a trait: personality ...); NULL: none - a name
static const char *record_of_key(const char *key) {
    for (int k = 0; k < L_N; k++) if (!lstrcmpiA(key, k_lines[k].key)) return "lines";
    for (int t = 0; t < T_N; t++) if (!lstrcmpiA(key, k_trait_keys[t])) return "personality";
    if (!lstrcmpiA(key, "name") || !lstrcmpiA(key, "civilization") || !lstrcmpiA(key, "character") || !lstrcmpiA(key, "color")) return "personality";
    if (decide_key(key) < D_N) return "decide";
    if (style_key(key) >= 0) return "style";
    if (tpl_key(key) >= 0) return "prompt";
    return NULL;
}
struct color_read { struct tlex *l; int line; };
static void color_check(const char *name, void *arg) {   // a colour name as written: known? (the rest at the game start)
    struct color_read *r = arg;
    if (color_index(name) < 0) x_error("%s:%d: unknown colour \"%s\"", r->l->file, r->line, name);
}
static void persona_key(void *arg, const char *sec, const char *key, struct tval *v, struct tlex *l) {
    struct persona *p = arg;
    if (!*sec && !lstrcmpiA(key, "include")) {   // include("a.eeai", ...) (tfile_parse: the list of its names)
        struct inc_add a = { &p->inc, l };
        if (!x_plain_strings(v->src, include_add, &a)) tl_error(l, "include(\"pack_base.eeai\", ...): file names expected");
        return;
    }
    if (!*sec && !lstrcmpiA(key, "register_personality")) { p->registered = 1; return; }
    if (!*sec) {   // a name outside the records (vars) - not one of a record's keys, written where it is no key
        const char *rec = record_of_key(key);
        if (rec) { char why[120]; snprintf(why, sizeof why, "%s belongs in %s = { ... }", key, rec); tl_error(l, why); return; }
        xvars_set(&p->vars, key, v->src);
        return;
    }
    if (!lstrcmpiA(sec, "personality")) {
        int civ = !lstrcmpiA(key, "civilization"), name = !lstrcmpiA(key, "name");
        if (civ || name || !lstrcmpiA(key, "character")) {
            if (v->kind != TV_STRING) { tl_error(l, "a string expected (\"...\")"); return; }
            if (civ) lstrcpynA(p->civ, v->text, sizeof p->civ);
            else if (name) lstrcpynA(p->name, v->text, sizeof p->name);
            else lstrcpynA(p->character, v->text, sizeof p->character);
            return;
        }
        if (!lstrcmpiA(key, "color") || !lstrcmpiA(key, "colour") || !lstrcmpiA(key, "farbe")) {
            struct color_read cr = { l, v->line };
            x_plain_strings(v->src, color_check, &cr);
            free(p->color);
            p->color = _strdup(v->src);
            return;
        }
        for (int t = 0; t < T_N; t++)
            if (!lstrcmpiA(key, k_trait_keys[t])) {
                // a share, 0% to 100%: aggression = 60% (a bare 60 is a mistake - not guessed to be 60%)
                char *e;
                double n = strtod(v->src, &e);
                while (*e == ' ' || *e == '\t') e++;
                if (e == v->src || *e != '%' || e[1]) { tl_error(l, "a share 0% to 100% expected (aggression = 60%)"); return; }
                p->trait[t] = n < 0 ? 0 : n > 100 ? 100 : (int)(n + 0.5);
                p->trait_set |= 1u << t;
                return;
            }
        tl_error(l, "an unknown key in personality");
    } else if (!lstrcmpiA(sec, "style")) {
        if (style_key(key) < 0) { tl_error(l, "an unknown style key"); return; }
        if (p->nstyle < 16) { p->style_key[p->nstyle] = _strdup(key); p->style_val[p->nstyle++] = _strdup(v->src); }
    } else if (!lstrcmpiA(sec, "decide")) {
        int k = decide_key(key);
        if (k == D_N) { tl_error(l, "an unknown decision (peace_score, seek_ally, ally_score)"); return; }
        free(p->decide[k]);
        p->decide[k] = _strdup(v->src);
    } else if (!lstrcmpiA(sec, "lines")) {
        int k = 0;
        while (k < L_N && lstrcmpiA(key, k_lines[k].key)) k++;
        if (k == L_N) { tl_error(l, "an unknown situation"); return; }
        if (v->kind == TV_NUMBER) { tl_error(l, "a string or a list of them expected"); return; }
        free(p->line[k]);
        p->line[k] = _strdup(v->src);
    } else if (!lstrcmpiA(sec, "prompt")) persona_prompt_key(p, key, v, l);
    else tl_error(l, "an unknown record (personality, style, decide, lines, prompt)");
}
// read file (in the folder dir) into p - its own keys only (persona_layers adds its includes'); 0: no such file
static int persona_parse(const char *dir, const char *file, struct persona *p) {
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s%s", dir, file);
    char *text = read_text_acp(path);
    if (!text) return 0;
    memset(p, 0, sizeof *p);
    lstrcpynA(p->file, file, sizeof p->file);
    char *dot = strrchr(p->file, '.');
    if (dot) *dot = 0;
    for (int t = 0; t < T_N; t++) p->trait[t] = 50;
    tfile_parse(file, text, persona_key, p);
    free(text);
    return 1;
}
// THE LIBRARIES: the included files, each read once (mod/computer.eeai into g_computer, mod/prompt.eeai into g_prompt)
#define MAX_LIBS 16
static struct persona *g_libs[MAX_LIBS];
static int g_nlibs;
static void persona_layers(struct persona *p, const char *dir, int depth);
static void prompt_check(const struct persona *p);   // texts.c
static struct persona *lib_get(const char *dir, const char *file, const char *by, int depth) {
    for (int i = 0; i < g_nlibs; i++)
        if (!lstrcmpiA(g_libs[i]->file, file) || (!_strnicmp(g_libs[i]->file, file, strlen(g_libs[i]->file)) && !lstrcmpiA(file + strlen(g_libs[i]->file), ".eeai"))) {
            if (g_libs[i]->resolving) { x_error("%s: include \"%s\": a circle", by, file); return NULL; }
            return g_libs[i];
        }
    if (depth > 4) { x_error("%s: include \"%s\": more than 4 deep", by, file); return NULL; }
    if (g_nlibs == MAX_LIBS) { x_error("%s: include \"%s\": more than %d included files", by, file, MAX_LIBS); return NULL; }
    struct persona *p = !lstrcmpiA(file, "mod/computer.eeai") ? &g_computer : !lstrcmpiA(file, "mod/prompt.eeai") ? &g_prompt :
                        malloc(sizeof *p);
    if (!persona_parse(dir, file, p)) {
        x_error("%s: include \"%s\": no such file in aimod_ai", by, file);
        if (p != &g_computer && p != &g_prompt) free(p);
        return NULL;
    }
    if (p == &g_computer) { g_has_computer = 1; include_unnamed(p, "mod/prompt.eeai"); }
    if (p == &g_prompt) prompt_check(p);
    g_libs[g_nlibs++] = p;
    persona_layers(p, dir, depth);
    return p;
}
// p's layers - its includes, each one's own right after it - and what it takes from them where it has none of its own
static void persona_layers(struct persona *p, const char *dir, int depth) {
    p->resolving = 1;
    for (int i = 0; i < p->inc.n; i++) {
        struct persona *lib = lib_get(dir, p->inc.name[i], p->file, depth + 1);
        if (!lib) continue;
        for (int j = -1; j < lib->nlayer; j++) {
            struct persona *add = j < 0 ? lib : lib->layer[j];
            int seen = add == p;
            for (int k = 0; k < p->nlayer && !seen; k++) seen = p->layer[k] == add;
            if (!seen && p->nlayer < LAYERS_MAX) p->layer[p->nlayer++] = add;
        }
    }
    p->resolving = 0;
    for (int i = 0; i < p->nlayer; i++) {
        const struct persona *q = p->layer[i];
        for (int k = 0; k < q->vars.n; k++)   // its names where p has no such name (a function too: p's own wins)
            if (xvars_find(&p->vars, q->vars.key[k]) < 0) {
                if (p->vars.n == XVARS_MAX) { x_error("%s: more than %d names (vars) with its includes", p->file, XVARS_MAX); break; }
                xvars_set(&p->vars, q->vars.key[k], q->vars.val[k]);
            }
        for (int k = 0; k < D_N; k++) if (!p->decide[k] && q->decide[k]) p->decide[k] = _strdup(q->decide[k]);
        for (int t = 0; t < T_N; t++) if (!(p->trait_set >> t & 1) && (q->trait_set >> t & 1)) { p->trait[t] = q->trait[t]; p->trait_set |= 1u << t; }
        if (!p->name[0]) lstrcpyA(p->name, q->name);
        if (!p->civ[0]) lstrcpyA(p->civ, q->civ);
        if (!p->character[0]) lstrcpyA(p->character, q->character);
        if (!p->color && q->color) p->color = _strdup(q->color);
        for (int k = 0; k < q->nstyle; k++) {
            int have = 0;
            for (int m = 0; m < p->nstyle && !have; m++) have = !lstrcmpiA(p->style_key[m], q->style_key[k]);
            if (!have && p->nstyle < 16) { p->style_key[p->nstyle] = _strdup(q->style_key[k]); p->style_val[p->nstyle++] = _strdup(q->style_val[k]); }
        }
    }
}
static int layer_has(const struct persona *p, int k) {   // a line for situation k (or a fallback) in p or its layers
    for (int i = -1; p && i < p->nlayer; i++)
        for (int t = k; t >= 0; t = line_fallback(t)) if ((i < 0 ? p : p->layer[i])->line[t]) return 1;
    return 0;
}
static void persona_must_check(const struct persona *p) {   // every situation of k_must has a line somewhere
    char miss[400] = "";
    for (size_t i = 0; i < sizeof k_must / sizeof *k_must; i++)
        if (!layer_has(p, k_must[i]) && strlen(miss) < sizeof miss - 30) { lstrcatA(miss, " "); lstrcatA(miss, k_lines[k_must[i]].key); }
    if (miss[0]) x_error("%s.eeai: no line for%s - a personality must have one (its own or a layer's: mod/computer.eeai)", p->file, miss);
}
static void personas_load(void) {
    char dir[MAX_PATH], pat[MAX_PATH];
    g_xfn_scanned = 0;   // the files may have changed since: scanned again when a function is first met
    g_nlibs = 0;
    g_has_computer = 0;
    lstrcpyA(dir, g_ai_dir);
    // the mod's own files first (prompt.eeai is computer.eeai's layer too, units.eeai both's): without them it says
    // nothing (texts.c)
    if (!lib_get(dir, "mod/units.eeai", "eeaimod", 0) || !lib_get(dir, "mod/prompt.eeai", "eeaimod", 0) ||
        !lib_get(dir, "mod/computer.eeai", "eeaimod", 0)) {
        g_texts_missing = 1;
        g_has_computer = 0;
        return;
    }
    persona_must_check(&g_computer);   // the AIs without a personality speak with it
    snprintf(pat, sizeof pat, "%s*.eeai", dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    static struct persona p;
    do {
        if (g_personas_only_computer || g_npersonas >= MAX_PERSONAS || !persona_parse(dir, fd.cFileName, &p)) continue;
        if (!p.registered) continue;   // a library (its mistakes are told when it is read)
        include_unnamed(&p, "mod/computer.eeai");
        include_unnamed(&p, "mod/prompt.eeai");
        g_personas[g_npersonas] = p;
        persona_layers(&g_personas[g_npersonas], dir, 0);
        persona_must_check(&g_personas[g_npersonas]);
        g_npersonas++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

// expr.c x_fn_known: the functions and names of every file in the folder and in mod\ (name(a, b) = ...), before any
// file is checked
static void x_fn_scan_folder(const char *dir) {
    char pat[MAX_PATH], path[MAX_PATH];
    snprintf(pat, sizeof pat, "%s*.eeai", dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        snprintf(path, sizeof path, "%s%s", dir, fd.cFileName);
        char *text = read_text_acp(path);
        if (text) { x_fn_scan(text); free(text); }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
static void x_fn_scan_dir(void) {
    char dir[MAX_PATH];
    g_xfn_n = g_xvn_n = 0;
    x_fn_scan_folder(g_ai_dir);   // <game>\aimod_ai\, [general] ai_dir or [autotest] ai_dir
    snprintf(dir, sizeof dir, "%smod\\", g_ai_dir);
    x_fn_scan_folder(dir);
}

static struct persona *persona_of(void *pl) {
    if (!pl) return NULL;
    int i = g_persona_of[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    return i >= 0 ? &g_personas[i] : NULL;
}
static int trait(void *pl, enum trait t) { struct persona *p = persona_of(pl); return p ? p->trait[t] : 50; }
// resources one point of pl's opinion is worth (tributes, peace terms): 20 for the greediest, 100 without greed
static float tribute_worth(void *pl) { return 20.0f + 0.8f * (float)(100 - trait(pl, T_GREED)); }
// the share of each resource pl offers in a capitulation, by its caution and greed
static float capit_share(void *pl) { return 0.2f + (float)trait(pl, T_CAUTION) / 400.0f + (float)(100 - trait(pl, T_GREED)) / 400.0f; }
// A situation without a line falls back to a more general one; then the next layer (line_source).
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
// A line's context for its {expressions} and conditions (expr.c): the speaker, them, enemy, the placeholder,
// the vars of the line's personality, the speaker's own, mod/system.eeai's
struct line_ctx { struct xctx c; const char *pn[2], *pv[2]; char trib[96], trib_total[16]; };   // {me} {them} {enemy} {epoch}: the game values (diplo.c x_builtin)
static void epoch_text(void *pl, char *out, int cap);
static void epoch_text_n(int e, char *out, int cap);
static const char *player_name(void *pl);
static void tribute_text(const int *amount, char *buf, int cap);
static int g_say_tribute[7];   // {tribute}, {tribute_total}: amounts by resource (stock index 2..6), set by the caller (diplo.c)
static void texts_load(void);   // texts.c: mod/system.eeai's vars
static void line_ctx_init(struct line_ctx *l, void *pl, void *other, void *third) {
    memset(&l->c, 0, sizeof l->c);
    texts_load();
    if (!other) {   // a line to everyone (greeting, epoch_up ...): them is the player at this PC
        unsigned h = *(unsigned *)GA(G_LOCAL_PLAYER);
        void *hp = h < 16 ? ((void **)GA(G_PLAYERS))[h] : NULL;
        if (hp != pl) other = hp;
    }
    l->c.pl = pl; l->c.other = other; l->c.third = third; l->c.found = 1;
    tribute_text(g_say_tribute, l->trib, sizeof l->trib);
    int total = 0;
    for (int r = 2; r < 7; r++) total += g_say_tribute[r] > 0 ? g_say_tribute[r] : 0;
    wsprintfA(l->trib_total, "%d", total);
    l->pn[0] = "tribute";   // what a capitulation offers / a tribute brought, by resource ("200 gold, 800 iron")
    l->pv[0] = l->trib;
    l->pn[1] = "tribute_total";   // the same in all, a number
    l->pv[1] = l->trib_total;
    l->c.pn = l->pn; l->c.pv = l->pv; l->c.np = 2;
    l->c.vars[1] = &(persona_of(pl) ? persona_of(pl) : &g_computer)->vars;
    l->c.vars[2] = &g_vars_sys;
}
// decision k of AI pl about them (enemy: its strongest enemy, or NULL): the number of its personality's decide
// expression, worked out with that file's names - or, when it fails now (a mistake, told once; no number), of the
// next layer's (one a layer took over from the file before it is that one again: skipped); none left: no
static double persona_decide(void *pl, int k, void *them, void *enemy) {
    static const double no[D_N] = { -100, 0, -100 };   // peace refused, no ally sought, nobody asked
    const struct persona *p = persona_of(pl);
    if (!p && g_has_computer) p = &g_computer;
    const char *tried[LAYERS_MAX + 1];
    int nt = 0;
    texts_load();
    for (int i = -1; p && i < p->nlayer; i++) {
        const struct persona *q = i < 0 ? p : p->layer[i];
        int seen = !q->decide[k];
        for (int t = 0; t < nt && !seen; t++) seen = !strcmp(tried[t], q->decide[k]);
        if (seen) continue;
        tried[nt++] = q->decide[k];
        struct xctx c;
        memset(&c, 0, sizeof c);
        c.pl = pl; c.other = them; c.third = enemy; c.found = 1;
        c.vars[0] = &q->vars;
        c.vars[2] = &g_vars_sys;
        c.game = 1;
        struct xval x;
        if (!x_eval(q->decide[k], &c, &x)) {
            if (!c.failed) x_log_bad("a decision that is no expression", q->decide[k]);
            continue;
        }
        if (!x.w || !x.has_n || x.pl) {   // nothing, a text, a player: no number to decide by
            x_error("decide %s of %s: not a number - the next layer's instead", k_decide_keys[k], q->file);
            continue;
        }
        return k == D_SEEK_ALLY ? x.n : x.n * 100;   // a score: the language's shares, the code's points (opinion -100..100)
    }
    return no[k];
}
static uint32_t text_hash(const char *s) { uint32_t h = 2166136261u; while (*s) h = (h ^ (uint8_t)*s++) * 16777619u; return h | 1; }
// p's line for situation k, worked out now (out, cap bytes): its value - one of a list's items, what holds of its
// conditions; not the text said last time (`last`, a hash) when it can be another (up to 3 tries). Returns how many
// it was chosen from, 0 when nothing is left (or the line is no expression: logged).
static int line_pick(struct persona *p, int k, struct xctx *c, uint32_t last, char *out, int cap) {
    struct xval x;
    c->vars[0] = &p->vars;
    int f0 = c->failed;
    if (!x_eval(p->line[k], c, &x)) {   // a mistake while working it out (told): nothing to say - the fallback
        if (c->failed == f0) x_log_bad("a line that is no expression", p->line[k]);
        return 0;
    }
    if (!x.w) return 0;
    for (int tries = 0; tries < 3; tries++) {   // the line said: one of a list's items (another than the last if it can)
        struct xval one = x;
        x_pick(c, &one);
        xto_text(&one, out, cap);
        if (x.ln < 2 || text_hash(out) != last) break;
    }
    return x.ln ? x.ln : 1;
}
// where situation k's line comes from for this AI: its personality (mod/computer.eeai without one), then its layers (its
// includes) - the first whose chain (k, then its fallbacks) has something to say now (*kk the situation, *n how many
// it was chosen from)
static struct persona *line_source(void *pl, int k, int *kk, struct xctx *c, uint32_t last, char *out, int cap, int *n) {
    struct persona *p = persona_of(pl);
    if (!p && g_has_computer) p = &g_computer;
    for (int i = -1; p && i < p->nlayer; i++) {
        struct persona *q = i < 0 ? p : p->layer[i];
        for (int t = k; t >= 0; t = line_fallback(t))
            if (q->line[t] && (*n = line_pick(q, t, c, t == k ? last : 0, out, cap)) > 0) { *kk = t; return q; }
    }
    return NULL;
}
static int persona_has(void *pl, enum line_kind k) {   // a line for it somewhere (whether it has something to say now or not)
    if (!pl || !g_ai_talk) return 0;
    struct persona *p = persona_of(pl);
    return layer_has(p ? p : g_has_computer ? &g_computer : NULL, k);
}

// Personalities in use this game (each once before any repeats)
static int g_persona_used[MAX_PERSONAS];
static const char *player_name(void *pl);
static const char *player_civ(void *pl);   // its civilization as the game shows it
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

// p's favoured colours, in order (out, at most cap): its color= worked out now - before the game, so with no players
// and no game values (xctx nogame) - each item a colour's name or number (color.red); returns how many
static int persona_colors(struct persona *p, int *out, int cap) {
    if (!p->color) return 0;
    struct xctx c = { 0 };
    c.found = 1;
    c.nogame = 1;
    c.vars[0] = &p->vars;
    c.vars[1] = &g_vars_sys;
    struct xval v;
    if (!x_eval(p->color, &c, &v)) { if (!c.failed) x_log_bad("a colour that is no expression", p->color); return 0; }
    int n = 0;
    for (int i = 0; i < xl_n(&v) && n < cap; i++) {
        const struct xval *it = xl_at(&v, i);
        int k = it->has_s ? color_index(it->s) : -1;   // a name, or a number 1..17 (color.red, 2 - "2" is a name: none)
        if (k < 0 && !it->has_s && it->has_n && it->n >= 1 && it->n <= 17 && it->n == (int)it->n) k = (int)it->n - 1;
        if (k >= 0) out[n++] = k;
        else { char t[XVAL_CAP]; xto_text(it, t, sizeof t); x_error("%s: unknown colour \"%s\" in color = %.100s", p->file, t, p->color); }
    }
    return n;
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
    memset(g_talk_said, 0, sizeof g_talk_said);
    memset(g_attack_news, 0, sizeof g_attack_news);   // an attack of the last game not yet told
    memset(g_say_tribute, 0, sizeof g_say_tribute);
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
        if (is_vanilla_idx(player)) continue;   // a vanilla player: the game's name and colour
        int pick = persona_forced(player);
        if (pick < 0) pick = persona_random();
        if (pick < 0) continue;
        g_persona_used[pick]++;
        g_plan_persona[s] = pick;
    }
    static int fav[16][17], nfav[16];   // each AI's favoured colours, worked out once (chance() in them: one draw)
    for (int s = 0; s < 16; s++) nfav[s] = g_plan_persona[s] >= 0 ? persona_colors(&g_personas[g_plan_persona[s]], fav[s], 17) : 0;
    for (int pass = 0; pass < 2; pass++)   // favoured colours first, then the AIs without (or with taken ones)
        for (int s = 0; s < 16; s++) {
            if (g_plan_persona[s] < 0 || g_plan_color[s] >= 0) continue;
            struct persona *p = &g_personas[g_plan_persona[s]];
            int c = -1, own = FIELD(setup + s * 0x38, 0x24, int);
            for (int k = 0; k < nfav[s] && c < 0; k++) if (!used[fav[s][k]]) c = fav[s][k];
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
static void *__cdecl hook_player_color(unsigned idx) {
    if (!(idx < 17 && g_color_human[idx]) && g_ai_color_k < g_ai_color_n) {
        int c = g_ai_colors[g_ai_color_k++];
        if (c >= 0) idx = (unsigned)c;
    }
    return GF(FN_COLOR_RGB)(idx);
}

// once per AI (its first update): pick and apply the personality - pinned by aimod.ini, planned at the game start
// (found by the player's colour, +0x438 = the colour table's rgb), the one with the player's name (a loaded game),
// or a random one
static void persona_apply(void *pl, unsigned idx, int pick);
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
    if (pick >= 0) persona_apply(pl, idx, pick);
}
// AI pl (player idx) is personality pick from now on: its name, civilization, style (persona_assign, cmd_persona)
static void persona_apply(void *pl, unsigned idx, int pick) {
    g_persona_of[idx] = pick;
    g_cfg_pl_on[idx] = 0;   // the last one's style (cmd_persona)
    struct persona *p = &g_personas[pick];
    if (g_ai_names && p->name[0])
        ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_ASSIGN))((uint8_t *)pl + 0x24, p->name);
    // its civilization's name as the game shows it: the UString at +0x488 (the text at +0x48C) - the database's own
    // name at +0x7AC stays (X.civilization_en)
    if (g_ai_names && p->civ[0] && player_civ(pl))
        ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_ASSIGN))((uint8_t *)pl + 0x488, p->civ);
    if (g_log) {
        fprintf(g_log, "# persona: P%u is %s (%s):", idx, p->file, p->name[0] ? p->name : "-");
        for (int t = 0; t < T_N; t++) fprintf(g_log, " %s %d", k_trait_keys[t], p->trait[t]);
        fprintf(g_log, "\n");
    }
    // PLAY STYLE (experimental, ai_style=1): the personality's style keys on top of the difficulty's settings,
    // from now on what cfg_of() returns for this player; settings_apply runs right after (hook_rm_update).
    // Each value an expression worked out now, before the game's values count (xctx nogame, like color =): constants,
    // chance(), rnd(), language and vars - its draws from the game's generator (game: every PC alike). A mistake or a
    // value out of its range: that key keeps the difficulty's value (told).
    if (g_ai_style && p->nstyle) {
        struct ai_cfg c = *cfg_of(pl);
        char used[600] = "";
        for (int k = 0; k < p->nstyle; k++) {
            int sk = style_key(p->style_key[k]);
            struct xctx xc = { 0 };
            xc.found = 1; xc.nogame = 1; xc.game = 1;
            xc.vars[0] = &p->vars;
            xc.vars[1] = &g_vars_sys;
            struct xval v;
            const char *why = "a number expected";
            if (sk < 0) continue;
            if (!x_eval(p->style_val[k], &xc, &v)) { if (!xc.failed) x_log_bad("a style value that is no expression", p->style_val[k]); continue; }
            if (!v.w || !v.has_n || v.pl || !cfg_style_set(&c, sk, v.n, &why)) {
                x_error("%s: style %s = %.60s: %s - the difficulty's value instead", p->file, p->style_key[k], p->style_val[k], why);
                continue;
            }
            size_t n = strlen(used);   // as set, in aimod.ini's units (a chance 40 for 40%)
            void *at = (uint8_t *)&c + k_style_keys[sk].off;
            if (k_style_keys[sk].kind == SK_SHARE) snprintf(used + n, sizeof used - n, " %s=%.2f", p->style_key[k], *(float *)at);
            else snprintf(used + n, sizeof used - n, " %s=%d", p->style_key[k], *(int *)at);
        }
        cfg_style_fix(&c);
        g_cfg_pl[idx] = c;
        g_cfg_pl_on[idx] = 1;
        if (g_log) fprintf(g_log, "# persona: P%u plays with %s's style:%s\n", idx, p->file, used);
    }
}

static const char *player_name(void *pl) {
    const char *s = pl ? FIELD(pl, 0x28, const char *) : NULL;
    return s ? s : "?";
}
// the player's civilization as the game shows it, in its language (+0x48C: "Karthago" - or a personality's
// civilization=); en: the database's name (+0x7B0: "Carthage") - NULL: none chosen
static const char *player_civ_of(void *pl, int en) {
    const char *s = pl ? FIELD(pl, en ? 0x7B0 : 0x48C, const char *) : NULL;
    return s && mem_readable(s, 1) && *s ? s : NULL;
}
static const char *player_civ(void *pl) { return player_civ_of(pl, 0); }
// the player's colour: its index 0..16 in the colour table (+0x438 holds the table entry's rgb) - -1: another one
static int player_color(void *pl) {
    const float *rgb = (const float *)((uint8_t *)pl + 0x438);
    for (int c = 0; c < 17; c++) {
        const float *t = color_rgb(c);
        if (t && t[0] == rgb[0] && t[1] == rgb[1] && t[2] == rgb[2]) return c;
    }
    return -1;
}
static const char *color_name(int c) {   // in the game's language: the German names in a German game, else English
    if (lang_matches("de")) return k_color_names[c][2][0] ? k_color_names[c][2] : k_color_names[c][1];
    return k_color_names[c][0];
}

// the speaker's age, localized (strings 30145 + epoch: Vorzeit, Steinzeit, ...)
static void epoch_text_n(int e, char *out, int cap) {   // epoch e (0 Prehistoric) by name
    void *res = *(void **)GA(G_RESOURCES);
    lstrcpynA(out, epoch_name(e), cap);
    if (!res || e < 0 || e > 14) return;
    __attribute__((aligned(8))) uint8_t s[32];
    ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_CTOR))(s);
    ((int (__thiscall *)(void *, int, void *))*(void **)GA(IAT_RESOURCE_STRING))(res, 30145 + e, s);
    if (FIELD(s, 4, const char *)) lstrcpynA(out, FIELD(s, 4, const char *), cap);
    ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_DTOR))(s);
}
static void epoch_text(void *pl, char *out, int cap) { epoch_text_n(current_epoch(pl), out, cap); }

// is the local player (0x918E84) in the audience of AI pl's line k said to `other`? Allies: both stances allied.
static int line_heard(void *pl, enum line_kind k, void *other) {
    unsigned h = *(unsigned *)GA(G_LOCAL_PLAYER);
    void *me = h < 16 ? ((void **)GA(G_PLAYERS))[h] : NULL;
    if (!me || me == pl || k_lines[k].aud == A_ALL) return 1;
    if (k_lines[k].aud == A_NAME) return other == me;
    return !FIELD(pl, 0x9E4 + h * 4, int) && !FIELD(me, 0x9E4 + (FIELD(pl, PL_INDEX, int) & 15) * 4, int);
}

// say a line of situation k (other = them, third = enemy); returns 1 if said (heard or not: the situation is
// answered, callers don't fall back to another line)
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
// the language model's words for the next line, instead of one of the personality's (a move it decided, llm.c
// llm_apply - its line is already written): said as it is, whatever the line's chance or cooldown
static const char *g_say_instead;
static int persona_say(void *pl, enum line_kind k, void *other, void *third) {
    struct persona *own = persona_of(pl);
    if (!pl || !g_ai_talk || (FIELD(pl, 0x467, uint8_t) && k != L_DEFEATED && k != L_SURRENDER)) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t now = game_time();
    if (g_say_instead) {
        const char *t = g_say_instead;
        g_say_instead = NULL;
        chat_line(pl, t, g_chat_ms);
        llm_heard(pl, other, t);
        g_talk_last[idx][k] = g_talk_any[idx] = now ? now : 1;
        if (g_log) fprintf(g_log, "# talk: P%u at %.1f min (%s, the model's words): %s\n", idx, now / 60000.0, k_lines[k].key, t);
        return 1;
    }
    if (g_talk_any[idx] && now - g_talk_any[idx] < 3000 && k_lines[k].chance < 100) return 0;
    if (g_talk_last[idx][k] && now - g_talk_last[idx][k] < (uint32_t)k_lines[k].cooldown * 1000u) return 0;
    if (k_lines[k].chance < 100 && rnd(100) >= k_lines[k].chance * (own ? own->trait[T_TALK] : 50) / 50) return 0;
    struct line_ctx l;
    line_ctx_init(&l, pl, other, third);
    int kk = k, n;
    char buf[256];
    struct persona *p = line_source(pl, k, &kk, &l.c, g_talk_said[idx][k], buf, sizeof buf, &n);
    if (!p) return 0;
    g_talk_said[idx][k] = text_hash(buf);
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
        snprintf(dbg, sizeof dbg, "[%s] %s", k_lines[k].aud == A_ALLIES ? "allies" : player_name(other), buf);
        chat_line(pl, dbg, g_chat_ms);
    }
    if (g_log) fprintf(g_log, "# talk: P%u at %.1f min (%s%s%s%s%s): %s\n", idx, now / 60000.0, k_lines[k].key,
                       kk != (int)k ? " as " : "", kk != (int)k ? k_lines[kk].key : "", p == &g_computer ? ", mod/computer.eeai" : "",
                       heard ? "" : ", not for the player", buf);
    return 1;
}

// an attack group sets out against target's base (the route hook, attack.c): a threat to the local player when
// they are the target (attack_threat), otherwise news for the AI's allies (attack). Once per attack wave: the route
// hook runs for every leg of every subgroup, so the line waits for a launch (the next-attack time +0xED8 redrawn,
// econ_tick) and goes with the first route after it.
static void persona_attack_launched(unsigned ai) { g_attack_news[ai & 15] = 1; }
static void persona_attack(void *pl, void *target) {
    unsigned ai = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_attack_news[ai]) return;
    g_attack_news[ai] = 0;
    if (target) g_attack_sent[ai][FIELD(target, PL_INDEX, int) & 15] = game_time() ? game_time() : 1;
    unsigned h = *(unsigned *)GA(G_LOCAL_PLAYER);
    void *me = h < 16 ? ((void **)GA(G_PLAYERS))[h] : NULL;
    persona_say(pl, target && target == me ? L_ATTACK_THREAT : L_ATTACK, target, NULL);
}

// The game's own AI lines (EECPPlayerInteractionManager 0x484EB3 thiscall(pim, base id, ms), pim +0x20 = player),
// called at 0x470040 (12500 war), 0x4715F8/0x47166E/0x47280E/0x472B9D (wonders, flares): the personality's line
// where it has one. 12500: diplo.c says the war line itself (with the declaring player's name), so it is dropped
// here when the personality has one.
static char __fastcall hook_native_chat(uint8_t *pim, void *edx, int base, int ms) {
    (void)edx;
    void *pl = pim ? FIELD(pim, 0x20, void *) : NULL;
    int k = base == 12000 ? L_WONDER_THREAT : base == 12100 ? L_WONDER_HELP : base == 12200 ? L_WONDER_NOHELP :
            base == 12300 ? L_HELP_LAND : base == 12330 ? L_HELP_AIR : base == 12360 ? L_HELP_NAVAL :
            base == 12400 ? L_HELP_NONE : base == 12500 ? L_WAR_DECLARED : -1;
    if (k >= 0 && !is_vanilla(pl) && persona_has(pl, (enum line_kind)k)) {
        if (k != L_WAR_DECLARED) persona_say(pl, (enum line_kind)k, NULL, NULL);
        return 1;
    }
    return GF(FN_NATIVE_CHAT)(pim, base, ms);
}

// cmd: persona [P name]: who is who; with arguments AI P gets personality <name> (file name) from now on
static void cmd_persona(char *out, const char *args) {
    int p = -1;
    char name[40] = "";
    sscanf(args, "%d %39s", &p, name);
    if (p >= 0 && p < 16 && name[0]) {
        for (int i = 0; i < g_npersonas; i++)
            if (!lstrcmpiA(g_personas[i].file, name)) {
                if (g_players[p]) persona_apply(g_players[p], (unsigned)p, i);   // name, civilization, style as at the start
                else g_persona_of[p] = i;
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

// the mod's files aren't there (g_texts_missing): told once, at startup - not only in the log nobody reads
static DWORD WINAPI texts_missing_box(LPVOID arg) {
    (void)arg;
    char text[600];
    wsprintfA(text, "eeaimod's text files are missing: %smod\\ needs computer.eeai, prompt.eeai, units.eeai and system.eeai.\n\n"
              "The computer players play on, but without personalities, AI voice and diplomacy. Copy the mod's aimod_ai "
              "folder next to Empire Earth.exe again (or set ai_dir in aimod.ini).", g_ai_dir);
    MessageBoxA(NULL, text, "eeaimod", MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    return 0;
}
// strict parsing: the mistakes in the files, at startup
static DWORD WINAPI x_errors_box(LPVOID arg) {
    (void)arg;
    char text[2400];
    int o = wsprintfA(text, "%d mistake%s in the aimod_ai files (strict_parsing=1 in aimod.ini) - the keys are skipped:\n\n",
                      g_x_total, g_x_total == 1 ? "" : "s");
    EnterCriticalSection(&g_x_lock);
    for (int i = 0; i < g_x_queued && o < (int)sizeof text - 260; i++) o += wsprintfA(text + o, "%s\n", g_x_queue[i]);
    if (g_x_dropped) wsprintfA(text + o, "... and %d more (aimod_econ.csv, # texts:)\n", g_x_dropped);
    g_x_queued = g_x_dropped = 0;   // said: not again in the game
    LeaveCriticalSection(&g_x_lock);
    MessageBoxA(NULL, text, "eeaimod", MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    return 0;
}
// strict parsing: the mistakes found while the game runs, as system lines (from the AIs' updates: the game's thread)
static void x_errors_show(void) {
    if (!g_strict || !g_x_queued) return;
    char line[300];
    EnterCriticalSection(&g_x_lock);
    for (int i = 0; i < g_x_queued; i++) {
        snprintf(line, sizeof line, "eeaimod texts: %s", g_x_queue[i]);
        system_line(line, 30000);
    }
    if (g_x_dropped) { snprintf(line, sizeof line, "eeaimod texts: ... and %d more (aimod_econ.csv, # texts:)", g_x_dropped); system_line(line, 30000); }
    if (g_log) fprintf(g_log, "# texts: strict parsing: %d mistake(s) shown in the game\n", g_x_queued + g_x_dropped);
    g_x_queued = g_x_dropped = 0;
    LeaveCriticalSection(&g_x_lock);
}
static void persona_install(const char *ini, const char *test_ini) {
    g_ai_talk = ini_int("general", "ai_talk", 1, ini);
    g_ai_names = ini_int("general", "ai_names", 1, ini);
    g_ai_style = ini_int("general", "ai_style", 0, ini);
    g_strict = ini_int("general", "strict_parsing", 0, ini);
    InitializeCriticalSection(&g_x_lock);
    for (int i = 0; i < 16; i++) {
        char key[24];
        wsprintfA(key, "personality_P%d", i);
        GetPrivateProfileStringA("general", key, "", g_persona_force[i], sizeof g_persona_force[i], ini);
        if (test_ini) GetPrivateProfileStringA("autotest", key, g_persona_force[i], g_persona_force[i], sizeof g_persona_force[i], test_ini);
    }
    if (test_ini) {
        g_ai_talk = ini_int("autotest", "ai_talk", g_ai_talk, test_ini);
        g_ai_names = ini_int("autotest", "ai_names", g_ai_names, test_ini);
        g_ai_style = ini_int("autotest", "ai_style", g_ai_style, test_ini);
        g_strict = ini_int("autotest", "strict_parsing", g_strict, test_ini);
        g_personas_only_computer = !ini_int("autotest", "personas", 1, test_ini);
    }
    g_rnd ^= test_ini ? 0x5EED : GetTickCount();
    g_rnd_game ^= test_ini ? 0x5EED : GetTickCount() * 2654435761u;
    if (!g_rnd) g_rnd = 1;
    GetPrivateProfileStringA("general", "language", "", g_lang, sizeof g_lang, ini);
    if (test_ini) GetPrivateProfileStringA("autotest", "language", g_lang, g_lang, sizeof g_lang, test_ini);
    if (!g_lang[0]) lang_detect();
    personas_load();
    texts_load();   // mod/system.eeai now: its mistakes come up with the personalities'
    if (g_texts_missing) {   // nobody speaks: diplo_install and llm_install switch off what needs the texts
        g_npersonas = 0;
        g_has_computer = 0;
        if (g_log) fprintf(g_log, "# texts: the mod's files are missing in %smod\\ - no personalities, no AI voice, no AI diplomacy\n", g_ai_dir);
        HANDLE t = test_ini ? NULL : CreateThread(NULL, 0, texts_missing_box, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
    if (g_strict && g_x_total && !test_ini) {   // at startup: a message box (its own thread - this runs in DllMain)
        HANDLE t = CreateThread(NULL, 0, x_errors_box, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
    int okc = patch_call(GA(CALL_PLAYER_COLOR), GA(FN_COLOR_RGB), (void *)hook_player_color), ok = 0;
    for (size_t i = 0; i < sizeof GL(NATIVE_CHAT_SITES) / sizeof *GL(NATIVE_CHAT_SITES); i++) ok += patch_call(GL(NATIVE_CHAT_SITES)[i], GA(FN_NATIVE_CHAT), (void *)hook_native_chat);
    if (g_log) fprintf(g_log, "# persona: %d personalities, mod/computer.eeai %d, language '%s', talk %d, names %d, native line hooks %d/5, colour hook %d\n", g_npersonas, g_has_computer, g_lang, g_ai_talk, g_ai_names, ok, okc);
}
