// THE AI'S MANAGERS (ee_civ_mgr, ee_resource_mgr, ee_build_mgr, ee_research_mgr, ee_unit_mgr) and the build plan:
// fields, functions, hooks.

// FIELDS
// A CIVILIZATION MANAGER (EECPCivilizationManager, a computer player's +0xB90; Update 0x46E2C7): the AI's managers
// +0x58 uint32_t[CM_TIMERS_N] game times: 0 next epoch, 1 building count check, 2 military buildings (31 s), 3 build
// (0x46E51E), 4 (0x46E817), 5 (59 s)
internal inline uint32_t civ_mgr_get_timer(const ee_civ_mgr *cm, int i) { return FIELD(cm, of_civ_mgr_timers + i * 4, uint32_t); }
internal inline void civ_mgr_set_timer(ee_civ_mgr *cm, int i, uint32_t v) { FIELD(cm, of_civ_mgr_timers + i * 4, uint32_t) = v; }
// +0x74.. flag bytes (CM_FLAGS_LEN): +0x75 the one-time setup done, +0x76 a request for the epoch's techs is out, the
// attention's activities +0x7A (0: request the epoch's techs), +0x7B (4: the build manager's pass), +0x7C (5); +0x78 not
// known
internal inline uint8_t civ_mgr_get_flag(const ee_civ_mgr *cm, int i) { return FIELD(cm, of_civ_mgr_flag + i, uint8_t); }

// A RESOURCE MANAGER (EECPResourceManager, CM +0x38; constructor/serializer 0x489F76 / 0x48C5B2)
// +0xA4 7 unit lists by resource (2 food .. 6 iron), deques 0x14 apart: their gatherers
internal inline uint8_t *resource_mgr_list_at(const ee_resource_mgr *rm, int r) { return (uint8_t *)FIELD_AT(rm, of_resource_mgr_lists + r * 0x14); }
// its five next times (game time) in a row, for the log: next_fishing_boat, _0x170, next_targets, next_assign, next_prune
internal inline uint32_t resource_mgr_get_timer(const ee_resource_mgr *rm, int i) { return FIELD(rm, of_resource_mgr_next_fishing_boat + i * 4, uint32_t); }

// A BUILD MANAGER (EECPBuildManager, CM +0x3C; Update 0x468540)
// +0x4 14 queues by epoch, deques 0x14 apart of {type, x, y}; the placement area follows at +0x11C
internal inline uint8_t *build_mgr_queue_at(const ee_build_mgr *bm, int e) { return (uint8_t *)FIELD_AT(bm, of_build_mgr_queues + e * 0x14); }
internal inline ivec2 build_mgr_get_base(const ee_build_mgr *bm) { return iv2(build_mgr_get_base_x(bm), build_mgr_get_base_y(bm)); }   // -10000: none

// A UNIT MANAGER (EECPUnitManager, CM +0x44; Update 0x490AD6)
// +0xC 14 request lists, deques 0x14 apart of 16-byte entries {type, count, ..}
// its request i (0x14 bytes each)
internal inline uint8_t *unit_mgr_request_at(const ee_unit_mgr *um, int i) { return (uint8_t *)FIELD_AT(um, of_unit_mgr_requests + i * 0x14); }

// A RESEARCH MANAGER (EECPResearchManager, CM +0x40)
internal inline int *research_mgr_get_pool_end(const ee_research_mgr *rs) { return FIELD(rs, of_research_mgr_pool_begin + 0x4, int *); }
internal inline void research_mgr_set_pool_end(ee_research_mgr *rs, int *e) { FIELD(rs, of_research_mgr_pool_begin + 0x4, int *) = e; }
// +0x18 14 lists by epoch, 0x10 apart: {begin, end} of tech ids
internal inline int *research_mgr_get_epoch_begin(const ee_research_mgr *rs, int e) { return FIELD(rs, of_research_mgr_epoch + e * 0x10, int *); }
internal inline int *research_mgr_get_epoch_end(const ee_research_mgr *rs, int e) { return FIELD(rs, of_research_mgr_epoch + 0x4 + e * 0x10, int *); }
// the build plan's list nodes (civ_mgr_get_plan): +0 the next, +0xC / +0x10 the entry's values

// FUNCTIONS
// the next epoch attempt's time drawn (civilization manager +0x58: SetNextEpochAdvanceTime)

// the build manager places type at x, y (0, 0: where it finds room) - the original of econ.c's hook_place
// one building of type into the build manager's list (as the AI's own requests: (1, 0))
internal inline void build_mgr_request_one(ee_build_mgr *bm, int type) { fn_build_mgr_request(bm, type, 1, 0); }
// the resource manager's turn (the originals of the wiring's hooks, eeaimod.c): its gatherer targets computed (1: they
// changed), its citizens assigned, its update
// the citizen u to work at site (a resource, a field, a building); resource_mgr_assign_as: with the game's third argument
internal inline void resource_mgr_assign(ee_resource_mgr *rm, ee_unit *u, ee_unit *site) { fn_resource_mgr_assign_to_site(rm, u, site, 0); }
internal inline void resource_mgr_assign_as(ee_resource_mgr *rm, ee_unit *u, ee_unit *site, int c) { fn_resource_mgr_assign_to_site(rm, u, site, c); }
// the unit manager's citizens trained (the original of trace.c's hook)
// the unit manager's units of type: a std::deque of ee_unit *, NULL: none
// the unit orders (req: type, count ...) - the original of econ.c's hook_train_units
// tech id to research (requests its building if missing, adds its cost to the demand)
// count of the next epoch's missing requirements asked for (buildings, units, techs)
// enemies near x, y as the build manager sees them (its threat test)
// entry (type, x, y) into the build manager's list of epoch e (the lists at +4, 0x14 each)
internal inline void build_mgr_queue_add(ee_build_mgr *bm, int e, int *entry) { fn_bq_add(FIELD_AT(bm, of_build_mgr_queues + e * 0x14), entry); }
// the unit (its state, unit +0x30) in none of the combat manager's groups (combat_mgr_unit_free)
// a unit of type ordered but not started yet, one in production (the unit manager's queues)
// the unit manager's citizens, the citizens it wants (share x population cap: the original of citizens.c's hook)
// the fishing boat type of a player (the original of navy.c's hook)

// THE RESOURCE MANAGER'S SEARCHES (the originals of sites.c's hooks; arguments as the game passes them)
// the lists of fields, unbuilt fields and animals to hunt (n wanted, into out)

// HOOKS
// the resource manager's turn (the wiring, eeaimod.c): its gatherer targets, its citizens' assignment, its update

// THE RESOURCE MANAGER'S SEARCHES (sites.c)
typedef void *(__thiscall *farms_hook)(ee_resource_mgr *rm, uintptr_t n, uintptr_t out);
internal inline int patch_farms(farms_hook h, farms_hook unbuilt, int *of) {   // unbuilt NULL: the fields only
    if (of) *of += unbuilt ? 2 : 1;
    return patch_call(data_ai_managers_call_farms, (uintptr_t)fn_resource_mgr_farms, (void *)h) + (unbuilt ? patch_call(data_ai_managers_call_unbuilt_farms, (uintptr_t)fn_resource_mgr_unbuilt_farms, (void *)unbuilt) : 0);
}
// a citizen sent to work at a site (the calls of the assignment: calls_gather), the citizens trained (trace.c)
