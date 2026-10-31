// [ai_base]: the computer players, a value per difficulty (g_set_ai[d]): the medium AI plays it as it is
///
/// --------------------------------------------------------------------------------------------
///  AI behaviour. Everything in [ai_base] applies to every difficulty (the medium AI plays it as it is).
///  To set something for the easy or the hard AI only, put the same line into [ai_easy] or [ai_hard] -
///  it wins over [ai_base].
/// --------------------------------------------------------------------------------------------
[section(70, "AI"), all]
class ai_base
{
    enum epoch_pacing_choice { classic, timer, rush, follow, independent }

    /// Numbers here may be written as small expressions: 35% is 0.35, 2 * 60, min(...) - shares always as shares
    /// (navy_share=25%, home_guard_share=35% or 0.35). A value the mod can't read (25 for a share: 2500%) is logged in
    /// ageofai_log.csv (# ini:) and the default is used.
    ///
    /// Resource discount: what the AI pays, in percent of the real price.
    ///   100% = fair (same prices as you), 75% = a quarter cheaper, 50% = half price.
    ///   0    = don't touch it: use the game's "Cheat" setting (scenario editor / data files).
    [group("Economy and epochs"), pct(1000), game("the price the AI pays, in percent of the real one (with the game's Cheat toggle)", "100%", 0)] int cost_percent = 100;
    ///
    /// Early-game economy plan (before farms, which arrive in the Copper Age):
    /// the AI builds up its economy, counts the food it can find, saves for the next epochs,
    /// keeps only a token army and advances as soon as it can pay. 1 = on, 0 = original behaviour
    /// 2 = experimental (docs/economy.md): only the next epoch is saved for, and above min_workforce the citizens keep
    /// training - epoch_share of the income goes into an epoch account instead; food_floor food workers before farms
    [num] int copper_plan = 1;
    [share(0, 100), commented] int epoch_share = 40;
    [num, commented] int food_floor = 18;
    ///
    /// Military budget: soldiers may only be paid from a share of the AI's starting stock and of
    /// everything it gathers; the rest goes into economy and epochs. 1 = on, 0 = original behaviour
    [num] int budget = 1;
    /// share of the starting stock / of all income that may go into the army (0% - 100%)
    [share(0, 100)] int military_start_share = 30;
    [share(0, 100)] int military_income_share = 40;
    /// from the Copper Age on (farms: food no longer runs out) a player puts most of his income into the army
    [share(0, 100)] int military_income_share_farms = 65;
    ///
    /// Epoch pacing - when the AI advances to the next epoch:
    ///   classic     = the original game: its epoch timer (epoch_min..epoch_max below), and at most one epoch ahead of
    ///                 the highest human epoch - the timer runs twice as long while ahead, half while behind. [ai_easy] below
    ///   timer       = only the epoch timer, the human players' epochs don't matter
    ///   rush        = as soon as the next epoch can be paid (the AI puts epochs before everything else)
    ///   follow      = never ahead of the highest human epoch: it prepares the next epoch's buildings and advances as soon
    ///                 as it can pay once a human is ahead (no human in the game: the timer)
    ///   independent = the epoch timer, and a few minutes after a human reaches an epoch the AI is behind in, it follows
    ///                 (for now; planned: the AI weighs when the next epoch pays off)
    /// Every mode but timer has the game's catch-up: a human reaching a new epoch makes every AI behind it try its next
    /// epoch a few minutes later (catch_up_ais: after other computer players too, not only humans; 0 = humans only, as the
    /// original). Before the Copper Age the economy plan (copper_plan) advances as soon as it can pay in
    /// every mode (follow: once a human is ahead; classic: one epoch ahead at most).
    epoch_pacing_choice epoch_pacing = epoch_pacing_choice.independent;
    /// the epoch timer: minutes between epochs (original game: 50 - 70; the game still scales this a little by difficulty)
    [num, game("the AI's epoch timer, minutes at least (the game scales it a little by difficulty)", "50", 0)] int epoch_min = 50;
    [num, game("the AI's epoch timer, minutes at most", "70", 0)] int epoch_max = 70;
    ///
    /// --- Attacks ---  (remove the ';' to use a line; without it the original value applies)
    /// minutes until the first attack (original 20 - 40)
    [group("Attacks"), num, game("minutes until the AI's first attack, at least", "20", 0), commented("20")] int first_attack_min = 0;
    [num, game("minutes until the AI's first attack, at most", "40", 0), commented("40")] int first_attack_max = 0;
    /// minutes between regular attack waves (original 14 - 17; the game scales this by difficulty)
    /// suggestion below: fewer but bigger waves - in tests the AIs kept ~20% more workers
    [num, game("minutes between the AI's attack waves, at least (scaled by difficulty)", "14", 0)] int attack_interval_min = 20;
    [num, game("minutes between the AI's attack waves, at most", "17", 0)] int attack_interval_max = 25;
    /// size of an attack group (original: easy 80%, medium 110%, hard computed)
    [pct(1000), game("the size of the AI's attack groups", "easy 80%, medium 110%, hard computed", 0)] int attack_group_percent = 150;
    /// chance of an extra, immediate attack - the source of many small, wasted waves:
    ///   on contact with an enemy (original 71%), to take an enemy's resources (original 20%),
    ///   out of revenge when attacked (original 10%). 0% = never.
    [pct(100), game("the chance of an extra attack on contact with an enemy", "71%", -1)] int attack_on_contact = 0;
    [pct(100), game("the chance of an extra attack to take an enemy's resources", "20%", -1)] int attack_for_resources = 0;
    [pct(100), game("the chance of an extra attack out of revenge", "10%", -1), commented("10%")] int attack_revenge = -1;
    /// share of the population cap for citizens (the rest is military), e.g. 60%; 0 = the game's own (default)
    [share(0, 100), game("the AI's share of the population cap for citizens", "45%", 0), commented("0")] int citizen_share = 0;
    ///
    /// --- Attack routing --- (only enemy buildings the AI has actually seen count; no map knowledge)
    /// Armies march around known enemy towers instead of straight through them. 1 = on, 0 = original routes
    [group("Attack routing")] bool avoid_towers = true;
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
    /// What the AIs know. The game shows them every unit of a human player, everywhere and always (measured: a rifleman
    /// alone at the map edge, far from every AI unit, counts as seen). 0 = the mod uses that as the game has it (all-knowing
    /// about the human). 1 = honest: an AI knows only what its units (and its allies') see now, remembers what it saw -
    /// buildings until it looks again and they are gone, soldiers for 10 minutes where it saw them - and assumes a quarter
    /// of the army it knows of at a place it hasn't looked at for 3 minutes; a landing's warships look at the beach first.
    /// The memories are saved with the game. (The game's own AI code still sees everything either way.)
    [group("What the AI sees")] bool honest_vision = true;
    /// Citizens, enemy towers and raiders: the AI no longer sends citizens to resources inside the range of enemy towers
    /// it has seen, past them on the way from its drop site, or where enemy soldiers stand (6 tiles), and calls back
    /// those already working there. 1 = on
    [group("Citizens under attack")] bool avoid_towers_citizens = true;
    /// Citizens fight back: they attack the enemy citizens building a tower near them (any age) and, up to the Bronze Age,
    /// small groups attacking them - only when clearly stronger (3x hit points x attack). 1 = on
    bool citizen_militia = true;
    /// Experimental: the citizens decide with the fight estimate instead, in every age - as a fallback when no soldiers
    /// near would win, unguarded builders when they win, soldiers only when they win clearly or are caught by faster
    /// units and still win. 0 = off (default)
    bool militia_estimate = false;
    /// ... with militia_estimate: caught by faster soldiers, the citizens fight back only when they would still destroy this
    /// share of the attackers (100% = all of them; 33% had 3 citizens attack a tank 42 times)
    [share(0, 100), commented] int militia_caught_share = 100;
    /// The early game (docs/economy.md), the playtester's opening against rushes:
    /// towers first - in the first two ages the town's first 2 towers stand by the Town Center and every unfinished tower
    /// near the town gets builders. 1 = on (default)
    [group("The early game")] bool early_towers = true;
    /// the bait - citizens attacked by soldiers (up to the Bronze Age) fall back under their nearest own tower. 1 = on
    bool tower_bait = true;
    /// against soldiers the citizens fight only next to own soldiers or under own towers, and only when they would win
    /// without a loss. 1 = on
    bool early_militia = true;
    /// no army in the first age, then food-free ranged soldiers (Slinger, Simple Bowman, Javelin, Composite Bow,
    /// Long Bow ...) while the economy grows; the army's account is its own money before farms. 0 = off (default)
    bool early_army = false;
    [share(0, 100), commented] int early_army_food_share = 10;
    [num, commented] int early_army_workers = 60;
    /// the alternative to the towers: this many soldiers at home in the first age (with early_army). 0 = none
    bool early_soldiers = false;
    /// Citizens keep away from what hurts them: artillery and ships at their full reach (up to 27 tiles), and spots where
    /// citizens were hit in the last 2 minutes (artillery firing from the fog, aircraft where they strike) - food and wood
    /// are left after one hit nearby, stone, gold and iron (worth some risk) after three. 1 = on
    [group("Safety and fortresses")] bool citizen_threats = true;
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
    /// ... and a reserve outside the population limit (units in a fortress don't count): with the map full, idle soldiers
    /// at home go in so training goes on; they come out after losses or when enemies come near. A mask of players
    /// (65535 = all, 0 = off)
    [num] int bunker_reserve = 65535;
    /// ... and a shelf for citizens it can't use: at the population limit with food, wood, gold and iron each at
    /// shelve_stock or more, idle citizens go into fortresses (more are built for them) so soldiers can be trained, and no
    /// new citizens are; they come out once a resource runs low or citizens are lost. 1 = on
    bool shelve_citizens = true;
    [num, range(500, 1000000)] int shelve_stock = 5000;
    /// Siege weapons like a player: they shell known enemy towers (that they outrange) and the walls to breach from a
    /// safe distance, while idle soldiers guard them; the AI builds a Siege Factory and trains 2-4 siege weapons of its
    /// epoch when it knows enemy defences (with priority on the stock above the epoch reserve); rams don't attack
    /// buildings guarded by enemy soldiers. 1 = on, 0 = off
    [group("Siege and tower routing")] bool attack_siege = true;
    /// advanced: route cost per tower (a normal step costs 10-14), extra tiles kept from a tower's range,
    /// search radius (tiles) around the original goal for a weak spot
    [num, commented] int avoid_towers_weight = 2000;
    [num, commented] int avoid_towers_margin = 6;
    [num, commented] int attack_weak_spot_radius = 60;
    ///
    /// Build queue fix: the game's build list for random maps starts with a settlement it can never place, which
    /// blocks every later building (granary, archery range, stable, siege factory...) for the whole game. It is
    /// built as a normal Settlement, and a building that can't be placed 3 times in a row is skipped. 1 = on
    [group("Building")] bool build_fix = true;
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
    /// Navy account: on a water map (found at the start: enemies across the sea) a share of the wood, gold and iron
    /// income is kept for warships, which the army's orders can't take, and the warships the AI asks for are paid from
    /// it instead of from the army's money. Fishing boats and the transports of a landing are paid as economy. The
    /// share scales with how many enemies live across the water (none: no navy account). 1 = on
    [group("Navy")] bool navy = true;
    /// The navy's share of the income on island maps (all enemies across the water)
    [pct(100)] int navy_share = 25;
    /// Fleet (water maps): the AI leads its warships itself instead of the game's sea groups - it guards its fishing
    /// grounds and home waters, fights enemy ships that come near when it is not weaker, hunts the enemy fleet, its
    /// fishing boats and transports and shells its coast when clearly stronger (retreating at half strength), clears the
    /// sea route of a landing and stays off its beach, and never sails into the reach of known enemy towers, cannons and
    /// artillery. Damaged ships go back to the dock; fishing boats flee to the dock from enemy warships it can't beat.
    /// 0 = the game's own sea groups. 1 = the fleet as described (test stage). 2 = a big fleet (default): the AI trains
    /// warships itself up to navy_fleet_pop of its population limit (the navy's share of the income rising toward
    /// navy_share_max while short), masses them under its towers while weaker than the enemy fleet (navy_fleet_mass),
    /// strikes as one force once stronger, and splits the fleet when clearly stronger into a home guard, hunters, raiders
    /// (fishing boats, coastal citizens and docks), a patrol on the sea lanes and a landing escort
    [num] int navy_fleet = 2;
    /// (navy_fleet=2) the fleet's size as a share of the population limit (15%: 26 warships at a limit of 176)
    [pct(100)] int navy_fleet_pop = 15;
    /// (navy_fleet=2) the navy's share of the income while the fleet is below that size
    [pct(100)] int navy_share_max = 50;
    /// (navy_fleet=2) mass before striking: while weaker than the enemy fleet known or below a quarter of its size the
    /// ships wait at a rally inside the reach of its towers (a tower is built by the dock if none covers the sea), fight
    /// there with the towers and strike as one force at 1.25 times the enemy's strength. 0 = stage 4 (shelter only)
    bool navy_fleet_mass = true;
    /// Water maps: gold and iron workers follow the navy's missing warships too, and from minute 15 (35+ workers) every
    /// mine slot on the home island near a drop site is worked - taken from piled-up wood, then from spare food. 1 = on
    [group("Workers and resources")] bool navy_mines = true;
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
    /// Water maps: the game puts idle citizens on a boat to a mine elsewhere, but takes any free mine
    /// for it - mostly a home mine; the boat rarely comes and they stand idle meanwhile. Only a mine on another island
    /// (as the expansion fixed it). 1 = on
    bool overseas_mines = true;
    /// Move workers from a resource with far more workers than it needs to stone, gold or iron mines that are
    /// short (the game only ever reassigns idle citizens). 1 = on
    bool rebalance_workers = true;
    /// Food (docs/economy.md "Food and mines"), bits to add up: 1 = before farms the worker plan counts the food of the
    /// citizens the Town Center trains (and the wood only of real buildings); 2 = gatherers that ended up elsewhere (after
    /// a raid, as leftovers on a mine) go back to food while it is short; 4 = forage at the bush nearest a drop site;
    /// 8 = hunt every animal near a drop site first (newborns too); 16 = from the Copper Age no fields while wild food is
    /// free; 32 = a Settlement by rich wild food far from the town. 3 = default (the first epoch ~8 minutes earlier,
    /// +7-9 workers at minute 30 on Continental); 8, 16, 32 cost workers or fights in the tests.
    [num] int food_plan = 3;
    [num, commented] int food_hunt_range = 15;
    /// Mines, bits: 1 = from the Copper Age every mine slot near a drop site gets a worker (from piled-up wood, spare
    /// food, idle citizens); 2 = a Settlement at the home continent's unworked mine; 4 = mines before wood and food.
    /// 0 = off (default). 7 on the large maps: kills - losses +12 to +80 a game against the base AI, its wins halved - the
    /// gold piled while the wood went short becomes an army before the war; on the small ones the war comes first and the
    /// gold lies unused: Small Islands with the cheat -6 to -7, Continental Stone Age head-to-head -12 (docs/economy.md).
    [num] int mine_fill = 0;
    /// A Settlement by the forest its woodcutters work when 8 or more of them cut 15+ tiles from any drop site (the walk
    /// with each load decides the wood income: 54 tiles gave a fifth of the usual). 1 = on
    bool wood_drop = true;
    /// Citizen production (docs/economy.md "Citizen production"), bits: 1 = from the Copper Age the Town Centers keep
    /// training citizens up to every mine slot near a drop site + the food and wood gatherers (not only the game's 45%
    /// of the population cap), the new ones to the mines' empty slots; 2 = food gets the gatherers that pay the
    /// citizens still missing; 4 = also a Settlement at the home continent's unworked mine; 8 = citizens before the
    /// epoch reserve while the food income pays both. 3 = default (+15-35 workers at 60 min, more on the mines, the base
    /// AI with the cheat won 31 of 120 games instead of 45); 0 = the game's citizen share.
    [group("Economy plans"), num] int citizen_plan = 3;
    /// the population cap's share kept for the army when the citizens' goal is larger (the home mines always fit)
    [share(0, 90), commented] int citizen_army_share = 40;
    /// Workforce into army (docs/economy.md "Workforce into army"), bits, from the Copper Age: 1 = the citizens wanted at
    /// most what the economy uses (mine slots, food, the wood actually spent, builders, a margin of cap / 40); 2 = wood's
    /// target at most the wood spent while wood piles, the rest to the mines' slots and the fields; 4 = army_plan's
    /// soldiers into the room and more production buildings on every map from a population cap of workforce_army_cap;
    /// 8 = a soldier refused for gold or iron replaced at once by the most strength per gold + iron while food or wood pile.
    /// 0 = off (default): fewer citizens (1) or 10-20 more soldiers on the large maps (4), but no gain in the fights.
    [num] int workforce_plan = 0;
    [num, commented] int workforce_army_cap = 250;
    /// Economy into army (docs/economy.md "Economy into army"), bits, from the Copper Age and only with an enemy on the
    /// AI's own continent: 1 = money lying unused for 2 minutes tops up the military account; 2 = soldiers trained into
    /// the population room the citizens don't need, while the account pays; 4 = one more Barracks / Archery Range /
    /// Stable when they train most of the time; 8 = workers from piled-up food to the wood, gold or iron the army lacks;
    /// 16 = line upgrades only for the lines in use; 32 = 1, 2 and 4 only from the epoch army_plan_epoch (the Middle Ages):
    /// before it the extra soldiers died under the enemy's towers (Continental from the Stone Age); 64 = 4 judges the
    /// buildings of the kind it trains from alone (an AI training only cavalry, its stables full and its barracks idle, got
    /// no stable more) and, with resources piled up, may have more of them. 103 = default (1+2+4+32+64): with 39 Continental
    /// Large without the cheat the AI won 61 of 80 games against the base AI instead of 35; 64 with resources piled up
    /// head-to-head soldiers +16, razed +16 a game; 0 = off.
    [num] int army_plan = 103;
    /// Outdated units attack: units of a type the AI's epoch has left behind (no upgrade reaches them any more) fill its
    /// population cap. Every 5 minutes, once 8 or more such land units stand free, they attack the nearest enemy building
    /// together (falling back as every attack does) and do some damage before they are completely outclassed. 1 = on
    bool outdated_attack = true;
    [num, commented] int army_plan_epoch = 5;
    [num, commented] int army_float_keep = 300;
    [num, commented] int army_float_keep_mineral = 100;
    [num, commented] int army_mix_food = 500;
    /// Gold and iron (docs/economy.md "Gold and iron"), bits, only from a population cap of metal_cap: 1 = a land soldier
    /// the game asks for is trained as the one that does most per gold + iron against the enemy army the AI has seen (the
    /// game's counters), when that is 25% better; 2 = no saving for a unit whose gold or iron won't come within
    /// substitute_wait; 4 = (with 1) food, wood and stone lying unused count little in the price; 8 = the fleet's size at
    /// most metal_fleet_pct of the enemy warships seen + 6 (at least metal_fleet_min) instead of 15% of the population cap;
    /// 16 = while the sea is calm the navy account keeps 100 gold and 100 iron at most. 0 = off (default): on the large
    /// island maps 8 (+16) gave 60-90 more soldiers trained and more kills - losses without the cheat, but against the
    /// cheating base AI the kills - losses fell on two maps; 1 cost wins on small Continental.
    [num] int metal_plan = 0;
    [num, commented] int metal_cap = 250;
    [num, commented] int metal_fleet_pct = 250;
    [num, commented] int metal_fleet_min = 20;
    /// The army as one (docs/economy.md "The army as one"), bits, only with an enemy on the AI's own continent:
    /// 1 = reinforcements for an attack that is out gather at a rally point by the towers and go as a group (5 together,
    /// 3 after 2 minutes); 8 = (with 1) only while that attack fights; 2 = the attack waits (10 minutes at most) while the
    /// land army is weaker than 1.3 x the strongest enemy army it knows; 16 = a member of an attack out picks no target
    /// more than army_mass_tether (12) tiles from its pack (128 with 16: only buildings); 32 = a member out far from its
    /// pack and idle walks back to it; 64 = (with 1) the released reinforcements walk to their attack's pack. 0 = off (default): 1
    /// lost fewer soldiers and killed more in head-to-head games and against the base AI without the cheat, but against
    /// the cheating base AI it killed fewer (kills - losses -10) and razed less; 16 the same (kills -14 against the
    /// cheat); 144 neutral everywhere.
    [group("The army"), num] int army_mass = 0;
    [num, commented] int army_mass_group = 5;
    [num, commented] int army_mass_hold = 120;
    [pct(1000), commented] int army_mass_ratio = 130;
    [num, commented] int army_mass_wait = 10;
    [num, commented] int army_mass_tether = 12;
    /// The staging point (with danger_routes): an attack's march from safety into the enemy's reach or a lost fight goes to
    /// one gathering point outside every danger on the way from there to the base; the army waits there and goes in
    /// together once its fighters there beat the enemy at the destination by army_stage_go. 0 = off
    bool army_stage = true;
    [pct(1000), commented] int army_stage_go = 130;
    /// The attack commits (with army_stage): no squad goes past the staging point alone - the whole wave gathers there and
    /// goes in together, fighting on the way (an attack-move), and for 2.5 minutes none of its units falls back. 1 = on
    bool attack_commit = true;
    /// One target a wave: an attack on a player has no target, so the game splits the wave into squads of about 10 that
    /// each walk their own legs; with this the wave goes to one building of that player the AI has seen, then the next
    /// nearest, all its land squads together. 1 = on (neutral in AI-vs-AI tests: off)
    bool wave_focus = false;
    /// Fall back to cover (docs/economy.md "Fall back to cover"), bits, per unit (an attack is never called back):
    /// 1 = a land unit worth fall_back_value (resources) or more below fall_back_hp of its hit points walks to the nearest
    /// own Hospital on its land (fall_back_dist tiles at most), heals there to 90% (4 minutes at most) and rejoins;
    /// 2 = a land unit whose fight is clearly lost - the fight estimate with what the AI sees (its fighters, the enemy's,
    /// the enemy towers known) below fall_back_ratio - falls back under the nearest own tower (else a Hospital, the base),
    /// 20 s there, then its attack has it again; 4 = the fleet retreats under its towers instead of to open water, and a
    /// warship below half its hit points breaks off to a dock (one an own tower covers first) until 80%. 7 (default):
    /// against the base AI on six maps (40 games each, both cheats) kills - losses +2 to +56 a game (one map -5 (7)),
    /// fewer losses, more soldiers and warships alive; head-to-head +12 to +28. 0 = off.
    /// 8 (experimental, off) = a unit that broke off from the same attack fall_back_breaks times gives it up (stays at
    /// the cover until that attack is over), a unit in a lost fight with no own cover near falls back toward home, and an
    /// attack most of whose units (fall_back_end) gave up, not winning where it stands, is called off - the next comes
    /// within 5 minutes. Measured 15 against 7 (16 cells, head-to-head): no clear gain, and the call-off can crash the game.
    [num] int fall_back = 7;
    [pct(1000), commented] int fall_back_ratio = 50;
    [pct(100), commented] int fall_back_hp = 40;
    [num, commented] int fall_back_value = 100;
    [num, commented] int fall_back_dist = 60;
    [num, commented] int fall_back_breaks = 2;
    [pct(100), commented] int fall_back_end = 50;
    /// Outposts on other islands (docs/economy.md "Other islands"), bits, water maps only (needs landing=1): 1 = once
    /// the home island's mines are half worked (or from minute 15), a transport takes expand_citizens citizens and
    /// expand_guard soldiers to the nearest free island with mines (no town, no defence and few buildings of an enemy
    /// there), they build a Settlement, two towers and work the mines; more citizens follow (expand_trips trips in all);
    /// given up after 3 losses; 2 = a barracks there too; 4 = islands with an ally's buildings count as free. Large maps
    /// (docs/economy.md "Large island maps"): 8 = from expand_minute without waiting for the home mines, its transport
    /// before every reserve; 16 = the citizens it ships are additional (the citizen plan trains them, wood cutters go
    /// first); 32 = more islands one after the other, each only once the last holds (expand_max at most, the nearest to
    /// what it holds); 64 = an outpost nearer the enemy than home gets a barracks and two more towers while the AI holds
    /// the sea (no enemy warship seen in its waters for 5 minutes, its warships at least the enemy's it knows); 128 = the
    /// outpost follows the sea and the army: planned and supplied only while the AI holds the sea and its army is 1.5 x
    /// the enemy's it knows, a guard of 4, four towers, the idle citizens board first (docs/economy.md "More metal"). Islands it
    /// has hardly looked at are scouted first by the empty transport. 0 = off (default): measured against the base AI
    /// (57 and 121 on Large / Huge maps, unit limit 1500) the outposts went up from minute 25-30 and the enemy's soldiers on
    /// the home island fell, but wins, kills - losses and the miners at 60 minutes moved both ways between the maps.
    [group("Other islands"), num] int expand_islands = 0;
    [num, commented] int expand_citizens = 8;
    [num, commented] int expand_guard = 3;
    [num, commented] int expand_minute = 8;
    [num, commented] int expand_trips = 4;
    [num, commented] int expand_max = 3;
    /// Idle citizens stranded on land without a drop site of their AI (a transport put them off there) are fetched home by
    /// one of its transports - the game gives a citizen work only on its own land. 0 = off
    bool rescue_citizens = true;
    /// Towns on the home continent, each with a purpose and only what it can hold: 1 = a resource town once the mines near
    /// its drop sites are full (fewer than 4 free places) and its army at least the enemy's it knows - the richest explored
    /// mine group no other drop site is within 18 tiles of, none of its own within 32, no enemy building it knows within 22,
    /// 32-90 tiles from its base: a Town Center (a Settlement when it can't pay one) and towers_per_town towers on the
    /// enemy's side; towns_citizens of its nearest citizens (never its miners) build it and then gather at its mines.
    /// 8 = a forward base where its army is (its staging point, else its biggest group in the field, just out of the
    /// enemy's reach), 32+ tiles out, in a stalemate too (its army 0.8 x the enemy's): 8 citizens walk there and put up two
    /// towers, a Hospital, a production building, two more towers and a wall in front, the towers first; then they go back. Not before the Copper Age, not with enemies in its base, one at a time, at most towns_max. 2 = a Settlement, never a
    /// Town Center. 4 = army_plan's next production building goes up in the town nearest the enemy when it is 15+ tiles
    /// nearer than the base. 16 (with 1) = spread: a town at the explored resources nearest to its towns, however far,
    /// without waiting for the home mines to fill - as many as it has the people for (15 workers a town; towns_max
    /// doesn't apply), one going up per 50 workers, only with a surplus (6 spare citizens, twice a Town Center's price
    /// in stock); within 45 tiles of a town where it can, never more than 20 tiles ahead of its front town; 2 towers
    /// and 1 per 4 mines, 5 houses, a wall 60+ tiles out. 32 (with 16) = a scout to unseen land. 64 (with 16) = with wood
    /// and stone to spare its towns grow (towers, Hospital, Temple, University, production) - measured worse, off. 0 = off. Default 17 (the player
    /// 2026-10-31: the AI must expand; 1v1 huge Continental 120 min, 24 games: 5 towns founded, even with off).
    [num, range(0, 127)] int towns = 17;
    [num, range(0, 6), commented] int towns_max = 3;
    [num, range(1, 12), commented] int towns_citizens = 6;
    [num, range(0, 7), commented] int towers_per_town = 2;
    /// Research the gathering upgrades (Hand Axe, Hafts, ...) as soon as they are available - the game's AI
    /// rarely gets to them. 1 = on
    [group("Research and defence")] bool gather_research = true;
    /// Defences like a player: towers in a line facing the nearest known enemy (and at outlying settlements)
    /// instead of scattered, extra towers while stone allows, and a wall in front of the towers. 1 = on
    bool defence_layout = true;
    bool defence_towers = true;
    /// (defence_walls: the wall in front of the towers - straight, with a gate in its middle)
    bool defence_walls = true;
    /// The game's own walls: the ring around the base its wall manager builds a segment at a time (in practice a stray
    /// piece with a gate) - its builders walk to the front one by one. 0 = off (the mod's AIs build none), 1 = as the game
    bool game_walls = false;
    /// tiles from the base centre to the tower line
    [num, commented] int defence_distance = 10;
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
    [num] int aa_ahead_buildings = 3;
    /// Hold the town (experimental): when a stronger enemy army camps near an AI town, its soldiers pull back behind the
    /// town and guard there instead of chasing it one by one (Guard behaviour), and strike together once they are 1.3x
    /// its strength. Tests against a camp that pushes and pulls back like a player: worse (the town is left to the
    /// pushes - fewer citizens, Town Center lost more often). 1 = on, 0 = off (default)
    bool hold_town = false;
    /// When the AI asks for a soldier it can't pay for or train (heroes, units of an earlier age, gold units without
    /// gold), it trains the strongest soldier it can afford instead (never for a ship, transport or fishing boat). 1 = on
    [group("Training")] bool unit_substitution = true;
    /// ... but a unit it can train and only can't pay yet is saved for first: this many seconds without a requested
    /// soldier before it trains a substitute (at once with enemy soldiers in its base). 0 = substitute at once
    [num] int substitute_wait = 90;
    /// The mod's own training orders (AA units, siege, home guard, transports) only up to the room under the population
    /// cap - at the cap they were paid for and stuck in the queue, blocking the AI's own training. A mask of players
    /// (65535 = all, 0 = off)
    [num] int train_room = 65535;
    /// The units in production the room for training leaves out are set to what the buildings really have queued: a loaded
    /// game starts that count at 0 (the save doesn't keep it) and the AI queued far more than its limit lets out. Off: on
    /// the player's save "standing army" its AIs lost more units with it (101 against 82 in 15 minutes). 1 = on
    bool production_sync = false;
    bool catch_up_ais = true;
    /// Units each finished production building may hold (the one in the making and its queue): the game queued every
    /// request however deep the queues were - 80 soldiers in one building are resources tied up for 40 minutes. A request
    /// is cut to what keeps each producer of the unit at this many; the resources stay free for more production buildings.
    /// 0 = no limit, as the original
    [num, range(0, 100)] int train_queue = 3;
    /// Wood after the Copper Age: the game's food demand never shrinks, so wood got 1-3 workers and nothing was built.
    /// 1 = wood gets the workers that close its gap (what the AI wants to build and train - ships too - beyond its stock)
    /// within about 2 minutes, and no more: a pile of wood buys nothing; 2 = surplus workers are moved to wood too;
    /// 4 = workers the farms can't take go to stone, gold and iron (the army's metals) instead of piling wood or food;
    /// 8 = while 4000+ food lie beyond the age reserve, farmers move to the metals (fields stand idle; off by default:
    ///     a bigger army in a long game, but in AI-only tests the AIs survived less often, 70% vs 78%);
    /// 16 = metal pull: when the army's orders fail for gold/iron/stone (more than for food) and food is above its
    ///     reserve, farmers move to the metals, 2 every 10 s (back 4 at a time when food runs short) - the game gives
    ///     gold and iron 1-6% of the workers next to its phantom food demand (off by default until tested);
    /// 7 = 1+2+4 (default), 31 = all, 0 = none (the original)
    [num] int late_targets = 7;
    /// Experimental: no new citizens while food is short and more than this many (and 15% of all) stand idle - stalled
    /// the growth in tests. 9999 = off (default)
    [num, commented] int idle_citizen_slack = 9999;
    /// Home guard: the AI keeps 6 + this many soldiers per age at home (it only trained for its next attack). 0 = off
    [group("Home guard"), num] int home_guard = 4;
    /// ... and at least this share of its room for soldiers (population cap x (1 - citizen share)) - grows with the cap
    [share(0, 100)] int home_guard_share = 35;
    /// Muster: when an enemy army in the base is hopeless to fight (at least 8 soldiers, twice the strength of the AI's
    /// soldiers and towers there), the AI no longer feeds its soldiers in one by one: they gather 18 tiles beyond the
    /// base and strike together once 1.3 x as strong; citizens next to the enemy flee there too (around the enemy, not
    /// through it). 0 = off
    bool muster = true;
    /// Citizen attacks: an AI that can't train soldiers still launched its attacks on time - with citizens, who walked
    /// into the enemy's army and towers. An attack group of at least half citizens is called off. 1 = allowed (original)
    bool citizen_attacks = false;
    /// Answer war: whoever declares war on an AI that is allied with them gets war back a few seconds later, with the
    /// stance line (stance_lines), a revenge attack and a taunt (the original AI has no diplomacy and stays allied). 1 = on
    [group("Diplomacy")] bool answer_war = true;
    /// Stance lines: 1 = only an AI's stance changes towards you, naming it ("<AI> is now your enemy / ally");
    /// 2 = the game's line for every AI stance change, also between AIs (it doesn't say towards whom); 0 = none
    [num] int stance_lines = 1;
    /// AI diplomacy: AIs judge peace offers (opinion, power, grudge, tribute - per personality), offer alliances against
    /// a much stronger enemy and may betray a weak ally (disloyal personalities). Not in scenarios or with locked teams.
    /// An alliance offered to you is only a message: the AI stays at war until you set it to allied (offer_minutes).
    /// An AI that is clearly losing to an attacker offers him peace and tribute (capitulation; set it to allied to
    /// accept). A vassal (capitulated, still allied with its lord) has no diplomacy of its own: it sends peace offers to
    /// its lord and makes none itself - a disloyal, proud one rebels once it is clearly stronger than its lord.
    /// Breaking an alliance costs reputation with every AI (by its loyalty; attacking
    /// one's own vassal x3, right after accepting its capitulation x5). Peace offers are always judged; 0 switches off
    /// only the AIs' own initiative
    bool ai_diplomacy = true;
    /// How long an offer to you stands - an alliance, peace, a capitulation - before it runs out (minutes, 1+)
    [num] int offer_minutes = 2;
    /// Alliance blocks (with ai_diplomacy): everyone linked by alliances is one block, and inside a block there is peace -
    /// two AIs allied with the same player make peace with each other; an AI at war with you that joins your block
    /// offers you peace (offer_minutes to accept). Whoever turns on a member splits the block: every other AI takes a side
    /// (the one it capitulated to, else its direct ally, else the one it likes better - the attacked a bit more) and a
    /// traitor betrays the victim's whole block. Joining means peace with the whole other block: an AI refuses when it
    /// or one of its allies holds a grudge (a war declared recently, opinion below -30) against a member of the other
    /// side, and names who stands in the way. A vassal has no say when its lord is part of the deal (it grumbles).
    /// 1 = on, 0 = off (alliances only between two players)
    [num] int alliance_blocks = 1;
    /// Surrender: an AI that has lost all its citizens and can't train new ones (no Town Center or nothing to pay with)
    /// resigns after a minute - or after 3 minutes without citizens in any case - instead of leaving its last buildings
    /// to be hunted down. Not while its army still has half the strength of its strongest enemy; not in scenarios. 1 = on
    bool surrender = true;
    /// The spoils of a resign: a player out of the game is at war with everyone; its land fighters roam in bands, hunting
    /// units and ignoring buildings, and 30 s later its buildings go to those who did 20% or more of the damage it took in
    /// its last 10 minutes, shared by that damage (nobody with 20%: they stay). Not in scenarios. 1 = on
    bool resign_spoils = true;
    ///
    /// Landings: the AI plans a landing behind your lines across the sea - the weakest defended beach next to your
    /// buildings, warships ahead, its transports gather and run in together, a bridgehead with towers and a forward base,
    /// then waves. landing_transports = the most transports (the size follows the beach), landing_citizens = citizens
    /// for the forward base, landing_waves = waves after the first. 1 = on
    [group("Landings and transports")] bool landing = true;
    [num] int landing_transports = 8;
    [num] int landing_citizens = 10;
    [num] int landing_waves = 3;
    /// Experimental: a beach too strong for the army it can send is shelled first by a fleet of warships (trained for it,
    /// 4-16), then the landing comes. 0 = off (default)
    [commented] bool landing_bombard = false;
    /// The game's transport manager takes a landing's transport for its own crossings (an unload order in any phase):
    /// refused. 1 = on (default), 0 = the game may take them
    [commented] bool landing_keep_transports = true;
    /// A transport waiting at the shore for passengers that will never come (they stand idle without an order, or went
    /// off on another one) lets go of them after 30 s and sails with those aboard. 0 = off (default: in tests the
    /// transports waited far less, but the games went no better)
    [commented] bool transport_watchdog = false;
    /// (tests only: landing_flag, landing_knows_map, landing_trace, transport_log)
    ///
    /// --- Experimental (off by default) ---
    /// The defence decisions (muster, hold the town, militia) compare sides with the fight estimate
    /// (range, minimum range, area damage, the game's damage tables) instead of hit points x attack - no gain in tests
    bool fight_estimate = false;
    ///
    /// --- advanced tuning (same for all difficulties) ---
    /// workers the AI grows to before it starts saving for the early epochs
    [group("Advanced tuning"), num] int min_workforce = 40;
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
    [num] int mineral_worker_margin = 300;
    /// share of the wild food it has found (berries) the AI counts on gathering
    [share(0, 100)] int wild_food_share = 60;
    /// food above the epoch reserve + this amount may always go into the army (no endless hoarding)
    [num] int hoard_cap = 3000;
    /// the same for stone, gold and iron (hardly any civilian use besides the epoch cost, which is reserved anyway) -
    /// 600 kept ~600 iron lying while the army failed for iron hundreds of times every 5 minutes
    [num] int hoard_cap_mineral = 200;
    /// Population limit of the AIs: 0 = the game limit, the same as yours (the AI at its strongest); 1 = follows the
    /// humans (original): 0.8 x the largest human population + 0.2 x the game limit, x 70% / 90% / 140% (easy / medium /
    /// hard) - the AI grows with you and doesn't flood you with units. Per difficulty: [ai_easy] below sets 1.
    bool pop_follow_human = false;
    /// from the Copper Age on: minutes before a planned epoch advance in which the AI starts saving
    [num] int reserve_window_min = 5;
    /// distance (tiles) from the base within which the wild food (berries) counts for the food budget before farms
    [num] int site_max_distance = 20;
}
