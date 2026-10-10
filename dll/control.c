// Remote control channel: a local named pipe \\.\pipe\eeaimod, one text command per connection, the reply
// is written back and the pipe closed. Game-state commands are executed on the simulation thread between
// steps (see hook_sim_receive), whether the game is running or paused. peek/poke use Read/WriteProcessMemory
// on the pipe thread so a bad address returns an error instead of crashing the game.
// Included by eeaimod.c (shares FIELD, game_time, apply_game_speed, g_auto).

#define G_PAUSED        0x00918EA2  // byte, toggled by EEAUserPauseGame::Execute (0x4064F1)
#define G_GAME_TYPE     0x00918BF4  // 0 random map, 1 scenario, 2 saved game, 3/4 other
#define G_CLIENT        0x0091984C  // EEClient/EEServer; UI checks [+0x10] != 0 and vtable+0x10 before pausing
#define G_WORLD         0x0091977C  // action queue owner: QueueAction 0x66192F (thiscall, takes action ptr)
#define G_CAMERA        0x00918CD0  // EECameraControl; +0x78/+0x7C/+0x80 look-at x/y/z (tiles), +0xE0 zoom
#define G_MAP           0x009183BC  // ptr; +0x1816FC = map size in tiles
#define FN_GAME_NEW     0x00683718  // operator new (cdecl)
#define FN_QUEUE_ACTION 0x0066192F
#define IAT_RELEASE     0x008231AC  // imported refcount release, thiscall on the action
#define VT_PAUSE_ACTION 0x00830E94  // EEAUserPauseGame vtable (0xC bytes: vtable, refcount, byte)
#define FN_CAMACT_CTOR  0x00560179  // EEACenterCamera ctor (0x1C bytes)
#define FN_CAMACT_SET   0x00401F3E  // thiscall (tile x, tile y): set target + queue (only if tile valid)

static void *g_players[16];         // EEComputerPlayer seen by the AI hooks, by player index
static void watch_set(uintptr_t addr, int len);
static void cmd_threat(char *out, const char *args);
static struct {
    char cmd[512];
    char out[16384];
    volatile LONG state;            // 0 idle, 1 pending, 2 running, 3 done, 4 cancelled
    HANDLE done;
} g_req;

static void ctl_log(const char *fmt, ...) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fputs("# control: ", g_log);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

static void ctl_note_player(void *pl) {
    g_players[(unsigned)FIELD(pl, PL_INDEX, int) & 15] = pl;
}

static int out_printf(char *out, size_t cap, const char *fmt, ...) {
    size_t len = strlen(out);
    if (len >= cap - 1) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out + len, cap - len, fmt, ap);
    va_end(ap);
    return n;
}
#define REPLY(...) out_printf(out, sizeof g_req.out, __VA_ARGS__)

typedef void (__thiscall *QueueFn)(void *world, void *action);
typedef void (__thiscall *ReleaseFn)(void *action);

static void queue_action(void *act) {
    ((QueueFn)FN_QUEUE_ACTION)(*(void **)G_WORLD, act);
    (*(ReleaseFn *)IAT_RELEASE)(act);
}

// Same as clicking the pause button (EEUIEMGPauseGame::Execute 0x649F9A); the action toggles the pause.
static int queue_pause_toggle(void) {
    void **client = *(void ***)G_CLIENT;
    if (!client || !((uint32_t *)client)[4]) return 0;
    if (!((char (__thiscall *)(void *))(*(void ***)client)[4])(client)) return 0;
    uint8_t *act = ((void *(__cdecl *)(size_t))FN_GAME_NEW)(0xC);
    if (!act) return 0;
    *(uint32_t *)act = VT_PAUSE_ACTION;
    *(uint32_t *)(act + 4) = 1;
    act[8] = 0;
    queue_action(act);
    return 1;
}

// save <name>: same as the save dialog (EEUIESLESaveFile::vf1 0x5D60CD) - queues an EEASaveGame
// action (ctor 0x404FA1: thiscall(const UString &file, bool, UString folder by value), 0x30 bytes) for
// <game>\data\Saved Games\<name>.ees; the file can then be started with run_test.ps1 -Save <name>.ees.
#define FN_SAVE_ACTION_CTOR 0x00404FA1
#define IAT_USTRING_CTOR    0x008231E4   // UString::UString()
#define IAT_USTRING_SETC    0x00823228   // UString::operator=(const char *)
#define IAT_USTRING_DTOR    0x008245BC   // UString::~UString()
struct ustring { uint8_t raw[16]; };
static void cmd_save(char *out, const char *args) {
    char name[96] = "", path[MAX_PATH];
    int n = 0;
    sscanf(args, "%95s %n", name, &n);
    if (!name[0] || strpbrk(name, "\\/:-")) { REPLY("error: usage: save <name> (no \\ / : -)\n"); return; }
    (void)n;
    // bare file name; 0x540B4A joins <startup dir> + folder + file
    wsprintfA(path, "%s%s", name, strstr(name, ".ees") ? "" : ".ees");
    void (__thiscall *uctor)(void *) = *(void **)IAT_USTRING_CTOR;
    void *(__thiscall *uset)(void *, const char *) = *(void **)IAT_USTRING_SETC;
    void (__thiscall *udtor)(void *) = *(void **)IAT_USTRING_DTOR;
    struct ustring upath, udir;
    uctor(&upath); uset(&upath, path);
    uctor(&udir); uset(&udir, "data\\Saved Games\\");   // relative to the game folder
    uint8_t *act = ((void *(__cdecl *)(size_t))FN_GAME_NEW)(0x30);
    if (!act) { udtor(&upath); udtor(&udir); REPLY("error: out of memory\n"); return; }
    // the ctor takes the folder by value and destroys its copy: ownership passes with the bytes
    ((void *(__thiscall *)(void *, void *, int, struct ustring))FN_SAVE_ACTION_CTOR)(act, &upath, 1, udir);
    udtor(&upath);
    queue_action(act);
    REPLY("ok: saving %s\n", path);
    ctl_log("save queued: %s", path);
}

static void cmd_status(char *out) {
    REPLY("time_ms=%u speed=%.2f paused=%d game_type=%d\n", game_time(), *(float *)G_GAME_SPEED,
        *(uint8_t *)G_PAUSED, *(int *)G_GAME_TYPE);
    for (int i = 0; i < 16; i++) {
        void *pl = g_players[i];
        if (!pl) continue;
        void *tt = FIELD(pl, PL_TECHTREE, void *);
        REPLY("P%d @%p epoch=%d difficulty=%d cheat=%d stock(food,wood,stone,gold,iron)=", i, pl,
            tt ? FIELD(tt, TT_EPOCH, int) : -1, FIELD(pl, PL_DIFFICULTY, int), FIELD(pl, PL_CHEAT, uint8_t));
        for (int r = 2; r < 7; r++) REPLY("%d%s", ((int *)((uint8_t *)pl + PL_STOCK))[r], r < 6 ? "," : "");
        void *cm = FIELD(pl, 0xB90, void *);
        void *rm = cm ? FIELD(cm, 0x38, void *) : NULL;
        if (rm) {
            REPLY(" workers=");
            for (int r = 2; r < 7; r++) REPLY("%d%s", FIELD(rm, RM_LISTS + r * 0x14 + 0x10, int), r < 6 ? "," : "");
            REPLY(" targets=");
            for (int r = 2; r < 7; r++) REPLY("%u%s", ((uint32_t *)((uint8_t *)rm + RM_TARGET))[r], r < 6 ? "," : "");
            REPLY(" demand=");
            for (int r = 2; r < 7; r++) REPLY("%d%s", ((int *)((uint8_t *)rm + RM_DEMAND))[r], r < 6 ? "," : "");
        }
        REPLY("\n");
    }
}

static void cmd_camera(char *out, const char *args) {
    float *cam = (float *)(G_CAMERA + 0x78);   // current look-at; +0x40 = previous (interpolated per frame)
    int x, y;
    if (sscanf(args, "%d %d", &x, &y) == 2) {
        uint32_t size = *(uint32_t *)(*(uint8_t **)G_MAP + 0x1816FC);
        if (x < 0 || y < 0 || (uint32_t)x >= size || (uint32_t)y >= size) { REPLY("error: tile outside map (size %u)\n", size); return; }
        void *act = ((void *(__cdecl *)(size_t))FN_GAME_NEW)(0x1C);
        if (!act) { REPLY("error: alloc\n"); return; }
        act = ((void *(__thiscall *)(void *))FN_CAMACT_CTOR)(act);
        ((void (__thiscall *)(void *, int, int))FN_CAMACT_SET)(act, x, y);
        (*(ReleaseFn *)IAT_RELEASE)(act);
        REPLY("ok: camera queued to tile %d %d\n", x, y);
        return;
    }
    REPLY("x=%.2f y=%.2f z=%.2f zoom=%.3f map_size=%u\n", cam[0], cam[1], cam[2],
        *(float *)(G_CAMERA + 0xE0), *(uint32_t *)(*(uint8_t **)G_MAP + 0x1816FC));
}

// sites <player>: resource sites the AI knows per town (EECP town 0x28 bytes at player+0xB50[i]: +0x18 active,
// +8/+0xC vector of sites; site: +0x38 resource type, +0x40 amount left, +0x1C/+0x20 position). The
// gatherer count is the world's per-site map (0x52E5F7 on 0x918300) that the AI compares against the
// cap from table 0x917BB0 entry 0x16 (0x48E817) when it picks a mine.
#define FN_SITE_GATHERERS 0x0052E5F7
#define FN_TABLE_VALUE    0x0048E817
static void cmd_sites(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d (AI players are registered on their first update)\n", p); return; }
    unsigned cap = ((unsigned (__thiscall *)(void *, int))FN_TABLE_VALUE)((void *)0x917BB0, 0x16);
    REPLY("P%d cap(table 0x16)=%u\n", p, cap);
    void **tb = FIELD(pl, 0xB50, void **), **te = FIELD(pl, 0xB54, void **);
    for (int t = 0; tb && tb + t < te; t++) {
        uint8_t *town = tb[t];
        if (!town) continue;
        REPLY(" town %d active=%d\n", t, town[0x18]);
        void **sb = FIELD(town, 8, void **), **se = FIELD(town, 0xC, void **);
        for (int s = 0; sb && sb + s < se; s++) {
            uint8_t *site = sb[s];
            if (!site) continue;
            unsigned n = ((unsigned (__thiscall *)(void *, void *))FN_SITE_GATHERERS)((void *)0x918300, site);
            REPLY("  site %p type=%d amount=%d pos=%d,%d gatherers=%u\n", (void *)site, FIELD(site, 0x38, int),
                  FIELD(site, 0x40, int), FIELD(site, 0x1C, int), FIELD(site, 0x20, int), n);
        }
    }
}

// foodcheck <player>: evaluates FindFoodSite's (0x48BBA8) conditions for the first food site of every
// zone (0x4886F2(2)), relative to the AI's base (build manager +0x12C/+0x130).
#define FN_ZONE_SITE   0x004886F2   // thiscall(zone, type) -> first site of that type with amount > 0
#define FN_REGION      0x00425F9F   // thiscall(0x917BC8, int pos[2], 2) -> short region id
#define FN_ENEMY_NEAR  0x004660B9   // thiscall(build manager, x, y) -> bool
#define FN_EXPLORED    0x00404A8A   // thiscall(site+0x14 object, player) -> bool
static void cmd_foodcheck(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
    if (!cm) { REPLY("error: no AI player %d\n", p); return; }
    void *bm = FIELD(cm, 0x3C, void *);
    int base[2] = { FIELD(bm, 0x12C, int), FIELD(bm, 0x130, int) };
    int r = FIELD(pl, 0xE10, int);
    short (__thiscall *region)(void *, int *, int) = (void *)FN_REGION;
    short base_region = region((void *)0x917BC8, base, 2);
    unsigned cap = ((unsigned (__thiscall *)(void *, int))FN_TABLE_VALUE)((void *)0x917BB0, 0x16);
    REPLY("P%d base=%d,%d region=%d radius=%d (pass1 d2<%d, pass3 d2<%d) cap=%u\n", p, base[0], base[1],
          base_region, r, r * r * 4, r * r * 36, cap);
    void **tb = FIELD(pl, 0xB50, void **), **te = FIELD(pl, 0xB54, void **);
    for (int t = 0; tb && tb + t < te; t++) {
        uint8_t *zone = tb[t];
        uint8_t *s = zone ? ((uint8_t *(__thiscall *)(void *, int))FN_ZONE_SITE)(zone, 2) : NULL;
        if (!s) continue;
        int pos[2] = { FIELD(s, 0x1C, int), FIELD(s, 0x20, int) };
        int dx = pos[0] - base[0], dy = pos[1] - base[1];
        REPLY(" zone %2d active=%d site %d,%d class=%#x region=%d explored=%d gatherers=%u amount=%d state=%d"
              " zone+14=%d enemy=%d d2=%d\n", t, zone[0x18], pos[0], pos[1], FIELD(FIELD(s, 0x2C, uint8_t *), 0x4B8, int),
              region((void *)0x917BC8, pos, 2),
              ((char (__thiscall *)(void *, void *))FN_EXPLORED)(FIELD(s, 0x14, void *), pl),
              ((unsigned (__thiscall *)(void *, void *))FN_SITE_GATHERERS)((void *)0x918300, s),
              FIELD(s, 0x40, int), FIELD(FIELD(s, 0x14, uint8_t *), 4, int), FIELD(zone, 0x14, int),
              ((char (__thiscall *)(void *, int, int))FN_ENEMY_NEAR)(bm, pos[0], pos[1]), dx * dx + dy * dy);
    }
}

// costs <player> [unit type ...]: next epoch tech cost (tech tree +8 + epoch*0x54 record, +8 -> data,
// costs at +0x24.. food,wood,stone,gold,iron) and unit costs (object data 0x9418A0[type], same layout).
#define G_OBJECT_DATA 0x009418A0
static void print_costs(char *out, const char *tag, uint8_t *data) {
    if (!data) { REPLY("%s: no data\n", tag); return; }
    REPLY("%s: data=%p cost(f,w,s,g,i)=%d,%d,%d,%d,%d\n", tag, (void *)data, FIELD(data, 0x24, int), FIELD(data, 0x28, int),
          FIELD(data, 0x2C, int), FIELD(data, 0x30, int), FIELD(data, 0x34, int));
}
static void cmd_costs(char *out, const char *args) {
    int p = 0, n = 0;
    sscanf(args, "%d %n", &p, &n);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    int ep = FIELD(tt, TT_EPOCH, int);
    for (int e = ep + 1; e <= ep + 2 && e <= 13; e++) {
        uint8_t *rec = tt + 8 + e * 0x54;
        char tag[48];
        wsprintfA(tag, "epoch %d tech (req %d)", e, FIELD(rec, 0x40, int));
        print_costs(out, tag, FIELD(rec, 8, uint8_t *));
    }
    const char *a = args + n;
    int type, k;
    while (sscanf(a, "%d%n", &type, &k) == 1) {
        char tag[32];
        wsprintfA(tag, "unit %d", type);
        uint8_t *obj = (type >= 0 && type < 2048) ? ((uint8_t **)G_OBJECT_DATA)[type] : NULL;
        uint8_t *tech = obj ? ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, FIELD(obj, 0x4E0, int)) : NULL;
        print_costs(out, tag, tech ? FIELD(tech, 8, uint8_t *) : NULL);   // costs live in the unit's tech
        a += k;
    }
}

// RTTI class name of a polymorphic object (MSVC: vtable[-1] -> complete object locator, +0xC type
// descriptor, name at +8 like ".?AVEEUGGather@@").
static const char *rtti_name(void *obj) {
    if (!obj) return "none";
    uint8_t *col = ((uint8_t **)*(void **)obj)[-1];
    const char *n = *(const char **)(col + 0xC) + 8;
    return n[0] == '.' && n[1] == '?' && n[2] == 'A' ? n + 4 : n;
}

// Element i of an MSVC std::deque<T*> (block of 1024 pointers; container +4 map, +8 map size), as the
// game's iterator deref 0x5F1BC6 computes it.
static void *deque_at(uint8_t *dq, unsigned off) {
    unsigned blk = off >> 10, mapsz = FIELD(dq, 8, unsigned);
    if (blk >= mapsz) blk -= mapsz;
    return FIELD(dq, 4, void ***)[blk][off & 1023];
}

// units <player>: the AI's citizens per resource list (rm+0xA4 + i*0x14, deque: +0xC first offset,
// +0x10 count), grouped by current goal (unit+0x1EC) and gather target (EEUGGather+0x38).
static void cmd_units(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    if (!rm) { REPLY("error: no AI player %d\n", p); return; }
    static const char *names[7] = { "list0", "list1", "food", "wood", "stone", "gold", "iron" };
    for (int i = 0; i < 7; i++) {
        uint8_t *dq = rm + RM_LISTS + i * 0x14;
        unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned);
        if (!n) continue;
        REPLY("%s: %u units\n", names[i], n);
        struct { const char *goal; void *target; int count, x, y; } g[48];
        int ng = 0;
        for (unsigned k = 0; k < n; k++) {
            uint8_t *u = deque_at(dq, first + k);
            if (!u) continue;
            void *goal = FIELD(u, 0x1EC, void *);
            const char *gn = rtti_name(goal);
            void *tgt = (goal && !strcmp(gn, "EEUGGather@@")) ? FIELD(goal, 0x38, void *) : NULL;
            int j = 0;
            while (j < ng && !(g[j].goal == gn && g[j].target == tgt)) j++;
            if (j == ng) {
                if (ng == 48) continue;
                g[ng].goal = gn; g[ng].target = tgt; g[ng].count = 0;
                g[ng].x = FIELD(u, 0x1C, int); g[ng].y = FIELD(u, 0x20, int);
                ng++;
            }
            g[j].count++;
        }
        for (int j = 0; j < ng; j++)
            REPLY("  %3d x %-24s target=%p (first unit at %d,%d)\n", g[j].count, g[j].goal, g[j].target, g[j].x, g[j].y);
    }
}

// Runs on the game thread.
static void execute(const char *cmd, char *out) {
    char verb[32] = "";
    int n = 0;
    sscanf(cmd, "%31s %n", verb, &n);
    const char *args = cmd + n;
    if (!strcmp(verb, "ping")) REPLY("pong\n");
    else if (!strcmp(verb, "status")) cmd_status(out);
    else if (!strcmp(verb, "camera")) cmd_camera(out, args);
    else if (!strcmp(verb, "sites")) cmd_sites(out, args);
    else if (!strcmp(verb, "units")) cmd_units(out, args);
    else if (!strcmp(verb, "foodcheck")) cmd_foodcheck(out, args);
    else if (!strcmp(verb, "costs")) cmd_costs(out, args);
    else if (!strcmp(verb, "threat")) cmd_threat(out, args);
    else if (!strcmp(verb, "save")) cmd_save(out, args);
    else if (!strcmp(verb, "watch")) { unsigned a = 0; int l = 4; sscanf(args, "%x %d", &a, &l); watch_set(a, l); REPLY("ok: watching %08x len %d\n", a, l); }
    else if (!strcmp(verb, "speed")) {
        float s = (float)atof(args);
        if (s > 0) { g_auto.speed = s; apply_game_speed(s); }   // also keeps autotest from reverting it
        REPLY("speed=%.2f\n", *(float *)G_GAME_SPEED);
    }
    else if (!strcmp(verb, "pause") || !strcmp(verb, "resume")) {
        int want = verb[0] == 'p';
        if (*(uint8_t *)G_PAUSED == want) REPLY("ok: already %s\n", want ? "paused" : "running");
        else REPLY(queue_pause_toggle() ? "ok: %s queued\n" : "error: game refused %s\n", verb);
    }
    else if (!strcmp(verb, "quit")) {
        if (g_log) { fprintf(g_log, "# control: quit\n"); fclose(g_log); g_log = NULL; }
        TerminateProcess(GetCurrentProcess(), 0);
    }
    else REPLY("error: unknown command '%s' (ping status camera [x y] speed [s] pause resume quit peek poke)\n", verb);
}

// RTSPhysics::Main (0x5532FB) is the simulation thread's loop; each pass, running or paused, starts with
// TSThread::Receive (imported, thiscall, 3 args) through `call [0x8236B0]` at 0x553348. Pending control
// commands run right before it: between simulation steps, on the thread that owns the game state.
#define SITE_SIM_RECEIVE 0x00553348
#define IAT_TS_RECEIVE   0x008236B0
typedef long (__thiscall *ReceiveFn)(void *thread, void *a, void *b, void *c);

static long __fastcall hook_sim_receive(void *thread, void *edx, void *a, void *b, void *c) {
    (void)edx;
    if (g_req.state == 1 && InterlockedCompareExchange(&g_req.state, 2, 1) == 1) {
        execute(g_req.cmd, g_req.out);
        g_req.state = 3;
        SetEvent(g_req.done);
    }
    return (*(ReceiveFn *)IAT_TS_RECEIVE)(thread, a, b, c);
}

// Replace the 6-byte `call [IAT_TS_RECEIVE]` with `call hook_sim_receive; nop`.
static BOOL patch_sim_loop(void) {
    uint8_t *p = (uint8_t *)SITE_SIM_RECEIVE;
    if (p[0] != 0xFF || p[1] != 0x15 || *(uint32_t *)(p + 2) != IAT_TS_RECEIVE) return FALSE;
    DWORD old;
    if (!VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    p[0] = 0xE8;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook_sim_receive - (SITE_SIM_RECEIVE + 5));
    p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return TRUE;
}

static void run_on_game_thread(const char *cmd, char *reply, size_t cap) {
    lstrcpynA(g_req.cmd, cmd, sizeof g_req.cmd);
    g_req.out[0] = 0;
    ResetEvent(g_req.done);
    g_req.state = 1;
    if (WaitForSingleObject(g_req.done, 5000) != WAIT_OBJECT_0) {
        if (InterlockedCompareExchange(&g_req.state, 4, 1) == 1) {
            snprintf(reply, cap, "error: simulation loop not running (menu or loading screen?)\n");
            return;
        }
        if (WaitForSingleObject(g_req.done, 10000) != WAIT_OBJECT_0) {   // already running: give it a bit more
            ctl_log("command '%s' still running after 15 s", cmd);
            snprintf(reply, cap, "error: command still running after 15 s\n");
            return;
        }
    }
    lstrcpynA(reply, g_req.out, (int)cap);
}

// peek <hexaddr> <len>      -> hex dump (16 bytes per line)
// poke <hexaddr> <hexbytes> -> writes bytes
static void mem_command(const char *verb, const char *args, char *out) {
    unsigned addr = 0;
    int n = 0;
    if (sscanf(args, "%x %n", &addr, &n) < 1) { REPLY("error: usage %s <hexaddr> ...\n", verb); return; }
    HANDLE self = GetCurrentProcess();
    if (verb[1] == 'e') {
        unsigned len = (unsigned)atoi(args + n);
        if (len == 0) len = 64;
        if (len > 4096) len = 4096;
        uint8_t buf[4096];
        SIZE_T got = 0;
        if (!ReadProcessMemory(self, (void *)(uintptr_t)addr, buf, len, &got) || got != len) { REPLY("error: unreadable\n"); return; }
        for (unsigned i = 0; i < len; i += 16) {
            REPLY("%08x:", addr + i);
            for (unsigned j = i; j < i + 16 && j < len; j++) REPLY(" %02x", buf[j]);
            REPLY("\n");
        }
    } else {
        uint8_t buf[256];
        unsigned len = 0, b;
        const char *p = args + n;
        int k;
        while (len < sizeof buf && sscanf(p, "%2x%n", &b, &k) == 1) { buf[len++] = (uint8_t)b; p += k; while (*p == ' ') p++; }
        SIZE_T put = 0;
        if (!len || !WriteProcessMemory(self, (void *)(uintptr_t)addr, buf, len, &put) || put != len) { REPLY("error: write failed\n"); return; }
        REPLY("ok: wrote %u bytes at %08x\n", len, addr);
    }
}

static DWORD WINAPI pipe_thread(LPVOID unused) {
    (void)unused;
    static char cmd[512], reply[sizeof g_req.out];
    for (;;) {
        HANDLE p = CreateNamedPipeA("\\\\.\\pipe\\eeaimod", PIPE_ACCESS_DUPLEX,
                                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                    1, sizeof reply, sizeof cmd, 0, NULL);
        if (p == INVALID_HANDLE_VALUE) return 1;
        if (ConnectNamedPipe(p, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            DWORD got = 0, total = 0;
            while (total < sizeof cmd - 1 && ReadFile(p, cmd + total, sizeof cmd - 1 - total, &got, NULL) && got) {
                total += got;
                if (memchr(cmd, '\n', total)) break;
            }
            cmd[total] = 0;
            ctl_log("request '%.*s'", (int)strcspn(cmd, "\r\n"), cmd);
            char *nl = strpbrk(cmd, "\r\n");
            if (nl) *nl = 0;
            reply[0] = 0;
            char verb[8] = "";
            int n = 0;
            sscanf(cmd, "%7s %n", verb, &n);
            if (!strcmp(verb, "peek") || !strcmp(verb, "poke")) {
                char *out = reply;
                mem_command(verb, cmd + n, out);
            } else {
                run_on_game_thread(cmd, reply, sizeof reply);
            }
            DWORD put;
            WriteFile(p, reply, (DWORD)strlen(reply), &put, NULL);
            FlushFileBuffers(p);
            ctl_log("replied %u bytes", (unsigned)strlen(reply));
            DisconnectNamedPipe(p);
        }
        CloseHandle(p);
    }
}

static void ctl_start(void) {
    ctl_log("sim loop hook %s", patch_sim_loop() ? "ok" : "FAILED");
    g_req.done = CreateEventA(NULL, TRUE, FALSE, NULL);
    CloseHandle(CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL));
}
