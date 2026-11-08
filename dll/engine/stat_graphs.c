// THE STATISTICS GRAPHS OF A LONG GAME (always; the player, 2026-11-08: "exiting to menu is still very slow"). The way out
// of a game builds the statistics screen, and its graphs took 21 s of a 770-minute save, the frames standing. The engine's
// AddDataPoint sorts the line and goes over all of it for each point; PruneGraph and CombineGraph look through whole lines
// for each point, and add their results by AddDataPoint: the work grows with the square of the game's length and more.
// The same results here in one pass over each line, whose points are in order of x (AddDataPoint keeps them so). A line
// that isn't, which no way into a line makes, goes to the game's own code. Nothing of this reaches the simulation.

internal graph_add_point_fn g_sg_add_point;
internal graph_prune_fn g_sg_prune;
internal graph_combine_fn g_sg_combine;

internal int sg_in_order(const ee_graph_series *s) {
    ee_graph_point *p = graph_series_get_points_begin(s);
    size_t n = graph_series_points(s);
    for (size_t i = 1; i < n; i++)
        if (graph_point_get_x(graph_point_at(p, i - 1)) > graph_point_get_x(graph_point_at(p, i))) return 0;
    return 1;
}
internal void sg_series_extremes(ee_graph_series *s) {   // RecomputeMinMaxValues
    int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
    ee_graph_point *p = graph_series_get_points_begin(s);
    size_t n = graph_series_points(s);
    for (size_t i = 0; i < n; i++) {
        int x = graph_point_get_x(graph_point_at(p, i)), y = graph_point_get_y(graph_point_at(p, i));
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }
    graph_series_set_min_x(s, min_x), graph_series_set_max_x(s, max_x), graph_series_set_min_y(s, min_y), graph_series_set_max_y(s, max_y);
}
internal void sg_graph_extremes(ee_graph *g) {           // RecomputeMinMaxValues
    int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
    for (size_t i = 0, n = graph_series_count(g); i < n; i++) {
        ee_graph_series *s = graph_series_at(g, i);
        if (graph_series_get_min_x(s) < min_x) min_x = graph_series_get_min_x(s);
        if (graph_series_get_max_x(s) > max_x) max_x = graph_series_get_max_x(s);
        if (graph_series_get_min_y(s) < min_y) min_y = graph_series_get_min_y(s);
        if (graph_series_get_max_y(s) > max_y) max_y = graph_series_get_max_y(s);
    }
    graph_set_min_x(g, min_x), graph_set_max_x(g, max_x), graph_set_min_y(g, min_y), graph_set_max_y(g, max_y);
}

// A point past the line's last: appended where there is room, the sort has nothing to do and the extremes only widen.
internal void __thiscall hook_sg_add_point(ee_graph_series *s, const ee_graph_point *p) {
    ee_graph_point *begin = graph_series_get_points_begin(s), *end = graph_series_get_points_end(s);
    int x = graph_point_get_x(p), y = graph_point_get_y(p);
    if (!begin || end == graph_series_get_points_cap(s) ||
        (end != begin && graph_point_get_x(graph_point_at(end, (size_t)-1)) >= x)) {
        g_sg_add_point(s, p);
        return;
    }
    graph_point_set_x(end, x), graph_point_set_y(end, y);
    graph_series_set_points_end(s, graph_point_at(end, 1));
    if (x < graph_series_get_min_x(s)) graph_series_set_min_x(s, x);
    if (x > graph_series_get_max_x(s)) graph_series_set_max_x(s, x);
    if (y < graph_series_get_min_y(s)) graph_series_set_min_y(s, y);
    if (y > graph_series_get_max_y(s)) graph_series_set_max_y(s, y);
}

// Of the points of one x the one of the highest y stays.
internal void __thiscall hook_sg_prune(ee_graph *g) {
    size_t count = graph_series_count(g);
    for (size_t l = 0; l < count; l++)
        if (!sg_in_order(graph_series_at(g, l))) { g_sg_prune(g); return; }
    for (size_t l = 0; l < count; l++) {
        ee_graph_series *s = graph_series_at(g, l);
        ee_graph_point *p = graph_series_get_points_begin(s);
        size_t n = graph_series_points(s), kept = 0;
        for (size_t i = 0; i < n; kept++) {
            int x = graph_point_get_x(graph_point_at(p, i)), y = graph_point_get_y(graph_point_at(p, i));
            for (i++; i < n && graph_point_get_x(graph_point_at(p, i)) == x; i++)
                if (graph_point_get_y(graph_point_at(p, i)) > y) y = graph_point_get_y(graph_point_at(p, i));
            graph_point_set_x(graph_point_at(p, kept), x), graph_point_set_y(graph_point_at(p, kept), y);
        }
        if (p) graph_series_set_points_end(s, graph_point_at(p, kept));
        sg_series_extremes(s);
    }
    sg_graph_extremes(g);
}

// What the engine's lookup of a line at x gives (0x100625E6: 0 before the line): for x from a point up to the next, the
// NEXT point's y, past the last the last's. *at: where the walk stands, x only rising from call to call.
internal int sg_value_at(const ee_graph_series *s, int x, size_t *at) {
    ee_graph_point *p = graph_series_get_points_begin(s);
    size_t n = graph_series_points(s);
    if (!n || x < graph_series_get_min_x(s)) return 0;
    while (*at + 1 < n && x >= graph_point_get_x(graph_point_at(p, *at + 1))) ++*at;
    return graph_point_get_y(graph_point_at(p, *at + 1 < n ? *at + 1 : n - 1));
}

// A new line in out for each pair of lines: at every x of either the two lines' values added.
internal void __thiscall hook_sg_combine(ee_graph *g, const ee_graph *other, ee_graph *out) {
    size_t count = graph_series_count(g), most = 0;
    if (graph_series_count(other) < count) count = graph_series_count(other);
    for (size_t l = 0; l < count; l++) {
        const ee_graph_series *a = graph_series_at(g, l), *b = graph_series_at(other, l);
        if (!sg_in_order(a) || !sg_in_order(b)) { g_sg_combine(g, other, out); return; }
        if (graph_series_points(a) + graph_series_points(b) > most) most = graph_series_points(a) + graph_series_points(b);
    }
    ee_graph_point *sum = graph_points_new(most + 1);
    ee_graph_series *line = graph_series_new();
    if (!sum || !line) { free(sum), free(line); g_sg_combine(g, other, out); return; }
    for (size_t l = 0; l < count; l++) {
        const ee_graph_series *a = graph_series_at(g, l), *b = graph_series_at(other, l);
        ee_graph_point *pa = graph_series_get_points_begin(a), *pb = graph_series_get_points_begin(b);
        size_t na = graph_series_points(a), nb = graph_series_points(b), ia = 0, ib = 0, ata = 0, atb = 0, n = 0;
        while (ia < na || ib < nb) {
            int xa = ia < na ? graph_point_get_x(graph_point_at(pa, ia)) : INT_MAX, xb = ib < nb ? graph_point_get_x(graph_point_at(pb, ib)) : INT_MAX;
            int x = ia < na && (ib >= nb || xa <= xb) ? xa : xb;
            while (ia < na && graph_point_get_x(graph_point_at(pa, ia)) == x) ia++;
            while (ib < nb && graph_point_get_x(graph_point_at(pb, ib)) == x) ib++;
            graph_point_set_x(graph_point_at(sum, n), x);
            graph_point_set_y(graph_point_at(sum, n++), (int)((unsigned)sg_value_at(a, x, &ata) + (unsigned)sg_value_at(b, x, &atb)));
        }
        // the line only lends its points to the copy graph_add_series makes: nothing of the engine's holds them, nothing frees them
        graph_series_construct(line);
        graph_series_set_points_begin(line, n ? sum : NULL);
        graph_series_set_points_end(line, n ? graph_point_at(sum, n) : NULL);
        graph_series_set_points_cap(line, n ? graph_point_at(sum, n) : NULL);
        sg_series_extremes(line);
        graph_add_series(out, line);
    }
    free(sum), free(line);
    sg_graph_extremes(out);
}

internal void stat_graphs_install(void) {
    int a = patch_graph_add_point(hook_sg_add_point, &g_sg_add_point), p = patch_graph_prune(hook_sg_prune, &g_sg_prune),
        c = patch_graph_combine(hook_sg_combine, &g_sg_combine);
    if (g_log && a + p + c != 3) fprintf(g_log, "# stat graphs: only %d of 3 imports hooked\n", a + p + c);
}
