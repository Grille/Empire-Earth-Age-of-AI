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

internal const uint32_t k_chat_cooldown[CHAT_KINDS] = { 0, 0, 30000, 60000, 60000, 60000, 60000, 30000, 60000, 0 };
internal int g_chat;
internal int chat_level(void) { return g_chat; }   // aimod.ini [general] chat: 0 off, 1 debug lines, 2 the status too
internal int g_chat_ms = 15000;
internal uint32_t g_chat_last[16], g_chat_kind_last[16][CHAT_KINDS];
internal unsigned g_chat_lines[16], g_chat_dropped[16];

internal const char *const k_epoch_names[15] = { "Prehistoric", "Stone", "Copper", "Bronze", "Dark", "Middle",
    "Renaissance", "Imperial", "Industrial", "Atomic - WWI", "Atomic - WWII", "Atomic - Modern", "Digital", "Nano",
    "Space" };
internal const char *epoch_name(int e) { return e >= 0 && e < 15 ? k_epoch_names[e] : "?"; }


// one chat line from player pl (name and colour from the player), shown for ms milliseconds
internal void chat_line(ee_player *pl, const char *text, int ms) {
    void *ui = game_world();
    if (!pl || !ui || !*text) return;
    __attribute__((aligned(8))) uint8_t s[32];
    wstring_init(s);
    wstring_set(s, text);
    ui_line(ui, player_color_at(pl), player_name_at(pl), s, ms);
    wstring_free(s);
}

// the game's own line for a stance change, as EEMSetDiplomaticStance::vf10 (0x4BDDCC) writes it: sprintf "%s %s %s"
// (UWideString data +4) of the player's name, string 0x7B71 "wechselt diplomatische Haltung zu" and 0x734A
// "Feindselig" / 0x7349 "Befreundet" (GetString thiscall(resource file [0x919770], id, UWideString &)), shown for
// the chat time setting ([0x917BB8] +0x2CC). Shown whatever the chat level.
// a line from nobody in particular, as the game shows "<name> wurde besiegt" (0x55BCEA): white, and as the
// sender string 0x7C45 of the language file
internal void system_line(const char *text, int ms) {
    void *res = string_resources(), *ui = game_world();
    if (!res || !ui || !*text) return;
    float white[3] = { 1.0f, 1.0f, 1.0f };
    __attribute__((aligned(8))) uint8_t name[32], line[32];
    wstring_init(name);
    wstring_init(line);
    resource_string(res, 0x7C45, name);
    wstring_set(line, text);
    ui_line(ui, white, name, line, ms);
    wstring_free(line);
    wstring_free(name);
}

internal void chat_stance_line(ee_player *pl, int stance) {
    void *res = string_resources(), *ui = game_world();
    ee_game_data *opt = game_data();
    if (!pl || !res || !ui || !opt) return;
    __attribute__((aligned(8))) uint8_t verb[32], word[32], line[32];
    wstring_init(verb);
    wstring_init(word);
    wstring_reserve(line, 0x400);
    resource_string(res, 0x7B71, verb);
    resource_string(res, stance ? 0x734A : 0x7349, word);
    wstring_format(line, "%s %s %s", ((void *)player_get_name(pl)), FIELD(verb, 4, void *), FIELD(word, 4, void *));
    ui_line(ui, player_color_at(pl), player_name_at(pl), line, game_data_get_line_ms(opt));
    wstring_free(line);
    wstring_free(word);
    wstring_free(verb);
}

// a line of kind `kind` if chat is on and the cooldowns allow it; returns 1 if it was shown
internal int ai_say(ee_player *pl, enum chat_kind kind, const char *fmt, ...) {
    if (!pl || (!g_chat && kind != CHAT_SAY)) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
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
internal int home_soldiers_now(ee_player *pl);
internal int current_epoch(ee_player *pl);
internal int g_chat_epoch[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
internal uint32_t g_chat_status[16];
internal void chat_new_game(void) {
    for (int i = 0; i < 16; i++) { g_chat_epoch[i] = -1; g_chat_status[i] = g_chat_last[i] = 0; }
    memset(g_chat_kind_last, 0, sizeof g_chat_kind_last);
    memset(g_chat_lines, 0, sizeof g_chat_lines);
    memset(g_chat_dropped, 0, sizeof g_chat_dropped);
}
internal void chat_tick(ee_player *pl, unsigned idx, uint32_t now) {
    int *last_epoch = g_chat_epoch;
    uint32_t *last_status = g_chat_status;
    if (!g_chat) return;
    int e = current_epoch(pl);
    if (last_epoch[idx] >= 0 && e != last_epoch[idx])
        ai_say(pl, CHAT_EPOCH, "[ai] now in the %s Age (%.0f min)", epoch_name(e), now / 60000.0);
    last_epoch[idx] = e;
    if (g_chat >= 2 && now / 300000 != last_status[idx] / 300000 && now >= 60000) {
        last_status[idx] = now;
        int *stock = player_stock_at(pl);
        int cap = unit_limit(pl);
        ai_say(pl, CHAT_STATUS, "[ai] %.0f min: %d at home of %d cap; food %d wood %d stone %d gold %d iron %d",
               now / 60000.0, home_soldiers_now(pl), cap, stock[2], stock[3], stock[4], stock[5], stock[6]);
    }
}

// chat [0|1|2]: show or set the level; say P text: player P writes text
internal void cmd_chat(char *out, const char *args) {
    int lv = -1;
    sscanf(args, "%d", &lv);
    if (lv >= 0) g_chat = lv;
    REPLY("chat %d:", g_chat);
    for (int i = 0; i < 16; i++)
        if (g_chat_lines[i] || g_chat_dropped[i]) REPLY(" P%d %u lines (%u dropped)", i, g_chat_lines[i], g_chat_dropped[i]);
    REPLY("\n");
}
internal void cmd_say(char *out, const char *args) {
    int p = -1, n = 0;
    sscanf(args, "%d %n", &p, &n);
    ee_player *pl = p >= 0 && p < 16 ? g_players[p] : NULL;
    if (!pl || !n) { REPLY("error: say P text (P = an AI player seen this game)\n"); return; }
    ai_say(pl, CHAT_SAY, "%s", args + n);
    REPLY("ok\n");
}

// chatme text (test): the local player types text into the chat box - as EEUIECESendMessage::vf1 (0x5C28D6) sends it:
// cheats on (0x918E9A): the cheat parser 0x520A17 (thiscall on [0x919858], UWideString *, int *code, int *arg) and a
// cheat message (new 0x18, 0x5C29FB(code, arg)); then the chat line (new 0x24, 0x5C2A2F(UWideString *)); both through
// [0x91984C] vtable +0x18 (message, 0xFF = all) - scenario triggers on chat messages and cheat codes see it as typed
internal void cmd_chatme(char *out, const char *args) {
    ee_server *net = game_server();
    while (*args == ' ') args++;
    if (!net || !*args) { REPLY("error: chatme text (in a game)\n"); return; }
    __attribute__((aligned(8))) uint8_t s[32];
    wstring_init_text(s, args);
    int code = 0, arg = 0, cheats = cheats_on() != 0;
    if (cheats && cheat_parser()) {
        cheat_parse(cheat_parser(), s, &code, &arg);
        if (code) {
            void *m = msg_cheat_alloc();
            if (m) server_send_all(net, msg_cheat_init(m, code, arg));
        }
    }
    void *m = msg_chat_alloc();
    if (m) server_send_all(net, msg_chat_init(m, s));
    wstring_free(s);
    REPLY("ok: sent \"%s\" (cheats %s, cheat code %d)\n", args, cheats ? "on" : "off", code);
}

internal void chat_install(void) {
    g_chat = g_set.chat;
    g_chat_ms = g_set.chat_seconds * 1000;
    if (g_log) fprintf(g_log, "# chat: level %d, %d s per line\n", g_chat, g_chat_ms / 1000);
}
