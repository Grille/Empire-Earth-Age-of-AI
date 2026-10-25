// THE SCENARIO EDITOR'S LISTS (scenario.c's objects hold them): spec/game/scenario_lists.cs has them, gen/enums.c
// makes the enums; here what isn't a list of values - a player by number, the class filter's ranges by build - and
// the lists' checks. Included by api.c after gen/enums.c, ahead of gen/gen.c.

#define SM_PLAYER(n) ((enum sm_player_spec)(SM_PLAYER_0 + (n) - 1))   // "Player n", 1-based as the editor counts
// an object's class filter (sm_class): any, an epoch (1..14, AoC 1..15), then a family, an RPS class, a unit group -
// their ranges move with AoC's 15th epoch
internal inline enum sm_class sm_class_epoch(int epoch) { return (enum sm_class)epoch; }
internal inline enum sm_class sm_class_family(int family) { return (enum sm_class)((int)value_scenario_lists_sm_class_family + family); }
internal inline enum sm_class sm_class_rps(int rps) { return (enum sm_class)((int)value_scenario_lists_sm_class_rps + rps); }
internal inline enum sm_class sm_class_group(int group) { return (enum sm_class)((int)value_scenario_lists_sm_class_group + group); }
_Static_assert(SM_UA_CAMERA_FOCUS == 41 && SM_UA_TOTAL_PLACE_HOLDER3 == 74, "the unit attributes are strings 29100..29174");
_Static_assert(SM_CAL_BATTLECRY_9 == 22 && SM_CAL_SPECIAL_LOCATION == 35, "dbcalamity.dat has 23 records, AoC's 36");
