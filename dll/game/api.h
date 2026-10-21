// THE GAME'S OBJECTS AND FUNCTIONS, TYPED: a pointer type of its own for each kind of object (fields.h's kinds) and
// the game's functions as plain C functions over them - explored_by(site, pl) instead of
// GF(FN_EXPLORED)(FIELD(site, OF(UNIT, 0x14), void *), pl). The types are never defined, only pointed to: a unit
// passed where a player is wanted, or the arguments swapped, doesn't build (a void * still passes - code not moved
// over yet). FIELD reads them as before. addr.h has the addresses and the raw signatures; the code outside game/
// calls these instead (moved over so far: ai/econ.c's calls; typed in every file: the players, the resource
// manager, the units). Each wrapper is the call itself: inlined, the machine code is the same as GF's
// (scripts/code_same.py; dll/build.sh's -fno-strict-aliasing keeps the types out of the optimizer's aliasing).

typedef struct ee_player ee_player;             // PL: a player (EEPlayer, EEComputerPlayer)
typedef struct ee_unit ee_unit;                 // UNIT: a unit, building or resource on the map
typedef struct ee_unit_type ee_unit_type;       // TD: a unit type's data (EEUnitClass)
typedef struct ee_goal ee_goal;                 // GOAL: a unit's goal (EEUnitGoal)
typedef struct ee_civ_mgr ee_civ_mgr;           // CM: the AI's civilization manager, player +0xB90
typedef struct ee_resource_mgr ee_resource_mgr; // RM: the AI's resource manager, CM +0x38
typedef struct ee_build_mgr ee_build_mgr;       // BM: the AI's build manager, CM +0x3C
typedef struct ee_research_mgr ee_research_mgr; // RS: the AI's research manager, CM +0x40
typedef struct ee_unit_mgr ee_unit_mgr;         // UM: the AI's unit manager, CM +0x44
typedef struct ee_tech_tree ee_tech_tree;       // TT: a player's tech tree, PL_TECHTREE
typedef struct ee_tech ee_tech;                 // TECH, TECHNODE, TECHCHILD: a tech tree node (EETechTreeNode)
typedef struct ee_tech_data ee_tech_data;       // TECHDATA: a tech's data, TECH +0x8
typedef struct ee_zone ee_zone;                 // ZONE: an AI town or resource zone

// the game's operator new: memory the game may free
static inline void *game_new(size_t n) { return GF(FN_NEW)(n); }

// PLAYERS
// the population: player +0xB10, with_list: and the units of its +0x18 list, with_aboard: and the units inside
// transports (type family 0x1D) - (1, 0) is what the AI's unit manager counts
static inline int population(ee_player *pl, int with_list, int with_aboard) { return GF(FN_POPULATION)(pl, with_list, with_aboard); }
// the game's unit limit (EEPlayer::vf31(0) called directly: a computer player's own vf31 returns its cap, +0xCCC)
static inline int game_unit_limit(ee_player *pl) { return GF(FN_GAME_LIMIT)(pl, 0); }
// can it build or train type now: available, affordable, a builder (0x20: as the AI's own checks ask)
static inline char can_build(ee_player *pl, int type) { return GF(FN_CAN_BUILD)(pl, type, 0x20, 0); }
// a foundation of type laid out at x, y (the player pays): the new building, NULL: none
static inline ee_unit *place_foundation(ee_player *pl, int type, int x, int y) { return GF(FN_PLACE_BUILDING)(pl, type, x, y); }
// is the map object explored by pl (a site's +0x14: its map object)
static inline char explored_by(ee_unit *site, ee_player *pl) { return GF(FN_EXPLORED)(FIELD(site, OF(UNIT, 0x14), void *), pl); }
// the highest epoch of the human players, 0 without
static inline int highest_human_epoch(void) { return GF(FN_HUMAN_EPOCH)(); }

// COMPUTER PLAYERS (the originals of econ.c's hooks)
static inline void cp_add_resource(ee_player *pl, int type, int amount) { GF(FN_CP_ADD_RESOURCE)(pl, type, amount); }
static inline void cp_pay(ee_player *pl, void *what, int b, int c) { GF(FN_CP_PAY)(pl, what, b, c); }
// the population cap that follows the humans, set every minute (player +0xCCC)
static inline void cp_update_pop_cap(ee_player *pl) { GF(FN_POP_CAP)(pl); }
// the game's catch-up: ai's next epoch attempt redrawn soon when human is ahead
static inline void cp_epoch_catch_up(ee_player *ai, ee_player *human) { GF(FN_EPOCH_CATCH_UP)(ai, human); }

// UNITS
// the player a unit belongs to (player 0: nature - wild animals, resources)
static inline ee_player *unit_owner(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x18), ee_player *); }
// the unit says sound (an answer to an order: 0x1F51 "building")
static inline void unit_say(ee_unit *u, int sound) { GF(FN_UNIT_SAY)(u, sound); }
// the unit's goal set (the unit takes it over)
static inline void unit_set_goal(ee_unit *u, ee_goal *g) { ((void (__thiscall *)(void *, void *, int, int))GA(FN_SET_GOAL))(u, g, 0, 0); }
// a build-or-repair goal constructed in mem (game_new(GA(SIZE_GOAL_BUILD))): pl's builder at building
static inline ee_goal *goal_build_init(void *mem, ee_player *pl, ee_unit *building, int flag) {
    return GF(FN_BUILD_GOAL)(mem, pl, building, flag);
}

// THE AI'S MANAGERS
// the build manager places type at x, y (0, 0: where it finds room) - the original of econ.c's hook_place
static inline char bm_place(ee_build_mgr *bm, int type, int x, int y) { return GF(FN_PLACE)(bm, type, x, y); }
static inline char bm_queued(ee_build_mgr *bm, int type) { return GF(FN_BM_QUEUED)(bm, type); }
// one building of type into the build manager's list (as the AI's own requests: (1, 0))
static inline void bm_request(ee_build_mgr *bm, int type) { GF(FN_BM_REQUEST)(bm, type, 1, 0); }
// the citizen u to work at site (a resource, a field, a building)
static inline void rm_assign(ee_resource_mgr *rm, ee_unit *u, ee_unit *site) { GF(FN_ASSIGN_TO_SITE)(rm, u, site, 0); }
// the unit manager's units of type: a std::deque of ee_unit *, NULL: none
static inline uint8_t *um_type_list(ee_unit_mgr *um, int type) { return GF(FN_UM_TYPE_LIST)(um, type); }
// the unit orders (req: type, count ...) - the original of econ.c's hook_train_units
static inline uintptr_t um_train(ee_unit_mgr *um, int *req, uintptr_t b) { return GF(FN_TRAIN_UNITS)(um, req, b); }
// tech id to research (requests its building if missing, adds its cost to the demand)
static inline void rs_schedule(ee_research_mgr *rs, int id) { GF(FN_RS_SCHEDULE)(rs, id); }
// count of the next epoch's missing requirements asked for (buildings, units, techs)
static inline void rs_request_requirements(ee_research_mgr *rs, int count) { GF(FN_REQUEST_TECHS)(rs, count); }
// a zone's site of kind (2: a resource site)
static inline ee_unit *zone_site(ee_zone *z, int kind) { return (ee_unit *)GF(FN_ZONE_SITE)(z, kind); }

// TECHS
static inline ee_tech *tech_lookup(ee_tech_tree *tt, int id) { return (ee_tech *)GF(FN_TECH_LOOKUP)(tt, id); }
// researched for nothing (EETechTreeNode::vf6) - the 2017 update's, AoC's takes an argument
static inline int tech_grant(ee_tech *t) { return GF(FN_TECH_GRANT)(t); }
// the epoch's line upgrades granted (node: the epoch's tech, cat 7); Art of Conquest: only_new - only the ones with
// data +0x81 set (econ.c PAID UPGRADES)
static inline void epoch_upgrades_grant(ee_tech *node, int cat, int only_new) {
    if (build_is("aoc")) ((void (__thiscall *)(void *, int, int))GA(FN_EPOCH_UPGRADES))(node, cat, only_new);
    else ((void (__thiscall *)(void *, int))GA(FN_EPOCH_UPGRADES))(node, cat);
}
