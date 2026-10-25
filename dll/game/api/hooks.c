// THE GAME'S CODE PATCHED: hooks of functions with no object of their own, and the game's code changed in place
// (operands, jumps, single bytes) - each checked against what it expects there first.

// THE GAME'S RUN (the wiring: eeaimod.c, persist.c; test/autotest.c's command line)
typedef char (__thiscall *game_start_hook)(void *state, ee_setup *setup, void *p2);
internal inline int patch_game_start(game_start_hook h) { return patch_call(GA(CALL_GAME_START), GA(FN_GAME_START), (void *)h); }
typedef char (__cdecl *parse_cmdline_hook)(const char *cmd);
internal inline int patch_parse_cmdline(parse_cmdline_hook h) { return patch_call(GA(CALL_PARSE_CMDLINE), GA(FN_PARSE_CMDLINE), (void *)h); }
typedef int (__cdecl *load_saved_hook)(void *name);
internal inline int patch_load_saved(load_saved_hook h) { return patch_call(GA(CALL_LOAD_SAVED), GA(FN_LOAD_SAVED), (void *)h); }
// the game saved (every call of it: 4)
typedef int (__cdecl *save_game_hook)(void *file, int a, int b, struct ustring folder);
internal inline int patch_save_game(save_game_hook h) { return patch_all_calls(GA(FN_SAVE_GAME), (void *)h); }

// THE ENGINE'S FIXES (engine/): hooks, and the game's own code changed in place (operands, jumps, single bytes) - each
// checked against what it expects there first
// the "under attack" alert of a hit (a thunk: it ends in game_attack_alert)
internal inline int patch_attack_alert(thunk_fn thunk) { return patch_call(GA(CALL_ATTACK_ALERT), GA(FN_ATTACK_ALERT), (void *)thunk); }
internal inline uintptr_t game_attack_alert(void) { return GA(FN_ATTACK_ALERT); }
// the minimap's scale (display.c): its 6 operands of 1/800 and 1/600 read *fx and *fy instead
internal inline int patch_minimap_scale(float *fx, float *fy) {
    int n = 0;
    for (int i = 0; i < 3; i++) {
        n += patch_code32(GL(MINIMAP_FX_OPS)[i], GA(G_FLOAT_1_800), (uint32_t)(uintptr_t)fx);
        n += patch_code32(GL(MINIMAP_FY_OPS)[i], GA(G_FLOAT_1_600), (uint32_t)(uintptr_t)fy);
    }
    return n;
}
// the resolution switches: the editor's two (menu, its own), every other (7)
typedef void (__stdcall *resolution_hook)(int w, int h, int force, int flag);
internal inline int patch_editor_resolution(resolution_hook h) {
    return patch_call(GA(SITE_EDITOR_MENU), GA(FN_SET_RESOLUTION), (void *)h) + patch_call(GA(SITE_EDITOR_RES), GA(FN_SET_RESOLUTION), (void *)h);
}
internal inline int patch_other_resolution(resolution_hook h, int *of) { return patch_calls(PATCH_LIST(OTHER_RESOLUTION_SITES), GA(FN_SET_RESOLUTION), (void *)h, of); }
internal inline int patch_input_coop(HMODULE lle) {
    uint8_t *b = (uint8_t *)lle;
    int n = 0;
    if (b[GA(RVA_LLE_MOUSE_COOP) - 1] == 0x6A) n += patch_byte((uintptr_t)b + GA(RVA_LLE_MOUSE_COOP), 5, 6);
    if (b[GA(RVA_LLE_INPUT_COOP) - 1] == 0x6A) n += patch_byte((uintptr_t)b + GA(RVA_LLE_INPUT_COOP), 5, 6);
    return n;
}
internal inline int patch_no_minimize(void) { return patch_byte(GA(SITE_CLOSE_WINDOW_JE), 0x74, 0xEB); }
// the loading splash's stop (display.c): jne -> nop nop, or jmp when the start is already a jmp (*nosplash: a test run
// started without the splash - then the stop, which waits for the splash's thread, is skipped too)
internal inline int patch_splash_stop(int *nosplash) {
    uint8_t *jne = (uint8_t *)GA(CODE_SPLASH_STOP_JNE), *jstart = (uint8_t *)GA(CODE_SPLASH_START_JNE);
    *nosplash = jstart[0] == 0xEB;
    if (jne[0] != 0x75 || jne[1] != 0x19) return 0;
    DWORD old;
    VirtualProtect(jne, 2, PAGE_EXECUTE_READWRITE, &old);
    if (*nosplash) jne[0] = 0xEB; else jne[0] = jne[1] = 0x90;
    VirtualProtect(jne, 2, old, &old);
    FlushInstructionCache(GetCurrentProcess(), jne, 2);
    return 1;
}
// the bitmap fonts' characters read unsigned (display_text.c): GEFontBitmap::Draw's two movsx -> movzx in Low-Level Engine
internal inline int patch_font_movzx(HMODULE lle) {
    const uint32_t sites[2] = { GA(RVA_LLE_FONT_MOVSX_1), GA(RVA_LLE_FONT_MOVSX_2) };
    int ok = 0;
    for (int i = 0; i < 2; i++) {
        uint8_t *p = (uint8_t *)lle + sites[i];
        if (p[0] != 0x0F || (p[1] != 0xBE && p[1] != 0xB6)) continue;
        DWORD old;
        if (!VirtualProtect(p, 2, PAGE_EXECUTE_READWRITE, &old)) continue;
        p[1] = 0xB6;
        VirtualProtect(p, 2, old, &old);
        ok++;
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    return ok;
}
// THE DRAW LIST (drawlist.c): the objects on screen each frame - a fixed array of DRAW_OLD_N records of DRAW_REC bytes at
// DRAW_OLD (EEROUnits::vf0 collects them, the render thread draws them) - moved to limit records: the operands (all
// in the render code, found by a disassembly scan: two 512 limits, the array's references) checked first, all or
// nothing. 1: moved (*recs the new records, each built with the game's constructor), 0: the exe differs (*ok of *n
// operands as expected), -1: no memory.
#define DRAW_REC 0x148
#define DRAW_OLD_N 0x200
internal inline int patch_draw_list(int limit, uint8_t **recs, int *ok_out, int *n_out) {
    const uint32_t *at = GL(DRAW_SITES);   // in this order (raw/hooks.h)
    const struct { uint32_t at, want; int kind; } k[] = {
        { at[0], 0x200, 0 },                     // cmp [0x912538], 0x200 (0x5008B8: room for one more)
        { at[1], 0x200, 0 },                     // push 0x200 (0x5035AD: its sub-objects)
        { at[2], GA(DRAW_OLD) + of_draw_record_0x58, 1 },       // mov esi, records +0x58 (init loop at game start)
        { at[3], GA(DRAW_OLD) + DRAW_OLD_N * DRAW_REC + of_draw_record_0x58, 2 },   // cmp esi, its end
        { at[4], GA(DRAW_OLD), 1 },              // push records (0x500D10: frame start)
        { at[5], GA(DRAW_OLD), 1 },              // mov edi, records (collection)
        { at[6], GA(DRAW_OLD) + of_draw_record_0xfc, 1 },       // add eax, records +0xFC
        { at[7], GA(DRAW_OLD), 1 },              // push records (frame end)
        { at[8], GA(DRAW_OLD), 1 },              // push records (0x50BD9D: drawing)
    };
    int n = (int)(sizeof k / sizeof k[0]), ok = 0;
    for (int i = 0; i < n; i++) ok += *(uint32_t *)(uintptr_t)k[i].at == k[i].want;
    *ok_out = ok; *n_out = n;
    if (ok != n) return 0;
    uint8_t *r = VirtualAlloc(NULL, (SIZE_T)limit * DRAW_REC, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!r) return -1;
    for (int i = 0; i < limit; i++)
        GF(FN_DRAW_REC_CTOR)(r + (size_t)i * DRAW_REC);
    uint32_t base = (uint32_t)(uintptr_t)r;
    for (int i = 0; i < n; i++) {
        uint32_t v = k[i].kind == 0 ? (uint32_t)limit
                   : k[i].kind == 1 ? base + (k[i].want - GA(DRAW_OLD))
                   : base + (uint32_t)limit * DRAW_REC + 0x58;
        patch_code32(k[i].at, k[i].want, v);
    }
    *recs = r;
    return 1;
}
// the small objects' list the same way (0x600 records of 0x3C at DRAW_SMALL_OLD; the static constructor 0x506333 only
// zeroes their first 3 dwords, +0xC a handle the frame's start releases (0x500CE6) - a zeroed allocation is the same)
#define DRAW_SMALL_REC 0x3C
#define DRAW_SMALL_OLD_N 0x600
internal inline int patch_draw_small(int limit, uint8_t **recs, int *ok_out, int *n_out) {
    const uint32_t *at = GL(DRAW_SMALL_SITES);   // in this order (raw/hooks.h)
    const struct { uint32_t at, want; int kind; } k[] = {
        { at[0], DRAW_SMALL_OLD_N, 0 },          // cmp [G_DRAW_SMALL_N], 0x600 (0x5008B8: room for one more)
        { at[1], DRAW_SMALL_OLD_N, 0 },          // push 0x600 (0x502224: its sub-objects)
        { at[2], GA(DRAW_SMALL_OLD), 1 },        // push records (0x500CE6: frame start)
        { at[3], GA(DRAW_SMALL_OLD) + 0x24, 1 }, // add eax, records +0x24 (collection)
        { at[4], GA(DRAW_SMALL_OLD), 1 },        // add eax, records (collection)
        { at[5], GA(DRAW_SMALL_OLD), 1 },        // add ecx, records (sub-objects)
        { at[6], GA(DRAW_SMALL_OLD), 1 },        // push records (frame end)
    };
    int n = (int)(sizeof k / sizeof k[0]), ok = 0;
    for (int i = 0; i < n; i++) ok += *(uint32_t *)(uintptr_t)k[i].at == k[i].want;
    *ok_out = ok; *n_out = n;
    if (ok != n) return 0;
    uint8_t *r = VirtualAlloc(NULL, (SIZE_T)limit * DRAW_SMALL_REC, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!r) return -1;
    uint32_t base = (uint32_t)(uintptr_t)r;
    for (int i = 0; i < n; i++)
        patch_code32(k[i].at, k[i].want, k[i].kind == 0 ? (uint32_t)limit : base + (k[i].want - GA(DRAW_SMALL_OLD)));
    *recs = r;
    return 1;
}

// THE SIMULATION AND THE TESTS (test/control.c, autotest.c, trace.c)
// the simulation thread's receive (each pass of its loop: the pipe's safe point) and its timer wait (turbo)
typedef long (__thiscall *sim_receive_hook)(void *thread, void *a, void *b, void *c);
internal inline int patch_sim_receive(sim_receive_hook h) { return patch_import_call(GA(SITE_SIM_RECEIVE), GA(IAT_TS_RECEIVE), (void *)h); }
typedef long (__thiscall *sim_wait_hook)(void *ev, unsigned long ms);
internal inline int patch_sim_wait(sim_wait_hook h) { return patch_import_call(GA(SITE_SIM_WAIT), GA(IAT_TS_WAIT), (void *)h); }
// the lockstep server's wait for its next tick (Low-Level Engine: NESynchronousServer::Send's WaitForMultipleObjects)
typedef DWORD (WINAPI *send_wait_hook)(DWORD n, const HANDLE *h, BOOL all, DWORD ms);
internal inline int patch_send_wait(send_wait_hook h) {
    uintptr_t base = (uintptr_t)GetModuleHandleA("low-level engine.dll");
    return base && patch_import_call(base + GA(RVA_LLE_SEND_WAIT), base + GA(RVA_LLE_IAT_WFMO), (void *)h);
}
// the game's srand (its msvcrt import): the previous function into *orig
typedef void (__cdecl *srand_hook)(unsigned seed);
internal inline int patch_srand(srand_hook h, srand_hook *orig) { return iat_hook(GA(IAT_SRAND), "msvcrt.dll", "srand", (void *)h, (void **)orig); }
// the intro movies skipped: each `call AddMovie` (the callee pops its argument) -> `add esp,4; nop; nop`
internal inline int patch_skip_intro(void) {
    const uint32_t *sites = GL(INTRO_CALLS);
    static const uint8_t stub[5] = { 0x83, 0xC4, 0x04, 0x90, 0x90 };
    for (int i = 0; i < 3; i++) {
        uint8_t *p = (uint8_t *)sites[i];
        if (p[0] != 0xE8 || sites[i] + 5 + *(int32_t *)(p + 1) != GA(FN_ADD_MOVIE)) return FALSE;
    }
    for (int i = 0; i < 3; i++) {
        DWORD old;
        VirtualProtect((void *)sites[i], 5, PAGE_EXECUTE_READWRITE, &old);
        memcpy((void *)sites[i], stub, 5);
        VirtualProtect((void *)sites[i], 5, old, &old);
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    return TRUE;
}
// the network ports (UDP 33334, TCP 33335: TCP Protocol.dll's immediates, the module m) moved by add - how many of 8
internal inline int patch_tcp_ports(HMODULE m, int add) {
    const uint32_t *imm = GL(RVA_TCP_PORTS);
    int n = 0;
    for (size_t i = 0; m && i < sizeof GL(RVA_TCP_PORTS) / sizeof *imm; i++) {
        uintptr_t at = (uintptr_t)m + imm[i];
        uint32_t old = *(uint32_t *)at;
        if (old == 33334 || old == 33335) n += patch_code32(at, old, old + (uint32_t)add);
    }
    return n;
}
typedef int (__thiscall *shore_to_unload_hook)(void *mgr, ee_unit *transport);
internal inline int patch_shore_to_unload(shore_to_unload_hook h) { return patch_call(GA(CALL_SHORE_TO_UNLOAD), GA(FN_SHORE_TO_UNLOAD), (void *)h); }

// THE PLAYERS' COLOURS AND LINES (voice/persona.c)
// the exit dialog's confirmation (a code pointer in the menu's table)
internal inline int patch_exit_confirm(void (__cdecl *h)(void)) { return patch_code32(GA(EXIT_CONFIRM_AT), GA(FN_EXIT_CONFIRM), (uint32_t)(uintptr_t)h); }

// The editor layout for any resolution from 800x600 up: Art of Conquest's 1024x768 values (generated from a
// comparison of both exes, scripts/re/layout_diff.py + gen_editor_layout.py), each moved with the screen size the way
// AoC moved it from EE's 800x600: anchored right/bottom, stretched, or centred. Computed when the editor is entered.
#define N_LAYOUT (sizeof k_editor_layout / sizeof *k_editor_layout)
internal uint32_t g_layout_consts[N_LAYOUT];
internal int8_t g_layout_state[N_LAYOUT];   // 0 unchecked, 1 patched, -1 didn't match (skipped)
internal int g_buttons_state;
internal int apply_editor_layout(int w, int h) {
    int ok = 0, bad = 0;
    if (g_exe_build != 0) return 0;   // raw/editor_layout.h is the 2017 update's (builds.h)
    for (size_t i = 0; i < N_LAYOUT; i++) {
        uint32_t *p = (uint32_t *)k_editor_layout[i].addr;
        if (!g_layout_state[i]) {   // first time: the original value must be there
            uint32_t *c = k_editor_layout[i].kind ? (uint32_t *)*p : p;
            int match = k_editor_layout[i].kind
                ? (uintptr_t)c >= g_image_lo && (uintptr_t)c < g_image_hi && *c == k_editor_layout[i].old
                : *p == k_editor_layout[i].old;
            g_layout_state[i] = match ? 1 : -1;
            if (match && k_editor_layout[i].kind) write_code32(p, (uint32_t)(uintptr_t)&g_layout_consts[i]);
        }
        if (g_layout_state[i] < 0) { bad++; continue; }
        float v = k_editor_layout[i].aoc + k_editor_layout[i].fx * 0.5f * (float)(w - 1024)
                + k_editor_layout[i].fy * 0.5f * (float)(h - 768);
        uint32_t bits;
        if (k_editor_layout[i].isf) memcpy(&bits, &v, 4);
        else bits = (uint32_t)(int32_t)lroundf(v);
        if (k_editor_layout[i].kind) g_layout_consts[i] = bits;
        else if (*p != bits) write_code32(p, bits);
        ok++;
    }
    // the file buttons (load, save, test, exit ...) come from a data table, not code: 0x8AE0A0, 6 entries of
    // {id, x, y, texture u0, v0, u1, v1, tooltip}; AoC (0x8C47B0) has them at x 647.. (step 35), y 736 for 1024x768
    int32_t *t = editor_file_buttons();
    if (!g_buttons_state) g_buttons_state = t[1] == 415 && t[2] == 567 ? 1 : -1;
    if (g_buttons_state > 0)
        for (int i = 0; i < 6; i++, t += 8) {
            t[1] = 647 + 35 * i + (w - 1024);
            t[2] = 736 + (h - 768);
            ok++;
        }
    else bad++;
    if (g_log && bad) log_locked("# editor: %d layout patch sites did not match and were skipped\n", bad);
    return ok;
}
