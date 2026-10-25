// A UNIT'S GOAL (ee_goal: EEUnitGoal and its kinds): fields, the goals the mod makes, the goal sets, hooks.

// FIELDS: a unit's +0x1EC; the kind by its vtable (goal_is_attack(g) ...), the fields of each kind from +0x20 on
// (GOAL_ARGS) its own
// a move goal (goal_is_location): where to
internal inline float goal_get_dest_y(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x4, float); }         // +0x24
// an attack goal (goal_is_attack): the unit it attacks
internal inline ee_unit *goal_get_target(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x10, ee_unit *); }   // +0x30
// a gather goal (EEUGGather): the resource site
internal inline ee_unit *goal_get_site(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x18, ee_unit *); }     // +0x38
// a build or repair goal (EEUGBuildRepair, goal_is_build): the building
internal inline ee_unit *goal_get_building(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x1C, ee_unit *); } // +0x3C
// a load goal (goal_is_load): the transport or building to board
// a load goal: the passengers still to come (a std::vector at +0x20: begin +0x28, end +0x2C) - NoMoreLoadableUnitsToPickUp
// counts them, a passenger leaves it by boarding, by a new order or by CannotReachContainer
internal inline ee_unit **goal_get_pending_begin(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x8, ee_unit **); }
internal inline ee_unit **goal_get_pending_end(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0xC, ee_unit **); }
// a load goal's meeting point, tiles: the transport's +0x54, its passengers' +0x5C - set by its constructor (the
// transport class's vf56: EEUCNavalTransport's toward the first passenger), read by HaveReachedRendezvousPoint
internal inline void goal_set_meeting(ee_goal *g, int sx, int sy, int px, int py) {
    FIELD(g, of_goal_dest_x + 0x34, int) = sx; FIELD(g, of_goal_dest_x + 0x38, int) = sy;
    FIELD(g, of_goal_dest_x + 0x3C, int) = px; FIELD(g, of_goal_dest_x + 0x40, int) = py;
}
internal inline int goal_get_meeting_ship_x(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x34, int); }
internal inline int goal_get_meeting_ship_y(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x38, int); }
internal inline int goal_get_meeting_x(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x3C, int); }
internal inline int goal_get_meeting_y(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x40, int); }
// an unload goal (goal_is_unload): the water tiles +0x24 / +0x2C, the point to unload at +0x34, the reach (squared)
internal inline int goal_get_water_x(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x4, int); }
internal inline int goal_get_water_y(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x8, int); }
internal inline int goal_get_water2_x(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0xC, int); }
internal inline int goal_get_water2_y(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x10, int); }
internal inline int goal_get_unload_x(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x14, int); }
internal inline int goal_get_unload_y(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x18, int); }
internal inline float goal_get_reach_sq(const ee_goal *g) { return FIELD(g, of_goal_dest_x + 0x1C, float); }

// FUNCTIONS
// where the goal sends unit u (its vtable: PrepareToMove asks it before the route request)
internal inline vec3 goal_target(ee_goal *goal, ee_unit *u) {
    float p[3] = { 0, 0, 0 };
    ((void (__thiscall *)(ee_goal *, ee_unit *, float *))(*(void ***)goal)[value_goal_slot_goal_target])(goal, u, p);
    return v3(p[0], p[1], p[2]);
}
// the goals the mod makes: built in game_new(value_goal_size_goal_..) memory, then given with unit_assign_goal
// build or repair: pl's builder at building
// attack target (Art of Conquest's constructor takes a byte more: 0)
internal inline ee_goal *goal_attack_init(void *mem, ee_unit *u, ee_unit *target) {
    if (build_is("aoc")) return ((ee_goal *(__thiscall *)(void *, void *, void *, int, int))data_goal_fn_goal_attack_ctor)(mem, u, target, 1, 0);
    return ((ee_goal *(__thiscall *)(void *, void *, void *, int))data_goal_fn_goal_attack_ctor)(mem, u, target, 1);
}
// the units of vec (a std::vector) into holder (a transport or a building)
internal inline ee_goal *goal_load_init(void *mem, ee_unit *holder, void *vec) { return (ee_goal *)fn_load_goal_init(mem, holder, vec, 1); }
// the holder's units out at x, y
// a move to xy (float[2])
internal inline ee_goal *goal_move_init(void *mem, float *xy) { return (ee_goal *)fn_location_goal_init(mem, xy, 1, 1, 0, 0.0f, 0, 0); }
// a goal's kind, by its vtable: an attack (EEUGAttackUnit, its target +0x30), a move (EEUGLocation), build or repair
// (EEUGBuildRepair, the building +0x3C), a load (EEUGLoadTransport, the holder +0x50), an unload (EEUGUnloadTransport)
internal inline int goal_is_attack(const ee_goal *g) { return *(const uintptr_t *)g == data_goal_vt_goal_attack; }
internal inline int goal_is_location(const ee_goal *g) { return *(const uintptr_t *)g == data_goal_vt_goal_location; }
internal inline int goal_is_build(const ee_goal *g) { return *(const uintptr_t *)g == data_goal_vt_goal_build; }
internal inline int goal_is_load(const ee_goal *g) { return *(const uintptr_t *)g == data_goal_vt_goal_load; }
internal inline int goal_is_unload(const ee_goal *g) { return *(const uintptr_t *)g == data_goal_vt_goal_unload; }
// the memory of a goal the game may free (game_new of its size), then made with its _init
internal inline void *goal_build_alloc(void) { return game_new(value_goal_size_goal_build); }
internal inline void *goal_attack_alloc(void) { return game_new(value_goal_size_goal_attack); }
internal inline void *goal_load_alloc(void) { return game_new(value_goal_size_goal_load); }
internal inline void *goal_unload_alloc(void) { return game_new(value_goal_size_goal_unload); }
internal inline void *goal_move_alloc(void) { return game_new(value_goal_size_goal_move); }
// a load goal's part the units walk into (the goal's vtable)
internal inline ee_goal *goal_part(ee_goal *goal) { return ((ee_goal *(__thiscall *)(void *))(*(void ***)goal)[value_goal_slot_goal_part])(goal); }
// a load goal's passenger u no longer awaited - the game's own when a passenger takes another order: only if u is in
// nothing (+0x1D8) and the holder's goal is a load goal, then u leaves that goal's pending passengers (if there)
// u into a load goal under way: one more pending passenger (if not there yet), as the player's load command adds to it
internal inline void goal_drop_passenger(ee_goal *goal, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)goal)[value_goal_slot_goal_drop])(goal, u); }
// may the goal be handed to the unit (the original of attack.c's hook_goal_cast)
// a goal set's answer for the unit (the original of landing.c's hook)

// HOOKS
// a group move's location goal for each unit at its slot (the group's dispatch, after units and slots are paired:
// groupmove.c); pt the caller's copy of the slot's point, the rest passed on as they are
internal inline void *goal_move_at(ee_goal *g, float *pt, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) {
    return fn_goal_move_at_game(g, pt, a, b, c, d, e, f);
}
// the unload goal's construction (landing.c)
// the goal set's read of a unit's goal object (6 bytes: mov ecx, [esi+0x1E8]) -> call thunk; nop
internal inline int patch_goalset_read(thunk_fn thunk) {
    static const uint8_t want[6] = { 0x8B, 0x8E, 0xE8, 0x01, 0x00, 0x00 };
    uint8_t *p = (uint8_t *)data_goal_at_goal_set_read;
    DWORD old;
    if (memcmp(p, want, 6) || !VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    p[0] = 0xE8; *(int32_t *)(p + 1) = (int32_t)((uintptr_t)thunk - (data_goal_at_goal_set_read + 5)); p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return 1;
}
