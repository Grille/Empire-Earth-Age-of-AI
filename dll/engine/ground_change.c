// THE GROUND CHANGED (the player, 2026-11-27, on the bent objects' kept copies left out of the scenario editor: "Maybe
// making such an hook/event would be valuable than? We already own terrain edits and building placement anyways right?").
// What keeps something made of the ground's heights - render_units.c's bent copies of the objects standing on it - asks
// here which parts of the map changed since it last looked. Told by every path the mod sees the ground change through:
// the game's SetZ, normals and shores in a running game (render_ground.c rg_touch: a building flattening its ground), the
// scenario editor's paint tools (editor_undo.c: their box, as they tell the minimap), the editor's undo and redo of a
// step, a map made anew. A count a cell of GC_ACROSS x GC_ACROSS over the map, and one in all: the telling costs an
// increment (a map's making sets a million corners), the asking compares the counts once a frame.
#define GC_ACROSS 128
#define GC_MARGIN 4          // tiles round the box: an object's bend reaches the ground under all of it (debris up to 4 x 4)
internal volatile LONG g_gc_gen;
internal volatile LONG g_gc_cell[GC_ACROSS * GC_ACROSS];
// a cell's side in tiles on a map of this size
internal int gc_side(int size) { return size > 0 ? (size + GC_ACROSS - 1) / GC_ACROSS : 1; }
// the cell of the map's point x, y; -1 off the map's cells
internal int gc_cell(float x, float y) {
    ee_map *map = game_map();
    if (!map || !(x >= 0) || !(y >= 0)) return -1;
    int side = gc_side((int)map_get_size(map)), cx = (int)x / side, cy = (int)y / side;
    return cx < GC_ACROSS && cy < GC_ACROSS ? cy * GC_ACROSS + cx : -1;
}
// the ground changed in the tiles x0 .. x1, y0 .. y1 (either order)
internal void ground_changed_box(int x0, int y0, int x1, int y1) {
    ee_map *map = game_map();
    if (!map) return;
    int size = (int)map_get_size(map), side = gc_side(size);
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    x0 = (x0 - GC_MARGIN) / side; x1 = (x1 + GC_MARGIN) / side;
    y0 = (y0 - GC_MARGIN) / side; y1 = (y1 + GC_MARGIN) / side;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= GC_ACROSS) x1 = GC_ACROSS - 1;
    if (y1 >= GC_ACROSS) y1 = GC_ACROSS - 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) InterlockedIncrement(&g_gc_cell[y * GC_ACROSS + x]);
    InterlockedIncrement(&g_gc_gen);
}
// the whole map (made anew in the editor: a new map may have the old one's address)
internal void ground_changed_all(void) { ground_changed_box(0, 0, 1 << 16, 1 << 16); }
