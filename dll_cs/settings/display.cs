// [display]: the window, the renderer and the picture (engine/display.c, render*.c)
namespace ageofai.settings;

///
[section(30)]
class display
{
    enum mode_choice
    {
        /// fullscreen (original)
        native,
        /// a normal window with the resolution chosen in the game settings
        windowed,
        /// a frameless window covering the whole monitor; the game keeps its resolution and the
        /// image is scaled to the screen (aspect ratio kept, black bars if it differs). For the
        /// sharpest picture choose your monitor's resolution in the game settings.
        borderless,
    }
    /// Display mode (in windowed and borderless mode Alt+Enter switches between the two):
    mode_choice display_mode = mode_choice.native;
    enum lock_mouse_choice
    {
        /// never
        off,
        /// always
        on,
        /// only in borderless, free in a window (follows Alt+Enter)
        auto,
    }
    /// Windowed/borderless only: keep the mouse inside the window while the game is in front (edge scrolling
    /// without overshooting onto another monitor; leave with Alt+Tab). Native fullscreen keeps the mouse anyway:
    lock_mouse_choice lock_mouse = lock_mouse_choice.auto;
    /// Objects drawn per frame (units, buildings, missiles - also arrows stuck in the ground). The game drew at most
    /// 512; in big fights and at high resolutions the farthest objects blinked out. 0 = the original 512
    [range(0, 1000000), game("the objects the game draws per frame", "512", 0), also("patches")]
    int draw_limit = 4096;
    /// Windowed/borderless only: smooth (bilinear) scaling when the image is scaled; 0 = sharp pixels
    bool smooth_scaling = false;
    /// Windowed/borderless only: 1 = wait for the screen refresh (no tearing, fps capped at the refresh rate),
    /// 0 = as fast as possible
    bool vsync = false;
    /// Frame watch (window modes): a line with the frame rate every minute of a game in ageofai_log.csv, and when it
    /// stays under this many frames per second for 10 s, a profile of where the time goes (at most 5 a session).
    /// A diagnostic (it times every frame and the profile pauses the drawing thread): 0 = off (default), e.g. 35
    [range(0, 1000)]
    int frame_watch = 0;
    /// Own text DC: the game's GDI text goes into a memory DC of the mod instead of a DC of the DirectDraw surface.
    /// With DDrawCompat the surface DC makes Windows keep a font cache entry for every text output (13-22 MB a minute,
    /// 111 fps at the start of a game, 13 fps after 100 minutes). 0 = the game's own way
    bool text_dc = true;
    /// Text as textures: the game's text is drawn by the graphics card from prerendered characters (the engine's own
    /// bitmap fonts) instead of with GDI on the 3D image, which made the driver wait for the graphics card every frame
    /// (~4 ms, about half a frame at 100 fps). Needs text_dc=1. 0 = GDI text
    bool text_bitmap = true;
    /// Diagnostics for display problems (a diagnostic, not for playing): surface copies and creations, render targets,
    /// clears, fog, samples of the picture - counted and logged in ageofai_log.csv. 0 = off (default)
    [commented]
    bool display_diag = false;
    /// Windowed/borderless only: 1 = the window uses real screen pixels at Windows display scaling of 125/150 %
    /// (no blur from Windows stretching it), 0 = Windows enlarges it like other old programs
    bool dpi_aware = true;
    /// [display] display_start - the parts of the quiet windowed start, each on its own until the player's starts show which work
    /// (engine/display.c play_cbt, play_front)
    struct display_start_fields
    {
        /// the display DLLs' ~15 test windows take no focus and no taskbar button
        bool probes;
        /// the window black instead of white until its first picture
        bool black;
        /// borderless: the screen's size from its creation
        bool popup;
        /// the window to the front at its first picture
        bool front;
    }
    /// Windowed/borderless only, being tried out: the parts of a quieter start (the window white for a moment, then
    /// behind other programs) - words apart by commas, all = every part, empty = none (default):
    display_start_fields display_start;
    enum frame_wait_choice
    {
        /// that pause
        game,
        /// only give up the thread's turn (more frames, but a CPU core stays busy even while nothing is
        /// drawn: paused, minimized)
        yield,
        /// yield while the game draws, its pause once it has drawn nothing for 0.1 s
        auto,
    }
    /// The drawing thread's pause after every frame: the game sleeps 1 ms or more (on most PCs about 15 ms without
    /// another program raising the timer resolution), which caps the frame rate. Only the picture: the game itself runs
    /// on another thread:
    frame_wait_choice frame_wait = frame_wait_choice.auto;
    class Renderer
    {
        enum renderer_choice
        {
            /// own when the game folder has no ddraw.dll (Windows' own DirectDraw can't run the game windowed),
            /// else wrapper
            auto,
            /// the game draws through the ddraw.dll in its folder (DDrawCompat, dgVoodoo ...)
            wrapper,
            /// the mod's own renderer on Direct3D 11 (screenshots, recording and the game's warning dialogs work;
            /// its picture matches DDrawCompat's - docs/renderer.md)
            own,
        }
        ///
        /// --- Renderer ---
        /// Windowed/borderless only: what draws the picture (with own, keep wrapper DLLs out of the game folder):
        renderer_choice renderer = renderer_choice.auto;
        /// [display] overlay - the debug overlay's layers by the names their producers give them
        /// (base/overlay.c; attack.c: routes, danger)
        struct overlay_fields
        {
            /// each attack group's route (its player's colour; red rings: waypoints in reach of known defences, a
            /// ring at the goal)
            bool routes;
            /// each AI's danger map (its colour: faint where enemies reach, stronger where it would lose the fight,
            /// the edges marked)
            bool danger;
            /// each AI's territory (faint), its border with land the enemy can reach (strong) and a circle at each
            /// front, the bigger the more of its army that front should get
            bool fronts;
        }
        /// The own renderer only: the AI's plans drawn on the map, a comma list of layers - or all; empty = none. The control
        /// pipe switches them while playing: overlay routes (overlay off: none):
        overlay_fields overlay;
        enum shadows_choice
        {
            /// the game's own: a soft patch under each object
            game,
            /// cast by the sun from the units', buildings', trees' and hills' shapes onto the ground and everything
            /// standing on it (docs/renderer.md "Shadows")
            dynamic,
        }
        /// The own renderer only: the shadows on the ground (the control pipe: shadows game / dynamic):
        shadows_choice shadows = shadows_choice.game;
        /// The own renderer only: the 3D scene's edges smoothed (FXAA - before the interface is drawn: its text stays
        /// sharp; the control pipe: picture aa on / off):
        bool antialias = true;
        /// With antialias: the scene's depth says where the outlines are, so that they are smoothed however little they
        /// stand out while the textures stay sharp, and each edge is followed to its ends (the control pipe: picture
        /// aadepth on / off):
        bool antialias_depth = false;
        /// The own renderer only: the picture darkened where things stand close to each other - at the foot of a unit or a
        /// building, in a building's corners -, worked out from the scene's depth (the control pipe: picture ao on / off /
        /// view, view shows it alone):
        bool ambient_occlusion = false;
        /// With ambient_occlusion: how far from a surface what stands near it still darkens it, in tiles (the control
        /// pipe: picture aoradius <n>):
        [range(0.1, 4)]
        share ambient_occlusion_radius = 0.6;
        /// With ambient_occlusion: how dark it gets (100% as the method works it out; the control pipe: picture aostrength
        /// <n>, 1 is 100%):
        [range(0, 4)]
        share ambient_occlusion_strength = 1.0;
        enum anisotropy_choice
        {
            /// as the game had it
            off,
            /// 2 samples
            x2,
            /// 4 samples
            x4,
            /// 8 samples
            x8,
            /// 16 samples, the sharpest
            x16,
        }
        /// The own renderer only: textures seen at a slant - the ground far off - kept sharp (anisotropic filtering; the
        /// control pipe: picture aniso <n>):
        anisotropy_choice anisotropy = anisotropy_choice.x16;
        /// The own renderer only: a frame that comes while the simulation runs a step is drawn from the last one's
        /// units, the camera and cursor moved, instead of waiting for the step (the picture stood, then jumped, in big
        /// games; the control pipe: picture steps on / off):
        bool draw_during_steps = true;
        /// The own renderer only, with draw_during_steps: when frames come during the simulation's steps, the
        /// simulation waits before its next step until a frame has shown the last one's result - only once the
        /// picture has gone this long without one (ms; a simulation running its steps back to back would otherwise
        /// starve the picture; 0 = after every step a frame came during, which cost the simulation 8-18% of its time
        /// in a 450-unit game; the control pipe: picture handoff <ms>):
        [range(0, 1000)]
        int step_handoff_ms = 50;
        /// Experimental: units moved smoothly between the simulation's steps - drawn from the mod's own copy of the
        /// world, a step behind the simulation, between where they were and where they are, instead of standing till
        /// the next step and jumping there (the mod's own collection of the units takes over from the game's; the
        /// control pipe: picture collect smooth on / off):
        bool units_smooth = false;
        /// The own renderer with dynamic shadows only: the water by its depth - the seabed loses its detail the deeper
        /// it lies, the shallows clearer, the shore without a hard line, the distance's fog on the sea as a whole instead
        /// of on the seabed under it (the control pipe: water on / off):
        bool water_look = false;
        /// With water_look: what is seen through the water fades into one colour, the deepest of the map's underwater
        /// colours, by how far the line of sight runs in the water to it, wholly after this way, in the scenario editor's
        /// height numbers (a depth of -20 there, about 4 tiles); with nothing behind the water, wholly. 0 = no fading (the
        /// control pipe: water opaque <n>):
        [range(0, 200)]
        int water_opaque_depth = 20;
        /// With water_look: the colour of that fading, as three whole numbers from 0 to 255: red green blue (50 101 187 is
        /// a blue like the game's deep water). It is lit as the water is. Empty = the deepest of the map's own underwater
        /// colours (the control pipe: water color <red green blue> / water color map):
        [length(32)]
        string water_fog_color = "";
        /// With water_look: the light the waves gather on the seabed, lines of light that run with the waves, none at
        /// the waterline and fading with the depth; how bright, of the light on the ground (0% = none; the control
        /// pipe: water caustics <0..2>):
        [range(0, 2)]
        share water_caustics = 0.6;
        /// With water_caustics: how large their pattern is (100%: its cells about a tile across):
        [range(0.25, 4)]
        factor water_caustics_size = 1;
        /// With water_caustics: how fast they move at the game's normal speed (0% = they stand):
        [range(0, 4)]
        factor water_caustics_speed = 1;
        /// With water_caustics: their light on what stands in the water - a ship's hull, a wading unit - times this, over
        /// the seabed's (0% = none on them; the control pipe: water objects <0..4>):
        [range(0, 4)]
        share water_caustics_objects = 2;
        /// With water_look: what lies under the water seen bent by the surface's waves (the waves the caustics come
        /// from), none at the shore, all where the line of sight runs a quarter tile or more through the water; how far
        /// (100%: 0.06 tiles at a wave of middling slope; 0% = none; the control pipe: water refract <0..4>):
        [range(0, 4)]
        share water_refraction = 1;
        /// With water_reflections: the mirrored picture moved by the same waves (100%: 0.06 tiles at a wave of
        /// middling slope; 0% = by the water's texture only, as before; the control pipe: water ripples <0..4>):
        [range(0, 4)]
        share water_reflection_ripples = 1;
        /// The own renderer: the water moves by the game time - its waves, its caustics and its animation stand while the
        /// game is paused and go slower at a slower speed. The game's own water moves by the real time, paused or not:
        bool water_game_time = false;
        /// The own renderer: the deeper ground under the water sways up and down, as the game
        /// draws it (its stand-in for the water bending the light); off, the seabed lies still:
        bool seabed_wobble = true;
        /// The own renderer: the half see-through edges of units' and buildings' textures (a ladder's rails) drawn after
        /// everything solid and without writing the depth, so that what lies behind them shows through them (the control
        /// pipe: water edges on / off):
        bool unit_soft_edges = false;
        /// The own renderer, with units_smooth: buildings stay whole, and units behind one are seen through it as a shape
        /// in their player's colour, lit as they are (the control pipe: picture sil on / off):
        bool unit_silhouettes = false;
        /// With units_smooth: the dead sink slowly into the ground instead of fading out, every kind the same time - the
        /// game's own fade takes from a few seconds to minutes by the unit's kind:
        bool corpses_sink = true;
        /// With corpses_sink: how long the dead lie as they fell (seconds of game time):
        [range(0, 600)]
        int corpses_lie_seconds = 30;
        /// With corpses_sink: and how long they then take to sink into the ground, at an even pace (seconds):
        [range(1, 600)]
        int corpses_sink_seconds = 120;
        /// With corpses_sink: a sunk ship goes down by its own weight and what keeps it afloat - it springs leaks fore,
        /// aft or on one side, so it dives by the bow or the stern, rolls over or settles level, and comes to rest on
        /// the bottom (in the shallows half out of the water) instead of the game's one tilt and sink:
        bool ship_sinking = true;
        /// With units_smooth: a moving unit is drawn turned the way it is actually going - units pushed along by the others
        /// in a crowd or a formation point at the spot they are heading for, across the way they all move. Only the picture
        /// turns, eased over a third of a second; the game's own heading (where it shoots, how it steers) stays:
        bool units_face_travel = true;
        /// With unit_silhouettes: a silhouette is at full strength up to this many tiles behind the building or tree it
        /// appears on (the control pipe: picture silfade <start> <end>):
        [range(0, 1000)]
        int silhouette_fade_start = 3;
        /// With unit_silhouettes: and gone at this many tiles behind it (not more than silhouette_fade_start: no fading):
        [range(0, 1000)]
        int silhouette_fade_end = 12;
        /// The own renderer: units and buildings drawn by GPU instancing, past the display DLL's part by part work
        /// (the control pipe: picture inst on / off). The same picture for less of the CPU's time in big armies:
        bool unit_instancing = false;
        /// The own renderer with dynamic shadows only: units, buildings and trees mirrored in the water, drawn a second
        /// time at half the picture's size (the control pipe: water mirror on / off):
        bool water_reflections = false;
        /// The own renderer only: the units' animation blended by the graphics card - each frame of a model kept there
        /// once, instead of the game blending two frames on the CPU and the renderer uploading the result at every
        /// draw (the control pipe: picture morph on / off):
        bool gpu_animation = true;
        /// The own renderer only: the borders between the ground's textures ragged instead of a soft fade along the tile
        /// grid: a texture's lighter clumps come first, its darker cracks last, and noise bends the border (the control
        /// pipe: picture gb on / off):
        bool ground_blend = false;
        /// With ground_blend: how much of the border goes by the noise and not by the texture's own heights (the control
        /// pipe: picture gbnoise <n>, 1 is 100%):
        [range(0, 1)]
        share ground_blend_noise = 0.5;
        /// With ground_blend: how soft the border is itself, from 2% (a hard line) (the control pipe: picture gbsoft <n>,
        /// 1 is 100%):
        [range(0.02, 1)]
        share ground_blend_soft = 0.25;
        /// The own renderer only: the ground is made this much wider and taller than the screen (100% = twice its
        /// size), so that a frame drawn during a step after a scroll or a zoom out still has ground at the edges
        /// (0% = the screen's, as the game had it; limited by the game's 101-tile region; the control pipe: picture
        /// margin <share>):
        [range(0, 2)]
        share ground_margin = 1.0;
        /// The own renderer only: the units, trees and buildings this much beyond the screen are drawn too, so that
        /// they don't vanish at the edge of a frame drawn during a step (10% = a tile or two; 0% as the game had it;
        /// the control pipe: picture objects <share>):
        [range(0, 0.5)]
        share object_margin = 0.1;
    }

    [title("Wrapper (only with renderer = wrapper)")]
    class Wrapper
    {
        enum present_choice
        {
            /// the best for the installed ddraw.dll (DDrawCompat: through its primary surface; dgVoodoo or others:
            /// Direct3D 11)
            auto,
            /// Direct3D 11
            d3d,
            /// the CPU path (use it if the window stays black or flickers)
            gdi,
            /// the game's primary surface
            primary,
        }
        ///
        /// --- Wrapper (only with renderer = wrapper) ---
        /// Windowed/borderless only: how the picture reaches the window:
        present_choice present = present_choice.auto;
        /// Hardware T&L only: the 3D models' vertex buffer in system memory, as the renderer without T&L has it. In video
        /// memory every update makes the graphics card finish its work first (with DDrawCompat 18 instead of 170 fps on a
        /// big map, natively 105 instead of 315). 0 = the game's video memory
        [game("the T&L renderer's vertex buffer in system memory", "0 (video memory)", 0), also("patches")]
        bool vertex_sysmem = true;
    }
}
