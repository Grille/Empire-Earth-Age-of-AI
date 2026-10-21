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
    void *ui = *(void **)GA(G_WORLD);
    if (!pl || !ui || !*text) return;
    __attribute__((aligned(8))) uint8_t s[32];
    ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_CTOR))(s);
    ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_ASSIGN))(s, text);
    GF(FN_UI_LINE)(ui, (uint8_t *)pl + 0x438,
                                                                              (uint8_t *)pl + 0x24, s, ms, 0);
    ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_DTOR))(s);
}

// the game's own line for a stance change, as EEMSetDiplomaticStance::vf10 (0x4BDDCC) writes it: sprintf "%s %s %s"
// (UWideString data +4) of the player's name, string 0x7B71 "wechselt diplomatische Haltung zu" and 0x734A
// "Feindselig" / 0x7349 "Befreundet" (GetString thiscall(resource file [0x919770], id, UWideString &)), shown for
// the chat time setting ([0x917BB8] +0x2CC). Shown whatever the chat level.
// a line from nobody in particular, as the game shows "<name> wurde besiegt" (0x55BCEA): white, and as the
// sender string 0x7C45 of the language file
static void system_line(const char *text, int ms) {
    void *res = *(void **)GA(G_RESOURCES), *ui = *(void **)GA(G_WORLD);
    if (!res || !ui || !*text) return;
    float white[3] = { 1.0f, 1.0f, 1.0f };
    __attribute__((aligned(8))) uint8_t name[32], line[32];
    void (__thiscall *ctor)(void *) = *(void **)GA(IAT_WSTRING_CTOR), (__thiscall *dtor)(void *) = *(void **)GA(IAT_WSTRING_DTOR);
    ctor(name);
    ctor(line);
    ((int (__thiscall *)(void *, int, void *))*(void **)GA(IAT_RESOURCE_STRING))(res, 0x7C45, name);
    ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_ASSIGN))(line, text);
    GF(FN_UI_LINE)(ui, white, name, line, ms, 0);
    dtor(line);
    dtor(name);
}

static void chat_stance_line(void *pl, int stance) {
    void *res = *(void **)GA(G_RESOURCES), *ui = *(void **)GA(G_WORLD);
    uint8_t *opt = *(uint8_t **)GA(G_GAME_DATA);
    if (!pl || !res || !ui || !opt) return;
    __attribute__((aligned(8))) uint8_t verb[32], word[32], line[32];
    void (__thiscall *ctor)(void *) = *(void **)GA(IAT_WSTRING_CTOR), (__thiscall *dtor)(void *) = *(void **)GA(IAT_WSTRING_DTOR);
    int (__thiscall *get)(void *, int, void *) = *(void **)GA(IAT_RESOURCE_STRING);
    ctor(verb);
    ctor(word);
    ((void (__thiscall *)(void *, unsigned long))*(void **)GA(IAT_WSTRING_RESERVE))(line, 0x400);
    get(res, 0x7B71, verb);
    get(res, stance ? 0x734A : 0x7349, word);
    ((int (__cdecl *)(void *, const char *, ...))*(void **)GA(IAT_WSTRING_FORMAT))(line, "%s %s %s", FIELD(pl, 0x28, void *),
                                                                       FIELD(verb, 4, void *), FIELD(word, 4, void *));
    GF(FN_UI_LINE)(ui, (uint8_t *)pl + 0x438,
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
static int g_chat_epoch[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static uint32_t g_chat_status[16];
static void chat_new_game(void) {
    for (int i = 0; i < 16; i++) { g_chat_epoch[i] = -1; g_chat_status[i] = g_chat_last[i] = 0; }
    memset(g_chat_kind_last, 0, sizeof g_chat_kind_last);
    memset(g_chat_lines, 0, sizeof g_chat_lines);
    memset(g_chat_dropped, 0, sizeof g_chat_dropped);
}
static void chat_tick(void *pl, unsigned idx, uint32_t now) {
    int *last_epoch = g_chat_epoch;
    uint32_t *last_status = g_chat_status;
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

// chatme text (test): the local player types text into the chat box - as EEUIECESendMessage::vf1 (0x5C28D6) sends it:
// cheats on (0x918E9A): the cheat parser 0x520A17 (thiscall on [0x919858], UWideString *, int *code, int *arg) and a
// cheat message (new 0x18, 0x5C29FB(code, arg)); then the chat line (new 0x24, 0x5C2A2F(UWideString *)); both through
// [0x91984C] vtable +0x18 (message, 0xFF = all) - scenario triggers on chat messages and cheat codes see it as typed
static void cmd_chatme(char *out, const char *args) {
    void **net = *(void ***)GA(G_SERVER);
    while (*args == ' ') args++;
    if (!net || !*args) { REPLY("error: chatme text (in a game)\n"); return; }
    __attribute__((aligned(8))) uint8_t s[32];
    ((void (__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_CTOR_C))(s, args);
    void (__thiscall *send)(void *, void *, int) = (void (__thiscall *)(void *, void *, int))(*(void ***)net)[6];
    int code = 0, arg = 0, cheats = *(char *)GA(G_CHEATS_ON) != 0;
    if (cheats && *(void **)GA(G_CHEAT_PARSER)) {
        GF(FN_CHEAT_PARSE)(*(void **)GA(G_CHEAT_PARSER), s, &code, &arg);
        if (code) {
            void *m = GF(FN_NEW)(0x18);
            if (m) send(net, GF(FN_CHEAT_MSG_CTOR)(m, code, arg), 0xFF);
        }
    }
    void *m = GF(FN_NEW)(0x24);
    if (m) send(net, GF(FN_CHAT_MSG_CTOR)(m, s), 0xFF);
    ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_DTOR))(s);
    REPLY("ok: sent \"%s\" (cheats %s, cheat code %d)\n", args, cheats ? "on" : "off", code);
}

static void chat_install(const char *ini, const char *test_ini) {
    g_chat = ini_int("general", "chat", 0, ini);
    g_chat_ms = ini_int("general", "chat_seconds", 15, ini) * 1000;
    if (test_ini) {
        g_chat = ini_int("autotest", "chat", g_chat, test_ini);
        g_chat_ms = ini_int("autotest", "chat_seconds", g_chat_ms / 1000, test_ini) * 1000;
    }
    if (g_log) fprintf(g_log, "# chat: level %d, %d s per line\n", g_chat, g_chat_ms / 1000);
}
