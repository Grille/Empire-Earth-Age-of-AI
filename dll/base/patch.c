// PATCHING the game's code: a call, a 32-bit operand, an import slot - each only while it holds what it is expected to

// the exe's code section and its whole image, from its PE header (the same test for every build): "the game's code" -
// the section that holds BaseOfCode: the 2017 update flags every section as data (AoC's .text says code), and NeoEE's
// copy protection layout shuffles the names (its game code is a section named .data)
internal uintptr_t g_code_lo, g_code_hi, g_image_lo, g_image_hi;
internal void image_ranges(void) {
    uint8_t *b = (uint8_t *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(b + *(uint32_t *)(b + 0x3C));
    g_image_lo = (uintptr_t)b;
    g_image_hi = g_image_lo + nt->OptionalHeader.SizeOfImage;
    uint32_t code = nt->OptionalHeader.BaseOfCode;
    IMAGE_SECTION_HEADER *s = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++)
        if (code >= s->VirtualAddress && code - s->VirtualAddress < s->Misc.VirtualSize) {
            g_code_lo = (uintptr_t)b + s->VirtualAddress;
            g_code_hi = g_code_lo + s->Misc.VirtualSize;
            break;
        }
}
// One 32-bit operand in the code, only while it still holds `expected` (another exe version is left alone).
internal int patch_code32(uintptr_t at, uint32_t expected, uint32_t val) {
    uint32_t *p = (uint32_t *)at;
    DWORD old;
    if (*p != expected || !VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *p = val;
    VirtualProtect(p, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 4);
    return 1;
}
// An import slot of the exe, only while it holds the import it is for (that module's export): the original in *orig.
// 0 when it holds something else (another hook, another exe) - left alone, *orig untouched.
internal int iat_hook(uintptr_t slot, const char *dll, const char *name, void *fn, void **orig) {
    HMODULE m = GetModuleHandleA(dll);
    void *want = m ? (void *)GetProcAddress(m, name) : NULL;
    DWORD old;
    if (!want || *(void **)slot != want || !VirtualProtect((void *)slot, 4, PAGE_READWRITE, &old)) return 0;
    *orig = want;
    *(void **)slot = fn;
    VirtualProtect((void *)slot, 4, old, &old);
    return 1;
}

// `n` bytes at p readable, from the page table (IsBadReadPtr probes by raising access violations: each one went
// through the crash log's first-chance handler)
internal int mem_readable(const void *p, size_t n) {
    uintptr_t a = (uintptr_t)p, end = a + n;
    if (!p || end < a) return 0;
    while (a < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void *)a, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
            || !(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
                                | PAGE_EXECUTE_WRITECOPY))) return 0;
        a = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return 1;
}

// Redirect an E8 rel32 call, verifying it currently targets `expected`.
internal BOOL patch_call(uintptr_t site, uintptr_t expected, void *hook) {
    uint8_t *p = (uint8_t *)site;
    if (!site) return FALSE;   // a build without that call (the spec's none: 0)
    if (p[0] != 0xE8 || site + 5 + *(int32_t *)(p + 1) != expected) return FALSE;
    DWORD old;
    if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook - (site + 5));
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    return TRUE;
}

// a vtable slot, only while it holds `expected`
internal int patch_vtable(uintptr_t vt, int slot, uintptr_t expected, void *hook) {
    void **p = (void **)(vt + slot * 4);
    if (*p != (void *)expected) return 0;
    DWORD old;
    if (!VirtualProtect(p, 4, PAGE_READWRITE, &old)) return 0;
    *p = hook;
    VirtualProtect(p, 4, old, &old);
    return 1;
}

// one byte of the code, only while it holds `expected`
internal int patch_byte(uintptr_t at, uint8_t expected, uint8_t val) {
    uint8_t *p = (uint8_t *)at;
    DWORD old;
    if (*p != expected || !VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *p = val;
    VirtualProtect(p, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
    return 1;
}

// an operand apply_editor_layout checked against the original the first time (or one it patched itself)
internal void write_code32(uint32_t *p, uint32_t v) {
    DWORD old;
    if (!VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old)) return;
    *p = v;
    VirtualProtect(p, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 4);
}

// A MODULE'S LOAD: fn(base) for the module of that file name once it is in - at once if it is already, else when the
// loader maps it (ntdll's LdrRegisterDllNotification: before its DllMain, before anyone calls it). The engine loads the
// game folder's DLLs one by one in the order of their names, so those after ageofai.dll (Miles Sound System Mixer.dll)
// aren't there yet when the mod installs. At most 8 watched; 0: it can't watch (no such loader function).
typedef void (*module_load_fn)(uintptr_t base);
internal struct { const char *name; module_load_fn fn; } g_mod_watch[8];
internal int g_mod_watches;
typedef struct { USHORT len, cap; PWSTR buf; } ldr_ustr;
typedef struct { ULONG flags; const ldr_ustr *full, *base_name; void *base; ULONG size; } ldr_dll_data;
internal void CALLBACK module_loaded(ULONG reason, const ldr_dll_data *d, void *ctx) {
    (void)ctx;
    if (reason != 1) return;   // LDR_DLL_NOTIFICATION_REASON_LOADED (2: unloaded)
    int len = d->base_name->len / 2;
    for (int i = 0; i < g_mod_watches; i++) {
        const char *n = g_mod_watch[i].name;
        int k = 0;
        for (; k < len && n[k]; k++) {
            WCHAR c = d->base_name->buf[k], w = (WCHAR)(unsigned char)n[k];
            if (c >= 'A' && c <= 'Z') c += 32;
            if (w >= 'A' && w <= 'Z') w += 32;
            if (c != w) break;
        }
        if (k == len && !n[k]) g_mod_watch[i].fn((uintptr_t)d->base);
    }
}
internal int on_module_load(const char *name, module_load_fn fn) {
    static void *cookie;
    HMODULE m = GetModuleHandleA(name);
    if (m) { fn((uintptr_t)m); return 1; }
    if (g_mod_watches == 8) return 0;
    if (!cookie) {
        typedef LONG (NTAPI *reg_fn)(ULONG, void *, void *, void **);
        reg_fn reg = (reg_fn)(void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrRegisterDllNotification");
        if (!reg || reg(0, (void *)module_loaded, NULL, &cookie) < 0) return 0;
    }
    g_mod_watch[g_mod_watches].name = name;
    g_mod_watch[g_mod_watches++].fn = fn;
    return 1;
}
