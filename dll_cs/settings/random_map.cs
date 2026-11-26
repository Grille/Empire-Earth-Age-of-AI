// [random_map]: the mod's own random map, "Age of AI" in the game's list of map types (engine/mapgen.c). The
// scenario editor's Create Map asks for these in a dialog when that type is chosen (engine/mapgen_dialog.c), which
// writes them back here, so does the game setup screen's Options button (engine/mapgen_button.c); a multiplayer game
// plays the host's (every PC makes the map itself: the host's Start sends them to every PC).
namespace ageofai.settings;

///
/// The "Age of AI" map type: the mod's own random maps. The scenario editor asks for these when you create such a
/// map, the game setup's Options button beside the map type too, and keeps what you chose here. In a multiplayer
/// game every player gets the host's. Whatever is chosen, every start keeps its own mines, forage, forests and herd.
/// The amounts: none, few (half as many as the map's size gives), normal, many (twice as many), lots (three times).
[section(45), title("Age of AI maps")]
class random_map
{
    enum map_layout_choice
    {
        /// two continents side by side, the sea between them, the players by turns on either
        two_continents,
    }
    /// What the map is:
    map_layout_choice map_layout = map_layout_choice.two_continents;
    enum map_land_choice
    {
        /// smaller continents, more sea
        small,
        /// as made so far
        normal,
        /// larger continents, a narrower sea between them
        large,
    }
    /// How large the land is:
    map_land_choice map_land = map_land_choice.normal;
    enum map_climate_choice
    {
        /// snow and ice over half the map
        cold,
        /// more of the cold, less of the hot
        cool,
        /// cold in the far north, hot in the near south
        normal,
        /// more of the hot, less of the cold
        warm,
        /// desert, savanna and tropics over half the map
        hot,
    }
    /// The climate (the north of the map is the far end):
    map_climate_choice map_climate = map_climate_choice.normal;
    enum map_climate_zones_choice
    {
        /// cold in the far north, hot in the near south
        north_south,
        /// cold at both ends of the continents, hot along the middle of the map
        poles,
        /// no zones: warm and cold places mixed, wherever the weather has them
        mixed,
    }
    /// Where it is cold and where hot:
    map_climate_zones_choice map_climate_zones = map_climate_zones_choice.north_south;
    enum map_rain_choice
    {
        /// deserts, steppes and tundra where it is dry
        arid,
        /// drier than normal
        dry,
        /// dry and wet places by the weather and the water near them
        normal,
        /// wetter than normal
        wet,
        /// forest, tropics and taiga where it is wet
        lush,
    }
    /// How much rain falls:
    map_rain_choice map_rain = map_rain_choice.normal;
    enum map_mountains_choice { none, few, normal, many, lots }
    /// How many mountain ranges and plateaus:
    map_mountains_choice map_mountains = map_mountains_choice.normal;
    enum map_rivers_choice { none, few, normal, many, lots }
    /// How many rivers (none: no lake has an outflow either):
    map_rivers_choice map_rivers = map_rivers_choice.normal;
    enum map_lakes_choice { none, few, normal, many, lots }
    /// How many lakes:
    map_lakes_choice map_lakes = map_lakes_choice.normal;
    enum map_islands_choice { none, few, normal, many, lots }
    /// How many islands off the coasts:
    map_islands_choice map_islands = map_islands_choice.normal;
    enum map_forests_choice { none, few, normal, many, lots }
    /// How many forests away from the starts:
    map_forests_choice map_forests = map_forests_choice.normal;
    enum map_mines_choice { none, few, normal, many, lots }
    /// How many gold, iron and stone mines away from the starts (on the land, at the mountains' feet, on plateaus):
    map_mines_choice map_mines = map_mines_choice.normal;
}
