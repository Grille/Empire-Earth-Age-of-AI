// THE GAME SETUP (game start 0x52599B(state, setup)): its settings and player slots.

// FIELDS: 16 player slots of 0x38, then the game's settings
internal inline ee_setup_slot *setup_slot_at(const ee_setup *setup, int i) { return (ee_setup_slot *)FIELD_AT(setup, i * of_slot_size); }
internal inline void setup_set_unknown_3fc(ee_setup *setup, int v) { FIELD(setup, of_setup_0x3fc, int) = v; }
// +0x400.. option bytes (the registry's Game Options): victory allowed, lock teams, lock speed, reveal map, cheat codes,
// record game, custom civs
internal inline void setup_set_flag(ee_setup *setup, int i, uint8_t v) { FIELD(setup, of_setup_flags + i, uint8_t) = v; }
// a slot: +0 its type (1 human, 0 computer, 4 closed), +0x20 the player's index, +0x24 its colour, +0x28 team,
// +0x2C citizens, +0x34 flags; +0x4 (3), +0x1C, +0x30 (0) not known
internal inline void slot_set_unknown_4(ee_setup_slot *slot, int v) { FIELD(slot, of_slot_0x4, int) = v; }
internal inline void slot_set_unknown_1c(ee_setup_slot *slot, int v) { FIELD(slot, of_slot_0x1c, int) = v; }
internal inline void slot_set_unknown_30(ee_setup_slot *slot, int v) { FIELD(slot, of_slot_0x30, int) = v; }
