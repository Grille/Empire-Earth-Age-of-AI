// THE RANDOM MAP's types (spec/game/random_map.cs): the game's list of them (random_map.types, a vector of ee_map_type)
// and its maker's hooks - gen/random_map.c's patch_random_map_create, patch_random_map_types_load.

// the list's types: how many (0 before the game's start has read its scripts), the i-th
internal inline unsigned map_types_count(void) {
    uint8_t *v = random_map_types_at(), *first = FIELD(v, 4, uint8_t *);
    return first ? (unsigned)(FIELD(v, 8, uint8_t *) - first) / size_ee_map_type : 0;
}
internal inline ee_map_type *map_types_at(unsigned i) { return (ee_map_type *)(FIELD(random_map_types_at(), 4, uint8_t *) + i * size_ee_map_type); }
internal inline const char *map_type_name_text(const ee_map_type *t) { return wstring_text(map_type_name_at(t)); }
// a type more at the list's end: name what a setup's map_name holds for it, title what the lists show, generator the
// plug-in's name its script would give ("Default" - only the game's own maker looks it up). The game's push copies
// the three strings: ours are let go of
internal void map_types_add(const char *name, const char *title, const char *generator) {
    uint8_t raw[0x40];   // an ee_map_type's 0x38 in both builds
    if (size_ee_map_type > sizeof raw) return;
    ee_map_type *e = (ee_map_type *)raw;
    wstring_init(map_type_name_at(e));
    wstring_init(map_type_title_at(e));
    ustring_init(map_type_generator_at(e));
    wstring_set(map_type_name_at(e), name);
    wstring_set(map_type_title_at(e), title);
    ustring_set(map_type_generator_at(e), generator);
    random_map_types_add(random_map_types_at(), e);
    ustring_free(map_type_generator_at(e));
    wstring_free(map_type_title_at(e));
    wstring_free(map_type_name_at(e));
}

// THE GROUND OF A NEW MAP set whole, as the game's random map sets it (0x66F59D: its three passes): h a
// height per corner in tiles, (size + 1) x (size + 1), [y * (size + 1) + x] - each corner of the map its height and,
// under the water, the water's colours; then the corners' normals and each tile its lowest corner's height and its
// water flag (the view table's byte 3); then the tiles' shorelines. The textures and the cliffs come after it
// (map_reset_tile per tile, map_cliff_pass)
internal void map_ground_set(ee_map *map, const float *h) {
    int n = (int)map_get_size(map);
    void *mesh = map_mesh_at(map);
    for (int y = 0; y <= n; y++)
        for (int x = 0; x <= n; x++) {
            if (!map_corner(map, x, y)) continue;
            float z = h[y * (n + 1) + x];
            mesh_set_z(mesh, (unsigned long)x, (unsigned long)y, z);
            if (z < 0) (*(mesh_at_fn *)data_map_iat_set_water_color_set)(mesh, (unsigned long)x, (unsigned long)y);
        }
    ee_tile **tiles = map_get_tiles(map);
    for (int y = 0; y <= n; y++)
        for (int x = 0; x <= n; x++) {
            if (!map_corner(map, x, y)) continue;
            mesh_compute_normal(mesh, (unsigned long)x, (unsigned long)y);
            ee_tile *t = x < n && y < n ? tiles[y * n + x] : NULL;
            if (!t || !map_corner(map, x + 1, y) || !map_corner(map, x, y + 1) || !map_corner(map, x + 1, y + 1)) continue;
            float lo = fminf(fminf(h[y * (n + 1) + x], h[y * (n + 1) + x + 1]), fminf(h[(y + 1) * (n + 1) + x], h[(y + 1) * (n + 1) + x + 1]));
            void *row = map_view_row(map, (unsigned)y);
            unsigned i = (unsigned)x - FIELD(row, of_view_row_start, unsigned);
            if (i < FIELD(row, of_view_row_len, unsigned)) FIELD(row, of_view_row_data, uint8_t *)[i * 4 + 3] = lo < 0;
            tile_set_height(t, lo);
        }
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++)
            if (tiles[y * n + x]) mesh_compute_shorelines(mesh, (unsigned long)x, (unsigned long)y);
}
// a ground texture's index by its name in the data ("Grass", "Underwater"), -1: none
internal int terrain_named(const char *name) {
    for (uint32_t i = 0; i < 256; i++) {
        const ee_terrain *t = terrain_at(i);
        if (t && terrain_get_name(t) && !strcmp((const char *)terrain_get_name(t), name)) return (int)i;
    }
    return -1;
}
// a texture whose tiles are land to stand and build on: its kind neither water's nor a cliff's
internal int terrain_is_land(const ee_terrain *t) {
    int k = terrain_get_kind(t);
    return k != (int)value_map_kind_water && k != (int)value_map_kind_water_other && k != (int)value_map_kind_cliff;
}
