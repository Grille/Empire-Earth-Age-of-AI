// EMPIRE EARTH'S SETTINGS IN A FILE: the game keeps its settings in the registry, HKCU\Software\SSSI\Empire Earth (60
// values, subkey "Game Options"; Art of Conquest the same under HKCU\Software\Mad Doc Software\EE-AOC), and reads them
// all before the engine's DLL scan loads ageofai.dll (docs/engine-notes.md, Exe builds - Settings). So this runs only
// when Windows loads ageofai.dll with the engine: an import of Low-Level Engine.dll that ageofai_config.exe adds (the
// player's choice, undone there; eeaimod.c DllMain, early_start). Then the engine is there with its imports bound and
// hasn't run yet: its registry calls (its 7 ADVAPI32 imports) for the game's own keys are pointed at eesettings.ini in
// the mod's folder (<game>\ageofai\, with ageofai.ini) - the settings travel with the folder, every copy of the game
// has its own (test slots: EEAIMOD_SETTINGS=<file>). Without the import - or when the file can't be read or written -
// the game uses the registry as before. The first start without the file copies the registry's values into it.
// EEAIMOD_SETTINGS_FILE tells the log (render.c) which one is in use. (Was dinput.dll's, a stand-in for the system's
// DirectInput that Windows loaded first - another program's file name, now gone.)

// ---- the settings: keys with their values, in memory; the file is written when the game has changed them ----
internal const char *const k_gs_roots[] = { "HKCU\\Software\\SSSI\\Empire Earth", "HKCU\\Software\\Neo\\Empire Earth",
                                            "HKCU\\Software\\Mad Doc Software\\EE-AOC" };
#define GS_KEYS 32
#define GS_VALS 128
struct gs_val { char name[80]; DWORD type, n; BYTE *data; };
struct gs_key { char path[200]; int nv, live; struct gs_val v[GS_VALS]; };
internal struct gs_key g_gs_keys[GS_KEYS];
internal char g_gs_file[MAX_PATH];
internal int g_gs_dirty, g_gs_on;
internal CRITICAL_SECTION g_gs_cs;

internal int gs_is_game(const char *path) {   // one of the game's own keys (or below)
    for (size_t i = 0; i < sizeof k_gs_roots / sizeof *k_gs_roots; i++) {
        size_t n = strlen(k_gs_roots[i]);
        if (!_strnicmp(path, k_gs_roots[i], n) && (!path[n] || path[n] == '\\')) return 1;
    }
    return 0;
}
internal struct gs_key *gs_key_find(const char *path, int create) {
    struct gs_key *free_k = NULL;
    for (int i = 0; i < GS_KEYS; i++) {
        if (g_gs_keys[i].live && !lstrcmpiA(g_gs_keys[i].path, path)) return &g_gs_keys[i];
        if (!g_gs_keys[i].live && !free_k) free_k = &g_gs_keys[i];
    }
    if (!create || !free_k || strlen(path) >= sizeof free_k->path) return NULL;
    memset(free_k, 0, sizeof *free_k);
    lstrcpyA(free_k->path, path);
    free_k->live = 1;
    return free_k;
}
internal int gs_key_exists(const char *path) {   // a key, or one below it (an empty key in between has no record)
    size_t n = strlen(path);
    for (int i = 0; i < GS_KEYS; i++)
        if (g_gs_keys[i].live && !_strnicmp(g_gs_keys[i].path, path, n) && (!g_gs_keys[i].path[n] || g_gs_keys[i].path[n] == '\\')) return 1;
    for (size_t i = 0; i < sizeof k_gs_roots / sizeof *k_gs_roots; i++) if (!lstrcmpiA(path, k_gs_roots[i])) return 1;   // a root: always
    return 0;
}
internal struct gs_val *gs_val_find(struct gs_key *k, const char *name) {
    for (int i = 0; i < k->nv; i++) if (!lstrcmpiA(k->v[i].name, name)) return &k->v[i];
    return NULL;
}
internal int gs_val_set(struct gs_key *k, const char *name, DWORD type, const BYTE *data, DWORD n) {
    struct gs_val *v = gs_val_find(k, name);
    if (!v) {
        if (k->nv == GS_VALS || strlen(name) >= sizeof v->name) return 0;
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
internal void gs_str_write(FILE *f, const BYTE *s, DWORD n) {
    fputc('"', f);
    for (DWORD i = 0; i < n && s[i]; i++) {
        if (s[i] == '"' || s[i] == '\\') fputc('\\', f);
        fputc(s[i], f);
    }
    fputc('"', f);
}
internal int gs_save(void) {   // 1: written (or nothing to write)
    if (!g_gs_dirty) return 1;
    char tmp[MAX_PATH + 8];
    wsprintfA(tmp, "%s.new", g_gs_file);
    FILE *f = fopen(tmp, "wb");
    if (!f) return 0;
    fputs("; Empire Earth's settings - kept here by Empire Earth: Age of AI instead of the registry.\r\n"
          "; The game writes this file when it closes. ageofai_config.exe (Game) switches it off: the registry again.\r\n"
          "; A plain number is a number, \"text\" in quotes a text.\r\n", f);
    for (int i = 0; i < GS_KEYS; i++) {
        struct gs_key *k = &g_gs_keys[i];
        if (!k->live) continue;
        fprintf(f, "\r\n[%s]\r\n", k->path);
        for (int j = 0; j < k->nv; j++) {
            struct gs_val *v = &k->v[j];
            fprintf(f, "%s=", v->name);
            if (v->type == REG_DWORD && v->n == 4) fprintf(f, "%lu", (unsigned long)*(DWORD *)v->data);
            else if ((v->type == REG_SZ || v->type == REG_EXPAND_SZ) && v->n) {   // (0 bytes, no 0 even: hex below)
                if (v->type == REG_EXPAND_SZ) fputs("expand:", f);
                gs_str_write(f, v->data, v->n);
            } else {
                fprintf(f, "hex(%lu):", (unsigned long)v->type);
                for (DWORD b = 0; b < v->n; b++) fprintf(f, "%s%02x", b ? "," : "", v->data[b]);
            }
            fputs("\r\n", f);
        }
    }
    int ok = !ferror(f);
    if (fclose(f)) ok = 0;
    if (ok) ok = MoveFileExA(tmp, g_gs_file, MOVEFILE_REPLACE_EXISTING);   // never half a file
    if (ok) g_gs_dirty = 0;
    else DeleteFileA(tmp);
    return ok;
}
internal int gs_load(void) {   // 1: read; 0: not there or not readable
    FILE *f = fopen(g_gs_file, "rb");
    if (!f) return 0;
    char line[2048];
    struct gs_key *k = NULL;
    while (fgets(line, sizeof line, f)) {
        char *e = line + strlen(line);
        while (e > line && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ')) *--e = 0;
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (!*s || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            char *c = strrchr(s, ']');
            if (c) { *c = 0; k = gs_is_game(s + 1) ? gs_key_find(s + 1, 1) : NULL; }
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
            gs_val_set(k, s, type, (BYTE *)out, n);
        } else if (!_strnicmp(v, "hex(", 4)) {
            char *p;
            DWORD type = strtoul(v + 4, &p, 10), n = 0;
            BYTE out[1024];
            if (*p == ')') p++;
            if (*p == ':') p++;
            while (*p && n < sizeof out) { out[n++] = (BYTE)strtoul(p, &p, 16); if (*p == ',') p++; else break; }
            gs_val_set(k, s, type, out, n);
        } else {
            DWORD d = strtoul(v, NULL, 10);
            gs_val_set(k, s, REG_DWORD, (BYTE *)&d, 4);
        }
    }
    fclose(f);
    return 1;
}
internal void gs_import_key(HKEY h, const char *path) {   // the registry's values below path, at the first start
    struct gs_key *k = gs_key_find(path, 1);
    if (!k) return;
    char name[256];
    BYTE data[2048];
    for (DWORD i = 0;; i++) {
        DWORD nn = sizeof name, dn = sizeof data, type;
        LONG e = RegEnumValueA(h, i, name, &nn, NULL, &type, data, &dn);
        if (e == ERROR_NO_MORE_ITEMS) break;
        if (e == ERROR_SUCCESS) gs_val_set(k, name, type, data, dn);
    }
    for (DWORD i = 0;; i++) {
        DWORD nn = sizeof name;
        if (RegEnumKeyExA(h, i, name, &nn, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
        HKEY c;
        char sub[200];
        if (snprintf(sub, sizeof sub, "%s\\%s", path, name) >= (int)sizeof sub) continue;
        if (RegOpenKeyExA(h, name, 0, KEY_READ, &c) == ERROR_SUCCESS) { gs_import_key(c, sub); RegCloseKey(c); }
    }
}
internal void gs_import_registry(void) {
    for (size_t i = 0; i < sizeof k_gs_roots / sizeof *k_gs_roots; i++) {
        HKEY h;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, k_gs_roots[i] + 5, 0, KEY_READ, &h) != ERROR_SUCCESS) continue;
        gs_import_key(h, k_gs_roots[i]);
        RegCloseKey(h);
    }
}

// ---- handles: the game's keys get handles of their own, any other key is the registry's (its path kept) ----
#define GS_HANDLES 64
#define GS_VBASE 0x7EA50000u   // far from the registry's own handles (small multiples of 4) and the 0x8000000x roots
internal struct { int used; char path[200]; } g_gs_vh[GS_HANDLES];
internal struct { HKEY h; char path[200]; } g_gs_rh[GS_HANDLES];
internal int gs_vh_index(HKEY h) {
    uintptr_t v = (uintptr_t)h;
    return v >= GS_VBASE && v < GS_VBASE + GS_HANDLES * 4 && !(v & 3) && g_gs_vh[(v - GS_VBASE) / 4].used ? (int)((v - GS_VBASE) / 4) : -1;
}
internal const char *gs_path_of(HKEY h) {
    if (h == HKEY_CURRENT_USER) return "HKCU";
    if (h == HKEY_LOCAL_MACHINE) return "HKLM";
    int i = gs_vh_index(h);
    if (i >= 0) return g_gs_vh[i].path;
    for (int j = 0; j < GS_HANDLES; j++) if (g_gs_rh[j].h && g_gs_rh[j].h == h) return g_gs_rh[j].path;
    return NULL;
}
internal int gs_join(char *out, const char *base, const char *sub) {
    if (!sub || !*sub) return snprintf(out, 200, "%s", base) < 200;
    return snprintf(out, 200, "%s\\%s", base, sub) < 200;
}
internal HKEY gs_vh_new(const char *path) {
    for (int i = 0; i < GS_HANDLES; i++)
        if (!g_gs_vh[i].used) { g_gs_vh[i].used = 1; lstrcpyA(g_gs_vh[i].path, path); return (HKEY)(uintptr_t)(GS_VBASE + 4 * i); }
    return NULL;
}
internal void gs_rh_note(HKEY h, const char *path) {
    for (int i = 0; i < GS_HANDLES; i++) if (!g_gs_rh[i].h) { g_gs_rh[i].h = h; lstrcpyA(g_gs_rh[i].path, path); return; }
}

internal LONG WINAPI gs_open(HKEY parent, LPCSTR sub, DWORD opt, REGSAM sam, PHKEY out) {
    const char *base = gs_path_of(parent);
    char full[200];
    if (base && gs_join(full, base, sub) && gs_is_game(full)) {
        EnterCriticalSection(&g_gs_cs);
        LONG e = !gs_key_exists(full) ? ERROR_FILE_NOT_FOUND : (*out = gs_vh_new(full)) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        LeaveCriticalSection(&g_gs_cs);
        return e;
    }
    LONG e = RegOpenKeyExA(parent, sub, opt, sam, out);
    if (e == ERROR_SUCCESS && base) { EnterCriticalSection(&g_gs_cs); gs_rh_note(*out, full); LeaveCriticalSection(&g_gs_cs); }
    return e;
}
internal LONG WINAPI gs_create(HKEY parent, LPCSTR sub, DWORD res, LPSTR cls, DWORD opt, REGSAM sam,
                               const LPSECURITY_ATTRIBUTES sa, PHKEY out, LPDWORD disp) {
    const char *base = gs_path_of(parent);
    char full[200];
    if (base && gs_join(full, base, sub) && gs_is_game(full)) {
        EnterCriticalSection(&g_gs_cs);
        int had = gs_key_exists(full);
        LONG e = gs_key_find(full, 1) && (*out = gs_vh_new(full)) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
        if (e == ERROR_SUCCESS && !had) g_gs_dirty = 1;
        if (disp) *disp = had ? REG_OPENED_EXISTING_KEY : REG_CREATED_NEW_KEY;
        LeaveCriticalSection(&g_gs_cs);
        return e;
    }
    LONG e = RegCreateKeyExA(parent, sub, res, cls, opt, sam, sa, out, disp);
    if (e == ERROR_SUCCESS && base) { EnterCriticalSection(&g_gs_cs); gs_rh_note(*out, full); LeaveCriticalSection(&g_gs_cs); }
    return e;
}
internal LONG WINAPI gs_query(HKEY h, LPCSTR name, LPDWORD res, LPDWORD type, LPBYTE data, LPDWORD cb) {
    int i = gs_vh_index(h);
    if (i < 0) return RegQueryValueExA(h, name, res, type, data, cb);
    EnterCriticalSection(&g_gs_cs);
    struct gs_key *k = gs_key_find(g_gs_vh[i].path, 0);
    struct gs_val *v = k ? gs_val_find(k, name ? name : "") : NULL;
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
    LeaveCriticalSection(&g_gs_cs);
    return e;
}
internal LONG WINAPI gs_set(HKEY h, LPCSTR name, DWORD res, DWORD type, const BYTE *data, DWORD n) {
    int i = gs_vh_index(h);
    if (i < 0) return RegSetValueExA(h, name, res, type, data, n);
    EnterCriticalSection(&g_gs_cs);
    struct gs_key *k = gs_key_find(g_gs_vh[i].path, 1);
    struct gs_val *old = k ? gs_val_find(k, name ? name : "") : NULL;
    int same = old && old->type == type && old->n == n && (!n || !memcmp(old->data, data, n));
    LONG e = k && (same || gs_val_set(k, name ? name : "", type, data, n)) ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
    if (e == ERROR_SUCCESS && !same) g_gs_dirty = 1;
    LeaveCriticalSection(&g_gs_cs);
    return e;
}
internal LONG WINAPI gs_delete_value(HKEY h, LPCSTR name) {
    int i = gs_vh_index(h);
    if (i < 0) return RegDeleteValueA(h, name);
    EnterCriticalSection(&g_gs_cs);
    struct gs_key *k = gs_key_find(g_gs_vh[i].path, 0);
    struct gs_val *v = k ? gs_val_find(k, name ? name : "") : NULL;
    if (v) {
        free(v->data);
        *v = k->v[--k->nv];
        g_gs_dirty = 1;
    }
    LeaveCriticalSection(&g_gs_cs);
    return v ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}
internal LONG WINAPI gs_delete_key(HKEY parent, LPCSTR sub) {
    const char *base = gs_path_of(parent);
    char full[200];
    if (!base || !gs_join(full, base, sub) || !gs_is_game(full)) return RegDeleteKeyA(parent, sub);
    EnterCriticalSection(&g_gs_cs);
    struct gs_key *k = gs_key_find(full, 0);
    if (k) {
        for (int j = 0; j < k->nv; j++) free(k->v[j].data);
        k->live = 0;
        g_gs_dirty = 1;
    }
    LeaveCriticalSection(&g_gs_cs);
    return k ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}
internal LONG WINAPI gs_close(HKEY h) {
    int i = gs_vh_index(h);
    if (i < 0) {
        EnterCriticalSection(&g_gs_cs);
        for (int j = 0; j < GS_HANDLES; j++) if (g_gs_rh[j].h == h) g_gs_rh[j].h = NULL;
        LeaveCriticalSection(&g_gs_cs);
        return RegCloseKey(h);
    }
    EnterCriticalSection(&g_gs_cs);
    g_gs_vh[i].used = 0;
    if (g_gs_dirty) gs_save();   // the game writes its settings key by key and closes each: the file follows
    LeaveCriticalSection(&g_gs_cs);
    return ERROR_SUCCESS;
}

// ---- the engine's imports ----
internal uintptr_t *gs_iat_slot(HMODULE m, const char *dll, const char *fn) {   // m's import of dll!fn, NULL: none
    uint8_t *b = (uint8_t *)m;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dd.VirtualAddress) return NULL;
    for (IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(b + dd.VirtualAddress); d->Name; d++) {
        if (lstrcmpiA((char *)(b + d->Name), dll) || !d->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(b + d->OriginalFirstThunk), *iat = (IMAGE_THUNK_DATA *)(b + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++)
            if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal) &&
                !strcmp((char *)((IMAGE_IMPORT_BY_NAME *)(b + names->u1.AddressOfData))->Name, fn)) return (uintptr_t *)&iat->u1.Function;
    }
    return NULL;
}
internal int gs_slot_write(uintptr_t *slot, uintptr_t v) {
    DWORD old;
    if (!VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &old)) return 0;
    *slot = v;
    VirtualProtect(slot, sizeof *slot, old, &old);
    return 1;
}
// the redirect, at the start (the engine there, not yet run): the file in use - or "registry (why)" - into
// EEAIMOD_SETTINGS_FILE
internal void game_settings_start(HINSTANCE self) {
    char why[MAX_PATH + 64] = "";
    InitializeCriticalSection(&g_gs_cs);
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");   // its import brought us in: it is there, not yet run
    DWORD n = GetEnvironmentVariableA("EEAIMOD_SETTINGS", g_gs_file, sizeof g_gs_file);   // a test slot's own file
    if (!n || n >= sizeof g_gs_file) {
        GetModuleFileNameA(self, g_gs_file, sizeof g_gs_file);
        char *s = strrchr(g_gs_file, '\\');
        if (s) {   // <game>\ageofai\eesettings.ini, the folder made if it isn't there yet
            lstrcpyA(s + 1, "ageofai");
            CreateDirectoryA(g_gs_file, NULL);
            lstrcatA(g_gs_file, "\\eesettings.ini");
        }
    }
    if (!lle) lstrcpyA(why, "registry (the engine isn't loaded - not Empire Earth?)");
    else if (!gs_load()) {   // the first start: the registry's values into the file
        gs_import_registry();
        g_gs_dirty = 1;
        if (!gs_save()) wsprintfA(why, "registry (%s can't be written)", g_gs_file);
    }
    if (!why[0]) {
        static const struct { const char *fn; void *hook; } k_hooks[] = {
            { "RegOpenKeyExA", (void *)gs_open }, { "RegCreateKeyExA", (void *)gs_create }, { "RegQueryValueExA", (void *)gs_query },
            { "RegSetValueExA", (void *)gs_set }, { "RegDeleteValueA", (void *)gs_delete_value },
            { "RegDeleteKeyA", (void *)gs_delete_key }, { "RegCloseKey", (void *)gs_close } };
        // all or nothing: a key opened in the file but read from the registry would lose settings
        static void *const k_real[7] = { (void *)RegOpenKeyExA, (void *)RegCreateKeyExA, (void *)RegQueryValueExA,
            (void *)RegSetValueExA, (void *)RegDeleteValueA, (void *)RegDeleteKeyA, (void *)RegCloseKey };
        uintptr_t *slot[7];
        int found = 0, put = 0, taken = 0;
        for (int i = 0; i < 7; i++) found += (slot[i] = gs_iat_slot(lle, "ADVAPI32.dll", k_hooks[i].fn)) != NULL;
        // taken already (another program's hook - an older dinput.dll of the mod's among them): left alone
        for (int i = 0; i < 7 && found == 7; i++) taken += *slot[i] != (uintptr_t)k_real[i];
        if (found == 7 && !taken) for (int i = 0; i < 7; i++) put += gs_slot_write(slot[i], (uintptr_t)k_hooks[i].hook);
        if (put == 7) { g_gs_on = 1; lstrcpyA(why, g_gs_file); }
        else if (taken) lstrcpyA(why, "registry (the engine's registry imports are hooked already)");
        else if (!put) wsprintfA(why, "registry (%d of 7 engine imports found)", found);
        else {   // can't happen short of a failing VirtualProtect: the registry's own functions back in
            for (int i = 0; i < 7; i++) gs_slot_write(slot[i], (uintptr_t)k_real[i]);
            lstrcpyA(why, "registry (the engine's imports couldn't be changed)");
        }
    }
    SetEnvironmentVariableA("EEAIMOD_SETTINGS_FILE", why);
}
internal void game_settings_end(void) { if (g_gs_on) gs_save(); }   // the process ends: what is left unwritten
