// Assignment trace (autotest ini: trace_player=<n>, optional trace_until_min=<m>): logs the resource
// manager's site searches and gather orders for one AI player as "# T ..." lines. All hooked functions
// are __thiscall on the EECPResourceManager. Call sites found with scripts/refs.py. Included by eeaimod.c.

static int g_trace_player = -1;
static uint32_t g_trace_until_ms;

static int tracing(void *rm) {
    if (g_trace_player < 0) return 0;
    void *pl = rm_player(rm);
    if (!pl || (FIELD(pl, PL_INDEX, int) != g_trace_player)) return 0;
    return !g_trace_until_ms || game_time() < g_trace_until_ms;
}

static void trace_obj(char *buf, size_t cap, const char *tag, void *o) {
    size_t n = strlen(buf);
    if (!o) { snprintf(buf + n, cap - n, " %s=0", tag); return; }
    snprintf(buf + n, cap - n, " %s=%p(%s %d,%d)", tag, o, rtti_name(o), FIELD(o, 0x1C, int), FIELD(o, 0x20, int));
}

static void trace_emit(const char *line) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# T %u %s\n", game_time(), line);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

#define FN_FIND_FOOD  0x0048BBA8   // (char needDropsite, char nearBase, unit) -> site
#define FN_GATHER     0x0048C81C   // (unit, target, 0)
#define FN_FIND_MINE  0x0048C204   // (type, unit, reverse) -> site
#define FN_FIND_WOOD  0x0048C060   // (flag, unit) -> target
#define FN_HUNT       0x0048BA3B   // (count, flag, out list)
#define FN_FARMS      0x0048BFD0   // (count, out list)
typedef void *(__thiscall *Fn3)(void *, uintptr_t, uintptr_t, uintptr_t);
typedef void *(__thiscall *Fn2)(void *, uintptr_t, uintptr_t);

#define CALLER() ((uintptr_t)__builtin_return_address(0) - 5)

static void *__fastcall tr_find_food(void *rm, void *edx, uintptr_t a, uintptr_t b, uintptr_t u) {
    (void)edx;
    void *r = ((Fn3)FN_FIND_FOOD)(rm, a, b, u);
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x find_food(%u,%u)", (unsigned)CALLER(), (unsigned)(a & 0xFF), (unsigned)(b & 0xFF));
        trace_obj(buf, sizeof buf, "unit", (void *)u);
        trace_obj(buf, sizeof buf, "->", r);
        if (r) { size_t n = strlen(buf); snprintf(buf + n, sizeof buf - n, " gatherers=%u",
                 ((unsigned (__thiscall *)(void *, void *))FN_SITE_GATHERERS)((void *)0x918300, r)); }
        trace_emit(buf);
    }
    return r;
}

static void *__fastcall tr_gather(void *rm, void *edx, uintptr_t u, uintptr_t t, uintptr_t c) {
    (void)edx;
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x gather", (unsigned)CALLER());
        trace_obj(buf, sizeof buf, "unit", (void *)u);
        trace_obj(buf, sizeof buf, "target", (void *)t);
        trace_emit(buf);
    }
    return ((Fn3)FN_GATHER)(rm, u, t, c);
}

static void *__fastcall tr_find_mine(void *rm, void *edx, uintptr_t type, uintptr_t u, uintptr_t rev) {
    (void)edx;
    void *r = ((Fn3)FN_FIND_MINE)(rm, type, u, rev);
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x find_mine(type %u)", (unsigned)CALLER(), (unsigned)type);
        trace_obj(buf, sizeof buf, "->", r);
        trace_emit(buf);
    }
    return r;
}

static void *__fastcall tr_find_wood(void *rm, void *edx, uintptr_t f, uintptr_t u) {
    (void)edx;
    void *r = ((Fn2)FN_FIND_WOOD)(rm, f, u);
    if (tracing(rm)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x find_wood(%u)", (unsigned)CALLER(), (unsigned)f);
        trace_obj(buf, sizeof buf, "->", r);
        trace_emit(buf);
    }
    return r;
}

static void *__fastcall tr_hunt(void *rm, void *edx, uintptr_t n, uintptr_t f, uintptr_t out) {
    (void)edx;
    void *r = ((Fn3)FN_HUNT)(rm, n, f, out);
    if (tracing(rm)) {
        char buf[200];
        uint8_t *v = (uint8_t *)out;   // std::vector: +4 begin, +8 end
        snprintf(buf, sizeof buf, "@%06x hunt(%u,%u) -> %d targets", (unsigned)CALLER(), (unsigned)n, (unsigned)(f & 0xFF),
                 FIELD(v, 4, void *) ? (int)((FIELD(v, 8, uint8_t *) - FIELD(v, 4, uint8_t *)) / 4) : 0);
        trace_emit(buf);
    }
    return r;
}

static void *__fastcall tr_farms(void *rm, void *edx, uintptr_t n, uintptr_t out) {
    (void)edx;
    void *r = ((Fn2)FN_FARMS)(rm, n, out);
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
#define FN_TRAIN_UNITS    0x0049014F
#define FN_TRAIN_CITIZENS 0x0048EE84
static void *um_player(void *um) {
    for (int i = 0; i < 16; i++) {
        void *pl = g_players[i];
        void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
        if (cm && FIELD(cm, 0x44, void *) == um) return pl;
    }
    return NULL;
}
static void stock_delta(char *buf, size_t cap, void *pl, const int *before) {
    size_t n = strlen(buf);
    snprintf(buf + n, cap - n, " spent(f,w,s,g,i)=%d,%d,%d,%d,%d", before[2] - FIELD(pl, PL_STOCK + 8, int),
             before[3] - FIELD(pl, PL_STOCK + 12, int), before[4] - FIELD(pl, PL_STOCK + 16, int),
             before[5] - FIELD(pl, PL_STOCK + 20, int), before[6] - FIELD(pl, PL_STOCK + 24, int));
}
static int trace_pl(void *pl) {
    return pl && FIELD(pl, PL_INDEX, int) == g_trace_player && (!g_trace_until_ms || game_time() < g_trace_until_ms);
}

// Called by econ.c's hook_train_units after every unit-training call.
static void trace_train(void *pl, int *req, uintptr_t r, const int *before, const int *reserve) {
    if (!trace_pl(pl)) return;
    if (!memcmp(before, (uint8_t *)pl + PL_STOCK, 7 * sizeof(int)) && r == 0) return;   // nothing happened
    char buf[300];
    snprintf(buf, sizeof buf, "train_units type=%d count=%d -> %u reserve_food=%d", req[0], req[1], (unsigned)r, reserve[2]);
    stock_delta(buf, sizeof buf, pl, before);
    trace_emit(buf);
}

static uintptr_t __fastcall tr_train_citizens(void *um, void *edx, uintptr_t a) {
    (void)edx;
    void *pl = um_player(um);
    int before[7];
    if (pl) memcpy(before, (uint8_t *)pl + PL_STOCK, sizeof before);
    uintptr_t r = ((uintptr_t (__thiscall *)(void *, uintptr_t))FN_TRAIN_CITIZENS)(um, a);
    if (trace_pl(pl) && memcmp(before, (uint8_t *)pl + PL_STOCK, sizeof before)) {
        char buf[300];
        snprintf(buf, sizeof buf, "@%06x train_citizens arg=%u -> %u", (unsigned)CALLER(), (unsigned)a, (unsigned)r);
        stock_delta(buf, sizeof buf, pl, before);
        trace_emit(buf);
    }
    return r;
}

static void trace_start(const char *ini) {
    g_trace_player = ini_int("autotest", "trace_player", -1, ini);
    if (g_trace_player < 0) return;
    g_trace_until_ms = (uint32_t)ini_int("autotest", "trace_until_min", 0, ini) * 60000u;
    static const uintptr_t food[] = { 0x48ab8d, 0x48ac2b, 0x48ae14, 0x48aeb4, 0x48aefa, 0x48af99 };
    static const uintptr_t gather[] = { 0x48a2a6, 0x48a372, 0x48a9b8, 0x48ab53, 0x48abf3, 0x48acb7, 0x48adc6,
                                        0x48ae7a, 0x48af60, 0x48b0a6, 0x48b261 };
    static const uintptr_t wood[] = { 0x48a28e, 0x48aaf6 };
    static const uintptr_t hunt[] = { 0x48ace4, 0x48afc4 };
    int ok = 0, total = 0;
    for (size_t i = 0; i < sizeof food / sizeof *food; i++, total++) ok += patch_call(food[i], FN_FIND_FOOD, tr_find_food);
    for (size_t i = 0; i < sizeof gather / sizeof *gather; i++, total++) ok += patch_call(gather[i], FN_GATHER, tr_gather);
    for (size_t i = 0; i < sizeof wood / sizeof *wood; i++, total++) ok += patch_call(wood[i], FN_FIND_WOOD, tr_find_wood);
    for (size_t i = 0; i < sizeof hunt / sizeof *hunt; i++, total++) ok += patch_call(hunt[i], FN_HUNT, tr_hunt);
    total++; ok += patch_call(0x48ac54, FN_FARMS, tr_farms);
    total++; ok += patch_call(0x430740, FN_TRAIN_CITIZENS, tr_train_citizens);
    // find_mine call sites
    static const uintptr_t mine[] = { 0x48a247, 0x48a25f, 0x48a277, 0x48a952, 0x48a9f4, 0x48aa2b };
    for (size_t i = 0; i < sizeof mine / sizeof *mine; i++) { total++; ok += patch_call(mine[i], FN_FIND_MINE, tr_find_mine); }
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
static FILE *g_sync;
static unsigned g_sync_left;
typedef void (__thiscall *StepFn)(void *);

// The game start message (type 0x5D, vtable 0x8285B0, built by 0x522DFC(flag, seed)) sets the simulation's
// random seed when it runs (slot 10, 0x4BEE57): EEPhysics +0x18 and world +0x54 = message +0x14. The menu's start
// sends rand() after srand(TMGetTime()) - hook_srand covers that - but the load-game and scenario starts (also a
// test run's random map, which randmap.c makes from a -loadgame start) build it with TMGetTime() itself (0x522113,
// 0x5D7A25, 0x5D8871, 0x65C955): there game_seed takes effect only here.
#define START_VTBL_EXEC 0x008285D8
#define START_EXEC      0x004BEE57
static void __fastcall hook_start_exec(void *msg, void *edx) {
    (void)edx;
    if (g_auto.game_seed) FIELD(msg, 0x14, unsigned) = (unsigned)g_auto.game_seed;
    uint8_t *phys = *(uint8_t **)G_WORLD_CLOCK, *world = *(uint8_t **)0x0091977C;
    unsigned map_seed = world ? FIELD(world, 0x54, unsigned) : 0;
    ((StepFn)START_EXEC)(msg);
    EnterCriticalSection(&g_lock);
    if (g_sync) fprintf(g_sync, "start message: seed %u (generator %u, world+0x54 was %u)\n", FIELD(msg, 0x14, unsigned),
                        phys ? FIELD(phys, 0x18, unsigned) : 0, map_seed);
    LeaveCriticalSection(&g_lock);
}
static int start_hook(void) {
    static int done;
    return done || (done = patch_code32(START_VTBL_EXEC, START_EXEC, (uint32_t)(uintptr_t)hook_start_exec));
}

// the units' part of a step's line: how many, and a sum of each one's hash (owner, type, tile, hit points) - the
// grid's order doesn't matter. sync_dump=<from ms>-<to ms>: every unit of each step in that window, "  u ..." lines
struct sync_units { unsigned n; uint32_t h; };
static uint32_t g_sync_dump0, g_sync_dump1;
static uint32_t sync_unit_hash(uint8_t *u) {
    void *owner = FIELD(u, 0x18, void *);
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    uint32_t v[5] = { owner ? (uint32_t)(FIELD(owner, PL_INDEX, int) & 15) : 99u, td ? (uint32_t)FIELD(td, 0x4B8, int) : 0u,
                      (uint32_t)FIELD(u, 0x1C, int), (uint32_t)FIELD(u, 0x20, int), (uint32_t)FIELD(u, 0x3C, int) }, h = 2166136261u;
    for (int i = 0; i < 5; i++) h = (h ^ v[i]) * 16777619u;
    return h;
}
static void sync_unit(uint8_t *u, void *ctx) {
    struct sync_units *s = ctx;
    s->n++;
    s->h += sync_unit_hash(u);
}
static void sync_unit_dump(uint8_t *u, void *ctx) {
    (void)ctx;
    void *owner = FIELD(u, 0x18, void *);
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    fprintf(g_sync, "  u P%d type %d at %d,%d hp %d\n", owner ? FIELD(owner, PL_INDEX, int) & 15 : -1, td ? FIELD(td, 0x4B8, int) : -1,
            FIELD(u, 0x1C, int), FIELD(u, 0x20, int), FIELD(u, 0x3C, int));
}
// the messages handed to the lockstep server (EEServer [0x91984C] vtable slot 6: (msg, 0xFF)): "  m <type> <caller>
// <S|thread>" - what goes into the next tick the server sends, and from where
static DWORD g_sync_tid;
static void (__thiscall *g_srv_send)(void *, void *, int);
static void __fastcall sync_srv_send(void *srv, void *edx, uint8_t *msg, int to) {
    (void)edx;
    uintptr_t ret = (uintptr_t)__builtin_return_address(0);
    DWORD tid = GetCurrentThreadId();
    EnterCriticalSection(&g_lock);
    if (g_sync) fprintf(g_sync, "  m type %u from %06x %s%lu\n", msg ? msg[8] : 0, (unsigned)ret, tid == g_sync_tid ? "S" : "thread ", tid == g_sync_tid ? 0 : tid);
    LeaveCriticalSection(&g_lock);
    g_srv_send(srv, msg, to);
}
static void sync_hook_server(void) {
    static int done;
    void **srv = *(void ***)G_SERVER;
    if (done || !srv) return;
    done = 1;
    void **slot = &((void **)srv[0])[6];
    DWORD old;
    if (!VirtualProtect(slot, 4, PAGE_READWRITE, &old)) return;
    g_srv_send = (void (__thiscall *)(void *, void *, int))*slot;
    *slot = (void *)sync_srv_send;
    VirtualProtect(slot, 4, old, &old);
    if (g_log) fprintf(g_log, "# sync trace: server messages hooked (vtable %p)\n", srv[0]);
}
static void sync_step(void *phys) {   // after each step (control.c hook_phys_step)
    g_sync_tid = GetCurrentThreadId();
    sync_hook_server();
    if (!g_sync || !g_sync_left) return;
    uint32_t h = 2166136261u;
    unsigned n = *(unsigned *)0x009183CC;
    for (unsigned i = 1; i < n && i < 16; i++) {
        uint8_t *pl = ((uint8_t **)0x00918374)[i];
        if (!pl) continue;
        for (int k = 0; k < 7 * 4; k++) h = (h ^ pl[PL_STOCK + k]) * 16777619u;
    }
    struct sync_units su = { 0, 0 };
    grid_each(sync_unit, &su);
    EnterCriticalSection(&g_lock);
    fprintf(g_sync, "%u %u %u %u %08x %u %08x\n", FIELD(phys, 0x80, unsigned), FIELD(phys, 0xD0, unsigned),
            FIELD(phys, 0x18, unsigned), *(unsigned *)0x00919B68, h, su.n, su.h);
    if (g_sync_dump0 && FIELD(phys, 0xD0, unsigned) >= g_sync_dump0 && FIELD(phys, 0xD0, unsigned) <= g_sync_dump1)
        grid_each(sync_unit_dump, NULL);
    if (!--g_sync_left) { fclose(g_sync); g_sync = NULL; }
    LeaveCriticalSection(&g_lock);
}

static void sync_trace_start(const char *ini) {
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
    int ok = g_sync && *(uint32_t *)VTBL_PHYS_STEP == (uint32_t)(uintptr_t)hook_phys_step;   // control.c's, autotest
    if (ok) g_step_after = sync_step;
    if (g_log) fprintf(g_log, "# sync trace: %u steps into %s (%s)\n", g_sync_left, path,
                       ok && start_hook() ? "ok" : "FAILED");
}
