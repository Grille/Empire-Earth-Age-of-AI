// Unit type (object) data 0x9418A0[type]: 0x300 slots (the game's own loops end at 0x9424A0; type ids 0..723 are
// used, the slots after them are empty), past the table other data. Every read goes through here: NULL outside
// the table and for an empty slot.
#define G_TYPES_N 0x300
static inline uint8_t *type_data(int t) { return t >= 0 && t < G_TYPES_N ? ((uint8_t **)GA(G_TYPES))[t] : NULL; }

static uint32_t game_time(void) {
    uint8_t *clock = *(uint8_t **)GA(G_WORLD_CLOCK);
    return clock ? FIELD(clock, 0xD0, uint32_t) : 0;
}

static void *rm_player(void *rm) {
    void *owner = FIELD(rm, RM_OWNER, void *);
    return owner ? FIELD(owner, 0x4C, void *) : NULL;
}
