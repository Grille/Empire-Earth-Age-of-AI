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

// Kinds not typed yet (attack groups and subgroups, goals, messages, the game state's tables) are void * here.

// THE GAME
// the game's operator new: memory the game may free
static inline void *game_new(size_t n) { return GF(FN_NEW)(n); }
// a value of the game's table of settings (0x16: the gatherers a site takes)
static inline unsigned table_value(int key) { return GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), key); }
// the player object of a player index (the game state's, humans too)
static inline ee_player *game_player(int index) { return GF(FN_GAME_PLAYER)((void *)GA(G_GAME_STATE), index); }
// the stance of player index from towards to (the game's own change, with its message)
static inline void game_set_stance(int from, int to, int stance) { GF(FN_SET_STANCE)((void *)GA(G_GAME_STATE), from, to, stance); }
// player index out of the game (the resign's first step)
static inline void game_player_out(int flag, int index) { GF(FN_GAME_PLAYER_OUT)((void *)GA(G_GAME_STATE), flag, index); }
// the citizens working site (all players)
static inline unsigned site_gatherers(ee_unit *site) { return GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), site); }
// the map's area id at pt (kind 1 water, 2 land): one id per connected region
static inline unsigned short map_area(int *pt, int kind) {
    return ((unsigned short (__thiscall *)(void *, int *, int))GA(FN_REGION))((void *)GA(G_MAP_AREAS), pt, kind);
}
// the damage bonus of an attacker type (data atd) against a target type (data ttd), the players' indices
static inline float attack_bonus(uint8_t *atd, uint8_t *ttd, int api, int tpi) {
    return GF(FN_ATTACK_BONUS)((void *)GA(G_BONUS_TABLE), FIELD(atd, OF(TD, 0x4C8), int), ttd, api, tpi);
}
// the local player resigns (the menu's message sent), the exit dialog's confirmation (the originals of diplo.c's hooks)
static inline void resign_send(void) { GF(FN_RESIGN_SEND)(); }
static inline void exit_confirm(void) { GF(FN_EXIT_CONFIRM)(); }
// the network messages: executed (the originals of diplo.c's hooks), a chat line built in mem, one sent to everyone
static inline void msg_resign(void *msg) { GF(FN_RESIGN)(msg); }
static inline void msg_pay_tribute(void *msg) { GF(FN_PAY_TRIBUTE)(msg); }
static inline void msg_ingame_chat(void *msg) { GF(FN_INGAME_CHAT)(msg); }
static inline void *msg_chat_init(void *mem, void *wide_text) { return GF(FN_CHAT_MSG_CTOR)(mem, wide_text); }
static inline void server_send_all(void *net, void *msg) {
    ((void (__thiscall *)(void *, void *, int))(*(void ***)net)[GA(SLOT_SERVER_SEND)])(net, msg, 0xFF);
}
// a statistics counter n of the stats object +d (the kill counter's original: diplo.c)
static inline void stat_add(void *stats, int n, int d) { GF(FN_STAT_ADD)(stats, n, d); }

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
// can a building of type stand at x, y (size x size tiles: the game's placement test, every tile)
static inline char can_place_area(ee_player *pl, int type, int x, int y, int size) { return GF(FN_CAN_PLACE)(pl, type, x, y, size, size, 0); }
// the player's per-type stats records (player +0x874, a vector): their count, the record of type
static inline unsigned type_stats_count(ee_player *pl) { return GF(FN_VECTOR_SIZE)(FIELD_AT(pl, OF(PL, 0x874))); }
static inline uint8_t **type_stats(ee_player *pl, int type) { return GF(FN_TYPE_STATS)(FIELD_AT(pl, OF(PL, 0x874)), type); }
// the player resigns (how: 2 as the resign message does)
static inline void player_resign(ee_player *pl, int how) { GF(FN_PLAYER_RESIGN)(pl, how); }
// the player's attacks on target called off (the units back to the pool)
static inline void call_off_attacks(ee_player *pl, ee_player *target, int flag) { GF(FN_CALL_OFF_ATTACK)(pl, target, flag); }
// the combat manager (player +0xB8C) answers an attack by target (the game's revenge attack)
static inline void revenge(void *combat_mgr, ee_player *target) { GF(FN_REVENGE)(combat_mgr, target); }
// units of the player deleted to make room for req (the original of fleet.c's hook)
static inline int delete_for_room(ee_player *pl, void *req, int flag) { return GF(FN_DELETE_FOR_ROOM)(pl, req, flag); }

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
// the unit's goal set (the unit takes it over); append: after its current one, prio: the order's priority
static inline void unit_set_goal(ee_unit *u, void *goal, int append, int prio) {
    ((void (__thiscall *)(void *, void *, char, int))GA(FN_SET_GOAL))(u, goal, (char)append, prio);
}
// the goal given into one of the unit's goal slots (the original of landing.c's unload retry)
static inline void unit_give_goal(ee_unit *u, void *goal, int slot) { GF(FN_GIVE_GOAL)(u, goal, slot); }
// the units aboard a transport or inside a building
static inline int units_inside(ee_unit *u) { return GF(FN_UNITS_INSIDE)(u); }
// the distance between two units (tiles)
static inline float unit_distance(ee_unit *a, ee_unit *b) { return GF(FN_UNIT_DISTANCE)(a, b); }
// the unit changed in place into the type of new_td, owner player index pi (defence.c: a line upgrade)
static inline int unit_convert(ee_unit *u, int pi, uint8_t *new_td) { return GF(FN_UNIT_CONVERT)(u, pi, (uint32_t)new_td); }
// the unit taken out of the map's unit list, given back to the game's pool (the originals of attack.c's guards)
static inline void unit_unlink(ee_unit *u) { GF(FN_UNIT_UNLINK)(u); }
static inline void unit_release(ee_unit *u) { GF(FN_UNIT_RELEASE)(u); }
// the unit's type told that the unit got a goal (EEUnitClass's vtable, as the game's own goal assignment does)
static inline void type_goal_assigned(uint8_t *td, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)td)[GA(SLOT_TYPE_ASSIGN)])(td, u); }
// a model's damage points (the original of defence.c's guard)
static inline int damage_points(void *model, int index, unsigned state) { return GF(FN_DAMAGE_POINTS)(model, index, state); }

// GOALS: built in game_new(GA(SIZE_GOAL_..)) memory, then given with unit_set_goal
// build or repair: pl's builder at building
static inline void *goal_build_init(void *mem, ee_player *pl, ee_unit *building, int flag) { return GF(FN_BUILD_GOAL)(mem, pl, building, flag); }
// attack target (Art of Conquest's constructor takes a byte more: 0)
static inline void *goal_attack_init(void *mem, ee_unit *u, ee_unit *target) {
    if (build_is("aoc")) return ((void *(__thiscall *)(void *, void *, void *, int, int))GA(FN_GOAL_ATTACK_CTOR))(mem, u, target, 1, 0);
    return ((void *(__thiscall *)(void *, void *, void *, int))GA(FN_GOAL_ATTACK_CTOR))(mem, u, target, 1);
}
// the units of vec (a std::vector) into holder (a transport or a building)
static inline void *goal_load_init(void *mem, ee_unit *holder, void *vec) { return GF(FN_GOAL_LOAD_CTOR)(mem, holder, vec, 1); }
// the holder's units out at x, y
static inline void *goal_unload_init(void *mem, ee_unit *holder, int x, int y, int flag) { return GF(FN_GOAL_UNLOAD_CTOR)(mem, holder, x, y, flag); }
// a move to xy (float[2])
static inline void *goal_move_init(void *mem, float *xy) { return GF(FN_GOAL_MOVE_CTOR)(mem, xy, 1, 1, 0, 0.0f, 0, 0); }
// a load goal's part the units walk into (the goal's vtable)
static inline void *goal_part(void *goal) { return ((void *(__thiscall *)(void *))(*(void ***)goal)[GA(SLOT_GOAL_PART)])(goal); }
// may the goal be handed to the unit (the original of attack.c's hook_goal_cast)
static inline char goal_fits(void *goal, void *unit) { return GF(FN_GOAL_FBA_CHECK)(goal, unit); }
// a goal set's answer for the unit (the original of landing.c's hook)
static inline int goalset_ask(void *set, ee_unit *u, int *out) { return GF(FN_GOALSET_ASK)(set, u, out); }

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
// enemies near x, y as the build manager sees them (its threat test)
static inline char bm_enemy_near(ee_build_mgr *bm, int x, int y) { return ((char (__thiscall *)(void *, int, int))GA(FN_ENEMY_NEAR))(bm, x, y); }
// entry (type, x, y) into the build manager's list of epoch e (the lists at +4, 0x14 each)
static inline void bm_queue_add(ee_build_mgr *bm, int e, int *entry) { GF(FN_BQ_ADD)(FIELD_AT(bm, OF(BM, 0x4) + e * 0x14), entry); }
// a wall of type from a to b (int[2] each) by the wall manager
static inline void build_wall(void *wall_mgr, int *a, int *b, int type) { GF(FN_BUILD_WALL)(wall_mgr, a, b, type, 0); }
// the unit (its state, unit +0x30) in none of the civilization manager's groups
static inline char cm_unit_free(void *cm, void *unit_state) { return GF(FN_CM_UNIT_FREE)(cm, unit_state); }
// a unit of type ordered but not started yet, one in production (the unit manager's queues)
static inline char um_pending(ee_unit_mgr *um, int type) { return ((char (__thiscall *)(void *, int))GA(FN_UM_PENDING))(um, type); }
static inline char um_in_production(ee_unit_mgr *um, int type) { return ((char (__thiscall *)(void *, int))GA(FN_UM_IN_PRODUCTION))(um, type); }
// the unit manager's citizens, the citizens it wants (share x population cap: the original of citizens.c's hook)
static inline int um_citizens(ee_unit_mgr *um) { return GF(FN_UM_CITIZENS)(um); }
static inline int um_citizens_wanted(ee_unit_mgr *um) { return GF(FN_UM_CITIZENS_WANTED)(um); }
// the shore an idle transport unloads at (the original of landing.c's hook)
static inline int shore_to_unload(void *mgr, ee_unit *transport) { return GF(FN_SHORE_TO_UNLOAD)(mgr, transport); }
// the fishing boat type of a player (the original of navy.c's hook)
static inline int fish_type(void *o) { return GF(FN_FISH_TYPE)(o); }

// THE RESOURCE MANAGER'S SEARCHES (the originals of sites.c's hooks; arguments as the game passes them)
static inline ee_unit *rm_find_mine(ee_resource_mgr *rm, uintptr_t type, ee_unit *unit, uintptr_t rev) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t, uintptr_t))GA(FN_FIND_MINE))(rm, type, (uintptr_t)unit, rev);
}
static inline ee_unit *rm_find_food(ee_resource_mgr *rm, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t, uintptr_t))GA(FN_FIND_FOOD))(rm, need_drop, near_base, (uintptr_t)unit);
}
static inline ee_unit *rm_find_wood(ee_resource_mgr *rm, uintptr_t flag, ee_unit *unit) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t))GA(FN_FIND_WOOD))(rm, flag, (uintptr_t)unit);
}
// the lists of fields, unbuilt fields and animals to hunt (n wanted, into out)
static inline void *rm_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t))GA(FN_FARMS))(rm, n, out);
}
static inline void *rm_unbuilt_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t))GA(FN_UNBUILT_FARMS))(rm, n, out);
}
static inline void *rm_hunt(ee_resource_mgr *rm, uintptr_t n, uintptr_t f, uintptr_t out) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t, uintptr_t))GA(FN_HUNT))(rm, n, f, out);
}

// ATTACK GROUPS AND SUBGROUPS (the originals of attack.c's and landing.c's hooks)
static inline char unit_to_group(void *cmd, ee_unit *u) { return GF(FN_UNIT_TO_GROUP)(cmd, u); }
static inline void group_stat(void *stats, ee_unit *u) { GF(FN_GROUP_STAT)(stats, u); }
// the units b..e sorted by distance to unit (the targets' range sort)
static inline void range_sort(void **b, void **e, ee_unit *unit) { GF(FN_RANGE_SORT)(b, e, unit); }
static inline void asg_update(void *sg, int now) { GF(FN_ASG_UPDATE)(sg, now); }
static inline void asg_sea_update(void *sg, int now) { GF(FN_ASG_SEA_UPDATE)(sg, now); }
static inline void asg_sea_transport_update(void *sg, int now) { GF(FN_ASG_SEA_TRANSPORT_UPDATE)(sg, now); }
static inline void asg_air_update(void *sg, int now) { GF(FN_ASG_AIR_UPDATE)(sg, now); }
static inline void asg_stats(void *sg) { GF(FN_ASG_STATS)(sg); }
static inline void asg_land_stats(void *sg) { GF(FN_ASG_LAND_STATS)(sg); }
static inline char asg_give_goal(void *sg, ee_unit *u, void *goal, int a, int prio) { return GF(FN_ASG_GIVE_GOAL)(sg, u, goal, a, prio); }
static inline char asg_air_give_goal(void *sg, ee_unit *u, void *goal, int a, int prio) { return GF(FN_ASG_AIR_GIVE_GOAL)(sg, u, goal, a, prio); }
// a land subgroup's route from start to dest (int[2] each), its arrival (waypoints at +0x60)
static inline void asg_route(void *sg, int *start, int *dest) { ((void (__thiscall *)(void *, int *, int *))GA(FN_ASG_ROUTE))(sg, start, dest); }
static inline void asg_route_done(void *sg) { ((void (__thiscall *)(void *))GA(FN_ASG_ROUTE_DONE))(sg); }
// a subgroup member's score for an enemy (info: what it scores)
static inline float asgu_score(void *member, uint8_t *info) { return ((float (__thiscall *)(void *, uint8_t *))GA(FN_ASGU_SCORE))(member, info); }
// the route planner's cost of an edge into node
static inline int route_cost(void *planner, int *edge, uint8_t *node) {
    return ((int (__thiscall *)(void *, int *, uint8_t *))GA(FN_ROUTE_COST))(planner, edge, node);
}

// TECHS
static inline ee_tech *tech_lookup(ee_tech_tree *tt, int id) { return (ee_tech *)GF(FN_TECH_LOOKUP)(tt, id); }
// can it be researched now (EETechTreeNode's vtable)
static inline char tech_available(ee_tech *t) { return ((char (__thiscall *)(void *))(*(void ***)t)[GA(SLOT_TECH_AVAILABLE)])(t); }
// researched for nothing (EETechTreeNode::vf6) - the 2017 update's, AoC's takes an argument
static inline int tech_grant(ee_tech *t) { return GF(FN_TECH_GRANT)(t); }
// the epoch's line upgrades granted (node: the epoch's tech, cat 7); Art of Conquest: only_new - only the ones with
// data +0x81 set (econ.c PAID UPGRADES)
static inline void epoch_upgrades_grant(ee_tech *node, int cat, int only_new) {
    if (build_is("aoc")) ((void (__thiscall *)(void *, int, int))GA(FN_EPOCH_UPGRADES))(node, cat, only_new);
    else ((void (__thiscall *)(void *, int))GA(FN_EPOCH_UPGRADES))(node, cat);
}
