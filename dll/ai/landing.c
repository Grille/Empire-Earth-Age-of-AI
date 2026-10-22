// LANDING ([all] landing, default 1 - the playtester: "landings must work, islands are quite popular"): a landing behind the enemy's lines. Included by eeaimod.c after
// forces.c (uses its area ids, home_threat, enemy_objective).
//
// The user (reference save): "my back is basically completely undefended - landing with ~3 full transports and a few
// citizens for a defended bridgehead could cause a lot of trouble; going over land means going through the heavily
// defended main front". The game's own transport handling hardly works (one transport per AI, loading for 8+
// minutes while its land subgroups wait elsewhere - docs/engine-notes.md). Here the mod does it itself:
// - Plan (every 30 s while idle, while an attack is on - also with an enemy army at its door): a landing site -
//   open land at the shore of the sea its dock lies on, on the continent of the target's buildings it has seen,
//   the weakest defended (the user: "the AI will rarely find a fully undefended spot - it just has to choose the
//   weakest one and bring an army that actually has a chance"), then the most buildings within 20 tiles; the
//   soldiers at home it can spare (land attack subgroups, one kept per enemy soldier at its door) 1.2 times as
//   strong as 1.4 times what holds the beach (ld_strength: groups, not units summed).
// - Size (the user: "early a single transport might do damage, later a large scale landing with 10 ships might be
//   needed"): 1.5 times what holds the beach and the soldiers it knows within 40 tiles behind it, as far as those it
//   can spare and the transports it has or can pay (stock now, one to save for, up to landing_transports, 8) go;
//   the transports full (their type's capacity +0x54C: 10-14), room for the citizens.
// - Ships: its transports (the newest type of its epoch), the missing ones trained (priority orders, their cost
//   reserved); not all there after 3 minutes: those it has if they carry enough; none within 6: given up for 10.
// - Board: whole land subgroups at home (their updates skipped while the landing leads them, as the transports'
//   subgroups), at a meeting point; the game's own load goal (EEUGLoadTransport 0x61487F on the transport with
//   the passenger list, each passenger the goal's vf4) - again every 30 s for those not aboard; all aboard, 9 in 10
//   after a minute, or 3 minutes: it sails with what is aboard (half at least, else given up). 3+ transports
//   gather at sea 10-18 tiles off the beach first (75 s at most) and run in together.
// - Sail and unload: EEUGUnloadTransport (0x615A1F, transport, tile x, y) at the site. Committed: no turning back
//   while transports with soldiers aboard are afloat (towers in the back can sink them - land before). Up to 4
//   warships go along (their subgroups led too): ahead to the water in front of the site, attacking the defences and
//   warships there so the towers shoot at them, then shelling the shore. Towers covering that water count against
//   a site (the beach itself must be out of every known tower's reach).
// - Ashore: one group - raids (ld_raid_target), else marches on the nearest building of the target it has seen that is
//   no defence. Citizens along build a forward base (ld_bridgehead: towers, barracks, town center, hospital, factory -
//   by the size; soldiers trained there join, ld_capture). Waves: the transports bring the soldiers
//   at home over while it holds (ld_wave, landing_waves 3). A quarter left: released to the game.
internal int g_land_bombard = 0;   // [all] landing_bombard (below: BOMBARD)
internal int g_landing = 1, g_land_ships = 8;   // aimod.ini [all] landing, landing_transports (the most; the size follows the beach)
internal int g_land_waves = 3;   // [all] landing_waves: how often the transports bring more soldiers over (0-10)
internal int g_land_cits = 10;   // [all] landing_citizens: the most citizens along to build the forward base (0-10)
internal int g_land_trace = 0;   // test: landing_trace=1 - each transport every tick while soldiers are aboard
internal int g_land_knows = 0;   // test: landing_knows_map=1 - every building of the target counts, seen or not
internal int g_land_flag = 0;   // the unload goal's last argument: the AI's code passes 1, the player's command 0 (test: landing_flag)
#define LD_PASS 128
#define LD_SHIPS 10
// a group's strength: sqrt(hit points x attack, each summed over the group) - N soldiers wear a tower down N times
// faster than one, so a group's hit points x attack per unit summed (unit_power) rated a tower as 49 soldiers
// strengths: soldiers, defences reaching the beach, over the water, the beach together, with the soldiers within 40 tiles
struct ld_def { float soldiers, towers, water, all, behind; int n, assumed; };
// the forward base: what the citizens along build, in this order (several at once while citizens are free)
enum { LDB_TOWER, LDB_BARRACKS, LDB_CENTER, LDB_HOSPITAL, LDB_FACTORY };
struct ld_proj { ee_unit *b; int kind, type, cits, state, tries, x, y; };   // state 0 to come, 1 going up, 2 standing, 3 dropped
internal struct landing {
    int phase;                           // 0 idle, 1 ships, 2 board, 3 sail, 4 ashore, 5 bombard (the fleet first)
    ee_player *target;
    int sx, sy, ocean, wx, wy;           // landing site, the sea, the water in front of it
    ee_unit *ships[LD_SHIPS]; int nships;
    int want_ships, want_n; float unit_pw, need;   // the size planned: transports, soldiers; a soldier's strength, what the beach needs
    ee_unit *esc[14]; int nesc;          // warships (up to 6) and aircraft (up to 8) along
    ee_unit *pass[LD_PASS]; int npass, launch_n;
    ee_attack_subgroup *frozen[96]; int nfrozen;    // subgroups led by the landing
    ee_unit *obj; int ox, oy;
    uint32_t since, last, next_try, reissue, trained_t, boarded_t;
    int switched;                        // the beach changed once at the sail check
    int tr_missing, tr_type, tr_got;     // transports still to come, their type, taken at the last order
    ee_unit *cit[10]; int ncit, want_cits; signed char cjob[10];   // citizens along for the forward base, each one's project (-1 none)
    struct ld_proj proj[6]; int nproj; uint32_t base_t;   // the forward base; the last project started
    int ex, ey, ewx, ewy, gathered;      // where they board: the shore nearest the passengers, the water in front
    struct ld_def def;                   // the beach's defence when planned
    int all_ashore, tries_at[LD_SHIPS], ux[LD_SHIPS], uy[LD_SHIPS];   // each transport's own unload point
    int stx, sty, staged; uint32_t stage_t;   // where the transports gather at sea before the run in
    int dkx, dky;                        // its dock nearest the beach (the way over, while the meeting point is not known)
    uint32_t sea_t;                      // since when the sea on the way is held (the sail check)
    int meet;                            // the meeting point found (ex, ey)
    int wst, nw, waves; uint32_t wt, wre;   // the next wave: 0 none, 1 to the meeting point, 2 boarding, 3 at sea; its soldiers
    ee_unit *wpass[LD_PASS];
    unsigned tries, landings, aborted, orders;
    int fleet_want, fleet0, war_type, tr_want, anti0, war_missing, trb_missing; float power, need0; uint32_t bomb_t, check_t, train_t, arrive_t;   // the bombardment (arrive_t: the fleet off the beach)
    unsigned bombards, bomb_won;
} g_ld[16];

internal int ld_cap(int t) {   // soldiers a transport holds: its type's +0x54C (raft, copper: 10, bronze: 12, imperial on: 14; copper held 10 in the board test)
    ee_unit_type *td = t > 0 ? type_data(t) : NULL;
    int c = td ? type_get_transport_capacity(td) : 0;
    return c >= 4 && c <= 30 ? c : 10;
}
// citizens along by the landing's size (the user: "gone completely for one ship landings ... 10 for larger landings - so
// five can build a town center, and the others barracks and factories beside the defences, a hospital"): one
// transport none, two 3 (towers), three 6 (a barracks too), four and more 10 (a forward base); landing_citizens at most
internal int ld_cits_for(int ships, int cap) {
    int c = ships <= 1 ? 0 : ships == 2 ? 3 : ships == 3 ? 6 : 10;
    if (c > g_land_cits) c = g_land_cits;
    if (c > cap) c = cap;
    return c;
}
internal int ld_type_avail(ee_player *pl, int t);
// the newest transport it can train: of the transport ships its tech record allows (an upgraded line's old type no
// longer), the one of the highest tech - one per epoch in the game's data (raft, copper, bronze, imperial, atomic,
// gargantua), a data mod's own included; 0: none
internal int ld_transport_type(ee_player *pl) {
    int best = 0, bt = 0;
    for (int t = 0; t < G_TYPES_N; t++) {
        if (!is_transport(t) || !ld_type_avail(pl, t)) continue;
        if (type_get_tech(type_data(t)) > bt) { bt = type_get_tech(type_data(t)); best = t; }
    }
    return best;
}
internal int ld_affordable(ee_player *pl, int t) {   // how many of type t its stock pays now
    int *st = player_stock_at(pl), n = 20;
    for (int r = 2; r < 7; r++) {
        int c = unit_cost(pl, t, r);
        if (c > 0 && st[r] / c < n) n = st[r] / c;
    }
    return n;
}

internal int landing_leads(ee_attack_subgroup *sg) {
    ee_player *pl = asg_get_player(sg);
    unsigned pi = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    if (!g_landing || !pl || !g_ld[pi].phase) return 0;
    for (int k = 0; k < g_ld[pi].nfrozen; k++) if (g_ld[pi].frozen[k] == sg) return 1;
    return 0;
}
// the transports still missing: their cost is kept from every other unit order (econ.c's reserve) while the landing
// waits for them - P3's 3000 of each went on other units within 4 minutes, its transports never came
internal void landing_reserve(ee_player *pl, int reserve[7]) {
    struct landing *l = &g_ld[(unsigned)player_get_index(pl) & 15];
    if (g_landing && l->phase == 5 && !l->bomb_t && !g_landing_order && !g_priority_mil) {   // the fleet and its transports
        int tt = ld_transport_type(pl);
        for (int r = 2; r < 7; r++)
            reserve[r] += (l->war_type ? l->war_missing * unit_cost(pl, l->war_type, r) : 0) + (tt ? l->trb_missing * unit_cost(pl, tt, r) : 0);
        return;
    }
    if (!g_landing || l->phase != 1 || g_landing_order || l->tr_missing <= 0 || !l->tr_type) return;
    for (int r = 2; r < 7; r++) reserve[r] += l->tr_missing * unit_cost(pl, l->tr_type, r);
}
// the population room a bombardment's fleet needs while it is built (its missing warships and transports): at the
// population cap the fleet never came (Small Islands e5 with money: 67 of 89 tries to train the warship type at the
// cap, 2 of 7 bombardments planned sailed) - the game's own soldier orders get only the room beyond it meanwhile
// (econ.c train_call), as landing_reserve holds the money
internal int landing_room_reserve(ee_player *pl) {
    struct landing *l = &g_ld[(unsigned)player_get_index(pl) & 15];
    return g_landing && l->phase == 5 && !l->bomb_t ? l->war_missing + l->trb_missing : 0;
}
internal int landing_busy(ee_player *pl) {   // boarding, at sea or ashore
    return g_landing && g_ld[(unsigned)player_get_index(pl) & 15].phase >= 2;
}
internal int landing_has_unit(ee_player *pl, ee_unit *u) {   // a transport, warship or soldier of a landing under way
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    struct landing *l = &g_ld[pi];
    if (!g_landing || l->phase < 2) return 0;
    for (int k = 0; k < l->nships; k++) if (l->ships[k] == u) return 1;
    for (int k = 0; k < l->nesc; k++) if (l->esc[k] == u) return 1;
    for (int k = 0; k < l->npass; k++) if (l->pass[k] == u) return 1;
    for (int k = 0; k < l->ncit; k++) if (l->cit[k] == u) return 1;
    for (int k = 0; k < l->nw; k++) if (l->wpass[k] == u) return 1;
    return 0;
}
internal int fleet_leads(ee_attack_subgroup *sg);                // fleet.c: a sea subgroup holding a ship the fleet leads
internal int fleet_refuses(ee_attack_subgroup *sg, ee_unit *u);  // fleet.c: an order to one of them
internal int mass_refuses(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal);   // mass.c: an order to a reinforcement held at the rally
internal int expand_ship(ee_unit *u);                 // expand.c: the transport of an outpost's ferry (its player), 0: none
internal int expand_refuses(ee_attack_subgroup *sg, ee_unit *u); // expand.c: the ferry's transport and passengers, an outpost's guard
internal int cover_refuses(ee_attack_subgroup *sg, ee_unit *u);  // cover.c: a unit falling back to cover or at the Hospital
internal int expand_leads(ee_attack_subgroup *sg);               // expand.c: a subgroup holding the ferry's transport
internal int expand_unload_ok(ee_unit *u, int gx, int gy);   // expand.c: the game's unload retry at the ferry's own point
internal int expand_has(ee_player *pl, ee_unit *u);        // expand.c: a passenger of the ferry (the landing leaves it)
internal void __thiscall hook_asg_sea_update(ee_attack_subgroup *sg, int now) {
    if (!is_vanilla(asg_get_player(sg)) && (landing_leads(sg) || fleet_leads(sg) || expand_leads(sg))) return;
    asg_sea_update(sg, now);
}
internal void __thiscall hook_asg_land_update(ee_attack_subgroup *sg, int now) {   // EECPASGLand slot 90
    if (!is_vanilla(asg_get_player(sg)) && landing_leads(sg)) return;
    asg_update(sg, now);
}
internal void __thiscall hook_asg_air_update(ee_attack_subgroup *sg, int now) {
    if (!is_vanilla(asg_get_player(sg)) && landing_leads(sg)) return;
    asg_air_update(sg, now);
}
internal void __thiscall hook_asg_sea_transport_update(ee_attack_subgroup *sg, int now) {
    if (!is_vanilla(asg_get_player(sg)) && (landing_leads(sg) || expand_leads(sg))) return;
    asg_sea_transport_update(sg, now);
}

// A unit's goal step (0x5F0F7E): its goal set (+0x1E8) answers 2, 0x5EE965 drops the goal - and can clear the set too;
// then, the unit still with a goal (+0x1F0), the set is asked again (0x5F1029, 0x619FF5 thiscall(set, unit, &out))
// unchecked: a read of [0 + 8] at 0x619FF9, 6 island games today (one in 24). No set: the unit gets the default one
// (5000, 0x5EFCF0 - as the game does for a unit without a goal there) and the answer is 0. Only skipping the call left
// the unit without a set, and the next order to it read [0 + 0x24] (0x5F063A, from the transport manager's 0x48CD3A).
// Where the null sets come from: the set answers 1 (take the next) with no next set (out 0) - stored as the unit's set
// (0x5F104E), and the unit type's vf29 right after read [0 + 5] (0x5FC4ED). That answer, at both asks (0x5F0FFA too),
// gives the default set and 0.
internal unsigned g_goalset_null;
internal int __thiscall hook_goalset_ask(void *set, ee_unit *unit, int *out) {
    if (!set) {
        if (g_log && g_goalset_null++ < 10) fprintf(g_log, "# engine guard: a unit's goal set gone after its goal was dropped - the default set\n");
        unit_say(unit, 5000);
        return 0;
    }
    int r = goalset_ask(set, unit, out);
    if (r == 1 && out && !*out) {
        if (g_log && g_goalset_null++ < 10) fprintf(g_log, "# engine guard: a unit's goal set ended without a next one - the default set\n");
        unit_say(unit, 5000);
        return 0;
    }
    return r;
}
// The same state elsewhere: a unit given a goal (0x5F05BD, 116 callers - the transport manager's 0x48CD3C, 0x4A28C3)
// reads its goal set's +0x24 unchecked (0x5F0634: mov ecx, [esi + 0x1E8]; 0x5F063A: [ecx + 0x24]) - 3 of 72 island
// games. That load goes through a thunk (8B 8E E8 01 00 00 -> E8 thunk, 90): no set, the default one first.
__attribute__((used)) void goalset_fix(ee_unit *unit) {
    if (g_log && g_goalset_null++ < 10) fprintf(g_log, "# engine guard: a unit without a goal set given a goal - the default set\n");
    unit_say(unit, 5000);
}
__attribute__((naked, used)) void goalset_read_thunk(void) {
    __asm__("cmpl $0, 0x1e8(%esi)\n\t"
            "jne 1f\n\t"
            "pushl %eax\n\t"
            "pushl %edx\n\t"
            "pushl %esi\n\t"
            "call _goalset_fix\n\t"
            "addl $4, %esp\n\t"
            "popl %edx\n\t"
            "popl %eax\n"
            "1:\n\t"
            "movl 0x1e8(%esi), %ecx\n\t"
            "ret");
}

// A subgroup gives one of its units an order through vf57 (thiscall(sg, unit, goal, 0, priority), 0x45CBEA, air
// 0x442AD8; 0 refused - the caller deletes the goal). Its update is skipped while the landing leads it, but its event
// handlers still give orders: the transport subgroup's (0x445A0F, through its second vtable) unloaded the waiting
// landing transports 5 times in one run (the unload at 0x445AAD). Refused while the landing leads the subgroup.
// One caller uses the goal after a refusal: the land subgroup's vf131 (0x44AC8E - the transport subgroup hands it the
// units no longer aboard, 0x4455D4) deletes the refused goal (0x44AF96) and then reads it and calls its vf4 (0x44AFB5):
// the deleted goal's vtable is the abstract base's - msvcrt's _purecall, exception 0xC0000002, 1-6 of 12 Small Islands
// games (more with the transport manager's unload refused, as more units come off transports while led). The game
// itself never refuses there; for that caller the goal is taken and not given (the goal's few bytes are lost).
internal unsigned g_ld_refused, g_ld_kept_goal;
internal void ld_fu_given(ee_goal *g, int ok);   // below (the game's unload orders to landing transports)
internal char __thiscall hook_asg_give_goal(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio) {
    if (!is_vanilla(asg_get_player(sg)) && (landing_leads(sg) || fleet_refuses(sg, u) || mass_refuses(sg, u, goal) || expand_refuses(sg, u) || cover_refuses(sg, u))) {   // the fleet (fleet.c): its ships in any subgroup; mass.c; expand.c; cover.c
        g_ld_refused++;
        if (from_land_handoff((uintptr_t)__builtin_return_address(0))) { g_ld_kept_goal++; return 1; }
        ld_fu_given(goal, 0);
        return 0;
    }
    char r = asg_give_goal(sg, u, goal, a, prio);
    ld_fu_given(goal, r != 0);
    return r;
}
internal char __thiscall hook_asg_air_give_goal(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio) {
    if (!is_vanilla(asg_get_player(sg)) && landing_leads(sg)) { g_ld_refused++; return 0; }
    return asg_air_give_goal(sg, u, goal, a, prio);
}

// nothing (trees, buildings, units of any owner) on the 3x3 tiles around (x, y): room to unload - a landing site
// on a wooded coast: every unload there ended at once, the transports circling with the soldiers aboard
internal int ld_tile_free(int x, int y) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid || x < 1 || y < 1) return 0;
    for (struct grid_it it = grid_it_box(x, y, 3, 1); grid_it_next(&it); )
        if (abs(unit_get_x(it.u) - x) <= 1 && abs(unit_get_y(it.u) - y) <= 1) return 0;
    return 1;
}

internal unsigned ocean_near(int x, int y, int r, int *wx, int *wy) {   // an ocean id within r tiles (and that tile)
    for (int d = 1; d <= r; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                unsigned o = area_id(x + dx, y + dy, 1);
                if (o) { if (wx) { *wx = x + dx; *wy = y + dy; } return o; }
            }
    return 0;
}

struct ld_scan { ee_player *pl; ee_player *target; int nb, bx[2000], by[2000]; int nd, dx[16], dy[16]; int nt; ee_unit *tr[32]; int nw; ee_unit *war[32]; int na; ee_unit *air[16]; };
internal void ld_unit(ee_unit *u, void *ctx) {
    struct ld_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    if (unit_get_hp(u) <= 0 || !o) return;
    int fam = type_get_family(td), t = type_get_id(td);
    if (o == s->target && (fam == 2 || fam == 29) && !is_wall(t) && (g_land_knows || seen_by(u, s->pl)) && s->nb < 2000) {
        s->bx[s->nb] = unit_get_x(u); s->by[s->nb++] = unit_get_y(u);
    } else if (o == s->pl && t == UT(B_DOCK) && s->nd < 16) {
        s->dx[s->nd] = unit_get_x(u); s->dy[s->nd++] = unit_get_y(u);
    } else if (o == s->pl && is_transport(t) && unit_get_x(u) >= 0 && !expand_ship(u) && s->nt < 32) s->tr[s->nt++] = u;   // not an outpost's ferry (expand.c)
    else if (o == s->pl && fam == 4 && !is_transport(t) && !is_fishing_boat(t) && type_get_attack(td) > 0 && s->nw < 32)
        s->war[s->nw++] = u;
    else if (o == s->pl && (fam == 3 || fam == 7 || fam == 17) && type_get_attack(td) > 0 && unit_get_x(u) >= 0 && !is_aa(u) && s->na < 16)
        s->air[s->na++] = u;
}

internal float ld_strength(float hp, float att) { return sqrtf(hp * att); }
internal float ld_attack(ee_unit *u) { return unit_get_hp(u) > 0 ? unit_power(u) / (float)unit_get_hp(u) : 0.0f; }
// what defends a beach (seen): soldiers within 15 tiles, defences reaching it, defences reaching the water in front
// (they shoot at the transports)
internal void ld_defence(ee_player *pl, int x, int y, int wx, int wy, struct ld_def *d) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    float sh = 0, sa = 0, th = 0, ta = 0, wh = 0, wa = 0, bh = 0, ba = 0;
    d->n = 0;
    if (grid) {
        int r = 40;
        for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            if (!td || unit_get_hp(u) <= 0) continue;
            ee_player *o = unit_get_owner(u);
            if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
            int ux = unit_get_x(u), uy = unit_get_y(u);
            float dx = (float)(ux - x), dy = (float)(uy - y), d2 = dx * dx + dy * dy, hp = (float)unit_get_hp(u), range, att;
            if (is_soldier(td) || is_siege(type_get_id(td))) {
                if (d2 <= 225.0f) { sh += hp; sa += ld_attack(u); d->n++; }
                else if (d2 <= 1600.0f) { bh += hp; ba += ld_attack(u); }
            } else if (defence_stats(u, &range, &att)) {
                float rr = (range + 2.0f) * (range + 2.0f), wdx = (float)(ux - wx), wdy = (float)(uy - wy);
                if (d2 <= rr) { th += hp; ta += att; }
                if (wdx * wdx + wdy * wdy <= rr) { wh += hp; wa += att; }
            }
        }
    }
    // honest_vision (intel.c): the soldiers seen there lately, out of sight now; a beach not looked at for 3 minutes is
    // assumed held by a quarter of the army it knows of (the user: "assuming the enemy likely has towers and a defence
    // army before I ever saw his base")
    float rh = 0, ra = 0; int rn = 0;
    intel_soldiers(pl, x, y, 15, &rh, &ra, NULL, &rn);
    sh += rh; sa += ra; d->n += rn;
    rh = ra = 0;
    intel_soldiers(pl, x, y, 40, &rh, &ra, NULL, &rn);   // within 40, the 15 counted above again: a little more caution
    bh += rh; ba += ra;
    d->assumed = 0;
    if (g_honest_vision && intel_looked_ago(pl, x, y) > 180000) {
        float ah = 0, aa = 0; int an = 0;
        intel_army(pl, &ah, &aa, NULL, &an);
        sh += 0.25f * ah; sa += 0.25f * aa; d->assumed = 1;
    }
    d->soldiers = ld_strength(sh, sa); d->towers = ld_strength(th, ta); d->water = ld_strength(wh, wa);
    d->all = ld_strength(sh + th, sa + ta);
    d->behind = ld_strength(sh + th + bh, sa + ta + ba);
}
// the strength a landing needs against a beach: 1.4 times what holds it - the user: "it just has to choose the weakest
// one and bring an army that actually has a chance". The defences over the water hit the transports, not the troops:
// the warships take them on first
internal float ld_need(const struct ld_def *d) { return 1.4f * d->all; }

// a landing site: open land at the shore of ocean `ocean` next to the target's buildings - the weakest defended
// (rarely an undefended one), against the strength `power` the landing can bring
internal struct { unsigned tried, land, shore, sea, safe, quiet, oc[8], ocn[8]; } g_ld_stat[16];   // why no site (landing P)
internal int ld_site(ee_player *pl, unsigned pi, struct ld_scan *s, unsigned ocean, float power, int *sx, int *sy, int *swx, int *swy, struct ld_def *bd) {
    memset(&g_ld_stat[pi], 0, sizeof g_ld_stat[pi]);
    float best = -1e9f;
    int bx = 0, by = 0, bwx = 0, bwy = 0, found = 0;
    if (power < 1.0f) power = 1.0f;
    int step = s->nb > 60 ? s->nb / 60 : 1;
    for (int b = 0; b < s->nb; b += step) {
        unsigned bc = area_id(s->bx[b], s->by[b], 2);
        for (int r = 6; r <= 30; r += 3)
            for (int a = 0; a < 16; a++) {
                float ang = (float)a * 0.3927f;
                int x = s->bx[b] + (int)(cosf(ang) * r), y = s->by[b] + (int)(sinf(ang) * r);
                g_ld_stat[pi].tried++;
                if (!bc || area_id(x, y, 2) != bc || area_id(x, y, 1) || !ld_tile_free(x, y)) continue;   // open land of the building's continent (an island in the bay was taken: the troops stuck there)
                g_ld_stat[pi].land++;
                int wx, wy;
                unsigned on = ocean_near(x, y, 2, &wx, &wy);
                if (on) {
                    g_ld_stat[pi].shore++;
                    int q = 0;
                    while (q < 8 && g_ld_stat[pi].oc[q] && g_ld_stat[pi].oc[q] != on) q++;
                    if (q < 8) { g_ld_stat[pi].oc[q] = on; g_ld_stat[pi].ocn[q]++; }
                }
                if (on != ocean) continue;
                g_ld_stat[pi].sea++;
                struct ld_def d;
                ld_defence(pl, x, y, wx, wy, &d);
                g_ld_stat[pi].safe += d.towers <= 0;
                g_ld_stat[pi].quiet += d.towers <= 0 && d.n <= 2;
                int close = 0;
                for (int k = 0; k < s->nb; k++) {
                    int ddx = s->bx[k] - x, ddy = s->by[k] - y;
                    close += ddx * ddx + ddy * ddy <= 400;
                }
                // the weakest beach first, then the most buildings near it (up to 12)
                float sc = (float)(close < 12 ? close : 12) - 20.0f * (ld_need(&d) + 0.5f * d.water) / power;
                if (close >= 1 && sc > best) { best = sc; bx = x; by = y; bwx = wx; bwy = wy; *bd = d; found = 1; }
            }
    }
    if (!found) return 0;
    *sx = bx; *sy = by; *swx = bwx; *swy = bwy;
    return 1;
}

internal struct threat_scan g_ld_home;
// its buildings on the continent of its base point: a forward base across the sea is no home (the troops at the beach
// were counted at home and boarded again)
internal void ld_home_scan(ee_player *pl) {
    g_ld_home.pl = pl; g_ld_home.nb = g_ld_home.ne = 0;
    grid_each(home_threat_unit, &g_ld_home);
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    unsigned hc = bm ? area_id(bm_get_base_x(bm), bm_get_base_y(bm), 2) : 0;
    if (!hc) return;
    int n = 0;
    for (int b = 0; b < g_ld_home.nb; b++)
        if (area_id(g_ld_home.bx[b], g_ld_home.by[b], 2) == hc) { g_ld_home.bx[n] = g_ld_home.bx[b]; g_ld_home.by[n++] = g_ld_home.by[b]; }
    g_ld_home.nb = n;
}
internal int ld_at_home(int x, int y) {
    if (x < 0) return 0;
    for (int b = 0; b < g_ld_home.nb; b++) {
        int dx = g_ld_home.bx[b] - x, dy = g_ld_home.by[b] - y;
        if (dx * dx + dy * dy <= 400) return 1;
    }
    return 0;
}

internal void ld_release(unsigned pi, const char *why, uint32_t now) {
    struct landing *l = &g_ld[pi];
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: %s\n", pi, now / 60000.0, why);
    l->phase = 0; l->nfrozen = 0; l->nships = 0; l->npass = 0; l->obj = NULL; l->ncit = 0; l->nproj = 0;
    l->wst = 0; l->nw = 0; l->waves = 0;
}

internal int ld_aboard(ee_unit *u) { return unit_get_x(u) < 0; }
internal int ld_alive(ee_unit *u) { return u && unit_get_hp(u) > 0 && unit_get_type_data(u); }

// the nearest enemy defence or warship (seen) within r tiles - what shoots at the transports
internal ee_unit *ld_naval_target(ee_player *pl, int x, int y, int r, int soldiers) {   // soldiers: enemy soldiers and siege as well
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return NULL;
    ee_unit *best = NULL;
    float bd = 1e9f;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_hp(u) <= 0) continue;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        float rr, a;
        int t = type_get_id(td);
        if (!defence_stats(u, &rr, &a) && !(type_get_family(td) == 4 && !is_transport(t) && !is_fishing_boat(t)) &&
            !(soldiers && (is_soldier(td) || is_siege(t))))
            continue;
        float dx = (float)(unit_get_x(u) - x), dy = (float)(unit_get_y(u) - y), d2 = dx * dx + dy * dy;
        if (d2 <= (float)(r * r) && d2 < bd) { bd = d2; best = u; }
    }
    return best;
}
// the warships along (from the sail on): they go ahead to the water in front of the site and take on the defences
// and warships there - the towers shoot at them instead of the transports (the user: "a player would send a few
// warships along to draw the towers' attention away from the transports") - then shell the shore
// From the plan on: they shell the beach free (its defences, the warships off it, then the soldiers on it) while
// the transports are built and boarded.
internal int ld_is_air(ee_unit *u) {
    int fam = type_get_family(unit_get_type_data(u));
    return fam == 3 || fam == 7 || fam == 17;
}
// THE SEA ON THE WAY (the user: "I certainly place subs or planes before my coasts on patrol now"; "subs are always
// visible, but can only be attacked by frigates and towers"; "best to take the game's own table as source of truth"):
// who can hit whom is the game's own factor 0x4D5F2C (thiscall on 0x917DE0: the attacker's weapon kind - type +0x4C8,
// the target's type, both players' indices) - the table 0x8D200C [target category, type +0x4C4][weapon kind], 0: it
// cannot (the game's attack subgroups test it the same way, 0x4504FD). A threat: an enemy unit afloat or in the air
// whose weapon hits the transport type; an escort counts against the threats of a category when its weapon hits them.
enum { LDS_SHIP, LDS_SUB, LDS_AIR };   // for the log only: afloat, submarines (family 10), aircraft
struct ld_sea { float h[64], a[64]; ee_unit_type *rep[64]; int rpi[64], n[3]; };
internal float ld_factor(ee_unit_type *atd, int api, ee_unit_type *ttd, int tpi) {
    if (!atd || !ttd || type_get_attack(atd) <= 0) return 0.0f;
    return attack_bonus(atd, ttd, api, tpi);
}
internal int ld_pidx(ee_unit *u) { ee_player *o = unit_get_owner(u); return o ? player_get_index(o) : -1; }
internal int ld_hits(ee_unit *a, ee_unit *t) {   // can a hit t (the game's table)
    return a && t && ld_factor(unit_get_type_data(a), ld_pidx(a), unit_get_type_data(t), ld_pidx(t)) > 0.0f;
}
// an enemy (seen) afloat or in the air that hits the transport type trtd (of player index tpi)
internal int ld_sea_foe(ee_player *pl, ee_unit *u, ee_unit_type *trtd, int tpi) {
    ee_player *o = unit_get_owner(u);
    int x = unit_get_x(u), y = unit_get_y(u);
    if (!o || player_get_index(o) <= 0 || unit_get_hp(u) <= 0 || x < 0 || !is_enemy(pl, o)) return 0;
    if (!ld_is_air(u) && !area_id(x, y, 1)) return 0;   // on land: the beach's defence counts it
    return ld_factor(unit_get_type_data(u), ld_pidx(u), trtd, tpi) > 0.0f && seen_by(u, pl);
}
// the threats on the way over: within 10 tiles of the line from (x0, y0) to the water in front of the beach, every
// 5 tiles; aircraft within 25 of the beach too (a patrol)
struct ld_sea_scan { ee_player *pl; ee_unit_type *trtd; int tpi, npt, px[48], py[48], sx, sy; struct ld_sea *t; };
internal void ld_sea_unit(ee_unit *u, void *ctx) {
    struct ld_sea_scan *c = ctx;
    if (!ld_sea_foe(c->pl, u, c->trtd, c->tpi)) return;
    int x = unit_get_x(u), y = unit_get_y(u), on = 0, air = ld_is_air(u);
    if (air) on = (x - c->sx) * (x - c->sx) + (y - c->sy) * (y - c->sy) <= 625;
    for (int q = 0; q < c->npt && !on; q++) on = (x - c->px[q]) * (x - c->px[q]) + (y - c->py[q]) * (y - c->py[q]) <= 100;
    if (!on) return;
    ee_unit_type *td = unit_get_type_data(u);
    int k = type_get_category(td) & 63;
    c->t->h[k] += (float)unit_get_hp(u); c->t->a[k] += ld_attack(u);
    if (!c->t->rep[k]) { c->t->rep[k] = td; c->t->rpi[k] = ld_pidx(u); }
    c->t->n[air ? LDS_AIR : type_get_family(td) == 10 ? LDS_SUB : LDS_SHIP]++;
}
internal void ld_sea_threat(ee_player *pl, ee_unit_type *trtd, int x0, int y0, int wx, int wy, int sx, int sy, struct ld_sea *t) {
    static struct ld_sea_scan c;
    memset(&c, 0, sizeof c); memset(t, 0, sizeof *t);
    c.pl = pl; c.trtd = trtd; c.tpi = player_get_index(pl); c.sx = sx; c.sy = sy; c.t = t;
    if (!trtd) return;
    float dx = (float)(wx - x0), dy = (float)(wy - y0), len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 5.0f);
    if (steps > 46) steps = 46;
    for (int i = 0; i <= steps; i++) {
        float f = steps ? (float)i / (float)steps : 1.0f;
        c.px[c.npt] = x0 + (int)(dx * f); c.py[c.npt++] = y0 + (int)(dy * f);
    }
    grid_each(ld_sea_unit, &c);
}
internal int ld_sea_useful(const struct ld_sea *e, ee_unit *u) {   // its weapon hits some threat on the way
    for (int k = 0; k < 64; k++)
        if (e->rep[k] && ld_factor(unit_get_type_data(u), ld_pidx(u), e->rep[k], e->rpi[k]) > 0.0f) return 1;
    return 0;
}
internal void ld_sea_add(struct ld_sea *o, const struct ld_sea *e, ee_unit *u) {   // u (own) against each category it hits
    if (!ld_alive(u)) return;
    for (int k = 0; k < 64; k++)
        if (e->rep[k] && ld_factor(unit_get_type_data(u), ld_pidx(u), e->rep[k], e->rpi[k]) > 0.0f) {
            o->h[k] += (float)unit_get_hp(u); o->a[k] += ld_attack(u);
        }
}
// what gets through to the transports, against their hit points: each category's attack for a minute in reach (an
// attack every 2 s), the escorts hitting it stopping at most half (strength against strength): with 2 cruisers
// "matching" 5 strafing aircraft over the beach P2 sailed, the aircraft sank a transport with 14 aboard while the
// cruisers chased them - the escorts must thin a patrol out before the transports go
internal float ld_sea_risk(const struct ld_sea *e, const struct ld_sea *o, float tr_hp) {
    float dmg = 0;
    for (int k = 0; k < 64; k++) {
        float es = ld_strength(e->h[k], e->a[k]), os = ld_strength(o->h[k], o->a[k]);
        if (es > 0) dmg += e->a[k] * (os >= es ? 0.5f : 1.0f - 0.5f * os / es) * 30.0f;
    }
    return tr_hp > 0 ? dmg / tr_hp : 0.0f;
}
// the nearest threat to the transports (type trtd) within r tiles that esc can hit (esc NULL: any)
internal ee_unit *ld_hunt(ee_player *pl, int x, int y, int r, ee_unit *esc, ee_unit_type *trtd) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid || !trtd) return NULL;
    int tpi = player_get_index(pl);
    ee_unit *best = NULL;
    float bd = 1e9f;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        float dx = (float)(unit_get_x(u) - x), dy = (float)(unit_get_y(u) - y), d2 = dx * dx + dy * dy;
        if (d2 > (float)(r * r) || d2 >= bd || !ld_sea_foe(pl, u, trtd, tpi) || (esc && !ld_hits(esc, u))) continue;
        bd = d2; best = u;
    }
    return best;
}
internal float ld_tr_hp(struct landing *l) {   // the transports' hit points
    float h = 0;
    for (int k = 0; k < l->nships; k++) h += ld_alive(l->ships[k]) ? (float)unit_get_hp(l->ships[k]) : 0.0f;
    return h;
}
internal ee_unit_type *ld_tr_td(struct landing *l) { return l->nships ? unit_get_type_data(l->ships[0]) : NULL; }
// the sail check: the threats on the way from (x0, y0) against its escorts
internal float ld_sea_now(ee_player *pl, struct landing *l, int x0, int y0, struct ld_sea *e) {
    static struct ld_sea o;
    memset(&o, 0, sizeof o);
    ld_sea_threat(pl, ld_tr_td(l), x0, y0, l->wx, l->wy, l->sx, l->sy, e);
    for (int k = 0; k < l->nesc; k++) ld_sea_add(&o, e, l->esc[k]);
    return ld_sea_risk(e, &o, ld_tr_hp(l));
}

// the shelling's target: the enemy's defence (seen, within 16 tiles of the beach) nearest to the fleet that the
// warship u can hit - every warship on the same tower, from the fleet's side inwards: a warship type that outranges
// the towers (263: 8 tiles, the tower 463: 5) stands off at its range from the first one, instead of sailing past
// the outer towers to one behind them (P3 lost 3 of 5 that way, the tower it went for untouched). Before, each took
// the nearest defence or warship to the water: towers shelled by one or two ships while the others chased warships
internal ee_unit *ld_beach_tower(ee_player *pl, struct landing *l, ee_unit *u) {
    float fx = 0, fy = 0, bd = 1e9f;
    int n = 0;
    for (int k = 0; k < l->nesc; k++)
        if (ld_alive(l->esc[k]) && !ld_is_air(l->esc[k]) && unit_get_x(l->esc[k]) >= 0) { fx += unit_get_x(l->esc[k]); fy += unit_get_y(l->esc[k]); n++; }
    if (!n) return NULL;
    fx /= n; fy /= n;
    ee_unit *best = NULL;
    for (struct grid_it it = grid_it_box(l->sx, l->sy, 16, 1); grid_it_next(&it); ) {
        float r, a;
        ee_unit *t = it.u;
        if (unit_get_hp(t) <= 0 || !is_enemy(pl, unit_get_owner(t)) || !defence_stats(t, &r, &a) || !seen_by(t, pl) || !ld_hits(u, t)) continue;
        float dx = unit_get_x(t) - fx, dy = unit_get_y(t) - fy, d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = t; }
    }
    return best;
}
internal void ld_escorts(ee_player *pl, struct landing *l) {
    for (int k = 0; k < l->nesc; k++) {
        ee_unit *u = l->esc[k];
        ee_goal *g = unit_get_goal(u);
        ee_unit *cur = g && goal_is_attack(g) && ld_alive(goal_get_target(g)) ? goal_get_target(g) : NULL;
        float cr, ca;
        if (cur && l->phase == 5 && !ld_is_air(u) && !defence_stats(cur, &cr, &ca)) {   // shelling: off a warship or soldier onto a tower of the beach
            ee_unit *t = unit_get_x(u) >= 0 ? ld_beach_tower(pl, l, u) : NULL;
            if (t) l->orders += unit_goal(u, goal_attack(u, t), 0, 0x1F40);
            continue;
        }
        if (cur) continue;
        if (unit_get_x(u) < 0) continue;   // landed at its airport (refuelling)
        int ux = unit_get_x(u), uy = unit_get_y(u);
        // the threats to the transports it can hit first (the game's table: frigates the submarines, cruisers and
        // fighters the aircraft ...): off the beach, on the way over, near itself
        ee_unit_type *trtd = ld_tr_td(l);
        ee_unit *h = ld_hunt(pl, l->wx, l->wy, 16, u, trtd);
        if (!h && l->meet) h = ld_hunt(pl, (l->wx + l->ewx) / 2, (l->wy + l->ewy) / 2, 14, u, trtd);
        if (!h && ld_is_air(u)) h = ld_hunt(pl, l->sx, l->sy, 16, u, trtd);
        if (!h) h = ld_hunt(pl, ux, uy, 12, u, trtd);
        if (h) { l->orders += unit_goal(u, goal_attack(u, h), 0, 0x1F40); continue; }
        if (ld_is_air(u)) {   // aircraft: the beach's defences, its soldiers, else over the beach
            ee_unit *e = ld_naval_target(pl, l->sx, l->sy, 12, 0);
            if (!ld_hits(u, e)) e = ld_naval_target(pl, l->wx, l->wy, 10, 0);
            if (!ld_hits(u, e)) e = ld_naval_target(pl, l->sx, l->sy, 12, 1);
            if (!ld_hits(u, e)) e = NULL;   // a fighter: nothing on the ground - over the beach
            if (e) l->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40);
            else if (!g || !goal_near(u, (float)l->sx + 0.5f, (float)l->sy + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->sx + 0.5f, (float)l->sy + 0.5f), 0, 0x1F40);
            continue;
        }
        // the beach's defences and the warships off it first, then what shoots at it where it is (nearest to itself
        // first: two warships shelled a tower 35 tiles from the beach for minutes), then the soldiers on the beach.
        // Shelling (phase 5): the towers around the beach first, the nearest to the fleet (ld_beach_tower)
        ee_unit *e = l->phase == 5 ? ld_beach_tower(pl, l, u) : NULL;
        if (e) { l->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40); continue; }
        e = ld_naval_target(pl, l->wx, l->wy, 12, 0);
        if (!ld_hits(u, e)) e = ld_naval_target(pl, l->sx, l->sy, 12, 0);
        if (!ld_hits(u, e)) e = ld_naval_target(pl, unit_get_x(u), unit_get_y(u), 8, 0);
        if (!ld_hits(u, e)) e = ld_naval_target(pl, l->sx, l->sy, 10, 1);
        if (!ld_hits(u, e)) e = NULL;
        if (e) l->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40);
        else if (!goal_near(u, (float)l->wx + 0.5f, (float)l->wy + 0.5f, 3.0f))
            l->orders += unit_goal(u, goal_move((float)l->wx + 0.5f, (float)l->wy + 0.5f), 0, 0x1F40);
    }
}

// water of the sea `ocean` with no land within 2 tiles, nearest to (x, y) within 10: a loaded transport waits there -
// idle at the shore with soldiers aboard, the game's own unit code (0x483E10: cargo, no order, land next to it)
// unloads it on the spot: P3 lost half of those aboard while its transports waited for the warships
internal int ld_open_water(int x, int y, unsigned ocean, int *ox, int *oy) {
    for (int d = 1; d <= 10; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                int tx = x + dx, ty = y + dy, ok = area_id(tx, ty, 1) == ocean;
                for (int ax = -2; ax <= 2 && ok; ax++)
                    for (int ay = -2; ay <= 2 && ok; ay++) ok = area_id(tx + ax, ty + ay, 1) == ocean;
                if (ok) { *ox = tx; *oy = ty; return 1; }
            }
    return 0;
}

internal void ld_fu_forget(ee_goal *g);   // below (the game's unload orders to landing transports)
internal void ld_fu_check(uint32_t now);
internal ee_goal *ld_unload(ee_unit *sh, int x, int y) {
    ee_goal *g = goal_unload(sh, x, y, g_land_flag);
    if (g) ld_fu_forget(g);
    return g;
}
// called off with soldiers aboard: each transport unloads on the land nearest to it (at home)
internal void ld_unload_home(struct landing *l) {
    for (int k = 0; k < l->nships; k++) {
        ee_unit *sh = l->ships[k];
        if (units_inside(sh) <= 0) continue;   // nothing aboard
        int x0 = unit_get_x(sh), y0 = unit_get_y(sh), ux = -1, uy = -1;
        for (int d = 1; d <= 10 && ux < 0; d++)
            for (int dx = -d; dx <= d && ux < 0; dx++)
                for (int dy = -d; dy <= d && ux < 0; dy += (abs(dx) == d ? 1 : 2 * d))
                    if (area_id(x0 + dx, y0 + dy, 2) && !area_id(x0 + dx, y0 + dy, 1) && ld_tile_free(x0 + dx, y0 + dy)) { ux = x0 + dx; uy = y0 + dy; }
        if (ux < 0) continue;
        l->orders += unit_goal(sh, ld_unload(sh, ux, uy), 0, 0x1F40);
    }
}

// every subgroup holding a transport, warship or soldier of the landing is led by it - checked every tick: the game
// moves units between subgroups, and a transport with soldiers aboard sailed off alone in its new subgroup
internal void ld_refreeze(struct landing *l, uint8_t *dq, unsigned first, unsigned cnt) {
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++) {
            ee_attack_subgroup *sg = *sp;
            int mine = 0, have = 0;
            for (int f = 0; f < l->nfrozen; f++) have |= l->frozen[f] == sg;
            if (!sg || have || l->nfrozen >= 96) continue;
            for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg) && !mine; r++) {
                ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                for (int j = 0; j < l->nships && !mine; j++) mine = u == l->ships[j];
                for (int j = 0; j < l->nesc && !mine; j++) mine = u == l->esc[j];
                for (int j = 0; j < l->npass && !mine; j++) mine = u == l->pass[j];
                for (int j = 0; j < l->nw && !mine; j++) mine = u == l->wpass[j];
            }
            if (mine) l->frozen[l->nfrozen++] = sg;
        }
    }
}

// a shore tile to unload on: land of the site's continent, room on the 3x3 tiles, the landing's sea within 2 tiles,
// within 15 of the site; nearest to (x, y), at least `gap` tiles from the other transports' points and from
// (ax, ay) - where the last unload failed
internal int ld_shore_tile(struct landing *l, int k, int x, int y, int ax, int ay, int gap, int *ox, int *oy) {
    unsigned cont = area_id(l->sx, l->sy, 2);
    int rr = (15 + l->nships) * (15 + l->nships);   // 10 transports 4 tiles apart need more beach
    for (int d = 0; d <= 10 + l->nships; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                int tx = x + dx, ty = y + dy, ok = 1;
                if ((tx - l->sx) * (tx - l->sx) + (ty - l->sy) * (ty - l->sy) > rr) continue;
                if (area_id(tx, ty, 2) != cont || area_id(tx, ty, 1) || (int)ocean_near(tx, ty, 2, NULL, NULL) != l->ocean) continue;
                if (ax >= 0 && abs(tx - ax) < 3 && abs(ty - ay) < 3) continue;
                for (int j = 0; j < l->nships && ok; j++)
                    if (j != k && abs(tx - l->ux[j]) < gap && abs(ty - l->uy[j]) < gap) ok = 0;
                if (!ok || !ld_tile_free(tx, ty)) continue;
                *ox = tx; *oy = ty;
                return 1;
            }
    return 0;
}
// the run in: each transport its own point on the beach, 4+ tiles apart (3 when the beach is short of room): with
// one point for all, the first put its soldiers there and the unload of the next ended at once (no room), soldiers
// aboard, the transport idle off the beach
internal void ld_run_in(struct landing *l, unsigned pi) {
    for (int k = 0; k < l->nships; k++) { l->ux[k] = -100; l->uy[k] = -100; }
    for (int k = 0; k < l->nships; k++)
        if (units_inside(l->ships[k]) <= 0) continue;   // empty (a wave on fewer transports): stays off the beach
        else if (!ld_shore_tile(l, k, l->sx, l->sy, -1, -1, 4, &l->ux[k], &l->uy[k]) && !ld_shore_tile(l, k, l->sx, l->sy, -1, -1, 3, &l->ux[k], &l->uy[k])) {
            l->ux[k] = l->sx; l->uy[k] = l->sy;
        }
    for (int k = 0; k < l->nships; k++) {
        if (l->ux[k] == -100) continue;
        ee_goal *g = ld_unload(l->ships[k], l->ux[k], l->uy[k]);
        if (g_log && g) fprintf(g_log, "# landing: P%u: transport at %d,%d runs in, the goal's unload point %d,%d\n", pi,
                                unit_get_x(l->ships[k]), unit_get_y(l->ships[k]), goal_get_unload_x(g), goal_get_unload_y(g));
        l->orders += unit_goal(l->ships[k], g, 0, 0x1F40);
    }
    l->staged = 1;
}
// transport k with soldiers aboard and no unload order: off the beach (within 10 tiles of its point) the unload ended
// there - the nearest shore tile with room to where it is, away from where it failed; farther off (another order took
// it): its point again
internal void ld_reunload(struct landing *l, unsigned pi, int k, uint32_t now) {
    ee_unit *sh = l->ships[k];
    int x0 = unit_get_x(sh), y0 = unit_get_y(sh), ux = l->ux[k], uy = l->uy[k], ddx = x0 - ux, ddy = y0 - uy;
    l->tries_at[k]++;
    if (ddx * ddx + ddy * ddy <= 100 && ld_shore_tile(l, k, x0, y0, ux, uy, 2, &ux, &uy)) { l->ux[k] = ux; l->uy[k] = uy; }
    ee_goal *g = ld_unload(sh, ux, uy);
    int ok = unit_goal(sh, g, 0, 0x1F40);   // refused: deleted
    l->orders += ok;
    if (g_log && l->tries_at[k] <= 8)   // the goal's own unload point (+0x34/+0x38, from the transport class vf57)
        fprintf(g_log, "# landing: P%u at %.1f min: transport at %d,%d (hp %d) still loaded - unloads at %d,%d, the goal's point %d,%d\n",
                pi, now / 60000.0, x0, y0, unit_get_hp(sh), ux, uy, ok ? goal_get_unload_x(g) : -1, ok ? goal_get_unload_y(g) : -1);
}
// where 3+ transports gather at sea before the run in, so they reach the beach together (one after another, the
// first ashore fought alone and each transport arriving late was the towers' and warships' only target): open water
// of the landing's sea 10-18 tiles off the water in front of the beach, towards where they come from, the least
// known threat
internal int ld_stage(struct landing *l, unsigned pi, int *ox, int *oy) {
    float dx = (float)(l->ewx - l->wx), dy = (float)(l->ewy - l->wy), len = sqrtf(dx * dx + dy * dy), best = 1e9f;
    if (len < 20.0f) return 0;   // the meeting point is near: they come together anyway
    int found = 0;
    for (int r = 10; r <= 18; r += 4) {
        int x, y;
        if (!ld_open_water(l->wx + (int)(dx * (float)r / len), l->wy + (int)(dy * (float)r / len), (unsigned)l->ocean, &x, &y)) continue;
        int bx = x - l->wx, by = y - l->wy;
        if (bx * bx + by * by < 64) continue;
        float t = threat_at(pi, x, y) + 0.01f * (float)r;
        if (ld_hunt(g_players[pi], x, y, 10, NULL, ld_tr_td(l)) || ld_naval_target(g_players[pi], x, y, 10, 0)) t += 5.0f;   // threats or defences near
        if (t < best) { best = t; *ox = x; *oy = y; found = 1; }
    }
    return found;
}

struct ld_cits { ee_player *pl; int x, y, n; ee_unit *u[32]; int d[32]; };
internal void ld_cit_unit(ee_unit *u, void *ctx) {
    struct ld_cits *c = ctx;
    int t = type_get_id(unit_get_type_data(u));
    if (unit_get_owner(u) != c->pl || !is_citizen_type(t) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || c->n >= 32 || expand_has(c->pl, u)) return;
    int dx = unit_get_x(u) - c->x, dy = unit_get_y(u) - c->y, d = dx * dx + dy * dy;
    if (d > 625) return;
    c->u[c->n] = u; c->d[c->n++] = d;
}

// The forward base (the user: "being able to produce units on site is very valuable"): the citizens ashore build, by
// the landing's size (ld_base_plan) - towers, a barracks, a town center (5 citizens), a hospital, a factory - near the
// beach on land of the landed continent: the newest type of each it can build and pay (0x555710), the spot by the
// game's own placement test (can_place), out of known tower reach if it can, 3+ tiles from its other buildings; the
// foundation by the game's own creation (0x62EF8A cdecl(player, type, x, y), the cost paid), the builders with
// EEUGBuildRepair (new 0x5C, 0x60BAD0(goal, player, building, 0)) as the game orders its builders. One started every
// 10 s while citizens are free; a project it cannot pay for a minute is dropped. Free citizens help the others; all
// done: the citizens go back to the game.
internal void ld_base_plan(struct landing *l) {
    static const int plan3[] = { LDB_TOWER, LDB_TOWER }, plan6[] = { LDB_TOWER, LDB_BARRACKS, LDB_TOWER },
                     plan10[] = { LDB_TOWER, LDB_BARRACKS, LDB_CENTER, LDB_HOSPITAL, LDB_FACTORY, LDB_TOWER };
    const int *pl = l->ncit >= 8 ? plan10 : l->ncit >= 5 ? plan6 : plan3;
    int n = l->ncit >= 8 ? 6 : l->ncit >= 5 ? 3 : l->ncit ? 2 : 0;
    static const int cits[] = { 3, 2, 5, 2, 3 };
    memset(l->proj, 0, sizeof l->proj);
    for (int k = 0; k < n; k++) { l->proj[k].kind = pl[k]; l->proj[k].cits = cits[pl[k]]; }
    l->nproj = n; l->base_t = 0;
}
internal int ld_can_build(ee_player *pl, int t) { return ut_has(t) && can_build(pl, t) != 0; }
internal int ld_kind_type(ee_player *pl, int kind) {   // the type of a kind it can build and pay now (0: none)
    static const enum ut_name centers[] = { UT_B_TOWN_CENTER, UT_B_SETTLEMENT },
        factories[] = { UT_B_CYBER_FACTORY, UT_B_TANK_FACTORY, UT_B_STABLE, UT_B_ARCHERY_RANGE };
    const enum ut_name *t = NULL; int n = 0;
    switch (kind) {
    case LDB_TOWER: return tower_newest(pl);
    case LDB_CENTER: t = centers; n = 2; break;
    case LDB_FACTORY: t = factories; n = 4; break;
    case LDB_BARRACKS: return ld_can_build(pl, UT(B_BARRACKS)) ? UT(B_BARRACKS) : 0;
    case LDB_HOSPITAL: return ld_can_build(pl, UT(B_HOSPITAL)) ? UT(B_HOSPITAL) : 0;
    }
    for (int k = 0; k < n; k++) if (ld_can_build(pl, g_ut[t[k]])) return g_ut[t[k]];
    return 0;
}
internal const char *ld_kind_name(int kind) {
    static const char *nm[] = { "tower", "barracks", "town center", "hospital", "factory" };
    return kind >= 0 && kind < 5 ? nm[kind] : "?";
}
internal int ld_base_spot(ee_player *pl, unsigned pi, struct landing *l, int type, int cx, int cy, int *ox, int *oy) {
    unsigned cont = area_id(cx, cy, 2);
    ee_unit_type *td = type_data(type);
    int sz = td ? type_get_size(td) : 1, found = 0;
    float best = 1e9f;
    if (sz < 1) sz = 1;
    for (int d = 2; d <= 12; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                int x = cx + dx, y = cy + dy, ok = area_id(x, y, 2) == cont && !area_id(x, y, 1);
                for (int k = 0; k < l->nproj && ok; k++)   // room between its buildings: a way through
                    if (l->proj[k].state && abs(l->proj[k].x - x) < sz + 3 && abs(l->proj[k].y - y) < sz + 3) ok = 0;
                if (!ok || !can_place(pl, type, x, y)) continue;
                float sc = (float)d + 10.0f * threat_at(pi, x, y);
                if (sc < best) { best = sc; *ox = x; *oy = y; found = 1; }
            }
    return found;
}
internal void ld_bridgehead(ee_player *pl, unsigned pi, struct landing *l, uint32_t now) {
    int n = 0, cx = 0, cy = 0;
    for (int k = 0; k < l->ncit; k++)
        if (!ld_alive(l->cit[k])) { l->ncit--; l->cit[k] = l->cit[l->ncit]; l->cjob[k] = l->cjob[l->ncit]; k--; }
    for (int k = 0; k < l->ncit; k++) if (!ld_aboard(l->cit[k])) { n++; cx += unit_get_x(l->cit[k]); cy += unit_get_y(l->cit[k]); }
    if (!n) return;
    cx /= n; cy /= n;
    // going up: finished or destroyed - its builders free
    for (int j = 0; j < l->nproj; j++) {
        struct ld_proj *p = &l->proj[j];
        if (p->state != 1) continue;
        ee_unit *bs = ld_alive(p->b) ? unit_get_physics(p->b) : NULL;
        if (ld_alive(p->b) && !(bs && unit_get_finished(bs))) continue;
        p->state = ld_alive(p->b) ? 2 : 3;
        if (p->state == 3) p->b = NULL;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - %s %d at %d,%d %s\n", pi, now / 60000.0, ld_kind_name(p->kind), p->type,
                           p->x, p->y, p->state == 2 ? "finished" : "destroyed");
        for (int k = 0; k < l->ncit; k++) if (l->cjob[k] == j) l->cjob[k] = -1;
    }
    // the next project: citizens free, 10 s after the last
    int free = 0, busy = 0;
    for (int k = 0; k < l->ncit; k++) free += !ld_aboard(l->cit[k]) && l->cjob[k] < 0;
    for (int j = 0; j < l->nproj; j++) busy += l->proj[j].state == 1;
    for (int j = 0; j < l->nproj && free && (!l->base_t || now - l->base_t >= 10000); j++) {
        struct ld_proj *p = &l->proj[j];
        if (p->state) continue;
        int need = p->cits < n ? p->cits : n;
        if (busy && free < need) break;   // in order: wait for builders
        int t = ld_kind_type(pl, p->kind), x = 0, y = 0;
        if (!t) {   // it cannot build or pay it: again for a minute (every 10 s), then dropped - the next
            l->base_t = now;
            if (++p->tries >= 6) {
                p->state = 3;
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - no %s it can build and pay, dropped\n", pi, now / 60000.0, ld_kind_name(p->kind));
                continue;
            }
            break;
        }
        ee_unit *b = ld_base_spot(pl, pi, l, t, cx, cy, &x, &y) ? place_foundation(pl, t, x, y) : NULL;
        l->base_t = now;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - %s %d at %d,%d (%s), %d citizens ashore, %d free\n", pi, now / 60000.0,
                           ld_kind_name(p->kind), t, x, y, b ? "foundation laid" : "no room", n, free);
        if (!b) { p->state = 3; continue; }
        p->b = b; p->type = t; p->x = x; p->y = y; p->state = 1;
        for (int m = 0; m < need; m++) {   // the nearest free builders
            int bk = -1, bd = 0;
            for (int k = 0; k < l->ncit; k++) {
                if (ld_aboard(l->cit[k]) || l->cjob[k] >= 0) continue;
                int dx = unit_get_x(l->cit[k]) - x, dy = unit_get_y(l->cit[k]) - y;
                if (bk < 0 || dx * dx + dy * dy < bd) { bk = k; bd = dx * dx + dy * dy; }
            }
            if (bk < 0) break;
            l->cjob[bk] = (signed char)j; free--;
        }
        break;
    }
    // orders: each on its building; the free ones help the first going up
    int helping = -1, left = 0;
    for (int j = 0; j < l->nproj; j++) {
        if (l->proj[j].state == 1 && helping < 0) helping = j;
        left += l->proj[j].state < 2;
    }
    if (!left) {
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base done - %d citizens back to the game\n", pi, now / 60000.0, l->ncit);
        l->ncit = 0;
        return;
    }
    for (int k = 0; k < l->ncit; k++) {
        int j = l->cjob[k] >= 0 ? l->cjob[k] : helping;
        ee_unit *u = l->cit[k];
        ee_goal *g = unit_get_goal(u);
        ee_unit *b = j >= 0 ? l->proj[j].b : NULL;
        if (!b || ld_aboard(u) || (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && goal_get_building(g) == b)) continue;
        void *bg = goal_build_alloc();
        if (!bg) continue;
        bg = goal_build_init(bg, pl, b, 0);
        l->orders += unit_goal(u, bg, 0, 0x1F51);
    }
}
// soldiers trained at the forward base (its barracks, town center or factory standing): within 8 tiles of it and not
// the landing's - they join the troops ashore instead of the game's groups at home
struct ld_cap_scan { ee_player *pl; struct landing *l; int got; };
internal void ld_capture_unit(ee_unit *u, void *ctx) {
    struct ld_cap_scan *c = ctx;
    struct landing *l = c->l;
    if (unit_get_owner(u) != c->pl || !ld_alive(u) || ld_aboard(u) || !is_soldier(unit_get_type_data(u)) || l->npass >= LD_PASS) return;
    int x = unit_get_x(u), y = unit_get_y(u), near_base = 0;
    for (int j = 0; j < l->nproj && !near_base; j++) {
        struct ld_proj *p = &l->proj[j];
        if (p->state == 2 && p->kind != LDB_TOWER && p->kind != LDB_HOSPITAL && ld_alive(p->b))
            near_base = (x - p->x) * (x - p->x) + (y - p->y) * (y - p->y) <= 64;
    }
    if (!near_base) return;
    for (int k = 0; k < l->npass; k++) if (l->pass[k] == u) return;
    l->pass[l->npass++] = u; l->launch_n++; c->got++;
}
internal void ld_capture(ee_player *pl, unsigned pi, struct landing *l, uint32_t now) {
    int prod = 0;
    for (int j = 0; j < l->nproj; j++) prod |= l->proj[j].state == 2 && l->proj[j].kind != LDB_TOWER && l->proj[j].kind != LDB_HOSPITAL;
    if (!prod) return;
    struct ld_cap_scan c = { pl, l, 0 };
    grid_each(ld_capture_unit, &c);
    if (c.got && g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - %d soldiers trained there join the troops\n", pi, now / 60000.0, c.got);
}

internal void ld_why(unsigned pi, uint32_t now, const char *why) {   // why no landing is planned (once a minute)
    static uint32_t t[16];
    if (!g_log || (t[pi] && now - t[pi] < 60000 && now >= t[pi])) return;
    t[pi] = now;
    fprintf(g_log, "# landing: P%u at %.1f min: none planned - %s\n", pi, now / 60000.0, why);
}

// subgroups not led by the landing any more: those without a transport, warship, passenger or wave soldier of it
internal void ld_prune_frozen(struct landing *l) {
    int nf = 0;
    for (int f = 0; f < l->nfrozen; f++) {
        ee_attack_subgroup *sg = l->frozen[f];
        int keep = 0;
        for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg) && !keep; r++) {
            ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
            for (int j = 0; j < l->npass && !keep; j++) keep = u == l->pass[j];
            for (int j = 0; j < l->nw && !keep; j++) keep = u == l->wpass[j];
            for (int j = 0; j < l->nships && !keep; j++) keep = u == l->ships[j];
            for (int j = 0; j < l->nesc && !keep; j++) keep = u == l->esc[j];
        }
        if (keep) l->frozen[nf++] = sg;
    }
    l->nfrozen = nf;
}

// The waves (the user: "early a single transport might do damage, later a large scale landing with 10 ships might be
// needed"): everyone ashore, the transports go back to the meeting point and bring the soldiers at home it can spare
// over to the bridgehead while it holds - up to landing_waves times. A player keeps a landing alive that way, and a
// few transports carry a big landing. 1: to the meeting point, 2: boarding, 3: at sea - ashore they join the others.
// a wave at sea (wst 3): a transport with soldiers aboard and no unload order unloads again - every 0.5 s (landing_tick
// runs it between its 2 s ticks; wre is free at sea)
internal void ld_wave_reunload(struct landing *l, unsigned pi, uint32_t now) {
    l->wre = now;
    for (int k = 0; k < l->nships; k++) {
        ee_unit *sh = l->ships[k];
        ee_goal *cur = unit_get_goal(sh);
        if (!ld_alive(sh) || (cur && goal_is_unload(cur)) || units_inside(sh) <= 0) continue;
        ld_reunload(l, pi, k, now);
    }
}
internal void ld_wave(ee_player *pl, unsigned pi, struct landing *l, uint8_t *dq, unsigned first, unsigned cnt, uint32_t now) {
    for (int k = 0; k < l->nw; k++) if (!ld_alive(l->wpass[k])) { l->wpass[k--] = l->wpass[--l->nw]; }
    int cap = ld_cap(type_get_id(unit_get_type_data(l->ships[0])));
    if (l->wst == 0) {
        // empty and idle off the enemy beach they were sunk (P2 kept 1 of 6): back to the meeting point meanwhile
        for (int k = 0; k < l->nships && l->all_ashore && l->meet; k++) {
            ee_unit *sh = l->ships[k];
            ee_goal *g = unit_get_goal(sh);
            int dx = unit_get_x(sh) - l->ewx, dy = unit_get_y(sh) - l->ewy;
            if (dx * dx + dy * dy > 49 && (!g || goal_get_state(g) == 1) && units_inside(sh) <= 0)
                l->orders += unit_goal(sh, goal_move((float)l->ewx + 0.5f, (float)l->ewy + 0.5f), 0, 0x1F40);
        }
        if (!l->all_ashore || l->waves >= g_land_waves || !l->meet || now - l->wt < 20000) return;
        l->wt = now;
        // soldiers at home in land subgroups the game leads (whole subgroups, as the first boarding); an enemy army
        // at its door: one for each of its soldiers stays
        ld_home_scan(pl);
        int home = 0;
        for (unsigned k = 0; k < cnt && k < 64; k++) {
            ee_attack_group *g = deque_at(dq, first + k);
            for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++) {
                if (!*sp || !asg_is_land(*sp) || landing_leads(*sp)) continue;
                for (ee_asg_unit **r = asg_get_units_begin(*sp); r && r < asg_get_units_end(*sp); r++) {
                    ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                    home += ld_alive(u) && is_soldier(unit_get_type_data(u)) && ld_at_home(unit_get_x(u), unit_get_y(u));
                }
            }
        }
        int want = home - home_threat(pl);
        if (want > l->nships * cap) want = l->nships * cap;
        if (want < 4) return;
        l->nw = 0;
        for (unsigned k = 0; k < cnt && k < 64 && l->nw < want; k++) {
            ee_attack_group *g = deque_at(dq, first + k);
            for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g) && l->nw < want; sp++) {
                ee_attack_subgroup *sg = *sp;
                if (!sg || l->nfrozen >= 96 || !asg_is_land(sg) || landing_leads(sg)) continue;
                int n = 0, at = 0;
                for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg); r++) {
                    ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                    if (!ld_alive(u)) continue;
                    n++; at += ld_at_home(unit_get_x(u), unit_get_y(u));
                }
                if (!n || at * 2 < n) continue;
                l->frozen[l->nfrozen++] = sg;
                for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg); r++) {
                    ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                    if (ld_alive(u) && !ld_aboard(u) && is_soldier(unit_get_type_data(u)) && !expand_has(pl, u) && l->nw < want && l->nw < LD_PASS) l->wpass[l->nw++] = u;
                }
            }
        }
        if (l->nw < 4) { l->nw = 0; ld_prune_frozen(l); return; }
        for (int a = 0; a < l->nships; a++)   // the nearest the meeting point first: a wave takes only those it needs
            for (int b = a + 1; b < l->nships; b++) {
                int da = (unit_get_x(l->ships[a]) - l->ewx) * (unit_get_x(l->ships[a]) - l->ewx) + (unit_get_y(l->ships[a]) - l->ewy) * (unit_get_y(l->ships[a]) - l->ewy);
                int db = (unit_get_x(l->ships[b]) - l->ewx) * (unit_get_x(l->ships[b]) - l->ewx) + (unit_get_y(l->ships[b]) - l->ewy) * (unit_get_y(l->ships[b]) - l->ewy);
                if (db < da) { ee_unit *t = l->ships[a]; l->ships[a] = l->ships[b]; l->ships[b] = t; }
            }
        l->wst = 1; l->wt = now; l->wre = 0;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - %d transports back for %d soldiers at home\n", pi, now / 60000.0, l->waves + 1, l->nships, l->nw);
        return;
    }
    if (l->wst == 1) {   // the transports to the water in front of the meeting point, the soldiers to the shore
        int there = 0, close = 0;
        for (int k = 0; k < l->nships; k++) {
            ee_unit *sh = l->ships[k];
            int dx = unit_get_x(sh) - l->ewx, dy = unit_get_y(sh) - l->ewy;
            if (dx * dx + dy * dy <= 49) { there++; continue; }
            if (!goal_near(sh, (float)l->ewx + 0.5f, (float)l->ewy + 0.5f, 2.0f))
                l->orders += unit_goal(sh, goal_move((float)l->ewx + 0.5f, (float)l->ewy + 0.5f), 0, 0x1F40);
        }
        for (int k = 0; k < l->nw; k++) {
            ee_unit *u = l->wpass[k];
            int dx = unit_get_x(u) - l->ex, dy = unit_get_y(u) - l->ey;
            if (dx * dx + dy * dy <= 64) { close++; continue; }
            if (!goal_near(u, (float)l->ex + 0.5f, (float)l->ey + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->ex + 0.5f, (float)l->ey + 0.5f), 0, 0x1F40);
        }
        int need = (l->nw + cap - 1) / cap;
        if (need > l->nships) need = l->nships;
        there = 0;
        for (int k = 0; k < need; k++) {
            int dx = unit_get_x(l->ships[k]) - l->ewx, dy = unit_get_y(l->ships[k]) - l->ewy;
            there += dx * dx + dy * dy <= 49;
        }
        if ((there == need && close * 4 >= l->nw * 3) || now - l->wt >= 180000) { l->wst = 2; l->wt = now; l->wre = 0; }
        return;
    }
    if (l->wst == 2) {   // boarding: loaded and idle at the shore out onto open water (the game unloads it there), one with
                         // the game's unload too (as in phase 2)
        int aboard = 0;
        for (int k = 0; k < l->nw; k++) aboard += ld_aboard(l->wpass[k]);
        for (int k = 0; k < l->nships; k++) {
            ee_unit *sh = l->ships[k];
            ee_goal *g = unit_get_goal(sh);
            int wx, wy;
            if ((g && goal_get_state(g) != 1 && !goal_is_unload(g)) || units_inside(sh) <= 0) continue;
            if (g && goal_is_unload(g) && g_log)
                fprintf(g_log, "# landing: P%u at %.1f min: transport %d with %d aboard had the game's unload to %d,%d while boarding - kept off the shore\n",
                        pi, now / 60000.0, k, units_inside(sh), goal_get_unload_x(g), goal_get_unload_y(g));
            if (ld_open_water(unit_get_x(sh), unit_get_y(sh), (unsigned)l->ocean, &wx, &wy))
                l->orders += unit_goal(sh, goal_move((float)wx + 0.5f, (float)wy + 0.5f), 0, 0x1F40);
        }
        if (aboard == l->nw || now - l->wt >= 150000) {
            if (aboard < 4) {
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - boarding failed (%d of %d aboard)\n", pi, now / 60000.0, l->waves + 1, aboard, l->nw);
                ld_unload_home(l);
                l->nw = 0; l->wst = 0; l->waves++; l->wt = now;
                ld_prune_frozen(l);
                return;
            }
            {   // the sea on the way held: the escorts get 2 more minutes, then the wave is unloaded at home
                struct ld_sea e;
                float risk = ld_sea_now(pl, l, l->ewx, l->ewy, &e);
                if (risk > 0.4f && now - l->wt < 270000) {
                    if (g_log && (now - l->wt) % 30000 < 2000)
                        fprintf(g_log, "# landing: P%u at %.1f min: wave %d waits - the sea holds %d warships, %d submarines, %d aircraft (risk %.2f)\n",
                                pi, now / 60000.0, l->waves + 1, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk);
                    return;
                }
                if (risk > 0.4f) {
                    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - the sea held, unloaded at home\n", pi, now / 60000.0, l->waves + 1);
                    ld_unload_home(l);
                    l->nw = 0; l->wst = 0; l->waves = g_land_waves; l->wt = now;
                    ld_prune_frozen(l);
                    return;
                }
            }
            int n = 0;
            for (int k = 0; k < l->nw; k++) if (ld_aboard(l->wpass[k])) l->wpass[n++] = l->wpass[k];
            l->nw = n;
            ld_prune_frozen(l);
            memset(l->tries_at, 0, sizeof l->tries_at);
            ld_run_in(l, pi);
            l->wst = 3; l->wt = now;
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d sails with %d soldiers\n", pi, now / 60000.0, l->waves + 1, n);
            return;
        }
        if (l->wre && now - l->wre < 30000) return;
        l->wre = now;
        int per = cap;   // full transports, the nearest first
        for (int k = 0; k < l->nships; k++) {
            ee_unit *list[LD_PASS]; int m = 0;
            for (int j = k * per; j < (k + 1) * per && j < l->nw; j++) if (!ld_aboard(l->wpass[j])) list[m++] = l->wpass[j];
            if (!m) continue;
            ee_goal *g = goal_load(l->ships[k], list, m);
            if (!g) continue;   // refused: deleted, no parts from it
            l->orders++;
            for (int j = k * per; j < (k + 1) * per && j < l->nw; j++)
                if (!ld_aboard(l->wpass[j])) l->orders += unit_goal(l->wpass[j], goal_load_part(g), 0, 0x1F40);
        }
        return;
    }
    // at sea: unloaded again at once when the unload ended with soldiers aboard; ashore they join the others
    ld_wave_reunload(l, pi, now);
    int landed = 0;
    for (int k = 0; k < l->nw; k++) {
        ee_unit *u = l->wpass[k];
        if (ld_aboard(u)) continue;
        if (l->npass < LD_PASS) { l->pass[l->npass++] = u; l->launch_n++; }
        l->wpass[k--] = l->wpass[--l->nw]; landed++;
    }
    if (landed && g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - %d ashore, %d still aboard\n", pi, now / 60000.0, l->waves + 1, landed, l->nw);
    if (!l->nw || now - l->wt >= 600000) {
        l->waves++; l->wst = 0; l->wt = now; l->nw = 0;
        ld_prune_frozen(l);
    }
}

internal ee_unit *ld_raid_target(ee_player *pl, ee_player *target, int x, int y, int r);   // below
internal ee_unit *ld_objective(ee_player *pl, unsigned pi, ee_player *target, unsigned cont, int x, int y);
// BOMBARD ([all] landing_bombard, default 0; the playtester: "the main island will have huge concentrated forces,
// especially if the AI never dares to leave - bomb the way free with huge warship fleets, then land a large force with
// a forward base, keep the ships near for support"): a beach too strong for the army it can send (power < 1.2 x the
// beach's need) is shelled first. The fleet: half the beach's need in warship strength (warships outrange most of
// what stands on a beach), 4 to 16, the warship type of its epoch that does the most to the beach's tower by the
// game's table (0x4D5F2C x attack x sqrt(hit points), factor 1 at least) - trained 3 at a time every 15 s with the
// siege's priority; only that type or one half as good (the fleet: its warships of that worth, ld_fleet), the
// population room for it held from the game's soldier orders (landing_room_reserve). It sails when complete (or
// after 6 minutes with 4), shells the beach (ld_escorts: the towers around it from the fleet's side inwards, then the
// warships off it and the soldiers) and is rechecked every 20 s: once the army is 1.2 x what holds the beach, the
// landing is planned at once (the warships along). Quick (the playtester: "the ships can't take too long - once the
// player notices he makes catapults and later cannons, a huge anti-ship bonus, and the chance to do damage with
// ships alone is over"): the transports for every soldier it can send are built while the fleet is, so the landing
// follows the shelling at once; 4 minutes of shelling at most (from the fleet's arrival, 7 from its sailing); and
// once 2 more enemy units near the beach hit the warships at 1.5 x or more
// (the game's table) than when it began, the landing goes now if the army at least equals the beach, else it is
// called off - as when half the fleet is lost and the beach still holds 70% of its need. The missing warships' and
// transports' cost is held back from every other order meanwhile (landing_reserve): P2 failed every warship for gold
// and iron and every transport for wood while its army orders took them.
// the warship of its epoch that does the most to the beach's tower: type ids end at 723 (the table has 0x300 slots,
// past it other data: type_data). Available: the type's tech record's own test (its vf1, the first check of
// EEPlayer::vf21 behind 0x555710 - without the price and the population room). Before, tech +4 was taken for it:
// set for the old line's type an upgrade replaced (251, never trainable again - the fleets were planned on it and
// built of whatever else it could train: 255 and 254, factor 0.4 against a tower, 185 with 11 attack) and not set for
// the new one (263) that trained. Only a factor of 1 or more (the game's table) - the others hardly scratch a tower.
internal int ld_type_avail(ee_player *pl, int t) {
    ee_unit_type *td = type_data(t);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!td || !tt || type_get_tech(td) < 2000) return 0;   // campaign heroes: tech 14xx (Golden Hind 611: 1446; ordering it crashed the game)
    ee_tech *tech = tech_lookup(tt, type_get_tech(td));
    return tech && tech_available(tech);
}
// the strongest defence of the target seen within 16 tiles of the beach (the fleet is picked against it: 463, the
// Middle Ages' tower, held the beaches of Small Islands e5); none seen: 465
internal ee_unit_type *ld_tower_td(ee_player *pl, ee_player *target, int x, int y) {
    ee_unit_type *best = NULL;
    int bh = 0;
    for (struct grid_it it = grid_it_box(x, y, 16, 1); grid_it_next(&it); ) {
        float r, a;
        if (unit_get_owner(it.u) != target || unit_get_hp(it.u) <= 0 || !defence_stats(it.u, &r, &a) || !seen_by(it.u, pl)) continue;
        if (type_get_hp(it.td) > bh) { bh = type_get_hp(it.td); best = it.td; }
    }
    return best ? best : type_data(UT(B_GUARD_TOWER_WW1));
}
internal float ld_war_value(ee_player *pl, ee_unit_type *td, ee_unit_type *tw, ee_player *target) {   // what a warship type does to the tower tw
    float f = ld_factor(td, player_get_index(pl), tw, target ? player_get_index(target) : 0);
    return f >= 1.0f ? f * (float)type_get_attack(td) * sqrtf((float)type_get_hp(td)) : 0.0f;
}
// now_only: of those it can train and pay now (0x555710); *bv: the best value of all available types
internal int ld_war_type(ee_player *pl, ee_player *target, ee_unit_type *tw, int now_only, float *bvp) {
    int best = 0;
    float bv = 0;
    for (int t = 1; t < 724; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || type_get_id(td) != t || type_get_family(td) != 4 || is_transport(t) || is_fishing_boat(t) || type_get_attack(td) <= 0) continue;
        if (!ld_type_avail(pl, t)) continue;
        if (now_only && !can_build(pl, t)) continue;   // trainable and payable now
        float v = ld_war_value(pl, td, tw, target);
        if (g_land_trace && g_log && !now_only)
            fprintf(g_log, "# ldbomb: war type %d against %d: factor %.2f attack %d hp %d range %.1f - value %.0f\n", t, type_get_id(tw),
                    ld_factor(td, player_get_index(pl), tw, target ? player_get_index(target) : 0), type_get_attack(td), type_get_hp(td), type_get_range(td), v);
        if (v > bv) { bv = v; best = t; }
    }
    if (bvp) *bvp = bv;
    return best;
}
// the fleet: its warships that do at least half what the best available type does to the tower (the others stay
// with the game)
internal int ld_fleet(ee_player *pl, struct landing *l, struct ld_scan *s, ee_unit **out) {
    ee_unit_type *tw = ld_tower_td(pl, l->target, l->sx, l->sy);
    float bv = 0;
    ld_war_type(pl, l->target, tw, 0, &bv);
    int n = 0;
    for (int k = 0; k < s->nw; k++)
        if (bv > 0 && ld_war_value(pl, unit_get_type_data(s->war[k]), tw, l->target) >= 0.5f * bv) out[n++] = s->war[k];
    return n;
}
internal int ld_bombard_plan(ee_player *pl, unsigned pi, struct landing *l, ee_player *target, struct ld_scan *s, unsigned ocean, float power,
                           int sx, int sy, int swx, int swy, struct ld_def *d, uint32_t now, int tr_want) {
    int wt = ld_war_type(pl, target, ld_tower_td(pl, target, sx, sy), 0, NULL);
    if (!wt) return 0;   // nothing that hurts a tower
    ee_unit_type *wtd = type_data(wt);
    float ship_pw = ld_strength((float)type_get_hp(wtd), (float)type_get_attack(wtd));
    int want = ship_pw > 0 ? (int)ceilf(0.5f * ld_need(d) / ship_pw) : 8;
    if (want < 4) want = 4;
    if (want > 16) want = 16;
    l->target = target; l->sx = sx; l->sy = sy; l->ocean = (int)ocean; l->wx = swx; l->wy = swy; l->def = *d;
    l->fleet_want = want; l->war_type = wt; l->power = power; l->need0 = ld_need(d); l->tr_want = tr_want; l->anti0 = -1;
    l->phase = 5; l->since = now; l->bomb_t = 0; l->arrive_t = 0; l->check_t = 0; l->train_t = 0; l->fleet0 = 0; l->nesc = 0; l->nfrozen = 0; l->bombards++;
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the beach %d,%d needs %.0f, its army %.0f - a fleet first: %d warships (type %d, has %d), %d transports (has %d)\n",
                       pi, now / 60000.0, sx, sy, ld_need(d), power, want, wt, s->nw, tr_want, s->nt);
    return 1;
}
struct ld_anti { ee_player *pl; ee_player *target; ee_unit_type *wtd; int x, y, n; };
internal void ld_anti_unit(ee_unit *u, void *ctx) {   // enemy land units near the beach that hit the warships hard
    struct ld_anti *a = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_owner(u) != a->target || unit_get_hp(u) <= 0 || !td || type_get_attack(td) <= 0) return;
    if (abs(unit_get_x(u) - a->x) > 16 || abs(unit_get_y(u) - a->y) > 16 || !seen_by(u, a->pl)) return;
    if (ld_factor(td, ld_pidx(u), a->wtd, player_get_index(a->pl)) >= 1.5f) a->n++;
}
internal int ld_anti_ship(ee_player *pl, struct landing *l) {
    struct ld_anti a = { pl, l->target, l->war_type ? type_data(l->war_type) : NULL, l->sx, l->sy, 0 };
    if (!a.wtd) return 0;
    grid_each(ld_anti_unit, &a);
    return a.n;
}
internal int bk_room(ee_player *pl);   // bunker.c
internal float ld_range(ee_unit *u) {   // its range with upgrades (stats +0x9C), else its type's (+0x420)
    ee_unit_stats *st = unit_get_stats(u); ee_unit_type *td = unit_get_type_data(u);
    return st ? stat_get_range(st) : td ? type_get_range(td) : 0.0f;
}
// landing_trace (test): at each check of the shelling, the warships (type, hit points, target) and the defences
// within 16 tiles of the beach (type, hit points, range, the nearest warship and whether each reaches the other)
internal void ld_bomb_trace(ee_player *pl, unsigned pi, struct landing *l, uint32_t now) {
    for (int k = 0; k < l->nesc; k++) {
        ee_unit *u = l->esc[k];
        ee_goal *g = ld_alive(u) ? unit_get_goal(u) : NULL;
        ee_unit *t = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
        if (!ld_alive(u) || ld_is_air(u)) continue;
        fprintf(g_log, "# ldbomb: P%u %.2f ship type %d hp %d/%d range %.1f at %d,%d goal %s", pi, now / 60000.0, type_get_id(unit_get_type_data(u)),
                unit_get_hp(u), type_get_hp(unit_get_type_data(u)), ld_range(u), unit_get_x(u), unit_get_y(u), rtti_name(g));
        if (ld_alive(t)) fprintf(g_log, " on type %d at %d,%d (%.1f tiles)", type_get_id(unit_get_type_data(t)), unit_get_x(t), unit_get_y(t),
                                 sqrtf((float)((unit_get_x(t) - unit_get_x(u)) * (unit_get_x(t) - unit_get_x(u)) + (unit_get_y(t) - unit_get_y(u)) * (unit_get_y(t) - unit_get_y(u)))));
        fprintf(g_log, "\n");
    }
    for (struct grid_it it = grid_it_box(l->sx, l->sy, 16, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        float r, a;
        if (unit_get_hp(u) <= 0 || !is_enemy(pl, unit_get_owner(u)) || !defence_stats(u, &r, &a)) continue;
        float bd = 1e9f; ee_unit *ne = NULL;
        for (int k = 0; k < l->nesc; k++) {
            ee_unit *e = l->esc[k];
            if (!ld_alive(e) || ld_is_air(e)) continue;
            float dx = (float)(unit_get_x(e) - unit_get_x(u)), dy = (float)(unit_get_y(e) - unit_get_y(u)), d = sqrtf(dx * dx + dy * dy);
            if (d < bd) { bd = d; ne = e; }
        }
        fprintf(g_log, "# ldbomb: P%u %.2f defence type %d hp %d/%d range %.1f attack %.0f at %d,%d seen %d", pi, now / 60000.0, type_get_id(it.td), unit_get_hp(u),
                type_get_hp(it.td), r, a, unit_get_x(u), unit_get_y(u), seen_by(u, pl));
        if (ne) fprintf(g_log, " - nearest warship %.1f tiles (reaches it %d, it reaches the warship %d, factor %.2f)", bd, ld_range(ne) + 1.0f >= bd, r + 1.0f >= bd,
                          ld_factor(unit_get_type_data(ne), ld_pidx(ne), it.td, ld_pidx(u)));
        fprintf(g_log, "\n");
    }
}
internal void ld_bombard_tick(ee_player *pl, unsigned pi, struct landing *l, ee_unit_mgr *um, uint32_t now) {
    static struct ld_scan s;
    memset(&s, 0, sizeof s); s.pl = pl; s.target = l->target;
    grid_each(ld_unit, &s);
    static ee_unit *fl[32];
    int nf = ld_fleet(pl, l, &s, fl);
    if (!l->bomb_t) {   // building the fleet
        l->war_missing = l->fleet_want - nf > 0 ? l->fleet_want - nf : 0;   // held back from other orders (landing_reserve)
        l->trb_missing = l->tr_want - s.nt > 0 ? l->tr_want - s.nt : 0;
        if (nf < l->fleet_want && (!l->train_t || now - l->train_t >= 15000)) {
            l->train_t = now ? now : 1;
            if (g_land_trace && g_log) {   // the warship types: why not trainable now
                static const enum ut_name tln[] = { UT_S04_BRONZE_CATAPULT_SHIP, UT_S06_MIDDLE_AGE_CATAPULT_SHIP,
                    UT_S06_MIDDLE_AGE_FRIGATE, UT_S06_MIDDLE_AGE_GALLEY, UT_S06_MIDDLE_AGES_BATTLESHIP_DECEREME,
                    UT_S05_BYZANTINE_BATTLESHIP_SEPTRIREME, UT_S04_BRONZE_BATTLESHIP_PENTAKONTER, UT_S03_COPPER_BATTLESHIP_GALLEY };
                int tl[sizeof tln / sizeof *tln];
                for (size_t j = 0; j < sizeof tln / sizeof *tln; j++) tl[j] = g_ut[tln[j]];
                ee_tech_tree *tt = player_get_tech_tree(pl);
                fprintf(g_log, "# ldbomb: P%u %.2f planned %d, epoch %d, room %d:", pi, now / 60000.0, l->war_type, current_epoch(pl), bk_room(pl));
                for (size_t j = 0; j < sizeof tl / sizeof *tl; j++) {
                    ee_unit_type *td = type_data(tl[j]);
                    ee_tech *tech = td && tt ? tech_lookup(tt, type_get_tech(td)) : NULL;
                    fprintf(g_log, " %d: tech %d/%d avail %d can %d;", tl[j], tech ? tech_get_researched(tech) : -1, tech ? tech_get_available(tech) : -1,
                            tech ? tech_available(tech) : -1, can_build(pl, tl[j]));
                }
                fprintf(g_log, "\n");
            }
            ee_unit_type *tw = ld_tower_td(pl, l->target, l->sx, l->sy);
            float bv = 0;
            int bt = ld_war_type(pl, l->target, tw, 0, &bv), wt = ld_war_type(pl, l->target, tw, 1, NULL);   // the best, and of those it can train now
            if (bt) l->war_type = bt;   // what is missing is held back for the best type
            if (wt && ld_war_value(pl, type_data(wt), tw, l->target) < 0.5f * bv) wt = 0;   // not half as good: waits for the best
            if (wt) {
                int req[4] = { wt, l->fleet_want - nf > 3 ? 3 : l->fleet_want - nf, 0, 0 };
                unsigned got = 0;
                g_priority_mil = 1;
                hook_train_units(um, req, (uintptr_t)&got);
                g_priority_mil = 0;
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: %s %u warships (type %d) for the fleet - %d of %d\n", pi, now / 60000.0, got ? "trains" : "refused:", got ? got : req[1], wt, nf, l->fleet_want);
            } else if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: no warship it can train and pay now (type %d, room %d) - %d of %d\n", pi, now / 60000.0,
                                      bt, bk_room(pl), nf, l->fleet_want);
            int tt = ld_transport_type(pl);
            if (tt && s.nt < l->tr_want) {   // the transports at the same time: the landing follows the shelling at once
                int req[4] = { tt, l->tr_want - s.nt > 2 ? 2 : l->tr_want - s.nt, 0, 0 };
                unsigned got = 0;
                g_landing_order = 1;
                hook_train_units(um, req, (uintptr_t)&got);
                g_landing_order = 0;
                if (got && g_log) fprintf(g_log, "# landing: P%u at %.1f min: trains %u transports (type %d) for after the shelling - %d of %d\n", pi, now / 60000.0, got, tt, s.nt, l->tr_want);
            }
        }
        if (nf >= l->fleet_want || (now - l->since >= 360000 && nf >= 4)) {
            l->nesc = 0;
            for (int k = 0; k < nf && l->nesc < 14; k++) l->esc[l->nesc++] = fl[k];
            for (int k = 0; k < s.na && l->nesc < 14; k++) l->esc[l->nesc++] = s.air[k];
            l->fleet0 = nf; l->bomb_t = now ? now : 1; l->arrive_t = 0; l->check_t = now; l->anti0 = ld_anti_ship(pl, l);
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the fleet sails - %d warships (of %d), %d aircraft to the beach %d,%d\n", pi, now / 60000.0, nf, s.nw,
                               l->nesc - (nf < 14 ? nf : 14), l->sx, l->sy);
        } else if (now - l->since >= 600000) {
            l->aborted++; l->next_try = now + 300000; ld_release(pi, "no fleet in 10 minutes - bombardment dropped", now);
        }
        return;
    }
    if (now - l->check_t < 20000) return;
    l->check_t = now;
    int alive = 0;
    for (int k = 0; k < l->nesc; k++) alive += !ld_is_air(l->esc[k]);
    struct ld_def d;
    ld_defence(pl, l->sx, l->sy, l->wx, l->wy, &d);
    float need = ld_need(&d);
    int anti = ld_anti_ship(pl, l);
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: bombardment - the beach needs %.0f (from %.0f; soldiers %.0f, defences %.0f), %d of %d warships, army %.0f, anti-ship %d (from %d)\n",
                       pi, now / 60000.0, need, l->need0, d.soldiers, d.towers, alive, l->fleet0, l->power, anti, l->anti0);
    if (g_land_trace && g_log) ld_bomb_trace(pl, pi, l, now);
    if (anti >= l->anti0 + 2) {   // the defender answers the ships: now or never
        if (l->power >= need) {
            l->bomb_won++;
            ld_release(pi, "anti-ship weapons come up - the landing goes now", now);
            l->next_try = now;
        } else {
            l->aborted++; l->next_try = now + 300000;
            ld_release(pi, "anti-ship weapons come up, the beach holds - bombardment called off", now);
        }
        return;
    }
    if (l->power >= 1.2f * need) {
        l->bomb_won++;
        ld_release(pi, "the beach is shelled free - the landing comes", now);
        l->next_try = now;   // planned at the next look, the warships along
        return;
    }
    // 4 minutes of shelling from when the fleet is off the beach (14 tiles of the water in front), 7 at most from its
    // sailing: across Small Islands the way alone took 2-3 of the 4 minutes
    for (int k = 0; k < l->nesc && !l->arrive_t; k++) {
        ee_unit *u = l->esc[k];
        int dx = unit_get_x(u) - l->wx, dy = unit_get_y(u) - l->wy;
        if (ld_alive(u) && !ld_is_air(u) && unit_get_x(u) >= 0 && dx * dx + dy * dy <= 196) l->arrive_t = now ? now : 1;
    }
    uint32_t shell_t = l->arrive_t ? l->arrive_t : l->bomb_t;
    if ((alive * 2 < l->fleet0 && need > 0.7f * l->need0) || now - shell_t >= 240000 || now - l->bomb_t >= 420000) {
        l->aborted++; l->next_try = now + 300000;
        ld_release(pi, alive * 2 < l->fleet0 ? "half the fleet lost, the beach holds - bombardment called off" : "4 minutes of shelling - bombardment called off", now);
    }
}
// the landing's phases (landing_tick): 0 plan, 1 transports and passengers, 2 meeting point and boarding, 3 at sea
// (gathering, run in), 4 ashore and waves, 5 bombardment (ld_bombard_tick). Each returns 1 when the tick ends there.
internal struct ld_scan g_ld_scan;   // the phases' scan of the map (ld_unit) - filled where it is used
// phase 0: a target (the attack with the most land members), a beach, the size - or why not (ld_why)
internal void ld_phase_plan(ee_player *pl, unsigned pi, struct landing *l, ee_unit_mgr *um, uint8_t *dq, unsigned first, unsigned cnt, uint32_t now) {
    struct ld_scan *s = &g_ld_scan;
    (void)um;
    if (now < l->next_try) return;
    l->next_try = now + 30000;
    // the attack's target: the group with the most land members
    ee_player *tgt[16] = { 0 }; int tn[16] = { 0 }, nt = 0;
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        ee_player *t = g ? ag_get_target(g) : NULL;
        if (!t || !is_enemy(pl, t)) continue;
        int j = 0;
        while (j < nt && tgt[j] != t) j++;
        if (j == nt) { if (nt == 16) continue; tgt[nt++] = t; }
        for (ee_attack_subgroup **sp = ag_get_subgroups_begin(g); sp && sp < ag_get_subgroups_end(g); sp++)
            if (*sp && asg_is_land(*sp)) tn[j] += (int)(asg_get_units_end(*sp) - asg_get_units_begin(*sp));
    }
    ee_player *target = NULL;
    for (int j = 0, bn = 0; j < nt; j++) if (tn[j] > bn) { bn = tn[j]; target = tgt[j]; }
    if (!target) { ld_why(pi, now, "no attack on"); return; }
    memset(s, 0, sizeof *s); s->pl = pl; s->target = target;
    grid_each(ld_unit, s);
    if (!s->nd || !s->nb) { ld_why(pi, now, !s->nd ? "no dock" : "no building of the target seen"); return; }
    unsigned ocean = 0;
    for (int d = 0; d < s->nd && !ocean; d++) ocean = ocean_near(s->dx[d], s->dy[d], 4, NULL, NULL);
    int sx, sy, swx, swy;
    if (!ocean) { ld_why(pi, now, "no sea at its docks"); return; }
    // soldiers at home in land subgroups (within 20 tiles of its buildings - the base point can lie far off the town)
    ld_home_scan(pl);
    int home = 0;
    float ahp = 0, aatt = 0;
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++) {
            if (!*sp || !asg_is_land(*sp)) continue;
            for (ee_asg_unit **r = asg_get_units_begin(*sp); r && r < asg_get_units_end(*sp); r++) {
                ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                if (!ld_alive(u) || !is_soldier(unit_get_type_data(u))) continue;
                if (ld_at_home(unit_get_x(u), unit_get_y(u))) { home++; ahp += (float)unit_get_hp(u); aatt += ld_attack(u); }
            }
        }
    }
    // the size (the user: "early a single transport might do damage, later a large scale landing with 10 ships
    // might be needed"): as many soldiers as the beach needs, 1.25 times for what lies behind it - from those at
    // home it can spare (an enemy army at its door: one for each of its soldiers stays), on the transports it has or
    // can pay (its stock now, and one to save for), up to landing_transports. Enough for what holds the beach and
    // the soldiers it knows within 40 tiles (they come running), 1.5 times: 9 sent against the beach's tower alone
    // (just enough) lost one boarding and were called off
    int keep = home_threat(pl), avail = home - keep;
    if (avail < 6) { ld_why(pi, now, keep ? "an enemy army at its door - no soldiers to spare" : "fewer than 6 soldiers at home"); return; }
    int ttype = ld_transport_type(pl), cap = ld_cap(ttype);
    if (!ttype) { ld_why(pi, now, "no transport in its epoch"); return; }
    int afford = s->nt + ld_affordable(pl, ttype) + 1;
    if (afford > g_land_ships) afford = g_land_ships;
    if (avail > afford * cap) avail = afford * cap;
    float unit_pw = ld_strength(ahp / (float)home, aatt / (float)home), power = unit_pw * (float)avail;   // n alike soldiers: n times one
    struct ld_def d;
    if (!ld_site(pl, pi, s, ocean, power, &sx, &sy, &swx, &swy, &d)) {
        static char why[96];
        snprintf(why, sizeof why, "no landing site (its docks on sea %u, %u at a shore, %u at that sea)", ocean, g_ld_stat[pi].shore, g_ld_stat[pi].sea);
        ld_why(pi, now, why);
        return;
    }
    if (power < 1.2f * ld_need(&d) && g_land_bombard && ld_bombard_plan(pl, pi, l, target, s, ocean, power, sx, sy, swx, swy, &d, now, (avail + cap - 1) / cap > g_land_ships ? g_land_ships : (avail + cap - 1) / cap)) return;
    if (power < 1.2f * ld_need(&d)) {
        static char why[160];
        snprintf(why, sizeof why, "the weakest beach %d,%d needs %.0f (1.2 x %.0f), its army %.0f (soldiers there %.0f, defences %.0f, over the water %.0f)",
                 sx, sy, 1.2f * ld_need(&d), ld_need(&d), power, d.soldiers, d.towers, d.water);
        ld_why(pi, now, why);
        return;
    }
    float big = 1.4f * d.behind;   // as ld_need, with those behind
    int n = (int)ceilf(1.5f * (big > ld_need(&d) ? big : ld_need(&d)) / unit_pw);
    if (n < 6) n = 6;
    if (n > avail) n = avail;
    // the citizens along take room too; fewer of them when the soldiers would not fit
    int ships = (n + cap - 1) / cap, cits = ld_cits_for(ships, cap);
    {   // the sea on the way over (from its dock nearest the beach) against what it could take along
        int bd = -1;
        for (int k = 0; k < s->nd; k++) {
            int dd = (s->dx[k] - swx) * (s->dx[k] - swx) + (s->dy[k] - swy) * (s->dy[k] - swy);
            if (bd < 0 || dd < bd) { bd = dd; l->dkx = s->dx[k]; l->dky = s->dy[k]; }
        }
        static struct ld_sea e, o;
        ee_unit_type *ttd = type_data(ttype);
        memset(&o, 0, sizeof o);
        ld_sea_threat(pl, ttd, l->dkx, l->dky, swx, swy, sx, sy, &e);
        for (int k = 0, m = 0; k < s->nw && m < 6; k++) if (ld_sea_useful(&e, s->war[k])) { ld_sea_add(&o, &e, s->war[k]); m++; }
        for (int k = 0, m = 0; k < s->na && m < 8; k++) if (ld_sea_useful(&e, s->air[k])) { ld_sea_add(&o, &e, s->air[k]); m++; }
        float risk = ld_sea_risk(&e, &o, (float)((ships + 1) * (ttd ? type_get_hp(ttd) : 1000)));
        if (risk > 0.4f) {
            static char why[200];
            snprintf(why, sizeof why, "the sea on the way to %d,%d is held - %d afloat, %d submarines, %d aircraft that hit transports (risk %.2f)",
                     sx, sy, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk);
            ld_why(pi, now, why);
            return;
        }
    }
    ships = (n + cits + cap - 1) / cap;
    if (ships > afford) ships = afford;
    cits = ld_cits_for(ships, cap);
    if (ships * cap - cits < n) cits = ships * cap - n > 0 ? ships * cap - n : 0;
    n = ships * cap - cits;   // the transports full
    if (n > avail) n = avail;
    l->want_cits = cits;
    l->want_ships = ships; l->want_n = n; l->unit_pw = unit_pw; l->need = ld_need(&d);
    l->target = target; l->sx = sx; l->sy = sy; l->ocean = (int)ocean; l->wx = swx; l->wy = swy; l->def = d;
    l->phase = 1; l->since = now; l->tries++; l->trained_t = 0; l->boarded_t = 0; l->switched = 0; l->tr_missing = 0; l->tr_got = 0; l->sea_t = 0;
    l->nesc = 0; l->nfrozen = 0;
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: plans a landing on P%d at %d,%d (sea %u) with %d soldiers and %d citizens on %d transports (type %d, %d each) - "
                       "%d at home, %d kept, %d can go (strength %.0f, %d transports it has or can pay); "
                       "the beach needs %.0f, with those behind %.0f (soldiers %.0f%s, defences %.0f, over the water %.0f), %d transports, %d warships\n",
                       pi, now / 60000.0, player_get_index(target), sx, sy, ocean, n, cits, ships, ttype, cap, home, keep, avail, power, afford,
                       ld_need(&d), big, d.soldiers, d.assumed ? " - not looked at, assumed" : "", d.towers, d.water, s->nt, s->nw);
    return;
}
// phase 1: the transports (trained when missing), the warships and aircraft along, the passengers' subgroups;
// 1 = waiting (or given up), 0 = boarding begins (phase 2)
internal int ld_phase_ships(ee_player *pl, unsigned pi, struct landing *l, ee_unit_mgr *um, uint8_t *dq, unsigned first, unsigned cnt, uint32_t now) {
    struct ld_scan *s = &g_ld_scan;
    memset(s, 0, sizeof *s); s->pl = pl; s->target = l->target;
    grid_each(ld_unit, s);
    int want = l->want_ships;
    if (s->nt < want && now - l->since >= 180000 && s->nt) {
        // not all there after 3 minutes: those it has, if they carry enough for the beach (a size it cannot reach in
        // time - no waiting for it)
        int cap = ld_cap(ld_transport_type(pl)), cits = ld_cits_for(s->nt, cap), n = s->nt * cap - cits < l->want_n ? s->nt * cap - cits : l->want_n;
        if (l->unit_pw * (float)n >= l->need) {
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: goes with %d of %d transports (%d soldiers, strength %.0f, the beach needs %.0f)\n",
                               pi, now / 60000.0, s->nt, want, n, l->unit_pw * (float)n, l->need);
            l->want_ships = want = s->nt; l->want_n = n; l->want_cits = cits;
        }
    }
    if (s->nt < want) {
        if (now - l->since >= 360000) { l->aborted++; l->next_try = now + 600000; ld_release(pi, "no transports - given up", now); return 1; }
        if (g_log && (now - l->since) % 60000 < 2000) {
            int *st = player_stock_at(pl);
            fprintf(g_log, "# landing: P%u at %.1f min: waits for transports - has %d of %d, %d docks, stock %d,%d,%d,%d,%d\n", pi, now / 60000.0,
                    s->nt, want, s->nd, st[2], st[3], st[4], st[5], st[6]);
        }
        l->tr_missing = want - s->nt;
        // what was ordered takes time (ordered every 20 s: 7); nothing taken (not affordable): again in 20 s
        if (!l->trained_t || now - l->trained_t >= (l->tr_got ? 120000u : 20000u)) {
            l->trained_t = now;
            int tt = ld_transport_type(pl);
            if (tt) {
                int req[4] = { tt, want - s->nt, 0, 0 };
                unsigned got = 0;
                l->tr_type = tt;
                g_priority_mil = 1; g_landing_order = 1;
                hook_train_units(um, req, (uintptr_t)&got);
                g_priority_mil = 0; g_landing_order = 0;
                l->tr_got = (int)got;
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: trains %u transports (type %d), has %d\n", pi, now / 60000.0, got, tt, s->nt);
            }
        }
        return 1;
    }
    // the ships, their subgroups, the passengers' subgroups
    ld_home_scan(pl);
    l->nships = 0; l->nfrozen = 0; l->npass = 0;
    for (int k = 0; k < s->nt && l->nships < want; k++) l->ships[l->nships++] = s->tr[k];
    // the warships go ahead from now on: they shell the beach while the troops board (the user: "bombing the landing
    // area free with planes and warships first") - not before the transports are there: sent while those were
    // being built, P3's warships waited off the beach and 3 of 4 were sunk, the transports never came
    // up to 6 warships - the frigates first when submarines are on the way, the cruisers when aircraft are; and
    // its fighters only against aircraft
    static struct ld_sea e;
    ld_sea_threat(pl, ld_tr_td(l), l->dkx, l->dky, l->wx, l->wy, l->sx, l->sy, &e);
    // up to 6 warships, first those whose weapons hit the threats on the way (the game's table)
    for (int pass = 0; pass < 2; pass++)
        for (int k = 0; k < s->nw && l->nesc < (g_land_bombard ? 10 : 6); k++) {   // bombard: the fleet stays along
            ee_unit *u = s->war[k];
            if (u && (pass || ld_sea_useful(&e, u))) { l->esc[l->nesc++] = u; s->war[k] = NULL; }
        }
    // its aircraft too: they bomb the beach's defences and soldiers, else fly over it - a look at the beach
    // (honest_vision) before the transports commit
    // aircraft: those that hit a threat on the way or the beach's tower (a fighter that hits neither stays home)
    ee_unit_type *tw = type_data(UT(B_GUARD_TOWER_WW1));
    for (int k = 0; k < s->na && l->nesc < 14; k++)
        if (ld_sea_useful(&e, s->air[k]) || ld_factor(unit_get_type_data(s->air[k]), ld_pidx(s->air[k]), tw, player_get_index(l->target)) > 0.0f)
            l->esc[l->nesc++] = s->air[k];
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++) {
            ee_attack_subgroup *sg = *sp;
            if (!sg || l->nfrozen >= 96) continue;
            int mine = 0, home = 0, n = 0;
            for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg); r++) {
                ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                if (!ld_alive(u)) continue;
                for (int j = 0; j < l->nships; j++) mine |= u == l->ships[j];
                for (int j = 0; j < l->nesc; j++) mine |= u == l->esc[j];
                n++; home += ld_at_home(unit_get_x(u), unit_get_y(u));
            }
            if (mine) { l->frozen[l->nfrozen++] = sg; continue; }
            if (!asg_is_land(sg) || !n || home * 2 < n ||
                l->npass >= l->want_n)
                continue;
            l->frozen[l->nfrozen++] = sg;
            for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg); r++) {
                ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                if (ld_alive(u) && unit_get_x(u) >= 0 && is_soldier(unit_get_type_data(u)) && !expand_has(pl, u) && l->npass < l->want_n && l->npass < LD_PASS)
                    l->pass[l->npass++] = u;
            }
        }
    }
    if (l->npass < 6) { l->aborted++; l->next_try = now + 300000; ld_release(pi, "too few passengers", now); return 1; }
    l->phase = 2; l->since = now; l->reissue = 0; l->gathered = 0;
    {   // the meeting point: the shore of the landing's sea nearest the passengers (on their land) - boarding
        // straight away, soldiers 30-40 tiles off walked to transports strewn along the coast: after 3 minutes
        // P2 had fewer than half aboard
        float px = 0, py = 0; int pn = 0;
        for (int k = 0; k < l->npass; k++) { px += unit_get_x(l->pass[k]); py += unit_get_y(l->pass[k]); pn++; }
        int x0 = (int)(px / pn), y0 = (int)(py / pn), found = 0;
        unsigned cont = area_id(unit_get_x(l->pass[0]), unit_get_y(l->pass[0]), 2);
        for (int d = 0; d <= 40 && !found; d++)
            for (int dx = -d; dx <= d && !found; dx++)
                for (int dy = -d; dy <= d && !found; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                    int tx = x0 + dx, ty = y0 + dy, wx, wy;
                    if (area_id(tx, ty, 2) != cont || area_id(tx, ty, 1) || !ld_tile_free(tx, ty)) continue;
                    if ((int)ocean_near(tx, ty, 2, &wx, &wy) != l->ocean) continue;
                    l->ex = tx; l->ey = ty; l->ewx = wx; l->ewy = wy; found = 1;
                }
        l->meet = found;
        if (!found) l->gathered = 1;   // none: board where they are
        // citizens for the bridgehead (the user: "a few citizens for a defended bridgehead"): the nearest to the
        // meeting point, within 25 tiles
        struct ld_cits c = { pl, found ? l->ex : x0, found ? l->ey : y0, 0, { 0 }, { 0 } };
        l->ncit = 0;
        if (l->want_cits) grid_each(ld_cit_unit, &c);
        for (int k = 0; k < c.n && l->ncit < l->want_cits; k++) {
            int b = -1;
            for (int j = 0; j < c.n; j++) if (c.u[j] && (b < 0 || c.d[j] < c.d[b])) b = j;
            if (b < 0) break;
            l->cjob[l->ncit] = -1; l->cit[l->ncit++] = c.u[b]; c.u[b] = NULL;
        }
        ld_base_plan(l);
        if (found && g_log) fprintf(g_log, "# landing: P%u at %.1f min: they meet at %d,%d (passengers around %d,%d)\n", pi, now / 60000.0, l->ex, l->ey, x0, y0);
    }
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: boarding %d soldiers on %d transports, %d warships and aircraft along\n", pi, now / 60000.0, l->npass, l->nships, l->nesc);
    return 0;
}
// phase 2: to the meeting point, then boarding - the last look at the beach and the sea before it sails (phase 3)
internal void ld_phase_board(ee_player *pl, unsigned pi, struct landing *l, ee_unit_mgr *um, uint8_t *dq, unsigned first, unsigned cnt, uint32_t now) {
    struct ld_scan *s = &g_ld_scan;
    (void)um; (void)dq; (void)first; (void)cnt;
    if (l->phase == 2 && !l->gathered) {   // to the meeting point first: the transports to the water in front, the soldiers to the shore
        int there = 0;
        for (int k = 0; k < l->nships; k++) {
            ee_unit *sh = l->ships[k];
            int dx = unit_get_x(sh) - l->ewx, dy = unit_get_y(sh) - l->ewy;
            if (dx * dx + dy * dy <= 49) { there++; continue; }
            if (!goal_near(sh, (float)l->ewx + 0.5f, (float)l->ewy + 0.5f, 2.0f))
                l->orders += unit_goal(sh, goal_move((float)l->ewx + 0.5f, (float)l->ewy + 0.5f), 0, 0x1F40);
        }
        int close = 0;
        for (int k = 0; k < l->npass; k++) {
            ee_unit *u = l->pass[k];
            if (ld_aboard(u)) { close++; continue; }
            int dx = unit_get_x(u) - l->ex, dy = unit_get_y(u) - l->ey;
            if (dx * dx + dy * dy <= 64) { close++; continue; }
            if (!goal_near(u, (float)l->ex + 0.5f, (float)l->ey + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->ex + 0.5f, (float)l->ey + 0.5f), 0, 0x1F40);
        }
        for (int k = 0; k < l->ncit; k++) {
            ee_unit *u = l->cit[k];
            if (!ld_alive(u) || ld_aboard(u) || goal_near(u, (float)l->ex + 0.5f, (float)l->ey + 0.5f, 3.0f)) continue;
            int dx = unit_get_x(u) - l->ex, dy = unit_get_y(u) - l->ey;
            if (dx * dx + dy * dy > 16) l->orders += unit_goal(u, goal_move((float)l->ex + 0.5f, (float)l->ey + 0.5f), 0, 0x1F40);
        }
        // the transports there and most soldiers near (or 2 minutes): board
        if ((there == l->nships && close * 4 >= l->npass * 3) || now - l->since >= 120000) {
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: at the meeting point - %d of %d transports, %d of %d soldiers\n", pi, now / 60000.0,
                               there, l->nships, close, l->npass);
            l->gathered = 1; l->since = now; l->reissue = 0;
        }
        return;
    }
    // loaded and idle at the shore: the game unloads it (0x483E10) - out onto open water. So is one with an unload: the
    // landing gives none while boarding - it is the game's (the transport manager's crossing, given before the landing
    // took the transport and resumed after the landing's load goal; its retry 0x48D022 kept it): it carried 8-12
    // soldiers 20-70 tiles off and put them ashore there (2 of 48 island runs)
    if (l->phase == 2)
        for (int k = 0; k < l->nships; k++) {
            ee_unit *sh = l->ships[k];
            ee_goal *g = unit_get_goal(sh);
            if ((g && goal_get_state(g) != 1 && !goal_is_unload(g)) || units_inside(sh) <= 0) continue;
            int wx, wy;
            if (g && goal_is_unload(g) && g_log)
                fprintf(g_log, "# landing: P%u at %.1f min: transport %d with %d aboard had the game's unload to %d,%d while boarding - kept off the shore\n",
                        pi, now / 60000.0, k, units_inside(sh), goal_get_unload_x(g), goal_get_unload_y(g));
            if (ld_open_water(unit_get_x(sh), unit_get_y(sh), (unsigned)l->ocean, &wx, &wy))
                l->orders += unit_goal(sh, goal_move((float)wx + 0.5f, (float)wy + 0.5f), 0, 0x1F40);
        }
    if (l->phase == 2) {   // board
        int aboard = 0;
        for (int k = 0; k < l->npass; k++) aboard += ld_aboard(l->pass[k]);
        // all aboard, 9 in 10 after a minute (P2 waited 3 minutes for its last 3 of 36), or 3 minutes
        if (aboard == l->npass || (now - l->since >= 60000 && aboard * 10 >= l->npass * 9) || now - l->since >= 180000) {
            if (aboard * 2 < l->npass || aboard < 6) { l->aborted++; l->next_try = now + 600000; ld_release(pi, "boarding failed", now); return; }
            // the last word before the commit: the beach now (the warships have shelled it meanwhile) against what is
            // aboard; too strong: the warships get up to 3 more minutes, then it is called off (unloaded at home).
            // Strong enough: it sails at once - waiting at home for the warships to take on the defences cost P3 half
            // of those aboard (unloaded again, 20 -> 11) and 2 warships did not bring down the tower in 4 minutes
            if (!l->boarded_t) l->boarded_t = now;
            struct ld_def d;
            ld_defence(pl, l->sx, l->sy, l->wx, l->wy, &d);
            float phh = 0, pa = 0;
            for (int k = 0; k < l->npass; k++) if (ld_aboard(l->pass[k])) { phh += (float)unit_get_hp(l->pass[k]); pa += ld_attack(l->pass[k]); }
            float pw = ld_strength(phh, pa);
            int esc_busy = l->nesc && now - l->boarded_t < 180000;
            if (pw < ld_need(&d) && !esc_busy) {
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the beach needs %.0f (soldiers %.0f, defences %.0f, over the water %.0f), aboard %.0f\n",
                                   pi, now / 60000.0, ld_need(&d), d.soldiers, d.towers, d.water, pw);
                ld_unload_home(l);
                l->aborted++; l->next_try = now + 600000; ld_release(pi, "too strong a beach - called off, unloaded at home", now);
                return;
            }
            // the warships found the beach held more strongly than thought (honest_vision: P3 planned on a beach it had
            // not looked at, its warships found 1967 of soldiers there): another beach, if one is weak enough now
            if (pw < ld_need(&d) && !d.assumed && !l->switched) {
                memset(s, 0, sizeof *s); s->pl = pl; s->target = l->target;
                grid_each(ld_unit, s);
                int nx, ny, nwx, nwy;
                struct ld_def nd;
                if (ld_site(pl, pi, s, (unsigned)l->ocean, pw, &nx, &ny, &nwx, &nwy, &nd) && ld_need(&nd) <= pw && (nx != l->sx || ny != l->sy)) {
                    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the beach %d,%d needs %.0f, aboard %.0f - switches to %d,%d (needs %.0f%s)\n",
                                       pi, now / 60000.0, l->sx, l->sy, ld_need(&d), pw, nx, ny, ld_need(&nd), nd.assumed ? ", not looked at" : "");
                    l->sx = nx; l->sy = ny; l->wx = nwx; l->wy = nwy; l->def = nd; l->switched = 1; l->boarded_t = now;
                    return;
                }
            }
            if (g_land_trace && g_log && (now - l->boarded_t) % 10000 < 2000)
                for (int k = 0; k < l->nships; k++)
                    fprintf(g_log, "# ldtrace: P%u %.2f waiting ship %d at %d,%d aboard %d goal %s\n", pi, now / 60000.0, k, unit_get_x(l->ships[k]),
                            unit_get_y(l->ships[k]), units_inside(l->ships[k]), rtti_name(unit_get_goal(l->ships[k])));
            {   // the sea on the way (the user: subs and planes on patrol before the coast): held - the escorts get 3 minutes
                // to clear it, then it is called off
                struct ld_sea e;
                float risk = ld_sea_now(pl, l, l->meet ? l->ewx : unit_get_x(l->ships[0]), l->meet ? l->ewy : unit_get_y(l->ships[0]), &e);
                if (risk > 0.4f) {
                    if (!l->sea_t) l->sea_t = now;
                    if (now - l->sea_t >= 180000) {
                        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the sea on the way is held - %d warships, %d submarines, %d aircraft (risk %.2f)\n",
                                           pi, now / 60000.0, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk);
                        ld_unload_home(l);
                        l->aborted++; l->next_try = now + 600000; ld_release(pi, "the sea held - called off, unloaded at home", now);
                        return;
                    }
                    if (g_log && (now - l->sea_t) % 30000 < 2000)
                        fprintf(g_log, "# landing: P%u at %.1f min: waits - on the way %d afloat, %d submarines, %d aircraft that hit transports (risk %.2f), %d escorts\n",
                                pi, now / 60000.0, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk, l->nesc);
                    return;
                }
                l->sea_t = 0;
            }
            // honest_vision: a beach nobody has looked at lately - the warships on their way look first
            if (pw < ld_need(&d) || (d.assumed && esc_busy)) {
                if (g_log && (now - l->boarded_t) % 30000 < 2000)
                    fprintf(g_log, "# landing: P%u at %.1f min: waits for the warships (%d) - the beach needs %.0f (soldiers %.0f%s, defences %.0f, over the water %.0f), aboard %.0f\n",
                            pi, now / 60000.0, l->nesc, ld_need(&d), d.soldiers, d.assumed ? " - not looked at, assumed" : "", d.towers, d.water, pw);
                return;
            }
            // those left ashore go back to the game: subgroups without a passenger, ship or warship of the landing
            int n = 0;
            for (int k = 0; k < l->npass; k++) if (ld_aboard(l->pass[k])) l->pass[n++] = l->pass[k];
            l->npass = n; l->launch_n = n;
            int nc = 0;
            for (int k = 0; k < l->ncit; k++) if (ld_alive(l->cit[k]) && ld_aboard(l->cit[k])) { l->cjob[nc] = -1; l->cit[nc++] = l->cit[k]; }
            l->ncit = nc;
            int nf = 0;
            for (int f = 0; f < l->nfrozen; f++) {
                ee_attack_subgroup *sg = l->frozen[f];
                int keep = 0;
                for (ee_asg_unit **r = asg_get_units_begin(sg); r && r < asg_get_units_end(sg) && !keep; r++) {
                    ee_unit *u = *r ? asgu_get_unit(*r) : NULL;
                    for (int j = 0; j < l->npass && !keep; j++) keep = u == l->pass[j];
                    for (int j = 0; j < l->nships && !keep; j++) keep = u == l->ships[j];
                    for (int j = 0; j < l->nesc && !keep; j++) keep = u == l->esc[j];
                }
                if (keep) l->frozen[nf++] = sg;
            }
            l->nfrozen = nf;
            l->staged = 0; l->stage_t = now;
            if (l->nships >= 3 && ld_stage(l, pi, &l->stx, &l->sty)) {
                for (int k = 0; k < l->nships; k++) l->orders += unit_goal(l->ships[k], goal_move((float)l->stx + 0.5f, (float)l->sty + 0.5f), 0, 0x1F40);
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the transports gather at sea at %d,%d\n", pi, now / 60000.0, l->stx, l->sty);
            } else ld_run_in(l, pi);
            l->phase = 3; l->since = now; l->reissue = 0; l->all_ashore = 0; memset(l->tries_at, 0, sizeof l->tries_at);
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: sails with %d soldiers on %d transports to %d,%d\n", pi, now / 60000.0, n, l->nships, l->sx, l->sy);
            ai_say(pl, CHAT_ATTACK, "[ai] landing with %d units", n);
            return;
        }
        if (l->reissue && now - l->reissue < 30000) return;
        l->reissue = now;
        // per ship its share of those not aboard
        // per ship its share of those not aboard, the citizens with the first (in its share: full transports had no
        // room for them)
        int per = (l->npass + l->ncit + l->nships - 1) / l->nships;
        for (int k = 0; k < l->nships; k++) {
            ee_unit *list[LD_PASS + 10]; int m = 0;   // the passengers and the citizens (cit[10])
            int lo = k ? k * per - l->ncit : 0, hi = (k + 1) * per - l->ncit;
            if (lo < 0) lo = 0;
            if (k == l->nships - 1 || hi > l->npass) hi = l->npass;
            for (int j = lo; j < hi; j++) if (!ld_aboard(l->pass[j])) list[m++] = l->pass[j];
            for (int j = 0; j < l->ncit && !k; j++) if (ld_alive(l->cit[j]) && !ld_aboard(l->cit[j])) list[m++] = l->cit[j];   // the citizens with the first
            if (!m) continue;
            ee_goal *g = goal_load(l->ships[k], list, m);
            if (!g) continue;   // refused: deleted, no parts from it
            l->orders++;
            for (int j = 0; j < l->ncit && !k; j++)
                if (ld_alive(l->cit[j]) && !ld_aboard(l->cit[j])) l->orders += unit_goal(l->cit[j], goal_load_part(g), 0, 0x1F40);
            for (int j = lo; j < hi; j++) {
                if (ld_aboard(l->pass[j])) continue;
                l->orders += unit_goal(l->pass[j], goal_load_part(g), 0, 0x1F40);
            }
        }
        return;
    }
}
internal void ld_trace_ships(struct landing *l, unsigned pi, uint32_t now) {   // landing_trace (test)
    for (int k = 0; k < l->nships; k++) {
        ee_unit *sh = l->ships[k];
        ee_goal *g = unit_get_goal(sh);
        int n = units_inside(sh);
        if (!n) continue;
        fprintf(g_log, "# ldtrace: P%u %.2f ship %d at %d,%d hp %d aboard %d goal %s", pi, now / 60000.0, k, unit_get_x(sh), unit_get_y(sh),
                unit_get_hp(sh), n, rtti_name(g));
        if (g && goal_is_unload(g))
            fprintf(g_log, " @%p water %d,%d / %d,%d point %d,%d reach %.1f state %d", (void *)g, goal_get_water_x(g), goal_get_water_y(g), goal_get_water2_x(g),
                    goal_get_water2_y(g), goal_get_unload_x(g), goal_get_unload_y(g), sqrtf(goal_get_reach_sq(g)), goal_get_state(g));
        fputs("\n", g_log);
    }
}
// phase 3, gathering at sea: all transports there (within 6 tiles) or 75 s - then the run in
internal void ld_phase_gather(struct landing *l, unsigned pi, uint32_t now) {
    int there = 0;
    for (int k = 0; k < l->nships; k++) {
        ee_unit *sh = l->ships[k];
        int dx = unit_get_x(sh) - l->stx, dy = unit_get_y(sh) - l->sty;
        if (dx * dx + dy * dy <= 36) { there++; continue; }
        if (!goal_near(sh, (float)l->stx + 0.5f, (float)l->sty + 0.5f, 2.0f))
            l->orders += unit_goal(sh, goal_move((float)l->stx + 0.5f, (float)l->sty + 0.5f), 0, 0x1F40);
    }
    if (there < l->nships && now - l->stage_t < 75000) return;
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: %d of %d transports gathered after %.0f s - the run in\n", pi, now / 60000.0,
                       there, l->nships, (now - l->stage_t) / 1000.0);
    ld_run_in(l, pi);
    l->since = now;
    return;
}
// phases 3 and 4: sailing and unloading, ashore - the waves, the bridgehead, raiding
internal void ld_phase_land(ee_player *pl, unsigned pi, struct landing *l, ee_unit_mgr *um, uint8_t *dq, unsigned first, unsigned cnt, uint32_t now) {
    (void)um;
    // sail and unload (3), ashore (4) - while some are still aboard, the ones ashore already fight
    int aboard = 0, ashore = 0;
    float cx = 0, cy = 0;
    for (int k = 0; k < l->npass; k++) {
        if (ld_aboard(l->pass[k])) { aboard++; continue; }
        ashore++; cx += unit_get_x(l->pass[k]); cy += unit_get_y(l->pass[k]);
    }
    if (aboard) {
        // a transport with soldiers aboard and no unload order (it ended without unloading, or something else gave the
        // transport an order): the unload again at once - every 0.5 s while sailing. Committed: no turning back while
        // transports with soldiers aboard are afloat (a player lands before they are sunk)
        for (int k = 0; k < l->nships; k++) {
            ee_unit *sh = l->ships[k];
            ee_goal *cur = unit_get_goal(sh);
            if (cur && goal_is_unload(cur)) continue;
            if (units_inside(sh) <= 0) continue;   // nothing aboard
            ld_reunload(l, pi, k, now);
        }
    }
    if (l->phase == 3) {
        if (ashore) {
            l->phase = 4; l->obj = NULL; l->landings++;
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: ashore near %d,%d - %d landed, %d still aboard\n", pi, now / 60000.0,
                               l->sx, l->sy, ashore, aboard);
        } else if (now - l->since >= 600000) { l->aborted++; ld_release(pi, "no landing within 10 minutes", now); return; }
        else return;
    }
    if (!aboard && l->nships && l->phase == 4 && !l->all_ashore) {
        l->all_ashore = 1; l->wt = now;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: all %d ashore\n", pi, now / 60000.0, ashore);
    }
    if (l->phase == 4 && l->nships) ld_wave(pl, pi, l, dq, first, cnt, now);
    // ashore: one group (a wave at sea goes on: it lands and holds the beach)
    int alive = ashore + aboard;
    if ((alive * 4 < l->launch_n || !alive) && l->wst != 3) {
        if (l->wst) ld_unload_home(l);
        ld_release(pi, "bridgehead spent", now);
        return;
    }
    if (!ashore) return;
    if (l->ncit) ld_bridgehead(pl, pi, l, now);
    ld_capture(pl, pi, l, now);
    cx /= ashore; cy /= ashore;
    ee_unit *foe = ld_raid_target(pl, l->target, (int)cx, (int)cy, 12);
    if (!l->obj || !ld_alive(l->obj) || unit_get_owner(l->obj) != l->target) {
        unsigned cont = area_id(l->sx, l->sy, 2);   // the beach's land: the centre of a big landing spread along the coast lay in the water
        l->obj = ld_objective(pl, pi, l->target, cont, (int)cx, (int)cy);
        if (!l->obj) l->obj = enemy_objective(pl, pi, l->target, cont, (int)cx, (int)cy);   // only defences left
        if (l->obj) { l->ox = unit_get_x(l->obj); l->oy = unit_get_y(l->obj); }
    }
    for (int k = 0; k < l->npass; k++) {
        ee_unit *u = l->pass[k];
        if (ld_aboard(u) || unit_fighting(u)) continue;
        if (foe) {
            ee_unit *e = ld_raid_target(pl, l->target, unit_get_x(u), unit_get_y(u), 14);
            l->orders += unit_goal(u, goal_attack(u, e ? e : foe), 0, 0x1F40);
        } else if (l->obj) {
            float dx = (float)(l->ox - unit_get_x(u)), dy = (float)(l->oy - unit_get_y(u));
            if (dx * dx + dy * dy <= 36.0f) l->orders += unit_goal(u, goal_attack(u, l->obj), 0, 0x1F40);
            else if (!goal_near(u, (float)l->ox + 0.5f, (float)l->oy + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->ox + 0.5f, (float)l->oy + 0.5f), 0, 0x1F40);
        }
    }
    if (!foe && !l->obj) ld_release(pi, "nothing left to attack ashore", now);
}
internal void landing_tick(ee_player *pl, uint32_t now) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    struct landing *l = &g_ld[pi];
    if (!g_landing) return;
    if (now < l->last) memset(l, 0, sizeof *l);   // a new game
    ld_fu_check(now);
    if (now - l->last < (l->phase == 3 ? 500u : 2000u)) {   // sailing: a transport stalled off the beach is sunk
        if (l->phase == 4 && l->wst == 3 && l->nships && now - l->wre >= 500u) ld_wave_reunload(l, pi, now);   // a wave at sea too
        return;
    }
    l->last = now;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    ee_unit_mgr *um = cm ? cm_get_unit_mgr(cm) : NULL;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? am_get_commander(cbm) : NULL;
    if (!cmd || !bm || !um || bm_get_base_x(bm) == -10000) { if (l->phase) ld_release(pi, "no base", now); return; }
    uint8_t *dq = ac_groups_at(cmd);
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    {   // subgroups the game deleted meanwhile (never read a stored one without this)
        int nf = 0;
        for (int f = 0; f < l->nfrozen; f++) if (asg_live(pl, l->frozen[f])) l->frozen[nf++] = l->frozen[f];
        l->nfrozen = nf;
    }
    if (l->phase == 0) { ld_phase_plan(pl, pi, l, um, dq, first, cnt, now); return; }
    if (!l->target || !is_enemy(pl, l->target)) { ld_release(pi, "no enemy any more", now); return; }
    if (l->phase == 5) {
        for (int k = 0; k < l->nesc; k++) if (!ld_alive(l->esc[k])) { l->esc[k--] = l->esc[--l->nesc]; }
        ld_refreeze(l, dq, first, cnt);
        if (l->bomb_t) ld_escorts(pl, l);
        ld_bombard_tick(pl, pi, l, um, now);
        return;
    }
    for (int k = 0; k < l->nesc; k++) if (!ld_alive(l->esc[k])) { l->esc[k--] = l->esc[--l->nesc]; }
    if (l->phase >= 2) { ld_refreeze(l, dq, first, cnt); if (l->phase != 5 || l->bomb_t) ld_escorts(pl, l); }
    if (l->phase == 1 && ld_phase_ships(pl, pi, l, um, dq, first, cnt, now)) return;
    // drop the dead - and a slot whose unit is no longer the player's transport/unit: a sunk transport goes back to
    // the unit pool (0x9413B0) and its memory can be another unit (alive) by the next tick; its load goal then crashed
    // in 0x61487F (the holder's type not a transport class, 0x61496C)
    for (int k = 0; k < l->nships; k++) {
        ee_unit *sh = l->ships[k];
        ee_unit_type *td = ld_alive(sh) ? unit_get_type_data(sh) : NULL;
        if (!td || unit_get_owner(sh) != pl || !is_transport(type_get_id(td))) { l->ships[k--] = l->ships[--l->nships]; }
    }
    for (int k = 0; k < l->npass; k++) if (!ld_alive(l->pass[k]) || unit_get_owner(l->pass[k]) != pl) { l->pass[k--] = l->pass[--l->npass]; }
    if (!l->nships && l->phase < 4) { l->aborted++; l->next_try = now + 600000; ld_release(pi, "transports lost", now); return; }
    if (!l->nships && l->phase == 4 && l->wst) {   // every transport sunk with a wave under way (ld_wave needs one): the wave
        int landed = 0;                            // ends - those ashore join the bridgehead, those at home are free again
        for (int k = 0; k < l->nw; k++) {          // (before: phase 4 for good - the release waited for wst != 3)
            ee_unit *u = l->wpass[k];
            if (l->wst == 3 && ld_alive(u) && !ld_aboard(u) && l->npass < LD_PASS) { l->pass[l->npass++] = u; l->launch_n++; landed++; }
        }
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - every transport lost (%d of %d ashore)\n", pi, now / 60000.0, l->waves + 1, landed, l->nw);
        l->nw = 0; l->wst = 0; l->waves = g_land_waves; l->wt = now;
        ld_prune_frozen(l);
    }
    if (l->phase == 2) { ld_phase_board(pl, pi, l, um, dq, first, cnt, now); return; }
    if (g_land_trace && g_log && l->phase >= 3) ld_trace_ships(l, pi, now);
    if (l->phase == 3 && !l->staged) { ld_phase_gather(l, pi, now); return; }
    ld_phase_land(pl, pi, l, um, dq, first, cnt, now);
}

// Ashore the landing raids (the user: "my back is basically undefended ... could cause a lot of trouble"): what it
// sees within r tiles, by kind - soldiers and siege within 8 (they fight back), citizens, buildings of the target
// within 8 that are no defence, defences only when they reach it; nearest first within a kind. (Before: soldiers,
// then any defence within 12, then buildings - the troops walked into towers past the citizens.)
internal ee_unit *ld_raid_target(ee_player *pl, ee_player *target, int x, int y, int r) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return NULL;
    ee_unit *best[4] = { 0 };
    float bd[4] = { 1e9f, 1e9f, 1e9f, 1e9f };
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_hp(u) <= 0) continue;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        float dx = (float)(unit_get_x(u) - x), dy = (float)(unit_get_y(u) - y), d2 = dx * dx + dy * dy, rr, a;
        if (d2 > (float)(r * r)) continue;
        int fam = type_get_family(td), type = type_get_id(td), k;
        if (fam == 3 || fam == 7 || fam == 17 || fam == 4) continue;   // aircraft, ships
        if (is_soldier(td) || is_siege(type)) { if (d2 > 64.0f) continue; k = 0; }
        else if (is_citizen_type(type)) k = 1;   // citizens
        else if (defence_stats(u, &rr, &a)) { if (d2 > (rr + 1.0f) * (rr + 1.0f)) continue; k = 3; }
        else if (o == target && (fam == 2 || fam == 29) && !is_wall(type) && d2 <= 64.0f) k = 2;
        else continue;
        if (d2 < bd[k]) { bd[k] = d2; best[k] = u; }
    }
    for (int k = 0; k < 4; k++) if (best[k]) return best[k];
    return NULL;
}
// the next objective ashore: the nearest building of the target it knows that is no defence (its distance plus 3 per
// tower covering it, on the landed continent)
struct ld_obj { ee_player *pl; ee_player *target; unsigned pi, cont; int x, y; ee_unit *best; float score; };
internal void ld_obj_unit(ee_unit *u, void *ctx) {
    struct ld_obj *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    int fam = type_get_family(td);
    float rr, a;
    if (unit_get_owner(u) != s->target || (fam != 2 && fam != 29) || unit_get_hp(u) <= 0 ||
        is_wall(type_get_id(td)) || defence_stats(u, &rr, &a) || !seen_by(u, s->pl))
        return;
    int x = unit_get_x(u), y = unit_get_y(u);
    float dx = (float)(x - s->x), dy = (float)(y - s->y), sc = sqrtf(dx * dx + dy * dy) + 3.0f * threat_at(s->pi, x, y);
    if ((s->best && sc >= s->score) || !cont_ok(x, y, s->cont)) return;
    s->best = u; s->score = sc;
}
internal ee_unit *ld_objective(ee_player *pl, unsigned pi, ee_player *target, unsigned cont, int x, int y) {
    struct ld_obj s = { pl, target, pi, cont, x, y, NULL, 0 };
    grid_each(ld_obj_unit, &s);
    return s.best;
}

// terrain x y [r] (test): the map around a tile, a character per tile: ~ ocean, . land (its continent; digit: another
// continent), ? neither id, then on top: T tree or other gaia object, B building, s soldier, c citizen (player's
// digit for buildings of others when P given: B for player P only... kept simple: B any building, u any unit)
internal void cmd_terrain(char *out, const char *args) {
    int x0 = 0, y0 = 0, r = 20;
    sscanf(args, "%d %d %d", &x0, &y0, &r);
    if (r > 40) r = 40;
    if (r < 2) r = 2;
    static char g[81 * 82];
    int w = 2 * r + 1;
    unsigned c0 = area_id(x0, y0, 2);
    for (int y = 0; y < w; y++)
        for (int x = 0; x < w; x++) {
            int tx = x0 - r + x, ty = y0 - r + y;
            unsigned o = area_id(tx, ty, 1), c = area_id(tx, ty, 2);
            g[y * w + x] = o ? '~' : c == c0 && c ? '.' : c ? (char)('0' + c % 10) : '?';
        }
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (grid) {
        for (struct grid_it it = grid_it_box(x0, y0, r, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            if (!td) continue;
            int x = unit_get_x(u) - x0 + r, y = unit_get_y(u) - y0 + r;
            if (x < 0 || y < 0 || x >= w || y >= w) continue;
            ee_player *o = unit_get_owner(u);
            int fam = type_get_family(td), pi = o ? player_get_index(o) : 0;
            char ch = pi <= 0 ? 'T' : (fam == 2 || fam == 29) ? (char)('A' + pi) : (char)('a' + pi);
            g[y * w + x] = ch;
        }
    }
    REPLY("terrain around %d,%d (r %d; row = y from %d, column = x from %d): ~ ocean, . continent %u, digit other continent, ? none, T gaia object, B/C/D.. building of P1/P2/P3.., b/c/d.. unit\n",
          x0, y0, r, y0 - r, x0 - r, c0);
    for (int y = 0; y < w; y++) {
        REPLY("  %3d ", y0 - r + y);
        for (int x = 0; x < w; x++) REPLY("%c", g[y * w + x]);
        REPLY("\n");
    }
}

// landing P [now] (test): the state of AI P's landing; now: plan at once
internal void cmd_landing(char *out, const char *args) {
    int p = atoi(args);
    if (p <= 0 || p >= 16) { REPLY("error: landing P [now]\n"); return; }
    struct landing *l = &g_ld[p];
    if (strstr(args, "now")) l->next_try = 0;
    int aboard = 0;
    for (int k = 0; k < l->npass; k++) aboard += ld_alive(l->pass[k]) && ld_aboard(l->pass[k]);
    REPLY("P%d landing %s: phase %d, target P%d, site %d,%d (sea %d, water %d,%d), %d ships, %d warships, %d passengers (%d aboard), %d subgroups led; "
          "tries %u, landings %u, aborted %u, orders %u, subgroup orders refused %u (%u taken, not given)\n", p, g_landing ? "on" : "off", l->phase,
          l->target ? player_get_index(l->target) : -1, l->sx, l->sy, l->ocean, l->wx, l->wy, l->nships, l->nesc, l->npass, aboard, l->nfrozen,
          l->tries, l->landings, l->aborted, l->orders, g_ld_refused, g_ld_kept_goal);
    REPLY("  last site search: %u points, %u land, %u at a shore, %u at its sea, %u out of tower reach, %u without enemy soldiers; seas at the shores:",
          g_ld_stat[p].tried, g_ld_stat[p].land, g_ld_stat[p].shore, g_ld_stat[p].sea, g_ld_stat[p].safe, g_ld_stat[p].quiet);
    for (int q = 0; q < 8 && g_ld_stat[p].oc[q]; q++) REPLY(" %u x%u", g_ld_stat[p].oc[q], g_ld_stat[p].ocn[q]);
    REPLY("\n");
    if (l->phase) {
        struct ld_def d;
        ld_defence(g_players[p], l->sx, l->sy, l->wx, l->wy, &d);
        REPLY("  the beach when planned: needs %.0f (soldiers %.0f, defences %.0f, over the water %.0f); now: needs %.0f (%.0f, %.0f, %.0f)\n",
              ld_need(&l->def), l->def.soldiers, l->def.towers, l->def.water, ld_need(&d), d.soldiers, d.towers, d.water);
    }
    for (int k = 0; k < l->nesc; k++)
        if (ld_alive(l->esc[k])) {
            ee_goal *g = unit_get_goal(l->esc[k]);
            ee_unit *t = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
            REPLY("  %s type %d at %d,%d hp %d goal %s", ld_is_air(l->esc[k]) ? "aircraft" : "warship", type_get_id(unit_get_type_data(l->esc[k])), unit_get_x(l->esc[k]),
                  unit_get_y(l->esc[k]), unit_get_hp(l->esc[k]), rtti_name(g));
            if (t) REPLY(" on type %d at %d,%d", type_get_id(unit_get_type_data(t)), unit_get_x(t), unit_get_y(t));
            REPLY("\n");
        }
    if (l->obj && ld_alive(l->obj))
        REPLY("  objective type %d at %d,%d\n", type_get_id(unit_get_type_data(l->obj)), l->ox, l->oy);
    for (int k = 0; k < l->npass && k < 24; k++) {
        ee_unit *u = l->pass[k];
        ee_goal *g;
        if (!ld_alive(u)) continue;
        g = unit_get_goal(u);
        REPLY("  passenger type %d at %d,%d hp %d goal %s", type_get_id(unit_get_type_data(u)), unit_get_x(u),
              unit_get_y(u), unit_get_hp(u), rtti_name(g));
        if (g && goal_is_location(g)) REPLY(" to %.0f,%.0f", goal_get_dest_x(g), goal_get_dest_y(g));
        if (g && goal_is_attack(g) && goal_get_target(g))
            REPLY(" on type %d at %d,%d", type_get_id(unit_get_type_data(goal_get_target(g))),
                  unit_get_x(goal_get_target(g)), unit_get_y(goal_get_target(g)));
        REPLY("\n");
    }
    for (int k = 0; k < l->nships; k++)
        if (ld_alive(l->ships[k]))
            REPLY("  ship type %d at %d,%d goal %s\n", type_get_id(unit_get_type_data(l->ships[k])), unit_get_x(l->ships[k]),
                  unit_get_y(l->ships[k]), rtti_name(unit_get_goal(l->ships[k])));
}

// board P x y [r] (test): each transport of P within r tiles (10) loads every soldier and siege unit of P within r
// (the game's load goal with the whole list); again later: how many each holds - a transport's capacity
struct ld_bd { ee_player *pl; int x, y, r, nt, ns; ee_unit *t[8], *s[64]; };
internal void ld_bd_unit(ee_unit *u, void *ctx) {
    struct ld_bd *b = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    int dx = unit_get_x(u) - b->x, dy = unit_get_y(u) - b->y, t = type_get_id(td);
    if (unit_get_owner(u) != b->pl || unit_get_hp(u) <= 0 || dx * dx + dy * dy > b->r * b->r) return;
    if (is_transport(t) && b->nt < 8) b->t[b->nt++] = u;
    else if ((is_soldier(td) || is_siege(t)) && b->ns < 64) b->s[b->ns++] = u;
}
internal void cmd_board(char *out, const char *args) {
    struct ld_bd b = { 0 };
    int p = -1;
    if (!strncmp(args, "reload", 6)) {   // board reload P type: the first unit of P of type - every int and float that could hold reload times
        int rp = 0, t = 0;
        sscanf(args + 6, "%d %d", &rp, &t);
        ee_map *map = game_map(); ee_grid *grid = map ? map_get_grid(map) : NULL;
        ee_unit *hit = NULL;
        if (!grid) return;
        for (struct grid_it it = grid_it_all(1); !hit && grid_it_next(&it); )
            if (type_get_id(it.td) == t && unit_get_owner(it.u) && player_get_index(unit_get_owner(it.u)) == rp) hit = it.u;
        if (!hit) { REPLY("no unit of type %d for P%d\n", t, rp); return; }
        ee_unit_type *td = unit_get_type_data(hit); ee_unit_stats *st = unit_get_stats(hit);
    ee_player *pl = unit_get_owner(hit);
        ee_unit_stats **rec = type_stats(pl, t);
        ee_unit_stats *r = rec ? *rec : NULL;
        REPLY("unit %p type %d: type +50C %d, stats %p, player record %p%s\n", (void *)hit, t, type_get_reload(td), (void *)st, (void *)r, r == st ? " (= stats)" : "");
        for (int pass = 0; pass < 3; pass++) {
            uint8_t *o = pass == 0 ? (uint8_t *)st : pass == 1 ? (uint8_t *)unit_get_physics(hit) : (uint8_t *)hit;
            int lo = 0, hi = pass == 2 ? 0x1000 : pass == 1 ? 0x400 : 0x140;
            if (!o) continue;
            REPLY("  %s:", pass == 0 ? "stats" : pass == 1 ? "physics" : "unit");
            for (int off = lo; off < hi; off += 4) {
                int iv = FIELD(o, off, int);
                if (iv == 0) continue;
                REPLY(" %X=%x", off, (unsigned)iv);
            }
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "stats", 5)) {   // a unit of P1 per type: its stats record (unit +0x34) floats +0x80..+0x13C
        static const enum ut_name ttn[] = { UT_INF10_MACHINE_GUN, UT_INF10_DOUGHBOY, UT_INF10_TRENCH_MORTAR,
            UT_GUN_ARTILLERY10_ARTILLERY, UT_GUN_SIEGE10_HOWITZER_CANNON, UT_GUN_AT10_57MM_AT_GUN, UT_TANK10_A7V,
            UT_B_GUARD_TOWER_WW1, UT_A_STRAFE_FIGHTER10_FOKKER_DR_1, UT_A_BOMBER_10_GOTHA, UT_S10_GOOD_HOPE_FRIGATE,
            UT_S10_CRUISER, UT_A_FIGHTER10_SOPWITH_CAMEL_F_1 };
        int tt[sizeof ttn / sizeof *ttn];
        for (size_t j = 0; j < sizeof ttn / sizeof *ttn; j++) tt[j] = g_ut[ttn[j]];
        struct ld_bd f = { 0 };
        for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) {
            ee_map *map = game_map(); ee_grid *grid = map ? map_get_grid(map) : NULL;
            ee_unit *hit = NULL;
            if (!grid) break;
            for (struct grid_it it = grid_it_all(1); !hit && grid_it_next(&it); )
                if (type_get_id(it.td) == tt[j] && unit_get_owner(it.u) && player_get_index(unit_get_owner(it.u)) == 1 && unit_get_hp(it.u) > 0) hit = it.u;
            ee_unit_stats *st = hit ? unit_get_stats(hit) : NULL;
            if (!st) { REPLY("type %d: none\n", tt[j]); continue; }
            REPLY("type %d:", tt[j]);
            for (int off = 0x80; off < 0x140; off += 4) REPLY(" %X=%g", off, FIELD(st, off, float));
            REPLY("\n");
        }
        for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) {
            ee_unit_type *td = type_data(tt[j]);
            if (!td) continue;
            REPLY("type data %d:", tt[j]);
            for (int off = 0x410; off < 0x440; off += 4) REPLY(" %X=%g", off, FIELD(td, off, float));
            REPLY(" | reload 50C=%d missile delay 588=%d | 510=%d 4C0=%d 4C4=%d 4C8=%d", type_get_reload(td), type_get_missile_delay(td), type_get_attack(td), type_get_damage_class(td), type_get_category(td), type_get_weapon(td));
            for (int off = 0x514; off < 0x530; off += 4) REPLY(" %X=%d", off, FIELD(td, off, int));
            REPLY("\n");
        }
        (void)f;
        return;
    }
    if (!strncmp(args, "hits", 4)) {   // the game's hit factor (0x4D5F2C): attackers (P2) by row, targets (P1) by column
        static const enum ut_name atn[] = { UT_S10_GOOD_HOPE_FRIGATE, UT_S10_CRUISER, UT_S10_DREADNOUGHT_BATTLESHIP, UT_S10_U_BOAT,
            UT_A_STRAFE_FIGHTER10_FOKKER_DR_1, UT_A_FIGHTER10_SOPWITH_CAMEL_F_1, UT_A_BOMBER_10_GOTHA, UT_A_HELICOPTERGUNSHIP12_GUN_SHIP,
            UT_B_GUARD_TOWER_WW1, UT_B_AA10_GUN_88MM, UT_INF10_TRENCH_MORTAR, UT_AA10_TANK },
            ttn[] = { UT_S10_ATOMIC_AGE_TRANSPORT, UT_S10_U_BOAT, UT_A_FIGHTER10_SOPWITH_CAMEL_F_1, UT_S10_DREADNOUGHT_BATTLESHIP,
            UT_INF10_TRENCH_MORTAR, UT_B_GUARD_TOWER_WW1 };
        int at[sizeof atn / sizeof *atn], tt[sizeof ttn / sizeof *ttn];
        for (size_t j = 0; j < sizeof atn / sizeof *atn; j++) at[j] = g_ut[atn[j]];
        for (size_t j = 0; j < sizeof ttn / sizeof *ttn; j++) tt[j] = g_ut[ttn[j]];
        REPLY("attacker:      " );
        for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) REPLY(" %5d", tt[j]);
        REPLY("\n");
        for (size_t k = 0; k < sizeof at / sizeof *at; k++) {
            if (!type_data(at[k])) continue;
            REPLY("  %3d (kind %2d):", at[k], type_get_weapon(type_data(at[k])));
            for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) REPLY(" %5.2f", ld_factor(type_data(at[k]), 2, type_data(tt[j]), 1));
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "types", 5)) {   // the type fields (ints) small for every transport, 0 for a soldier (20): capacity?
        static const enum ut_name ttn[] = { UT_S02_STONE_TRANSPORT, UT_S03_COPPER_TRANSPORT, UT_S04_BRONZE_TRANSPORT,
            UT_S08_IMPERIAL_TRANSPORT, UT_S10_ATOMIC_AGE_TRANSPORT, UT_S13_GARGANTUA_TRANSPORT, UT_INF01_CLUBMAN };
        int tt[7];
        for (int j = 0; j < 7; j++) tt[j] = g_ut[ttn[j]];
        for (int off = 0; off < 0x800; off += 4) {
            int ok = 1, v[7];
            for (int k = 0; k < 7; k++) {
                ee_unit_type *td = type_data(tt[k]);
                v[k] = td ? FIELD(td, off, int) : -1;
                if (k < 6) ok &= v[k] >= 2 && v[k] <= 60;
            }
            if (ok && v[6] != v[0]) REPLY("+0x%X: %d %d %d %d %d %d, soldier %d\n", off, v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
        }
        return;
    }
    b.r = 10;
    if (sscanf(args, "%d %d %d %d", &p, &b.x, &b.y, &b.r) < 3 || p <= 0 || p >= 16 || !g_players[p]) { REPLY("error: board P x y [r]\n"); return; }
    b.pl = g_players[p];
    grid_each(ld_bd_unit, &b);
    for (int k = 0; k < b.nt; k++) {
        ee_unit *sh = b.t[k];
        int n = units_inside(sh);
        REPLY("transport type %d at %d,%d holds %d", type_get_id(unit_get_type_data(sh)), unit_get_x(sh), unit_get_y(sh), n);
        if (!n && b.ns) {
            ee_goal *g = goal_load(sh, b.s, b.ns);
            if (g) {   // refused: deleted, no parts from it
                for (int j = 0; j < b.ns; j++)
                    if (!ld_aboard(b.s[j])) unit_goal(b.s[j], goal_load_part(g), 0, 0x1F40);
                REPLY(" - loads %d", b.ns);
            }
            b.ns = 0;   // the next transport: the rest later
        }
        REPLY("\n");
    }
    if (!b.nt) REPLY("no transport of P%d within %d of %d,%d\n", p, b.r, b.x, b.y);
}

// who else orders a landing transport to unload (the soldiers aboard while it waited at home were unloaded again):
// the game's calls of the unload goal's constructor go through here; for a transport of a landing, the caller is logged.
// The transport manager's (0x48CC5D in 0x48CC10, from 0x48A382: it takes a transport it thinks free for one of its own
// crossings - an unload point, then its own units loaded) was most of them, 393 of 794 in 30 island runs, in every
// phase: refused (NULL - the caller skips to its end on a failed allocation; the 0x40 bytes are lost). The subgroup's
// (0x445AAD) is refused at its vf57 (hook_asg_give_goal); 0x48D022 calls the goal unchecked, so it stays (its give
// at 0x48D03C is checked instead: hook_unload_retry_give). Followed to the end (ld_fu, "# ldforeign:"), 48 island runs
// (Small e5 / Large e9, 12 seeds each, twice): 0x445AAD 216 orders, every one refused at vf57 or never the transport's
// goal (empty transports: the caller deletes it) - log noise; 0x48D022 5, three the landing's own unload at its own
// point (phase 3/4), two while boarding (phase 2) - an unload of the game's crossing the transport had before the
// landing took it (resumed after the load goal, not made at any of these sites) carried 8 and 12 soldiers 40-70
// tiles off and put them ashore there. 0x615AE6 is the unload goal's copy (vf4) - never seen.
internal unsigned g_foreign_unload[8], g_ld_mgr_refused;
internal int g_land_keep = 1;   // landing_keep_transports: the transport manager's unload refused
// the game's unload orders let through (diagnostic): each one followed until it is refused, never becomes the
// transport's goal, or was its goal and ended - then a "# ldforeign:" line: the soldiers aboard before and after and
// where the landing itself wanted that transport (state 0 made, 1 given, 2 the transport's goal)
internal struct ld_fu { ee_unit *sh; ee_goal *g; int site, p, x, y, aboard, state, phase; uint32_t t; } g_fu[16];
internal uint32_t ld_clock(void) { ee_physics *c = game_physics(); return c ? phys_get_time(c) : 0; }
internal void ld_fu_log(struct ld_fu *f, const char *what, int now_aboard) {
    struct landing *l = &g_ld[f->p & 15];
    int k = 0, lx = -1, ly = -1;
    while (k < l->nships && l->ships[k] != f->sh) k++;
    if (k < l->nships) { lx = l->phase == 3 && !l->staged ? l->stx : l->ux[k]; ly = l->phase == 3 && !l->staged ? l->sty : l->uy[k]; }
    if (g_log) fprintf(g_log, "# ldforeign: P%d 0x%X at %.1f min (phase %d, wave state %d): unload at %d,%d, the landing's point %d,%d, ship at %d,%d - %s, aboard %d -> %d\n",
                       f->p, (unsigned)unload_site_at(f->site), f->t / 60000.0, f->phase, l->wst, f->x, f->y, lx, ly,
                       ld_alive(f->sh) ? unit_get_x(f->sh) : -1, ld_alive(f->sh) ? unit_get_y(f->sh) : -1, what, f->aboard, now_aboard);
    f->sh = NULL;
}
internal void ld_fu_given(ee_goal *g, int ok) {   // a subgroup's vf57 or the game's own give: taken or refused
    for (int j = 0; j < 16; j++)
        if (g_fu[j].sh && g_fu[j].g == g && g_fu[j].state == 0) {
            if (ok) g_fu[j].state = 1;
            else ld_fu_log(&g_fu[j], "refused", g_fu[j].aboard);
        }
}
internal void ld_fu_forget(ee_goal *g) {   // the landing's own unload goal at that address: the old one is gone
    for (int j = 0; j < 16; j++)
        if (g_fu[j].sh && g_fu[j].g == g) ld_fu_log(&g_fu[j], g_fu[j].state == 2 ? "replaced by the landing's" : "never its goal", g_fu[j].aboard);
}
internal void ld_fu_check(uint32_t now) {
    for (int j = 0; j < 16; j++) {
        struct ld_fu *f = &g_fu[j];
        if (!f->sh) continue;
        ee_unit_type *td = ld_alive(f->sh) ? unit_get_type_data(f->sh) : NULL;
        if (!td || !is_transport(type_get_id(td))) { ld_fu_log(f, "transport lost", 0); continue; }
        ee_goal *cur = unit_get_goal(f->sh);
        int n = units_inside(f->sh), is = cur == f->g && goal_is_unload(cur);
        if (f->state < 2 && is) { f->state = 2; f->aboard = n; continue; }
        if (f->state == 2 && !is) ld_fu_log(f, "was its goal", n);
        else if (f->state < 2 && now - f->t > 30000) ld_fu_log(f, "never its goal", n);
    }
}
internal void *__thiscall hook_unload_ctor(void *g, ee_unit *tr, int x, int y, int flag) {
    uintptr_t ret = (uintptr_t)__builtin_return_address(0) - 5;
    if (unload_from_rm(ret) && tr && expand_ship(tr)) return NULL;   // an outpost's ferry (expand.c): not the transport manager's crossing
    for (int p = 0; p < 16 && tr && !is_vanilla(unit_get_owner(tr)); p++)   // a vanilla player's transport: never a landing's
        for (int k = 0; k < g_ld[p].nships && g_ld[p].phase; k++)
            if (g_ld[p].ships[k] == tr) {
                int site = unload_site(ret);   // 7 (0x615AE6): the unload goal's copy (vf4), and an unknown one
                if (site < 0) site = 7;
                g_foreign_unload[site]++;
                if (unload_from_rm(ret) && g_land_keep) {
                    g_ld_mgr_refused++;
                    if (g_log) fprintf(g_log, "# landing: P%d's transport %d (phase %d) - the game's transport manager wanted it for a crossing to %d,%d: refused\n", p, k, g_ld[p].phase, x, y);
                    return NULL;
                }
                if (g_log) fprintf(g_log, "# landing: P%d's transport %d (phase %d) told to unload at %d,%d by the game (call at 0x%X)\n", p, k, g_ld[p].phase, x, y, (unsigned)ret);
                void *r = goal_unload_init(g, tr, x, y, flag);
                int j = 0;
                while (j < 15 && g_fu[j].sh) j++;
                if (r) {
                    if (g_fu[j].sh) ld_fu_log(&g_fu[j], "dropped from the list", g_fu[j].aboard);
                    g_fu[j] = (struct ld_fu){ tr, r, site, p, x, y, units_inside(tr), 0, g_ld[p].phase, ld_clock() };
                }
                return r;
            }
    return goal_unload_init(g, tr, x, y, flag);
}
// the game's retry of an unload (0x48CE8F, EEComputerPlayer vf50: a transport with cargo whose goal is an unload makes
// a new one at the same point and gives it, 0x4412D4 at 0x48D03C). 48 island runs: at sea and ashore (phase 3, 4)
// it was the landing's own unload at the transport's own point (kept - the landing re-unloads the same way); while
// boarding (phase 2) the game's own crossing kept going with the landing's soldiers aboard. A landing transport gets
// it only while the landing unloads (phase 3, phase 4 with the first landing or a wave at sea) and at its own point;
// otherwise the new goal is deleted, never given (the caller does not touch it after this call)
internal unsigned g_ld_retry_refused;
internal void __thiscall hook_unload_retry_give(ee_unit *u, ee_goal *goal, int slot) {
    if (goal && expand_ship(u) && !expand_unload_ok(u, goal_get_unload_x(goal), goal_get_unload_y(goal))) {   // an outpost's ferry (expand.c)
        ld_fu_given(goal, 0);
        ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
        return;
    }
    for (int p = 0; p < 16 && goal && !is_vanilla(unit_get_owner(u)); p++) {
        struct landing *l = &g_ld[p];
        for (int k = 0; k < l->nships && l->phase; k++)
            if (l->ships[k] == u) {
                int unloads = l->phase == 3 || (l->phase == 4 && (l->wst == 0 || l->wst == 3));
                int gx = goal_get_unload_x(goal), gy = goal_get_unload_y(goal);
                if (unloads && abs(gx - l->ux[k]) <= 3 && abs(gy - l->uy[k]) <= 3) goto give;
                g_ld_retry_refused++;
                if (g_log) fprintf(g_log, "# landing: P%d's transport %d (phase %d, wave state %d) - the game's unload retry at %d,%d refused (its point %d,%d)\n",
                                   p, k, l->phase, l->wst, gx, gy, l->ux[k], l->uy[k]);
                ld_fu_given(goal, 0);
                ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
                return;
            }
    }
give:
    ld_fu_given(goal, 1);
    unit_give_goal(u, goal, slot);
}

// the game's idle-transport code (0x483E10: cargo aboard, no order, at a shore - 0x42C49A at 0x483E24 says where to
// land) unloads a transport of a landing waiting for its last passengers or for the warships: for those "nowhere"
internal int __thiscall hook_idle_unload_shore(void *mgr, ee_unit *tr) {
    if (tr && expand_ship(tr)) return 0;   // an outpost's ferry (expand.c) unloads where it decides
    for (int p = 0; p < 16 && tr && !is_vanilla(unit_get_owner(tr)); p++)   // a vanilla player's transport: never a landing's
        for (int k = 0; k < g_ld[p].nships && g_ld[p].phase; k++)
            if (g_ld[p].ships[k] == tr) return 0;
    return shore_to_unload(mgr, tr);
}

// the subgroups' orders (vf57: transport, sea, land, air) through hook_asg_give_goal - once, for the landing and the
// fleet (fleet.c)
internal int asg_order_hooks(void) {
    static int done;
    if (!done)
        done = patch_asg_give_goal(hook_asg_give_goal, hook_asg_air_give_goal);
    return done;
}
internal void landing_install(const char *aimod_ini, const char *autotest_ini) {
    BOOL gs = patch_goalset_ask(hook_goalset_ask, NULL) == 2 && patch_goalset_read(goalset_read_thunk);
    if (g_log) fprintf(g_log, "# engine guard: goal set after a dropped goal %s\n", gs ? "ok" : "FAILED");
    g_landing = cfg_int(aimod_ini, autotest_ini, 1, "landing", 1);
    g_land_bombard = cfg_int(aimod_ini, autotest_ini, 1, "landing_bombard", 0);
    g_land_keep = cfg_int(aimod_ini, autotest_ini, 1, "landing_keep_transports", 1);
    g_land_ships = cfg_int(aimod_ini, autotest_ini, 1, "landing_transports", 8);
    g_land_flag = cfg_int(aimod_ini, autotest_ini, 1, "landing_flag", 0);
    g_land_knows = cfg_int(aimod_ini, autotest_ini, 1, "landing_knows_map", 0);
    g_land_trace = cfg_int(aimod_ini, autotest_ini, 1, "landing_trace", 0);
    g_land_cits = cfg_int(aimod_ini, autotest_ini, 1, "landing_citizens", 10);
    g_land_waves = cfg_int(aimod_ini, autotest_ini, 1, "landing_waves", 3);
    if (g_land_waves < 0) g_land_waves = 0;
    if (g_land_waves > 10) g_land_waves = 10;
    if (g_land_cits < 0) g_land_cits = 0;
    if (g_land_cits > 10) g_land_cits = 10;
    if (g_land_ships < 1) g_land_ships = 1;
    if (g_land_ships > LD_SHIPS) g_land_ships = LD_SHIPS;
    if (g_landing) {
        patch_shore_to_unload(hook_idle_unload_shore);
        patch_unload_ctor(hook_unload_ctor, NULL);
        patch_unload_retry_give(hook_unload_retry_give);
    }
    int h = patch_asg_update(hook_asg_land_update) & patch_asg_sea_transport_update(hook_asg_sea_transport_update) &
            patch_asg_sea_update(hook_asg_sea_update) & patch_asg_air_update(hook_asg_air_update);
    int gg = g_landing ? asg_order_hooks() : 0;   // the subgroups' own orders refused while the landing leads them
    if (g_log) fprintf(g_log, "# landing: %s, %d transports (subgroup update hooks %s, order hooks %d/4)\n", g_landing ? "on" : "off", g_land_ships, h ? "ok" : "FAILED", gg);
}

// a new game (the game start hook): every landing's ships, passengers and subgroups are the last game's (the tick's
// own "time went backwards" reset missed a save loaded at a later time and slots without an AI now; landing_leads
// and the unload hooks kept matching the freed pointers)
internal void landing_new_game(void) {
    memset(g_ld, 0, sizeof g_ld);
    memset(g_ld_stat, 0, sizeof g_ld_stat);
    memset(g_fu, 0, sizeof g_fu);
    memset(&g_ld_home, 0, sizeof g_ld_home);
}
