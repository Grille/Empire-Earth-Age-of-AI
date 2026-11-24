// THE GAME SETUP (game start 0x52599B(state, setup)): its settings and player slots.

// FIELDS: 16 player slots of 0x38, then the game's settings
internal inline ee_setup_slot *setup_slot_at(const ee_setup *setup, int i) { return (ee_setup_slot *)FIELD_AT(setup, i * of_setup_slot_size); }
internal inline void setup_set_unknown_3fc(ee_setup *setup, int v) { FIELD(setup, of_setup_0x3fc, int) = v; }
// +0x400.. option bytes (the registry's Game Options): victory allowed, lock teams, lock speed, reveal map, cheat codes,
// record game, custom civs
internal inline void setup_set_flag(ee_setup *setup, int i, uint8_t v) { FIELD(setup, of_setup_flags + i, uint8_t) = v; }
// a slot: +0 its type (1 human, 0 computer, 4 closed), +0x20 the player's index, +0x24 its colour, +0x28 team,
// +0x2C citizens, +0x34 flags; +0x4 (3), +0x1C, +0x30 (0) not known
internal inline void setup_slot_set_unknown_4(ee_setup_slot *slot, int v) { FIELD(slot, of_setup_slot_0x4, int) = v; }
internal inline void setup_slot_set_unknown_1c(ee_setup_slot *slot, int v) { FIELD(slot, of_setup_slot_0x1c, int) = v; }
internal inline int setup_slot_get_unknown_1c(const ee_setup_slot *slot) { return FIELD(slot, of_setup_slot_0x1c, int); }   // a lobby's human: the network's slot
internal inline void setup_slot_set_unknown_30(ee_setup_slot *slot, int v) { FIELD(slot, of_setup_slot_0x30, int) = v; }

// MORE MAP SIZES (spec/game/setup.cs, random_map.cs, path.cs): the game's lists of sizes made n long (its own
// map_scales and more), the setup copy letting the scales up to n through, and a map wider than Gigantic given
// Gigantic's script size and pathing sectors. A scale past Gigantic's needs its sides from patch_map_dims's hook and
// its name from the language lookup (map_scale_text + scale). How many of the six went in
internal inline unsigned setup_map_scales(void) { return value_setup_map_scales; }
internal inline int setup_map_scale_text(void) { return (int)value_setup_map_scale_text; }
internal int patch_map_scales(unsigned n) {
    unsigned was = value_setup_map_scales;
    if (n <= was || n > 127) return 0;
    return patch_byte(data_setup_site_scale_limit, (uint8_t)was, (uint8_t)n) + patch_byte(data_setup_site_editor_scales, (uint8_t)was, (uint8_t)n) +
           patch_byte(data_setup_site_editor_scales_end, (uint8_t)was, (uint8_t)n) +
           patch_code32(data_setup_site_setup_scales_end, value_setup_map_scale_text + was - 1, value_setup_map_scale_text + n - 1) +
           patch_code32(data_random_map_site_script_gigantic, value_path_gigantic_width, 0xFFFFFFFFu) +   // ja: never
           patch_code32(data_path_site_sectors_gigantic, value_path_gigantic_width, 0x7FFFFFFFu);        // jg: never
}
#define MAP_SCALES_PATCHES 6
