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
// Multiplayer: everything here changes the simulation, so every PC must compute the same bits (lockstep). Only
// + - * / and sqrt (SSE, exact) and the game's own math DLL - the C runtime's cosf comes with Windows and may differ
// between versions. UMCos (cdecl float(float)) through the game's import table.
#define GAME_COS(a) game_cos(a)

internal int g_lead = 1;
internal int g_lead_log;       // autotest aim_log: the first N shots logged
internal int g_measure;        // tests: the measurement runs (autotest, or after "aim reset")
internal unsigned g_lead_logged;

internal int tile_height(ivec2 p, float *h) {
    ee_map *map = game_map();
    if (!map) return 0;
    unsigned n = map_get_size(map);
    if (p.x < 0 || p.y < 0 || (unsigned)p.x >= n || (unsigned)p.y >= n) return 0;
    ee_tile *t = map_get_tiles(map)[(unsigned)p.y * n + (unsigned)p.x];
    if (!t) return 0;
    *h = tile_get_height(t);
    return 1;
}

// Seconds the projectile flies from `from` towards `to` with the arc the launch will pick, < 0 if unknown. Out of
// the arc's reach the game launches at 45 degrees anyway and the projectile comes down short, after flying its
// longest distance v^2 / g (*reach = 0).
internal float flight_time(ee_unit *u, vec3 from, vec3 to, int *reach) {
    ee_unit_stats *type = unit_get_stats(u); ee_unit_type *cls = type ? unit_stats_get_type_data(type) : NULL;
    ee_unit_type *pc = cls ? unit_type_get_projectile(cls) : NULL;
    float v = type ? unit_stats_get_shot_speed(type) : 0, g = pc ? unit_type_get_accel(pc) : 0, pitch = 0, a[3], b[3];
    if (v <= 0.001f || g <= 0.001f) return -1;
    vec3_store(a, from); vec3_store(b, to);   // the solver's own copies (it may write them)
    float d = vec2_len(vec2_sub(vec3_xy(to), vec3_xy(from)));
    *reach = ballistic(u, type, a, b, &pitch) != 0;
    float c;
    if (!*reach) {
        c = 0.70710678f;   // 45 degrees
        if (d > v * v / g) d = v * v / g;
    } else c = GAME_COS(pitch);
    if (c < 0.05f) return -1;
    return d / (v * c);
}

internal vec3 launch_point(ee_unit *u) {   // about the launch point (0x62028B: the model's fire point)
    vec3 p = unit_get_pos(u);
    p.z += 0.5f * unit_stats_get_height(unit_get_stats(u));
    return p;
}

internal int target_velocity(ee_unit *t, vec2 *v) {   // on the ground, tiles/s
    float s = unit_get_speed(t);
    vec2 f = vec3_xy(unit_get_facing(t));
    float l = vec2_len(f);
    *v = v2(0, 0);
    if (!(s > 0.01f && s < 50 && l > 0.01f)) return 0;
    *v = v2(s * f.x / l, s * f.y / l);
    return 1;
}

// The aim point for the target's velocity: the original aim moved to where the target is after the flight.
internal float lead_aim(ee_unit *u, ee_unit *t, vec3 aim, float T, vec2 v, vec3 *out, int *reach) {
    vec3 from = launch_point(u);
    ivec2 tile = unit_get_tile(t);
    vec2 centre = ivec2_centre(tile);
    int centred = aim.x == centre.x && aim.y == centre.y;
    float h0 = 0, h;
    int has_h0 = tile_height(tile, &h0);
    *out = aim;
    for (int i = 0; i < 3; i++) {
        if (T > 6) T = 6;
        vec2 m = vec2_scale(v, T), q = centred ? vec2_add(vec3_xy(unit_get_pos(t)), m) : vec2_add(vec3_xy(aim), m);
        vec3 p = centred ? v3(floorf(q.x) + 0.5f, floorf(q.y) + 0.5f, aim.z) : v3(q.x, q.y, aim.z);
        if (has_h0 && tile_height(vec2_tile(vec3_xy(p)), &h)) p.z += h - h0;
        float T2 = flight_time(u, from, p, reach);
        if (T2 < 0) break;
        *out = p;
        T = T2;
    }
    return T;
}

// Measurement (aim command): for each projectile shot at a unit, where the target really is when the projectile
// comes down, against the original aim [0] and the led aim [1] (each at its own flight time). "On tile" = the
// target stands on the aim's tile then - the condition for a hit (if the shot reaches). Split by targets that moved
// (> 0.3 tiles during the flight) or stood, and shots in or out of the arc's reach.
enum { AIM_PEND = 512 };
internal struct { ee_unit *t; uint32_t due[2]; vec2 a[2], p0; uint8_t done, reach; } g_pend[AIM_PEND];   // aims, the target's start
internal unsigned g_pend_n;
internal struct { unsigned n, on_tile; double err; } g_ast[2][2][2];   // [moved][in reach][aim]
internal unsigned g_shots, g_shots_unit, g_led, g_lost;

internal ee_unit *find_unit_near(ee_unit *want, vec2 p, int r) {
    for (struct grid_it it = grid_it_box((int)p.x, (int)p.y, r, 1); grid_it_next(&it); )
        if (it.u == want) return unit_get_hp(want) > 0 ? want : NULL;
    return NULL;
}

internal void missile_tick(uint32_t now);
internal void aim_tick(void) {   // the simulation thread, every pass
    uint32_t now = game_time();
    missile_tick(now);
    if (!g_measure) return;
    if (!g_pend_n) return;
    for (unsigned i = 0; i < AIM_PEND; i++) {
        if (!g_pend[i].t) continue;
        for (int e = 0; e < 2; e++) {
            if ((g_pend[i].done >> e & 1) || now < g_pend[i].due[e]) continue;
            g_pend[i].done |= (uint8_t)(1 << e);
            ee_unit *t = find_unit_near(g_pend[i].t, g_pend[i].p0, 12);
            if (!t) { if (e == 0) g_lost++; continue; }
            vec2 p = vec3_xy(unit_get_pos(t));
            int moved = vec2_dist2(p, g_pend[i].p0) > 0.09f, r = g_pend[i].reach;
            g_ast[moved][r][e].n++;
            g_ast[moved][r][e].err += vec2_len(vec2_sub(p, g_pend[i].a[e]));
            if (ivec2_eq(vec2_tile(p), vec2_tile(g_pend[i].a[e]))) g_ast[moved][r][e].on_tile++;
        }
        if (g_pend[i].done == 3) { g_pend[i].t = NULL; g_pend_n--; }
    }
}

internal void aim_measure(ee_unit *t, vec3 aim, float T0, vec3 led, float T1, int reach) {
    if (T0 < 0 || g_pend_n >= AIM_PEND) return;
    for (unsigned i = 0; i < AIM_PEND; i++) {
        if (g_pend[i].t) continue;
        uint32_t now = game_time();
        g_pend[i].t = t; g_pend[i].done = 0; g_pend[i].reach = (uint8_t)reach;
        g_pend[i].p0 = vec3_xy(unit_get_pos(t));
        g_pend[i].due[0] = now + (uint32_t)(T0 * 1000.0f);
        g_pend[i].due[1] = now + (uint32_t)((T1 >= 0 ? T1 : T0) * 1000.0f);
        g_pend[i].a[0] = vec3_xy(aim); g_pend[i].a[1] = vec3_xy(led);
        g_pend_n++;
        return;
    }
}

internal ee_unit *g_retarget;            // spread_missiles: set while 0x61E11C fires at another target
internal void __cdecl hook_aim(ee_unit *u, ee_unit **target, float *aim, void *zero) {
    unit_aim(u, target, aim, g_retarget ? (void *)g_retarget : zero);
    ee_unit *t = *target;
    ee_unit_type *cls = unit_get_type_data(u); ee_unit_type *pc = cls ? unit_type_get_projectile(cls) : NULL;
    g_shots++;
    if (!t || t == u || !pc || !is_projectile_class(pc)) return;
    g_shots_unit++;
    vec3 a = vec3_load(aim), led = a;
    vec2 v;
    float T1 = -1;
    int reach0 = 0, reach1 = 0;
    float T0 = flight_time(u, launch_point(u), a, &reach0);
    if (T0 >= 0 && target_velocity(t, &v)) T1 = lead_aim(u, t, a, T0, v, &led, &reach1);
    else reach1 = reach0;
    if (g_lead_log && g_lead_logged < (unsigned)g_lead_log && g_log) {
        g_lead_logged++;
        vec3 up = unit_get_pos(u), tp = unit_get_pos(t), tf = unit_get_facing(t);
        fprintf(g_log, "# aim: t %u shooter %p at %.2f,%.2f -> target %p at %.2f,%.2f spd %.2f fwd %.2f,%.2f | aim %.2f,%.2f,%.2f"
                " T %.2f reach %d | led %.2f,%.2f,%.2f T %.2f reach %d\n", game_time(), u, up.x, up.y, t, tp.x, tp.y,
                unit_get_speed(t), tf.x, tf.y, a.x, a.y, a.z, T0, reach0, led.x, led.y, led.z, T1, reach1);
    }
    if (g_measure) aim_measure(t, a, T0, led, T1, g_lead ? reach1 : reach0);
    if (g_lead && T1 >= 0 && memcmp(&led, &a, sizeof led) && !is_vanilla(unit_get_owner(u))) {   // a vanilla player's units: measured, not led
        vec3_store(aim, led);
        g_led++;
    }
}

// Tests: real hits. EEUCProjectile vf55 (0x6078F9, thiscall(class, projectile)) returns 1 when the projectile is down:
// on the target's footprint (a hit) or not (it hits the ground there).
internal unsigned g_down, g_hits;
internal char __thiscall hook_proj_down(void *cls, ee_unit *pr) {
    ee_goal *goal = unit_get_goal_object(pr);
    ee_unit *t = goal && goal_get_state(goal) == 2 ? goal_get_target(goal) : NULL;
    char r = proj_down(cls, pr);
    if (r && t && g_measure) {
        g_down++;
        unsigned x = ((unsigned)unit_get_x(pr)), y = ((unsigned)unit_get_y(pr)), tx = ((unsigned)unit_get_x(t)),
                 ty = ((unsigned)unit_get_y(t)), n = ((unsigned)unit_type_get_size(unit_get_type_data(t)));
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
internal int g_lead_missiles = 0;
enum { MSL_MAX = 256 };
internal struct {
    ee_unit *m, *t;
    uint32_t first, last;
    vec3 p, v;                // target position at the last step, its velocity (tiles/s)
    float dist;
    int nv, moving, dmg;      // dmg: what it will do to its target (spread_missiles)
} g_msl[MSL_MAX];
internal struct { unsigned n, close, gone; double ms, speed; } g_mst[2];   // tests: [target moving at launch]
internal int missile_damage(ee_unit *m, ee_unit *t);

// A missile is gone when it hasn't been steered for 3 steps (last = the end of its climb until it steers).
internal void missile_tick(uint32_t now) {
    ee_physics *clock = game_physics();
    int gone_ms = clock ? (int)(physics_get_step_s(clock) * 3000.0f) : 0;
    if (gone_ms < 100) gone_ms = 100;
    for (int i = 0; i < MSL_MAX; i++)
        if (g_msl[i].m && (int32_t)(now - g_msl[i].last) > gone_ms) {
            if (g_measure) {
                int mv = g_msl[i].moving;
                g_mst[mv].n++;
                g_mst[mv].ms += g_msl[i].last - g_msl[i].first;
                if (g_msl[i].dist <= 1.5f) g_mst[mv].close++;
                else if (!find_unit_near(g_msl[i].t, vec3_xy(g_msl[i].p), 6)) g_mst[mv].gone++;
            }
            g_msl[i].m = NULL;
        }
}

internal void __cdecl hook_missile_aim(ee_unit *m, ee_unit **target, float *aim, void *zero) {
    unit_aim(m, target, aim, zero);
    ee_unit *t = *target;
    if (!t || t == m) return;
    uint32_t now = game_time();
    vec3 tp = unit_get_pos(t), r = vec3_sub(vec3_load(aim), unit_get_pos(m));
    float dist = vec3_len(r), s = unit_stats_get_speed(unit_get_stats(m));
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
        g_msl[k].moving = unit_get_speed(t) > 0.01f;
        g_msl[k].dmg = missile_damage(m, t);
        if (g_measure) g_mst[g_msl[k].moving].speed += s;
    } else if (g_msl[k].t != t) { g_msl[k].nv = 0; g_msl[k].dmg = missile_damage(m, t); }   // a new target
    else if ((int32_t)(now - g_msl[k].last) > 200) g_msl[k].nv = 0;   // a gap (the climb): start over
    else if (now > g_msl[k].last) {
        float dt = (float)(now - g_msl[k].last) * 0.001f;
        vec3 mv = vec3_div(vec3_sub(tp, g_msl[k].p), dt);
        if (vec3_len2(mv) < 400) {   // < 20 tiles/s, else a jump (teleport, new unit)
            g_msl[k].v = g_msl[k].nv ? vec3_scale(vec3_add(g_msl[k].v, mv), 0.5f) : mv;
            g_msl[k].nv++;
        } else g_msl[k].nv = 0;
    }
    g_msl[k].t = t; g_msl[k].last = now;
    g_msl[k].p = tp;
    if (dist < g_msl[k].dist) g_msl[k].dist = dist;
    if (!g_lead_missiles || !g_msl[k].nv || !(s > 0.01f) || is_vanilla(unit_get_owner(m))) return;   // tracked, not steered
    vec3 v = g_msl[k].v;
    float a = vec3_len2(v) - s * s, b = 2 * vec3_dot(r, v);
    float c = dist * dist, T = -1;
    if (vec3_len2(v) < 0.0025f) return;   // standing (< 0.05 tiles/s)
    if (fabsf(a) < 1e-4f) { if (b < 0) T = -c / b; }
    else {
        float disc = b * b - 4 * a * c;
        if (disc < 0) return;
        float q = sqrtf(disc), t1 = (-b - q) / (2 * a), t2 = (-b + q) / (2 * a);
        T = t1 > 0 && (t1 < t2 || t2 <= 0) ? t1 : t2;
    }
    if (!(T > 0)) return;
    if (T > 10) T = 10;
    vec3_store(aim, vec3_add(vec3_load(aim), vec3_scale(v, T)));
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
internal int g_spread = 1;
internal unsigned g_spread_held, g_spread_switched, g_launched;

internal int missile_damage(ee_unit *m, ee_unit *t) {
    ee_unit_type *lc = unit_get_attacker_type(m); ee_unit_type *tc = unit_get_type_data(t);
    ee_player *pl = unit_get_owner(m);
    ee_player *tpl = unit_get_owner(t);
    ee_unit_stats *ts = unit_get_stats(t);
    if (!lc || !pl || !tc || !tpl || !ts) return 0;
    int ac = unit_type_get_damage_class(lc), dc = unit_type_get_damage_class(tc);
    if (ac < 0 || ac > 5 || dc < 0 || dc > 15) return 0;
    ee_unit_stats **rec = type_stats(pl, unit_type_get_id(lc));
    float atk = rec && *rec ? unit_stats_get_attack(*rec) : 0;
    if (!(atk > 0)) return 0;
    float bonus = attack_bonus(lc, tc, player_get_index(pl), player_get_index(tpl));
    float mult = damage_table()[ac + dc * 6] * bonus;
    if (!(mult > 0)) return 0;
    float d = mult * atk - unit_stats_get_armor(ts, ac);
    int n = (int)floorf(d + 0.5f);
    return n < 1 ? 1 : n;
}

internal int incoming(ee_unit *t) {
    int sum = 0;
    for (int i = 0; i < MSL_MAX; i++)
        if (g_msl[i].m && g_msl[i].t == t) sum += g_msl[i].dmg;
    return sum;
}
internal int doomed(ee_unit *t) { return unit_get_hp(t) > 0 && incoming(t) >= unit_get_hp(t); }

internal int __thiscall hook_missile_launch(uint8_t *cls, ee_unit *m, float *aim) {   // ret 8: (missile, aim)
    int r = missile_launch(cls, m, aim);
    ee_goal *goal = unit_get_goal(m);
    ee_unit *t = goal && goal_get_state(goal) == 2 ? goal_get_target(goal) : NULL;
    g_launched++;
    if (!t) return r;
    uint32_t now = game_time();
    for (int i = 0; i < MSL_MAX; i++)
        if (!g_msl[i].m) {
            memset(&g_msl[i], 0, sizeof g_msl[i]);
            g_msl[i].m = m; g_msl[i].t = t; g_msl[i].first = now;
            g_msl[i].last = now + (uint32_t)(unit_get_flight_ms(m) > 0 ? unit_get_flight_ms(m) : 0);
            g_msl[i].dist = 1e9f; g_msl[i].moving = unit_get_speed(t) > 0.01f;
            g_msl[i].dmg = missile_damage(m, t);
            g_msl[i].p = unit_get_pos(t);
            if (g_measure) g_mst[g_msl[i].moving].speed += unit_stats_get_speed(unit_get_stats(m));
            break;
        }
    return r;
}

// The nearest enemy of the same kind as t within u's range, seen by u's owner and not covered yet.
internal ee_unit *spread_target(ee_unit *u, ee_unit *t) {
    ee_player *pl = unit_get_owner(u);
    if (!pl) return NULL;
    int fam = unit_type_get_family(unit_get_type_data(t));
    vec2 p = vec3_xy(unit_get_pos(u));
    float range = unit_stats_get_range(unit_get_stats(u));
    int r = (int)range + 1;
    ee_unit *best = NULL;
    float bd = range * range;
    if (!(range > 0)) return NULL;
    for (struct grid_it it = grid_it_box((int)p.x, (int)p.y, r, 1); grid_it_next(&it); ) {
        ee_unit *c = it.u;
        ee_unit_type *cc = it.td;
        if (c == t || unit_get_hp(c) <= 0 || unit_type_get_family(cc) != fam) continue;
        if (!is_enemy(pl, unit_get_owner(c)) || !seen_by(c, pl) || doomed(c)) continue;
        float d = vec2_dist2(vec3_xy(unit_get_pos(c)), p);
        if (d <= bd) { bd = d; best = c; }
    }
    return best;
}

// firerate (test): per unit firing, its shots and the game time of the first and the last - the time between shots
// by type (the median of the units with 4+ shots)
internal struct { ee_unit *u; int type, n; uint32_t t0, t1; } g_fr[512];
internal int g_fr_n, g_fr_on;   // on in tests
internal void fr_note(ee_unit *u) {
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    if (!td || !g_fr_on) return;
    uint32_t now = game_time();
    for (int k = 0; k < g_fr_n; k++)
        if (g_fr[k].u == u) { g_fr[k].n++; g_fr[k].t1 = now; return; }
    if (g_fr_n < 512) { g_fr[g_fr_n].u = u; g_fr[g_fr_n].type = unit_type_get_id(td); g_fr[g_fr_n].n = 1; g_fr[g_fr_n].t0 = g_fr[g_fr_n].t1 = now; g_fr_n++; }
}
internal void cmd_firerate(char *out, const char *args) {
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
        ee_unit_type *td = type_data(t);
        REPLY("type %d: %d units with 4+ shots, median %.2f s between shots (min %.2f max %.2f); type +0x42C %.3g +0x430 %.3g +0x438 %.3g\n", t, m,
              m ? iv[m / 2] : 0.0f, m ? iv[0] : 0.0f, m ? iv[m - 1] : 0.0f, td ? unit_type_get_unknown_42c(td) : 0.0f, td ? unit_type_get_unknown_430(td) : 0.0f,
              td ? unit_type_get_accel(td) : 0.0f);
    }
}
internal void __thiscall hook_fire(ee_unit_type *cls, ee_unit *u) {
    fr_note(u);
    ee_unit_type *pc = unit_type_get_projectile(cls); ee_goal *goal = unit_get_goal(u);
    ee_unit *t = goal && goal_get_state(goal) == 2 ? goal_get_target(goal) : NULL;
    if (g_spread && t && pc && is_missile_class(pc) && !is_vanilla(unit_get_owner(u)) && doomed(t)) {
        ee_unit *alt = spread_target(u, t);
        if (!alt) { g_spread_held++; return; }
        g_spread_switched++;
        g_retarget = alt;
    }
    unit_type_fire(cls, u);
    g_retarget = NULL;
}

// OWN HITS: EEUnit::vf28 (0x6172BC, damage) raises the "under attack" alert (0x519BC1 thiscall([0x9428A8], float
// pos[2]), ret 4: minimap flash and the voice) for every hit on a non-citizen object of the local player - also when
// his own arrow hit it (a bowman shooting a wolf beside his tower; leading the target lands more of the misses on
// what stands behind it). The attacker's player is vf28's third argument ([ebp+0x10]); its own: no alert. The damage
// stays.
__attribute__((used)) uintptr_t g_alert_fn, g_local_player_at;   // game_attack_alert(), local_player_index_at(): aim_install
__attribute__((used)) volatile unsigned g_own_hits;
__attribute__((naked, used)) void own_hit_alert_thunk(void) {
    __asm__("movl 0x10(%ebp), %eax\n\t"
            "pushl %ecx\n\t"
            "movl _g_local_player_at, %ecx\n\t"
            "cmpl (%ecx), %eax\n\t"
            "popl %ecx\n\t"               // the flags stay
            "je 1f\n\t"
            "jmp *_g_alert_fn\n"
            "1:\n\t"
            "lock incl _g_own_hits\n\t"
            "ret $4");
}

internal void aim_install(const char *autotest_ini) {
    g_alert_fn = game_attack_alert();
    g_local_player_at = local_player_index_at();
    BOOL own_hit = patch_attack_alert(own_hit_alert_thunk);
    if (g_log) fprintf(g_log, "# aim: no attack alert for the player's own hits (hook %s)\n", own_hit ? "ok" : "FAILED");
    if (autotest_ini && g_set.aim_hits)
        patch_proj_down(hook_proj_down);
    g_lead = g_set.lead_targets;
    g_lead_missiles = g_set.lead_missiles;
    g_spread = g_set.spread_missiles;
    if (autotest_ini) {
        g_lead_log = g_set.aim_log;
        g_measure = 1;
    }
    // hooked also when off in tests (the measurement); in play only when on
    BOOL ok = (g_lead || g_spread || autotest_ini) ? patch_aim(hook_aim) : TRUE;
    BOOL ok2 = (g_lead_missiles || g_spread || autotest_ini) ? patch_missile_aim(hook_missile_aim) : TRUE;
    int ok3 = 0;
    g_fr_on = autotest_ini != NULL;
    if (g_spread || autotest_ini) {
        ok3 += patch_missile_launch(hook_missile_launch);
        ok3 += patch_fire(hook_fire, NULL);
    }
    if (g_log) fprintf(g_log, "# aim: lead_targets=%d (hook %s), lead_missiles=%d (hook %s), spread_missiles=%d (hooks %d/5)\n",
                       g_lead, ok ? "ok" : "FAILED", g_lead_missiles, ok2 ? "ok" : "FAILED", g_spread, ok3);
}

// aim [lead 0|1] [reset]: shots and the measurement (measuring starts with "aim reset")
internal void cmd_aim(char *out, const char *args) {
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
            ee_unit_type *td = type_data(ty); ee_unit_type *pc = td ? unit_type_get_projectile(td) : NULL;
            if (pc && is_missile_class(pc)) REPLY(" %d", ty);
        }
        REPLY("\n");
    }
}

// a new game (the game start hook): missiles in flight and shots measured - their age test (now - last) went
// negative after the clock reset, so the slots never freed and spread_missiles counted their damage on new units
internal void aim_new_game(void) {
    memset(g_msl, 0, sizeof g_msl);
    memset(g_pend, 0, sizeof g_pend);
    g_pend_n = 0;
    g_fr_n = 0;
    g_retarget = NULL;
}
