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
// ageofai.ini [general] chat = 0 off (default), 1 debug lines, 2 also the 5-minute status line; console: chat [level],
// say P text. Every line also goes to the log (# chat:). Per player at most one line per 4 s and per kind one per
// kind's cooldown, the rest is dropped (not queued: a late line would be wrong).

internal const uint32_t k_chat_cooldown[CHAT_KINDS] = { 0, 0, 30000, 60000, 60000, 60000, 60000, 30000, 60000, 0 };
internal int g_chat;
internal int chat_level(void) { return g_chat; }   // ageofai.ini [general] chat: 0 off, 1 debug lines, 2 the status too
internal int g_chat_ms = 15000;
internal uint32_t g_chat_last[16], g_chat_kind_last[16][CHAT_KINDS];
internal unsigned g_chat_lines[16], g_chat_dropped[16];

internal const char *const k_epoch_names[15] = { "Prehistoric", "Stone", "Copper", "Bronze", "Dark", "Middle",
    "Renaissance", "Imperial", "Industrial", "Atomic - WWI", "Atomic - WWII", "Atomic - Modern", "Digital", "Nano",
    "Space" };
internal const char *epoch_name(int e) { return e >= 0 && e < 15 ? k_epoch_names[e] : "?"; }


// one chat line from player pl (name and colour from the player), shown for ms milliseconds
internal void chat_line(ee_player *pl, const char *text, int ms) {
    void *ui = game_get_ui();
    if (!pl || !ui || !*text) return;
    __attribute__((aligned(8))) uint8_t s[32];
    wstring_init(s);
    wstring_set(s, text);
    ui_show_line(ui, player_color_at(pl), player_name_at(pl), s, ms);
    wstring_free(s);
}

// the game's own line for a stance change, as EEMSetDiplomaticStance::vf10 (0x4BDDCC) writes it: sprintf "%s %s %s"
// (UWideString data +4) of the player's name, string 0x7B71 "wechselt diplomatische Haltung zu" and 0x734A
// "Feindselig" / 0x7349 "Befreundet" (GetString thiscall(resource file [0x919770], id, UWideString &)), shown for
// the chat time setting ([0x917BB8] +0x2CC). Shown whatever the chat level.
// a line from nobody in particular, as the game shows "<name> wurde besiegt" (0x55BCEA): white, and as the
// sender string 0x7C45 of the language file
internal void system_line(const char *text, int ms) {
    void *res = string_resources(), *ui = game_get_ui();
    if (!res || !ui || !*text) return;
    float white[3] = { 1.0f, 1.0f, 1.0f };
    __attribute__((aligned(8))) uint8_t name[32], line[32];
    wstring_init(name);
    wstring_init(line);
    resource_string(res, 0x7C45, name);
    wstring_set(line, text);
    ui_show_line(ui, white, name, line, ms);
    wstring_free(line);
    wstring_free(name);
}

// the same with no sender at all: a line that is only its text
internal void bare_line(const char *text, int ms) {
    void *ui = game_get_ui();
    if (!ui || !*text) return;
    float white[3] = { 1.0f, 1.0f, 1.0f };
    __attribute__((aligned(8))) uint8_t name[32], line[32];
    wstring_init(name);
    wstring_init(line);
    wstring_set(name, "");
    wstring_set(line, text);
    ui_show_line(ui, white, name, line, ms);
    wstring_free(line);
    wstring_free(name);
}

internal void chat_stance_line(ee_player *pl, int stance) {
    void *res = string_resources(), *ui = game_get_ui();
    ee_game_data *opt = game_data();
    if (!pl || !res || !ui || !opt) return;
    __attribute__((aligned(8))) uint8_t verb[32], word[32], line[32];
    wstring_init(verb);
    wstring_init(word);
    wstring_reserve(line, 0x400);
    resource_string(res, 0x7B71, verb);
    resource_string(res, stance ? 0x734A : 0x7349, word);
    wstring_format(line, "%s %s %s", ((void *)player_get_name(pl)), FIELD(verb, 4, void *), FIELD(word, 4, void *));
    ui_show_line(ui, player_color_at(pl), player_name_at(pl), line, game_data_get_line_ms(opt));
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

// every AI update (econ_tick): epoch changes, the 5-minute status line (chat=status)
internal int home_soldiers_now(ee_player *pl);
internal int current_epoch(ee_player *pl);
internal int g_chat_epoch[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
internal uint32_t g_chat_status[16];
internal volatile int g_speed_steps = 1;   // the + and - keys above the game's own speeds: so many times the steps a second (hook_speed_msg)
internal volatile int g_speed_fast;        // that was used in this game: hook_speed_send
internal void chat_new_game(void) {
    g_speed_steps = 1, g_speed_fast = 0;
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
        cheat_codes_parse(cheat_parser(), s, &code, &arg);
        if (code) {
            void *m = msg_cheat_alloc();
            if (m) server_send_all(net, cheat_msg_init(m, code, arg));
        }
    }
    void *m = msg_chat_alloc();
    if (m) server_send_all(net, chat_msg_init(m, s));
    wstring_free(s);
    REPLY("ok: sent \"%s\" (cheats %s, cheat code %d)\n", args, cheats ? "on" : "off", code);
}

// The game speed told when the + and - keys change it (the player, 2026-11-09). Up to 2.5 the game's own speed: longer steps.
// Above it the same steps as at 2.5, more of them a second (the player: "the last delta scaled value from the game is 2.5
// evrething after should be handled by steps"; "that they might not be reached is a given"): the game plays what it plays
// at 2.5, only faster. The message says the speed wanted and comes back on every PC, also at the end of the range.
#define SPEED_LINE_MS 3000
#define SPEED_STEP_TOP 2.5f
internal float speed_shown(void) { return g_speed_steps > 1 ? SPEED_STEP_TOP * (float)g_speed_steps : *game_speed_var(); }
internal void __thiscall hook_speed_msg(ee_speed_msg *msg) {
    float want = speed_msg_get_speed(msg);
    g_speed_steps = want > SPEED_STEP_TOP + 0.01f ? (int)(want / SPEED_STEP_TOP + 0.5f) : 1;
    if (g_speed_steps > 1) g_speed_fast = 1, speed_msg_set_speed(msg, SPEED_STEP_TOP);
    speed_msg_execute(msg);
    char line[16];
    float speed = speed_shown();
    snprintf(line, sizeof line, speed < 0.5f ? "x%.2f" : speed < 10.0f ? "x%.1f" : "x%.0f", speed);
    bare_line(line, SPEED_LINE_MS);
}

// The speeds of the + and - keys (the player, 2026-11-09; the game's own: 0.5 to 2.5 by 0.5). The event stays the game's, its
// checks and its message: for the one press the game data's limits and step are those that make its sum the next speed.
internal const float k_speed_steps[] = { 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, SPEED_STEP_TOP, 5.0f, 10.0f, 20.0f, 40.0f };
internal uint32_t __thiscall hook_speed_key(eeuiemg_change_game_speed *ev, ee_ui_form *form, uint32_t arg) {
    ee_game_data *opt = game_data();
    if (!opt) return eeuiemg_change_game_speed_execute(ev, form, arg);
    int n = (int)(sizeof k_speed_steps / sizeof *k_speed_steps);
    float cur = speed_shown(), now = *game_speed_var(), to = cur;
    int up = eeuiemg_change_game_speed_get_up(ev);
    if (up) {
        for (int i = n - 1; i >= 0 && k_speed_steps[i] > cur + 0.01f; i--) to = k_speed_steps[i];
    } else {
        for (int i = 0; i < n && k_speed_steps[i] < cur - 0.01f; i++) to = k_speed_steps[i];
    }
    float lo = game_data_get_speed_min(opt), hi = game_data_get_speed_max(opt), step = game_data_get_speed_step(opt);
    game_data_set_speed_min(opt, 0.0f), game_data_set_speed_max(opt, to + 1.0f), game_data_set_speed_step(opt, up ? to - now : now - to);
    uint32_t done = eeuiemg_change_game_speed_execute(ev, form, arg);
    game_data_set_speed_min(opt, lo), game_data_set_speed_max(opt, hi), game_data_set_speed_step(opt, step);
    return done;
}

// The simulation's wait before its steps (period 22 ms, up to 2 steps a pass): so many times shorter, by the clock so that
// the time of the steps themselves doesn't slow the pace; with no step granted yet a short wait, as turbo's.
internal long __thiscall hook_speed_wait(void *ev, unsigned long ms) {
    int k = g_speed_steps;
    ee_physics *phys = game_physics();
    if (k <= 1 || !phys) return ts_wait(ev, ms);
    if (!physics_get_steps_granted(phys)) return ts_wait(ev, 1);
    static LONGLONG next;
    LARGE_INTEGER now, f;
    QueryPerformanceCounter(&now), QueryPerformanceFrequency(&f);
    LONGLONG period = f.QuadPart * physics_get_tick_ms(phys) / (1000 * k);
    if (now.QuadPart < next) Sleep((DWORD)((next - now.QuadPart) * 1000 / f.QuadPart));
    next = (now.QuadPart > next + period * 4 ? now.QuadPart : next) + period;
    return 0;
}
// The server's wait for its next tick of 15 steps: so many times shorter. Its time is reckoned from when the tick was due, which
// the shorter waits leave far ahead: once fast, never longer than a tick, or it would stand for as long after coming back.
#define SPEED_TICK_MS 350u
internal DWORD WINAPI hook_speed_send(DWORD n, const HANDLE *h, BOOL all, DWORD ms) {
    DWORD most = SPEED_TICK_MS / (DWORD)g_speed_steps;
    if (ms != INFINITE && g_speed_fast && ms > most) ms = most;
    DWORD r = WaitForMultipleObjects(n, h, all, ms);
    if (r != WAIT_TIMEOUT || g_speed_steps <= 1 || g_mp) return r;
    // In single player no tick while the last one's steps aren't used up (the player, 2026-11-09: "x10, x20 can queue steps far
    // into the furture, making the game unresponsive"): a speed the machine can't reach is left out, not owed; one tick, 15 steps, ahead at most.
    for (;;) {
        ee_server *srv = game_server();
        ee_physics *phys = game_physics();
        if (!srv || !phys || game_paused() || !game_time() ||
            (server_get_sent(srv) == server_get_done(srv) && physics_get_steps_granted(phys) == 0)) return WAIT_TIMEOUT;
        r = WaitForMultipleObjects(n, h, all, 1);
        if (r != WAIT_TIMEOUT) return r;
    }
}

internal void chat_install(void) {
    g_chat = g_set.chat;
    g_chat_ms = g_set.chat_seconds * 1000;
    if (g_log) fprintf(g_log, "# chat: level %d, %d s per line\n", g_chat, g_chat_ms / 1000);
    if ((!patch_msg_game_speed(hook_speed_msg) || !patch_speed_key(hook_speed_key)) && g_log) fprintf(g_log, "# chat: the game speed keys' hooks FAILED\n");
    if (!g_autotest && (!patch_sim_wait(hook_speed_wait) || !patch_send_wait(hook_speed_send)) && g_log) fprintf(g_log, "# chat: the fast speeds' hooks FAILED\n");   // a test has its own at both places (turbo)
}
