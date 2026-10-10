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
    g_trace_player = GetPrivateProfileIntA("autotest", "trace_player", -1, ini);
    if (g_trace_player < 0) return;
    g_trace_until_ms = (uint32_t)GetPrivateProfileIntA("autotest", "trace_until_min", 0, ini) * 60000u;
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
