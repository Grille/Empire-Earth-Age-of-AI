// THE WRECKS' FALL (collect_remains.c includes it after the corpses' entries): a ship's or an aircraft's wreck as
// points on a frame - their forces, the water's and the air's, the ground, their pose and look
// THE SHIPS SINKING ([display] remains_physics, with remains_sink; the player, 2026-11-11: "i wonderd if we could make sinking
// ships more intretsing. maybe 4 points (one front, one back, and 2 in the middle to keep it upright) - each point has a
// buoyancy, 1 to 3 point get a negative buoyancy ... ships that sink similar to now, ships that fall over etc", the
// outcomes weighted, a wreck half out of the water in the shallows welcome). The game sinks every ship alike - it moves
// the unit itself, its animation still: turned about one axis to 36 degrees in 2 s, down at 0.28 tiles a second to
// 1.3 tiles under, 2 s still rotting, gone (logged 2026-11-11, a frigate: state 6000, then the rotting 6002 with a rot
// time of 2 s). Here a dead ship (its hit points at 0) is drawn by four floats of its own from its first sight on: bow and
// stern 0.8 of the way to its hull's ends along its longer side (the frame's bounding box, GEModel boxes), port and
// starboard as far to its sides, at its waterline; a rigid frame (their six distances held) under gravity, each float
// lifted by its buoyancy times how deep it lies (0 dry, 1 fully under over range: half under at rest), slowed by the
// water's drag, and stopped by the seabed under it at its keel's depth. Their buoyancy leaks away: all of them slowly to
// less than their weight (it goes down in the end), and by the plan of its serial (the same on every PC, the picture only)
// one or two fast to below nothing - a dive by the bow or the stern two thirds of the time (now and then with a list), a
// roll onto one side a third (the other side holding on longest). A quarter went down level until 2026-11-12 (the
// player: "a ship that sits level is indisguingisble from a alive one, but bow or stern must dip and/or its falling
// over"). Stepped by the game time (stands while paused),
// US_DT a step; kept after the game removes it as the other corpses, its time theirs from its death.
// A ship's fifth way (the player, 2026-11-12: "what would it take for a wrek to stqand up when sinking?" - "the middle
// buoyant so it stay at the waterline, with the other side rasing in the air"): a seesaw, US_SEESAW of them, the four
// above sharing the rest as before - its bow or its stern flooded at once to a little below nothing, its sides lifting
// US_SEESAW_LIFT (the middle carries the whole ship and that end's pull at the waterline: at 2 it went under with it)
// until they leak, late and slowly - one side from 3 - 6 s, the other 3 - 6 s after it, each over 14 - 22 s (both from
// 8 - 15 s over 10 - 20 until 2026-11-13, the player: "the seesaw looks a bit stiff, the middle should ease into the
// sinking a bit earlier", "also make one point fail before the other": it heels to that side as it settles) -, the
// other end's slow leak as any; the flooded end goes down, the other rises out of the water, it hangs about its middle
// and slides under. Not a submarine's or an aircraft's. Each float goes into
// the seabed by a depth of its own, up to US_DIG of the beam by the same plan (the player, the same day: "each point
// should have a random value how deep it can sink into the ground"): a wreck comes to rest unevenly buried, tilted.
// It goes on at the speed it had when it died (the player: "also let the unit keep its velocity on death"): the floats
// start with its last step's move along the water (us_velocity), the drag slows it, the seabed stops it.
// A fifth point, its ballast (the player: "a thried point in the middle that a bit lower than the others with a heigher
// wieght, to simulate a real ships balance better"): under its middle at US_LOW of the keel, US_BALLAST times a float's
// weight, no buoyancy of its own - the floats carry it (their lift times all the weight over theirs: half under at rest
// as before, the leaks' shares of the weight as before). Its weight below the floats rights a heeling hull; the frame's
// distances are held by the points' weights (the heavy one moves the least). Heeled past US_SHIFT the ballast breaks
// loose and slides to the low side over US_SHIFT_S, as a cargo shifts, and the hull stays over: held in the middle it
// rights every rolled ship as it fills (tried: all ten of a test came to rest upright, their sails out of the water).
#define US_PTS 5        // the four floats and the ballast (the last)
#define US_LOW 0.6f     // the ballast under the waterline: a share of the keel's depth
#define US_BALLAST 3.0f // its weight, a float's 1
#define US_SHIFT 0.8f   // the heel (its sine: 53 degrees) past which the ballast slides
#define US_SHIFT_S 1.5f // ... over this long (s), 0.8 of the way to the low side
#define US_SEESAW (1.0f / 6.0f)   // the ships that go down as a seesaw
#define US_SEESAW_LIFT 3.0f        // its side floats' buoyancy, fully under (g's), until they leak
// A SHIP BREAKING IN TWO (the player, 2026-11-13: "could ships split in halp on sink?", "short ones like fisihing boats
// shouldnt snap"): a ship US_SNAP_LEN tiles long or more draws it, US_SNAP of them (the pipe's wreck snap all / none /
// random), before the other plans. Kicked at its death (us_kick), US_SNAP_AT .. + US_SNAP_AT_RND s after it - the kick's
// turn begun, the halves take it on - it breaks (the player, the same day: "make the ship break the moment it dies after
// the kick"; 1.5 .. 3.5 s before, settling with slow leaks) at 0.45 .. 0.55 of its length: its entry the bow's half
// from then on and a new one the stern's (twin), each with four floats and its ballast of its own over its own stretch of the hull - as far as its
// hull's ends from its middle as a whole ship's - and all the rest the ship's, its pose and way at the break (both
// halves' points from the frame's pose then and a step before). The broken end's float floods at once, its far end
// holds longest, its sides between: each half stands up on its broken end and goes down by it; their broken ends drop
// (US_SNAP_DROP) and they drift apart (US_SNAP_PART). Each half drawn as a model of its own, made from the ship's when it is
// first drawn broken (render_units.c A BROKEN SHIP'S HALF: us_clip_tag). Not an aircraft's (a submarine's by its own shares: A SUBMARINE
// BREAKING); one met already sinking stays whole.
#define US_SNAP 0.3f
#define US_SNAP_LEN 1.5f
#define US_SNAP_AT 0.25f
#define US_SNAP_AT_RND 0.15f
#define US_SNAP_DROP 0.3f   // tiles/s
#define US_SNAP_PART 0.12f  // tiles/s, each away from the other
// BREAKING ON THE SEABED (the player, 2026-11-13: "what do you think about a 50% chance a ship breaks apart after it hit
// seafloor?" - "yes"): a ship long enough that doesn't break at its death draws it again, US_SNAP_FLOOR of them (the
// pipe's wreck floor all / none / random), and breaks at its first touch of the bottom going down faster than US_FLOOR_V
// (one that settles gently stays whole), lying as it hit: its halves go on as broken halves do (their broken ends
// flooded, their far ends holding: one may stand up again - flooded through at once they dropped, jarring: the player,
// "dont do that ... them standing up etc based on their simulation isnt nercesarly a problem"), their broken ends sunk US_FLOOR_SAG deeper into the seabed, one side of each a little (a list), drifting apart
#define US_SNAP_FLOOR 0.5f
// A SUBMARINE BREAKING (the player, 2026-11-13: "next subs a 50% chance to break apart on death (a modifer not an extra
// mode) and a 100% when they land on the floor"): as a ship's, by these shares, its own way down kept (its leaks; it
// hangs at its depth until they take it down)
#define US_SNAP_SUB 0.5f
#define US_SNAP_FLOOR_SUB 1.0f
#define US_FLOOR_V 0.05f    // tiles/s
#define US_FLOOR_SAG 0.12f  // tiles
#define US_FLOOR_STEPS 10  // the steps after its hit it breaks: the hit's change of its way and turn gone through all its
                           // points (the player, the same day, the halves broken at the hit: "the part hitting the floor
                           // overreacts and the other continues its part usualy sliding into the other piece" - "break
                           // the ship apart a few steps after the sea floor hit, than the new speed rotation etc could
                           // travel trough all points": the hit stopped the one float that touched, and the halves'
                           // poses made of that turned that one too hard while the other kept all its way)
internal int g_us_floor;   // the pipe's: 1 every ship long enough that doesn't break at its death breaks on the seabed, -1 none, 0 drawn
// its hull's top: this share of its model's height over the water, at most US_HULL_MAX (the models' own space: a
// sailing ship's 0 .. 1 to its masts' tops, a modern one's 0 .. 0.35 - 0.55; an Imperial battleship's lower sails hang
// to 0.2) - a piece of the model reaching under it along most of the ship is the hull, cut; one that doesn't (a sail,
// a mast, an oar) goes whole to one half
#define US_HULL_TOP 0.3f
#define US_HULL_MAX 0.15f
internal int g_us_snap;     // the pipe's: 1 every ship long enough breaks, -1 none, 0 drawn (US_SNAP)
// a ship's leaks a second on at its death (the player, 2026-11-12: "most shift take a second before they sink, makes
// sence, looks good, but sadly reduces readabilithy... maybe skip a second of the flooding simulation so tzhat certan
// point will drag down imediatly?"): its fast leak pulls at once, the slow ones a second sooner; an aircraft's clock
// from its first touch of the water as before
#define US_LEAK_HEAD 1.0f
// The blow that kills it (the player, the same day: "on death an units should get a bit of random rotation for a start.
// even if it levels out again, the kick is suposed to make clearer something happend"): it starts turning about its
// own axes, half to all of these (radians/s) by its serial, either way - the buoyancy and the drag level it out again
#define US_KICK_ROLL 0.8f    // about its length: a heel of up to about 25 degrees
#define US_KICK_PITCH 0.35f  // about its beam
#define US_KICK_YAW 0.3f     // about its up: slewed round
// an aircraft's (the player: "helicopters get a strong yaw kick, and airplanes an roll kick"): nothing in the air holds it
// back, the turn goes on until it hits - a helicopter spins down, an airplane rolls over
#define US_KICK_HELI_YAW 1.0f    // a start: its spin builds up as it falls (US_HELI_TWIST; 4 until 2026-11-12)
#define US_KICK_PLANE_ROLL 0.5f   // 2.5 until 2026-11-12: now its controls and its own turn (us_spin) show the hit
#define US_KICK_AIR_OTHER 0.2f   // its other two axes
// gravity (tiles/s^2); the ships' 3 until 2026-11-12, then 1 for the player to try ("just set it to 1 for a quick test,
// i wnat to play around with it"): buoyancy scales with it (the same depth afloat), the water's drag and the planing
// don't - sinking about 3 times slower, heeling and bobbing about 1.7, a ship under way rearing about 3 times higher
#define US_G 1.0f
// an aircraft's (the player, the same day: "reduce grafithy to 1 tile/s"): it falls slower, its spin or roll and its
// planing have longer to show before it hits; its float in the water scaled alike (buoyancy is lift x gravity)
#define US_G_AIR 1.0f
#define US_DT 0.01      // the step (s of game time)
// The water's drag by the hull's own axes (the player, the same day: "what do you think of adding a lift plane? ... real
// ships glide when they sink right?"): a hull slips along its length and is held across it - sideways, and through its
// deck and keel - so one going down by the bow or the stern glides on along its line instead of dropping (the Titanic's
// bow came to rest about 600 m from its stern), one that sinks level goes straight down, and its way at its death
// carries it on where its bow points (a slew of the kick doesn't drift it sideways). Until then by the world's axes:
// 0.5 along the water, 1.6 up and down. The air's alike every way.
#define US_DRAG 1.6f    // the water's across the hull (1/s), along its length, the air's
#define US_ALONG 0.35f  // a frigate at 0.8 tiles/s at its death coasts about 2.3 tiles
// Its turning held harder still (the player: "the plane should heavely discurage quick rotation underwater right?"):
// a turning hull sweeps its whole flanks through the water, its ends the most - its points' way about their middle
// (by their weights; the way of the whole kept) slowed by US_TURN times how much of it is under. With the drag
// alone a leaking bow pivoted a frigate on end in about a second, and a diver at a slant now stays slanted longer.
#define US_TURN 8.0f    // 1/s, wholly under (4 hardly slowed the pivots)
// A hull under way lifts its leading end (the player: "how about an slight upwards rotation scaled by speed?"): as a
// boat planes, the water pushes the end it moves toward up and the other down - a pair, nothing lifted as a whole -
// by its way along its length, the bow going ahead, the stern going astern, as far as each end is under. A ship
// killed under way rears a little and settles as it slows; a dive's glide is flattened a little.
#define US_RISE 3.0f    // the push (tiles/s^2) per tile/s: about 5 degrees at a frigate's 0.8 tiles/s
#define US_RISE_TOP 2.0f   // tiles/s: no more than this counted (a ship of 170 rears, it doesn't flip)
// AN AIRPLANE'S WINGS AND FINS in the air (the player, the same day: "lets disable the plaing for planes in the air,
// underwater it could make for a nice dive"; then "keep the middle point as a coliider only, no lift. add 4 new points,
// two for each horizontal wing and one for the yaw wing. they are towards the back of the plane. the 4 point is a pure
// weight placed in the front", and "2 extra point for the main wings? along the center of weight? the ideas is that the
// wing points can be disabled or weakend randomly, to simulate failing wings"). Each surface point cancels its own way
// through its surface (its up: length x span; the fin's: its span) as far as it is dry - each point on its own, so the
// surfaces turn it: a dart, its weight ahead of its fins, turns its nose into its way; it falls as a thing thrown, its
// nose following the curve. The wings are its side floats (at the middle of its length: no pitch of their own), so a
// sinking plane is held up and noses over slowly - a glide that steepens; a weak wing drops, it rolls toward it and
// spirals in; both whole hold a roll back (the kick's levels out). No planing in the air (in the water as a ship's).
#define US_PTS_AIR 9    // the floats, the ballast; the tailplane's port and starboard points, the fin's, the nose weight
#define US_WING 16.0f   // a wing point's (1/s, wholly in the air)
#define US_TAILPLANE 12.0f
#define US_FIN 12.0f
// A surface's push is across the plane's flight path, a share of it braking (the player, 2026-11-12: "roll is very
// unstable, instead of rolling properly it start to tumble very quickly", "its the base 4", "also it stillloses speed
// on death"): pushed along its own up, a surface tilted by a degrees braked the plane by sin(a)^2 of its way - at
// roll 45 two flat plates into the wind, 45% of its energy gone in 2.5 s, and with the base the two wings at 49 and
// -41 degrees braked unequally (0.57 against 0.43) and yawed it into a tumble (logged, P-51s in level flight). Now
// the part of each push along the flight path (the points' mean way: a turn about its middle stays held whole) is
// left out but US_SURF_DRAG of it when it brakes
#define US_SURF_DRAG 0.1f
// Its surfaces meet the air at angles of their own (the player, the same day: "no plane started rotating, are you sure
// the brocken wings work?" - a surface that only cancels the way across it pushes nothing once the tail has turned the
// nose into the way, so a weak wing made no difference after its first second; then "maybe disabeling/softening wings is
// the wrong lever, your (8 degrees nose-up) ... add a random number ... the same for the tailfins, instead of just not
// doing their job when failing they stear the plane. pitch the plane up and stall it, yaw it into a trun"): each
// pushes by the speed along it, across its tilted surface. Each wing between 8 degrees front up and 6 front down (by
// its serial, us_wings): two alike glide, two apart roll it; the tailplane pitches its nose up (a stall: it rears and
// sinks, slowed by its wings across the way) or down, the fin yaws it into a turn.
// Since the same day no surface draws its own angle (the player: "the airplanes dont make me happy yet, disable the per
// point randomness. the planes points should get feed 3 angle values: roll ... pitch ... and a yaw value"): every
// plane alike, its wings at a base angle (front up: the glide) plus the roll on the port wing and minus it on the
// starboard one, the tailplane's two points at the pitch, the fin at the yaw - degrees, set by the pipe's wreck plane
// for the planes killed from then on (the player tries them before any is drawn at random). Then drawn (the player:
// "4 lift is good as a modifier. roll: -30 to 30 pitch: -5 to 30 yae -30 to 30 would be the random values i would
// try"): the base the same for all, the three by its serial, even over these ranges - unless the pipe fixed them
internal float g_us_ctl[4] = { 4.0f, 0.0f, 0.0f, 0.0f };   // base, roll (+ port wing up: it banks right), pitch (+ nose up), yaw (+ nose right)
internal int g_us_fixed;   // the pipe's: roll, pitch and yaw g_us_ctl's for every plane, not drawn
#define US_RND_ROLL 30.0f      // degrees, either way
#define US_RND_PITCH_LO -5.0f
#define US_RND_PITCH_HI 30.0f
#define US_RND_YAW 30.0f
// A HELICOPTER'S FALL (the player, 2026-11-12: "now lets improve the helicopters, bougancy, some other ideas?", then
// "yes do 1- 4": it floats a little and rolls over, its rotor holds it, its spin builds up, its rotors turn until it
// hits). Six points: the floats, its ballast high - its engine and rotor over its cabin: top-heavy, a helicopter in
// the water rolls over and floats on its back before it goes, as ditched ones do - and its rotor's hub at its top.
// Its floats carry it briefly (US_HELI_FLOAT, leaking at US_HELI_LEAK of a ship's times). The hub's way through the
// rotor's disc is slowed by US_ROTOR times its hold, 0 .. 1 by its serial (autorotating down slowly, or dropping), and
// a little every way (held at its top it falls upright, a shuttlecock); the disc tilted up to US_DISC_TILT any way
// drifts it off sideways as it sinks.
// Its main rotor's torque, no tail rotor to hold it, turns it about its up the more the longer it falls (US_HELI_TWIST,
// the same way for all: their rotors turn alike), and its rotors turn on until it hits or is wet - all of it in the air
// only. Its rotors are its graphics' turned
// models, an Apache's the first values of its list a (logged: the main rotor's +7 radians/s, the tail rotor's -3.5,
// no parts): falling dry, they are the game's own while it is met, each one's rate measured, and on by those rates
// while it isn't (out of the view, or taken off where the game's copy lands); held from its hit or its first wet on
#define US_PTS_HELI 6       // the floats, the ballast (high), the rotor's hub
// A FLIER THAT SHATTERS (the player, 2026-11-12, of the Ares: "they have an shatter animation when hitting the ground, but
// its triggerd by the game, not by us, and it also triggers when hitting the water surface. it should only trigger when
// hitting land, and the remains position has to be corected than to sit properly on the terrain"). The game lets an
// Ares down to z 0 - the water's surface too - and there switches it to its death animation (5: its pieces flying
// apart, played as it dies and again from the start of its 14 s rot, then held; logged),
// while the wreck drawn here still fell, slowed by its rotor: it burst in the air and its pieces sank with the floats.
// A helicopter whose type has a rot time and a death animation of more than one frame is drawn with its animation at
// its death until its wreck hits land; from then its death animation plays once - its own stretch of its model's
// frames at its own frames' time (anim_first .. anim_last, anim_frame_ms: an Ares' 0 .. 15 at 97 ms, an Apollo's 14 ..
// 29 at 60; the model's every frame at the Ares' 95 ms until the same day showed an Apollo's other frames 0 .. 13
// first, rising - the player: "apol is a bit weired, on hitting the ground it get the death texture, moves up a bit,
// and than shatters after a second maybe") - and holds its last frame, upright on the ground under its floats,
// turned as the wreck lay. In the water it never shatters (it sinks whole), and it is never drawn burnt
#define US_SHATTER_ANIM 5
#define US_SHATTER_MS 95.0f   // a frame's time when its graphics give none
#define US_HELI_FLOAT 1.6f  // buoyancy, fully under (g's)
#define US_HELI_LEAK 0.3f   // its leaks' start and length: a share of a ship's (afloat a few seconds)
#define US_HELI_HIGH 0.6f   // the ballast at this share of its height over its floats
#define US_ROTOR 12.0f      // 1/s, a whole hold
#define US_ROTOR_SIDE 3.0f  // 1/s, the hub's every way, a whole hold
#define US_ROTOR_W 2.0f     // the hub's weight, a float's 1
#define US_DISC_TILT 12.0f  // degrees, the most
#define US_HELI_TWIST 1.5f  // radians/s^2: about 4.5 radians/s after 3 s
#define US_TAIL_SPAN 0.35f   // the tailplane's half span: a share of the wings' (a P-51's: 4 m to 11 m)
#define US_FIN_W 0.5f   // the tail's points' weights, a float's 1
#define US_NOSE 2.0f    // the nose weight's
#define US_AIR 0.3f
#define US_FLOAT 2.0f   // a float's buoyancy, fully under (g's): half under at rest
#define US_SUB_FLOAT 1.0f   // a submarine's: it hangs at its depth (all under) until the leaks take it down
#define US_DIG 0.6f     // the deepest a float goes into the seabed: a share of the beam
#define US_DIG_AIR 0.1f // ... an aircraft's into the ground (a flat thing: 0.6 buried it whole)
// an aircraft's ground collider up in its body (us_step's end; the player, 2026-11-12: "the ground collider should be
// placed a bit heigher, so that upright crashes sink in a bit and upside ones dont completly vanish"): at the floats -
// its model's foot - an upright one stopped on its skids or belly, untouched, and one on its back went in to its
// floats, its whole body buried
#define US_HIT_HIGH 0.35f   // of its model's height over its foot
#define US_FAST 1.5f    // a move between its poses faster than this times its top speed (at least US_JUMP tiles/s)
#define US_JUMP 4.0f    // was no sailing but a jump: a ship of 170 tiles/s stopped dead at a fixed 4 (the player)
// the ships on the water and the submarines (the player, the same day: "next add subs to the system"): a submarine's
// floats at its hull's middle height, neutral (US_SUB_FLOAT) - it hangs at its depth until the leaks take it down
internal int uc_is_aircraft(int k) {
    return k == CLS_AIRPLANE || k == CLS_HELICOPTER || k == CLS_HELICOPTER_TRANSPORT || k == CLS_HELICOPTER_PRIEST;
}
// ... and the aircraft (the player: "i want helicopters and airplanes to use the same system, with the difrence betwen
// water and air being just the drag. aiplanes mayb could also have boyancy that only drains once they hit water, so
// they raise up a bit before sinking"). The game glides a plane shot down to the ground over about 3.5 s (its dying
// state 6000, on forward), then it lies as a wreck for 30 s; a helicopter it lets down at about a tile a second and
// removes at the ground (rot time 0; logged 2026-11-11, P-51, B-17, Apache). Here it falls from where it was hit
// with its way and the kick, through the air's light drag, onto the ground or into the water: an airplane floats there
// as a ship does, its leaks' clock from its first touch of the water (it comes up before it goes down), a helicopter
// has no buoyancy. Its length is its model's y (the way it faces; a fighter is wider than long), a tilt is no sign of
// its going down (it banks in flight), and it digs into the ground by US_DIG_AIR of its span at most.
// Not a fish: its class a ship's (it moves on the ships' model), killed by a fishing boat's first hit (1 hp) it was taken
// for a wreck and went down under the boats fishing it - the game leaves it where it swims until its food is gone (the
// player, 2026-11-13: "fisherboats kill the fish and now the fish sinks to the seafloor where it cant be reached", "fish
// have no death animation, they just stay where they are und vanish when the resource is fully gathered")
internal int uc_is_ship(const ee_unit_type *ty) {
    int k = unit_type_get_class_id(ty);
    return (class_is_ship(k) || k == CLS_SUBMERGED_NAVAL || uc_is_aircraft(k)) && unit_type_get_family(ty) != KIND_FISH;
}
internal float us_rnd(uint32_t *s) { *s = *s * 1664525u + 1013904223u; return (float)(*s >> 8) / 16777216.0f; }
// a stream's start by a serial, its bits mixed whole (salt: the stream's own): a product alone drew alike for ids a power
// of two or a few apart - the leaks' plan climbed evenly with the id and wrapped round, three neighbours the same way
// down and ids 32 or 64 apart dozens alike (the player, 2026-11-12: "it seems ships of the same type nerly always pick
// the same sinking": built in a batch among other units)
internal uint32_t us_seed(int32_t serial, uint32_t salt) {
    uint32_t s = (uint32_t)serial ^ salt;
    s ^= s >> 16, s *= 0x7FEB352Du, s ^= s >> 15, s *= 0x846CA68Bu, s ^= s >> 16;
    return s;
}
// a point's weight (a float's 1): the ballast, a helicopter's hub, an airplane's tail points and nose weight
internal float us_w(const uc_corpse *k, int i) {
    return i < 4 ? 1.0f : i == 4 ? US_BALLAST : k->heli ? US_ROTOR_W : i == 8 ? US_NOSE : US_FIN_W;
}
// a helicopter's rotor by its serial (a stream of its own, hashed): its hold even 0 .. 1, its disc's tilt any way
internal void us_rotor(uc_corpse *k, int32_t serial) {
    uint32_t s = us_seed(serial, 0x27D4EB2Fu);
    k->rotor[0] = us_rnd(&s);
    float dir = 6.2831853f * us_rnd(&s), a = sinf(US_DISC_TILT * us_rnd(&s) * 3.14159265f / 180.0f);
    k->rotor[1] = a * cosf(dir), k->rotor[2] = a * sinf(dir);
}
// a held turn for the record (kind: 0 a part's, 1 list a's, 2 list b's first value): a falling helicopter's on by its rate
internal uint32_t us_turn(const uc_corpse *k, int kind, int32_t i, uint32_t held) {
    if (!k->ship || !k->heli || k->turn_ms <= 0.0) return held;
    float f;
    uint32_t r;
    memcpy(&f, &held, 4);
    f = fmodf(f + k->turn_rate[kind][i] * (k->spin_t - k->turn_at), 6.2831853f);
    memcpy(&r, &f, 4);
    return r;
}
// ... the game's turn now into held, its rate from the one held dms ms before (the shorter way round)
internal void us_turn_seen(float *rate, uint32_t *held, uint32_t now, double dms) {
    float a, b;
    memcpy(&a, held, 4), memcpy(&b, &now, 4);
    if (dms > 0.0 && dms < 500.0) {
        float d = b - a;
        d -= 6.2831853f * floorf(d / 6.2831853f + 0.5f);
        *rate = d / (float)(dms / 1000.0);
    }
    *held = now;
}
// an airplane's surfaces' angles from its controls: the base and, drawn by its serial (a stream of their own, its seed
// hashed: ids one apart drew nearly alike by a product alone) or the pipe's, its roll, pitch and yaw
internal void us_wings(uc_corpse *k, int32_t serial) {
    const float rad = 3.14159265f / 180.0f;
    float roll = g_us_ctl[1], pitch = g_us_ctl[2], yaw = g_us_ctl[3];
    if (!g_us_fixed) {
        uint32_t s = us_seed(serial, 0x165667B1u);
        roll = US_RND_ROLL * (2.0f * us_rnd(&s) - 1.0f);
        pitch = US_RND_PITCH_LO + (US_RND_PITCH_HI - US_RND_PITCH_LO) * us_rnd(&s);
        yaw = US_RND_YAW * (2.0f * us_rnd(&s) - 1.0f);
    }
    k->tilt[0] = sinf((g_us_ctl[0] + roll) * rad), k->tilt[1] = sinf((g_us_ctl[0] - roll) * rad);
    k->tilt[2] = sinf(pitch * rad), k->tilt[3] = sinf(yaw * rad);
}
// the pipe: wreck plane [<base> <roll> <pitch> <yaw> | random [<base>]] - a falling airplane's controls (degrees,
// -45 .. 45) for every plane killed from then on, or drawn for each again; without more what they are
// ...; wreck snap [all | none | random]: which ships long enough break in two (A SHIP BREAKING IN TWO)
internal void cmd_wreck(char *out, const char *args) {
    char mode[16] = "", word[16] = "";
    float v[4];
    int n = sscanf(args, "%15s %f %f %f %f", mode, &v[0], &v[1], &v[2], &v[3]), nb = 0;
    if (!strcmp(mode, "snap")) {
        if (sscanf(args, "%*s %15s", word) == 1) {
            if (!strcmp(word, "all")) g_us_snap = 1;
            else if (!strcmp(word, "none")) g_us_snap = -1;
            else if (!strcmp(word, "random")) g_us_snap = 0;
            else { REPLY("error: usage: wreck snap [all | none | random]\n"); return; }
        }
        if (g_us_snap) REPLY("ok: ships of %.1f tiles or longer break in two: %s\n", US_SNAP_LEN, g_us_snap > 0 ? "all" : "none");
        else REPLY("ok: ships of %.1f tiles or longer break in two: %.0f%% of them, drawn by their ids\n", US_SNAP_LEN, US_SNAP * 100.0f);
        return;
    }
    if (!strcmp(mode, "floor")) {
        if (sscanf(args, "%*s %15s", word) == 1) {
            if (!strcmp(word, "all")) g_us_floor = 1;
            else if (!strcmp(word, "none")) g_us_floor = -1;
            else if (!strcmp(word, "random")) g_us_floor = 0;
            else { REPLY("error: usage: wreck floor [all | none | random]\n"); return; }
        }
        if (g_us_floor) REPLY("ok: ships of %.1f tiles or longer that don't break at their death break on the seabed: %s\n", US_SNAP_LEN, g_us_floor > 0 ? "all" : "none");
        else REPLY("ok: ships of %.1f tiles or longer that don't break at their death break on the seabed: %.0f%% of them, drawn by their ids\n", US_SNAP_LEN, US_SNAP_FLOOR * 100.0f);
        return;
    }
    if (n < 1 || strcmp(mode, "plane")) { REPLY("error: usage: wreck plane [<base> <roll> <pitch> <yaw> | random [<base>]] (degrees) | wreck snap [all | none | random] | wreck floor [all | none | random]\n"); return; }
    int drawn = n == 1 && sscanf(args, "%*s %15s", word) == 1 && !strcmp(word, "random");
    if (drawn) nb = sscanf(args, "%*s %*s %f", &v[0]);
    else if (n != 1 && n != 5) { REPLY("error: usage: wreck plane [<base> <roll> <pitch> <yaw> | random [<base>]] (degrees)\n"); return; }
    for (int i = 0; i < (drawn ? nb : n == 5 ? 4 : 0); i++)
        if (!(v[i] >= -45.0f && v[i] <= 45.0f)) { REPLY("error: an angle is -45 .. 45 degrees\n"); return; }
    if (drawn) g_us_fixed = 0, g_us_ctl[0] = nb == 1 ? v[0] : g_us_ctl[0];
    else if (n == 5) memcpy(g_us_ctl, v, sizeof v), g_us_fixed = 1;
    if (!g_us_fixed)
        REPLY("ok: a falling plane's base %.1f, roll %.0f .. %.0f, pitch %.0f .. %.0f, yaw %.0f .. %.0f drawn for each (degrees: + port wing up, nose up, nose right)\n",
              g_us_ctl[0], -US_RND_ROLL, US_RND_ROLL, US_RND_PITCH_LO, US_RND_PITCH_HI, -US_RND_YAW, US_RND_YAW);
    else
        REPLY("ok: a falling plane's base %.1f roll %.1f pitch %.1f yaw %.1f for all (degrees: + port wing up, nose up, nose right)\n",
              g_us_ctl[0], g_us_ctl[1], g_us_ctl[2], g_us_ctl[3]);
}
// its leaks by the plan of its serial; late: met already sinking (come into the view): at their end at once; seesaw: a
// ship's (a submarine's and an aircraft's plans as before)
internal void us_leaks(uc_corpse *k, int32_t serial, int late, int seesaw) {
    uint32_t s = us_seed(serial, 0x9E3779B9u);
    float plan = us_rnd(&s);
    for (int i = 0; i < 4; i++)   // the slow leaks: each float to less than its weight
        k->leak[i][0] = 0.55f + 0.3f * us_rnd(&s), k->leak[i][1] = 1.0f + 2.0f * us_rnd(&s), k->leak[i][2] = 6.0f + 6.0f * us_rnd(&s),
        k->leak[i][3] = k->lift;
    if (seesaw && plan >= 1.0f - US_SEESAW) {   // one end flooded at once, the middle holding late
        int e = us_rnd(&s) < 0.5f;   // the bow or the stern
        k->leak[e][0] = -0.3f - 0.3f * us_rnd(&s), k->leak[e][1] = 0.2f * us_rnd(&s), k->leak[e][2] = 1.5f + 2.0f * us_rnd(&s);
        int a = 2 + (us_rnd(&s) < 0.5f);   // the side that fails first
        k->leak[a][1] = 3.0f + 3.0f * us_rnd(&s), k->leak[a ^ 1][1] = k->leak[a][1] + 3.0f + 3.0f * us_rnd(&s);
        for (int i = 2; i < 4; i++) k->leak[i][2] = 14.0f + 8.0f * us_rnd(&s), k->leak[i][3] = US_SEESAW_LIFT;
        plan = -1.0f;
    } else if (seesaw) plan /= 1.0f - US_SEESAW;   // the others share the rest as before
    int fast = plan < 0.0f ? -1 : plan < 1.0f / 3.0f ? 0 : plan < 2.0f / 3.0f ? 1 : 2 + (us_rnd(&s) < 0.5f);   // bow, stern, a side
    if (fast >= 0) {
        k->leak[fast][0] = -0.6f - 0.6f * us_rnd(&s), k->leak[fast][1] = 0.2f * us_rnd(&s), k->leak[fast][2] = 1.5f + 2.0f * us_rnd(&s);
        if (fast >= 2) {   // the other side holds on longest: it rolls over before it goes
            int o = fast ^ 1;
            k->leak[o][0] = 0.8f + 0.15f * us_rnd(&s), k->leak[o][1] = 4.0f + 2.0f * us_rnd(&s), k->leak[o][2] = 10.0f + 8.0f * us_rnd(&s);
        } else if (us_rnd(&s) < 0.4f) {   // a dive with a list to one side
            int o = 2 + (us_rnd(&s) < 0.5f);
            k->leak[o][0] = 0.1f + 0.4f * us_rnd(&s), k->leak[o][1] = 0.5f + us_rnd(&s), k->leak[o][2] = 3.0f + 3.0f * us_rnd(&s);
        }
    }
    for (int i = 0; i < US_PTS; i++) k->dig[i] = us_rnd(&s);   // its share of US_DIG (drawn last: the leaks stay as they were)
    if (late) for (int i = 0; i < 4; i++) k->leak[i][1] = -1000.0f, k->leak[i][2] = 0.01f;
}
// its velocity at its death (tiles/s) into v: its newest move between the poses the collection kept of it (the game may
// stop it in the step it dies: then the one before), none if it is met already sinking; along the water - an aircraft's
// up or down too (the player, 2026-11-12: "can it be that airplanes lose speed when they die?" - "its espacialy
// noticable at launch when they are pointing up": a P-51 shot climbing from its airfield, 0.9 tiles/s on and 0.45
// up, its nose 27 degrees up, went on level only - across its wings, which took half its way in 1.5 s; logged)
internal void us_velocity(const uc_obj *o, float *v, int air, int *pair) {
    v[0] = v[1] = v[2] = 0.0f, *pair = -1;
    const float *now = (const float *)o->pm;
    float top = fmaxf(o->st ? unit_stats_get_speed(o->st) : 0.0f, o->cst ? unit_stats_get_speed(o->cst) : 0.0f);
    float fast = fmaxf(US_FAST * top, US_JUMP);
    for (int i = 0; i < 2; i++) {
        const float *a = i ? o->was2 : o->was, *b = i ? o->was : now;
        float dt = (o->ms[i] - o->ms[i + 1]) / 1000.0f;
        if (!(i ? o->has_was2 : o->has_was) || dt <= 0.0f || dt > 1.0f) return;
        float dx = (b[3] - a[3]) / dt, dy = (b[7] - a[7]) / dt, s = dx * dx + dy * dy;
        if (s > fast * fast) return;
        if (s > 1e-4f) { v[0] = dx, v[1] = dy, v[2] = air ? (b[11] - a[11]) / dt : 0.0f, *pair = i; return; }
    }
}
// an aircraft's own turn between the same two poses (pair: us_velocity's; the game banks and turns it), each point a
// step behind by it (the player, 2026-11-12: "are the point copping the planes rotation and velocithy peroperly?" -
// only its way was, the kick instead of its turn); a snap of more than 30 degrees between them is no turn
internal void us_spin(uc_corpse *k, const uc_obj *o, const float *t, int pair) {
    const float *a = pair ? o->was2 : o->was, *b = pair ? o->was : (const float *)o->pm;
    float dt = (o->ms[pair] - o->ms[pair + 1]) / 1000.0f, sa = 0.0f, sb = 0.0f, m[3][3];
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) sa += a[r * 4 + c] * a[r * 4 + c], sb += b[r * 4 + c] * b[r * 4 + c];
    if (dt <= 0.0f || sa < 1e-8f || sb < 1e-8f) return;
    float n = 3.0f / sqrtf(sa * sb);   // their scales out
    for (int j = 0; j < 3; j++)   // its turn over dt (b times a's transpose), less the unit
        for (int l = 0; l < 3; l++) {
            float s = 0.0f;
            for (int c = 0; c < 3; c++) s += b[j * 4 + c] * a[l * 4 + c];
            m[j][l] = n * s - (j == l ? 1.0f : 0.0f);
        }
    if (1.0f + 0.5f * (m[0][0] + m[1][1] + m[2][2]) < 0.866f) return;
    float f = (float)US_DT / dt;
    for (int i = 0; i < k->n; i++) {
        float r[3] = { k->sp[i][0] - t[3], k->sp[i][1] - t[7], k->sp[i][2] - t[11] };
        for (int j = 0; j < 3; j++) k->sq[i][j] -= f * (m[j][0] * r[0] + m[j][1] * r[1] + m[j][2] * r[2]);
    }
}
// the kick at its death: each point's way a step behind by the turn about its own axes (the frame's as it starts,
// about its points' middle by their weights), random by its serial - a stream of its own, the leaks' plans as before
internal void us_kick(uc_corpse *k, int32_t serial) {
    uint32_t s = us_seed(serial, 0x85EBCA6Bu);
    float w[3], m[3] = { 0, 0, 0 }, al[3], ac[3], up[3], la = 0.0f, lc = 0.0f, lu = 0.0f, sum = 0.0f;
    static const float ship[3] = { US_KICK_ROLL, US_KICK_PITCH, US_KICK_YAW },
                       plane[3] = { US_KICK_PLANE_ROLL, US_KICK_AIR_OTHER, US_KICK_AIR_OTHER },
                       heli[3] = { US_KICK_AIR_OTHER, US_KICK_AIR_OTHER, US_KICK_HELI_YAW };
    const float *rate = !k->air ? ship : k->heli ? heli : plane;
    for (int a = 0; a < 3; a++) w[a] = rate[a] * (0.5f + 0.5f * us_rnd(&s)) * (us_rnd(&s) < 0.5f ? -1.0f : 1.0f);
    for (int j = 0; j < 3; j++) al[j] = k->sp[0][j] - k->sp[1][j], ac[j] = k->sp[2][j] - k->sp[3][j], la += al[j] * al[j], lc += ac[j] * ac[j];
    la = sqrtf(la), lc = sqrtf(lc);
    if (la < 1e-6f || lc < 1e-6f) return;
    for (int j = 0; j < 3; j++) al[j] /= la, ac[j] /= lc;
    up[0] = al[1] * ac[2] - al[2] * ac[1], up[1] = al[2] * ac[0] - al[0] * ac[2], up[2] = al[0] * ac[1] - al[1] * ac[0];
    for (int j = 0; j < 3; j++) lu += up[j] * up[j];
    lu = sqrtf(lu);
    if (lu < 1e-6f) return;
    float om[3];   // the turn in the world: about its length, its beam, its up
    for (int j = 0; j < 3; j++) om[j] = w[0] * al[j] + w[1] * ac[j] + w[2] * up[j] / lu;
    for (int i = 0; i < k->n; i++) {
        float wt = us_w(k, i);
        for (int j = 0; j < 3; j++) m[j] += wt * k->sp[i][j];
        sum += wt;
    }
    for (int i = 0; i < k->n; i++) {
        float r[3], dt = (float)US_DT;
        for (int j = 0; j < 3; j++) r[j] = k->sp[i][j] - m[j] / sum;
        k->sq[i][0] -= (om[1] * r[2] - om[2] * r[1]) * dt;
        k->sq[i][1] -= (om[2] * r[0] - om[0] * r[2]) * dt;
        k->sq[i][2] -= (om[0] * r[1] - om[1] * r[0]) * dt;
    }
}
// its floats set up from the game's pose t (rows; the model's scale apart) and its model's frame
internal void us_start(uc_corpse *k, const uc_obj *o, const float *t) {
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(o->gfx))[o->anim];
    ee_box *box = m ? model_box(m, (uint32_t)o->anim_frame) : NULL;
    float r = m ? model_get_radii(m)[o->anim_frame] : 1.0f, s = o->cu_height;
    float lo[3] = { -0.9f * r, -0.3f * r, -0.2f * r }, hi[3] = { 0.9f * r, 0.3f * r, 0.5f * r };
    if (box) {
        vec3 a = box_get_min(box), b = box_get_max(box);
        if (b.x > a.x && b.y > a.y) lo[0] = a.x, lo[1] = a.y, lo[2] = a.z, hi[0] = b.x, hi[1] = b.y, hi[2] = b.z;
    }
    // a ship's floats at its waterline (its model's 0), a submarine's at its hull's middle height: its model stands on
    // its keel (0 .. 0.5 of its scale, logged), the game holds it 0.4 tiles under the water, wholly under
    int cls = unit_type_get_class_id(o->ty), sub = cls == CLS_SUBMERGED_NAVAL, air = uc_is_aircraft(cls);
    int x_long = !air && hi[0] - lo[0] >= hi[1] - lo[1];
    float fz = sub ? 0.5f * (lo[2] + hi[2]) * s : 0.0f;
    float c[3] = { 0.5f * (lo[0] + hi[0]) * s, 0.5f * (lo[1] + hi[1]) * s, fz };
    float half_long = 0.4f * (x_long ? hi[0] - lo[0] : hi[1] - lo[1]) * s, half_across = 0.4f * (x_long ? hi[1] - lo[1] : hi[0] - lo[0]) * s;
    float al[3] = { x_long ? 1.0f : 0.0f, x_long ? 0.0f : 1.0f, 0.0f }, ac[3] = { x_long ? 0.0f : -1.0f, x_long ? 1.0f : 0.0f, 0.0f };   // across: up x along
    memcpy(k->along, al, sizeof al), memcpy(k->across, ac, sizeof ac);
    for (int j = 0; j < 3; j++) {
        k->body[0][j] = c[j] + half_long * al[j], k->body[1][j] = c[j] - half_long * al[j];
        k->body[2][j] = c[j] + half_across * ac[j], k->body[3][j] = c[j] - half_across * ac[j];
    }
    k->keel = fminf(fmaxf(fz - lo[2] * s, 0.05f), 1.0f);
    k->body[4][0] = c[0], k->body[4][1] = c[1], k->body[4][2] = fz - US_LOW * k->keel;
    k->heli = (uint8_t)(air && cls != CLS_AIRPLANE), k->spin_t = 0.0f;
    ee_model *sm = ((ee_model **)dmg_model_anim_model_at(o->gfx))[US_SHATTER_ANIM];
    k->shatters = (uint8_t)(k->heli && unit_type_get_rot_time(o->ty) > 0 && sm &&
                            ((const int32_t *)dmg_model_anim_last_at(o->gfx))[US_SHATTER_ANIM] > ((const int32_t *)dmg_model_anim_first_at(o->gfx))[US_SHATTER_ANIM]);
    k->shatter_t = -1.0f;   // its look at its death (met already shattered: its first animation)
    k->fly_anim = o->anim == US_SHATTER_ANIM ? 0 : o->anim, k->fly_frame = o->anim == US_SHATTER_ANIM ? 0 : o->anim_frame;
    k->n = cls == CLS_AIRPLANE ? US_PTS_AIR : k->heli ? US_PTS_HELI : US_PTS;
    if (k->heli) {   // its ballast high over its floats (its model stands on them), its hub at its top
        float top = fmaxf(hi[2] * s, 0.1f);
        k->body[4][2] = fz + US_HELI_HIGH * top;
        k->body[5][0] = c[0], k->body[5][1] = c[1], k->body[5][2] = fz + 0.95f * top;
        us_rotor(k, o->serial);
    }
    if (k->n == US_PTS_AIR) {   // the tail at the stern float, the fin over it, the nose weight at the bow float's height
        float tail = US_TAIL_SPAN * half_across, fin = fmaxf(0.6f * hi[2] * s, 0.2f * half_across);
        for (int j = 0; j < 3; j++) {
            k->body[5][j] = c[j] - half_long * al[j] + tail * ac[j], k->body[6][j] = c[j] - half_long * al[j] - tail * ac[j];
            k->body[7][j] = c[j] - half_long * al[j], k->body[8][j] = c[j] + 0.9f * half_long * al[j];
        }
        k->body[7][2] += fin;
        // its ballast at its wings' height: under them it hung from their lift as a pendulum and rolled a plane with a
        // broken wing level again (logged: banked to 40 degrees in a second, back to 25 by its crash)
        k->body[4][2] = fz;
        us_wings(k, o->serial);
    }
    k->lift = sub ? US_SUB_FLOAT : k->heli ? US_HELI_FLOAT : US_FLOAT;
    k->wet_at = air ? -1.0f : 0.0f;
    k->serial = o->serial, k->air = (uint8_t)air, k->fx_done = k->fx_due = k->stuck = 0, k->in_game = 1;
    // (a shattering flier's at its foot: raised, its body went a third of its height into the ground before its pieces
    // flew - the player, 2026-11-12: "they still react to slugish when hitting the ground, maybe becaus the ground
    // colidor is abit raised? for helicopter sthis makes sence, the 3 flying cybor unit need it at 0 again i think")
    k->hit = air && !k->shatters ? US_HIT_HIGH * fmaxf(hi[2] * s, 0.0f) : 0.0f;
    k->shift = 0.0f, k->shift_to = 0;
    k->range = fminf(fmaxf(1.2f * half_across, 0.1f), 0.5f);
    // the water's level: the drawn water's, known while some is in the view; else a ship's own height (it lies on the
    // water), an aircraft's the game's water line, z 0 (its crash steps' splash) - its own height made the air at its death
    // its water: with no water on the screen every helicopter killed splashed at once and floated where it died (the
    // player, 2026-11-12, "standing army waterhely": "all helecopter in cmaera view that die imdeiatly splash into water
    // even tough they are over land and heigh in the air")
    k->zw = g_sh.water_on ? g_sh.level : air ? 0.0f : t[11];
    for (int i = 0; i < k->n; i++)
        for (int j = 0; j < 3; j++) {
            const float *b = k->body[i];
            k->sp[i][j] = k->sq[i][j] = t[j * 4 + 3] + t[j * 4] * b[0] + t[j * 4 + 1] * b[1] + t[j * 4 + 2] * b[2];
        }
    int late = o->rotting || (!air && (t[10] < 0.98f || (!sub && t[11] < k->zw - 0.05f)));   // (a submarine is under anyway)
    memset(k->dig, 0, sizeof k->dig);
    us_leaks(k, o->serial, late, !air && !sub);
    for (int i = 0; i < 4 && k->heli; i++) k->leak[i][1] *= US_HELI_LEAK, k->leak[i][2] *= US_HELI_LEAK;
    for (int i = 0; i < US_PTS; i++) k->dig[i] *= (air ? US_DIG_AIR : US_DIG) * 2.5f * half_across;   // the beam: half_across is 0.4 of it
    // breaking in two (A SHIP BREAKING IN TWO): long enough and drawn by its id (a stream of its own) - its leaks slow
    // until then
    float len = (x_long ? hi[0] - lo[0] : hi[1] - lo[1]) * s;
    k->ends[0] = (x_long ? lo[0] : lo[1]) * s, k->ends[1] = (x_long ? hi[0] : hi[1]) * s;
    k->half = k->twin = k->floor_snap = k->floor_hit = 0, k->hit_v = 0.0f, k->snap_t = -1.0f, k->deck = fminf(US_HULL_TOP * hi[2], US_HULL_MAX);
    if (!air && !late && len >= US_SNAP_LEN && ud_cuts()) {   // (its halves drawn: the mod's record draw)
        uint32_t ss = us_seed(o->serial, 0xC2B2AE3Du);
        float drawn = us_rnd(&ss);
        if (g_us_snap > 0 || (!g_us_snap && drawn < (sub ? US_SNAP_SUB : US_SNAP))) {
            k->snap_t = US_SNAP_AT + US_SNAP_AT_RND * us_rnd(&ss);
            k->cut = k->ends[0] + (0.45f + 0.1f * us_rnd(&ss)) * (k->ends[1] - k->ends[0]);
            for (int i = 0; i < 4 && !sub; i++)   // (a submarine's own leaks kept: it hangs at its depth as it would)
                k->leak[i][0] = 0.6f + 0.25f * us_rnd(&ss), k->leak[i][1] = 1.0f + 2.0f * us_rnd(&ss), k->leak[i][2] = 6.0f + 6.0f * us_rnd(&ss),
                k->leak[i][3] = k->lift;
        } else {   // on the seabed, by a stream of its own (its leaks as any ship's)
            uint32_t sf = us_seed(o->serial, 0x165667B1u);
            if (g_us_floor > 0 || (!g_us_floor && us_rnd(&sf) < (sub ? US_SNAP_FLOOR_SUB : US_SNAP_FLOOR)))
                k->floor_snap = 1, k->cut = k->ends[0] + (0.45f + 0.1f * us_rnd(&sf)) * (k->ends[1] - k->ends[0]);
        }
    }
    // never pushed up out of the bottom where the game had it - a submarine in the shallows (0.4 under the water) stood
    // in the sand and came up with its nose out: a point below its stop at the start digs in as deep as it was
    const void *mesh = game_map() ? map_mesh_at(game_map()) : NULL;
    for (int i = 0; i < US_PTS && mesh; i++) {
        const float *p = k->sp[i];
        if (!mesh_has_tile(mesh, (int)floorf(p[0]), (int)floorf(p[1]))) continue;
        float stop = mesh_z(mesh, p[0], p[1]) + (i == 4 ? 1.0f - US_LOW : 1.0f) * k->keel;   // upright: the keel under it
        if (p[2] < stop - k->dig[i]) k->dig[i] = stop - p[2];
    }
    float v[3] = { 0.0f, 0.0f, 0.0f };
    int pair = -1;
    if (!late) us_velocity(o, v, air, &pair);
    for (int i = 0; i < k->n; i++)   // Verlet: a step's way behind
        for (int j = 0; j < 3; j++) k->sq[i][j] -= v[j] * (float)US_DT;
    if (air && pair >= 0) us_spin(k, o, t, pair);
    if (!late) us_kick(k, o->serial);
    k->sim = k->died;
    k->ship = 1;
}
// THE EFFECTS OF ITS FALL (the player, the same day: "the explosion and water splash still get triggerd by the old system,
// smoke also dosnt follow the demaged unit"). The game crashes an aircraft by moving it itself (EEPMAirplane's and
// EEPMHelicopter's crash steps): it makes the splash where it comes down through the water and the death effect where it
// reaches the ground - at its own place and moment, while the wreck drawn here fell elsewhere and sooner; and every
// effect on a unit (the damage smoke's puffs, the fire, the trails, a ship's death effect) reads the unit's matrix at
// each of its steps. So the game's two calls in the crash steps are let out (us_hook_crash_*) while the wrecks are the
// mod's, the fall here makes them where it splashes (a float going down through the water over a tile under it) and
// where it hits the ground going down faster than US_CRASH_V (an aircraft killed standing on its airfield none), and
// around the effects' step (us_hook_effects_step) each wreck's unit has the wreck's pose for its matrix - the game's put
// back right after, so the simulation never sees it. The effects are the picture only (made or left out by each PC's
// detail setting, their own random numbers). Both made on the simulation's thread: the fall runs on the frame's, so it
// only notes them (fx_due).
#define US_FX_SPLASH 1
#define US_FX_CRASH 2
// tiles/s down; 0.5 until 2026-11-12 (the player: "if i delete the selected helicopter it wont explode on impact for
// some reason"): a helicopter its rotor holds comes down at 0.35 - 0.55 tiles/s and mostly blew up no more - one killed
// standing doesn't move at all
#define US_CRASH_V 0.15f
internal void us_fx(uc_corpse *k, uint8_t f, float x, float y, float z) {
    float *at = k->fx_at[f == US_FX_SPLASH ? 0 : 1];
    at[0] = x, at[1] = y, at[2] = z;
    k->fx_done |= f, k->fx_due |= f;
}
// AN AIRCRAFT'S CRASH (the player, the same day: "the settling on the floor like sunken ships after an crash is silly ...
// airplanes and helicopters have only ground collison on a single point in the middle, once that point hits ground its
// stuck. no air drag, only planing"): in the air nothing slows it, only the planing turns a helicopter (US_RISE: the end it
// moves toward up) and an airplane's wings and fins hold it (US_PTS_AIR); the ground holds only its middle (its floats' mean), and once that touches the ground or the seabed it stays
// as it hit - nose in, a wing down - no more steps. In the water it floats and is slowed as a ship is.
// one step at t s after its death: each float moved by its forces (Verlet: its velocity its way since the step before),
// then the frame's distances and the seabed held, four times round
// how far the hull reaches under each point as it lies now (us_step's ground: its place's bottom this much above it, less
// its dig): the keel as far as it is upright, and under the bow and the stern half its beam as far as it lies on its side
// (the side floats are at its sides) - a ship rolled over comes to rest on its side; held at the keel alone, all four
// came to rest level and it stood up again on the bottom. The ballast above the keel on the middle line, on its side
// half the beam up - less as it slid to the low side. Its side's share (0 upright, 1 on its side)
internal float us_below(const uc_corpse *k, float *below) {
    float al[3], ac[3], lal = 0.0f, lac = 0.0f, uz;
    for (int j = 0; j < 3; j++) al[j] = k->sp[0][j] - k->sp[1][j], ac[j] = k->sp[2][j] - k->sp[3][j], lal += al[j] * al[j], lac += ac[j] * ac[j];
    lal = sqrtf(lal), lac = sqrtf(lac);
    uz = lal > 1e-6f && lac > 1e-6f ? (al[0] * ac[1] - al[1] * ac[0]) / (lal * lac) : 1.0f;   // the hull's up, its height
    float side = lac > 1e-6f ? fabsf(ac[2]) / lac : 0.0f, beam = 0.5f * lac;
    below[0] = below[1] = k->keel * fmaxf(uz, 0.0f) + beam * side;
    below[2] = below[3] = k->keel * fmaxf(uz, 0.0f);
    below[4] = (1.0f - US_LOW) * k->keel * fmaxf(uz, 0.0f) + beam * side * (1.0f - 0.8f * k->shift);
    return side;
}
internal void us_step(uc_corpse *k, float t, const void *mesh) {
    if (k->stuck) return;
    const float dt = (float)US_DT;
    float carry = 0.0f;   // a float's lift: its share of all the weight
    for (int i = 0; i < k->n; i++) carry += 0.25f * us_w(k, i);
    // the hull's length as it lies (from the stern to the bow): the way along it the water's drag spares
    float ln[3], ll = 0.0f;
    for (int j = 0; j < 3; j++) ln[j] = k->sp[0][j] - k->sp[1][j], ll += ln[j] * ln[j];
    ll = sqrtf(ll);
    for (int j = 0; j < 3; j++) ln[j] = ll > 1e-6f ? ln[j] / ll : 0.0f;
    const float f_across = expf(-US_DRAG * dt), f_along = expf(-US_ALONG * dt), f_air = k->air ? 1.0f : expf(-US_AIR * dt);
    float wet = 0.0f;   // how much of it is under: its floats' mean
    // its way along its length (the points' by their weights; + bow-ward), as far as it is counted
    float way = 0.0f, ws = 0.0f, fl[3] = { 0, 0, 0 }, lf = 0.0f;   // fl: its flight path (the mean way's direction)
    for (int i = 0; i < k->n; i++) {
        float w = us_w(k, i);
        for (int j = 0; j < 3; j++) way += w * (k->sp[i][j] - k->sq[i][j]) * ln[j], fl[j] += w * (k->sp[i][j] - k->sq[i][j]);
        ws += w;
    }
    for (int j = 0; j < 3; j++) lf += fl[j] * fl[j];
    lf = sqrtf(lf);
    for (int j = 0; j < 3; j++) fl[j] = lf > 1e-7f ? fl[j] / lf : 0.0f;
    way = fminf(fmaxf(way / (ws * dt), -US_RISE_TOP), US_RISE_TOP);
    // an airplane's surfaces' ups: its wings' and tailplane's (length x span), its fin's (its span)
    float sn[3], un[3], ls = 0.0f, lu = 0.0f;
    for (int j = 0; j < 3; j++) sn[j] = k->sp[2][j] - k->sp[3][j], ls += sn[j] * sn[j];
    ls = sqrtf(ls);
    for (int j = 0; j < 3; j++) sn[j] = ls > 1e-6f ? sn[j] / ls : 0.0f;
    un[0] = ln[1] * sn[2] - ln[2] * sn[1], un[1] = ln[2] * sn[0] - ln[0] * sn[2], un[2] = ln[0] * sn[1] - ln[1] * sn[0];
    for (int j = 0; j < 3; j++) lu += un[j] * un[j];
    lu = sqrtf(lu);
    for (int j = 0; j < 3; j++) un[j] = lu > 1e-6f ? un[j] / lu : 0.0f;
    // each surface's up tilted by its angle: a wing's front up (back), the tailplane's to pitch the nose up (forward: it
    // pushes the tail down), the fin's to yaw it
    float nw[4][3], cm[3] = { 0, 0, 0 }, cw = 0.0f, hub = 0.0f;   // hub: the rotor's push this step (tiles/step, along its up)
    if (k->heli) {   // its middle by the weights (the spin's axis through it)
        for (int i = 0; i < k->n; i++) {
            for (int j = 0; j < 3; j++) cm[j] += us_w(k, i) * k->sp[i][j];
            cw += us_w(k, i);
        }
        for (int j = 0; j < 3; j++) cm[j] /= cw;
    }
    if (k->n == US_PTS_AIR)
        for (int s = 0; s < 4; s++) {
            const float *base = s == 3 ? sn : un;
            float st = k->tilt[s] * (s == 2 ? 1.0f : -1.0f), ct = sqrtf(1.0f - k->tilt[s] * k->tilt[s]);
            for (int j = 0; j < 3; j++) nw[s][j] = ct * base[j] + st * ln[j];
        }
    for (int i = 0; i < k->n; i++) {
        float *p = k->sp[i], *q = k->sq[i], b = 0.0f, under = 0.5f + (k->zw - p[2]) / k->range;
        under = under < 0.0f ? 0.0f : under > 1.0f ? 1.0f : under;
        if (i < 4) wet += 0.25f * under;
        if (i < 4 && under > 0.0f && k->wet_at < 0.0f) k->wet_at = t;   // the leaks' clock starts
        if (i < 4 && k->lift > 0.0f) {
            float u = k->wet_at < 0.0f ? -1.0f : (t + (k->air ? 0.0f : US_LEAK_HEAD) - k->wet_at - k->leak[i][1]) / k->leak[i][2];
            u = u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u * u * (3.0f - 2.0f * u);
            b = (k->leak[i][3] + (k->leak[i][0] - k->leak[i][3]) * u) * carry;
        }
        float v[3], a = 0.0f;
        for (int j = 0; j < 3; j++) v[j] = p[j] - q[j], a += v[j] * ln[j];
        // in the water its way along the hull kept by the light drag, the rest across it by the heavy one; in the air the
        // air's every way - between them by how wet the point is: wholly in the water from the waterline down (the hull
        // under it), wholly in the air half its range up (one wholly in the water as soon as it was wet, until 2026-11-11)
        float wd = fminf(2.0f * under, 1.0f);
        for (int j = 0; j < 3; j++) {
            float in_water = a * ln[j] * f_along + (v[j] - a * ln[j]) * f_across, in_air = v[j] * f_air;
            v[j] = in_air + (in_water - in_air) * wd;
        }
        float surf = k->heli ? (i == 5 ? US_ROTOR * k->rotor[0] : 0.0f)
                   : k->n != US_PTS_AIR ? 0.0f : i == 2 || i == 3 ? US_WING : i == 5 || i == 6 ? US_TAILPLANE : i == 7 ? US_FIN : 0.0f;
        if (surf > 0.0f && k->heli) {   // the rotor: its hub's way along its up slowed, a little every way (it hangs
            // under its rotor as under a shuttlecock's feathers: upright); the disc's tilt only drifts the whole (below) -
            // pushed by a tilted disc at its top it tumbled in the air (logged: on its side in 2 s)
            float c = 0.0f, f = expf(-surf * (1.0f - wd) * dt), fs = expf(-US_ROTOR_SIDE * k->rotor[0] * (1.0f - wd) * dt);
            for (int j = 0; j < 3; j++) c += v[j] * un[j];
            for (int j = 0; j < 3; j++) v[j] = (v[j] - (1.0f - f) * c * un[j]) * fs;
            hub = -(1.0f - f) * c;
        } else if (surf > 0.0f) {   // its way through its surface cancelled, as far as it is dry
            const float *nv = nw[i == 2 ? 0 : i == 3 ? 1 : i == 7 ? 3 : 2];
            float c = 0.0f, f = expf(-surf * (1.0f - wd) * dt), dv[3], br = 0.0f;
            for (int j = 0; j < 3; j++) c += v[j] * nv[j];
            for (int j = 0; j < 3; j++) dv[j] = -(1.0f - f) * c * nv[j], br += dv[j] * fl[j];
            if (br < 0.0f) br *= 1.0f - US_SURF_DRAG;   // along the flight path: a share of a brake, nothing that speeds it
            for (int j = 0; j < 3; j++) v[j] += dv[j] - br * fl[j];
        }
        float g = k->air ? US_G_AIR : US_G;
        v[2] += (-g + b * g * under) * dt * dt;
        if (k->heli) {   // the main rotor's torque about its up through its middle, as far as it is dry
            float r[3], tw = US_HELI_TWIST * (1.0f - wd) * dt * dt;
            for (int j = 0; j < 3; j++) r[j] = p[j] - cm[j];
            v[0] += tw * (un[1] * r[2] - un[2] * r[1]), v[1] += tw * (un[2] * r[0] - un[0] * r[2]), v[2] += tw * (un[0] * r[1] - un[1] * r[0]);
        }
        if (i < 2)   // the leading end up, the other down: in the water only (a helicopter's in the air too until 2026-11-12:
                     // spinning, its drift's way along it turned to and fro and rocked it over)
            v[2] += (i == 0 ? 1.0f : -1.0f) * US_RISE * way * under * dt * dt;
        for (int j = 0; j < 3; j++) q[j] = p[j], p[j] += v[j];
        if (k->air && i < 4 && !(k->fx_done & US_FX_SPLASH) && q[2] > k->zw && p[2] <= k->zw &&   // into the water
            mesh_has_tile(mesh, (int)floorf(p[0]), (int)floorf(p[1])) && mesh_z(mesh, p[0], p[1]) < k->zw)
            us_fx(k, US_FX_SPLASH, p[0], p[1], k->zw);
    }
    if (k->heli) {   // its rotors turn while it is dry; the disc's tilt drifts the whole by the rotor's push, sideways
        k->spin_t += dt * (1.0f - fminf(2.0f * wet, 1.0f));
        float dr = hub * US_ROTOR_W / cw;
        for (int i = 0; i < k->n; i++)
            for (int j = 0; j < 3; j++) k->sp[i][j] += dr * (k->rotor[1] * ln[j] + k->rotor[2] * sn[j]);
    }
    // its turning held: each point's way about the points' middle (by their weights) slowed, the whole's way kept
    float vm[3] = { 0, 0, 0 }, wsum = 0.0f, f_turn = expf(-US_TURN * wet * dt);
    for (int i = 0; i < k->n; i++) {
        float w = us_w(k, i);
        for (int j = 0; j < 3; j++) vm[j] += w * (k->sp[i][j] - k->sq[i][j]);
        wsum += w;
    }
    for (int i = 0; i < k->n; i++)
        for (int j = 0; j < 3; j++) {
            float m = vm[j] / wsum;
            k->sp[i][j] = k->sq[i][j] + m + (k->sp[i][j] - k->sq[i][j] - m) * f_turn;
        }
    float below[US_PTS], side = us_below(k, below);
    if (!k->air && !k->shift_to && side > US_SHIFT) k->shift_to = k->sp[2][2] < k->sp[3][2] ? 2 : 3;
    if (k->shift_to && k->shift < 1.0f) {   // sliding: its place in the hull from the middle toward the low side's float
        k->shift = fminf(k->shift + dt / US_SHIFT_S, 1.0f);
        const float *a = k->body[2], *b = k->body[3], *to = k->body[k->shift_to];
        for (int j = 0; j < 2; j++) {
            float mid = 0.5f * (a[j] + b[j]);
            k->body[4][j] = mid + 0.8f * k->shift * (to[j] - mid);
        }
    }
    us_below(k, below);   // (the ballast's as it slid)
    for (int round = 0; round < 4; round++) {
        for (int i = 0; i < k->n; i++)
            for (int j = i + 1; j < k->n; j++) {
                float d[3], e[3], len = 0.0f, rest = 0.0f;
                for (int c = 0; c < 3; c++) d[c] = k->sp[j][c] - k->sp[i][c], e[c] = k->body[j][c] - k->body[i][c], len += d[c] * d[c], rest += e[c] * e[c];
                len = sqrtf(len), rest = sqrtf(rest);
                if (len < 1e-6f) continue;
                // by their weights: the lighter moves the more (a float 1, the ballast US_BALLAST ...: us_w)
                float wi = 1.0f / us_w(k, i), wj = 1.0f / us_w(k, j), w = (len - rest) / (len * (wi + wj));
                for (int c = 0; c < 3; c++) k->sp[i][c] += d[c] * w * wi, k->sp[j][c] -= d[c] * w * wj;
            }
        for (int i = 0; i < US_PTS && !k->air; i++) {   // (an aircraft's ground: its middle, below)
            float *p = k->sp[i], *q = k->sq[i];
            int x = (int)floorf(p[0]), y = (int)floorf(p[1]);
            if (!mesh_has_tile(mesh, x, y)) continue;
            float bottom = mesh_z(mesh, p[0], p[1]) + below[i] - k->dig[i];
            if (p[2] < bottom) {   // on the bottom: held up, and dragged along it
                if (!k->floor_hit && (q[2] - p[2]) / dt > US_FLOOR_V) k->floor_hit = 1, k->hit_v = (q[2] - p[2]) / dt;   // (BREAKING ON THE SEABED)
                p[2] = bottom;
                if (q[2] < p[2]) q[2] = p[2];
                q[0] = p[0] - (p[0] - q[0]) * 0.8f, q[1] = p[1] - (p[1] - q[1]) * 0.8f;
            }
        }
    }
    if (k->air) {   // its middle on the ground or the seabed: it stays as it hit (a crash on land going down fast)
        float m[3] = { 0, 0, 0 }, mq = 0.0f, a[3], b[3], u[3], lu2 = 0.0f;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) m[j] += 0.25f * k->sp[i][j];
            mq += 0.25f * k->sq[i][2];
        }
        for (int j = 0; j < 3; j++) a[j] = k->sp[0][j] - k->sp[1][j], b[j] = k->sp[2][j] - k->sp[3][j];
        u[0] = a[1] * b[2] - a[2] * b[1], u[1] = a[2] * b[0] - a[0] * b[2], u[2] = a[0] * b[1] - a[1] * b[0];
        for (int j = 0; j < 3; j++) lu2 += u[j] * u[j];
        if (lu2 > 1e-12f) {   // the collider up its body, by US_HIT_HIGH (the way down its own as well)
            float h = k->hit / sqrtf(lu2);
            for (int j = 0; j < 3; j++) m[j] += h * u[j];
            mq += h * u[2];
        }
        if (mesh_has_tile(mesh, (int)floorf(m[0]), (int)floorf(m[1]))) {
            float ground = mesh_z(mesh, m[0], m[1]);
            if (m[2] <= ground) {
                if (ground >= k->zw && !(k->fx_done & US_FX_CRASH) && mq - m[2] > US_CRASH_V * dt) us_fx(k, US_FX_CRASH, m[0], m[1], ground);
                k->stuck = 1, k->burnt = !k->shatters && ground >= k->zw;
                if (k->shatters && ground >= k->zw && k->shatter_t < 0.0f) k->shatter_t = t;
            }
        }
    }
}
internal void us_snap(uc_corpse *k, float t, int floor);   // below
// THE HALVES KEPT APART (the player, 2026-11-13: "one remaining problem is them sliding into each other, maybe a simple
// sphere collison betwen the two pieces ... the "collison" is only betwen the two halfes"; a capsule each instead of a
// sphere - one big enough to keep them apart at the break held their middles apart too, and a V or the two on end side
// by side, the broken ends together, couldn't be): each half is a line along its length with a radius (US_HALF_R of its
// side floats' spacing), the line short of its ends by it, so at the break the two just touch. Closer than their radii,
// each half is moved apart by half the overlap, most where they touch (its points by how near they lie along it), and
// loses its way toward the other there - its places moved with it, its step before too, so the push adds no way: they
// come to rest against each other, not bounce. The bow's half does it after each of its steps against the stern's as it
// lies (stepped in its own run, a step apart at most); nothing else touches them.
#define US_HALF_R 0.5f
// a half's line a .. b (along its length u, bow-ward), its radius, its length, its middle
internal int us_capsule(const uc_corpse *k, float *a, float *b, float *u, float *r, float *len, float *mid) {
    float ll = 0.0f, w = 0.0f;
    for (int j = 0; j < 3; j++) {
        u[j] = k->sp[0][j] - k->sp[1][j], ll += u[j] * u[j];
        w += (k->sp[2][j] - k->sp[3][j]) * (k->sp[2][j] - k->sp[3][j]);
        mid[j] = 0.5f * (k->sp[0][j] + k->sp[1][j]);
    }
    ll = sqrtf(ll);
    if (ll < 1e-6f) return 0;
    *len = ll / 0.8f, *r = US_HALF_R * sqrtf(w);   // (the floats lie 0.4 of its length from its middle: us_body_half)
    float h = fmaxf(0.5f * *len - *r, 0.0f);
    for (int j = 0; j < 3; j++) u[j] /= ll, a[j] = mid[j] + h * u[j], b[j] = mid[j] - h * u[j];
    return 1;
}
internal float us_clamp01(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
// the closest points c1 on p1 .. q1 and c2 on p2 .. q2 (Ericson, Real-Time Collision Detection 5.1.9)
internal void us_closest(const float *p1, const float *q1, const float *p2, const float *q2, float *c1, float *c2) {
    float d1[3], d2[3], rr[3], a = 0, e = 0, f = 0, c = 0, b = 0, s = 0.0f, t = 0.0f;
    for (int j = 0; j < 3; j++) d1[j] = q1[j] - p1[j], d2[j] = q2[j] - p2[j], rr[j] = p1[j] - p2[j];
    for (int j = 0; j < 3; j++) a += d1[j] * d1[j], e += d2[j] * d2[j], f += d2[j] * rr[j], c += d1[j] * rr[j], b += d1[j] * d2[j];
    if (a < 1e-9f && e < 1e-9f) s = t = 0.0f;
    else if (a < 1e-9f) s = 0.0f, t = us_clamp01(f / e);
    else if (e < 1e-9f) t = 0.0f, s = us_clamp01(-c / a);
    else {
        float den = a * e - b * b;
        s = den > 1e-12f ? us_clamp01((b * f - c * e) / den) : 0.0f;
        t = (b * s + f) / e;
        if (t < 0.0f) t = 0.0f, s = us_clamp01(-c / a);
        else if (t > 1.0f) t = 1.0f, s = us_clamp01((b - c) / a);
    }
    for (int j = 0; j < 3; j++) c1[j] = p1[j] + s * d1[j], c2[j] = p2[j] + t * d2[j];
}
// one half moved by push along n (most at its contact c) and its way toward the other there taken (n: away from it)
internal void us_half_push(uc_corpse *k, const float *n, float push, const float *c, const float *u, float len, const float *mid) {
    float at = 0.0f;
    for (int j = 0; j < 3; j++) at += (c[j] - mid[j]) * u[j];
    for (int i = 0; i < k->n; i++) {
        float al = 0.0f, vn = 0.0f;
        for (int j = 0; j < 3; j++) al += (k->sp[i][j] - mid[j]) * u[j];
        float f = us_clamp01(1.0f - fabsf(al - at) / len);
        for (int j = 0; j < 3; j++) vn += (k->sp[i][j] - k->sq[i][j]) * n[j];
        for (int j = 0; j < 3; j++) {
            k->sp[i][j] += f * push * n[j], k->sq[i][j] += f * push * n[j];
            if (vn < 0.0f) k->sq[i][j] += f * vn * n[j];   // (its way toward the other gone)
        }
    }
}
internal void us_halves_apart(uc_corpse *k, uc_corpse *o) {
    float a1[3], b1[3], u1[3], m1[3], r1, l1, a2[3], b2[3], u2[3], m2[3], r2, l2, c1[3], c2[3], n[3], d = 0.0f;
    if (!us_capsule(k, a1, b1, u1, &r1, &l1, m1) || !us_capsule(o, a2, b2, u2, &r2, &l2, m2)) return;
    us_closest(a1, b1, a2, b2, c1, c2);
    for (int j = 0; j < 3; j++) n[j] = c1[j] - c2[j], d += n[j] * n[j];
    d = sqrtf(d);
    if (d >= r1 + r2) return;
    if (d > 1e-6f) for (int j = 0; j < 3; j++) n[j] /= d;
    else {   // (their lines cross: apart along the bow's half's length)
        for (int j = 0; j < 3; j++) n[j] = u1[j];
    }
    float push = 0.5f * (r1 + r2 - d), m[3] = { -n[0], -n[1], -n[2] };
    us_half_push(k, n, push, c1, u1, l1, m1);
    us_half_push(o, m, push, c2, u2, l2, m2);
}
// simulated on to the walk's game time (at most 3 s at once: one long out of the view jumps there); a ship that breaks
// in two broken as its time comes
internal void us_run(uc_corpse *k) {
    if (g_uc_now < 0.0) return;
    if (g_uc_now - k->sim > 3000.0) k->sim = g_uc_now - 3000.0;
    const void *mesh = map_mesh_at(game_map());
    uc_corpse *other = NULL;   // the bow's half: the stern's (THE HALVES KEPT APART)
    for (uint32_t i = 0; i < g_kc_n && k->half == 1 && !other; i++)
        if (g_kc[i].twin && g_kc[i].half == 2 && g_kc[i].serial == k->serial) other = &g_kc[i];
    while (k->sim + US_DT * 1000.0 <= g_uc_now) {
        k->sim += US_DT * 1000.0;
        float t = (float)((k->sim - k->died) / 1000.0);
        if (k->snap_t >= 0.0f && !k->half && t >= k->snap_t) us_snap(k, t, 0);
        us_step(k, t, mesh);
        if (k->floor_snap && k->floor_hit && !k->half && ++k->floor_hit > US_FLOOR_STEPS) us_snap(k, t, 1);   // (BREAKING ON THE SEABED)
        if (other) us_halves_apart(k, other);
    }
}
// its pose into t (rows, the place in the fourth column) from its floats p (its own, or where they were a step before):
// bow-ward from the stern to the bow, port-ward from starboard to port square to it, up square to both - the hull's own
// axes there
internal void us_pose_at(const uc_corpse *k, const float (*p)[3], float *t) {
    float a[3], b[3], u[3], la = 0.0f, lb = 0.0f, ab = 0.0f, mean[3] = { 0, 0, 0 }, body[3] = { 0, 0, 0 };
    for (int j = 0; j < 3; j++) a[j] = p[0][j] - p[1][j], b[j] = p[2][j] - p[3][j], la += a[j] * a[j];
    la = sqrtf(la);
    if (la < 1e-6f) return;
    for (int j = 0; j < 3; j++) a[j] /= la, ab += a[j] * b[j];
    for (int j = 0; j < 3; j++) b[j] -= ab * a[j], lb += b[j] * b[j];
    lb = sqrtf(lb);
    if (lb < 1e-6f) return;
    for (int j = 0; j < 3; j++) b[j] /= lb;
    u[0] = a[1] * b[2] - a[2] * b[1], u[1] = a[2] * b[0] - a[0] * b[2], u[2] = a[0] * b[1] - a[1] * b[0];
    const float up[3] = { 0.0f, 0.0f, 1.0f };
    for (int r = 0; r < 3; r++)   // the world's axes over the hull's: [a b u] times [along across up] turned
        for (int c = 0; c < 3; c++) t[r * 4 + c] = a[r] * k->along[c] + b[r] * k->across[c] + u[r] * up[c];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) mean[j] += 0.25f * p[i][j], body[j] += 0.25f * k->body[i][j];
    for (int r = 0; r < 3; r++) t[r * 4 + 3] = mean[r] - (t[r * 4] * body[0] + t[r * 4 + 1] * body[1] + t[r * 4 + 2] * body[2]);
}
internal void us_pose(const uc_corpse *k, float *t) { us_pose_at(k, (const float (*)[3])k->sp, t); }
// a half's points in the hull's own space: four floats and the ballast over its stretch a .. b along its length, as a
// whole ship's over its hull (the same beam, the ballast as deep)
internal void us_body_half(uc_corpse *k, float a, float b) {
    float c[3], ca = 0.0f, ha = 0.0f, z4 = k->body[4][2];
    for (int j = 0; j < 3; j++) c[j] = 0.25f * (k->body[0][j] + k->body[1][j] + k->body[2][j] + k->body[3][j]);
    for (int j = 0; j < 3; j++) ca += c[j] * k->along[j], ha += 0.5f * (k->body[2][j] - k->body[3][j]) * k->across[j];
    float mid = 0.5f * (a + b), hl = 0.4f * (b - a);
    for (int j = 0; j < 3; j++) {
        float base = c[j] + (mid - ca) * k->along[j];
        k->body[0][j] = base + hl * k->along[j], k->body[1][j] = base - hl * k->along[j];
        k->body[2][j] = base + ha * k->across[j], k->body[3][j] = base - ha * k->across[j];
        k->body[4][j] = base;
    }
    k->body[4][2] = z4;
    k->shift = 0.0f, k->shift_to = 0;
}
// a half's points placed by the frame's pose now and a step before (now, was), its leaks from now (t s after the death)
// by its own stream, starting from b - the ship's floats' mean buoyancy then -, its broken end dropping and the half
// drifting away from the other (sign: + bow-ward); floor: broken on the seabed (BREAKING ON THE SEABED)
internal void us_half_go(uc_corpse *k, const float *now, const float *was, float t, float b, float sign, int floor) {
    for (int i = 0; i < US_PTS; i++)
        for (int r = 0; r < 3; r++) {
            const float *p = k->body[i];
            k->sp[i][r] = now[r * 4 + 3] + now[r * 4] * p[0] + now[r * 4 + 1] * p[1] + now[r * 4 + 2] * p[2];
            k->sq[i][r] = was[r * 4 + 3] + was[r * 4] * p[0] + was[r * 4 + 1] * p[1] + was[r * 4 + 2] * p[2];
        }
    uint32_t s = us_seed(k->serial, 0x27220A95u + k->half);
    int cut = k->half == 1 ? 1 : 0, end = 1 - cut;   // the bow's half's stern float lies at the break, the stern's half's bow float
    float at = t + US_LEAK_HEAD - k->wet_at;   // the leaks' clock now (us_step's u)
    for (int i = 0; i < 4; i++) k->leak[i][3] = b;
    k->leak[cut][0] = -0.4f - 0.4f * us_rnd(&s), k->leak[cut][1] = at, k->leak[cut][2] = 0.6f + 0.8f * us_rnd(&s);
    k->leak[end][0] = 0.3f + 0.4f * us_rnd(&s), k->leak[end][1] = at + 5.0f + 4.0f * us_rnd(&s), k->leak[end][2] = 10.0f + 6.0f * us_rnd(&s);
    for (int i = 2; i < 4; i++)
        k->leak[i][0] = 0.2f + 0.4f * us_rnd(&s), k->leak[i][1] = at + 1.5f + 2.0f * us_rnd(&s), k->leak[i][2] = 6.0f + 5.0f * us_rnd(&s);
    if (floor) {   // its broken end and one side deeper in the seabed (its leaks as a broken half's on the water)
        // a point of it placed into the seabed counts as dug in as deep as it lies (the player, the same day: "the pice
        // hitting the ground bounces/ fling up a bit sometimes": its new points - over its own stretch, each with its
        // own dig - could start under their ground, and the push out of it went through the half as a fling)
        const void *mesh = game_map() ? map_mesh_at(game_map()) : NULL;
        float below[US_PTS];
        us_below(k, below);
        for (int i = 0; i < US_PTS && mesh; i++) {
            const float *p = k->sp[i];
            if (!mesh_has_tile(mesh, (int)floorf(p[0]), (int)floorf(p[1]))) continue;
            float under = mesh_z(mesh, p[0], p[1]) + below[i] - k->dig[i] - p[2];
            if (under > 0.0f) k->dig[i] += under;
        }
        k->dig[cut] += US_FLOOR_SAG;
        k->dig[2 + (us_rnd(&s) < 0.5f)] += 0.4f * US_FLOOR_SAG * us_rnd(&s);
    }
    float al[3], dt = (float)US_DT;
    for (int r = 0; r < 3; r++) al[r] = now[r * 4] * k->along[0] + now[r * 4 + 1] * k->along[1] + now[r * 4 + 2] * k->along[2];
    for (int i = 0; i < US_PTS; i++)
        for (int r = 0; r < 3; r++) k->sq[i][r] -= sign * US_SNAP_PART * dt * al[r];
    k->sq[cut][2] += US_SNAP_DROP * dt;
}
// it breaks (A SHIP BREAKING IN TWO) t s after its death - floor: on the seabed (BREAKING ON THE SEABED): this entry the
// bow's half, a new one the stern's; none when the table is full (it goes down whole)
internal void us_snap(uc_corpse *k, float t, int floor) {
    k->snap_t = -1.0f, k->floor_snap = 0;
    static LONG logged;
    if (floor && g_log && InterlockedIncrement(&logged) <= 16)
        log_locked("# wrecks: a ship (serial %d) broke on the seabed %.1f s after its death, going down at %.2f tiles/s\n", (int)k->serial, (double)t, (double)k->hit_v);
    if (g_kc_n >= UC_CORPSES) return;
    float now[16], was[16], b = 0.0f;
    us_pose_at(k, (const float (*)[3])k->sp, now), us_pose_at(k, (const float (*)[3])k->sq, was);
    for (int i = 0; i < 4; i++) {   // the floats' buoyancy now (us_step's), their mean
        float u = (t + US_LEAK_HEAD - k->wet_at - k->leak[i][1]) / k->leak[i][2];
        u = u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u * u * (3.0f - 2.0f * u);
        b += 0.25f * (k->leak[i][3] + (k->leak[i][0] - k->leak[i][3]) * u);
    }
    uc_corpse *o = &g_kc[g_kc_n++];
    *o = *k;
    graphics_acquire(o->gfx);
    o->twin = 1, o->gone = 1, o->in_game = 0, o->half = 2, k->half = 1;
    if (!floor) o->sim -= US_DT * 1000.0;   // (on the water the step at t is still to come for both: us_run takes this one's next)
    us_body_half(k, k->cut, k->ends[1]);
    us_body_half(o, k->ends[0], k->cut);
    us_half_go(k, now, was, t, b, 1.0f, floor);
    us_half_go(o, now, was, t, b, -1.0f, floor);
}
// a broken half's cut for its record (render_units.c A BROKEN SHIP'S HALF): the break's plane in the world by its pose t,
// its normal toward the half kept
internal ee_texture *us_death_texture(ee_player *pl);   // collect_remains.c
internal void us_clip_tag(const void *rec, const uc_corpse *k, const float *t) {
    if (!k->ship || !k->half) return;
    float sg = k->half == 1 ? 1.0f : -1.0f, pl[4], nt = 0.0f;
    for (int r = 0; r < 3; r++) {
        pl[r] = sg * (t[r * 4] * k->along[0] + t[r * 4 + 1] * k->along[1] + t[r * 4 + 2] * k->along[2]);
        nt += pl[r] * t[r * 4 + 3];
    }
    pl[3] = -nt - sg * k->cut;   // (the hull's place x: along . (x - its place) = cut, turned into the world)
    us_death_texture(k->owner);   // (the filling's)
    ud_clip_tag(rec, g_uc_walk_gen, (uint32_t)k->serial, k->half, pl, k->deck);
}
internal int us_burnt(const uc_corpse *k);   // below: a crashed aircraft drawn burnt out
// a shattering flier's animation, frame, next frame and fraction now, and its pose t upright on the ground once it
// shatters; others' left as they are
internal void us_shatter(const uc_corpse *k, float *t, int32_t *anim, int32_t *frame, int32_t *next, float *fraction) {
    if (!k->ship || !k->shatters) return;
    if (k->shatter_t < 0.0f) {
        *anim = k->fly_anim, *frame = *next = k->fly_frame, *fraction = 0.0f;
        return;
    }
    int32_t first = ((const int32_t *)dmg_model_anim_first_at(k->gfx))[US_SHATTER_ANIM],
            last = ((const int32_t *)dmg_model_anim_last_at(k->gfx))[US_SHATTER_ANIM],
            ms = ((const int32_t *)dmg_model_anim_frame_ms_at(k->gfx))[US_SHATTER_ANIM], n = last - first + 1;
    float f = (float)((k->sim - k->died) / 1000.0 - k->shatter_t) * 1000.0f / (ms > 0 ? (float)ms : US_SHATTER_MS);
    f = f < 0.0f ? 0.0f : f;
    int32_t i = (int32_t)f;
    *anim = US_SHATTER_ANIM, *frame = first + (i < n - 1 ? i : n - 1), *next = first + (i + 1 < n - 1 ? i + 1 : n - 1);
    *fraction = i < n - 1 ? f - (float)i : 0.0f;
    float ex = t[0], ey = t[4], l = sqrtf(ex * ex + ey * ey), cx = 0.0f, cy = 0.0f;   // its x axis laid flat
    if (l < 1e-4f) ex = 1.0f, ey = 0.0f, l = 1.0f;
    ex /= l, ey /= l;
    for (int i2 = 0; i2 < 4; i2++) cx += 0.25f * k->sp[i2][0], cy += 0.25f * k->sp[i2][1];
    const void *mesh = game_map() ? map_mesh_at(game_map()) : NULL;
    t[0] = ex, t[1] = -ey, t[2] = 0.0f, t[3] = cx;
    t[4] = ey, t[5] = ex, t[6] = 0.0f, t[7] = cy;
    t[8] = 0.0f, t[9] = 0.0f, t[10] = 1.0f;
    t[11] = mesh && mesh_has_tile(mesh, (int)floorf(cx), (int)floorf(cy)) ? mesh_z(mesh, cx, cy) : t[11];
}
