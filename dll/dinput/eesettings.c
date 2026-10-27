// EMPIRE EARTH'S SETTINGS IN A FILE (optional, built as dinput.dll): the game keeps its settings in the registry,
// HKCU\Software\SSSI\Empire Earth (60 values, subkey "Game Options"; Art of Conquest the same under
// HKCU\Software\Mad Doc Software\EE-AOC), and reads them all before the engine loads
// eeaimod.dll (docs/engine-notes.md, Exe builds - Settings). This DLL is loaded by Windows at the start instead: the
// engine (Low-Level Engine.dll) imports DINPUT.dll, which isn't one of the system's KnownDLLs, so a dinput.dll in the
// game folder comes first. It passes the engine's one DirectInput call through to the system's dinput.dll and points
// the engine's registry calls (its 7 ADVAPI32 imports) for the game's own keys to eesettings.ini in the mod's folder
// (<game>\aimod\, with aimod.ini):
// the settings travel with the folder, every copy of the game has its own (test slots: EEAIMOD_SETTINGS=<file>).
// Optional in every way: without this DLL - or with another program's dinput.dll in its place - the game uses the
// registry as before, and so does it when the file can't be read or written. The first start without the file copies
// the registry's values into it. EEAIMOD_SETTINGS_FILE tells eeaimod.dll (its log) which one is in use.
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../base/lang.h"   // the words base/ is written with (internal)
#include "../base/headless_win.c"

// ---- the system's dinput.dll: the engine's DirectInputCreateA (and the rest of its exports, for anyone else) ----
static HMODULE g_real;
static FARPROC real_fn(const char *name) {
    if (!g_real) {
        char p[MAX_PATH];
        UINT n = GetSystemDirectoryA(p, MAX_PATH - 12);   // a 32-bit process: Windows maps it to SysWOW64
        if (!n || n >= MAX_PATH - 12) return NULL;
        lstrcatA(p, "\\dinput.dll");
        g_real = LoadLibraryA(p);
    }
    return g_real ? GetProcAddress(g_real, name) : NULL;
}
typedef HRESULT (WINAPI *Create4)(HINSTANCE, DWORD, void **, void *);
typedef HRESULT (WINAPI *Create5)(HINSTANCE, DWORD, REFIID, void **, void *);
HRESULT WINAPI DirectInputCreateA(HINSTANCE h, DWORD v, void **o, void *u) {
    Create4 f = (Create4)(void *)real_fn("DirectInputCreateA");
    return f ? f(h, v, o, u) : E_FAIL;
}
HRESULT WINAPI DirectInputCreateW(HINSTANCE h, DWORD v, void **o, void *u) {
    Create4 f = (Create4)(void *)real_fn("DirectInputCreateW");
    return f ? f(h, v, o, u) : E_FAIL;
}
HRESULT WINAPI DirectInputCreateEx(HINSTANCE h, DWORD v, REFIID r, void **o, void *u) {
    Create5 f = (Create5)(void *)real_fn("DirectInputCreateEx");
    return f ? f(h, v, r, o, u) : E_FAIL;
}
HRESULT WINAPI DllCanUnloadNow(void) { return S_FALSE; }
HRESULT WINAPI DllGetClassObject(REFCLSID c, REFIID r, void **o) {
    typedef HRESULT (WINAPI *F)(REFCLSID, REFIID, void **);
    F f = (F)(void *)real_fn("DllGetClassObject");
    return f ? f(c, r, o) : CLASS_E_CLASSNOTAVAILABLE;
}
HRESULT WINAPI DllRegisterServer(void) { return E_NOTIMPL; }
HRESULT WINAPI DllUnregisterServer(void) { return E_NOTIMPL; }
// this DLL's mark (build.sh install replaces a dinput.dll only when it has it): the settings file in use, or NULL
const char *WINAPI EEAimodSettings(void);

// ---- the settings: keys with their values, in memory; the file is written when the game has changed them ----
// (Art of Conquest's: Mad Doc Software\EE-AOC, the same values)
static const char *const k_roots[] = { "HKCU\\Software\\SSSI\\Empire Earth", "HKCU\\Software\\Neo\\Empire Earth",
                                       "HKCU\\Software\\Mad Doc Software\\EE-AOC" };
#define NKEYS 32
#define NVALS 128
struct sval { char name[80]; DWORD type, n; BYTE *data; };
struct skey { char path[200]; int nv, live; struct sval v[NVALS]; };
static struct skey g_keys[NKEYS];
static char g_file[MAX_PATH];
static int g_dirty, g_on;
static CRITICAL_SECTION g_cs;

static int is_virtual(const char *path) {   // one of the game's own keys (or below)
    for (size_t i = 0; i < sizeof k_roots / sizeof *k_roots; i++) {
        size_t n = strlen(k_roots[i]);
        if (!_strnicmp(path, k_roots[i], n) && (!path[n] || path[n] == '\\')) return 1;
    }
    return 0;
}
static struct skey *key_find(const char *path, int create) {
    struct skey *free_k = NULL;
    for (int i = 0; i < NKEYS; i++) {
        if (g_keys[i].live && !lstrcmpiA(g_keys[i].path, path)) return &g_keys[i];
        if (!g_keys[i].live && !free_k) free_k = &g_keys[i];
    }
    if (!create || !free_k || strlen(path) >= sizeof free_k->path) return NULL;
    memset(free_k, 0, sizeof *free_k);
    lstrcpyA(free_k->path, path);
    free_k->live = 1;
    return free_k;
}
static int key_exists(const char *path) {   // a key, or one below it (an empty key in between has no record)
    size_t n = strlen(path);
    for (int i = 0; i < NKEYS; i++)
        if (g_keys[i].live && !_strnicmp(g_keys[i].path, path, n) && (!g_keys[i].path[n] || g_keys[i].path[n] == '\\')) return 1;
    for (size_t i = 0; i < sizeof k_roots / sizeof *k_roots; i++) if (!lstrcmpiA(path, k_roots[i])) return 1;   // a root: always
    return 0;
}
static struct sval *val_find(struct skey *k, const char *name) {
    for (int i = 0; i < k->nv; i++) if (!lstrcmpiA(k->v[i].name, name)) return &k->v[i];
    return NULL;
}
static int val_set(struct skey *k, const char *name, DWORD type, const BYTE *data, DWORD n) {
    struct sval *v = val_find(k, name);
    if (!v) {
        if (k->nv == NVALS || strlen(name) >= sizeof v->name) return 0;
        v = &k->v[k->nv++];
        memset(v, 0, sizeof *v);
        lstrcpyA(v->name, name);
    }
    BYTE *d = malloc(n ? n : 1);
    if (!d) return 0;
    if (n) memcpy(d, data, n);
    free(v->data);
    v->data = d;
    v->type = type;
    v->n = n;
    return 1;
}

// ---- the file: [<key path>] sections; Name=123 a DWORD, Name="text" a string (\" \\ escaped), Name=hex(t):aa,bb ----
static void str_write(FILE *f, const BYTE *s, DWORD n) {
    fputc('"', f);
    for (DWORD i = 0; i < n && s[i]; i++) {
        if (s[i] == '"' || s[i] == '\\') fputc('\\', f);
        fputc(s[i], f);
    }
    fputc('"', f);
}
static int file_save(void) {   // 1: written (or nothing to write)
    if (!g_dirty) return 1;
    char tmp[MAX_PATH + 8];
    wsprintfA(tmp, "%s.new", g_file);
    FILE *f = fopen(tmp, "wb");
    if (!f) return 0;
    fputs("; Empire Earth's settings - kept here by dinput.dll of Empire Earth: Age of AI (eeaimod) instead of the registry.\r\n"
          "; The game writes this file when it closes. Without that dinput.dll the game uses the registry again.\r\n"
          "; A plain number is a number, \"text\" in quotes a text.\r\n", f);
    for (int i = 0; i < NKEYS; i++) {
        struct skey *k = &g_keys[i];
        if (!k->live) continue;
        fprintf(f, "\r\n[%s]\r\n", k->path);
        for (int j = 0; j < k->nv; j++) {
            struct sval *v = &k->v[j];
            fprintf(f, "%s=", v->name);
            if (v->type == REG_DWORD && v->n == 4) fprintf(f, "%lu", (unsigned long)*(DWORD *)v->data);
            else if ((v->type == REG_SZ || v->type == REG_EXPAND_SZ) && v->n) {   // (0 bytes, no 0 even: hex below)
                if (v->type == REG_EXPAND_SZ) fputs("expand:", f);
                str_write(f, v->data, v->n);
            } else {
                fprintf(f, "hex(%lu):", (unsigned long)v->type);
                for (DWORD b = 0; b < v->n; b++) fprintf(f, "%s%02x", b ? "," : "", v->data[b]);
            }
            fputs("\r\n", f);
        }
    }
    int ok = !ferror(f);
    if (fclose(f)) ok = 0;
    if (ok) ok = MoveFileExA(tmp, g_file, MOVEFILE_REPLACE_EXISTING);   // never half a file
    if (ok) g_dirty = 0;
    else DeleteFileA(tmp);
    return ok;
}
static int file_load(void) {   // 1: read; 0: not there or not readable
    FILE *f = fopen(g_file, "rb");
    if (!f) return 0;
    char line[2048];
    struct skey *k = NULL;
    while (fgets(line, sizeof line, f)) {
        char *e = line + strlen(line);
        while (e > line && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ')) *--e = 0;
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (!*s || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            char *c = strrchr(s, ']');
            if (c) { *c = 0; k = is_virtual(s + 1) ? key_find(s + 1, 1) : NULL; }
            continue;
        }
        char *eq = strchr(s, '=');
        if (!k || !eq) continue;
        *eq = 0;
        char *v = eq + 1;
        if (*v == '"' || !_strnicmp(v, "expand:\"", 8)) {
            DWORD type = *v == '"' ? REG_SZ : REG_EXPAND_SZ;
            if (type == REG_EXPAND_SZ) v += 7;
            char out[2048];
            DWORD n = 0;
            for (v++; *v && *v != '"' && n < sizeof out - 1; v++) { if (*v == '\\' && v[1]) v++; out[n++] = *v; }
            out[n++] = 0;
            val_set(k, s, type, (BYTE *)out, n);
        } else if (!_strnicmp(v, "hex(", 4)) {
            char *p;
            DWORD type = strtoul(v + 4, &p, 10), n = 0;
            BYTE out[1024];
            if (*p == ')') p++;
            if (*p == ':') p++;
            while (*p && n < sizeof out) { out[n++] = (BYTE)strtoul(p, &p, 16); if (*p == ',') p++; else break; }
            val_set(k, s, type, out, n);
        } else {
            DWORD d = strtoul(v, NULL, 10);
            val_set(k, s, REG_DWORD, (BYTE *)&d, 4);
        }
    }
    fclose(f);
    return 1;
}
static void import_key(HKEY h, const char *path) {   // the registry's values below path, at the first start
    struct skey *k = key_find(path, 1);
    if (!k) return;
    char name[256];
    BYTE data[2048];
    for (DWORD i = 0;; i++) {
        DWORD nn = sizeof name, dn = sizeof data, type;
        LONG e = RegEnumValueA(h, i, name, &nn, NULL, &type, data, &dn);
        if (e == ERROR_NO_MORE_ITEMS) break;
        if (e == ERROR_SUCCESS) val_set(k, name, type, data, dn);
    }
    for (DWORD i = 0;; i++) {
        DWORD nn = sizeof name;
        if (RegEnumKeyExA(h, i, name, &nn, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
        HKEY c;
        char sub[200];
        if (snprintf(sub, sizeof sub, "%s\\%s", path, name) >= (int)sizeof sub) continue;
        if (RegOpenKeyExA(h, name, 0, KEY_READ, &c) == ERROR_SUCCESS) { import_key(c, sub); RegCloseKey(c); }
    }
}
static void import_registry(void) {
    for (size_t i = 0; i < sizeof k_roots / sizeof *k_roots; i++) {
        HKEY h;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, k_roots[i] + 5, 0, KEY_READ, &h) != ERROR_SUCCESS) continue;
        import_key(h, k_roots[i]);
        RegCloseKey(h);
    }
}

// ---- handles: the game's keys get handles of their own, any other key is the registry's (its path kept) ----
#define NHANDLES 64
#define VBASE 0x7EA50000u   // far from the registry's own handles (small multiples of 4) and the 0x8000000x roots
static struct { int used; char path[200]; } g_vh[NHANDLES];
static struct { HKEY h; char path[200]; } g_rh[NHANDLES];
static int vh_index(HKEY h) {
    uintptr_t v = (uintptr_t)h;
    return v >= VBASE && v < VBASE + NHANDLES * 4 && !(v & 3) && g_vh[(v - VBASE) / 4].used ? (int)((v - VBASE) / 4) : -1;
}
static const char *path_of(HKEY h) {
    if (h == HKEY_CURRENT_USER) return "HKCU";
    if (h == HKEY_LOCAL_MACHINE) return "HKLM";
    int i = vh_index(h);
    if (i >= 0) return g_vh[i].path;
    for (int j = 0; j < NHANDLES; j++) if (g_rh[j].h && g_rh[j].h == h) return g_rh[j].path;
    return NULL;
}
static int join(char *out, const char *base, const char *sub) {
    if (!sub || !*sub) return snprintf(out, 200, "%s", base) < 200;
    return snprintf(out, 200, "%s\\%s", base, sub) < 200;
}
static HKEY vh_new(const char *path) {
    for (int i = 0; i < NHANDLES; i++)
        if (!g_vh[i].used) { g_vh[i].used = 1; lstrcpyA(g_vh[i].path, path); return (HKEY)(uintptr_t)(VBASE + 4 * i); }
    return NULL;
}
static void rh_note(HKEY h, const char *path) {
    for (int i = 0; i < NHANDLES; i++) if (!g_rh[i].h) { g_rh[i].h = h; lstrcpyA(g_rh[i].path, path); return; }
}

static LONG WINAPI s_open(HKEY parent, LPCSTR sub, DWORD opt, REGSAM sam, PHKEY out) {
    const char *base = path_of(parent);
    char full[200];
    if (base && join(full, base, sub) && is_virtual(full)) {
        EnterCriticalSection(&g_cs);
        LONG e = !key_exists(full) ? ERROR_FILE_NOT_FOUND : (*out = vh_new(full)) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        LeaveCriticalSection(&g_cs);
        return e;
    }
    LONG e = RegOpenKeyExA(parent, sub, opt, sam, out);
    if (e == ERROR_SUCCESS && base) { EnterCriticalSection(&g_cs); rh_note(*out, full); LeaveCriticalSection(&g_cs); }
    return e;
}
static LONG WINAPI s_create(HKEY parent, LPCSTR sub, DWORD res, LPSTR cls, DWORD opt, REGSAM sam,
                            const LPSECURITY_ATTRIBUTES sa, PHKEY out, LPDWORD disp) {
    const char *base = path_of(parent);
    char full[200];
    if (base && join(full, base, sub) && is_virtual(full)) {
        EnterCriticalSection(&g_cs);
        int had = key_exists(full);
        LONG e = key_find(full, 1) && (*out = vh_new(full)) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        if (e == ERROR_SUCCESS && !had) g_dirty = 1;
        if (disp) *disp = had ? REG_OPENED_EXISTING_KEY : REG_CREATED_NEW_KEY;
        LeaveCriticalSection(&g_cs);
        return e;
    }
    LONG e = RegCreateKeyExA(parent, sub, res, cls, opt, sam, sa, out, disp);
    if (e == ERROR_SUCCESS && base) { EnterCriticalSection(&g_cs); rh_note(*out, full); LeaveCriticalSection(&g_cs); }
    return e;
}
static LONG WINAPI s_query(HKEY h, LPCSTR name, LPDWORD res, LPDWORD type, LPBYTE data, LPDWORD cb) {
    int i = vh_index(h);
    if (i < 0) return RegQueryValueExA(h, name, res, type, data, cb);
    EnterCriticalSection(&g_cs);
    struct skey *k = key_find(g_vh[i].path, 0);
    struct sval *v = k ? val_find(k, name ? name : "") : NULL;
    LONG e = ERROR_SUCCESS;
    if (!v) e = ERROR_FILE_NOT_FOUND;
    else {
        if (type) *type = v->type;
        if (data) {
            if (!cb) e = ERROR_INVALID_PARAMETER;
            else if (*cb < v->n) { *cb = v->n; e = ERROR_MORE_DATA; }
            else { memcpy(data, v->data, v->n); *cb = v->n; }
        } else if (cb) *cb = v->n;
    }
    LeaveCriticalSection(&g_cs);
    return e;
}
static LONG WINAPI s_set(HKEY h, LPCSTR name, DWORD res, DWORD type, const BYTE *data, DWORD n) {
    int i = vh_index(h);
    if (i < 0) return RegSetValueExA(h, name, res, type, data, n);
    EnterCriticalSection(&g_cs);
    struct skey *k = key_find(g_vh[i].path, 1);
    struct sval *old = k ? val_find(k, name ? name : "") : NULL;
    int same = old && old->type == type && old->n == n && (!n || !memcmp(old->data, data, n));
    LONG e = k && (same || val_set(k, name ? name : "", type, data, n)) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
    if (e == ERROR_SUCCESS && !same) g_dirty = 1;
    LeaveCriticalSection(&g_cs);
    return e;
}
static LONG WINAPI s_delete_value(HKEY h, LPCSTR name) {
    int i = vh_index(h);
    if (i < 0) return RegDeleteValueA(h, name);
    EnterCriticalSection(&g_cs);
    struct skey *k = key_find(g_vh[i].path, 0);
    struct sval *v = k ? val_find(k, name ? name : "") : NULL;
    if (v) {
        free(v->data);
        *v = k->v[--k->nv];
        g_dirty = 1;
    }
    LeaveCriticalSection(&g_cs);
    return v ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}
static LONG WINAPI s_delete_key(HKEY parent, LPCSTR sub) {
    const char *base = path_of(parent);
    char full[200];
    if (!base || !join(full, base, sub) || !is_virtual(full)) return RegDeleteKeyA(parent, sub);
    EnterCriticalSection(&g_cs);
    struct skey *k = key_find(full, 0);
    if (k) {
        for (int j = 0; j < k->nv; j++) free(k->v[j].data);
        k->live = 0;
        g_dirty = 1;
    }
    LeaveCriticalSection(&g_cs);
    return k ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}
static LONG WINAPI s_close(HKEY h) {
    int i = vh_index(h);
    if (i < 0) {
        EnterCriticalSection(&g_cs);
        for (int j = 0; j < NHANDLES; j++) if (g_rh[j].h == h) g_rh[j].h = NULL;
        LeaveCriticalSection(&g_cs);
        return RegCloseKey(h);
    }
    EnterCriticalSection(&g_cs);
    g_vh[i].used = 0;
    if (g_dirty) file_save();   // the game writes its settings key by key and closes each: the file follows
    LeaveCriticalSection(&g_cs);
    return ERROR_SUCCESS;
}

// ---- the engine's imports ----
static uintptr_t *iat_slot(HMODULE m, const char *fn) {   // the engine's import of ADVAPI32!fn, NULL: none
    uint8_t *b = (uint8_t *)m;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dd.VirtualAddress) return NULL;
    for (IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(b + dd.VirtualAddress); d->Name; d++) {
        if (lstrcmpiA((char *)(b + d->Name), "ADVAPI32.dll") || !d->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(b + d->OriginalFirstThunk), *iat = (IMAGE_THUNK_DATA *)(b + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++)
            if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal) &&
                !strcmp((char *)((IMAGE_IMPORT_BY_NAME *)(b + names->u1.AddressOfData))->Name, fn)) return (uintptr_t *)&iat->u1.Function;
    }
    return NULL;
}
static int slot_write(uintptr_t *slot, void *hook) {
    DWORD old;
    if (!VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &old)) return 0;
    *slot = (uintptr_t)hook;
    VirtualProtect(slot, sizeof *slot, old, &old);
    return 1;
}
static void start(HINSTANCE self) {
    // a copy of this DLL beside it (dinput.old.dll, a backup): the engine LoadLibrary()s every *.dll in the game folder
    // and FreeLibrary()s those that aren't its own again. A copy that hooked too crashed the game: the registry hooked
    // twice, a key opened in one copy queried in the other (0x52AB16, reading the settings), a window hook left in
    // freed code (2026-10-19). The one Windows loaded for the engine does everything, a copy nothing.
    HMODULE first = GetModuleHandleA("dinput.dll");
    if (first && first != (HMODULE)self && GetProcAddress(first, "EEAimodSettings")) return;
    char why[MAX_PATH + 64] = "";
    InitializeCriticalSection(&g_cs);
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");   // its imports bring us in: it is there, not yet run
    DWORD n = GetEnvironmentVariableA("EEAIMOD_SETTINGS", g_file, sizeof g_file);   // a test slot's own file
    if (!n || n >= sizeof g_file) {
        GetModuleFileNameA(self, g_file, sizeof g_file);
        char *s = strrchr(g_file, '\\');
        if (s) {   // <game>\aimod\eesettings.ini, the folder made if it isn't there yet
            lstrcpyA(s + 1, "aimod");
            CreateDirectoryA(g_file, NULL);
            lstrcatA(g_file, "\\eesettings.ini");
        }
    }
    if (!lle) lstrcpyA(why, "registry (the engine isn't loaded - not Empire Earth?)");
    else if (!file_load()) {   // the first start: the registry's values into the file
        import_registry();
        g_dirty = 1;
        if (!file_save()) wsprintfA(why, "registry (%s can't be written)", g_file);
    }
    if (!why[0]) {
        static const struct { const char *fn; void *hook; } k_hooks[] = {
            { "RegOpenKeyExA", (void *)s_open }, { "RegCreateKeyExA", (void *)s_create }, { "RegQueryValueExA", (void *)s_query },
            { "RegSetValueExA", (void *)s_set }, { "RegDeleteValueA", (void *)s_delete_value },
            { "RegDeleteKeyA", (void *)s_delete_key }, { "RegCloseKey", (void *)s_close } };
        // all or nothing: a key opened in the file but read from the registry would lose settings
        static void *const k_real[7] = { (void *)RegOpenKeyExA, (void *)RegCreateKeyExA, (void *)RegQueryValueExA,
            (void *)RegSetValueExA, (void *)RegDeleteValueA, (void *)RegDeleteKeyA, (void *)RegCloseKey };
        uintptr_t *slot[7];
        int found = 0, put = 0, taken = 0;
        for (int i = 0; i < 7; i++) found += (slot[i] = iat_slot(lle, k_hooks[i].fn)) != NULL;
        // taken already (another program's hook, or a copy of this DLL under another name loaded first): left alone
        for (int i = 0; i < 7 && found == 7; i++) taken += *slot[i] != (uintptr_t)k_real[i];
        if (found == 7 && !taken) for (int i = 0; i < 7; i++) put += slot_write(slot[i], k_hooks[i].hook);
        if (put == 7) { g_on = 1; lstrcpyA(why, g_file); }
        else if (taken) lstrcpyA(why, "registry (the engine's registry imports are hooked already)");
        else if (!put) wsprintfA(why, "registry (%d of 7 engine imports found)", found);
        else {   // can't happen short of a failing VirtualProtect: the registry's own functions back in
            for (int i = 0; i < 7; i++) slot_write(slot[i], k_real[i]);
            lstrcpyA(why, "registry (the engine's imports couldn't be changed)");
        }
    }
    SetEnvironmentVariableA("EEAIMOD_SETTINGS_FILE", why);
    char hl[4];
    if (GetEnvironmentVariableA("EEAIMOD_HEADLESS", hl, sizeof hl) == 1 && hl[0] == '1') headless_windows();   // a test
}
const char *WINAPI EEAimodSettings(void) { return g_on ? g_file : NULL; }

BOOL WINAPI DllMain(HINSTANCE self, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) { DisableThreadLibraryCalls(self); start(self); }
    else if (reason == DLL_PROCESS_DETACH) {
        if (g_on) file_save();
        if (g_hl_cbt) UnhookWindowsHookEx(g_hl_cbt);
    }
    return TRUE;
}
