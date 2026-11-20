// THE SQUADRONS ([ai_base] sea_squadrons; fleet.c hands its warships over after its scan). The player 2026-11-17:
// "water needs a rework i think, the ai has a single fleet, it dosnt try to hold a sea i think, just move its single
// blob" - and, of how he plays it: "small scout and raid groups, and one main fleet where its needed, its shouldnt be a
// hardcode single fleet tough, the fleet is slow, on large maps its be to late to matter if not near alredy. it needs
// multiple fleets that can hold on their own for a bit until the other fleets come for help if needed"; frigates (with
// their speed upgrades) hunt fishing boats and transports, warships are the slow damage dealers and tanks, galleys and
// later submarines hunt warships; "galleys are a middle ground to me, mostly warship hunter wich is the role of subs
// later". navy.c trains by these roles while squadrons lead (battleships the core), research.c buys the frigates'
// speed with their range.
// The old fleet (fleet.c) is one force with one guard point; its task forces split only when it is far the stronger,
// and its massing gathers everything at one rally first. Here, by sea (the game's connected water, area_id 1):
//   ZONES (every SQ_ZONES) - the sea territories that matter to it: its coast along land it holds (THE SEAS' coast),
//     the fish it has seen there, enemy warships seen there lately (doubled). The most valuable is a zone's anchor,
//     every such water within SQ_REACH of it is the zone's; the next uncovered one anchors the next zone. As many main
//     squadrons as zones and ships allow (SQ_MAIN_MIN each), the most valuable zones first: one zone on a small sea, a
//     squadron per stretch of coast on a large one - each near enough to matter where it is.
//   SHIPS - frigates (navy.c NR_FRIG) first to the scout groups (SQ_SCOUT_N each, one more group with two zones), the
//     rest to the main squadrons by their zones' worth (SQ_MAIN_MIN at least); a ship keeps its squadron, one is moved
//     a look while one squadron is 2+ over its share and another 2+ under.
//   EACH SHIP (every fleet look, 2 s - the squadrons' own looks are every SQ_LOOK) as the old fleet's: one in a danger
//     zone out of it at once (fleet.c fl_flee), one whose target is in one back to its squadron's point - the group's
//     attack-move let them chase into the coastal guns (a third of the warships the squadrons lost in fights went down
//     inside a danger zone) -, a hurt one (under SQ_REPAIR of its hit points, the old fleet's half) to the nearest
//     dock until it has 80% again (fleet.c counts the repair); under fire it waits behind the nearest main squadron
//     first (BEHIND THE SQUADRON), its mates between it and the chasers. At 40% and every 4 s most were sent at 5-25%
//     and sunk within 15 s and 10 tiles of where they turned - in the fight still, not on the way.
//   MAIN - at rest at its zone's anchor, out of every danger zone (fleet.c: coastal defences, siege); enemy warships in
//     its zone: it fights them when clearly the stronger (SQ_ENGAGE on the fight estimate, fleet.c fl_ratio) and on
//     while not losing (SQ_HOLD) - from an even estimate half the ships it sent to repair were sunk leaving the fight,
//     nearly all its losses -, else it holds under
//     its own towers in the zone (or by its dock) and calls for help; a quiet squadron on the same sea with half its hit
//     points or more answers - sails there and fights with it - until the caller's zone is quiet again. Quiet and
//     clearly stronger (SQ_PUSH) than the enemy warships known within 2 x SQ_REACH: it goes after them, back at 1.1.
//   HUNT - the submarines and galleys (navy.c NR_SUB, NR_ANTI: both hit battleships hard and lose to frigates), in
//     groups of SQ_HUNT_N at most (the player, the same day: "subs in the later ages are also very (lone, small group)
//     good hunters, as long as they avoid fregattes"): after the nearest enemy warship known within SQ_HUNT_R that no
//     enemy frigate is near (SQ_FRIG_R) and whose company they beat; any enemy frigate within SQ_FRIG_R of them: away
//     to the nearest main squadron. Nothing to hunt: with the nearest one.
//   HOME - enemy warships within SQ_HOME_R of one of its docks (where the hurt ones are repaired) or fishing boats on
//     the sea are the nearest main squadron's threat, wherever its zone lies (the first head-to-heads: 15 of the 23
//     warships a squadron AI lost went down on the way to the dock or at it, nobody came).
//   SCOUT - looks into the waters of its sea not seen longest (enemy and unheld ones first; A LOOK AGAIN as the land
//     scouts), raids enemy fishing boats and transports it sees within SQ_RAID_R where no enemy warship within 12 tiles of
//     them stands a fight with it (SQ_SCOUT_FIGHT), and runs from warships within SQ_SCOUT_FLEE it doesn't beat as
//     clearly to the nearest main squadron: raiders, not fighters (the player: frigates hunt fishing boats and
//     transports) - at 1.2 and 14 tiles the scouts' frigates were most of the hurt ships lost from epoch 5 (255 of 467,
//     92% with an enemy warship within 10 tiles: the Middle Ages' frigate has 540 hit points and 25 attack).
//   ESCORT (convoy.c, transport_escort) - a transport asks from the start of its trip (on its way to the boarding
//     shore, boarding, crossing): the nearest scout group on its sea that isn't running, else the nearest main squadron
//     at rest. Before the crossing the scouts sweep its way (its middle: what lies there is seen and met before the
//     transport gets there), a main squadron waits with it; nobody waits for the escort. At sea the scouts sail
//     SQ_AHEAD ahead of it, and THE SCREEN: enemy warships within SQ_SCREEN_R of the transport or of its escort - the
//     escort goes at the one nearest to the transport, whatever the odds, and doesn't run: it draws the fire the
//     transport would take and may sink something, the transport is ashore in about a minute. A loaded transport is a
//     HOME point too. The player 2026-11-17: "it should escort its transport and scout ahed with a ship"; 2026-11-18: "a
//     transports best defense is reaching its goal and dropping its load before it can be sunk, an escort scouts ahead
//     and is an distraction from the valuable transports with the bonus of maybe sinking something. it has to happen
//     cordinated and fast" - the first escort (asked for once loaded, a crossing waiting up to a minute for it, the
//     scouts running from what they didn't beat) slowed no crossing (at sea 69 s against 74 s, boarding 73 s against
//     68 s) and saved nobody (lost aboard 11.7 against 12.3 per AI): it came late and ran when it was needed.
//   CONCENTRATION - Lanchester's square law (fleet.c's fight estimate): split forces lose piece by piece to one fleet
//     (the first head-to-head against the old fleet, Small Islands large from epoch 9, 12 games mirrored: warships lost
//     30.5 against 15.3, sunk 19.9 against 25.8, afloat at the end 9.5 against 20.8). So on a sea where all its
//     warships do not beat all the enemy's it knows there (seen, remembered 5 minutes; SQ_SPLIT, back together under
//     1.0) there is one main squadron; a squadron fights in its zone only when the stronger there (SQ_ENGAGE), and
//     pushes only when it beats what the enemy is known to have on the whole sea too.
//   Every move is the game's group move on the squadron (the formations; the slow set the pace) - a move or a hold,
//   an attack-move otherwise - and goes to the nearest safe water toward its goal (fleet.c fl_safe_toward).
// "# squadrons:" lines each minute (the warships it lost, by squadron and by what it did then); the AI overview shows
// each squadron as a fleet.
// MEASURED 2026-11-17, head-to-head against the old fleet (mirrored, 12 games, 60 min, Small Islands large from epoch 9,
// 4 AIs, 2 with squadrons; B - A per game): kills +12.6 (se 4.4), losses +0.8 (6.4), soldiers -3.3 (6.9), workers -0.9
// (3.0); at sea per AI warships lost 22.4 against 14.0, sunk 16.6 against 19.7, afloat at the end 16.5 against 22.5,
// fishing boats lost 1.4 against 2.7 and sunk 2.9 against 1.2. Of its warships lost: 13.6 hurt ones on the way to the
// dock, 5.8 between squadrons, 2.8 in them. So built off (sea_squadrons).
// Then EACH SHIP's care, BEHIND THE SQUADRON, the roles (navy.c's mix, research.c's frigate speed, galleys hunting) and
// a ship no longer in two squadrons at once (sq_assign: up to 1570 moves a game, two sets of orders each look) - the
// same test, 24 games: kills +7.0 (se 2.1), losses -5.0 (4.9), soldiers -1.5 (5.6), workers -1.8 (2.6); at sea per AI
// warships afloat at the end 20.2 against 19.8, lost 18.0 against 17.5, sunk 19.1 against 16.4, fishing boats lost 1.7
// against 2.8, transports sunk 3.8 against 1.7, boats fishing 13.3 against 10.0. But from epoch 5 and 11 (12 games each)
// even, at sea a little behind (sunk - lost -0.8 and -1.9 per AI, the old fleet +0.8 and +1.9).
// Then a fight begun only from SQ_ENGAGE 1.3 (was SQ_HOLD 1.0): epoch 9 (24 games) kills +7.1 (se 2.9), losses -0.8
// (4.4), at sea sunk - lost +0.6 against -0.5; epoch 11 (12) kills +8.3 (4.9), losses -3.3 (4.3), warships afloat at
// the end 24.5 against 23.1, sunk - lost +1.5 against -1.6; epoch 5 (24) kills +2.6 (3.1), losses +2.0 (4.1), sunk -
// lost -0.6 against +0.6 (fewer hulls, 11.7 against 14.4: its battleships, 4 frigates' hit points at 2 frigates'
// price). The old mix (frigates 2, battleships 1) with it: epoch 11 sunk - lost -4.2 against +4.2. On since then.
// Then the scouts raiders only (SQ_SCOUT_FIGHT 2.0, SQ_SCOUT_FLEE 16; were 1.2 and 14), against the old fleet: epoch 5
// (24 games) sunk - lost +2.4 per AI against -2.4 (before -0.6 against +0.6), afloat 13.9 against 13.7 (11.7 against
// 14.4), kills +3.3 (se 2.7); epoch 9 (24) +1.7 against -1.7 (+0.6 against -0.5), kills +7.5 (4.2); epoch 11 (12) +2.2
// against -2.2 (+1.5 against -1.6), kills +9.2 (5.4).
#define SQ_MAX 8
#define SQ_MAIN_MIN 5
#define SQ_REACH 40
#define SQ_ZONES 60000
#define SQ_LOOK 4000
#define SQ_SCOUT_N 3
#define SQ_HUNT_N 3
#define SQ_HUNT_R 50
#define SQ_FRIG_R 12
#define SQ_RESCOUT 300000
#define SQ_RAID_R 40
#define SQ_SCOUT_FIGHT 2.0f    // SCOUT: a fight only this clearly the stronger
#define SQ_SCOUT_FLEE 16       //   warships so near looked at
#define SQ_HOLD 1.0f
#define SQ_ENGAGE 1.3f
#define SQ_PUSH 1.5f
#define SQ_SPLIT 1.3f          // CONCENTRATION: the whole sea's ratio from which it splits into zones (together under 1.0)
#define SQ_REPAIR 0.5f
#define SQ_BEHIND 10           // BEHIND THE SQUADRON: so far past the main squadron's middle a hurt ship under fire waits
#define SQ_BEHIND_E 12         //   under fire: an enemy warship seen so near
#define SQ_BEHIND_M 30         //   a main squadron so near
#define SQ_HOME_R 15
#define SQ_AHEAD 10            // ESCORT: the scouts so far ahead of the transport on its way
#define SQ_ESC_NEAR 14         //   with it: its escort's middle so near
#define SQ_SCREEN_R 16         //   THE SCREEN: enemy warships so near the loaded transport at sea, or its escort, are attacked
#define SQ_SEAS 64
#define SQ_T 4096
enum { SQK_MAIN, SQK_SCOUT, SQK_HUNT };
internal const char *const k_sqk[3] = { "main", "scouts", "hunters" };
enum { SQS_REST, SQS_ENGAGE, SQS_HOLD, SQS_HELP, SQS_PUSH, SQS_SCOUT, SQS_RAID, SQS_FLEE, SQS_HUNT, SQS_ESCORT, SQS_SCREEN, SQS_SUPPORT, SQS_SHELL, SQS_N };
internal const char *const k_sqs[SQS_N] = { "at rest", "fighting in its zone", "holding, help called", "helping", "pushing", "scouting", "raiding", "running", "hunting", "escorting", "screening", "ahead of a push's landing", "shelling the shore" };
// THE SHORE ([ai_base] sea_shore_fire, on since 2026-11-20 by the player's call ("switch it on"); [autotest] v_sea_shore_fire; the player 2026-11-20: "ships
// attacking targets on land are very skitish, unable to decide wether to do it or not"). Traced (Small Islands large
// from World War I, 2 games, each warship every 2.7 s): the game's own target pick - at rest, and on the squadrons' moves,
// which were attack-moves sent again as their middle shifted - took citizens, priests and buildings 10-13 tiles off;
// the next look found the target in a danger zone (EACH SHIP) and sent the ship back, where the game picked the next:
// attack and back every 5-10 s for minutes, Dreadnoughts too, and nothing shelled. Stand-your-ground alone kept it
// (the attack-moves pick on their way). So the shore is decided, not stumbled into:
//   - the quiet moves (at rest, scouting, to the main squadron) are plain moves; their own stance stays (standing their
//     ground too, against the sea opponent - tests/scenarios/seaop.eeai, from World War I, 6 games -, they sank 12.7 of
//     its warships against 19.3 and its raiders killed 13.2 citizens against 6.7: they let the raiders shell the coast);
//   - a land target a ship has taken anyway is kept only within its reach from water out of every danger zone;
//   - SHELLING: a main squadron at rest with SQ_SHELL_HP of its hit points looks at the enemy buildings it sees on the
//     coast of its sea within SQ_SHELL_R of its zone, the nearest first (SQ_SHELL_LOOKS): against the defences that reach
//     the water off one - towers and the land units that threaten ships (fleet.c fl_danger_r), seen within their reach +
//     SQ_SHELL_DEF - its estimate (Lanchester's square law as fl_ratio, the game's damage table both ways) from
//     SQ_SHELL_GO: it shells it, the defences reaching it first, the nearest the squadron first, then the building - its
//     ships out of EACH SHIP's danger rules while it lasts (the hurt still go to repair) - until the estimate is under
//     SQ_SHELL_HOLD or its hit points under SQ_SHELL_OFF, then the next. Enemy warships in its zone come first.
internal int g_sq_shore = 0, g_v_sq_shore = -1;
#define SQ_SHORE(idx) PV(idx, g_v_sq_shore, g_sq_shore)
#define SQ_SHELL_HP 0.7f
#define SQ_SHELL_OFF 0.4f
#define SQ_SHELL_R 30
#define SQ_SHELL_DEF 4
#define SQ_SHELL_GO 1.5f
#define SQ_SHELL_HOLD 1.1f
#define SQ_SHELL_LOOKS 6
struct sq {
    int on, kind, ocean, anchor;          // anchor: the zone's water territory (main)
    ivec2 zc, go, ordered;                // the anchor's middle, where it goes, where it was sent last
    int want, n, state, help, target_r, call, escort, support;   // call: it asks for help; help: whom it helps (+1); escort: convoy.c's wish (+1); support: the push it goes ahead of (+1, THE SQUADRON AHEAD)
    ee_unit *u[FL_MAX];
    float ratio, zvalue;
    uint32_t order_t, state_t, target_t;
    unsigned engages, holds, helps, pushes, raids, scouted, flees, escorted, screens, shelled;
    ee_unit *shell; int shell_id;         // THE SHORE: the building it shells
};
internal struct sq_st {
    struct sq s[SQ_MAX];
    struct { int oc, split; float ratio; } sea[4]; int nsea;   // CONCENTRATION: per sea, the whole fleets' ratio and whether it is split
    struct { ee_unit *u; int id; signed char sq; } m[FL_MAX]; int nm;
    uint32_t t, zone_t, log_t;
    unsigned repairs, moved, flees, drops, behind, shore_drops;   // flees: ships out of a danger zone; drops: a target in one given up; shore_drops: THE SHORE's, a land target out of reach
    unsigned lost_kind[3], lost_state[SQS_N], lost_repair, lost_landing, lost_none;   // its warships sunk: by squadron kind, by what it did then, sent to repair, a landing's escort, in none
    int free_nosea, free_nofit, docked;   // the last look: ships in no squadron - on no sea, none that takes them; at the dock
} g_sq[16];
internal uint32_t g_sq_look[16][SQ_T];   // a water territory last seen by one of its warships (A LOOK AGAIN at sea)
internal struct fleet g_sqf;   // a squadron as fleet.c's fight estimate reads one (its ships only)

internal unsigned sq_ocean_of(int r) { return area_id(g_ter.t[r].c, 1); }
internal float sq_sea_value(const struct tv_st *s, int r, uint32_t now) {   // what a sea territory is worth holding
    const struct tv_terr *t = &s->t[r];
    int recent = t->border || (t->eship_t && now >= t->eship_t && now - t->eship_t < 300000);   // a border sea (territories.c THE SEAS' CLAIM) as one where enemy warships were seen
    float v = (float)t->coast + (float)t->fish / 200.0f;
    if (v <= 0 && !recent) return 0;
    return v * (recent ? 2.0f : 1.0f) + (recent ? 20.0f : 0.0f);
}
internal ivec2 sq_center(const struct sq *q) {
    long x = 0, y = 0;
    for (int i = 0; i < q->n; i++) { x += unit_get_x(q->u[i]); y += unit_get_y(q->u[i]); }
    return q->n ? iv2((int)(x / q->n), (int)(y / q->n)) : q->zc;
}
internal float sq_hp_share(const struct sq *q) {
    float h = 0, m = 0;
    for (int i = 0; i < q->n; i++) { h += (float)unit_get_hp(q->u[i]); m += fl_hp_max(q->u[i]); }
    return m > 0 ? h / m : 0;
}
internal float sq_ratio(const struct sq *q, const unsigned char *sel) {
    g_sqf.n = q->n;
    memcpy(g_sqf.u, q->u, (size_t)q->n * sizeof *q->u);
    return fl_ratio(&g_sqf, iv2(0, 0), 0, sel);
}
// the enemy warships known on sea oc within r of p (sel marked): how many
internal int sq_enemies(int oc, ivec2 p, int r, unsigned char *sel) {
    int n = 0;
    for (int j = 0; j < g_fs.ne; j++) {
        sel[j] = (int)area_id(g_fs.es[j].p, 1) == oc && ivec2_dist2(g_fs.es[j].p, p) <= r * r;
        n += sel[j];
    }
    return n;
}
// THE SHORE: a target on land (not afloat, not in the air; a dock is)
internal int sq_on_land(ee_unit *t) {
    ee_unit_type *td = t ? unit_get_type_data(t) : NULL;
    int c = td ? unit_type_get_class_id(td) : -1;
    return td && c != CLS_NAVAL && c != CLS_SUBMERGED_NAVAL && c != CLS_NAVAL_TRANSPORT && c != CLS_NAVAL_CITIZEN && c != CLS_AIRPLANE &&
           c != CLS_HELICOPTER && c != CLS_HELICOPTER_TRANSPORT && c != CLS_HELICOPTER_PRIEST && c != CLS_SEA_ANIMAL && c != CLS_AIR_ANIMAL;
}
// THE SHORE: the defences that reach the water off p (towers, land units that threaten ships, seen within their reach +
// SQ_SHELL_DEF of p) into out (max n), how many; the one nearest c in *near
#define SQ_SHELL_N 48
internal int sq_shore_defs(ee_player *pl, ivec2 p, ivec2 c, ee_unit **out, int n, ee_unit **closest) {
    int k = 0, bd = 0x7FFFFFFF;
    *closest = NULL;
    for (struct grid_it it = grid_it_box(p.x, p.y, 24, 1); k < n && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_enemy(pl, unit_get_owner(u)) || !seen_by(u, pl)) continue;
        float r = fl_danger_r(u, it.td) + SQ_SHELL_DEF;
        ivec2 q = unit_get_tile(u);
        if (r <= SQ_SHELL_DEF || (float)ivec2_dist2(q, p) > r * r) continue;
        out[k++] = u;
        int d = ivec2_dist2(q, c);
        if (d < bd) { bd = d; *closest = u; }
    }
    return k;
}
// THE SHORE: q's ships against the defences def[0..nd) - fl_ratio's square law, the game's damage table both ways; 99 none
internal float sq_shore_ratio(const struct sq *q, ee_unit *const *def, int nd) {
    float ho = 0, he = 0, dofs = 0, de = 0;
    for (int j = 0; j < nd; j++) he += (float)unit_get_hp(def[j]);
    for (int i = 0; i < q->n; i++) ho += (float)unit_get_hp(q->u[i]);
    if (he <= 0) return 99.0f;
    if (ho <= 0) return 0.0f;
    for (int i = 0; i < q->n; i++) {
        ee_unit_type *td = unit_get_type_data(q->u[i]);
        float fh = 0, hpu = (float)unit_get_hp(q->u[i]);
        for (int j = 0; j < nd; j++) {
            ee_unit_type *dt = unit_get_type_data(def[j]);
            fh += (float)unit_get_hp(def[j]) * nv_f(unit_type_get_weapon(td), unit_type_get_family(dt));
            de += fl_dps(def[j], dt) * hpu * nv_f(unit_type_get_weapon(dt), unit_type_get_family(td));
        }
        dofs += fl_dps(q->u[i], td) * fh / he;
    }
    de /= ho;
    if (de <= 0) return dofs > 0 ? 99.0f : 1.0f;
    if (dofs <= 0) return 0.0f;
    return sqrtf(ho * dofs / (he * de));
}
internal ivec2 sq_safe(ivec2 from, ivec2 to, int oc) {   // the nearest safe water toward `to`
    ivec2 o;
    if (fl_safe_toward(from, to, (unsigned)oc, &o)) return o;
    return fl_water(to, (unsigned)oc, &o) ? o : to;
}
// the haven of a zone: water by its own tower within the zone (the farthest from the threat), else by a dock
internal ivec2 sq_haven(const struct sq *q, ivec2 threat) {
    ivec2 best = q->zc;
    int bd = -1;
    for (int pass = 0; pass < 2 && bd < 0; pass++) {
        int n = pass ? g_fs.nd : g_fs.ntw;
        for (int k = 0; k < n; k++) {
            ivec2 b = pass ? g_fs.dk[k] : g_fs.tw[k], w, o;
            if (!pass && ivec2_dist2(b, q->zc) > SQ_REACH * SQ_REACH) continue;
            if ((int)ocean_near(b, pass ? 4 : 6, &w) != q->ocean || !fl_water(w, (unsigned)q->ocean, &o) || fl_in_danger(o)) continue;
            int d = ivec2_dist2(o, threat);
            if (d > bd) { bd = d; best = o; }
        }
    }
    return best;
}
// the squadron's group move to p: the game's move on a selection of its ships (fight: an attack-move)
internal void sq_move(ee_player *pl, struct sq *q, ivec2 p, int fight, uint32_t now) {
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    static uint32_t ids[FL_MAX];
    if (!q->n) return;
    ivec2 c = sq_center(q);
    int prio = q->state == SQS_SUPPORT || q->state == SQS_ESCORT || q->state == SQS_SCREEN ? CLP_TASK : CLP_ROUTINE, m = 0;
    for (int i = 0; i < q->n; i++) if (cl_may(q->u[i], prio, CLW_SQUADRON)) ids[m++] = (uint32_t)unit_get_serial(q->u[i]);   // ORDERS BY PRIORITY: not those another holds
    if (!m) return;
    uint32_t *sel[3] = { NULL, ids, ids + m };
    msg_as_select(pl, sel);
    memset(mem, 0, sizeof mem);
    float x = (float)p.x + 0.5f, y = (float)p.y + 0.5f;
    path_msg_make((ee_path_msg *)mem, msg_as_begin(pl), x, y, 1, 0);
    path_msg_set_forced((ee_path_msg *)mem, !fight);
    path_msg_set_facing((ee_path_msg *)mem, atan2f(-(x - (float)c.x), y - (float)c.y));
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
    q->ordered = p; q->order_t = now ? now : 1;
    g_fl[g_fs.idx].orders++;
}
// sent again when its goal moved, or now and then while its ships stand idle away from it
internal void sq_go(ee_player *pl, struct sq *q, ivec2 p, int fight, uint32_t now) {
    q->go = p;
    int far_ = 0;
    for (int i = 0; i < q->n; i++) far_ += ivec2_dist2(unit_get_tile(q->u[i]), p) > 64 && !unit_get_goal(q->u[i]);
    if (ivec2_dist2(q->ordered, p) > 25 || (far_ && now - q->order_t >= 20000)) sq_move(pl, q, p, fight, now);
}
internal void sq_state(struct sq *q, int st, uint32_t now) {
    if (q->state == st) return;
    q->state = st; q->state_t = now ? now : 1;
    if (st == SQS_ENGAGE) q->engages++;
    else if (st == SQS_HOLD) q->holds++;
    else if (st == SQS_HELP) q->helps++;
    else if (st == SQS_PUSH) q->pushes++;
    else if (st == SQS_RAID) q->raids++;
    else if (st == SQS_FLEE) q->flees++;
    else if (st == SQS_HUNT) q->raids++;
    else if (st == SQS_ESCORT) q->escorted++;
    else if (st == SQS_SCREEN) q->screens++;
}
// a ship of the role a squadron of kind k takes (main: all but the hunters; scouts: frigates; hunters: submarines and galleys)
internal int sq_hunter(int role) { return role == NR_SUB || role == NR_ANTI; }
internal int sq_fits(int k, int role) { return k == SQK_HUNT ? sq_hunter(role) : k == SQK_SCOUT ? role == NR_FRIG : !sq_hunter(role); }
// ZONES: the main squadrons' anchors per sea, scout groups - squadrons kept where their anchor stays
internal void sq_zones(unsigned idx, const int *oceans, const int *nship, const int *nfrig, const int *nsub, int noc, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    struct tv_st *s = &g_tv[idx];
    int n = s->n < SQ_T ? s->n : SQ_T;
    static uint8_t keep[SQ_MAX];
    memset(keep, 0, sizeof keep);
    for (int o = 0; o < noc; o++) {
        int oc = oceans[o];
        static struct { int r; float v; } sea[SQ_SEAS];
        int ns = 0;
        for (int r = 0; r < n && ns < SQ_SEAS; r++) {
            if (!g_ter.t[r].water || (int)sq_ocean_of(r) != oc) continue;
            float v = sq_sea_value(s, r, now);
            if (v > 0) { sea[ns].r = r; sea[ns++].v = v; }
        }
        for (int a = 1; a < ns; a++) for (int b = a; b > 0 && sea[b].v > sea[b - 1].v; b--) { typeof(sea[0]) t = sea[b]; sea[b] = sea[b - 1]; sea[b - 1] = t; }
        static int anchor[SQ_MAX]; static float zval[SQ_MAX];
        static uint8_t covered[SQ_SEAS];
        int na = 0;
        memset(covered, 0, sizeof covered);
        for (int a = 0; a < ns && na < SQ_MAX; a++) {
            if (covered[a]) continue;
            anchor[na] = sea[a].r; zval[na] = 0;
            for (int b = a; b < ns; b++)
                if (!covered[b] && ivec2_dist2(g_ter.t[sea[b].r].c, g_ter.t[sea[a].r].c) <= SQ_REACH * SQ_REACH) { covered[b] = 1; zval[na] += sea[b].v; }
            na++;
        }
        int scouts = nfrig[o] >= 2 ? (na > 1 ? 2 : 1) : 0;
        int split = 0;   // CONCENTRATION
        for (int k = 0; k < z->nsea; k++) if (z->sea[k].oc == oc) split = z->sea[k].split;
        if (!split) na = na > 1 ? 1 : na;
        int hunters = (nsub[o] + SQ_HUNT_N - 1) / SQ_HUNT_N;
        if (hunters > 3) hunters = 3;
        int mains = (nship[o] - nsub[o] - (scouts * SQ_SCOUT_N < nfrig[o] ? scouts * SQ_SCOUT_N : nfrig[o])) / SQ_MAIN_MIN;
        if (mains < 1) mains = 1;
        int had = 0;   // STEADY: as many as before unless the ships make a clear difference (one squadron's minimum and 2)
        for (int i = 0; i < SQ_MAX; i++) had += z->s[i].on && z->s[i].kind == SQK_MAIN && z->s[i].ocean == oc;
        if (had && mains != had && abs((nship[o] - nsub[o]) - had * SQ_MAIN_MIN) < SQ_MAIN_MIN + 2) mains = had;
        if (mains > na) mains = na > 0 ? na : 1;
        if (!na) { anchor[0] = -1; zval[0] = 1; na = 1; }   // no water of worth on this sea yet: one squadron by its ships
        for (int k = 0; k < mains; k++) {   // a main squadron per anchor - the nearest there is (its ships stay with it, the zone moves), else a new one
            int found = -1, fd = 0x7FFFFFFF;
            ivec2 ac = anchor[k] >= 0 ? g_ter.t[anchor[k]].c : iv2(0, 0);
            for (int i = 0; i < SQ_MAX; i++) {
                struct sq *q = &z->s[i];
                if (!q->on || keep[i] || q->kind != SQK_MAIN || q->ocean != oc) continue;
                int d = anchor[k] < 0 ? 0 : ivec2_dist2(q->n ? sq_center(q) : q->zc, ac);
                if (d < fd) { fd = d; found = i; }
            }
            for (int i = 0; i < SQ_MAX && found < 0; i++) if (!z->s[i].on && !keep[i]) { memset(&z->s[i], 0, sizeof z->s[i]); found = i; }
            if (found < 0) break;
            struct sq *q = &z->s[found];
            q->on = 1; q->kind = SQK_MAIN; q->ocean = oc; q->anchor = anchor[k]; q->zvalue = zval[k];
            if (anchor[k] >= 0) q->zc = ac;
            keep[found] = 1;
        }
        for (int k = 0; k < scouts; k++) {
            int found = -1;
            for (int i = 0; i < SQ_MAX && found < 0; i++) if (z->s[i].on && !keep[i] && z->s[i].kind == SQK_SCOUT && z->s[i].ocean == oc) found = i;
            for (int i = 0; i < SQ_MAX && found < 0; i++) if (!z->s[i].on && !keep[i]) { memset(&z->s[i], 0, sizeof z->s[i]); found = i; }
            if (found < 0) break;
            struct sq *q = &z->s[found];
            if (!q->on) q->target_r = -1;
            q->on = 1; q->kind = SQK_SCOUT; q->ocean = oc; q->anchor = -1; q->want = SQ_SCOUT_N;
            keep[found] = 1;
        }
        for (int k = 0; k < hunters; k++) {
            int found = -1;
            for (int i = 0; i < SQ_MAX && found < 0; i++) if (z->s[i].on && !keep[i] && z->s[i].kind == SQK_HUNT && z->s[i].ocean == oc) found = i;
            for (int i = 0; i < SQ_MAX && found < 0; i++) if (!z->s[i].on && !keep[i]) { memset(&z->s[i], 0, sizeof z->s[i]); found = i; }
            if (found < 0) break;
            struct sq *q = &z->s[found];
            q->on = 1; q->kind = SQK_HUNT; q->ocean = oc; q->anchor = -1; q->want = (nsub[o] + hunters - 1) / hunters;
            keep[found] = 1;
        }
    }
    for (int i = 0; i < SQ_MAX; i++) if (z->s[i].on && !keep[i]) z->s[i].on = 0;   // its zone gone: its ships go to the others
    z->zone_t = now ? now : 1;
}
internal void sq_assign(unsigned idx, struct fleet *f, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    static signed char was[FL_MAX];
    static uint8_t kept[FL_MAX];   // taken in pass 0 (asked again in pass 1, the squadron filled since: in two squadrons, ordered twice a look)
    for (int i = 0; i < SQ_MAX; i++) z->s[i].n = 0;
    // the main squadrons' shares on each sea by their zones' worth
    for (int i = 0; i < SQ_MAX; i++) {
        struct sq *q = &z->s[i];
        if (!q->on || q->kind != SQK_MAIN) continue;
        float sum = 0; int ships = 0, scouts = 0;
        for (int j = 0; j < SQ_MAX; j++) if (z->s[j].on && z->s[j].ocean == q->ocean && z->s[j].kind == SQK_MAIN) sum += z->s[j].zvalue;
        for (int j = 0; j < SQ_MAX; j++) if (z->s[j].on && z->s[j].ocean == q->ocean && z->s[j].kind == SQK_SCOUT) scouts += SQ_SCOUT_N;
        for (int k = 0; k < f->n; k++) ships += fl_ship_ocean(f->u[k]) == q->ocean && !sq_hunter(nv_role(unit_get_type_data(f->u[k])));
        ships -= scouts;
        q->want = sum > 0 ? (int)((float)ships * q->zvalue / sum + 0.5f) : ships;
        if (q->want < SQ_MAIN_MIN) q->want = SQ_MAIN_MIN;
    }
    // who was where
    for (int k = 0; k < f->n; k++) {
        ee_unit *u = f->u[k];
        was[k] = -1;
        for (int j = 0; j < z->nm; j++) if (z->m[j].u == u && z->m[j].id == unit_get_serial(u)) { was[k] = z->m[j].sq; break; }
    }
    int nm = 0;
    memset(kept, 0, sizeof kept);
    z->free_nosea = z->free_nofit = 0; z->docked = f->ndocked;
    for (int k = 0; k < f->n; k++) {
        int oc = fl_ship_ocean(f->u[k]), role = nv_role(unit_get_type_data(f->u[k])), fit = 0;
        for (int i = 0; i < SQ_MAX; i++) fit |= z->s[i].on && z->s[i].ocean == oc && sq_fits(z->s[i].kind, role);
        if (!oc) z->free_nosea++; else if (!fit) z->free_nofit++;
    }
    for (int pass = 0; pass < 2; pass++)
        for (int k = 0; k < f->n; k++) {
            ee_unit *u = f->u[k];
            int oc = fl_ship_ocean(u), role = nv_role(unit_get_type_data(u)), q = -1;
            int docked = 0;
            for (int j = 0; j < f->ndocked; j++) docked |= f->docked[j] == u;
            if (docked) continue;
            if (pass == 0) {   // its squadron still, while that one has room for it
                q = was[k];
                if (q < 0 || q >= SQ_MAX || !z->s[q].on || z->s[q].ocean != oc || !sq_fits(z->s[q].kind, role) || z->s[q].n >= z->s[q].want + 2) continue;
            } else {
                if (kept[k]) continue;
                float best = -1e9f;
                for (int i = 0; i < SQ_MAX; i++) {
                    struct sq *s2 = &z->s[i];
                    if (!s2->on || s2->ocean != oc || !sq_fits(s2->kind, role)) continue;
                    float short_ = (float)(s2->want - s2->n) + (s2->kind != SQK_MAIN && s2->n < s2->want ? 100.0f : 0.0f);
                    if (short_ > best) { best = short_; q = i; }
                }
                if (q < 0) continue;
                if (was[k] >= 0) z->moved++;
            }
            kept[k] = pass == 0;
            struct sq *t = &z->s[q];
            if (t->n < FL_MAX) t->u[t->n++] = u;
            if (nm < FL_MAX) { z->m[nm].u = u; z->m[nm].id = unit_get_serial(u); z->m[nm++].sq = (signed char)q; }
        }
    z->nm = nm;
    (void)now;
}
// hurt ships to the dock (fleet.c's list: the repair counted there); back with 80%. To the water beside the dock the old
// fleet uses (fl_repair_dock: out of danger, one with a tower near counting half the way; ocean_near within 4 tiles)
// and sent again while it is farther than 4 tiles - open water off the dock healed nothing (900 hit points regained a
// game against 11 000-18 000). Kept with its squadron while an enemy warship is near, they went down in the squadrons
// instead and more of them (27 lost against 22). Of those sunk after they were sent (most of its losses), a look at each
// (6 games): 87 of 122 within 15 s, 97 within 10 tiles of where they turned, 106 with an enemy warship within 10 tiles
// - lost in the fight they were leaving, not on a long way
internal void sq_repair(ee_player *pl, unsigned idx, struct fleet *f) {
    struct sq_st *z = &g_sq[idx];
    for (int k = 0; k < f->n; k++) {
        ee_unit *u = f->u[k];
        int j = 0;
        while (j < f->ndocked && f->docked[j] != u) j++;
        if (j == f->ndocked && (float)unit_get_hp(u) >= SQ_REPAIR * fl_hp_max(u)) continue;
        int d = fl_repair_dock(unit_get_tile(u));
        ivec2 w;
        if (d < 0 || (int)ocean_near(g_fs.dk[d], 4, &w) != fl_ship_ocean(u)) continue;
        if (j == f->ndocked) { if (j >= FL_MAX) continue; f->docked[j] = u; f->docked_hp[j] = unit_get_hp(u); f->ndocked++; z->repairs++; }
        if (!cl_take(u, CLP_REPAIR, CLW_REPAIR, CL_REPAIR_T)) continue;   // ORDERS BY PRIORITY (claims.c): the repair's, out of a storm first
        cl_stance(u, 1);   // stand ground till repaired: its own AI chased what it saw by the dock and was called back again and again
        // BEHIND THE SQUADRON: under fire (an enemy warship seen within SQ_BEHIND_E), first behind the nearest main
        // squadron within SQ_BEHIND_M - SQ_BEHIND tiles past its middle, away from the enemy -, the dock once none is near
        ivec2 up = unit_get_tile(u), e = iv2(0, 0);
        int ne = 0;
        for (int x = 0; x < g_fs.ne; x++)
            if (g_fs.es[x].seen && ivec2_dist2(g_fs.es[x].p, up) <= SQ_BEHIND_E * SQ_BEHIND_E) { e.x += g_fs.es[x].p.x; e.y += g_fs.es[x].p.y; ne++; }
        if (ne) {
            e = iv2(e.x / ne, e.y / ne);
            ivec2 mc = iv2(0, 0);
            int md = SQ_BEHIND_M * SQ_BEHIND_M + 1;
            for (int i = 0; i < SQ_MAX; i++) {
                struct sq *q = &z->s[i];
                if (!q->on || q->kind != SQK_MAIN || q->ocean != fl_ship_ocean(u) || q->n < 2) continue;
                ivec2 c = sq_center(q);
                int dd = ivec2_dist2(c, up);
                if (dd < md) { md = dd; mc = c; }
            }
            float dx = (float)(mc.x - e.x), dy = (float)(mc.y - e.y), len = sqrtf(dx * dx + dy * dy);
            if (md <= SQ_BEHIND_M * SQ_BEHIND_M && len > 0.5f) {
                ivec2 b = iv2(mc.x + (int)(dx / len * SQ_BEHIND), mc.y + (int)(dy / len * SQ_BEHIND)), o;
                if (fl_water(b, (unsigned)fl_ship_ocean(u), &o) && !fl_in_danger(o)) { fl_move(f, u, o, 0); z->behind++; continue; }
            }
        }
        if (ivec2_dist2(g_fs.dk[d], up) > 16) fl_move(f, u, w, 0);   // round a danger zone on the way, not again while it goes
    }
    (void)pl;
}
// EACH SHIP: out of a danger zone, no target in one (fleet.c fl_flee, fl_orders' rules)
internal void sq_care(unsigned idx, struct fleet *f) {
    struct sq_st *z = &g_sq[idx];
    for (int k = 0; k < f->n; k++) {
        ee_unit *u = f->u[k];
        int docked = 0;
        for (int j = 0; j < f->ndocked; j++) docked |= f->docked[j] == u;
        if (docked) continue;
        int id = unit_get_serial(u);
        struct sq *mq = NULL;
        for (int j = 0; j < z->nm; j++)
            if (z->m[j].u == u && z->m[j].id == id && z->m[j].sq >= 0 && z->m[j].sq < SQ_MAX && z->s[(int)z->m[j].sq].on) { mq = &z->s[(int)z->m[j].sq]; break; }
        int shore = SQ_SHORE(idx);
        if (shore && mq && mq->state == SQS_SHELL && cl_may(u, CLP_ROUTINE, CLW_SQUADRON)) continue;   // THE SHORE: shelling, decided
        if (fl_flee(f, u)) { z->flees++; continue; }
        ee_unit *t = fl_target_of(u);
        if (!t) continue;
        ivec2 up = unit_get_tile(u), tp = unit_get_tile(t);
        int out = 0;   // THE SHORE: a land target only within its reach, from where it is
        if (shore && sq_on_land(t)) {
            float rg = fl_range(u, unit_get_type_data(u)) + 0.5f;
            out = (float)ivec2_dist2(up, tp) > rg * rg;
        }
        if (!out && !fl_in_danger_m(tp, 1.0f)) continue;
        ivec2 back = mq ? mq->go : f->g;
        if (!back.x && !back.y) back = f->g;
        if (fl_in_danger(back) || !cl_may(u, CLP_ROUTINE, CLW_SQUADRON)) continue;
        unit_goal(u, goal_move((float)back.x + 0.5f, (float)back.y + 0.5f), 0, 0x1F40);
        z->drops++; z->shore_drops += (unsigned)out;
    }
}
// ESCORT (convoy.c): each transport that asked within CY_WISH - the nearest scout group on its sea not running, else
// the nearest main squadron at rest (or escorting, screening); the answer back to it (0 none, 1 on its way, 2 with it)
// THE SQUADRON AHEAD (stations.c ACROSS, ferry.c - as landing.c's warships went ahead; the player 2026-11-18: "unifing
// the systems dosnt mean throwing out what worked"): from a push's first transport on until the push is over, the
// nearest main squadron on that sea not fighting in its zone goes ahead to the water off the push's beach and takes on
// the enemy warships within SQ_SCREEN_R of it or of itself, THE SCREEN's way - whatever the odds: it draws the fire the
// transports would take; the wave's transports wait for it at their meeting point (ferry.c TOGETHER, FY_MEET at most).
// Out of the coast's guns as every move (sq_safe): the beach lies out of every tower's reach it knows (stations.c
// sn_beach), and what stands on it is the soldiers' to take.
#define SQ_SUP_FRESH 10000
internal struct { ivec2 at; int oc, state; uint32_t t; } g_sq_sup[16][SN_PUSHES];
internal void squadrons_support(unsigned idx, int k, ivec2 at, int oc, uint32_t now) {
    if (k < 0 || k >= SN_PUSHES) return;
    g_sq_sup[idx & 15][k].at = at; g_sq_sup[idx & 15][k].oc = oc; g_sq_sup[idx & 15][k].t = now ? now : 1;
}
internal int squadrons_support_state(unsigned idx, int k) {   // 0 none to be had, 1 on its way, 2 there
    uint32_t now = game_time();
    if (k < 0 || k >= SN_PUSHES || !g_sq_sup[idx & 15][k].t || now - g_sq_sup[idx & 15][k].t >= SQ_SUP_FRESH) return 0;
    return g_sq_sup[idx & 15][k].state;
}
internal void sq_supports(unsigned idx, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    int had[SN_PUSHES];
    for (int k = 0; k < SN_PUSHES; k++) had[k] = -1;
    for (int i = 0; i < SQ_MAX; i++) { if (z->s[i].support) had[z->s[i].support - 1] = i; z->s[i].support = 0; }
    for (int k = 0; k < SN_PUSHES; k++) {
        typeof(&g_sq_sup[0][0]) w = &g_sq_sup[idx][k];
        w->state = 0;
        if (!w->t || now - w->t >= SQ_SUP_FRESH) continue;
        int best = -1, bd = 0x7FFFFFFF;
        for (int i = 0; i < SQ_MAX; i++) {
            struct sq *q = &z->s[i];
            if (!q->on || !q->n || q->kind != SQK_MAIN || q->ocean != w->oc || q->support) continue;
            if (i != had[k] && (q->state == SQS_ENGAGE || q->state == SQS_HOLD)) continue;   // fighting in its zone
            int d = i == had[k] ? -1 : ivec2_dist2(sq_center(q), w->at);   // the one it had keeps it
            if (d < bd) { bd = d; best = i; }
        }
        if (best < 0) continue;
        struct sq *q = &z->s[best];
        q->support = k + 1;
        ivec2 c = sq_center(q), to = sq_safe(c, w->at, q->ocean);
        w->state = ivec2_dist2(c, to) <= SQ_ESC_NEAR * SQ_ESC_NEAR ? 2 : 1;
        if (best != had[k] && g_log)
            log_locked("# squadrons: P%u at %.1f min: main squadron %d (%d ships) goes ahead of a push's landing to %d,%d\n", idx, now / 60000.0, best, q->n, w->at.x, w->at.y);
    }
}
internal void sq_support(ee_player *pl, unsigned idx, struct sq *q, uint32_t now) {
    ivec2 at = g_sq_sup[idx][q->support - 1].at, c = sq_center(q);
    int best = -1, bd = 0x7FFFFFFF, r2 = SQ_SCREEN_R * SQ_SCREEN_R;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if ((int)area_id(e->p, 1) != q->ocean || fl_in_danger(e->p)) continue;
        int d = ivec2_dist2(e->p, at);
        if ((d > r2 && ivec2_dist2(e->p, c) > r2) || d >= bd) continue;
        bd = d; best = j;
    }
    sq_state(q, SQS_SUPPORT, now);
    if (best >= 0) {   // the nearest to the beach's water, as THE SCREEN
        struct fl_ship *e = &g_fs.es[best];
        if (e->seen && e->u && now - q->order_t >= 3000) {
            for (int k = 0; k < q->n; k++) { if (!cl_may(q->u[k], CLP_ROUTINE, CLW_SQUADRON)) continue; ee_goal *g = goal_attack(q->u[k], e->u); if (g) unit_goal(q->u[k], g, 0, 0x1F40); }
            q->go = q->ordered = e->p; q->order_t = now ? now : 1;
        } else if (!e->seen) sq_go(pl, q, e->p, 1, now);
        return;
    }
    sq_go(pl, q, sq_safe(c, at, q->ocean), 1, now);
}
internal void sq_convoys(unsigned idx, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    struct cy_st *c = &g_cy[idx];
    for (int i = 0; i < SQ_MAX; i++) z->s[i].escort = 0;
    for (int k = 0; k < c->nw; k++) {
        struct cy_wish *w = &c->w[k];
        w->state = 0;
        if (!w->t || now - w->t >= CY_WISH || !w->u || unit_get_serial(w->u) != w->id || unit_get_hp(w->u) <= 0) continue;
        ivec2 p = unit_get_tile(w->u);
        int oc = fl_ship_ocean(w->u), best = -1, bd = 0x7FFFFFFF;
        for (int pass = 0; pass < 2 && best < 0; pass++)
            for (int i = 0; i < SQ_MAX; i++) {
                struct sq *q = &z->s[i];
                if (!q->on || !q->n || q->escort || q->ocean != oc) continue;
                if (pass == 0 ? q->kind != SQK_SCOUT || q->state == SQS_FLEE : q->kind != SQK_MAIN || (q->state != SQS_REST && q->state != SQS_ESCORT && q->state != SQS_SCREEN)) continue;
                int d = ivec2_dist2(sq_center(q), p);
                if (d < bd) { bd = d; best = i; }
            }
        if (best < 0) continue;
        z->s[best].escort = k + 1;
        w->state = bd <= SQ_ESC_NEAR * SQ_ESC_NEAR ? 2 : 1;
    }
}
// ESCORT: before the crossing the scouts on its way's middle, at sea SQ_AHEAD ahead of the transport; a main squadron
// with it; at sea THE SCREEN first
internal int sq_escort_sea(unsigned idx, const struct sq *q) { return q->escort && g_cy[idx].w[q->escort - 1].sea; }
internal void sq_escort(ee_player *pl, unsigned idx, struct sq *q, uint32_t now) {
    struct cy_wish *w = &g_cy[idx].w[q->escort - 1];
    ivec2 p = unit_get_tile(w->u), c = sq_center(q), to = p;
    if (w->sea) {   // THE SCREEN
        int best = -1, bd = 0x7FFFFFFF, r2 = SQ_SCREEN_R * SQ_SCREEN_R;
        for (int j = 0; j < g_fs.ne; j++) {
            struct fl_ship *e = &g_fs.es[j];
            if ((int)area_id(e->p, 1) != q->ocean || fl_in_danger(e->p)) continue;
            int d = ivec2_dist2(e->p, p);
            if ((d > r2 && ivec2_dist2(e->p, c) > r2) || d >= bd) continue;
            bd = d; best = j;
        }
        if (best >= 0) {
            struct fl_ship *e = &g_fs.es[best];
            sq_state(q, SQS_SCREEN, now);
            if (e->seen && e->u && now - q->order_t >= 3000) {   // the warship itself, seen: each ship attacks it
                for (int k = 0; k < q->n; k++) { if (!cl_may(q->u[k], CLP_ROUTINE, CLW_SQUADRON)) continue; ee_goal *g = goal_attack(q->u[k], e->u); if (g) unit_goal(q->u[k], g, 0, 0x1F40); }
                q->go = q->ordered = e->p; q->order_t = now ? now : 1;
            } else if (!e->seen) sq_go(pl, q, e->p, 1, now);
            return;
        }
    } else if (q->kind == SQK_SCOUT) {   // the sweep: on its way's middle
        sq_state(q, SQS_ESCORT, now);
        sq_go(pl, q, sq_safe(c, iv2((w->from.x + w->dest.x) / 2, (w->from.y + w->dest.y) / 2), q->ocean), 1, now);
        return;
    }
    float dx = (float)(w->dest.x - p.x), dy = (float)(w->dest.y - p.y), len = sqrtf(dx * dx + dy * dy);
    if (q->kind == SQK_SCOUT && len > 0.5f) {
        float a = len < (float)SQ_AHEAD ? len : (float)SQ_AHEAD;
        to = iv2(p.x + (int)(dx / len * a), p.y + (int)(dy / len * a));
    }
    sq_state(q, SQS_ESCORT, now);
    sq_go(pl, q, sq_safe(c, to, q->ocean), 1, now);
}
// THE SHORE: SHELLING - 1 while q shells (its orders given), 0 not
internal int sq_shell(ee_player *pl, unsigned idx, struct sq *q, ivec2 home, uint32_t now) {
    static ee_unit *def[SQ_SHELL_N];
    ee_unit *closest = NULL, *t = q->shell && unit_get_serial(q->shell) == q->shell_id && unit_get_hp(q->shell) > 0 && unit_get_owner(q->shell) &&
                               is_enemy(pl, unit_get_owner(q->shell)) ? q->shell : NULL;
    ivec2 c = sq_center(q);
    int nd = 0;
    if (t) {
        nd = sq_shore_defs(pl, unit_get_tile(t), c, def, SQ_SHELL_N, &closest);
        float r = sq_shore_ratio(q, def, nd);
        if (r < SQ_SHELL_HOLD || sq_hp_share(q) < SQ_SHELL_OFF) t = NULL; else q->ratio = r;
    }
    if (!t && q->n >= 3 && sq_hp_share(q) >= SQ_SHELL_HP) {   // the nearest buildings on its sea's coast near its zone, while it beats what reaches them
        static unsigned char done[96];
        memset(done, 0, sizeof done);
        for (int look = 0; look < SQ_SHELL_LOOKS && !t; look++) {
            int bj = -1, bd = SQ_SHELL_R * SQ_SHELL_R + 1;
            for (int j = 0; j < g_fs.nec && j < 96; j++) {
                struct fl_ship *e = &g_fs.ec[j];
                int cl = e->u ? unit_type_get_class_id(e->td) : -1;
                if (done[j] || (cl != CLS_BUILDING && cl != CLS_FORTRESS) || unit_get_hp(e->u) <= 0) continue;
                int d = ivec2_dist2(e->p, home);
                if (d < bd && (int)ocean_near(e->p, 8, NULL) == q->ocean) { bd = d; bj = j; }
            }
            if (bj < 0) break;
            done[bj] = 1;
            nd = sq_shore_defs(pl, g_fs.ec[bj].p, c, def, SQ_SHELL_N, &closest);
            float r = sq_shore_ratio(q, def, nd);
            if (r >= SQ_SHELL_GO) {
                t = g_fs.ec[bj].u; q->ratio = r; q->shelled++;
                if (g_log) log_locked("# squadrons: P%u at %.1f min: squadron %d (%d ships, %.0f%% of their hit points) shells type %d at %d,%d - %d defences reach it, estimate %.2f\n",
                                      idx, now / 60000.0, (int)(q - g_sq[idx].s), q->n, 100.0 * sq_hp_share(q), unit_type_get_id(g_fs.ec[bj].td), g_fs.ec[bj].p.x, g_fs.ec[bj].p.y, nd, (double)r);
            }
        }
    }
    if (!t) { q->shell = NULL; return 0; }
    q->shell = t; q->shell_id = unit_get_serial(t);
    ee_unit *aim = closest ? closest : t;   // the defences reaching it first, the nearest
    sq_state(q, SQS_SHELL, now);
    if (!q->order_t || now - q->order_t >= 3000) {
        for (int k = 0; k < q->n; k++) {
            if (!cl_may(q->u[k], CLP_ROUTINE, CLW_SQUADRON) || fl_target_of(q->u[k]) == aim) continue;
            ee_goal *g = goal_attack(q->u[k], aim);
            if (g) unit_goal(q->u[k], g, 0, 0x1F40);
        }
        q->go = q->ordered = unit_get_tile(aim); q->order_t = now ? now : 1;
    }
    return 1;
}
internal void sq_main(ee_player *pl, unsigned idx, int i, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    struct sq *q = &z->s[i];
    static unsigned char sel[FL_ES];
    ivec2 c = sq_center(q), home = q->anchor >= 0 ? q->zc : c;
    q->call = 0;
    if (sq_escort_sea(idx, q)) { sq_escort(pl, idx, q, now); return; }   // THE SCREEN: its transport at sea first
    if (q->support) { sq_support(pl, idx, q, now); return; }   // THE SQUADRON AHEAD
    int threat = sq_enemies(q->ocean, home, SQ_REACH, sel);
    if (!threat) {   // HOME: enemy warships by a dock or fishing boat of its on this sea, this the nearest main squadron to it
        int bd = 0x7FFFFFFF, nt = 0;
        ivec2 hp = iv2(0, 0);
        static ivec2 tp[CY_SHIPS];   // ESCORT: its loaded transports
        for (int j = 0; CONVOY_ESCORT(idx) && j < g_cy[idx].n; j++) {
            struct cy_ship *e = &g_cy[idx].s[j];
            if (e->aboard > 0 && unit_get_serial(e->u) == e->id && unit_get_hp(e->u) > 0) tp[nt++] = unit_get_tile(e->u);
        }
        for (int k = 0; k < g_fs.nd + g_fs.nb + nt; k++) {
            ivec2 b = k < g_fs.nd ? g_fs.dk[k] : k < g_fs.nd + g_fs.nb ? unit_get_tile(g_fs.boat[k - g_fs.nd]) : tp[k - g_fs.nd - g_fs.nb];
            if (k < g_fs.nd ? (int)ocean_near(b, 4, NULL) != q->ocean : (int)area_id(b, 1) != q->ocean) continue;
            int d = ivec2_dist2(b, c);
            if (d >= bd || !sq_enemies(q->ocean, b, SQ_HOME_R, sel)) continue;
            int nearer = 0;
            for (int j = 0; j < SQ_MAX && !nearer; j++)
                nearer = j != i && z->s[j].on && z->s[j].kind == SQK_MAIN && z->s[j].ocean == q->ocean && z->s[j].n && ivec2_dist2(sq_center(&z->s[j]), b) < d;
            if (!nearer) { bd = d; hp = b; }
        }
        if (bd != 0x7FFFFFFF) threat = sq_enemies(q->ocean, hp, SQ_HOME_R, sel);
    }
    if (threat) {   // enemy warships in its zone: fight them when not losing, else hold under its towers and call
        q->ratio = sq_ratio(q, sel);
        ivec2 e = iv2(0, 0);
        int ne = 0;
        for (int j = 0; j < g_fs.ne; j++) if (sel[j]) { e.x += g_fs.es[j].p.x; e.y += g_fs.es[j].p.y; ne++; }
        e = iv2(e.x / ne, e.y / ne);
        if (q->state == SQS_HELP || q->ratio >= SQ_ENGAGE || (q->state == SQS_ENGAGE && q->ratio >= SQ_HOLD)) { sq_state(q, SQS_ENGAGE, now); sq_go(pl, q, sq_safe(c, e, q->ocean), 1, now); }
        else { sq_state(q, SQS_HOLD, now); q->call = 1; sq_go(pl, q, sq_haven(q, e), 0, now); }
        return;
    }
    // a call from another squadron of the same sea: answered while its own zone is quiet and it has half its hit points
    if (q->state == SQS_HELP && (q->help <= 0 || !z->s[q->help - 1].on || !z->s[q->help - 1].call)) q->help = 0;
    if (!q->help && sq_hp_share(q) >= 0.5f)
        for (int j = 0; j < SQ_MAX; j++)
            if (j != i && z->s[j].on && z->s[j].kind == SQK_MAIN && z->s[j].ocean == q->ocean && z->s[j].call) { q->help = j + 1; break; }
    if (q->help) {
        struct sq *h = &z->s[q->help - 1];
        sq_state(q, SQS_HELP, now);
        sq_go(pl, q, sq_safe(c, sq_center(h), q->ocean), 1, now);
        return;
    }
    if (q->escort) { sq_escort(pl, idx, q, now); return; }   // ESCORT: its zone quiet, with the transport
    // A PUSH: clearly the stronger against the enemy warships known within 2 x SQ_REACH of its zone - after them -, and
    // not weaker than all it knows on the sea (CONCENTRATION)
    float whole = 99.0f;
    for (int k = 0; k < z->nsea; k++) if (z->sea[k].oc == q->ocean) whole = z->sea[k].ratio;
    int far_ = sq_enemies(q->ocean, home, 2 * SQ_REACH, sel);
    if (far_ && q->n >= SQ_MAIN_MIN && sq_hp_share(q) >= 0.6f && whole >= 1.0f) {
        q->ratio = sq_ratio(q, sel);
        if (q->ratio >= SQ_PUSH || (q->state == SQS_PUSH && q->ratio >= 1.1f)) {
            ivec2 e = iv2(0, 0);
            int bd = 0x7FFFFFFF;   // the nearest of them
            for (int j = 0; j < g_fs.ne; j++) if (sel[j]) { int d = ivec2_dist2(g_fs.es[j].p, c); if (d < bd) { bd = d; e = g_fs.es[j].p; } }
            sq_state(q, SQS_PUSH, now);
            sq_go(pl, q, sq_safe(c, e, q->ocean), 1, now);
            return;
        }
    }
    if (SQ_SHORE(idx) && sq_shell(pl, idx, q, home, now)) return;   // THE SHORE: SHELLING
    q->shell = NULL;
    sq_state(q, SQS_REST, now);
    if (q->anchor >= 0) sq_go(pl, q, sq_safe(c, q->zc, q->ocean), !SQ_SHORE(idx), now);   // THE SHORE: a plain move
}
// the nearest main squadron's middle on q's sea (none: the haven)
internal ivec2 sq_to_main(struct sq_st *z, const struct sq *q, ivec2 c) {
    ivec2 to = c;
    int bd = 0x7FFFFFFF;
    for (int j = 0; j < SQ_MAX; j++) {
        struct sq *m = &z->s[j];
        if (!m->on || m->kind != SQK_MAIN || m->ocean != q->ocean || !m->n) continue;
        ivec2 mc = sq_center(m);
        int d = ivec2_dist2(mc, c);
        if (d < bd) { bd = d; to = mc; }
    }
    return bd == 0x7FFFFFFF ? sq_haven(q, c) : to;
}
internal int sq_frigate_near(int oc, ivec2 p, int r) {   // an enemy frigate known within r of p
    for (int j = 0; j < g_fs.ne; j++)
        if ((int)area_id(g_fs.es[j].p, 1) == oc && ivec2_dist2(g_fs.es[j].p, p) <= r * r && nv_role(g_fs.es[j].td) == NR_FRIG) return 1;
    return 0;
}
internal void sq_hunt(ee_player *pl, unsigned idx, int i, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    struct sq *q = &z->s[i];
    static unsigned char sel[FL_ES];
    ivec2 c = sq_center(q);
    if (sq_frigate_near(q->ocean, c, SQ_FRIG_R)) { sq_state(q, SQS_FLEE, now); sq_go(pl, q, sq_to_main(z, q, c), 0, now); return; }
    int bj = -1, bd = 0x7FFFFFFF;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if ((int)area_id(e->p, 1) != q->ocean || nv_role(e->td) == NR_FRIG || fl_in_danger(e->p)) continue;
        int d = ivec2_dist2(e->p, c);
        if (d > SQ_HUNT_R * SQ_HUNT_R || d >= bd || sq_frigate_near(q->ocean, e->p, SQ_FRIG_R)) continue;
        sq_enemies(q->ocean, e->p, 8, sel);
        float rr = sq_ratio(q, sel);
        if (rr < 1.0f) continue;
        bd = d; bj = j; q->ratio = rr;
    }
    if (bj >= 0) {
        struct fl_ship *e = &g_fs.es[bj];
        sq_state(q, SQS_HUNT, now);
        if (e->u && now - q->order_t >= 3000) {
            for (int k = 0; k < q->n; k++) { if (!cl_may(q->u[k], CLP_ROUTINE, CLW_SQUADRON)) continue; ee_goal *g = goal_attack(q->u[k], e->u); if (g) unit_goal(q->u[k], g, 0, 0x1F40); }
            q->go = q->ordered = e->p; q->order_t = now ? now : 1;
        } else if (!e->u) sq_go(pl, q, sq_safe(c, e->p, q->ocean), 1, now);
        return;
    }
    sq_state(q, SQS_REST, now);
    sq_go(pl, q, sq_safe(c, sq_to_main(z, q, c), q->ocean), !SQ_SHORE(idx), now);   // THE SHORE: a plain move
}
internal void sq_scout(ee_player *pl, unsigned idx, int i, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    struct sq *q = &z->s[i];
    struct tv_st *s = &g_tv[idx];
    static unsigned char sel[FL_ES];
    ivec2 c = sq_center(q);
    if (sq_escort_sea(idx, q)) { sq_escort(pl, idx, q, now); return; }   // THE SCREEN: its transport at sea - it doesn't run
    if (sq_enemies(q->ocean, c, SQ_SCOUT_FLEE, sel) && (q->ratio = sq_ratio(q, sel)) < SQ_SCOUT_FIGHT) {   // warships it doesn't beat: to the nearest main squadron
        sq_state(q, SQS_FLEE, now);
        sq_go(pl, q, sq_to_main(z, q, c), 0, now);
        return;
    }
    if (q->escort) { sq_escort(pl, idx, q, now); return; }   // ESCORT: ahead of the transport
    // A RAID: an enemy fishing boat or transport within SQ_RAID_R, no warship near it that beats the group, no danger
    int bp = -1, bd = 0x7FFFFFFF;
    for (int k = 0; k < g_fs.nev; k++) {
        struct fl_ship *e = &g_fs.ev[k];
        if ((int)area_id(e->p, 1) != q->ocean || fl_in_danger(e->p)) continue;
        int d = ivec2_dist2(e->p, c);
        if (d > SQ_RAID_R * SQ_RAID_R || d >= bd) continue;
        if (sq_enemies(q->ocean, e->p, 12, sel) && sq_ratio(q, sel) < SQ_SCOUT_FIGHT) continue;
        bd = d; bp = k;
    }
    if (bp >= 0) {
        struct fl_ship *e = &g_fs.ev[bp];
        sq_state(q, SQS_RAID, now);
        if (e->u && now - q->order_t >= 3000) {   // the prey itself, seen: each ship attacks it
            for (int k = 0; k < q->n; k++) { if (!cl_may(q->u[k], CLP_ROUTINE, CLW_SQUADRON)) continue; ee_goal *g = goal_attack(q->u[k], e->u); if (g) unit_goal(q->u[k], g, 0, 0x1F40); }
            q->go = q->ordered = e->p; q->order_t = now ? now : 1;
        } else if (!e->u) sq_go(pl, q, e->p, 1, now);
        return;
    }
    // SCOUTING: the water of its sea not seen longest - the unheld first; a new one when there, or after 90 s
    int n = s->n < SQ_T ? s->n : SQ_T;
    int there = q->target_r >= 0 && g_sq_look[idx][q->target_r] && g_sq_look[idx][q->target_r] >= q->target_t;   // one of its warships has been in it since
    if (q->target_r >= 0 && (there || now - q->target_t >= 90000)) { q->scouted += (unsigned)there; q->target_r = -1; }
    if (q->target_r < 0) {
        float best = -1;
        for (int r = 0; r < n; r++) {
            if (!g_ter.t[r].water || (int)sq_ocean_of(r) != q->ocean || fl_in_danger(g_ter.t[r].c)) continue;
            uint32_t seen = g_sq_look[idx][r];
            float age = !seen ? 1e7f : (float)(now - seen);
            if (seen && age < SQ_RESCOUT) continue;
            int taken = 0;   // not the one another scout group goes to
            for (int j = 0; j < SQ_MAX; j++) taken |= j != i && z->s[j].on && z->s[j].kind == SQK_SCOUT && z->s[j].target_r == r;
            if (taken) continue;
            float w = age * (s->t[r].role ? 1.0f : 2.0f) / (1.0f + sqrtf((float)ivec2_dist2(g_ter.t[r].c, c)) / 20.0f);
            if (w > best) { best = w; q->target_r = r; }
        }
        q->target_t = now ? now : 1;
    }
    if (q->target_r >= 0) { sq_state(q, SQS_SCOUT, now); sq_go(pl, q, sq_safe(c, g_ter.t[q->target_r].c, q->ocean), !SQ_SHORE(idx), now); return; }   // THE SHORE: a plain move
    // nothing to look at: with the nearest main squadron
    for (int j = 0; j < SQ_MAX; j++)
        if (z->s[j].on && z->s[j].kind == SQK_MAIN && z->s[j].ocean == q->ocean && z->s[j].n) { sq_state(q, SQS_REST, now); sq_go(pl, q, sq_safe(c, sq_center(&z->s[j]), q->ocean), !SQ_SHORE(idx), now); return; }
}
internal void sq_log(unsigned idx, uint32_t now) {
    cl_log(idx, now);   // claims.c
    struct sq_st *z = &g_sq[idx];
    if (!g_log || (z->log_t && now >= z->log_t && now - z->log_t < 60000)) return;
    z->log_t = now ? now : 1;
    char b[900] = "";
    int nsq = 0;
    for (int i = 0; i < SQ_MAX; i++) {
        struct sq *q = &z->s[i];
        if (!q->on) continue;
        nsq++;
        ivec2 c = sq_center(q);
        size_t l = strlen(b);
        if (l < sizeof b - 160)
            snprintf(b + l, sizeof b - l, " | %s %d (sea %d%s%d): %d ships of %d at %d,%d, %s, ratio %.2f, so far fought %u held %u helped %u pushed %u raided/hunted %u ran %u looked %u escorted %u screened %u shelled %u",
                     k_sqk[q->kind], i, q->ocean, q->anchor >= 0 ? ", zone " : ", no zone ", q->anchor, q->n, q->want, c.x, c.y, k_sqs[q->state],
                     (double)q->ratio, q->engages, q->holds, q->helps, q->pushes, q->raids, q->flees, q->scouted, q->escorted, q->screens, q->shelled);
    }
    char lb[200];
    snprintf(lb, sizeof lb, "lost: main %u scouts %u hunters %u, sent to repair %u, landings' escorts %u, in none %u; by what they did:", z->lost_kind[SQK_MAIN],
             z->lost_kind[SQK_SCOUT], z->lost_kind[SQK_HUNT], z->lost_repair, z->lost_landing, z->lost_none);
    for (int k = 0; k < SQS_N; k++) if (z->lost_state[k]) { size_t l = strlen(lb); snprintf(lb + l, sizeof lb - l, " %s %u", k_sqs[k], z->lost_state[k]); }
    log_locked("# squadrons: P%u at %.1f min: %d squadrons, %u ships moved between them, %u sent to repair (%d at the dock now), %u out of a danger zone, %u targets in one given up (%u land targets out of reach), %u hurt ones behind a squadron (looks), in no squadron now: on no sea %d, none takes them %d; %s%s\n",
               idx, now / 60000.0, nsq, z->moved, z->repairs, z->docked, z->flees, z->drops, z->shore_drops, z->behind, z->free_nosea, z->free_nofit, lb, b);
}
internal void av_fleet_sync(unsigned idx, int sub, ee_unit *const *u, int n, const char *what, const char *task, uint32_t now);   // army_view.c
// fleet.c fleet_tick, after its scan, the fleet's ships in f (all its warships afloat)
internal void sq_tick(ee_player *pl, unsigned idx, struct fleet *f, uint32_t now) {
    struct sq_st *z = &g_sq[idx];
    if (now < z->t) { memset(z, 0, sizeof *z); memset(g_sq_look[idx], 0, sizeof g_sq_look[idx]); }
    struct tv_st *s = &g_tv[idx];
    int n = s->n < SQ_T ? s->n : SQ_T;
    for (int k = 0; k < f->n; k++) {   // A LOOK AGAIN: the waters its warships stand in
        ivec2 p = unit_get_tile(f->u[k]);
        unsigned r = ter_near(p.x, p.y);
        if (r < (unsigned)n) g_sq_look[idx][r] = now ? now : 1;
    }
    sq_repair(pl, idx, f);   // EACH SHIP, every look of the fleet's
    sq_care(idx, f);
    if (z->t && now - z->t < SQ_LOOK) return;
    z->t = now ? now : 1;
    if (!n || s->version != g_ter.version) return;
    int oceans[4], nship[4] = { 0 }, nfrig[4] = { 0 }, nsub[4] = { 0 }, noc = 0;
    for (int k = 0; k < f->n; k++) {
        int oc = fl_ship_ocean(f->u[k]), o = 0;
        if (!oc) continue;
        while (o < noc && oceans[o] != oc) o++;
        if (o == noc) { if (noc == 4) continue; oceans[noc++] = oc; }
        int role = nv_role(unit_get_type_data(f->u[k]));
        nship[o]++; nfrig[o] += role == NR_FRIG; nsub[o] += sq_hunter(role);
    }
    {   // CONCENTRATION: each sea's whole ratio - split from SQ_SPLIT, together under 1.0 (at once: the zones anew)
        static unsigned char sel[FL_ES];
        int regroup = 0;
        for (int o = 0; o < noc; o++) {
            int k = 0;
            while (k < z->nsea && z->sea[k].oc != oceans[o]) k++;
            if (k == z->nsea) { if (k == 4) continue; z->sea[k].oc = oceans[o]; z->sea[k].split = 0; z->nsea++; }
            struct sq all = { 0 };
            for (int j = 0; j < f->n && all.n < FL_MAX; j++) if (fl_ship_ocean(f->u[j]) == oceans[o]) all.u[all.n++] = f->u[j];
            sq_enemies(oceans[o], iv2(0, 0), 30000, sel);
            float r = sq_ratio(&all, sel);
            z->sea[k].ratio = r;
            if (!z->sea[k].split && r >= SQ_SPLIT) { z->sea[k].split = 1; regroup = 1; }
            else if (z->sea[k].split && r < 1.0f) { z->sea[k].split = 0; regroup = 1; }
        }
        if (regroup) z->zone_t = 0;
    }
    if (!z->zone_t || now - z->zone_t >= SQ_ZONES) sq_zones(idx, oceans, nship, nfrig, nsub, noc, now);
    sq_assign(idx, f, now);
    sq_convoys(idx, now);
    sq_supports(idx, now);
    for (int i = 0; i < SQ_MAX; i++) {
        struct sq *q = &z->s[i];
        if (!q->on || !q->n) continue;
        if (q->kind == SQK_MAIN) sq_main(pl, idx, i, now); else if (q->kind == SQK_HUNT) sq_hunt(pl, idx, i, now); else sq_scout(pl, idx, i, now);
    }
    for (int i = 0; i < SQ_MAX; i++) {
        struct sq *q = &z->s[i];
        av_fleet_sync(idx, i, q->u, q->on ? q->n : 0, q->kind == SQK_MAIN ? "main squadron" : q->kind == SQK_HUNT ? "hunters" : "scouts", k_sqs[q->state], now);
    }
    sq_log(idx, now);
}
internal void sq_lost(unsigned idx, ee_unit *dead) {   // diplo.c's kill hook, every unit of its that dies: its warships only
    ee_unit_type *td = unit_get_type_data(dead);
    int k = td ? navy_kind(td) : NV_NONE;
    if (k != NV_WAR && k != NV_SUB) return;
    struct sq_st *z = &g_sq[idx & 15];
    struct fleet *f = &g_fl[idx & 15];
    for (int j = 0; j < f->ndocked; j++)
        if (f->docked[j] == dead) {
            z->lost_repair++;
            return;
        }
    ee_player *o = unit_get_owner(dead);
    if (o && landing_has_unit(o, dead)) { z->lost_landing++; return; }   // landing.c's: not the fleet's
    int id = unit_get_serial(dead);
    for (int j = 0; j < z->nm; j++)
        if (z->m[j].u == dead && z->m[j].id == id && z->m[j].sq >= 0 && z->m[j].sq < SQ_MAX) {
            struct sq *q = &z->s[(int)z->m[j].sq];
            z->lost_kind[q->kind]++; z->lost_state[q->state]++;
            return;
        }
    z->lost_none++;
}
internal void squadrons_install(const char *autotest_ini) {   // fleet.c fleet_install
    g_sq_shore = g_set_ai[1].sea_shore_fire;
    if (autotest_ini) g_v_sq_shore = ini_int("autotest", "v_sea_shore_fire", -1, autotest_ini);
}
internal void squadrons_new_game(void) { memset(g_sq, 0, sizeof g_sq); memset(g_sq_sup, 0, sizeof g_sq_sup); memset(g_sq_look, 0, sizeof g_sq_look); }
