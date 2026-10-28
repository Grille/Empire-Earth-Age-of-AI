unsafe struct ee_draw_state
{
    [offset(0x1C), found(c)]
    void* device;
}

[c_type("ee_font")]
unsafe struct GEFont
{
    [offset(0x18), found(c)]
    block hfont;

    [offset(0x24), found(c)]
    void* bitmap;
}

[c_type("ee_input_mouse")]
unsafe struct ISMouse
{
    [offset(0x4), found(c)]
    uint8 suspended;

    [offset(0x18), found(c)]
    block buttons;

    [offset(0x44), found(c)]
    void** device;
}

/// DX7HRTnLDisplay.dll's (DX7HRDisplay.dll's without T&L) implementation of Low-Level Engine's GERasterizer: everything
/// the game draws goes through it (game.rasterizer). Its 82 virtual functions are exported by name in both builds, in
/// the same slots (scripts/re/dll_vtable.py) - those of a frame below, the exported name in each one's comment
[c_type("ee_rasterizer")]
unsafe struct DX7Rasterizer
{
    [offset(0x20), found(c)]
    ee_screen_state** screen_states;

    [offset(0x6C), found(c)]
    ee_draw_state** dstates;

    // VIRTUAL FUNCTIONS (screen: the screen's index, game.screen)

    /// BeginScene(GESurface *, K): the frame's start, into the screen's back surface (RTSGraphics.frame); 0 = drawn
    [slot(4)]
    extern int32 begin_scene(DX7Surface* target, uint32 screen);

    /// Clear(GEColor const &, GERectangle const &, K)
    [slot(6)]
    extern int32 clear(void* color, void* rect, uint32 screen);

    /// ClearZBuffer(M, GERectangle const &, K): the depth buffer in rect to depth (the render objects' clear slot)
    [slot(7)]
    extern int32 clear_z(float32 depth, void* rect, uint32 screen);

    /// Draw(GEPolygonArray &, K, GEVertexArray &, K, GETexture *, K, bool): one array of polygons, the units' and the
    /// effects' (DrawModelHierarchy's for each of a model's parts)
    [slot(13)]
    extern int32 draw_polygons(void* polygons, uint32 p2, void* vertices, uint32 p4, void* texture, uint32 screen, int8 p7);

    /// Draw(GETerrainMesh &, K, bool): the map's ground
    [slot(14)]
    extern int32 draw_terrain(void* mesh, uint32 screen, int8 p3);

    /// Draw(GEModel *, K, M, bool): a model with its children (DrawModelHierarchy)
    [slot(19)]
    extern int32 draw_model(void* model, uint32 screen, float32 p3, int8 p4);

    /// DrawSky(GEModel *, K, M, bool)
    [slot(24)]
    extern int32 draw_sky(void* model, uint32 screen, float32 p3, int8 p4);

    /// DrawWater(GETerrainMesh &, K, bool)
    [slot(27)]
    extern int32 draw_water(void* mesh, uint32 screen, int8 p3);

    /// EndScene(K)
    [slot(28)]
    extern int32 end_scene(uint32 screen);

    /// SetAlphaTestingRefValue(M, K): RTSGraphics.draw_objects sets 0 before the render objects draw
    [slot(39)]
    extern void set_alpha_ref(float32 value, uint32 screen);

    /// SetViewport(GEViewport const &, K): the camera the next draws use
    [slot(62)]
    extern void set_viewport(void* viewport, uint32 screen);

    /// ShowScene(K): the back surface onto the screen (the mod's windowed present hooks its exported function)
    [slot(66)]
    extern int32 show_scene(uint32 screen);

    /// DrawModelHierarchy(GEModel *, K, M, bool), protected: a model's parts, each through draw_polygons
    [slot(75)]
    extern int32 draw_model_hierarchy(void* model, uint32 screen, float32 p3, int8 p4);
}

/// the graphics thread's object: it draws a screen's frames through the rasterizer, each by its render objects. Its
/// thread (0x50FE16) takes a message (message), then - unless frames are held (+0x56) or it is ending (+0x59) - the
/// mouse's update (engine_display.call_frame_mouse) and frame(1, 1), then Sleep(1) (engine_display.call_frame_sleep)
[c_type("ee_graphics")]
[vtable((ee)0x0082C4BC, (aoc)0x00841C50)]
unsafe struct RTSGraphics
{
    /// the rasterizer it draws with
    [offset(0x8), found(c)]
    DX7Rasterizer* rasterizer;

    /// its screen's index, passed to every rasterizer call
    [offset(0xC), found(c)]
    uint32 screen;

    /// its render objects (a vector from +0x14), drawn in its order (message 0x404 adds one: engine_display's
    /// render objects, created by the game's start 0x5377B4 ..., added by 0x504DAF)
    [offset(0x18), found(c)]
    RTSRenderObject** objects_begin;

    /// the end of its render objects
    [offset(0x1C), found(c)]
    RTSRenderObject** objects_end;

    /// the screen's back surface (its screen state's +0x2B0): begin_scene's target
    [offset(0x34), found(c)]
    DX7Surface* back;

    /// a movie playing (GEMovie *, message 0x40C): frame shows it instead of drawing
    [offset(0x44), found(c)]
    void* movie;

    /// frames held (message 0x402 sets, 0x403 clears; the thread loop draws none meanwhile)
    [offset(0x56), found(c)]
    uint8 held;

    /// the thread ends (message 0x401)
    [offset(0x59), found(c)]
    uint8 ending;

    // VIRTUAL FUNCTIONS

    /// a message from TSThread::Receive: 0x401 end, 0x402 / 0x403 hold / release the frames, 0x404 add a render object
    /// (p), 0x407 another rasterizer (p) and screen (b), 0x40C play a movie, 0x40D stop it ...
    [slot(2), address((ee)0x0050FF22, (aoc)0x0051A4E1)]
    extern void message(uint32 msg, void* p, uint32 b);

    /// every render object's update (slot 0), before the frame
    [slot(3), address((ee)0x0050FDF0, (aoc)0x0051A3AF)]
    extern void update_objects();

    /// a frame: update_objects, clear_objects, the rasterizer's begin_scene; drawn (0): draw_objects, end_scene,
    /// finish_objects; then with extras 0x510C6E, 0x51078A, 0x510BC2 and 0x5108EE (not looked at), with show the
    /// rasterizer's show_scene. A movie playing is drawn instead. The thread loop calls it with (1, 1)
    [slot(4), address((ee)0x0051015D, (aoc)0x0051A71C)]
    extern void frame(int8 extras, int8 show);

    /// every render object's clear (slot 1)
    [slot(5), address((ee)0x0051033C, (aoc)0x0051A8FB)]
    extern void clear_objects();

    /// the rasterizer's alpha test reference to 0, then every render object's draw (slot 2) - inside the scene
    [slot(6), address((ee)0x00510363, (aoc)0x0051A922)]
    extern void draw_objects();

    /// every render object's finish (slot 3), after end_scene
    [slot(7), address((ee)0x0051039E, (aoc)0x0051A95D)]
    extern void finish_objects();
}

/// a part of the picture RTSGraphics draws each frame, in four steps: update before the scene, clear (the depth
/// buffer where it needs), draw inside the scene, finish after it. Its own slots are pure (0x683712); a step a class
/// has nothing for is the empty function 0x5248D4
[size(4), c_type("ee_render_object")]
[vtable((ee)0x0082C334, (aoc)0x00841390)]
unsafe struct RTSRenderObject
{
    // VIRTUAL FUNCTIONS

    /// before the frame (RTSGraphics.update_objects)
    [slot(0)]
    extern void update();

    /// before the scene: depth buffer clears (RTSGraphics.clear_objects)
    [slot(1)]
    extern void clear();

    /// its part of the picture, inside the scene (RTSGraphics.draw_objects)
    [slot(2)]
    extern void draw();

    /// after the scene (RTSGraphics.finish_objects)
    [slot(3)]
    extern void finish();
}

/// the map's ground (engine_display.ro_terrain): update 0x506FB9, draw
[@base(typeof(RTSRenderObject)), size(8), c_type("ee_ro_terrain")]
[vtable((ee)0x0082C3C4, (aoc)0x00841B7C)]
unsafe struct EEROTerrain
{
    // VIRTUAL FUNCTIONS

    /// the ground's draw (the rasterizer's draw_terrain, the grid, the highlight)
    [address((ee)0x00508390, (aoc)0x005131A4), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_terrain_draw")]
    extern void draw();
}

/// the units' shadows on the ground (engine_display.ro_shadows)
[@base(typeof(RTSRenderObject)), size(4), c_type("ee_ro_shadows")]
[vtable((ee)0x0082C398, (aoc)0x008413F0)]
unsafe struct EEROGroundShadows
{
    // VIRTUAL FUNCTIONS

    [address((ee)0x00506AD8, (aoc)0x0051207F), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_shadows_draw")]
    extern void draw();
}

/// the units, buildings, trees and their effects (engine_display.ro_units): draw walks the objects in sight, finish
/// (0x50DC3A) the draw list's records (hooks.draw_old - the mod's larger list)
[@base(typeof(RTSRenderObject)), size(4), c_type("ee_ro_units")]
[vtable((ee)0x0082C42C, (aoc)0x00841BBC)]
unsafe struct EEROUnits
{
    // VIRTUAL FUNCTIONS

    [address((ee)0x0050D90E, (aoc)0x00518203), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_units_draw")]
    extern void draw();
}

/// the water (engine_display.ro_water): the rasterizer's draw_water
[@base(typeof(RTSRenderObject)), size(4), c_type("ee_ro_water")]
[vtable((ee)0x0082C480, (aoc)0x00841C10)]
unsafe struct EEROWater
{
    // VIRTUAL FUNCTIONS

    [address((ee)0x0050E1F4, (aoc)0x00518AE9), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_water_draw")]
    extern void draw();
}

/// the interface (engine_display.ro_ui, made by 0x50DE78 with the UI manager): update 0x50DEBA, clear 0x50DF0D (the
/// depth under the panels), finish 0x50E1A4
[@base(typeof(RTSRenderObject)), c_type("ee_ro_ui")]
[vtable((ee)0x0082C45C, (aoc)0x00841BEC)]
unsafe struct EEROUserInterface
{
    // VIRTUAL FUNCTIONS

    [address((ee)0x0050E012, (aoc)0x00518907), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_ui_draw")]
    extern void draw();
}

/// effects (engine_display.ro_effects): update 0x5067CF and clear 0x5067D4 (the whole view's depth to 1) only - no
/// draw of its own
[@base(typeof(RTSRenderObject)), size(4), c_type("ee_ro_effects")]
[vtable((ee)0x0082C378, (aoc)0x008413D0)]
unsafe struct EEROEffects
{
}

[c_type("ee_screen")]
unsafe struct DX7Screen
{
    [offset(0x10), found(c)]
    uint8* modes_begin;

    /// IDirectDraw7 *
    [offset(0x30), found(c)]
    void* ddraw;

    [offset(0x38), found(c)]
    uint32 mode;

    [offset(0x3C), found(c)]
    uint8 desktop;

    [offset(0x3D), found(c)]
    uint8 fullscreen;
}

unsafe struct ee_screen_state
{
    [offset(0x2A8), found(c)]
    DX7Screen* screen;

    [offset(0x2AC), found(c)]
    DX7Surface* front;

    [offset(0x2B0), found(c)]
    DX7Surface* back;

    [offset(0x2BC), found(c)]
    GEWindow* window;

    [offset(0x2C4), found(c)]
    block stats;
}

[c_type("ee_surface")]
unsafe struct DX7Surface
{
    /// IDirectDrawSurface7 *
    [offset(0x48), found(c)]
    void* dds;

    [offset(0x54), found(c)]
    block dc;

    [offset(0x5D), found(c)]
    uint8 dc_out;
}

[c_type("ee_text")]
unsafe struct GEText
{
    [offset(0x34), found(c)]
    char8* @string;

    [offset(0x44), found(c)]
    GEFont* font;
}

[c_type("ee_window")]
unsafe struct GEWindow
{
    [offset(0x0), found(c)]
    block hwnd;

    [offset(0x4), found(c)]
    float32 _4;

    [offset(0x8), found(c)]
    float32 _8;

    [offset(0xC), found(c)]
    int32 width;

    [offset(0x10), found(c)]
    int32 height;
}

/// engine_display's data of the game, places in its code and numbers
static unsafe class engine_display
{
    /// the graphics thread's Sleep(1) after every pass of its loop (the thread function RTSGraphics starts, 0x50FE16:
    /// a message, then a frame unless paused, then this): call [Sleep]
    [address((ee)0x0050FEE8, (aoc)0x0051A4A7)]
    static code call_frame_sleep;

    /// its call of ISMouse::update (one of that function's call sites): on the path that draws a frame only
    [address((ee)0x0050FEC8, (aoc)0x0051A487)]
    static code call_frame_mouse;

    /// the exe's import slot of Sleep
    [import_slot("kernel32.dll", "Sleep")]
    [address((ee)0x0082318C, (aoc)0x00837174)]
    static block iat_sleep;

    /// DX7HRTnLDisplay.dll: the caps of the vertex buffer each rasterizer state creates (D3DVERTEXBUFFERDESC.dwCaps, a
    /// mov [ebp-0xC], imm32: D3DVBCAPS_WRITEONLY - video memory; its other buffer and DX7HRDisplay.dll's add
    /// D3DVBCAPS_SYSTEMMEMORY)
    [value((ee)0xF475, (aoc)0x103CE)]
    static uint32 rva_tnl_vb_caps;

    /// the render objects RTSGraphics draws (pointers; the game's start makes them, 0x504DAF adds them in this
    /// order: effects, terrain, shadows, units, water, interface)
    [address((ee)0x00943318, (aoc)0x00965360)]
    static block ro_effects;

    [address((ee)0x00943314, (aoc)0x0096535C)]
    static block ro_terrain;

    [address((ee)0x00943310, (aoc)0x00965358)]
    static block ro_shadows;

    [address((ee)0x0094330C, (aoc)0x00965354)]
    static block ro_units;

    [address((ee)0x00943308, (aoc)0x00965350)]
    static block ro_water;

    [address((ee)0x00943304, (aoc)0x0096534C)]
    static block ro_ui;
}
