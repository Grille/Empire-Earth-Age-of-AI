// THE SORT MAPS BUILT AT ONCE (the pipe: picture maps game | own | check; own with the mod's walk - units_smooth). The
// units' update puts every record it makes into one of six std::multimaps by its graphics' key (engine_display.sort_maps:
// the objects', the objects' see-through, the shadows', the same three of the trees) - each a node from the heap and an
// insert down the red-black tree - and empties them at the next update, a node at a time to operator delete. Zoomed out
// over 3000 units the inserts, the trees' frees and the heap under them were a large part of the render thread, even
// paused (docs/renderer.md "The units' draw, the mod's own"). Here the mod's walk pushes its records into plain lists;
// after the walk each list is sorted by its key - stably: the multimap keeps equal keys in the order they came - and
// linked into the game's map as a balanced red-black tree of the game's own nodes (MSVC's _Tree: left, parent, right, the
// key and the record, colour, nil; the head the nil), so what reads the maps - the draw's lists, the circles and bars,
// the ground's shadows - reads them as before. The nodes are the game's (its operator new): the hooks of the maps' clear
// keep them for the next update instead of deleting them, and anything else that frees them can.
#define UM_MAPS 6
#define UM_RED 0
#define UM_BLACK 1
#define UM_SPARE (1u << 18)   // nodes kept for the next update at most (beyond them: operator delete)
typedef struct um_node { struct um_node *left, *parent, *right; uint32_t key; void *rec; uint8_t color, isnil; } um_node;
typedef struct { uint32_t key; void *rec; } um_entry;
internal struct {
    int force;               // the pipe's: 0 the game's inserts, 1 the lists, 2 both (the game's kept) and compared; -1 the lists
    int hooked;              // both clears' twelve calls hooked
    int walking;             // inside the mod's walk (collect_at): its inserts are pushed
    um_entry *e[UM_MAPS], *tmp;
    uint32_t n[UM_MAPS], room[UM_MAPS], tmp_room;
    um_node *spare, **nodes;
    uint32_t nspare, nodes_room;
    LONG builds, unclean, compared, differ, logged;
} g_um = { .force = -1 };
internal int um_mode(void) { return g_um.force >= 0 ? g_um.force : g_um.hooked; }
internal int um_index(const void *map) {
    ptrdiff_t d = (const uint8_t *)map - engine_display_sort_maps_at();
    return d >= 0 && d < UM_MAPS * 0xC && d % 0xC == 0 ? (int)(d / 0xC) : -1;
}
internal um_node *um_head(const void *map) { return *(um_node **)((uint8_t *)map + 4); }
internal uint32_t *um_size(void *map) { return (uint32_t *)((uint8_t *)map + 8); }
internal int um_grow(void **p, uint32_t *room, uint32_t need, size_t each) {
    if (need <= *room) return 1;
    uint32_t r = *room ? *room : 1024;
    while (r < need) r *= 2;
    void *q = realloc(*p, (size_t)r * each);
    if (!q) return 0;
    *p = q, *room = r;
    return 1;
}
// a record into a map: the game's insert, or the walk's list (collect.c's fills call this for every insert)
internal void uc_map_insert(void *map, void **pair) {
    int mode = um_mode(), i = um_index(map);
    int push = g_um.walking && i >= 0 && mode && um_grow((void **)&g_um.e[i], &g_um.room[i], g_um.n[i] + 1, sizeof(um_entry));
    if (!push || mode == 2) {
        void *found[2];
        sort_map_insert(map, found, pair);
        if (!push) return;
    }
    um_entry *e = &g_um.e[i][g_um.n[i]++];
    e->key = (uint32_t)(uintptr_t)pair[0], e->rec = pair[1];
}
// the list in its key's order, equal keys in their order (least significant digit first, 11 bits a pass)
internal void um_sort(um_entry *a, uint32_t n) {
    if (n < 2 || !um_grow((void **)&g_um.tmp, &g_um.tmp_room, n, sizeof(um_entry))) return;
    um_entry *src = a, *dst = g_um.tmp;
    for (int shift = 0; shift < 32; shift += 11) {
        uint32_t cnt[2048];
        memset(cnt, 0, sizeof cnt);
        for (uint32_t k = 0; k < n; k++) cnt[(src[k].key >> shift) & 2047]++;
        if (cnt[(src[0].key >> shift) & 2047] == n) continue;   // one digit for all: nothing to move
        for (uint32_t d = 0, at = 0; d < 2048; d++) { uint32_t c = cnt[d]; cnt[d] = at; at += c; }
        for (uint32_t k = 0; k < n; k++) dst[cnt[(src[k].key >> shift) & 2047]++] = src[k];
        um_entry *t = src; src = dst; dst = t;
    }
    if (src != a) memcpy(a, src, n * sizeof *a);
}
internal um_node *um_take(void) {
    um_node *x = g_um.spare;
    if (x) { g_um.spare = x->left; g_um.nspare--; return x; }
    return game_new(sizeof(um_node));
}
// nodes lo .. hi as a subtree under parent: the middle its root; red on the deepest level (red: its depth), so every
// way down meets as many black nodes and no red node has a red child
internal um_node *um_link(um_node **nd, uint32_t lo, uint32_t hi, int depth, int red, um_node *parent, um_node *head) {
    if (lo >= hi) return head;
    uint32_t mid = lo + (hi - lo) / 2;
    um_node *x = nd[mid];
    x->parent = parent;
    x->left = um_link(nd, lo, mid, depth + 1, red, x, head);
    x->right = um_link(nd, mid + 1, hi, depth + 1, red, x, head);
    x->color = depth == red && depth > 0 ? UM_RED : UM_BLACK;
    x->isnil = 0;
    return x;
}
// a map made of its list (after the walk; the map emptied by the update's clear)
internal void um_build(int i) {
    void *map = engine_display_sort_maps_at() + i * 0xC;
    um_node *head = um_head(map);
    uint32_t n = g_um.n[i], *size = um_size(map);
    if (!n || !head) return;
    if (head->parent != head || *size) {   // something put records in besides the walk: the game's insert for ours too
        InterlockedIncrement(&g_um.unclean);
        for (uint32_t k = 0; k < n; k++) {
            void *found[2], *pair[2] = { (void *)(uintptr_t)g_um.e[i][k].key, g_um.e[i][k].rec };
            sort_map_insert(map, found, pair);
        }
        return;
    }
    if (!um_grow((void **)&g_um.nodes, &g_um.nodes_room, n, sizeof *g_um.nodes)) return;
    um_sort(g_um.e[i], n);
    for (uint32_t k = 0; k < n; k++) {
        um_node *x = um_take();
        if (!x) { while (k--) { g_um.nodes[k]->left = g_um.spare; g_um.spare = g_um.nodes[k]; g_um.nspare++; } return; }
        x->key = g_um.e[i][k].key, x->rec = g_um.e[i][k].rec;
        g_um.nodes[k] = x;
    }
    int levels = 0;
    while (levels < 32 && (1u << levels) <= n) levels++;
    head->parent = um_link(g_um.nodes, 0, n, 0, levels - 1, head, head);
    head->left = g_um.nodes[0];
    head->right = g_um.nodes[n - 1];
    *size = n;
}
// the walk's lists against the maps the game's inserts made of them (picture maps check): the same records in the same order
internal void um_compare(void) {
    for (int i = 0; i < UM_MAPS; i++) {
        void *map = engine_display_sort_maps_at() + i * 0xC;
        um_node *head = um_head(map);
        uint32_t n = g_um.n[i];
        if (!head) continue;
        um_sort(g_um.e[i], n);
        InterlockedIncrement(&g_um.compared);
        uint32_t k = 0, at = (uint32_t)-1;
        for (um_node *x = head->left; x != head && k <= n; k++) {
            if (k < n && at == (uint32_t)-1 && (x->key != g_um.e[i][k].key || x->rec != g_um.e[i][k].rec)) at = k;
            if (!x->right->isnil) { x = x->right; while (!x->left->isnil) x = x->left; }   // the next in order
            else { um_node *p = x->parent; while (p != head && x == p->right) x = p, p = p->parent; x = p; }
        }
        if (k == n && at == (uint32_t)-1 && *um_size(map) == n) continue;
        InterlockedIncrement(&g_um.differ);
        if (g_um.logged++ < 12) log_locked("# maps: map %d differs: %u in the game's (%u counted), %u in the list, first at %u\n", i, *um_size(map), k, n, at);
    }
}
// collect.c collect_at, round the mod's walk
internal void um_walk_begin(void) {
    memset(g_um.n, 0, sizeof g_um.n);
    g_um.walking = 1;
}
internal void um_walk_end(void) {
    g_um.walking = 0;
    int mode = um_mode();
    if (mode == 1) {
        for (int i = 0; i < UM_MAPS; i++) um_build(i);
        InterlockedIncrement(&g_um.builds);
    } else if (mode == 2) um_compare();
}
// a map emptied: its nodes kept for the next update (beyond UM_SPARE given back)
internal void um_clear(void *map) {
    um_node *head = um_head(map), *stack[256];
    if (!head) return;
    int sp = 0;
    if (!head->parent->isnil) stack[sp++] = head->parent;
    while (sp) {
        um_node *x = stack[--sp];
        if (!x->left->isnil && sp < 255) stack[sp++] = x->left;
        if (!x->right->isnil && sp < 255) stack[sp++] = x->right;
        if (g_um.nspare < UM_SPARE) x->left = g_um.spare, g_um.spare = x, g_um.nspare++;
        else game_delete(x);
    }
    head->parent = head->left = head->right = head;
    *um_size(map) = 0;
}
internal void __thiscall um_hook_clear(void *map) { if (um_mode()) um_clear(map); else sort_map_clear(map); }
internal void __thiscall um_hook_clear_trees(void *map) { if (um_mode()) um_clear(map); else sort_map_clear_trees(map); }
// collect.c collect_install
internal void collect_maps_install(void) {
    int of = 0, a = patch_units_maps_clear(um_hook_clear, &of), b = patch_units_trees_clear(um_hook_clear_trees, &of);
    g_um.hooked = a + b == of;
    if (g_log) fprintf(g_log, "# collect: the sort maps %s (their clears hooked %d of %d)\n",
                       g_um.hooked ? "built at once after the mod's walk" : "the game's inserts (exe differs)", a + b, of);
}
// the pipe: picture maps [game | own | check]
internal int um_pipe(char *out, const char *arg) {
    if (!strcmp(arg, "game") || !strcmp(arg, "own") || !strcmp(arg, "check")) {
        if (arg[0] != 'g' && !g_um.hooked) { REPLY("error: the maps' clears aren't hooked\n"); return 1; }
        g_um.force = arg[0] == 'g' ? 0 : arg[0] == 'o' ? 1 : 2;
        if (g_um.force == 2) g_um.compared = g_um.differ = g_um.logged = 0;
        REPLY("the sort maps: %s\n", arg[0] == 'g' ? "the game's inserts" : arg[0] == 'o' ? "built after the walk" : "the game's, compared with the walk's lists");
        return 1;
    }
    if (arg[0]) { REPLY("error: usage: picture maps [game | own | check]\n"); return 1; }
    REPLY("the sort maps: %s; built %ld times (%ld with records of others in them), nodes kept %u; compared %ld maps, %ld differ; the last walk's lists",
          um_mode() == 0 ? "the game's inserts" : um_mode() == 1 ? "built after the walk" : "compared", (long)g_um.builds, (long)g_um.unclean,
          g_um.nspare, (long)g_um.compared, (long)g_um.differ);
    for (int i = 0; i < UM_MAPS; i++) REPLY(" %u", g_um.n[i]);
    REPLY("\n");
    return 1;
}
