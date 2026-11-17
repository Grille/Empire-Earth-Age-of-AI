// THE AI'S TACTICAL MAP (the overlay's layer "ai" - the pipe: overlay ai, [display] overlay = ai; the player 2026-11-13:
// "mostly for when zoomed far out, draw symbols for the ais towns and armies, draw line to where they are moving, a few
// stats as text etc. an tactical overlay map of what the ai is doing"). Each AI once a second, in its colour, as the
// screen's shapes (base/overlay.c OV_MARK, OV_ARROW, OV_TEXT: their size the screen's, over everything):
// - its base (ms_base): a large square, its name and epoch above it, its population, army and stock below, and when the
//   force goes out next;
// - the force (armies.c, army 0): a diamond at its members' middle (those ashore), its size and health, what it does -
//   at home, out to a target (an arrow there; on the approach march an arrow to the approach's point, a thin one on to
//   the target; a way round through its corners), at a raid; home its station when the force stands away from it;
// - its other armies (army_view.c: the landing's, raid parties, the fleet's forces): a triangle (a circle at sea), what
//   it is and its task, an arrow to its mission's point;
// - its towns (towns.c): a square, hollow while it is being founded, "fwd" for a forward base, its citizens.
// With the game's computer player off there is no force: THE TERRITORIES instead (ao_territories, below).
// Nothing here decides anything: it reads what the parts keep. A layer off costs the test.
#define AO_EVERY 1000
#define AO_TTL 5000
internal int ao_kfmt(char *o, int cap, int v) {   // 950, 9.7k, 14k
    return v < 1000 ? snprintf(o, (size_t)cap, "%d", v) : v < 10000 ? snprintf(o, (size_t)cap, "%.1fk", v / 1000.0) : snprintf(o, (size_t)cap, "%dk", v / 1000);
}
internal vec2 ao_at(ivec2 p) { return v2((float)p.x + 0.5f, (float)p.y + 0.5f); }
internal int ao_has(ivec2 p) { return p.x > 0 || p.y > 0; }
internal vec2 ao_pos(ee_unit *u) { vec3 p = unit_get_pos(u); return v2(p.x, p.y); }   // where it is, not its tile: no jumps from tile to tile
// the anchors (base/overlay.c overlay_anchor: their marks glide between the updates): a unit's its serial, an army's
// AO_ARMY with the player and the army's own number (0 the force)
#define AO_ARMY 0x80000000u
internal uint32_t ao_army_id(unsigned idx, int army) { return AO_ARMY | (uint32_t)idx << 24 | ((uint32_t)army & 0xFFFFFF); }
// THE TERRITORIES (the game's computer player off; the player 2026-11-15: "can you overwork the overlay? maybe moving
// continous arrow from the center of both src and dst territory? with text naming the move reason and units count
// would be one idea to make it more readable"):
// - a territory it holds or has soldiers stationed in: a hollow square at its middle, "T<number> front / backland",
//   its soldiers there of those stationed and its share of the force, its citizens (work.c ITS TERRITORY), and
//   "under attack" with the enemy soldiers seen there;
// - A SEA (the player 2026-11-16: "the ai overview dosnt show wether a water territory is "backland" or "front""): one
//   along land it holds (its waters), one it holds or has warships on - a hollow circle at its middle, "S<number> sea
//   front / backland / not held" (territories.c: held with TV_SHIPS of its warships there and more than twice the
//   enemy's seen there; a front when it borders what nobody of its side holds), its warships and the enemy's, the fish
//   it has seen there and its shore along its land;
// - a MOVE: the soldiers that stand in one territory and are stationed in another, by reason - one arrow from the
//   middle of the one to the middle of the other, wider with more of them, dots running along it (AO_RUN a
//   passage; each dot an anchor of its own, so it glides between the updates), and at its middle the reason and the
//   count: "defend" (enemy soldiers in the territory they go to), "reinforce" (it is short of its share),
//   "siege", "scout";
// - a PUSH: from the middle of the territory it starts in to the building it goes at, "push" and its soldiers.
#define AO_MOVES 96
#define AO_RUN 4000
#define AO_T 4096
enum { AOW_DEFEND, AOW_REINFORCE, AOW_SIEGE, AOW_SCOUT, AOW_PUSH };
internal void ao_move(unsigned idx, vec2 a, vec2 b, int key, int why, int n, uint32_t col, uint32_t now) {
    static const char *nm[] = { "defend", "reinforce", "siege", "scout", "push" };
    float w = 2.0f + (float)n / 4.0f;
    overlay_arrow(a, b, w > 8.0f ? 8.0f : w, why == AOW_SCOUT ? (col & 0xFFFFFF) | 0x90000000u : col);
    vec2 mid = v2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
    overlay_text(mid, 0, 0xFFFFFFFF, "%s %d", nm[why], n);
    for (int j = 0; j < 3; j++) {   // the dots: a third of the way apart, a passage every AO_RUN
        uint32_t t = now + (uint32_t)j * (AO_RUN / 3), cycle = t / AO_RUN;
        float f = (float)(t % AO_RUN) / (float)AO_RUN;
        overlay_anchor(AO_ARMY | (uint32_t)idx << 24 | 0x800000u | ((uint32_t)key & 0x1FFF) << 5 | (uint32_t)j << 3 | (cycle & 7));   // a new passage, a new anchor: no glide back to the start
        overlay_mark(v2(a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f), OVM_CIRCLE, 9, 0xFFFFFFFF);
    }
    overlay_anchor(0);
}
internal void ao_territories(ee_player *pl, unsigned idx, uint32_t col, uint32_t now) {
    static short stn[AO_T], here[AO_T], cits[AO_T];
    static struct { short from, to; uint8_t why; short n; } mv[AO_MOVES];
    struct sn_st *z = &g_sn[idx];
    struct tv_st *s = &g_tv[idx];
    int n = s->n < AO_T ? s->n : AO_T, nmv = 0;
    (void)pl;
    if (!n || s->version != g_ter.version) return;
    memset(stn, 0, (size_t)n * sizeof *stn); memset(here, 0, (size_t)n * sizeof *here); memset(cits, 0, (size_t)n * sizeof *cits);
    for (int k = 0; k < g_wk[idx].nh; k++) { int h = g_wk[idx].home[k].ter; if (h >= 0 && h < n) cits[h]++; }
    for (int i = 0; i < z->n; i++) {
        struct sn_unit *m = &z->m[i];
        if (!cv_alive(m->u) || (int)unit_get_serial(m->u) != m->id || m->ter < 0 || m->ter >= n) continue;   // stations.c knows them by the id
        ivec2 p = unit_get_tile(m->u);
        unsigned cur = ter_near(p.x, p.y);
        if (!m->scout) stn[m->ter]++;
        if (cur >= (unsigned)n) continue;
        if ((int)cur == m->ter) { if (!m->scout) here[cur]++; continue; }
        int pushing = 0;
        for (int k = 0; k < SN_PUSHES; k++) pushing |= z->push[k].on && z->push[k].from == m->ter;
        if (pushing && !m->scout) continue;   // a push's: its own arrow
        int why = m->scout ? AOW_SCOUT : m->pool == SNP_SIEGE ? AOW_SIEGE : s->t[m->ter].foe > 0 ? AOW_DEFEND : AOW_REINFORCE, k = 0;
        while (k < nmv && (mv[k].from != (short)cur || mv[k].to != m->ter || mv[k].why != why)) k++;
        if (k == nmv) { if (nmv == AO_MOVES) continue; mv[nmv].from = (short)cur; mv[nmv].to = m->ter; mv[nmv].why = (uint8_t)why; mv[nmv++].n = 0; }
        mv[k].n++;
    }
    for (int r = 0; r < n; r++) {
        struct tv_terr *t = &s->t[r];
        if (g_ter.t[r].water) {   // A SEA
            if (t->role == TVR_NONE && !t->coast && !t->ships) continue;
            vec2 at = ao_at(g_ter.t[r].c);
            overlay_mark(at, OVM_CIRCLE | OVM_HOLLOW, 16, col);
            overlay_text(at, -1, col, "S%d sea %s", r, t->role == TVR_FRONT ? "front" : t->role == TVR_BACK ? "backland" : "not held");
            overlay_text(at, 0, 0xFFFFFFFF, "%d warships, %d enemy", t->ships, t->eships);
            if (t->fish || t->coast) overlay_text(at, 1, 0xFFE0E0E0, "fish %d food, shore %d", t->fish, t->coast);
            continue;
        }
        if (t->role == TVR_NONE && !stn[r]) continue;
        vec2 at = ao_at(g_ter.t[r].c);
        overlay_mark(at, OVM_SQUARE | OVM_HOLLOW, 16, col);
        overlay_text(at, -1, col, "T%d %s", r, t->role == TVR_FRONT ? "front" : t->role == TVR_BACK ? "backland" : "not held");
        overlay_text(at, 0, 0xFFFFFFFF, "%d of %d soldiers, share %d%%", here[r], stn[r], (g_pr_share[idx][r] + 5) / 10);
        if (cits[r]) overlay_text(at, 1, 0xFFE0E0E0, "%d citizens", cits[r]);
        if (t->foe > 0) overlay_text(at, cits[r] ? 2 : 1, 0xFFFF6060, "under attack: %d enemy soldiers", t->foe);
    }
    for (int k = 0; k < nmv; k++)
        ao_move(idx, ao_at(g_ter.t[mv[k].from].c), ao_at(g_ter.t[mv[k].to].c), mv[k].from * 131 + mv[k].to * 7 + mv[k].why, mv[k].why, mv[k].n, col, now);
    for (int k = 0; k < SN_PUSHES; k++) {
        struct sn_push *pu = &z->push[k];
        if (!pu->on || pu->from < 0 || pu->from >= n) continue;
        overlay_mark(ao_at(pu->at), OVM_DIAMOND | OVM_HOLLOW, 18, col);
        ao_move(idx, ao_at(g_ter.t[pu->from].c), ao_at(pu->at), pu->from * 131 + 5000 + k, AOW_PUSH, stn[pu->from], col, now);
    }
}
internal void ai_overlay_tick(ee_player *pl, uint32_t now) {
    static int layer = -2;
    static uint32_t last[16];
    if (layer == -2) layer = overlay_layer("ai");
    if (!pl || !overlay_on(layer)) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (last[idx] && now >= last[idx] && now - last[idx] < AO_EVERY) return;
    last[idx] = now ? now : 1;
    uint32_t col = overlay_player_argb((int)idx, 255), dim = overlay_player_argb((int)idx, 150);   // its own colour (game/players.c player_rgb)
    static struct av_sum sum;
    char b[5][16];
    overlay_begin(layer, idx, now, AO_TTL);
    // the base and the player's numbers
    ivec2 base;
    struct armies_st *a = &g_armies[idx];
    int force_n = 0;
    if (!is_vanilla(pl)) {
        av_clear(&sum);
        for (int k = 0; k < a->n; k++) if (cv_alive(a->held[k].u)) av_add(&sum, a->held[k].u);
        av_done(&sum);
        force_n = sum.n;
    }
    if (ms_base(pl, &base)) {
        vec2 at = ao_at(base);
        ee_tech_tree *tt = player_get_tech_tree(pl);
        const char *name = player_get_name(pl);
        overlay_mark(at, OVM_SQUARE, 32, col);
        overlay_text(at, -1, col, "P%u %s, epoch %d", idx, name ? name : "", tt ? tech_tree_get_epoch(tt) : 0);
        if (!is_vanilla(pl) && !GAME_AI(idx)) force_n = g_pr[idx].force;   // the territories' (produce.c)
        overlay_text(at, 0, 0xFFFFFFFF, "pop %d/%d, army %d", player_population(pl, 1, 0), unit_limit(pl), force_n);
        const int *st = player_stock_at(pl);
        for (int r = 0; r < 5; r++) ao_kfmt(b[r], (int)sizeof b[r], st[r + 2]);
        overlay_text(at, 1, 0xFFE0E0E0, "F %s W %s S %s G %s I %s", b[0], b[1], b[2], b[3], b[4]);
        uint32_t next = player_get_next_attack(pl);
        if (!is_vanilla(pl) && ar_on(pl, idx) && !a->out && next > now)
            overlay_text(at, 2, 0xFFE0E0E0, "next push in %.1f min", (next - now) / 60000.0);
    }
    if (is_vanilla(pl)) { overlay_end(); return; }
    if (!GAME_AI(idx)) { ao_territories(pl, idx, col, now); overlay_end(); return; }   // THE TERRITORIES: no force, no town plan then
    // the force
    if (ar_on(pl, idx) && sum.n) {
        // each member a dot (out: in its colour, those left home dim), the diamond at the middle the force's judgements
        // take (armies.c ar_side: the plain mean of the members' tiles, out only those out) - between its groups when
        // it is strung out (the player, 2026-11-13: "the marker and where the units are is very off, is that what the
        // ai think their force is?")
        // Out, those it left home (and those trained since, held but not sent) are a group of their own: a hollow
        // diamond at their middle, "held back" (the player, 2026-11-13: "reds main force is wholy unacounted for" - 19
        // of 333 out for half an hour, the 314 at home as faint dots only)
        double sx = 0, sy = 0, bx = 0, by = 0;
        int nm = 0, nb = 0;
        for (int k = 0; k < a->n; k++) {
            ee_unit *u = a->held[k].u;
            if (!cv_alive(u)) continue;
            vec2 p = ao_pos(u);
            int in = !a->out || a->held[k].out;
            overlay_anchor((uint32_t)a->held[k].id);
            overlay_mark(p, OVM_SQUARE, 7, in ? col : (col & 0xFFFFFF) | 0xA0000000u);
            if (in) sx += p.x, sy += p.y, nm++;
            else bx += p.x, by += p.y, nb++;
        }
        vec2 mid = nm ? v2((float)(sx / nm), (float)(sy / nm)) : ao_at(a->home);
        if (a->out && nb) {
            vec2 back = v2((float)(bx / nb), (float)(by / nb));
            overlay_anchor(ao_army_id(idx, 0xFFFFFF));
            overlay_mark(back, OVM_DIAMOND | OVM_HOLLOW, 24, col);
            overlay_text(back, 0, col, "%d of the force held back", nb);
            overlay_text(back, 1, 0xFFE0E0E0, "(not in the push)");
        }
        overlay_anchor(0);
        int hp = sum.hp_full > 0 ? (int)(100.0f * sum.hp / sum.hp_full + 0.5f) : 0;
        int away = !a->out && ao_has(a->home) && nm && (mid.x - a->home.x) * (mid.x - a->home.x) + (mid.y - a->home.y) * (mid.y - a->home.y) > 12.0f * 12.0f;
        int way = a->out && a->stage == 2 && a->nway > 0 && a->way_i < a->nway;   // the way round: its corners still ahead
        vec2 dest = ao_at(a->dest);
        if (a->out) overlay_mark(dest, OVM_DIAMOND | OVM_HOLLOW, 18, col);   // the places it goes to stand still
        else if (away) overlay_mark(ao_at(a->home), OVM_TRIANGLE | OVM_HOLLOW, 14, col);
        if (a->out && a->stage == 1 && ao_has(a->approach)) overlay_arrow(ao_at(a->approach), dest, 2, dim);
        if (way) {
            for (int k = a->way_i; k + 1 < a->nway; k++) overlay_arrow(ao_at(a->way[k]), ao_at(a->way[k + 1]), 2, dim);
            overlay_arrow(ao_at(a->way[a->nway - 1]), dest, 2, dim);
        }
        overlay_anchor(ao_army_id(idx, 0));   // the diamond first (the place its anchor glides by), its text and the arrow from it
        overlay_mark(mid, OVM_DIAMOND, 26, col);
        if (a->out) overlay_arrow(mid, a->stage == 1 && ao_has(a->approach) ? ao_at(a->approach) : way ? ao_at(a->way[a->way_i]) : dest, 4, col);
        else if (away) overlay_arrow(mid, ao_at(a->home), 2, dim);
        if (a->out && nm < sum.n - sum.aboard) overlay_text(mid, 0, col, "force %d of %d out, health %d%%%s", nm, sum.n, hp, sum.siege ? ", siege" : "");
        else overlay_text(mid, 0, col, "force %d, health %d%%%s", sum.n, hp, sum.siege ? ", siege" : "");
        if (a->out == 1)
            overlay_text(mid, 1, 0xFFFFFFFF, "%s %d,%d (odds %.1f)", a->stage == 1 ? "approach to" : a->stage == 2 ? "way round to" : a->stage == 3 ? "breach at" : "push at",
                         a->dest.x, a->dest.y, a->ratio > 99.0f ? 99.0 : (double)a->ratio);
        else if (a->out == 2) overlay_text(mid, 1, 0xFFFFFFFF, "answering a raid at %d,%d", a->dest.x, a->dest.y);
        else overlay_text(mid, 1, 0xFFE0E0E0, "at home");
        {   // the territory it is in and the one it is bound for (territories.c)
            unsigned in = tv_terr_of(iv2((int)mid.x, (int)mid.y)), to = tv_terr_of(a->out ? a->dest : a->home);
            if (in != TER_NONE && to != TER_NONE && in != to) overlay_text(mid, sum.aboard ? 3 : 2, 0xFFE0E0E0, "territory %u -> %u", in, to);
            else if (in != TER_NONE) overlay_text(mid, sum.aboard ? 3 : 2, 0xFFE0E0E0, "territory %u", in);
        }
        if (sum.aboard) overlay_text(mid, 2, 0xFFE0E0E0, "%d aboard", sum.aboard);
        overlay_anchor(0);
    }
    // the other armies
    for (int j = 0; j < g_av[idx].n; j++) {
        struct av_army *o = &g_av[idx].a[j];
        av_clear(&sum);
        double mx = 0, my = 0;
        int na = 0;
        for (int k = 0; k < o->n; k++)
            if (av_live(&o->m[k], pl)) {
                av_add(&sum, o->m[k].u);
                if (unit_get_x(o->m[k].u) < 0) continue;   // aboard
                vec2 p = ao_pos(o->m[k].u);
                overlay_anchor((uint32_t)o->m[k].id);
                overlay_mark(p, OVM_SQUARE, 7, col);
                mx += p.x, my += p.y, na++;
            }
        av_done(&sum);
        overlay_anchor(0);
        if (!na) continue;
        vec2 mid = v2((float)(mx / na), (float)(my / na));
        overlay_anchor(ao_army_id(idx, o->id));
        overlay_mark(mid, o->field == AVF_SEA ? OVM_CIRCLE : OVM_TRIANGLE, 22, col);
        if (ao_has(o->dest)) overlay_arrow(mid, ao_at(o->dest), 3, dim);
        overlay_text(mid, 0, col, "%s %d", o->what ? o->what : k_av_kind[o->kind], sum.n);
        if (o->task) overlay_text(mid, 1, 0xFFE0E0E0, "%.40s", o->task);
        {
            unsigned in = tv_terr_of(iv2((int)mid.x, (int)mid.y)), to = ao_has(o->dest) ? tv_terr_of(o->dest) : TER_NONE;
            if (in != TER_NONE && to != TER_NONE && in != to) overlay_text(mid, 2, 0xFFE0E0E0, "territory %u -> %u", in, to);
            else if (in != TER_NONE) overlay_text(mid, 2, 0xFFE0E0E0, "territory %u", in);
        }
        overlay_anchor(0);
    }
    // the towns
    struct tn_st *tn = &g_tn[idx];
    for (int i = 0; i < tn->n; i++) {
        struct tn_town *t = &tn->town[i];
        if (!t->state) continue;
        vec2 at = ao_at(t->c);
        overlay_mark(at, OVM_SQUARE | (t->state == 1 ? OVM_HOLLOW : 0), 20, col);
        overlay_text(at, 0, col, "%s%s, %d citizens", t->forward ? "forward base" : "town", t->state == 1 ? " (founding)" : "", t->ncit);
    }
    overlay_end();
}
