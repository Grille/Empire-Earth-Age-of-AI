// [editor]: the scenario editor (engine/editor*.c)
namespace ageofai.settings;

///
[section(40), title("Scenario editor")]
class editor
{
    // the names the sizes below may use (game: the code's own, the resolution of Game Settings)
    static readonly vec2 off = vec(0, 0);
    static readonly vec2 ee = vec(800, 600);
    static readonly vec2 aoc = vec(1024, 768);
    static readonly vec2 game = vec(1024, 768);

    /// Resolution of the scenario editor:
    ///   ee   = 800x600 (original Empire Earth)
    ///   aoc  = 1024x768 with the editor layout of The Art of Conquest
    ///   game = the resolution you play games at (Game Settings), with the AoC layout extended to it: the map view
    ///          fills the screen, the panels sit at the bottom, minimap and file buttons at the bottom right
    ///   or any size above 800x600, as an expression: 1920x1080, game * 150%, aoc * 1.5 (same layout rules)
    /// The background art is the original 800x600 one, stretched. Full-screen pages (players, texts) keep their
    /// controls at the top left, as in AoC.
    [also("patches")]
    vec2 editor_resolution = ee;

    /// The size the editor's panels, buttons and texts are laid out at, the picture still drawn at editor_resolution
    /// (not with editor_resolution = ee, and only with renderer = own). A smaller size makes them larger; its height
    /// counts, 600 at least:
    ///   off = as editor_resolution makes them
    ///   ee  = as large as at 800x600
    ///   aoc = as large as at 1024x768
    ///   or any size as an expression: 1280x720, aoc * 1.5 or aoc * 150% (1536x1152: smaller than aoc), game * 50%
    /// The map, the shapes and the texts stay sharp. The map view shows what it shows at that size.
    [also("patches")]
    vec2 editor_uiscale = off;

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
    /// it to change or delete it. With this on, a cliff can be painted over too: its texture and colours stay, in the
    /// map's file, and it still can't be walked. The terrain panel's list has the cliff textures in it too (on flat
    /// ground only a texture: units walk on it). The elevation panel gets two check boxes, both ticked: untick "Cliff
    /// color" for the game not to add or take off cliff textures, "Water color" not to colour the ground when it goes
    /// under water or comes back
    [title("Terrain colours")]
    bool terrain_palette = false;
}
