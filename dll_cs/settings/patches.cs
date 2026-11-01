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
    enum reload_choice
    {
        /// AoC's as it is
        game,
        /// Empire Earth's: the whole reload every time
        full,
        /// AoC's random wait, but a swing under way is never restarted before its blow
        swing,
    }

    class EngineLimits
    {
        /// Sight counts: a map tile counts the units that see it in one byte, so with more than 254 units seeing it the
        /// count wrapped - the tile took itself for unexplored, and it and everything on it (units, trees) blinked out in
        /// big armies. 1 = the whole count kept (up to 65000); 0 = the original
        [title("Sight counts over 254 units")]
        bool sight_counts = true;
    }

    [title("Art of Conquest and Empire Earth")]
    class ArtOfConquestAndEmpireEarth
    {
        /// Turn fix: Art of Conquest's fix brought back - units that turn with momentum (ships, helicopters, tanks ...)
        /// stop on their heading instead of swinging past it and back. 0 = the original
        [title("Turn fix (from Art of Conquest)")]
        bool turn_fix = true;
        /// Reload in The Art of Conquest: AoC waits a random share (0..100%) of a unit's reload between blows, and a wait
        /// shorter than the attack animation restarts the swing before the blow lands - citizens stood beside an animal
        /// for the whole game (AoC's own AI and the player's alike):
        [title("Reload in Art of Conquest")]
        reload_choice aoc_reload = reload_choice.swing;
        /// Overseas mines: on a water map the game puts idle citizens on a boat to a mine elsewhere, but took any free
        /// mine for it - mostly one at home; the boat rarely came and they stood idle meanwhile. Only a mine on another
        /// island, as Art of Conquest fixed it (the AI's citizens). 0 = the original
        [title("Overseas mines (from Art of Conquest)")]
        bool overseas_mines = true;
    }
}
