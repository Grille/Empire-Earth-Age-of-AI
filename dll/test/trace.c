// Assignment trace (autotest ini: trace_player=<n>, optional trace_until_min=<m>): logs the resource
// manager's site searches and gather orders for one AI player as "# T ..." lines. All hooked functions
// are __thiscall on the EECPResourceManager. Call sites found with scripts/refs.py. Included by eeaimod.c.

internal int g_trace_player = -1;
internal uint32_t g_trace_until_ms;

internal int tracing(void *rm) {
    if (g_trace_player < 0) return 0;
    ee_player *pl = rm_player(rm);
    if (!pl || (player_get_index(pl) != g_trace_player)) return 0;
    return !g_trace_until_ms || game_time() < g_trace_until_ms;
}

internal void trace_obj(char *buf, size_t cap, const char *tag, void *o) {
    size_t n = strlen(buf);
    if (!o) { snprintf(buf + n, cap - n, " %s=0", tag); return; }
    snprintf(buf + n, cap - n, " %s=%p(%s %d,%d)", tag, o, rtti_name(o), unit_get_x(o), unit_get_y(o));
}

internal void trace_emit(const char *line) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# T %u %s\n", game_time(), line);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

#define CALLER() ((uintptr_t)__builtin_return_address(0) - 5)

internal void *__fastcall tr_find_food(ee_resource_mgr *rm, void *edx, uintptr_t a, uintptr_t b, ee_unit *u) {
    (void)edx;
    void *r = rm_find_food(rm, a, b, u);
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x find_food(%u,%u)", (unsigned)CALLER(), (unsigned)(a & 0xFF), (unsigned)(b & 0xFF));
        trace_obj(buf, sizeof buf, "unit", (void *)u);
        trace_obj(buf, sizeof buf, "->", r);
        if (r) { size_t n = strlen(buf); snprintf(buf + n, sizeof buf - n, " gatherers=%u",
                 site_gatherers(r)); }
        trace_emit(buf);
    }
    return r;
}

internal void __fastcall tr_gather(ee_resource_mgr *rm, void *edx, ee_unit *u, ee_unit *t, int c) {
    (void)edx;
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x gather", (unsigned)CALLER());
        trace_obj(buf, sizeof buf, "unit", (void *)u);
        trace_obj(buf, sizeof buf, "target", (void *)t);
        trace_emit(buf);
    }
    rm_assign_as(rm, u, t, c);
}

internal void *__fastcall tr_find_mine(ee_resource_mgr *rm, void *edx, int type, ee_unit *u, uintptr_t rev) {
    (void)edx;
    void *r = rm_find_mine(rm, (uintptr_t)type, u, rev);
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x find_mine(type %u)", (unsigned)CALLER(), (unsigned)type);
        trace_obj(buf, sizeof buf, "->", r);
        trace_emit(buf);
    }
    return r;
}

internal void *__fastcall tr_find_wood(ee_resource_mgr *rm, void *edx, uintptr_t f, ee_unit *u) {
    (void)edx;
    void *r = rm_find_wood(rm, f, u);
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x find_wood(%u)", (unsigned)CALLER(), (unsigned)f);
        trace_obj(buf, sizeof buf, "->", r);
        trace_emit(buf);
    }
    return r;
}

internal void *__fastcall tr_hunt(ee_resource_mgr *rm, void *edx, uintptr_t n, uintptr_t f, uintptr_t out) {
    (void)edx;
    void *r = rm_hunt(rm, n, f, out);
    if (tracing(rm)) {
        char buf[200];
        uint8_t *v = (uint8_t *)out;   // std::vector: +4 begin, +8 end
        snprintf(buf, sizeof buf, "@%06x hunt(%u,%u) -> %d targets", (unsigned)CALLER(), (unsigned)n, (unsigned)(f & 0xFF),
                 FIELD(v, 4, void *) ? (int)((FIELD(v, 8, uint8_t *) - FIELD(v, 4, uint8_t *)) / 4) : 0);
        trace_emit(buf);
    }
    return r;
}

internal void *__fastcall tr_farms(ee_resource_mgr *rm, void *edx, uintptr_t n, uintptr_t out) {
    (void)edx;
    void *r = rm_farms(rm, n, out);
    if (tracing(rm)) {
        char buf[200];
        uint8_t *v = (uint8_t *)out;
        snprintf(buf, sizeof buf, "@%06x farms(%u) -> %d targets", (unsigned)CALLER(), (unsigned)n,
                 FIELD(v, 4, void *) ? (int)((FIELD(v, 8, uint8_t *) - FIELD(v, 4, uint8_t *)) / 4) : 0);
        trace_emit(buf);
    }
    return r;
}

// Purchases by the unit manager (cm+0x44): 0x49014F(request, ?) trains units (request: [0] unit type id,
// [1] count, ...), 0x48EE84(?) is the citizen-training path. Logged with the player's stock deltas.
internal void *um_player(void *um) {
    for (int i = 0; i < 16; i++) {
        ee_player *pl = g_players[i];
        void *cm = pl ? player_get_civ_mgr(pl) : NULL;
        if (cm && cm_get_unit_mgr(cm) == um) return pl;
    }
    return NULL;
}
internal void stock_delta(char *buf, size_t cap, ee_player *pl, const int *before) {
    size_t n = strlen(buf);
    snprintf(buf + n, cap - n, " spent(f,w,s,g,i)=%d,%d,%d,%d,%d", before[2] - player_get_stock(pl, 2),
             before[3] - player_get_stock(pl, 3), before[4] - player_get_stock(pl, 4),
             before[5] - player_get_stock(pl, 5), before[6] - player_get_stock(pl, 6));
}
internal int trace_pl(ee_player *pl) {
    return pl && player_get_index(pl) == g_trace_player && (!g_trace_until_ms || game_time() < g_trace_until_ms);
}

// Called by econ.c's hook_train_units after every unit-training call.
internal void trace_train(ee_player *pl, int *req, uintptr_t r, const int *before, const int *reserve) {
    if (!trace_pl(pl)) return;
    if (!memcmp(before, player_stock_at(pl), 7 * sizeof(int)) && r == 0) return;   // nothing happened
    char buf[300];
    snprintf(buf, sizeof buf, "train_units type=%d count=%d -> %u reserve_food=%d", req[0], req[1], (unsigned)r, reserve[2]);
    stock_delta(buf, sizeof buf, pl, before);
    trace_emit(buf);
}

internal uintptr_t __fastcall tr_train_citizens(ee_unit_mgr *um, void *edx, uintptr_t a) {
    (void)edx;
    ee_player *pl = um_player(um);
    int before[7];
    if (pl) memcpy(before, player_stock_at(pl), sizeof before);
    uintptr_t r = um_train_citizens(um, a);
    if (trace_pl(pl) && memcmp(before, player_stock_at(pl), sizeof before)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x train_citizens arg=%u -> %u", (unsigned)CALLER(), (unsigned)a, (unsigned)r);
        stock_delta(buf, sizeof buf, pl, before);
        trace_emit(buf);
    }
    return r;
}

internal void trace_start(const char *ini) {
    g_trace_player = ini_int("autotest", "trace_player", -1, ini);
    if (g_trace_player < 0) return;
    g_trace_until_ms = (uint32_t)ini_int("autotest", "trace_until_min", 0, ini) * 60000u;
    int total = 1;   // the citizens trained
    int ok = patch_find_food(tr_find_food, &total) + patch_gather(tr_gather, &total) + patch_find_wood(tr_find_wood, &total) +
             patch_hunt(tr_hunt, &total) + patch_farms(tr_farms, NULL, &total) + patch_train_citizens(tr_train_citizens) +
             patch_find_mine(tr_find_mine, &total);
    if (g_log) fprintf(g_log, "# trace: player %d, %d/%d call sites hooked\n", g_trace_player, ok, total);
}

// Sync trace (autotest ini: sync_trace=<steps>): synctrace.txt next to the log. "start message: ..." when the
// game start message runs, then after each of the first <steps> simulation steps (control.c hook_phys_step) a line
// "<step> <game ms> <seed> <inputs> <stock hash> <units> <unit hash>": the step counter (EEPhysics +0x80, 15 per
// lockstep tick), the synchronised random generator's seed (EEPhysics +0x14 URandom, seed +0x18: the
// "mRandomNumberGenerator" of the game's out-of-synch dump; the exe draws from it with inlined code, not through
// LLE's exports), the user-input messages processed so far (0x919B68), a hash of the stock of players 1.. (player 0's
// first fields are not set), the units on the grid and a hash of them (owner, type, tile, hit points). Before a
// step's line, each message the step handed to the lockstep server: "  m type <n> from <return address> S" (S: the
// simulation thread). sync_dump=<from ms>-<to ms>: every unit of each step in that window ("  u ..."). Two runs of
// one game_seed diffed show the first step they part - and whether the generator, an input or the units went first.
internal FILE *g_sync;
internal unsigned g_sync_left;

// The game start message (type 0x5D, vtable 0x8285B0, built by 0x522DFC(flag, seed)) sets the simulation's
// random seed when it runs (slot 10, 0x4BEE57): EEPhysics +0x18 and world +0x54 = message +0x14. The menu's start
// sends rand() after srand(TMGetTime()) - hook_srand covers that - but the load-game and scenario starts (also a
// test run's random map, which randmap.c makes from a -loadgame start) build it with TMGetTime() itself (0x522113,
// 0x5D7A25, 0x5D8871, 0x65C955): there game_seed takes effect only here.
internal void __fastcall hook_start_exec(void *msg, void *edx) {
    (void)edx;
    if (g_auto.game_seed) start_msg_set_seed(msg, (unsigned)g_auto.game_seed);
    ee_physics *phys = game_physics(); ee_world *world = game_world();
    unsigned map_seed = world ? world_get_seed(world) : 0;
    start_exec(msg);
    EnterCriticalSection(&g_lock);
    if (g_sync) fprintf(g_sync, "start message: seed %u (generator %u, world+0x54 was %u)\n", start_msg_get_seed(msg),
                        phys ? phys_get_seed(phys) : 0, map_seed);
    LeaveCriticalSection(&g_lock);
}
internal int start_hook(void) {
    static int done;
    return done || (done = patch_start_exec(hook_start_exec));
}

// the units' part of a step's line: how many, and a sum of each one's hash (owner, type, tile, hit points) - the
// grid's order doesn't matter. sync_dump=<from ms>-<to ms>: every unit of each step in that window, "  u ..." lines
struct sync_units { unsigned n; uint32_t h; };
internal uint32_t g_sync_dump0, g_sync_dump1;
internal uint32_t sync_unit_hash(ee_unit *u) {
    ee_player *owner = unit_get_owner(u);
    ee_unit_type *td = unit_get_type_data(u);
    uint32_t v[5] = { owner ? (uint32_t)(player_get_index(owner) & 15) : 99u, td ? (uint32_t)type_get_id(td) : 0u,
                      (uint32_t)unit_get_x(u), (uint32_t)unit_get_y(u), (uint32_t)unit_get_hp(u) }, h = 2166136261u;
    for (int i = 0; i < 5; i++) h = (h ^ v[i]) * 16777619u;
    return h;
}
internal void sync_unit(ee_unit *u, void *ctx) {
    struct sync_units *s = ctx;
    s->n++;
    s->h += sync_unit_hash(u);
}
internal void sync_unit_dump(ee_unit *u, void *ctx) {
    (void)ctx;
    ee_player *owner = unit_get_owner(u);
    ee_unit_type *td = unit_get_type_data(u);
    fprintf(g_sync, "  u P%d type %d at %d,%d hp %d\n", owner ? player_get_index(owner) & 15 : -1, td ? type_get_id(td) : -1,
            unit_get_x(u), unit_get_y(u), unit_get_hp(u));
}
// the messages handed to the lockstep server (EEServer [0x91984C] vtable slot 6: (msg, 0xFF)): "  m <type> <caller>
// <S|thread>" - what goes into the next tick the server sends, and from where
internal DWORD g_sync_tid;
internal server_send_fn g_srv_send;
internal void __fastcall sync_srv_send(void *srv, void *edx, void *msg, int to) {
    (void)edx;
    uintptr_t ret = (uintptr_t)__builtin_return_address(0);
    DWORD tid = GetCurrentThreadId();
    EnterCriticalSection(&g_lock);
    if (g_sync) fprintf(g_sync, "  m type %u from %06x %s%lu\n", msg ? ((uint8_t *)msg)[8] : 0, (unsigned)ret, tid == g_sync_tid ? "S" : "thread ", tid == g_sync_tid ? 0 : tid);
    LeaveCriticalSection(&g_lock);
    g_srv_send(srv, msg, to);
}
internal void sync_hook_server(void) {
    static int done;
    ee_server *srv = game_server();
    if (done || !srv) return;
    done = 1;
    g_srv_send = patch_server_send(srv, sync_srv_send);
    if (g_log) fprintf(g_log, "# sync trace: server messages hooked (vtable %p)\n", *(void **)srv);
}
internal void sync_step(void *phys) {   // after each step (control.c hook_phys_step)
    g_sync_tid = GetCurrentThreadId();
    sync_hook_server();
    if (!g_sync || !g_sync_left) return;
    uint32_t h = 2166136261u;
    unsigned n = game_player_count();
    for (unsigned i = 1; i < n && i < 16; i++) {
        ee_player *pl = game_players()[i];
        if (!pl) continue;
        for (int k = 0; k < 7 * 4; k++) h = (h ^ ((uint8_t *)player_stock_at(pl))[k]) * 16777619u;
    }
    struct sync_units su = { 0, 0 };
    grid_each(sync_unit, &su);
    EnterCriticalSection(&g_lock);
    fprintf(g_sync, "%u %u %u %u %08x %u %08x\n", phys_get_step(phys), phys_get_time(phys),
            phys_get_seed(phys), input_msgs_done(), h, su.n, su.h);
    if (g_sync_dump0 && phys_get_time(phys) >= g_sync_dump0 && phys_get_time(phys) <= g_sync_dump1)
        grid_each(sync_unit_dump, NULL);
    if (!--g_sync_left) { fclose(g_sync); g_sync = NULL; }
    LeaveCriticalSection(&g_lock);
}

internal void sync_trace_start(const char *ini) {
    g_sync_left = (unsigned)ini_int("autotest", "sync_trace", 0, ini);
    if (!g_sync_left) return;
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s", ini);
    char *slash = strrchr(path, '\\');
    snprintf(slash ? slash + 1 : path, sizeof path - (slash ? (size_t)(slash + 1 - path) : 0), "synctrace.txt");
    g_sync = fopen(path, "w");
    char dump[40];
    GetPrivateProfileStringA("autotest", "sync_dump", "", dump, sizeof dump, ini);
    if (dump[0]) { g_sync_dump0 = (uint32_t)strtoul(dump, NULL, 10); const char *m = strchr(dump, '-'); g_sync_dump1 = m ? (uint32_t)strtoul(m + 1, NULL, 10) : g_sync_dump0; }
    int ok = g_sync && phys_step_hooked_by(hook_phys_step);   // control.c's, autotest
    if (ok) g_step_after = sync_step;
    if (g_log) fprintf(g_log, "# sync trace: %u steps into %s (%s)\n", g_sync_left, path,
                       ok && start_hook() ? "ok" : "FAILED");
}
