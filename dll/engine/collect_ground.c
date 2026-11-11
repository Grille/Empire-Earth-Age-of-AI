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
