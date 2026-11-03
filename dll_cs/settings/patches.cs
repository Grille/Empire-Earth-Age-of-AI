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
    /// onto the ground under its two ends (its sides stay upright) and stands a little into it. In multiplayer every
    /// player needs the same setting. 0 = the original
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
}
