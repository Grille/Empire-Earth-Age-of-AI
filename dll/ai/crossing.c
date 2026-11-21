// THE CROSSING ([ai_base] ferry_safety; the game's computer player off): what a transport risks on its way over, and the
// way it takes. The player 2026-11-18: "the safety check should be smart, and plan its route based on what it knows
// and how important the delivery is - 1. is it ok to lose the transport as long the delivery is made? (invasions or
// defensive reinforcements, not regular army or citizens balancing) 2. how dense is the enemy coverage? will it get
// stuck and sunk before it can deliver? 3. are the enemy ships already bound/distracted in a battle with an own fleet?"
// and yes to the rest: fog and old knowledge, a fuller load into danger, the places transports were sunk, storms,
// aircraft and submarines.
// THE THREATS (a look each CX_LOOK per AI, what it knows - honest_vision): whatever hits its transport type (the game's
// table, landing.c ld_factor) - ships, submarines and aircraft it sees, the defences and long guns on a coast it has
// seen (buildings until their spot is looked at again), ships it saw in the last 5 minutes where it saw them (the reach
// grown by how far they may have sailed since, CX_DRIFT, and counted half: they may be anywhere) -, enemy storms it sees,
// and the places its transports were sunk in the last CX_SUNK_T (fy: cx_sunk). A ship within its reach of a warship of
// its own is BUSY: in a fight with its fleet, counted a quarter.
// THE RATING of a way (the water at the pick-up to the water off the drop, straight or through one point off the line):
// every 2 tiles of it the time the transport gets there at its speed; a ship or aircraft goes for it from the first
// point that passes within its sight (CX_NOTICE beyond its reach - one that never sees it lets it be: the first rating
// had every ship that could reach the way in time come for it, and 29 of 29 trips of one AI were held) and hits it
// from when it has closed in - to the drop and through the unload when it is as fast as the transport (it follows),
// else for as long as the transport takes to pass its reach; a standing one (a
// tower, a storm, a place of losses) while the way lies in its reach. Its damage a second times those seconds, summed:
// the damage expected before the load is off, a share of the transport's hit points (risk; over 1: sunk before).
// THE WAY: the straight one and through points CX_SIDE tiles off its middle to either side (the sea's water nearest
// them), the least risk (a minute's sailing counts as CX_TIME of it) - the transport goes through that point first.
// CALIBRATED (2026-11-18, the check off and every ferry trip rated - Small Islands large, 4 AIs, epoch 5, 60 min, 24
// seeds, 1218 trips): the rating is a rank, not the share it says - by its band the trips delivered, those that sailed
// and were put off on a beach under fire or sunk, and the transports lost before they sailed (on the way to the
// pick-up or waiting there):  under 0.05  62% delivered, 20% of the sailed in trouble, 8% lost before;  0.2-1  45%,
// about 35%, 12%;  1-2.5  31%, 47%, 28%;  2.5-5  20%, 66%, 30%;  over 5  7%, 80%, 50%.
// MEASURED 2026-11-18 (head-to-head, mirrored, 12 seeds, Small Islands large, 4 AIs, epoch 5, 60 min; without -> with
// per AI): trips started 10.8 -> 8.7, delivered 5.0 -> 5.3, put off on a beach under fire 2.2 -> 0.6, sunk at sea 0.5 ->
// 0.3, transports lost before sailing 1.7 -> 0.9; citizens put off 18.2 -> 18.9, soldiers 8.5 -> 9.5; soldiers -1.4 (se
// 3.9), kills +3.1 (4.1), workers +1.0 (2.3). As much delivered for less than half the transports lost: ON.
// THE DECISION (ferry.c), on that scale: an important delivery - soldiers for a land under attack (later: a push across
// the water) - goes while it has a fair chance to arrive (under CX_GO_HIGH: the transport may be lost after); any other
// - the shares, the citizens - only while it is likely to come through (CX_GO_LOW, CX_GO_STEP more each minute it has
// waited, up to CX_GO_WAIT); into danger (over CX_FULL) with a full load only. A trip that may not go waits off the shore, unloaded (loaded
// at a shore 9 of 10 transports were sunk with everyone aboard, expand.c), its escort asked for meanwhile.
#define CX_LOOK 2000
#define CX_N 160
#define CX_DRIFT 0.5f        // tiles a second a remembered ship may have sailed (half a frigate's speed) ...
#define CX_DRIFT_MAX 12.0f   // ... at most this many
#define CX_SUNK_T 300000
#define CX_SUNK_R 8.0f
#define CX_SIDE 10           // the ways off the line: this many tiles to a side, and twice that
#define CX_TIME 0.1f         // risk a minute's sailing counts as
#define CX_UNLOAD 8.0f       // seconds an unload takes
#define CX_NOTICE 8.0f       // a ship goes for a transport that passes this far beyond its reach (its sight)
#define CX_GO_HIGH 5.0f      // THE DECISION's limits on the rating's scale (CALIBRATED, above)
#define CX_GO_LOW 1.0f
#define CX_GO_WAIT 2.5f
#define CX_GO_STEP 0.25f     // ... a minute's wait adds this to the limit of a trip that is no important one
#define CX_FULL 0.35f
#define CROSSING_SAFETY(idx) PV(idx, g_v_cx, g_cx)
struct cx_threat { ivec2 p; float r, dps, v; uint8_t air, busy, old; };
internal int g_cx_top;            // the last way's threat that does the most damage (the log), -1: none
internal float g_cx_top_dmg;
internal struct cx_st {
    uint32_t t; int n, trtype; struct cx_threat th[CX_N];
    struct { ivec2 p; uint32_t t; } sunk[8];
    unsigned rated, detours, held, held_load, gone_high;   // ways rated, taken through a point off the line; trips held for the risk, for a load too small for it; important ones that went over CX_GO_LOW
} g_cx_st[16];
// a new game or a load: the last game's sunk places came back as threats once the clock reached their times again
// (a load's clock often past them already), at the old map's coordinates, and its threat list stood in for a look
internal void crossing_new_game(void) { memset(g_cx_st, 0, sizeof g_cx_st); }
internal void cx_sunk(unsigned idx, ivec2 p, uint32_t now) {   // a transport of its sunk there
    struct cx_st *c = &g_cx_st[idx & 15];
    int k = 0;
    for (int j = 1; j < 8; j++) if (c->sunk[j].t < c->sunk[k].t) k = j;
    c->sunk[k].p = p; c->sunk[k].t = now ? now : 1;
}
internal void cx_add(struct cx_st *c, ivec2 p, float r, float dps, float v, int air, int old) {
    if (c->n >= CX_N || r <= 0 || dps <= 0) return;
    struct cx_threat *t = &c->th[c->n++];
    t->p = p; t->r = r; t->dps = dps; t->v = v; t->air = (uint8_t)air; t->busy = 0; t->old = (uint8_t)old;
}
// the threats to transports of type trtd as player pl knows them (each CX_LOOK)
internal void cx_scan(ee_player *pl, unsigned idx, ee_unit_type *trtd, uint32_t now) {
    struct cx_st *c = &g_cx_st[idx & 15];
    int tt = trtd ? unit_type_get_id(trtd) : 0;
    if (c->t && now >= c->t && now - c->t < CX_LOOK && c->trtype == tt) return;
    c->t = now ? now : 1; c->n = 0; c->trtype = tt;
    if (!trtd) return;
    int tpi = player_get_index(pl);
    static ivec2 own[256]; static float ownr[256];
    int no = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        int x = unit_get_x(u), y = unit_get_y(u), fam = unit_type_get_class_id(it.td);
        if (!o || x < 0 || unit_get_hp(u) <= 0) continue;
        if (o == pl) {   // its warships: what they fight is busy
            if ((fam == CLS_NAVAL || fam == CLS_SUBMERGED_NAVAL) && unit_type_get_attack(it.td) > 0 && !is_transport(unit_type_get_id(it.td)) && no < 256) { own[no] = iv2(x, y); ownr[no++] = fl_range(u, it.td); }
            continue;
        }
        if (player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        if (is_storm(unit_type_get_id(it.td))) { cx_add(c, iv2(x, y), (float)ST_R, 100.0f, 0, 0, 0); continue; }   // storms.c: 100 a second to its owner's enemies within its reach
        float f = ld_factor(it.td, ld_pidx(u), trtd, tpi);
        if (f <= 0) continue;
        int air = ld_is_air(u);
        ee_unit_stats *st = unit_get_stats(u);
        float v = st ? unit_stats_get_speed(st) : 0;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            float r, a;
            if (defence_stats(u, &r, &a)) cx_add(c, iv2(x, y), r, fl_dps(u, it.td) * f, 0, 0, 0);
            continue;
        }
        if (!air && !area_id(iv2(x, y), 1)) {   // on land: a long gun on the coast (fleet.c fl_danger_r), where it stands
            float r = fl_danger_r(u, it.td);
            if (r > 0) cx_add(c, iv2(x, y), r, fl_dps(u, it.td) * f, 0, 0, 0);
            continue;
        }
        cx_add(c, iv2(x, y), fl_range(u, it.td) > 1.0f ? fl_range(u, it.td) : 1.5f, fl_dps(u, it.td) * f, v, air, 0);
    }
    if (g_honest_vision) {   // remembered (intel.c): buildings till their spot is looked at again, ships 5 minutes
        struct intel *in = &g_intel[idx & 15];
        for (int k = 0; k < in->n; k++) {
            struct intel_rec *e = &in->r[k];
            if (e->t == in->last || e->x < 0 || (e->kind != 0 && e->kind != 2)) continue;   // seen now: above
            ee_unit_type *td = type_data(e->type);
            uint32_t age = in->now - e->t;
            if (!td || (e->kind == 2 && age > 300000)) continue;
            float f = ld_factor(td, e->owner, trtd, tpi);
            if (f <= 0) continue;
            if (e->kind == 0) { float r = fl_danger_r(NULL, td); if (r > 0) cx_add(c, iv2(e->x, e->y), r, fl_dps(NULL, td) * f, 0, 0, 0); continue; }
            float drift = (float)age / 1000.0f * CX_DRIFT;
            if (drift > CX_DRIFT_MAX) drift = CX_DRIFT_MAX;
            float r = fl_range(NULL, td);
            cx_add(c, iv2(e->x, e->y), (r > 1.0f ? r : 1.5f) + drift, fl_dps(NULL, td) * f * 0.5f, 0, 0, 1);
        }
    }
    for (int k = 0; k < 8; k++)   // where its transports were sunk lately: as if something sank them there still
        if (c->sunk[k].t && now >= c->sunk[k].t && now - c->sunk[k].t < CX_SUNK_T) cx_add(c, c->sunk[k].p, CX_SUNK_R, (float)unit_type_get_hp(trtd) / 20.0f, 0, 0, 1);
    for (int i = 0; i < c->n; i++) {   // BUSY: within its reach of a warship of its own (and so in its fight)
        struct cx_threat *t = &c->th[i];
        if (t->air || t->v <= 0) continue;
        for (int j = 0; j < no && !t->busy; j++) {
            float r = (t->r > ownr[j] ? t->r : ownr[j]) + 2.0f;
            t->busy = (uint8_t)((float)ivec2_dist2(t->p, own[j]) <= r * r);
        }
    }
}
// the damage expected on the way a -> (via) -> b at speed vt, through the unload at b, as a share of hp; *secs the
// sailing time
internal float cx_way(const struct cx_st *c, ivec2 a, const ivec2 *via, ivec2 b, float vt, float hp, unsigned ocean, float *secs) {
    static vec2 pt[256];
    static float tm[256];
    int n = 0;
    ivec2 legs[3] = { a, via ? *via : b, b };
    int nl = via ? 3 : 2;
    float t = 0;
    for (int l = 0; l + 1 < nl; l++) {
        float dx = (float)(legs[l + 1].x - legs[l].x), dy = (float)(legs[l + 1].y - legs[l].y), len = sqrtf(dx * dx + dy * dy);
        int steps = (int)(len / 2.0f) + 1;
        for (int i = l ? 1 : 0; i <= steps && n < 256; i++) {
            float f = (float)i / (float)steps;
            pt[n] = v2((float)legs[l].x + dx * f, (float)legs[l].y + dy * f);
            tm[n++] = t + len * f / vt;
        }
        t += len / vt;
    }
    if (secs) *secs = t;
    float dmg = 0, end = t + CX_UNLOAD;
    g_cx_top = -1; g_cx_top_dmg = 0;
    for (int i = 0; i < c->n; i++) {
        const struct cx_threat *e = &c->th[i];
        if (e->v > 0 && !e->air && ocean && area_id(e->p, 1) != ocean) continue;   // a ship on another sea
        float w = e->busy ? 0.25f : 1.0f, exp_ = 0;
        if (e->v <= 0) {   // standing: while the way lies in its reach, and the unload if it ends there
            for (int k = 0; k < n; k++) {
                float dx = pt[k].x - (float)e->p.x, dy = pt[k].y - (float)e->p.y;
                if (dx * dx + dy * dy <= e->r * e->r) exp_ += (k ? tm[k] - tm[k - 1] : 0) + (k == n - 1 ? CX_UNLOAD : 0);
            }
        } else {   // moving: from the first point within its sight - as fast, it follows; slower, while it passes
            for (int k = 0; k < n; k++) {
                float dx = pt[k].x - (float)e->p.x, dy = pt[k].y - (float)e->p.y, d = sqrtf(dx * dx + dy * dy) - e->r;
                if (d > CX_NOTICE) continue;
                float close = d > 0 ? d / e->v : 0, pass = 2.0f * e->r / vt, left = end - tm[k] - close;
                exp_ = left <= 0 ? 0 : e->v >= vt * 0.9f ? left : (pass < left ? pass : left);
                break;
            }
        }
        dmg += e->dps * exp_ * w;
        if (e->dps * exp_ * w > g_cx_top_dmg) { g_cx_top_dmg = e->dps * exp_ * w; g_cx_top = i; }
    }
    return hp > 0 ? dmg / hp : 0;
}
// the way for transport sh from the water a to the water b on sea `ocean`: its risk (the expected damage before the
// load is off, a share of its hit points); *via the point off the line it goes through first, *has_via 0: straight
internal float crossing_rate(ee_player *pl, unsigned idx, ee_unit *sh, ivec2 a, ivec2 b, unsigned ocean, ivec2 *via, int *has_via) {
    ee_unit_type *td = unit_get_type_data(sh);
    struct cx_st *c = &g_cx_st[idx & 15];
    uint32_t now = game_time();
    cx_scan(pl, idx, td, now);
    ee_unit_stats *st = unit_get_stats(sh);
    float vt = st ? unit_stats_get_speed(st) : 0, hp = (float)unit_get_hp(sh), secs;
    if (vt < 0.3f) vt = 0.3f;
    c->rated++;
    float best = cx_way(c, a, NULL, b, vt, hp, ocean, &secs), bscore = best + secs / 60.0f * CX_TIME, tdmg = g_cx_top_dmg;
    int top = g_cx_top;
    *has_via = 0;
    if (best <= 0) return 0;
    ivec2 via0 = *via;
    float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y), len = sqrtf(dx * dx + dy * dy);
    if (len < 4.0f) return best;
    ivec2 mid = iv2((a.x + b.x) / 2, (a.y + b.y) / 2);
    for (int s = -2; s <= 2; s++) {
        if (!s) continue;
        ivec2 q = iv2(mid.x + (int)(-dy / len * (float)(CX_SIDE * s)), mid.y + (int)(dx / len * (float)(CX_SIDE * s))), w;
        if (!fl_water(q, ocean, &w)) continue;
        float r = cx_way(c, a, &w, b, vt, hp, ocean, &secs), score = r + secs / 60.0f * CX_TIME;
        if (score < bscore) { bscore = score; best = r; *via = w; *has_via = 1; top = g_cx_top; tdmg = g_cx_top_dmg; }
    }
    if (*has_via) c->detours++; else *via = via0;
    g_cx_top = top; g_cx_top_dmg = tdmg;
    return best;
}
// TOGETHER (ferry.c): where a push's transports wait for each other off the water b they unload at, coming from a - as
// landing.c's staging: open water of the sea 10-18 tiles out towards a, 8 to either side too, the least damage a second
// a transport waiting there would take of what it knows (a ship's reach and sight, a defence's or coast gun's reach;
// one busy with its warships a quarter). 0: b is near enough - they come together anyway
internal int cx_meet(ee_player *pl, unsigned idx, ee_unit *sh, ivec2 a, ivec2 b, unsigned ocean, ivec2 *o) {
    struct cx_st *c = &g_cx_st[idx & 15];
    cx_scan(pl, idx, unit_get_type_data(sh), game_time());
    float dx = (float)(a.x - b.x), dy = (float)(a.y - b.y), len = sqrtf(dx * dx + dy * dy), best = 1e9f;
    if (len < 20.0f) return 0;
    dx /= len; dy /= len;
    int found = 0;
    for (int r = 10; r <= 18; r += 4)
        for (int s = -1; s <= 1; s++) {
            ivec2 w, q = iv2(b.x + (int)(dx * (float)r - dy * (float)(8 * s)), b.y + (int)(dy * (float)r + dx * (float)(8 * s)));
            if (!fl_water(q, ocean, &w) || ivec2_dist2(w, b) < 64) continue;
            float dps = 0;
            for (int i = 0; i < c->n; i++) {
                const struct cx_threat *e = &c->th[i];
                if (e->v > 0 && !e->air && ocean && area_id(e->p, 1) != ocean) continue;   // a ship on another sea
                float reach = e->r + (e->v > 0 ? CX_NOTICE : 0);
                if ((float)ivec2_dist2(w, e->p) <= reach * reach) dps += e->dps * (e->busy ? 0.25f : 1.0f);
            }
            float sc = dps + 0.01f * (float)r + (s ? 0.005f : 0);   // alike: the nearer, on the line
            if (sc < best) { best = sc; *o = w; found = 1; }
        }
    return found;
}
// the log's word on the threat g_cx_top of AI idx's last rating
internal void cx_top_text(unsigned idx, char *b, size_t n) {
    const struct cx_st *c = &g_cx_st[idx & 15];
    if (g_cx_top < 0 || g_cx_top >= c->n) { snprintf(b, n, "none"); return; }
    const struct cx_threat *e = &c->th[g_cx_top];
    snprintf(b, n, "%.0f damage from %s at %d,%d (reach %.1f, %.1f a second, speed %.2f%s%s)", (double)g_cx_top_dmg,
             e->air ? "an aircraft" : e->v > 0 ? "a ship" : e->old ? "a remembered ship or a loss's place" : "a defence or storm",
             e->p.x, e->p.y, (double)e->r, (double)e->dps, (double)e->v, e->busy ? ", busy" : "", e->old ? ", remembered" : "");
}
