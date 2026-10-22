// Remote control channel: a local named pipe \\.\pipe\eeaimod, one text command per connection, the reply
// is written back and the pipe closed. Game-state commands are executed on the simulation thread between
// steps (see hook_sim_receive), whether the game is running or paused. peek/poke use Read/WriteProcessMemory
// on the pipe thread so a bad address returns an error instead of crashing the game.
// Included by eeaimod.c (shares FIELD, game_time, apply_game_speed, g_auto).


internal void watch_set(uintptr_t addr, int len);
internal void cmd_threat(char *out, const char *args);
internal void cmd_nullmember(char *out, const char *args);
internal void cmd_attacknow(char *out, const char *args);
internal void cmd_attacks(char *out, const char *args);
internal void cmd_asgtrace(char *out, const char *args);
internal void cmd_train(char *out, const char *args);
internal void cmd_give(char *out, const char *args);
internal void cmd_continent(char *out, const char *args);
internal void cmd_assault(char *out, const char *args);
internal void cmd_aigroup(char *out, const char *args);
internal void cmd_landing(char *out, const char *args);
internal void cmd_vis(char *out, const char *args);
internal void cmd_intel(char *out, const char *args);
internal void cmd_calloff(char *out, const char *args);
internal void cmd_vistrack(char *out, const char *args);
internal void cmd_terrain(char *out, const char *args);
internal void cmd_board(char *out, const char *args);
internal void cmd_bunker(char *out, const char *args);
internal void cmd_firerate(char *out, const char *args);
internal void cmd_targets(char *out, const char *args);
internal void cmd_army(char *out, const char *args);
internal void cmd_minecheck(char *out, const char *args);   // sites.c
internal void cmd_texts(char *out, const char *args);       // texts.c
internal void cmd_wave(char *out, const char *args);
internal void cmd_charge(char *out, const char *args);
internal void cmd_seaop(char *out, const char *args);   // testopp.c
internal void cmd_count(char *out, const char *args);
internal void cmd_gridcheck(char *out, const char *args);
internal void cmd_objs(char *out, const char *args);
internal void cmd_sndstate(char *out, const char *args);
internal void cmd_sndchan(char *out, const char *args);
internal void cmd_seltest(char *out, const char *args);
internal void cmd_drawcount(char *out, const char *args);
internal void cmd_textlog(char *out, const char *args);
internal void cmd_textstate(char *out, const char *args);
internal void cmd_llm(char *out, const char *args);
internal void cmd_watchtech(char *out, const char *args);
internal void cmd_watchtype(char *out, const char *args);
internal void cmd_techs(char *out, const char *args);
internal void cmd_build(char *out, const char *args);
internal void cmd_found(char *out, const char *args);
internal void cmd_plan(char *out, const char *args);
internal void cmd_hurt(char *out, const char *args);
internal void cmd_drops(char *out, const char *args);
internal void cmd_bqueue(char *out, const char *args);
internal void cmd_canbuild(char *out, const char *args);
internal void cmd_tech(char *out, const char *args);
internal void cmd_epochup(char *out, const char *args);
internal void cmd_rlist(char *out, const char *args);
internal void cmd_basemap(char *out, const char *args);
internal void cmd_defence(char *out, const char *args);
internal void cmd_tinfo(char *out, const char *args);
internal void cmd_navy(char *out, const char *args);
internal void cmd_chat(char *out, const char *args);
internal void cmd_say(char *out, const char *args);
internal void cmd_chatme(char *out, const char *args);
internal void cmd_trig(char *out, const char *args);
internal void cmd_stats(char *out, const char *args);
internal void cmd_diplo(char *out, const char *args);
internal void cmd_persona(char *out, const char *args);
internal void cmd_spawn(char *out, const char *args);
internal void cmd_spawnline(char *out, const char *args);
internal void cmd_script(char *out, const char *args);   // scripts.c: script dryrun <file> <out>
internal volatile int g_turbo;           // autotest turbo: the simulation loop skips its timer wait
internal void cmd_scrolltest(char *out, const char *args);
internal void cmd_camtrace(char *out, const char *args);
internal void cmd_aim(char *out, const char *args);     // aim.c
internal void cmd_uinfo(char *out, const char *args);
internal void cmd_move(char *out, const char *args);
internal void aim_tick(void);
internal void cmd_ddinfo(char *out, const char *args);
internal void cmd_topbar(char *out, const char *args);
internal void cmd_ddpoke(char *out, const char *args);
internal void cmd_headless(const char *verb, const char *args, char *out);
internal BOOL CALLBACK close_own_window(HWND w, LPARAM lp);
internal void cmd_profile(const char *args, char *out);
internal struct {
    char cmd[512];
    char out[REPLY_CAP];
    volatile LONG state;            // 0 idle, 1 pending, 2 running, 3 done, 4 cancelled
    HANDLE done;
} g_req;

internal PRINTF_LIKE(1, 2) void ctl_log(const char *fmt, ...) {
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



internal void queue_action(void *act) {
    world_queue_action(act);
    action_release(act);
}

// Same as clicking the pause button (EEUIEMGPauseGame::Execute 0x649F9A); the action toggles the pause.
internal int queue_pause_toggle(void) {
    void **client = game_server();
    if (!client || !((uint32_t *)client)[4]) return 0;
    if (!((char (__thiscall *)(void *))(*(void ***)client)[4])(client)) return 0;
    uint8_t *act = pause_action_alloc();
    if (!act) return 0;
    queue_action(pause_action_init(act));
    return 1;
}

// save <name>: same as the save dialog (EEUIESLESaveFile::vf1 0x5D60CD) - queues an EEASaveGame
// action (ctor 0x404FA1: thiscall(const UString &file, bool, UString folder by value), 0x30 bytes) for
// <game>\data\Saved Games\<name>.ees; the file can then be started with run_test.ps1 -Save <name>.ees.
internal void cmd_save(char *out, const char *args) {
    char name[96] = "", path[MAX_PATH];
    int n = 0;
    sscanf(args, "%95s %n", name, &n);
    if (!name[0] || strpbrk(name, "\\/:-")) { REPLY("error: usage: save <name> (no \\ / : -)\n"); return; }
    (void)n;
    // bare file name; 0x540B4A joins <startup dir> + folder + file
    wsprintfA(path, "%s%s", name, strstr(name, ".ees") ? "" : ".ees");
    struct ustring upath, udir;
    ustring_init(&upath); ustring_set(&upath, path);
    ustring_init(&udir); ustring_set(&udir, "data\\Saved Games\\");   // relative to the game folder
    uint8_t *act = save_action_alloc();
    if (!act) { ustring_free(&upath); ustring_free(&udir); REPLY("error: out of memory\n"); return; }
    // the ctor takes the folder by value and destroys its copy: ownership passes with the bytes
    save_action_init(act, &upath, 1, udir);
    ustring_free(&upath);
    queue_action(act);
    REPLY("ok: saving %s\n", path);
    ctl_log("save queued: %s", path);
}

internal void cmd_status(char *out) {
    REPLY("time_ms=%u speed=%.2f paused=%d game_type=%d\n", game_time(), *game_speed_var(),
        game_paused(), *game_type_var());
    for (int i = 0; i < 16; i++) {
        ee_player *pl = g_players[i];
        if (!pl) continue;
        void *tt = ((void *)player_get_tech_tree(pl));
        REPLY("P%d @%p epoch=%d difficulty=%d cheat=%d popcap=%d (own %d, game %d) citizen_share=%.2f stock(food,wood,stone,gold,iron)=", i, pl,
            tt ? FIELD(tt, TT_EPOCH, int) : -1, player_get_difficulty(pl), player_get_cheat(pl),
            unit_limit(pl), player_get_pop_cap(pl), pop_limit_setting(),
            player_get_citizen_share(pl));
        for (int r = 2; r < 7; r++) REPLY("%d%s", (player_stock_at(pl))[r], r < 6 ? "," : "");
        void *cm = player_get_civ_mgr(pl);
        void *rm = cm ? cm_get_resource_mgr(cm) : NULL;
        if (rm) {
            REPLY(" workers=");
            for (int r = 2; r < 7; r++) REPLY("%d%s", FIELD(rm_list_at(rm, r), 0x10, int), r < 6 ? "," : "");
            REPLY(" targets=");
            for (int r = 2; r < 7; r++) REPLY("%u%s", rm_target_at(rm)[r], r < 6 ? "," : "");
            REPLY(" demand=");
            for (int r = 2; r < 7; r++) REPLY("%d%s", rm_demand_at(rm)[r], r < 6 ? "," : "");
        }
        REPLY("\n");
    }
}

internal void cmd_camera(char *out, const char *args) {
    float *cam = (float *)(game_camera() + OF(CAMERA, 0x78));   // current look-at; +0x40 = previous (interpolated per frame)
    int x, y;
    if (sscanf(args, "%d %d", &x, &y) == 2) {
        uint32_t size = *(uint32_t *)(game_map() + OF(MAP, 0x1816FC));
        if (x < 0 || y < 0 || (uint32_t)x >= size || (uint32_t)y >= size) { REPLY("error: tile outside map (size %u)\n", size); return; }
        void *act = camera_action_alloc();
        if (!act) { REPLY("error: alloc\n"); return; }
        act = camera_action_init(act);
        camera_action_set(act, x, y);
        action_release(act);
        REPLY("ok: camera queued to tile %d %d\n", x, y);
        return;
    }
    REPLY("x=%.2f y=%.2f z=%.2f zoom=%.3f map_size=%u\n", cam[0], cam[1], cam[2],
        *(float *)(game_camera() + OF(CAMERA, 0xE0)), *(uint32_t *)(game_map() + OF(MAP, 0x1816FC)));
}

// sites <player>: resource sites the AI knows per town (EECP town 0x28 bytes at player+0xB50[i]: +0x18 active,
// +8/+0xC vector of sites; site: +0x38 resource type, +0x40 amount left, +0x1C/+0x20 position). The
// gatherer count is the world's per-site map (0x52E5F7 on 0x918300) that the AI compares against the
// cap from table 0x917BB0 entry 0x16 (0x48E817) when it picks a mine.
internal void cmd_sites(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d (AI players are registered on their first update)\n", p); return; }
    unsigned cap = table_value(0x16);
    REPLY("P%d cap(table 0x16)=%u\n", p, cap);
    void **tb = player_get_zones_begin(pl), **te = player_get_zones_end(pl);
    for (int t = 0; tb && tb + t < te; t++) {
        uint8_t *town = tb[t];
        if (!town) continue;
        REPLY(" town %d active=%d\n", t, town[OF(ZONE, 0x18)]);
        void **sb = FIELD(town, OF(ZONE, 0x8), void **), **se = FIELD(town, OF(ZONE, 0xC), void **);
        for (int s = 0; sb && sb + s < se; s++) {
            ee_unit *site = sb[s];
            if (!site) continue;
            unsigned n = site_gatherers(site);
            REPLY("  site %p type=%d amount=%d pos=%d,%d gatherers=%u\n", (void *)site, unit_get_resource(site),
                  unit_get_amount(site), unit_get_x(site), unit_get_y(site), n);
        }
    }
}

// foodcheck <player>: evaluates FindFoodSite's (0x48BBA8) conditions for the first food site of every
// zone (0x4886F2(2)), relative to the AI's base (build manager +0x12C/+0x130).
internal void cmd_foodcheck(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    if (!cm) { REPLY("error: no AI player %d\n", p); return; }
    void *bm = cm_get_build_mgr(cm);
    int base[2] = { bm_get_base_x(bm), bm_get_base_y(bm) };
    int r = player_get_dropoff_distance(pl);
    short base_region = (short)map_area(base, 2);
    unsigned cap = table_value(0x16);
    REPLY("P%d base=%d,%d region=%d radius=%d (pass1 d2<%d, pass3 d2<%d) cap=%u\n", p, base[0], base[1],
          base_region, r, r * r * 4, r * r * 36, cap);
    void **tb = player_get_zones_begin(pl), **te = player_get_zones_end(pl);
    for (int t = 0; tb && tb + t < te; t++) {
        ee_zone *zone = tb[t];
        ee_unit *s = zone ? zone_site(zone, 2) : NULL;
        if (!s) continue;
        int pos[2] = { unit_get_x(s), unit_get_y(s) };
        int dx = pos[0] - base[0], dy = pos[1] - base[1];
        REPLY(" zone %2d active=%d site %d,%d class=%#x region=%d explored=%d gatherers=%u amount=%d state=%d"
              " zone+14=%d enemy=%d d2=%d\n", t, FIELD(zone, OF(ZONE, 0x18), uint8_t), pos[0], pos[1], type_get_id(unit_get_type_data(s)),
              (short)map_area(pos, 2),
              explored_by(s, pl),
              site_gatherers(s),
              unit_get_amount(s), FIELD(unit_get_vis(s), OF(SITEOWN, 0x4), int), FIELD(zone, OF(ZONE, 0x14), int),
              bm_enemy_near(bm, pos[0], pos[1]), dx * dx + dy * dy);
    }
}

// costs <player> [unit type ...]: next epoch tech cost (tech tree +8 + epoch*0x54 record, +8 -> data,
// costs at +0x24.. food,wood,stone,gold,iron) and unit costs (object data 0x9418A0[type], same layout).
internal void print_costs(char *out, const char *tag, uint8_t *data) {
    if (!data) { REPLY("%s: no data\n", tag); return; }
    REPLY("%s: data=%p cost(f,w,s,g,i)=%d,%d,%d,%d,%d\n", tag, (void *)data, FIELD(data, OF(TECHDATA, 0x24), int), FIELD(data, OF(TECHDATA, 0x28), int),
          FIELD(data, OF(TECHDATA, 0x2C), int), FIELD(data, OF(TECHDATA, 0x30), int), FIELD(data, OF(TECHDATA, 0x34), int));
}
internal void cmd_costs(char *out, const char *args) {
    int p = 0, n = 0;
    sscanf(args, "%d %n", &p, &n);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    uint8_t *tt = ((uint8_t *)player_get_tech_tree(pl));
    int ep = FIELD(tt, TT_EPOCH, int);
    for (int e = ep + 1; e <= ep + 2 && e <= 13; e++) {
        uint8_t *rec = tt + 8 + e * 0x54;
        char tag[48];
        wsprintfA(tag, "epoch %d tech (req %d)", e, FIELD(rec, OF(TTREC, 0x40), int));
        print_costs(out, tag, FIELD(rec, OF(TTREC, 0x8), uint8_t *));
    }
    const char *a = args + n;
    int type, k;
    while (sscanf(a, "%d%n", &type, &k) == 1) {
        char tag[32];
        wsprintfA(tag, "unit %d", type);
        ee_unit_type *obj = type_data(type);
        uint8_t *tech = obj ? (uint8_t *)tech_lookup((ee_tech_tree *)tt, type_get_tech(obj)) : NULL;
        print_costs(out, tag, tech ? FIELD(tech, OF(TECH, 0x8), uint8_t *) : NULL);   // costs live in the unit's tech
        a += k;
    }
}



// units <player>: the AI's citizens per resource list (rm+0xA4 + i*0x14, deque: +0xC first offset,
// +0x10 count), grouped by current goal (unit+0x1EC) and gather target (EEUGGather+0x38).
internal void cmd_units(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    if (!rm) { REPLY("error: no AI player %d\n", p); return; }
    static const char *names[7] = { "list0", "list1", "food", "wood", "stone", "gold", "iron" };
    for (int i = 0; i < 7; i++) {
        uint8_t *dq = rm_list_at(rm, i);
        unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned);
        if (!n) continue;
        REPLY("%s: %u units\n", names[i], n);
        struct { const char *goal; void *target; int count, x, y; } g[48];
        int ng = 0;
        for (unsigned k = 0; k < n; k++) {
            ee_unit *u = deque_at(dq, first + k);
            if (!u) continue;
            ee_goal *goal = unit_get_goal(u);
            const char *gn = rtti_name(goal);
            void *tgt = (goal && !strcmp(gn, "EEUGGather@@")) ? goal_get_site(goal) : NULL;
            int j = 0;
            while (j < ng && !(g[j].goal == gn && g[j].target == tgt)) j++;
            if (j == ng) {
                if (ng == 48) continue;
                g[ng].goal = gn; g[ng].target = tgt; g[ng].count = 0;
                g[ng].x = unit_get_x(u); g[ng].y = unit_get_y(u);
                ng++;
            }
            g[j].count++;
        }
        for (int j = 0; j < ng; j++)
            REPLY("  %3d x %-24s target=%p (first unit at %d,%d)\n", g[j].count, g[j].goal, g[j].target, g[j].x, g[j].y);
    }
}

// Runs on the game thread.
internal void execute(const char *cmd, char *out) {
    char verb[32] = "";
    int n = 0;
    sscanf(cmd, "%31s %n", verb, &n);
    const char *args = cmd + n;
    char targs[512];   // a type's identifier in its place: its number (unitnames.c)
    const char *bad;
    int bad_n, ta = type_args(verb, args, targs, sizeof targs, &bad, &bad_n);
    if (ta < 0) { REPLY("error: %s: '%.*s' is no type (the identifiers: docs/units.md)\n", verb, bad_n, bad); return; }
    if (ta > 0) args = targs;
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
    else if (!strcmp(verb, "seaop")) cmd_seaop(out, args);
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
    else if (!strcmp(verb, "canbuild")) cmd_canbuild(out, args);
    else if (!strcmp(verb, "tech")) cmd_tech(out, args);
    else if (!strcmp(verb, "epochup")) cmd_epochup(out, args);
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
    else if (!strcmp(verb, "script")) cmd_script(out, args);
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
        REPLY("speed=%.2f\n", *game_speed_var());
    }
    else if (!strcmp(verb, "turbo")) {
        if (*args) g_turbo = atoi(args);
        REPLY("turbo=%d\n", g_turbo);
    }
    else if (!strcmp(verb, "pause") || !strcmp(verb, "resume")) {
        int want = verb[0] == 'p';
        if (game_paused() == want) REPLY("ok: already %s\n", want ? "paused" : "running");
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

internal void diplo_safe_point(void);   // diplo.c: queued surrenders
internal void upgrade_stragglers(void); // defence.c: towers/walls/gates finished after their upgrade
internal void sound_tick(uint32_t now);  // sound.c: sound_log
internal void llm_tick(void);           // llm.c: the language model's answers
internal long __fastcall hook_sim_receive(void *thread, void *edx, void *a, void *b, void *c) {
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
    return ts_receive(thread, a, b, c);
}

// the simulation loop's `call [TSThread::Receive]` -> hook_sim_receive
internal BOOL patch_sim_loop(void) { return patch_sim_receive(hook_sim_receive); }

// Turbo (autotest): each pass of the simulation loop first waits for a TMTimer (thread +0x48, period +0x54 =
// 22 ms) to set an event - TSEvent::Wait(3000) through `call [0x823688]` at 0x5533C5 (0 = signalled, 0x11 =
// timeout) - then runs up to 2 steps of 1000 * speed / 44 ms game time each. So the game speed only makes the
// steps coarser; the sim thread itself idles most of the time. With turbo the wait returns at once and the
// steps follow each other as fast as the CPU allows, same step size, same results.
internal long __fastcall hook_sim_wait(void *ev, void *edx, unsigned long ms) {
    (void)edx;
    // only while steps are granted: otherwise a real (short) wait, so the client, server and pathfinding threads
    // the simulation waits for get the CPU instead of a spinning loop
    uint8_t *phys = game_physics();
    if (g_turbo && phys && FIELD(phys, OF(PHYS, 0xB8), unsigned)) return 0;
    return ts_wait(ev, g_turbo ? 1 : ms);
}


// Turbo, part 2: the steps themselves are granted by the lockstep server (EEServer [0x91984C], a
// NESynchronousServer in low-level engine.dll): its Send thread sends a "communications tick" every
// SendInterval ms (+0x198, 350) granting PTs-per-CT physics steps (+0x1F9, 15) - about 43 steps a second, the
// real limit. Its wait is `call [WaitForMultipleObjects]` at module+0x55DF5 (3 events, timeout = next tick).
// With turbo the wait times out as soon as the simulation has processed every tick sent (+0x10 processed in
// physics == +0x190 current tick) and used up its steps (EEPhysics [0x919774] +0xB8 == 0): the next tick follows
// at once. Ticks are thereby sent at the same points of the simulation as at normal pace, where the simulation
// always finishes a tick long before the next one - player and AI inputs land in the same ticks. "Used up" means out
// of the last step too: +0xB8 is 0 already while the last step runs, and the AI hands its messages to the server
// during a step (the last one's too) - sent before it ends, the next tick went out without them and they landed a
// tick late, now and then (game_seed runs parted: docs/testing.md). g_in_step: hook_phys_step around each step.
// And the messages travel: the server's send (slot 6) hands each to its communications layer, and another thread puts
// it into the list the next tick takes (+0x84, its count +0x94, under the lock +0x15C; rva 0x54D3F) - a message handed
// in a tick's 14th of 15 steps missed the next tick in 1 of 8 runs (2026-10-22). So the next tick also waits until the
// list holds every message handed since the last one (g_handed, counted in hook_srv_send) - 200 ms at most, then it goes
// as before (logged).
internal volatile LONG g_in_step, g_handed, g_handed_at_tick;
internal server_send_fn g_srv_send_next;
internal void __fastcall hook_srv_send(void *srv, void *edx, void *msg, int to) {
    (void)edx;
    InterlockedIncrement(&g_handed);
    g_srv_send_next(srv, msg, to);
}
internal void hook_server_send(void) {   // the server's send slot (with trace.c's sync_srv_send: whichever comes second calls the first)
    void *srv = game_server();
    if (g_srv_send_next || !srv) return;
    g_srv_send_next = patch_server_send(srv, hook_srv_send);
}
internal void (*g_step_after)(void *phys);   // trace.c: the sync trace's line after each step
internal void __fastcall hook_phys_step(void *phys, void *edx) {
    (void)edx;
    if (!g_srv_send_next) hook_server_send();
    InterlockedExchange(&g_in_step, 1);
    phys_step(phys);
    InterlockedExchange(&g_in_step, 0);
    if (g_step_after) g_step_after(phys);
}

internal int sim_drained(void) {
    uint8_t *srv = game_server(), *phys = game_physics();
    return srv && phys && FIELD(srv, OF(SERVER, 0x10), unsigned) == FIELD(srv, OF(SERVER, 0x190), unsigned) && FIELD(phys, OF(PHYS, 0xB8), unsigned) == 0 &&
           !g_in_step;
}

// At normal pace (turbo 0) the tick is due by the clock - and under load (8 large games at a time) the simulation falls
// behind it now and then: the tick went out while the simulation was still in the last one's steps, and a message
// handed in them missed it (1 of 8 Small Islands runs parted at minute 0.8, 2026-10-22). So a due tick waits the same
// way: for the simulation to have used up every tick sent and for the messages handed since the last one. A test game
// slower than real time then runs slower instead of moving its inputs.
internal DWORD WINAPI hook_send_wait(DWORD n, const HANDLE *h, BOOL all, DWORD ms) {
    if (!g_turbo || !game_time()) {
        // turbo off (or switched off over the pipe): the real wait, in slices so "turbo 1" takes effect at once
        DWORD t0 = GetTickCount();
        for (;;) {
            DWORD spent = GetTickCount() - t0, left = ms == INFINITE ? 50 : spent >= ms ? 0 : ms - spent;
            DWORD r = WaitForMultipleObjects(n, h, all, left < 50 ? left : 50);
            if (r != WAIT_TIMEOUT) return r;
            if (ms != INFINITE && left <= 50) {
                if (!game_time()) return r;
                break;   // the tick is due: as with turbo
            }
            if (g_turbo && game_time()) break;
        }
    }
    DWORD since = 0;   // when the simulation was drained but a message not in the list yet
    for (;;) {   // 1 ms waits, no spinning: the client thread must answer each tick before the next carries steps
        if (sim_drained()) {
            uint8_t *srv = game_server();
            LONG handed = g_handed, waiting = handed - g_handed_at_tick;
            if (!g_srv_send_next || waiting <= 0 || FIELD(srv, OF(SERVER, 0x94), LONG) >= waiting) {
                g_handed_at_tick = handed;
                return WAIT_TIMEOUT;
            }
            if (!since) since = GetTickCount();
            else if (GetTickCount() - since > 200) {
                EnterCriticalSection(&g_lock);   // the server's thread: the simulation writes the log too
                if (g_log) fprintf(g_log, "# tick: at %u ms sent with %ld of %ld messages in the server's list\n", game_time(),
                                   FIELD(srv, OF(SERVER, 0x94), LONG), waiting);
                LeaveCriticalSection(&g_lock);
                g_handed_at_tick = handed;
                return WAIT_TIMEOUT;
            }
        }
        DWORD r = WaitForMultipleObjects(n, h, all, 1);
        if (r != WAIT_TIMEOUT) return r;
    }
}


internal void run_on_game_thread(const char *cmd, char *reply, size_t cap) {
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
internal void mem_command(const char *verb, const char *args, char *out) {
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

internal BOOL CALLBACK close_own_window(HWND w, LPARAM lp) {   // wmclose: the game's own top-level windows
    struct { DWORD pid; int n; } *c = (void *)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == c->pid && PostMessageA(w, WM_CLOSE, 0, 0)) c->n++;
    return TRUE;
}

internal DWORD WINAPI pipe_thread(LPVOID unused) {
    (void)unused;
    static char cmd[512], reply[sizeof g_req.out];
    for (;;) {
        char name[32] = "\\\\.\\pipe\\eeaimod";
        if (g_slot) wsprintfA(name + lstrlenA(name), "%d", g_slot);   // a test slot's own (base/paths.c, TEST SLOTS)
        HANDLE p = CreateNamedPipeA(name, PIPE_ACCESS_DUPLEX,
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
                uint8_t *ui = game_world();
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
internal DWORD WINAPI gameover_watch(LPVOID unused) {
    (void)unused;
    for (;;) {
        Sleep(500);
        unsigned me = local_player_index();
        ee_player *pl = me < 16 ? game_players()[me] : NULL;
        if (!pl || !game_time() || !player_get_result(pl)) continue;
        if (g_log) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# autotest: game over at %.1f min:", game_time() / 60000.0);
            for (int i = 1; i < 16; i++) {
                ee_player *p = game_players()[i];
                if (p) fprintf(g_log, " P%d=%d", i, player_get_result(p));
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
internal void ctl_start(int pipe) {
    ctl_log("sim loop hook %s", patch_sim_loop() ? "ok" : "FAILED");
    if (g_autotest) CloseHandle(CreateThread(NULL, 0, gameover_watch, NULL, 0, NULL));
    if (g_autotest) {
        g_turbo = g_auto.turbo;
        int a = patch_sim_wait(hook_sim_wait), b = patch_send_wait(hook_send_wait);
        int c = patch_phys_step(hook_phys_step);
        ctl_log("turbo hooks: sim wait %s, tick send %s, step %s; turbo %d", a ? "ok" : "FAILED", b ? "ok" : "FAILED",
                c ? "ok" : "FAILED", g_turbo);
    }
    g_req.done = CreateEventA(NULL, TRUE, FALSE, NULL);
    ctl_log("pipe %s", pipe ? "on" : "off (aimod.ini [general] control_pipe=0)");
    if (pipe) CloseHandle(CreateThread(NULL, 0, pipe_thread, NULL, 0, NULL));
}
