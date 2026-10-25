// A UNIT'S GOAL (ee_goal: EEUnitGoal and its kinds): fields, the goals the mod makes, the goal sets, hooks.

// FIELDS: a unit's +0x1EC; the kind by its vtable (goal_is_attack(g) ...), the fields of each kind from +0x20 on
// (GOAL_ARGS) its own
internal inline uint8_t goal_get_state(const ee_goal *g) { return FIELD(g, GOAL_STATE, uint8_t); }        // +0x5 (aim.c: 2 while it attacks)
internal inline void *goal_get_0x38(const ee_goal *g) { return FIELD(g, GOAL_0x38, void *); }          // +0x38 a kind 6 goal's target
internal inline void *goal_get_0x3c(const ee_goal *g) { return FIELD(g, GOAL_0x3C, void *); }          // +0x3C a kind 4 goal's target
// a move goal (goal_is_location): where to
internal inline float goal_get_dest_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS, float); }         // +0x20
internal inline float goal_get_dest_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x4, float); }         // +0x24
// an attack goal (goal_is_attack): the unit it attacks
internal inline ee_unit *goal_get_target(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x10, ee_unit *); }   // +0x30
// a gather goal (EEUGGather): the resource site
internal inline ee_unit *goal_get_site(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x18, ee_unit *); }     // +0x38
// a build or repair goal (EEUGBuildRepair, goal_is_build): the building
internal inline ee_unit *goal_get_building(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x1C, ee_unit *); } // +0x3C
// a load goal (goal_is_load): the transport or building to board
internal inline ee_unit *goal_get_holder(const ee_goal *g) { return FIELD(g, GOAL_HOLDER, ee_unit *); }   // +0x50
// a load goal: the passengers still to come (a std::vector at +0x20: begin +0x28, end +0x2C) - NoMoreLoadableUnitsToPickUp
// counts them, a passenger leaves it by boarding, by a new order or by CannotReachContainer
internal inline ee_unit **goal_get_pending_begin(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x8, ee_unit **); }
internal inline ee_unit **goal_get_pending_end(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0xC, ee_unit **); }
// a load goal's meeting point, tiles: the transport's +0x54, its passengers' +0x5C - set by its constructor (the
// transport class's vf56: EEUCNavalTransport's toward the first passenger), read by HaveReachedRendezvousPoint
internal inline void goal_set_meeting(ee_goal *g, int sx, int sy, int px, int py) {
    FIELD(g, GOAL_ARGS + 0x34, int) = sx; FIELD(g, GOAL_ARGS + 0x38, int) = sy;
    FIELD(g, GOAL_ARGS + 0x3C, int) = px; FIELD(g, GOAL_ARGS + 0x40, int) = py;
}
internal inline int goal_get_meeting_ship_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x34, int); }
internal inline int goal_get_meeting_ship_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x38, int); }
internal inline int goal_get_meeting_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x3C, int); }
internal inline int goal_get_meeting_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x40, int); }
// an unload goal (goal_is_unload): the water tiles +0x24 / +0x2C, the point to unload at +0x34, the reach (squared)
internal inline int goal_get_water_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x4, int); }
internal inline int goal_get_water_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x8, int); }
internal inline int goal_get_water2_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0xC, int); }
internal inline int goal_get_water2_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x10, int); }
internal inline int goal_get_unload_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x14, int); }
internal inline int goal_get_unload_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x18, int); }
internal inline float goal_get_reach_sq(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x1C, float); }

// FUNCTIONS
// where the goal sends unit u (its vtable: PrepareToMove asks it before the route request)
internal inline vec3 goal_target(ee_goal *goal, ee_unit *u) {
    float p[3] = { 0, 0, 0 };
    ((void (__thiscall *)(ee_goal *, ee_unit *, float *))(*(void ***)goal)[GA(SLOT_GOAL_TARGET)])(goal, u, p);
    return v3(p[0], p[1], p[2]);
}
// the goals the mod makes: built in game_new(GA(SIZE_GOAL_..)) memory, then given with unit_set_goal
// build or repair: pl's builder at building
internal inline ee_goal *goal_build_init(void *mem, ee_player *pl, ee_unit *building, int flag) { return (ee_goal *)GF(FN_BUILD_GOAL)(mem, pl, building, flag); }
// attack target (Art of Conquest's constructor takes a byte more: 0)
internal inline ee_goal *goal_attack_init(void *mem, ee_unit *u, ee_unit *target) {
    if (build_is("aoc")) return ((ee_goal *(__thiscall *)(void *, void *, void *, int, int))GA(FN_GOAL_ATTACK_CTOR))(mem, u, target, 1, 0);
    return ((ee_goal *(__thiscall *)(void *, void *, void *, int))GA(FN_GOAL_ATTACK_CTOR))(mem, u, target, 1);
}
// the units of vec (a std::vector) into holder (a transport or a building)
internal inline ee_goal *goal_load_init(void *mem, ee_unit *holder, void *vec) { return (ee_goal *)GF(FN_GOAL_LOAD_CTOR)(mem, holder, vec, 1); }
// the holder's units out at x, y
internal inline ee_goal *goal_unload_init(void *mem, ee_unit *holder, int x, int y, int flag) { return (ee_goal *)GF(FN_GOAL_UNLOAD_CTOR)(mem, holder, x, y, flag); }
// a move to xy (float[2])
internal inline ee_goal *goal_move_init(void *mem, float *xy) { return (ee_goal *)GF(FN_GOAL_MOVE_CTOR)(mem, xy, 1, 1, 0, 0.0f, 0, 0); }
// a goal's kind, by its vtable: an attack (EEUGAttackUnit, its target +0x30), a move (EEUGLocation), build or repair
// (EEUGBuildRepair, the building +0x3C), a load (EEUGLoadTransport, the holder +0x50), an unload (EEUGUnloadTransport)
internal inline int goal_is_attack(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_ATTACK); }
internal inline int goal_is_location(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_LOCATION); }
internal inline int goal_is_build(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_BUILD); }
internal inline int goal_is_load(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_LOAD); }
internal inline int goal_is_unload(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_UNLOAD); }
// the memory of a goal the game may free (game_new of its size), then made with its _init
internal inline void *goal_build_alloc(void) { return game_new(GA(SIZE_GOAL_BUILD)); }
internal inline void *goal_attack_alloc(void) { return game_new(GA(SIZE_GOAL_ATTACK)); }
internal inline void *goal_load_alloc(void) { return game_new(GA(SIZE_GOAL_LOAD)); }
internal inline void *goal_unload_alloc(void) { return game_new(GA(SIZE_GOAL_UNLOAD)); }
internal inline void *goal_move_alloc(void) { return game_new(GA(SIZE_GOAL_MOVE)); }
// a load goal's part the units walk into (the goal's vtable)
internal inline ee_goal *goal_part(ee_goal *goal) { return ((ee_goal *(__thiscall *)(void *))(*(void ***)goal)[GA(SLOT_GOAL_PART)])(goal); }
// a load goal's passenger u no longer awaited - the game's own when a passenger takes another order: only if u is in
// nothing (+0x1D8) and the holder's goal is a load goal, then u leaves that goal's pending passengers (if there)
// u into a load goal under way: one more pending passenger (if not there yet), as the player's load command adds to it
internal inline void goal_add_passenger(ee_goal *goal, ee_unit *u) { GF(FN_LOAD_ADD)(goal, u); }
internal inline void goal_drop_passenger(ee_goal *goal, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)goal)[GA(SLOT_GOAL_DROP)])(goal, u); }
// may the goal be handed to the unit (the original of attack.c's hook_goal_cast)
internal inline char goal_fits(ee_goal *goal, void *unit) { return GF(FN_GOAL_FBA_CHECK)(goal, unit); }
// a goal set's answer for the unit (the original of landing.c's hook)
internal inline int goalset_ask(void *set, ee_unit *u, int *out) { return GF(FN_GOALSET_ASK)(set, u, out); }

// HOOKS
typedef int (__thiscall *goalset_ask_hook)(void *set, ee_unit *unit, int *out);
internal inline int patch_goalset_ask(goalset_ask_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_GOALSET_ASK), GA(CALL_GOALSET_AGAIN) };
    return patch_calls(sites, 2, GA(FN_GOALSET_ASK), (void *)h, of);
}
typedef char (__thiscall *goal_fits_hook)(ee_goal *goal, void *unit);
internal inline int patch_goal_fits(goal_fits_hook h) { return patch_call(GA(CALL_GOAL_FBA_CHECK), GA(FN_GOAL_FBA_CHECK), (void *)h); }
// a group move's location goal for each unit at its slot (the group's dispatch, after units and slots are paired:
// groupmove.c); pt the caller's copy of the slot's point, the rest passed on as they are
typedef void *(__thiscall *slot_goal_hook)(ee_goal *g, float *pt, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);
internal inline void *goal_move_at(ee_goal *g, float *pt, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) {
    return GF(FN_GOAL_MOVE_AT)(g, pt, a, b, c, d, e, f);
}
internal inline int patch_slot_goal(slot_goal_hook h) { return patch_call(GA(CALL_SLOT_GOAL), GA(FN_GOAL_MOVE_AT), (void *)h); }
// the unload goal's construction (landing.c)
typedef void *(__thiscall *unload_ctor_hook)(ee_goal *g, ee_unit *transport, int x, int y, int flag);
internal inline int patch_unload_ctor(unload_ctor_hook h, int *of) { return patch_calls(PATCH_LIST(UNLOAD_SITES), GA(FN_GOAL_UNLOAD_CTOR), (void *)h, of); }
// the goal set's read of a unit's goal object (6 bytes: mov ecx, [esi+0x1E8]) -> call thunk; nop
internal inline int patch_goalset_read(thunk_fn thunk) {
    static const uint8_t want[6] = { 0x8B, 0x8E, 0xE8, 0x01, 0x00, 0x00 };
    uint8_t *p = (uint8_t *)GA(AT_GOAL_SET_READ);
    DWORD old;
    if (memcmp(p, want, 6) || !VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    p[0] = 0xE8; *(int32_t *)(p + 1) = (int32_t)((uintptr_t)thunk - (GA(AT_GOAL_SET_READ) + 5)); p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return 1;
}
