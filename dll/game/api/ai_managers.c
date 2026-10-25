// THE AI'S MANAGERS (ee_civ_mgr, ee_resource_mgr, ee_build_mgr, ee_research_mgr, ee_unit_mgr) and the build plan:
// fields, functions, hooks.

// FIELDS
// A CIVILIZATION MANAGER (EECPCivilizationManager, a computer player's +0xB90; Update 0x46E2C7): the AI's managers
// +0x58 uint32_t[CM_TIMERS_N] game times: 0 next epoch, 1 building count check, 2 military buildings (31 s), 3 build
// (0x46E51E), 4 (0x46E817), 5 (59 s)
internal inline uint32_t cm_get_timer(const ee_civ_mgr *cm, int i) { return FIELD(cm, of_cm_timers + i * 4, uint32_t); }
internal inline void cm_set_timer(ee_civ_mgr *cm, int i, uint32_t v) { FIELD(cm, of_cm_timers + i * 4, uint32_t) = v; }
// +0x74.. flag bytes (CM_FLAGS_LEN): +0x75 the one-time setup done, +0x76 a request for the epoch's techs is out, the
// attention's activities +0x7A (0: request the epoch's techs), +0x7B (4: the build manager's pass), +0x7C (5); +0x78 not
// known
internal inline uint8_t cm_get_flag(const ee_civ_mgr *cm, int i) { return FIELD(cm, of_cm_flag + i, uint8_t); }

// A RESOURCE MANAGER (EECPResourceManager, CM +0x38; constructor/serializer 0x489F76 / 0x48C5B2)
internal inline void **rm_get_hunt_end(const ee_resource_mgr *rm) { return FIELD(rm, of_rm_hunt_begin + 0x4, void **); }
internal inline void **rm_get_fish_end(const ee_resource_mgr *rm) { return FIELD(rm, of_rm_fish_begin + 0x4, void **); }
// +0xA4 7 unit lists by resource (2 food .. 6 iron), deques 0x14 apart: their gatherers
internal inline uint8_t *rm_list_at(const ee_resource_mgr *rm, int r) { return (uint8_t *)FIELD_AT(rm, of_rm_lists + r * 0x14); }
internal inline uint32_t rm_get_timer(const ee_resource_mgr *rm, int i) { return FIELD(rm, of_rm_timers + i * 4, uint32_t); } // +0x16C uint[5] next runs (game time)

// A BUILD MANAGER (EECPBuildManager, CM +0x3C; Update 0x468540)
// +0x4 14 queues by epoch, deques 0x14 apart of {type, x, y}; the placement area follows at +0x11C
internal inline uint8_t *bm_queue_at(const ee_build_mgr *bm, int e) { return (uint8_t *)FIELD_AT(bm, of_bm_queues + e * 0x14); }
internal inline ivec2 bm_get_base(const ee_build_mgr *bm) { return iv2(bm_get_base_x(bm), bm_get_base_y(bm)); }   // -10000: none

// A UNIT MANAGER (EECPUnitManager, CM +0x44; Update 0x490AD6)
// +0xC 14 request lists, deques 0x14 apart of 16-byte entries {type, count, ..}
// its request i (0x14 bytes each)
internal inline uint8_t *um_request_at(const ee_unit_mgr *um, int i) { return (uint8_t *)FIELD_AT(um, of_um_requests + i * 0x14); }

// A RESEARCH MANAGER (EECPResearchManager, CM +0x40)
internal inline int *rs_get_pool_end(const ee_research_mgr *rs) { return FIELD(rs, of_rs_pool_begin + 0x4, int *); }
internal inline void rs_set_pool_end(ee_research_mgr *rs, int *e) { FIELD(rs, of_rs_pool_begin + 0x4, int *) = e; }
// +0x18 14 lists by epoch, 0x10 apart: {begin, end} of tech ids
internal inline int *rs_get_epoch_begin(const ee_research_mgr *rs, int e) { return FIELD(rs, of_rs_epoch + e * 0x10, int *); }
internal inline int *rs_get_epoch_end(const ee_research_mgr *rs, int e) { return FIELD(rs, of_rs_epoch + 0x4 + e * 0x10, int *); }
// the build plan's list nodes (cm_get_plan): +0 the next, +0xC / +0x10 the entry's values

// FUNCTIONS
// the next epoch attempt's time drawn (civilization manager +0x58: SetNextEpochAdvanceTime)
internal inline void cm_set_epoch_time(ee_civ_mgr *cm, uint32_t now, char flag) { GF(FN_SET_EPOCH_TIME)(cm, now, flag); }

// the build manager places type at x, y (0, 0: where it finds room) - the original of econ.c's hook_place
internal inline char bm_place(ee_build_mgr *bm, int type, int x, int y) { return GF(FN_PLACE)(bm, type, x, y); }
internal inline char bm_queued(ee_build_mgr *bm, int type) { return GF(FN_BM_QUEUED)(bm, type); }
// one building of type into the build manager's list (as the AI's own requests: (1, 0))
internal inline void bm_request(ee_build_mgr *bm, int type) { GF(FN_BM_REQUEST)(bm, type, 1, 0); }
// the resource manager's turn (the originals of the wiring's hooks, eeaimod.c): its gatherer targets computed (1: they
// changed), its citizens assigned, its update
internal inline char rm_compute_targets(ee_resource_mgr *rm) { return GF(FN_COMPUTE_TARGETS)(rm); }
internal inline char rm_assign_citizens(ee_resource_mgr *rm) { return GF(FN_ASSIGN_CITIZENS)(rm); }
internal inline void rm_update(ee_resource_mgr *rm, uint32_t now) { GF(FN_RM_UPDATE)(rm, now); }
// the citizen u to work at site (a resource, a field, a building); rm_assign_as: with the game's third argument
internal inline void rm_assign(ee_resource_mgr *rm, ee_unit *u, ee_unit *site) { GF(FN_ASSIGN_TO_SITE)(rm, u, site, 0); }
internal inline void rm_assign_as(ee_resource_mgr *rm, ee_unit *u, ee_unit *site, int c) { GF(FN_ASSIGN_TO_SITE)(rm, u, site, c); }
// the unit manager's citizens trained (the original of trace.c's hook)
internal inline uintptr_t um_train_citizens(ee_unit_mgr *um, uintptr_t a) { return GF(FN_TRAIN_CITIZENS)(um, a); }
// the unit manager's units of type: a std::deque of ee_unit *, NULL: none
internal inline uint8_t *um_type_list(ee_unit_mgr *um, int type) { return GF(FN_UM_TYPE_LIST)(um, type); }
// the unit orders (req: type, count ...) - the original of econ.c's hook_train_units
internal inline uintptr_t um_train(ee_unit_mgr *um, int *req, uintptr_t b) { return GF(FN_TRAIN_UNITS)(um, req, b); }
// tech id to research (requests its building if missing, adds its cost to the demand)
internal inline void rs_schedule(ee_research_mgr *rs, int id) { GF(FN_RS_SCHEDULE)(rs, id); }
// count of the next epoch's missing requirements asked for (buildings, units, techs)
internal inline void rs_request_requirements(ee_research_mgr *rs, int count) { GF(FN_REQUEST_TECHS)(rs, count); }
// enemies near x, y as the build manager sees them (its threat test)
internal inline char bm_enemy_near(ee_build_mgr *bm, int x, int y) { return GF(FN_ENEMY_NEAR)(bm, x, y); }
// entry (type, x, y) into the build manager's list of epoch e (the lists at +4, 0x14 each)
internal inline void bm_queue_add(ee_build_mgr *bm, int e, int *entry) { GF(FN_BQ_ADD)(FIELD_AT(bm, of_bm_queues + e * 0x14), entry); }
// the unit (its state, unit +0x30) in none of the civilization manager's groups
internal inline char cm_unit_free(void *cm, void *unit_state) { return GF(FN_CM_UNIT_FREE)(cm, unit_state); }
// a unit of type ordered but not started yet, one in production (the unit manager's queues)
internal inline char um_pending(ee_unit_mgr *um, int type) { return GF(FN_UM_PENDING)(um, type); }
internal inline char um_in_production(ee_unit_mgr *um, int type) { return GF(FN_UM_IN_PRODUCTION)(um, type); }
// the unit manager's citizens, the citizens it wants (share x population cap: the original of citizens.c's hook)
internal inline int um_citizens(ee_unit_mgr *um) { return GF(FN_UM_CITIZENS)(um); }
internal inline int um_citizens_wanted(ee_unit_mgr *um) { return GF(FN_UM_CITIZENS_WANTED)(um); }
// the fishing boat type of a player (the original of navy.c's hook)
internal inline int fish_type(ee_civ_mgr *cm) { return GF(FN_FISH_TYPE)(cm); }

// THE RESOURCE MANAGER'S SEARCHES (the originals of sites.c's hooks; arguments as the game passes them)
internal inline ee_unit *rm_find_mine(ee_resource_mgr *rm, uintptr_t type, ee_unit *unit, uintptr_t rev) {
    return GF(FN_FIND_MINE)(rm, type, (uintptr_t)unit, rev);
}
internal inline ee_unit *rm_find_food(ee_resource_mgr *rm, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit) {
    return GF(FN_FIND_FOOD)(rm, need_drop, near_base, (uintptr_t)unit);
}
internal inline ee_unit *rm_find_wood(ee_resource_mgr *rm, uintptr_t flag, ee_unit *unit) {
    return GF(FN_FIND_WOOD)(rm, flag, (uintptr_t)unit);
}
// the lists of fields, unbuilt fields and animals to hunt (n wanted, into out)
internal inline void *rm_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    return GF(FN_FARMS)(rm, n, out);
}
internal inline void *rm_unbuilt_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    return GF(FN_UNBUILT_FARMS)(rm, n, out);
}
internal inline void *rm_hunt(ee_resource_mgr *rm, uintptr_t n, uintptr_t f, uintptr_t out) {
    return GF(FN_HUNT)(rm, n, f, out);
}

// HOOKS
// the resource manager's turn (the wiring, eeaimod.c): its gatherer targets, its citizens' assignment, its update
typedef char (__thiscall *rm_hook)(ee_resource_mgr *rm);
internal inline int patch_rm_compute_targets(rm_hook h) { return patch_call(GA(CALL_COMPUTE_TARGETS), GA(FN_COMPUTE_TARGETS), (void *)h); }
internal inline int patch_rm_assign_citizens(rm_hook h) { return patch_call(GA(CALL_ASSIGN_CITIZENS), GA(FN_ASSIGN_CITIZENS), (void *)h); }
typedef void (__thiscall *rm_update_hook)(ee_resource_mgr *rm, uint32_t now);
internal inline int patch_rm_update(rm_update_hook h) { return patch_call(GA(CALL_RM_UPDATE), GA(FN_RM_UPDATE), (void *)h); }
typedef char (__thiscall *place_hook)(ee_build_mgr *bm, int type, int x, int y);
internal inline int patch_place(place_hook h) { return patch_call(GA(CALL_PLACE), GA(FN_PLACE), (void *)h); }
typedef uintptr_t (__thiscall *train_units_hook)(ee_unit_mgr *um, int *req, uintptr_t b);
internal inline int patch_train_units(train_units_hook h, int *of) { return patch_calls(PATCH_LIST(TRAIN_SITES), GA(FN_TRAIN_UNITS), (void *)h, of); }
typedef int (__thiscall *citizens_wanted_hook)(ee_unit_mgr *um);
internal inline int patch_citizens_wanted(citizens_wanted_hook h) { return patch_call(GA(CALL_CITIZENS_WANTED), GA(FN_UM_CITIZENS_WANTED), (void *)h); }
typedef int (__thiscall *fish_type_hook)(ee_civ_mgr *cm);
internal inline int patch_fish_type(fish_type_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_FISH_TYPE_1), GA(CALL_FISH_TYPE_2) };
    return patch_calls(sites, 2, GA(FN_FISH_TYPE), (void *)h, of);
}

// THE RESOURCE MANAGER'S SEARCHES (sites.c)
typedef void *(__thiscall *find_mine_hook)(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev);
internal inline int patch_find_mine(find_mine_hook h, int *of) { return patch_calls(PATCH_LIST(MINE_SITES), GA(FN_FIND_MINE), (void *)h, of); }
typedef void *(__thiscall *find_food_hook)(ee_resource_mgr *rm, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit);
internal inline int patch_find_food(find_food_hook h, int *of) { return patch_calls(PATCH_LIST(FOOD_SITES), GA(FN_FIND_FOOD), (void *)h, of); }
typedef void *(__thiscall *find_wood_hook)(ee_resource_mgr *rm, uintptr_t flag, ee_unit *unit);
internal inline int patch_find_wood(find_wood_hook h, int *of) { return patch_calls(PATCH_LIST(WOOD_SITES), GA(FN_FIND_WOOD), (void *)h, of); }
typedef void *(__thiscall *farms_hook)(ee_resource_mgr *rm, uintptr_t n, uintptr_t out);
internal inline int patch_farms(farms_hook h, farms_hook unbuilt, int *of) {   // unbuilt NULL: the fields only
    if (of) *of += unbuilt ? 2 : 1;
    return patch_call(GA(CALL_FARMS), GA(FN_FARMS), (void *)h) + (unbuilt ? patch_call(GA(CALL_UNBUILT_FARMS), GA(FN_UNBUILT_FARMS), (void *)unbuilt) : 0);
}
// a citizen sent to work at a site (the calls of the assignment: GATHER_SITES), the citizens trained (trace.c)
typedef void (__thiscall *gather_hook)(ee_resource_mgr *rm, ee_unit *u, ee_unit *site, int c);
internal inline int patch_gather(gather_hook h, int *of) { return patch_calls(PATCH_LIST(GATHER_SITES), GA(FN_ASSIGN_TO_SITE), (void *)h, of); }
typedef uintptr_t (__thiscall *train_citizens_hook)(ee_unit_mgr *um, uintptr_t a);
internal inline int patch_train_citizens(train_citizens_hook h) { return patch_call(GA(CALL_TRAIN_CITIZENS), GA(FN_TRAIN_CITIZENS), (void *)h); }
typedef void *(__thiscall *hunt_hook)(ee_resource_mgr *rm, uintptr_t n, uintptr_t f, uintptr_t out);
internal inline int patch_hunt(hunt_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_HUNT_1), GA(CALL_HUNT_2) };
    return patch_calls(sites, 2, GA(FN_HUNT), (void *)h, of);
}
