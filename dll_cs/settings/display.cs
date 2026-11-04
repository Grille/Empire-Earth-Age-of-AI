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
            /// cast by the sun onto the ground from the units', buildings' and trees' shapes; a unit standing in a
            /// shadow is darkened as a whole (being tried out - docs/renderer.md "Shadows")
            dynamic,
        }
        /// The own renderer only: the shadows on the ground (the control pipe: shadows game / dynamic):
        shadows_choice shadows = shadows_choice.game;
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
