// THE MOD'S ARMIES ([ai_base] mod_armies; [autotest] v_mod_armies for the variant players; land maps - navy_land).
// Included by eeaimod.c after stage.c (mass.c ms_land_soldier, landing.c landing_has_unit, army.c ou_sortie,
// attack.c hook_unit_to_group, navy.c navy_land, stage.c stage_foe, fronts.c fronts_station).
// The player (2026-11-01): the game's attack commander is "dumb as rocks" - it grabs every soldier the moment it is
// trained, splits a wave into squads of about ten and sends each on its own tour of enemy buildings with waits at every
// stop; the mod's mass, stage, commit and focus each patch one symptom of that through the same veto on its orders.
// So the commander is replaced for land armies, nothing below it: the game keeps training (army.c shapes the
// requests), unit fighting and movement, ships and aircraft.
// OWNERSHIP: every land soldier the commander would take (combat_mgr_unit_free: a type that fights, alive, moving, not
// chained) is held the game's own way (player hold_unit: the unit's ai_held set, taken out of the commander's pool and
// groups) within AR_TICK of being free, and the commander's offer of a freed unit to its groups (unit_to_group,
// attack.c hook_unit_to_group) is refused for these players. The commander's groups form from those offers - a new
// unit joins a group or founds one, long before the attack time; the launch (0x46F4FF) only sends the formed groups
// (one with no units formed when the offers were refused: 2026-11-02) - so with the offers refused its groups stay
// empty, its launch finds nothing and schedules the next attack time: the commander's own timing stays.
// THE FORCE: the held soldiers are one force, ordered as one (the player's own way: the whole force of the region
// goes, holds, and comes back together). Its home: the front station (fronts.c: a bit behind the tower nearest the
// heaviest front), the staging point when there is none, else the base - a member idle away from it walks there. At
// the attack time (player next_attack) the force takes the enemy building nearest its home that it knows (seen_by:
// seen or remembered) when the whole force beats the enemy there (stage_foe: the fighters seen and remembered within
// SG_FOE, the defences covering it) by AR_GO_PCT: every member attack-moves to it (the game's attack-move: it fights
// what it meets and walks on). Under that the force waits at home and grows - the ratio grows with its numbers. Out,
// every AR_JUDGE_EVERY the force is judged as a whole - all its members out against the enemy at its middle and at
// its destination, the lesser - and under AR_BACK_PCT twice running it goes home as one (a plain move: no fighting on
// the way; as a group: AR_WAY below). The building down (none seen within AR_DONE of the destination with a member there), the next: the known
// enemy building nearest the force's middle, judged the same - under AR_GO_PCT: home. A raid (AR_RAID_BAND enemy land
// fighters seen within AR_RAID_R of an own building, or one attacking something of ours, within AR_RAID_REACH of
// home, seen there twice running) while the force is home: the whole force attack-moves to it - one order to the
// force, not one per soldier - and goes home once no enemy fighter is seen there. Soldiers trained while the force
// is out gather at home (not sent after it one by one) and join the next push. A member idle away from where the
// force was sent is ordered again, at most every AR_REORDER.
// Measured 2026-11-02 (head-to-head, Continental huge 1v1 late, 75 min, 24 mirrored games; a game: 1.6 pushes, 3.5
// further legs, 12.3 raids answered, 1.7 retreats as losing, 0.25 next targets refused, no wait at the attack time -
// the enemy there is unknown then -, 3260 orders): soldiers +9.3 (se 7.7), kills +9.3 (12.6), losses -15.8 (13.9),
// razed +2.4 (4.4), workers +14.6 (11.3); buildings lost 8.7 against the commander's 11.2 - the first owned army
// that loses fewer than the commander (the parity release: 6.2 against 4.5). A second 24 (seeds 13-24) brought the
// 48 to soldiers +10.6 (5.1), kills +1.2 (9.3), losses -5.6 (9.9), razed +1.4 (2.4), buildings lost 8.2 / 9.6: level.
// The judge lines of a push showed why: the force of 31 walked into the enemy base judged 1.3-3 all the way while its
// hit points went 10700 -> 2900 in 3 minutes - the enemy's soldiers arrive one by one at the middle, the ratio stays
// above 1 and the force is ground down; it was called losing with 19 of 71 left on average (84 of 96 pushes). So
// home when the force is no longer winning: AR_BACK_PCT 70 -> 100 (48 games): soldiers +12.9 (se 5.5), kills +30.4
// (7.9), losses -31.9 (7.9), razed +3.3 (1.6), buildings lost 7.2 against 10.6. The same with 70 and a bleed rule
// instead (home once the force has lost half the hit points it went out with, AR_BLEED_PCT 50; 48 games): kills
// +31.9 (6.9), losses -33.0 (7.0), razed +1.8 (1.3), soldiers +6.9 (4.1), buildings lost 5.6 / 7.4 - as good on the
// fight, less on the buildings; 100 kept, the bleed rule left in at 0. Confirmed with 100 on other setups (24
// mirrored games each): 4 AIs on Continental huge late - soldiers +18.4 (6.0), kills -22.0 (9.0), losses -33.1
// (10.7), razed +7.3 (3.0), buildings lost 11.7 / 20.2; Mediterranean huge 1v1 late - kills -6.8 (13.0), losses
// +5.8 (13.4), razed +3.3 (1.5), workers -10.2 (5.7), buildings lost 6.8 / 10.1; from epoch 1 on Continental large,
// 100 min - kills +19.3 (13.1), losses -3.7 (18.5), razed +11.3 (5.4), buildings lost 15.5 / 28.7 (23 raids a game
// there). On by default since 2026-11-02 ([ai_base] mod_armies). The raid's band (AR_RAID_BAND 3, before: any
// enemy fighter seen near a building; 48 games): raids 12.4 -> 9.8 a game, orders 2876 -> 2567, every score level
// (kills +1.5 se 7.4, losses -1.1, razed -0.4, buildings lost 6.5 / 6.9) - nearly every raid answered is one enemy
// actually attacking something of ours; kept as the sanity bound it is. PRESSING ON (AR_PRESS 1: after a push, the
// next as soon as the force beats by AR_GO_PCT the enemy that sent it home, 1.9 presses a game; 48 games): kills
// -35.3 (se 9.6), losses +34.2 (9.5), razed -0.1, buildings lost 5.7 / 5.6 - the judge at the target from home sees
// nothing of a base it hasn't scouted, so the force walks back into what beat it and bleeds again before the retreat
// rule bites. Only after a push that razed and wasn't driven off (AR_PRESS 2): 0.2 presses a game, level (kills -6.9
// se 8.1, razed +0.7 se 1.5). Off: the game's attack times stay the push times. THE FORTIFIED LINE (AR_FORT: home at
// the own defence near the station covered by the most defences, found on 2026 of 2250 home checks in a smoke game;
// 48 games): level - soldiers -7.9 (se 5.2), kills +1.7 (7.2), losses -3.1 (7.1), razed -0.3, buildings lost 6.6 /
// 6.3. Off. The Mediterranean workers -10.2 of the confirmation was noise: a second 24 there gave +0.4 (6.5), razed
// +2.7 (2.0). THE ASSUMED ENEMY (AR_ASSUME; Continental huge 1v1 late, 75 min): at 100 - kills +6.9 (se 7.7),
// losses -6.8, razed -1.0 (0.7), buildings lost 5.5 / 4.5, 0.5 waits a game (48 games): more careful, nothing won;
// at 50 - 48 games kills +12.4 (7.1), losses -12.4, the next 48 the exact reverse: 96 games level (kills -0.0 se
// 5.3). In a 75-minute game the assumption is full only for the last quarter, so from epoch 1 on Continental large,
// 100 min, at 100 (24 games): pushes 2.1 -> 0.9 a game, 1.1 waits, legs 4.3 -> 1.5, razed -4.0 (se 5.4), buildings
// lost 19.2 / 15.7, kills and losses level - the force sat at home while the enemy kept coming. The blind push is
// already cheap: it goes, meets what is there and turns home when it is no longer winning; the pushes the assumption
// skipped would have razed. Off. THE WAY HOME, measured (24 games, both sides the force; "home from" lines): a push
// starts with 69 soldiers, turns home with 45 (35% lost in the fight) and has 33 alive two minutes after the home
// order - 27% of what turned, half of what went, dies on the way; a raid's way home costs 12%. The retreat is a
// plain move with the enemy at the force's back. Against that: the way home as an attack-move (AR_BACK_FIGHT, the
// members fight what catches them) - 48 games razed +2.7 (se 1.2), buildings lost 4.2 / 6.9, the next 48 kills -17.5
// (8.4), losses +15.8, razed -0.9, and more lost on the way (72 against 55 a game: the members stop to fight the
// pursuers); 96 games level (kills -8.7 se 6.1, razed +0.9 se 1.0). Off. Home at 120% instead of 100 (v_ar_back_pct;
// 48 games): level, 15.8 retreats a game instead of 11.7. 100 kept. Who dies on the way (cover.c's "# cvd" lines
// matched to the pushes' home orders, 24 games): ranged 37% of the dead against 25% in the fight, cavalry 6%
// against 15% - the slow die, the fast get away. A cavalry rearguard (AR_BACK_FIGHT 2: the cavalry attack-moves
// home, the rest walks; 48 games): kills +10.5 (se 7.5), losses -11.1 (7.7), razed -1.5 (0.8), buildings lost 5.6 /
// 4.2, lost on the way 71.7 against 70.6 a game - the cavalry's fight doesn't cover the slow. Off. THE WAY HOME AS
// A GROUP (AR_WAY, the player's design: stay a group, walk home together, turn on the chasers you beat once out of
// the enemy base): the convoy alone (AR_WAY 1; 48 games) cut the losses on the way from 27 to 12 a game but the
// scores stayed level (kills -6.4 se 8.4, losses +7.8) - the saved soldiers died later anyway; the convoy with the
// stand (AR_WAY 3; 96 games): lost on the way 27 -> 7 a game, 2.8 stands a game, razed +2.3 (se 1.05), buildings
// lost 4.6 against 6.9, kills -0.7 (5.7), losses +1.0 (5.7), further legs 1.2 -> 2.0 a game (the force survives its
// retreats and pushes on); orders 2750 -> 5540 a game (the convoy's every 10 s). On by default (AR_WAY 3). The
// game's group move home instead of the convoy (AR_WAY bit 4: the mod's formations, formation_march and
// formation_follow keep the group together - the player: "we added a proper formation march"; 48 games against the
// convoy, both with the stand): lost on the way 33 against the convoy's 7 a game, kills -26.1 (se 9.4), losses
// +24.2 (9.3), razed +0.6 - the march at the slowest unit's speed is slower than the convoy's bursts (the fast run
// AR_WAY_STEP ahead and wait) and the chasers catch it; two minutes after the home order half the group was still
// walking. Off: the convoy is the retreat. FORMATIONS elsewhere (AR_FORM, the player: "use these formations more
// where possible"; 48 games each): a push's approach march (bit 1) - kills -17.9 (se 9.4), losses +18.0, razed -1.3
// (1.05), workers -10 (5.9), buildings lost 5.9 / 4.5: a loss, the force arrives later and the enemy's force meets it
// whole; the gather at home as a group move (bit 2, ~800 a game as members come) - level (kills +3.5 se 8.9, razed
// +0.25, buildings lost 6.4 / 6.7); the gather and a raid's approach (bits 2 + 4) - kills +4.7 (8.5), losses -4.8,
// razed -1.0 (1.2), buildings lost 6.2 / 5.2; all three - razed -1.6 (1.7), buildings lost 7.5 / 5.9, workers -7.
// On: the gather (the force stands at its station in formation, at no cost); off: the approach marches.
// RE-MEASURED 2026-11-11 after 85378bbe (until then every group move of the second computer player went to the
// first's selection: in a 1v1 head-to-head P3's gathers walked P2's last group to P3's station, and every result above
// from the gather's day on carries it). Mirrored head-to-head, Continental huge 1v1 from epoch 5, 120 min, 20 games a
// cell, B - A: the gather off - kills -19 (se 29), losses +29 (33), razed +4.7 (4.6): on; a push's approach march -
// kills +20 (24), losses -24 (30), then 60 more games: soldiers +4.6 (4.8), kills +2.9 (12), losses 0.0 (17), razed
// +0.4 (3.5) - level, not the loss it was, off; a raid's approach - kills +13 (23), losses -10 (27), razed -1.5: off;
// the group move home (AR_WAY 7) - kills -8 (20), losses -19 (35), razed -5.5 (6.3): off. The force against the
// commander (v_mod_armies=0 B): soldiers -18 (7.4), kills -41 (24), losses +21 (20), razed +12 (6.1), gold +27 a
// minute (14) for the commander - the force keeps it. AR_JUDGE off: kills -7 (25), losses -19 (28); AR_ROUTE off:
// soldiers -7.7 (8.2), razed -3.3 (7.4); AR_FIRE on: kills -18 (26), losses +15 (25); AR_BREACH on: soldiers -8.4
// (5.5), losses +27 (27); AR_PRICE on: soldiers +13 (6.7), kills +13 (22), losses -27 (34), then 60 more: soldiers +0.3
// (4.6), losses +7.5 (14), gold+iron -0.9 (11) - level. Every default stays. The towns' parts (towns.c), same setup:
// fort - razed -6.9 (6.5), losses +22 (34); take_mines - soldiers -9.8 (5.6); mine_raids - kills -142 (22), losses +127
// (33): all three off. ally_aid 100 against 0 (Continental large, 4 AIs in pairs, 90 min, 20 games each against the
// game's AI): the mod's team won 6 against 10 of 20, soldiers +73 against +98 over the game's AI: off.
// BEFORE THE FORCE (2026-11-02, head-to-head, Continental huge 1v1 late, 75 min, 24 mirrored games each): PARITY - the
// held let go to the game's own launch a tick ahead of the attack time (ai_held cleared, register_unit): soldiers
// +14.5 (se 6.2), kills +1.3 (6.9), losses -8.0 (8.5), razed -1.8 (1.6), workers -8.1 (10.5) - the soldiers trained
// between two waves waiting at home instead of trickling after one out. On top of it: the held standing at the front
// station between waves (front_army, ~720 station walks a game) - soldiers +8.5 (6.1), kills -2.7 (11.8), losses +4.3
// (12.9), razed -0.8 (1.8): nothing; the release only when the army beats the strongest enemy army known by
// army_mass_ratio (army_mass WAIT) - it never waited (the enemy army is rarely known before the attack: intel's
// soldiers seen within 10 min), parity with noise. A retreat of the game's wave (called off under 70%, judged at the
// members' middle with the own fighters within SG_NEAR of it): soldiers +10 (4.3), kills -5.8 (9.1), losses +3.7
// (10.0), razed -4.5 (1.7), workers -17 (7.3) - the judge misread a spread wave (a wave of 95 against one unit and
// towers called off). A defence on top - every held soldier within 30 of a raid sent at the raider nearest it: kills
// -34 (15.6), losses +29 (16.3), razed -3.6 (1.2): 195 raids a game answered with 1353 single orders, the soldiers
// thrown at the enemy's wave one at a time. Buildings lost A / B: parity 4.5 / 6.2, station 5.8 / 6.7, retreat 3.0 /
// 7.5, defence 2.6 / 6.2 - holding the soldiers idle between waves costs about 1.7 buildings each way: the commander
// keeps 3-4 squads out all the time (continuous pressure), and its soldiers at home stand in groups that go for what
// comes near them together. Every add-on that ordered the held one by one lost or changed nothing: the trickle. A
// launch of the mod's own as one of the game's groups (ag_launch) at the nearest known enemy building: razed 3.5
// against the commander's 10.3 (-6.8, se 2.1) - the wave went home once its one building fell. The game's
// attack-on-a-player form (a task with the target player only, strategy kind 0) is refused by attack_plan_ok (it
// wants a target unit or a location); the game's own launch doesn't ask it.
// A SAVED GAME (the player's save "attack", 2026-11-03: of P3's 252 soldiers the force knew 103 after the load, 102
// stood idle in its base for good): the game saves its units' ai_held, the mod's force was not saved - so every
// soldier held at the save was nobody's after a load (the commander skips the held, the force took only the free) and
// a push under way was forgotten. Now g_armies goes into the save (persist.c; its unit pointers are of the old session:
// found again by id at the player's first tick), and a load without it (a save of before, a block of another layout)
// takes the held land soldiers into the force as they are.
// "# armies:" once a game minute (autotest) and at every order to the force (every game: why a push turned home).
#define AR_TICK 2000
#define AR_MIN_LAUNCH 8            // soldiers a push needs at least (the outdated sortie's)
#define AR_MIN_RAID 3
#define AR_HELD 2048
#define AR_HOME_R 10               // at home: within this of it
#define AR_DET_MIN 30              // MINE RAIDS: soldiers at home a detachment, or a site asked of the force, needs
#define AR_DET_PAUSE 180000        // MINE RAIDS: between detachments, and after the force came home before a site asked of it
#define AR_CLAIM_MS 240000         // TAKE MINES: the force holds a razed mine for its town at most this long
#define AR_GO_PCT 130              // a push: the force beats the enemy at the target by this (army_stage_go's 130)
#ifndef AR_BACK_PCT
#define AR_BACK_PCT 100            // out: the force's fight under this twice running - home (70 bled: below)
#endif
// THE JUDGE OUT (the player, 2026-11-04, after a raid of 75 (hp 34588) on 41 (16797) turned home 27 tiles short of
// them without a shot: judged 1.11 as it went, 0.85 thirty seconds later as more of his base came into view - "let it
// only turn back on a clear deficit"; his complaint "it aborted attacks that could have overrun me"). AR_JUDGE 1:
// before contact (fewer than AR_CONTACT_MIN of the members out fighting - their order an attack) the estimate turns
// it home only under AR_FAR_PCT twice running; in contact the fight itself judges - the hit points the force lost
// (members out: their drop between two judgements, a dead one's last) against those it took off the enemy (the enemy
// seen within SG_FOE of its middle at the last judgement: their drop, a dead one's last; buildings too), summed over
// the last AR_TRADE_N judgements in contact: home once it lost AR_TRADE_BAD % of what it dealt, over AR_TRADE_MIN of
// them at least. AR_JUDGE 0: the estimate under AR_BACK_PCT twice running, in contact or not (v_ar_judge for tests).
#ifndef AR_JUDGE
#define AR_JUDGE 1
#endif
#define AR_FAR_PCT 70
#define AR_CONTACT_MIN 3           // members fighting: at least this many, or a tenth of the force out
#define AR_TRADE_N 8               // judgements in contact summed (AR_JUDGE_EVERY apart: half a minute)
#define AR_TRADE_MIN 4
#define AR_TRADE_BAD 150           // lost / dealt in % that sends it home
#define AR_TRADE_FLOOR 10          // ... once what it lost is this % of the hit points it went out with (the player's save "choke wolf": raids of 140 went home after losing 9 hit points, nothing dealt - the shooters out of sight)
#define AR_FOE 512                 // enemy units kept for the trade
// THE PRICE (AR_PRICE; v_ar_price): the player, 2026-11-04, on his save "choke wolf": "the problem isn't that the ai
// trains armies, it's that it burns them without being able to show anything for it ... it will get hurt badly if it
// isn't smart about it". The estimate (st_ratio: both sides' hit points and damage) assumes both sides can hit each
// other; at his chokepoint his artillery, mortars and towers fired while P3's soldiers could not reach them - the
// force judged itself 2-3x the enemy, went, turned home trading badly (lost 5000-8000 hit points against 0-1200
// dealt) and judged the same 4 minutes later: 61-78% of its gold + iron income lost in units. So a push sent home by
// its trade leaves the price at its target - what it lost and dealt there, with the hit points it went out with -
// for AR_PRICE_MS: an attack time (or pressing on) at a target within AR_PRICE_R of it judges by the trade it got
// there, dealt / lost x the force's hit points now over then, when that is less than the estimate. Too strong so, it
// waits (and the air force bombs it: a->strong); a wing's targets there all down clears the price (airstrike.c).
// OFF: on the save (3 timings, P3 the variant) 153 -> 180 min units lost 13-17k gold + iron instead of 22-28k, the army
// 323-431 instead of 160-245 - but razed 13-20 instead of 29-47 (2 pushes, not 4-6); on to 210 min razed 63, 22, 108
// against 56, 24, 80 without, and twice it sat with 736-750 soldiers and razed 22: dealt nothing, the price stays
// whatever the force's size. A priced target should send it to another, not keep it home: the way search (AR_ROUTE)
// takes a target whose price the force can't pay only when every other is priced too. Measured so (to 210 min, 3
// timings): razed 99, 44, 63 (with the air force 84, 22, 105) against 56, 24, 80 off and 86, 108, 93 with the air force
// alone; three of the six still sat with 720-740 soldiers - behind his chokepoint every target is near the priced
// spot. Still off.
#ifndef AR_PRICE
#define AR_PRICE 0
#endif
#define AR_PRICE_R 20
#define AR_PRICE_MS 900000
#define AR_PRICES 8
#ifndef AR_BACK_FIGHT
#define AR_BACK_FIGHT 0            // the way home: 1 an attack-move (the members fight what catches them), 2 the cavalry's only (a rearguard: the fast cover the slow), 0 a plain move
#endif
#ifndef AR_WAY
#define AR_WAY 3                   // the way home: bit 1 a convoy (every AR_WAY_EVERY every member to a point AR_WAY_STEP ahead of the group's middle: the fast wait for the slow), bit 2 a stand (the chasers attacked once the group is clear of the enemy's defences and AR_WAY_CLEAR from the target, when it beats them by AR_GO_PCT), bit 4 the game's group move instead of the convoy (the mod's formation march and follow keep the group together)
#endif
#define AR_WAY_EVERY 10000
#define AR_WAY_STEP 12
#define AR_WAY_CLEAR 20
#ifndef AR_FORM
#define AR_FORM 2                  // formations (the game's group move, the mod's march and follow): bit 1 a push's approach march to AR_APPROACH short of the target, the attack-move from there; bit 2 the gather at home; bit 4 a raid's approach
#endif
#define AR_APPROACH 16             // tiles short of the target the approach march ends (outside the towers' reach)
#define AR_APPROACH_MAX 150000     // the approach at most this long - then the attack-move as they are
#define AR_APPROACH_PCT 60         // the approach done: this much of the force within 2 x AR_DONE of its point
#ifndef AR_BLEED_PCT
#define AR_BLEED_PCT 0             // out: the force's hit points down by this much of what it went out with - home (0: never)
#endif
#define AR_JUDGE_EVERY 4000
#define AR_DONE 8                  // the destination reached: a member within this of it
#define AR_RAID_EVERY 4000
#define AR_RAID_R 14               // a raid: an enemy land fighter seen within this of an own building
#define AR_RAID_REACH 60           // raids the force answers: within this of home
#define AR_REORDER 20000           // a member ordered again at most every this
#define AR_JIT 5                   // the orders' spread: a 5 x 5 square of tiles
#define AR_HOME_MOVE 12            // home moves when the station is this far from it for AR_HOME_STAY (at once when home is in danger)
#define AR_HOME_STAY 60000
#define AR_RAID_SAME 10            // a raid: a raider seen twice running (AR_RAID_EVERY apart) within this of each other
#ifndef AR_PRESS
#define AR_PRESS 0                 // pressing on: 1 after any push, 2 after one that razed and wasn't driven off - the next as soon as the force beats what stopped it (0: the attack time only; 1 lost: below)
#endif
#define AR_PRESS_EVERY 10000
#define AR_PRESS_HOME_PCT 75       // pressing on: this much of the force gathered at home
#ifndef AR_FORT
#define AR_FORT 0                  // home at the own fortified line: the own defence within AR_FORT_R of the station covered by the most own defences (level: above)
#endif
#define AR_FORT_R 25
#ifndef AR_ASSUME
#define AR_ASSUME 0                // the push: the unseen enemy at the target assumed this % of the own army's strength (0: the seen only)
#endif
#define AR_ASSUME_MIN 60           // the assumption grows with the game's minutes to its full at this
#define AR_WIN 15                  // the kills and losses of the last this many minutes correct it
#ifndef AR_RAID_BAND
#define AR_RAID_BAND 3             // a raid: this many enemy fighters seen within AR_RAID_R of the raider, or one of them attacking ours
#endif
internal int g_v_ar_judge = -1, g_v_ar_far_pct = -1;
internal float g_ar_raid_answer = 1.0f;   // [ai_base] raid_answer: a raid answered when the force beats it by this
internal int g_ar_leash = 0;               // [ai_base] raid_leash: tiles past a raid's spot a member is called home (0 off)
internal int g_ar_aid = 0;                 // [ai_base] ally_aid: ALLY AID's reach from home (0 off)
internal unsigned g_ar_aids[16];           // ALLY AID: raids on an ally answered (the log's; not saved)
internal int g_mod_armies = 0, g_v_mod_armies = -1, g_v_ar_raid_band = -1, g_v_ar_press = -1, g_v_ar_fort = -1, g_v_ar_assume = -1, g_v_ar_back_pct = -1, g_v_ar_back_fight = -1, g_v_ar_way = -1, g_v_ar_form = -1;   // [autotest] v_ar_way: AR_WAY, v_ar_form: AR_FORM; v_ar_raid_band, v_ar_press, v_ar_fort, v_ar_assume, v_ar_back_pct, v_ar_back_fight: the variant players' AR_RAID_BAND, AR_PRESS, AR_FORT, AR_ASSUME, AR_BACK_PCT, AR_BACK_FIGHT
#define MOD_ARMIES(idx) PV(idx, g_v_mod_armies, g_mod_armies)
internal struct armies_st {
    uint32_t t, log_t;
    int n;
    struct { ee_unit *u; int id; uint32_t order_t; uint8_t out, back, pick; int hp; } held[AR_HELD];   // id: the unit's serial (unit_by_id's key - unit +0x60 is no unit's own: a save's members were never found again by it, and the trade counted every enemy it had seen as killed); out: went with the force; back: on its way home from the last home order; pick: in the group move being given; hp: at the last judgement (-1 none yet)
    int out;                               // 0 home, 1 a push, 2 a raid
    ivec2 home, dest, cand, raid_c;         // home; the force's destination; the next home seen (since cand_t); the raider seen last
    ee_unit *target; int target_id;        // the push's building (NULL: a raid's point)
    uint32_t judge_t, raid_t, out_t, wait_due, home_t, cand_t, raid_c_t, press_t;
    int weak, clear, stopped;              // judgements running under AR_BACK_PCT; raid checks running with no enemy seen; a push has ended (2: with its target down, not driven off)
    float ratio, out_hp, stop_hp;          // the last judgement; the force's hit points as it went out; the enemy's that ended the last push
    unsigned held_all, pushes, presses, legs, raids, backs, waits, orders, refused, forted;   // forted: home checks that found a defence to stand at
    unsigned leashed;                      // members called home chasing past raid_leash
    int kills[AR_WIN + 1], losses[AR_WIN + 1], win_n;   // the game's own counts of units killed and lost, a sample a minute (the oldest first)
    uint32_t back_t; int back_n, back_kind; float back_hp;   // the last home order: when, how many went, from what (1 a push, 2 a raid), their hit points
    unsigned way_n, way_lost; float way_hp, way_hp_lost;     // the measure: soldiers sent home and lost on the way, their hit points and those lost
    uint32_t way_t; unsigned stands, group_moves;            // the convoy's last order; stands against the chasers; group moves given
    int stage; ivec2 approach; uint32_t stage_t;             // out: 1 on the approach march to approach (since stage_t), 0 the attack-move
    unsigned approaches, gathers;                            // approach marches, gathers at home as group moves
    uint32_t win_t;
    struct { int id, hp; } foe[AR_FOE]; int nfoe;              // the trade: the enemy near the force at the last judgement
    float dealt[AR_TRADE_N], lost[AR_TRADE_N]; int trade_n;  // the trade of the last judgements in contact (a ring, trade_n of them so far)
    unsigned contacts, traded_home;                          // judgements in contact; homes sent by the trade
    ivec2 way[16]; int nway, way_i;                          // the way round (AR_ROUTE): its corners, the one walked to (stage 2)
    int breach, way_walls; ivec2 focus, standoff; uint32_t breach_t, breach_chk; unsigned breaches, breach_pushes, breach_homes;   // the breach (AR_BREACH, stage 3)
    unsigned routed, round_ways;                             // pushes routed; of them with a way round (corners)
    struct { ivec2 at; uint32_t t; } grind[8]; unsigned grinds;   // the meat grinders: where the force traded badly, when (AR_GRIND_MS)
    struct { ivec2 at; int r; uint32_t t; } fire[16]; unsigned fires;   // the fire marks (AR_FIRE): where shots at the force came from, from how far, when
    ivec2 strong; uint32_t strong_t;                         // the target the last attack time found too strong, when (airstrike.c bombs it)
    struct { ivec2 at; float lost, dealt, hp; uint32_t t; } price[AR_PRICES]; unsigned priced, price_waits;   // THE PRICE: a push's target, its trade, the force's hit points out, when; recorded, attack times waited for it
    int scout_id, backs_breach; uint32_t scout_t; unsigned scouts, stand_backs;   // the breach's look: the member sent (its serial), when; the stand-off moved back this breach
} g_armies[16];
internal struct st_side g_ar_own;
internal int g_ar_debug = 0, g_ar_foe_mid_n; internal float g_ar_foe_mid_hp;   // DEBUG
internal uint32_t g_ar_claim_t[16];   // TAKE MINES: the force holds the razed mine since (towns.c tn_claim; not saved: a load only ends the hold)
internal ivec2 g_ar_site[16]; internal int g_ar_site_set[16];   // TAKE MINES: the mine being taken (not saved either)
internal uint32_t g_ar_ask_t[16];   // THE SITES' MEMORY: when the force at home last looked for a site asked of it
internal int ar_det_out(unsigned idx);   // MINE RAIDS: a detachment is out (it has the citizens)
#define AR_CREW_MAIN(idx) (TN_TAKE(idx) || (TN_RAID(idx) && !ar_det_out(idx)))   // the force may take and keep the citizens
// OVER THE SEA ([ai_base] mod_armies_sea, on since 2026-11-11 (the player's call, measured level or a little better, docs/game-ai-routines.md); [autotest] v_mod_armies_sea; the player 2026-11-10:
// the game's commander is to go on every map, and no map types): the force on island maps too. It fights what it can
// reach over land; at an attack time with no enemy building known on its own land the nearest known one across the
// water is the landing's (landing.c landing_ask), whose soldiers come from the force at home (armies_lend: out of the
// force while the landing has them) and go back to it once the landing lets them go on the force's land. Before: on
// island maps the commander kept every land soldier and the landing took its groups, and on a map with enemies on
// both (mixed) the landing found the commander's groups empty and never went.
internal int g_ar_sea = 0, g_v_ar_sea = -1;
#define AR_SEA(idx) PV(idx, g_v_ar_sea, g_ar_sea)
internal int ar_on(ee_player *pl, unsigned idx) { return pl && !is_vanilla(pl) && MOD_ARMIES(idx) && (navy_land(idx) || (AR_SEA(idx) && navy_islands(idx))); }
internal struct { unsigned lent, back, asked; uint32_t look_t, ask_t; } g_ar_lent[16];   // OVER THE SEA: the landing's counts; the soldiers it has are an army of their own (army_view.c AV_LANDING)
internal int ar_ptr_cmp(const void *x, const void *y) { ee_unit *a = *(ee_unit *const *)x, *b = *(ee_unit *const *)y; return a < b ? -1 : a > b; }
internal int ou_members(ee_player *pl, ee_unit **out, int max);   // army.c: the units in pl's attack subgroups
internal int ar_held(struct armies_st *a, ee_unit *u) {
    for (int k = 0; k < a->n; k++) if (a->held[k].u == u) return a->held[k].id == unit_get_serial(u);
    return 0;
}
// attack.c hook_unit_to_group: the commander's offer of a freed unit to its groups - refused for a land soldier of a
// mod-army player (it stays free; the next tick holds it)
internal int armies_refuses_offer(ee_attack_cmd *cmd, ee_unit *u) {
    (void)cmd;
    ee_player *pl = u ? unit_get_owner(u) : NULL;
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    if (af_takes(u)) return 1;   // airstrike.c: the air force's bomber
    if (td && OWN_SOLDIERS(idx) && ar_on(pl, idx) && (is_siege(unit_type_get_id(td)) || is_ram(unit_type_get_id(td))) && !landing_has_unit(pl, u)) {
        g_armies[idx].refused++;   // own_soldiers: siege and rams stay out of its groups (armies_tick holds them)
        return 1;
    }
    if (!td || !ar_on(pl, idx) || !ms_land_soldier(u, td)) return 0;
    g_armies[idx].refused++;
    return 1;
}
__attribute__((format(printf, 3, 4))) internal void ar_log(unsigned idx, uint32_t now, const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# armies: P%u at %.1f min: ", idx, now / 60000.0);
    vfprintf(g_log, fmt, ap);
    fputc('\n', g_log);
    LeaveCriticalSection(&g_lock);
    va_end(ap);
}
// THE ASSUMED ENEMY (AR_ASSUME, the player: "I don't need to see the enemy after an hour of playing to know he
// probably built up similar strength to me"): the enemy at the target as the push judges it is at least the own
// army's strength - the own land soldiers' hit points, grown with the game's minutes to its full at AR_ASSUME_MIN -
// plus what the enemy gained on us in the last AR_WIN minutes (the units we lost less those we killed, the game's own
// counts for the player, in the own soldiers' average hit points), AR_ASSUME % of that: the seen and remembered
// enemy (stage_foe) stays the floor. Without it a push judged an unscouted base at 1000000 (enemy 0) and walked
// into the whole enemy army.
internal int ar_stat(ee_player *pl, int k) {   // the player's statistics series k, its latest value (attack.c "# battle")
    ee_player_stats *st = player_stats_at(pl);
    if (!st) return 0;
    uint8_t *b = player_stats_get_begin(st, k), *e = player_stats_get_end(st, k);
    return b && e > b && (e - b) % 8 == 0 && e - b < (1 << 20) && mem_readable(e - 8, 8) ? FIELD(e, -8, int) : 0;
}
internal void ar_window(ee_player *pl, struct armies_st *a, uint32_t now) {   // a sample a minute: series 3 units killed, 4 units lost
    if (a->win_t && now >= a->win_t && now - a->win_t < 60000) return;
    a->win_t = now ? now : 1;
    if (a->win_n == AR_WIN + 1) { memmove(a->kills, a->kills + 1, AR_WIN * sizeof *a->kills); memmove(a->losses, a->losses + 1, AR_WIN * sizeof *a->losses); a->win_n--; }
    a->kills[a->win_n] = ar_stat(pl, 3); a->losses[a->win_n] = ar_stat(pl, 4); a->win_n++;
}
// the force (all of it, g_ar_own) against the assumed enemy: the ratio, 99 when nothing is assumed
internal float ar_assumed(unsigned idx, struct armies_st *a, uint32_t now, float *foe_hp) {
    int pct = PV(idx, g_v_ar_assume, AR_ASSUME);
    *foe_hp = 0.0f;
    if (!pct || !g_ar_own.n) return 99.0f;
    float avg = g_ar_own.hp / (float)g_ar_own.n;
    int net = a->win_n > 1 ? (a->losses[a->win_n - 1] - a->losses[0]) - (a->kills[a->win_n - 1] - a->kills[0]) : 0;
    float ramp = now < (uint32_t)AR_ASSUME_MIN * 60000u ? (float)now / (float)(AR_ASSUME_MIN * 60000) : 1.0f;
    float foe = (g_ar_own.hp + (float)net * avg) * ramp * (float)pct / 100.0f;
    if (foe <= 0.0f) return 99.0f;
    *foe_hp = foe;
    return g_ar_own.hp / foe;
}
// the force's home: the front station when it is safe, else the staging point, else the base (0: none yet). The
// station moves with the fronts' weights (every 10 s), the staging point with the marches: home follows only a
// place that stays AR_HOME_MOVE away for AR_HOME_STAY (the smoke game: home jumped 20-60 tiles every few minutes and
// the whole force walked after it, 1900 orders in 45 min), at once when home is in danger.
// THE FORTIFIED LINE (AR_FORT, the player: "retreat into fortified positions"): home is not the bare station but the
// own built defence within AR_FORT_R of it that the most own defences cover (the nearest on ties), when there is one
// and it isn't in danger - the force stands under its towers and walls and the enemy that follows it home meets them.
internal int defence_stats(ee_unit *u, float *range, float *attack);   // attack.c
internal ivec2 ar_fort(ee_player *pl, unsigned idx, ivec2 at) {
    static ee_unit *def[256]; static float rg[256];
    int nd = 0;
    for (struct grid_it it = grid_it_box(at.x, at.y, AR_FORT_R, 1); grid_it_next(&it) && nd < 256; ) {
        ee_unit *u = it.u;
        int fam = unit_type_get_class_id(it.td);
        float r, atk;
        if (unit_get_owner(u) != pl || (fam != CLS_BUILDING && fam != CLS_FORTRESS) || !cv_alive(u) || !cv_built(u)) continue;
        if (!defence_stats(u, &r, &atk) || atk <= 0 || ivec2_dist2(unit_get_tile(u), at) > AR_FORT_R * AR_FORT_R) continue;
        def[nd] = u; rg[nd] = r; nd++;
    }
    int best = -1, bc = 0;
    long bd = 0;
    for (int k = 0; k < nd; k++) {
        ivec2 p = unit_get_tile(def[k]);
        if (stage_danger(idx, p)) continue;
        int c = 0;
        for (int j = 0; j < nd; j++) c += (float)ivec2_dist2(unit_get_tile(def[j]), p) <= (rg[j] + 1.0f) * (rg[j] + 1.0f);
        long d = ivec2_dist2(p, at);
        if (best < 0 || c > bc || (c == bc && d < bd)) { best = k; bc = c; bd = d; }
    }
    return best >= 0 ? unit_get_tile(def[best]) : at;
}
// FRONT STATION ([ai_base] force_at_front; [autotest] v_force_at_front): the force's home is in the land front with
// the heaviest open border (territories.c tv_front_station) - the player 2026-11-14: "my focus is on not letting
// anething go so far in"; where there is none (an island) as before. Until then the heaviest front of fronts.c's
// cells, a tower's place - the same idea on the older map.
internal int tv_front_station(ee_player *pl, unsigned idx, ivec2 *out);   // territories.c
internal int g_ar_front_ter = 0, g_v_ar_front_ter = -1;
#define AR_FRONT_TER(idx) PV(idx, g_v_ar_front_ter, g_ar_front_ter)
internal unsigned g_ar_front_n[16];   // the log's: looks its home came from a territory's front
internal int ar_home(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    ivec2 at;
    struct stage *st = &g_stg[idx];
    if (AR_FRONT_TER(idx) && tv_front_station(pl, idx, &at) && !stage_danger(idx, at)) g_ar_front_n[idx]++;
    else if (!(fronts_station(idx, 0, &at) && !stage_danger(idx, at)) && !(st->on && !stage_danger(idx, (at = st->s))) && !ms_base(pl, &at)) return 0;
    if (PV(idx, g_v_ar_fort, AR_FORT)) { ivec2 f = ar_fort(pl, idx, at); a->forted += f.x != at.x || f.y != at.y; at = f; }
    if (!a->home_t) { a->home = at; a->home_t = now ? now : 1; return 1; }
    if (ivec2_dist2(at, a->home) <= AR_HOME_MOVE * AR_HOME_MOVE) { a->cand_t = 0; return 1; }
    if (stage_danger(idx, a->home)) { a->home = at; a->cand_t = 0; return 1; }
    if (!a->cand_t || ivec2_dist2(at, a->cand) > AR_HOME_MOVE * AR_HOME_MOVE) { a->cand = at; a->cand_t = now ? now : 1; return 1; }
    if (now - a->cand_t >= AR_HOME_STAY) { a->home = at; a->cand_t = 0; }
    return 1;
}
// THE WAY ROUND (AR_ROUTE; the player, 2026-11-04: "like with towers, going around when possible is always preferable to
// trying breaking through - it's a heavily defended position, it will always be costly even with a well executed
// attack"). The commander's subgroups went round the towers the AI knows (avoid_towers, attack_weak_spots: the
// commander's marches); the force replaced them with one attack-move straight at the nearest known building, through
// whatever stood between. Now a push weighs the AR_ROUTE_TARGETS known enemy buildings nearest home by the way to each:
// a search over AR_CELL x AR_CELL cells of the force's land (map_area) from the force's middle, a cell's cost its length
// plus AR_ROUTE_DANGER for each threat on it (DANGER_THREATS, game/danger.c: the reach of the enemy defences seen, of
// the enemy soldiers seen - artillery at theirs -, where its citizens were hurt), the last AR_ROUTE_FREE tiles to the
// building free (the fight there is the push's own: counted, the choice left the player's weakly held spot that the
// push razed 45 buildings at, "choke wolf", for better held ones - nothing razed). The target: the least danger on the
// way, then the shortest. A wall on the way (an enemy wall or gate seen, AR_ROUTE_WALL) is counted to the end. The
// force walks the way as attack-moves to its corners (a corner: the furthest point of the way still in a straight line
// no more dangerous than the way itself) - the next once AR_APPROACH_PCT of it is there or AR_LEG_MAX has passed -,
// the attack-move at the building from the last. "# armies:" tells the way: its corners, the danger on it and on the
// straight line.
#ifndef AR_ROUTE
#define AR_ROUTE 1
#endif
#define AR_CELL 2
#define AR_ROUTE_DANGER 40         // a threat on a cell costs as much as this many cells' length
#define AR_ROUTE_TARGETS 12
#define AR_ROUTE_FREE 16          // tiles before the building free of danger
#define AR_ROUTE_WALL 30           // an enemy wall or gate on a cell costs as much as this many cells' length (a gap is free)
#define AR_WAYS 16                 // corners at most
#define AR_LEG_MAX 90000
// THE BREACH (AR_BREACH; the player, 2026-11-04: "if I have to go through I bombard it ... a wall with archers is often
// used, there you'd need a way to break the wall and units that can counter the archers ... don't overfocus on the way
// I defend these"): when even the way with the least danger carries AR_BREACH_D threats, the force walks it only to the
// stand-off - AR_STANDOFF cells before the danger begins - and holds there (stage 3), judging its own spot only; the
// siege weapons gather at the stand-off and shell what holds the position, the nearest first (attack.c siege_tick:
// walls and gates, defences and shooters they outrange within AR_BREACH_R of where the danger begins; siege_want 4).
// Every AR_BREACH_EVERY the way on is searched again: its danger down to AR_BREACH_OPEN, the push goes (the rams
// along at the position's walls and defences); after AR_BREACH_MAX it goes if it beats the enemy at the target by
// AR_GO_PCT, else home. OFF (2026-11-04): on the player's save "choke wolf" (a push forced every 4 minutes, 153 -> 180
// min, against the same without) razed 16 against 20, lost 1308 against 1350 units - the danger it stood off from began
// at its own home (where it had traded badly), the stand-off was home and nothing there to shell for 4 minutes; and
// before that, a stand-off 8 tiles from the position was under the fire of artillery it never saw (lost 9145 hit
// points, dealt 106). What holds a position has to be seen before it can be shelled: a look first (open).
// breach_wall.eeai (a closed wall, archers and towers behind it) tests it; the force targets what it sees there.
#ifndef AR_BREACH
#define AR_BREACH 0
#endif
#define AR_BREACH_D 10
#define AR_STANDOFF 4
#define AR_BREACH_OPEN 3
#define AR_BREACH_EVERY 10000
#define AR_BREACH_MAX 240000
// THE MEAT GRINDER: where the force traded badly (its middle as the trade sent it home) counts as AR_GRIND_LEVEL threats
// within AR_GRIND_R for AR_GRIND_MS on the way search - the danger map knows only the shooters seen, and the player's
// artillery fires from beyond sight (his save "choke wolf": a push straight at 73,110 lost 7682 hit points and dealt
// 180 at 64,88)
#define AR_GRIND_R 12
#define AR_GRIND_LEVEL 3
#define AR_GRIND_MS 1200000
// THE FIRE MARKS (AR_FIRE): where the shots that hit the force came from - each member hit since the last judgement,
// its attacker (unit +0x1DC, the game's: dropped once that attack ends) AR_FIRE_MIN tiles or more off it: its tile and
// that distance, kept for AR_GRIND_MS (a mark within AR_FIRE_JOIN of one taken into it). A player reads as much off
// the shells coming in; the danger map knows only the shooters seen, and artillery fires from beyond sight ("choke
// wolf": a stand-off 8 tiles from the position lost 9145 hit points to guns it never saw, and dealt 106). The way
// search counts AR_GRIND_LEVEL threats within a mark's distance + AR_FIRE_PAD: the way round and the stand-off keep
// out of the reach the force has felt. No meat grinder is kept within AR_GRIND_R of home then (a bad trade there is a
// defence of home, not the enemy's position: the danger of "choke wolf"'s breach began at home, the stand-off with it).
// At the breach (stage 3): a mark since it began over the stand-off - the way searched again from home, the stand-off
// back (a plain move; AR_BREACH_BACKS times, then home); a mark with nothing seen near it, or the position with nothing
// seen near its focus - the fastest member walks to its sight short of it (a plain move: it looks, the siege and the
// counter-fire shoot what it sees; back to the stand-off after AR_SCOUT_MS), one at a time. OFF (2026-11-04): the
// player's save "choke wolf" (P3 pushed every 4 minutes from 30, 45 and 90 s, 153 -> 180 min; means of the three)
// without - with - with the breach too: its buildings lost 12.0 / 6.7 / 5.7, razed 32.3 / 21.0 / 16.3 (the 32 one
// run's 57), killed 318 / 304 / 277, lost 1415 / 1360 / 1360 - fewer losses, less razed; the breach once a run (2
// and 8 looks, one stand-off back). Head-to-head (late.eeai, 75 min, 24 mirrored games, 4 AIs) with - without: kills
// -10.4 (se 20.2), losses +6.4 (11.0), razed -2.3 (7.1), soldiers -0.3 (6.1) - noise.
#ifndef AR_FIRE
#define AR_FIRE 0
#endif
#define AR_FIRES 16
#define AR_FIRE_MIN 5
#define AR_FIRE_JOIN 4
#define AR_FIRE_PAD 2
#define AR_BREACH_BACKS 3
#define AR_SCOUT_MS 20000
internal int g_v_ar_route = -1, g_v_ar_breach = -1, g_v_ar_fire = -1, g_v_ar_price = -1;
// THE PRICE of a target at d (AR_PRICE): the force (g_ar_own, as judged just before) against the trade it got near d
// last, 99 when none
internal float ar_price(unsigned idx, struct armies_st *a, ivec2 d, uint32_t now) {
    float best = 99.0f;
    if (!PV(idx, g_v_ar_price, AR_PRICE)) return best;
    for (int k = 0; k < AR_PRICES; k++) {
        if (!a->price[k].t || now - a->price[k].t >= AR_PRICE_MS || ivec2_dist2(a->price[k].at, d) > AR_PRICE_R * AR_PRICE_R) continue;
        float q = (a->price[k].dealt + 1.0f) / (a->price[k].lost + 1.0f) * g_ar_own.hp / (a->price[k].hp > 1.0f ? a->price[k].hp : 1.0f);
        if (q < best) best = q;
    }
    return best;
}
// a wing's targets down near d (airstrike.c): the price there is paid
internal void ar_price_clear(struct armies_st *a, ivec2 d) {
    for (int k = 0; k < AR_PRICES; k++) if (a->price[k].t && ivec2_dist2(a->price[k].at, d) <= AR_PRICE_R * AR_PRICE_R) a->price[k].t = 0;
}
internal unsigned g_ar_push_now;   // test (attacknow): the players whose force pushes at its next tick at home
internal struct ar_route_buf { int n; int *g, *par; uint64_t *heap; uint8_t *seen, *dang, *wall; unsigned short *land; int hn; } g_arr;   // seen: 1 open, 2 done; wall: an enemy wall or gate seen on the cell
internal int ar_route_alloc(int n) {
    if (g_arr.n == n) return 1;
    free(g_arr.g); free(g_arr.par); free(g_arr.heap); free(g_arr.seen); free(g_arr.dang); free(g_arr.wall); free(g_arr.land);
    memset(&g_arr, 0, sizeof g_arr);
    size_t c = (size_t)n * (size_t)n;
    g_arr.g = malloc(c * sizeof(int)); g_arr.par = malloc(c * sizeof(int)); g_arr.heap = malloc(c * 8 * sizeof(uint64_t));   // a cell is pushed again at most once per neighbour
    g_arr.seen = calloc(c, 1); g_arr.dang = malloc(c); g_arr.wall = malloc(c); g_arr.land = malloc(c * sizeof(unsigned short));
    if (!g_arr.g || !g_arr.par || !g_arr.heap || !g_arr.seen || !g_arr.dang || !g_arr.wall || !g_arr.land) { g_arr.n = 0; return 0; }
    g_arr.n = n;
    return 1;
}
// the open cells' heap: f in the high 32 bits of the key, the cell in the low
internal void ar_hpush(uint64_t k) {
    uint64_t *h = g_arr.heap;
    int i = g_arr.hn++;
    while (i > 0 && h[(i - 1) / 2] > k) { h[i] = h[(i - 1) / 2]; i = (i - 1) / 2; }
    h[i] = k;
}
internal uint64_t ar_hpop(void) {
    uint64_t *h = g_arr.heap, top = h[0], last = h[--g_arr.hn];
    int i = 0;
    for (;;) {
        int c = 2 * i + 1;
        if (c >= g_arr.hn) break;
        if (c + 1 < g_arr.hn && h[c + 1] < h[c]) c++;
        if (h[c] >= last) break;
        h[i] = h[c]; i = c;
    }
    if (g_arr.hn) h[i] = last;
    return top;
}
// the cells' land and danger for pl's map now (danger: the most threats on a tile of the cell)
internal int ar_route_cells(unsigned idx, int size, unsigned short land, const struct armies_st *a, uint32_t now) {
    int n = (size + AR_CELL - 1) / AR_CELL, fire = PV(idx, g_v_ar_fire, AR_FIRE);
    if (!ar_route_alloc(n)) return 0;
    const uint8_t *dm = danger_map(idx, DANGER_THREATS, size);
    for (int cy = 0; cy < n; cy++)
        for (int cx = 0; cx < n; cx++) {
            int c = cy * n + cx, dmax = 0;
            ivec2 m = iv2(cx * AR_CELL + AR_CELL / 2, cy * AR_CELL + AR_CELL / 2);
            if (m.x >= size) m.x = size - 1;
            if (m.y >= size) m.y = size - 1;
            g_arr.land[c] = map_area(m, 2) == land;
            for (int y = cy * AR_CELL; dm && y < cy * AR_CELL + AR_CELL && y < size; y++)
                for (int x = cx * AR_CELL; x < cx * AR_CELL + AR_CELL && x < size; x++) if (dm[y * size + x] > dmax) dmax = dm[y * size + x];
            for (int k = 0; k < 8; k++)
                if (a->grind[k].t && now - a->grind[k].t < AR_GRIND_MS && ivec2_dist2(m, a->grind[k].at) <= AR_GRIND_R * AR_GRIND_R) dmax += AR_GRIND_LEVEL;
            for (int k = 0; fire && k < AR_FIRES; k++) {
                int r = a->fire[k].r + AR_FIRE_PAD;
                if (a->fire[k].t && now - a->fire[k].t < AR_GRIND_MS && ivec2_dist2(m, a->fire[k].at) <= r * r) dmax += AR_GRIND_LEVEL;
            }
            g_arr.dang[c] = (uint8_t)(dmax > 255 ? 255 : dmax);
        }
    memset(g_arr.wall, 0, (size_t)n * n);
    ee_player *pl = g_players[idx];
    for (struct grid_it it = grid_it_all(1); pl && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !(is_wall(unit_type_get_id(it.td)) || unit_type_get_class_id(it.td) == CLS_GATE) || unit_get_hp(u) <= 0 || !seen_by(u, pl)) continue;
        ivec2 t = unit_get_tile(u);
        if (t.x >= 0 && t.y >= 0 && t.x < size && t.y < size) g_arr.wall[(t.y / AR_CELL) * n + t.x / AR_CELL] = 1;
    }
    return n;
}
// the way from a to b over the cells (ar_route_cells first): its cells from b back to a into path, their count (0 none);
// *danger the threats summed over its cells but the last AR_ROUTE_FREE tiles', *walls the wall cells on it
internal int ar_route_find(int n, ivec2 a, ivec2 b, int *path, int max, int *danger, int *walls) {
    int s = (a.y / AR_CELL) * n + a.x / AR_CELL, t = (b.y / AR_CELL) * n + b.x / AR_CELL;
    if (s < 0 || t < 0 || s >= n * n || t >= n * n) return 0;
    memset(g_arr.seen, 0, (size_t)n * n);
    g_arr.hn = 0;
    int tx = t % n, ty = t / n, free_r = AR_ROUTE_FREE / AR_CELL;
    g_arr.g[s] = 0; g_arr.par[s] = -1; g_arr.seen[s] = 1;
    ar_hpush((uint64_t)0 << 32 | (uint32_t)s);
    static const int ddx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, ddy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    int found = 0;
    while (g_arr.hn) {
        int c = (int)(uint32_t)ar_hpop();
        if (g_arr.seen[c] == 2) continue;
        g_arr.seen[c] = 2;
        if (c == t) { found = 1; break; }
        int cx = c % n, cy = c / n;
        for (int d = 0; d < 8; d++) {
            int nx = cx + ddx[d], ny = cy + ddy[d];
            if (nx < 0 || ny < 0 || nx >= n || ny >= n) continue;
            int j = ny * n + nx;
            if ((!g_arr.land[j] && j != t) || g_arr.seen[j] == 2) continue;
            if (d >= 4 && (!g_arr.land[cy * n + nx] || !g_arr.land[ny * n + cx])) continue;
            int near_t = abs(nx - tx) <= free_r && abs(ny - ty) <= free_r;
            int cost = (d >= 4 ? 14 : 10) + (near_t ? 0 : g_arr.dang[j] * AR_ROUTE_DANGER * 10) + g_arr.wall[j] * AR_ROUTE_WALL * 10;
            int ng = g_arr.g[c] + cost;
            if (g_arr.seen[j] == 1 && ng >= g_arr.g[j]) continue;
            g_arr.g[j] = ng; g_arr.par[j] = c; g_arr.seen[j] = 1;
            int hx = abs(nx - tx), hy = abs(ny - ty), h = 10 * (hx > hy ? hx : hy) + 4 * (hx < hy ? hx : hy);
            ar_hpush((uint64_t)(uint32_t)(ng + h) << 32 | (uint32_t)j);
        }
    }
    if (!found) return 0;
    int k = 0, dsum = 0;
    *walls = 0;
    for (int c = t; c >= 0 && k < max; c = g_arr.par[c]) {
        path[k++] = c;
        *walls += g_arr.wall[c];
        int cx = c % n, cy = c / n;
        if (!(abs(cx - tx) <= free_r && abs(cy - ty) <= free_r)) dsum += g_arr.dang[c];
    }
    *danger = dsum;
    return k;
}
// the threats on the straight line from cell a to cell b and whether it stays on land (the corners' test)
internal int ar_line(int n, int a, int b, int *dsum) {
    int ax = a % n, ay = a / n, bx = b % n, by = b / n, steps = abs(bx - ax) > abs(by - ay) ? abs(bx - ax) : abs(by - ay);
    *dsum = 0;
    for (int i = 1; i <= steps; i++) {
        int x = ax + (int)lroundf((float)(bx - ax) * (float)i / (float)steps), y = ay + (int)lroundf((float)(by - ay) * (float)i / (float)steps);
        int c = y * n + x;
        if (!g_arr.land[c]) return 0;
        *dsum += g_arr.dang[c];
    }
    return 1;
}
// the push's target and way (AR_ROUTE): 1 with a target; its corners into a->way (a->nway, 0: straight there)
internal int ar_route(ee_player *pl, unsigned idx, struct armies_st *a, ivec2 from, ee_unit **target, int *danger, int *straight, uint32_t now) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    a->nway = 0; a->breach = 0;
    if (size <= 0) return 0;
    int n = ar_route_cells(idx, size, map_area(from, 2), a, now);
    if (!n) return 0;
    // the known enemy buildings nearest home
    ee_unit *cand[AR_ROUTE_TARGETS];
    long cd[AR_ROUTE_TARGETS];
    int nc = 0;
    for (int pass = TN_RAID(idx) ? -1 : TN_TAKE(idx) ? 0 : 1; pass < 2 && !nc; pass += pass < 0 && !TN_TAKE(idx) ? 2 : 1)   // the mines asked of it (THE SITES' MEMORY), TAKE MINES: the drop sites by mines; the others when there is none
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !fr_building(it.td) || is_wall(unit_type_get_id(it.td)) || !seen_by(u, pl)) continue;
        if (pass < 0 && !site_wants_army(idx, unit_get_tile(u))) continue;
        if (pass <= 0 && !tn_mine_drop(pl, u, it.td)) continue;
        long d = ivec2_dist2(unit_get_tile(u), a->home);
        if (nc == AR_ROUTE_TARGETS && cd[nc - 1] <= d) continue;   // the list full of nearer ones
        int k = nc < AR_ROUTE_TARGETS ? nc++ : AR_ROUTE_TARGETS - 1;
        while (k > 0 && cd[k - 1] > d) { cand[k] = cand[k - 1]; cd[k] = cd[k - 1]; k--; }
        cand[k] = u; cd[k] = d;
    }
    static int path[65536], best_path[65536];
    int best = -1, best_len = 0, best_d = 0, best_cost = 0, best_w = 0;
    for (int i = 0; i < nc; i++) {
        int d = 0, w = 0, len = ar_route_find(n, from, unit_get_tile(cand[i]), path, 65536, &d, &w);
        if (!len) continue;
        int cost = (d + w * AR_ROUTE_WALL / 10) * 1000 + len;   // the least danger (a wall as much as 3 threats), then the shortest
        if (ar_price(idx, a, unit_get_tile(cand[i]), now) * 100.0f < (float)AR_GO_PCT) cost += 100000000;   // THE PRICE: another first
        if (best >= 0 && cost >= best_cost) continue;
        best = i; best_cost = cost; best_d = d; best_len = len; best_w = w;
        memcpy(best_path, path, (size_t)len * sizeof *path);
    }
    if (best < 0) return 0;
    *target = cand[best]; *danger = best_d; a->way_walls = best_w;
    int s = (from.y / AR_CELL) * n + from.x / AR_CELL;
    ar_line(n, s, best_path[0], straight);
    // the breach: the first cell with danger from the start (the last AR_ROUTE_FREE tiles' free) - the stand-off
    // AR_STANDOFF cells before it, the corners only up to there
    int stop = 0;
    if ((best_d >= AR_BREACH_D || best_w) && PV(idx, g_v_ar_breach, AR_BREACH)) {   // a wall in the way is a breach by itself
        int tx = best_path[0] % n, ty = best_path[0] / n, free_r = AR_ROUTE_FREE / AR_CELL, first = -1;
        for (int j = best_len - 1; j >= 0 && first < 0; j--) {
            int c = best_path[j];
            if (g_arr.wall[c] || (best_d >= AR_BREACH_D && g_arr.dang[c] && !(abs(c % n - tx) <= free_r && abs(c / n - ty) <= free_r))) first = j;
        }
        if (first >= 0) {
            stop = first + AR_STANDOFF < best_len - 1 ? first + AR_STANDOFF : best_len - 1;
            int fc = best_path[first], sc = best_path[stop];
            a->breach = 1;
            a->focus = iv2((fc % n) * AR_CELL + AR_CELL / 2, (fc / n) * AR_CELL + AR_CELL / 2);
            a->standoff = iv2((sc % n) * AR_CELL + AR_CELL / 2, (sc / n) * AR_CELL + AR_CELL / 2);
        }
    }
    // the corners: from the start the furthest cell of the way still in a land line no more dangerous than the way there
    int at = best_len - 1, way_d = 0;
    while (at > stop && a->nway < AR_WAYS) {
        int next = at - 1, dn = 0, wd = way_d;
        for (int j = at - 1; j >= 0; j--) {
            wd += g_arr.dang[best_path[j]];
            int ld;
            if (ar_line(n, best_path[at], best_path[j], &ld) && ld <= wd - way_d) { next = j; dn = wd; }
        }
        if (next <= stop) break;   // straight on to the building (or the stand-off)
        way_d = dn ? dn : way_d + g_arr.dang[best_path[next]];
        int c = best_path[next];
        a->way[a->nway++] = iv2((c % n) * AR_CELL + AR_CELL / 2, (c / n) * AR_CELL + AR_CELL / 2);
        at = next;
    }
    if (a->breach) {   // the stand-off ends the way
        if (a->nway == AR_WAYS) a->nway--;
        a->way[a->nway++] = a->standoff;
    }
    return 1;
}
// the force (out: the members out with it, else all) as a side, its middle and count
internal unsigned tv_terr_of(ivec2 p);            // territories.c: the territory of a tile, TER_NONE none
internal int tv_holds(unsigned idx, ivec2 p);     // ... AI idx holds the land territory of that tile
// HELD GROUND ([ai_base] raid_held_ground; [autotest] v_raid_held_ground): a raid is answered wherever it stands in a
// territory the AI holds on the force's land, not only within AR_RAID_REACH of where the force stands (the player
// 2026-11-14: "the backland is nerly undefended in my case, my focus is on not letting anething go so far in" - no
// guard out there, so the force has to come: towns 180-290 tiles out were raided with the force at its station).
// The same judgement and the same parties as a raid near home (ar_at_home); a party looks as far.
internal int g_ar_held_ground = 0, g_v_ar_held_ground = -1;
#define AR_HELD_GROUND(idx) PV(idx, g_v_ar_held_ground, g_ar_held_ground)
internal unsigned g_ar_far[16];   // the log's: raids answered beyond AR_RAID_REACH for it
internal int tv_foes(unsigned idx, unsigned terr);   // ... the enemy soldiers the AI counts in or by it, -1 not known
internal int ar_side(struct armies_st *a, int out, ivec2 *mid) {
    long sx = 0, sy = 0;
    int n = 0;
    st_side_clear(&g_ar_own);
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if ((out && !a->held[k].out) || !cv_alive(u)) continue;
        ivec2 p = unit_get_tile(u);
        sx += p.x; sy += p.y; n++;
        st_side_add(&g_ar_own, u);
    }
    if (mid) *mid = n ? iv2((int)(sx / n), (int)(sy / n)) : a->home;
    return n;
}
// the force against the enemy at d as pl knows it (stage_foe)
internal float ar_judge(ee_player *pl, struct armies_st *a, int out, ivec2 d) {
    stage_foe(pl, d);
    ar_side(a, out, NULL);
    return st_ratio(&g_ar_own, &g_sgs_foe);
}
// the enemy building nearest from that pl knows of (seen or remembered; no walls: seen_by's intel_remembers covers
// buildings and fortresses, walls are neither)
internal ee_unit *ar_target_on(ee_player *pl, ivec2 from, unsigned land) {   // land: only on that land (map_area), 0 any
    ee_unit *best = NULL;
    long bd = 0;
    for (int pass = TN_TAKE((unsigned)player_get_index(pl) & 15) ? 0 : 1; pass < 2 && !best; pass++)   // TAKE MINES: as ar_route
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !fr_building(it.td) || is_wall(unit_type_get_id(it.td)) || !seen_by(u, pl)) continue;
        if (land && map_area(unit_get_tile(u), 2) != land) continue;
        if (!pass && !tn_mine_drop(pl, u, it.td)) continue;
        long d = ivec2_dist2(unit_get_tile(u), from);
        if (!best || d < bd) { best = u; bd = d; }
    }
    return best;
}
internal ee_unit *ar_target(ee_player *pl, ivec2 from) { return ar_target_on(pl, from, 0); }
// TAKE MINES: the known enemy building nearest p within r of it - what still stands in the way of a town there
internal ee_unit *ar_near_building(ee_player *pl, ivec2 p, int r) {
    ee_unit *best = NULL;
    int bd = r * r + 1;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !fr_building(it.td) || is_wall(unit_type_get_id(it.td)) || !seen_by(u, pl)) continue;
        int d = ivec2_dist2(unit_get_tile(u), p);
        if (d < bd) { bd = d; best = u; }
    }
    return best;
}
internal ee_unit *fl_target_of(ee_unit *u);   // fleet.c: its attack order's target
internal int ar_enemy_near(ee_player *pl, ivec2 d, int r, int buildings) {   // enemy land fighters (or buildings) pl sees within r of d
    for (struct grid_it it = grid_it_box(d.x, d.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || ivec2_dist2(unit_get_tile(u), d) > r * r) continue;
        if ((buildings ? fr_building(it.td) : ms_land_soldier(u, it.td)) && seen_by(u, pl)) return 1;
    }
    return 0;
}
internal int ar_enemy_seen(ee_player *pl, ivec2 d, int r) {   // anything of an enemy pl sees within r of d
    for (struct grid_it it = grid_it_box(d.x, d.y, r, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (o && player_get_index(o) > 0 && is_enemy(pl, o) && cv_alive(it.u) && ivec2_dist2(unit_get_tile(it.u), d) <= r * r && seen_by(it.u, pl)) return 1;
    }
    return 0;
}
internal int ar_enemy_band(ee_player *pl, ivec2 d, int r) {   // enemy land fighters pl sees within r of d
    int n = 0;
    for (struct grid_it it = grid_it_box(d.x, d.y, r, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(it.u) || !ms_land_soldier(it.u, it.td) || !seen_by(it.u, pl)) continue;
        n += ivec2_dist2(unit_get_tile(it.u), d) <= r * r;
    }
    return n;
}
internal void ar_order(struct armies_st *a, int k, ee_goal *g, uint32_t now) {
    if (!unit_goal(a->held[k].u, g, 0, 0x1F40)) return;
    a->held[k].order_t = now ? now : 1;
    a->orders++;
}
internal ee_goal *ar_goal_at(ivec2 d, int k, int fight) {   // the force's order at d, spread over AR_JIT x AR_JIT tiles
    float xy[2] = { (float)d.x + 0.5f + (float)(k % AR_JIT) - (float)(AR_JIT / 2), (float)d.y + 0.5f + (float)(k / AR_JIT % AR_JIT) - (float)(AR_JIT / 2) };
    if (!fight) return goal_move(xy[0], xy[1]);
    void *mem = goal_move_alloc();
    return mem ? goal_attack_move_init(mem, xy) : NULL;
}
// the force goes (kind 1 a push at target, 2 a raid at d): all its members, or only those out already (a next leg)
internal int ar_group_move(ee_player *pl, struct armies_st *a, int which, ivec2 d, uint32_t now);
// FORMATIONS (AR_FORM, the player: "I would generally like the AI to use these formations more where possible"): a
// push (bit 1) and a raid (bit 4) set out as the game's group move to the point AR_APPROACH short of the target
// (the mod's formations lay the force out, the ranged behind the infantry, and its march and follow keep it
// together), and attack-move from there - once AR_APPROACH_PCT of it has arrived, AR_RAID_BAND enemy fighters are
// seen within SG_NEAR of its middle (the fight is on: they fight as they stand; one scout seen dropped a push's march
// 60 tiles short) or AR_APPROACH_MAX has passed (the march blocked). The way home
// stays the convoy: a march at the slowest unit's speed was too slow under pursuit (AR_WAY bit 4, above).
internal void ar_attack_out(struct armies_st *a, int out_only, uint32_t now) {   // the attack-move at the destination for the members out
    a->stage = 0;
    for (int k = 0; k < a->n; k++) {
        if (out_only && !a->held[k].out) continue;
        a->held[k].out = 1;
        ar_order(a, k, ar_goal_at(a->dest, k, 1), now);
    }
}
// the breach's push: the rams (no members of the force: is_soldier) within 40 of the stand-off at the position's wall,
// gate or defence nearest each
internal void ar_rams(ee_player *pl, struct armies_st *a) {
    for (struct grid_it it = grid_it_box(a->standoff.x, a->standoff.y, 40, 1); grid_it_next(&it); ) {
        ee_unit *r = it.u;
        if (unit_get_owner(r) != pl || !is_ram(unit_type_get_id(it.td)) || !cv_alive(r)) continue;
        ee_unit *best = NULL;
        long bd = 0;
        ivec2 rp = unit_get_tile(r);
        for (struct grid_it jt = grid_it_box(a->focus.x, a->focus.y, AR_BREACH_R, 1); grid_it_next(&jt); ) {
            ee_unit *b = jt.u;
            ee_player *o = unit_get_owner(b);
            int fam = unit_type_get_class_id(jt.td);
            if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(b) || !seen_by(b, pl)) continue;
            if (!(is_wall(unit_type_get_id(jt.td)) || fam == CLS_GATE || fam == CLS_BUILDING || fam == CLS_FORTRESS)) continue;
            long d = ivec2_dist2(unit_get_tile(b), rp);
            if (!best || d < bd) { best = b; bd = d; }
        }
        if (best) unit_goal(r, goal_attack(r, best), 0, 0x1F40);
    }
}
internal void ar_go(ee_player *pl, unsigned idx, struct armies_st *a, int kind, ivec2 d, ee_unit *target, int out_only, uint32_t now) {
    a->out = kind; a->dest = d; a->target = target; a->target_id = target ? unit_get_serial(target) : 0; g_ar_claim_t[(unsigned)(a - g_armies)] = 0;
    if (!out_only) g_ar_site_set[(unsigned)(a - g_armies)] = 0;
    if (kind == 1 && !out_only && target && (TN_TAKE(idx) || (AR_CREW_MAIN(idx) && site_wants_army(idx, d))) && tn_mine_drop(pl, target, unit_get_type_data(target))) tn_crew_start(pl, idx, a->home, now);   // TAKE MINES: the citizens along
    if (kind != 1 || out_only) { a->nway = 0; a->breach = 0; }   // a push's way was laid by ar_route; a raid's or a next leg's goes straight
    g_ar_breach[idx].on = 0;
    a->out_t = now ? now : 1; a->judge_t = 0; a->weak = 0; a->clear = 0; a->stage = 0;
    if (!out_only) { a->trade_n = 0; a->nfoe = 0; }   // a next leg goes on with the fight's trade
    ivec2 mid;
    int n = ar_side(a, out_only, &mid), form = PV(idx, g_v_ar_form, AR_FORM);
    if (!out_only && n && (form & (kind == 1 ? 1 : 4))) {
        float dx = (float)(d.x - mid.x), dy = (float)(d.y - mid.y), dist = sqrtf(dx * dx + dy * dy);
        if (dist > (float)(AR_APPROACH + 2 * AR_DONE)) {
            a->approach = iv2(d.x - (int)(dx / dist * (float)AR_APPROACH), d.y - (int)(dy / dist * (float)AR_APPROACH));
            a->stage = 1; a->stage_t = now ? now : 1; a->approaches++;
            for (int k = 0; k < a->n; k++) a->held[k].out = cv_alive(a->held[k].u);
            ar_group_move(pl, a, 0, a->approach, now);
        }
    }
    if (!a->stage && kind == 1 && !out_only && a->nway) {   // the way round: to its first corner
        a->stage = 2; a->way_i = 0; a->stage_t = now ? now : 1;
        for (int k = 0; k < a->n; k++) if ((a->held[k].out = (uint8_t)cv_alive(a->held[k].u))) ar_order(a, k, ar_goal_at(a->way[0], k, 1), now);
    }
    if (!a->stage) ar_attack_out(a, out_only, now);
    if (!out_only) { ar_side(a, 1, NULL); a->out_hp = g_ar_own.hp; }
}
internal void ar_stopped(struct armies_st *a, float foe_hp, int won) { a->stopped = won ? 2 : 1; a->stop_hp = foe_hp; }   // a push ends: what stopped it
internal int mt_line(ee_player *pl, int t);   // metal.c: a unit type's line (MB_INF, MB_RNG, MB_CAV, MB_SIE)
// A GROUP MOVE of the force's members marked (out, on their way home, or picked) to d: the game's own move order on a
// selection of them, as a click of the player's would be (test/fleetmove.c's recipe) - so the mod's formations lay
// them out, and formation_march and formation_follow keep them together on the way (engine/groupmove.c: nothing of
// that is reachable through a unit's own goal). The AI decides from the game's state and the message is carried out
// locally: the same on every PC.
internal int ar_group_move(ee_player *pl, struct armies_st *a, int which, ivec2 d, uint32_t now) {   // which: 0 the members out, 1 those on their way home, 2 the picked
    static uint32_t ids[AR_HELD];
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    int n = 0;
    long sx = 0, sy = 0;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (!(which == 1 ? a->held[k].back : which == 2 ? a->held[k].pick : a->held[k].out) || !cv_alive(u)) continue;
        ivec2 p = unit_get_tile(u);
        sx += p.x; sy += p.y;
        ids[n++] = (uint32_t)unit_get_serial(u);
        a->held[k].order_t = now ? now : 1;
    }
    if (!n) return 0;
    uint32_t *sel[3] = { NULL, ids, ids + n };
    player_select(pl, sel);
    memset(mem, 0, sizeof mem);
    float x = (float)d.x + 0.5f, y = (float)d.y + 0.5f;
    path_msg_make((ee_path_msg *)mem, msg_as_begin(pl), x, y, 0, 0);
    path_msg_set_facing((ee_path_msg *)mem, atan2f(-(x - (float)sx / (float)n), y - (float)sy / (float)n));
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
    a->orders++; a->group_moves++;
    return n;
}
internal void ar_home_all(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {   // the force out comes home as one
    g_ar_breach[idx].on = 0; a->breach = 0; a->stage = 0;
    int fight = PV(idx, g_v_ar_back_fight, AR_BACK_FIGHT), group = PV(idx, g_v_ar_way, AR_WAY) & 4;
    a->back_t = now ? now : 1; a->back_n = 0; a->back_hp = 0.0f; a->back_kind = a->out;
    if (AR_CREW_MAIN(idx)) tn_crew_release(pl, idx);
    a->out = 0; a->target = NULL; a->backs++;
    for (int k = 0; k < a->n; k++) {
        a->held[k].back = 0;
        if (!a->held[k].out) continue;
        a->held[k].out = 0;
        int f = fight == 2 ? mt_line(pl, unit_type_get_id(unit_get_type_data(a->held[k].u))) == MB_CAV : fight;
        if (!group) ar_order(a, k, ar_goal_at(a->home, k, f), now);
        if (cv_alive(a->held[k].u)) { a->held[k].back = 1; a->back_n++; a->back_hp += unit_get_hp(a->held[k].u); }
    }
    if (group) ar_group_move(pl, a, 1, a->home, now);
}
// THE WAY HOME, measured (2026-11-02): what of the force sent home arrives - once every member sent is home (within
// AR_HOME_R), dead or two minutes have passed - against what went
// THE WAY HOME AS A GROUP (AR_WAY, the player: "on a retreat they should still stay a group ... walk home together;
// after they are out of the enemy base, attacking smaller groups that still chase them is the right move, as long as
// they can win these fights"): the convoy - every AR_WAY_EVERY every member on its way is ordered to the point
// AR_WAY_STEP tiles ahead of the group's middle towards home, so the fast wait there for the slow (the game's group
// move has no such thing: each unit walks to its slot at its own pace, docs/engine-notes.md "Group moves") - and
// the stand: once the middle is AR_WAY_CLEAR from the push's target and no enemy defence covers it, the chasers
// (the enemy land fighters seen within SG_FOE of the middle) are judged against the group as a whole, and beaten by
// AR_GO_PCT the group attack-moves at their middle as a raid (home again when they are cleared or it is losing).
internal void ar_way_home(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    if (!a->back_t || a->out) return;
    int mode = PV(idx, g_v_ar_way, AR_WAY), alive = 0, home = 0;
    long sx = 0, sy = 0;
    float hp = 0.0f;
    for (int k = 0; k < a->n; k++) {
        if (!a->held[k].back || !cv_alive(a->held[k].u)) continue;
        ivec2 p = unit_get_tile(a->held[k].u);
        alive++; hp += unit_get_hp(a->held[k].u); sx += p.x; sy += p.y;
        home += ivec2_dist2(p, a->home) <= AR_HOME_R * AR_HOME_R;
    }
    if (alive > home && now - a->back_t < 120000) {
        ivec2 mid = iv2((int)(sx / alive), (int)(sy / alive));
        if ((mode & 2) && (!a->judge_t || now - a->judge_t >= AR_JUDGE_EVERY)) {
            a->judge_t = now ? now : 1;
            if (ivec2_dist2(mid, a->dest) >= AR_WAY_CLEAR * AR_WAY_CLEAR && !cv_enemy_def(pl, mid)) {
                long cx = 0, cy = 0;
                int nc = 0;
                for (struct grid_it it = grid_it_box(mid.x, mid.y, SG_FOE, 1); grid_it_next(&it); ) {
                    ee_player *o = unit_get_owner(it.u);
                    if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(it.u) || !ms_land_soldier(it.u, it.td) || !seen_by(it.u, pl)) continue;
                    ivec2 p = unit_get_tile(it.u);
                    if (ivec2_dist2(p, mid) > SG_FOE * SG_FOE) continue;
                    cx += p.x; cy += p.y; nc++;
                }
                if (nc) {
                    stage_foe(pl, mid);
                    st_side_clear(&g_ar_own);
                    for (int k = 0; k < a->n; k++) if (a->held[k].back && cv_alive(a->held[k].u)) st_side_add(&g_ar_own, a->held[k].u);
                    float ratio = st_ratio(&g_ar_own, &g_sgs_foe);
                    if (ratio * 100.0f >= (float)AR_GO_PCT) {
                        ivec2 c = iv2((int)(cx / nc), (int)(cy / nc));
                        a->stands++;
                        ar_log(idx, now, "%d chasers at %d,%d - the group of %d on its way home turns on them (%.2f: own %d, enemy %d)", nc, c.x, c.y, alive, ratio, g_ar_own.n, g_sgs_foe.n);
                        for (int k = 0; k < a->n; k++) a->held[k].out = a->held[k].back && cv_alive(a->held[k].u);
                        a->back_t = 0;
                        ar_go(pl, idx, a, 2, c, NULL, 1, now);
                        return;
                    }
                }
            }
        }
        if ((mode & 1) && !(mode & 4) && (!a->way_t || now - a->way_t >= AR_WAY_EVERY)) {
            a->way_t = now ? now : 1;
            float dx = (float)(a->home.x - mid.x), dy = (float)(a->home.y - mid.y), d = sqrtf(dx * dx + dy * dy);
            ivec2 wp = d <= (float)AR_WAY_STEP ? a->home : iv2(mid.x + (int)(dx / d * (float)AR_WAY_STEP), mid.y + (int)(dy / d * (float)AR_WAY_STEP));
            for (int k = 0; k < a->n; k++) {
                ee_unit *u = a->held[k].u;
                if (!a->held[k].back || !cv_alive(u) || ivec2_dist2(unit_get_tile(u), a->home) <= AR_HOME_R * AR_HOME_R) continue;
                ar_order(a, k, ar_goal_at(wp, k, 0), now);
            }
        }
        return;
    }
    a->way_n += (unsigned)a->back_n; a->way_lost += (unsigned)(a->back_n - alive); a->way_hp += a->back_hp; a->way_hp_lost += a->back_hp - hp;
    ar_log(idx, now, "home from %s: %d of %d arrived (hp %.0f of %.0f) in %.1f min, %d still on the way", a->back_kind == 1 ? "a push" : "a raid", alive, a->back_n, hp, a->back_hp, (now - a->back_t) / 60000.0, alive - home);
    a->back_t = 0;
    for (int k = 0; k < a->n; k++) a->held[k].back = 0;
}
internal int ar_idle(ee_unit *u) { return unit_ai_state_id(u) == 5000 && !unit_get_goal(u); }
internal int ar_count(struct armies_st *a, int out) {
    int n = 0;
    for (int k = 0; k < a->n; k++) n += (!out || a->held[k].out) && cv_alive(a->held[k].u);
    return n;
}
// a fire mark at e's tile, d tiles from what it hit (AR_FIRE)
internal void ar_fire_mark(struct armies_st *a, ivec2 e, int d, uint32_t now) {
    int k, old = 0;
    for (k = 0; k < AR_FIRES; k++) {
        if (a->fire[k].t && now - a->fire[k].t < AR_GRIND_MS && ivec2_dist2(a->fire[k].at, e) <= AR_FIRE_JOIN * AR_FIRE_JOIN) break;
        if (a->fire[k].t < a->fire[old].t) old = k;
    }
    if (k == AR_FIRES) { k = old; a->fire[k].at = e; a->fire[k].r = 0; a->fires++; }
    if (d > a->fire[k].r) a->fire[k].r = d;
    a->fire[k].t = now ? now : 1;
}
// THE TRADE since the last judgement (AR_JUDGE above): the hit points the members out lost and those the enemy seen
// near mid lost; the enemy near mid kept for the next; where the shots came from (AR_FIRE). Returns the members fighting.
internal int ar_trade(ee_player *pl, unsigned idx, struct armies_st *a, ivec2 mid, float *dealt, float *lost, uint32_t now) {
    int fighting = 0, fire = PV(idx, g_v_ar_fire, AR_FIRE);
    *dealt = *lost = 0.0f;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (!a->held[k].out) { a->held[k].hp = -1; continue; }
        int hp = cv_alive(u) ? unit_get_hp(u) : 0;
        if (a->held[k].hp > hp) *lost += (float)(a->held[k].hp - hp);
        if (fire && hp > 0 && a->held[k].hp > hp) {
            ee_unit *e = unit_get_attacker(u);
            ee_player *o = e ? unit_get_owner(e) : NULL;
            if (o && player_get_index(o) > 0 && is_enemy(pl, o) && cv_alive(e)) {
                long d2 = ivec2_dist2(unit_get_tile(e), unit_get_tile(u));
                if (d2 >= AR_FIRE_MIN * AR_FIRE_MIN) ar_fire_mark(a, unit_get_tile(e), (int)ceilf(sqrtf((float)d2)), now);
            }
        }
        a->held[k].hp = hp;
        ee_goal *g = hp > 0 ? unit_get_goal(u) : NULL;
        fighting += g && goal_is_attack(g);
    }
    for (int i = 0; i < a->nfoe; i++) {
        ee_unit *u = unit_by_id((unsigned)a->foe[i].id);
        int hp = u && unit_get_serial(u) == a->foe[i].id && cv_alive(u) ? unit_get_hp(u) : 0;
        if (a->foe[i].hp > hp) *dealt += (float)(a->foe[i].hp - hp);
    }
    a->nfoe = 0;
    for (struct grid_it it = grid_it_box(mid.x, mid.y, SG_FOE, 1); grid_it_next(&it) && a->nfoe < AR_FOE; ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !seen_by(u, pl) || ivec2_dist2(unit_get_tile(u), mid) > SG_FOE * SG_FOE) continue;
        if (!cv_mil(it.td) && !fr_building(it.td)) continue;
        a->foe[a->nfoe].id = unit_get_serial(u); a->foe[a->nfoe].hp = unit_get_hp(u); a->nfoe++;
    }
    return fighting;
}
// the breach's look (AR_FIRE): the stand-off under fire - back; what fires at it, or what holds the position, unseen -
// the fastest member looks. 1: the breach is over or the force walks back (nothing more this check)
internal int ar_breach_look(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    int hit = 0;
    for (int k = 0; k < AR_FIRES && !hit; k++) {
        int r = a->fire[k].r + AR_FIRE_PAD;
        hit = a->fire[k].t >= a->breach_t && now - a->fire[k].t < AR_GRIND_MS && ivec2_dist2(a->fire[k].at, a->standoff) <= r * r;
    }
    if (hit) {
        ivec2 old = a->standoff, focus = a->focus;
        ee_unit *t = NULL;
        int d = 0, line = 0, n = ar_count(a, 1);
        if (a->backs_breach >= AR_BREACH_BACKS || !ar_route(pl, idx, a, a->home, &t, &d, &line, now)) {
            ar_log(idx, now, "the breach at %d,%d: the stand-off %d,%d under fire %s - the force of %d home to %d,%d", focus.x, focus.y, old.x, old.y,
                   a->backs_breach >= AR_BREACH_BACKS ? "again" : "and no way left", n, a->home.x, a->home.y);
            a->breach_homes++;
            ar_stopped(a, g_sgs_foe.hp, 0);
            ar_home_all(pl, idx, a, now);
            return 1;
        }
        a->backs_breach++; a->stand_backs++;
        a->target = t; a->target_id = unit_get_serial(t); a->dest = unit_get_tile(t);
        a->stage_t = now ? now : 1; a->way_i = 0;
        if (a->breach) {
            g_ar_breach[idx].focus = a->focus; g_ar_breach[idx].standoff = a->standoff;
            ar_log(idx, now, "the breach at %d,%d: the stand-off %d,%d under fire - back to %d,%d (the position at %d,%d, danger on the way %d)", focus.x, focus.y, old.x, old.y,
                   a->standoff.x, a->standoff.y, a->focus.x, a->focus.y, d);
            for (int k = 0; k < a->n; k++) if (a->held[k].out && cv_alive(a->held[k].u)) ar_order(a, k, ar_goal_at(a->standoff, k, 0), now);
        } else {
            g_ar_breach[idx].on = 0;
            a->stage = a->nway ? 2 : 0;
            ar_log(idx, now, "the breach at %d,%d: the stand-off %d,%d under fire - a way round it now (%d corners, danger %d) to %d,%d", focus.x, focus.y, old.x, old.y, a->nway, d, a->dest.x, a->dest.y);
            if (a->nway) { for (int k = 0; k < a->n; k++) if (a->held[k].out && cv_alive(a->held[k].u)) ar_order(a, k, ar_goal_at(a->way[0], k, 1), now); }
            else ar_attack_out(a, 1, now);
        }
        return 1;
    }
    // the look: the member sent back after AR_SCOUT_MS; then the next where something fires unseen or the position is unseen
    ee_unit *sc = a->scout_id ? unit_by_id((unsigned)a->scout_id) : NULL;
    if (sc && (unit_get_serial(sc) != a->scout_id || !cv_alive(sc))) sc = NULL;
    if (sc && now - a->scout_t < AR_SCOUT_MS) return 0;
    if (sc) {
        for (int k = 0; k < a->n; k++) if (a->held[k].u == sc) { ar_order(a, k, ar_goal_at(a->standoff, k, 0), now); break; }
        a->scout_id = 0;
        return 0;
    }
    a->scout_id = 0;
    ivec2 look = a->focus;
    long best = -1;
    for (int k = 0; k < AR_FIRES; k++) {
        if (!a->fire[k].t || now - a->fire[k].t >= AR_GRIND_MS || ivec2_dist2(a->fire[k].at, a->focus) > (AR_BREACH_R + AR_GRIND_R) * (AR_BREACH_R + AR_GRIND_R)) continue;
        if (ar_enemy_seen(pl, a->fire[k].at, AR_FIRE_JOIN)) continue;
        long d = ivec2_dist2(a->fire[k].at, a->standoff);
        if (best < 0 || d < best) { best = d; look = a->fire[k].at; }
    }
    if (best < 0 && ar_enemy_seen(pl, a->focus, AR_FIRE_JOIN * 2)) return 0;   // the position is seen
    int kb = -1;
    float vb = 0.0f;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (!a->held[k].out || !cv_alive(u) || unit_get_hp(u) * 2 < unit_type_get_hp(unit_get_type_data(u))) continue;
        ee_unit_stats *st = unit_get_stats(u);
        float v = st ? unit_stats_get_speed(st) : 0.0f;
        if (kb < 0 || v > vb) { kb = k; vb = v; }
    }
    if (kb < 0) return 0;
    ee_unit *u = a->held[kb].u;
    ee_unit_stats *st = unit_get_stats(u);
    float sight = st ? unit_stats_get_sight(st) : 4.0f, dx = (float)(a->standoff.x - look.x), dy = (float)(a->standoff.y - look.y), dl = sqrtf(dx * dx + dy * dy);
    ivec2 at = look;
    if (dl > sight) { at.x = look.x + (int)(dx / dl * (sight - 1.0f)); at.y = look.y + (int)(dy / dl * (sight - 1.0f)); }
    ar_order(a, kb, ar_goal_at(at, 0, 0), now);
    a->scout_id = unit_get_serial(u); a->scout_t = now ? now : 1; a->scouts++;
    ar_log(idx, now, "the breach at %d,%d: a look at %d,%d (%s) - type %d to %d,%d (sight %.0f)", a->focus.x, a->focus.y, look.x, look.y, best >= 0 ? "fire from there, nothing seen" : "the position unseen",
           unit_type_get_id(unit_get_type_data(u)), at.x, at.y, sight);
    return 0;
}
// the force out: its judgement, its next leg or its way home
internal void ar_out(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    ivec2 mid;
    int n = ar_side(a, 1, &mid);
    if (n < (a->out == 2 ? 1 : AR_MIN_LAUNCH / 2)) {
        ar_log(idx, now, "the force out is down to %d - home to %d,%d", n, a->home.x, a->home.y);
        if (a->out == 1) ar_stopped(a, g_sgs_foe.hp, 0);
        ar_home_all(pl, idx, a, now);
        return;
    }
    if (!a->judge_t || now - a->judge_t >= AR_JUDGE_EVERY) {
        a->judge_t = now ? now : 1;
        float r = ar_judge(pl, a, 1, mid);
        g_ar_foe_mid_n = g_sgs_foe.n; g_ar_foe_mid_hp = g_sgs_foe.hp;
        float rd = ar_judge(pl, a, 1, a->dest);
        int own = g_ar_own.n, foe = g_sgs_foe.n;
        a->ratio = a->stage == 3 ? r : rd < r ? rd : r;   // at the stand-off: its own spot (the target is beyond the position it waits out)
        float dealt, lost, sd = 0.0f, sl = 0.0f;
        int fighting = ar_trade(pl, idx, a, mid, &dealt, &lost, now), judge = PV(idx, g_v_ar_judge, AR_JUDGE);
        int contact = fighting >= AR_CONTACT_MIN && fighting * 10 >= n;
        if (contact) {
            a->dealt[a->trade_n % AR_TRADE_N] = dealt; a->lost[a->trade_n % AR_TRADE_N] = lost;
            a->trade_n++; a->contacts++;
        }
        int tn = a->trade_n < AR_TRADE_N ? a->trade_n : AR_TRADE_N;
        for (int i = 0; i < tn; i++) { sd += a->dealt[i]; sl += a->lost[i]; }
        if (g_ar_debug) ar_log(idx, now, "  judge: %d out (hp %.0f) at %d,%d - at the middle %.2f (enemy %d, hp %.0f), at %d,%d %.2f (enemy %d, hp %.0f); %d fighting, dealt %.0f lost %.0f (%d in contact: %.0f / %.0f)", n, g_ar_own.hp, mid.x, mid.y, r, g_ar_foe_mid_n, g_ar_foe_mid_hp, a->dest.x, a->dest.y, rd, foe, g_sgs_foe.hp, fighting, dealt, lost, tn, sd, sl);
        if (judge && contact) {   // the fight judges
            a->weak = 0;
            if (tn >= AR_TRADE_MIN && sl * 100.0f > sd * (float)AR_TRADE_BAD && sl * 100.0f >= a->out_hp * (float)AR_TRADE_FLOOR) {
                ar_log(idx, now, "the force of %d at %d,%d is trading badly (lost %.0f, dealt %.0f in %d judgements; %d fighting) - home to %d,%d", n, mid.x, mid.y, sl, sd, tn, fighting, a->home.x, a->home.y);
                a->traded_home++;
                int gk = 0;   // the meat grinder remembered (the oldest place taken) - not at home with the fire marks
                for (int k = 1; k < 8; k++) if (a->grind[k].t < a->grind[gk].t) gk = k;
                if (!PV(idx, g_v_ar_fire, AR_FIRE) || ivec2_dist2(mid, a->home) > AR_GRIND_R * AR_GRIND_R) { a->grind[gk].at = mid; a->grind[gk].t = now ? now : 1; a->grinds++; }
                if (PV(idx, g_v_ar_price, AR_PRICE)) {   // the price at its target (the oldest record taken)
                    int pk = 0;
                    for (int k = 1; k < AR_PRICES; k++) if (a->price[k].t < a->price[pk].t) pk = k;
                    a->price[pk].at = a->dest; a->price[pk].lost = sl; a->price[pk].dealt = sd; a->price[pk].hp = a->out_hp; a->price[pk].t = now ? now : 1;
                    a->priced++;
                }
                if (a->out == 1) ar_stopped(a, rd < r ? g_sgs_foe.hp : g_ar_foe_mid_hp, 0);
                ar_home_all(pl, idx, a, now);
                return;
            }
        } else if (a->ratio * 100.0f < (float)(judge ? PV(idx, g_v_ar_far_pct, AR_FAR_PCT) : PV(idx, g_v_ar_back_pct, AR_BACK_PCT))) {
            if (++a->weak >= 2) {
                ar_log(idx, now, "the force of %d at %d,%d is losing (%.2f: own %d, enemy %d at %d,%d%s) - home to %d,%d", n, mid.x, mid.y, a->ratio, own, foe, a->dest.x, a->dest.y, judge ? ", before contact" : "", a->home.x, a->home.y);
                if (a->out == 1) ar_stopped(a, rd < r ? g_sgs_foe.hp : g_ar_foe_mid_hp, 0);
                ar_home_all(pl, idx, a, now);
                return;
            }
        } else a->weak = 0;
        if (AR_BLEED_PCT && g_ar_own.hp < a->out_hp * (float)(100 - AR_BLEED_PCT) / 100.0f) {
            ar_log(idx, now, "the force of %d at %d,%d has bled (hp %.0f of %.0f) - home to %d,%d", n, mid.x, mid.y, g_ar_own.hp, a->out_hp, a->home.x, a->home.y);
            if (a->out == 1) ar_stopped(a, g_sgs_foe.hp, 0);
            ar_home_all(pl, idx, a, now);
            return;
        }
    }
    if (a->stage == 3) {   // the breach: the way on searched again - open, the push; too long, the push if it wins there, else home
        if (now - a->breach_chk < AR_BREACH_EVERY) return;
        a->breach_chk = now ? now : 1;
        if (PV(idx, g_v_ar_fire, AR_FIRE) && ar_breach_look(pl, idx, a, now)) return;
        ee_map *map = game_map();
        int size = map ? (int)map_get_size(map) : 0, cn = size > 0 ? ar_route_cells(idx, size, map_area(a->standoff, 2), a, now) : 0, d = 0, w = 0, len = 0;
        static int path[65536];
        if (cn) len = ar_route_find(cn, a->standoff, a->dest, path, 65536, &d, &w);
        int open = len && d <= AR_BREACH_OPEN && !w, late = now - a->breach_t >= AR_BREACH_MAX;
        float r = open || !late ? 0.0f : ar_judge(pl, a, 1, a->dest);
        if (open || (late && r * 100.0f >= (float)AR_GO_PCT)) {
            ar_log(idx, now, "the breach at %d,%d after %.1f min: %s - the force of %d attack-moves at %d,%d", a->focus.x, a->focus.y, (now - a->breach_t) / 60000.0,
                   open ? "the way open" : "still held, but it wins there", n, a->dest.x, a->dest.y);
            a->breach_pushes++;
            g_ar_breach[idx].on = 0;
            ar_rams(pl, a);
            ar_attack_out(a, 1, now);
        } else if (late) {
            ar_log(idx, now, "the breach at %d,%d after %.1f min: the way still held (danger %d, walls %d) and the target too strong (%.2f) - the force of %d home to %d,%d", a->focus.x, a->focus.y,
                   (now - a->breach_t) / 60000.0, d, w, r, n, a->home.x, a->home.y);
            a->breach_homes++;
            ar_stopped(a, g_sgs_foe.hp, 0);
            ar_home_all(pl, idx, a, now);
        }
        return;
    }
    if (a->stage == 2) {   // the way round: the next corner once most are there (or the leg took too long), the building from the last
        ivec2 c = a->way[a->way_i];
        int arrived = 0;
        for (int k = 0; k < a->n; k++) arrived += a->held[k].out && cv_alive(a->held[k].u) && ivec2_dist2(unit_get_tile(a->held[k].u), c) <= 4 * AR_DONE * AR_DONE;
        if (arrived * 100 >= n * AR_APPROACH_PCT || now - a->stage_t >= AR_LEG_MAX) {
            a->stage_t = now ? now : 1;
            if (++a->way_i >= a->nway && a->breach) {
                a->stage = 3; a->breach_t = a->breach_chk = now ? now : 1; a->breaches++; a->backs_breach = 0; a->scout_id = 0;
                g_ar_breach[idx].on = 1; g_ar_breach[idx].focus = a->focus; g_ar_breach[idx].standoff = a->standoff;
                ar_log(idx, now, "at the stand-off %d,%d (%d of %d there): the position at %d,%d is shelled before the push at %d,%d", a->standoff.x, a->standoff.y, arrived, n, a->focus.x, a->focus.y, a->dest.x, a->dest.y);
            } else if (a->way_i >= a->nway) {
                ar_log(idx, now, "the way round done (%d corners, %d of %d at the last) - the force attack-moves at %d,%d", a->nway, arrived, n, a->dest.x, a->dest.y);
                ar_attack_out(a, 1, now);
            } else
                for (int k = 0; k < a->n; k++) if (a->held[k].out && cv_alive(a->held[k].u)) ar_order(a, k, ar_goal_at(a->way[a->way_i], k, 1), now);
        }
        return;
    }
    if (a->stage == 1) {   // the approach march: done - the attack-move; else nothing more (the members at their spots are not idle-away)
        int arrived = 0;
        for (int k = 0; k < a->n; k++) arrived += a->held[k].out && cv_alive(a->held[k].u) && ivec2_dist2(unit_get_tile(a->held[k].u), a->approach) <= 4 * AR_DONE * AR_DONE;
        if (arrived * 100 >= n * AR_APPROACH_PCT || now - a->stage_t >= AR_APPROACH_MAX || ar_enemy_band(pl, mid, SG_NEAR) >= AR_RAID_BAND) {
            ar_log(idx, now, "the approach to %d,%d done (%d of %d there, %.1f min) - the force attack-moves at %d,%d", a->approach.x, a->approach.y, arrived, n, (now - a->stage_t) / 60000.0, a->dest.x, a->dest.y);
            ar_attack_out(a, 1, now);
        }
        return;
    }
    // THE RAID'S LEASH ([ai_base] raid_leash, default 0 until measured; the player 2026-11-07: "i dont blindly run after
    // leaving raiders, i even call the soldiers back that automatically followed"): the force attack-moves at a raid
    // and the game's attack-move follows its target wherever it rides - on a raid a member raid_leash tiles past the
    // raid's spot is no longer out: home (a plain move: it stops chasing), where it waits for the force.
    if (a->out == 2 && g_ar_leash > 0) {
        int lim = AR_DONE + g_ar_leash, called = 0;
        for (int k = 0; k < a->n; k++) {
            ee_unit *u = a->held[k].u;
            if (!a->held[k].out || !cv_alive(u) || ivec2_dist2(unit_get_tile(u), a->dest) <= lim * lim) continue;
            a->held[k].out = 0;
            ar_order(a, k, ar_goal_at(a->home, k, 0), now);
            called++;
        }
        if (called) { a->leashed += (unsigned)called; ar_log(idx, now, "the raid at %d,%d: %d chasing past %d tiles called home", a->dest.x, a->dest.y, called, lim); }
    }
    if (a->out == 1 && AR_CREW_MAIN(idx)) tn_crew_follow(pl, idx, mid, a->home, now);
    // the leg done: the building down (or gone from the tile with a member there), the raid cleared
    int there = 0;
    for (int k = 0; k < a->n && !there; k++) there = a->held[k].out && cv_alive(a->held[k].u) && ivec2_dist2(unit_get_tile(a->held[k].u), a->dest) <= AR_DONE * AR_DONE;
    int done;
    if (a->out == 2) { a->clear = ar_enemy_near(pl, a->dest, SG_FOE, 0) ? 0 : a->clear + 1; done = a->clear >= 2; }
    else done = (a->target && (unit_get_serial(a->target) != a->target_id || !cv_alive(a->target))) || (there && !ar_enemy_near(pl, a->dest, 3, 1));
    if (done && a->out == 1 && (TN_TAKE(idx) || (TN_RAID(idx) && (g_ar_site_set[idx] || site_wants_army(idx, a->dest))))) {   // TAKE MINES: the razed mine becomes its own, the force over it until it stands
        if (!g_ar_site_set[idx] && tn_mine_near(pl, a->dest, 12)) { g_ar_site[idx] = a->dest; g_ar_site_set[idx] = 1; }
        ee_unit *nb = g_ar_site_set[idx] && !g_ar_claim_t[idx] ? ar_near_building(pl, g_ar_site[idx], 22) : NULL;
        if (nb) {   // what else stands there first
            ivec2 nd = unit_get_tile(nb);
            float nr = ar_judge(pl, a, 1, nd);
            if (nr * 100.0f >= (float)AR_GO_PCT) {
                a->legs++;
                ar_log(idx, now, "%d,%d is down - clearing the mines at %d,%d: on to %d,%d (%.2f)", a->dest.x, a->dest.y, g_ar_site[idx].x, g_ar_site[idx].y, nd.x, nd.y, nr);
                ar_go(pl, idx, a, 1, nd, nb, 1, now);
                return;
            }
            g_ar_site_set[idx] = 0;   // too strong: the ordinary next leg below judges it
        }
        if (g_ar_site_set[idx] && !g_ar_claim_t[idx] && tn_claim(pl, idx, g_ar_site[idx], now)) { g_ar_claim_t[idx] = now ? now : 1; ar_log(idx, now, "%d,%d is down - the force of %d holds it for its town", g_ar_site[idx].x, g_ar_site[idx].y, n); }
        if (g_ar_claim_t[idx] && now - g_ar_claim_t[idx] < AR_CLAIM_MS && g_ar_site_set[idx] && tn_claim_going(idx, g_ar_site[idx])) return;
        if (g_ar_claim_t[idx]) ar_log(idx, now, "%d,%d: the force leaves after %.1f min, its town %s", g_ar_site[idx].x, g_ar_site[idx].y, (now - g_ar_claim_t[idx]) / 60000.0, tn_claim_going(idx, g_ar_site[idx]) ? "not standing yet" : "standing");
        g_ar_site_set[idx] = 0; g_ar_claim_t[idx] = 0;
    }
    if (done) {
        if (a->out == 2) { ar_log(idx, now, "the raid at %d,%d is over - the force of %d home to %d,%d", a->dest.x, a->dest.y, n, a->home.x, a->home.y); ar_home_all(pl, idx, a, now); return; }
        ee_unit *t = ar_target(pl, mid);
        ivec2 d = t ? unit_get_tile(t) : mid;
        float r = t ? ar_judge(pl, a, 1, d) : 0.0f;
        if (!t || r * 100.0f < (float)AR_GO_PCT) {
            ar_log(idx, now, "%d,%d is down - %s - the force of %d home to %d,%d", a->dest.x, a->dest.y, t ? "the next is too strong" : "no enemy building known", n, a->home.x, a->home.y);
            ar_stopped(a, t ? g_sgs_foe.hp : 0.0f, 1);
            ar_home_all(pl, idx, a, now);
            return;
        }
        a->legs++;
        ar_log(idx, now, "%d,%d is down - the force of %d on to %d,%d (%.2f: own %d, enemy %d)", a->dest.x, a->dest.y, n, d.x, d.y, r, g_ar_own.n, g_sgs_foe.n);
        ar_go(pl, idx, a, 1, d, t, 1, now);
        return;
    }
    // a member idle away from the destination: ordered again
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (!a->held[k].out || !cv_alive(u) || !ar_idle(u) || now - a->held[k].order_t < AR_REORDER) continue;
        if (ivec2_dist2(unit_get_tile(u), a->dest) <= AR_DONE * AR_DONE) continue;
        ar_order(a, k, ar_goal_at(a->dest, k, 1), now);
    }
}
// THE SITES' MEMORY: a mine asked of the force still stands, known to it
internal int ar_site_asked(ee_player *pl, unsigned idx) {
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (o && player_get_index(o) > 0 && is_enemy(pl, o) && cv_alive(it.u) && seen_by(it.u, pl) && site_wants_army(idx, unit_get_tile(it.u)) && tn_mine_drop(pl, it.u, it.td)) return 1;
    }
    return 0;
}
// RAID PARTIES ([ai_base] raid_parties, on since 2026-11-11; [autotest] v_raid_parties; army view step 3 - the player
// 2026-11-11: the AI splits and merges its armies as it needs, as many as it needs): a raid near home is answered by the
// part of the force nearest it that beats it by AR_PART_PCT (the fight estimate, stage_foe: what the AI knows there), as
// many as the raid's band at least, instead of the whole force, which stays home for the attack time and the next raid.
// The part is an army of its own (army_view.c AV_RAID): it attack-moves at the raiders - the nearest land fighter it sees
// within AR_PART_LOOK of where it goes or of its middle, not beyond AR_RAID_REACH of home - and goes back into the force
// once there is none for two looks running, or
// once it no longer beats them by AR_BACK_PCT. A raider a party is at already isn't answered again. When even the whole
// force doesn't beat the raid by AR_PART_PCT, the whole force goes, as before. Measured (10 games each, Continental large,
// 90 min): against the non-cheating game AI off / on - won 10 / 10, buildings lost 2.3 / 1.5, losses 46 / 66 (2 se), razed
// 29.6 / 30.5, pushes 1.5 / 2.2; the mod against itself (h2h, 4 AIs, mirrored, two batches of 10; B with parties): razed
// +20.8 (se 8.5) and +13.1 (5.6), soldiers +24.2 (10.8) and +9.5 (10.8), kills +25.4 and +19.8 (about 1 se), losses -0.9
// and +14.5 (noise), alive +0.10 and +0.05. On.
#define AR_PART_PCT 200
#define AR_PART_LOOK 20
internal int g_ar_parts = 0, g_v_ar_parts = -1;
#define AR_PARTS(idx) PV(idx, g_v_ar_parts, g_ar_parts)
internal struct { unsigned sent, cleared, losing, wiped; int men, lost; } g_ar_pt[16];   // the log's (not saved)
internal int ar_part_at(unsigned idx, ivec2 p) {   // a raid party goes for p already
    for (int j = 0; j < g_av[idx & 15].n; j++) {
        struct av_army *o = &g_av[idx & 15].a[j];
        if (o->kind == AV_RAID && ivec2_dist2(o->dest, p) <= AR_PART_LOOK * AR_PART_LOOK) return 1;
    }
    return 0;
}
internal void ar_part_side(ee_player *pl, struct av_army *o) {   // its members into g_ar_own
    st_side_clear(&g_ar_own);
    for (int k = 0; k < o->n; k++) if (av_live(&o->m[k], pl) && cv_alive(o->m[k].u)) st_side_add(&g_ar_own, o->m[k].u);
}
internal void ar_part_order(struct av_army *o, ee_player *pl, ivec2 d, int idle_only, uint32_t now) {
    (void)now;
    for (int k = 0; k < o->n; k++) {
        ee_unit *u = o->m[k].u;
        if (!av_live(&o->m[k], pl) || !cv_alive(u) || (idle_only && !ar_idle(u))) continue;
        ee_goal *g = ar_goal_at(d, k, 1);
        if (g) unit_goal(u, g, 0, 0x1F40);
    }
}
// the members at home nearest d that beat what pl knows at d by AR_PART_PCT, out of the force into a party of their own;
// 0: no part does (or only the whole force) - the caller sends the force
struct ar_cand { int k; long d; };
internal float ar_part_ratio(struct armies_st *a, const struct ar_cand *c, int m) {   // the m nearest against g_sgs_foe
    st_side_clear(&g_ar_own);
    for (int i = 0; i < m; i++) st_side_add(&g_ar_own, a->held[c[i].k].u);
    return st_ratio(&g_ar_own, &g_sgs_foe);
}
internal int ar_part_send(ee_player *pl, unsigned idx, struct armies_st *a, ivec2 d, int band, uint32_t now) {
    static struct ar_cand c[AR_HELD];
    int nc = 0;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (a->held[k].out || a->held[k].back || !cv_alive(u)) continue;
        c[nc].k = k; c[nc++].d = ivec2_dist2(unit_get_tile(u), d);
    }
    for (int i = 1; i < nc; i++) {   // nearest first (insertion: stable, the held order among equals)
        int j = i; struct ar_cand x = c[i];
        while (j > 0 && c[j - 1].d > x.d) { c[j] = c[j - 1]; j--; }
        c[j] = x;
    }
    stage_foe(pl, d);
    float want = (float)AR_PART_PCT / 100.0f;
    int lo = 1, hi = nc;   // the fewest nearest that beat it by want: a binary search (more soldiers never judge worse)
    int least = band > AR_MIN_RAID ? band : AR_MIN_RAID;   // never fewer than the raid's band: what the AI knows there can be gone (a party of 1 went at "nothing")
    if (nc <= least || ar_part_ratio(a, c, nc) < want) return 0;
    lo = least;
    while (lo < hi) { int mid = (lo + hi) / 2; if (ar_part_ratio(a, c, mid) >= want) hi = mid; else lo = mid + 1; }
    float r = ar_part_ratio(a, c, lo);
    if (lo >= nc) return 0;
    struct av_army *o = av_new(idx, AV_RAID, now);
    if (!o) return 0;
    static uint8_t sel[AR_HELD];
    memset(sel, 0, (size_t)a->n);
    for (int i = 0; i < lo; i++) sel[c[i].k] = 1;
    int m = 0;
    for (int k = 0; k < a->n; k++) {
        if (sel[k] && av_put(o, a->held[k].u)) { o->sent_hp += (float)unit_get_hp(a->held[k].u); continue; }
        a->held[m++] = a->held[k];
    }
    a->n = m;
    o->dest = d; o->sent = o->n; o->look_t = now ? now : 1;
    ar_part_order(o, pl, d, 0, now);
    g_ar_pt[idx].sent++; g_ar_pt[idx].men += o->n;
    ar_log(idx, now, "a raid of %d at %d,%d - a party of %d of the force's %d goes (%.2f: own %d, enemy %d), army %d", band, d.x, d.y, o->n, m + o->n, r, g_ar_own.n, g_sgs_foe.n, o->id);
    return 1;
}
// each force tick: the parties out - the dead dropped, every AR_RAID_EVERY a look at the raiders, home and merged once done
internal void ar_parts_tick(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    for (int j = g_av[idx].n - 1; j >= 0; j--) {
        struct av_army *o = &g_av[idx].a[j];
        if (o->kind != AV_RAID) continue;
        int m = 0;
        for (int k = 0; k < o->n; k++) if (av_live(&o->m[k], pl)) o->m[m++] = o->m[k];
        o->n = m;
        if (!o->n) { g_ar_pt[idx].wiped++; g_ar_pt[idx].lost += o->sent; ar_log(idx, now, "the raid party of %d at %d,%d (army %d) is lost", o->sent, o->dest.x, o->dest.y, o->id); av_end(idx, o); continue; }
        if (now >= o->look_t && now - o->look_t < AR_RAID_EVERY) continue;
        o->look_t = now ? now : 1;
        long sx = 0, sy = 0; int na = 0;
        for (int k = 0; k < o->n; k++) if (cv_alive(o->m[k].u)) { ivec2 p = unit_get_tile(o->m[k].u); sx += p.x; sy += p.y; na++; }
        ivec2 mid = na ? iv2((int)(sx / na), (int)(sy / na)) : o->dest;
        ee_unit *best = NULL; long bd = 0;   // round where it goes and round itself (on its way the raiders are out of its sight: it turned back after 6 s)
        for (int at = 0; at < 2; at++) {
            ivec2 c = at ? mid : o->dest;
            for (struct grid_it it = grid_it_box(c.x, c.y, AR_PART_LOOK, 1); grid_it_next(&it); ) {
                ee_player *ow = unit_get_owner(it.u);
                if (!ow || player_get_index(ow) <= 0 || !is_enemy(pl, ow) || !ms_land_soldier(it.u, it.td) || !seen_by(it.u, pl)) continue;
                ivec2 p = unit_get_tile(it.u);
                long dd = ivec2_dist2(p, mid);
                if (ivec2_dist2(p, c) > (long)AR_PART_LOOK * AR_PART_LOOK || (ivec2_dist2(p, a->home) > (long)AR_RAID_REACH * AR_RAID_REACH && !(AR_HELD_GROUND(idx) && tv_holds(idx, p))) || (best && dd >= bd)) continue;
                best = it.u; bd = dd;
            }
        }
        int done = 0;
        if (!best) done = ++o->clear >= 2 ? 1 : 0;
        else {
            o->clear = 0;
            ivec2 t = unit_get_tile(best);
            stage_foe(pl, t);
            ar_part_side(pl, o);
            float r = st_ratio(&g_ar_own, &g_sgs_foe);
            if (r < (float)AR_BACK_PCT / 100.0f) done = 2;
            else if (ivec2_dist2(t, o->dest) > 4 * 4) { o->dest = t; ar_part_order(o, pl, t, 0, now); }
            else ar_part_order(o, pl, o->dest, 1, now);
        }
        if (!done) continue;
        float hp = 0;
        int back = 0;
        for (int k = 0; k < o->n; k++) {   // home, and into the force: its gather takes them on from there
            ee_unit *u = o->m[k].u;
            hp += (float)unit_get_hp(u);
            if (a->n >= AR_HELD) break;
            ee_goal *g = ar_goal_at(a->home, a->n, 0);
            if (g) unit_goal(u, g, 0, 0x1F40);
            a->held[a->n].u = u; a->held[a->n].id = o->m[k].id; a->held[a->n].order_t = now ? now : 1; a->held[a->n].out = 0; a->held[a->n].back = 0; a->held[a->n].pick = 0; a->held[a->n].hp = -1; a->n++;
            back++;
        }
        if (done == 1) g_ar_pt[idx].cleared++; else g_ar_pt[idx].losing++;
        g_ar_pt[idx].lost += o->sent - o->n;
        ar_log(idx, now, "the raid party at %d,%d (army %d) back into the force, %s: %d of %d (hp %.0f of %.0f) after %.1f min", o->dest.x, o->dest.y, o->id,
               done == 1 ? "the raid over" : "losing", back, o->sent, hp, o->sent_hp, (now - o->made_t) / 60000.0);
        av_end(idx, o);
    }
}
// whose buildings a raid on is answered: its own within AR_RAID_REACH of home; ALLY AID ([ai_base] ally_aid, off until
// measured): an ally's within ally_aid tiles of home - the game's aid to allies (0x474E85) finds no land soldiers in the
// commander's pool of a player whose force holds them, so nothing helped an ally since mod_armies
internal int ar_answers_for(ee_player *pl, ee_player *o, int far_out) {
    if (!o) return 0;
    if (o == pl) return !far_out;
    return g_ar_aid > 0 && player_get_index(o) > 0 && !is_enemy(pl, o) && !is_out(o);
}
// the force at home: its members gather there; a raid near answered; the attack time's push
internal void ar_at_home(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    int convoy = a->back_t && (PV(idx, g_v_ar_way, AR_WAY) & 5), gather = PV(idx, g_v_ar_form, AR_FORM) & 2, picked = 0;   // the convoy's or the group move's members are theirs
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        a->held[k].pick = 0;
        if (a->held[k].out || (convoy && a->held[k].back) || !cv_alive(u) || !ar_idle(u) || now - a->held[k].order_t < AR_REORDER) continue;
        if (ivec2_dist2(unit_get_tile(u), a->home) <= AR_HOME_R * AR_HOME_R) continue;
        if (gather) { a->held[k].pick = 1; picked++; continue; }
        ar_order(a, k, ar_goal_at(a->home, k, 0), now);
    }
    if (picked) { a->gathers++; ar_group_move(pl, a, 2, a->home, now); }
    int n = ar_count(a, 0);
    if (a->out) return;
    if (n >= AR_MIN_RAID && (!a->raid_t || now - a->raid_t >= AR_RAID_EVERY)) {   // the raid nearest home
        a->raid_t = now ? now : 1;
        ee_unit *raider = NULL;
        ee_player *raided = NULL;
        long bd = 0;
        int band = 0, reach = g_ar_aid > AR_RAID_REACH ? g_ar_aid : AR_RAID_REACH;
        int wide = AR_HELD_GROUND(idx) != 0, rfar = 0;   // HELD GROUND: the whole map looked at
        unsigned hland = wide ? area_id(a->home, 2) : 0;
        for (struct grid_it it = wide ? grid_it_all(1) : grid_it_box(a->home.x, a->home.y, reach, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_player *o = unit_get_owner(u);
            if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !ms_land_soldier(u, it.td) || !seen_by(u, pl)) continue;
            ivec2 p = unit_get_tile(u);
            long d = ivec2_dist2(p, a->home);
            int ours = wide && d > (long)AR_RAID_REACH * AR_RAID_REACH && hland && area_id(p, 2) == hland && tv_holds(idx, p);   // out of reach, in ground it holds on its land
            if ((d > (long)reach * reach && !ours) || (raider && d >= bd)) continue;
            if (AR_PARTS(idx) && ar_part_at(idx, p)) continue;   // RAID PARTIES: one is at it already
            int far_out = d > (long)AR_RAID_REACH * AR_RAID_REACH && !ours;   // only an ally's buildings are answered out there
            // a raid, not a scout passing: the raider's band (enemy fighters seen within AR_RAID_R of it) is AR_RAID_BAND
            // strong, or one of them is attacking something of ours - the whole force went for every single scout before
            // (12-23 raids a game, 3000 orders, the force away when the real attack came)
            ee_player *close = NULL;
            int nb = 0, hitting = 0;
            for (struct grid_it jt = grid_it_box(p.x, p.y, AR_RAID_R, 1); grid_it_next(&jt) && !close; )
                if (cv_alive(jt.u) && fr_building(jt.td) && ivec2_dist2(unit_get_tile(jt.u), p) <= AR_RAID_R * AR_RAID_R && ar_answers_for(pl, unit_get_owner(jt.u), far_out))
                    close = unit_get_owner(jt.u);
            if (!close) continue;
            for (struct grid_it jt = grid_it_box(p.x, p.y, AR_RAID_R, 1); grid_it_next(&jt) && !hitting; ) {
                ee_player *jo = unit_get_owner(jt.u);
                if (!jo || !is_enemy(pl, jo) || !ms_land_soldier(jt.u, jt.td) || !seen_by(jt.u, pl) || ivec2_dist2(unit_get_tile(jt.u), p) > AR_RAID_R * AR_RAID_R) continue;
                nb++;
                ee_unit *t = fl_target_of(jt.u);
                hitting = t && ar_answers_for(pl, unit_get_owner(t), far_out);
            }
            if (nb < PV(idx, g_v_ar_raid_band, AR_RAID_BAND) && !hitting) continue;
            raider = u; bd = d; band = nb; raided = close; rfar = ours;
        }
        ivec2 d = raider ? unit_get_tile(raider) : iv2(-1000, -1000);
        int again = raider && a->raid_c_t && now - a->raid_c_t <= 2 * AR_RAID_EVERY && ivec2_dist2(d, a->raid_c) <= AR_RAID_SAME * AR_RAID_SAME;   // seen there twice running
        a->raid_c = d; a->raid_c_t = raider ? (now ? now : 1) : 0;
        if (again) {
            float r = ar_judge(pl, a, 0, d), need = (float)AR_BACK_PCT / 100.0f;
            if (g_ar_raid_answer > need) need = g_ar_raid_answer;   // raid_answer: only with soldiers to spare
            if (r >= need) {
                a->raids++;
                g_ar_far[idx] += rfar != 0;
                if (raided && raided != pl) g_ar_aids[idx]++;
                if (AR_PARTS(idx) && ar_part_send(pl, idx, a, d, band, now)) return;   // RAID PARTIES: a part goes, the rest stays
                ar_log(idx, now, "a raid of %d at %d,%d%s - the force of %d goes (%.2f: own %d, enemy %d)", band, d.x, d.y, raided && raided != pl ? " on an ally's" : "", n, r, g_ar_own.n, g_sgs_foe.n);
                ar_go(pl, idx, a, 2, d, NULL, 0, now);
                return;
            }
        }
    }
    // PRESSING ON: the game's attack times are minutes apart; after a push the force goes again as soon as it beats
    // what stopped it (the enemy's hit points at the judgement that sent it home: the judge at the target from home
    // sees nothing of an enemy base it hasn't scouted) by AR_GO_PCT, AR_PRESS_HOME_PCT of it gathered, and the target
    // judged as at the attack time. The first try waited for the force's own hit points as it went out: a push of 123
    // that razed three buildings and came home with 50 never pressed again.
    // OVER THE SEA: home with nothing known on its land, the landing is asked all along (every 30 s) - asked only at the
    // attack times it went 0.5 times a game against the commander's 1.7 (10 games each on Small Islands); when it goes
    // is the landing's (the beach against what the force can spare, its tries' pauses)
    if (AR_SEA(idx) && n >= AR_MIN_LAUNCH && (!g_ar_lent[idx].look_t || now < g_ar_lent[idx].look_t || now - g_ar_lent[idx].look_t >= 30000)) {
        g_ar_lent[idx].look_t = now ? now : 1;
        unsigned land = map_area(a->home, 2);
        ee_unit *across = land && !ar_target_on(pl, a->home, land) ? ar_target(pl, a->home) : NULL;
        if (across) {
            if (!g_ar_lent[idx].ask_t || now < g_ar_lent[idx].ask_t || now - g_ar_lent[idx].ask_t > 300000) {
                g_ar_lent[idx].asked++;
                ar_log(idx, now, "the force of %d at %d,%d has no enemy known on its land - a landing asked on P%d", n, a->home.x, a->home.y, player_get_index(unit_get_owner(across)));
            }
            g_ar_lent[idx].ask_t = now ? now : 1;
            landing_ask(pl, unit_get_owner(across), now);
        }
    }
    uint32_t due = player_get_next_attack(pl);
    int press = 0;
    int press_mode = PV(idx, g_v_ar_press, AR_PRESS);
    if (press_mode && a->stopped && (press_mode < 2 || a->stopped == 2) && n >= AR_MIN_LAUNCH && (!a->press_t || now - a->press_t >= AR_PRESS_EVERY) && !(now + 2 * AR_TICK >= due && a->wait_due != due)) {
        a->press_t = now ? now : 1;
        int at_home = 0;
        for (int k = 0; k < a->n; k++) at_home += cv_alive(a->held[k].u) && ivec2_dist2(unit_get_tile(a->held[k].u), a->home) <= AR_HOME_R * AR_HOME_R;
        ar_side(a, 0, NULL);
        press = at_home * 100 >= n * AR_PRESS_HOME_PCT && g_ar_own.hp * 100.0f >= a->stop_hp * (float)AR_GO_PCT;
    }
    int forced = g_ar_push_now >> idx & 1;   // test: attacknow (the game moves its own attack time on before this tick sees it)
    g_ar_push_now &= ~(1u << idx);
    int asked = 0;   // THE SITES' MEMORY: a defended mine is the force's without waiting for the attack time
    if (TN_RAID(idx) && n >= AR_DET_MIN && (!g_ar_ask_t[idx] || now - g_ar_ask_t[idx] >= 30000) && (!a->back_t || now - a->back_t >= AR_DET_PAUSE)) {
        g_ar_ask_t[idx] = now ? now : 1;
        asked = ar_site_asked(pl, idx);
    }
    if (press || asked || (n >= AR_MIN_LAUNCH && ((now + 2 * AR_TICK >= due && a->wait_due != due) || forced))) {   // the attack time: once per attack time
        if (!press && !asked) a->wait_due = due;
        ee_unit *t = NULL;
        int way_d = 0, line_d = 0, routed = 0;
        a->nway = 0;
        if (PV(idx, g_v_ar_route, AR_ROUTE)) { ivec2 mid; ar_side(a, 0, &mid); routed = ar_route(pl, idx, a, mid, &t, &way_d, &line_d, now); }
        unsigned land = AR_SEA(idx) ? map_area(a->home, 2) : 0;   // OVER THE SEA: the force walks only to what it can reach
        if (!routed) t = ar_target_on(pl, a->home, land);
        if (!t && land && !press && !asked) {   // nothing on its land: the nearest known across the water is the landing's
            ee_unit *across = ar_target(pl, a->home);
            if (across) {
                ivec2 fp = unit_get_tile(across);
                g_ar_lent[idx].asked++;
                landing_ask(pl, unit_get_owner(across), now);
                ar_log(idx, now, "the attack time: the force of %d stays at %d,%d - the nearest enemy building known, %d,%d of P%d, lies across the water: a landing asked",
                       n, a->home.x, a->home.y, fp.x, fp.y, player_get_index(unit_get_owner(across)));
                return;
            }
        }
        ivec2 d = t ? unit_get_tile(t) : a->home;
        float r = t ? ar_judge(pl, a, 0, d) : 0.0f, foe_hp = 0.0f;
        float ra = t ? ar_assumed(idx, a, now, &foe_hp) : 99.0f;
        if (ra < r) r = ra;
        float rp = t ? ar_price(idx, a, d, now) : 99.0f;
        int priced = rp < r && rp * 100.0f < (float)AR_GO_PCT;
        if (rp < r) r = rp;
        if (!t || r * 100.0f < (float)AR_GO_PCT) {
            if (press || asked) return;   // told at the attack time
            a->waits++; a->price_waits += priced;
            if (t) { a->strong = d; a->strong_t = now ? now : 1; }
            if (t) ar_log(idx, now, "the attack time: the force of %d waits at %d,%d - %d,%d is too strong (%.2f: own %d, enemy %d; assumed hp %.0f against own %.0f, %.2f; the price %.2f)", n, a->home.x, a->home.y, d.x, d.y, r, g_ar_own.n, g_sgs_foe.n, foe_hp, g_ar_own.hp, ra, rp);
            else ar_log(idx, now, "the attack time: the force of %d waits at %d,%d - no enemy building known", n, a->home.x, a->home.y);
            return;
        }
        if (press) a->presses++; else a->pushes++;
        if (routed) {
            a->routed++; a->round_ways += a->nway > 0;
            ar_log(idx, now, "the way to %d,%d: %d corners, danger on it %d (the straight line %d), walls on it %d%s", d.x, d.y, a->nway, way_d, line_d, a->way_walls, a->breach ? " - no way round: a breach" : "");
        }
        ar_log(idx, now, "%s: the force of %d from %d,%d at %d,%d (%.2f: own %d, enemy %d; assumed hp %.0f against own %.0f, %.2f)", press ? "pressing on" : "the attack time", n, a->home.x, a->home.y, d.x, d.y, r, g_ar_own.n, g_sgs_foe.n, foe_hp, g_ar_own.hp, ra);
        ar_go(pl, idx, a, 1, d, t, 0, now);
    }
}
internal int g_ar_load;          // the game came from a save: 1 without the force, 2 with it (pointers to find again)
internal unsigned g_ar_loaded;   // players whose first tick after the load has run
internal void ar_found_again(struct armies_st *a) {
    int m = 0;
    for (int k = 0; k < a->n; k++) if ((a->held[k].u = unit_by_id((unsigned)a->held[k].id))) a->held[m++] = a->held[k];
    a->n = m;
    a->target = a->target_id ? unit_by_id((unsigned)a->target_id) : NULL;
}
// a soldier of the force standing at home handed over (a town's garrison, towns.c): it stays held, out of the force
// HIT AND RUN: THE RAIDERS (the player 2026-11-08: "trained horses per game should easily reach 100+"). Cavalry at home was short nearly every time a raid had a target (28 of 30 looks a game against the
// game's AI, 47 of 52 head-to-head, with 78 and 282 horses trained a game: they left with the force). AR_GROUP of them stay out of the force, kept up by ar_det_train once the economy allows it and
// handed back when enemies are in the base. Not saved: a loaded game's are found by the force again (ar_adopt).
#define AR_GROUP 16
internal struct ar_group { ee_unit *u[AR_GROUP]; int id[AR_GROUP], n; uint32_t t; } g_ar_group[16];
internal int ar_group_has(unsigned idx, ee_unit *u) {
    struct ar_group *g = &g_ar_group[idx & 15];
    for (int k = 0; k < g->n; k++) if (g->u[k] == u) return 1;
    return 0;
}
internal int ar_take(ee_player *pl, ee_unit *u) {
    struct armies_st *a = &g_armies[(unsigned)player_get_index(pl) & 15];
    for (int k = 0; k < a->n; k++) {
        if (a->held[k].u != u || a->held[k].id != unit_get_serial(u)) continue;
        if (a->held[k].out || a->held[k].back) return 0;
        memmove(&a->held[k], &a->held[k + 1], (size_t)(a->n - k - 1) * sizeof a->held[0]);
        a->n--;
        return 1;
    }
    struct ar_group *g = &g_ar_group[(unsigned)player_get_index(pl) & 15];
    for (int k = 0; k < g->n; k++) {
        if (g->u[k] != u || g->id[k] != unit_get_serial(u)) continue;
        g->u[k] = g->u[g->n - 1]; g->id[k] = g->id[g->n - 1]; g->n--;
        return 1;
    }
    return 0;
}
// OVER THE SEA: the force's soldiers at home a landing may take (landing.c), at most max; none while the force is out
internal int armies_home_soldiers(ee_player *pl, unsigned idx, ee_unit **out, int max) {
    struct armies_st *a = &g_armies[idx & 15];
    if (!ar_on(pl, idx) || a->out) return 0;
    int n = 0;
    for (int k = 0; k < a->n && n < max; k++) {
        ee_unit *u = a->held[k].u;
        if (!a->held[k].back && cv_alive(u) && unit_get_x(u) >= 0 && ivec2_dist2(unit_get_tile(u), a->home) <= 4 * AR_HOME_R * AR_HOME_R) out[n++] = u;
    }
    return n;
}
// ... one taken by the landing: out of the force (still held) until the landing lets it go (armies_tick)
internal void armies_lend(ee_player *pl, ee_unit *u, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct armies_st *a = &g_armies[idx];
    for (int k = 0; k < a->n; k++) {
        if (a->held[k].u != u) continue;
        a->held[k] = a->held[--a->n];
        struct av_army *l = av_of(idx, AV_LANDING);   // a split: the landing's army, made with its first soldier
        if (!l) l = av_new(idx, AV_LANDING, now);
        if (l) av_put(l, u);
        g_ar_lent[idx].lent++;
        return;
    }
}
// the lent the landing has let go: back into the force on its land; those left on other land stay held where they are
internal void ar_lent_back(ee_player *pl, unsigned idx, struct armies_st *a) {
    struct av_army *l = av_of(idx, AV_LANDING);   // a merge: the landing's army back into the force, ended once empty
    if (!l) return;
    unsigned land = map_area(a->home, 2);
    int m = 0;
    for (int k = 0; k < l->n; k++) {
        ee_unit *u = l->m[k].u;
        if (!av_live(&l->m[k], pl)) continue;
        if (landing_has_unit(pl, u) || unit_get_x(u) < 0 || map_area(unit_get_tile(u), 2) != land || a->n >= AR_HELD) { l->m[m++] = l->m[k]; continue; }
        a->held[a->n].u = u; a->held[a->n].id = unit_get_serial(u); a->held[a->n].order_t = 0; a->held[a->n].out = 0; a->held[a->n].back = 0; a->held[a->n].pick = 0; a->held[a->n].hp = -1; a->n++;
        g_ar_lent[idx].back++;
    }
    l->n = m;
    if (!m) av_end(idx, l);
}
internal void ar_adopt(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    if (!ar_on(pl, idx)) return;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && a->n < AR_HELD; ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td) || !unit_get_ai_held(u) || ar_held(a, u) || landing_has_unit(pl, u)) continue;
        a->held[a->n].u = u; a->held[a->n].id = unit_get_serial(u); a->held[a->n].order_t = 0; a->held[a->n].out = 0; a->held[a->n].back = 0; a->held[a->n].pick = 0; a->held[a->n].hp = -1; a->n++;
        a->held_all++; n++;
    }
    if (n || g_ar_load != 2) ar_log(idx, now, "after the load: %d members found again, %d other held soldiers taken in", a->n - n, n);
}
// MINE RAIDS ([ai_base] towns mine_raids; the player 2026-11-08: "small detachments that stay until the mine is secured"): the force
// goes out 2-3.5 times a game, too rarely to take mines (0.3-0.7 taken a game with take_mines). Every AR_DET_LOOK, none out and
// AR_DET_PAUSE after the last: AR_DET_SIZE soldiers (half of those at home at most, AR_DET_MIN at home needed) leave the force for
// the enemy drop site by a mine it knows on its own land that they beat by AR_GO_PCT, the nearest such (stage_foe: only what it
// has seen). Six citizens behind them (towns.c THE CREW). The site down: what else it sees within 22 tiles, then tn_claim; they
// stand over the town until tn_claim_secured, AR_DET_HOLD at most, and go back to the force. They turn back as the force does
// (AR_JUDGE: before contact only on a clear deficit, in contact when the fight itself goes badly - turned back on the estimate
// alone, 99 of 120 went home after 2.8 minutes, 5 of 19 soldiers lost, nothing done). Their orders only from here (tn_garrison).
// HIT AND RUN (the player 2026-11-08: "especially when behind raiding mines gets important, ideally taking them, but just going in
// killing a few citizens and getting out again is worth much already"): when its army is outmatched, too few are at home for a
// detachment or it would beat no mine: AR_HIT_MIN to AR_HIT_SIZE cavalry (the player: "cavalry archers specially, you need fast
// units for this"; 6-8 soldiers of any kind killed 0.7 citizens a raid: ar_fast ranks them, ar_det_train keeps AR_HIT_SIZE at home) ride to
// the least defended mine it knows that no tower it knows covers (a covered one is asked of the army: site_defended), not hit in the
// last 6 minutes, no citizens along; there each attacks the nearest enemy citizen it
// sees. They leave after AR_HIT_TIME, with nobody left to kill for 20 s, once AR_HIT_LOSS % of their hit points are gone or as many
// enemy fighters as they are stand within 10 tiles.
#define AR_DET_SIZE 20
#define AR_DET_LOOK 15000
#define AR_DET_HOLD 900000
#define AR_DET_TARGETS 32
#define AR_HIT_SIZE 12
#define AR_HIT_MIN 10
#define AR_HIT_ECON 50             // HIT AND RUN: the share of the citizens planned (%) before the first horse, unless the AI is outmatched
#define AR_HIT_PAUSE 90000
#define AR_HIT_TIME 90000
#define AR_HIT_LOSS 40
#define AR_HIT_NEAR 6              // HIT AND RUN: at the mine itself (14 tiles short of the drop site 11 riders saw 1 citizen with 6 there, 5 of them gathering)
#define AR_HIT_NEXT 8000           // nobody in sight this long: on to the next mine within 12 tiles of the first
#define AR_HIT_MINES 4
#define AR_HIT_R 16
#define AR_DET_TRADE_FLOOR 50   // in contact it turns back on a bad trade only after losing this % of its hit points (the player: "the gold the ai gets and the enemy loses per minute is worth way more than a few unit loses")
internal struct ar_det {
    int state;   // 0 none, 1 out, 2 holding its town
    int mode;    // 0 to take the mine, 1 a hit and run
    ivec2 vis[AR_HIT_MINES]; int nvis;   // HIT AND RUN: the mines it has ridden to
    uint32_t hit_t, nocit_t, since; int tgt[32], ntgt, seen_max;   // HIT AND RUN: there since, citizens last seen, sent, the citizens attacked, most in sight at once
    int n, id[AR_DET_SIZE]; ee_unit *u[AR_DET_SIZE];   // id: unit_get_serial (unit_by_id's key)
    ivec2 dest, site, origin; int site_set, target_id, weak;   // origin: the mine it was sent for (THE SITES' MEMORY)
    int hp[AR_DET_SIZE], nfoe, trade_n; float dead_hp, out_hp, dealt[AR_TRADE_N], lost[AR_TRADE_N];   // the trade, as the force's (ar_trade)
    struct { int id, hp; } foe[128];
    uint32_t t, last, judge_t, order_t, claim_t;
    unsigned sent, cleared, founded, secured, losing, no_town, timed_out, killed, trained;
    uint32_t train_t, stable_t;   // HIT AND RUN: cavalry last ordered, a Stable last queued
} g_ar_det[16];
internal int ar_det_has(unsigned idx, ee_unit *u) {
    struct ar_det *d = &g_ar_det[idx & 15];
    for (int k = 0; d->state && k < d->n; k++) if (d->u[k] == u) return 1;
    return 0;
}
internal void ar_det_order(struct ar_det *d, int idle_only, int fight) {
    g_sg_order = 1;
    for (int k = 0; k < d->n; k++) if (!idle_only || ar_idle(d->u[k])) unit_goal(d->u[k], ar_goal_at(d->dest, k, fight), 0, 0x1F40);
    g_sg_order = 0;
}
// over: its soldiers back to the force (free again: the next tick holds them), the citizens not yet used home
internal int ar_det_out(unsigned idx) { return g_ar_det[idx & 15].state != 0; }
internal void ar_det_end(ee_player *pl, unsigned idx, struct ar_det *d, uint32_t now, const char *why, int failed) {
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    if (failed && !d->mode) site_failed(idx, d->origin, g_sgs_foe.hp, now);
    if (d->mode) {   // out at once, a plain move: they don't stop to fight on the way
        ivec2 h = g_armies[idx].home;
        g_sg_order = 1;
        for (int k = 0; k < d->n; k++) unit_goal(d->u[k], ar_goal_at(h, k, 0), 0, 0x1F40);
        g_sg_order = 0;
        site_hit(idx, d->origin, now);
    }
    ar_log(idx, now, "the detachment at %d,%d (%d left) - %s", d->dest.x, d->dest.y, d->n, why);
    for (int k = 0; k < d->n; k++) { unit_set_ai_held(d->u[k], 0); if (cm) combat_mgr_register_unit(cm, d->u[k]); }
    tn_crew_release(pl, idx);
    d->state = 0; d->n = 0; d->site_set = 0; d->last = now ? now : 1;
}
// the fight since the last judgement, as ar_trade: the hit points its members lost and those it took off the enemy seen round its middle; members fighting
internal int ar_det_trade(ee_player *pl, struct ar_det *d, ivec2 mid, float *dealt, float *lost) {
    int fighting = 0;
    *dealt = 0.0f; *lost = d->dead_hp; d->dead_hp = 0.0f;
    for (int k = 0; k < d->n; k++) {
        int hp = unit_get_hp(d->u[k]);
        if (d->hp[k] > hp) *lost += (float)(d->hp[k] - hp);
        d->hp[k] = hp;
        ee_goal *g = unit_get_goal(d->u[k]);
        fighting += g && goal_is_attack(g);
    }
    for (int i = 0; i < d->nfoe; i++) {
        ee_unit *u = unit_by_id((unsigned)d->foe[i].id);
        int hp = u && unit_get_serial(u) == d->foe[i].id && cv_alive(u) ? unit_get_hp(u) : 0;
        if (d->foe[i].hp > hp) *dealt += (float)(d->foe[i].hp - hp);
    }
    d->nfoe = 0;
    for (struct grid_it it = grid_it_box(mid.x, mid.y, SG_FOE, 1); grid_it_next(&it) && d->nfoe < 128; ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !seen_by(u, pl) || ivec2_dist2(unit_get_tile(u), mid) > SG_FOE * SG_FOE) continue;
        if (!cv_mil(it.td) && !fr_building(it.td)) continue;
        d->foe[d->nfoe].id = unit_get_serial(u); d->foe[d->nfoe].hp = unit_get_hp(u); d->nfoe++;
    }
    return fighting;
}
// HIT AND RUN: there - each soldier on the nearest enemy citizen it sees; out when it is over
internal void ar_det_hit(ee_player *pl, unsigned idx, struct ar_det *d, ivec2 mid, int m, uint32_t now) {
    const char *why = NULL;
    int there = ivec2_dist2(mid, d->dest) <= AR_HIT_NEAR * AR_HIT_NEAR, nc = 0;
    static ee_unit *cit[64];
    for (struct grid_it it = grid_it_box(mid.x, mid.y, AR_HIT_R, 1); grid_it_next(&it) && nc < 64; ) {
        ee_player *o = unit_get_owner(it.u);
        if (o && player_get_index(o) > 0 && is_enemy(pl, o) && cv_alive(it.u) && seen_by(it.u, pl) && is_citizen_type(unit_type_get_id(it.td)) && ivec2_dist2(unit_get_tile(it.u), mid) <= AR_HIT_R * AR_HIT_R) cit[nc++] = it.u;
    }
    if (g_ar_own.hp * 100.0f < d->out_hp * (float)(100 - AR_HIT_LOSS)) why = "hurt";
    else if (!there && !nc) {
        if (now - d->since >= 360000) why = "never got there";
        else { if (!d->order_t || now - d->order_t >= 20000) { d->order_t = now ? now : 1; ar_det_order(d, 1, 0); } return; }
    }
    else {
        if (!d->hit_t) {
            d->hit_t = now ? now : 1; d->nocit_t = d->hit_t; d->seen_max = 0;
            if (g_autotest && g_log) {   // DIAG: what a raid finds - the real count beside what it sees (the log only: no decision reads it)
                int all = 0, gath = 0;
                for (struct grid_it it = grid_it_box(d->dest.x, d->dest.y, AR_HIT_R, 1); grid_it_next(&it); ) {
                    ee_player *o = unit_get_owner(it.u);
                    if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(it.u) || !is_citizen_type(unit_type_get_id(it.td)) || ivec2_dist2(unit_get_tile(it.u), d->dest) > AR_HIT_R * AR_HIT_R) continue;
                    ee_goal *g = unit_get_goal(it.u);
                    all++; gath += g && !strcmp(rtti_name(g), "EEUGGather@@");
                }
                ar_log(idx, now, "hit and run: there, %d riders at %d,%d, mine %d,%d: %d citizens in sight (really %d within %d tiles of it, %d gathering), %d mines within 12, %d enemy fighters within 10", m, mid.x, mid.y, d->dest.x, d->dest.y, nc, all, AR_HIT_R, gath, tn_mine_near(pl, d->dest, 12), ar_enemy_band(pl, mid, 10));
            }
        }
        if (nc > d->seen_max) d->seen_max = nc;
        if (!nc && there && now - d->nocit_t >= AR_HIT_NEXT && d->nvis < AR_HIT_MINES) {   // on to the next mine
            ivec2 nx;
            if (tn_mine_next(pl, d->origin, 12, d->vis, d->nvis, &nx)) {
                d->vis[d->nvis++] = nx; d->dest = nx; d->nocit_t = now ? now : 1; d->order_t = now ? now : 1;
                ar_det_order(d, 0, 0);
                return;
            }
        }
        if (nc) d->nocit_t = now ? now : 1;
        if (now - d->hit_t >= AR_HIT_TIME) why = "time is up";
        else if (now - d->nocit_t >= 20000) why = "nobody left to kill";
        else if (ar_enemy_band(pl, mid, 10) >= m) why = "soldiers coming";
    }
    if (why) {
        int killed = 0;
        for (int k = 0; k < d->ntgt; k++) { ee_unit *u = unit_by_id((unsigned)d->tgt[k]); killed += !u || unit_get_serial(u) != d->tgt[k] || !cv_alive(u); }
        char msg[64];
        snprintf(msg, sizeof msg, "hit and run, %d citizens killed - %s", killed, why);
        d->killed += (unsigned)killed;
        ar_log(idx, now, "hit and run over: %d citizens attacked, %d killed, %d in sight at most, %.0f s there", d->ntgt, killed, d->seen_max, d->hit_t ? (now - d->hit_t) / 1000.0 : 0.0);
        ar_det_end(pl, idx, d, now, msg, 0);
        return;
    }
    g_sg_order = 1;
    for (int k = 0; k < d->n && nc; k++) {
        ee_unit *u = d->u[k];
        ee_goal *g = unit_get_goal(u);
        if (g && goal_is_attack(g)) continue;
        int bj = 0, bd = 0x7FFFFFFF;
        for (int j = 0; j < nc; j++) { int q = ivec2_dist2(unit_get_tile(cit[j]), unit_get_tile(u)); if (q < bd) { bd = q; bj = j; } }
        if (unit_goal(u, goal_attack(u, cit[bj]), 0, 0x1F40)) {
            int id = unit_get_serial(cit[bj]), seen = 0;
            for (int j = 0; j < d->ntgt && !seen; j++) seen = d->tgt[j] == id;
            if (!seen && d->ntgt < 32) d->tgt[d->ntgt++] = id;
        }
    }
    g_sg_order = 0;
}
// HIT AND RUN: how fit a type is for a raid - mounted archers 3, cavalry with guns 2, other cavalry 1, the rest 0
internal int ar_fast(ee_unit_type *td) {
    int f = td ? unit_type_get_family(td) : 0;
    return f == KIND_CAVALRY_ARCHER || f == KIND_MOUNTED_ARCHER ? 3 : f == KIND_CURIASSIER ? 2 : f == KIND_LANCER || f == KIND_MOUNTED_SPEAR ? 1 : 0;
}
// HIT AND RUN: the economy first (the player 2026-11-08: "like always, the ai getting its economy on track is most important, until i have atleast 2 farms i dont
// build more than one of the prodcution building"). As a priority order from minute 3 the horses took a quarter of the income to minute 20 and all of its gold, 10 of them
// standing at home until the first raid at minute 22, and the fields and with them the citizens fell behind (46.9 -> 43.5 at minute 20, 103.7 -> 90.8 at 40).
// Two farms, none of it while the economy recovers; then the citizens on track, or the AI outmatched, when raids matter most (the player: "espacialy when behind").
internal int ar_det_econ(ee_player *pl, unsigned idx, ee_unit_mgr *um) {
    void *fq = type_list(um, UT(B_FARM));
    if ((fq ? deque_count(fq) : 0) < 2 || recovering(pl)) return 0;
    int goal = g_czp[idx].goal ? g_czp[idx].goal : unit_mgr_citizens_wanted(um);
    if (unit_mgr_citizens(um) * 100 >= goal * AR_HIT_ECON) return 1;
    float mine = 0.0f, theirs = 0.0f;
    return ex_outmatched(pl, &mine, &theirs);
}
// HIT AND RUN: cavalry at home kept up, once the economy allows it: the best kind it can train, two every 30 s, out of the army's account like any soldier; nothing trains: a Stable queued (5 minutes apart)
internal void ar_det_train(ee_player *pl, unsigned idx, struct ar_det *d, int have, uint32_t now) {
    if (have >= AR_GROUP || (d->train_t && now - d->train_t < 30000)) return;
    d->train_t = now ? now : 1;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!um || !ar_det_econ(pl, idx, um)) return;
    int n = AR_GROUP - have > 2 ? 2 : AR_GROUP - have;
    for (int rank = 3; rank >= 1; rank--) {
        int best = 0, bt = 0;
        for (int t = 0; t < G_TYPES_N; t++) {
            ee_unit_type *td = type_data(t);
            if (!td || ar_fast(td) != rank || !ld_type_avail(pl, t)) continue;
            if (unit_type_get_tech(td) > bt) { bt = unit_type_get_tech(td); best = t; }
        }
        if (!best) continue;
        for (int r = 2; r < 7; r++) { int c = unit_cost(pl, best, r); if (c > 0 && (int)g_bud[idx].mil[r] / c < n) n = (int)g_bud[idx].mil[r] / c; }
        if (n <= 0) return;   // the account can't pay: it waits, not a worse kind
        int req[4] = { best, n, 0, 0 };
        unsigned got = 0;
        g_priority_mil = 1;
        hook_train_units(um, req, (uintptr_t)&got);
        g_priority_mil = 0;
        if (got) { d->trained += got; ar_log(idx, now, "hit and run: trains %u x type %d (%d cavalry at home)", got, best, have); return; }
    }
    int e = current_epoch(pl);
    if (bm && e >= 0 && e < 14 && ut_has(UT(B_STABLE)) && can_build(pl, UT(B_STABLE)) && !bq_has(bm, UT(B_STABLE)) && (!d->stable_t || now - d->stable_t >= 300000)) {
        d->stable_t = now ? now : 1;
        int entry[3] = { UT(B_STABLE), 0, 0 };
        build_mgr_queue_add(bm, e, entry);
    }
}
// HIT AND RUN: THE RAIDERS kept up: the dead dropped, enemies in the base: back to the force; every 15 s cavalry at home taken out of the force, and the next ones ordered
internal void ar_group_tick(ee_player *pl, unsigned idx, struct armies_st *a, struct ar_det *d, uint32_t now) {
    struct ar_group *g = &g_ar_group[idx];
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    int m = 0;
    for (int k = 0; k < g->n; k++) if (unit_get_serial(g->u[k]) == g->id[k] && unit_get_owner(g->u[k]) == pl && cv_alive(g->u[k])) { if (!unit_get_ai_held(g->u[k])) player_hold_unit(pl, g->u[k]); g->u[m] = g->u[k]; g->id[m++] = g->id[k]; }
    g->n = m;
    if (base_threatened(idx)) {
        for (int k = 0; k < g->n; k++) { unit_set_ai_held(g->u[k], 0); if (cm) combat_mgr_register_unit(cm, g->u[k]); }
        g->n = 0;
        return;
    }
    if (g->t && now - g->t < AR_DET_LOOK && now >= g->t) return;
    g->t = now ? now : 1;
    ee_civ_mgr *cvm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cvm ? civ_mgr_get_unit_mgr(cvm) : NULL;
    if (!um || !ar_det_econ(pl, idx, um)) return;
    for (int k = a->n - 1; k >= 0 && g->n < AR_GROUP; k--) {
        ee_unit *u = a->held[k].u;
        if (a->held[k].out || a->held[k].back || !cv_alive(u) || !ar_fast(unit_get_type_data(u)) || !ar_take(pl, u)) continue;
        g->u[g->n] = u; g->id[g->n] = unit_get_serial(u);
        g_sg_order = 1;
        unit_goal(u, ar_goal_at(a->home, g->n, 0), 0, 0x1F40);
        g_sg_order = 0;
        g->n++;
    }
    ar_det_train(pl, idx, d, g->n + (d->state && d->mode ? d->n : 0), now);
}
// HIT AND RUN: THE RIDERS' LOOK. A mine is only known if someone has seen it (the player: "no cheating"): of 6-19 enemy drop sites standing the AI had seen 1-3, and half to three quarters of its looks for
// a target found none. A rider alone, the fastest cavalry at home, goes to the land not looked at for 10 minutes that lies nearest to the enemy buildings it knows (at first nearest to its own base); what it
// sees goes into the memory as anything seen. Two out at a time, a new one a minute, while fewer than AR_LOOK_ENOUGH mines to raid are known. It doesn't run from anything: on to the next place, until killed
// or its time is up - a horse for a look.
#define AR_LOOK_N 2
#define AR_LOOK_EVERY 60000
#define AR_LOOK_STALE 600000u
#define AR_LOOK_CELL 12
#define AR_LOOK_ENOUGH 3
#define AR_LOOK_LIFE 480000
#define AR_LOOK_LEG 120000         // a leg it doesn't finish: the next place
internal struct ar_look { ee_unit *u[AR_LOOK_N]; int id[AR_LOOK_N]; ivec2 to[AR_LOOK_N]; uint32_t t[AR_LOOK_N], leg[AR_LOOK_N], last; int known; unsigned sent, died, home; } g_ar_look[16];
internal int ar_look_has(unsigned idx, ee_unit *u) { struct ar_look *l = &g_ar_look[idx & 15]; for (int k = 0; k < AR_LOOK_N; k++) if (l->u[k] == u) return 1; return 0; }
internal int ar_look_cell(ee_player *pl, unsigned idx, struct ar_look *l, int self, ivec2 home, ivec2 *o, uint32_t stale) {   // stale: not looked at for that long
    static ivec2 foe[32];
    ee_map *map = game_map();
    int nf = 0, size = map ? (int)map_get_size(map) : 0, best = 0x7FFFFFFF;
    unsigned land = (unsigned)area_id(home, 2);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && nf < 32; ) {
        ee_player *ow = unit_get_owner(it.u);
        int fam = unit_type_get_class_id(it.td);
        if (ow && player_get_index(ow) > 0 && is_enemy(pl, ow) && (fam == CLS_BUILDING || fam == CLS_FORTRESS) && cv_alive(it.u) && seen_by(it.u, pl)) foe[nf++] = unit_get_tile(it.u);
    }
    for (int y = AR_LOOK_CELL / 2; y < size; y += AR_LOOK_CELL)
        for (int x = AR_LOOK_CELL / 2; x < size; x += AR_LOOK_CELL) {
            ivec2 p = iv2(x, y);
            if (intel_looked_ago(pl, x, y) <= stale ||(unsigned)area_id(p, 2) != land || threat_at(idx, p) > 0) continue;
            int skip = 0;
            for (int k = 0; k < AR_LOOK_N && !skip; k++) skip = k != self && l->u[k] && ivec2_dist2(p, l->to[k]) < 16 * 16;
            if (skip) continue;
            int d = nf ? 0x7FFFFFFF : ivec2_dist2(p, home);
            for (int f = 0; f < nf; f++) if (ivec2_dist2(p, foe[f]) < d) d = ivec2_dist2(p, foe[f]);
            if (d < best) { best = d; *o = p; }
        }
    return best != 0x7FFFFFFF;
}
internal void ar_look_order(ee_unit *u, ivec2 to) {
    g_sg_order = 1;
    unit_goal(u, goal_move((float)to.x + 0.5f, (float)to.y + 0.5f), 0, 0x1F40);
    g_sg_order = 0;
}
internal void ar_look_tick(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    struct ar_look *l = &g_ar_look[idx];
    if (now < l->last) memset(l, 0, sizeof *l);
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    int busy = 0;
    for (int k = 0; k < AR_LOOK_N; k++) {
        ee_unit *u = l->u[k];
        if (!u) continue;
        if (unit_get_serial(u) != l->id[k] || unit_get_owner(u) != pl || !cv_alive(u)) { l->u[k] = NULL; l->died++; continue; }
        busy++;
        if (ivec2_dist2(unit_get_tile(u), l->to[k]) > 16 && now - l->leg[k] < AR_LOOK_LEG) continue;
        ivec2 nx;
        if (now - l->t[k] < AR_LOOK_LIFE && ar_look_cell(pl, idx, l, k, a->home, &nx, AR_LOOK_STALE)) { l->to[k] = nx; l->leg[k] = now ? now : 1; ar_look_order(u, nx); continue; }
        ar_look_order(u, a->home);   // its time is up: home, the force's again
        unit_set_ai_held(u, 0);
        if (cm) combat_mgr_register_unit(cm, u);
        l->u[k] = NULL; l->home++; busy--;
    }
    if (l->known >= AR_LOOK_ENOUGH || busy >= AR_LOOK_N || (l->last && now - l->last < AR_LOOK_EVERY)) return;
    ee_civ_mgr *cvm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cvm ? civ_mgr_get_unit_mgr(cvm) : NULL;
    if (!um || base_threatened(idx) || !ar_det_econ(pl, idx, um)) return;
    l->last = now ? now : 1;
    ee_unit *sc = NULL;
    float fast = 0.0f;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (a->held[k].out || a->held[k].back || !cv_alive(u) || !ar_fast(unit_get_type_data(u))) continue;
        if (st_speed(u) > fast) { fast = st_speed(u); sc = u; }
    }
    for (int k = 0; k < g_ar_group[idx].n; k++) {   // the raiders are where the cavalry is
        ee_unit *u = g_ar_group[idx].u[k];
        if (cv_alive(u) && st_speed(u) > fast) { fast = st_speed(u); sc = u; }
    }
    int free_slot = 0;
    while (free_slot < AR_LOOK_N && l->u[free_slot]) free_slot++;
    ivec2 to;
    if (!sc || free_slot == AR_LOOK_N || !ar_look_cell(pl, idx, l, free_slot, a->home, &to, AR_LOOK_STALE) || !ar_take(pl, sc)) return;
    l->u[free_slot] = sc; l->id[free_slot] = unit_get_serial(sc); l->to[free_slot] = to; l->t[free_slot] = l->leg[free_slot] = now ? now : 1; l->sent++;
    ar_look_order(sc, to);
    ar_log(idx, now, "a rider looks at %d,%d (%d mines to raid known; %u sent, %u killed, %u home so far)", to.x, to.y, l->known, l->sent, l->died, l->home);
}
// SCOUTING ([ai_base] scouting, on since 2026-11-11 - measured with training by need, which it feeds (TRAINING BY NEED); [autotest] v_scouting; army view step 6 - the AI knew 1-9 of the
// enemy's units at most looks: the game's explorers are off for it (docs/game-ai-routines.md row 43) and it saw the enemy
// only where its army went). When it hasn't seen an enemy land army for AR_SCOUT_STALE (or never), every AR_SCOUT_EVERY at
// most and not while its base is threatened or the force is out: the fastest soldier of the force at home looks at the
// land nearest the enemy buildings it knows that it hasn't looked at for AR_SCOUT_STALE (the riders' look, ar_look_cell:
// not into a known threat), leg after leg until AR_SCOUT_LIFE, then home to the force. What it sees goes into the memory
// - the enemy's armies, what it fields, what it saw of the scout (army_view.c).
#define AR_SCOUT_EVERY 180000
#define AR_SCOUT_STALE 180000u
#define AR_SCOUT_LIFE 240000
internal int g_ar_scout_on = 1, g_v_ar_scout = -1;
#define AR_SCOUT(idx) PV(idx, g_v_ar_scout, g_ar_scout_on)
internal struct ar_look g_ar_scout[16];
internal void ar_scout_tick(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    if (!AR_SCOUT(idx)) return;
    struct ar_look *l = &g_ar_scout[idx & 15];
    if (now < l->last) memset(l, 0, sizeof *l);
    ee_unit *u = l->u[0];
    if (u) {
        if (unit_get_serial(u) != l->id[0] || unit_get_owner(u) != pl || !cv_alive(u)) { l->u[0] = NULL; l->died++; return; }
        if (ivec2_dist2(unit_get_tile(u), l->to[0]) > 16 && now - l->leg[0] < AR_LOOK_LEG) return;
        ivec2 nx;
        if (now - l->t[0] < AR_SCOUT_LIFE && ar_look_cell(pl, idx, l, 0, a->home, &nx, AR_SCOUT_STALE)) { l->to[0] = nx; l->leg[0] = now ? now : 1; ar_look_order(u, nx); return; }
        ar_look_order(u, a->home);   // its time is up: home, the force's again (as the riders' look)
        unit_set_ai_held(u, 0);
        ee_combat_mgr *cm = player_get_combat_mgr(pl);
        if (cm) combat_mgr_register_unit(cm, u);
        l->u[0] = NULL; l->home++;
        return;
    }
    if ((l->last && now - l->last < AR_SCOUT_EVERY) || base_threatened(idx) || a->out) return;
    av_enemy_build(pl, now);
    uint32_t seen = 0;
    for (int j = 0; j < g_ave[idx].n; j++) if (g_ave[idx].a[j].field == AVF_LAND && g_ave[idx].a[j].seen_t > seen) seen = g_ave[idx].a[j].seen_t;
    if (seen && now >= seen && now - seen < AR_SCOUT_STALE) return;
    l->last = now ? now : 1;
    ee_unit *sc = NULL;
    float fast = 0.0f;
    for (int k = 0; k < a->n; k++) {   // the fastest at home - a rider when it has one, else whoever is quickest (no siege)
        ee_unit *v = a->held[k].u;
        ee_unit_type *td = cv_alive(v) ? unit_get_type_data(v) : NULL;
        if (a->held[k].out || a->held[k].back || !td || is_siege(unit_type_get_id(td))) continue;
        if (st_speed(v) > fast) { fast = st_speed(v); sc = v; }
    }
    ivec2 to;
    if (!sc || !ar_look_cell(pl, idx, l, 0, a->home, &to, AR_SCOUT_STALE) || !ar_take(pl, sc)) return;
    l->u[0] = sc; l->id[0] = unit_get_serial(sc); l->to[0] = to; l->t[0] = l->leg[0] = now ? now : 1; l->sent++;
    ar_look_order(sc, to);
    if (seen) ar_log(idx, now, "a scout looks at %d,%d (an enemy land army last seen %.1f min ago; %u sent, %u killed, %u home so far)", to.x, to.y,
                     (now - seen) / 60000.0, l->sent, l->died, l->home);
    else ar_log(idx, now, "a scout looks at %d,%d (no enemy land army in its memory; %u sent, %u killed, %u home so far)", to.x, to.y, l->sent, l->died, l->home);
}
internal void ar_det_start(ee_player *pl, unsigned idx, struct armies_st *a, struct ar_det *d, uint32_t now) {
    static ee_unit *home[AR_HELD];
    int nh = 0, ncav = 0;
    for (int k = 0; k < a->n; k++) if (!a->held[k].out && !a->held[k].back && cv_alive(a->held[k].u)) { home[nh++] = a->held[k].u; ncav += ar_fast(unit_get_type_data(a->held[k].u)) > 0; }
    for (int k = 0; k < g_ar_group[idx].n && nh < AR_HELD; k++) { home[nh++] = g_ar_group[idx].u[k]; ncav++; }   // last: a detachment to take a mine takes the front of the list
    if (base_threatened(idx)) return;
    int size = nh / 2 < AR_DET_SIZE ? nh / 2 : AR_DET_SIZE;
    float mine = 0.0f, theirs = 0.0f;
    int behind = ex_outmatched(pl, &mine, &theirs) || nh < AR_DET_MIN;
    unsigned land = (unsigned)area_id(a->home, 2);
    ee_unit *cand[AR_DET_TARGETS]; long cd[AR_DET_TARGETS];
    int nc = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !seen_by(u, pl) || (unsigned)area_id(unit_get_tile(u), 2) != land || !tn_mine_drop(pl, u, it.td)) continue;
        long dist = ivec2_dist2(unit_get_tile(u), a->home);
        if (nc == AR_DET_TARGETS && cd[nc - 1] <= dist) continue;
        int k = nc < AR_DET_TARGETS ? nc++ : AR_DET_TARGETS - 1;
        while (k > 0 && cd[k - 1] > dist) { cand[k] = cand[k - 1]; cd[k] = cd[k - 1]; k--; }
        cand[k] = u; cd[k] = dist;
    }
    g_ar_look[idx].known = nc;
    if (nh < AR_HIT_MIN) return;
    st_side_clear(&g_ar_own);
    for (int k = 0; k < size; k++) st_side_add(&g_ar_own, home[k]);
    ee_unit *t = NULL;
    float r = 0.0f;
    for (int i = 0; i < nc && !t && !behind; i++) {   // the nearest it beats
        if (!site_for_det(idx, unit_get_tile(cand[i]), now)) continue;
        stage_foe(pl, unit_get_tile(cand[i]));
        r = st_ratio(&g_ar_own, &g_sgs_foe);
        if (r * 100.0f >= (float)AR_GO_PCT) t = cand[i];
    }
    d->mode = !t;
    if (!t) {   // HIT AND RUN: the least defended of those no tower it knows covers, its cavalry
        float least = 0.0f;
        ee_unit *covered = NULL;
        for (int i = 0; i < nc; i++) {
            ivec2 p = unit_get_tile(cand[i]);
            if (!site_hit_ok(idx, p, now)) continue;
            if (route_covered(pl, -10000, 0, p.x, p.y)) { if (!covered) covered = cand[i]; continue; }
            stage_foe(pl, p);
            if (!t || g_sgs_foe.hp < least) { t = cand[i]; least = g_sgs_foe.hp; r = least; }
        }
        if (!t) { if (covered) site_defended(idx, unit_get_tile(covered), now); return; }
        if (ncav < AR_HIT_MIN) return;
        int nf = 0;   // the cavalry to the front, mounted archers first, then the fastest
        for (int j = 0; j < nh; j++) if (ar_fast(unit_get_type_data(home[j]))) { ee_unit *sw = home[nf]; home[nf++] = home[j]; home[j] = sw; }
        size = nf < AR_HIT_SIZE ? nf : AR_HIT_SIZE;
        for (int k = 0; k < size; k++) {
            int b = k;
            for (int j = k + 1; j < nf; j++) {
                int rj = ar_fast(unit_get_type_data(home[j])), rb = ar_fast(unit_get_type_data(home[b]));
                if (rj > rb || (rj == rb && st_speed(home[j]) > st_speed(home[b]))) b = j;
            }
            ee_unit *sw = home[k]; home[k] = home[b]; home[b] = sw;
        }
    }
    d->n = 0;
    d->out_hp = 0.0f; d->dead_hp = 0.0f; d->nfoe = 0; d->trade_n = 0;
    for (int k = 0; k < size; k++) if (ar_take(pl, home[k])) { d->hp[d->n] = unit_get_hp(home[k]); d->out_hp += (float)d->hp[d->n]; d->u[d->n] = home[k]; d->id[d->n++] = unit_get_serial(home[k]); }
    d->dest = d->origin = unit_get_tile(t); d->target_id = unit_get_serial(t);
    site_tried(idx, d->origin);
    d->state = 1; d->weak = 0; d->site_set = 0; d->judge_t = d->order_t = 0; d->sent++; d->hit_t = d->nocit_t = 0; d->ntgt = 0; d->since = now ? now : 1;
    d->nvis = 0;
    if (d->mode) { ivec2 nx; if (tn_mine_next(pl, d->origin, 12, d->vis, 0, &nx)) { d->vis[d->nvis++] = nx; d->dest = nx; } }   // HIT AND RUN: the mine itself
    if (!d->mode) tn_crew_start(pl, idx, a->home, now);
    ar_det_order(d, 0, !d->mode);
    ar_log(idx, now, "a detachment of %d for the mines at %d,%d (%s %.2f; %d at home, %d candidates)", d->n, d->dest.x, d->dest.y, d->mode ? "hit and run, enemy hp" : "to take it,", r, nh, nc);
}
internal void ar_det_tick(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    struct ar_det *d = &g_ar_det[idx];
    if (!TN_RAID(idx)) return;
    ar_group_tick(pl, idx, a, d, now);
    ar_look_tick(pl, idx, a, now);
    if (!d->state) {
        if ((d->last && now - d->last < (d->mode ? AR_HIT_PAUSE : AR_DET_PAUSE)) || (d->t && now - d->t < AR_DET_LOOK)) return;
        d->t = now ? now : 1;
        ar_det_start(pl, idx, a, d, now);
        return;
    }
    int m = 0;
    long sx = 0, sy = 0;
    st_side_clear(&g_ar_own);
    for (int k = 0; k < d->n; k++) {
        ee_unit *u = unit_by_id((unsigned)d->id[k]);
        if (!u || unit_get_serial(u) != d->id[k] || unit_get_owner(u) != pl || !cv_alive(u)) { d->dead_hp += (float)d->hp[k]; continue; }
        d->u[m] = u; d->hp[m] = d->hp[k]; d->id[m++] = d->id[k];
        sx += unit_get_x(u); sy += unit_get_y(u);
        st_side_add(&g_ar_own, u);
    }
    d->n = m;
    if (m < (d->mode ? 2 : 4)) { d->losing++; ar_det_end(pl, idx, d, now, "too few left", 1); return; }
    ivec2 mid = iv2((int)(sx / m), (int)(sy / m));
    if (d->mode) { ar_det_hit(pl, idx, d, mid, m, now); return; }
    if (d->state == 1) tn_crew_follow(pl, idx, mid, a->home, now);
    if (!d->judge_t || now - d->judge_t >= AR_JUDGE_EVERY) {
        d->judge_t = now ? now : 1;
        stage_foe(pl, mid);
        float r = st_ratio(&g_ar_own, &g_sgs_foe);
        stage_foe(pl, d->dest);
        float rd = st_ratio(&g_ar_own, &g_sgs_foe);
        if (rd < r) r = rd;
        float dealt, lost, sd = 0.0f, sl = 0.0f;
        int fighting = ar_det_trade(pl, d, mid, &dealt, &lost);
        if (fighting >= AR_CONTACT_MIN && fighting * 10 >= m) {   // the fight judges
            d->dealt[d->trade_n % AR_TRADE_N] = dealt; d->lost[d->trade_n % AR_TRADE_N] = lost; d->trade_n++;
            int tn = d->trade_n < AR_TRADE_N ? d->trade_n : AR_TRADE_N;
            for (int i = 0; i < tn; i++) { sd += d->dealt[i]; sl += d->lost[i]; }
            d->weak = 0;
            if (tn >= AR_TRADE_MIN && sl * 100.0f > sd * (float)AR_TRADE_BAD && sl * 100.0f >= d->out_hp * (float)AR_DET_TRADE_FLOOR) { d->losing++; ar_det_end(pl, idx, d, now, "trading badly - back", 1); return; }
        }
        else if (r * 100.0f < (float)AR_FAR_PCT) { if (++d->weak >= 2) { d->losing++; ar_det_end(pl, idx, d, now, "far too weak before contact - back", 1); return; } }
        else d->weak = 0;
    }
    if (d->state == 2) {
        int sec = tn_claim_secured(idx, d->site);
        if (sec) { d->secured += sec > 0; ar_det_end(pl, idx, d, now, sec > 0 ? "the mine is secured" : "its town is gone", sec < 0); return; }
        if (now - d->claim_t >= AR_DET_HOLD) { d->timed_out++; ar_det_end(pl, idx, d, now, "held 15 minutes - not secured", 0); return; }
        if (!d->order_t || now - d->order_t >= 20000) { d->order_t = now ? now : 1; ar_det_order(d, 1, 1); }
        return;
    }
    ee_unit *t = unit_by_id((unsigned)d->target_id);
    if (t && unit_get_serial(t) == d->target_id && cv_alive(t)) {
        if (!d->order_t || now - d->order_t >= 20000) { d->order_t = now ? now : 1; ar_det_order(d, 1, 1); }
        return;
    }
    if (!d->site_set && tn_mine_near(pl, d->dest, 12)) { d->site = d->dest; d->site_set = 1; }
    if (!d->site_set) { ar_det_end(pl, idx, d, now, "no mine there", 0); return; }
    ee_unit *nb = ar_near_building(pl, d->site, 22);
    if (nb) {
        stage_foe(pl, unit_get_tile(nb));
        float r = st_ratio(&g_ar_own, &g_sgs_foe);
        if (r * 100.0f < (float)AR_GO_PCT) { d->no_town++; ar_det_end(pl, idx, d, now, "what is left there is too strong", 1); return; }
        d->dest = unit_get_tile(nb); d->target_id = unit_get_serial(nb); d->cleared++; d->order_t = now ? now : 1;
        ar_det_order(d, 0, 1);
        return;
    }
    if (!tn_claim(pl, idx, d->site, now)) { d->no_town++; ar_det_end(pl, idx, d, now, "no town there", 0); return; }
    d->state = 2; d->dest = d->site; d->claim_t = now ? now : 1; d->founded++; d->order_t = 0;
    ar_log(idx, now, "the detachment of %d holds %d,%d for its town", d->n, d->site.x, d->site.y);
}
internal void armies_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct armies_st *a = &g_armies[idx];
    if (now < a->t) { memset(a, 0, sizeof *a); memset(&g_ar_lent[idx], 0, sizeof g_ar_lent[idx]); memset(&g_ar_pt[idx], 0, sizeof g_ar_pt[idx]); av_clear_player(idx); }   // a new game
    if (g_ar_load && !(g_ar_loaded & (1u << idx))) {   // the first tick after a load: the force found again, or the held taken in
        g_ar_loaded |= 1u << idx;
        if (g_ar_load == 2) ar_found_again(a);
        ar_adopt(pl, idx, a, now);   // the held it doesn't know (a save without the force, or one whose members aren't found again)
    }
    if (!ar_on(pl, idx) || (a->t && now - a->t < AR_TICK)) return;
    a->t = now ? now : 1;
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    if (!cm) return;
    // the held: the dead and the lost dropped
    int m = 0;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (unit_get_serial(u) == a->held[k].id && unit_get_owner(u) == pl && unit_get_hp(u) > 0) a->held[m++] = a->held[k];
    }
    a->n = m;
    ar_lent_back(pl, idx, a);
    // every free land soldier held - not a member of an attack subgroup (unit_free is no group test: a member passes
    // it, army.c)
    static ee_unit *mem[4096];
    int nm = ou_members(pl, mem, 4096);
    qsort(mem, (size_t)nm, sizeof *mem, ar_ptr_cmp);
    int kept_group = 0, kept_landing = 0, kept_sortie = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && a->n < AR_HELD; ) {
        ee_unit *u = it.u;
        // own_soldiers: siege weapons and rams out of the game's groups too - not the force's (attack.c siege_tick and
        // ar_rams lead every one of them), only out of the commander's reach (its groups ordered them 70 - 200 times a
        // game on land, 2026-11-13)
        if (OWN_SOLDIERS(idx) && unit_get_owner(u) == pl && (is_siege(unit_type_get_id(it.td)) || is_ram(unit_type_get_id(it.td))) &&
            !unit_get_ai_held(u) && unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && !landing_has_unit(pl, u)) { player_hold_unit(pl, u); continue; }
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td) || ar_held(a, u) || unit_get_ai_held(u) || ar_group_has(idx, u) || ar_det_has(idx, u) || ar_look_has(idx, u)) continue;   // the last two: the force took them in again though they are taken (the raiders marched off with it, in both lists)
        int is_free = combat_mgr_unit_free(cm, unit_get_physics(u)) != 0;
        // own_soldiers (econ.c EVERY UNIT THE MOD'S): a member of the game's groups is taken too - the hold
        // (unregister_unit) takes it out of them
        if (bsearch(&u, mem, (size_t)nm, sizeof *mem, ar_ptr_cmp) && !OWN_SOLDIERS(idx)) { kept_group += is_free; continue; }
        if (!is_free) continue;
        if (landing_has_unit(pl, u)) { kept_landing++; continue; }
        if (ou_sortie(pl, u)) { kept_sortie++; continue; }
        player_hold_unit(pl, u);
        a->held[a->n].u = u; a->held[a->n].id = unit_get_serial(u); a->held[a->n].order_t = 0; a->held[a->n].out = 0; a->held[a->n].back = 0; a->held[a->n].pick = 0; a->held[a->n].hp = -1; a->n++;
        a->held_all++;
    }
    // many free soldiers the force doesn't take (the player's game of 2026-11-09: P8's 233 stood idle in no one's army for
    // half an hour, a load freed them, the cause not known): what keeps them out, once a minute
    static uint32_t kept_t[16];
    if (now < kept_t[idx]) kept_t[idx] = 0;
    if (kept_group + kept_landing + kept_sortie >= 20 && now - kept_t[idx] >= 60000) {
        kept_t[idx] = now ? now : 1;
        ar_log(idx, now, "%d free soldiers kept out of the force: %d in the game's attack groups, %d on a landing, %d on an outdated units' sortie",
               kept_group + kept_landing + kept_sortie, kept_group, kept_landing, kept_sortie);
    }
    ar_window(pl, a, now);
    if (!ar_home(pl, idx, a, now)) return;
    ar_det_tick(pl, idx, a, now);
    ar_scout_tick(pl, idx, a, now);
    if (a->out) ar_out(pl, idx, a, now);
    ar_way_home(pl, idx, a, now);
    ar_at_home(pl, idx, a, now);
    ar_parts_tick(pl, idx, a, now);
    if (g_log && g_autotest && (!a->log_t || now < a->log_t || now - a->log_t >= 60000)) {
        a->log_t = now ? now : 1;
        ar_log(idx, now, "held %d (in all %u), %s%d,%d (judged %.2f), pushes %u (pressing on %u) legs %u raids %u backs %u waits %u, orders %u, offers refused %u, forted %u, on the way home lost %u of %u (hp %.0f of %.0f), stands %u, group moves %u (approaches %u, gathers %u), judgements in contact %u, homes by the trade %u, pushes routed %u (a way round %u), breaches %u (pushed %u, home %u, stand-offs back %u, looks %u), fire marks %u, next attack at %.1f min",
               a->n, a->held_all, a->out == 1 ? "out at " : a->out == 2 ? "at a raid at " : "home at ", a->out ? a->dest.x : a->home.x, a->out ? a->dest.y : a->home.y,
               a->ratio > 99.0f ? 99.0f : a->ratio, a->pushes, a->presses, a->legs, a->raids, a->backs, a->waits, a->orders, a->refused, a->forted, a->way_lost, a->way_n, a->way_hp_lost, a->way_hp, a->stands, a->group_moves, a->approaches, a->gathers, a->contacts, a->traded_home, a->routed, a->round_ways, a->breaches, a->breach_pushes, a->breach_homes, a->stand_backs, a->scouts, a->fires, player_get_next_attack(pl) / 60000.0);
        {   // the territory it is in and the one it is bound for (territories.c; read only so far)
            ivec2 tmid = a->home;   // its members' middle (not ar_side: that fills the judgements' side)
            int tn = 0;
            long tsx = 0, tsy = 0;
            for (int k = 0; k < a->n; k++) {
                if ((a->out && !a->held[k].out) || !cv_alive(a->held[k].u)) continue;
                tsx += unit_get_x(a->held[k].u); tsy += unit_get_y(a->held[k].u); tn++;
            }
            if (tn) tmid = iv2((int)(tsx / tn), (int)(tsy / tn));
            unsigned tin = tv_terr_of(tmid), tto = tv_terr_of(a->out ? a->dest : a->home);
            if (tn && tin != TER_NONE)
                ar_log(idx, now, "territory: the force of %d in %u, %s %u (%d enemy soldiers it knows there); raids answered in held ground out of its reach %u, looks its home was a territory's front %u", tn, tin, a->out == 1 ? "pushing into" : a->out == 2 ? "answering a raid in" : "its home in",
                       tto, tv_foes(idx, tto), g_ar_far[idx], g_ar_front_n[idx]);
        }
        if (g_ar_lent[idx].lent || g_ar_lent[idx].asked)
            ar_log(idx, now, "over the sea: landings asked %u, soldiers lent %u, back %u, still away %d", g_ar_lent[idx].asked, g_ar_lent[idx].lent, g_ar_lent[idx].back, av_of(idx, AV_LANDING) ? av_of(idx, AV_LANDING)->n : 0);
        if (g_ar_aid > 0) ar_log(idx, now, "ally aid: raids on an ally answered %u", g_ar_aids[idx]);
        if (AR_PARTS(idx)) ar_log(idx, now, "raid parties: sent %u (%d soldiers), back with the raid over %u, losing %u, lost whole %u, soldiers lost %d", g_ar_pt[idx].sent, g_ar_pt[idx].men, g_ar_pt[idx].cleared, g_ar_pt[idx].losing, g_ar_pt[idx].wiped, g_ar_pt[idx].lost);
        static struct av_sum sum;   // ARMY VIEW (army_view.c): the force as one, for the log only so far
        static char line[1600];
        av_clear(&sum);
        for (int k = 0; k < a->n; k++) if (cv_alive(a->held[k].u)) av_add(&sum, a->held[k].u);
        av_done(&sum);
        av_foes_scan(pl);
        av_line(&sum, av_kinds_for(&sum, AVF_LAND), line, (int)sizeof line);
        ar_log(idx, now, "the force as one: %s", line);
        uint64_t ours = av_kinds_in(&sum);
        for (int j = 0; j < g_av[idx].n; j++) {   // the other armies
            struct av_army *o = &g_av[idx].a[j];
            av_clear(&sum);
            for (int k = 0; k < o->n; k++) if (av_live(&o->m[k], pl)) av_add(&sum, o->m[k].u);
            av_done(&sum);
            av_line(&sum, av_kinds_for(&sum, o->field), line, (int)sizeof line);
            ar_log(idx, now, "army %d, %s%s%s%s%s (%d members, since %.1f min): %s", o->id, k_av_kind[o->kind], o->what ? " " : "", o->what ? o->what : "",
                   o->task ? ", " : "", o->task ? o->task : "", o->n, o->made_t / 60000.0, line);
            ours |= av_kinds_in(&sum);
        }
        av_enemy_build(pl, now);   // THE ENEMY'S ARMIES (army_view.c): what it knows of them, each against its own armies' families
        int small = 0, small_n = 0, all_n = 0;
        for (int j = 0; j < g_ave[idx].n; j++) {
            const struct av_enemy *e = &g_ave[idx].a[j];
            all_n += e->n;
            if (e->n < 3 && e->mid.x >= 0) { small++; small_n += e->n; continue; }
            av_enemy_sum(idx, e, &sum);
            av_line(&sum, ours, line, (int)sizeof line);
            ar_log(idx, now, "the enemy's %s army of P%d %s(%d, seen %.1f min ago): %s", k_av_field_name[e->field & 15], e->owner,
                   e->mid.x >= 0 ? "" : "whereabouts unknown ", e->n, now >= e->seen_t ? (now - e->seen_t) / 60000.0 : 0.0, line);
        }
        ar_log(idx, now, "the enemy's armies known: %d (%d units), of them %d of 1-2 units (%d) not listed", g_ave[idx].n, all_n, small, small_n);
        avx_log(idx, now, ar_log);   // what each enemy fields as it expects, what it saw of its own, its counters to expect
    }
}
// TRAINING BY NEED ([ai_base] train_by_need, on since step 6 (below); [autotest] v_train_by_need; army view step 4b - the
// player 2026-11-11: the AI sees its armies as one, how strong, what they are good against, has it siege): FILL (army.c)
// trains the land type that adds most to the force against the enemy it will meet, for what it costs - every land type
// with an attack the game lets it train now (siege and rams too, no citizens, priests) that the money FILL may spend
// pays, 1000 resources' worth of it (one at least) added to the force at home, the fight estimate (st_ratio) against the
// defenders and towers at its next target (stage_foe at ar_target_on: what the AI knows there) and the enemy fighters it
// sees on its own land besides: the most gain over the force as it is, per 1000. Nothing known of the enemy: FILL's own
// pick (the game's request lists, else the strongest). The dry run (4 games, Continental large from epoch 5, the log
// only): it differed from FILL's pick in 21-33 of 40-60 looks a game - pikemen, crossbows, pilums, longswords, heavy
// rams against FILL's cataphracts -, with 1.5 to 3 times the gain a 1000 where both had one; no enemy known in half.
// Measured (Continental large from epoch 5, 60 min, 10 games each): the mod against itself (h2h mirrored; B by need) kills
// +13.8 (se 10.1), losses -1.3 (19.1), soldiers -0.7 (15.2), razed -0.8 (1.8); against the non-cheating game AI off / on
// razed 14.3 / 8.4, buildings lost 2.2 / 4.3, kills 151 / 143, losses 52 / 60, won 5 / 4 - no gain, if anything worse. Why:
// what it judges against is what the AI knows at the target and sees on its land, often 1-9 units (ratios like 98 -> 126
// against one), and in most looks nothing (a pick in 4 of 58); the enemy's army as the AI has seen it, remembered, is
// what the need has to be measured against. Off.
// Since step 5 (army_view.c THE ENEMY'S ARMIES) the enemy beyond the target is its land armies as the AI knows them - every
// enemy land unit seen within 10 minutes on its land, where it was seen, and those whose whereabouts it lost (counted at
// the target) - instead of the fighters it sees now; and FILL's own pick is an honest one since honest_intel (5beceafb:
// the game's counter vote had counted the enemy's whole army). Measured so (the same setup): h2h B by need - A soldiers
// +27.3 (se 14.8), kills +4.0 (14.4), losses -20.3 (21.7), razed +3.4 (3.0), workers +12.1 (12.7); against the game AI
// off / on won 7 / 5, razed 18.6 / 17.2, own losses 60 / 57, buildings lost 4.4 / 3.6 - a lean towards it among its own
// kind, nothing beyond the noise; still judged against 1-9 known units in most looks (the AI sees little of the enemy's
// army: 7-13 units remembered at minute 29 of a game). Still off.
// Since step 6 (army_view.c WHAT THE ENEMY FIELDS) it judges against the army each enemy it will meet fields as the AI
// expects it and that enemy's counters to what it saw of the AI's. Measured with scouting (Continental large from epoch 5,
// 60 min, 10 games each): the mod against itself (h2h mirrored, B both on) soldiers +20.1 (se 7.1), kills +21.3 (9.3),
// razed +2.6 (1.1), losses -13.3 (15.2); against the non-cheating game AI off / on won 3 / 4, razed 17.7 / 21.4, kills
// 160 / 165, buildings lost 2.8 / 1.7 - judged against 22-54 units a look instead of 1-9. On since 2026-11-11.
// "# armies: ... by need:" once a game minute in tests.
internal int g_ar_need_on = 1, g_v_ar_need = -1;
#define AR_NEED(idx) PV(idx, g_v_ar_need, g_ar_need_on)
internal struct { unsigned looks, same, other, none; } g_ar_need[16];
// the pick (0: none - nothing known of the enemy, nothing payable); vis: what may be spent (NULL: anything)
internal int ar_need_pick(ee_player *pl, unsigned idx, const int *vis, int filled, uint32_t now, int log) {
    idx &= 15;
    struct armies_st *a = &g_armies[idx];
    if (!ar_on(pl, idx)) return 0;
    unsigned land = map_area(a->home, 2);
    ee_unit *tg = land ? ar_target_on(pl, a->home, land) : NULL;
    ivec2 at = tg ? unit_get_tile(tg) : a->home;
    stage_foe(pl, at);
    // the enemies it will meet - the target's owner, those with a land army it knows of on its land or lost track of - as it
    // expects them (army_view.c WHAT THE ENEMY FIELDS): the land army each fields and its counters to what it saw of the
    // AI's, at the target beside what stage_foe has there already (the defenders it remembers, the towers)
    av_enemy_build(pl, now);
    unsigned owners = tg && unit_get_owner(tg) ? 1u << (player_get_index(unit_get_owner(tg)) & 15) : 0;
    for (int j = 0; j < g_ave[idx].n; j++) {
        const struct av_enemy *e = &g_ave[idx].a[j];
        if (e->field == AVF_LAND && (e->mid.x < 0 || e->area == land) && e->owner > 0 && e->owner < 16) owners |= 1u << e->owner;
    }
    static float want[G_TYPES_N];
    for (int o = 1; o < 16; o++) {
        ee_player *op = owners >> o & 1 ? player_at(o) : NULL;
        if (!op) continue;
        for (int t = 0; t < G_TYPES_N; t++) { ee_unit_type *td = type_data(t); want[t] = td && av_field_of(td) == AVF_LAND ? g_avx[idx][o][t] : 0.0f; }
        avx_counters(idx, o, AVF_LAND, AVF_LAND, want);
        for (int g = 0; g < g_sgs_foe.ng; g++) if (g_sgs_foe.g[g].owner == o && (unsigned)g_sgs_foe.g[g].type < G_TYPES_N) want[g_sgs_foe.g[g].type] -= (float)g_sgs_foe.g[g].count;
        for (int t = 0; t < G_TYPES_N; t++) {
            int n = (int)(want[t] + 0.5f);
            ee_unit_type *td = n > 0 ? type_data(t) : NULL;
            int hp = td ? st_maxhp(td, st_rec(op, t)) : 0;
            for (int k = 0; k < n && hp > 0; k++) if (!st_side_add_rec(&g_sgs_foe, t, o, hp, at.x, at.y)) break;
        }
    }
    if (log) g_ar_need[idx].looks++;
    if (!g_sgs_foe.n) { if (log) g_ar_need[idx].none++; return 0; }
    ar_side(a, 0, NULL);
    float r0 = st_ratio(&g_ar_own, &g_sgs_foe);
    int best = 0, nbest = 0; float bg = 0, gfill = 0, rbest = r0;
    for (int ty = 0; ty < G_TYPES_N; ty++) {
        ee_unit_type *td = type_data(ty);
        if (!td || unit_type_get_id(td) != ty || unit_type_get_class_id(td) != CLS_LAND || unit_type_get_attack(td) <= 0) continue;
        if (ut_role(ty, UTR_CITIZEN) || ty == UT(PRIEST) || ty == UT(PROPHET) || !can_build(pl, ty)) continue;
        int cost = 0, pays = 1;
        for (int r = 2; r < 7; r++) { int c = unit_cost(pl, ty, r); cost += c; if (vis && vis[r] < c) pays = 0; }
        if (cost <= 0 || (!pays && ty != filled)) continue;
        int n = 1000 / cost > 1 ? 1000 / cost : 1;
        if (n > 40) n = 40;
        ar_side(a, 0, NULL);
        ee_unit_stats *st = st_rec(pl, ty);
        int hp = st_maxhp(td, st);
        for (int k = 0; k < n; k++) st_side_put(&g_ar_own, NULL, td, st, (int)idx, hp, a->home.x, a->home.y);
        float r = st_ratio(&g_ar_own, &g_sgs_foe), g = (r - r0) * 1000.0f / (float)(n * cost);
        if (ty == filled) gfill = g;
        if (pays && (!best || g > bg)) { best = ty; bg = g; nbest = n; rbest = r; }
    }
    if (!best || !log) return best;
    if (best == filled) g_ar_need[idx].same++; else g_ar_need[idx].other++;
    ar_log(idx, now, "by need: FILL's own pick %s (%d, %+.3f a 1000), the need %s (%d, %+.3f a 1000: %d of them %.2f -> %.2f) against %d known%s at %d,%d; looks %u, the same %u, other %u, no enemy known %u%s",
           filled ? unit_ident(filled) : "none", filled, gfill, unit_ident(best), best, bg, nbest, r0, rbest, g_sgs_foe.n, tg ? " (the next target's and its land's armies)" : " (its land's armies)", at.x, at.y,
           g_ar_need[idx].looks, g_ar_need[idx].same, g_ar_need[idx].other, g_ar_need[idx].none, AR_NEED(idx) ? " - trained" : "");
    return best;
}
// FILL's type (army.c): the need's when train_by_need is on and it has one, else FILL's own; once a game minute in tests the
// comparison logged
internal int ar_fill_type(ee_player *pl, unsigned idx, const int *vis, int own, uint32_t now) {
    static uint32_t t[16];
    idx &= 15;
    int log = g_log && g_autotest && (!t[idx] || now < t[idx] || now - t[idx] >= 60000);
    if (log) t[idx] = now ? now : 1;
    if (!AR_NEED(idx) && !log) return own;
    int need = ar_need_pick(pl, idx, vis, own, now, log);
    return AR_NEED(idx) && need ? need : own;
}
internal void armies_new_game(int from_save) { memset(g_armies, 0, sizeof g_armies); memset(g_ar_det, 0, sizeof g_ar_det); memset(g_ar_group, 0, sizeof g_ar_group); memset(g_ar_look, 0, sizeof g_ar_look); memset(g_ar_lent, 0, sizeof g_ar_lent); memset(g_ar_aids, 0, sizeof g_ar_aids); memset(g_ar_pt, 0, sizeof g_ar_pt); memset(g_ar_need, 0, sizeof g_ar_need); av_new_game(); g_ar_load = from_save; g_ar_loaded = 0; }
// persist.c, after a save's blocks were read: the force came with it - its pointers wait for the first tick
internal void armies_loaded(void) {
    int any = 0;
    for (int p = 0; p < 16; p++) any |= g_armies[p].n || g_armies[p].t;
    if (!any) return;
    g_ar_load = 2;
    for (int p = 0; p < 16; p++) {
        for (int k = 0; k < g_armies[p].n; k++) g_armies[p].held[k].u = NULL;
        g_armies[p].target = NULL;
    }
}
internal void armies_install(const char *autotest_ini) {
    g_mod_armies = g_set_ai[1].mod_armies;
    g_ar_sea = g_set_ai[1].mod_armies_sea;
    g_ar_raid_answer = g_set_ai[1].raid_answer;
    g_ar_held_ground = g_set_ai[1].raid_held_ground;
    g_ar_front_ter = g_set_ai[1].force_at_front;
    g_ar_leash = g_set_ai[1].raid_leash;
    g_ar_aid = g_set_ai[1].ally_aid;
    g_ar_parts = g_set_ai[1].raid_parties;
    g_ar_need_on = g_set_ai[1].train_by_need;
    g_ar_scout_on = g_set_ai[1].scouting;
    g_fln_on = g_set_ai[1].fleet_by_need;   // army_view.c THE FLEET BY NEED
    if (autotest_ini) {
        g_v_ar_scout = ini_int("autotest", "v_scouting", -1, autotest_ini);
        g_v_fln = ini_int("autotest", "v_fleet_by_need", -1, autotest_ini);
        g_v_aa_exp = ini_int("autotest", "v_aa_expected", -1, autotest_ini);   // defence.c anti_air
        g_v_aa_strong = ini_int("autotest", "v_aa_strong", -1, autotest_ini);
        g_v_shelter_first = ini_int("autotest", "v_shelter_first", -1, autotest_ini);   // bunker.c
        g_v_mod_armies = ini_int("autotest", "v_mod_armies", -1, autotest_ini);
        g_v_ar_sea = ini_int("autotest", "v_mod_armies_sea", -1, autotest_ini);
        g_v_ar_parts = ini_int("autotest", "v_raid_parties", -1, autotest_ini);
        g_v_ar_held_ground = ini_int("autotest", "v_raid_held_ground", -1, autotest_ini);
        g_v_ar_front_ter = ini_int("autotest", "v_force_at_front", -1, autotest_ini);
        g_v_ar_need = ini_int("autotest", "v_train_by_need", -1, autotest_ini);
        g_v_ar_raid_band = ini_int("autotest", "v_ar_raid_band", -1, autotest_ini);
        g_v_ar_press = ini_int("autotest", "v_ar_press", -1, autotest_ini);
        g_v_ar_fort = ini_int("autotest", "v_ar_fort", -1, autotest_ini);
        g_v_ar_assume = ini_int("autotest", "v_ar_assume", -1, autotest_ini);
        g_v_ar_back_pct = ini_int("autotest", "v_ar_back_pct", -1, autotest_ini);
        g_v_ar_back_fight = ini_int("autotest", "v_ar_back_fight", -1, autotest_ini);
        g_v_ar_way = ini_int("autotest", "v_ar_way", -1, autotest_ini);
        g_v_ar_form = ini_int("autotest", "v_ar_form", -1, autotest_ini);
        g_v_ar_judge = ini_int("autotest", "v_ar_judge", -1, autotest_ini);
        g_v_ar_far_pct = ini_int("autotest", "v_ar_far_pct", -1, autotest_ini);
        g_v_ar_route = ini_int("autotest", "v_ar_route", -1, autotest_ini);
        g_v_ar_breach = ini_int("autotest", "v_ar_breach", -1, autotest_ini);
        g_v_ar_fire = ini_int("autotest", "v_ar_fire", -1, autotest_ini);
        g_v_ar_price = ini_int("autotest", "v_ar_price", -1, autotest_ini);
    }
    if (g_log) fprintf(g_log, "# mod armies: %d (variant %d), over the sea %d (variant %d)\n", g_mod_armies, g_v_mod_armies, g_ar_sea, g_v_ar_sea);
}
