// Leading moving targets ([general] lead_targets, default 1).
//
// A ranged attack (unit or building with a projectile class, attacker class +0x3FC) fires in FUN 0x61E11C: the aim
// point comes from 0x633413 (cdecl: attacker, &target out, float aim[3], 0) - the target's tile centre and ground
// height, or its model's hit offset for types with one (type vf21 +0x26C) - then 0x62028B / 0x623A53 create the
// projectile, turn it towards the aim point and launch it (EEUCProjectile::vf42 0x607E2B): 0x62CBB3 solves the
// ballistic arc (attacker, attacker type: speed +0xC0, gravity class +0x3FC -> +0x438; low arc unless the class or
// unit asks for the high one; out of reach 45 degrees) and the projectile flies that fixed arc (EEPMProjectile::vf1
// 0x550695, no steering). It hits only when it lands inside the target's footprint (tiles +0x1C/+0x20, size class
// +0x58C; 0x6078F9) - so a unit walking across the line of fire is missed by every slow projectile (arrows,
// catapults, cannon balls). Missiles (EEUCMissile, vtable 0x832E90) steer on their own and are left alone.
//
// The hook moves the aim point to where the target will be when the projectile lands: target velocity x flight
// time, the flight time from the game's own arc solver (horizontal distance / (speed x cos pitch)), iterated since
// the new point changes the arc. The target's velocity: its current speed (+0x238, tiles/s, written by the movement
// code 0x54A55B; 0 when it stands) along its facing (matrix +0x130, local y axis). Tile-centre aims stay tile
// centres (the tile the target will be on), hit-offset aims move by the same distance; the height follows the
// ground. (+0x118/+0x11C is not the previous position but a waypoint.)
#define CALL_AIM       0x0061E1D7
#define FN_AIM         0x00633413
#define FN_BALLISTIC   0x0062CBB3
#define VT_PROJECTILE  0x00833060   // EEUCProjectile
typedef void (__cdecl *AimFn)(uint8_t *u, uint8_t **target, float *aim, void *zero);
typedef char (__cdecl *BallisticFn)(uint8_t *u, uint8_t *type, float *from, float *to, float *pitch);

static int g_lead = 1;
static int g_lead_log;       // autotest aim_log: the first N shots logged
static int g_measure;        // tests: the measurement runs (autotest, or after "aim reset")
static unsigned g_lead_logged;

static int tile_height(int x, int y, float *h) {
    uint8_t *map = *(uint8_t **)G_MAP;
    if (!map) return 0;
    unsigned n = FIELD(map, 0x1816FC, unsigned);
    if (x < 0 || y < 0 || (unsigned)x >= n || (unsigned)y >= n) return 0;
    uint8_t *t = FIELD(map, 0x1816D4, uint8_t **)[(unsigned)y * n + (unsigned)x];
    if (!t) return 0;
    *h = FIELD(t, 0x10, float);
    return 1;
}

// Seconds the projectile flies from `from` towards `to` with the arc the launch will pick, < 0 if unknown. Out of
// the arc's reach the game launches at 45 degrees anyway and the projectile comes down short, after flying its
// longest distance v^2 / g (*reach = 0).
static float flight_time(uint8_t *u, const float *from, const float *to, int *reach) {
    uint8_t *type = FIELD(u, 0x34, uint8_t *), *cls = type ? FIELD(type, 0x60, uint8_t *) : NULL;
    uint8_t *pc = cls ? FIELD(cls, 0x3FC, uint8_t *) : NULL;
    float v = type ? FIELD(type, 0xC0, float) : 0, g = pc ? FIELD(pc, 0x438, float) : 0, pitch = 0, a[3], b[3];
    if (v <= 0.001f || g <= 0.001f) return -1;
    memcpy(a, from, sizeof a); memcpy(b, to, sizeof b);
    float dx = to[0] - from[0], dy = to[1] - from[1], d = sqrtf(dx * dx + dy * dy);
    *reach = ((BallisticFn)FN_BALLISTIC)(u, type, a, b, &pitch) != 0;
    if (!*reach) {
        pitch = 0.78539819f;
        if (d > v * v / g) d = v * v / g;
    }
    float c = cosf(pitch);
    if (c < 0.05f) return -1;
    return d / (v * c);
}

static void launch_point(uint8_t *u, float *from) {   // about the launch point (0x62028B: the model's fire point)
    from[0] = FIELD(u, 0x13C, float);
    from[1] = FIELD(u, 0x14C, float);
    from[2] = FIELD(u, 0x15C, float) + 0.5f * FIELD(FIELD(u, 0x34, uint8_t *), 0x98, float);
}

static int target_velocity(uint8_t *t, float *vx, float *vy) {
    float s = FIELD(t, 0x238, float), fx = FIELD(t, 0x134, float), fy = FIELD(t, 0x144, float);
    float l = sqrtf(fx * fx + fy * fy);
    *vx = *vy = 0;
    if (!(s > 0.01f && s < 50 && l > 0.01f)) return 0;
    *vx = s * fx / l; *vy = s * fy / l;
    return 1;
}

// The aim point for the target's velocity: the original aim moved to where the target is after the flight.
static float lead_aim(uint8_t *u, uint8_t *t, const float *aim, float T, float vx, float vy, float *out, int *reach) {
    float from[3];
    launch_point(u, from);
    int tx = FIELD(t, 0x1C, int), ty = FIELD(t, 0x20, int);
    int centred = aim[0] == (float)tx + 0.5f && aim[1] == (float)ty + 0.5f;
    float h0 = 0, h;
    int has_h0 = tile_height(tx, ty, &h0);
    memcpy(out, aim, 3 * sizeof(float));
    for (int i = 0; i < 3; i++) {
        if (T > 6) T = 6;
        float p[3], mx = vx * T, my = vy * T;
        if (centred) {
            p[0] = floorf(FIELD(t, 0x13C, float) + mx) + 0.5f;
            p[1] = floorf(FIELD(t, 0x14C, float) + my) + 0.5f;
        } else { p[0] = aim[0] + mx; p[1] = aim[1] + my; }
        p[2] = aim[2];
        if (has_h0 && tile_height((int)floorf(p[0]), (int)floorf(p[1]), &h)) p[2] += h - h0;
        float T2 = flight_time(u, from, p, reach);
        if (T2 < 0) break;
        memcpy(out, p, sizeof p);
        T = T2;
    }
    return T;
}

// Measurement (aim command): for each projectile shot at a unit, where the target really is when the projectile
// comes down, against the original aim [0] and the led aim [1] (each at its own flight time). "On tile" = the
// target stands on the aim's tile then - the condition for a hit (if the shot reaches). Split by targets that moved
// (> 0.3 tiles during the flight) or stood, and shots in or out of the arc's reach.
enum { AIM_PEND = 512 };
static struct { uint8_t *t; uint32_t due[2]; float ax[2], ay[2], x0, y0; uint8_t done, reach; } g_pend[AIM_PEND];
static unsigned g_pend_n;
static struct { unsigned n, on_tile; double err; } g_ast[2][2][2];   // [moved][in reach][aim]
static unsigned g_shots, g_shots_unit, g_led, g_lost;

static uint8_t *find_unit_near(uint8_t *want, float x, float y, int r) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    if (cs <= 0 || !cells) return NULL;
    for (int cx = ((int)x - r) / cs; cx <= ((int)x + r) / cs; cx++)
        for (int cy = ((int)y - r) / cs; cy <= ((int)y + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++)
                if (*b == want) return FIELD(want, 0x3C, int) > 0 ? want : NULL;
        }
    return NULL;
}

static void aim_tick(void) {   // the simulation thread, every pass
    if (!g_pend_n) return;
    uint32_t now = game_time();
    for (unsigned i = 0; i < AIM_PEND; i++) {
        if (!g_pend[i].t) continue;
        for (int e = 0; e < 2; e++) {
            if ((g_pend[i].done >> e & 1) || now < g_pend[i].due[e]) continue;
            g_pend[i].done |= (uint8_t)(1 << e);
            uint8_t *t = find_unit_near(g_pend[i].t, g_pend[i].x0, g_pend[i].y0, 12);
            if (!t) { if (e == 0) g_lost++; continue; }
            float x = FIELD(t, 0x13C, float), y = FIELD(t, 0x14C, float);
            float mx = x - g_pend[i].x0, my = y - g_pend[i].y0, dx = x - g_pend[i].ax[e], dy = y - g_pend[i].ay[e];
            int moved = mx * mx + my * my > 0.09f, r = g_pend[i].reach;
            g_ast[moved][r][e].n++;
            g_ast[moved][r][e].err += sqrtf(dx * dx + dy * dy);
            if ((int)floorf(x) == (int)floorf(g_pend[i].ax[e]) && (int)floorf(y) == (int)floorf(g_pend[i].ay[e]))
                g_ast[moved][r][e].on_tile++;
        }
        if (g_pend[i].done == 3) { g_pend[i].t = NULL; g_pend_n--; }
    }
}

static void aim_measure(uint8_t *t, const float *aim, float T0, const float *led, float T1, int reach) {
    if (T0 < 0 || g_pend_n >= AIM_PEND) return;
    for (unsigned i = 0; i < AIM_PEND; i++) {
        if (g_pend[i].t) continue;
        uint32_t now = game_time();
        g_pend[i].t = t; g_pend[i].done = 0; g_pend[i].reach = (uint8_t)reach;
        g_pend[i].x0 = FIELD(t, 0x13C, float); g_pend[i].y0 = FIELD(t, 0x14C, float);
        g_pend[i].due[0] = now + (uint32_t)(T0 * 1000.0f);
        g_pend[i].due[1] = now + (uint32_t)((T1 >= 0 ? T1 : T0) * 1000.0f);
        g_pend[i].ax[0] = aim[0]; g_pend[i].ay[0] = aim[1];
        g_pend[i].ax[1] = led[0]; g_pend[i].ay[1] = led[1];
        g_pend_n++;
        return;
    }
}

static void __cdecl hook_aim(uint8_t *u, uint8_t **target, float *aim, void *zero) {
    ((AimFn)FN_AIM)(u, target, aim, zero);
    uint8_t *t = *target, *cls = FIELD(u, 0x2C, uint8_t *), *pc = cls ? FIELD(cls, 0x3FC, uint8_t *) : NULL;
    g_shots++;
    if (!t || t == u || !pc || *(uintptr_t *)pc != VT_PROJECTILE) return;
    g_shots_unit++;
    float from[3], led[3], vx, vy, T1 = -1;
    int reach0 = 0, reach1 = 0;
    launch_point(u, from);
    float T0 = flight_time(u, from, aim, &reach0);
    memcpy(led, aim, sizeof led);
    if (T0 >= 0 && target_velocity(t, &vx, &vy)) T1 = lead_aim(u, t, aim, T0, vx, vy, led, &reach1);
    else reach1 = reach0;
    if (g_lead_log && g_lead_logged < (unsigned)g_lead_log && g_log) {
        g_lead_logged++;
        fprintf(g_log, "# aim: t %u shooter %p at %.2f,%.2f -> target %p at %.2f,%.2f spd %.2f fwd %.2f,%.2f | aim %.2f,%.2f,%.2f"
                " T %.2f reach %d | led %.2f,%.2f,%.2f T %.2f reach %d\n", game_time(), u, FIELD(u, 0x13C, float),
                FIELD(u, 0x14C, float), t, FIELD(t, 0x13C, float), FIELD(t, 0x14C, float), FIELD(t, 0x238, float),
                FIELD(t, 0x134, float), FIELD(t, 0x144, float), aim[0], aim[1], aim[2], T0, reach0, led[0], led[1], led[2],
                T1, reach1);
    }
    if (g_measure) aim_measure(t, aim, T0, led, T1, g_lead ? reach1 : reach0);
    if (g_lead && T1 >= 0 && memcmp(led, aim, sizeof led)) {
        memcpy(aim, led, sizeof led);
        g_led++;
    }
}

// Tests: real hits. EEUCProjectile vf55 (0x6078F9, thiscall(class, projectile)) returns 1 when the projectile is down:
// on the target's footprint (a hit) or not (it hits the ground there).
#define FN_PROJ_DOWN 0x006078F9
static unsigned g_down, g_hits;
static char __fastcall hook_proj_down(void *cls, void *edx, uint8_t *pr) {
    (void)edx;
    uint8_t *goal = FIELD(pr, 0x1F0, uint8_t *), *t = goal && goal[5] == 2 ? FIELD(goal, 0x30, uint8_t *) : NULL;
    char r = ((char (__thiscall *)(void *, uint8_t *))FN_PROJ_DOWN)(cls, pr);
    if (r && t && g_measure) {
        g_down++;
        unsigned x = FIELD(pr, 0x1C, unsigned), y = FIELD(pr, 0x20, unsigned), tx = FIELD(t, 0x1C, unsigned),
                 ty = FIELD(t, 0x20, unsigned), n = FIELD(FIELD(t, 0x2C, uint8_t *), 0x58C, unsigned);
        if (x >= tx && x < tx + n && y >= ty && y < ty + n) g_hits++;
    }
    return r;
}

static void aim_install(const char *aimod_ini, const char *autotest_ini) {
    if (autotest_ini && GetPrivateProfileIntA("autotest", "aim_hits", 1, autotest_ini))
        patch_vtable(VT_PROJECTILE, 55, FN_PROJ_DOWN, hook_proj_down);
    g_lead = GetPrivateProfileIntA("general", "lead_targets", 1, aimod_ini) != 0;
    if (autotest_ini) {
        g_lead = GetPrivateProfileIntA("autotest", "lead_targets", g_lead, autotest_ini) != 0;
        g_lead_log = GetPrivateProfileIntA("autotest", "aim_log", 0, autotest_ini);
        g_measure = 1;
    }
    // hooked also with lead_targets=0 in tests (the measurement); in play only when on
    BOOL ok = (g_lead || autotest_ini) ? patch_call(CALL_AIM, FN_AIM, hook_aim) : TRUE;
    if (g_log) fprintf(g_log, "# aim: lead_targets=%d (hook %s)\n", g_lead, ok ? "ok" : "FAILED");
}

// aim [lead 0|1] [reset]: shots and the measurement (measuring starts with "aim reset")
static void cmd_aim(char *out, const char *args) {
    int v;
    if (sscanf(args, "lead %d", &v) == 1) g_lead = v != 0;
    if (strstr(args, "reset")) {
        memset(g_ast, 0, sizeof g_ast); memset(g_pend, 0, sizeof g_pend);
        g_pend_n = g_shots = g_shots_unit = g_led = g_lost = g_down = g_hits = 0;
        g_measure = 1;
    }
    REPLY("lead=%d shots %u, projectile shots at units %u, led %u, pending %u, target gone %u; down %u, hits %u\n", g_lead, g_shots,
          g_shots_unit, g_led, g_pend_n, g_lost, g_down, g_hits);
    for (int m = 1; m >= 0; m--)
        for (int r = 1; r >= 0; r--)
            for (int e = 0; e < 2; e++)
                if (g_ast[m][r][e].n)
                    REPLY("  %-5s %-12s %-8s n %4u  on tile %5.1f%%  mean error %.2f tiles\n", m ? "moved" : "stood",
                          r ? "in reach" : "out of reach", e ? "led" : "original", g_ast[m][r][e].n,
                          100.0 * g_ast[m][r][e].on_tile / g_ast[m][r][e].n, g_ast[m][r][e].err / g_ast[m][r][e].n);
}

// uinfo x y [r]: units within r tiles: position, speed, facing, hit points, goal
static void cmd_uinfo(char *out, const char *args) {
    int x = 0, y = 0, r = 3;
    sscanf(args, "%d %d %d", &x, &y, &r);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    REPLY("t %u\n", game_time());
    for (int cx = (x - r) / cs; cx <= (x + r) / cs; cx++)
        for (int cy = (y - r) / cs; cy <= (y + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td) continue;
                int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int);
                if (abs(ux - x) > r || abs(uy - y) > r || strlen(out) > 15000) continue;
                void *owner = FIELD(u, 0x18, void *);
                REPLY("%p type %d P%d hp %d pos %.3f,%.3f,%.2f spd %.3f/%.3f fwd %.2f,%.2f goal %s\n",
                      u, FIELD(td, 0x4B8, int), owner ? FIELD(owner, PL_INDEX, int) : -1, FIELD(u, 0x3C, int),
                      FIELD(u, 0x13C, float), FIELD(u, 0x14C, float), FIELD(u, 0x15C, float), FIELD(u, 0x238, float),
                      FIELD(u, 0x21C, float), FIELD(u, 0x134, float), FIELD(u, 0x144, float),
                      rtti_name(FIELD(u, 0x1EC, void *)));
            }
        }
}

// move P x y r tx ty: units of player P within r tiles of x, y walk to tx, ty keeping their spread (tests)
static void cmd_move(char *out, const char *args) {
    int p, x, y, r, tx, ty, n = 0;
    if (sscanf(args, "%d %d %d %d %d %d", &p, &x, &y, &r, &tx, &ty) < 6) { REPLY("error: usage: move P x y r tx ty\n"); return; }
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    uint8_t *found[200];
    int nf = 0;
    for (int cx = (x - r) / cs; cx <= (x + r) / cs; cx++)
        for (int cy = (y - r) / cs; cy <= (y + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **) && nf < 200; b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                void *owner = u ? FIELD(u, 0x18, void *) : NULL;
                if (!td || !owner || FIELD(owner, PL_INDEX, int) != p || FIELD(td, 0x4B4, int) == 2) continue;
                if (abs(FIELD(u, 0x1C, int) - x) > r || abs(FIELD(u, 0x20, int) - y) > r) continue;
                found[nf++] = u;
            }
        }
    for (int i = 0; i < nf; i++)
        n += unit_goal(found[i], goal_move((float)tx + FIELD(found[i], 0x13C, float) - (float)x,
                                           (float)ty + FIELD(found[i], 0x14C, float) - (float)y), 0, 0x1F40);
    REPLY("ok: %d of %d units of P%d moving\n", n, nf, p);
}
