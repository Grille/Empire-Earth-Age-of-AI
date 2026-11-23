// THE LANGUAGE SERVER (ageofai_check --lsp): the checks of this program while a file is typed, for any editor that
// speaks the Language Server Protocol (JSON-RPC over stdin/stdout; VS Code: extensions/eeaimod-texts/extension.js).
// The editor sends the .eeai files it has open and every change to them; a moment after the last change (LSP_QUIET)
// each one is checked as `ageofai_check <it>` checks it - a personality file its whole folder, a script the script -
// with the open files' unsaved texts in place of the files (base/edits.c), and each mistake goes back as a diagnostic
// on its line. The mistakes of every file a check reads count: a mistake in mod\prompt.eeai shows there while a
// personality is typed.
// Each check runs in a process of its own (LSP_CHECK: this program again, the texts on its stdin): the personality
// folder's state is made to be read once (one game, one load) - a fresh process reads it as the game would, every time.
// Stage 1 (the player 2026-11-22): the mistakes, by line. Stage 2: completion and hover (check/lsp_know.c: from the
// game's tables and the files' names, answered by this process at once). Stage 3: definitions, references, the types
// in the hover and the members by type after a dot (check/lsp_refs.c: a check's process asked for its record of the
// names - expr.c THE LANGUAGE SERVER'S RECORD).
#include <fcntl.h>
#include <io.h>
#define LSP_QUIET 300   // ms after the last change
#define LSP_CHECK "--lsp-check"

// JSON: the few things read of a message - a member of an object, an item of an array, a string decoded
internal const char *js_ws(const char *s) { while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++; return s; }
internal const char *js_skip(const char *s) {   // past one value
    s = js_ws(s);
    if (*s == '"') {
        for (s++; *s && *s != '"'; s++) if (*s == '\\' && s[1]) s++;
        return *s ? s + 1 : s;
    }
    if (*s == '{' || *s == '[') {
        int d = 0;
        while (*s) {
            if (*s == '"') { s = js_skip(s); continue; }
            if (*s == '{' || *s == '[') d++;
            else if ((*s == '}' || *s == ']') && !--d) return s + 1;
            s++;
        }
        return s;
    }
    while (*s && *s != ',' && *s != '}' && *s != ']' && *s != ' ' && *s != '\r' && *s != '\n') s++;
    return s;
}
internal const char *js_get(const char *obj, const char *key) {   // obj's member key, NULL none
    if (!obj) return NULL;
    const char *s = js_ws(obj);
    if (*s != '{') return NULL;
    size_t kl = strlen(key);
    for (s++;;) {
        s = js_ws(s);
        if (*s != '"') return NULL;
        const char *k = s + 1, *e = js_skip(s);
        int hit = (size_t)(e - 1 - k) == kl && !strncmp(k, key, kl);
        s = js_ws(e);
        if (*s != ':') return NULL;
        s = js_ws(s + 1);
        if (hit) return s;
        s = js_ws(js_skip(s));
        if (*s != ',') return NULL;
        s++;
    }
}
internal const char *js_at(const char *arr, int i) {   // arr's item i, NULL none
    if (!arr || *(arr = js_ws(arr)) != '[') return NULL;
    const char *s = js_ws(arr + 1);
    for (int k = 0; *s && *s != ']'; k++) {
        if (k == i) return s;
        s = js_ws(js_skip(s));
        if (*s == ',') s = js_ws(s + 1);
    }
    return NULL;
}
internal void js_utf8(char **o, unsigned c) {
    char *p = *o;
    if (c < 0x80) *p++ = (char)c;
    else if (c < 0x800) { *p++ = (char)(0xC0 | c >> 6); *p++ = (char)(0x80 | (c & 63)); }
    else if (c < 0x10000) { *p++ = (char)(0xE0 | c >> 12); *p++ = (char)(0x80 | (c >> 6 & 63)); *p++ = (char)(0x80 | (c & 63)); }
    else { *p++ = (char)(0xF0 | c >> 18); *p++ = (char)(0x80 | (c >> 12 & 63)); *p++ = (char)(0x80 | (c >> 6 & 63)); *p++ = (char)(0x80 | (c & 63)); }
    *o = p;
}
internal char *js_str(const char *v, size_t *len) {   // a string value decoded (UTF-8), malloc'd - NULL: no string
    if (!v || *(v = js_ws(v)) != '"') return NULL;
    const char *e = js_skip(v);
    char *out = malloc((size_t)(e - v) + 1), *o = out;
    if (!out) return NULL;
    for (const char *s = v + 1; s < e - 1; s++) {
        if (*s != '\\') { *o++ = *s; continue; }
        switch (*++s) {
        case 'n': *o++ = '\n'; break;
        case 't': *o++ = '\t'; break;
        case 'r': *o++ = '\r'; break;
        case 'b': *o++ = '\b'; break;
        case 'f': *o++ = '\f'; break;
        case 'u': {
            unsigned c = (unsigned)strtoul((char[5]){ s[1], s[2], s[3], s[4], 0 }, NULL, 16);
            s += 4;
            if (c >= 0xD800 && c < 0xDC00 && s[1] == '\\' && s[2] == 'u') {   // a surrogate pair
                unsigned lo = (unsigned)strtoul((char[5]){ s[3], s[4], s[5], s[6], 0 }, NULL, 16);
                if (lo >= 0xDC00 && lo < 0xE000) { c = 0x10000 + ((c - 0xD800) << 10) + (lo - 0xDC00); s += 6; }
            }
            js_utf8(&o, c);
            break;
        }
        default: *o++ = *s;   // " \ /
        }
    }
    *o = 0;
    if (len) *len = (size_t)(o - out);
    return out;
}
// a growing text: the messages written
struct lsb { char *s; size_t n, cap; };
internal void lsb_put(struct lsb *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) {
        size_t c = b->cap ? b->cap * 2 : 4096;
        while (c < b->n + n + 1) c *= 2;
        char *t = realloc(b->s, c);
        if (!t) return;
        b->s = t; b->cap = c;
    }
    memcpy(b->s + b->n, s, n);
    b->n += n;
    b->s[b->n] = 0;
}
internal void lsb_str(struct lsb *b, const char *s) { lsb_put(b, s, strlen(s)); }
internal void lsb_json(struct lsb *b, const char *s) {   // s (UTF-8) as a JSON string
    lsb_put(b, "\"", 1);
    for (; *s; s++) {
        char e[8];
        if (*s == '"' || *s == '\\') { e[0] = '\\'; e[1] = *s; lsb_put(b, e, 2); }
        else if ((unsigned char)*s < 0x20) { snprintf(e, sizeof e, "\\u%04x", (unsigned char)*s); lsb_put(b, e, 6); }
        else lsb_put(b, s, 1);
    }
    lsb_put(b, "\"", 1);
}
internal void lsp_send(const char *body) {
    char head[64];
    snprintf(head, sizeof head, "Content-Length: %u\r\n\r\n", (unsigned)strlen(body));
    fputs(head, stdout);
    fputs(body, stdout);
    fflush(stdout);
}
// a file:// URI and a path: file:///x%3A/a%20b/c.eeai <-> x:\a b\c.eeai
internal void lsp_uri_path(const char *uri, char *out, int cap) {
    const char *s = !_strnicmp(uri, "file:///", 8) ? uri + 8 : uri;
    int n = 0;
    for (; *s && n < cap - 1; s++) {
        char c = *s;
        if (c == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) { c = (char)strtoul((char[3]){ s[1], s[2], 0 }, NULL, 16); s += 2; }
        out[n++] = c == '/' ? '\\' : c;
    }
    out[n] = 0;
}
internal void lsp_path_uri(const char *path, struct lsb *b) {   // as a JSON string
    lsb_str(b, "\"file:///");
    for (const char *s = path; *s; s++) {
        unsigned char c = (unsigned char)*s;
        char e[4];
        if (c == '\\') lsb_put(b, "/", 1);
        else if (s == path && s[1] == ':' && isalpha(c)) { char lc = (char)tolower(c); lsb_put(b, &lc, 1); }   // x: as the editor writes it
        else if (isalnum(c) || strchr("-._~/", c) || (c == ':' && s == path + 1)) lsb_put(b, s, 1);
        else { snprintf(e, sizeof e, "%%%02X", c); lsb_put(b, e, 3); }
    }
    lsb_str(b, "\"");
}

// THE OPEN FILES: as the editor has them
#define LSP_DOCS 32
internal struct { char uri[1024], path[MAX_PATH]; char *text; size_t n; } g_lsp_doc[LSP_DOCS];
internal int g_lsp_ndoc;
internal int lsp_doc(const char *uri) { for (int i = 0; i < g_lsp_ndoc; i++) if (!strcmp(g_lsp_doc[i].uri, uri)) return i; return -1; }
// what a check of path checks (ageofai_check's own rule): a personality file its folder, a script itself
internal void lsp_scope(const char *path, char *out) {
    char dir[MAX_PATH], up[MAX_PATH];
    lstrcpyA(dir, path);
    char *s = strrchr(dir, '\\');
    if (s) *s = 0;
    lstrcpyA(up, dir);
    s = strrchr(up, '\\');
    if (s && !lstrcmpiA(s + 1, "mod")) *s = 0;
    if (chk_is_folder(dir)) lstrcpyA(out, dir);
    else if (chk_is_folder(up)) lstrcpyA(out, up);
    else lstrcpyA(out, path);
}

// THE MISTAKES, by file: those published last (cleared when a check finds none there)
struct lsp_mis { char path[MAX_PATH]; int line; char *what; };
internal struct lsp_mis *g_lsp_mis;
internal int g_lsp_nmis, g_lsp_mis_cap;
internal char (*g_lsp_pub)[MAX_PATH];   // the files published with mistakes
internal int g_lsp_npub;
internal void lsp_mis_add(const char *path, int line, const char *what) {
    for (int i = 0; i < g_lsp_nmis; i++) if (g_lsp_mis[i].line == line && !lstrcmpiA(g_lsp_mis[i].path, path) && !strcmp(g_lsp_mis[i].what, what)) return;
    if (g_lsp_nmis == g_lsp_mis_cap) {
        int c = g_lsp_mis_cap ? g_lsp_mis_cap * 2 : 64;
        struct lsp_mis *t = realloc(g_lsp_mis, sizeof *t * (size_t)c);
        if (!t) return;
        g_lsp_mis = t; g_lsp_mis_cap = c;
    }
    lstrcpynA(g_lsp_mis[g_lsp_nmis].path, path, MAX_PATH);
    g_lsp_mis[g_lsp_nmis].line = line;
    g_lsp_mis[g_lsp_nmis++].what = _strdup(what);
}
// one check: this program again on scope, the open files' texts on its stdin - and a question (query: the record of
// the names, expr.c THE LANGUAGE SERVER'S RECORD - "" all of it, "path<tab>line<tab>column" the type there too; NULL
// none) - its output into *out
internal void lsp_run(const char *scope, const char *query, struct lsb *out) {
    char self[MAX_PATH], cmd[MAX_PATH * 2 + 32];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    snprintf(cmd, sizeof cmd, "\"%s\" " LSP_CHECK " \"%s\"", self, scope);
    SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
    HANDLE in_r, in_w, out_r, out_w;
    if (!CreatePipe(&in_r, &in_w, &sa, 1 << 20)) return;
    if (!CreatePipe(&out_r, &out_w, &sa, 1 << 16)) { CloseHandle(in_r); CloseHandle(in_w); return; }
    SetHandleInformation(in_w, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = in_r; si.hStdOutput = out_w; si.hStdError = out_w;
    PROCESS_INFORMATION pi;
    BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(in_r);
    CloseHandle(out_w);
    if (!ok) { CloseHandle(in_w); CloseHandle(out_r); return; }
    for (int i = 0; i < g_lsp_ndoc; i++) {   // path, its length, its text
        char head[MAX_PATH + 32];
        DWORD w;
        snprintf(head, sizeof head, "%s\n%u\n", g_lsp_doc[i].path, (unsigned)g_lsp_doc[i].n);
        WriteFile(in_w, head, (DWORD)strlen(head), &w, NULL);
        for (size_t at = 0; at < g_lsp_doc[i].n; at += w) if (!WriteFile(in_w, g_lsp_doc[i].text + at, (DWORD)(g_lsp_doc[i].n - at), &w, NULL) || !w) break;
    }
    if (query) {   // the question: a text of the path "?"
        char head[64];
        DWORD w;
        snprintf(head, sizeof head, "?\n%u\n", (unsigned)strlen(query));
        WriteFile(in_w, head, (DWORD)strlen(head), &w, NULL);
        WriteFile(in_w, query, (DWORD)strlen(query), &w, NULL);
    }
    CloseHandle(in_w);
    char buf[4096];
    DWORD got;
    while (ReadFile(out_r, buf, sizeof buf, &got, NULL) && got) lsb_put(out, buf, got);
    CloseHandle(out_r);
    if (WaitForSingleObject(pi.hProcess, 10000) == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
// a check of scope for its mistakes: each line "path.eeai:line: what" one (in the code page's text: made UTF-8)
internal void lsp_check(const char *scope) {
    struct lsb out = { 0 };
    lsp_run(scope, NULL, &out);
    for (char *l = out.s; l && *l; ) {
        char *e = strchr(l, '\n');
        if (e) *e = 0;
        size_t n = strlen(l);
        if (n && l[n - 1] == '\r') l[--n] = 0;
        char *m = l[0] == '@' && l[1] == '@' ? NULL : strstr(l, ".eeai:");   // (@@: the record's)
        if (m && isdigit((unsigned char)m[6])) {
            m[5] = 0;
            int line = atoi(m + 6);
            const char *what = m + 6 + strspn(m + 6, "0123456789");
            if (*what == ':') what++;
            while (*what == ' ') what++;
            wchar_t w[2048];
            char u[6144];
            int wn = MultiByteToWideChar(CP_ACP, 0, what, -1, w, 2048);
            if (wn <= 0 || !WideCharToMultiByte(CP_UTF8, 0, w, -1, u, sizeof u, NULL, NULL)) lstrcpynA(u, what, sizeof u);
            lsp_mis_add(l, line > 0 ? line : 1, u);
        }
        l = e ? e + 1 : NULL;
    }
    free(out.s);
}
internal void lsp_publish_one(const char *path) {   // the mistakes in path now (none: cleared)
    struct lsb b = { 0 };
    lsb_str(&b, "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":");
    int d = -1;
    for (int i = 0; i < g_lsp_ndoc && d < 0; i++) if (!lstrcmpiA(g_lsp_doc[i].path, path)) d = i;
    if (d >= 0) lsb_json(&b, g_lsp_doc[d].uri);   // an open file: the editor's own URI for it
    else lsp_path_uri(path, &b);
    lsb_str(&b, ",\"diagnostics\":[");
    int k = 0;
    for (int i = 0; i < g_lsp_nmis; i++) {
        if (lstrcmpiA(g_lsp_mis[i].path, path)) continue;
        char pos[160];
        snprintf(pos, sizeof pos, "%s{\"range\":{\"start\":{\"line\":%d,\"character\":0},\"end\":{\"line\":%d,\"character\":10000}},"
                 "\"severity\":1,\"source\":\"ageofai_check\",\"message\":", k++ ? "," : "", g_lsp_mis[i].line - 1, g_lsp_mis[i].line - 1);
        lsb_str(&b, pos);
        lsb_json(&b, g_lsp_mis[i].what);
        lsb_str(&b, "}");
    }
    lsb_str(&b, "]}}");
    lsp_send(b.s);
    free(b.s);
}
// every open file's scope checked (each once), the mistakes published - and cleared where there are none now
internal void lsp_check_all(void) {
    for (int i = 0; i < g_lsp_nmis; i++) free(g_lsp_mis[i].what);
    g_lsp_nmis = 0;
    char done[LSP_DOCS][MAX_PATH];
    int nd = 0;
    for (int i = 0; i < g_lsp_ndoc; i++) {
        char sc[MAX_PATH];
        lsp_scope(g_lsp_doc[i].path, sc);
        int seen = 0;
        for (int k = 0; k < nd && !seen; k++) seen = !lstrcmpiA(done[k], sc);
        if (seen) continue;
        lstrcpyA(done[nd++], sc);
        lsp_check(sc);
    }
    char (*now)[MAX_PATH] = malloc(sizeof *now * (size_t)(g_lsp_nmis + g_lsp_ndoc + 1));
    int nn = 0;
    for (int i = 0; now && i < g_lsp_nmis; i++) {   // the files with mistakes now, and the open ones (published: cleared too)
        int seen = 0;
        for (int k = 0; k < nn && !seen; k++) seen = !lstrcmpiA(now[k], g_lsp_mis[i].path);
        if (!seen) lstrcpyA(now[nn++], g_lsp_mis[i].path);
    }
    for (int i = 0; now && i < g_lsp_ndoc; i++) {
        int seen = 0;
        for (int k = 0; k < nn && !seen; k++) seen = !lstrcmpiA(now[k], g_lsp_doc[i].path);
        if (!seen) lstrcpyA(now[nn++], g_lsp_doc[i].path);
    }
    for (int i = 0; i < nn; i++) lsp_publish_one(now[i]);
    for (int i = 0; i < g_lsp_npub; i++) {   // those of before without mistakes now
        int seen = 0;
        for (int k = 0; k < nn && !seen; k++) seen = !lstrcmpiA(now[k], g_lsp_pub[i]);
        if (!seen) lsp_publish_one(g_lsp_pub[i]);
    }
    free(g_lsp_pub);
    g_lsp_pub = now;
    g_lsp_npub = now ? nn : 0;
}

// a message from the editor: its headers, then Content-Length bytes - NULL at the end of its input
internal char *lsp_read(HANDLE in) {
    char line[256];
    size_t len = 0;
    for (;;) {
        int n = 0;
        DWORD got;
        char c;
        while (n < (int)sizeof line - 1) {
            if (!ReadFile(in, &c, 1, &got, NULL) || !got) return NULL;
            if (c == '\n') break;
            if (c != '\r') line[n++] = c;
        }
        line[n] = 0;
        if (!n) break;
        if (!_strnicmp(line, "Content-Length:", 15)) len = (size_t)strtoul(line + 15, NULL, 10);
    }
    char *body = malloc(len + 1);
    if (!body) return NULL;
    for (size_t at = 0; at < len; ) {
        DWORD got;
        if (!ReadFile(in, body + at, (DWORD)(len - at), &got, NULL) || !got) { free(body); return NULL; }
        at += got;
    }
    body[len] = 0;
    return body;
}
internal void lsp_reply(const char *id, const char *result) {   // id: its JSON as sent
    struct lsb b = { 0 };
    lsb_str(&b, "{\"jsonrpc\":\"2.0\",\"id\":");
    lsb_put(&b, id, (size_t)(js_skip(id) - id));
    lsb_str(&b, ",");
    lsb_str(&b, result);
    lsb_str(&b, "}");
    lsp_send(b.s);
    free(b.s);
}
#include "check/lsp_know.c"   // completion and hover
#include "check/lsp_refs.c"   // definitions, references, types: the check's record
internal int lsp_main(void) {
    _setmode(_fileno(stdout), _O_BINARY);
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    int pending = 0;
    DWORD changed = 0;
    for (;;) {
        if (pending) {   // a moment of quiet after the last change: the checks
            DWORD avail = 0;
            if (!PeekNamedPipe(in, NULL, 0, NULL, &avail, NULL)) avail = 1;   // (no pipe: read on, blocking)
            if (!avail) {
                if (GetTickCount() - changed < LSP_QUIET) { Sleep(15); continue; }
                pending = 0;
                lsp_check_all();
                continue;
            }
        }
        char *msg = lsp_read(in);
        if (!msg) return 0;
        char *method = js_str(js_get(msg, "method"), NULL);
        const char *id = js_get(msg, "id"), *params = js_get(msg, "params");
        const char *td = js_get(params, "textDocument");
        char *uri = js_str(js_get(td, "uri"), NULL);
        if (!method) ;   // a reply to the server: it asks nothing
        else if (!strcmp(method, "initialize") && id)
            lsp_reply(id, "\"result\":{\"capabilities\":{\"textDocumentSync\":{\"openClose\":true,\"change\":1,\"save\":{\"includeText\":false}},"
                          "\"completionProvider\":{\"triggerCharacters\":[\".\"]},\"hoverProvider\":true,"
                          "\"definitionProvider\":true,\"referencesProvider\":true},"
                          "\"serverInfo\":{\"name\":\"ageofai_check\"}}");
        else if ((!strcmp(method, "textDocument/completion") || !strcmp(method, "textDocument/hover") ||
                  !strcmp(method, "textDocument/definition") || !strcmp(method, "textDocument/references")) && id) {
            int d = uri ? lsp_doc(uri) : -1;
            const char *pos = js_get(params, "position"), *pl = js_get(pos, "line"), *pc = js_get(pos, "character");
            const char *incl = js_get(js_get(params, "context"), "includeDeclaration");
            struct lsb r = { 0 };
            if (d < 0 || !g_lsp_doc[d].text || !pl || !pc) lsb_str(&r, "\"result\":null");
            else if (!strcmp(method, "textDocument/completion")) lsp_complete(d, atoi(pl), atoi(pc), &r);
            else if (!strcmp(method, "textDocument/hover")) lsp_hover(d, atoi(pl), atoi(pc), &r);
            else if (!strcmp(method, "textDocument/definition")) lsp_definition(d, atoi(pl), atoi(pc), &r);
            else lsp_references(d, atoi(pl), atoi(pc), !incl || *incl != 'f', &r);
            lsp_reply(id, r.s);
            free(r.s);
        }
        else if (!strcmp(method, "shutdown") && id) lsp_reply(id, "\"result\":null");
        else if (!strcmp(method, "exit")) { free(method); free(uri); free(msg); return 0; }
        else if ((!strcmp(method, "textDocument/didOpen") || !strcmp(method, "textDocument/didChange")) && uri && strlen(uri) < sizeof g_lsp_doc[0].uri) {
            size_t n = 0;
            int open = !strcmp(method, "textDocument/didOpen");   // the whole text; a change: the whole text too (change 1)
            char *text = open ? js_str(js_get(td, "text"), &n) : js_str(js_get(js_at(js_get(params, "contentChanges"), 0), "text"), &n);
            int d = lsp_doc(uri);
            if (d < 0 && g_lsp_ndoc < LSP_DOCS && !_strnicmp(uri, "file:", 5)) {
                d = g_lsp_ndoc++;
                lstrcpyA(g_lsp_doc[d].uri, uri);
                lsp_uri_path(uri, g_lsp_doc[d].path, MAX_PATH);
                g_lsp_doc[d].text = NULL;
            }
            if (d >= 0 && text) { free(g_lsp_doc[d].text); g_lsp_doc[d].text = text; g_lsp_doc[d].n = n; text = NULL; pending = 1; changed = GetTickCount(); g_lsp_gen++; }
            free(text);
        } else if (!strcmp(method, "textDocument/didSave") && uri) { pending = 1; changed = GetTickCount() - LSP_QUIET; }
        else if (!strcmp(method, "textDocument/didClose") && uri) {
            int d = lsp_doc(uri);
            if (d >= 0) { free(g_lsp_doc[d].text); g_lsp_doc[d] = g_lsp_doc[--g_lsp_ndoc]; pending = 1; changed = GetTickCount(); g_lsp_gen++; }
        } else if (id) lsp_reply(id, "\"error\":{\"code\":-32601,\"message\":\"not known to ageofai_check\"}");
        free(method);
        free(uri);
        free(msg);
    }
}
internal void edit_text_set(const char *path, const char *text, size_t n) {   // base/edits.c: path read as text (a copy)
    char full[MAX_PATH];
    if (g_edits_n == EDITS_MAX || !GetFullPathNameA(path, MAX_PATH, full, NULL)) return;
    char *t = malloc(n + 1);
    if (!t) return;
    memcpy(t, text, n);
    t[n] = 0;
    lstrcpyA(g_edits[g_edits_n].path, full);
    g_edits[g_edits_n].text = t;
    g_edits[g_edits_n++].n = n;
}
internal void lsp_xr_out(const char *line) { fputs(line, stdout); fputc('\n', stdout); }   // a line of the record
// the check's own process (LSP_CHECK scope): the editor's texts from stdin - each its path, its length, its bytes -
// in place of the files, then scope checked as the command line would; its mistakes to stdout. A text of the path
// "?" is the server's question: the record of the names (expr.c THE LANGUAGE SERVER'S RECORD) to stdout too - its
// text "path<tab>line<tab>column" asks for the type that ends there as well, "" for none
internal void lsp_child_texts(void) {
    _setmode(_fileno(stdin), _O_BINARY);
    char path[MAX_PATH], num[32];
    while (fgets(path, sizeof path, stdin) && fgets(num, sizeof num, stdin)) {
        path[strcspn(path, "\r\n")] = 0;
        size_t n = (size_t)strtoul(num, NULL, 10);
        char *t = malloc(n + 1);
        if (!t || fread(t, 1, n, stdin) != n) { free(t); break; }
        t[n] = 0;
        if (!strcmp(path, "?")) {
            char *tab = strchr(t, '\t'), *tab2 = tab ? strchr(tab + 1, '\t') : NULL;
            g_xr_out = lsp_xr_out;
            if (tab2) {
                *tab = 0;
                if (!GetFullPathNameA(t, MAX_PATH, g_xr_qpath, NULL)) lstrcpynA(g_xr_qpath, t, MAX_PATH);
                g_xr_qline = atoi(tab + 1);
                g_xr_qcol = atoi(tab2 + 1);
            }
        } else edit_text_set(path, t, n);
        free(t);
    }
}
