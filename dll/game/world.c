// Unit type (object) data 0x9418A0[type]: 0x300 slots (the game's own loops end at 0x9424A0; type ids 0..723 are
// used, the slots after them are empty), past the table other data. Every read goes through here: NULL outside
// the table and for an empty slot.
#define G_TYPES_N 0x300
internal inline ee_unit_type *type_data(int t) { return t >= 0 && t < G_TYPES_N ? type_table()[t] : NULL; }

internal uint32_t game_time(void) {
    ee_physics *clock = game_physics();
    return clock ? physics_get_time(clock) : 0;
}

internal ee_player *resource_mgr_player(ee_resource_mgr *rm) {
    ee_civ_mgr *owner = resource_mgr_get_owner(rm);
    return owner ? civ_mgr_get_player(owner) : NULL;
}

// RTTI class name of a polymorphic object (MSVC: vtable[-1] -> complete object locator, +0xC type
// descriptor, name at +8 like ".?AVEEUGGather@@").
internal const char *rtti_name(void *obj) {
    if (!obj) return "none";
    uint8_t *col = ((uint8_t **)*(void **)obj)[-1];
    const char *n = *(const char **)(col + 0xC) + 8;
    return n[0] == '.' && n[1] == '?' && n[2] == 'A' ? n + 4 : n;
}

// An MSVC std::deque (the runtime library's, the same in both builds): +4 its map of blocks, +8 the map's size, +0xC
// the front element's index, +0x10 the count - its elements' size sets how many a block holds.
internal inline unsigned deque_map_size(const void *dq) { return FIELD(dq, 8, unsigned); }
internal inline unsigned deque_front(const void *dq) { return FIELD(dq, 0xC, unsigned); }
internal inline unsigned deque_count(const void *dq) { return FIELD(dq, 0x10, unsigned); }

// Element i of an MSVC std::deque<T*> (block of 1024 pointers; container +4 map, +8 map size), as the
// game's iterator deref 0x5F1BC6 computes it.
internal void *deque_at(uint8_t *dq, unsigned off) {
    unsigned blk = off >> 10, mapsz = deque_map_size(dq);
    if (blk >= mapsz) blk -= mapsz;
    return FIELD(dq, 4, void ***)[blk][off & 1023];
}
