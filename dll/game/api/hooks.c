// THE GAME'S FUNCTIONS THE MOD HOOKS: for each, the type of its hook and the installer. A hook stands in for the game's
// function at its calls (patch_call: the call instruction redirected, only while it still calls that function) or in
// its class's vtable (patch_vtable: only while the slot still holds it); it calls the original through functions.c's
// wrapper when it wants the game's own work done. The hook types are the game's real arguments - a hook declared with
// other parameters doesn't build (a thiscall hook is __fastcall with the unused edx second). An installer returns how
// many places it patched; of (may be NULL) gets the number there are. Kinds not typed yet are void *, as in functions.c.
// Assembler stubs (they read registers) are void (*)(void).

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
// a site search called from the citizen assignment's leftover pass (0x48A1AE: its calls lie below LEFTOVER_SITE_END)
internal inline int in_leftover_pass(uintptr_t at) { return at < GA(LEFTOVER_SITE_END); }

// THE GAME'S RUN (the wiring: eeaimod.c, persist.c; test/autotest.c's command line)
typedef char (__fastcall *game_start_hook)(void *state, void *edx, uint8_t *setup, void *p2);
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
// shots (aim.c): the aim of a unit's and a missile's shot, a projectile down, a missile launched, a shot fired
typedef void (__cdecl *aim_hook)(ee_unit *u, ee_unit **target, float *aim, void *zero);
internal inline int patch_aim(aim_hook h) { return patch_call(GA(CALL_AIM), GA(FN_AIM), (void *)h); }
internal inline int patch_missile_aim(aim_hook h) { return patch_call(GA(CALL_MISSILE_AIM), GA(FN_AIM), (void *)h); }
typedef char (__fastcall *proj_down_hook)(void *cls, void *edx, uint8_t *projectile);
internal inline int patch_proj_down(proj_down_hook h) { return patch_vtable(GA(VT_PROJECTILE), GA(SLOT_PROJ_DOWN), GA(FN_PROJ_DOWN), (void *)h); }
typedef int (__fastcall *missile_launch_hook)(uint8_t *cls, void *edx, ee_unit *m, float *aim);
internal inline int patch_missile_launch(missile_launch_hook h) { return patch_vtable(GA(VT_MISSILE), GA(SLOT_MISSILE_LAUNCH), GA(FN_MISSILE_LAUNCH), (void *)h); }
typedef void (__fastcall *fire_hook)(uint8_t *cls, void *edx, ee_unit *u);
internal inline int patch_fire(fire_hook h, int *of) { return patch_calls(PATCH_LIST(FIRE_CALLS), GA(FN_FIRE), (void *)h, of); }
// the "under attack" alert of a hit (a thunk: it ends in game_attack_alert)
internal inline int patch_attack_alert(thunk_fn thunk) { return patch_call(GA(CALL_ATTACK_ALERT), GA(FN_ATTACK_ALERT), (void *)thunk); }
internal inline uintptr_t game_attack_alert(void) { return GA(FN_ATTACK_ALERT); }
// the attack subgroups' pop of a waiting unit (asg_queue.c: a thunk, it calls game_asg_queue_pop) - the land and the air ones
internal inline int patch_asg_queue_pop(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { GA(CALL_ASG_QUEUE_POP), GA(CALL_ASG_AIR_QUEUE_POP) };
    return patch_calls(sites, 2, GA(FN_ASG_QUEUE_POP), (void *)thunk, of);
}
internal inline uintptr_t game_asg_queue_pop(void) { return GA(FN_ASG_QUEUE_POP); }
// the camera's scroll action (camera.c), and its timer's period: push 25 (AoC 10) ms -> value
typedef void (__fastcall *scroll_hook)(uint8_t *self, void *edx);
internal inline int patch_scroll_execute(scroll_hook h) { return patch_vtable(GA(VT_SCROLL), GA(SLOT_SCROLL_EXECUTE), GA(FN_SCROLL_EXECUTE), (void *)h); }
internal inline int patch_scroll_timer(uint8_t old_ms, uint8_t ms) {
    uint8_t *p = (uint8_t *)GA(SITE_SCROLL_TIMER);
    if (p[-1] != 0x6A || p[0] != old_ms) return 0;
    DWORD old;
    VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old);
    *p = ms;
    VirtualProtect(p, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
    return 1;
}
// the selection's packing into a message (select.c)
typedef unsigned (__cdecl *sel_pack_hook)(uint8_t *list, uint32_t player_byte, uint8_t *out);
internal inline int patch_sel_pack(sel_pack_hook h) { return patch_call(GA(CALL_SEL_PACK), GA(FN_SEL_PACK), (void *)h); }
// the sound manager (sound.c, through the exe's imports of Low-Level Engine: the slots checked against the exports)
typedef uint32_t (__thiscall *snd_play_fn)(void *sm, uint32_t id, float vol, float pan, float pitch, int queue, void *name);
typedef uint32_t (__fastcall *snd_play_hook)(void *sm, void *edx, uint32_t id, float vol, float pan, float pitch, int queue, void *name);
internal inline int patch_snd_play(snd_play_hook h, snd_play_fn *orig) {
    return iat_hook(GA(IAT_SND_PLAY), "Low-Level Engine.dll", "?Play@SESoundManager@@QAEKKMMM_NPAD@Z", (void *)h, (void **)orig);
}
internal inline int patch_snd_playref(snd_play_hook h, snd_play_fn *orig) {
    return iat_hook(GA(IAT_SND_PLAYREF), "Low-Level Engine.dll", "?PlayAndReturnReferencedNode@SESoundManager@@QAEPAVSESMInstanceNode@@KMMM_NPAD@Z",
                    (void *)h, (void **)orig);
}
typedef int (__thiscall *set_max_ch_fn)(void *sm, uint32_t n);
typedef int (__fastcall *set_max_ch_hook)(uint8_t *sm, void *edx, uint32_t n);
internal inline int patch_snd_set_max_channels(set_max_ch_hook h, set_max_ch_fn *orig) {
    return iat_hook(GA(IAT_SND_SETMAXCH), "Low-Level Engine.dll", "?SetMaxChannels@SESoundManager@@QAEJK@Z", (void *)h, (void **)orig);
}
// the single-player menu's six buttons (playername.c): their handlers (vtable slot 1) -> hooks[i], the originals into orig
internal inline int patch_sp_buttons(void *const hooks[6], void *orig[6]) {
    int ok = 0;
    for (int i = 0; i < 6; i++) {
        orig[i] = (void *)GL(SP_FNS)[i];
        ok += patch_vtable(GL(SP_VTABLES)[i], GA(SLOT_SP_BUTTON), GL(SP_FNS)[i], hooks[i]);
    }
    return ok;
}
// the minimap's scale (display.c): its 6 operands of 1/800 and 1/600 read *fx and *fy instead
internal inline int patch_minimap_scale(float *fx, float *fy) {
    int n = 0;
    for (int i = 0; i < 3; i++) {
        n += patch_code32(GL(MINIMAP_FX_OPS)[i], GA(G_FLOAT_1_800), (uint32_t)(uintptr_t)fx);
        n += patch_code32(GL(MINIMAP_FY_OPS)[i], GA(G_FLOAT_1_600), (uint32_t)(uintptr_t)fy);
    }
    return n;
}
// the resolution switches: the editor's two (menu, its own), every other (7), the editor's main form built
typedef void (__stdcall *resolution_hook)(int w, int h, int force, int flag);
internal inline int patch_editor_resolution(resolution_hook h) {
    return patch_call(GA(SITE_EDITOR_MENU), GA(FN_SET_RESOLUTION), (void *)h) + patch_call(GA(SITE_EDITOR_RES), GA(FN_SET_RESOLUTION), (void *)h);
}
internal inline int patch_other_resolution(resolution_hook h, int *of) { return patch_calls(PATCH_LIST(OTHER_RESOLUTION_SITES), GA(FN_SET_RESOLUTION), (void *)h, of); }
typedef void *(__fastcall *editor_form_hook)(void *form, void *edx, void *owner);
internal inline int patch_editor_form(editor_form_hook h) { return patch_call(GA(SITE_EDITOR_FORM), GA(FN_EDITOR_FORM), (void *)h); }
// the windowed mode's input (display.c): the mouse's update (its 4 calls), the cooperative level of Low-Level Engine's
// input (push 5 -> push 6, exclusive -> non-exclusive: the mouse's Resume and the device setup), no minimize on focus
// loss (je -> jmp)
typedef void (__fastcall *mouse_update_hook)(uint8_t *m, void *edx);
internal inline int patch_mouse_update(mouse_update_hook h, int *of) { return patch_calls(PATCH_LIST(MOUSE_UPDATE_SITES), GA(FN_MOUSE_UPDATE), (void *)h, of); }
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
        { at[2], GA(DRAW_OLD) + OF(DRAWREC, 0x58), 1 },       // mov esi, records +0x58 (init loop at game start)
        { at[3], GA(DRAW_OLD) + DRAW_OLD_N * DRAW_REC + OF(DRAWREC, 0x58), 2 },   // cmp esi, its end
        { at[4], GA(DRAW_OLD), 1 },              // push records (0x500D10: frame start)
        { at[5], GA(DRAW_OLD), 1 },              // mov edi, records (collection)
        { at[6], GA(DRAW_OLD) + OF(DRAWREC, 0xFC), 1 },       // add eax, records +0xFC
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

// THE SIMULATION AND THE TESTS (test/control.c, autotest.c, trace.c)
// the simulation thread's receive (each pass of its loop: the pipe's safe point) and its timer wait (turbo)
typedef long (__fastcall *sim_receive_hook)(void *thread, void *edx, void *a, void *b, void *c);
internal inline int patch_sim_receive(sim_receive_hook h) { return patch_import_call(GA(SITE_SIM_RECEIVE), GA(IAT_TS_RECEIVE), (void *)h); }
typedef long (__fastcall *sim_wait_hook)(void *ev, void *edx, unsigned long ms);
internal inline int patch_sim_wait(sim_wait_hook h) { return patch_import_call(GA(SITE_SIM_WAIT), GA(IAT_TS_WAIT), (void *)h); }
// the lockstep server's wait for its next tick (Low-Level Engine: NESynchronousServer::Send's WaitForMultipleObjects)
typedef DWORD (WINAPI *send_wait_hook)(DWORD n, const HANDLE *h, BOOL all, DWORD ms);
internal inline int patch_send_wait(send_wait_hook h) {
    uintptr_t base = (uintptr_t)GetModuleHandleA("low-level engine.dll");
    return base && patch_import_call(base + GA(RVA_LLE_SEND_WAIT), base + GA(RVA_LLE_IAT_WFMO), (void *)h);
}
// one simulation step (EEPhysics' vtable slot 8): the hook, and whether it is in place
typedef void (__fastcall *phys_step_hook)(void *phys, void *edx);
internal inline int patch_phys_step(phys_step_hook h) { return patch_code32(GA(VTBL_PHYS_STEP), GA(FN_PHYS_STEP), (uint32_t)(uintptr_t)h); }
internal inline int phys_step_hooked_by(phys_step_hook h) { return *(uint32_t *)GA(VTBL_PHYS_STEP) == (uint32_t)(uintptr_t)h; }
// the game start message's execution (its vtable slot 10: it seeds the simulation's generator)
typedef void (__fastcall *start_exec_hook)(void *msg, void *edx);
internal inline int patch_start_exec(start_exec_hook h) { return patch_code32(GA(START_VTBL_EXEC), GA(START_EXEC), (uint32_t)(uintptr_t)h); }
// the lockstep server's send (its vtable slot 6, on the live object: EEServer or EEClient) - the slot's previous
// function, NULL when not patched; two hooks chain (the second calls the first)
typedef void (__thiscall *server_send_fn)(void *net, void *msg, int to);
typedef void (__fastcall *server_send_hook)(void *net, void *edx, void *msg, int to);
internal inline server_send_fn patch_server_send(void *net, server_send_hook h) {
    void **slot = &((void **)*(void ***)net)[GA(SLOT_SERVER_SEND)];
    DWORD old;
    if (!VirtualProtect(slot, 4, PAGE_READWRITE, &old)) return NULL;
    server_send_fn prev = (server_send_fn)*slot;
    *slot = (void *)h;
    VirtualProtect(slot, 4, old, &old);
    return prev;
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

// COMPUTER PLAYERS (econ.c)
// the computer player's update (engine/scenario_ai.c: the scenario flag cleared meanwhile)
typedef void (__fastcall *cp_update_hook)(ee_player *pl, void *edx, uint32_t now);
internal inline int patch_cp_update(cp_update_hook h) { return patch_call(GA(CALL_CP_UPDATE), GA(FN_CP_UPDATE), (void *)h); }
typedef void (__fastcall *cp_pay_hook)(ee_player *pl, void *edx, void *what, int b, int c);
internal inline int patch_cp_pay(cp_pay_hook h) { return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_PAY), GA(FN_CP_PAY), (void *)h); }
typedef void (__fastcall *cp_add_resource_hook)(ee_player *pl, void *edx, int type, int amount);
internal inline int patch_cp_add_resource(cp_add_resource_hook h) {
    return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_ADD_RESOURCE), GA(FN_CP_ADD_RESOURCE), (void *)h);
}
typedef void (__fastcall *pop_cap_hook)(ee_player *pl, void *edx);
internal inline int patch_pop_cap(pop_cap_hook h, int *of) {
    const uint32_t sites[3] = { GA(CALL_POP_CAP_1), GA(CALL_POP_CAP_2), GA(CALL_POP_CAP_3) };
    return patch_calls(sites, 3, GA(FN_POP_CAP), (void *)h, of);
}
typedef void (__fastcall *epoch_catch_up_hook)(ee_player *ai, void *edx, ee_player *human);
internal inline int patch_epoch_catch_up(epoch_catch_up_hook h) { return patch_call(GA(CALL_EPOCH_CATCH_UP), GA(FN_EPOCH_CATCH_UP), (void *)h); }
// the epoch's upgrades: the 2017 update's (node, category), Art of Conquest's (node, category, only new)
typedef void (__fastcall *epoch_upgrades_hook)(ee_tech *node, void *edx, int cat);
typedef void (__fastcall *epoch_upgrades_aoc_hook)(ee_tech *node, void *edx, int cat, int only_new);
internal inline int patch_epoch_upgrades(epoch_upgrades_hook h, epoch_upgrades_aoc_hook h_aoc) {
    return patch_call(GA(CALL_EPOCH_UPGRADES), GA(FN_EPOCH_UPGRADES), build_is("aoc") ? (void *)h_aoc : (void *)h);
}
// a tech researched for nothing (EETechTreeNode::vf6, its vtable slot VTS_TECH_RESEARCHED)
typedef int (__fastcall *tech_grant_hook)(ee_tech *t, void *edx);
internal inline int patch_tech_grant(tech_grant_hook h) { return patch_vtable(GA(VTS_TECH_RESEARCHED), 0, GA(FN_TECH_GRANT), (void *)h); }

// THE AI'S MANAGERS
// the resource manager's turn (the wiring, eeaimod.c): its gatherer targets, its citizens' assignment, its update
typedef char (__fastcall *rm_hook)(ee_resource_mgr *rm, void *edx);
internal inline int patch_rm_compute_targets(rm_hook h) { return patch_call(GA(CALL_COMPUTE_TARGETS), GA(FN_COMPUTE_TARGETS), (void *)h); }
internal inline int patch_rm_assign_citizens(rm_hook h) { return patch_call(GA(CALL_ASSIGN_CITIZENS), GA(FN_ASSIGN_CITIZENS), (void *)h); }
typedef void (__fastcall *rm_update_hook)(ee_resource_mgr *rm, void *edx, uint32_t now);
internal inline int patch_rm_update(rm_update_hook h) { return patch_call(GA(CALL_RM_UPDATE), GA(FN_RM_UPDATE), (void *)h); }
typedef char (__fastcall *place_hook)(ee_build_mgr *bm, void *edx, int type, int x, int y);
internal inline int patch_place(place_hook h) { return patch_call(GA(CALL_PLACE), GA(FN_PLACE), (void *)h); }
typedef uintptr_t (__fastcall *train_units_hook)(ee_unit_mgr *um, void *edx, int *req, uintptr_t b);
internal inline int patch_train_units(train_units_hook h, int *of) { return patch_calls(PATCH_LIST(TRAIN_SITES), GA(FN_TRAIN_UNITS), (void *)h, of); }
typedef int (__fastcall *citizens_wanted_hook)(ee_unit_mgr *um, void *edx);
internal inline int patch_citizens_wanted(citizens_wanted_hook h) { return patch_call(GA(CALL_CITIZENS_WANTED), GA(FN_UM_CITIZENS_WANTED), (void *)h); }
typedef int (__fastcall *fish_type_hook)(void *o, void *edx);
internal inline int patch_fish_type(fish_type_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_FISH_TYPE_1), GA(CALL_FISH_TYPE_2) };
    return patch_calls(sites, 2, GA(FN_FISH_TYPE), (void *)h, of);
}
// the maintenance's deletion of units for room (the navy keeps its ships: fleet.c)
typedef int (__fastcall *delete_for_room_hook)(ee_player *pl, void *edx, void *req, int flag);
internal inline int patch_maint_delete(delete_for_room_hook h) { return patch_call(GA(CALL_NV_MAINT_DELETE), GA(FN_DELETE_FOR_ROOM), (void *)h); }

// THE RESOURCE MANAGER'S SEARCHES (sites.c)
typedef void *(__fastcall *find_mine_hook)(ee_resource_mgr *rm, void *edx, int type, ee_unit *unit, uintptr_t rev);
internal inline int patch_find_mine(find_mine_hook h, int *of) { return patch_calls(PATCH_LIST(MINE_SITES), GA(FN_FIND_MINE), (void *)h, of); }
typedef void *(__fastcall *find_food_hook)(ee_resource_mgr *rm, void *edx, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit);
internal inline int patch_find_food(find_food_hook h, int *of) { return patch_calls(PATCH_LIST(FOOD_SITES), GA(FN_FIND_FOOD), (void *)h, of); }
typedef void *(__fastcall *find_wood_hook)(ee_resource_mgr *rm, void *edx, uintptr_t flag, ee_unit *unit);
internal inline int patch_find_wood(find_wood_hook h, int *of) { return patch_calls(PATCH_LIST(WOOD_SITES), GA(FN_FIND_WOOD), (void *)h, of); }
typedef void *(__fastcall *farms_hook)(ee_resource_mgr *rm, void *edx, uintptr_t n, uintptr_t out);
internal inline int patch_farms(farms_hook h, farms_hook unbuilt, int *of) {   // unbuilt NULL: the fields only
    if (of) *of += unbuilt ? 2 : 1;
    return patch_call(GA(CALL_FARMS), GA(FN_FARMS), (void *)h) + (unbuilt ? patch_call(GA(CALL_UNBUILT_FARMS), GA(FN_UNBUILT_FARMS), (void *)unbuilt) : 0);
}
// a citizen sent to work at a site (the calls of the assignment: GATHER_SITES), the citizens trained (trace.c)
typedef void (__fastcall *gather_hook)(ee_resource_mgr *rm, void *edx, ee_unit *u, ee_unit *site, int c);
internal inline int patch_gather(gather_hook h, int *of) { return patch_calls(PATCH_LIST(GATHER_SITES), GA(FN_ASSIGN_TO_SITE), (void *)h, of); }
typedef uintptr_t (__fastcall *train_citizens_hook)(ee_unit_mgr *um, void *edx, uintptr_t a);
internal inline int patch_train_citizens(train_citizens_hook h) { return patch_call(GA(CALL_TRAIN_CITIZENS), GA(FN_TRAIN_CITIZENS), (void *)h); }
typedef void *(__fastcall *hunt_hook)(ee_resource_mgr *rm, void *edx, uintptr_t n, uintptr_t f, uintptr_t out);
internal inline int patch_hunt(hunt_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_HUNT_1), GA(CALL_HUNT_2) };
    return patch_calls(sites, 2, GA(FN_HUNT), (void *)h, of);
}

// UNITS AND GOALS
typedef void (__fastcall *set_goal_hook)(ee_unit *u, void *edx, void *goal, int append, int prio);
// the game's repair orders (the handing-over 0x46DA88: attack.c refuses them under attack)
internal inline int patch_repair_goal(set_goal_hook h) { return patch_call(GA(CALL_REPAIR_GOAL), GA(FN_SET_GOAL), (void *)h); }
typedef void (__fastcall *give_goal_hook)(ee_unit *u, void *edx, void *goal, int slot);
internal inline int patch_unload_retry_give(give_goal_hook h) { return patch_call(GA(CALL_UNLOAD_RETRY_GIVE), GA(FN_GIVE_GOAL), (void *)h); }
typedef void *(__fastcall *unload_ctor_hook)(void *g, void *edx, ee_unit *transport, int x, int y, int flag);
internal inline int patch_unload_ctor(unload_ctor_hook h, int *of) { return patch_calls(PATCH_LIST(UNLOAD_SITES), GA(FN_GOAL_UNLOAD_CTOR), (void *)h, of); }
typedef int (__fastcall *shore_to_unload_hook)(void *mgr, void *edx, ee_unit *transport);
internal inline int patch_shore_to_unload(shore_to_unload_hook h) { return patch_call(GA(CALL_SHORE_TO_UNLOAD), GA(FN_SHORE_TO_UNLOAD), (void *)h); }
typedef int (__fastcall *goalset_ask_hook)(void *set, void *edx, ee_unit *unit, int *out);
internal inline int patch_goalset_ask(goalset_ask_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_GOALSET_ASK), GA(CALL_GOALSET_AGAIN) };
    return patch_calls(sites, 2, GA(FN_GOALSET_ASK), (void *)h, of);
}
// the goal set's read of a unit's goal object (6 bytes: mov ecx, [esi+0x1E8]) -> call thunk; nop
internal inline int patch_goalset_read(thunk_fn thunk) {
    static const uint8_t want[6] = { 0x8B, 0x8E, 0xE8, 0x01, 0x00, 0x00 };
    uint8_t *p = (uint8_t *)GA(AT_GOAL_SET_READ);
    DWORD old;
    if (memcmp(p, want, 6) || !VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    p[0] = 0xE8; *(int32_t *)(p + 1) = (int32_t)((uintptr_t)thunk - (GA(AT_GOAL_SET_READ) + 5)); p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return 1;
}
typedef char (__fastcall *goal_fits_hook)(void *goal, void *edx, void *unit);
internal inline int patch_goal_fits(goal_fits_hook h) { return patch_call(GA(CALL_GOAL_FBA_CHECK), GA(FN_GOAL_FBA_CHECK), (void *)h); }
typedef void (__fastcall *unit_release_hook)(ee_unit *u, void *edx);
internal inline int patch_unit_release(unit_release_hook h) { return patch_vtable(GA(VT_COMPLEX_UNIT), GA(SLOT_UNIT_RELEASE), GA(FN_UNIT_RELEASE), (void *)h); }
// the unit list's node deletions (operator delete of a node, the unit in ESI): the thunk ends in game_op_delete
internal inline int patch_unit_node_free(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { GA(CALL_NODE_DEL_1), GA(CALL_NODE_DEL_2) };
    return patch_calls(sites, 2, GA(FN_OP_DELETE), (void *)thunk, of);
}
internal inline uintptr_t game_op_delete(void) { return GA(FN_OP_DELETE); }
// a model's damage points (the guard: defence.c), a kill counted (diplo.c): thunks
internal inline int patch_damage_points(thunk_fn thunk) { return patch_call(GA(CALL_DAMAGE_POINTS), GA(FN_DAMAGE_POINTS), (void *)thunk); }
internal inline int patch_kill_stat(thunk_fn thunk) { return patch_call(GA(CALL_KILL_STAT), GA(FN_STAT_ADD), (void *)thunk); }

// ATTACK GROUPS AND SUBGROUPS (attack.c, assault.c, landing.c)
typedef char (__fastcall *unit_to_group_hook)(void *cmd, void *edx, ee_unit *u);
internal inline int patch_unit_to_group(unit_to_group_hook h) { return patch_call(GA(CALL_UNIT_TO_GROUP), GA(FN_UNIT_TO_GROUP), (void *)h); }
typedef void (__fastcall *group_stat_hook)(void *stats, void *edx, ee_unit *u);
internal inline int patch_group_stat(group_stat_hook h) { return patch_call(GA(CALL_GROUP_STAT), GA(FN_GROUP_STAT), (void *)h); }
typedef void (__cdecl *range_sort_hook)(void **b, void **e, ee_unit *unit);
internal inline int patch_range_sort(range_sort_hook h) { return patch_call(GA(CALL_RANGE_SORT), GA(FN_RANGE_SORT), (void *)h); }
typedef void (__fastcall *asg_hook)(void *sg, void *edx);
// the subgroups' stats: the land subgroups' own, the others' (ASG_VTABLES)
internal inline int patch_asg_land_stats(asg_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_STATS), GA(FN_ASG_LAND_STATS), (void *)h); }
internal inline int patch_asg_stats(asg_hook h, int *of) { return patch_vtables(PATCH_LIST(ASG_VTABLES), GA(SLOT_ASG_STATS), GA(FN_ASG_STATS), (void *)h, of); }
typedef void (__fastcall *asg_update_hook)(void *sg, void *edx, int now);
internal inline int patch_asg_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_UPDATE), GA(FN_ASG_UPDATE), (void *)h); }
internal inline int patch_asg_sea_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_SEA), GA(SLOT_ASG_UPDATE), GA(FN_ASG_SEA_UPDATE), (void *)h); }
internal inline int patch_asg_sea_transport_update(asg_update_hook h) {
    return patch_vtable(GA(VT_ASG_SEA_TRANSPORT), GA(SLOT_ASG_UPDATE), GA(FN_ASG_SEA_TRANSPORT_UPDATE), (void *)h);
}
internal inline int patch_asg_air_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_AIR), GA(SLOT_ASG_UPDATE), GA(FN_ASG_AIR_UPDATE), (void *)h); }
// the subgroups' orders to a unit (vf57): sea transport, sea, land through h, air through h_air - of 4
typedef char (__fastcall *asg_give_goal_hook)(void *sg, void *edx, ee_unit *u, void *goal, int a, int prio);
internal inline int patch_asg_give_goal(asg_give_goal_hook h, asg_give_goal_hook h_air) {
    return patch_vtable(GA(VT_ASG_SEA_TRANSPORT), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_SEA), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_AIR), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_AIR_GIVE_GOAL), (void *)h_air);
}
// a land subgroup's route and its arrival, the route planner's edge cost
typedef void (__fastcall *asg_route_hook)(void *sg, void *edx, int *start, int *dest);
internal inline int patch_asg_route(asg_route_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_ROUTE), GA(FN_ASG_ROUTE), (void *)h); }
internal inline int patch_asg_route_done(asg_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_ROUTE_DONE), GA(FN_ASG_ROUTE_DONE), (void *)h); }
typedef int (__fastcall *route_cost_hook)(void *planner, void *edx, int *edge, uint8_t *node);
internal inline int patch_route_cost(route_cost_hook h) { return patch_vtable(GA(VT_ROUTE_PLANNER), GA(SLOT_ROUTE_COST), GA(FN_ROUTE_COST), (void *)h); }
// the land members' score of an enemy (base, hero, priests and prophets: ASGU_VTABLES)
typedef float (__fastcall *asgu_score_hook)(void *member, void *edx, uint8_t *info);
internal inline int patch_asgu_score(asgu_score_hook h, int *of) {
    return patch_vtables(PATCH_LIST(ASGU_VTABLES), GA(SLOT_ASGU_SCORE), GA(FN_ASGU_SCORE), (void *)h, of);
}

// THE PLAYERS' COLOURS AND LINES (voice/persona.c)
typedef void *(__cdecl *player_color_hook)(unsigned idx);
internal inline int patch_player_color(player_color_hook h) { return patch_call(GA(CALL_PLAYER_COLOR), GA(FN_COLOR_RGB), (void *)h); }
typedef char (__fastcall *native_chat_hook)(uint8_t *pim, void *edx, int base, int ms);
internal inline int patch_native_chat(native_chat_hook h, int *of) { return patch_calls(PATCH_LIST(NATIVE_CHAT_SITES), GA(FN_NATIVE_CHAT), (void *)h, of); }

// MESSAGES AND THE MENU (diplo.c)
typedef void (__fastcall *msg_hook)(void *msg, void *edx);
internal inline int patch_msg_pay_tribute(msg_hook h) { return patch_vtable(GA(VT_PAY_TRIBUTE), GA(SLOT_MSG_EXECUTE), GA(FN_PAY_TRIBUTE), (void *)h); }
internal inline int patch_msg_ingame_chat(msg_hook h) { return patch_vtable(GA(VT_INGAME_CHAT), GA(SLOT_MSG_EXECUTE), GA(FN_INGAME_CHAT), (void *)h); }
internal inline int patch_msg_resign(msg_hook h) { return patch_vtable(GA(VT_RESIGN), GA(SLOT_MSG_EXECUTE), GA(FN_RESIGN), (void *)h); }
// the exit dialog's confirmation (a code pointer in the menu's table)
internal inline int patch_exit_confirm(void (__cdecl *h)(void)) { return patch_code32(GA(EXIT_CONFIRM_AT), GA(FN_EXIT_CONFIRM), (uint32_t)(uintptr_t)h); }
