// FLEET MOVES (tests only; docs/testing.md `fmove`): how long a group given one move order takes to get going and to
// arrive - the measure of the local movement (units blocking each other, the 3-5 s wait before ObstacleDetected, the
// repaths) before anything there is changed. `fmove go` gives the order as the player's right-click does (the units
// selected, EEMPathTo executed: a group per kind, each handed the move); `fmove stat`, called every second or so by
// the setup script, samples each unit: moving (it covered 0.15 tiles a second since the last sample - not its speed:
// a ship pressed against another keeps the least speed, 0.1, and goes nowhere), standing short of its goal (creeping
// when its speed says it moves), or done (no move goal left); `fmove sum` the times per unit.

#define FM_MAX 512
internal struct fm_unit {
    unsigned id;                    // +0x24, found back with unit_by_id (a sunk unit isn't)
    int type;
    float moved, done, stood;       // s after the order: first moving, done (-1 not yet); s standing short of the goal
    float crept;                    // s of it with a speed (pressed against another)
    float x, y;                     // its position at the last sample
    int repaths, states;            // samples newly in RepathAroundObstacle (9002), state changes seen
    unsigned state;
} g_fm[FM_MAX];
internal int g_fm_n;
internal uint32_t g_fm_t0, g_fm_last;   // the order's game time, the last sample's (ms)
internal float g_fm_tx, g_fm_ty;
internal unsigned g_fm_trace;            // `fmove trace`: the unit logged every step (its id), 0 none
internal uint32_t g_fm_trace_last;

// each simulation step (control.c's safe point) while a unit is traced: its position, heading (degrees, 0 = +y,
// clockwise), speed, script state and the raw +0x8C (waypoints) and +0x118/+0x11C (a waypoint - engine-notes) - a
// raw dump to see how it turns
internal int g_fm_watch;                 // `fmove watch 1`: every unit of the order logged every step (# fmwatch)
internal void fm_tick(void) {
    if ((!g_fm_trace && !g_fm_watch) || !g_log) return;
    uint32_t now = game_time();
    if (now == g_fm_trace_last) return;
    g_fm_trace_last = now;
    if (g_fm_watch)   // id, position, heading, speed, state, the foundation flag (a building's: the column kept for the readers) and the blocked time
        for (int i = 0; i < g_fm_n; i++) {
            ee_unit *w = unit_by_id(g_fm[i].id);
            if (!w) continue;
            vec3 q = unit_get_pos(w), fq = unit_get_facing(w);
            fprintf(g_log, "# fmwatch %u %06x %.3f %.3f %.1f %.3f %u %u %.2f push %.3f %.3f obst %d contacts %d size %.2f %.2f %.2f class %d ff %d orders %d\n", now, g_fm[i].id & 0xFFFFFF, q.x, q.y,
                    atan2f(fq.x, fq.y) * 57.29578f, unit_get_speed(w), unit_ai_state_id(w), unit_get_foundation(w), unit_get_blocked_ms(w),
                    FIELD(w, 0x230, float), FIELD(w, 0x234, float), unit_get_blocker(w) != NULL, unit_get_contacts_count(w),
                    unit_get_size(w), unit_type_get_collider_size(unit_get_type_data(w)), unit_type_get_formation_size(unit_get_type_data(w)), unit_type_get_class_id(unit_get_type_data(w)),
                    ff_watch_state(w), unit_orders(w));
        }
    if (!g_fm_trace) return;
    ee_unit *u = unit_by_id(g_fm_trace);
    if (!u) { fprintf(g_log, "# fmtrace %u gone\n", now); g_fm_trace = 0; return; }
    vec3 p = unit_get_pos(u), f = unit_get_facing(u);
    ee_goal *g0 = unit_order_at(u, 0), *g1 = unit_order_at(u, 1);
    fprintf(g_log, "# fmtrace %u pos %.3f,%.3f head %.1f spd %.3f state %u wp %d at %.2f,%.2f orders %d (%d %d)\n", now, p.x, p.y, atan2f(f.x, f.y) * 57.29578f,
            unit_get_speed(u), unit_ai_state_id(u), FIELD(u, 0x8C, int), unit_guard_pos_at(u)->x, unit_guard_pos_at(u)->y, unit_orders(u),
            g0 ? goal_get_state(g0) : -1, g1 ? goal_get_state(g1) : -1);
    static unsigned last_state;
    unsigned st = unit_ai_state_id(u);
    if (st == 9002 && last_state != 9002)   // RepathAroundObstacle: what is near
        for (struct grid_it it = grid_it_box(unit_get_x(u), unit_get_y(u), 4, 1); grid_it_next(&it); ) {
            if (it.u == u) continue;
            vec3 q = unit_get_pos(it.u);
            if (fabsf(q.x - p.x) > 4 || fabsf(q.y - p.y) > 4) continue;
            ee_player *o = unit_get_owner(it.u);
            fprintf(g_log, "# fmtrace near %.2f,%.2f type %d P%d family %d\n", q.x, q.y, unit_type_get_id(it.td), o ? player_get_index(o) : -1, unit_type_get_class_id(it.td));
        }
    last_state = st;
}

internal void fm_sample(char *out, int quiet) {
    uint32_t now = game_time();
    float t = (float)(now - g_fm_t0) / 1000.0f, dt = (float)(now - g_fm_last) / 1000.0f;
    g_fm_last = now;
    int moving = 0, standing = 0, creeping = 0, done = 0, gone = 0, hist_n = 0;
    unsigned hist_id[16];
    int hist_c[16];
    float dist = 0, off = 0, off_most = 0;
    static float dists[FM_MAX];
    int n_off = 0, pairs = 0, n_pt = 0;
    float pt_off = 0, pt_most = 0;
    int nd = 0;
    for (int i = 0; i < g_fm_n; i++) {
        struct fm_unit *f = &g_fm[i];
        ee_unit *u = unit_by_id(f->id);
        if (!u || unit_get_hp(u) <= 0) { gone++; continue; }
        ee_goal *g = unit_get_goal(u);
        unsigned s = unit_ai_state_id(u);
        float v = unit_get_speed(u);
        vec3 p = unit_get_pos(u);
        dists[nd] = sqrtf((p.x - g_fm_tx) * (p.x - g_fm_tx) + (p.y - g_fm_ty) * (p.y - g_fm_ty));
        dist += dists[nd++];
        vec2 spot;
        if (ff_spot(u, &spot)) {   // off its formation spot
            float o = sqrtf((p.x - spot.x) * (p.x - spot.x) + (p.y - spot.y) * (p.y - spot.y));
            off += o;
            n_off++;
            if (o > off_most) off_most = o;
        }
        float po = ff_point_off(u);   // off its point in the marching formation
        if (po >= 0) { pt_off += po; n_pt++; if (po > pt_most) pt_most = po; }
        for (int j = 0; j < i; j++) {   // pairs of the order closer than 3/4 of their sizes' mean
            ee_unit *w = unit_by_id(g_fm[j].id);
            if (!w || unit_get_hp(w) <= 0) continue;
            vec3 q = unit_get_pos(w);
            float lim = (unit_get_size(u) + unit_get_size(w)) * 0.375f;
            if ((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y) < lim * lim) pairs++;
        }
        if (s != f->state) { f->states++; if (s == 9002) f->repaths++; f->state = s; }
        float step = sqrtf((p.x - f->x) * (p.x - f->x) + (p.y - f->y) * (p.y - f->y));
        f->x = p.x;
        f->y = p.y;
        if (!g || !goal_is_location(g)) { done++; if (f->done < 0) f->done = t; }
        else if (dt > 0 && step > 0.15f * dt) { moving++; if (f->moved < 0) f->moved = t; }
        else { standing++; f->stood += dt; if (v > 0.1f) { creeping++; f->crept += dt; } }
        int k = 0;
        while (k < hist_n && hist_id[k] != s) k++;
        if (k == hist_n && hist_n < 16) { hist_id[hist_n] = s; hist_c[hist_n++] = 0; }
        if (k < hist_n) hist_c[k]++;
    }
    if (quiet) return;
    int alive = g_fm_n - gone;
    for (int a = 1; a < nd; a++) {   // the spread: the distances sorted, the radius around the goal holding half, 90% and all
        float v = dists[a];
        int b = a - 1;
        while (b >= 0 && dists[b] > v) { dists[b + 1] = dists[b]; b--; }
        dists[b + 1] = v;
    }
    // the column: how far the 95th unit from the goal is behind the 5th - on the way, how strung out the group marches
    REPLY("fmove t %.1f: moving %d standing %d (creeping %d) done %d gone %d, dist %.1f (half within %.1f, 90%% %.1f, all %.1f, column %.1f), states",
          t, moving, standing, creeping, done, gone, alive ? dist / (float)alive : 0.0f, nd ? dists[nd / 2] : 0.0f, nd ? dists[nd * 9 / 10] : 0.0f,
          nd ? dists[nd - 1] : 0.0f, nd ? dists[nd * 19 / 20] - dists[nd / 20] : 0.0f);
    for (int k = 0; k < hist_n; k++) REPLY(" %u:%d", hist_id[k], hist_c[k]);
    REPLY(", off the formation spots %.2f (most %.2f), %d pairs within 3/4 of their size, %d marching off their points %.2f (most %.2f)\n", n_off ? off / (float)n_off : 0.0f,
          off_most, pairs, n_pt, n_pt ? pt_off / (float)n_pt : 0.0f, pt_most);
}

// pipe `fmove go P x y r tx ty [together [facing]]`: P's units within r tiles of x, y (no buildings) selected and sent to tx, ty
// by the right-click's message (together 1: its +0x1F set - the group's speed for all; facing in degrees: as a right-button
// drag that way, 0 as a click); `fmove stat`: a sample (above); `fmove sum`: per unit its type, first moving, done, time
// standing short of the goal, repaths, state changes - and the slowest of each
internal void cmd_fmove(char *out, const char *args) {
    char what[8] = "";
    int p = 0, x = 0, y = 0, r = 0, tx = 0, ty = 0, together = 0;
    float facing = 0;
    if (sscanf(args, "%7s %d %d %d %d %d %d %d %f", what, &p, &x, &y, &r, &tx, &ty, &together, &facing) < 1 || !game_map()) {
        REPLY("fmove go P x y r tx ty [together] | stat | sum\n");
        return;
    }
    if (!strcmp(what, "trace")) {   // fmove trace i: the order's unit i (0-based) logged every step, -1 off
        int i = p;
        g_fm_trace = i >= 0 && i < g_fm_n ? g_fm[i].id : 0;
        REPLY("fmove: tracing %06x\n", g_fm_trace & 0xFFFFFF);
        return;
    }
    if (!strcmp(what, "watch")) { g_fm_watch = p; REPLY("fmove: watch %s\n", p ? "on" : "off"); return; }   // fmove watch 1|0
    if (!strcmp(what, "stat")) { if (g_fm_n) fm_sample(out, 0); else REPLY("fmove: no order given\n"); return; }
    if (!strcmp(what, "sum")) {
        float moved = 0, done = 0, stood = 0, crept = 0;
        int n_done = 0;
        for (int i = 0; i < g_fm_n; i++) {
            struct fm_unit *f = &g_fm[i];
            REPLY("fmove %06x type %d moved %.1f done %.1f stood %.1f (crept %.1f) repaths %d states %d\n", f->id & 0xFFFFFF, f->type, f->moved, f->done, f->stood,
                  f->crept, f->repaths, f->states);
            if (f->moved > moved) moved = f->moved;
            if (f->done >= 0) { n_done++; if (f->done > done) done = f->done; }
            stood += f->stood;
            crept += f->crept;
        }
        REPLY("fmove sum: %d units, the last moving at %.1f s, %d done (the last at %.1f s), standing short of the goal %.1f s together (creeping %.1f); "
              "ship_turns so far: %u ticks kept the speed, %u steered along the route, %u waypoints dropped, %u passing a queued point; "
              "collide: %u steps, %u overlaps pushed off, the deepest %.2f tiles, %u pushes at rest; %u walks back to a formation spot; land routes: %u searches, %.1f ms, the longest %.2f ms, %u of %u corners widened (room %.2f -> %.2f tiles); formations: %u group moves laid out (%u queued), %u marching, %u following (%u moves split into blocks, %u went on in the same formation (%u legs ended where the unit stood), %u ticks steered, %u on the own route)\n", g_fm_n, moved, n_done,
              done, stood, crept, g_st_kept, g_st_steered, g_st_skipped, g_st_passing, g_uc_steps, g_uc_hits, g_uc_deepest, g_uc_rest, g_ff_walks, g_lr_routes,
              g_lr_ms, g_lr_ms_max, g_lr_widened, g_lr_corners, g_lr_corners ? g_lr_room0 / g_lr_corners : 0.0, g_lr_corners ? g_lr_room1 / g_lr_corners : 0.0, g_fo_moves, g_fo_queued_n, g_fo_marches, g_ff_marches, g_ff_split, g_ff_arcs, g_ff_finished, g_ff_steered, g_ff_own);
        return;
    }
    if (!strcmp(what, "then")) {   // fmove then P tx ty: the order's units (still there) also sent on to tx, ty, queued as with shift, the
        // facing the game's view gives a click (the way from where they stand to the point)
        static uint32_t qids[FM_MAX];
        static __attribute__((aligned(8))) uint8_t qmem[0x40];
        ee_player *qpl = p > 0 && p < 16 ? game_players()[p] : NULL;
        int n = 0;
        float cx = 0, cy = 0;
        for (int i = 0; i < g_fm_n; i++) {
            ee_unit *w = unit_by_id(g_fm[i].id);
            if (!w) continue;
            vec3 q = unit_get_pos(w);
            cx += q.x; cy += q.y;
            qids[n++] = g_fm[i].id;
        }
        if (!qpl || !n) { REPLY("fmove: no order's units of P%d left\n", p); return; }
        uint32_t *sel[3] = { NULL, qids, qids + n };
        player_select(qpl, sel);
        memset(qmem, 0, sizeof qmem);
        path_msg_make((ee_path_msg *)qmem, player_get_msg_id(qpl), (float)x + 0.5f, (float)y + 0.5f, 0, 1);
        path_msg_set_facing((ee_path_msg *)qmem, atan2f(-((float)x + 0.5f - cx / (float)n), (float)y + 0.5f - cy / (float)n));
        msg_execute((ee_msg *)qmem);
        REPLY("fmove: %d units of P%d then to %d,%d (queued)\n", n, p, x, y);
        return;
    }
    int also = !strcmp(what, "also"), add = !strcmp(what, "add");   // fmove also ...: as go, the units added to the order's (the watch, sum); add P x y r: added without an order
    if (strcmp(what, "go") && !also && !add) { REPLY("fmove go|also P x y r tx ty | add P x y r | then P tx ty | watch 1|0 | stat | sum\n"); return; }
    ee_player *pl = p > 0 && p < 16 ? game_players()[p] : NULL;
    if (!pl) { REPLY("fmove: no player %d\n", p); return; }
    static uint32_t ids[FM_MAX];
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    int first = also || add ? g_fm_n : 0;
    g_fm_n = first;
    for (struct grid_it it = grid_it_box(x, y, r, 1); g_fm_n < FM_MAX && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_type_get_class_id(it.td) == CLS_BUILDING) continue;
        if (abs(unit_get_x(u) - x) > r || abs(unit_get_y(u) - y) > r) continue;
        struct fm_unit *f = &g_fm[g_fm_n];
        memset(f, 0, sizeof *f);
        f->id = (unsigned)unit_get_serial(u);
        f->type = unit_type_get_id(it.td);
        f->moved = f->done = -1;
        f->state = unit_ai_state_id(u);
        vec3 q = unit_get_pos(u);
        f->x = q.x;
        f->y = q.y;
        ids[g_fm_n++] = f->id;
    }
    if (g_fm_n == first) { REPLY("fmove: no units of P%d within %d of %d,%d\n", p, r, x, y); return; }
    if (add) { REPLY("fmove: %d units of P%d added\n", g_fm_n - first, p); return; }
    uint32_t *sel[3] = { NULL, ids + first, ids + g_fm_n };   // a select message's selection: its ids at +4 .. +8 (only the units just added)
    player_select(pl, sel);
    memset(mem, 0, sizeof mem);
    g_fm_tx = (float)tx + 0.5f;
    g_fm_ty = (float)ty + 0.5f;
    path_msg_make((ee_path_msg *)mem, player_get_msg_id(pl), g_fm_tx, g_fm_ty, together, 0);
    path_msg_set_facing((ee_path_msg *)mem, facing * 0.01745329f);
    msg_execute((ee_msg *)mem);
    if (!also) g_fm_t0 = g_fm_last = game_time();
    int *grp = player_get_selected_group(pl);
    REPLY("fmove: %d units of P%d sent to %d,%d%s (%u in the selected group)\n", g_fm_n - first, p, tx, ty, together ? " together" : "", grp ? group_count(grp) : 0);
}
