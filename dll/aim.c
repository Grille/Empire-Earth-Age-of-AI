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
// Multiplayer: everything here changes the simulation, so every PC must compute the same bits (lockstep). Only
// + - * / and sqrt (SSE, exact) and the game's own math DLL - the C runtime's cosf comes with Windows and may differ
// between versions. UMCos (cdecl float(float)) through the game's import table.
#define GAME_COS(a) ((*(float (__cdecl **)(float))0x00823304)(a))

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
    float c;
    if (!*reach) {
        c = 0.70710678f;   // 45 degrees
        if (d > v * v / g) d = v * v / g;
    } else c = GAME_COS(pitch);
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
    for (struct grid_it it = grid_it_box((int)x, (int)y, r, 0); grid_it_next(&it); )
        if (it.u == want) return FIELD(want, 0x3C, int) > 0 ? want : NULL;
    return NULL;
}

static void missile_tick(uint32_t now);
static void aim_tick(void) {   // the simulation thread, every pass
    uint32_t now = game_time();
    missile_tick(now);
    if (!g_measure) return;
    if (!g_pend_n) return;
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

static uint8_t *g_retarget;            // spread_missiles: set while 0x61E11C fires at another target
static void __cdecl hook_aim(uint8_t *u, uint8_t **target, float *aim, void *zero) {
    ((AimFn)FN_AIM)(u, target, aim, g_retarget ? (void *)g_retarget : zero);
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

// Homing missiles intercept ([general] lead_missiles, default 0). A missile (EEUCMissile: AA towers and troopers,
// missile bases, helicopters, ...) climbs, then every step EEPMMissile 0x54E790 asks 0x633413 (call at 0x54E850, the
// missile as the "attacker") for the target's current position, turns towards it by at most type +0xB4 x step time
// and flies slower the further it has to turn (speed: +0x238 now, type +0xAC top, +0xA8 acceleration); it hits
// within 1.5 tiles (EEUCMissile vf55 0x605C01: squared distance <= 2.25 at 0x831860; else it flies on until it
// hits the ground - no fuel) - pure pursuit: it trails a moving target in a curve
// and catches up from behind. The hook replaces that point by the intercept point: target position + velocity x t,
// t the first time the missile at top speed can be there (|r + v t| = s t), recomputed every step, so it converges
// as the missile closes in; no solution (a target faster than the missile) stays pursuit. The velocity is measured:
// the target's position change between the missile's steps (averaged) - aircraft fly sideways and turn, their
// facing is no good for it.
#define CALL_MISSILE_AIM 0x0054E850
static int g_lead_missiles = 0;
enum { MSL_MAX = 256 };
static struct {
    uint8_t *m, *t;
    uint32_t first, last;
    float p[3], v[3], dist;   // target position at the last step, its velocity (tiles/s)
    int nv, moving, dmg;      // dmg: what it will do to its target (spread_missiles)
} g_msl[MSL_MAX];
static struct { unsigned n, close, gone; double ms, speed; } g_mst[2];   // tests: [target moving at launch]
static int missile_damage(uint8_t *m, uint8_t *t);

// A missile is gone when it hasn't been steered for 3 steps (last = the end of its climb until it steers).
static void missile_tick(uint32_t now) {
    uint8_t *clock = *(uint8_t **)G_WORLD_CLOCK;
    int gone_ms = clock ? (int)(FIELD(clock, 0x8C, float) * 3000.0f) : 0;
    if (gone_ms < 100) gone_ms = 100;
    for (int i = 0; i < MSL_MAX; i++)
        if (g_msl[i].m && (int32_t)(now - g_msl[i].last) > gone_ms) {
            if (g_measure) {
                int mv = g_msl[i].moving;
                g_mst[mv].n++;
                g_mst[mv].ms += g_msl[i].last - g_msl[i].first;
                if (g_msl[i].dist <= 1.5f) g_mst[mv].close++;
                else if (!find_unit_near(g_msl[i].t, g_msl[i].p[0], g_msl[i].p[1], 6)) g_mst[mv].gone++;
            }
            g_msl[i].m = NULL;
        }
}

static void __cdecl hook_missile_aim(uint8_t *m, uint8_t **target, float *aim, void *zero) {
    ((AimFn)FN_AIM)(m, target, aim, zero);
    uint8_t *t = *target;
    if (!t || t == m) return;
    uint32_t now = game_time();
    float tp[3] = { FIELD(t, 0x13C, float), FIELD(t, 0x14C, float), FIELD(t, 0x15C, float) };
    float r[3] = { aim[0] - FIELD(m, 0x13C, float), aim[1] - FIELD(m, 0x14C, float), aim[2] - FIELD(m, 0x15C, float) };
    float dist = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]), s = FIELD(FIELD(m, 0x34, uint8_t *), 0xAC, float);
    int k = -1, free_i = -1;
    for (int i = 0; i < MSL_MAX && k < 0; i++) {
        if (g_msl[i].m == m) k = i;
        else if (!g_msl[i].m && free_i < 0) free_i = i;
    }
    if (k < 0) {   // a new missile
        if (free_i < 0) return;
        k = free_i;
        memset(&g_msl[k], 0, sizeof g_msl[k]);
        g_msl[k].m = m; g_msl[k].first = now; g_msl[k].dist = dist;
        g_msl[k].moving = FIELD(t, 0x238, float) > 0.01f;
        g_msl[k].dmg = missile_damage(m, t);
        if (g_measure) g_mst[g_msl[k].moving].speed += s;
    } else if (g_msl[k].t != t) { g_msl[k].nv = 0; g_msl[k].dmg = missile_damage(m, t); }   // a new target
    else if ((int32_t)(now - g_msl[k].last) > 200) g_msl[k].nv = 0;   // a gap (the climb): start over
    else if (now > g_msl[k].last) {
        float dt = (float)(now - g_msl[k].last) * 0.001f, mv[3];
        for (int i = 0; i < 3; i++) mv[i] = (tp[i] - g_msl[k].p[i]) / dt;
        if (mv[0] * mv[0] + mv[1] * mv[1] + mv[2] * mv[2] < 400) {   // < 20 tiles/s, else a jump (teleport, new unit)
            for (int i = 0; i < 3; i++) g_msl[k].v[i] = g_msl[k].nv ? 0.5f * (g_msl[k].v[i] + mv[i]) : mv[i];
            g_msl[k].nv++;
        } else g_msl[k].nv = 0;
    }
    g_msl[k].t = t; g_msl[k].last = now;
    memcpy(g_msl[k].p, tp, sizeof tp);
    if (dist < g_msl[k].dist) g_msl[k].dist = dist;
    if (!g_lead_missiles || !g_msl[k].nv || !(s > 0.01f)) return;
    float *v = g_msl[k].v;
    float a = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] - s * s, b = 2 * (r[0] * v[0] + r[1] * v[1] + r[2] * v[2]);
    float c = dist * dist, T = -1;
    if (v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < 0.0025f) return;   // standing (< 0.05 tiles/s)
    if (fabsf(a) < 1e-4f) { if (b < 0) T = -c / b; }
    else {
        float disc = b * b - 4 * a * c;
        if (disc < 0) return;
        float q = sqrtf(disc), t1 = (-b - q) / (2 * a), t2 = (-b + q) / (2 * a);
        T = t1 > 0 && (t1 < t2 || t2 <= 0) ? t1 : t2;
    }
    if (!(T > 0)) return;
    if (T > 10) T = 10;
    for (int i = 0; i < 3; i++) aim[i] += v[i] * T;
}

// Spreading missiles ([general] spread_missiles, default 1). Every launcher picks its target on its own, so many AA
// towers all fire at the same aircraft - most of the missiles hit a wreck. Each missile is counted from its launch
// (EEUCMissile vf42 0x606109; the climb +0x28c ms before it steers) with the damage it will do, the game's own
// formula (0x61DE61): attack (launcher type of the owner, lookup 0x492969 on player +0x874, +0xD8) x the table
// 0x8D2050[attack class +0x4C0 + target class +0x4C0 x 6] x the bonus 0x4D5F2C - armour (target stats +0xE0 + attack
// class x 4), at least 1. Before a missile launcher fires (0x61E11C, its 4 callers), if what is already on the way
// covers the target's hit points, it takes the nearest other enemy of the same kind (aircraft for AA) in its range
// that it can see and that isn't covered yet - the projectile and its goal get that unit (0x633413 with the unit) -
// or holds this shot. A missile that misses frees its share again when it is gone.
#define VT_MISSILE        0x00832E90
#define FN_MISSILE_LAUNCH 0x00606109   // EEUCMissile vf42, thiscall(class, missile, aim) - ret 8, the aim unused
#define FN_FIRE           0x0061E11C   // thiscall(attacker class, attacker)
static const uintptr_t k_fire_calls[4] = { 0x00606067, 0x00607D79, 0x0060ABB0, 0x0060B33C };
static int g_spread = 1;
static unsigned g_spread_held, g_spread_switched, g_launched;

static int missile_damage(uint8_t *m, uint8_t *t) {
    uint8_t *lc = FIELD(m, 0x1C0, uint8_t *), *pl = FIELD(m, 0x18, uint8_t *), *tc = FIELD(t, 0x2C, uint8_t *);
    uint8_t *tpl = FIELD(t, 0x18, uint8_t *), *ts = FIELD(t, 0x34, uint8_t *);
    if (!lc || !pl || !tc || !tpl || !ts) return 0;
    int ac = FIELD(lc, 0x4C0, int), dc = FIELD(tc, 0x4C0, int);
    if (ac < 0 || ac > 5 || dc < 0 || dc > 15) return 0;
    uint8_t **rec = ((uint8_t **(__thiscall *)(void *, int))0x00492969)(pl + 0x874, FIELD(lc, 0x4B8, int));
    float atk = rec && *rec ? FIELD(*rec, 0xD8, float) : 0;
    if (!(atk > 0)) return 0;
    float bonus = ((float (__thiscall *)(void *, int, void *, int, int))0x004D5F2C)((void *)0x00917DE0, FIELD(lc, 0x4C8, int),
                  tc, FIELD(pl, 0x45C, int), FIELD(tpl, 0x45C, int));
    float mult = ((float *)0x008D2050)[ac + dc * 6] * bonus;
    if (!(mult > 0)) return 0;
    float d = mult * atk - FIELD(ts, 0xE0 + ac * 4, float);
    int n = (int)floorf(d + 0.5f);
    return n < 1 ? 1 : n;
}

static int incoming(uint8_t *t) {
    int sum = 0;
    for (int i = 0; i < MSL_MAX; i++)
        if (g_msl[i].m && g_msl[i].t == t) sum += g_msl[i].dmg;
    return sum;
}
static int doomed(uint8_t *t) { return FIELD(t, 0x3C, int) > 0 && incoming(t) >= FIELD(t, 0x3C, int); }

static int __fastcall hook_missile_launch(uint8_t *cls, void *edx, uint8_t *m, float *aim) {   // ret 8: (missile, aim)
    (void)edx;
    int r = ((int (__thiscall *)(void *, uint8_t *, float *))FN_MISSILE_LAUNCH)(cls, m, aim);
    uint8_t *goal = FIELD(m, 0x1EC, uint8_t *), *t = goal && goal[5] == 2 ? FIELD(goal, 0x30, uint8_t *) : NULL;
    g_launched++;
    if (!t) return r;
    uint32_t now = game_time();
    for (int i = 0; i < MSL_MAX; i++)
        if (!g_msl[i].m) {
            memset(&g_msl[i], 0, sizeof g_msl[i]);
            g_msl[i].m = m; g_msl[i].t = t; g_msl[i].first = now;
            g_msl[i].last = now + (uint32_t)(FIELD(m, 0x28C, int) > 0 ? FIELD(m, 0x28C, int) : 0);
            g_msl[i].dist = 1e9f; g_msl[i].moving = FIELD(t, 0x238, float) > 0.01f;
            g_msl[i].dmg = missile_damage(m, t);
            g_msl[i].p[0] = FIELD(t, 0x13C, float);
            g_msl[i].p[1] = FIELD(t, 0x14C, float); g_msl[i].p[2] = FIELD(t, 0x15C, float);
            if (g_measure) g_mst[g_msl[i].moving].speed += FIELD(FIELD(m, 0x34, uint8_t *), 0xAC, float);
            break;
        }
    return r;
}

// The nearest enemy of the same kind as t within u's range, seen by u's owner and not covered yet.
static uint8_t *spread_target(uint8_t *u, uint8_t *t) {
    void *pl = FIELD(u, 0x18, void *);
    if (!pl) return NULL;
    int fam = FIELD(FIELD(t, 0x2C, uint8_t *), 0x4B4, int);
    float x = FIELD(u, 0x13C, float), y = FIELD(u, 0x14C, float), range = FIELD(FIELD(u, 0x34, uint8_t *), 0x9C, float);
    int r = (int)range + 1;
    uint8_t *best = NULL;
    float bd = range * range;
    if (!(range > 0)) return NULL;
    for (struct grid_it it = grid_it_box((int)x, (int)y, r, 0); grid_it_next(&it); ) {
        uint8_t *c = it.u, *cc = it.td;
        if (c == t || FIELD(c, 0x3C, int) <= 0 || FIELD(cc, 0x4B4, int) != fam) continue;
        if (!is_enemy(pl, FIELD(c, 0x18, void *)) || !seen_by(c, pl) || doomed(c)) continue;
        float dx = FIELD(c, 0x13C, float) - x, dy = FIELD(c, 0x14C, float) - y, d = dx * dx + dy * dy;
        if (d <= bd) { bd = d; best = c; }
    }
    return best;
}

// firerate (test): per unit firing, its shots and the game time of the first and the last - the time between shots
// by type (the median of the units with 4+ shots)
static struct { uint8_t *u; int type, n; uint32_t t0, t1; } g_fr[512];
static int g_fr_n, g_fr_on;   // on in tests
static void fr_note(uint8_t *u) {
    uint8_t *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
    if (!td || !g_fr_on) return;
    uint32_t now = game_time();
    for (int k = 0; k < g_fr_n; k++)
        if (g_fr[k].u == u) { g_fr[k].n++; g_fr[k].t1 = now; return; }
    if (g_fr_n < 512) { g_fr[g_fr_n].u = u; g_fr[g_fr_n].type = FIELD(td, 0x4B8, int); g_fr[g_fr_n].n = 1; g_fr[g_fr_n].t0 = g_fr[g_fr_n].t1 = now; g_fr_n++; }
}
static void cmd_firerate(char *out, const char *args) {
    if (strstr(args, "reset")) { g_fr_n = 0; REPLY("ok: reset\n"); return; }
    int seen[64], ns = 0;
    for (int k = 0; k < g_fr_n; k++) {
        int t = g_fr[k].type, j = 0;
        while (j < ns && seen[j] != t) j++;
        if (j < ns || ns == 64) continue;
        seen[ns++] = t;
        float iv[64]; int m = 0;
        for (int q = k; q < g_fr_n && m < 64; q++)
            if (g_fr[q].type == t && g_fr[q].n >= 4) iv[m++] = (float)(g_fr[q].t1 - g_fr[q].t0) / 1000.0f / (float)(g_fr[q].n - 1);
        for (int a = 0; a < m; a++) for (int b = a + 1; b < m; b++) if (iv[b] < iv[a]) { float x = iv[a]; iv[a] = iv[b]; iv[b] = x; }
        uint8_t *td = type_data(t);
        REPLY("type %d: %d units with 4+ shots, median %.2f s between shots (min %.2f max %.2f); type +0x42C %.3g +0x430 %.3g +0x438 %.3g\n", t, m,
              m ? iv[m / 2] : 0.0f, m ? iv[0] : 0.0f, m ? iv[m - 1] : 0.0f, td ? FIELD(td, 0x42C, float) : 0.0f, td ? FIELD(td, 0x430, float) : 0.0f,
              td ? FIELD(td, 0x438, float) : 0.0f);
    }
}
static void __fastcall hook_fire(uint8_t *cls, void *edx, uint8_t *u) {
    (void)edx;
    fr_note(u);
    uint8_t *pc = FIELD(cls, 0x3FC, uint8_t *), *goal = FIELD(u, 0x1EC, uint8_t *);
    uint8_t *t = goal && goal[5] == 2 ? FIELD(goal, 0x30, uint8_t *) : NULL;
    if (g_spread && t && pc && *(uintptr_t *)pc == VT_MISSILE && doomed(t)) {
        uint8_t *alt = spread_target(u, t);
        if (!alt) { g_spread_held++; return; }
        g_spread_switched++;
        g_retarget = alt;
    }
    ((void (__thiscall *)(void *, uint8_t *))FN_FIRE)(cls, u);
    g_retarget = NULL;
}

// OWN HITS: EEUnit::vf28 (0x6172BC, damage) raises the "under attack" alert (0x519BC1 thiscall([0x9428A8], float
// pos[2]), ret 4: minimap flash and the voice) for every hit on a non-citizen object of the local player - also when
// his own arrow hit it (a bowman shooting a wolf beside his tower; leading the target lands more of the misses on
// what stands behind it). The attacker's player is vf28's third argument ([ebp+0x10]); its own: no alert. The damage
// stays.
#define CALL_ATTACK_ALERT 0x006177CF
#define FN_ATTACK_ALERT   0x00519BC1
__attribute__((used)) uintptr_t g_alert_fn = FN_ATTACK_ALERT;
__attribute__((used)) volatile unsigned g_own_hits;
__attribute__((naked, used)) void own_hit_alert_thunk(void) {
    __asm__("movl 0x10(%ebp), %eax\n\t"
            "cmpl 0x00918E84, %eax\n\t"
            "je 1f\n\t"
            "jmp *_g_alert_fn\n"
            "1:\n\t"
            "lock incl _g_own_hits\n\t"
            "ret $4");
}

static void aim_install(const char *aimod_ini, const char *autotest_ini) {
    BOOL own_hit = patch_call(CALL_ATTACK_ALERT, FN_ATTACK_ALERT, own_hit_alert_thunk);
    if (g_log) fprintf(g_log, "# aim: no attack alert for the player's own hits (hook %s)\n", own_hit ? "ok" : "FAILED");
    if (autotest_ini && GetPrivateProfileIntA("autotest", "aim_hits", 1, autotest_ini))
        patch_vtable(VT_PROJECTILE, 55, FN_PROJ_DOWN, hook_proj_down);
    g_lead = GetPrivateProfileIntA("general", "lead_targets", 1, aimod_ini) != 0;
    g_lead_missiles = GetPrivateProfileIntA("general", "lead_missiles", 0, aimod_ini) != 0;
    g_spread = GetPrivateProfileIntA("general", "spread_missiles", 1, aimod_ini) != 0;
    if (autotest_ini) {
        g_spread = GetPrivateProfileIntA("autotest", "spread_missiles", g_spread, autotest_ini) != 0;
        g_lead = GetPrivateProfileIntA("autotest", "lead_targets", g_lead, autotest_ini) != 0;
        g_lead_missiles = GetPrivateProfileIntA("autotest", "lead_missiles", g_lead_missiles, autotest_ini) != 0;
        g_lead_log = GetPrivateProfileIntA("autotest", "aim_log", 0, autotest_ini);
        g_measure = 1;
    }
    // hooked also when off in tests (the measurement); in play only when on
    BOOL ok = (g_lead || g_spread || autotest_ini) ? patch_call(CALL_AIM, FN_AIM, hook_aim) : TRUE;
    BOOL ok2 = (g_lead_missiles || g_spread || autotest_ini) ? patch_call(CALL_MISSILE_AIM, FN_AIM, hook_missile_aim) : TRUE;
    int ok3 = 0;
    g_fr_on = autotest_ini != NULL;
    if (g_spread || autotest_ini) {
        ok3 += patch_vtable(VT_MISSILE, 42, FN_MISSILE_LAUNCH, hook_missile_launch);
        for (int i = 0; i < 4; i++) ok3 += patch_call(k_fire_calls[i], FN_FIRE, hook_fire) ? 1 : 0;
    }
    if (g_log) fprintf(g_log, "# aim: lead_targets=%d (hook %s), lead_missiles=%d (hook %s), spread_missiles=%d (hooks %d/5)\n",
                       g_lead, ok ? "ok" : "FAILED", g_lead_missiles, ok2 ? "ok" : "FAILED", g_spread, ok3);
}

// aim [lead 0|1] [reset]: shots and the measurement (measuring starts with "aim reset")
static void cmd_aim(char *out, const char *args) {
    int v;
    if (sscanf(args, "lead %d", &v) == 1) g_lead = v != 0;
    if (sscanf(args, "missiles %d", &v) == 1) g_lead_missiles = v != 0;
    if (sscanf(args, "spread %d", &v) == 1) g_spread = v != 0;
    if (strstr(args, "reset")) {
        memset(g_ast, 0, sizeof g_ast); memset(g_pend, 0, sizeof g_pend);
        memset(g_msl, 0, sizeof g_msl); memset(g_mst, 0, sizeof g_mst);
        g_pend_n = g_shots = g_shots_unit = g_led = g_lost = g_down = g_hits = 0;
        g_spread_held = g_spread_switched = g_launched = 0;
        g_measure = 1;
    }
    REPLY("lead=%d shots %u, projectile shots at units %u, led %u, pending %u, target gone %u; down %u, hits %u; own hits without an alert %u\n",
          g_lead, g_shots, g_shots_unit, g_led, g_pend_n, g_lost, g_down, g_hits, g_own_hits);
    for (int m = 1; m >= 0; m--)
        for (int r = 1; r >= 0; r--)
            for (int e = 0; e < 2; e++)
                if (g_ast[m][r][e].n)
                    REPLY("  %-5s %-12s %-8s n %4u  on tile %5.1f%%  mean error %.2f tiles\n", m ? "moved" : "stood",
                          r ? "in reach" : "out of reach", e ? "led" : "original", g_ast[m][r][e].n,
                          100.0 * g_ast[m][r][e].on_tile / g_ast[m][r][e].n, g_ast[m][r][e].err / g_ast[m][r][e].n);
    REPLY("missiles launched %u; spread_missiles=%d: target switched %u, shot held %u\n", g_launched, g_spread,
          g_spread_switched, g_spread_held);
    for (int mv = 1; mv >= 0; mv--)
        if (g_mst[mv].n)
            REPLY("missiles (lead_missiles=%d) at %s targets: %u done, mean steered flight %.0f ms, hit (within 1.5) %u,"
                  " target died first %u, missed %u, top speed %.2f tiles/s\n", g_lead_missiles, mv ? "moving" : "standing",
                  g_mst[mv].n, g_mst[mv].ms / g_mst[mv].n, g_mst[mv].close, g_mst[mv].gone,
                  g_mst[mv].n - g_mst[mv].close - g_mst[mv].gone, g_mst[mv].speed / g_mst[mv].n);
    if (strstr(args, "types")) {   // unit types that fire missiles
        REPLY("missile types:");
        for (int ty = 0; ty < 0x300; ty++) {
            uint8_t *td = type_data(ty), *pc = td ? FIELD(td, 0x3FC, uint8_t *) : NULL;
            if (pc && *(uintptr_t *)pc == 0x00832E90) REPLY(" %d", ty);
        }
        REPLY("\n");
    }
}

// a new game (the game start hook): missiles in flight and shots measured - their age test (now - last) went
// negative after the clock reset, so the slots never freed and spread_missiles counted their damage on new units
static void aim_new_game(void) {
    memset(g_msl, 0, sizeof g_msl);
    memset(g_pend, 0, sizeof g_pend);
    g_pend_n = 0;
    g_fr_n = 0;
    g_retarget = NULL;
}
