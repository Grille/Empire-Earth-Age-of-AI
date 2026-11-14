// THE TERRITORIES AS AN AI SEES THEM (game/territory.c splits the map; the player 2026-11-13: "the ai can than decide if
// it want an teretory, armies ... are stationed in one or send to move to one" - and, the split looked at: "the default
// 40 look good, we can continue"). The first part of the AI that decides by them: the citizens of a territory that is
// being lost ([ai_base] territory_shelter; [autotest] v_territory_shelter for the variant players).
// What was wrong (expand.c THE OUTPOST, 2026-11-13): 57-347 citizens killed at the outposts a 12 games, 5-11 enemy land
// soldiers near each kill against the outpost's own 1-3. citizens_safety (attack.c) decides citizen by citizen and site
// by site - one whose site lies beside its drop site is "at home already" and works on, one sent back walks 5 tiles
// and is given the same mine again. Nobody says "this place is being lost".
// Every TV_EVERY per AI, honestly (what it sees now and the soldiers it saw there in the last TV_MEMORY - not
// territory.c's facts, which are the truth):
//   a territory's foes - enemy land soldiers in it or within TV_NEAR tiles of it (a fight knows no border: a drop
//             site 3 tiles across one was the first "refuge"); TV_FORCE of them and it is looked at closer (4 at
//             first: 3 soldiers killed two landings of 8 citizens with a guard of 1 each before it counted);
//   its guard - the AI's land soldiers in it and its defences standing in it;
//   UNSAFE  - the guard loses (stats.c st_ratio under TV_LOSING), until it wins (TV_HOLD) or the foes are gone, and
//             for TV_CALM after the last look that said so.
// While a territory is unsafe (level 1): every citizen in it goes into a Fortress with room on its land within TV_REACH
// tiles (bunker.c's shelter, let out when the territory is safe again), else to its nearest drop site on the same land
// in a territory that is not (none on a one-territory island: then as before); the site searches give no site in it
// (sites.c), so nobody is sent back in. Level 2 (the setting's): an outpost with the fort option builds a Fortress
// right after its drop site (expand.c XK_SHELTER) - without one an island's citizens have nowhere to go (after the
// second tower at first: laid 14 minutes after the landing; of 241 citizens killed at outposts 215 died before one stood).
// MEASURED 2026-11-14 (head-to-head, mirrored, 12 games a cell, 60 min, Small Islands from epoch 9, 4 AIs, the standard
// map and map_scale=large, with - without per game; citizens killed A -> B summed over the cells):
//   under 1.0, calm 20 s, level 2 (Fortress after the second tower; 4 cells, seeds 1-12): citizens 1305 -> 1053,
//     soldiers -3 (se 7), +8 (9), -7 (7), -8 (10), losses -1, -16 (6), +1, -4, workers +3, +3, +8, 0;
//   the same, level 1 (2 cells): citizens 775 -> 484, soldiers -3 (11), -7 (8), workers +4 (5), +4 (6);
//   under 1.5 and safe again from 3.0, calm 45 s (one outpost's trace: 12 soldiers against its 3 towers came out 1.9 -
//     each tower counted as if it covered the fight - the Fortress let 14 citizens out, dead 3 minutes later; the
//     state turned 8 times in 15 minutes), Fortress right after the drop site, level 2 (2 cells): citizens 739 -> 547,
//     soldiers -5 (6), -14 (13), workers -12 (6), -4 (7);
//   that with the Fortress's wood reserved (expand.c expand_hold: 8-9 built a 12 games instead of 2-4), level 2 (4
//     cells): citizens 1532 -> 1281, soldiers -7 (9), -11 (11), +2 (12), -5 (13), workers -14 (7), -7 (12), -5 (6), -7 (4);
//   that at level 1 (2 cells): citizens 634 -> 655, soldiers -4 (10), -1 (11), workers -2 (6), 0 (6).
// All 192 games: citizens killed 4985 -> 4020 (-19%), at the outposts up to -46% - and the army no larger for it: 11
// of 14 cells a few soldiers fewer (-5 on average, each within its noise). The outpost's Fortress costs workers and
// comes too late all the same: with the wood reserved 339 of 399 citizens killed at outposts still died before one
// stood, a quarter within a minute or two of the landing (soldiers already on the island: nothing is built yet).
// The holding-on saved no more citizens. So: OFF; the setting means level 1 with the first bars, level 2 for the tests
// (v_territory_shelter=2). What the shelter can't mend is the landing itself - where to land and with what guard.
// THE VIEW ALWAYS RUNS (2026-11-14, the armies next - the player: "turn it on ... than continue with armies"): the
// counting and the judging above for every AI whatever the shelter's setting (tv_unsafe alone asks that), so that
// what comes next can read it. Read only so far: each territory it HOLDS (TV_HELD or more of its buildings) with the
// guard it has there and the guard it would want - the towns' and outposts' rule (a third of its citizens there, 3
// at least, 8 when enemy soldiers were seen there in the last TV_FOE_MEMORY, TV_GUARD at most) - in the log
// (# tview: ... holds) and on the overlay; armies.c and ai_overlay.c name the territory each army is in and bound
// for.
// FRONT AND BACKLAND (2026-11-14, read only so far). The player, asked how he splits his army between holding ground
// and the main force: "the backland is nerly undefended in my case, my focus is on not letting anething go so far in,
// that includes islands too, piece by piece they become backland too as i gain more map control, water for the ai:
// terretories can also be held and guarded". So each AI's territories get a ROLE: a land territory it holds, or a
// water territory it holds (TV_SHIPS of its warships there, more than twice the enemy's it sees there), is a
// FRONT when it borders ground or water that neither it nor an ally holds - the open border (its own kind) and the
// shore (the other kind) counted in tiles - and BACKLAND when everything round it is held. An island is a front
// until the water round it is held; a sea is a front until the shores and seas beyond are.
#define TV_SHIPS 2
enum { TVR_NONE, TVR_FRONT, TVR_BACK };
#define TV_HELD 3
#define TV_GUARD 12
#define TV_FOE_MEMORY 300000
#define TV_EVERY 2000
#define TV_MEMORY 60000
#define TV_FORCE 2
#define TV_NEAR 8                 // an enemy soldier counts for every territory this near to it
#define TV_LOSING 1.0f            // the guard under this against the foes: being lost
#define TV_HOLD 1.0f              // ... and safe again from this on (the same: no holding on - MEASURED below)
#define TV_CALM 20000
#define TV_HOT 8                  // territories looked at closer a pass
enum { TV_QUIET, TV_FOES, TV_UNSAFE };
struct tv_terr { uint8_t state, role, friendly; short foe, sold, def, cit, bld, want, ships, eships, abld, open, shore; float ratio; uint32_t unsafe_t, foe_t, eship_t; };   // bld: its buildings there; want: the guard a held one would want; foe_t: enemy soldiers seen there last
internal struct tv_st {
    unsigned version; int n; struct tv_terr *t;
    uint32_t tick, log_t;
    unsigned turned, sheltered, refuge, refused, kept_in;   // the log's: territories turned unsafe, citizens into a fortress, sent to a refuge, sites refused, fortresses kept shut
} g_tv[16];
internal int g_ter_shelter = 0, g_v_ter_shelter = -1;
#define TER_SHELTER(idx) PV(idx, g_v_ter_shelter, g_ter_shelter)
internal struct st_side g_tv_own, g_tv_foe;
internal void tv_border_towers(ee_player *pl, unsigned idx, uint32_t now);   // BORDER TOWERS (below)

// an enemy land soldier of pl's that it sees
internal int tv_foe_unit(ee_player *pl, ee_unit *u, ee_unit_type *td) {
    ee_player *o = unit_get_owner(u);
    int fam = unit_type_get_class_id(td);
    return o && o != pl && (fam == CLS_LAND || fam == CLS_LAND_MILITARY_HERO) && unit_type_get_attack(td) > 0 && !is_citizen_type(unit_type_get_id(td)) &&
           player_get_index(o) > 0 && is_enemy(pl, o) && seen_by(u, pl);
}
// the territories a soldier at (x, y) counts for: its own and those within TV_NEAR tiles (the eight ways); the count
internal int tv_reach(int x, int y, unsigned *out) {
    int n = 0;
    for (int k = -1; k < 8; k++) {
        unsigned r = k < 0 ? ter_near(x, y) : territory_at(x + TV_NEAR * k_rm_dx[k], y + TV_NEAR * k_rm_dy[k]);
        int seen = r == TER_NONE || g_ter.t[r].water;
        for (int j = 0; j < n && !seen; j++) seen = out[j] == r;
        if (!seen) out[n++] = r;
    }
    return n;
}
internal int tv_reaches(int x, int y, unsigned terr) {
    unsigned r[9];
    int n = tv_reach(x, y, r);
    for (int j = 0; j < n; j++) if (r[j] == terr) return 1;
    return 0;
}
internal int tv_warship(ee_unit_type *td) {
    int fam = unit_type_get_class_id(td), ty = unit_type_get_id(td);
    return (fam == CLS_NAVAL || fam == CLS_SUBMERGED_NAVAL) && unit_type_get_attack(td) > 0 && !is_transport(ty) && !is_fishing_boat(ty);
}
internal int tv_own_soldier(ee_unit_type *td) { return is_soldier(td) && !is_ship_or_air(td) && !is_transport(unit_type_get_id(td)); }
internal void tv_tick(ee_player *pl, unsigned idx, uint32_t now) {
    struct tv_st *s = &g_tv[idx];
    if (now < s->tick) { free(s->t); memset(s, 0, sizeof *s); }   // a new game
    if (s->tick && now - s->tick < TV_EVERY) return;
    s->tick = now ? now : 1;
    if (!ter_ready()) return;
    if (!s->t || s->version != g_ter.version || s->n != g_ter.n) {
        free(s->t);
        s->t = calloc((size_t)g_ter.n + 1, sizeof *s->t);
        s->version = g_ter.version; s->n = s->t ? g_ter.n : 0;
    }
    if (!s->n) return;
    for (int r = 0; r < s->n; r++) { s->t[r].foe = s->t[r].sold = s->t[r].def = s->t[r].cit = s->t[r].bld = s->t[r].ships = s->t[r].eships = s->t[r].abld = 0; s->t[r].ratio = 0; }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        float rng, att;
        if (unit_get_owner(u) != pl) {
            unsigned reach[9];
            int nn = tv_foe_unit(pl, u, it.td) ? tv_reach(unit_get_x(u), unit_get_y(u), reach) : 0;
            for (int j = 0; j < nn; j++) if (reach[j] < (unsigned)s->n) s->t[reach[j]].foe++;
            ee_player *o = unit_get_owner(u);   // the enemy's warships it sees, an ally's buildings
            unsigned ro = o && player_get_index(o) > 0 ? ter_near(unit_get_x(u), unit_get_y(u)) : TER_NONE;
            if (ro < (unsigned)s->n) {
                int ofam = unit_type_get_class_id(it.td);
                if (is_enemy(pl, o)) { if (tv_warship(it.td) && seen_by(u, pl)) { s->t[ro].eships++; s->t[ro].eship_t = now ? now : 1; } }
                else if (ofam == CLS_BUILDING || ofam == CLS_FORTRESS) s->t[ro].abld++;
            }
            continue;
        }
        unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
        if (r >= (unsigned)s->n) continue;
        {
            int fam = unit_type_get_class_id(it.td);
            if (is_citizen_type(unit_type_get_id(it.td))) s->t[r].cit++;
            else if (fam == CLS_BUILDING || fam == CLS_FORTRESS) { s->t[r].bld++; s->t[r].def += defence_stats(u, &rng, &att) != 0; }
            else if (tv_own_soldier(it.td)) s->t[r].sold++;
            else if (tv_warship(it.td)) s->t[r].ships++;
        }
    }
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n; k++) {   // the soldiers it saw there and doesn't see now
        struct intel_rec *q = &in->r[k];
        ee_player *o = q->owner > 0 && q->owner < 16 ? player_at(q->owner) : NULL;
        if (q->kind != 1 || q->x < 0 || !o || !is_enemy(pl, o) || q->t == in->last || now < q->t || now - q->t > TV_MEMORY) continue;
        unsigned reach[9];
        int nn = tv_reach(q->x, q->y, reach);
        for (int j = 0; j < nn; j++) if (reach[j] < (unsigned)s->n) s->t[reach[j]].foe++;
    }
    int hot[TV_HOT], nh = 0;
    for (int r = 0; r < s->n; r++) if (s->t[r].foe >= TV_FORCE && (s->t[r].cit || s->t[r].def || s->t[r].sold) && nh < TV_HOT) hot[nh++] = r;
    for (int h = 0; h < nh; h++) {   // guard against foes, each such territory
        struct tv_terr *t = &s->t[hot[h]];
        st_side_clear(&g_tv_own); st_side_clear(&g_tv_foe);
        ivec2 c = g_ter.t[hot[h]].c;
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || abs(unit_get_x(u) - c.x) > 120 || abs(unit_get_y(u) - c.y) > 120) continue;
            float rng, att;
            if (unit_get_owner(u) != pl) {
                if (tv_foe_unit(pl, u, it.td) && tv_reaches(unit_get_x(u), unit_get_y(u), (unsigned)hot[h])) st_side_add(&g_tv_foe, u);
                continue;
            }
            if (ter_near(unit_get_x(u), unit_get_y(u)) != (unsigned)hot[h]) continue;
            {
                int fam = unit_type_get_class_id(it.td);
                if ((fam == CLS_BUILDING || fam == CLS_FORTRESS) ? defence_stats(u, &rng, &att) != 0 : tv_own_soldier(it.td)) st_side_add(&g_tv_own, u);
            }
        }
        for (int k = 0; k < in->n; k++) {
            struct intel_rec *q = &in->r[k];
            ee_player *o = q->owner > 0 && q->owner < 16 ? player_at(q->owner) : NULL;
            if (q->kind != 1 || q->x < 0 || !o || !is_enemy(pl, o) || q->t == in->last || now < q->t || now - q->t > TV_MEMORY) continue;
            if (tv_reaches(q->x, q->y, (unsigned)hot[h])) st_side_add_rec(&g_tv_foe, q->type, q->owner, q->hp, q->x, q->y);
        }
        t->ratio = st_ratio(&g_tv_own, &g_tv_foe);
        if (t->ratio < (t->state == TV_UNSAFE ? TV_HOLD : TV_LOSING)) t->unsafe_t = now ? now : 1;
    }
    for (int r = 0; r < s->n; r++) {
        struct tv_terr *t = &s->t[r];
        int was = t->state;
        t->state = (uint8_t)(t->unsafe_t && now >= t->unsafe_t && now - t->unsafe_t < TV_CALM ? TV_UNSAFE : t->foe ? TV_FOES : TV_QUIET);
        if (t->state != TV_UNSAFE) t->unsafe_t = 0;
        if ((was == TV_UNSAFE) == (t->state == TV_UNSAFE)) continue;
        s->turned += t->state == TV_UNSAFE;
        if (g_log && g_autotest) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# tview: P%u at %.1f min: territory %d at %d,%d %s: %d enemy soldiers against %d soldiers and %d defences (ratio %.2f), %d citizens there\n", idx, now / 60000.0, r,
                    g_ter.t[r].c.x, g_ter.t[r].c.y, t->state == TV_UNSAFE ? "is being lost" : "safe again", t->foe, t->sold, t->def, (double)t->ratio, t->cit);
            LeaveCriticalSection(&g_lock);
        }
    }
    for (int r = 0; r < s->n; r++) {   // the guard a held one would want (read only so far)
        struct tv_terr *t = &s->t[r];
        if (t->foe) t->foe_t = now ? now : 1;
        int w = t->cit / 3 > 3 ? t->cit / 3 : 3;
        if (t->foe_t && now >= t->foe_t && now - t->foe_t < TV_FOE_MEMORY && w < 8) w = 8;
        t->want = (short)(!g_ter.t[r].water && t->bld >= TV_HELD ? (w < TV_GUARD ? w : TV_GUARD) : 0);   // land: a sea is held by ships (below)
    }
    for (int r = 0; r < s->n; r++) {   // FRONT AND BACKLAND: what it or an ally holds ...
        struct tv_terr *t = &s->t[r];
        // a sea: TV_SHIPS of its warships there and more than twice the enemy's it sees there now (no enemy warship seen
        // there for 5 minutes at first: with 4 AIs on 5 seas one always passes - 11 to 23 warships a player at minute 45
        // and no sea held, in 77 of 80 looks)
        t->role = (uint8_t)(g_ter.t[r].water ? t->ships >= TV_SHIPS && t->ships > 2 * t->eships : t->bld >= TV_HELD);   // its own (the role below)
        t->friendly = (uint8_t)(t->role || (!g_ter.t[r].water && t->abld >= TV_HELD));
    }
    for (int r = 0; r < s->n; r++) {   // ... and of its own, which border what nobody of its side holds
        struct tv_terr *t = &s->t[r];
        t->open = t->shore = 0;
        if (!t->role) continue;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            struct ter_nb *nb = &g_ter.nb[g_ter.t[r].nb0 + k];
            if (nb->to >= (unsigned)s->n || s->t[nb->to].friendly) continue;
            if (g_ter.t[nb->to].water == g_ter.t[r].water) t->open = (short)(t->open + nb->len); else t->shore = (short)(t->shore + nb->len);
        }
        t->role = (uint8_t)(t->open || t->shore ? TVR_FRONT : TVR_BACK);
    }
    tv_border_towers(pl, idx, now);
    static int layer = -2;
    if (layer == -2) layer = overlay_layer("territories");
    if (overlay_on(layer)) {   // the unsafe ones marked in the AI's colour, the held ones short of their guard named
        overlay_begin(layer, 0x100 + idx, now, 3 * TV_EVERY);
        for (int r = 0; r < s->n; r++) {
            if (s->t[r].state != TV_UNSAFE && s->t[r].role) {   // its role, its guard
                vec2 rc = v2((float)g_ter.t[r].c.x + 0.5f, (float)g_ter.t[r].c.y + 0.5f);
                uint32_t col = overlay_player_argb((int)idx, 255);
                if (s->t[r].role == TVR_BACK) overlay_text(rc, -1 - (int)(idx & 3), col, "P%u backland", idx);
                else if (g_ter.t[r].water) overlay_text(rc, -1 - (int)(idx & 3), col, "P%u sea front %d+%d, %d ships", idx, s->t[r].open, s->t[r].shore, s->t[r].ships);
                else overlay_text(rc, -1 - (int)(idx & 3), col, "P%u front %d+%d, guard %d of %d", idx, s->t[r].open, s->t[r].shore, s->t[r].sold, s->t[r].want);
            }
            if (s->t[r].state != TV_UNSAFE) continue;
            vec2 c = v2((float)g_ter.t[r].c.x + 0.5f, (float)g_ter.t[r].c.y + 0.5f);
            overlay_mark(c, OVM_TRIANGLE, 14.0f, overlay_player_argb((int)idx, 255));
            overlay_text(c, -1 - (int)(idx & 3), overlay_player_argb((int)idx, 255), "P%u: lost here, %d foes, %.1f", idx, s->t[r].foe, (double)s->t[r].ratio);
        }
        overlay_end();
    }
    if (g_log && g_autotest && (!s->log_t || now - s->log_t >= 300000)) {
        s->log_t = now ? now : 1;
        int unsafe = 0, foes = 0;
        for (int r = 0; r < s->n; r++) { unsafe += s->t[r].state == TV_UNSAFE; foes += s->t[r].state == TV_FOES; }
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# tview: P%u at %.1f min: %d territories, %d with enemy soldiers, %d being lost; so far %u turned unsafe, %u citizens into a fortress, %u sent to a refuge, %u sites refused, %u looks a fortress was kept shut\n",
                idx, now / 60000.0, s->n, foes, unsafe, s->turned, s->sheltered, s->refuge, s->refused, s->kept_in);
        fprintf(g_log, "# tview: P%u at %.1f min: holds", idx, now / 60000.0);
        int held = 0, short_of = 0;
        for (int r = 0; r < s->n; r++) {
            if (!s->t[r].want) continue;
            held++; short_of += s->t[r].want > s->t[r].sold ? s->t[r].want - s->t[r].sold : 0;
            if (held <= 16) {
                fprintf(g_log, " | territory %d: %d buildings, %d citizens, guard %d of %d%s", r, s->t[r].bld, s->t[r].cit, s->t[r].sold, s->t[r].want, s->t[r].foe ? ", enemies there" : "");
                if (s->t[r].role == TVR_FRONT) fprintf(g_log, ", a front (open border %d, shore %d)", s->t[r].open, s->t[r].shore); else fprintf(g_log, ", backland");
            }
        }
        int seas = 0, sea_fronts = 0, lfronts = 0;
        for (int r = 0; r < s->n; r++) { lfronts += !g_ter.t[r].water && s->t[r].role == TVR_FRONT; if (g_ter.t[r].water && s->t[r].role) { seas++; sea_fronts += s->t[r].role == TVR_FRONT; } }
        fprintf(g_log, " | %d of them fronts; seas held %d (%d fronts)", lfronts, seas, sea_fronts);
        {   // its warships by sea: where they stand, and what keeps a sea from being held
            int wsum = 0, wmost = 0, wn = 0, wfoe = 0;
            for (int r = 0; r < s->n; r++) {
                if (!g_ter.t[r].water || !s->t[r].ships) continue;
                wsum += s->t[r].ships; wn++; if (s->t[r].ships > wmost) wmost = s->t[r].ships;
                wfoe += s->t[r].ships >= TV_SHIPS && s->t[r].ships <= 2 * s->t[r].eships;
            }
            fprintf(g_log, "; its %d warships stand in %d seas (most in one: %d), %d of those with 2 or more not held for the enemy warships there", wsum, wn, wmost, wfoe);
        }
        fprintf(g_log, " | %d territories held, %d soldiers short of the guards wanted\n", held, short_of);
        LeaveCriticalSection(&g_lock);
    }
}
// ---- BORDER TOWERS ([ai_base] border_towers; [autotest] v_border_towers) ----
// The player, 2026-11-14: "placing towers and walls along outside borders, tousends of stone or wood in the bank are
// useless, player completly cover their shores with towers for example, a bit of peace of mind in case anyone tries to
// sneak in from behind. stuff like that wont pay in the tests, since sneak attacks from behind arent really in the ai
// yet". So this part is NOT judged by the AI-against-AI games.
// While BW_STONE of stone and BW_WOOD of wood lie in the bank: every BW_EVERY one tower on the outside border of a
// territory it holds (3 of its citizens there, not being lost) - a tile of it within 2 of water or of land nobody of
// its side holds - where no tower of its stands within BW_SPACE tiles; the place nearest that territory's middle
// first, so the cover grows from the core outwards and ends as a tower every BW_SPACE tiles of shore and open border.
// Walls: not yet (the ring of defence.c THE TOWN RING is the place for them).
#define BW_EVERY 20000
#define BW_STONE 1500
#define BW_WOOD 1500
#define BW_SPACE 12
#define BW_TOWERS 512
internal int g_border_towers = 0, g_v_border_towers = -1;
#define BORDER_TOWERS(idx) PV(idx, g_v_border_towers, g_border_towers)
internal struct { uint32_t t; unsigned queued, no_place; int ntw; ivec2 tw[BW_TOWERS]; struct { ivec2 p; uint32_t t; } bad[32]; int nbad; } g_bw[16];   // bad: places no tower could be put by, not tried again for 5 minutes
internal void tv_border_towers(ee_player *pl, unsigned idx, uint32_t now) {
    struct tv_st *s = &g_tv[idx];
    if (!BORDER_TOWERS(idx) || !s->n || (g_bw[idx].t && now >= g_bw[idx].t && now - g_bw[idx].t < BW_EVERY)) return;
    g_bw[idx].t = now ? now : 1;
    const int *stock = player_stock_at(pl);
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    int e = current_epoch(pl), ttype = tower_newest(pl), size = g_ter.size;
    if (!bm || !ttype || e < 0 || e >= 14 || stock[4] < BW_STONE || stock[3] < BW_WOOD || tower_queued(bm)) return;
    g_bw[idx].ntw = 0;   // its towers (and other defences) as they stand
    for (ee_unit *u = player_get_units(pl); u && g_bw[idx].ntw < BW_TOWERS; u = unit_get_next(u)) {
        float rng, att;
        ee_unit_type *td = unit_get_type_data(u);
        int fam = td ? unit_type_get_class_id(td) : 0;
        if ((fam == CLS_BUILDING || fam == CLS_FORTRESS) && unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && defence_stats(u, &rng, &att)) g_bw[idx].tw[g_bw[idx].ntw++] = unit_get_tile(u);
    }
    ivec2 best = iv2(-1, -1);
    long bd = 0x7FFFFFFF;
    for (int y = 3; y < size - 3; y += 3)
        for (int x = 3; x < size - 3; x += 3) {
            unsigned r = g_ter.lab[y * size + x];
            if (r >= (unsigned)s->n || g_ter.t[r].water || s->t[r].role == TVR_NONE || s->t[r].cit < 3 || s->t[r].state == TV_UNSAFE) continue;
            int edge = 0;
            for (int d = 0; d < 8 && !edge; d += 2) {
                unsigned o = g_ter.lab[(y + 2 * k_rm_dy[d]) * size + x + 2 * k_rm_dx[d]];
                edge = o != r && o < (unsigned)s->n && (g_ter.t[o].water || !s->t[o].friendly);
            }
            if (!edge) continue;
            long dc = ivec2_dist2(iv2(x, y), g_ter.t[r].c);
            if (dc >= bd) continue;
            int covered = 0;
            for (int k = 0; k < g_bw[idx].ntw && !covered; k++) covered = ivec2_dist2(g_bw[idx].tw[k], iv2(x, y)) <= BW_SPACE * BW_SPACE;
            for (int k = 0; k < 32 && !covered; k++) covered = g_bw[idx].bad[k].t && now - g_bw[idx].bad[k].t < 300000 && ivec2_dist2(g_bw[idx].bad[k].p, iv2(x, y)) <= 16;
            if (!covered) { bd = dc; best = iv2(x, y); }
        }
    if (best.x < 0 || raiders_near(pl, best, 12)) return;
    for (int r = 0; r <= 3; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                ivec2 q = ivec2_add(best, iv2(ax, ay));
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, ttype, q)) continue;
                int entry[3] = { ttype, q.x, q.y };
                build_mgr_queue_add(bm, e, entry);
                g_bw[idx].queued++;
                if (g_log && (g_autotest || !(g_bw[idx].queued % 10))) fprintf(g_log, "# border: P%u at %.1f min: tower %d queued at %d,%d on the outside border of its territory %u (stone %d, wood %d in the bank; %d towers standing, %u queued so far)\n",
                                                                                 idx, now / 60000.0, ttype, q.x, q.y, g_ter.lab[best.y * size + best.x], stock[4], stock[3], g_bw[idx].ntw, g_bw[idx].queued);
                return;
            }
    g_bw[idx].bad[g_bw[idx].nbad++ & 31].p = best; g_bw[idx].bad[(g_bw[idx].nbad - 1) & 31].t = now ? now : 1;   // no place for a tower by it: the next one the next time
    g_bw[idx].no_place++;
    g_bw[idx].t = now - BW_EVERY + 2000;
}
// is the territory of tile (x, y) being lost for AI idx (0 too: the setting off, no look yet, no territory there)
internal int tv_unsafe(unsigned idx, int x, int y) {
    struct tv_st *s = &g_tv[idx & 15];
    if (!s->n || !TER_SHELTER(idx & 15) || s->version != g_ter.version) return 0;
    unsigned r = ter_near(x, y);
    return r < (unsigned)s->n && s->t[r].state == TV_UNSAFE;
}
// where a citizen at p goes when its territory is being lost and no fortress takes it: its nearest drop site on the
// same land in a territory that is not, with no raiders by it; 0 none
internal int tv_refuge(ee_player *pl, unsigned idx, ivec2 p, ivec2 *out) {
    drops_cached(pl);
    unsigned land = area_id(p, 2);
    int best = -1, bd = 0x7FFFFFFF;
    for (int k = 0; k < g_drops[idx].n; k++) {
        ivec2 d = g_drops[idx].d[k].p;
        int dist = ivec2_dist2(d, p);
        if (dist >= bd || tv_unsafe(idx, d.x, d.y) || raiders_known(pl, d, 12)) continue;
        if (!land || area_id(d, 2) != land) continue;   // on its own land only: a citizen walks
        bd = dist; best = k;
    }
    if (best < 0) return 0;
    *out = g_drops[idx].d[best].p;
    return 1;
}
// armies.c, ai_overlay.c: the territory of a tile for naming it (TER_NONE: none, or nothing built yet), and the enemy
// soldiers AI idx counts in or by a territory (-1: not known)
internal unsigned tv_terr_of(ivec2 p) { return g_ter.lab ? ter_near(p.x, p.y) : TER_NONE; }
internal int tv_foes(unsigned idx, unsigned terr) {
    struct tv_st *s = &g_tv[idx & 15];
    return s->n && s->version == g_ter.version && terr < (unsigned)s->n ? s->t[terr].foe : -1;
}
internal int tv_holds(unsigned idx, ivec2 p) {   // armies.c HELD GROUND: AI idx holds the land territory of that tile
    struct tv_st *s = &g_tv[idx & 15];
    unsigned r = g_ter.lab && s->n && s->version == g_ter.version ? ter_near(p.x, p.y) : TER_NONE;
    return r < (unsigned)s->n && !g_ter.t[r].water && s->t[r].role != TVR_NONE;
}
// armies.c FRONT STATION: where the force stands - the land front on its base's land with the heaviest open border
// (each border's tiles x (1 + the enemy soldiers it knows beyond it), the front's sum x (1 + its buildings there / 8)),
// a third of the way from that territory's middle towards the neighbour the heaviest border is with; 0: no land front
// there (an island: its fronts are shores - the ships' matter)
internal int tv_front_station(ee_player *pl, unsigned idx, ivec2 *out) {
    struct tv_st *s = &g_tv[idx & 15];
    ivec2 base;
    if (!s->n || s->version != g_ter.version || !ms_base(pl, &base)) return 0;
    unsigned land = area_id(base, 2);
    int best = -1, bto = -1;
    float bw = 0;
    for (int r = 0; r < s->n; r++) {
        if (g_ter.t[r].water || s->t[r].role != TVR_FRONT || !s->t[r].open || area_id(g_ter.t[r].c, 2) != land) continue;
        float sum = 0, most = 0;
        int to = -1;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            struct ter_nb *nb = &g_ter.nb[g_ter.t[r].nb0 + k];
            if (nb->to >= (unsigned)s->n || g_ter.t[nb->to].water || s->t[nb->to].friendly) continue;
            float w = (float)nb->len * (1.0f + (float)s->t[nb->to].foe);
            sum += w;
            if (w > most) { most = w; to = nb->to; }
        }
        sum *= 1.0f + (float)s->t[r].bld / 8.0f;
        if (to >= 0 && sum > bw) { bw = sum; best = r; bto = to; }
    }
    if (best < 0) return 0;
    ivec2 c = g_ter.t[best].c, n = g_ter.t[bto].c;
    *out = c;
    for (int f = 35; f > 0; f -= 7) {
        ivec2 q = iv2(c.x + (n.x - c.x) * f / 100, c.y + (n.y - c.y) * f / 100);
        if (territory_at(q.x, q.y) == (unsigned)best) { *out = q; break; }
    }
    return 1;
}
// expand.c FRONT GUARDS: the land territory of tile p is a front of AI idx - its soldiers there and the guard wanted
internal int tv_front_guard(unsigned idx, ivec2 p, int *have, int *want) {
    struct tv_st *s = &g_tv[idx & 15];
    unsigned r = g_ter.lab && s->n && s->version == g_ter.version ? ter_near(p.x, p.y) : TER_NONE;
    if (r >= (unsigned)s->n || g_ter.t[r].water || s->t[r].role != TVR_FRONT) return 0;
    *have = s->t[r].sold; *want = s->t[r].want;
    return 1;
}
// fleet.c FLEET STATION: the water territory of sea `ocean` the fleet stands in - the one with the longest shore along
// land AI idx holds, x (1 + the enemy warships it sees there, + 2 when it saw any there in the last 5 minutes); its
// middle into *out and its number into *terr; 0: no sea by held land. The one it stands in (*terr on entry, TER_NONE
// none) stays unless another weighs a third more: 17 changes of station a player in a game without (a sighting here,
// a sighting there)
internal int tv_sea_station(unsigned idx, unsigned ocean, ivec2 *out, unsigned *terr) {
    struct tv_st *s = &g_tv[idx & 15];
    if (!s->n || s->version != g_ter.version || !ocean) return 0;
    uint32_t now = game_time();
    int best = -1;
    float bw = 0;
    for (int r = 0; r < s->n; r++) {
        if (!g_ter.t[r].water || area_id(g_ter.t[r].c, 1) != ocean) continue;
        int shore = 0;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            struct ter_nb *nb = &g_ter.nb[g_ter.t[r].nb0 + k];
            if (nb->to < (unsigned)s->n && !g_ter.t[nb->to].water && s->t[nb->to].role != TVR_NONE) shore += nb->len;
        }
        if (!shore) continue;
        float w = (float)shore * (1.0f + (float)s->t[r].eships + (s->t[r].eship_t && now >= s->t[r].eship_t && now - s->t[r].eship_t < TV_FOE_MEMORY ? 2.0f : 0.0f));
        if ((unsigned)r == *terr) w *= 1.34f;
        if (w > bw) { bw = w; best = r; }
    }
    if (best < 0) return 0;
    *out = g_ter.t[best].c; *terr = (unsigned)best;
    return 1;
}
// fleet.c HARBOURS: where the next harbour goes - a sea territory that borders land AI idx holds and has no dock of its
// on it yet (dock_seas[nd]: the sea territory each of its docks stands on), and the held land territory with the
// longest shore on that sea that has `cits` of its citizens and is not being lost. Of several such seas: the more so
// with enemy warships seen there, by a front, with the longer shore. *shore: the last tile of that land territory on
// the line from its middle to the sea's; 0: every sea by its land has a harbour
internal int tv_harbour_next(unsigned idx, const unsigned *dock_seas, int nd, int cits, ivec2 *shore, unsigned *land, unsigned *sea) {
    struct tv_st *s = &g_tv[idx & 15];
    if (!s->n || s->version != g_ter.version) return 0;
    uint32_t now = game_time();
    int bl = -1, bs = -1;
    float bw = 0;
    for (int w = 0; w < s->n; w++) {
        if (!g_ter.t[w].water) continue;
        int has = 0, to = -1, len = 0;
        for (int k = 0; k < nd && !has; k++) has = dock_seas[k] == (unsigned)w;
        if (has) continue;
        for (int k = 0; k < g_ter.t[w].nbn; k++) {   // the held land with the longest shore on it
            struct ter_nb *nb = &g_ter.nb[g_ter.t[w].nb0 + k];
            if (nb->to >= (unsigned)s->n || g_ter.t[nb->to].water) continue;
            struct tv_terr *t = &s->t[nb->to];
            if (t->role == TVR_NONE || t->cit < cits || t->state == TV_UNSAFE || nb->len <= len) continue;
            len = nb->len; to = nb->to;
        }
        if (to < 0) continue;
        float wt = (float)len * (s->t[to].role == TVR_FRONT ? 2.0f : 1.0f) * (s->t[w].eship_t && now >= s->t[w].eship_t && now - s->t[w].eship_t < 2 * TV_FOE_MEMORY ? 3.0f : 1.0f);
        if (wt > bw) { bw = wt; bl = to; bs = w; }
    }
    if (bl < 0) return 0;
    ivec2 c = g_ter.t[bl].c, wc = g_ter.t[bs].c, sp = c;
    float dx = (float)(wc.x - c.x), dy = (float)(wc.y - c.y), l = sqrtf(dx * dx + dy * dy);
    for (int i = 1; i < (int)l; i++) {
        ivec2 q = iv2(c.x + (int)(dx * (float)i / l), c.y + (int)(dy * (float)i / l));
        unsigned tq = territory_at(q.x, q.y);
        if (tq == (unsigned)bl) sp = q;
        else if (tq != TER_NONE && g_ter.t[tq].water) break;
    }
    *shore = sp; *land = (unsigned)bl; *sea = (unsigned)bs;
    return 1;
}
// ... enemy warships seen on that sea in the last 10 minutes: its harbour gets its second tower
internal int tv_sea_threat(unsigned idx, unsigned sea) {
    struct tv_st *s = &g_tv[idx & 15];
    uint32_t now = game_time();
    return s->n && s->version == g_ter.version && sea < (unsigned)s->n && s->t[sea].eship_t && now >= s->t[sea].eship_t && now - s->t[sea].eship_t < 2 * TV_FOE_MEMORY;
}
internal void tv_count(unsigned idx, int what) {   // the log's counters: 0 into a fortress, 1 to a refuge, 2 a site refused, 3 a fortress kept shut
    struct tv_st *s = &g_tv[idx & 15];
    if (what == 0) s->sheltered++; else if (what == 1) s->refuge++; else if (what == 2) s->refused++; else s->kept_in++;
}
internal int tv_outpost_fortress(unsigned idx) { return TER_SHELTER(idx & 15) >= 2; }   // expand.c: the outpost's Fortress
internal void territories_install(const char *autotest_ini) {
    g_ter_shelter = g_set_ai[1].territory_shelter ? 1 : 0;
    g_border_towers = g_set_ai[1].border_towers;
    if (autotest_ini) g_v_border_towers = ini_int("autotest", "v_border_towers", -1, autotest_ini);
    if (autotest_ini) g_v_ter_shelter = ini_int("autotest", "v_territory_shelter", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# territories: territory_shelter %d (variant %d)\n", g_ter_shelter, g_v_ter_shelter);
}
internal void territories_new_game(void) { for (int i = 0; i < 16; i++) { free(g_tv[i].t); memset(&g_tv[i], 0, sizeof g_tv[i]); } }
