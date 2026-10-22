// THE GAME SETUP (game start 0x52599B(state, setup)): its settings and player slots.

// FIELDS: 16 player slots of 0x38, then the game's settings
internal inline uint8_t *setup_slot_at(const void *setup, int i) { return (uint8_t *)FIELD_AT(setup, i * SETUPSLOT_SIZE); }
internal inline uint8_t *setup_map_name_at(const void *setup) { return (uint8_t *)FIELD_AT(setup, SETUP_MAP_NAME); } // a UString
internal inline int setup_get_mode(const void *setup) { return FIELD(setup, SETUP_MODE, int); } // 0 random map, 1 scenario, 2 saved game, 3 editor test, 4 campaign
internal inline void setup_set_mode(void *setup, int v) { FIELD(setup, SETUP_MODE, int) = v; }
internal inline void setup_set_unknown_3d4(void *setup, int v) { FIELD(setup, SETUP_0x3D4, int) = v; }
internal inline void setup_set_map_scale(void *setup, int v) { FIELD(setup, SETUP_MAP_SCALE, int) = v; }   // 1 Small ..
internal inline void setup_set_start_epoch(void *setup, int v) { FIELD(setup, SETUP_START_EPOCH, int) = v; }
internal inline void setup_set_end_epoch(void *setup, int v) { FIELD(setup, SETUP_END_EPOCH, int) = v; }
internal inline void setup_set_map_size(void *setup, int v) { FIELD(setup, SETUP_MAP_SIZE, int) = v; }
internal inline void setup_set_unknown_3ec(void *setup, int v) { FIELD(setup, SETUP_0x3EC, int) = v; }
internal inline void setup_set_unit_limit(void *setup, int v) { FIELD(setup, SETUP_UNIT_LIMIT, int) = v; }
internal inline void setup_set_unknown_3f4(void *setup, int v) { FIELD(setup, SETUP_0x3F4, int) = v; }
internal inline void setup_set_difficulty(void *setup, int v) { FIELD(setup, SETUP_DIFFICULTY, int) = v; }
internal inline void setup_set_unknown_3fc(void *setup, int v) { FIELD(setup, SETUP_0x3FC, int) = v; }
// +0x400 .. +0x403 four flag bytes (the random map setup's 1, 0, 0, 1)
internal inline void setup_set_flag(void *setup, int i, uint8_t v) { FIELD(setup, SETUP_FLAGS + i, uint8_t) = v; }
// a slot: +0 its type (1 human, 0 computer, 4 closed), +0x20 the player's index, +0x24 its colour, +0x28 team,
// +0x2C citizens, +0x34 flags; +0x4 (3), +0x1C, +0x30 (0) not known
internal inline int slot_get_type(const uint8_t *slot) { return FIELD(slot, SETUPSLOT_TYPE, int); }
internal inline void slot_set_type(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_TYPE, int) = v; }
internal inline void slot_set_unknown_4(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_0x4, int) = v; }
internal inline uint8_t *slot_name_at(const uint8_t *slot) { return (uint8_t *)FIELD_AT(slot, SETUPSLOT_NAME); } // +0x8 the player's name (a UString)
internal inline void slot_set_unknown_1c(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_0x1C, int) = v; }
internal inline void slot_set_index(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_INDEX, int) = v; }
internal inline int slot_get_color(const uint8_t *slot) { return FIELD(slot, SETUPSLOT_COLOR, int); }
internal inline void slot_set_color(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_COLOR, int) = v; }
internal inline void slot_set_team(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_TEAM, int) = v; }
internal inline void slot_set_citizens(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_CITIZENS, int) = v; }
internal inline void slot_set_unknown_30(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_0x30, int) = v; }
internal inline void slot_set_flags(uint8_t *slot, uint32_t v) { FIELD(slot, SETUPSLOT_FLAGS, uint32_t) = v; }
