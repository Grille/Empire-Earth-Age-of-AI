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
