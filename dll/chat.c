// AI CHAT: the AI players write chat lines, for now as a debug channel (what the AI decides: attacks, epochs,
// defences, home guard, siege), later for taunts and comments. Included by eeaimod.c after settings.c.
//
// The game's own AI chat: EECPPlayerInteractionManager 0x484EB3 thiscall(pim, base string id, display ms) picks one
// of 10 string-table variants (base + 0..9, not the one used last for that base) and shows it through 0x484FD8:
//   12000 a wonder is threatened, 12100 / 12200 help for an ally's wonder / none to give, 12300 / 12330 / 12360 land /
//   air / naval help sent on an ally's flare, 12400 no help to send, 12500 a player declared war on the AI
//   ("Sie wollen ein Problem? Sie HABEN ein Problem!") - all from diplomacy, wonders and flares, nothing about the game.
// The chat line itself: 0x661E91 thiscall(ui = [0x91977C], const int colour[3] = player +0x438, UWideString *name =
//   player +0x24, UWideString *text, display ms, 0, ret 0x14) copies both strings into a 0x3C-byte message and posts
//   it (TSThread::Post 0x418) to the UI thread - so any thread may call it. The AI passes 15000 ms.
// UWideString (Low-Level Engine.dll, used through EE.exe's import table): ctor [0x8231A4], operator=(const char *)
//   [0x823224], dtor [0x823AEC]; the message keeps 0x14 bytes per string.
//
// aimod.ini [general] chat = 0 off (default), 1 debug lines, 2 also the 5-minute status line; console: chat [level],
// say P text. Every line also goes to the log (# chat:). Per player at most one line per 4 s and per kind one per
// kind's cooldown, the rest is dropped (not queued: a late line would be wrong).

enum chat_kind { CHAT_SAY, CHAT_EPOCH, CHAT_ATTACK, CHAT_ROUTE, CHAT_TOWER, CHAT_WALL, CHAT_SIEGE, CHAT_COUNTER,
                 CHAT_GUARD, CHAT_STATUS, CHAT_KINDS };
static const uint32_t k_chat_cooldown[CHAT_KINDS] = { 0, 0, 30000, 60000, 60000, 60000, 60000, 30000, 60000, 0 };
static int g_chat;
static int g_chat_ms = 15000;
static uint32_t g_chat_last[16], g_chat_kind_last[16][CHAT_KINDS];
static unsigned g_chat_lines[16], g_chat_dropped[16];

static const char *const k_epoch_names[15] = { "Prehistoric", "Stone", "Copper", "Bronze", "Dark", "Middle",
    "Renaissance", "Imperial", "Industrial", "Atomic - WWI", "Atomic - WWII", "Atomic - Modern", "Digital", "Nano",
    "Space" };
static const char *epoch_name(int e) { return e >= 0 && e < 15 ? k_epoch_names[e] : "?"; }

static void persona_attack(void *pl, void *target);   // persona.c: the attack line

// one chat line from player pl (name and colour from the player), shown for ms milliseconds
static void chat_line(void *pl, const char *text, int ms) {
    void *ui = *(void **)0x0091977C;
    if (!pl || !ui || !*text) return;
    __attribute__((aligned(8))) uint8_t s[32];
    ((void (__thiscall *)(void *))*(void **)0x008231A4)(s);
    ((void *(__thiscall *)(void *, const char *))*(void **)0x00823224)(s, text);
    ((void (__thiscall *)(void *, void *, void *, void *, int, int))0x00661E91)(ui, (uint8_t *)pl + 0x438,
                                                                              (uint8_t *)pl + 0x24, s, ms, 0);
    ((void (__thiscall *)(void *))*(void **)0x00823AEC)(s);
}

// the game's own line for a stance change, as EEMSetDiplomaticStance::vf10 (0x4BDDCC) writes it: sprintf "%s %s %s"
// (UWideString data +4) of the player's name, string 0x7B71 "wechselt diplomatische Haltung zu" and 0x734A
// "Feindselig" / 0x7349 "Befreundet" (GetString thiscall(resource file [0x919770], id, UWideString &)), shown for
// the chat time setting ([0x917BB8] +0x2CC). Shown whatever the chat level.
static void chat_stance_line(void *pl, int stance) {
    void *res = *(void **)0x00919770, *ui = *(void **)0x0091977C;
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    if (!pl || !res || !ui || !opt) return;
    __attribute__((aligned(8))) uint8_t verb[32], word[32], line[32];
    void (__thiscall *ctor)(void *) = *(void **)0x008231A4, (__thiscall *dtor)(void *) = *(void **)0x00823AEC;
    int (__thiscall *get)(void *, int, void *) = *(void **)0x0082319C;
    ctor(verb);
    ctor(word);
    ((void (__thiscall *)(void *, unsigned long))*(void **)0x008235D0)(line, 0x400);
    get(res, 0x7B71, verb);
    get(res, stance ? 0x734A : 0x7349, word);
    ((int (__cdecl *)(void *, const char *, ...))*(void **)0x008232C8)(line, "%s %s %s", FIELD(pl, 0x28, void *),
                                                                       FIELD(verb, 4, void *), FIELD(word, 4, void *));
    ((void (__thiscall *)(void *, void *, void *, void *, int, int))0x00661E91)(ui, (uint8_t *)pl + 0x438,
                                                                              (uint8_t *)pl + 0x24, line, FIELD(opt, 0x2CC, int), 0);
    dtor(line);
    dtor(word);
    dtor(verb);
}

// a line of kind `kind` if chat is on and the cooldowns allow it; returns 1 if it was shown
static int ai_say(void *pl, enum chat_kind kind, const char *fmt, ...) {
    if (!pl || (!g_chat && kind != CHAT_SAY)) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t now = game_time();
    if (kind != CHAT_SAY) {
        uint32_t *kl = &g_chat_kind_last[idx][kind];
        if ((g_chat_last[idx] && now - g_chat_last[idx] < 4000) ||
            (*kl && k_chat_cooldown[kind] && now - *kl < k_chat_cooldown[kind])) {
            g_chat_dropped[idx]++;
            return 0;
        }
        *kl = now ? now : 1;
    }
    g_chat_last[idx] = now ? now : 1;
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    chat_line(pl, buf, g_chat_ms);
    g_chat_lines[idx]++;
    if (g_log) fprintf(g_log, "# chat: P%u at %.1f min: %s\n", idx, now / 60000.0, buf);
    return 1;
}

// every AI update (econ_tick): epoch changes, the 5-minute status line (chat=2)
static int home_soldiers_now(void *pl);
static int current_epoch(void *pl);
static void chat_tick(void *pl, unsigned idx, uint32_t now) {
    static int last_epoch[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    static uint32_t last_status[16];
    if (!g_chat) return;
    int e = current_epoch(pl);
    if (last_epoch[idx] >= 0 && e != last_epoch[idx])
        ai_say(pl, CHAT_EPOCH, "[ai] now in the %s Age (%.0f min)", epoch_name(e), now / 60000.0);
    last_epoch[idx] = e;
    if (g_chat >= 2 && now / 300000 != last_status[idx] / 300000 && now >= 60000) {
        last_status[idx] = now;
        int *stock = &FIELD(pl, 0xAF0, int);
        int cap = ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0);
        ai_say(pl, CHAT_STATUS, "[ai] %.0f min: %d at home of %d cap; food %d wood %d stone %d gold %d iron %d",
               now / 60000.0, home_soldiers_now(pl), cap, stock[2], stock[3], stock[4], stock[5], stock[6]);
    }
}

// chat [0|1|2]: show or set the level; say P text: player P writes text
static void cmd_chat(char *out, const char *args) {
    int lv = -1;
    sscanf(args, "%d", &lv);
    if (lv >= 0) g_chat = lv;
    REPLY("chat %d:", g_chat);
    for (int i = 0; i < 16; i++)
        if (g_chat_lines[i] || g_chat_dropped[i]) REPLY(" P%d %u lines (%u dropped)", i, g_chat_lines[i], g_chat_dropped[i]);
    REPLY("\n");
}
static void cmd_say(char *out, const char *args) {
    int p = -1, n = 0;
    sscanf(args, "%d %n", &p, &n);
    void *pl = p >= 0 && p < 16 ? g_players[p] : NULL;
    if (!pl || !n) { REPLY("error: say P text (P = an AI player seen this game)\n"); return; }
    ai_say(pl, CHAT_SAY, "%s", args + n);
    REPLY("ok\n");
}

static void chat_install(const char *ini, const char *test_ini) {
    g_chat = GetPrivateProfileIntA("general", "chat", 0, ini);
    g_chat_ms = GetPrivateProfileIntA("general", "chat_seconds", 15, ini) * 1000;
    if (test_ini) {
        g_chat = GetPrivateProfileIntA("autotest", "chat", g_chat, test_ini);
        g_chat_ms = GetPrivateProfileIntA("autotest", "chat_seconds", g_chat_ms / 1000, test_ini) * 1000;
    }
    if (g_log) fprintf(g_log, "# chat: level %d, %d s per line\n", g_chat, g_chat_ms / 1000);
}
