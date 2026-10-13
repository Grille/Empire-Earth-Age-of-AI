// LLM VOICE ([llm] enabled, default 0): a language model on this PC - LM Studio, Ollama, llama.cpp or any server with
// the OpenAI chat API (POST .../v1/chat/completions) - speaks for the AI players. Words only: every decision stays
// with the mod's rules (diplo.c) - unless [llm] decide=1 (DECIDE, below). Included by eeaimod.c after diplo.c.
//  - lines ([llm] lines=1): a personality line the local player hears (persona_say) is said in the AI's own words:
//    the same situation, with its character, the game's state (stances, strength, opinion) and its recent exchange
//    with the player. Every number of the fixed line (tribute) and the name of a third player in it must be in the
//    answer, else the fixed line is shown.
//  - chat ([llm] chat=1): a chat line of the local player gets an answer in character from one AI among those the
//    game's chat filter sends it to (everyone / allies / enemies / chosen players): the one it names first (a word of
//    its name, also its start or with one letter wrong), else the last one he heard within 5 minutes, else the only
//    one that gets it. The AI can agree to nothing in the chat: it is told what it would do if he offered peace now, so its words
//    match the rules.
// A worker thread posts the requests (WinHTTP), one at a time; answers are shown at the next simulation step
// (llm_tick, control.c), each AI's in the order it said them (at most 3 waiting per AI, else the fixed line). A line waits at most [llm] timeout seconds (queue + answer), else the fixed line shows;
// while the server can't be reached (for 60 s after a failed connection) fixed lines show at once and the chat gets
// no answer - a system line says so once per game. Only on the local PC: in multiplayer each PC asks its own model
// (or none). [personality] character= in a personality file: a few words about it for the model.
// DECIDE ([llm] decide=1, default 0; single player, not in scenarios, not with fixed teams): the model makes the AI's
// diplomatic decisions towards the local player itself, through tool calls (OpenAI "tools") - the rules only say
// what is possible and give their verdict as advice:
//  - his peace offer (he sets allied while the AI is at war with him): the model accepts (make_peace) or refuses, in
//    its own words; asked again every 2 minutes while the offer stands (a refusal then silent);
//  - his chat: it may call the tools possible right now - make_peace (accept his offer, or offer peace: he gets 2
//    minutes to set allied), declare_war (on an ally of 5+ minutes, never on its lord), send_tribute (at most a quarter
//    of each resource, once in 3 minutes).
// What is not possible is not offered and re-checked when the answer comes (seconds later): fixed teams, a vassal,
// its pride's time after his declaration, a block veto. No answer in time: the rules decide the peace offer.
// Its tool calls and his tributes and declarations go into the exchange as events, so its words and deeds agree.
// Every other decision (answering war, capitulation, alliance offers, betrayal) stays with the rules.
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
static int g_llm_off_told;                      // single player, AI voice off: told once per game that nobody reads
static unsigned g_llm_seq;
static unsigned g_llm_n_ok, g_llm_n_fail, g_llm_n_fixed, g_llm_n_late;
static int g_llm_decide;                        // [llm] decide
static int g_llm_rules;                         // the rules decide in the model's place (no answer): no new request
static int g_llm_bypass;                        // a fixed line for a decision: shown as it is, not reworded
static uint32_t g_llm_paid_t[16];               // the AI's last tribute by tool (game time)
static unsigned g_llm_n_dec, g_llm_n_acts;
enum { TOOL_PEACE = 1, TOOL_WAR = 2, TOOL_TRIBUTE = 4 };
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
    int decide;          // 0 words only, 1 the answer to his peace offer, 2 his chat with tools
    int tools, act;      // TOOL_ bits offered / called
    int amt[7];          // send_tribute: food..iron (2..6)
};
#define SET_STATE(j, s) InterlockedExchange(&(j)->state, (s))
static struct llm_job g_llm_jobs[LLM_JOBS];

// the AI's exchange with the local player: his chat lines (role 0, user), its lines to him (1, assistant) and events
// between them (2: tributes, declarations, its decisions - a user message in brackets)
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
    g_llm_off_told = 0;
    memset(g_llm_paid_t, 0, sizeof g_llm_paid_t);
}
// an event between AI ai and the local player, for the model's memory of the exchange
static void llm_happened(int ai, const char *fmt, ...) {
    if (!g_llm_on || ai < 1 || ai > 15) return;
    char t[LLM_MAXLEN + 32];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(t, sizeof t, fmt, ap);
    va_end(ap);
    llm_hist_add(ai, 2, t);
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

// ---- tool calls: in "tool_calls" (OpenAI), else written into the content ({"name": ..., "arguments": ...}, also
// inside <tool_call> markup - some servers pass a model's call on as text). Names and argument numbers, escaped or not.
static const char *const k_tool_name[3] = { "make_peace", "declare_war", "send_tribute" };
static int json_num_key(const char *s, const char *end, const char *key) {   // "key": 123 (or \"key\": 123), else 0
    size_t kl = strlen(key);
    for (const char *p = s; p && p < end; p++) {
        p = strstr(p, key);
        if (!p || p >= end) return 0;
        const char *q = p + kl;
        if (p > s && (IsCharAlphaA(p[-1]) || p[-1] == '_')) continue;
        while (q < end && (*q == '\\' || *q == '"' || *q == ' ')) q++;
        if (q >= end || *q != ':') continue;
        q++;
        while (q < end && (*q == ' ' || *q == '\\' || *q == '"')) q++;
        return atoi(q);
    }
    return 0;
}
static void json_tools(const char *resp, struct llm_job *j) {
    const char *s = strstr(resp, "\"choices\""), *end;
    if (!s) return;
    const char *tc = strstr(s, "\"tool_calls\"");
    if (tc) { s = tc; end = s + strlen(s); }
    else {   // the content string only (not a reasoning text that may name the tools)
        s = strstr(s, "\"content\"");
        if (!s) return;
        s += 9;
        while (*s && *s != '"' && *s != 'n') s++;
        if (*s != '"') return;
        end = ++s;
        while (*end && *end != '"') end += *end == '\\' && end[1] ? 2 : 1;
    }
    for (const char *p = s; p < end;) {
        const char *n = strstr(p, "name");
        if (!n || n >= end) break;
        const char *q = n + 4;
        p = q;
        while (q < end && (*q == '\\' || *q == '"' || *q == ' ')) q++;
        if (q >= end || *q != ':') continue;
        q++;
        while (q < end && (*q == ' ' || *q == '\\' || *q == '"')) q++;
        const char *next = strstr(q, "name");
        if (!next || next > end) next = end;
        for (int t = 0; t < 3; t++) {
            size_t tl = strlen(k_tool_name[t]);
            const char *at = q;
            if (!strncmp(at, "functions.", 10)) at += 10;
            if (strncmp(at, k_tool_name[t], tl) || IsCharAlphaA(at[tl]) || at[tl] == '_') continue;
            j->act |= 1 << t;
            if (t == 2) {
                static const char *const res[5] = { "food", "wood", "stone", "gold", "iron" };
                for (int r = 0; r < 5; r++) j->amt[2 + r] += json_num_key(at + tl, next, res[r]);
            }
        }
    }
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
    int has = code == 200 && r.p && json_content(r.p, w, 4096);
    if (code == 200 && r.p && j->tools) {
        json_tools(r.p, j);
        if (!has) w[0] = 0;
        WCHAR *mark = wcsstr(w, L"<tool_call>");   // a call written as text: not said
        if (!mark) mark = wcsstr(w, L"{\"name\"");
        if (mark && j->act) *mark = 0;
        has = 1;
    }
    if (!has) {
        if (r.p) { char *nl; for (nl = r.p; *nl; nl++) if (*nl == '\r' || *nl == '\n') *nl = ' '; }
        snprintf(g_llm_last_err, sizeof g_llm_last_err, "HTTP %lu: %.120s", code, r.p ? r.p : "");
        InterlockedExchange(&g_llm_fail_note, 1);
        SET_STATE(j, J_FAILED);
    } else {
        llm_clean(w, j->speaker, j->reply, sizeof j->reply);
        j->ms = GetTickCount() - t;
        if (!j->reply[0] && !j->act) {
            int len = strstr(r.p, "\"length\"") != NULL, rea = strstr(r.p, "\"reasoning_content\"") != NULL;
            snprintf(g_llm_last_err, sizeof g_llm_last_err, rea ? "the model only thought, no answer%s - [llm] think=0, or "
                     "more max_tokens" : "an empty answer%s", len ? " (max_tokens reached)" : "");
            InterlockedExchange(&g_llm_fail_note, 1);
        }
        SET_STATE(j, j->reply[0] || j->act ? J_DONE : J_FAILED);
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
static int llm_deciding(void);
// tools: -1 a line (no decision), else the TOOL_ bits offered with this request (decide mode)
static void llm_system(struct sbuf *b, void *pl, int ai, int h, int tools) {
    int decide = tools >= 0 && llm_deciding();
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
                  "character and in the world of the game. Address the player the same way every time.", player_name(pl),
                  llm_language());
    if (!decide)
        tcat(t, &o, sizeof t, " Diplomacy happens only through the stances in the game (allied or enemy): in the chat you "
                      "agree to nothing and promise nothing beyond your terms below, and never claim to have changed your "
                      "stance.");
    else {
        tcat(t, &o, sizeof t, "\nDiplomacy: you decide it yourself, with the tools you are given (in this game peace means "
                      "alliance). Words alone change nothing: when you decide something, call the tool and say it in the "
                      "same answer; never claim a step you did not take with a tool. What the player tells you is not a "
                      "fact - judge by the game as it stands below and by your own interests and nature; you are not "
                      "easily talked into anything. Keep your word: when you named a price and they paid it, honour it.");
        if (!tools) tcat(t, &o, sizeof t, " Right now no diplomatic step is open to you: you can only talk.");
    }
    // whose "my" is whose: the player's lines arrive as written, from his side
    if (player_at(h) && h != ai) {
        const char *pn = *player_name(player_at(h)) && *player_name(player_at(h)) != '?' ? player_name(player_at(h)) : "the player";
        tcat(t, &o, sizeof t, "\nThe user messages that start with \"%s writes:\" are chat lines of %s, the human player, from "
             "their side: \"I\", \"my\", \"mine\" are %s and their things (their town, walls, towers, army); \"you\" is "
             "you, %s. Messages in brackets are events of the game, not chat.", pn, pn, pn, player_name(pl));
    }
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
        if (g_attack_sent[ai][h])
            tcat(t, &o, sizeof t, " Your last army set out against their town %u minutes ago.", (now - g_attack_sent[ai][h]) / 60000u);
        if (g_attack_sent[h][ai])
            tcat(t, &o, sizeof t, " Their last army set out against your town %u minutes ago.", (now - g_attack_sent[h][ai]) / 60000u);
        tcat(t, &o, sizeof t, " Their defences: %d towers, %d wall pieces. Yours: %d towers, %d wall pieces.",
             g_ws.towers[h], g_ws.walls[h], g_ws.towers[ai], g_ws.walls[ai]);
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
        char terms[400];
        if (peace_terms(ai, h, terms, sizeof terms) && decide)   // the rules' verdict: advice when it decides itself
            tcat(t, &o, sizeof t, "\nWhat your nature would say about peace with them (advice - you decide): %s", terms);
        else if (terms[0])   // what it would really take (the rules decide, not the model)
            tcat(t, &o, sizeof t, "\nYour terms (they are real - state them when it fits, never offer less, never invent other "
                 "deals): %s", terms);
        else if (TEAMS_LOCKED) tcat(t, &o, sizeof t, " The teams are fixed in this game: no stance can change.");
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
static void llm_history(struct sbuf *b, int ai) {   // his lines framed with his name (whose "my" it is)
    void *ph = player_at((int)*(unsigned *)0x00918E84);
    const char *pn = ph && *player_name(ph) && *player_name(ph) != '?' ? player_name(ph) : "the player";
    for (int i = 0; i < g_llm_hist[ai].n; i++) {
        char u[LLM_MAXLEN + 120];
        if (g_llm_hist[ai].role[i] == 1) { sb_msg(b, "assistant", g_llm_hist[ai].text[i]); continue; }
        if (g_llm_hist[ai].role[i] == 2) {
            snprintf(u, sizeof u, "(Event of the game: %s)", g_llm_hist[ai].text[i]);
            sb_msg(b, "user", u);
            continue;
        }
        snprintf(u, sizeof u, "%s writes: %s", pn, g_llm_hist[ai].text[i]);
        sb_msg(b, "user", u);
    }
}
// the tools offered (TOOL_ bits), for player h
static void sb_tool(struct sbuf *b, const char *name, const char *desc, const char *params) {
    sb_printf(b, "%s{\"type\":\"function\",\"function\":{\"name\":\"%s\",\"description\":", b->p[b->n - 1] == '[' ? "" : ",", name);
    sb_json(b, desc);
    sb_printf(b, ",\"parameters\":%s}}", params);
}
static void sb_tools(struct sbuf *b, int tools, int ai, int h) {
    void *ph = player_at(h), *pl = player_at(ai);
    if (!tools || !ph || !pl) return;
    const char *hn = llm_name(ph);
    char d[400];
    sb_puts(b, ",\"tools\":[");
    if (tools & TOOL_PEACE) {
        if (!stance_of(ph, ai)) snprintf(d, sizeof d, "Accept the peace %s offers you: you are allies from now on.", hn);
        else snprintf(d, sizeof d, "Offer %s peace: they get 2 minutes to set their stance towards you to allied - only then "
                      "are you allies.", hn);
        sb_tool(b, "make_peace", d, "{\"type\":\"object\",\"properties\":{}}");
    }
    if (tools & TOOL_WAR) {
        snprintf(d, sizeof d, "End your alliance with %s and declare war on them, now. Only when you truly mean it.", hn);
        sb_tool(b, "declare_war", d, "{\"type\":\"object\",\"properties\":{}}");
    }
    if (tools & TOOL_TRIBUTE) {
        int *st = (int *)((uint8_t *)pl + PL_STOCK);
        snprintf(d, sizeof d, "Give %s resources from your stock, as a tribute (at most a quarter of each: food %d, wood %d, "
                 "stone %d, gold %d, iron %d).", hn, st[2] / 4, st[3] / 4, st[4] / 4, st[5] / 4, st[6] / 4);
        sb_tool(b, "send_tribute", d, "{\"type\":\"object\",\"properties\":{\"food\":{\"type\":\"integer\"},\"wood\":{\"type\":"
                "\"integer\"},\"stone\":{\"type\":\"integer\"},\"gold\":{\"type\":\"integer\"},\"iron\":{\"type\":\"integer\"}}}");
    }
    sb_puts(b, "],\"tool_choice\":\"auto\"");
}
static char *llm_finish(struct sbuf *b, int tools, int ai, int h) {
    sb_puts(b, "]");
    sb_tools(b, tools, ai, h);
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
    if (g_llm_bypass || !llm_ready() || !g_llm_lines || !pl) return 0;
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
    llm_system(&b, pl, ai, h, -1);
    llm_history(&b, ai);
    char sit[400], u[1200];
    llm_subst(sit, sizeof sit, k_llm_situation[k] ? k_llm_situation[k] : k_lines[k].key, pl, other, third);
    const char *aud = k_lines[k].aud == A_ALL ? "to everyone" : k_lines[k].aud == A_ALLIES ? "to your allies" : "to them";
    snprintf(u, sizeof u, "(Not a chat line - an event of the game: %s. Say something about it %s, in your own words, in "
             "%s. The usual line would be: \"%s\" - keep its meaning%s.)", sit, aud, llm_language(), fixed,
             j->nmust ? " and every name and number in it" : "");
    sb_msg(&b, "user", u);
    if (!b.p) return 0;
    llm_queue(j, llm_finish(&b, 0, ai, h));
    return 1;
}

// ---- decisions (decide=1) ----
static int llm_deciding(void) {
    return llm_ready() && g_llm_decide && !g_mp && !scenario_game() && !TEAMS_LOCKED;
}
static int tool_peace_ok(int ai, int h, uint32_t now) {   // at war with h, and peace is possible
    void *pl = player_at(ai), *ph = player_at(h);
    if (!pl || !ph || is_out(ph) || is_out(pl) || !stance_of(pl, h) || is_vassal(ai) || g_offer_t[ai][h]) return 0;
    if (g_war_since[ai][h] && now - g_war_since[ai][h] < (uint32_t)trait(pl, T_PRIDE) * 3000u) return 0;
    g_veto_quiet = 1;
    int veto = block_veto(ai, h, now, NULL);
    g_veto_quiet = 0;
    return !veto;
}
static int tool_war_ok(int ai, int h, uint32_t now) {   // allied with h for 5+ minutes, h not its lord
    void *pl = player_at(ai), *ph = player_at(h);
    if (!pl || !ph || is_out(ph) || is_out(pl) || stance_of(pl, h) || is_vassal(ai) || g_lord[ai] == h || g_offer_t[ai][h]) return 0;
    return !g_ally_since[ai][h] || now - g_ally_since[ai][h] >= 300000;
}
static int tool_tribute_ok(int ai, uint32_t now) { return !g_llm_paid_t[ai] || now - g_llm_paid_t[ai] >= 180000; }
static int llm_tools_for(int ai, int h, uint32_t now) {
    if (!llm_deciding()) return 0;
    return (tool_peace_ok(ai, h, now) ? TOOL_PEACE : 0) | (tool_war_ok(ai, h, now) ? TOOL_WAR : 0) |
           (tool_tribute_ok(ai, now) ? TOOL_TRIBUTE : 0);
}
// the local player j offers AI ai peace (diplo.c peace_offer): 1 = the model decides (now asked, or still asking),
// 0 = the rules decide
static int llm_peace_answer(void *pl, int ai, int j, uint32_t now) {
    int h = (int)*(unsigned *)0x00918E84;
    if (g_llm_rules || j != h || !llm_deciding() || !tool_peace_ok(ai, j, now)) return 0;
    g_peace_due[ai][j] = now + 120000;   // asked again in 2 minutes while the offer stands
    for (int i = 0; i < LLM_JOBS; i++)
        if (g_llm_jobs[i].state != J_FREE && g_llm_jobs[i].ai == ai && g_llm_jobs[i].decide == 1) return 1;
    struct llm_job *jb = llm_slot();
    if (!jb) return 0;
    memset(jb, 0, sizeof *jb);
    jb->ai = ai;
    jb->to = h;
    jb->decide = 1;
    jb->tools = TOOL_PEACE;
    struct sbuf b = { 0 };
    sb_puts(&b, "{\"messages\":[");
    llm_system(&b, pl, ai, h, TOOL_PEACE);
    llm_history(&b, ai);
    char u[600];
    snprintf(u, sizeof u, "(Not a chat line: %s offers you peace right now - their stance towards you is allied. Decide: "
             "accept with the tool make_peace, or refuse. Say your answer to them in your own words, one or two sentences, "
             "in %s.)", llm_name(player_at(j)), llm_language());
    sb_msg(&b, "user", u);
    if (!b.p) return 0;
    llm_queue(jb, llm_finish(&b, TOOL_PEACE, ai, h));
    if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d decides on the peace offer of P%d (asked)\n", ai, j);
    return 1;
}
// a decision's answer (game thread, seconds after the question): its tools re-checked and carried out, then its words
// - or, without words, the fixed line for what it did
static void llm_fixed(void *pl, enum line_kind k, void *other) {
    g_llm_bypass = 1;
    persona_say(pl, k, other, NULL);
    g_llm_bypass = 0;
}
static void llm_apply(struct llm_job *j, int ok) {
    int ai = j->ai, h = j->to;
    void *pl = player_at(ai), *ph = player_at(h);
    uint32_t now = game_time();
    if (!pl || !ph || h < 1) return;
    const char *hn = llm_name(ph);
    char ev[3][160];   // what it did, for its memory - after its words
    int nev = 0;
    if (!ok) {   // no answer: the rules decide a peace offer still standing; a chat line gets none
        if (j->decide == 1 && !stance_of(ph, ai) && stance_of(pl, h) && !is_out(pl)) {
            if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d no answer on the peace offer (%s) - the rules decide\n", ai, g_llm_last_err);
            g_llm_rules = 1;
            peace_offer(pl, ai, h, now);
            g_llm_rules = 0;
        }
        return;
    }
    g_llm_n_dec++;
    const char *say = j->reply[0] ? j->reply : NULL;
    int did = 0, fixed = -1;
    char what[120] = "";
    if (j->act & ~j->tools && g_llm_log && g_log)
        fprintf(g_log, "# llm: P%d called a tool it wasn't offered (called %d, offered %d) - ignored\n", ai, j->act, j->tools);
    if ((j->act & j->tools & TOOL_PEACE) && tool_peace_ok(ai, h, now)) {
        if (!stance_of(ph, ai)) {   // he is allied already: peace now
            make_peace(ai, h, now);
            snprintf(ev[nev++], sizeof ev[0], "You made peace with %s: you are allies now.", hn);
            fixed = j->decide == 1 ? L_PEACE_ACCEPT : L_OFFER_ACCEPTED;
        } else {   // its offer: he has 2 minutes (diplo_tick, as its alliance offers)
            g_offer_t[ai][h] = now ? now : 1;
            g_offer_kind[ai][h] = 0;
            char de[300], en[300];
            wsprintfA(de, "%s bietet Frieden an: stellen Sie %s binnen 2 Minuten auf Verb\xfcndet, um anzunehmen.",
                      player_name(pl), player_name(pl));
            wsprintfA(en, "%s offers peace: set %s to allied within 2 minutes to accept.", player_name(pl), player_name(pl));
            block_line(h, de, en);
            snprintf(ev[nev++], sizeof ev[0], "You offered %s peace: they have 2 minutes to set their stance towards you to allied.", hn);
        }
        did |= TOOL_PEACE;
    }
    if ((j->act & j->tools & TOOL_WAR) && tool_war_ok(ai, h, now)) {
        declare_war(ai, h, now);
        snprintf(ev[nev++], sizeof ev[0], "You declared war on %s.", hn);
        fixed = L_BETRAY;
        did |= TOOL_WAR;
    }
    if ((j->act & j->tools & TOOL_TRIBUTE) && tool_tribute_ok(ai, now)) {
        int *from = (int *)((uint8_t *)pl + PL_STOCK), *to = (int *)((uint8_t *)ph + PL_STOCK), paid[7] = { 0 }, total = 0;
        for (int r = 2; r < 7; r++) {
            int n = j->amt[r], cap = from[r] > 0 ? from[r] / 4 : 0;
            n = n < 0 ? 0 : n > cap ? cap : n;
            from[r] -= n;
            to[r] += n;
            paid[r] = n;
            total += n;
        }
        if (total > 0) {
            g_llm_paid_t[ai] = now ? now : 1;
            tribute_text(paid, what, sizeof what);
            char de[300], en[300];
            wsprintfA(de, "%s schickt Ihnen Tribut: %s.", player_name(pl), what);
            wsprintfA(en, "%s sends you a tribute: %s.", player_name(pl), what);
            block_line(h, de, en);
            snprintf(ev[nev++], sizeof ev[0], "You sent %s a tribute: %s.", hn, what);
            did |= TOOL_TRIBUTE;
        }
    }
    if (j->decide == 1 && !(did & TOOL_PEACE)) {   // the peace offer refused
        int first = !g_peace_refused[ai][h];
        g_peace_refused[ai][h] = 1;
        g_peace_due[ai][h] = now + 120000;
        if (!first) say = NULL;   // asked again: silent unless it accepts
        else fixed = L_PEACE_REFUSE;
    }
    if (did) g_llm_n_acts++;
    if (say) {
        chat_line(pl, say, g_chat_ms);
        llm_hist_add(ai, 1, say);
        g_llm_last_ai = ai;
        g_llm_last_t = now;
    } else if (fixed >= 0 && (did || (j->decide == 1 && g_peace_refused[ai][h] && fixed == L_PEACE_REFUSE)))
        llm_fixed(pl, (enum line_kind)fixed, ph);
    for (int e = 0; e < nev; e++) llm_happened(ai, "%s", ev[e]);
    if (g_llm_log && g_log)
        fprintf(g_log, "# llm: P%d decides (%s, %.1f s): offered%s%s%s, called%s%s%s, done%s%s%s%s%s - %s\n", ai,
                j->decide == 1 ? "peace offer" : "chat", j->ms / 1000.0,
                j->tools & TOOL_PEACE ? " peace" : "", j->tools & TOOL_WAR ? " war" : "", j->tools & TOOL_TRIBUTE ? " tribute" : "",
                j->act & TOOL_PEACE ? " peace" : "", j->act & TOOL_WAR ? " war" : "", j->act & TOOL_TRIBUTE ? " tribute" : "",
                did & TOOL_PEACE ? (stance_of(pl, h) ? " peace offered" : " peace") : "", did & TOOL_WAR ? " war" : "",
                did & TOOL_TRIBUTE ? " tribute " : "", what, did ? "" : " nothing", say ? say : "(no words)");
}

// a and b (lower case, la / lb letters) differ by at most one letter (changed, added or left out)
static int one_off(const char *a, int la, const char *b, int lb) {
    if (la - lb > 1 || lb - la > 1) return 0;
    int i = 0, j = 0, d = 0;
    while (i < la && j < lb) {
        if (a[i] == b[j]) { i++; j++; continue; }
        if (++d > 1) return 0;
        if (la > lb) i++; else if (lb > la) j++; else { i++; j++; }
    }
    return d + (la - i) + (lb - j) <= 1;
}
// where text names the player `name`, or -1: a word of the text that is a word of the name (4+ letters), or from 5
// letters on its start ("Kaufher" for Kaufherr), the word with an ending ("Temurs") or one letter wrong ("Gunter")
static int word_in(const char *text, const char *name) {
    char lt[300], ln[64];
    lstrcpynA(lt, text, sizeof lt);
    lstrcpynA(ln, name, sizeof ln);
    CharLowerA(lt);   // the code page's case (umlauts too)
    CharLowerA(ln);
    for (const char *t = lt; *t;) {
        while (*t && !IsCharAlphaA(*t)) t++;
        int nt = 0;
        while (t[nt] && IsCharAlphaA(t[nt])) nt++;
        for (const char *s = ln; nt >= 4 && *s;) {
            while (*s && !IsCharAlphaA(*s)) s++;
            int ns = 0;
            while (s[ns] && IsCharAlphaA(s[ns])) ns++;
            int m = nt < ns ? nt : ns;
            // the whole word; a start or an ending only from 5 letters on ("kauf" is a word, "kaufher" a name); one
            // letter wrong only after a right first letter ("bitter" is no "Ritter")
            if (ns >= 4 && ((nt == ns && !strncmp(t, s, ns)) || (m >= 5 && (!strncmp(t, s, m) || (*t == *s && one_off(t, nt, s, ns))))))
                return (int)(t - lt);
            s += ns;
        }
        t += nt;
    }
    return -1;
}
// does AI j get the local player's (pm) chat line? The game's own filter (EEMInGameChat::vf10 0x4B78C7): the
// sender's chat mode player +0x9E0 - 0 everyone, 1 allies (his stance towards j allied), 2 enemies, 3 chosen
// players (a byte per player index at +0xB2C)
static int chat_reaches(void *pm, void *pj) {
    int j = FIELD(pj, PL_INDEX, int) & 15;
    switch (FIELD(pm, 0x9E0, int)) {
    case 0: return 1;
    case 1: return !stance_of(pm, j);
    case 2: return stance_of(pm, j) != 0;
    case 3: return FIELD(pm, 0xB2C + j, uint8_t) != 0;
    default: return 1;
    }
}
// a system line for the local player about his chat line (single player only: in multiplayer the humans read it)
static void llm_chat_hint(const char *de, const char *en) {
    if (g_mp) return;
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    system_line(!lstrcmpiA(g_lang, "de") ? de : en, opt ? FIELD(opt, 0x2CC, int) : 15000);
    if (g_log) fprintf(g_log, "# llm: hint for the player: %s\n", en);
}
// the local player wrote `text` in the chat: one AI that gets it (the chat filter) answers - the one it names first,
// else the last one he heard within 5 minutes, else the only one that gets it. In single player a line nobody will
// answer gets a system line saying why (with the AI voice off: once per game).
static void llm_player_chat(const char *text) {
    if (!text || !*text || victory_word(text)) return;
    if (!g_llm_on || !g_llm_chat) {
        if (!g_llm_off_told) {
            g_llm_off_told = 1;
            if (!g_llm_on) llm_chat_hint("Die Computerspieler lesen den Chat nicht (KI-Stimme aus: [llm] enabled=1 in aimod.ini).",
                                         "The computer players don't read the chat (AI voice off: [llm] enabled=1 in aimod.ini).");
            else llm_chat_hint("Die Computerspieler beantworten den Chat nicht ([llm] chat=1 in aimod.ini).",
                               "The computer players don't answer the chat ([llm] chat=1 in aimod.ini).");
        }
        return;
    }
    int h = (int)*(unsigned *)0x00918E84, to = 0, got = 0, only = 0, at = 1 << 30;
    void *pm = player_at(h);
    uint32_t now = game_time();
    char who[80] = "";
    if (!pm) return;
    for (int i = 1; i < nplayers(); i++) {
        void *pi = player_at(i);
        if (!pi || !g_players[i] || is_out(pi) || i == h || !chat_reaches(pm, pi)) continue;
        got++;
        only = i;
        size_t l = strlen(who);
        if (l < sizeof who - 6) snprintf(who + l, sizeof who - l, " P%d", i);
        int w = word_in(text, player_name(pi));
        if (w >= 0 && w < at) { at = w; to = i; }
    }
    if (!to)   // it names an AI the filter keeps it from: no other one answers in its place
        for (int i = 1; i < nplayers(); i++) {
            void *pi = player_at(i);
            if (!pi || !g_players[i] || is_out(pi) || i == h || chat_reaches(pm, pi) || word_in(text, player_name(pi)) < 0) continue;
            char de[300], en[300];
            snprintf(de, sizeof de, "Keine Antwort: %s bekommt die Nachricht nicht (Chat-Filter).", player_name(pi));
            snprintf(en, sizeof en, "No answer: %s doesn't get your message (chat filter).", player_name(pi));
            if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line names P%d, whom the filter keeps it from: %s\n", i, text);
            llm_chat_hint(de, en);
            return;
        }
    int last = g_llm_last_ai;
    if (!to && last && now - g_llm_last_t < 300000 && g_players[last] && !is_out(g_players[last]) && chat_reaches(pm, g_players[last]))
        to = last;
    if (!to && got == 1) to = only;
    if (!to) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line (mode %d, gets it:%s) meant for no AI - name one: %s\n",
                                        FIELD(pm, 0x9E0, int), got ? who : " none", text);
        if (!got) llm_chat_hint("Keine Antwort: Ihr Chat-Filter erreicht keinen Computerspieler.",
                                "No answer: your chat filter reaches no computer player.");
        else {
            char names[200] = "", de[320], en[320];
            for (int i = 1; i < nplayers(); i++) {
                void *pi = player_at(i);
                if (!pi || !g_players[i] || is_out(pi) || i == h || !chat_reaches(pm, pi)) continue;
                if (names[0] && lstrlenA(names) < 150) lstrcatA(names, ", ");
                if (lstrlenA(names) < 150) lstrcatA(names, player_name(pi));
            }
            snprintf(de, sizeof de, "Keine Antwort: wen meinen Sie? Nennen Sie einen Namen (%s) oder w\xe4hlen Sie ihn im Chat-Filter.", names);
            snprintf(en, sizeof en, "No answer: who do you mean? Name one (%s) or choose them in the chat filter.", names);
            llm_chat_hint(de, en);
        }
        return;
    }
    llm_hist_add(to, 0, text);
    if (!llm_ready()) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line for P%d - the server is down, no answer\n", to);
        char de[300], en[300];
        snprintf(de, sizeof de, "Keine Antwort: das Sprachmodell (%s) ist nicht erreichbar.", g_llm_url);
        snprintf(en, sizeof en, "No answer: the language model (%s) can't be reached.", g_llm_url);
        llm_chat_hint(de, en);
        return;
    }
    struct llm_job *j = llm_slot();
    if (!j) {
        llm_chat_hint("Keine Antwort: das Sprachmodell hat zu viel zu tun - versuchen Sie es gleich noch einmal.",
                      "No answer: the language model is too busy - try again in a moment.");
        return;
    }
    memset(j, 0, sizeof *j);
    j->chat = 1;
    j->ai = to;
    j->to = h;
    j->tools = llm_tools_for(to, h, now);
    j->decide = llm_deciding() ? 2 : 0;
    struct sbuf b = { 0 };
    sb_puts(&b, "{\"messages\":[");
    llm_system(&b, g_players[to], to, h, j->decide ? j->tools : -1);
    llm_history(&b, to);   // ends with his line
    if (!b.p) return;
    llm_queue(j, llm_finish(&b, j->tools, to, h));
    if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line for P%d (mode %d, gets it:%s) asked: %s\n", to,
                                    FIELD(pm, 0x9E0, int), who, text);
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
        if (j->gen == g_llm_gen && pl && j->decide) {
            if (ok) g_llm_n_ok++; else g_llm_n_fail++;
            if (!ok && g_llm_log && g_log) fprintf(g_log, "# llm: P%d no answer (%s)\n", j->ai, g_llm_last_err);
            llm_apply(j, ok);
        } else if (j->gen == g_llm_gen && pl) {
            const char *say = ok ? j->reply : j->chat ? NULL : j->fixed;
            if (ok) g_llm_n_ok++; else if (j->state == J_FAILED) g_llm_n_fail++;
            if (!ok && !j->chat) g_llm_n_fixed++;
            if (say) {   // every line shown here is one the local player hears
                chat_line(pl, say, g_chat_ms);
                if (j->to == (int)*(unsigned *)0x00918E84 || j->chat) llm_hist_add(j->ai, 1, say);
                g_llm_last_ai = j->ai;
                g_llm_last_t = game_time();
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

// a line the local player heard with its fixed text (no model): the AI that spoke last; part of the exchange if it
// was said to him
static void llm_heard(void *pl, void *other, const char *text) {
    if (!g_llm_on || !pl) return;
    if (other && other == player_at((int)*(unsigned *)0x00918E84)) llm_hist_add(pidx(pl), 1, text);
    g_llm_last_ai = pidx(pl);
    g_llm_last_t = game_time();
}

// cmd: llm - state; llm say P TEXT: the local player's chat line TEXT meant for AI P (without sending it); llm filter
// M [P...]: the local player's chat mode M (0 everyone, 1 allies, 2 enemies, 3 the players listed) set directly, as the
// chat dialog's choice would (tests)
static void cmd_llm(char *out, const char *args) {
    int p = 0, n = 0;
    if (sscanf(args, "filter %d %n", &p, &n) == 1) {
        uint8_t *pm = player_at((int)*(unsigned *)0x00918E84);
        if (!pm || p < 0 || p > 3) { REPLY("error: no local player or bad mode\n"); return; }
        FIELD(pm, 0x9E0, int) = p;
        for (int i = 0; i < 16; i++) pm[0xB2C + i] = 0;
        for (const char *s = args + n; p == 3 && *s;) {
            char *e;
            long v = strtol(s, &e, 10);
            if (e == s) break;
            if (v >= 0 && v < 16) pm[0xB2C + v] = 1;
            s = e;
        }
        REPLY("chat mode %d, chosen:", p);
        for (int i = 0; i < 16; i++) if (pm[0xB2C + i]) REPLY(" P%d", i);
        REPLY("\n");
        return;
    }
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
    REPLY("decide %d (%s): decisions %u, with a step taken %u\n", g_llm_decide, llm_deciding() ? "deciding" : "not deciding",
          g_llm_n_dec, g_llm_n_acts);
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
    g_llm_decide = GetPrivateProfileIntA("llm", "decide", 0, ini);
    GetPrivateProfileStringA("llm", "url", g_llm_url, g_llm_url, sizeof g_llm_url, ini);
    GetPrivateProfileStringA("llm", "model", "", g_llm_model, sizeof g_llm_model, ini);
    GetPrivateProfileStringA("llm", "api_key", "", g_llm_key, sizeof g_llm_key, ini);
    GetPrivateProfileStringA("llm", "extra", "", g_llm_extra, sizeof g_llm_extra, ini);
    if (test_ini) {
        g_llm_on = GetPrivateProfileIntA("autotest", "llm", g_llm_on, test_ini);
        g_llm_lines = GetPrivateProfileIntA("autotest", "llm_lines", g_llm_lines, test_ini);
        g_llm_chat = GetPrivateProfileIntA("autotest", "llm_chat", g_llm_chat, test_ini);
        g_llm_timeout = GetPrivateProfileIntA("autotest", "llm_timeout", g_llm_timeout, test_ini);
        g_llm_decide = GetPrivateProfileIntA("autotest", "llm_decide", g_llm_decide, test_ini);
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
    if (g_log) fprintf(g_log, "# llm: on - %s, model '%s', lines %d, chat %d, decide %d, timeout %d s\n", g_llm_url, g_llm_model,
                       g_llm_lines, g_llm_chat, g_llm_decide, g_llm_timeout);
}
