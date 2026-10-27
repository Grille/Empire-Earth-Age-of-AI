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
#define PATCH_LIST(a) (a), sizeof (a) / sizeof *(a)   // a list of places of the spec (data_hooks_train_sites): it and its count

// every call of fn in the exe's code (E8 rel32 to it in the code section, base/patch.c's): for a function called from
// many places
internal inline int patch_all_calls(uintptr_t fn, void *hook) {
    int sites = 0;
    for (uint8_t *p = (uint8_t *)g_code_lo, *e = (uint8_t *)g_code_hi - 5; p < e; p++)
        if (*p == 0xE8 && (uintptr_t)p + 5 + *(int32_t *)(p + 1) == fn)
            sites += patch_call((uintptr_t)p, fn, hook);
    return sites;
}

// every vtable whose slot `slot` holds fn and whose slot `key_slot` holds key (a 4-aligned word outside the code
// section, a code address on both sides of it - the other entries of its vtable): for a virtual function many classes
// share, in the classes that call it from key; their count
internal inline int patch_vtables_with(uintptr_t key, int key_slot, int slot, uintptr_t fn, void *hook) {
    uint8_t *base = (uint8_t *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    uintptr_t lo = g_code_lo, hi = g_code_hi;
    if (!lo) return 0;
    int n = 0, d = slot - key_slot;
    for (int s = 0; s < nt->FileHeader.NumberOfSections; s++) {
        if ((uintptr_t)base + sec[s].VirtualAddress == lo) continue;
        uint32_t *p = (uint32_t *)(base + sec[s].VirtualAddress), *e = p + sec[s].Misc.VirtualSize / 4;
        for (uint32_t *q = p + 1; q + 1 < e; q++) {
            if (*q != key || q[-1] < lo || q[-1] >= hi || q[1] < lo || q[1] >= hi || q + d < p || q + d >= e || q[d] != fn) continue;
            n += patch_vtable((uintptr_t)(q - key_slot), slot, fn, hook);
        }
    }
    return n;
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
internal inline int train_site(uintptr_t at) { return site_index(PATCH_LIST(calls_train_units), at); }    // the game's 5 unit orders
internal inline int mine_site(uintptr_t at) { return site_index(PATCH_LIST(calls_find_mine), at); }      // 5: the citizen assignment's last
internal inline int unload_site(uintptr_t at) { return site_index(PATCH_LIST(calls_unload_ctor), at); }  // 7: the unload goal's copy
internal inline uintptr_t unload_site_at(int i) { return calls_unload_ctor[i]; }
internal inline int unload_from_rm(uintptr_t at) { return at == data_patching_call_unload_rm; }   // the resource manager's ferry
// a land subgroup's goal handed over with its unit (the return address of its give_goal call: landing.c)
internal inline int from_land_handoff(uintptr_t ret) { return ret == data_patching_ret_asg_land_handoff_give; }
