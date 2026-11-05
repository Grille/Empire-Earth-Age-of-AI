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

    /// the buttons' counters as the frame before (Refresh copies +0x18 here first; RecomputeCurrentButtonState sets both
    /// alike): a press is a counter odd now and different here
    [offset(0x1C), found(c)]
    block buttons_before;

    /// set by Refresh when the device's buffer held input this frame (cleared first): with a button changed or a
    /// movement delta it makes GetShouldUpdate true - the UI's forms take the mouse's place only then
    /// (UIManager::UpdateMouse, unless a control holds the mouse)
    [offset(0x5C), found(c)]
    uint8 had_input;

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

    /// CreatePolygonArray(K): count triangles (the engine's model loading: GERasterizer::sActiveRasterizer's)
    [slot(10)]
    extern DX7PolygonArray* create_polygon_array(uint32 count);

    /// CreateVertexArray(K, K): type 0 a DX7StandardVertexArray, 1 a DX7StdColorVertexArray; count vertices
    [slot(11)]
    extern DX7StandardVertexArray* create_vertex_array(uint32 type, uint32 count);

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

/// the game's graphics (RTTI, the object the graphics thread draws with): RTSGraphics with the world's lock
[@base(typeof(RTSGraphics)), vtable((ee)0x0082C264, (aoc)0x008412C0)]
unsafe struct EEGraphics
{
    /// the render objects' update under the world's read lock (with the interface's critical section; another mode
    /// +0x70 takes the write lock): the frame's only read of the world under it - draw_objects draws from what the
    /// update left. Before it the UI manager's form, its own call (0x823804) unless the rasterizer's +0x84 says so
    [address((ee)0x004FED2D, (aoc)0x00509840), overrides(typeof(RTSGraphics))]
    [vtable_site("graphics_update")]
    extern void update_objects();
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

    /// the ground's update: the camera's matrices, the terrain mesh's patches and fog for it - the visible region the
    /// units' draw walks with them
    [address((ee)0x00506FB9, (aoc)0x00512638), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_terrain_update")]
    extern void update();

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

    /// the shadows' update (the units' shadows from the world's units)
    [address((ee)0x00506840, (aoc)0x00511DE7), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_shadows_update")]
    extern void update();

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

    /// the units' update (from the world's units)
    [address((ee)0x00509D06, (aoc)0x00514521), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_units_update")]
    extern void update();

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
    // VIRTUAL FUNCTIONS

    /// the effects' update: in the 2017 update a jump to 0x5010A9, which reads the units the effects belong to
    [address((ee)0x005067CF, (aoc)0x00511D76), overrides(typeof(RTSRenderObject))]
    [vtable_site("ro_effects_update")]
    extern void update();
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

/// a model as the engine loaded it from its .cem (GEModel, Low-Level Engine.dll: a unit's EEUnit.model), the same
/// offsets in both builds' engines (their exports ComputeBoundingBoxes, SetModel)
[c_type("ee_model")]
unsafe struct GEModel
{
    /// its transformation (3 x 4 floats by row, as a unit's matrix): DrawModelHierarchy draws it there - under the
    /// hierarchy's stack's top when one is pushed - so a model drawn for several objects is set to each one's before
    [offset(0x88), found(c)]
    fixed float32 transform[12];

    /// the frame drawn: CopyStdVertices copies vertices[frame]
    [offset(0x134), found(c)]
    uint32 frame;

    /// a box per frame of its animation (0x24 each), in the model's own space: ComputeBoundingBoxes (engine 0x31218)
    /// from the frame's vertices
    [offset(0x110), found(c)]
    GEBoundingBox* boxes;

    /// per level of detail (lods): its triangles - DrawModelHierarchy takes one by the model's size on screen, each
    /// material's from its polygon_first[lod] (AllocateInternalData makes the first, LoadPolygonsChunk the others)
    [offset(0x100), found(c)]
    DX7PolygonArray** polygons;

    /// per frame: its vertices (a frame like another shares that one's array: AllocateInternalData); every material's
    /// are copied from the frame drawn into the vertex buffer each time it is drawn (CopyStdVertices)
    [offset(0x104), found(c)]
    DX7StandardVertexArray** vertices;

    /// its materials (material_count): each a texture and its share of the vertices and of each lod's triangles
    [offset(0x108), found(c)]
    GEMaterial** materials;

    /// its frames, the boxes' count
    [offset(0x140), found(c)]
    uint32 frames;

    /// LoadCountsChunk
    [offset(0x144), found(c)]
    uint32 material_count;

    /// the first lod's triangles (LoadCountsChunk; AllocateInternalData's polygons[0])
    [offset(0x148), found(c)]
    uint32 polygon_count;

    /// each frame's (LoadCountsChunk; AllocateInternalData's vertex arrays)
    [offset(0x14C), found(c)]
    uint32 vertex_count;

    /// levels of detail, polygons' count (LoadCountsChunk; 1 if the file has none)
    [offset(0x154), found(c)]
    uint32 lods;
}

/// a model's material (GEMaterial, Low-Level Engine.dll, 0x48: AllocateInternalData; its ranges LoadMaterialsChunk):
/// its vertices a run of the model's, its triangles a run of each lod's whose indices count from vertex_first - drawn
/// as one: the run copied into the vertex buffer, the triangles indexed into it (DrawModelHierarchy)
[c_type("ee_material")]
unsafe struct GEMaterial
{
    /// per screen: its texture (GETexture, the texture manager's - one per file, shared by the models that load it:
    /// LoadTexturesForScreen, SetTexture counts its users)
    [offset(0x24), found(c)]
    void** textures;

    /// per lod: its first triangle in the model's polygons[lod]
    [offset(0x2C), found(c)]
    uint32* polygon_first;

    /// per lod: its triangles (none: not drawn at that lod)
    [offset(0x30), found(c)]
    uint32* polygon_count;

    [offset(0x34), found(c)]
    uint32 vertex_first;

    [offset(0x38), found(c)]
    uint32 vertex_count;
}

/// the display DLLs' triangles (GEPolygonArray; DX7PolygonArray, the same in both and both builds:
/// DX7Rasterizer.create_polygon_array) - three 16-bit vertex indices each
[c_type("ee_polygons")]
unsafe struct DX7PolygonArray
{
    [offset(0x4), found(c)]
    uint16* indices;

    /// 3 x count
    [offset(0x8), found(c)]
    uint32 index_count;

    [offset(0xC), found(c)]
    uint32 count;
}

/// the display DLLs' vertices without colours (GEVertexArray; DX7StandardVertexArray, type 0 of
/// DX7Rasterizer.create_vertex_array - type 1 DX7StdColorVertexArray, another layout)
[c_type("ee_vertices")]
unsafe struct DX7StandardVertexArray
{
    [offset(0x4), found(c)]
    D3DVERTEX* data;

    [offset(0x8), found(c)]
    uint32 count;

    /// GetType: 0 this class, 1 DX7StdColorVertexArray
    [slot(8)]
    extern uint32 get_type();
}

/// a vertex of DX7StandardVertexArray (Direct3D 7's D3DVERTEX: CopyStdVertices)
[c_type("ee_vertex"), size(0x20)]
unsafe struct D3DVERTEX
{
    [offset(0x0), found(c)]
    vec3 point;

    [offset(0xC), found(c)]
    vec3 normal;

    [offset(0x18), found(c)]
    float32 u;

    [offset(0x1C), found(c)]
    float32 v;
}

/// the engine's box (GEBoundingBox::Define: the corners, then ComputeDimensions its size)
[c_type("ee_box"), size(0x24)]
unsafe struct GEBoundingBox
{
    [offset(0x0), found(c)]
    vec3 min;

    [offset(0xC), found(c)]
    vec3 max;
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

    /// the units' render object's collection (EEROUnits.draw): the terrain mesh's visible tile rows - how many, then
    /// each by its index (its y, first and last x: map tiles) - and the objects on their tiles; call
    /// [GetNumVisibleTileRows], call [GetVisibleTileRow] (the other GetNumVisibleTileRows, 0x4DB8A9, isn't the
    /// collection's)
    [address((ee)0x00503EBB, (aoc)0x0050EDF6)]
    static code call_units_rows;

    [address((ee)0x00503EE9, (aoc)0x0050EE24)]
    static code call_units_row;

    /// the collection's test of an object's model against the view (call [Intersects]): 0x5008B8's three (an object
    /// of its own, one riding another, a small object), the remembered small objects' (0x502224) and buildings'
    /// (0x5035AD) in the fog - all of the collection's (the other one, 0x625991, isn't)
    [address((ee)0x00500A2C, (aoc)0x0050B527)]
    [address((ee)0x00500B7A, (aoc)0x0050B675)]
    [address((ee)0x00500CD9, (aoc)0x0050B7D4)]
    [address((ee)0x0050239B, (aoc)0x0050D133)]
    [address((ee)0x00503741, (aoc)0x0050E55C)]
    static code calls_units_view;

    /// the exe's import slots of GETerrainMesh::GetNumVisibleTileRows, GetVisibleTileRow (thiscall: the row's index,
    /// then its y, first x, last x written) and GEPhysicalModel::Intersects (thiscall: the viewport, a flag; a bool)
    [import_slot("Low-Level Engine.dll", "?GetNumVisibleTileRows@GETerrainMesh@@QBEKXZ")]
    [address((ee)0x00823768, (aoc)0x00837728)]
    static block iat_terrain_rows;

    [import_slot("Low-Level Engine.dll", "?GetVisibleTileRow@GETerrainMesh@@QBEXKAAJ00@Z")]
    [address((ee)0x00823864, (aoc)0x00837838)]
    static block iat_terrain_row;

    [import_slot("Low-Level Engine.dll", "?Intersects@GEPhysicalModel@@QBE_NABVGEViewport@@_N@Z")]
    [address((ee)0x00823850, (aoc)0x00837820)]
    static block iat_model_intersects;

    /// the ground's update's (0x503A7F, AoC 0x50E8D1) call of GETerrainMesh::UpdateVisibleTerrain - thiscall on the mesh
    /// (0x91831C): the camera's transformation, the viewport, 2, 1, 3, game.camera_fixed; ret 0x18. The visible region
    /// it makes - the tiles under the frustum through the viewport's rectangle (GetFrustrumWorldPoints, all eight
    /// corners, the far plane's too, then GetTileRowsInVolume) - is the ground drawn and the rows the units' collection
    /// walks: call [UpdateVisibleTerrain], its only call in both builds
    [address((ee)0x00503B05, (aoc)0x0050E9AA)]
    static code call_terrain_visible;

    [import_slot("Low-Level Engine.dll", "?UpdateVisibleTerrain@GETerrainMesh@@QAEXABVGETransformation@@AAVGEViewport@@KKK_N@Z")]
    [address((ee)0x00823860, (aoc)0x00837830)]
    static block iat_terrain_visible;

    /// Low-Level Engine.dll's GETerrainMesh: the visible region's size in tiles less one, x then y at +4 - set by
    /// UpdateVisibleTerrain from GetTileRowsInVolume's corners (the 2017 update's engine 0x1003CD4D, AoC's 0x1002C299
    /// writes it at +0x1905B8). Its arrays per tile of the region are 101 x 101 (a row 0x65, memset 0x27D9 twice, a tile
    /// and its +1 neighbours written) and nothing checks the size against them
    [value((ee)0x17C6AC, (aoc)0x1905B8)]
    static uint32 of_mesh_visible_size;

    /// Low-Level Engine.dll's GEViewport: its rectangle (GERectangle, 4 floats: bottom, left, right, top - pixels;
    /// GetFrustrumWorldPoints' x from +4 and +8, y from +0xC and +0), the one UpdateVisibleTerrain's frustum goes through;
    /// both engines
    [value((ee)0x10, (aoc)0x10)]
    static uint32 of_viewport_rect;

    /// GEViewport, the rest GetFrustrumWorldPoints reads (2017 update 0x1004928A, AoC 0x100375A6 the same): the pixels'
    /// scale (+0x26C: a point x = (px - centre x) / it, y = -(py - centre y) / it), the centre (+0x274 x, +0x278 y),
    /// the near depth (+0x264: the corners' first points, z 0) and the far (+0x270: z 1), each corner then through
    /// DenormalizePerspectivePoint (x, y, z times the depth, the camera's inverse +0x170 applied)
    [value((ee)0x26C, (aoc)0x26C)]
    static uint32 of_viewport_scale;

    [value((ee)0x274, (aoc)0x274)]
    static uint32 of_viewport_centre;

    [value((ee)0x264, (aoc)0x264)]
    static uint32 of_viewport_near;

    [value((ee)0x270, (aoc)0x270)]
    static uint32 of_viewport_far;

    /// GEViewport: its four side planes' normals in the camera's space (ComputePlanes, 2017 update 0x10048E28, AoC the
    /// same: a point and a normal each, 0x18 apart - the left's normal at +0x5C, the right's +0x74, the top's +0x8C,
    /// the bottom's +0xA4; through the eye, y the depth: the left (cos h, sin h, 0), h half the field of view), what
    /// the display DLLs' TransformToCamera gives the ground's vertices their clip codes by
    [value((ee)0x5C, (aoc)0x5C)]
    static uint32 of_viewport_side_normals;

    /// GEViewport: its field of view (radians; ComputePlanes' left and right from half of it). The display DLLs'
    /// TransformToCamera makes the top and bottom planes from it itself (atan(+0x268 x tan(it / 2) / (+0x26C - 1))),
    /// not from the stored ones (2017 update DX7HRDisplay.dll 0x10007204)
    [value((ee)0x25C, (aoc)0x25C)]
    static uint32 of_viewport_fov;

    /// DX7HRTnLDisplay.dll: the caps of the vertex buffer each rasterizer state creates (D3DVERTEXBUFFERDESC.dwCaps, a
    /// mov [ebp-0xC], imm32: D3DVBCAPS_WRITEONLY - video memory; its other buffer and DX7HRDisplay.dll's add
    /// D3DVBCAPS_SYSTEMMEMORY)
    [value((ee)0xF475, (aoc)0x103CE)]
    static uint32 rva_tnl_vb_caps;

    /// the display DLLs' vertex buffer, in vertices: DX7Rasterizer::LockVB starts it anew when fewer than a material's
    /// are left - a material of more would overrun it (the 2017 update's both DLLs; AoC's DX7HRTnLDisplay.dll)
    [value((ee)0x98A, (aoc)0x960)]
    static uint32 vb_vertices;

    /// DX7Rasterizer::LockVB (exported by name): the immediate of its mov ecx, vb_vertices - the room it subtracts the
    /// buffer's place (+0x28 of the screen's state) from, unsigned: past it the place is never started anew and grows
    /// with every draw. Both DLLs of both builds
    [value((ee)0x15, (aoc)0x15)]
    static uint32 of_lock_vb_room;

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
