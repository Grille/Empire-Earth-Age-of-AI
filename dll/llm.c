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
// AI TALK ([llm] ai_talk=1, default 0; single player): the AIs talk with each other too - words only, their decisions
// stay with the rules. A line one AI says to another (persona_say: a declaration of war, an alliance offer, thanks for
// a tribute, a capitulation - lines the local player never hears) opens an exchange: that message in the speaker's
// own words, the other's answer, the speaker's reply ([llm] ai_talk_turns messages, default 4, no upper limit) - or
// fewer: a message ending in [end] (talk_answer tells the model) ends the exchange. Each pair of AIs keeps its
// own memory of what they told each other (the last [llm] memory messages, 8) - what one says to another, the local player doesn't
// know, and the AI may tell him something else. One exchange at a time, a new one at most every ai_talk_gap seconds
// (an opener waiting longer than 90 s is dropped, a newer one of the same pair replaces it); the local player's
// requests go to the model first. ai_talk_show=1: he reads along ("to <name>: ..." in the speaker's colour), 0: only
// the log.
// AI DECIDE ([llm] ai_decide=1, default 0; single player, not in scenarios, not with fixed teams): the model makes
// the AIs' decisions towards each other too. An AI's alliance offer or capitulation to another AI (the rules decided
// those at once) stands like one to a human (2 minutes) and opens an exchange: the offer in the offerer's words,
// the other's answer with the tool make_peace to accept (the rules check what is possible: an ally's veto, a
// vassal), the offerer's reply. In every exchange between AIs (with ai_talk too) a message may carry a step towards
// the other: offer peace (the other decides in its answer), declare war (on an ally of 5+ minutes, never on its
// lord), send a tribute (at most a quarter of each resource, once in 3 minutes) - whoever acts gets an answer. No
// answer from the model: the rules decide the offer. Answering war, betrayal and taking sides in a block split stay
// with the rules.
// EQUAL ([llm] equal=1, default 0): the prompts don't say who is human - the local player is "another leader in this
// game" like every AI, and no AI is told it is played by the computer. (What the rules do differently for a human -
// his offers come as lines - the model doesn't see.)
#include <winhttp.h>

#define LLM_JOBS 16
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
static volatile LONG g_llm_fail_note;           // tell the player (llm_tick): 1 the worker can't connect, 2 one answer failed
static int g_llm_told, g_llm_told_fail;         // told this game: that it can't connect, that an answer failed
static int g_llm_off_told;                      // single player, AI voice off: told once per game that nobody reads
static unsigned g_llm_seq;
static unsigned g_llm_n_ok, g_llm_n_fail, g_llm_n_fixed, g_llm_n_late;
static int g_llm_decide;                        // [llm] decide
// [llm] fallback=pause (single player): a model that can't be reached or doesn't answer in time gets no fixed line
// or rules decision in its place - the game pauses (llm_wait_tick) while the worker tries again every 5 s, and runs
// on when it answers. The player may resume it: then it plays on while the answers still come (no pause again until
// they have). fallback=rules (default): the fixed lines and the rules.
static int g_llm_wait;
static volatile LONG g_llm_unreach;             // fallback=pause: the worker can't reach the server (trying again)
static int g_llm_paused;                        // 1 the game paused for the model, 2 the player resumed it meanwhile
static DWORD g_llm_paused_t;
// [llm] constrained=0: the model's steps are limited only by the game - peace whenever at war (no pride, no ally's
// veto), war on any ally at once (its lord too: a vassal rebels), a vassal's own diplomacy, tributes of everything it
// has whenever it likes. Silly things may happen. 1 (default): the rules' limits (tool_peace_ok ...)
static int g_llm_constrained = 1;
static int llm_waits(void) { return g_llm_wait && !g_mp; }
static int g_llm_rules;                         // the rules decide in the model's place (no answer): no new request
static int g_llm_bypass;                        // a fixed line for a decision: shown as it is, not reworded
static uint32_t g_llm_paid_t[16];               // the AI's last tribute by tool (game time)
static unsigned g_llm_n_dec, g_llm_n_acts;
enum { TOOL_PEACE = 1, TOOL_WAR = 2, TOOL_TRIBUTE = 4, TOOL_REFUSE = 8, TOOL_CAPIT = 16 };
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
    int decide;          // 0 words only, 1 the answer to his peace offer, 2 his chat with tools, 3 a move of the rules
    int prop, third;     // decide 3: P_ALLY ... (diplo.c), the third player (an alliance's enemy)
    int tools, act;      // TOOL_ bits offered / called
    int amt[7];          // send_tribute: food..iron (2..6)
    int talk;            // AI TALK: the message's number in the exchange (1 the opener), 0 not a talk between AIs
    int dec;             // AI DECIDE: the exchange is about an offer (1 alliance, 2 capitulation), 0 a talk
    int offer;           // AI DECIDE: the listener's offer to the speaker it may accept (0 alliance, 1 capitulation), -1 none
    int confirm;         // AI DECIDE: asked again for its decision (it answered an offer without a tool): words not shown
};
#define SET_STATE(j, s) InterlockedExchange(&(j)->state, (s))
static struct llm_job g_llm_jobs[LLM_JOBS];

// AI TALK state: each pair's messages ([lo][hi], who = the speaker; who >= 16: the event that opened an exchange, in
// the third person), the exchange running, openers waiting
static int g_llm_ai_talk, g_llm_talk_show = 1, g_llm_talk_gap = 60, g_llm_talk_turns = 4, g_llm_ai_decide, g_llm_equal;

static uint32_t g_pair_last[16][16];   // [lo][hi]: the pair's last request (game time) - events then open no other talk
static unsigned g_talk_n_acts;
// THE MEMORIES ([llm] memory, default 8; 0 none, no upper limit): the AI's exchange with the local player and each
// pair's talks, the last <memory> entries in order - chat, lines and events together, as they happened. Grown as they
// fill (a long memory costs only where it is used); kept in the saved game's state file (llm_mem_save / persist.c).
#define MEM_LEN (LLM_MAXLEN + 32)
struct mem { int n, cap; uint8_t *kind; char (*text)[MEM_LEN]; };
static int g_llm_memory = 8;
static void mem_add(struct mem *m, int kind, const char *text) {
    if (g_llm_memory <= 0 || !*text) return;
    if (m->n >= g_llm_memory) {   // full: the oldest goes
        int drop = m->n - g_llm_memory + 1;
        memmove(m->kind, m->kind + drop, (size_t)(m->n - drop));
        memmove(m->text, m->text + drop, (size_t)(m->n - drop) * MEM_LEN);
        m->n -= drop;
    }
    if (m->n == m->cap) {
        int cap = m->cap ? m->cap * 2 : 8;
        if (cap > g_llm_memory) cap = g_llm_memory;
        uint8_t *k = realloc(m->kind, (size_t)cap);
        char (*t)[MEM_LEN] = k ? realloc(m->text, (size_t)cap * MEM_LEN) : NULL;
        if (k) m->kind = k;
        if (!t) return;
        m->text = t;
        m->cap = cap;
    }
    m->kind[m->n] = (uint8_t)kind;
    lstrcpynA(m->text[m->n++], text, MEM_LEN);
}
// [llm] memory_mode=player: one memory per AI instead - all its exchanges, with the local player and with every AI, and
// its events, in the order they happened; kind = role << 4 | the other player (0 they wrote, 1 it said, 2 an event
// between them, 3 the event that opened an AI talk, in the third person). The pair memories stay empty then.
static int g_llm_mem_player;
static struct mem g_self_hist[16];
static void self_add(int ai, int role, int other, const char *text) {
    if (ai < 1 || ai > 15 || other < 0 || other > 15 || !g_players[ai]) return;   // only the AIs remember
    mem_add(&g_self_hist[ai], role << 4 | other, text);
}
static struct mem g_pair_hist[16][16];   // [lo][hi]: kind = the speaker, 16+ the event that opened an exchange
static struct { int a, b, turn; uint32_t t; } g_talk;   // a 0: none running
static struct { int a, b; char sit[400], ev[400], fixed[256]; uint32_t t; } g_talk_wait[4];
static uint32_t g_talk_started;   // game time the last exchange started
static unsigned g_talk_n_ex, g_talk_n_msg, g_talk_n_fail, g_talk_n_drop;
static void pair_add(int a, int b, int who, const char *text) {
    if (g_llm_mem_player) {   // in both memories, each from its side
        if (who >= 16) { self_add(a, 3, b, text); self_add(b, 3, a, text); }
        else { int o = who == a ? b : a; self_add(who, 1, o, text); self_add(o, 0, who, text); }
        return;
    }
    int lo = a < b ? a : b, hi = a < b ? b : a;
    if (lo < 0 || hi > 15) return;
    mem_add(&g_pair_hist[lo][hi], who, text);
}

// the AI's exchange with the local player: his chat lines (role 0, user), its lines to him (1, assistant) and events
// between them (2: tributes, declarations, its decisions - a user message in brackets)
static struct mem g_llm_hist[16];   // kind = the role
static int g_llm_last_ai;                       // the AI that last said something to the local player
static uint32_t g_llm_last_t;

static void llm_hist_add(int ai, int role, const char *text) {
    if (ai < 0 || ai > 15) return;
    if (g_llm_mem_player) { self_add(ai, role, (int)(*(unsigned *)0x00918E84 & 15), text); return; }
    mem_add(&g_llm_hist[ai], role, text);
}
// the memories for the saved game: records {0 = with the local player / 1 = a pair / 2 = an AI's own (memory_mode=player),
// a, b, count (4 bytes)} then each entry {kind, length, text} - malloc'd, *size its length (NULL: nothing to keep)
#define MEM_RECS (16 * 17 + 16)
static struct mem *mem_rec(int i) {   // record i: 0..15 with the local player, then the pairs, then the AIs' own
    return i < 16 ? &g_llm_hist[i] : i < 16 * 17 ? &g_pair_hist[(i - 16) / 16][(i - 16) % 16] : &g_self_hist[i - 16 * 17];
}
static uint8_t *llm_mem_save(uint32_t *size) {
    uint32_t cap = 0, o = 0;
    for (int i = 0; i < MEM_RECS; i++) {
        const struct mem *m = mem_rec(i);
        cap += 7;
        for (int e = 0; e < m->n; e++) cap += 3 + (uint32_t)strlen(m->text[e]);
    }
    uint8_t *p = malloc(cap);
    if (!p) { *size = 0; return NULL; }
    for (int i = 0; i < MEM_RECS; i++) {
        const struct mem *m = mem_rec(i);
        if (!m->n) continue;
        p[o++] = i < 16 ? 0 : i < 16 * 17 ? 1 : 2;
        p[o++] = (uint8_t)(i < 16 ? i : i < 16 * 17 ? (i - 16) / 16 : i - 16 * 17);
        p[o++] = (uint8_t)(i < 16 || i >= 16 * 17 ? 0 : (i - 16) % 16);
        memcpy(p + o, &m->n, 4);
        o += 4;
        for (int e = 0; e < m->n; e++) {
            uint32_t len = (uint32_t)strlen(m->text[e]);
            p[o++] = m->kind[e];
            p[o++] = (uint8_t)(len & 255);
            p[o++] = (uint8_t)(len >> 8);
            memcpy(p + o, m->text[e], len);
            o += len;
        }
    }
    *size = o;
    return p;
}
static void llm_mem_load(const uint8_t *p, uint32_t size) {   // (after llm_new_game emptied them)
    uint32_t o = 0;
    int entries = 0;
    while (o + 7 <= size) {
        int what = p[o], a = p[o + 1] & 15, b = p[o + 2] & 15, n;
        memcpy(&n, p + o + 3, 4);
        o += 7;
        struct mem *m = what == 2 ? &g_self_hist[a] : what ? &g_pair_hist[a][b] : &g_llm_hist[a];
        for (int e = 0; e < n && o + 3 <= size; e++) {
            int kind = p[o];
            uint32_t len = (uint32_t)(p[o + 1] | p[o + 2] << 8);
            o += 3;
            if (o + len > size || len >= MEM_LEN) return;
            char t[MEM_LEN];
            memcpy(t, p + o, len);
            t[len] = 0;
            o += len;
            mem_add(m, kind, t);   // a smaller memory now: the oldest drop out
            entries++;
        }
    }
    if (g_log) fprintf(g_log, "# llm: memories restored, %d entries (memory %d)\n", entries, g_llm_memory);
}

static void llm_new_game(void) {
    InterlockedIncrement(&g_llm_gen);
    for (int i = 0; i < 16; i++) g_llm_hist[i].n = 0;   // (their room stays for the next game)
    g_llm_last_ai = 0;
    g_llm_last_t = 0;
    g_llm_told = g_llm_told_fail = 0;
    g_llm_off_told = 0;
    memset(g_llm_paid_t, 0, sizeof g_llm_paid_t);
    for (int i = 0; i < 16 * 16; i++) g_pair_hist[i / 16][i % 16].n = 0;
    for (int i = 0; i < 16; i++) g_self_hist[i].n = 0;
    memset(&g_talk, 0, sizeof g_talk);
    memset(g_talk_wait, 0, sizeof g_talk_wait);
    memset(g_pair_last, 0, sizeof g_pair_last);
    g_talk_started = 0;
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
// the value of "key" in the answer (the first one), past the colon - NULL: not there
static const char *json_value(const char *resp, const char *key) {
    char k[48];
    wsprintfA(k, "\"%s\"", key);
    const char *s = strstr(resp, k);
    if (!s) return NULL;
    s += strlen(k);
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    if (*s++ != ':') return NULL;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    return s;
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
static const char *const k_tool_name[5] = { "make_peace", "declare_war", "send_tribute", "refuse_offer", "capitulate" };
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
        for (int t = 0; t < 5; t++) {
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
// fallback=pause: a server that can't be reached or doesn't answer is tried again (llm_post) instead of failing
static int llm_retry(struct llm_job *j, const char *what, DWORD err) {
    if (!llm_waits() || j->gen != g_llm_gen) return 0;
    snprintf(g_llm_last_err, sizeof g_llm_last_err, "%s (error %lu)", what, err);
    InterlockedExchange(&g_llm_unreach, 1);
    return 1;
}
static void llm_post(struct llm_job *j, DWORD wait_ms) {
    DWORD limit = (DWORD)g_llm_timeout * 1000u, t = GetTickCount();
    int retry = 0;
again:
    if (llm_waits()) wait_ms = 0;   // it waits as long as it takes (the game pauses: llm_wait_tick)
    if (wait_ms >= limit - 500) { g_llm_n_late++; snprintf(g_llm_last_err, sizeof g_llm_last_err, "waited too long"); SET_STATE(j, J_FAILED); return; }
    DWORD left = llm_waits() ? 600000u : limit - wait_ms;
    HINTERNET ses = WinHttpOpen(L"eeaimod", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET con = ses ? WinHttpConnect(ses, g_llm_host, g_llm_port, 0) : NULL;
    HINTERNET req = con ? WinHttpOpenRequest(con, L"POST", g_llm_path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             g_llm_https ? WINHTTP_FLAG_SECURE : 0) : NULL;
    if (!req) { if (llm_retry(j, "open", GetLastError())) retry = 1; else llm_fail(j, "open", GetLastError(), 1); goto done; }
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
        if (llm_retry(j, e == ERROR_WINHTTP_TIMEOUT ? "no answer in time" : "cannot connect", e)) retry = 1;
        else llm_fail(j, e == ERROR_WINHTTP_TIMEOUT ? "no answer in time" : "cannot connect", e, e != ERROR_WINHTTP_TIMEOUT);
        goto done;
    }
    if (!WinHttpReceiveResponse(req, NULL)) {
        DWORD e = GetLastError();
        if (e == ERROR_WINHTTP_TIMEOUT) g_llm_n_late++;
        if (llm_retry(j, e == ERROR_WINHTTP_TIMEOUT ? "no answer in time" : "no response", e)) retry = 1;
        else llm_fail(j, e == ERROR_WINHTTP_TIMEOUT ? "no answer in time" : "no response", e, e != ERROR_WINHTTP_TIMEOUT);
        goto done;
    }
    InterlockedExchange(&g_llm_unreach, 0);
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
        // a tool's name in the words ("... make_peace()" - Orion wrote it without calling it): never said
        static const WCHAR *const names[] = { L"make_peace", L"declare_war", L"send_tribute", L"refuse_offer", L"[call",
                                              L"<tool", L"<|" };   // also half-written calls ("... [call")
        for (size_t n = 0; n < sizeof names / sizeof *names; n++)
            if ((mark = wcsstr(w, names[n])) != NULL) *mark = 0;
        size_t wl = wcslen(w);   // "... ohne Bedingungen. call" - the rest of a call
        while (wl && w[wl - 1] == ' ') w[--wl] = 0;
        if (wl >= 5 && !_wcsicmp(w + wl - 4, L"call") && w[wl - 5] == ' ') w[wl - 5] = 0;
        has = 1;
    }
    if (!has) {
        if (r.p) { char *nl; for (nl = r.p; *nl; nl++) if (*nl == '\r' || *nl == '\n') *nl = ' '; }
        snprintf(g_llm_last_err, sizeof g_llm_last_err, "HTTP %lu: %.120s", code, r.p ? r.p : "");
        InterlockedCompareExchange(&g_llm_fail_note, 2, 0);   // this answer only (the server is there)
        SET_STATE(j, J_FAILED);
    } else {
        llm_clean(w, j->speaker, j->reply, sizeof j->reply);
        j->ms = GetTickCount() - t;
        const char *fr = json_value(r.p, "finish_reason"), *rc = json_value(r.p, "reasoning_content");
        int len = fr && !strncmp(fr, "\"length\"", 8);   // cut off at max_tokens
        // a move of the rules (decide 3) is told "call no tool and write nothing" for no: an empty answer that ended by
        // itself is that no, not a failure (LM Studio sends an empty "reasoning_content" with every answer)
        int no = !j->reply[0] && !j->act && j->decide == 3 && !len;
        if (!j->reply[0] && !j->act && !no) {
            int rea = rc && *rc == '"' && rc[1] != '"';   // it did think
            snprintf(g_llm_last_err, sizeof g_llm_last_err, rea ? "the model only thought, no answer%s - [llm] think=0, or "
                     "more max_tokens" : "an empty answer%s", len ? " (max_tokens reached)" : "");
            InterlockedCompareExchange(&g_llm_fail_note, 2, 0);
        }
        SET_STATE(j, j->reply[0] || j->act || no ? J_DONE : J_FAILED);
    }
    free(r.p);
done:
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
    if (retry) {   // fallback=pause: again in 5 s, until it answers (or a new game drops the job)
        retry = 0;
        Sleep(5000);
        t = GetTickCount();
        goto again;
    }
}
static DWORD WINAPI llm_worker(void *arg) {
    (void)arg;
    for (;;) {
        WaitForSingleObject(g_llm_event, 1000);
        for (;;) {
            struct llm_job *j = NULL;
            for (int i = 0; i < LLM_JOBS; i++) {   // the oldest queued one - the local player's before talks between AIs
                struct llm_job *c = &g_llm_jobs[i];
                if (c->state != J_QUEUED) continue;
                if (!j || (!c->talk && j->talk) || (!c->talk == !j->talk && (int)(c->seq - j->seq) < 0)) j = c;
            }
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
// what situation k is, for the model (_template.eeai sit_<line key>); in the third person for both sides of a talk
// between AIs (pair_<line key>; in the speaker's words "has betrayed you" the other's model took the betrayal for its
// own) - NULL where there is none
static const char *llm_situation(int k) {
    char key[64];
    wsprintfA(key, "sit_%s", k_lines[k].key);
    return text_index(k_tpl, N_TPL, key) >= 0 ? tpl_text(key) : k_lines[k].key;
}
static const char *llm_event3(int k) {
    char key[64];
    wsprintfA(key, "pair_%s", k_lines[k].key);
    return text_index(k_tpl, N_TPL, key) >= 0 ? tpl_text(key) : NULL;
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
static int llm_ai_deciding(void);
// tools: -1 a line (no decision), else the TOOL_ bits offered with this request (decide mode)
static void llm_system(struct sbuf *b, void *pl, int ai, int h, int tools) {
    int decide = tools >= 0 && (g_players[h & 15] ? llm_ai_deciding() : llm_deciding());
    struct persona *p = persona_of(pl);
    char t[4096];
    int o = 0;
    g_xc_pl = pl;   // the expressions in the pieces: this AI towards h
    g_xc_other = player_at(h);
    tput(t, &o, sizeof t, "intro", NULL);
    if (p && p->character[0]) tcat(t, &o, sizeof t, " %s", p->character);
    tput(t, &o, sizeof t, "nature", NULL);
    // a few of its own lines, as examples of its voice: worked out as it would say them to h now (its conditions,
    // the names filled) - a situation whose line has nothing to say now is left out
    if (p) {
        char ex_text[600] = "", l[XVAL_CAP];
        int shown = 0, eo = 0;
        static const int ex[] = { L_GREETING, L_WAR_DECLARED, L_PEACE_REFUSE, L_WINNING, L_LOSING, L_UNDER_ATTACK, L_EPOCH_UP };
        for (size_t i = 0; i < sizeof ex / sizeof *ex && shown < 4; i++) {
            if (!p->line[ex[i]]) continue;
            struct line_ctx lc;
            line_ctx_init(&lc, pl, player_at(h), NULL);
            if (!line_pick(p, ex[i], &lc.c, 0, l, sizeof l) || !l[0]) continue;
            tcat(ex_text, &eo, sizeof ex_text, "%s\"%s\"", shown ? " / " : "", l);
            shown++;
        }
        if (shown) tput(t, &o, sizeof t, "voice", "examples", ex_text, NULL);
    }
    tput(t, &o, sizeof t, "rules", "language_name", llm_language(), NULL);
    if (!decide) tput(t, &o, sizeof t, "rules_words", NULL);
    else {
        tput(t, &o, sizeof t, "rules_decide", NULL);
        if (!tools) tput(t, &o, sizeof t, "no_step", NULL);
    }
    // whose "my" is whose: the other's lines arrive as written, from their side
    if (player_at(h) && h != ai)
        tput(t, &o, sizeof t, "whose", NULL);
    if (g_llm_mem_player && player_at(h) && h != ai) tput(t, &o, sizeof t, "mem_rule", NULL);   // its notes of the others
    // the game as it stands
    void *ph = player_at(h);
    tput(t, &o, sizeof t, "game_state_me", NULL);
    if (ph && h != ai) {
        tput(t, &o, sizeof t, "game_state_other", NULL);   // stance, forces, attacks, fighting ...: the [vars] of _template.eeai
        char terms[600];
        if (peace_terms(ai, h, terms, sizeof terms) && decide)   // the rules' verdict: advice when it decides itself
            tput(t, &o, sizeof t, "terms_advice", "terms", terms, NULL);
        else if (terms[0])   // what it would really take (the rules decide, not the model)
            tput(t, &o, sizeof t, "terms_real", "terms", terms, NULL);
        else if (TEAMS_LOCKED) tput(t, &o, sizeof t, "teams_fixed", NULL);
    }
    char list[600] = "";
    int lo = 0;
    for (int j = 1; j < nplayers() && lo < (int)sizeof list - 120; j++) {
        void *pj = player_at(j);
        if (!pj || j == ai || j == h || is_out(pj)) continue;
        if (lo) tcat(list, &lo, sizeof list, ", ");
        lo += tsub(list + lo, sizeof list - lo, tpl_text(stance_of(pl, j) ? "other_enemy" : "other_ally"), pl, pj, NULL);
    }
    if (lo) tput(t, &o, sizeof t, "others", "list", list, NULL);
    g_xc_pl = g_xc_other = NULL;
    sb_msg(b, "system", t);
}
// memory_mode=player: AI ai's memory for a request to other - their exchange as in the other modes (its words the
// assistant's, theirs and the events user messages), all else in between as notes (mem_heard, mem_said, mem_event)
static void self_history(struct sbuf *b, int ai, int other) {
    void *pa = player_at(ai), *po = player_at(other);
    const struct mem *m = &g_self_hist[ai];
    for (int i = 0; i < m->n; i++) {
        char u[LLM_MAXLEN + 200];
        int role = m->kind[i] >> 4, with = m->kind[i] & 15;
        const char *t = m->text[i];
        if (with == other) {
            if (role == 1) { sb_msg(b, "assistant", t); continue; }
            if (role == 3) {   // the event that opened an AI talk
                char ev[LLM_MAXLEN + 40];
                wsprintfA(ev, "%s.", t);
                tsub(u, sizeof u, tpl_text("event"), pa, po, "text", ev, NULL);
            } else tsub(u, sizeof u, tpl_text(role == 2 ? "event" : "writes"), pa, po, "text", t, NULL);
        } else {
            void *pw = player_at(with);
            if (!pw) continue;
            tsub(u, sizeof u, tpl_text(role == 0 ? "mem_heard" : role == 1 ? "mem_said" : "mem_event"), pa, pw, "text", t, NULL);
        }
        sb_msg(b, "user", u);
    }
}
static void llm_history(struct sbuf *b, int ai) {   // his lines framed with his name (whose "my" it is)
    void *pl = player_at(ai), *ph = player_at((int)*(unsigned *)0x00918E84);
    if (g_llm_mem_player) { self_history(b, ai, (int)(*(unsigned *)0x00918E84 & 15)); return; }
    for (int i = 0; i < g_llm_hist[ai].n; i++) {
        char u[LLM_MAXLEN + 120];
        if (g_llm_hist[ai].kind[i] == 1) { sb_msg(b, "assistant", g_llm_hist[ai].text[i]); continue; }
        if (g_llm_hist[ai].kind[i] == 2) {
            tsub(u, sizeof u, tpl_text("event"), pl, ph, "text", g_llm_hist[ai].text[i], NULL);
            sb_msg(b, "user", u);
            continue;
        }
        tsub(u, sizeof u, tpl_text("writes"), pl, ph, "text", g_llm_hist[ai].text[i], NULL);
        sb_msg(b, "user", u);
    }
}
// the tools offered (TOOL_ bits), for player h
static void sb_tool(struct sbuf *b, const char *name, const char *desc, const char *params) {
    sb_printf(b, "%s{\"type\":\"function\",\"function\":{\"name\":\"%s\",\"description\":", b->p[b->n - 1] == '[' ? "" : ",", name);
    sb_json(b, desc);
    sb_printf(b, ",\"parameters\":%s}}", params);
}
// AI DECIDE: h's offer to AI ai stands and ai may take it - 0 an alliance, 1 a capitulation; -1 none
static int pair_accept(int ai, int h) {
    void *pl = player_at(ai), *ph = player_at(h);
    if (!pl || !ph || is_out(pl) || is_out(ph) || !g_offer_t[h][ai] || g_offer_kind[h][ai] > 1 || (g_llm_constrained && is_vassal(ai))) return -1;
    g_veto_quiet = 1;
    int veto = block_veto(ai, h, game_time(), NULL);
    g_veto_quiet = 0;
    return veto ? -1 : g_offer_kind[h][ai];
}
static void sb_tools(struct sbuf *b, int tools, int ai, int h) {
    void *ph = player_at(h), *pl = player_at(ai);
    if (!tools || !ph || !pl) return;
    char d[600], what[120];
    sb_puts(b, ",\"tools\":[");
    int k = (tools & TOOL_PEACE) && g_players[h] ? pair_accept(ai, h) : -1;
    if (tools & TOOL_PEACE) {
        if (k == 1) tribute_text(g_promise[h], what, sizeof what);
        tsub(d, sizeof d, tpl_text(k == 1 ? "tool_accept_capitulation" : k == 0 ? "tool_accept_alliance" :
                                   !stance_of(ph, ai) ? "tool_accept_peace" : g_players[h] ? "tool_offer_alliance" :
                                   "tool_offer_alliance_automated"), pl, ph, "tribute", what, NULL);
        sb_tool(b, "make_peace", d, "{\"type\":\"object\",\"properties\":{}}");
    }
    if (tools & TOOL_WAR) {
        tsub(d, sizeof d, tpl_text("tool_war"), pl, ph, NULL);
        sb_tool(b, "declare_war", d, "{\"type\":\"object\",\"properties\":{}}");
    }
    if (tools & TOOL_TRIBUTE) {
        tsub(d, sizeof d, tpl_text("tool_tribute"), pl, ph, NULL);   // what it may give
        sb_tool(b, "send_tribute", d, "{\"type\":\"object\",\"properties\":{\"food\":{\"type\":\"integer\"},\"wood\":{\"type\":"
                "\"integer\"},\"stone\":{\"type\":\"integer\"},\"gold\":{\"type\":\"integer\"},\"iron\":{\"type\":\"integer\"}}}");
    }
    if (tools & TOOL_REFUSE) {
        tsub(d, sizeof d, tpl_text("tool_refuse"), pl, ph, "what", tpl_text(k == 1 ? "what_capitulation" : "what_alliance"), NULL);
        sb_tool(b, "refuse_offer", d, "{\"type\":\"object\",\"properties\":{}}");
    }
    if (tools & TOOL_CAPIT) {   // what it would offer (capitulate: the same share of its stock then)
        int offer[7] = { 0 };
        capit_offer(pl, offer);
        tribute_text(offer, what, sizeof what);
        tsub(d, sizeof d, tpl_text("tool_capitulate"), pl, ph, "tribute", what, NULL);
        sb_tool(b, "capitulate", d, "{\"type\":\"object\",\"properties\":{}}");
    }
    sb_puts(b, "],\"tool_choice\":\"auto\"");
}
static int g_llm_tok_once;   // max_tokens for the next request only (0: the setting)
static char *llm_finish(struct sbuf *b, int tools, int ai, int h) {
    sb_puts(b, "]");
    sb_tools(b, tools, ai, h);
    if (g_llm_model[0]) { sb_puts(b, ",\"model\":"); sb_json(b, g_llm_model); }
    sb_printf(b, ",\"max_tokens\":%d,\"temperature\":%.2f,\"stream\":false", g_llm_tok_once ? g_llm_tok_once : g_llm_max_tokens,
              g_llm_temp / 100.0);
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
static int llm_busy(void) {   // requests queued or running (display.c: stalls)
    int n = 0;
    for (int i = 0; i < LLM_JOBS; i++) n += g_llm_jobs[i].state == J_QUEUED || g_llm_jobs[i].state == J_RUNNING;
    return n;
}
static void llm_queue(struct llm_job *j, char *body) {
    j->body = body;
    lstrcpynA(j->speaker, g_players[j->ai & 15] ? player_name(g_players[j->ai & 15]) : "", sizeof j->speaker);
    j->gen = g_llm_gen;
    j->t0 = GetTickCount();
    j->seq = ++g_llm_seq;
    SET_STATE(j, J_QUEUED);
    SetEvent(g_llm_event);
}
// a situation for the model: its expressions filled (x_fill, _template.eeai's [vars]) - pl speaking to other (none:
// to everyone), third the enemy - and {tribute}
static void llm_subst(char *out, int cap, const char *s, void *pl, void *other, void *third) {
    char trib[96];
    tribute_text(g_say_tribute, trib, sizeof trib);
    const char *pn[1] = { "tribute" };
    const char *pv[1] = { trib };
    struct xctx c = { 0 };
    c.pl = pl; c.other = other; c.third = third; c.found = 1;
    c.pn = pn; c.pv = pv; c.np = 1;
    c.vars[0] = &g_vars_tpl;
    x_fill(out, cap, s, &c);
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
    llm_subst(sit, sizeof sit, llm_situation(k), pl, other, third);
    char keep[160] = "";
    if (j->nmust) wsprintfA(keep, " %s", tpl_text("keep"));
    tsub(u, sizeof u, tpl_text("line"), pl, other, "situation", sit, "audience", tpl_text(k_lines[k].aud == A_ALL ? "audience_all" :
         k_lines[k].aud == A_ALLIES ? "audience_allies" : "audience_them"), "language_name", llm_language(), "line", fixed, "keep",
         keep, NULL);
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
    if (!pl || !ph || is_out(ph) || is_out(pl) || !stance_of(pl, h) || g_offer_t[ai][h]) return 0;
    if (!g_llm_constrained) return 1;
    if (is_vassal(ai)) return 0;
    if (g_war_since[ai][h] && now - g_war_since[ai][h] < (uint32_t)trait(pl, T_PRIDE) * 3000u) return 0;
    g_veto_quiet = 1;
    int veto = block_veto(ai, h, now, NULL);
    g_veto_quiet = 0;
    return !veto;
}
static int tool_war_ok(int ai, int h, uint32_t now) {   // allied with h for 5+ minutes, h not its lord
    void *pl = player_at(ai), *ph = player_at(h);
    if (!pl || !ph || is_out(ph) || is_out(pl) || stance_of(pl, h) || g_offer_t[ai][h]) return 0;
    if (!g_llm_constrained) return 1;
    if (is_vassal(ai) || g_lord[ai] == h) return 0;
    return !g_ally_since[ai][h] || now - g_ally_since[ai][h] >= 300000;
}
static int tool_tribute_ok(int ai, uint32_t now) {   // every 3 minutes
    return !g_llm_constrained || !g_llm_paid_t[ai] || now - g_llm_paid_t[ai] >= 180000;
}
static int tribute_cap(int have) { return have <= 0 ? 0 : g_llm_constrained ? have / 4 : have; }   // a quarter of each
// constrained=0 and the model decides towards the local player: a vassal answers peace offers itself (diplo.c)
static int llm_unbound(void) { return !g_llm_constrained && llm_deciding() && !g_llm_rules; }
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
    tsub(u, sizeof u, tpl_text("peace_answer"), pl, player_at(j), "language_name", llm_language(), NULL);
    sb_msg(&b, "user", u);
    if (!b.p) return 0;
    llm_queue(jb, llm_finish(&b, TOOL_PEACE, ai, h));
    if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d decides on the peace offer of P%d (asked)\n", ai, j);
    return 1;
}
// a move of the rules towards the local player (diplo.c P_ALLY ...): the model decides - 1 (asked now, still asking,
// or it declined that move in the last 10 minutes: not now), 0: the rules go ahead (no model deciding, or no room)
static uint32_t g_prop_no[16][P_N];   // [ai][move]: when the model last declined it
static int llm_propose(int ai, int h, int kind, int third, uint32_t now) {
    void *pl = player_at(ai), *ph = player_at(h);
    if (g_llm_rules || !pl || !ph || h != (int)*(unsigned *)0x00918E84 || !llm_deciding() || kind < 0 || kind >= P_N) return 0;
    if (g_prop_no[ai][kind] && now - g_prop_no[ai][kind] < 600000) return 1;
    for (int i = 0; i < LLM_JOBS; i++)
        if (g_llm_jobs[i].state != J_FREE && g_llm_jobs[i].ai == ai && g_llm_jobs[i].decide == 3) return 1;
    struct llm_job *jb = llm_slot();
    if (!jb) return 0;
    static const char *const key[P_N] = { "prop_alliance", "prop_betray", "prop_capitulate", "prop_rebel" };
    static const int tool[P_N] = { TOOL_PEACE, TOOL_WAR, TOOL_CAPIT, TOOL_WAR };
    memset(jb, 0, sizeof *jb);
    jb->ai = ai;
    jb->to = h;
    jb->decide = 3;
    jb->prop = kind;
    jb->third = third;
    jb->tools = tool[kind];
    struct sbuf b = { 0 };
    sb_puts(&b, "{\"messages\":[");
    llm_system(&b, pl, ai, h, jb->tools);
    llm_history(&b, ai);
    char move[600], u[1200], what[120] = "";
    int offer[7] = { 0 };
    if (kind == P_CAPIT) { capit_offer(pl, offer); tribute_text(offer, what, sizeof what); }
    tsub3(move, sizeof move, tpl_text(key[kind]), pl, ph, third ? player_at(third) : NULL, "tribute", what, NULL);
    tsub(u, sizeof u, tpl_text("proposal"), pl, ph, "move", move, "tool", k_tool_name[kind == P_CAPIT ? 4 : kind == P_ALLY ? 0 : 1],
         "language_name", llm_language(), NULL);
    sb_msg(&b, "user", u);
    if (!b.p) { SET_STATE(jb, J_FREE); return 0; }
    llm_queue(jb, llm_finish(&b, jb->tools, ai, h));
    if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d decides on a move towards P%d (asked): %s\n", ai, h, key[kind] + 5);
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
    char ev[3][300];   // what it did, for its memory - after its words
    int nev = 0;
    if (j->decide == 3) {   // a move of the rules: its yes carries it out with its words, its no leaves it undone
        int yes = ok && (j->act & j->tools) && prop_valid(ai, h, j->prop, j->third);
        if (!ok && prop_valid(ai, h, j->prop, j->third)) {   // no answer: the rules go ahead after all
            if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d no answer on a move (%s) - the rules decide\n", ai, g_llm_last_err);
            g_llm_rules = 1;
            prop_do(ai, h, j->prop, j->third, now);
            g_llm_rules = 0;
            return;
        }
        if (!ok) return;
        g_llm_n_dec++;
        if (yes) {
            g_llm_n_acts++;
            g_say_instead = j->reply[0] ? j->reply : NULL;   // its words instead of the personality's line
            prop_do(ai, h, j->prop, j->third, now);
            g_say_instead = NULL;
            char e[300];
            tsub(e, sizeof e, tpl_text(j->prop == P_ALLY ? "ev_offered_peace" : j->prop == P_CAPIT ? "ev_capitulated" : "ev_declared_war"),
                 pl, ph, NULL);
            llm_happened(ai, "%s", e);
        } else g_prop_no[ai][j->prop] = now ? now : 1;   // not now: 10 minutes before the rules propose it again
        if (g_llm_log && g_log) fprintf(g_log, "# llm: P%d decides on a move towards P%d (%.1f s): %s - %s\n", ai, h, j->ms / 1000.0,
                                        yes ? "does it" : j->act & j->tools ? "would, but it isn't possible any more" : "declines",
                                        j->reply[0] ? j->reply : "(no words)");
        return;
    }
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
            tsub(ev[nev++], sizeof ev[0], tpl_text("ev_made_peace"), pl, ph, NULL);
            fixed = j->decide == 1 ? L_PEACE_ACCEPT : L_OFFER_ACCEPTED;
        } else {   // its offer: he has 2 minutes (diplo_tick, as its alliance offers)
            g_offer_t[ai][h] = now ? now : 1;
            g_offer_kind[ai][h] = 0;
            g_offer_vs[ai][h] = 0;
            block_line(h, sysf("peace_offer", pl, ph, NULL, NULL));
            tsub(ev[nev++], sizeof ev[0], tpl_text("ev_offered_peace"), pl, ph, NULL);
        }
        did |= TOOL_PEACE;
    }
    if ((j->act & j->tools & TOOL_WAR) && tool_war_ok(ai, h, now)) {
        declare_war(ai, h, now);
        tsub(ev[nev++], sizeof ev[0], tpl_text("ev_declared_war"), pl, ph, NULL);
        fixed = L_BETRAY;
        did |= TOOL_WAR;
    }
    if ((j->act & j->tools & TOOL_TRIBUTE) && tool_tribute_ok(ai, now)) {
        int *from = (int *)((uint8_t *)pl + PL_STOCK), *to = (int *)((uint8_t *)ph + PL_STOCK), paid[7] = { 0 }, total = 0;
        for (int r = 2; r < 7; r++) {
            int n = j->amt[r], cap = tribute_cap(from[r]);
            n = n < 0 ? 0 : n > cap ? cap : n;
            from[r] -= n;
            to[r] += n;
            paid[r] = n;
            total += n;
        }
        if (total > 0) {
            hist_tribute(ai, h, total);
            g_llm_paid_t[ai] = now ? now : 1;
            tribute_text(paid, what, sizeof what);
            block_line(h, sysf("tribute_sent", pl, ph, NULL, "tribute", what, NULL));
            tsub(ev[nev++], sizeof ev[0], tpl_text("ev_sent_tribute"), pl, ph, "tribute", what, NULL);
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
static void llm_chat_hint(const char *text) {
    if (g_mp) return;
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    system_line(text, opt ? FIELD(opt, 0x2CC, int) : 15000);
    if (g_log) fprintf(g_log, "# llm: hint for the player: %s\n", text);
}
// the local player wrote `text` in the chat: one AI that gets it (the chat filter) answers - the one it names first,
// else the last one he heard within 5 minutes, else the only one that gets it. In single player a line nobody will
// answer gets a system line saying why (with the AI voice off: once per game).
static void llm_player_chat(const char *text) {
    if (!text || !*text || victory_word(text)) return;
    if (!g_llm_on || !g_llm_chat) {
        if (!g_llm_off_told) {
            g_llm_off_told = 1;
            llm_chat_hint(sysf(!g_llm_on ? "chat_unread" : "chat_unanswered", NULL, NULL, NULL, NULL));
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
            if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line names P%d, whom the filter keeps it from: %s\n", i, text);
            llm_chat_hint(sysf("chat_filtered", pi, NULL, NULL, NULL));
            return;
        }
    int last = g_llm_last_ai;
    if (!to && last && now - g_llm_last_t < 300000 && g_players[last] && !is_out(g_players[last]) && chat_reaches(pm, g_players[last]))
        to = last;
    if (!to && got == 1) to = only;
    if (!to) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line (mode %d, gets it:%s) meant for no AI - name one: %s\n",
                                        FIELD(pm, 0x9E0, int), got ? who : " none", text);
        if (!got) llm_chat_hint(sysf("chat_nobody", NULL, NULL, NULL, NULL));
        else {
            char names[200] = "";
            for (int i = 1; i < nplayers(); i++) {
                void *pi = player_at(i);
                if (!pi || !g_players[i] || is_out(pi) || i == h || !chat_reaches(pm, pi)) continue;
                if (names[0] && lstrlenA(names) < 150) lstrcatA(names, ", ");
                if (lstrlenA(names) < 150) lstrcatA(names, player_name(pi));
            }
            llm_chat_hint(sysf("chat_who", NULL, NULL, NULL, "names", names, NULL));
        }
        return;
    }
    llm_hist_add(to, 0, text);
    if (!llm_ready()) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: chat line for P%d - the server is down, no answer\n", to);
        llm_chat_hint(sysf("chat_down", NULL, NULL, NULL, "url", g_llm_url, NULL));
        return;
    }
    struct llm_job *j = llm_slot();
    if (!j) {
        llm_chat_hint(sysf("chat_busy", NULL, NULL, NULL, NULL));
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

// ---- AI TALK ----
static void pair_history(struct sbuf *b, int ai, int other) {   // from ai's side: its own messages are the assistant's
    if (g_llm_mem_player) { self_history(b, ai, other); return; }
    int lo = ai < other ? ai : other, hi = ai < other ? other : ai;
    void *pa = player_at(ai), *po = player_at(other);
    for (int i = 0; i < g_pair_hist[lo][hi].n; i++) {
        char u[LLM_MAXLEN + 120];
        int w = g_pair_hist[lo][hi].kind[i];
        if (w == ai) { sb_msg(b, "assistant", g_pair_hist[lo][hi].text[i]); continue; }
        if (w >= 16) {   // the event that opened an exchange
            char ev[LLM_MAXLEN + 40];
            wsprintfA(ev, "%s.", g_pair_hist[lo][hi].text[i]);
            tsub(u, sizeof u, tpl_text("event"), pa, po, "text", ev, NULL);
            sb_msg(b, "user", u);
            continue;
        }
        tsub(u, sizeof u, tpl_text("writes"), pa, po, "text", g_pair_hist[lo][hi].text[i], NULL);
        sb_msg(b, "user", u);
    }
}
static int llm_ai_deciding(void) {
    return llm_ready() && g_llm_ai_decide && g_ai_diplomacy && !g_mp && !scenario_game() && !TEAMS_LOCKED;
}
// the steps open to AI ai towards AI h in a message
static int pair_tools(int ai, int h, uint32_t now) {
    void *pl = player_at(ai), *ph = player_at(h);
    if (!llm_ai_deciding() || !pl || !ph || is_out(pl) || is_out(ph)) return 0;
    int t = 0;
    if (pair_accept(ai, h) >= 0) t |= TOOL_PEACE;   // accept its offer
    else if (tool_peace_ok(ai, h, now) && !g_offer_t[h][ai]) t |= TOOL_PEACE;   // offer peace (or accept its allied stance)
    if (tool_war_ok(ai, h, now)) t |= TOOL_WAR;
    if (tool_tribute_ok(ai, now)) t |= TOOL_TRIBUTE;
    return t;
}
// a message of speaker to listener: turn 1 opens the exchange on an event (sit, its usual line fixed - or none), later
// turns answer the other's last message; dec: the exchange is about an offer (AI DECIDE)
static int talk_request(int speaker, int listener, int turn, const char *sit, const char *fixed, int dec) {
    void *pl = player_at(speaker), *po = player_at(listener);
    if (!pl || !po || !g_players[speaker] || is_out(pl) || is_out(po) || !llm_ready()) return 0;
    struct llm_job *j = llm_slot();
    if (!j) return 0;
    memset(j, 0, sizeof *j);
    j->ai = speaker;
    j->to = listener;
    j->talk = turn;
    j->dec = dec;
    uint32_t now = game_time();
    j->tools = turn > 1 ? pair_tools(speaker, listener, now) : 0;
    j->offer = j->tools & TOOL_PEACE ? pair_accept(speaker, listener) : -1;
    if (j->offer >= 0) j->tools |= TOOL_REFUSE;
    j->confirm = !sit && fixed;   // talk_done: asked again (fixed is "" then)
    g_pair_last[speaker < listener ? speaker : listener][speaker < listener ? listener : speaker] = now ? now : 1;
    struct sbuf b = { 0 };
    sb_puts(&b, "{\"messages\":[");
    llm_system(&b, pl, speaker, listener, j->tools ? j->tools : -1);
    pair_history(&b, speaker, listener);
    char u[1200];
    const char *lang = llm_language();
    if (turn == 1) {
        char usual[400] = "";
        if (fixed && *fixed) { usual[0] = ' '; tsub(usual + 1, sizeof usual - 1, tpl_text("talk_usual"), pl, po, "line", fixed, NULL); }
        tsub(u, sizeof u, tpl_text("talk_open"), pl, po, "situation", sit, "language_name", lang, "usual", usual, NULL);
    } else if (j->confirm && j->offer >= 0)
        tsub(u, sizeof u, tpl_text("talk_confirm"), pl, po, "what", tpl_text(j->offer ? "what_capitulation" : "what_alliance"), NULL);
    else if (j->offer == 1) {
        char what[120];
        tribute_text(g_promise[listener], what, sizeof what);
        tsub(u, sizeof u, tpl_text("talk_capitulation"), pl, po, "language_name", lang, "tribute", what, NULL);
    } else if (j->offer == 0) tsub(u, sizeof u, tpl_text("talk_alliance"), pl, po, "language_name", lang, NULL);
    else tsub(u, sizeof u, tpl_text(turn >= g_llm_talk_turns ? "talk_last" : "talk_answer"), pl, po, "language_name", lang, NULL);
    sb_msg(&b, "user", u);
    if (!b.p) { SET_STATE(j, J_FREE); return 0; }
    if (j->confirm) g_llm_tok_once = g_llm_max_tokens > 1200 ? g_llm_max_tokens : 1200;   // asked again: it thinks first
    llm_queue(j, llm_finish(&b, j->tools, speaker, listener));
    g_llm_tok_once = 0;
    return 1;
}
// AI DECIDE: AI from offers AI to an alliance (kind 0, against third) or capitulates to it (1) - diplo.c sets the
// offer up when this returns 1 (the exchange is queued)
static int llm_ai_offer(int from, int to, int kind, int third) {
    if (!llm_ai_deciding() || !g_players[from] || !g_players[to]) return 0;
    if (g_offer_t[from][to] || g_offer_t[to][from]) return 0;   // one offer at a time between them: the rules
    void *pf = player_at(from), *pt = player_at(to), *p3 = third ? player_at(third) : NULL;
    enum line_kind k = kind ? L_CAPITULATE : L_ALLIANCE_OFFER;
    char sit[400], ev[400];
    llm_subst(sit, sizeof sit, llm_situation(k), pf, pt, p3);
    llm_subst(ev, sizeof ev, llm_event3(k), pf, pt, p3);
    if (!talk_request(from, to, 1, sit, "", kind + 1)) return 0;
    for (int i = 0; i < 4; i++)   // the offer's line waits as a talk: this exchange is it
        if (g_talk_wait[i].t && ((g_talk_wait[i].a == from && g_talk_wait[i].b == to) || (g_talk_wait[i].a == to && g_talk_wait[i].b == from)))
            g_talk_wait[i].t = 0;
    pair_add(from, to, 16, ev);
    if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d -> P%d opened (the model decides): %s\n", from, to, sit);
    return 1;
}
// AI DECIDE: the steps a message called, checked again and carried out; events for the pair's memory
static int talk_apply(struct llm_job *j) {
    int ai = j->ai, h = j->to, did = 0, act = j->act & j->tools;
    void *pl = player_at(ai), *ph = player_at(h);
    uint32_t now = game_time();
    if (!pl || !ph || is_out(pl) || is_out(ph)) return 0;
    char ev[400], what[120] = "";
    if (j->act & ~j->tools && g_llm_log && g_log)
        fprintf(g_log, "# llm: P%d called a tool it wasn't offered (called %d, offered %d) - ignored\n", ai, j->act, j->tools);
    if (act & TOOL_PEACE) {
        int k = pair_accept(ai, h);
        if (k >= 0 || (!stance_of(ph, ai) && tool_peace_ok(ai, h, now))) {   // its offer, or its allied stance: accepted
            make_peace(ai, h, now);   // h's turn (diplo_tick) completes an offer: allied both ways / tribute and vassal
            tsub(ev, sizeof ev, tpl_text("ev3_accepted"), pl, ph, "what",
                 tpl_text(k == 1 ? "what_capitulation" : k == 0 ? "what_alliance" : "what_peace"), NULL);
            pair_add(ai, h, 16, ev);
            did |= TOOL_PEACE;
        } else if (stance_of(ph, ai) && tool_peace_ok(ai, h, now) && !g_offer_t[h][ai]) {   // its offer: h decides next
            g_offer_t[ai][h] = now ? now : 1;
            g_offer_kind[ai][h] = 0;
            g_offer_vs[ai][h] = 0;
            g_offer_last[ai][h] = now;
            tsub(ev, sizeof ev, tpl_text("ev3_offered"), pl, ph, NULL);
            pair_add(ai, h, 16, ev);
            did |= TOOL_PEACE;
        }
    }
    if ((act & TOOL_WAR) && tool_war_ok(ai, h, now)) {
        declare_war(ai, h, now);
        tsub(ev, sizeof ev, tpl_text("ev3_war"), pl, ph, NULL);
        pair_add(ai, h, 16, ev);
        did |= TOOL_WAR;
    }
    if ((act & TOOL_TRIBUTE) && tool_tribute_ok(ai, now)) {
        int *from = (int *)((uint8_t *)pl + PL_STOCK), *to = (int *)((uint8_t *)ph + PL_STOCK), paid[7] = { 0 }, total = 0;
        for (int r = 2; r < 7; r++) {
            int n = j->amt[r], cap = tribute_cap(from[r]);
            n = n < 0 ? 0 : n > cap ? cap : n;
            from[r] -= n;
            to[r] += n;
            paid[r] = n;
            total += n;
        }
        if (total > 0) {
            hist_tribute(ai, h, total);
            g_llm_paid_t[ai] = now ? now : 1;
            tribute_text(paid, what, sizeof what);
            tsub(ev, sizeof ev, tpl_text("ev3_tribute"), pl, ph, "tribute", what, NULL);
            pair_add(ai, h, 16, ev);
            diplo_tribute(ph, pl, total);   // its opinion; its thanks wait (the pair is talking)
            did |= TOOL_TRIBUTE;
        }
    }
    if (did) g_talk_n_acts++;
    if (g_llm_log && g_log)
        fprintf(g_log, "# llm: talk P%d -> P%d: offered%s%s%s, called%s%s%s, done%s%s%s%s%s\n", ai, h,
                j->tools & TOOL_PEACE ? " peace" : "", j->tools & TOOL_WAR ? " war" : "", j->tools & TOOL_TRIBUTE ? " tribute" : "",
                j->act & TOOL_PEACE ? " peace" : "", j->act & TOOL_WAR ? " war" : "", j->act & TOOL_TRIBUTE ? " tribute" : "",
                did & TOOL_PEACE ? (j->offer >= 0 || !stance_of(pl, h) ? " accepted" : " peace offered") : "",
                did & TOOL_WAR ? " war" : "", did & TOOL_TRIBUTE ? " tribute " : "", what, did ? "" : " nothing");
    return did;
}
// persona_say: AI pl's line k to AI other, which the local player doesn't hear - an exchange between them (waits)
static void llm_talk_event(void *pl, enum line_kind k, void *other, void *third, const char *fixed) {
    if (!g_llm_on || !g_llm_ai_talk || g_mp || !pl || !other || k_lines[k].aud != A_NAME) return;
    int a = pidx(pl), b = pidx(other), h = (int)*(unsigned *)0x00918E84;
    if (a == b || b == h || a == h || !g_players[a] || !g_players[b]) return;
    uint32_t pt = g_pair_last[a < b ? a : b][a < b ? b : a];
    if (pt && game_time() - pt < 60000) return;   // they are talking (an answer, an offer): its own words are in it
    int w = -1;
    for (int i = 0; i < 4 && w < 0; i++)   // the same pair waiting: the newer event replaces it
        if (g_talk_wait[i].t && ((g_talk_wait[i].a == a && g_talk_wait[i].b == b) || (g_talk_wait[i].a == b && g_talk_wait[i].b == a))) w = i;
    for (int i = 0; i < 4 && w < 0; i++) if (!g_talk_wait[i].t) w = i;
    if (w < 0) {   // all taken: the oldest goes
        w = 0;
        for (int i = 1; i < 4; i++) if (g_talk_wait[i].t < g_talk_wait[w].t) w = i;
        g_talk_n_drop++;
    }
    g_talk_wait[w].a = a;
    g_talk_wait[w].b = b;
    llm_subst(g_talk_wait[w].sit, sizeof g_talk_wait[w].sit, llm_situation(k), pl, other, third);
    llm_subst(g_talk_wait[w].ev, sizeof g_talk_wait[w].ev, llm_event3(k) ? llm_event3(k) : g_talk_wait[w].sit, pl,
              other, third);
    lstrcpynA(g_talk_wait[w].fixed, fixed, sizeof g_talk_wait[w].fixed);
    g_talk_wait[w].t = game_time() ? game_time() : 1;
}
static void talk_tick(void) {
    uint32_t now = game_time();
    if (!g_llm_ai_talk) return;
    if (now < g_talk_started) { memset(&g_talk, 0, sizeof g_talk); memset(g_talk_wait, 0, sizeof g_talk_wait); g_talk_started = 0; }
    if (g_talk.a && now - g_talk.t > 180000) g_talk.a = 0;   // lost (should not happen): free the channel
    for (int i = 0; i < 4; i++)
        if (g_talk_wait[i].t && now - g_talk_wait[i].t > 90000) { g_talk_wait[i].t = 0; g_talk_n_drop++; }
    if (g_talk.a || (g_talk_started && now - g_talk_started < (uint32_t)g_llm_talk_gap * 1000u)) return;
    int w = -1;
    for (int i = 0; i < 4; i++) if (g_talk_wait[i].t && (w < 0 || g_talk_wait[i].t < g_talk_wait[w].t)) w = i;
    if (w < 0) return;
    g_talk_wait[w].t = 0;
    if (!talk_request(g_talk_wait[w].a, g_talk_wait[w].b, 1, g_talk_wait[w].sit, g_talk_wait[w].fixed, 0)) return;
    g_talk.a = g_talk_wait[w].a;
    g_talk.b = g_talk_wait[w].b;
    g_talk.turn = 1;
    g_talk.t = now ? now : 1;
    g_talk_started = g_talk.t;
    g_talk_n_ex++;
    pair_add(g_talk.a, g_talk.b, 16, g_talk_wait[w].ev);   // after the opener's request: it has it there
    if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d -> P%d opened: %s\n", g_talk.a, g_talk.b, g_talk_wait[w].sit);
}
// "[end]" in a message of a talk between AIs (any case): taken out, the spaces around it too - 1 if it was there
static int talk_end_mark(char *s) {
    int found = 0;
    for (char *p = s; (p = strchr(p, '[')) != NULL;) {
        if (_strnicmp(p, "[end]", 5)) { p++; continue; }
        memmove(p, p + 5, strlen(p + 5) + 1);
        found = 1;
    }
    int n = (int)strlen(s);
    while (n && s[n - 1] == ' ') s[--n] = 0;
    while (*s == ' ') memmove(s, s + 1, strlen(s));
    return found;
}
static void talk_done(struct llm_job *j, int ok) {
    if (j->gen != g_llm_gen) return;
    int a = j->ai, b = j->to;
    void *pl = player_at(a), *po = player_at(b);
    if (!ok || (!j->reply[0] && !j->act) || !pl || !po) {
        g_talk_n_fail++;
        if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d -> P%d (message %d) no answer (%s)\n", a, b, j->talk, g_llm_last_err);
        if (j->offer >= 0) ai_offer_rules(b, a);   // its decision: the rules
        else if (j->dec && j->talk == 1 && g_offer_t[a][b]) {   // the offer's words failed: the other still answers it
            if (!talk_request(b, a, 2, NULL, NULL, j->dec)) ai_offer_rules(a, b);
            return;
        }
        if (!j->dec) g_talk.a = 0;
        return;
    }
    int did = j->act ? talk_apply(j) : 0;
    int ended = talk_end_mark(j->reply);   // "[end]": nothing more to say (its words before it still count)
    if (j->offer >= 0 && !(j->act & (TOOL_PEACE | TOOL_REFUSE)) && !j->confirm && g_offer_t[b][a]) {
        // words without a decision ("I accept your alliance, but I pay no gold" - and no tool): its words stand, it is
        // asked once more for the tool alone
        if (j->reply[0]) {
            pair_add(a, b, a, j->reply);
            g_talk_n_msg++;
            if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d -> P%d (message %d, %.1f s, no decision - asked again): %s\n", a, b,
                                            j->talk, j->ms / 1000.0, j->reply);
            if (g_llm_talk_show) {
                chat_line(pl, sysf("talk_to", pl, po, NULL, "text", j->reply, NULL), g_chat_ms);
            }
        }
        if (talk_request(a, b, j->talk, NULL, "", j->dec)) return;
    }
    if (j->offer >= 0 && !(did & TOOL_PEACE) && g_offer_t[b][a]) {   // it didn't take the offer: refused
        if (j->offer == 0) {   // closed now: the offerer may offer again in its answer (haggling)
            g_offer_t[b][a] = 0;
            g_offer_ignored[b][a]++;
        } else offer_refused(b, a);   // a capitulation: its cease-fire ends (diplo_tick)
        if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d refuses the %s of P%d\n", a, j->offer ? "capitulation" : "alliance offer", b);
    }
    if (j->confirm) {   // only its decision counts, not the words
        if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d decided when asked again: %s\n", a, did & TOOL_PEACE ? "accepted" : "refused");
        j->reply[0] = 0;
    }
    if (j->reply[0]) {
        pair_add(a, b, a, j->reply);
        g_talk_n_msg++;
        if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d -> P%d (message %d, %.1f s): %s\n", a, b, j->talk, j->ms / 1000.0, j->reply);
        if (g_llm_talk_show) {
            chat_line(pl, sysf("talk_to", pl, po, NULL, "text", j->reply, NULL), g_chat_ms);
        }
    }
    // the end it wrote - unless it took a step or its offer stands: those get the other's answer
    if (ended && !did && !g_offer_t[a][b]) {
        if (g_llm_log && g_log) fprintf(g_log, "# llm: talk P%d -> P%d: ends the exchange after message %d\n", a, b, j->talk);
        if (!j->dec) g_talk.a = 0;
        return;
    }
    // the next message: the other's, up to ai_talk_turns (an offer: 2 at least); whoever acted gets an answer
    int next = j->talk + 1, last = j->dec && g_llm_talk_turns < 2 ? 2 : g_llm_talk_turns;
    if (did && next > last) last = next;
    if (next <= last) {
        if (talk_request(b, a, next, NULL, NULL, j->dec)) { if (!j->dec) g_talk.turn = next; return; }
        if (g_offer_t[a][b] && g_players[b]) ai_offer_rules(a, b);   // its offer stands and nobody asks the other
    }
    if (!j->dec) g_talk.a = 0;
}

// fallback=pause: a request older than the timeout pauses the game until no such request is left (then it runs on -
// unless the player paused or resumed it meanwhile: theirs stands)
static void llm_wait_tick(void) {
    if (!llm_waits()) return;
    DWORD now = GetTickCount(), limit = (DWORD)g_llm_timeout * 1000u;
    int stalled = 0;
    for (int i = 0; i < LLM_JOBS; i++) {
        struct llm_job *j = &g_llm_jobs[i];
        LONG st = InterlockedCompareExchange(&j->state, J_FREE, J_FREE);
        stalled |= (st == J_QUEUED || st == J_RUNNING) && j->gen == g_llm_gen && now - j->t0 > limit;
    }
    int paused = *(uint8_t *)G_PAUSED != 0;
    if (stalled && !g_llm_paused && !paused) {
        if (!queue_pause_toggle()) return;
        g_llm_paused = 1;
        g_llm_paused_t = now;
        uint8_t *opt = *(uint8_t **)0x00917BB8;
        const char *l = sysf(g_llm_unreach ? "llm_wait_down" : "llm_wait", NULL, NULL, NULL, "url", g_llm_url, NULL);
        system_line(l, opt ? FIELD(opt, 0x2CC, int) * 2 : 20000);
        if (g_log) fprintf(g_log, "# llm: paused at %.1f min - waiting for the model (%s)\n", game_time() / 60000.0,
                           g_llm_unreach ? g_llm_last_err : "slow");
    } else if (stalled && g_llm_paused == 1 && !paused && now - g_llm_paused_t > 3000) {   // the player resumed it
        g_llm_paused = 2;
        if (g_log) fprintf(g_log, "# llm: the player resumed the game - the answers still come, no pause again for them\n");
    } else if (!stalled && g_llm_paused) {
        if (g_llm_paused == 1 && paused) queue_pause_toggle();
        if (g_log) fprintf(g_log, "# llm: answered after %.0f s - %s\n", (now - g_llm_paused_t) / 1000.0,
                           g_llm_paused == 1 && paused ? "the game runs on" : "the game was already running");
        g_llm_paused = 0;
    }
}
// every simulation step: show what is answered
static void llm_tick(void) {
    if (!g_llm_on) return;
    llm_wait_tick();

    // the server can't be reached: fixed lines for a while (llm_down); one answer failed - the server is there, only
    // that request went by the fixed line or the rules (llm_failed). Each once a game.
    LONG note = InterlockedExchange(&g_llm_fail_note, 0);
    int *told = note == 1 ? &g_llm_told : &g_llm_told_fail;
    if (note && !*told) {
        *told = 1;
        block_line((int)*(unsigned *)0x00918E84,   // shown and logged
                   sysf(note == 1 ? "llm_down" : "llm_failed", NULL, NULL, NULL, "url", g_llm_url, "error", g_llm_last_err, NULL));
    }
    talk_tick();
    for (int i = 0; i < LLM_JOBS; i++) {
        struct llm_job *j = &g_llm_jobs[i];
        LONG st = InterlockedCompareExchange(&j->state, J_FREE, J_FREE);   // read with a barrier: the reply is complete
        if (st != J_DONE && st != J_FAILED) continue;
        if (j->talk) {   // between AIs: its own order (one exchange at a time)
            talk_done(j, st == J_DONE);
            free(j->body);
            j->body = NULL;
            SET_STATE(j, J_FREE);
            continue;
        }
        int older = 0;   // an earlier request of the same AI still open: this one waits for it
        for (int o = 0; o < LLM_JOBS; o++)
            older |= o != i && g_llm_jobs[o].state != J_FREE && !g_llm_jobs[o].talk && g_llm_jobs[o].ai == j->ai &&
                     (int)(g_llm_jobs[o].seq - j->seq) < 0;
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
    int h2 = 0;
    if (sscanf(args, "prompt %d %d", &p, &h2) == 2) {   // the system message and the tools AI p would get towards h2: the log
        void *pl = player_at(p);
        if (!pl || !g_players[p] || !player_at(h2)) { REPLY("error: P%d is no AI or no player %d\n", p, h2); return; }
        struct sbuf b = { 0 };
        llm_system(&b, pl, p, h2, TOOL_PEACE | TOOL_WAR | TOOL_TRIBUTE);
        sb_tools(&b, TOOL_PEACE | TOOL_WAR | TOOL_TRIBUTE | TOOL_REFUSE, p, h2);
        if (g_log && b.p) fprintf(g_log, "# llm prompt P%d -> P%d: %s\n", p, h2, b.p);
        free(b.p);
        int open = g_players[h2] ? pair_tools(p, h2, game_time()) : llm_tools_for(p, h2, game_time());
        REPLY("written to the log; steps open now:%s%s%s%s (constrained %d)\n", open & TOOL_PEACE ? " peace" : "",
              open & TOOL_WAR ? " war" : "", open & TOOL_TRIBUTE ? " tribute" : "", open ? "" : " none", g_llm_constrained);
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
    REPLY("ai_talk %d (show %d, gap %d s, turns %d), ai_decide %d (%s): exchanges %u, messages %u, steps taken %u, failed %u, "
          "dropped %u%s\n", g_llm_ai_talk, g_llm_talk_show, g_llm_talk_gap, g_llm_talk_turns, g_llm_ai_decide,
          llm_ai_deciding() ? "deciding" : "not deciding", g_talk_n_ex, g_talk_n_msg, g_talk_n_acts, g_talk_n_fail,
          g_talk_n_drop, g_talk.a ? " (one running)" : "");
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
    g_llm_ai_talk = GetPrivateProfileIntA("llm", "ai_talk", 0, ini);
    g_llm_talk_show = GetPrivateProfileIntA("llm", "ai_talk_show", 1, ini);
    g_llm_talk_gap = GetPrivateProfileIntA("llm", "ai_talk_gap", 60, ini);
    g_llm_talk_turns = GetPrivateProfileIntA("llm", "ai_talk_turns", 4, ini);
    g_llm_ai_decide = GetPrivateProfileIntA("llm", "ai_decide", 0, ini);
    g_llm_equal = GetPrivateProfileIntA("llm", "equal", 0, ini);
    g_llm_memory = GetPrivateProfileIntA("llm", "memory", 8, ini);
    char mm[16];
    GetPrivateProfileStringA("llm", "memory_mode", "pair", mm, sizeof mm, ini);
    char fb[16];
    GetPrivateProfileStringA("llm", "fallback", "rules", fb, sizeof fb, ini);
    g_llm_constrained = GetPrivateProfileIntA("llm", "constrained", 1, ini);
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
        g_llm_ai_talk = GetPrivateProfileIntA("autotest", "llm_ai_talk", g_llm_ai_talk, test_ini);
        g_llm_talk_show = GetPrivateProfileIntA("autotest", "llm_ai_talk_show", g_llm_talk_show, test_ini);
        g_llm_talk_gap = GetPrivateProfileIntA("autotest", "llm_ai_talk_gap", g_llm_talk_gap, test_ini);
        g_llm_talk_turns = GetPrivateProfileIntA("autotest", "llm_ai_talk_turns", g_llm_talk_turns, test_ini);
        g_llm_ai_decide = GetPrivateProfileIntA("autotest", "llm_ai_decide", g_llm_ai_decide, test_ini);
        g_llm_equal = GetPrivateProfileIntA("autotest", "llm_equal", g_llm_equal, test_ini);
        g_llm_memory = GetPrivateProfileIntA("autotest", "llm_memory", g_llm_memory, test_ini);
        GetPrivateProfileStringA("autotest", "llm_memory_mode", mm, mm, sizeof mm, test_ini);
        GetPrivateProfileStringA("autotest", "llm_fallback", fb, fb, sizeof fb, test_ini);
        g_llm_constrained = GetPrivateProfileIntA("autotest", "llm_constrained", g_llm_constrained, test_ini);
        GetPrivateProfileStringA("autotest", "llm_url", g_llm_url, g_llm_url, sizeof g_llm_url, test_ini);
        GetPrivateProfileStringA("autotest", "llm_model", g_llm_model, g_llm_model, sizeof g_llm_model, test_ini);
    }
    g_llm_wait = !lstrcmpiA(fb, "pause");
    g_llm_mem_player = !lstrcmpiA(mm, "player");
    if (g_llm_timeout < 2) g_llm_timeout = 2;
    if (g_llm_timeout > 120) g_llm_timeout = 120;
    if (g_llm_max_tokens < 16) g_llm_max_tokens = 16;
    if (g_llm_max_tokens > 8000) g_llm_max_tokens = 8000;
    if (g_llm_talk_gap < 10) g_llm_talk_gap = 10;
    if (g_llm_talk_turns < 1) g_llm_talk_turns = 1;   // no upper limit: [end] or the turns, whatever comes first
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
    if (g_log) fprintf(g_log, "# llm: on - %s, model '%s', lines %d, chat %d, decide %d, ai_talk %d (show %d, gap %d s, turns %d), "
                       "ai_decide %d, equal %d, timeout %d s, fallback %s, constrained %d, memory %d (%s)\n", g_llm_url, g_llm_model, g_llm_lines,
                       g_llm_chat, g_llm_decide, g_llm_ai_talk, g_llm_talk_show, g_llm_talk_gap, g_llm_talk_turns, g_llm_ai_decide,
                       g_llm_equal, g_llm_timeout, g_llm_wait ? "pause" : "rules", g_llm_constrained, g_llm_memory,
                       g_llm_mem_player ? "per player" : "per pair");

}
