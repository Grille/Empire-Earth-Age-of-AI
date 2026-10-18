// Remote control channel: a local named pipe \\.\pipe\eeaimod, one text command per connection, the reply
// is written back and the pipe closed. Game-state commands are executed on the simulation thread between
// steps (see hook_sim_receive), whether the game is running or paused. peek/poke use Read/WriteProcessMemory
// on the pipe thread so a bad address returns an error instead of crashing the game.
// Included by eeaimod.c (shares FIELD, game_time, apply_game_speed, g_auto).

#define G_PAUSED        0x00918EA2  // byte, toggled by EEAUserPauseGame::Execute (0x4064F1)
#define G_GAME_TYPE     0x00918BF4  // 0 random map, 1 scenario, 2 saved game, 3/4 other
#define G_SERVER        0x0091984C  // EEClient/EEServer (lockstep server, slot 6 takes a message); UI checks [+0x10] != 0 and vtable+0x10 before pausing
#define G_WORLD         0x0091977C  // action queue owner: QueueAction 0x66192F (thiscall, takes action ptr)
#define G_CAMERA        0x00918CD0  // EECameraControl; +0x78/+0x7C/+0x80 look-at x/y/z (tiles), +0xE0 zoom
#define G_MAP           0x009183BC  // ptr; +0x1816FC = map size in tiles
#define G_UNIT_GRID_OFS 0x181724    // map +: unit grid (see attack.c)
#define FN_GAME_NEW     0x00683718  // operator new (cdecl)
#define FN_QUEUE_ACTION 0x0066192F
#define IAT_RELEASE     0x008231AC  // imported refcount release, thiscall on the action
#define VT_PAUSE_ACTION 0x00830E94  // EEAUserPauseGame vtable (0xC bytes: vtable, refcount, byte)
#define FN_CAMACT_CTOR  0x00560179  // EEACenterCamera ctor (0x1C bytes)
#define FN_CAMACT_SET   0x00401F3E  // thiscall (tile x, tile y): set target + queue (only if tile valid)

static void watch_set(uintptr_t addr, int len);
static void cmd_threat(char *out, const char *args);
static void cmd_nullmember(char *out, const char *args);
static void cmd_attacknow(char *out, const char *args);
static void cmd_attacks(char *out, const char *args);
static void cmd_asgtrace(char *out, const char *args);
static void cmd_train(char *out, const char *args);
static void cmd_give(char *out, const char *args);
static void cmd_continent(char *out, const char *args);
static void cmd_assault(char *out, const char *args);
static void cmd_aigroup(char *out, const char *args);
static void cmd_landing(char *out, const char *args);
static void cmd_vis(char *out, const char *args);
static void cmd_intel(char *out, const char *args);
static void cmd_calloff(char *out, const char *args);
static void cmd_vistrack(char *out, const char *args);
static void cmd_terrain(char *out, const char *args);
static void cmd_board(char *out, const char *args);
static void cmd_bunker(char *out, const char *args);
static void cmd_firerate(char *out, const char *args);
static void cmd_targets(char *out, const char *args);
static void cmd_army(char *out, const char *args);
static void cmd_minecheck(char *out, const char *args);   // sites.c
static void cmd_texts(char *out, const char *args);       // texts.c
static void cmd_wave(char *out, const char *args);
static void cmd_charge(char *out, const char *args);
static void cmd_count(char *out, const char *args);
static void cmd_gridcheck(char *out, const char *args);
static void cmd_objs(char *out, const char *args);
static void cmd_sndstate(char *out, const char *args);
static void cmd_sndchan(char *out, const char *args);
static void cmd_seltest(char *out, const char *args);
static void cmd_drawcount(char *out, const char *args);
static void cmd_textlog(char *out, const char *args);
static void cmd_textstate(char *out, const char *args);
static void cmd_llm(char *out, const char *args);
static void cmd_watchtech(char *out, const char *args);
static void cmd_watchtype(char *out, const char *args);
static void cmd_techs(char *out, const char *args);
static void cmd_build(char *out, const char *args);
static void cmd_found(char *out, const char *args);
static void cmd_plan(char *out, const char *args);
static void cmd_hurt(char *out, const char *args);
static void cmd_drops(char *out, const char *args);
static void cmd_bqueue(char *out, const char *args);
static void cmd_tech(char *out, const char *args);
static void cmd_rlist(char *out, const char *args);
static void cmd_basemap(char *out, const char *args);
static void cmd_defence(char *out, const char *args);
static void cmd_tinfo(char *out, const char *args);
static void cmd_navy(char *out, const char *args);
static void cmd_chat(char *out, const char *args);
static void cmd_say(char *out, const char *args);
static void cmd_chatme(char *out, const char *args);
static void cmd_trig(char *out, const char *args);
static void cmd_stats(char *out, const char *args);
static void cmd_diplo(char *out, const char *args);
static void cmd_persona(char *out, const char *args);
static void cmd_spawn(char *out, const char *args);
static void cmd_spawnline(char *out, const char *args);
static volatile int g_turbo;           // autotest turbo: the simulation loop skips its timer wait
static void cmd_scrolltest(char *out, const char *args);
static void cmd_camtrace(char *out, const char *args);
static void cmd_aim(char *out, const char *args);     // aim.c
static void cmd_uinfo(char *out, const char *args);
static void cmd_move(char *out, const char *args);
static void aim_tick(void);
static void cmd_ddinfo(char *out, const char *args);
static void cmd_topbar(char *out, const char *args);
static void cmd_ddpoke(char *out, const char *args);
static void cmd_headless(const char *verb, const char *args, char *out);
static BOOL CALLBACK close_own_window(HWND w, LPARAM lp);
static void cmd_profile(const char *args, char *out);
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
    void **client = *(void ***)G_SERVER;
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
        REPLY("P%d @%p epoch=%d difficulty=%d cheat=%d popcap=%d (own %d, game %d) citizen_share=%.2f stock(food,wood,stone,gold,iron)=", i, pl,
            tt ? FIELD(tt, TT_EPOCH, int) : -1, FIELD(pl, PL_DIFFICULTY, int), FIELD(pl, PL_CHEAT, uint8_t),
            ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0), FIELD(pl, 0xCCC, int), *(int *)0x009183D4,
            FIELD(pl, 0xBDC, float));
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
        uint8_t *obj = type_data(type);
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
    else if (!strcmp(verb, "minecheck")) cmd_minecheck(out, args);
    else if (!strcmp(verb, "texts")) cmd_texts(out, args);
    else if (!strcmp(verb, "units")) cmd_units(out, args);
    else if (!strcmp(verb, "foodcheck")) cmd_foodcheck(out, args);
    else if (!strcmp(verb, "costs")) cmd_costs(out, args);
    else if (!strcmp(verb, "threat")) cmd_threat(out, args);
    else if (!strcmp(verb, "nullmember")) cmd_nullmember(out, args);
    else if (!strcmp(verb, "attacknow")) cmd_attacknow(out, args);
    else if (!strcmp(verb, "attacks")) cmd_attacks(out, args);
    else if (!strcmp(verb, "asgtrace")) cmd_asgtrace(out, args);
    else if (!strcmp(verb, "train")) cmd_train(out, args);
    else if (!strcmp(verb, "give")) cmd_give(out, args);
    else if (!strcmp(verb, "continent")) cmd_continent(out, args);
    else if (!strcmp(verb, "assault")) cmd_assault(out, args);
    else if (!strcmp(verb, "aigroup")) cmd_aigroup(out, args);
    else if (!strcmp(verb, "landing")) cmd_landing(out, args);
    else if (!strcmp(verb, "vis")) cmd_vis(out, args);
    else if (!strcmp(verb, "intel")) cmd_intel(out, args);
    else if (!strcmp(verb, "calloff")) cmd_calloff(out, args);
    else if (!strcmp(verb, "vistrack")) cmd_vistrack(out, args);
    else if (!strcmp(verb, "terrain")) cmd_terrain(out, args);
    else if (!strcmp(verb, "board")) cmd_board(out, args);
    else if (!strcmp(verb, "bunker")) cmd_bunker(out, args);
    else if (!strcmp(verb, "firerate")) cmd_firerate(out, args);
    else if (!strcmp(verb, "targets")) cmd_targets(out, args);
    else if (!strcmp(verb, "army")) cmd_army(out, args);
    else if (!strcmp(verb, "wave")) cmd_wave(out, args);
    else if (!strcmp(verb, "charge")) cmd_charge(out, args);
    else if (!strcmp(verb, "count")) cmd_count(out, args);
    else if (!strcmp(verb, "gridcheck")) cmd_gridcheck(out, args);
    else if (!strcmp(verb, "objs")) cmd_objs(out, args);
    else if (!strcmp(verb, "sndstate")) cmd_sndstate(out, args);
    else if (!strcmp(verb, "sndchan")) cmd_sndchan(out, args);
    else if (!strcmp(verb, "seltest")) cmd_seltest(out, args);
    else if (!strcmp(verb, "drawcount")) cmd_drawcount(out, args);
    else if (!strcmp(verb, "textlog")) cmd_textlog(out, args);
    else if (!strcmp(verb, "textstate")) cmd_textstate(out, args);
    else if (!strcmp(verb, "llm")) cmd_llm(out, args);
    else if (!strcmp(verb, "watchtech")) cmd_watchtech(out, args);
    else if (!strcmp(verb, "watchtype")) cmd_watchtype(out, args);
    else if (!strcmp(verb, "techs")) cmd_techs(out, args);
    else if (!strcmp(verb, "build")) cmd_build(out, args);
    else if (!strcmp(verb, "found")) cmd_found(out, args);
    else if (!strcmp(verb, "plan")) cmd_plan(out, args);
    else if (!strcmp(verb, "hurt")) cmd_hurt(out, args);
    else if (!strcmp(verb, "drops")) cmd_drops(out, args);
    else if (!strcmp(verb, "bqueue")) cmd_bqueue(out, args);
    else if (!strcmp(verb, "tech")) cmd_tech(out, args);
    else if (!strcmp(verb, "rlist")) cmd_rlist(out, args);
    else if (!strcmp(verb, "basemap")) cmd_basemap(out, args);
    else if (!strcmp(verb, "defence")) cmd_defence(out, args);
    else if (!strcmp(verb, "tinfo")) cmd_tinfo(out, args);
    else if (!strcmp(verb, "navy")) cmd_navy(out, args);
    else if (!strcmp(verb, "chat")) cmd_chat(out, args);
    else if (!strcmp(verb, "say")) cmd_say(out, args);
    else if (!strcmp(verb, "chatme")) cmd_chatme(out, args);
    else if (!strcmp(verb, "trig")) cmd_trig(out, args);
    else if (!strcmp(verb, "stats")) cmd_stats(out, args);
    else if (!strcmp(verb, "diplo")) cmd_diplo(out, args);
    else if (!strcmp(verb, "persona")) cmd_persona(out, args);
    else if (!strcmp(verb, "spawn")) cmd_spawn(out, args);
    else if (!strcmp(verb, "spawnline")) cmd_spawnline(out, args);
    else if (!strcmp(verb, "save")) cmd_save(out, args);
    else if (!strcmp(verb, "scrolltest")) cmd_scrolltest(out, args);
    else if (!strcmp(verb, "camtrace")) cmd_camtrace(out, args);
    else if (!strcmp(verb, "aim")) cmd_aim(out, args);
    else if (!strcmp(verb, "uinfo")) cmd_uinfo(out, args);
    else if (!strcmp(verb, "move")) cmd_move(out, args);
    else if (!strcmp(verb, "ddinfo")) cmd_ddinfo(out, args);
    else if (!strcmp(verb, "topbar")) cmd_topbar(out, args);
    else if (!strcmp(verb, "ddpoke")) cmd_ddpoke(out, args);
    else if (!strcmp(verb, "watch")) { unsigned a = 0; int l = 4; sscanf(args, "%x %d", &a, &l); watch_set(a, l); REPLY("ok: watching %08x len %d\n", a, l); }
    else if (!strcmp(verb, "speed")) {
        float s = (float)atof(args);
        if (s > 0) { g_auto.speed = s; apply_game_speed(s); }   // also keeps autotest from reverting it
        REPLY("speed=%.2f\n", *(float *)G_GAME_SPEED);
    }
    else if (!strcmp(verb, "turbo")) {
        if (*args) g_turbo = atoi(args);
        REPLY("turbo=%d\n", g_turbo);
    }
    else if (!strcmp(verb, "pause") || !strcmp(verb, "resume")) {
        int want = verb[0] == 'p';
        if (*(uint8_t *)G_PAUSED == want) REPLY("ok: already %s\n", want ? "paused" : "running");
        else REPLY(queue_pause_toggle() ? "ok: %s queued\n" : "error: game refused %s\n", verb);
    }
    else if (!strcmp(verb, "wmclose")) {
        // WM_CLOSE to the game's windows - the game's own way out, with its teardown (quit and the autotest's end
        // terminate the process and never run it)
        struct { DWORD pid; int n; } c = { GetCurrentProcessId(), 0 };
        EnumWindows(close_own_window, (LPARAM)&c);
        REPLY("ok: WM_CLOSE posted to %d windows\n", c.n);
    }
    else if (!strcmp(verb, "quit")) {
        EnterCriticalSection(&g_lock);   // the LLM worker and the pipe thread write the log too
        if (g_log) { fprintf(g_log, "# control: quit\n"); fclose(g_log); g_log = NULL; }
        LeaveCriticalSection(&g_lock);
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

static void diplo_safe_point(void);   // diplo.c: queued surrenders
static void upgrade_stragglers(void); // defence.c: towers/walls/gates finished after their upgrade
static void sound_tick(uint32_t now);  // sound.c: sound_log
static void llm_tick(void);           // llm.c: the language model's answers
static long __fastcall hook_sim_receive(void *thread, void *edx, void *a, void *b, void *c) {
    (void)edx;
    diplo_safe_point();
    upgrade_stragglers();
    aim_tick();
    sound_tick(game_time());
    llm_tick();
    if (!g_rm_seen && game_time() > 0) autotest_tick(game_time());   // games without AI (scenarios)
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

// Turbo (autotest): each pass of the simulation loop first waits for a TMTimer (thread +0x48, period +0x54 =
// 22 ms) to set an event - TSEvent::Wait(3000) through `call [0x823688]` at 0x5533C5 (0 = signalled, 0x11 =
// timeout) - then runs up to 2 steps of 1000 * speed / 44 ms game time each. So the game speed only makes the
// steps coarser; the sim thread itself idles most of the time. With turbo the wait returns at once and the
// steps follow each other as fast as the CPU allows, same step size, same results.
#define SITE_SIM_WAIT    0x005533C5
#define IAT_TS_WAIT      0x00823688
typedef long (__thiscall *EventWaitFn)(void *ev, unsigned long ms);

static long __fastcall hook_sim_wait(void *ev, void *edx, unsigned long ms) {
    (void)edx;
    // only while steps are granted: otherwise a real (short) wait, so the client, server and pathfinding threads
    // the simulation waits for get the CPU instead of a spinning loop
    uint8_t *phys = *(uint8_t **)0x00919774;
    if (g_turbo && phys && FIELD(phys, 0xB8, unsigned)) return 0;
    return (*(EventWaitFn *)IAT_TS_WAIT)(ev, g_turbo ? 1 : ms);
}

static BOOL patch_sim_wait(void) {
    uint8_t *p = (uint8_t *)SITE_SIM_WAIT;
    if (p[0] != 0xFF || p[1] != 0x15 || *(uint32_t *)(p + 2) != IAT_TS_WAIT) return FALSE;
    DWORD old;
    if (!VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    p[0] = 0xE8;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook_sim_wait - (SITE_SIM_WAIT + 5));
    p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return TRUE;
}

// Turbo, part 2: the steps themselves are granted by the lockstep server (EEServer [0x91984C], a
// NESynchronousServer in low-level engine.dll): its Send thread sends a "communications tick" every
// SendInterval ms (+0x198, 350) granting PTs-per-CT physics steps (+0x1F9, 15) - about 43 steps a second, the
// real limit. Its wait is `call [WaitForMultipleObjects]` at module+0x55DF5 (3 events, timeout = next tick).
// With turbo the wait times out as soon as the simulation has processed every tick sent (+0x10 processed in
// physics == +0x190 current tick) and used up its steps (EEPhysics [0x919774] +0xB8 == 0): the next tick follows
// at once. Ticks are thereby sent at the same points of the simulation as at normal pace, where the simulation
// always finishes a tick long before the next one - player and AI inputs land in the same ticks.
#define LLE_SEND_WAIT_RVA 0x55DF5
#define LLE_IAT_WFMO_RVA  0x940EC

static int sim_drained(void) {
    uint8_t *srv = *(uint8_t **)G_SERVER, *phys = *(uint8_t **)G_WORLD_CLOCK;
    return srv && phys && FIELD(srv, 0x10, unsigned) == FIELD(srv, 0x190, unsigned) && FIELD(phys, 0xB8, unsigned) == 0;
}

static DWORD WINAPI hook_send_wait(DWORD n, const HANDLE *h, BOOL all, DWORD ms) {
    if (!g_turbo || !game_time()) {
        // turbo off (or switched off over the pipe): the real wait, in slices so "turbo 1" takes effect at once
        DWORD t0 = GetTickCount();
        for (;;) {
            DWORD spent = GetTickCount() - t0, left = ms == INFINITE ? 50 : spent >= ms ? 0 : ms - spent;
            DWORD r = WaitForMultipleObjects(n, h, all, left < 50 ? left : 50);
            if (r != WAIT_TIMEOUT || (ms != INFINITE && left <= 50)) return r;
            if (g_turbo && game_time()) break;
        }
    }
    for (;;) {   // 1 ms waits, no spinning: the client thread must answer each tick before the next carries steps
        if (sim_drained()) return WAIT_TIMEOUT;
        DWORD r = WaitForMultipleObjects(n, h, all, 1);
        if (r != WAIT_TIMEOUT || !g_turbo) return r;
    }
}

static BOOL patch_send_wait(void) {
    uint8_t *base = (uint8_t *)GetModuleHandleA("low-level engine.dll");
    if (!base) return FALSE;
    uint8_t *p = base + LLE_SEND_WAIT_RVA;
    if (p[0] != 0xFF || p[1] != 0x15 || *(uint8_t **)(p + 2) != base + LLE_IAT_WFMO_RVA) return FALSE;
    DWORD old;
    if (!VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    p[0] = 0xE8;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook_send_wait - ((uintptr_t)p + 5));
    p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return TRUE;
}

static void run_on_game_thread(const char *cmd, char *reply, size_t cap) {
    // one still running from an earlier request (timed out after 15 s) reads g_req.cmd and writes g_req.out until it
    // ends: the next one waits for it - written over, the old command ran on with the new text and replied to it
    if (g_req.state == 2 && WaitForSingleObject(g_req.done, 15000) != WAIT_OBJECT_0) {
        ctl_log("command '%s' refused: the one before still running", cmd);
        snprintf(reply, cap, "error: the previous command is still running\n");
        return;
    }
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

static BOOL CALLBACK close_own_window(HWND w, LPARAM lp) {   // wmclose: the game's own top-level windows
    struct { DWORD pid; int n; } *c = (void *)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == c->pid && PostMessageA(w, WM_CLOSE, 0, 0)) c->n++;
    return TRUE;
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
            } else if (!strcmp(verb, "profile")) {
                char *out = reply;
                cmd_profile(cmd + n, out);
            } else if (!strcmp(verb, "vmouse") || !strcmp(verb, "vclick") || !strcmp(verb, "wclick") || !strcmp(verb, "wtype") || !strcmp(verb, "shot")) {
                char *out = reply;
                cmd_headless(verb, cmd + n, out);
            } else if (!strcmp(verb, "uipost")) {
                // uipost <hex msg> [hex p1]: post to the UI thread's queue (TSThread [0x91977C]+0x40), works in the
                // menus where no simulation runs - e.g. 413 opens the scenario editor from the main menu
                unsigned msg = 0, p1 = 0;
                sscanf(cmd + n, "%x %x", &msg, &p1);
                uint8_t *ui = *(uint8_t **)0x0091977C;
                void *thread = ui ? *(void **)(ui + 0x40) : NULL;
                typedef long (__thiscall *PostFn)(void *, unsigned long, void *, void *, int);
                PostFn post = (PostFn)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?Post@TSThread@@QAEJKPAX0_N@Z");
                if (thread && post && msg) wsprintfA(reply, "ok: posted %x (result %ld)\n", msg, post(thread, msg, (void *)(uintptr_t)p1, NULL, 1));
                else lstrcpyA(reply, "error: no UI thread\n");
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

// Autotest: when the game is decided (local player +0xA30 != 0, set when a game ends - 1 won, 2 lost; the end
// screen then pauses the lockstep server, so the simulation never reaches quit_minutes), log it and exit.
static DWORD WINAPI gameover_watch(LPVOID unused) {
    (void)unused;
    for (;;) {
        Sleep(500);
        unsigned me = *(unsigned *)G_LOCAL_PLAYER;
        uint8_t *pl = me < 16 ? ((uint8_t **)0x00918374)[me] : NULL;
        if (!pl || !game_time() || !FIELD(pl, 0xA30, int)) continue;
        if (g_log) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# autotest: game over at %.1f min:", game_time() / 60000.0);
            for (int i = 1; i < 16; i++) {
                uint8_t *p = ((uint8_t **)0x00918374)[i];
                if (p) fprintf(g_log, " P%d=%d", i, FIELD(p, 0xA30, int));
            }
            fputc(10, g_log);
            fclose(g_log);
            g_log = NULL;
            LeaveCriticalSection(&g_lock);
        }
        TerminateProcess(GetCurrentProcess(), 0);
    }
}

// The pipe (peek/poke/spawn/give... - a command on one PC of a multiplayer game desyncs it) only under autotest or
// with aimod.ini [general] control_pipe=1 (scripts/eectl.py, the MCP server scripts/ee_mcp.py on a normal game).
static void ctl_start(int pipe) {
    ctl_log("sim loop hook %s", patch_sim_loop() ? "ok" : "FAILED");
    if (g_auto.enabled) CloseHandle(CreateThread(NULL, 0, gameover_watch, NULL, 0, NULL));
    if (g_auto.enabled) {
        g_turbo = g_auto.turbo;
        int a = patch_sim_wait(), b = patch_send_wait();
        ctl_log("turbo hooks: sim wait %s, tick send %s; turbo %d", a ? "ok" : "FAILED", b ? "ok" : "FAILED", g_turbo);
    }
    g_req.done = CreateEventA(NULL, TRUE, FALSE, NULL);
    ctl_log("pipe %s", pipe ? "on" : "off (aimod.ini [general] control_pipe=0)");
    if (pipe) CloseHandle(CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL));
}
