// THE MOVEMENT MODELS (EEPhysicsModel and its kinds: EEPMNaval, EEPMLand ...; one per kind of unit, shared): hooks.
// Each tick a model steers its unit towards the front waypoint (vtable +0x1C, the turn) and sets its speed (+0x20,
// EEPhysicsModel::vf8) from a decision (+0x24, FN_SPEED_DECIDE): 2 hold, 3 faster (up to unit_max_speed; on the
// last waypoint no faster than the distance), 4 slower (by the acceleration a tick, 0 below 0.005), else stop. The
// decision: more than 45 degrees off the bow (+-pi/4: 0x82C5C0, 0x82B668), slower - the unit crawls at its least
// speed while it turns, then speeds up; within, faster; slower too when the way left is no longer than it needs to
// stop. docs/engine-notes.md "Routes".

// HOOKS
// the speed decision of the ships' and the submarines' models (their vtables' slot; the land units', aircraft's and
// the others' keep the game's) - the hook calls model_speed_decide for what it leaves to the game
typedef int (__thiscall *speed_decide_hook)(void *model, ee_unit *u, float dist, float angle);
internal inline int patch_ship_speed_decide(speed_decide_hook h) {
    return patch_vtable(data_movement_vt_pm_naval, value_movement_slot_pm_speed_decide, (uintptr_t)fn_model_speed_decide, (void *)h)
         + patch_vtable(data_movement_vt_pm_submerged, value_movement_slot_pm_speed_decide, (uintptr_t)fn_model_speed_decide, (void *)h);
}
// the turn decision (the turn step 0x548266 asks it each tick with the unit's turn rate now, +0x17C, and the angle
// left, both signed): 3 turn faster towards it (up to FN_UNIT_MAX_TURN), 4 slower (by the turn acceleration a tick, 0
// at the end), 2 hold. The 2017 update's takes both as sizes and doesn't look a step ahead, so a unit swings past the
// heading and back; AoC's (slot 12) has both - not patched there (0). Patched in every model whose turn is the shared
// FN_TURN_TOWARDS (slot 7: ships, submarines and 7 others - land units turn at a fixed rate in their own tick, without
// it), slot 11 - not where some models hold it in slot 10 too (another axis's turn, 0x5480A2, which AoC kept): their count
typedef int (__thiscall *turn_decide_hook)(void *model, ee_unit *u, float rate, float left);
internal inline int patch_turn_decide(turn_decide_hook h) { return build_is("aoc") ? 0 : patch_vtables_with((uintptr_t)fn_turn_towards, value_movement_slot_pm_turn_towards, value_movement_slot_pm_turn_decide, (uintptr_t)fn_model_turn_decide, (void *)h); }
// the tick's turn towards a point (slot 7: the front waypoint, from EEPMNaval::vf1, which measures the arrival to
// the waypoint itself): its outputs the angle left (the speed decision's angle), the turn rate and the new heading
typedef void (__thiscall *turn_towards_hook)(void *model, ee_unit *u, float *pos, float *target, float *left, float *rate, float *heading, char facing);
internal inline void model_turn_towards(void *model, ee_unit *u, float *pos, float *target, float *left, float *rate, float *heading, char facing) {
    fn_turn_towards(model, u, pos, target, left, rate, heading, facing);
}
internal inline int patch_ship_turn_towards(turn_towards_hook h) {
    return patch_vtable(data_movement_vt_pm_naval, value_movement_slot_pm_turn_towards, (uintptr_t)fn_turn_towards, (void *)h)
         + patch_vtable(data_movement_vt_pm_submerged, value_movement_slot_pm_turn_towards, (uintptr_t)fn_turn_towards, (void *)h);
}
internal inline int patch_land_turn_towards(turn_towards_hook h) {   // the land units' and the hovercraft's (EEPMLand::vf1 calls it too)
    return patch_vtable(data_movement_vt_pm_land, value_movement_slot_pm_turn_towards, (uintptr_t)fn_turn_towards, (void *)h)
         + patch_vtable(data_movement_vt_pm_hover, value_movement_slot_pm_turn_towards, (uintptr_t)fn_turn_towards, (void *)h);
}
// the tick's speed step (slot 8: FN_SPEED_DECIDE asked with the same distance and angle; on the last waypoint it
// speeds up no further than the distance, and the decision brakes when the way left is within the stopping distance)
typedef float (__thiscall *speed_step_hook)(void *model, ee_unit *u, float dist, float angle);
internal inline int patch_ship_speed_step(speed_step_hook h) {
    return patch_vtable(data_movement_vt_pm_naval, value_movement_slot_pm_speed_step, (uintptr_t)fn_model_speed_step, (void *)h)
         + patch_vtable(data_movement_vt_pm_submerged, value_movement_slot_pm_speed_step, (uintptr_t)fn_model_speed_step, (void *)h);
}
// the collision step (slot 18, after each step of the movement: docs/engine-notes.md "Collision") of the ships',
// the submarines', the land units' and the hovercraft's models (all hold the same thunk; the submarines' slot 14 tests
// the tiles for other submarines) - the hook calls model_collide for what it leaves to the game, model_push to move
// the unit by its push velocity (the model's own: the terrain and buildings tested, the larger axis no faster than
// the top speed, under 0.02 tiles/s nothing; the push zeroed)
typedef void (__thiscall *collide_hook)(void *model, ee_unit *u);
internal inline void model_push(void *model, ee_unit *u) { ((void (__thiscall *)(void *, ee_unit *))(*(void ***)model)[value_movement_slot_pm_push])(model, u); }
// which of them a model is: MODEL_SHIPS, MODEL_SUBS, MODEL_LAND (the land units' and the hovercraft's), else 0
enum { MODEL_SHIPS = 1, MODEL_SUBS, MODEL_LAND };
internal inline int model_kind(const void *model) {
    uintptr_t vt = model ? *(const uintptr_t *)model : 0;
    return vt == data_movement_vt_pm_naval ? MODEL_SHIPS : vt == data_movement_vt_pm_submerged ? MODEL_SUBS : vt == data_movement_vt_pm_land || vt == data_movement_vt_pm_hover ? MODEL_LAND : 0;
}
internal inline int patch_collide(int kind, collide_hook h) {   // a kind's models
    if (kind == MODEL_SHIPS) return patch_vtable(data_movement_vt_pm_naval, value_movement_slot_pm_collide, (uintptr_t)fn_model_collide, (void *)h);
    if (kind == MODEL_SUBS) return patch_vtable(data_movement_vt_pm_submerged, value_movement_slot_pm_collide, (uintptr_t)fn_model_collide, (void *)h);
    return patch_vtable(data_movement_vt_pm_land, value_movement_slot_pm_collide, (uintptr_t)fn_model_collide, (void *)h)
         + patch_vtable(data_movement_vt_pm_hover, value_movement_slot_pm_collide, (uintptr_t)fn_model_collide, (void *)h);
}
