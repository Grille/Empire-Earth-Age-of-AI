// HOW THE MODULE PATCHES: the installers' helpers (a list of call sites, of vtables, every call of a function, an
// import call) and which call a hook came from.

typedef void (*thunk_fn)(void);

internal inline int patch_calls(const uint32_t *sites, size_t n, uintptr_t fn, void *hook, int *of) {
    int ok = 0;
    for (size_t i = 0; i < n; i++) ok += patch_call(sites[i], fn, hook);
    if (of) *of += (int)n;
    return ok;
}
internal inline int patch_vtables(const uint32_t *vts, size_t n, int slot, uintptr_t fn, void *hook, int *of) {
    int ok = 0;
    for (size_t i = 0; i < n; i++) ok += patch_vtable(vts[i], slot, fn, hook);
    if (of) *of += (int)n;
    return ok;
}
#define PATCH_LIST(L) GL(L), sizeof GL(L) / sizeof *GL(L)

// every call of fn in the exe's code (E8 rel32 to it in .text): for a function called from many places
internal inline int patch_all_calls(uintptr_t fn, void *hook) {
    uint8_t *base = (uint8_t *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    int sites = 0;
    for (int s = 0; s < nt->FileHeader.NumberOfSections; s++, sec++) {
        if (memcmp(sec->Name, ".text", 6)) continue;   // flagged as data like every section of this exe
        uint8_t *p = base + sec->VirtualAddress, *e = p + sec->Misc.VirtualSize - 5;
        for (; p < e; p++)
            if (*p == 0xE8 && (uintptr_t)p + 5 + *(int32_t *)(p + 1) == fn)
                sites += patch_call((uintptr_t)p, fn, hook);
    }
    return sites;
}

// a 6-byte `call [import]` at `at` (the import slot iat) -> `call hook; nop`, only while it still calls through iat
internal inline int patch_import_call(uintptr_t at, uintptr_t iat, void *hook) {
    uint8_t *p = (uint8_t *)at;
    if (p[0] != 0xFF || p[1] != 0x15 || *(uint32_t *)(p + 2) != iat) return 0;
    DWORD old;
    if (!VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    p[0] = 0xE8;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook - (at + 5));
    p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return 1;
}

// WHICH CALL A HOOK CAME FROM: at = its return address - 5 (the call instruction); the index in the list, -1: none
internal inline int site_index(const uint32_t *sites, size_t n, uintptr_t at) {
    for (size_t i = 0; i < n; i++)
        if (sites[i] == at) return (int)i;
    return -1;
}
internal inline int train_site(uintptr_t at) { return site_index(PATCH_LIST(TRAIN_SITES), at); }    // the game's 5 unit orders
internal inline int mine_site(uintptr_t at) { return site_index(PATCH_LIST(MINE_SITES), at); }      // 5: the citizen assignment's last
internal inline int unload_site(uintptr_t at) { return site_index(PATCH_LIST(UNLOAD_SITES), at); }  // 7: the unload goal's copy
internal inline uintptr_t unload_site_at(int i) { return GL(UNLOAD_SITES)[i]; }
internal inline int unload_from_rm(uintptr_t at) { return at == GA(CALL_UNLOAD_RM); }   // the resource manager's ferry
// a land subgroup's goal handed over with its unit (the return address of its give_goal call: landing.c)
internal inline int from_land_handoff(uintptr_t ret) { return ret == GA(RET_ASG_LAND_HANDOFF_GIVE); }
