// [patches]: fixes of the game's own behaviour - what it does wrong or can't have meant (an overflow, a unit stuck for
// good, a change of Art of Conquest's that misfired), or one build's fix carried to the other. Nothing new: a patch
// plays the game as it was meant to, so each is on by default even where a test shows no gain, and each can be set
// back to the original. The mod's own changes of how the game plays (movement, combat, the AI) are in [general] and
// the AI's sections.
namespace ageofai.settings;

///
[section(15), title("Game fixes")]
class patches
{
    /// Sight counts: a map tile counts the units that see it in one byte, so with more than 254 units seeing it the
    /// count wrapped - the tile took itself for unexplored, and it and everything on it (units, trees) blinked out in
    /// big armies. 1 = the whole count kept (up to 65000); 0 = the original
    [title("Sight counts over 254 units")]
    bool sight_counts = true;
    /// Turn fix: Art of Conquest's fix brought back - units that turn with momentum (ships, helicopters, tanks ...)
    /// stop on their heading instead of swinging past it and back. 0 = the original
    [title("Turn fix (from Art of Conquest)"), target(ee)]
    bool turn_fix = true;
    /// Data order in The Art of Conquest: AoC takes what it didn't change from Empire Earth's folder, and the game
    /// looked in both folders' loose files before either archive - a loose file in Empire Earth's Data folder
    /// (a tweaked dbworld.dat ...) beat AoC's own data, and AoC read tables shorter than its own (units with no
    /// reload). 1 = AoC's files first, loose or packed, then Empire Earth's; 0 = the original
    [title("Data order in Art of Conquest"), target(aoc)]
    bool aoc_data_order = true;
    enum reload_choice
    {
        /// AoC's as it is
        game,
        /// Empire Earth's: the whole reload every time
        full,
        /// AoC's random wait, but a swing under way is never restarted before its blow
        swing,
    }
    /// Reload in The Art of Conquest: AoC waits a random share (0..100%) of a unit's reload between blows, and a wait
    /// shorter than the attack animation restarts the swing before the blow lands - citizens stood beside an animal
    /// for the whole game (AoC's own AI and the player's alike):
    [title("Reload in Art of Conquest"), target(aoc)]
    reload_choice aoc_reload = reload_choice.swing;
    /// Overseas mines: on a water map the game puts idle citizens on a boat to a mine elsewhere, but took any free
    /// mine for it - mostly one at home; the boat rarely came and they stood idle meanwhile. Only a mine on another
    /// island, as Art of Conquest fixed it (the AI's citizens). 0 = the original
    [title("Overseas mines (from Art of Conquest)"), target(ee)]
    bool overseas_mines = true;
    /// Walls on slopes: a wall piece stood flat at the height of one corner of its tile, so on a hill one end floated
    /// and the other sank into the ground, and neighbours at other heights stuck out of each other in steps; the
    /// game's own sloped pieces fit one steepness and were hardly ever chosen. Each piece now leans along its length
    /// onto the ground under its two ends (its sides stay upright; barbed wire leans across the slope too), and below
    /// it its tower's foundation reaches down into the hill. In multiplayer every player needs the same setting.
    /// 0 = the original
    [title("Walls on slopes")]
    bool wall_slopes = true;
    /// Wall joins (with wall_slopes): the game put a round post at the end of every stretch of wall it laid and
    /// beside every joint, so a wall drawn on from another had two posts side by side, walls crossing or meeting
    /// three or four in a cluster, and a post stood between a wall and the tower or gate it ran into. A post now
    /// stands only where a wall ends, turns or branches: a wall runs on through, and into a tower or a gate. Set
    /// when a wall or a gate is built; a destroyed wall's gap stays open. In multiplayer every player needs the
    /// same setting. 0 = the original
    [title("Wall joins")]
    bool wall_joins = true;
    /// Walls a game starts with (with wall_slopes): a scenario's walls (made without the mod, or with these settings
    /// off) and a game's saved before are fitted and joined at the game's start, as if built with them. 0 = they stay
    /// as they were made; walls built in the game are fitted, and an old one a new one is drawn onto with it. In the
    /// scenario editor 1 has every wall looked at whenever walls change there, 0 only those around the change.
    /// In multiplayer every player needs the same setting
    [title("Fit the walls a game starts with")]
    bool wall_start_fit = true;
    /// Buildings on slopes: some buildings lean with the ground under them (the AA guns), but one built in a game
    /// stood level, its legs in the hill on one side and in the air on the other, and the scenario editor leaned
    /// one only forward and back (on a hill falling sideways it sank in and floated) or, turned, stood it level.
    /// Now they lean with the ground both ways wherever they are built or placed, and a scenario's are mended at its
    /// start. In multiplayer every player needs the same setting. 0 = the original
    [title("Buildings on slopes")]
    bool building_lean = true;
}
