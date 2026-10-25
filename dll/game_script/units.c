// UNITS IN THE LANGUAGE (lang/unit.cs: the kind's members; relations.c x_obj_member_of hands them here): a unit as a
// value is its id (xval obj: the id + 1, okind XK_UNIT) - a handle, not a pointer: it stays a value when the unit is
// destroyed, and the unit is looked up again (unit_by_id) whenever it is read, so a script keeps no pointer into the
// game across a wait. Alive: on the map with hit points left; a gone unit's members are a mistake but for id and alive.
// A unit p.spawn returned before the game made it is a handle to come (obj -(its slot + 1): SPAWNING below).

// SCRIPTS' EFFECTS ON UNITS (p.spawn, u.kill(), u.remove(), u.owner = p, u.hp = n ...): done as the scenario's effects
// do them - Create (game/api/unit.c unit_create), Kill / Remove, Owner, Unit-Attribute (docs/triggers.md) - in the
// simulation on every PC alike, no message, at the next point between the simulation's steps (x_units_flush from
// relations.c x_effects_flush, where the scripts' wins are carried out and the game's own triggers run), in the order
// they were asked for. The script goes on at once: p.spawn returns handles to come (obj -(its slot + 1)), spawns and
// effects one after another are done together. It holds - waits for that point - only when it reads a unit (a member,
// ==) or asks the map what is on it (p.units(), game.unit(id)) while one is still to be done. A trigger runs at that
// point already: its effects are done at once. A unit given to another player is made anew with another id: its
// handles follow it (g_xmoved). A dry run does none: its spawned handles can't be read.
#define X_SPAWN_SLOTS 8192
enum { XS_COMING, XS_THERE, XS_NEVER, XS_DRY };
internal struct { int x, y, type, pl; unsigned id; unsigned char state; } g_xslot[X_SPAWN_SLOTS];
internal int g_xslot_n;               // slots used
internal int g_x_safe;                // 1 while the triggers run (at that point: test/scripts.c script_triggers)
enum { XJ_SPAWN, XJ_KILL, XJ_REMOVE, XJ_OWNER, XJ_HP, XJ_AMMO, XJ_POWER };
struct x_job { int op, obj, v; };     // the unit (its handle: xval obj), the value (the owner's index, hit points ...)
internal struct x_job *g_xjob;
internal int g_xjob_n, g_xjob_cap;    // still to be done
internal struct { unsigned from, to; } *g_xmoved;   // a unit given away: its id before, after
internal int g_xmoved_n, g_xmoved_cap;
internal int x_id_order(const void *a, const void *b) { unsigned x = *(const unsigned *)a, y = *(const unsigned *)b; return x < y ? -1 : x > y; }
internal unsigned x_moved(unsigned id) {   // the id it has now (given away, perhaps more than once)
    for (int i = 0; i < g_xmoved_n; i++)
        if (g_xmoved[i].from == id) id = g_xmoved[i].to;
    return id;
}
// a handle's state as it is (no waiting): there (*id, as it is now), or why it is none
internal int x_unit_peek(const struct xval *o, unsigned *id) {
    if (o->obj > 0) { *id = x_moved((unsigned)(o->obj - 1)); return XS_THERE; }
    int k = -o->obj - 1;
    *id = g_xslot[k].state == XS_THERE ? x_moved(g_xslot[k].id) : 0;
    return g_xslot[k].state;
}
internal ee_unit *x_unit_live(unsigned id) {   // the unit of an id while it is on the map with hit points left
    ee_unit *u = unit_by_id(id);
    return u && unit_get_type_data(u) && unit_get_hp(u) > 0 ? u : NULL;
}
internal void x_job_do(const struct x_job *j) {   // one effect done (a unit gone meanwhile: nothing)
    unsigned id;
    if (j->op == XJ_SPAWN) {   // slot k's unit made - or never (its owner out, the type made none)
        int k = -j->obj - 1;
        ee_unit_type *t = type_data(g_xslot[k].type);
        void *p = player_at(g_xslot[k].pl);
        ee_unit *u = t && p && !is_out(p) ? unit_create(t, g_xslot[k].x, g_xslot[k].y, g_xslot[k].pl) : NULL;
        g_xslot[k].state = u ? XS_THERE : XS_NEVER;
        if (u) g_xslot[k].id = (unsigned)unit_get_serial(u);
        return;
    }
    struct xval o = { 0 };
    o.obj = j->obj;
    ee_unit *u = x_unit_peek(&o, &id) == XS_THERE ? x_unit_live(id) : NULL;
    if (!u) return;
    if (j->op == XJ_KILL || j->op == XJ_REMOVE) {   // as the scenario's Kill / Remove (0x5AF26F)
        unit_destroy(u, j->op == XJ_KILL, 0, 0);
        if (!unit_get_list_node(u)) player_drop(unit_get_owner(u), u, 0);
    } else if (j->op == XJ_OWNER) {   // as its Owner (0x5ACC11) - the new unit's id given at once (the player's adopt)
        ee_player *to = game_players()[j->v];
        if (!to || unit_get_owner(u) == to) return;
        ee_unit *n = unit_give(u, j->v, 0);
        if (!n) return;
        player_adopt(to, 0, 1);
        if (g_xmoved_n == g_xmoved_cap) {
            void *m = realloc(g_xmoved, (size_t)(g_xmoved_cap = g_xmoved_cap ? g_xmoved_cap * 2 : 64) * sizeof *g_xmoved);
            if (!m) return;
            g_xmoved = m;
        }
        g_xmoved[g_xmoved_n].from = id, g_xmoved[g_xmoved_n++].to = (unsigned)unit_get_serial(n);
    } else   // as its Unit-Attribute, exactly (0x5B8823: hit points 0 or less and the unit vanishes)
        unit_attr_set(j->op == XJ_HP ? SM_UA_HITPOINTS : j->op == XJ_AMMO ? SM_UA_AMMUNITION : SM_UA_POWER, 0, u, j->v, 0, 0, SM_EXACTLY);
}
internal void x_units_flush(void) {   // the effects to be done, in the order they were asked for
    for (int i = 0; i < g_xjob_n; i++) x_job_do(&g_xjob[i]);
    g_xjob_n = 0;
}
// an effect asked for: done at once at the safe point (a trigger's), else at the next
internal void x_job(int op, int obj, int v) {
    struct x_job j = { op, obj, v };
    if (g_x_safe) { x_job_do(&j); return; }
    if (g_xjob_n == g_xjob_cap) {
        void *m = realloc(g_xjob, (size_t)(g_xjob_cap = g_xjob_cap ? g_xjob_cap * 2 : 256) * sizeof *g_xjob);
        if (!m) return;
        g_xjob = m;
    }
    g_xjob[g_xjob_n++] = j;
}
// the effects to be done done: a script's turn waits for the safe point (the game's next step), a trigger can't wait
internal void x_units_settle(int wait) {
    while (g_xjob_n > 0 && wait && g_x_hooks.wait) g_x_hooks.wait((g_x_hooks.now ? g_x_hooks.now() : game_time() / 1000.0) + 0.001);
}
// a handle's state when it is read: the effects asked for done first (where waiting may be), then as x_unit_peek
internal int x_unit_state(const struct xval *o, unsigned *id) {
    if (g_xjob_n) x_units_settle(1);
    return x_unit_peek(o, id);
}
internal int x_unit_key(const struct xval *v) {   // == of units: the id of one that came (expr.c xcmp, host.h obj_key)
    unsigned id;
    if (v->okind != XK_UNIT) return v->obj;
    return x_unit_state(v, &id) == XS_THERE ? (int)id + 1 : v->obj;
}

// a unit as a value: its type's identifier in a text, its id
internal void x_unit(struct xval *v, ee_unit *u) {
    xstr(v, unit_ident(unit_type_get_id(unit_get_type_data(u))));
    v->obj = unit_get_serial(u) + 1;
    v->okind = XK_UNIT;
}
// the unit a value holds while it is on the map, NULL gone (*why: what is told then)
internal ee_unit *x_unit_of(const struct xval *o, const char **why) {
    unsigned id;
    int st = x_unit_state(o, &id);
    ee_unit *u = st == XS_THERE ? x_unit_live(id) : NULL;
    *why = st == XS_COMING ? "not on the map yet - a spawned unit is made at the next step" :
           st == XS_NEVER ? "the spawned unit was never made (its owner out of the game?)" :
           st == XS_DRY ? "a dry run makes no units - a spawned one can't be read" : "the unit is gone (destroyed) - check u.alive first";
    return u;
}
// u.<member> (a member lang/unit.cs declares - expr.c checked the name)
internal int x_unit_member_of(struct xctx *c, const struct xval *o, const char *name, struct xval *v) {
    const char *why;
    ee_unit *u = x_unit_of(o, &why);
    unsigned id;
    if (!strcmp(name, "alive")) { xnum(v, u != NULL); return 1; }
    if (!strcmp(name, "id") && x_unit_state(o, &id) == XS_THERE) { xnum(v, id); return 1; }
    if (!u) { x_host_fail(c, "%s.%s: %s", xtext_of(o), name, why); return 1; }
    int type = unit_type_get_id(unit_get_type_data(u));
    if (!strcmp(name, "type")) xdual(v, type, (unsigned)type < G_TYPES_N ? g_unit_ident[type] : "?");   // as unit.<identifier>
    else if (!strcmp(name, "owner")) x_player(v, unit_get_owner(u), "owner");
    else if (!strcmp(name, "pos")) { vec3 p = unit_get_pos(u); double xy[2] = { p.x, p.y }; xvec(v, xy, 2); }
    else if (!strcmp(name, "hp")) xnum(v, unit_get_hp(u));
    else if (!strcmp(name, "max_hp")) { ee_unit_stats *st = unit_get_stats(u); xnum(v, st ? unit_stats_get_hp(st) : 0); }
    else if (!strcmp(name, "ammo")) xnum(v, unit_get_ammo(u));
    else if (!strcmp(name, "power")) xnum(v, unit_get_power(u));
    else return 0;
    return 1;
}
// a unit in what an effect tells (the log, a dry run's commands): by its id, a spawned one to come by its slot
internal const char *x_unit_said(const struct xval *o, char *buf, int cap) {
    if (o->obj > 0) snprintf(buf, (size_t)cap, "unit %d", o->obj - 1);
    else snprintf(buf, (size_t)cap, "spawned unit %d", -o->obj);
    return buf;
}
// may an effect be asked for on it: a mistake when it is gone already (one to come may be)
internal int x_unit_effect_ok(struct xctx *c, const struct xval *o, const char *what) {
    unsigned id;
    int st = x_unit_peek(o, &id);
    if (st == XS_COMING || st == XS_DRY || (st == XS_THERE && x_unit_live(id))) return 1;
    x_host_fail(c, "%s: %s", what, st == XS_NEVER ? "the spawned unit was never made" : "the unit is gone (destroyed) - check u.alive first");
    return 0;
}
// u.kill(), u.remove() (lang/unit.cs's effects): 1 one of them (done, told, or a mistake told)
internal int x_unit_do(struct xctx *c, const struct xval *o, const char *name, struct xval *v) {
    char t[80], u[40];
    if (strcmp(name, "kill") && strcmp(name, "remove")) return 0;
    xnothing(v);
    snprintf(t, sizeof t, "%s.%s()", x_unit_said(o, u, sizeof u), name);
    if (x_unit_effect_ok(c, o, t) && !x_did(c, t)) x_job(name[0] == 'k' ? XJ_KILL : XJ_REMOVE, o->obj, 0);
    return 1;
}
// u.owner = p, u.hp = n, u.ammo = n, u.power = n (lang/unit.cs's [set] members): 1 done (or told), 0 a mistake (told)
internal int x_unit_set(struct xctx *c, const struct xval *o, const char *name, const struct xval *v) {
    char t[100], u[40];
    int op = !strcmp(name, "owner") ? XJ_OWNER : !strcmp(name, "hp") ? XJ_HP : !strcmp(name, "ammo") ? XJ_AMMO : !strcmp(name, "power") ? XJ_POWER : -1, n;
    x_unit_said(o, u, sizeof u);
    if (op < 0) { x_host_fail(c, "%s.%s can't be set", u, name); return 0; }
    if (op == XJ_OWNER) {
        void *q = x_player_of(v);
        if (!q || is_out(q)) { x_host_fail(c, "%s.owner = %.40s: %s", u, xtext_of(v), q ? "not in the game" : "not a player"); return 0; }
        n = pidx(q);
        snprintf(t, sizeof t, "%s.owner = player %d", u, n);
    } else {
        if (!v->has_n || v->ln || v->kind || !(v->n >= 0 && v->n <= 1e9) || v->n != (double)(long)v->n) {
            x_host_fail(c, "%s.%s = %.40s: a whole number, 0 or more - round(...)", u, name, xtext_of(v));
            return 0;
        }
        n = (int)v->n;
        snprintf(t, sizeof t, "%s.%s = %d", u, name, n);
    }
    if (!x_unit_effect_ok(c, o, t)) return 0;
    if (!x_did(c, t)) x_job(op, o->obj, n);
    return c->script;
}
// player pl's units on the map now (p.units(filter) - all without one), by id: a list the same on every PC
struct x_units_scan { void *owner; const unsigned char *sel; unsigned *ids; int n, cap; };
internal void x_units_add(ee_unit *u, void *ctx) {
    struct x_units_scan *s = ctx;
    if (unit_get_owner(u) != s->owner || unit_get_hp(u) <= 0) return;
    int t = unit_type_get_id(unit_get_type_data(u));
    if (s->sel && ((unsigned)t >= G_TYPES_N || !s->sel[t])) return;
    if (s->n == s->cap) {
        unsigned *m = realloc(s->ids, (size_t)(s->cap = s->cap ? s->cap * 2 : 256) * sizeof *m);
        if (!m) return;
        s->ids = m;
    }
    s->ids[s->n++] = (unsigned)unit_get_serial(u);
}
internal int x_units_of(struct xctx *c, int pl, const struct xval *a, int na, struct xval *v) {
    static unsigned char sel[G_TYPES_N];
    struct x_units_scan s = { player_at(pl), NULL, NULL, 0, 0 };
    if (na) {
        memset(sel, 0, sizeof sel);
        if (!x_type_filter("units", &a[0], sel)) { c->failed++; c->missed = 1; xnothing(v); return 1; }
        s.sel = sel;
    }
    x_units_settle(c->script);   // the spawns and effects asked for done
    grid_each(x_units_add, &s);
    qsort(s.ids, (size_t)s.n, sizeof *s.ids, x_id_order);
    struct xval *it = s.n ? x_alloc((size_t)s.n * sizeof *it) : NULL;
    xnothing(v);
    for (int i = 0; it && i < s.n; i++) x_unit(&it[i], unit_by_id(s.ids[i]));
    if (it) { v->it = it; v->ln = v->w = s.n; }
    free(s.ids);
    return 1;
}
// game.unit(id): the unit while it is on the map, else nothing (null)
internal void x_unit_by_id(struct xctx *c, const struct xval *id, struct xval *v) {
    double n = xto_num(id);
    const char *why;
    xnothing(v);
    if (!id->has_n || n != (double)(unsigned)n) { x_host_fail(c, "game.unit(%.40s): an id - u.id", xtext_of(id)); return; }
    x_units_settle(c->script);
    struct xval t = { 0 };
    t.obj = (int)((unsigned)n + 1);
    ee_unit *u = x_unit_of(&t, &why);
    if (u) x_unit(v, u);
}
// p.spawn(type, at, count): the units asked for, the handles to come returned (a trigger's units made at once)
internal void x_units_spawn(struct xctx *c, int pl, const struct xval *a, int na, struct xval *v) {
    int type = a[0].has_n ? (int)a[0].n : -1, n = na > 2 ? (int)xto_num(&a[2]) : 1;
    char t[120];
    xnothing(v);
    if (g_x_setup) { x_host_fail(c, "player %d.spawn: not before the game starts", pl); return; }
    if (type <= 0 || type >= G_TYPES_N || !a[0].has_s || strcmp(a[0].s, g_unit_ident[type]) || a[0].ln) {
        x_host_fail(c, "player %d.spawn(%.40s, ...): one type - unit.<identifier>", pl, xtext_of(&a[0]));
        return;
    }
    if (a[1].kind != XV_VEC || a[1].vn != 2) { x_host_fail(c, "player %d.spawn: its place a vec2 - vec(x, y) in tiles", pl); return; }
    if (na > 2 && (!a[2].has_n || a[2].n != (double)n || n < 1 || n > 100)) { x_host_fail(c, "player %d.spawn: 1 to 100 units", pl); return; }
    ee_map *map = game_map();
    if (!map || !type_data(type)) { x_host_fail(c, "player %d.spawn: no game, or no type %d", pl, type); return; }
    if (g_xslot_n + n > X_SPAWN_SLOTS) { x_host_fail(c, "player %d.spawn: more than %d spawned units in this game", pl, X_SPAWN_SLOTS); return; }
    int side = 1, first = g_xslot_n;
    while (side * side < n) side++;
    for (int i = 0; i < n; i++) {   // as the spawn command lays them out: a square, 2 tiles apart
        int x = (int)a[1].vc[0] + (i % side) * 2, y = (int)a[1].vc[1] + (i / side) * 2;
        float xy[2] = { (float)x + 0.5f, (float)y + 0.5f };
        if (!spawn_on_map(map, xy)) { x_host_fail(c, "player %d.spawn: %d,%d is off the map's land diamond", pl, x, y); return; }
        g_xslot[first + i].x = x, g_xslot[first + i].y = y, g_xslot[first + i].type = type, g_xslot[first + i].pl = pl;
    }
    snprintf(t, sizeof t, "player %d.spawn(unit.%s, vec(%g, %g), %d)", pl, g_unit_ident[type], a[1].vc[0], a[1].vc[1], n);
    int dry = x_did(c, t);
    struct xval *it = x_alloc((size_t)n * sizeof *it);
    if (!it) return;
    g_xslot_n += n;
    for (int i = 0; i < n; i++) {
        g_xslot[first + i].state = dry ? XS_DRY : XS_COMING;
        if (!dry) x_job(XJ_SPAWN, -(first + i + 1), 0);
        xstr(&it[i], g_unit_ident[type]);
        it[i].obj = -(first + i + 1);
        it[i].okind = XK_UNIT;
    }
    v->it = it;
    v->ln = v->w = n;
}
