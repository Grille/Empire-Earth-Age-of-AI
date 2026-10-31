// [general]: the mod as a whole, the AI chat and personalities, tools
/// ============================================================================================
///  Empire Earth AI mod (ageofai.dll) - settings
///  Changes take effect when a game starts (or a saved game is loaded); enabled= when Empire Earth starts.
///  This file is required: without it Empire Earth shows an error and closes. For the original game set
///  enabled=0 below, or delete ageofai.dll.
/// ============================================================================================
///
[section(10, "General")]
class general
{
    enum improved_unit_movement_choice { @default, enabled, disabled }

    /// 1 = the mod is active. 0 = the mod does nothing at all: the original game (no AI changes, no scenario fix,
    /// no display options, no personalities)
    [group("The mod")] bool enabled = true;
    /// Scenario fix: AIs in scenarios started from the menu play like in skirmish games
    /// (the original game switches most of their planning off in scenarios). 1 = on, 0 = off
    bool scenario_ai = true;
    /// Automatic victory: 1 = the game ends by itself once one alliance is left (original). 0 = it goes on until you end
    /// it: once everyone still in is allied with you (vassals or allies) or nobody is left, a hint says so, and typing
    /// sieg / victory in the chat, Resign or Exit ends it as a victory (your allies win with you, your vassals resign).
    /// Players without units are still out, as in the original. Like the editor's victory flag, for every game
    bool auto_victory = true;
    /// Skip the intro movies at game start. 1 = skip, 0 = play them (original)
    bool skip_intro = false;
    /// Ranged attacks lead moving targets: arrows, bullets, cannon balls and catapult stones fly a fixed arc to where
    /// the target stood when they were fired, so a unit walking across the line of fire was almost never hit. They now
    /// aim where the target will be when they come down (its speed and direction x the flight time), for every player.
    /// Homing missiles are unchanged. In multiplayer every player needs the same setting. 1 = on, 0 = off (original)
    [group("Combat")] bool lead_targets = true;
    /// Homing missiles fly towards where the target will be (intercept) instead of where it is (pursuit). Tested
    /// against helicopters: about the same hits and flight times - missiles are 2.5x faster than aircraft and hit
    /// within 1.5 tiles, so plain pursuit rarely misses. 1 = on, 0 = off (original)
    bool lead_missiles = false;
    /// Missile launchers (AA towers, missile troopers, ...) don't all fire at the same target: when the missiles already
    /// on their way will destroy it, a launcher takes another enemy of the same kind in range, or holds its shot.
    /// 1 = on, 0 = off (original)
    bool spread_missiles = true;
    /// Big selections: double-click (every unit of that kind on screen) and the selection box took at most 63 units.
    /// 1 = up to about 240 (as many as the game's selection message can carry). 0 = the original 63
    [group("Selection and orders")] bool big_selection = true;
    /// Goal markers: a move order showed the circle at each unit's goal for only the first 15 units of the group (20 in
    /// the other lines and wedges). 1 = every unit's circle, 0 = the original
    bool goal_markers = true;
    /// Loading transports (ships, helicopters, trucks): units right-clicked onto a transport are shared out over it and
    /// the other selected transports (each the nearest units it has room for, a ship its own meeting point at the beach; a
    /// yellow circle where each one goes); units without room keep what they did, with one transport too.
    /// 0 = the original (every unit to the clicked transport; those without room are turned back)
    bool transport_split = true;
    /// Unloading several transports at one spot: each gets its own tile there (a yellow circle on each). 0 = the original
    /// (one tile for all)
    bool transport_unload_spread = true;
    /// Citizens sent to build: one that ends its walk a tile off the building site (pushed off it by the others - five
    /// citizens moved together all head for the same spot) is sent again, up to three times, instead of standing there.
    /// 0 = the original (it stands idle next to the site)
    bool build_retry = true;
    /// A unit that picked a building to attack by itself turns on the nearest enemy unit within its range + 4 tiles between
    /// its shots (the original keeps shooting the building while the enemy's soldiers shoot it). Every player's units; an
    /// ordered attack is never changed. 0 = the original
    bool units_first = false;
    /// An idle soldier shot by an enemy beyond its reply range (a mortar, a sniper, a tower further off than it shoots) goes
    /// for it when it can see it, hurt it and walk there, within 24 tiles (the original stands and is shot). 1 = every
    /// player's units, 2 = the computer players' only, 0 = the original
    [range(0, 2)] int answer_fire = 0;
    /// Improved unit movement: the mod's changes to how units move (the routes, turns and collisions below), as one
    /// setting. default = each as the mod ships it (all on), enabled = all of them on, disabled = all off (the game's
    /// own movement). A key below set on its own (remove the ; in front) wins over this: improved_unit_movement=default
    /// with formation_follow=0 is everything but the marching formations.
    [group("Movement")] improved_unit_movement_choice improved_unit_movement = improved_unit_movement_choice.@default;
    /// Ship routes: a ship's route planned whole and in straight lines where the water allows (the game's comes in pieces
    /// of 8-18 tiles in the 8 tile directions, so at any other angle it zigzags), off the shallows and the coast.
    /// 0 = the original (the player's and the AI's ships alike)
    [child(nameof(improved_unit_movement)), commented("0")] bool ship_routes = true;
    /// Land routes: the same for land units (and hovercraft) - a route planned whole, straight wherever the ground
    /// between two points is clear, instead of the game's pieces in the 8 tile directions. 0 = the original
    [child(nameof(improved_unit_movement)), commented("0")] bool land_routes = true;
    /// Land route corners (with land routes): each corner of a route moved up to 1.5 tiles away from the buildings,
    /// trees and cliffs it rounds, as far as there is room (through a gap it keeps to the middle), so units don't scrape
    /// every corner on the way. 0 = the corners as the search finds them
    [child(nameof(improved_unit_movement)), commented("0")] bool land_widen = true;
    /// Shared routes (with ship or land routes): a route planned is kept for a second, and the next unit sent to the same
    /// spot joins it from where it stands instead of planning its own over the whole map - a group sent far asked for a
    /// route per unit, together long enough to make the picture stand. 0 = each unit its own route
    [child(nameof(improved_unit_movement)), commented("0")] bool share_routes = true;
    /// Route budget (with ship or land routes): the route planning of one simulation step is limited - past it a unit
    /// sent far gets the first part of its route at once and the rest a moment later, so a big army's order doesn't
    /// hold up the picture. 0 = every route planned whole at once
    [child(nameof(improved_unit_movement)), commented("0")] bool route_budget = true;
    /// Ship turns: a ship whose next waypoint lies more than 45 degrees off its bow (a new order while under way, a corner)
    /// turns in an arc at the speed the turn allows, where the circle is free, instead of slowing to a crawl and turning
    /// almost on the spot, steers at a point sliding along its route and sails through shift-queued points. 0 = the original
    [child(nameof(improved_unit_movement)), commented("0")] bool ship_turns = true;
    /// Land turns: land units (and hovercraft) steer at a point sliding along their route instead of at each tile's
    /// centre - the heading no longer jumps at every waypoint - and walk on through shift-queued points. 0 = the original
    [child(nameof(improved_unit_movement)), commented("0")] bool land_turns = true;
    /// Ship collision: ships (and submarines among themselves) are kept apart by their hulls (a capsule along the ship,
    /// its length the game's size) and slide past each other - the overlap undone gently over a few steps, the smaller
    /// ship giving way more, a standing ship of the same player nudged aside - instead of the game's thin-line test, which
    /// lets them pass through each other, halves the speed and jumps them back. 0 = the original
    [child(nameof(improved_unit_movement)), commented("0")] bool ship_collide = true;
    /// Land collision: the same for land units (and hovercraft): circles as big as the game's size for them, pushed
    /// apart gently instead of halved and jumped back, a standing unit of the same player nudged aside; units at work
    /// (gathering, building) are passed as in the game. 0 = the original
    [child(nameof(improved_unit_movement)), commented("0")] bool land_collide = true;
    /// Formations: a group sent somewhere takes up its formation (line, staggered, wedge, circle - as chosen) laid out
    /// by its units' own sizes around the clicked point, each unit keeping its place in the group (the front ones in front,
    /// the left ones left) - instead of the game's grid, spaced by the largest unit in only 5 columns behind the point, with
    /// units and places paired by distance alone, so a big mixed army spread over half the map. 0 = the original
    /// (its front row on the clicked point, turned by a right-button drag)
    [child(nameof(improved_unit_movement)), commented("0")] bool formations = true;
    /// Formation march (with formations): a group sent somewhere marches at its slowest unit's speed, so riders and
    /// tanks stay with the infantry instead of running ahead. 0 = the original, each at its own speed
    [child(nameof(improved_unit_movement)), commented("0")] bool formation_march = true;
    /// Formation follow (with formations; land units, or ships): a group sent somewhere marches in its formation behind a leader,
    /// each unit keeping its place beside and behind it, a blocked one taking its own way for a while; near the goal each
    /// goes to its own spot. 0 = the original, each on its own route (the formations' places stay)
    [child(nameof(improved_unit_movement)), commented("0")] bool formation_follow = true;
    /// Formation catch-up (with formation follow): how fast a unit behind its place in the marching formation may go to
    /// keep it, in percent of its own top speed - all of it within its own size of its place, falling off to its own top
    /// speed at four times that, and for 5 seconds after it was last in its place (a re-form or a new order on the march:
    /// no more than the boost it had then; none from rest). 100% = never faster than its own top speed
    [share(100, 300), under(nameof(improved_unit_movement))] int formation_catchup = 125;
    /// Formation wait (with formation follow): the formation goes at most this share of each unit's speed now - a unit
    /// fallen behind has only its top speed, so the formation slows to this share of it and the unit catches up at the
    /// rest; all near their places (with formation catchup) let it go at its slowest unit's top speed. Lower = the
    /// formation waits more for units behind (tighter, slower), 100% = it doesn't slow below a straggler's speed (the
    /// straggler never gains on it)
    [share(25, 100)] int formation_wait = 100;
    /// Friendly collision (with land collide / ship collide): two units of one player or of allies push each other off
    /// at this share of their size, so they slip past each other instead of getting stuck (a formation's places stay as
    /// far apart); a standing ally steps aside for a moving unit like one's own. 100% = as any two units
    [share(25, 100), under(nameof(improved_unit_movement))] int friendly_collision = 75;
    /// AI chat lines (shown in the game's chat as the AI player): 0 = off (original), 1 = debug: the AI says what it
    /// decides (attacks, new epoch, extra towers, walls, siege, counter-siege, home guard), 2 = also a status line every
    /// 5 minutes. Rate-limited per player. chat_seconds = how long a line stays on screen
    [group("AI chat and personalities"), range(0, 2)] int chat = 0;
    [range(0, 3600)] int chat_seconds = 15;
    /// AI personalities: copy the mod's folder personalities into this folder (ageofai). Every AI gets one of the personalities in
    /// it (name, favoured colour on random maps, traits for diplomacy, lines for situations of the game - see
    /// personalities\README.md for the format). A saved game finds its personalities again by their names.
    /// ai_talk = 1 the AIs speak their lines, 0 silent; ai_names = 1 they take the personality's name
    bool ai_talk = true;
    bool ai_names = true;
    /// the personality folder somewhere else (absolute, or relative to the game folder) - for whoever works on the texts:
    /// the game reads them where they are edited, e.g. the mod's repository; empty = personalities in this folder
    [length(limits.MAX_PATH), commented("_aimod\\config\\personalities")] string ai_dir = "";
    /// the game's language: the AI voice answers in it, colour names follow it - the texts in personalities are English. Detected from the game (all 12 retail languages:
    /// de, en, fr, es, it, pl, pt-br, ru, ko, zh-cn, zh-tw); set it here if that fails or to choose another, e.g.
    /// language=en
    [length(16), commented("de")] string language = "";
    /// EXPERIMENTAL: 1 = the personalities' style settings change how they play (earlier/later attacks, army
    /// share, home guard). 0 (default) = every AI plays the same, tested way; personalities only talk and negotiate
    bool ai_style = false;
    /// For whoever writes the texts in personalities (personalities, mod/computer.eeai, mod/prompt.eeai, mod/system.eeai): 1 = every mistake comes
    /// up at once - the files' at startup in a message box, those found while playing (an unknown name in a condition, a
    /// broken expression, a placeholder nobody fills) as a line in the game. 0 (default) = a mistake is only logged
    /// (ageofai_log.csv, # texts:) and that key is skipped - a bad file never stops the game
    bool strict_parsing = false;
    /// Saved games: the mod's own state (opinions, grudges, who capitulated to whom, pending offers, army accounts)
    /// goes into <save name>.ageofai next to the save and comes back when it is loaded. 0 = off (all of it starts
    /// fresh after loading)
    bool save_state = true;
    /// a fixed personality for one player slot (file name without .eeai), e.g.:
    [per_player, length(40), commented("eroberer", "personality_P2")] string personality_P = "";
    /// Remote control for tools (scripts/test/eectl.py, the MCP server scripts/test/ee_mcp.py): a local named pipe that can read
    /// and change the game (peek/poke, spawn units, give resources...). 1 = open, 0 = closed (default). Never in a
    /// multiplayer game - a command on one PC desyncs it. Test runs (ageofai_autotest.ini) always open it
    [group("Tools and debugging")] bool control_pipe = false;
    /// Memory dumps for debugging: 2 files per AI and game in ageofai_dumps (never deleted). 1 = on, 0 = off (default;
    /// test runs always write them)
    bool memory_dumps = false;
    /// The log ageofai_log.csv: over 20 MB at the start of a game it becomes ageofai_log.old.csv and a new one starts
}
