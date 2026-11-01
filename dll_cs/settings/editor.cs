// [editor]: the scenario editor (engine/editor*.c)
namespace ageofai.settings;

///
[section(40), title("Scenario editor")]
class editor
{
    /// Resolution of the scenario editor:
    ///   ee   = 800x600 (original Empire Earth)
    ///   aoc  = 1024x768 with the editor layout of The Art of Conquest
    ///   game = the resolution you play games at (Game Settings), with the AoC layout extended to it: the map view
    ///          fills the screen, the panels sit at the bottom, minimap and file buttons at the bottom right
    ///   or any WIDTHxHEIGHT from 800x600 up, e.g. 1920x1080 (same layout rules)
    /// The background art is the original 800x600 one, stretched. Full-screen pages (players, texts) keep their
    /// controls at the top left, as in AoC.
    [length(16), also("patches")]
    string editor_resolution = "ee";
}
