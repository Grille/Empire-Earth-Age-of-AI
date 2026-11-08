// THE STATISTICS SCREEN'S GRAPHS (Low-Level Engine.dll: UGraph, UGraphSeries): functions, hooks.

// FIELDS
// a graph's line i, a line's point i: the vectors' elements by the game's sizes
internal inline size_t graph_series_count(const ee_graph *g) { return graph_get_series_begin(g) ? (size_t)((uint8_t *)graph_get_series_end(g) - (uint8_t *)graph_get_series_begin(g)) / size_u_graph_series : 0; }
internal inline ee_graph_series *graph_series_at(const ee_graph *g, size_t i) { return (ee_graph_series *)((uint8_t *)graph_get_series_begin(g) + i * size_u_graph_series); }
internal inline size_t graph_series_points(const ee_graph_series *s) { return graph_series_get_points_begin(s) ? (size_t)((uint8_t *)graph_series_get_points_end(s) - (uint8_t *)graph_series_get_points_begin(s)) / size_u2_d_point : 0; }
internal inline ee_graph_point *graph_point_at(ee_graph_point *p, size_t i) { return (ee_graph_point *)((uint8_t *)p + i * size_u2_d_point); }

// FUNCTIONS
// memory of the mod's own for n points, for a line (free() both)
internal inline ee_graph_point *graph_points_new(size_t n) { return malloc(n * size_u2_d_point); }
internal inline ee_graph_series *graph_series_new(void) { return malloc(size_u_graph_series); }
// a new line in that memory: no points, white, shown; it allocates nothing
internal inline void graph_series_construct(ee_graph_series *s) { (*(void (__thiscall **)(ee_graph_series *))data_graph_iat_series_ctor)(s); }
internal inline void graph_add_series(ee_graph *g, const ee_graph_series *s) { (*(void (__thiscall **)(ee_graph *, const ee_graph_series *))data_graph_iat_add_series)(g, s); }

// HOOKS
// through the exe's imports of Low-Level Engine (the slots checked against the exports)
typedef void (__thiscall *graph_add_point_fn)(ee_graph_series *s, const ee_graph_point *p);
typedef void (__thiscall *graph_prune_fn)(ee_graph *g);
typedef void (__thiscall *graph_combine_fn)(ee_graph *g, const ee_graph *other, ee_graph *out);
internal inline int patch_graph_add_point(graph_add_point_fn h, graph_add_point_fn *orig) {
    return iat_hook(data_graph_iat_add_point, "Low-Level Engine.dll", "?AddDataPoint@UGraphSeries@@QAEXABV?$U2DPoint@J@@@Z", (void *)h, (void **)orig);
}
internal inline int patch_graph_prune(graph_prune_fn h, graph_prune_fn *orig) {
    return iat_hook(data_graph_iat_prune, "Low-Level Engine.dll", "?PruneGraph@UGraph@@QAEXXZ", (void *)h, (void **)orig);
}
internal inline int patch_graph_combine(graph_combine_fn h, graph_combine_fn *orig) {
    return iat_hook(data_graph_iat_combine, "Low-Level Engine.dll", "?CombineGraph@UGraph@@QAEXABV1@AAV1@@Z", (void *)h, (void **)orig);
}
