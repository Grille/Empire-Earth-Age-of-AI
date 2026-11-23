// THE GROUND'S HEIGHT KEPT: the fills ask the engine for the ground's height under every tree, building and remembered
// object at every update (GETerrainMesh::GetMeshZ: the mesh's triangle under the point - about 5 % of the render thread
// zoomed out over 3000 units, paused). The ground doesn't change while a game runs, so each point's answer is kept,
// the engine's own to the bit, by the point in a table (a point met again: its slot's; another in its slot: asked
// anew). Made anew with a new game (the same mesh's memory may hold another map), never while none runs (the
// scenario editor changes the ground under them).
#define UG_SLOTS 32768
typedef struct { uint32_t x, y; float z; const void *mesh; } ug_entry;
internal struct { ug_entry e[UG_SLOTS]; LONG starts; } g_ug;
internal float uc_mesh_z(const void *mesh, float x, float y) {
    if (!game_time()) return mesh_z(mesh, x, y);
    if (g_ug.starts != g_game_starts) { memset(g_ug.e, 0, sizeof g_ug.e); g_ug.starts = g_game_starts; }
    uint32_t a, b;
    memcpy(&a, &x, 4), memcpy(&b, &y, 4);
    ug_entry *e = &g_ug.e[(a * 2654435761u ^ b * 2246822519u) >> 17];
    if (e->mesh == mesh && e->x == a && e->y == b) return e->z;
    float z = mesh_z(mesh, x, y);
    e->x = a, e->y = b, e->z = z, e->mesh = mesh;
    return z;
}
// A TREE'S, BY ITS TILE: a tree stands at one point of its tile (shifted by its id), kept by the tile in blocks of
// UGT_B x UGT_B tiles made as trees are met. The walk goes along the rows, so neighbouring trees' entries lie together;
// in the points' table above a hash put each tree anywhere in its 512 KB - zoomed out over a 1024 map's forests (15 500
// trees) that one read was 7 % of the render thread (the player's planets, 2026-11-23). A point other than the one kept
// for the tile (another tree there): asked through the points' table, as before.
#define UGT_B 64
typedef struct { uint32_t x, y; float z; uint32_t ok; } ugt_entry;
internal struct { ugt_entry **blk; uint32_t bw, size; const void *mesh; LONG starts; } g_ugt;
internal float uc_tree_z(const void *mesh, uint32_t size, int tx, int ty, float x, float y) {
    if (!game_time()) return mesh_z(mesh, x, y);
    if (g_ugt.starts != g_game_starts || g_ugt.mesh != mesh || g_ugt.size != size) {
        for (uint32_t i = 0; g_ugt.blk && i < g_ugt.bw * g_ugt.bw; i++) free(g_ugt.blk[i]);
        free(g_ugt.blk);
        g_ugt.bw = (size + UGT_B - 1) / UGT_B, g_ugt.size = size, g_ugt.mesh = mesh, g_ugt.starts = g_game_starts;
        g_ugt.blk = g_ugt.bw ? calloc((size_t)g_ugt.bw * g_ugt.bw, sizeof *g_ugt.blk) : NULL;
    }
    if (!g_ugt.blk || (uint32_t)tx >= size || (uint32_t)ty >= size) return uc_mesh_z(mesh, x, y);
    ugt_entry **b = &g_ugt.blk[(uint32_t)ty / UGT_B * g_ugt.bw + (uint32_t)tx / UGT_B];
    if (!*b && !(*b = calloc(UGT_B * UGT_B, sizeof **b))) return uc_mesh_z(mesh, x, y);
    ugt_entry *e = &(*b)[(uint32_t)ty % UGT_B * UGT_B + (uint32_t)tx % UGT_B];
    uint32_t a, c;
    memcpy(&a, &x, 4), memcpy(&c, &y, 4);
    if (e->ok && e->x == a && e->y == c) return e->z;
    if (e->ok) return uc_mesh_z(mesh, x, y);
    float z = mesh_z(mesh, x, y);
    e->x = a, e->y = c, e->z = z, e->ok = 1;
    return z;
}
