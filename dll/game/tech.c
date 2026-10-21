// A TECH TREE NODE'S CHILDREN - the techs an epoch's node grants (FN_EPOCH_UPGRADES walks them). Each child is a tech
// node of its own (vtable, +4 researched, +8 its data). The 2017 update keeps them in the node: a vector of 0x20-byte
// child nodes at +0x24 .. +0x28. Art of Conquest keeps pointers to them in a map, its head at +0x28 (the runtime
// library's tree: node +0 left, +4 parent, +8 right, the child +0x10, +0x15 the head's mark).
// for (struct tech_kids k = tech_kids(node); (c = tech_kid_next(&k)); ) ...
// Included by eeaimod.c.

struct tech_kids { uint8_t *pos, *end; int tree; };

static struct tech_kids tech_kids(ee_tech *node) {
    struct tech_kids k = { NULL, NULL, build_is("aoc") };
    if (k.tree) {
        k.end = FIELD(node, OF(TECHNODE, 0x28), uint8_t *);
        k.pos = k.end ? FIELD(k.end, 0, uint8_t *) : NULL;   // the head's left: the first entry
    } else {
        k.pos = FIELD(node, OF(TECHNODE, 0x24), uint8_t *);
        k.end = FIELD(node, OF(TECHNODE, 0x28), uint8_t *);
    }
    return k;
}

static ee_tech *tech_kid_next(struct tech_kids *k) {   // the next child, NULL: none left
    if (!k->pos || k->pos == k->end) return NULL;
    uint8_t *n = k->pos;
    if (!k->tree) { k->pos += 0x20; return (ee_tech *)n; }
    uint8_t *r = FIELD(n, 8, uint8_t *);   // the in-order successor: the leftmost below the right, else up
    if (!r[0x15]) {
        while (!FIELD(r, 0, uint8_t *)[0x15]) r = FIELD(r, 0, uint8_t *);
        k->pos = r;
    } else {
        uint8_t *c = n, *p = FIELD(n, 4, uint8_t *);
        while (!p[0x15] && c == FIELD(p, 8, uint8_t *)) { c = p; p = FIELD(p, 4, uint8_t *); }
        k->pos = p;
    }
    return FIELD(n, 0x10, ee_tech *);
}
