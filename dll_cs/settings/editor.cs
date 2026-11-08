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

    /// How large the editor's panels, buttons and texts are, the picture still drawn at editor_resolution (not with
    /// editor_resolution = ee, and only with renderer = own):
    ///   off = as that resolution makes them
    ///   ee  = as large as at 800x600
    ///   aoc = as large as at 1024x768
    ///   or a percentage over 100, e.g. 150
    /// The map, the shapes and the texts stay sharp. The map view shows what it shows at that smaller size.
    [length(16), also("patches")]
    string editor_uiscale = "off";

    /// A search box above the unit list of the editor's units panel: type part of a unit's name - as the list shows
    /// it, or the game's internal (English) name - and the list shows only the units that match, at once as you type;
    /// several words must all match. Empty = the whole list, as without it
    [title("Unit search")]
    bool editor_unit_search = false;

    /// Terrain colours of your own per map. The game's 54 terrain colours fill its 256-colour palette, but many are
    /// the same shades in another place (nine grass and sand types each have their own copy of one ramp): with this
    /// on they share one place, which leaves 96 of the 256 free for a map's own colours, kept in its file (a scenario,
    /// a saved game; the original game shows its own colours there). Painted ground looks exactly as before, in the
    /// original game too. In the terrain panel the colour list ends in a strip of all hues: click it for a colour of
    /// the map's own - one colour, a ramp between two, or each slot its own colour, 1 to 16 slots, the colours picked
    /// in the dialog. It comes after the game's colours in the list and paints as they do; double-click
    /// it to change or delete it
    [title("Terrain colours")]
    bool terrain_palette = false;
}
