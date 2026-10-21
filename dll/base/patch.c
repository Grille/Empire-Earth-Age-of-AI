// PATCHING the game's code: a call, a 32-bit operand, an import slot - each only while it holds what it is expected to
// One 32-bit operand in the code, only while it still holds `expected` (another exe version is left alone).
static int patch_code32(uintptr_t at, uint32_t expected, uint32_t val) {
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
static int iat_hook(uintptr_t slot, const char *dll, const char *name, void *fn, void **orig) {
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
static int mem_readable(const void *p, size_t n) {
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
static BOOL patch_call(uintptr_t site, uintptr_t expected, void *hook) {
    uint8_t *p = (uint8_t *)site;
    if (p[0] != 0xE8 || site + 5 + *(int32_t *)(p + 1) != expected) return FALSE;
    DWORD old;
    if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook - (site + 5));
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    return TRUE;
}

// a vtable slot, only while it holds `expected`
static int patch_vtable(uintptr_t vt, int slot, uintptr_t expected, void *hook) {
    void **p = (void **)(vt + slot * 4);
    if (*p != (void *)expected) return 0;
    DWORD old;
    if (!VirtualProtect(p, 4, PAGE_READWRITE, &old)) return 0;
    *p = hook;
    VirtualProtect(p, 4, old, &old);
    return 1;
}

// one byte of the code, only while it holds `expected`
static int patch_byte(uintptr_t at, uint8_t expected, uint8_t val) {
    uint8_t *p = (uint8_t *)at;
    DWORD old;
    if (*p != expected || !VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *p = val;
    VirtualProtect(p, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
    return 1;
}

// an operand apply_editor_layout checked against the original the first time (or one it patched itself)
static void write_code32(uint32_t *p, uint32_t v) {
    DWORD old;
    if (!VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old)) return;
    *p = v;
    VirtualProtect(p, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 4);
}
