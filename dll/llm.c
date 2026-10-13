// LLM VOICE ([llm] enabled, default 0): a language model on this PC - LM Studio, Ollama, llama.cpp or any server with
// the OpenAI chat API (POST .../v1/chat/completions) - speaks for the AI players. Words only: every decision stays
// with the mod's rules (diplo.c); nothing of it goes into the simulation. Included by eeaimod.c after diplo.c.
//  - lines ([llm] lines=1): a personality line the local player hears (persona_say) is said in the AI's own words:
//    the same situation, with its character, the game's state (stances, strength, opinion) and its recent exchange
//    with the player. Every number of the fixed line (tribute) and the name of a third player in it must be in the
//    answer, else the fixed line is shown.
//  - chat ([llm] chat=1): a chat line of the local player gets an answer in character from one AI: the one it names
//    first (a word of 4+ letters of its name), else the last one that spoke to him within 3 minutes, else the only AI
//    left. The AI can agree to nothing in the chat: it is told what it would do if he offered peace now, so its words
//    match the rules.
// A worker thread posts the requests (WinHTTP), one at a time; answers are shown at the next simulation step
// (llm_tick, control.c), each AI's in the order it said them (at most 3 waiting per AI, else the fixed line). A line waits at most [llm] timeout seconds (queue + answer), else the fixed line shows;
// while the server can't be reached (for 60 s after a failed connection) fixed lines show at once and the chat gets
// no answer - a system line says so once per game. Only on the local PC: in multiplayer each PC asks its own model
// (or none). [personality] character= in a personality file: a few words about it for the model.
#include <winhttp.h>

#define LLM_JOBS 16
#define LLM_HIST 8
#define LLM_MAXLEN 220
static int g_llm_on, g_llm_lines = 1, g_llm_chat = 1, g_llm_timeout = 20, g_llm_max_tokens = 300, g_llm_log = 1;
static int g_llm_temp = 80;   // temperature x100
// think=0 (default): "reasoning_effort":"none" - a reasoning model answers at once (LM Studio, Orion-26B-A4B: 4 s
// instead of 15 s of thinking that ran into max_tokens with an empty answer); a server that refuses the field (HTTP
// 400/422 naming it) gets the request again without it, and from then on none
static int g_llm_think;
static volatile LONG g_llm_effort_refused;
#define LLM_NO_THINK ",\"reasoning_effort\":\"none\""
static char g_llm_url[256] = "http://localhost:1234/v1/chat/completions", g_llm_model[96], g_llm_key[128], g_llm_extra[256];
static WCHAR g_llm_host[128], g_llm_path[256];
static INTERNET_PORT g_llm_port;
static int g_llm_https;
static HANDLE g_llm_event, g_llm_thread;
static volatile LONG g_llm_gen;                 // a new game: jobs of the last one are dropped
static volatile DWORD g_llm_down_until;         // GetTickCount: the server failed, fixed lines until then
static volatile LONG g_llm_fail_note;           // the worker failed to connect: tell the player (llm_tick)
static int g_llm_told;                          // told this game
static unsigned g_llm_seq;
static unsigned g_llm_n_ok, g_llm_n_fail, g_llm_n_fixed, g_llm_n_late;
static char g_llm_last_err[160];

enum { J_FREE, J_QUEUED, J_RUNNING, J_DONE, J_FAILED };
struct llm_job {
    volatile LONG state;
    int chat;            // 0 a personality line, 1 an answer to the player's chat
    int ai, to;          // the speaker; the player it is said to (-1 everyone / allies)
    LONG gen;
    unsigned seq;        // order of the requests: an AI's lines show in the order it said them
    DWORD t0, ms;        // queued at (GetTickCount); answer time
    char *body;          // the request (JSON, malloc'd)
    char fixed[256];     // the fixed line (lines)
    char must[6][48];    // what the answer must contain (lines)
    int nmust;
    char speaker[48];    // its name (the worker reads no game state)
    char reply[LLM_MAXLEN + 32];
};
#define SET_STATE(j, s) InterlockedExchange(&(j)->state, (s))
static struct llm_job g_llm_jobs[LLM_JOBS];

// the AI's exchange with the local player: his chat lines (user) and its lines to him (assistant)
static struct { uint8_t role[LLM_HIST]; char text[LLM_HIST][LLM_MAXLEN + 32]; int n; } g_llm_hist[16];
static int g_llm_last_ai;                       // the AI that last said something to the local player
static uint32_t g_llm_last_t;

static void llm_hist_add(int ai, int role, const char *text) {
    if (ai < 0 || ai > 15 || !*text) return;
    if (g_llm_hist[ai].n == LLM_HIST) {
        memmove(g_llm_hist[ai].role, g_llm_hist[ai].role + 1, LLM_HIST - 1);
        memmove(g_llm_hist[ai].text, g_llm_hist[ai].text + 1, (LLM_HIST - 1) * sizeof g_llm_hist[ai].text[0]);
        g_llm_hist[ai].n--;
    }
    g_llm_hist[ai].role[g_llm_hist[ai].n] = (uint8_t)role;
    lstrcpynA(g_llm_hist[ai].text[g_llm_hist[ai].n++], text, LLM_MAXLEN + 32);
}

static void llm_new_game(void) {
    InterlockedIncrement(&g_llm_gen);
    memset(g_llm_hist, 0, sizeof g_llm_hist);
    g_llm_last_ai = 0;
    g_llm_last_t = 0;
    g_llm_told = 0;
}

// ---- JSON out: a growing buffer; strings in the game's code page, written as ASCII (\u escapes) ----
struct sbuf { char *p; size_t n, cap; };
static void sb_put(struct sbuf *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) {
        size_t c = b->cap ? b->cap * 2 : 4096;
        while (c < b->n + n + 1) c *= 2;
        char *q = realloc(b->p, c);
        if (!q) return;
        b->p = q;
        b->cap = c;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}
static void sb_puts(struct sbuf *b, const char *s) { sb_put(b, s, strlen(s)); }
static void sb_printf(struct sbuf *b, const char *fmt, ...) {
    char t[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(t, sizeof t, fmt, ap);
    va_end(ap);
    if (n > 0) sb_put(b, t, (size_t)n < sizeof t ? (size_t)n : sizeof t - 1);
}
static void sb_json(struct sbuf *b, const char *acp) {   // a JSON string literal
    int n = MultiByteToWideChar(CP_ACP, 0, acp, -1, NULL, 0);
    WCHAR *w = n > 0 ? malloc((size_t)n * sizeof *w) : NULL;
    sb_puts(b, "\"");
    if (w && MultiByteToWideChar(CP_ACP, 0, acp, -1, w, n))
        for (WCHAR *c = w; *c; c++) {
            char t[8];
            if (*c == '"' || *c == '\\') { t[0] = '\\'; t[1] = (char)*c; sb_put(b, t, 2); }
            else if (*c == '\n') sb_puts(b, "\\n");
            else if (*c >= 0x20 && *c < 0x7F) { t[0] = (char)*c; sb_put(b, t, 1); }
            else { snprintf(t, sizeof t, "\\u%04x", (unsigned)*c); sb_puts(b, t); }
        }
    free(w);
    sb_puts(b, "\"");
}
static void sb_msg(struct sbuf *b, const char *role, const char *text) {
    sb_printf(b, "%s{\"role\":\"%s\",\"content\":", b->p && b->p[b->n - 1] == '}' ? "," : "", role);
    sb_json(b, text);
    sb_puts(b, "}");
}

// ---- JSON in: the first "content" string after "choices" (OpenAI chat answer), as UTF-16 ----
static int hex4(const char *s) {
    int v = 0;
    for (int i = 0; i < 4; i++) {
        char c = s[i];
        v = v * 16 + (c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1000);
    }
    return v;
}
static int json_content(const char *resp, WCHAR *out, int cap) {
    const char *s = strstr(resp, "\"choices\"");
    s = s ? strstr(s, "\"content\"") : NULL;
    if (!s) return 0;
    s += 9;
    while (*s == ' ' || *s == ':' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    if (*s != '"') return 0;   // null: nothing said
    s++;
    // collect UTF-8 bytes (escapes as UTF-16 directly), then convert runs
    int o = 0;
    char u8[8];
    int nu8 = 0;
    for (; *s && *s != '"' && o < cap - 2; s++) {
        WCHAR wc = 0;
        int have = 0;
        if (*s == '\\') {
            s++;
            switch (*s) {
            case 'n': case 'r': case 't': wc = ' '; break;
            case 'u': { int v = hex4(s + 1); if (v < 0) return 0; wc = (WCHAR)v; s += 4; break; }
            case 0: return 0;
            default: wc = (WCHAR)(unsigned char)*s;
            }
            have = 1;
        } else if ((unsigned char)*s < 0x80) { wc = (WCHAR)*s; have = 1; }
        else {   // a UTF-8 sequence
            u8[nu8++] = *s;
            int need = (u8[0] & 0xE0) == 0xC0 ? 2 : (u8[0] & 0xF0) == 0xE0 ? 3 : 4;
            if (nu8 == need || nu8 == (int)sizeof u8) {
                o += MultiByteToWideChar(CP_UTF8, 0, u8, nu8, out + o, cap - 2 - o);
                nu8 = 0;
            }
            continue;
        }
        if (have) { nu8 = 0; out[o++] = wc; }
    }
    out[o] = 0;
    return 1;
}

// the answer as a chat line: no reasoning, no *actions*, no quotes or name prefix, one line, at most LLM_MAXLEN
// characters, in the game's code page
static void llm_clean(const WCHAR *in, const char *speaker, char *out, int cap) {
    const WCHAR *think = wcsstr(in, L"</think>");
    if (think) in = think + 8;
    WCHAR w[1024];
    int o = 0, star = 0;
    for (const WCHAR *c = in; *c && o < 1000; c++) {
        WCHAR ch = *c;
        if (ch == '*') { star = !star; continue; }   // *smiles* - stage directions
        if (star) continue;
        if (ch == 0x2018 || ch == 0x2019 || ch == 0x201A) ch = '\'';
        else if (ch == 0x201C || ch == 0x201D || ch == 0x201E || ch == 0xAB || ch == 0xBB || ch == '"') continue;
        else if (ch == 0x2013 || ch == 0x2014) ch = '-';
        else if (ch == 0x2026) { if (o < 997) { w[o++] = '.'; w[o++] = '.'; } ch = '.'; }
        else if (ch < 0x20 || ch == 0xA0) ch = ' ';
        else if (ch >= 0x2000 || (ch >= 0xD800 && ch <= 0xDFFF)) continue;   // emojis, symbols
        if (ch == ' ' && (!o || w[o - 1] == ' ')) continue;
        w[o++] = ch;
    }
    while (o && w[o - 1] == ' ') o--;
    w[o] = 0;
    char a[1024];
    int n = WideCharToMultiByte(CP_ACP, 0, w, -1, a, sizeof a, "?", NULL);
    if (n <= 0) { out[0] = 0; return; }
    char *s = a;
    char *colon = strchr(s, ':');   // "Khan Temur: ..." - a name prefix
    if (colon && colon - s < 40 && speaker && *speaker) {
        char pre[48];
        lstrcpynA(pre, s, (int)(colon - s) + 1 < (int)sizeof pre ? (int)(colon - s) + 1 : (int)sizeof pre);
        if (strstr(speaker, pre) || strstr(pre, speaker)) { s = colon + 1; while (*s == ' ') s++; }
    }
    int len = lstrlenA(s);
    if (len > LLM_MAXLEN) {   // the last sentence end that fits, else the last space
        int cut = 0;
        for (int i = LLM_MAXLEN - 1; i > LLM_MAXLEN / 3 && !cut; i--)
            if ((s[i] == '.' || s[i] == '!' || s[i] == '?') && s[i + 1] == ' ') cut = i + 1;
        for (int i = LLM_MAXLEN - 3; i > LLM_MAXLEN / 3 && !cut; i--)
            if (s[i] == ' ') { s[i] = '.'; s[i + 1] = '.'; s[i + 2] = '.'; cut = i + 3; }
        s[cut ? cut : LLM_MAXLEN] = 0;
    }
    lstrcpynA(out, s, cap);
}

// ---- the worker: one request at a time ----
static void llm_fail(struct llm_job *j, const char *what, DWORD err, int down) {
    snprintf(g_llm_last_err, sizeof g_llm_last_err, "%s (error %lu)", what, err);
    if (down) { g_llm_down_until = GetTickCount() + 60000; InterlockedExchange(&g_llm_fail_note, 1); }
    SET_STATE(j, J_FAILED);
}
static void llm_post(struct llm_job *j, DWORD wait_ms) {
    DWORD limit = (DWORD)g_llm_timeout * 1000u, t = GetTickCount();
again:
    if (wait_ms >= limit - 500) { g_llm_n_late++; snprintf(g_llm_last_err, sizeof g_llm_last_err, "waited too long"); SET_STATE(j, J_FAILED); return; }
    DWORD left = limit - wait_ms;
    HINTERNET ses = WinHttpOpen(L"eeaimod", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET con = ses ? WinHttpConnect(ses, g_llm_host, g_llm_port, 0) : NULL;
    HINTERNET req = con ? WinHttpOpenRequest(con, L"POST", g_llm_path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             g_llm_https ? WINHTTP_FLAG_SECURE : 0) : NULL;
    if (!req) { llm_fail(j, "open", GetLastError(), 1); goto done; }
    WinHttpSetTimeouts(req, 3000, 3000, (int)left, (int)left);
    WCHAR hdr[300] = L"Content-Type: application/json\r\n";
    if (g_llm_key[0]) {
        WCHAR k[160];
        MultiByteToWideChar(CP_ACP, 0, g_llm_key, -1, k, 160);
        lstrcatW(hdr, L"Authorization: Bearer ");
        lstrcatW(hdr, k);
        lstrcatW(hdr, L"\r\n");
    }
    DWORD blen = (DWORD)strlen(j->body);
    if (!WinHttpSendRequest(req, hdr, (DWORD)-1, j->body, blen, blen, 0)) {
        DWORD e = GetLastError();
        llm_fail(j, e == ERROR_WINHTTP_TIMEOUT ? "no answer in time" : "cannot connect", e, e != ERROR_WINHTTP_TIMEOUT);
        goto done;
    }
    if (!WinHttpReceiveResponse(req, NULL)) {
        DWORD e = GetLastError();
        if (e == ERROR_WINHTTP_TIMEOUT) g_llm_n_late++;
        llm_fail(j, e == ERROR_WINHTTP_TIMEOUT ? "no answer in time" : "no response", e, e != ERROR_WINHTTP_TIMEOUT);
        goto done;
    }
    DWORD code = 0, csz = sizeof code;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &csz,
                        WINHTTP_NO_HEADER_INDEX);
    struct sbuf r = { 0 };
    for (;;) {
        DWORD avail = 0, got = 0;
        if (!WinHttpQueryDataAvailable(req, &avail) || !avail || r.n > (1u << 20)) break;
        char tmp[8192];
        if (avail > sizeof tmp) avail = sizeof tmp;
        if (!WinHttpReadData(req, tmp, avail, &got) || !got) break;
        sb_put(&r, tmp, got);
    }
    static WCHAR w[4096];
    char *nt = strstr(j->body, LLM_NO_THINK);
    if ((code == 400 || code == 422) && r.p && strstr(r.p, "reasoning") && nt) {   // the server doesn't know the field
        memmove(nt, nt + strlen(LLM_NO_THINK), strlen(nt + strlen(LLM_NO_THINK)) + 1);
        InterlockedExchange(&g_llm_effort_refused, 1);
        free(r.p);
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);
        req = con = ses = NULL;
        wait_ms += GetTickCount() - t;
        t = GetTickCount();
        goto again;
    }
    if (code != 200 || !r.p || !json_content(r.p, w, 4096)) {
        if (r.p) { char *nl; for (nl = r.p; *nl; nl++) if (*nl == '\r' || *nl == '\n') *nl = ' '; }
        snprintf(g_llm_last_err, sizeof g_llm_last_err, "HTTP %lu: %.120s", code, r.p ? r.p : "");
        InterlockedExchange(&g_llm_fail_note, 1);
        SET_STATE(j, J_FAILED);
    } else {
        llm_clean(w, j->speaker, j->reply, sizeof j->reply);
        j->ms = GetTickCount() - t;
        if (!j->reply[0]) {
            int len = strstr(r.p, "\"length\"") != NULL, rea = strstr(r.p, "\"reasoning_content\"") != NULL;
            snprintf(g_llm_last_err, sizeof g_llm_last_err, rea ? "the model only thought, no answer%s - [llm] think=0, or "
                     "more max_tokens" : "an empty answer%s", len ? " (max_tokens reached)" : "");
            InterlockedExchange(&g_llm_fail_note, 1);
        }
        SET_STATE(j, j->reply[0] ? J_DONE : J_FAILED);
    }
    free(r.p);
done:
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
}
static DWORD WINAPI llm_worker(void *arg) {
    (void)arg;
    for (;;) {
        WaitForSingleObject(g_llm_event, 1000);
        for (;;) {
            struct llm_job *j = NULL;
            for (int i = 0; i < LLM_JOBS; i++)   // the oldest queued one
                if (g_llm_jobs[i].state == J_QUEUED && (!j || (int)(g_llm_jobs[i].seq - j->seq) < 0)) j = &g_llm_jobs[i];
            if (!j) break;
            SET_STATE(j, J_RUNNING);
            if (j->gen != g_llm_gen || (LONG)(g_llm_down_until - GetTickCount()) > 0) { SET_STATE(j, J_FAILED); continue; }
            llm_post(j, GetTickCount() - j->t0);
        }
    }
    return 0;
}

// ---- requests (game thread) ----
static const char *llm_language(void) {
    static const char *const k[][2] = { { "de", "German" }, { "fr", "French" }, { "es", "Spanish" }, { "it", "Italian" },
        { "pt", "Portuguese" }, { "pl", "Polish" }, { "ru", "Russian" }, { "cs", "Czech" }, { "zh", "Chinese" },
        { "ko", "Korean" }, { "ja", "Japanese" } };
    for (size_t i = 0; i < sizeof k / sizeof *k; i++) if (!_strnicmp(g_lang, k[i][0], 2)) return k[i][1];
    return "English";
}
// what situation k is, for the model
static const char *const k_llm_situation[L_N] = {
    "the game begins - a first word to everyone", "{name} has just declared war on you",
    "{name}, your ally until now, has betrayed you and declared war", "you accept {name}'s offer of peace (you are allied now)",
    "you refuse {name}'s offer of peace", "you offer {name} an alliance against {enemy}, your common enemy",
    "{name} has accepted your alliance offer", "your alliance offer to {name} has expired unanswered",
    "you betray {name}, your ally, and declare war on them", "{name} sent you a generous tribute",
    "{name} sent you a tribute - a small one", "your army sets out to attack {name}", "your town is under attack",
    "you lost a building", "you have just reached the {epoch} age", "an enemy is a whole age ahead of you",
    "you are winning against {name}", "you are losing the war", "you are close to defeat",
    "{name} has been defeated", "your ally {name} has been defeated", "you have been defeated",
    "a wonder is threatened", "you send help for an ally's wonder", "you have no help to give for an ally's wonder",
    "you send land troops to help an ally", "you send aircraft to help an ally", "you send ships to help an ally",
    "you have no troops to send an ally who asked for help", "you give up the game",
    "you capitulate: {name} is beating you, and you offer to PAY them a tribute ({tribute}) for peace - you give, they "
    "receive",
    "{name} accepted your capitulation: you PAY them the tribute ({tribute}) and serve them from now on",
    "{name} did not accept your capitulation: the war goes on", "your army is about to attack {name}",
    "every other player has submitted to {name}, your lord", "{name}, your lord, has won the game",
    "you refuse {name}'s peace offer because {enemy} in their alliance cannot be at peace with your side",
    "you refuse {name}'s peace offer because your own ally {enemy} won't make peace with them",
    "your lord {name} forces you to make peace with {enemy}, whom you hate",
    ("{name} offers you peace, but you are the vassal of {enemy} and have no diplomacy of your own - they must talk to "
     "{enemy}"),
};
static const char *llm_words(float v, const char *const *w) {   // -100..100 in five words
    return w[v < -50 ? 0 : v < -15 ? 1 : v <= 15 ? 2 : v <= 50 ? 3 : 4];
}
// append to t (cap bytes, *o used): never past the end, however long the names and lines are
static void tcat(char *t, int *o, int cap, const char *fmt, ...) {
    if (*o >= cap - 1) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(t + *o, (size_t)(cap - *o), fmt, ap);
    va_end(ap);
    if (n > 0) *o += n < cap - 1 - *o ? n : cap - 1 - *o;
}
// the system message: who the AI is, how it talks, the rules, and the game as it stands for the listener h
static void llm_system(struct sbuf *b, void *pl, int ai, int h) {
    struct persona *p = persona_of(pl);
    char t[4096];
    int o = 0;
    tcat(t, &o, sizeof t, "You are %s, a leader in the real-time strategy game Empire Earth (played by the "
                  "computer). ", player_name(pl));
    if (p && p->character[0]) tcat(t, &o, sizeof t, "%s ", p->character);
    tcat(t, &o, sizeof t, "Your nature (0-100): aggression %d, loyalty %d, pride %d, greed %d, forgiveness %d, "
                  "caution %d.", trait(pl, T_AGGRESSION), trait(pl, T_LOYALTY), trait(pl, T_PRIDE), trait(pl, T_GREED),
                  trait(pl, T_FORGIVENESS), trait(pl, T_CAUTION));
    // a few of its own lines, as examples of its voice (without placeholders)
    if (p) {
        int shown = 0;
        static const int ex[] = { L_GREETING, L_WAR_DECLARED, L_PEACE_REFUSE, L_WINNING, L_LOSING, L_UNDER_ATTACK, L_EPOCH_UP };
        for (size_t i = 0; i < sizeof ex / sizeof *ex && shown < 4; i++) {
            if (!p->nline[ex[i]]) continue;
            const char *l = p->line[ex[i]][rnd(p->nline[ex[i]])];
            if (strchr(l, '{')) continue;
            tcat(t, &o, sizeof t, "%s\"%s\"", shown ? " / " : " How you talk, for example: ", l);
            shown++;
        }
        if (shown) tcat(t, &o, sizeof t, ".");
    }
    tcat(t, &o, sizeof t, "\nRules: write only what %s says - one or two short sentences, at most 200 "
                  "characters, in %s. No quotation marks, no stage directions, no emojis, no name in front. Stay in "
                  "character and in the world of the game. Diplomacy happens only through the stances in the game (allied "
                  "or enemy): in the chat you agree to nothing and promise nothing, and never claim to have changed your "
                  "stance.", player_name(pl), llm_language());
    // the game as it stands
    void *ph = player_at(h);
    uint32_t now = game_time();
    char ep[96];
    epoch_text(pl, ep, sizeof ep);
    tcat(t, &o, sizeof t, "\nThe game now (minute %u): you are in the %s.", now / 60000u, ep);
    if (ph && h != ai) {
        static const char *const op[5] = { "you hate them", "you dislike them", "you are indifferent to them",
                                           "you like them", "you trust them" };
        static const char *const pw[5] = { "much weaker than yours", "weaker than yours", "about as strong as yours",
                                           "stronger than yours", "much stronger than yours" };
        epoch_text(ph, ep, sizeof ep);
        int mine = stance_of(pl, h), his = stance_of(ph, ai);
        float r = power_of(h) / power_of(ai);
        const char *hn = *player_name(ph) && *player_name(ph) != '?' ? player_name(ph) : "The player";
        tcat(t, &o, sizeof t, " %s (the player you talk to) is in the %s. %s Their forces are %s. "
                      "Opinion: %s.", hn, ep,
                      !mine && !his ? "You are allied with each other." : mine && his ? "You are at war with each other."
                      : mine ? "You treat them as an enemy, but they have set allied towards you (they offer peace)."
                             : "You are allied with them, but they treat you as an enemy.",
                      llm_words(r < 0.25f ? -100 : r < 0.67f ? -30 : r <= 1.5f ? 0 : r <= 4 ? 30 : 100, pw),
                      llm_words(g_opinion[ai][h], op));
        if (g_pressure[ai][h] >= 10) tcat(t, &o, sizeof t, " Their soldiers are in your town right now.");
        const uint32_t *mk = g_kills[ai][h], *tk = g_kills[h][ai];
        uint32_t rm[K_N], rt[K_N];
        kills_recent(ai, h, rm);
        kills_recent(h, ai, rt);
        int any = 0, all_recent = 1, none_recent = 1;
        for (int c = 0; c < K_N; c++) {
            any |= mk[c] || tk[c];
            all_recent &= rm[c] == mk[c] && rt[c] == tk[c];
            none_recent &= !rm[c] && !rt[c];
        }
        if (any) {
            tcat(t, &o, sizeof t, " The fighting between you so far: you killed %u of their soldiers and %u of their "
                          "citizens (workers) and destroyed %u of their buildings; they killed %u of your soldiers and %u of "
                          "your citizens and destroyed %u of your buildings.", mk[K_SOLDIERS], mk[K_CITIZENS],
                          mk[K_BUILDINGS], tk[K_SOLDIERS], tk[K_CITIZENS], tk[K_BUILDINGS]);
            if (all_recent)
                tcat(t, &o, sizeof t, " All of it in the last %d minutes.", KILLS_RECENT_MIN);
            else if (none_recent)
                tcat(t, &o, sizeof t, " No fighting between you in the last %d minutes.", KILLS_RECENT_MIN);
            else
                tcat(t, &o, sizeof t, " In the last %d minutes: you killed %u soldiers and %u citizens and "
                              "destroyed %u buildings of theirs; they killed %u soldiers and %u citizens and destroyed %u "
                              "buildings of yours.", KILLS_RECENT_MIN, rm[K_SOLDIERS], rm[K_CITIZENS], rm[K_BUILDINGS],
                              rt[K_SOLDIERS], rt[K_CITIZENS], rt[K_BUILDINGS]);
        }
        if (g_lord[ai] == h) tcat(t, &o, sizeof t, " You have capitulated to them: they are your lord, you pay "
                                                                "and serve them.");
        if (g_lord[h] == ai) tcat(t, &o, sizeof t, " They have capitulated to you: they are your vassal.");
        if (mine && !TEAMS_LOCKED) {
            int yes = peace_score(pl, ai, h, now) > 0 && !is_vassal(ai);
            tcat(t, &o, sizeof t, " If they offered peace now (set allied towards you), you would %s.",
                          yes ? "accept" : "refuse");
        }
        if (TEAMS_LOCKED) tcat(t, &o, sizeof t, " The teams are fixed in this game: no stance can change.");
    }
    int others = 0;
    for (int j = 1; j < nplayers() && o < (int)sizeof t - 120; j++) {
        void *pj = player_at(j);
        if (!pj || j == ai || j == h || is_out(pj)) continue;
        tcat(t, &o, sizeof t, "%s%s (%s)", others++ ? ", " : " Other players: ", player_name(pj),
                      stance_of(pl, j) ? "your enemy" : "your ally");
    }
    if (others && o < (int)sizeof t - 2) tcat(t, &o, sizeof t, ".");
    sb_msg(b, "system", t);
}
static void llm_history(struct sbuf *b, int ai) {
    for (int i = 0; i < g_llm_hist[ai].n; i++) sb_msg(b, g_llm_hist[ai].role[i] ? "assistant" : "user", g_llm_hist[ai].text[i]);
}
static char *llm_finish(struct sbuf *b) {
    sb_puts(b, "]");
    if (g_llm_model[0]) { sb_puts(b, ",\"model\":"); sb_json(b, g_llm_model); }
    sb_printf(b, ",\"max_tokens\":%d,\"temperature\":%.2f,\"stream\":false", g_llm_max_tokens, g_llm_temp / 100.0);
    if (!g_llm_think && !g_llm_effort_refused) sb_puts(b, LLM_NO_THINK);
    if (g_llm_extra[0]) { sb_puts(b, ","); sb_puts(b, g_llm_extra); }
    sb_puts(b, "}");
    return b->p;
}
static struct llm_job *llm_slot(void) {
    for (int i = 0; i < LLM_JOBS; i++) if (g_llm_jobs[i].state == J_FREE) return &g_llm_jobs[i];
    return NULL;
}
static int llm_ready(void) { return g_llm_on && g_llm_thread && (LONG)(g_llm_down_until - GetTickCount()) <= 0; }
static void llm_queue(struct llm_job *j, char *body) {
    j->body = body;
    lstrcpynA(j->speaker, g_players[j->ai & 15] ? player_name(g_players[j->ai & 15]) : "", sizeof j->speaker);
    j->gen = g_llm_gen;
    j->t0 = GetTickCount();
    j->seq = ++g_llm_seq;
    SET_STATE(j, J_QUEUED);
    SetEvent(g_llm_event);
}
static void llm_subst(char *out, int cap, const char *s, void *pl, void *other, void *third) {
    int o = 0;
    char ep[96];
    while (*s && o < cap - 1) {
        const char *rep = NULL;
        if (!strncmp(s, "{name}", 6)) { rep = other && *player_name(other) ? player_name(other) : "the player"; s += 6; }
        else if (!strncmp(s, "{enemy}", 7)) { rep = third ? player_name(third) : "the enemy"; s += 7; }
        else if (!strncmp(s, "{epoch}", 7)) { epoch_text(pl, ep, sizeof ep); rep = ep; s += 7; }
        else if (!strncmp(s, "{tribute}", 9)) { tribute_text(g_say_tribute, ep, sizeof ep); rep = ep; s += 9; }
        if (rep) while (*rep && o < cap - 1) out[o++] = *rep++;
        else out[o++] = *s++;
    }
    out[o] = 0;
}

// persona_say: a line the local player hears - 1 if the model says it (later), 0: show the fixed line now
static int llm_line(void *pl, enum line_kind k, void *other, void *third, const char *fixed) {
    if (!llm_ready() || !g_llm_lines || !pl) return 0;
    int ai = pidx(pl), h = (int)*(unsigned *)0x00918E84;
    int pending = 0;
    for (int i = 0; i < LLM_JOBS; i++) pending += g_llm_jobs[i].state != J_FREE && g_llm_jobs[i].ai == ai;
    if (pending >= 3) return 0;   // the model can't keep up with this AI: its fixed line
    struct llm_job *j = llm_slot();
    if (!j) return 0;
    memset(j, 0, sizeof *j);
    j->ai = ai;
    j->to = other ? pidx(other) : -1;
    lstrcpynA(j->fixed, fixed, sizeof j->fixed);
    for (const char *s = fixed; *s && j->nmust < 5;) {   // every number of the fixed line
        if (*s >= '0' && *s <= '9') {
            int n = 0;
            while (s[n] >= '0' && s[n] <= '9' && n < 40) n++;
            lstrcpynA(j->must[j->nmust++], s, n + 1);
            s += n;
        } else s++;
    }
    if (third && strstr(fixed, player_name(third)) && j->nmust < 6) lstrcpynA(j->must[j->nmust++], player_name(third), 48);
    struct sbuf b = { 0 };
    sb_puts(&b, "{\"messages\":[");
    llm_system(&b, pl, ai, h);
    llm_history(&b, ai);
    char sit[400], u[1200];
    llm_subst(sit, sizeof sit, k_llm_situation[k] ? k_llm_situation[k] : k_lines[k].key, pl, other, third);
    const char *aud = k_lines[k].aud == A_ALL ? "to everyone" : k_lines[k].aud == A_ALLIES ? "to your allies" : "to them";
    snprintf(u, sizeof u, "(Not a chat line - an event of the game: %s. Say something about it %s, in your own words, in "
             "%s. The usual line would be: \"%s\" - keep its meaning%s.)", sit, aud, llm_language(), fixed,
             j->nmust ? " and every name and number in it" : "");
    sb_msg(&b, "user", u);
    if (!b.p) return 0;
    llm_queue(j, llm_finish(&b));
    return 1;
}

static int word_in(const char *text, const char *name) {   // where a word (4+ letters) of name is in text, or -1
    char lt[300], ln[64];
    lstrcpynA(lt, text, sizeof lt);
    lstrcpynA(ln, name, sizeof ln);
    CharLowerA(lt);   // the code page's case (umlauts too)
    CharLowerA(ln);
    int best = -1;
    for (const char *s = ln; *s;) {
        while (*s == ' ' || *s == '-') s++;
        int n = 0;
        while (s[n] && s[n] != ' ' && s[n] != '-') n++;
        for (const char *t = lt; n >= 4 && *t; t++) {
            if (strncmp(t, s, n)) continue;
            unsigned char before = t > lt ? (unsigned char)t[-1] : ' ', after = (unsigned char)t[n];
            if (!IsCharAlphaA((char)before) && !IsCharAlphaA((char)after) && (best < 0 || t - lt < best)) best = (int)(t - lt);
        }
        s += n;
    }
    return best;
}
// the local player wrote `text` in the chat: the AI it is meant for answers (the one named first)
static void llm_player_chat(const char *text) {
    if (!g_llm_on || !g_llm_chat || !text || !*text || victory_word(text)) return;
    int h = (int)*(unsigned *)0x00918E84, to[1], n = 0, alive = 0, only = 0, at = 1 << 30;
    uint32_t now = game_time();
    for (int i = 1; i < nplayers(); i++) {
        void *pi = player_at(i);
        if (!pi || !g_players[i] || is_out(pi) || i == h) continue;
        alive++;
        only = i;
        int w = word_in(text, player_name(pi));
        if (w >= 0 && w < at) { at = w; to[0] = i; n = 1; }
    }
    if (!n && g_llm_last_ai && now - g_llm_last_t < 180000 && g_players[g_llm_last_ai] && !is_out(g_players[g_llm_last_ai]))
        to[n++] = g_llm_last_ai;
    if (!n && alive == 1) to[n++] = only;
    if (!n) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line meant for no AI (name one): %s\n", text);
        return;
    }
    for (int k = 0; k < n; k++) llm_hist_add(to[k], 0, text);
    if (!llm_ready()) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line for P%d - the server is down, no answer\n", to[0]);
        return;
    }
    for (int k = 0; k < n; k++) {
        struct llm_job *j = llm_slot();
        if (!j) return;
        memset(j, 0, sizeof *j);
        j->chat = 1;
        j->ai = to[k];
        j->to = h;
        struct sbuf b = { 0 };
        sb_puts(&b, "{\"messages\":[");
        llm_system(&b, g_players[to[k]], to[k], h);
        llm_history(&b, to[k]);   // ends with his line
        if (!b.p) return;
        llm_queue(j, llm_finish(&b));
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line for P%d asked: %s\n", to[k], text);
    }
}

// every simulation step: show what is answered
static void llm_tick(void) {
    if (!g_llm_on) return;
    if (InterlockedExchange(&g_llm_fail_note, 0) && !g_llm_told) {
        g_llm_told = 1;
        char l[400];
        snprintf(l, sizeof l, !lstrcmpiA(g_lang, "de") ? "KI-Stimme: keine Antwort von %s (%s) - die Computerspieler "
                 "sprechen ihre festen Zeilen." : "AI voice: no answer from %s (%s) - the computer players say their fixed "
                 "lines.", g_llm_url, g_llm_last_err);
        uint8_t *opt = *(uint8_t **)0x00917BB8;
        system_line(l, opt ? FIELD(opt, 0x2CC, int) * 2 : 20000);
    }
    for (int i = 0; i < LLM_JOBS; i++) {
        struct llm_job *j = &g_llm_jobs[i];
        LONG st = InterlockedCompareExchange(&j->state, J_FREE, J_FREE);   // read with a barrier: the reply is complete
        if (st != J_DONE && st != J_FAILED) continue;
        int older = 0;   // an earlier request of the same AI still open: this one waits for it
        for (int o = 0; o < LLM_JOBS; o++)
            older |= o != i && g_llm_jobs[o].state != J_FREE && g_llm_jobs[o].ai == j->ai && (int)(g_llm_jobs[o].seq - j->seq) < 0;
        if (older) continue;
        void *pl = j->ai > 0 && j->ai < 16 ? g_players[j->ai] : NULL;
        int ok = j->state == J_DONE;
        for (int m = 0; ok && m < j->nmust; m++)
            if (!strstr(j->reply, j->must[m])) {
                if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d answer lacks \"%s\" - its fixed line instead (answer: %s)\n", j->ai, j->must[m], j->reply);
                ok = 0;
            }
        if (j->gen == g_llm_gen && pl) {
            const char *say = ok ? j->reply : j->chat ? NULL : j->fixed;
            if (ok) g_llm_n_ok++; else if (j->state == J_FAILED) g_llm_n_fail++;
            if (!ok && !j->chat) g_llm_n_fixed++;
            if (say) {
                chat_line(pl, say, g_chat_ms);
                if (j->to == (int)*(unsigned *)0x00918E84 || j->chat) {
                    llm_hist_add(j->ai, 1, say);
                    g_llm_last_ai = j->ai;
                    g_llm_last_t = game_time();
                }
            }
            if (g_llm_log && g_log) {
                if (ok) fprintf(g_log, "# llm: P%d %s (%.1f s): %s\n", j->ai, j->chat ? "answers" : "says", j->ms / 1000.0, j->reply);
                else if (j->state == J_FAILED) fprintf(g_log, "# llm: P%d no answer (%s)%s%s\n", j->ai, g_llm_last_err,
                                                       j->chat ? "" : " - the fixed line: ", j->chat ? "" : j->fixed);
            }
        }
        free(j->body);
        j->body = NULL;
        SET_STATE(j, J_FREE);
    }
}

// a line said to the local player with its fixed text (no model): part of the exchange too
static void llm_heard(void *pl, void *other, const char *text) {
    if (!g_llm_on || !pl || !other || other != player_at((int)*(unsigned *)0x00918E84)) return;
    llm_hist_add(pidx(pl), 1, text);
    g_llm_last_ai = pidx(pl);
    g_llm_last_t = game_time();
}

// cmd: llm - state; llm say P TEXT: the local player's chat line TEXT meant for AI P (without sending it; tests)
static void cmd_llm(char *out, const char *args) {
    int p = 0, n = 0;
    if (sscanf(args, "say %d %n", &p, &n) == 1 && n) {
        void *pl = player_at(p);
        if (!pl || !g_players[p]) { REPLY("error: P%d is no AI\n", p); return; }
        g_llm_last_ai = p;
        g_llm_last_t = game_time();
        llm_player_chat(args + n);
        REPLY("asked\n");
        return;
    }
    int q = 0;
    for (int i = 0; i < LLM_JOBS; i++) q += g_llm_jobs[i].state != J_FREE;
    REPLY("llm %s, url %s, model '%s', lines %d, chat %d, timeout %d s; %s; answers %u, failed %u (late %u), fixed lines "
          "%u, pending %d, think %d%s%s%s\n", g_llm_on ? "on" : "off", g_llm_url, g_llm_model, g_llm_lines, g_llm_chat, g_llm_timeout,
          (LONG)(g_llm_down_until - GetTickCount()) > 0 ? "DOWN" : "up", g_llm_n_ok, g_llm_n_fail, g_llm_n_late,
          g_llm_n_fixed, q, g_llm_think, g_llm_effort_refused ? " (refused by the server)" : "", g_llm_last_err[0] ? "; last error: " : "", g_llm_last_err);
}

static void llm_install(const char *ini, const char *test_ini) {
    g_llm_on = GetPrivateProfileIntA("llm", "enabled", 0, ini);
    g_llm_lines = GetPrivateProfileIntA("llm", "lines", 1, ini);
    g_llm_chat = GetPrivateProfileIntA("llm", "chat", 1, ini);
    g_llm_timeout = GetPrivateProfileIntA("llm", "timeout", 20, ini);
    g_llm_max_tokens = GetPrivateProfileIntA("llm", "max_tokens", 300, ini);
    g_llm_temp = GetPrivateProfileIntA("llm", "temperature", 80, ini);
    g_llm_log = GetPrivateProfileIntA("llm", "log", 1, ini);
    g_llm_think = GetPrivateProfileIntA("llm", "think", 0, ini);
    GetPrivateProfileStringA("llm", "url", g_llm_url, g_llm_url, sizeof g_llm_url, ini);
    GetPrivateProfileStringA("llm", "model", "", g_llm_model, sizeof g_llm_model, ini);
    GetPrivateProfileStringA("llm", "api_key", "", g_llm_key, sizeof g_llm_key, ini);
    GetPrivateProfileStringA("llm", "extra", "", g_llm_extra, sizeof g_llm_extra, ini);
    if (test_ini) {
        g_llm_on = GetPrivateProfileIntA("autotest", "llm", g_llm_on, test_ini);
        g_llm_lines = GetPrivateProfileIntA("autotest", "llm_lines", g_llm_lines, test_ini);
        g_llm_chat = GetPrivateProfileIntA("autotest", "llm_chat", g_llm_chat, test_ini);
        g_llm_timeout = GetPrivateProfileIntA("autotest", "llm_timeout", g_llm_timeout, test_ini);
        GetPrivateProfileStringA("autotest", "llm_url", g_llm_url, g_llm_url, sizeof g_llm_url, test_ini);
        GetPrivateProfileStringA("autotest", "llm_model", g_llm_model, g_llm_model, sizeof g_llm_model, test_ini);
    }
    if (g_llm_timeout < 2) g_llm_timeout = 2;
    if (g_llm_timeout > 120) g_llm_timeout = 120;
    if (g_llm_max_tokens < 16) g_llm_max_tokens = 16;
    if (g_llm_max_tokens > 8000) g_llm_max_tokens = 8000;
    if (!g_llm_on) { if (g_log) fprintf(g_log, "# llm: off\n"); return; }
    WCHAR wurl[256];
    MultiByteToWideChar(CP_ACP, 0, g_llm_url, -1, wurl, 256);
    URL_COMPONENTS uc;
    memset(&uc, 0, sizeof uc);
    uc.dwStructSize = sizeof uc;
    uc.lpszHostName = g_llm_host;
    uc.dwHostNameLength = 128;
    uc.lpszUrlPath = g_llm_path;
    uc.dwUrlPathLength = 256;
    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) {
        if (g_log) fprintf(g_log, "# llm: bad url '%s' - off\n", g_llm_url);
        g_llm_on = 0;
        return;
    }
    g_llm_port = uc.nPort;
    g_llm_https = uc.nScheme == INTERNET_SCHEME_HTTPS;
    g_llm_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    g_llm_thread = CreateThread(NULL, 0, llm_worker, NULL, 0, NULL);
    if (g_log) fprintf(g_log, "# llm: on - %s, model '%s', lines %d, chat %d, timeout %d s\n", g_llm_url, g_llm_model,
                       g_llm_lines, g_llm_chat, g_llm_timeout);
}
