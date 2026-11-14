// [ai_base]: the computer players, a value per difficulty (g_set_ai[d]): the medium AI plays it as it is
namespace ageofai.settings;

///
/// --------------------------------------------------------------------------------------------
///  AI behaviour. Everything in [ai_base] applies to every difficulty (the medium AI plays it as it is).
///  To set something for the easy or the hard AI only, put the same line into [ai_easy] or [ai_hard] -
///  it wins over [ai_base].
/// --------------------------------------------------------------------------------------------
[section(70), title("AI"), all]
class ai_base
{
    class EconomyAndEpochs
    {
        /// Numbers here may be written as small expressions: 35% is 0.35, 2 * 60, min(...) - shares always as shares
        /// (navy_share=25%, home_guard_share=35% or 0.35). A value the mod can't read (25 for a share: 2500%) is logged in
        /// ageofai_log.csv (# ini:) and the default is used.
        ///
        /// Resource discount: what the AI pays, in percent of the real price.
        ///   100% = fair (same prices as you), 75% = a quarter cheaper, 50% = half price.
        ///   0    = don't touch it: use the game's "Cheat" setting (scenario editor / data files).
        [range(0, 10), game("the price the AI pays, in percent of the real one (with the game's Cheat toggle)", "100%", 0)]
        share cost_percent = 1;
        enum copper_plan_choice
        {
            /// the original behaviour
            game,
            /// the plan
            on,
            /// experimental (docs/economy.md): only the next epoch is saved for, and above min_workforce the
            /// citizens keep training - epoch_share of the income goes into an epoch account instead; food_floor
            /// food workers before farms
            next_epoch,
        }
        ///
        /// Early-game economy plan (before farms, which arrive in the Copper Age):
        /// the AI builds up its economy, counts the food it can find, saves for the next epochs,
        /// keeps only a token army and advances as soon as it can pay:
        copper_plan_choice copper_plan = copper_plan_choice.on;
        [range(0, 1), commented]
        share epoch_share = 0.4;
        [commented]
        int food_floor = 18;
        ///
        /// Military budget: soldiers may only be paid from a share of the AI's starting stock and of
        /// everything it gathers; the rest goes into economy and epochs. 1 = on, 0 = original behaviour
        int budget = 1;
        /// share of the starting stock / of all income that may go into the army (0% - 100%)
        [range(0, 1)]
        share military_start_share = 0.3;
        [range(0, 1)]
        share military_income_share = 0.4;
        /// from the Copper Age on (farms: food no longer runs out) a player puts most of his income into the army
        [range(0, 1)]
        share military_income_share_farms = 0.65;
        enum epoch_pacing_choice
        {
            /// the original game: its epoch timer (epoch_min..epoch_max below), and at most one epoch ahead of
            /// the highest human epoch - the timer runs twice as long while ahead, half while behind. [ai_easy] below
            classic,
            /// only the epoch timer, the human players' epochs don't matter
            timer,
            /// as soon as the next epoch can be paid (the AI puts epochs before everything else)
            rush,
            /// never ahead of the highest human epoch: it prepares the next epoch's buildings and advances as soon
            /// as it can pay once a human is ahead (no human in the game: the timer)
            follow,
            /// the epoch timer, and a few minutes after a human reaches an epoch the AI is behind in, it follows
            /// (for now; planned: the AI weighs when the next epoch pays off)
            independent,
        }
        ///
        /// Epoch pacing - when the AI advances to the next epoch. Every mode but timer has the game's catch-up: a human
        /// reaching a new epoch makes every AI behind it try its next epoch a few minutes later (catch_up_ais: after other
        /// computer players too, not only humans; 0 = humans only, as the original). Before the Copper Age the economy plan
        /// (copper_plan) advances as soon as it can pay in every mode (follow: once a human is ahead; classic: one epoch
        /// ahead at most):
        epoch_pacing_choice epoch_pacing = epoch_pacing_choice.independent;
        /// the epoch timer: minutes between epochs (original game: 50 - 70; the game still scales this a little by difficulty)
        [game("the AI's epoch timer, minutes at least (the game scales it a little by difficulty)", "50", 0)]
        int epoch_min = 50;
        [game("the AI's epoch timer, minutes at most", "70", 0)]
        int epoch_max = 70;
    }

    class Attacks
    {
        ///
        /// --- Attacks ---  (remove the ';' to use a line; without it the original value applies)
        /// minutes until the first attack (original 20 - 40)
        [game("minutes until the AI's first attack, at least", "20", 0), commented("20")]
        int first_attack_min = 0;
        [game("minutes until the AI's first attack, at most", "40", 0), commented("40")]
        int first_attack_max = 0;
        /// minutes between regular attack waves (original 14 - 17; the game scales this by difficulty)
        /// suggestion below: fewer but bigger waves - in tests the AIs kept ~20% more workers
        [game("minutes between the AI's attack waves, at least (scaled by difficulty)", "14", 0)]
        int attack_interval_min = 20;
        [game("minutes between the AI's attack waves, at most", "17", 0)]
        int attack_interval_max = 25;
        /// size of an attack group (original: easy 80%, medium 110%, hard computed)
        [range(0, 10), game("the size of the AI's attack groups", "easy 80%, medium 110%, hard computed", 0)]
        share attack_group_percent = 1.5;
        /// chance of an extra, immediate attack - the source of many small, wasted waves:
        ///   on contact with an enemy (original 71%), to take an enemy's resources (original 20%),
        ///   out of revenge when attacked (original 10%). 0% = never.
        [range(0, 1), game("the chance of an extra attack on contact with an enemy", "71%", -1)]
        share attack_on_contact = 0;
        [range(0, 1), game("the chance of an extra attack to take an enemy's resources", "20%", -1)]
        share attack_for_resources = 0;
        [range(-0.01, 1), game("the chance of an extra attack out of revenge", "10%", -1), commented("10%")]
        share attack_revenge = -0.01;
        /// share of the population cap for citizens (the rest is military), e.g. 60%; 0 = the game's own (default)
        [range(0, 1), game("the AI's share of the population cap for citizens", "45%", 0), commented("0")]
        share citizen_share = 0;
        /// how many of its 9 activities the AI attends to each 30 seconds (citizens, gatherers, research, the building plan,
        /// the epoch's techs, defence, exploring, its attacks, houses): the game takes the first ones that pass its own stock
        /// checks. 0 = the game's own (5), 9 = all that pass
        [range(0, 9), game("how many of its activities the AI attends to each period", "5", 0)]
        int attention_activities = 0;
        /// with fewer buildings than this the game sends every citizen at an enemy (its Bonzai Attack, repeated every 5
        /// minutes). 0 = never, -1 = the game's own (9)
        [range(-1, 999), game("the buildings left under which the AI's citizens make a last all-out attack", "9", -1)]
        int last_stand_buildings = 0;
        /// the game's answer to a Wonder: its groups sent at an enemy's Wonder, or to defend its own or an ally's (only with
        /// Wonders for victory). Off (the player 2026-11-10: "it just baits the ai to its death"); the mod has no answer of
        /// its own yet
        bool wonder_reactions = false;
    }

    class AttackRouting
    {
        ///
        /// --- Attack routing --- (only enemy buildings the AI has actually seen count; no map knowledge)
        /// Armies march around known enemy towers instead of straight through them. 1 = on, 0 = original routes
        bool avoid_towers = true;
        /// Experimental: if the goal of an attack lies inside known tower range, attack an undefended enemy
        /// building nearby instead (the economy behind the towers, the open flank). 1 = on, 0 = off
        bool attack_weak_spots = true;
        /// Soldiers on an attack don't take on enemies standing inside known tower range (defenders waiting next to
        /// their towers would draw the whole army in) - unless the attack's goal is in there anyway. 1 = on, 0 = off
        bool attack_leash = true;
        /// When the way to the goal crosses a known enemy wall, break through it away from the towers instead of piling up
        /// in front of it. 1 = on, 0 = off
        bool attack_breach = true;
        /// Soldiers on an attack ignore enemy aircraft out of their reach (their range + 3 tiles): they chased the
        /// player's planes circling over their own town and the whole attack waited for them (6+ minutes at its docks on the
        /// reference save). In reach they still shoot. 1 = on, 0 = off (the original)
        bool attack_ignore_air = true;
        /// An attack whose target player is out (or no enemy any more) turns on the living enemy whose known building is
        /// nearest to it - the game keeps a wave on a beaten player, walking its legs about the map. 1 = on
        bool attack_retarget = false;
    }

    class WhatTheAISees
    {
        /// What the AIs know. The game shows them every unit of a human player, everywhere and always (measured: a rifleman
        /// alone at the map edge, far from every AI unit, counts as seen). 0 = the mod uses that as the game has it (all-knowing
        /// about the human). 1 = honest: an AI knows only what its units (and its allies') see now, remembers what it saw -
        /// buildings until it looks again and they are gone, soldiers for 10 minutes where it saw them - and assumes a quarter
        /// of the army it knows of at a place it hasn't looked at for 3 minutes; a landing's warships look at the beach first.
        /// The memories are saved with the game. (The game's own AI code still sees everything either way - but
        /// honest_intel.)
        bool honest_vision = true;
        /// With honest_vision: the game's own AI code keeps a file on each player - what units it has, how strong it is on
        /// land, at sea and in the air, where it stands - filled from all of that player's units, unseen ones too: its unit
        /// choice trains counters to an army it never saw, its attacks pick their target by it. 1 = the file holds only the
        /// enemy units the AI sees or remembers (the units of allies and the AI's own as before). 0 = the game's file
        bool honest_intel = true;
        /// With honest_vision: an enemy unit that is hitting one of the AI's units counts as seen while it does - as a player
        /// sees the shooter for a moment when his units are hit -, so the AI remembers artillery and mortars that shell it
        /// from beyond its sight where they stood. 0 = off
        bool reveal_attackers = true;
    }

    class CitizensUnderAttack
    {
        /// Citizens, enemy towers and raiders: the AI no longer sends citizens to resources inside the range of enemy towers
        /// it has seen, past them on the way from its drop site, or where enemy soldiers stand (6 tiles), and calls back
        /// those already working there. 1 = on
        bool avoid_towers_citizens = true;
        /// One citizen at a time to a building: the game's repair round (every 31 s) sends a citizen to each damaged or
        /// unfinished building nobody is working on - also while the last one it sent is still walking there, so a
        /// foundation far away drew a citizen every 31 s (an AI lost its farmers and miners to four of them in ten
        /// minutes). The round now skips a building a citizen is already on its way to. 1 = on
        bool repair_one_walker = true;
        /// The repair round's citizens go only by a safe way: no order whose way passes enemy soldiers the AI sees or
        /// remembers, or known enemy towers. 0 = off (default)
        bool repair_safe_way = false;
        /// Citizens fight back: they attack the enemy citizens building a tower near them (any age) and, up to the Bronze Age,
        /// small groups attacking them - only when clearly stronger (3x hit points x attack). 1 = on
        bool citizen_militia = true;
        /// Experimental: the citizens decide with the fight estimate instead, in every age - as a fallback when no soldiers
        /// near would win, unguarded builders when they win, soldiers only when they win clearly or are caught by faster
        /// units and still win. 0 = off (default)
        bool militia_estimate = false;
        /// ... with militia_estimate: caught by faster soldiers, the citizens fight back only when they would still destroy this
        /// share of the attackers (100% = all of them; 33% had 3 citizens attack a tank 42 times)
        [range(0, 1), commented]
        share militia_caught_share = 1;
    }

    class TheEarlyGame
    {
        /// The early game (docs/economy.md), the playtester's opening against rushes:
        /// towers first - in the first two ages the town's first 2 towers stand by the Town Center and every unfinished tower
        /// near the town gets builders. 1 = on (default)
        bool early_towers = true;
        /// the bait - citizens attacked by soldiers (up to the Bronze Age) fall back under their nearest own tower. 1 = on
        bool tower_bait = true;
        /// against soldiers the citizens fight only next to own soldiers or under own towers, and only when they would win
        /// without a loss. 1 = on
        bool early_militia = true;
        /// no army in the first age, then food-free ranged soldiers (Slinger, Simple Bowman, Javelin, Composite Bow,
        /// Long Bow ...) while the economy grows; the army's account is its own money before farms. 0 = off (default)
        bool early_army = false;
        [range(0, 1), commented]
        share early_army_food_share = 0.1;
        [commented]
        int early_army_workers = 60;
        /// the alternative to the towers: this many soldiers at home in the first age (with early_army). 0 = none
        bool early_soldiers = false;
    }

    class SafetyAndFortresses
    {
        /// Citizens keep away from what hurts them: artillery and ships at their full reach (up to 27 tiles), and spots where
        /// citizens were hit in the last 2 minutes (artillery firing from the fog, aircraft where they strike) - food and wood
        /// are left after one hit nearby, stone, gold and iron (worth some risk) after three. 1 = on
        bool citizen_threats = true;
        /// Raids the town holds (the player: citizens keep working even under attack - can my towers and soldiers there
        /// beat the raiders before they kill my citizens?): a site with enemy soldiers near stays worked while the AI's
        /// soldiers within 10 tiles and its towers covering it beat them by this much on the fight estimate (150% =
        /// clearly, the default - with hurt_seen_short against the cheating game AI, 40 long games a map: Small Islands
        /// won 0 -> 2, lost 36 -> 29, the worker gap -116 -> -62; Large Islands won 11 -> 16; Continental the same). 0 = off
        [range(0, 5), commented]
        share raid_held = 1.5;
        /// A spot where a citizen was hurt with an enemy soldier in sight near it counts 20 s instead of 2 minutes (it
        /// was seen - its own reach covers the rest); hurt by what it can't see (artillery from the fog, aircraft) still
        /// 2 minutes. 1 = on (default, measured with raid_held)
        bool hurt_seen_short = true;
        /// Enemies behind its walls: where its own walls close off the land around its town (or its island's coast), an
        /// enemy outside them no longer keeps the citizens inside from their work if it can't get at them - one that
        /// fights hand to hand, or a shooter whose line of fire a wall blocks (crossbows, guns, ships). Bowmen, slingers,
        /// trebuchets and catapults fire over a wall and still count; a wall broken open lets them all in again.
        /// 0 = off (default until measured)
        bool behind_walls = false;
        /// Ships at their own reach: an enemy ship keeps citizens off their work only within its range and a tile, round -
        /// not its range and 3 tiles in a square, which on a small island covered nearly all of it (most citizens sent
        /// home on the island maps were sent for ships alone, while ships killed a quarter of the dead). 0 = off
        /// (default until measured)
        bool ship_reach_exact = false;
        /// Danger routes: the AI keeps a map of where its enemies can reach (their defences it has seen, soldiers and warships
        /// it sees at their reach, where its citizens were hurt lately), and its units that can't fight - citizens, fishing
        /// boats, transports - take their way round those tiles where there is one, through only where there is none (with
        /// land_routes / ship_routes). 0 = off
        bool danger_routes = true;
        /// Lost fights (with danger routes): where the AI sees an enemy force its soldiers nearby can't beat, its soldiers on
        /// their way somewhere go round it, its attack groups' marches stop at its edge (and walk out of it), and its members
        /// pick no target inside it beyond their own reach - they wait at the edge till enough have come to win, instead of
        /// running in a few at a time. 0 = off
        bool avoid_lost_fights = true;
        /// The fight judgements of fall-back, lost fights and the staging point count the enemy soldiers the AI saw within
        /// the last 3 minutes where it saw them, not only those in sight at that moment (with honest_vision). 0 = off
        bool fight_memory = true;
        /// The citizens' safety (builders' and gatherers' sites, build orders, their routes) counts the enemy soldiers the AI saw
        /// within the last minute where it saw them, not only those in sight (with honest_vision). 0 = off
        bool threat_memory = true;
        /// Fortresses (Copper Age, 350 wood, hold 40 units off the map): citizens under fire go into one nearby and come out
        /// when it is quiet; the AI builds one at its main drop site once enemy aircraft are possible or citizens get hurt
        /// (a second from 40 citizens). 1 = on
        bool bunkers = true;
        /// ... first: its first fortress once enemy aircraft are possible and it has 20 citizens, another while 3 or more
        /// of its citizens were hurt in the last 2 minutes - each goes ahead of everything it saves for (the priority
        /// list's first row, with the anti-air): under bombing runs it waited behind the farms. 1 = on
        bool shelter_first = true;
        /// ... and a reserve outside the population limit (units in a fortress don't count): with the map full, idle soldiers
        /// at home go in so training goes on; they come out after losses or when enemies come near. A mask of players
        /// (65535 = all, 0 = off)
        int bunker_reserve = 65535;
        /// ... and town guards (with the reserve): a fortress also at each front (by the AI's tower nearest it), each filled
        /// from the idle soldiers round it, letting them out when enemies come near it. 1 = on, being measured
        bool town_guards = false;
        /// ... and a shelf for citizens it can't use: at the population limit with food, wood, gold and iron each at
        /// shelve_stock or more, idle citizens go into fortresses (more are built for them) so soldiers can be trained, and no
        /// new citizens are; they come out once a resource runs low or citizens are lost. 1 = on
        bool shelve_citizens = true;
        [range(500, 1000000)]
        int shelve_stock = 5000;
        /// Settlement upgrades: spare citizens (idle once the AI trains no more or its food keeps every producer training, or wood cutters while the wood stock is over 5000) go into a Settlement by
        /// metal mines - 5 make it a Town Center -, then into the Town Center with the most metal mines near it - 10 more make
        /// it a Capitol, which takes up to 50. Each citizen inside adds to the stone, gold and iron dropped there from mines
        /// within 4.5 tiles (+25% at 5, +50% at 15, +102% at 50); the citizens are used up. 1 = on
        bool garrison_upgrades = true;
        /// Territory shelter (ai/territories.c): the map is split into territories, and where 2 or more enemy land soldiers
        /// stand in or by one and the AI's soldiers and towers there lose against them, all its citizens in that territory stop
        /// working: into a Fortress on that land within 30 tiles, else to a drop site in a territory that is safe; no
        /// citizen is sent to a site there until it is safe again. 1 = on. Off (2026-11-14, 192 games on island maps: a
        /// fifth fewer citizens killed, the army at 60 minutes no larger - a few soldiers fewer, within the noise)
        /// Idle transports: the game's own order for one more transport ship trains none while this many of the AI's
        /// transports stand empty with nothing to do (seen: an AI with 40 idle transports at minute 176, 250 wood and a
        /// place under the population cap each). The mod's landings and outposts order their own as before. 0 = off
        [range(0, 50)]
        int idle_transports = 2;
        bool territory_shelter = false;
        /// Held ground (ai/armies.c HELD GROUND): the army answers a raid wherever it stands in a territory the AI holds
        /// on the army's land, not only within 60 tiles of where the army stands - a town far out has no guard, so the
        /// army (or a part of it that wins two to one) comes. 1 = on. Off until the territory system is measured as a whole
        bool raid_held_ground = false;
        /// The army at the front (ai/armies.c FRONT STATION): between attacks the army stands in the territory it holds
        /// whose border with ground nobody of its side holds is the heaviest (its length, the enemy soldiers known
        /// beyond it, its buildings there), on the side of that border; on an island with no such border as before.
        /// 1 = on. Off until the territory system is measured as a whole
        bool force_at_front = false;
        /// Front guards (ai/expand.c FRONT GUARDS): an island the AI holds whose territory is a front - ground or water by
        /// it that nobody of its side holds - gets a boat of soldiers when it has fewer than its guard wanted (a third of
        /// its citizens there, 3 to 12, 8 with enemies seen), also after the outpost was handed back; an island that has
        /// become backland gets none; the army leaves the soldiers standing on such an island alone. 1 = on. Off until
        /// the territory system is measured as a whole
        bool front_guards = false;
        /// The fleet's station (ai/fleet.c FLEET STATION): the fleet's guard point lies in the water territory with the
        /// longest shore along land the AI holds, the more so where enemy warships were seen - holding that sea is what
        /// turns an island into backland. 1 = on. Off until the territory system is measured as a whole
        bool fleet_station = false;
        /// Harbours (ai/fleet.c HARBOURS): every sea territory that borders land the AI holds gets one harbour - a dock from
        /// the held territory with the longest shore there, a tower by it, a second where enemy warships were seen on that
        /// sea or with 600 stone in the bank - so each sea by its land is covered once, for the fleet to fall back to and for the fishing, on every kind
        /// of map. 1 = on. Off until the territory system is measured as a whole
        bool harbours = false;
        /// Border towers (ai/territories.c BORDER TOWERS): while 1500 stone and 1500 wood lie in the bank, one tower every
        /// 20 seconds on the outside border of a territory the AI holds - by the water or by land nobody of its side
        /// holds - where none of its towers stands within 12 tiles; from each territory's middle outwards, until its
        /// shores and open borders are covered. Peace of mind against a landing from behind; not something the AI
        /// against AI games can show. 1 = on. Off with the rest of the territory system
        bool border_towers = false;
    }

    class SiegeAndTowerRouting
    {
        /// Siege weapons like a player: they shell known enemy towers (that they outrange) and the walls to breach from a
        /// safe distance, while idle soldiers guard them; the AI builds a Siege Factory and trains 2-4 siege weapons of its
        /// epoch when it knows enemy defences (with priority on the stock above the epoch reserve); rams don't attack
        /// buildings guarded by enemy soldiers. 1 = on, 0 = off
        bool attack_siege = true;
        /// advanced: route cost per tower (a normal step costs 10-14), extra tiles kept from a tower's range,
        /// search radius (tiles) around the original goal for a weak spot
        [commented]
        int avoid_towers_weight = 2000;
        [commented]
        int avoid_towers_margin = 6;
        [commented]
        int attack_weak_spot_radius = 60;
    }

    class Building
    {
        ///
        /// Build queue fix: the game's build list for random maps starts with a settlement it can never place, which
        /// blocks every later building (granary, archery range, stable, siege factory...) for the whole game. It is
        /// built as a normal Settlement, and a building that can't be placed 3 times in a row is skipped. 1 = on
        bool build_fix = true;
        /// Idle citizens home: the game sends an AI's idle citizens to one base point for the whole empire - after loading a
        /// game the spot of the first unit it finds, a ship at sea or a soldier far away. An idle citizen away from its towns,
        /// or one sent to that point, goes to its nearest safe Town Center, Capitol or Settlement instead, and a base point at
        /// sea or away from all towns moves to the biggest town. Unfinished buildings stacked on one tile (the AI laid one
        /// on top of another, again and again) are cleared to one. 1 = on
        bool citizens_home = true;
        /// Farm fields: the game's AI left the fields around its granary as bare foundations (always on maps with fish,
        /// often on land), put nobody on finished ones and never replaced destroyed ones. The AI builds them as far as
        /// its wood pays (50 a field; unit training leaves that wood alone), puts a farmer on every finished field and
        /// lays out lost fields again. 1 = on
        bool build_fields = true;
        /// Paid upgrades: on reaching an epoch the game's AI got all unit and tower upgrades of that epoch for free, on the
        /// spot. It now researches them at their buildings and pays, like a player; an upgrade the game's AI drops on the way
        /// (its building refused or was lost) is asked for again. 1 = on
        bool paid_upgrades = true;
        /// Upgrade reserve: the price of up to 4 waiting unit upgrades of units the AI fields is kept from its soldiers, so an
        /// army that spends every coin of gold still gets its upgrades - an upgrade is worth more than a few more soldiers. 1 = on
        bool upgrade_reserve = true;
        /// More granaries: the game's AI builds a single granary (8 fields), so its food stays short and the citizens it
        /// can't put on food go to gold. It asks for another one when the food workers are 6+ below the target and every
        /// field is farmed (about one per 8 wanted food workers, at most 8). The first granary comes once the food workers
        /// are 2 below the target - on water maps the game's AI builds none at all (it counts on fish). 1 = on
        bool more_granaries = true;
        /// Fishing boats: the game's AI asks for a fishing boat by epoch alone and in the late epochs for one it can't
        /// train yet (the Modern boat from the World War I age), so it stopped fishing. It trains the newest boat it has.
        /// 1 = on
        bool fishing_boat = true;
    }

    class Navy
    {
        /// Navy account: on a water map (found at the start: enemies across the sea) a share of the wood, gold and iron
        /// income is kept for warships, which the army's orders can't take, and the warships the AI asks for are paid from
        /// it instead of from the army's money. Fishing boats and the transports of a landing are paid as economy. The
        /// share scales with how many enemies live across the water (none: no navy account). 1 = on
        bool navy = true;
        /// The navy's share of the income on island maps (all enemies across the water)
        [range(0, 1)]
        share navy_share = 0.25;
        enum navy_fleet_choice
        {
            /// the game's own sea groups
            game,
            /// the fleet as described (test stage)
            own,
            /// a big fleet (default): the AI trains warships itself up to navy_fleet_pop of its population limit (the
            /// navy's share of the income rising toward navy_share_max while short), masses them under its towers while
            /// weaker than the enemy fleet (navy_fleet_mass), strikes as one force once stronger, and splits the fleet when
            /// clearly stronger into a home guard, hunters, raiders (fishing boats, coastal citizens and docks), a patrol
            /// on the sea lanes and a landing escort
            big,
        }
        /// Fleet (water maps): the AI leads its warships itself instead of the game's sea groups - it guards its fishing
        /// grounds and home waters, fights enemy ships that come near when it is not weaker, hunts the enemy fleet, its
        /// fishing boats and transports and shells its coast when clearly stronger (retreating at half strength), clears the
        /// sea route of a landing and stays off its beach, and never sails into the reach of known enemy towers, cannons and
        /// artillery. Damaged ships go back to the dock; fishing boats flee to the dock from enemy warships it can't beat:
        navy_fleet_choice navy_fleet = navy_fleet_choice.big;
        /// (navy_fleet=big) the fleet's size as a share of the population limit (15%: 26 warships at a limit of 176)
        [range(0, 1)]
        share navy_fleet_pop = 0.15;
        /// (navy_fleet=big) the navy's share of the income while the fleet is below that size
        [range(0, 1)]
        share navy_share_max = 0.5;
        /// (navy_fleet=big) mass before striking: while weaker than the enemy fleet known or below a quarter of its size the
        /// ships wait at a rally inside the reach of its towers (a tower is built by the dock if none covers the sea), fight
        /// there with the towers and strike as one force at 1.25 times the enemy's strength. 0 = stage 4 (shelter only)
        bool navy_fleet_mass = true;
        /// The game's own transports (the crossings that take citizens to mines on other islands) are economy orders like the
        /// fishing boats, not soldiers' - the soldiers' account kept the AI from ever taking another island. 0 = military
        /// (default until measured)
        bool transports_economy = false;
    }

    class WorkersAndResources
    {
        /// Water maps: gold and iron workers follow the navy's missing warships too, and from minute 15 (35+ workers) every
        /// mine slot on the home island near a drop site is worked - taken from piled-up wood, then from spare food. 1 = on
        bool navy_mines = true;
        /// Mines near a drop site: citizens mine at the free mine closest to a Town Center, Capitol or Settlement
        /// instead of the first one the game finds (often far away). 1 = on
        bool mine_choice = true;
        /// A mine counts as full when the AI's citizens walking to it or waiting beside it fill its places, not only those
        /// working there (the game counts only those): every search saw a crowded mine free and sent more - one AI had 86
        /// citizens on one gold mine while dozens of its mines had nobody. Every 10 s the citizens beyond a mine's places go to
        /// the nearest free mine of the kind. 0 = off
        bool mine_crowds = true;
        /// Mines of the citizen's own town: a citizen sent to a mine gets one near its own town (a drop site within 40
        /// tiles of it) first - the mine nearest to any drop site was often another town's, and citizens walked 200 tiles
        /// across the map and back. Another town's mine only when its own has no free one. 0 = off
        bool local_mines = false;
        /// Citizens work in their own town: the mod decides where the AI's citizens gather. A site whose drop site is more
        /// than 60 tiles from the citizen is swapped for work in its own town (the same resource, else the one shortest of
        /// workers, else wood or food), idle citizens get their town's work, mines are searched in the own town only.
        /// Another town's site only when its own has no work at all. 0 = off
        bool town_work = false;
        /// Move workers from a resource with far more workers than it needs to stone, gold or iron mines that are
        /// short (the game only ever reassigns idle citizens). 1 = on
        bool rebalance_workers = true;
        struct food_plan_fields
        {
            /// before farms the worker plan counts the food of the citizens the Town Center trains (and the
            /// wood only of real buildings)
            bool count_trained;
            /// gatherers that ended up elsewhere (after a raid, as leftovers on a mine) go back to food while
            /// it is short
            bool back_to_food;
            /// forage at the bush nearest a drop site
            bool forage_near;
            /// hunt every animal near a drop site first (newborns too)
            bool hunt_near;
            /// from the Copper Age no fields while wild food is free
            bool no_fields_wild;
            /// a Settlement by rich wild food far from the town
            bool wild_settlement;
        }
        /// Food (docs/economy.md "Food and mines").
        /// Default count_trained|back_to_food (the first epoch ~8 minutes earlier, +7-9 workers at minute 30 on Continental);
        /// hunt_near, no_fields_wild, wild_settlement cost workers or fights in the tests. Its parts:
        [on("count_trained, back_to_food")]
        food_plan_fields food_plan;
        [commented]
        int food_hunt_range = 15;
        struct mine_fill_fields
        {
            /// from the Copper Age every mine slot near a drop site gets a worker (from piled-up wood, spare
            /// food, idle citizens)
            bool fill_slots;
            /// a Settlement at the home continent's unworked mine
            bool mine_settlement;
            /// mines before wood and food
            bool mines_first;
        }
        /// Mines.
        /// Empty = off (default). All three on the large maps: kills - losses +12 to +80 a game against the base AI, its wins
        /// halved - the gold piled while the wood went short becomes an army before the war; on the small ones the war comes
        /// first and the gold lies unused: Small Islands with the cheat -6 to -7, Continental Stone Age head-to-head -12
        /// (docs/economy.md). Its parts:
        mine_fill_fields mine_fill;
        /// A Settlement by the forest its woodcutters work when 8 or more of them cut 15+ tiles from any drop site (the walk
        /// with each load decides the wood income: 54 tiles gave a fifth of the usual). 1 = on
        bool wood_drop = true;
    }

    class EconomyPlans
    {
        struct citizen_plan_fields
        {
            /// from the Copper Age the Town Centers keep training citizens up to every mine slot near a drop
            /// site + the food and wood gatherers (not only the game's 45% of the population cap), the new ones
            /// to the mines' empty slots
            bool mine_slots;
            /// food gets the gatherers that pay the citizens still missing
            bool food_pays;
            /// also a Settlement at the home continent's unworked mine
            bool mine_settlement;
            /// citizens before the epoch reserve while the food income pays both
            bool before_reserve;
        }
        /// Citizen production (docs/economy.md "Citizen production").
        /// Default mine_slots|food_pays (+15-35 workers at 60 min, more on the mines, the base AI with the cheat won 31 of 120
        /// games instead of 45); empty = the game's citizen share. Its parts:
        [on("mine_slots, food_pays")]
        citizen_plan_fields citizen_plan;
        /// the population cap's share kept for the army when the citizens' goal is larger (the home mines always fit)
        [range(0, 0.9), commented]
        share citizen_army_share = 0.4;
        /// From the Copper Age the citizens wanted are at most half the population cap and at most the stone, gold and
        /// iron mines the AI knows x the gatherers a mine takes x 1.5 (the half for food and wood): in a game of a high
        /// population cap on a small map the rest has no work (the player's rule; on his 4000-population save the AI razed
        /// about twice as many buildings with it). 1 = on
        bool citizen_mine_cap = true;
        struct workforce_plan_fields
        {
            /// the citizens wanted at most what the economy uses (mine slots, food, the wood actually spent,
            /// builders, a margin of cap / 40)
            bool what_is_used;
            /// wood's target at most the wood spent while wood piles, the rest to the mines' slots and the fields
            bool wood_spent;
            /// army_plan's soldiers into the room and more production buildings on every map from a population
            /// cap of workforce_army_cap
            bool army_room;
            /// a soldier refused for gold or iron replaced at once by the most strength per gold + iron while
            /// food or wood pile
            bool replace_refused;
        }
        /// Workforce into army (docs/economy.md "Workforce into army"), from the Copper Age.
        /// Empty = off (default): fewer citizens (what_is_used) or 10-20 more soldiers on the large maps (army_room), but no
        /// gain in the fights. Its parts:
        workforce_plan_fields workforce_plan;
        [commented]
        int workforce_army_cap = 250;
        struct army_plan_fields
        {
            /// money lying unused for 2 minutes tops up the military account
            bool top_up;
            /// soldiers trained into the population room the citizens don't need, while the account pays
            bool room_soldiers;
            /// one more Barracks / Archery Range / Stable when they train most of the time
            bool more_buildings;
            /// workers from piled-up food to the wood, gold or iron the army lacks
            bool workers_to_metals;
            /// line upgrades only for the lines in use
            bool used_lines;
            /// top_up, room_soldiers and more_buildings only from the epoch army_plan_epoch (the Middle Ages):
            /// before it the extra soldiers died under the enemy's towers (Continental from the Stone Age)
            bool from_epoch;
            /// more_buildings judges the buildings of the kind it trains from alone (an AI training only
            /// cavalry, its stables full and its barracks idle, got no stable more) and, with resources piled
            /// up, may have more of them
            bool per_kind;
        }
        /// Economy into army (docs/economy.md "Economy into army"), from the Copper Age and only with an enemy on the
        /// AI's own continent.
        /// Default top_up|room_soldiers|more_buildings|from_epoch|per_kind: with 39 Continental Large without the cheat the AI
        /// won 61 of 80 games against the base AI instead of 35; per_kind with resources piled up head-to-head soldiers +16,
        /// razed +16 a game; empty = off. Its parts:
        [on("top_up, room_soldiers, more_buildings, from_epoch, per_kind")]
        army_plan_fields army_plan;
        /// Outdated units attack: units of a type the AI's epoch has left behind (no upgrade reaches them any more) fill its
        /// population cap. Every 5 minutes, once 8 or more such land units stand free, they attack the nearest enemy building
        /// together (falling back as every attack does) and do some damage before they are completely outclassed. 1 = on
        bool outdated_attack = true;
        [commented]
        int army_plan_epoch = 5;
        [commented]
        int army_float_keep = 300;
        [commented]
        int army_float_keep_mineral = 100;
        [commented]
        int army_mix_food = 500;
        struct metal_plan_fields
        {
            /// a land soldier the game asks for is trained as the one that does most per gold + iron against
            /// the enemy army the AI has seen (the game's counters), when that is 25% better
            bool best_per_metal;
            /// no saving for a unit whose gold or iron won't come within substitute_wait
            bool no_waiting;
            /// (with best_per_metal) food, wood and stone lying unused count little in the price
            bool surplus_cheap;
            /// the fleet's size at most metal_fleet_pct of the enemy warships seen + 6 (at least
            /// metal_fleet_min) instead of 15% of the population cap
            bool fleet_by_enemy;
            /// while the sea is calm the navy account keeps 100 gold and 100 iron at most
            bool calm_navy_cap;
        }
        /// Gold and iron (docs/economy.md "Gold and iron"), only from a population cap of metal_cap.
        /// Empty = off (default): on the large island maps fleet_by_enemy (+calm_navy_cap) gave 60-90 more soldiers trained and
        /// more kills - losses without the cheat, but against the cheating base AI the kills - losses fell on two maps;
        /// best_per_metal cost wins on small Continental. Its parts:
        metal_plan_fields metal_plan;
        [commented]
        int metal_cap = 250;
        [commented]
        int metal_fleet_pct = 250;
        [commented]
        int metal_fleet_min = 20;
    }

    class TheArmy
    {
        struct army_mass_fields
        {
            /// reinforcements for an attack that is out gather at a rally point by the towers and go as a group
            /// (5 together, 3 after 2 minutes)
            bool rally;
            /// the attack waits (10 minutes at most) while the land army is weaker than 1.3 x the strongest
            /// enemy army it knows
            bool wait_stronger;
            /// (with rally) only while that attack fights
            bool only_fighting;
            /// a member of an attack out picks no target more than army_mass_tether (12) tiles from its pack
            bool tether;
            /// a member out far from its pack and idle walks back to it
            bool walk_back;
            /// (with rally) the released reinforcements walk to their attack's pack
            bool to_pack;
            /// (with tether) the tether only for buildings
            bool only_buildings;
        }
        /// The army as one (docs/economy.md "The army as one"), only with an enemy on the AI's own continent.
        /// Empty = off (default): rally lost fewer soldiers and killed more in head-to-head games and against the base AI
        /// without the cheat, but against the cheating base AI it killed fewer (kills - losses -10) and razed less; tether the
        /// same (kills -14 against the cheat); tether|only_buildings neutral everywhere. Its parts:
        army_mass_fields army_mass;
        [commented]
        int army_mass_group = 5;
        [commented]
        int army_mass_hold = 120;
        [range(0, 10), commented]
        share army_mass_ratio = 1.3;
        [commented]
        int army_mass_wait = 10;
        [commented]
        int army_mass_tether = 12;
        /// The staging point (with danger_routes): an attack's march from safety into the enemy's reach or a lost fight goes to
        /// one gathering point outside every danger on the way from there to the base; the army waits there and goes in
        /// together once its fighters there beat the enemy at the destination by army_stage_go. 0 = off
        bool army_stage = true;
        /// The mod's armies (land maps): the AI's land soldiers belong to the mod's army instead of the game's attack
        /// commander - held out of its reach as they are trained, one force at its front station that goes at the attack
        /// time as one when it beats the enemy at the target, takes the next while it wins and comes home as one when it
        /// is losing; a raid near home is answered by the whole force. Measured against the commander on four setups
        /// (README): fewer losses, more razed, far fewer buildings lost. 0 = the game's commander
        bool mod_armies = true;
        /// ... on every map, islands too (the player 2026-11-10: no map types): the force fights what it can reach over land,
        /// and an enemy only across the water is the landing's - its soldiers taken from the force at home, given back
        /// once they are home again. Off: on island maps the game's commander keeps the land soldiers and the landing
        /// takes its groups. On by the player's call (2026-11-11), measured level or a little better (README)
        bool mod_armies_sea = true;
        /// EVERY UNIT THE MOD'S (the player 2026-11-13: "i want every unit under our control ... its ok if things break at
        /// first, at least we see the holes"; docs/game-ai-routines.md "Who decides in play"): the land soldiers in the
        /// game's own attack groups taken into the force too, and the game's group requests train no land soldiers - the
        /// force's own orders do. 0 = the force takes only free soldiers, the game's requests train for its groups
        bool own_soldiers = true;
        /// ... the warships on every map led by the fleet, a land map's too (else only where the water matters), none of
        /// them taken into the game's groups, and the game's requests train none: the fleet orders them - up to its size
        /// target where the enemy is across the water, elsewhere only against enemy warships seen in its own sea
        /// (fleet.c EVERY WARSHIP THE MOD'S). 0 = the game's groups lead a land map's warships, its requests train them
        bool own_ships = true;
        /// ... the citizens ordered by the mod: how many from citizen_plan (the game's share of the cap as its floor), the
        /// orders its own, every 2 s, the game's citizen request left with nothing to ask for. Against the game's
        /// request: level, from epoch 9 and from the start (econ.c EVERY UNIT THE MOD'S). The fishing boats too: one
        /// while food is short, fish is left and no boat stands idle, none after a boat had to flee an enemy warship,
        /// paid before the wood savings (fleet.c THE FISHING BOATS THE MOD'S). 1 = on
        bool own_citizens = true;
        /// ... a raid is answered only when the force beats it by this much (the player: he follows raiders only with soldiers
        /// to spare; 100% = an even fight, as before)
        [range(1, 5), commented]
        share raid_answer = 1;
        /// ... on a raid, a member this many tiles past the raid's spot is called home instead of chasing raiders that ride
        /// off (the player calls back the soldiers that followed on their own). 0 = off (default until measured)
        [range(0, 60), commented]
        int raid_leash = 0;
        /// ... a raid on an ally's buildings within this many tiles of the force's home is answered as one on its own, by the
        /// same rules (the game's aid to allies finds no land soldiers: the force holds them). 0 = off (default until measured)
        [range(0, 1000), commented]
        int ally_aid = 0;
        /// ... a raid is answered by a part of the force, the soldiers nearest it that beat it two to one, an army of its own
        /// that comes back once the raid is over; the rest stays home for the next raid and the attack time. The whole force
        /// goes only when no part is enough. 0 = the whole force. On since 2026-11-11: against itself (20 games) it razed 17
        /// more buildings a game, nothing worse beyond the noise (armies.c RAID PARTIES)
        bool raid_parties = true;
        /// What the army trains to fill the room (army_plan FILL): the land unit that adds most to the force against the enemy it
        /// will meet - the defenders and towers at its next target, the army it expects each enemy there to field (the most it
        /// has seen of it, fading) and the counters that enemy will train against what it saw of the AI's -, by the fight
        /// estimate, for what it costs, siege and rams included; nothing known of the enemy: as before. 0 = the game's wish, else
        /// the strongest (default on since 2026-11-11, measured with scouting: more soldiers, kills and buildings razed)
        bool train_by_need = true;
        /// A rider of the army looks at the land near the enemy's buildings it hasn't looked at for 3 minutes, when it hasn't
        /// seen an enemy army for that long: what it sees, it remembers - for training by need, among others (default on since
        /// 2026-11-11, measured with train_by_need)
        bool scouting = true;
        /// The warship the navy trains: the one that adds most to its fleet against the enemy's fleet as it expects it - the
        /// ships it has seen each enemy field and the counters that enemy will train against what it saw of the AI's ships -
        /// for what it costs, when it can pay it now; nothing expected at sea: the navy's mix as before (default off: measured
        /// without a gain over the mix)
        bool fleet_by_need = false;
        /// The front army: between attacks the idle soldiers stand at the AI's heaviest front (and a second one weighing at
        /// least half of it), at its tower nearest the front, instead of idling at home; the attacks take them from there.
        /// 1 = on, being measured
        bool front_army = false;
        [range(0, 10), commented]
        share army_stage_go = 1.3;
        /// The attack commits (with army_stage): no squad goes past the staging point alone - the whole wave gathers there and
        /// goes in together, fighting on the way (an attack-move), and for 2.5 minutes none of its units falls back. 1 = on
        bool attack_commit = true;
        /// One target a wave: an attack on a player has no target, so the game splits the wave into squads of about 10 that
        /// each walk their own legs; with this the wave goes to one building of that player the AI has seen, then the next
        /// nearest, all its land squads together. 1 = on (neutral in AI-vs-AI tests: off)
        bool wave_focus = false;
        struct fall_back_fields
        {
            /// a land unit worth fall_back_value (resources) or more below fall_back_hp of its hit points walks to
            /// the nearest own Hospital on its land (fall_back_dist tiles at most), heals there to 90% (4 minutes at
            /// most) and rejoins
            bool hospital;
            /// a land unit whose fight is clearly lost - the fight estimate with what the AI sees (its fighters, the
            /// enemy's, the enemy towers known) below fall_back_ratio - falls back under the nearest own tower (else
            /// a Hospital, the base), 20 s there, then its attack has it again
            bool tower_cover;
            /// the fleet retreats under its towers instead of to open water, and a warship below half its hit
            /// points breaks off to a dock (one an own tower covers first) until 80%
            bool fleet_cover;
            /// (experimental, off) a unit that broke off from the same attack fall_back_breaks times gives it up
            /// (stays at the cover until that attack is over), a unit in a lost fight with no own cover near falls
            /// back toward home, and an attack most of whose units (fall_back_end) gave up, not winning where it
            /// stands, is called off - the next comes within 5 minutes
            bool give_up;
        }
        /// Fall back to cover (docs/economy.md "Fall back to cover"), per unit (an attack is never called back).
        /// Default hospital|tower_cover|fleet_cover: against the base AI on six maps (40 games each, both cheats) kills - losses
        /// +2 to +56 a game (one map -5), fewer losses, more soldiers and warships alive; head-to-head +12 to +28. Empty = off.
        /// All four against the default (16 cells, head-to-head): no clear gain, and give_up's call-off can crash the game.
        /// Its parts:
        [on("hospital, tower_cover, fleet_cover")]
        fall_back_fields fall_back;
        [range(0, 10), commented]
        share fall_back_ratio = 0.5;
        [range(0, 1), commented]
        share fall_back_hp = 0.4;
        [commented]
        int fall_back_value = 100;
        [commented]
        int fall_back_dist = 60;
        [commented]
        int fall_back_breaks = 2;
        [range(0, 1), commented]
        share fall_back_end = 0.5;
    }

    class OtherIslands
    {
        struct expand_islands_fields
        {
            /// once the home island's mines are half worked (or from minute 15), a transport takes
            /// expand_citizens citizens and expand_guard soldiers to the nearest free island with mines (no
            /// town, no defence and few buildings of an enemy there), they build a Settlement, two towers and
            /// work the mines; more citizens follow (expand_trips trips in all); given up after 3 losses
            bool outposts;
            /// a barracks there too
            bool barracks;
            /// islands with an ally's buildings count as free
            bool ally_islands;
            /// from expand_minute without waiting for the home mines, its transport before every reserve
            bool early;
            /// the citizens it ships are additional (the citizen plan trains them, wood cutters go first)
            bool extra_citizens;
            /// more islands one after the other, each only once the last holds (expand_max at most, the nearest
            /// to what it holds)
            bool more_islands;
            /// an outpost nearer the enemy than home gets a barracks and two more towers while the AI holds the
            /// sea (no enemy warship seen in its waters for 5 minutes, its warships at least the enemy's it knows)
            bool forward_outpost;
            /// the outpost follows the sea and the army: planned and supplied only while the AI holds the sea
            /// and its army is 1.5 x the enemy's it knows, a guard of 4, four towers, the idle citizens board
            /// first (docs/economy.md "More metal")
            bool hold_sea;
            /// the outpost keeps what it claims (the player: "the ai must invest in keeping what it claims"): a fort round its
            /// nearest mine (a tower at each of the four corners, rebuilt when lost and never given up for want of money, walls
            /// on the three sides away from its drop site), a garrison that grows with the citizens there (one soldier per
            /// three, 8 at least with enemies seen in the last 5 minutes, 12 at most) brought by boats of soldiers only, and an
            /// outpost lost is tried again after 15 minutes, once
            bool fort;
            /// with shore_guns: enemy warships near an outpost make its landing shore a battery like a dock's - a Hospital by it
            /// and the guns, trained at home and brought by a boat of their own (the warships killed half the citizens lost at the
            /// outposts where the mod had no guns there)
            bool fort_guns;
            /// the landing as a player does it: a tower by the landing first, every citizen on it, the guard on to the mines;
            /// the site's tower and the Settlement once it has looked there and found nobody; the transport waits by the shore
            /// and takes them off again when enemies there win before a tower stands (2026-11-14, 24 games on island maps:
            /// level with the landing without it - soldiers +5 and 0, fewer outposts lost on a large map, 8 of 22 landings
            /// taken off again; in the default by the player's call, his pattern)
            bool beachhead;
        }
        /// Outposts on other islands (docs/economy.md "Other islands"), water maps only (needs landing=1); the parts from
        /// early on are for the large maps (docs/economy.md "Large island maps").
        /// Islands it has hardly looked at are scouted first by the empty transport. Default outposts, extra_citizens,
        /// more_islands, fort, beachhead (beachhead since 2026-11-14; the others 2026-11-13: with the game's own ferrying of citizens kept, soldiers +9 to +19 from epoch 9 on
        /// Small Islands and a large one, level from the start; expand.c THE OUTPOST). Empty = off. Earlier, measured against the
        /// base AI (outposts|ally_islands|early|extra_citizens|more_islands and more on Large / Huge maps, unit limit 1500) the
        /// outposts went up from minute 25-30 and the enemy's soldiers on the home island fell, but wins, kills - losses and the
        /// miners at 60 minutes moved both ways between the maps. Against the game's own ferrying of citizens (2026-11-13, in its
        /// place: outposts|extra_citizens|more_islands) level on Small Islands and behind on a large one - an outpost that holds
        /// is worth 10-25 soldiers, one that is lost costs as much, and half are lost (econ.c THE TRANSPORTS THE MOD'S). Its parts:
        [on("outposts, extra_citizens, more_islands, fort, beachhead")]
        expand_islands_fields expand_islands;
        [commented]
        int expand_citizens = 8;
        [commented]
        int expand_guard = 3;
        [commented]
        int expand_minute = 8;
        [commented]
        int expand_trips = 4;
        [commented]
        int expand_max = 3;
        /// Idle citizens stranded on land without a drop site of their AI (a transport put them off there) are fetched home by
        /// one of its transports - the game gives a citizen work only on its own land. 0 = off
        bool rescue_citizens = true;
        struct towns_fields
        {
            /// a resource town once the mines near its drop sites are full (fewer than 4 free places) and its
            /// army at least the enemy's it knows - the richest explored mine group no other drop site is within 18 tiles
            /// of, none of its own within 32, no enemy building it knows within 22, 32-90 tiles from its base: a Town
            /// Center (a Settlement when it can't pay one) and towers_per_town towers on the enemy's side; towns_citizens
            /// of its nearest citizens (never its miners) build it and then gather at its mines
            bool resource_towns;
            /// the drop site a Settlement, never a Town Center
            bool settlement;
            /// army_plan's next production building goes up in the town nearest the enemy when it is 15+
            /// tiles nearer than the base
            bool forward_production;
            /// a forward base where its army is (its staging point, else its biggest group in the field, just out
            /// of the enemy's reach), 32+ tiles out (its army 0.8 x the enemy's): 8 citizens put up two towers, a Hospital,
            /// a production building, two more towers and a wall in front; then they go back - measured even, off
            bool forward_base;
            /// (with resource_towns) a town at the explored resources nearest to its towns, however far, without
            /// waiting for the home mines to fill - as many as it has the people for (15 workers a town; towns_max doesn't
            /// apply), one going up per 50 workers, only with a surplus (6 spare citizens, twice a Town Center's price in
            /// stock); within 45 tiles of a town where it can, never more than 20 tiles ahead of its front town; 2 towers
            /// and 1 per 4 mines, 5 houses, a wall 60+ tiles out. On (the player 2026-10-31: the AI must expand; 1v1 huge
            /// Continental 120 min, 24 games: 5 towns founded, even with off)
            bool spread;
            /// (with spread) a scout to unseen land - measured a cost, off
            bool scout;
            /// (with spread) with wood and stone to spare its towns grow (towers, Hospital, Temple, University,
            /// production) - measured worse, off
            bool grow;
            /// a resource town keeps what it claims while its continent isn't fully claimed (an enemy building known on it, or a
            /// mine no drop site is near - the player: "if an island/region isnt fully claimed yet the ai should reinforce its
            /// outposts"): at least four towers, on the corners of a square round the mine nearest its drop site, rebuilt up to 8
            /// times and never dropped for want of money, walls on the three sides away from the drop site, and a garrison on foot
            /// (one soldier per three citizens, 3 at least, 8 with enemies seen there in the last 5 minutes, 12 at most, a quarter
            /// of its soldiers over all its towns)
            bool fort;
            /// the force's pushes go for the enemy drop sites it knows beside mines first, and a drop site razed with no other enemy
            /// building it knows near becomes its own town (the player: "attacking these mines and fortefing them after is an
            /// important step, it gives more resouces and starves the enemy"): a drop site and four towers on the corners of a
            /// square round the mine, walls, rebuilt as a fort's; the force stays until the drop site and two towers stand (4 minutes at most)
            bool take_mines;
            /// a detachment of up to 20 soldiers from those at home, apart from the force, goes for the enemy drop site by a mine
            /// it knows that it beats best and is nearest, six citizens behind it; it clears what else stands within 22 tiles, founds
            /// a town there as take_mines does and stays until its drop site and towers stand (the player: "small detachments that
            /// stay until the mine is secured"), 15 minutes at most; one at a time, 3 minutes apart (not together with take_mines).
            /// Behind the enemy, or with too few at home or no mine it would beat: a hit and run instead, 10 to 12 cavalry (mounted archers
            /// first, trained for it) at the least defended mine no tower it knows covers, no citizens along; they kill the citizens there and
            /// leave; a mine towers cover is asked of the main army
            bool mine_raids;
            /// a new town founded as a player does it: its citizens walk with an escort of 3 soldiers; where they cross into the
            /// site's territory a tower first, every citizen on it, the escort on to the site; once that has been looked at
            /// and nobody is there a tower at the site, then the drop site; home again when enemies by them win before a tower
            /// of theirs stands (2026-11-14, 24 games on Continental maps: as many towns standing, none saved - AI towns
            /// are hardly ever lost while founded - and the drop site a minute and a half later; on by the player's call,
            /// his pattern)
            bool party;
        }
        /// Towns on the home continent, a list of parts (apart by commas or '|'), not before the Copper Age, not with
        /// enemies in its base; empty = none:
        [on("resource_towns, spread, party")]
        towns_fields towns;
        [range(0, 6), commented]
        int towns_max = 3;
        [range(1, 12), commented]
        int towns_citizens = 6;
        [range(0, 7), commented]
        int towers_per_town = 2;
        /// A town still going up whose citizens are all dead is given up and its unfinished buildings are removed
        /// (refunded) - left standing, the game's repair round kept sending citizens to them. 0 = off (default)
        bool towns_remove_unbuilt = false;
        /// (with spread) Towns with sense, however far: no spot with enemy soldiers it knows near it, in the
        /// enemy's reach or where it lost a fight, and none whose way from its nearest town passes enemy soldiers,
        /// towers or buildings. 1 = on (default): 1v1 huge Continental, 96 games, losses -18, soldiers +5
        bool towns_care = true;
    }

    class ResearchAndDefence
    {
        /// Research the gathering upgrades (Hand Axe, Hafts, ...) as soon as they are available - the game's AI
        /// rarely gets to them. 1 = on
        bool gather_research = true;
        /// Defences like a player: towers in a line facing the nearest known enemy (and at outlying settlements)
        /// instead of scattered, extra towers while stone allows, and a wall in front of the towers. 1 = on
        bool defence_layout = true;
        bool defence_towers = true;
        /// (defence_walls: the wall in front of the towers - straight, with a gate in its middle)
        bool defence_walls = true;
        /// The game's own walls: the ring around the base its wall manager builds a segment at a time (in practice a stray
        /// piece with a gate) - its builders walk to the front one by one. 0 = off (the mod's AIs build none), 1 = as the game
        bool game_walls = false;
        /// A closed ring of walls around the town instead of a tower line with a wall in front (the player: such a line is
        /// walked around, "a propper ring with gates and towers around the base"): eight straight sides a few tiles beyond
        /// the town's buildings and the mines worked near them, the sea, cliffs and forest closing it where they stand,
        /// a gate in the middle of each side, towers just inside - beside the gates first. A side the town grows up to
        /// moves out, the old stretch stays as an inner wall with its gate. Built once enclose_citizens work. 0 = off
        /// (default until measured)
        bool base_ring = false;
        /// Towers first where the citizens work by the sea (the player: towers defend the resources inland and sink ships
        /// on the coast): on an island map each tower goes to the busiest place of work within a ship's reach of the sea
        /// that has fewer than this many towers by it, near the shore; the usual tower line after those. 0 = off (default
        /// until measured)
        [range(0, 4)]
        int coast_towers = 0;
        /// Forts round the mines (the player's picture: a mine in a small square, a tower at each corner, walls between them,
        /// one side open for the citizens): up to this many, the busiest mines first - those in a ship's reach of the sea or
        /// where citizens were hurt before the others. Towers first, then the walls of the three sides away from the town;
        /// the side towards the town stays open. 0 = off (default until measured)
        [range(0, 6)]
        int site_forts = 0;
        /// Siege weapons or artillery by each dock enemy warships came near (the player: "trebuchets or cannons near the docks,
        /// they reach further than the towers and have a bonus against ships"): this many a dock, the longest reach it can
        /// train, posted on the shore on the ships' side. 0 = off (default until measured)
        [range(0, 8)]
        int shore_guns = 0;
        /// With shore_guns: wall pieces on the coast in front of each battery once its Hospital stands - they stop the
        /// ships' flat shots and a landing, the catapults and trebuchets fire over them (the player: "maybe they work
        /// better behind a wall"); the coast by a dock stays open. 0 = no wall (default until measured)
        bool shore_walls = false;
        /// Enclose its island fully (the player: a tower line with a wall is walked around): on an island of its own a
        /// wall along the whole coast at the water's edge, so transports find nowhere to unload, and towers packed along
        /// it; open by the docks. 0 = off (default until measured)
        bool enclose = false;
        /// With enclose or base_ring: the ring only once the AI has this many citizens, the usual towers before (with
        /// enclose the front walls too) - the economy first (the ring from the start took most of the citizens off
        /// gathering: every Small Islands game lost)
        [range(0, 400)]
        int enclose_citizens = 40;
        /// With enclose or base_ring: runs of wall (up to 12 pieces each) ordered every 20 s
        [range(1, 4)]
        int enclose_runs = 1;
        /// With enclose or base_ring: a new run of wall only while fewer than this many ordered stand unfinished - a few builders at a
        /// time, so the ring can start early (0 = no limit)
        [range(0, 8)]
        int enclose_in_flight = 0;
        /// With enclose, on an island: only enemy land units near keep the builders off a part of the coast wall, not ships
        /// (the coast is where the ships are; a piece that stands stops their fire). 0 = ships too
        bool enclose_under_ships = false;
        /// With enclose, on an island: the coast by each own dock is left open 4 tiles each way (the dock's ships and room to
        /// board beside it) and gets a short wall of its own on the land side with a gate towards the town - a landing there
        /// meets the gate. 0 = 2 tiles open, no wall
        bool enclose_dock_gates = false;
        /// With enclose, on its own land in the sea: no wall round the coast when at least this many of its tiles lie
        /// beyond a ship's reach of the sea (9 tiles) - room enough to build and work safe without walling itself in
        /// (the player: a small island needs the wall, a large one has safe space inland). 0 = the coast always
        [range(0, 5000)]
        int enclose_safe_tiles = 0;
        /// tiles from the base centre to the tower line
        [commented]
        int defence_distance = 10;
        /// Soldiers near the base attack enemy siege weapons shelling it - only when clearly stronger than the escort.
        /// 1 = on
        bool counter_siege = true;
        /// Soldiers shelled from out of their reach (mortars, artillery, snipers, a tower further off than they shoot): the
        /// AI's soldiers within 24 tiles go after the shooters together - only as many as clearly beat the shooters and the
        /// enemy's fighters beside them, and nobody when they would not. 1 = on
        bool counter_fire = true;
        /// Anti-air: when the AI sees enemy aircraft over its towns, it builds AA guns next to them and trains AA units
        /// (2 + half the aircraft seen, at most 16; replaced when lost). 1 = on
        bool anti_air = true;
        /// ... and ahead of time: from World War I, once an enemy is there too, AA guns at its main drop sites and 2 AA units
        /// before any aircraft come (getting them up while the town is bombed is too late); aircraft seen anywhere count.
        /// 1 = on; aa_ahead_buildings = how many drop sites get one
        bool aa_ahead = true;
        int aa_ahead_buildings = 3;
        /// ... and by the air force it expects: half the aircraft it has seen each enemy field (the most at once, fading) and
        /// of those that enemy will train against what it saw of the AI's units count as aircraft seen (default off: not
        /// measured where aircraft fly yet)
        bool aa_expected = false;
        /// Strong anti-air, in place of the counts above: the threat weighed (a bomber counts as 2-3 fighters, an atomic
        /// bomber as 6) from the aircraft seen, the air force it expects of its enemies and, before any are seen, an enemy
        /// in World War I or with an airport; blocks of AA buildings - 2, or 4 in a 2x2 when the threat is real - at its
        /// Town Centers first, then its drop sites, one more place per 6 of the threat; AA units 2 + the threat, up to a
        /// quarter of its unit limit (default off until measured)
        bool aa_strong = false;
        /// Hold the town (experimental): when a stronger enemy army camps near an AI town, its soldiers pull back behind the
        /// town and guard there instead of chasing it one by one (Guard behaviour), and strike together once they are 1.3x
        /// its strength. Tests against a camp that pushes and pulls back like a player: worse (the town is left to the
        /// pushes - fewer citizens, Town Center lost more often). 1 = on, 0 = off (default)
        bool hold_town = false;
    }

    class Training
    {
        /// When the AI asks for a soldier it can't pay for or train (heroes, units of an earlier age, gold units without
        /// gold), it trains the strongest soldier it can afford instead (never for a ship, transport or fishing boat). 1 = on
        bool unit_substitution = true;
        /// ... but a unit it can train and only can't pay yet is saved for first: this many seconds without a requested
        /// soldier before it trains a substitute (at once with enemy soldiers in its base). 0 = substitute at once
        int substitute_wait = 90;
        /// The mod's own training orders (AA units, siege, home guard, transports) only up to the room under the population
        /// cap - at the cap they were paid for and stuck in the queue, blocking the AI's own training. A mask of players
        /// (65535 = all, 0 = off)
        int train_room = 65535;
        /// Special orders (shore guns, anti-air, the attack's siege) are saved up for: while one waits, the cost of its next
        /// unit is held from the ordinary soldiers' and warships' orders and counted in what the citizens gather (the
        /// player: "sometimes you have special orders that need saving up"). 0 = they take what is left over (default
        /// until measured)
        bool special_orders = false;
        /// Spending tiers, step 1 (the player: "a lower tier only gets produced if the resources can pay for the higher
        /// tiers"): what an outpost on another island waits for - its transport, its drop site and first tower - is the top
        /// tier: its transport sees the whole stock, and no building and no warship with a target spends what it needs. Without
        /// expand_islands nothing is held. 0 = buildings spend what comes in, first come first served (default until measured)
        bool spend_tiers = false;
        /// The production buildings go side by side near the base instead of each with an empty tile round it (the
        /// game's search, used when no spot fits), never where they would cut a way through. 0 = the game's search
        bool tight_build = true;
        /// The first two Granaries and the 8 fields of each are an entry of the priority list, one at a time: from the start of the
        /// Copper Age's research their wood is held back from soldiers and from the buildings that aren't food, houses, towns
        /// or docks, and the second follows when the first's fields are built and worked. 0 = they get what the others leave
        bool farm_first = true;
        /// The units in production the room for training leaves out are set to what the buildings really have queued: a loaded
        /// game starts that count at 0 (the save doesn't keep it) and the AI queued far more than its limit lets out. Off: on
        /// the player's save "standing army" its AIs lost more units with it (101 against 82 in 15 minutes). 1 = on
        bool production_sync = false;
        bool catch_up_ais = true;
        /// Units each finished production building may hold (the one in the making and its queue): the game queued every
        /// request however deep the queues were - 80 soldiers in one building are resources tied up for 40 minutes. A request
        /// is cut to what keeps each producer of the unit at this many; the resources stay free for more production buildings.
        /// 0 = no limit, as the original
        [range(0, 100)]
        int train_queue = 3;
        struct late_targets_fields
        {
            /// wood gets the workers that close its gap (what the AI wants to build and train - ships too -
            /// beyond its stock) within about 2 minutes, and no more: a pile of wood buys nothing
            bool wood_gap;
            /// surplus workers are moved to wood too
            bool surplus_to_wood;
            /// workers the farms can't take go to stone, gold and iron (the army's metals) instead of piling
            /// wood or food
            bool metals;
            /// while 4000+ food lie beyond the age reserve, farmers move to the metals (fields stand idle; off by
            /// default: a bigger army in a long game, but in AI-only tests the AIs survived less often, 70% vs 78%)
            bool food_to_metals;
            /// when the army's orders fail for gold/iron/stone (more than for food) and food is above its
            /// reserve, farmers move to the metals, 2 every 10 s (back 4 at a time when food runs short) - the
            /// game gives gold and iron 1-6% of the workers next to its phantom food demand (off by default
            /// until tested)
            bool metal_pull;
        }
        /// Wood after the Copper Age: the game's food demand never shrinks, so wood got 1-3 workers and nothing was built.
        /// Default wood_gap|surplus_to_wood|metals, all = every part, empty = none (the original). Its parts:
        [on("wood_gap, surplus_to_wood, metals")]
        late_targets_fields late_targets;
        /// Experimental: no new citizens while food is short and more than this many (and 15% of all) stand idle - stalled
        /// the growth in tests. 9999 = off (default)
        [commented]
        int idle_citizen_slack = 9999;
    }

    class HomeGuard
    {
        /// Home guard: the AI keeps 6 + this many soldiers per age at home (it only trained for its next attack). 0 = off
        int home_guard = 4;
        /// ... and at least this share of its room for soldiers (population cap x (1 - citizen share)) - grows with the cap
        [range(0, 1)]
        share home_guard_share = 0.35;
        /// Muster: when an enemy army in the base is hopeless to fight (at least 8 soldiers, twice the strength of the AI's
        /// soldiers and towers there), the AI no longer feeds its soldiers in one by one: they gather 18 tiles beyond the
        /// base and strike together once 1.3 x as strong; citizens next to the enemy flee there too (around the enemy, not
        /// through it). 0 = off
        bool muster = true;
        /// Citizen attacks: an AI that can't train soldiers still launched its attacks on time - with citizens, who walked
        /// into the enemy's army and towers. An attack group of at least half citizens is called off. 1 = allowed (original)
        bool citizen_attacks = false;
    }

    class Diplomacy
    {
        /// Answer war: whoever declares war on an AI that is allied with them gets war back a few seconds later, with the
        /// stance line (stance_lines), a revenge attack and a taunt (the original AI has no diplomacy and stays allied). 1 = on
        bool answer_war = true;
        enum stance_lines_choice
        {
            /// none
            none,
            /// only an AI's stance changes towards you, naming it ("<AI> is now your enemy / ally")
            yours,
            /// the game's line for every AI stance change, also between AIs (it doesn't say towards whom)
            all,
        }
        /// Stance lines:
        stance_lines_choice stance_lines = stance_lines_choice.yours;
        /// AI diplomacy: AIs judge peace offers (opinion, power, grudge, tribute - per personality), offer alliances against
        /// a much stronger enemy and may betray a weak ally (disloyal personalities). Not in scenarios or with locked teams.
        /// An offer to you - an alliance, peace, a capitulation - is only a message that the AI would take peace now: it
        /// fights on until you set it to allied, whenever that is, and withdraws the offer once its reason is gone (the
        /// common enemy isn't one any more, it holds its own again). An AI that is clearly losing to an attacker offers
        /// peace and tribute (capitulation; the tribute is paid as far as its stock still has it). A vassal (capitulated,
        /// still allied with its lord) has no diplomacy of its own: it sends peace offers to its lord and makes none
        /// itself - a disloyal, proud one rebels once it is clearly stronger than its lord.
        /// Breaking an alliance costs reputation with every AI (by its loyalty; attacking
        /// one's own vassal x3, right after accepting its capitulation x5). Peace offers are always judged; 0 switches off
        /// only the AIs' own initiative
        bool ai_diplomacy = true;
        /// Alliance blocks (with ai_diplomacy): everyone linked by alliances is one block, and inside a block there is peace -
        /// two AIs allied with the same player make peace with each other; an AI at war with you that joins your block
        /// offers you peace (it stands while you are both in the block). Whoever turns on a member splits the block: every
        /// other AI takes a side (the one it capitulated to, else its direct ally, else the one it likes better - the
        /// attacked a bit more) and a
        /// traitor betrays the victim's whole block. Joining means peace with the whole other block: an AI refuses when it
        /// or one of its allies holds a grudge (a war declared recently, opinion below -30) against a member of the other
        /// side, and names who stands in the way. A vassal has no say when its lord is part of the deal (it grumbles).
        /// 1 = on, 0 = off (alliances only between two players)
        int alliance_blocks = 1;
        /// Surrender: an AI that has lost all its citizens and can't train new ones (no Town Center or nothing to pay with)
        /// resigns after a minute - or after 3 minutes without citizens in any case - instead of leaving its last buildings
        /// to be hunted down. Not while its army still has half the strength of its strongest enemy; not in scenarios. 1 = on
        bool surrender = true;
        /// The spoils of a resign: a player out of the game is at war with everyone; its land fighters roam in bands, hunting
        /// units and ignoring buildings, and 30 s later its buildings go to those who did 20% or more of the damage it took in
        /// its last 10 minutes, shared by that damage (nobody with 20%: they stay). Not in scenarios. 1 = on
        bool resign_spoils = true;
    }

    class LandingsAndTransports
    {
        ///
        /// Landings: the AI plans a landing behind your lines across the sea - the weakest defended beach next to your
        /// buildings, warships ahead, its transports gather and run in together, a bridgehead with towers and a forward base,
        /// then waves. landing_transports = the most transports (the size follows the beach), landing_citizens = citizens
        /// for the forward base, landing_waves = waves after the first. 1 = on
        bool landing = true;
        int landing_transports = 8;
        int landing_citizens = 10;
        int landing_waves = 3;
        /// Experimental: a beach too strong for the army it can send is shelled first by a fleet of warships (trained for it,
        /// 4-16), then the landing comes. 0 = off (default)
        [commented]
        bool landing_bombard = false;
        /// Air transport (the player 2026-11-13: "expansion and landings should also include (and prefer) air transport in
        /// later ages"): where an Airport stands, a landing and an outpost order transport helicopters before ships and take
        /// the ones they have first - no dock, no sea to hold, ashore in seconds; with a helicopter to be had and no
        /// Airport, one is wanted through the priority list while a landing or an outpost waits for its transport.
        /// 0 = ships only
        bool air_transport = true;
        /// The game's transport manager takes a landing's transport for its own crossings (an unload order in any phase):
        /// refused. 1 = on (default), 0 = the game may take them
        [commented]
        bool landing_keep_transports = true;
        /// A transport waiting at the shore for passengers that will never come (they stand idle without an order, or went
        /// off on another one) lets go of them after 30 s and sails with those aboard. 0 = off (default: in tests the
        /// transports waited far less, but the games went no better)
        [commented]
        bool transport_watchdog = false;
        /// (tests only: landing_flag, landing_knows_map, landing_trace, transport_log)
        ///
        /// --- Experimental (off by default) ---
        /// The defence decisions (muster, hold the town, militia) compare sides with the fight estimate
        /// (range, minimum range, area damage, the game's damage tables) instead of hit points x attack - no gain in tests
        bool fight_estimate = false;
    }

    class AdvancedTuning
    {
        ///
        /// --- advanced tuning (same for all difficulties) ---
        /// workers the AI grows to before it starts saving for the early epochs
        int min_workforce = 40;
        /// Recovery after the Copper Age: while the workforce is under min_workforce, two thirds of the workers gather food,
        /// the citizens may spend the whole stock and no soldiers are trained unless enemies are in the base - an AI with 3
        /// citizens and finished fields left empty spent the food it got on soldiers and never rebuilt. 0 = off
        bool recover_economy = true;
        /// Epoch requirements: besides the cost, an epoch needs two "new things" of the current one (a new unit type,
        /// building or tech). The AI asked for them only once it could pay, and often picked unbuildable decoration
        /// (statues) - it waited 4-14 minutes. 1 = ask early, never decoration, requirement units past the army budget
        bool early_requirements = true;
        /// Reserve plan (from the Copper Age on): hold back for the next epoch only what can't be gathered again in time -
        /// food spent now while gold and iron are still missing grows back on the farms. 1 = on
        bool reserve_plan = true;
        /// before the Copper Age: citizens on stone, gold and iron only for the epoch costs + this much for the army
        /// (else they sat on 1000 iron while the Stone Age waited for food); -1 = follow the army's demand
        int mineral_worker_margin = 300;
        /// share of the wild food it has found (berries) the AI counts on gathering
        [range(0, 1)]
        share wild_food_share = 0.6;
        /// food above the epoch reserve + this amount may always go into the army (no endless hoarding)
        int hoard_cap = 3000;
        /// the same for stone, gold and iron (hardly any civilian use besides the epoch cost, which is reserved anyway) -
        /// 600 kept ~600 iron lying while the army failed for iron hundreds of times every 5 minutes
        int hoard_cap_mineral = 200;
        /// Population limit of the AIs: 0 = the game limit, the same as yours (the AI at its strongest); 1 = follows the
        /// humans (original): 0.8 x the largest human population + 0.2 x the game limit, x 70% / 90% / 140% (easy / medium /
        /// hard) - the AI grows with you and doesn't flood you with units. Per difficulty: [ai_easy] below sets 1.
        bool pop_follow_human = false;
        /// from the Copper Age on: minutes before a planned epoch advance in which the AI starts saving
        int reserve_window_min = 5;
        /// distance (tiles) from the base within which the wild food (berries) counts for the food budget before farms
        int site_max_distance = 20;
    }
}
