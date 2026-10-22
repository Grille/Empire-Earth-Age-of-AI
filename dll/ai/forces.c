// FORCES: what the AI knows of the land and the armies on it - continents (area ids), whether a stored attack
// subgroup is still one of its own, an objective building of the target, an enemy army in its territory, a unit's
// walk and fight. Included by eeaimod.c after defence.c (grid_each, is_soldier) and attack.c (threat_at, seen_by);
// used by landing.c, fleet.c, expand.c, cover.c, mass.c and the test commands. (Was assault.c: the mod-led land
// assault, removed - docs/experiments.md.)

// The continent of a tile: EEAIContinentManager (global object 0x917BC8) 0x425F9F thiscall(mgr, int pt[2], kind) - the
// area at the tile (0x425F16), its +0x84 (kind 1: the ocean) or +0x86 (kind 2: the land continent). Reference save:
// both AIs' towns, the player's base and outposts on land continent 2, the water at P3's docks ocean 1; a few areas
// (the player's camp at 108,75) have neither - taken as passable.
internal unsigned area_id(int x, int y, int kind) {
    if (x < 0 || y < 0) return 0;
    return map_area(iv2(x, y), kind);
}
internal unsigned cont_at(int x, int y) { return area_id(x, y, 2); }
internal int cont_ok(int x, int y, unsigned hc) {   // land of continent hc (or an area without ids that is no ocean)
    unsigned c = area_id(x, y, 2);
    return c == hc || (c == 0 && area_id(x, y, 1) == 0 && x >= 0 && y >= 0);
}

// still one of AI pl's attack subgroups? A stored subgroup pointer can be deleted by the game between two ticks (a
// group finished, an attack called off) - reading its members then crashed (landing test: a freed record list)
internal int asg_live(ee_player *pl, ee_attack_subgroup *sg) {
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? am_get_commander(cbm) : NULL;
    if (!cmd || !sg) return 0;
    uint8_t *dq = ac_groups_at(cmd);
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++)
            if (*sp == sg) return 1;
    }
    return 0;
}

struct obj_scan { ee_player *pl; ee_player *target; unsigned pi, cont; int x, y; ee_unit *best; float score; };
internal void objective_unit(ee_unit *u, void *ctx) {
    struct obj_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    int fam = type_get_family(td);
    if (unit_get_owner(u) != s->target || (fam != 2 && fam != 29) || unit_get_hp(u) <= 0 ||
        is_wall(type_get_id(td)) || !seen_by(u, s->pl))
        return;
    int x = unit_get_x(u), y = unit_get_y(u);
    float dx = (float)(x - s->x), dy = (float)(y - s->y), sc = sqrtf(dx * dx + dy * dy) + 3.0f * threat_at(s->pi, x, y);
    if (s->best && sc >= s->score) return;
    if (!cont_ok(x, y, s->cont)) return;
    s->best = u; s->score = sc;
}
internal ee_unit *enemy_objective(ee_player *pl, unsigned pi, ee_player *target, unsigned cont, int x, int y) {
    struct obj_scan s = { pl, target, pi, cont, x, y, NULL, 0 };
    grid_each(objective_unit, &s);
    return s.best;
}

internal int is_aa(ee_unit *u) {   // anti-air units (AA tank, Stinger, Skywatcher): they stay home (landing.c)
    int t = type_get_id(unit_get_type_data(u));
    return t == UT(AA10_TANK) || t == UT(AA10_STINGER_SOLDIER) || t == UT(AA13_TANK_SKYWATCHER);
}
internal int goal_near(ee_unit *u, float x, float y, float r) {   // walking there already
    ee_goal *g = unit_get_goal(u);
    return g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - x) < r && fabsf(goal_get_dest_y(g) - y) < r;
}
internal int unit_fighting(ee_unit *u) {
    ee_goal *g = unit_get_goal(u); ee_unit *t = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
    return t && unit_get_hp(t) > 0;
}

struct threat_scan { ee_player *pl; int nb, bx[512], by[512]; int ne, ex[512], ey[512]; };
internal void home_threat_unit(ee_unit *u, void *ctx) {
    struct threat_scan *h = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    if (!o || unit_get_hp(u) <= 0) return;
    int fam = type_get_family(td);
    if (o == h->pl) {
        if ((fam == 2 || fam == 29) && !is_wall(type_get_id(td)) && h->nb < 512) { h->bx[h->nb] = unit_get_x(u); h->by[h->nb++] = unit_get_y(u); }
    } else if (player_get_index(o) > 0 && is_enemy(h->pl, o) && (is_soldier(td) || is_siege(type_get_id(td))) &&
               seen_by(u, h->pl) && h->ne < 512) { h->ex[h->ne] = unit_get_x(u); h->ey[h->ne++] = unit_get_y(u); }
}
// enemy soldiers and siege the AI sees within 20 tiles of any of its buildings: an army in its territory
internal int home_threat(ee_player *pl) {
    static struct threat_scan h;
    h.pl = pl; h.nb = h.ne = 0;
    grid_each(home_threat_unit, &h);
    int n = 0;
    for (int e = 0; e < h.ne; e++)
        for (int b = 0; b < h.nb; b++) {
            int dx = h.ex[e] - h.bx[b], dy = h.ey[e] - h.by[b];
            if (dx * dx + dy * dy <= 400) { n++; break; }
        }
    return n;
}
