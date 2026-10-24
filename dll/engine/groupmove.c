// BIG GROUPS AS A BLOCK (`group_block`; docs/engine-notes.md "Group moves"): a group move puts each unit on a slot of
// the group's formation, and every row and column of it is spaced by the largest unit's collision size (type +0x470) -
// one tank stretches the grid of 170 soldiers - in as few as 5 columns, the rows stacked behind the clicked point: a
// 174-unit army sent 20 tiles ended with half of it more than 28 tiles from the click (tests/probes/army_move.eeai).
// For a group of more than group_block units the slots are laid out again as one block centred on the clicked point,
// GB_WIDE times as wide as deep, facing the way from the group to the point, each slot as wide as the units' mean
// size (the root of the mean of the squares) by GB_SPACE. The game then pairs units and slots as before, each unit
// gets its own spot and arrives there; the collision sorts out the rest. Only + - * / and sqrtf: the same on every PC.

#define GB_SPACE 1.25f   // a slot's side: the units' mean size times this
#define GB_WIDE 2        // the block this many times as wide as deep

internal int g_gb_min;              // group_block: 0 off, else groups of more units than this
internal unsigned g_gb_moves;       // group moves laid out as a block

internal void __thiscall hook_formation_slots(void *f, void *group, void *slots) {
    formation_slots(f, group, slots);
    int n = formation_slot_count(slots);
    if (!g_gb_min || n <= g_gb_min) return;
    vec3 c = formation_get_center(f);
    float s2 = 0, mx = 0, my = 0;
    int m = 0;
    for (int i = 0; i < n; i++) {
        ee_unit *u = fslot_get_unit(formation_slot(slots, i));
        if (!u) continue;
        float s = unit_get_size(u);
        vec3 p = unit_get_pos(u);
        s2 += s * s;
        mx += p.x;
        my += p.y;
        m++;
    }
    if (!m) return;
    float sp = sqrtf(s2 / (float)m) * GB_SPACE;
    if (sp < 0.3f) sp = 0.3f;
    vec2 fwd = v2(c.x - mx / (float)m, c.y - my / (float)m);
    float len = vec2_len(fwd);
    fwd = len > 0.5f ? vec2_scale(fwd, 1.0f / len) : v2(0, 1);
    vec2 right = v2(fwd.y, -fwd.x);
    int cols = 1;
    while (cols * cols < n * GB_WIDE) cols++;
    if (cols > n) cols = n;
    int rows = (n + cols - 1) / cols;
    for (int i = 0; i < n; i++) {   // front row first, the last row centred
        int r = i / cols, k = i % cols, in_row = r == rows - 1 ? n - r * cols : cols;
        float x = ((float)k - (float)(in_row - 1) * 0.5f) * sp, y = ((float)(rows - 1) * 0.5f - (float)r) * sp;
        void *s = formation_slot(slots, i);
        vec3 p = fslot_get_pos(s);
        p.x = c.x + right.x * x + fwd.x * y;
        p.y = c.y + right.y * x + fwd.y * y;
        fslot_set_pos(s, p);
    }
    g_gb_moves++;
}

internal void groupmove_install(void) {
    g_gb_min = g_set.group_block;
    int n = g_gb_min ? patch_formation_slots(hook_formation_slots) : 0;
    if (g_log) fprintf(g_log, "# groupmove: group_block=%d (%d formation vtables)\n", g_gb_min, n);
}
