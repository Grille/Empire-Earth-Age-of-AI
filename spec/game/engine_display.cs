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
    // FUNCTIONS (the update's - the collection, 0x503B83 - for each object it meets on the visible tiles; the AoC
    // decompilation's names)

    /// IsUnitDrawn: whether the collection draws the object - its model against the view (calls_units_view), and not
    /// met on another tile before (met: a set of the update's own)
    [address((ee)0x005008B8, (aoc)0x0050B3B3)]
    extern uint8 drawn(EEUnit* u, ee_unit_vis* tile, void* met);

    /// GetUnitOpacity: the object's opacity into *opacity (its record's: +0xFC, a tree's +0x24); whether it is drawn
    /// see-through - the sort map it goes into (engine_display.sort_maps)
    [address((ee)0x005044D7, (aoc)0x0050F412)]
    extern uint8 opacity(EEUnit* u, float32* opacity);

    /// FillObject: a tree's record (hooks.draw_small_old, 0x3C each) into the sort map; info: the update's
    /// (walk_info)
    [address((ee)0x00501DFF, (aoc)0x0050CBA3)]
    extern void fill_object(EEUnit* u, void* record, void* sort_map, void* info);

    /// FillDoppelgangerUnit: a building as the player remembers it under the fog (the tile's doppelgangers) - its
    /// records from record on, counted in *count up to room (the list's: the operand drawlist.c moves), the tile x, y
    [address((ee)0x005035AD, (aoc)0x0050E3C8)]
    extern void fill_doppelganger(EEDoppelganger* d, void* record, uint32* count, void* sort_map, uint32 room, int32 x, int32 y, void* info);

    /// FillDoppelgangerObject: the same for a remembered tree (the trees' list and room)
    [address((ee)0x00502224, (aoc)0x0050CFBC)]
    extern void fill_doppelganger_object(EEDoppelganger* d, void* record, uint32* count, void* sort_map, uint32 room, int32 x, int32 y, void* info);

    /// the ground's update (the AoC decompilation's Fn10E8D1: the visible region of the ground for the camera, its
    /// light, the camera's transformation) as the units' update calls it at its start - collect.c leaves it out of an
    /// update run during a step (the region stays the last one's, as the ground's own update is skipped then)
    [address((ee)0x00503A7F, (aoc)0x0050E8D1)]
    [call_site("units_ground_pass", (ee)0x00503C4F, (aoc)0x0050EB8A)]
    extern void ground_pass();

    /// FillGhostRecords: after the walk, a record (ghost_records' element) for each placement ghost (ghost_units),
    /// when they changed (ghosts_changed), inside their lock (ghost_lock) - collect.c's hook of its call
    [address((ee)0x00501C29, (aoc)0x0050C952)]
    [call_site("fill_ghosts", (ee)0x005041F7, (aoc)0x0050F132)]
    extern void fill_ghosts();

    /// one record of the draw list drawn: its model, its parts - render_silhouette.c's hook of its calls, which tells the
    /// renderer whose draws follow
    [address((ee)0x0050B4AC, (aoc)0x00515CC7)]
    [call_site("units_draw_record", (ee)0x0050A578, (aoc)0x00514D93)]
    [call_site("units_draw_record", (ee)0x0050A67A, (aoc)0x00514E95)]
    [call_site("units_draw_record", (ee)0x0050BB78, (aoc)0x0051646D)]
    extern void draw_record(void* record);

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

/// a line from an airport to its target (the graphical goal manager's kind 0): an effect of the third queue, drawn in pixels
/// from world points (0x50DD81, AoC 0x518676)
[vtable((ee)0x0082B9D4, (aoc)0x0084099C)]
unsafe struct EEE3DMarchingLine
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

/// a model as the engine loaded it from its .cem (GEModel, Low-Level Engine.dll: a unit's EEUnit.model), the same
/// offsets in both builds' engines (their exports ComputeBoundingBoxes, SetModel)
[c_type("ee_model")]
unsafe struct GEModel
{
    /// its centre in its own space (GE3DPoint; the AoC decompilation's mCenterPoint): the units' opacity puts it
    /// through the unit's transformation and the camera's inverse for its depth
    [offset(0xC8), found(c)]
    vec3 center;

    /// a bounding radius per frame (the AoC decompilation's mBoundingRadii): the units' opacity takes the frame's,
    /// times the unit's scale, off the centre's depth
    [offset(0x124), found(c)]
    float32* radii;

    /// its transformation (3 x 4 floats by row, as a unit's matrix): DrawModelHierarchy draws it there - under the
    /// hierarchy's stack's top when one is pushed - so a model drawn for several objects is set to each one's before
    [offset(0x88), found(c)]
    fixed float32 transform[12];

    /// the frame drawn: CopyStdVertices copies vertices[frame]
    [offset(0x134), found(c)]
    uint32 frame;

    /// the frame blended toward: CopyAndSmoothStdVertices copies (1 - fraction) x vertices[frame] + fraction x
    /// vertices[next_frame] (position and normal; the texture coordinates frame's)
    [offset(0x138), found(c)]
    uint32 next_frame;

    /// how far between frame and next_frame (SetFractionalFramePosition); the exe's unit draw sets 0 unless smoothing
    /// is on (0x50B4AC), and 0 draws by CopyStdVertices
    [offset(0x130), found(c)]
    float32 fraction;

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

    /// each material's texture file name (material_count of them, in the materials' order): LoadTexturesForScreen
    /// loads each by it, one texture a file. A model can have its names before its textures (a tower of a scenario with
    /// none on the map, read in a running game: its names there, its materials' textures 0)
    [offset(0x11C), found(c)]
    UString* texture_names;

    /// its frames, the boxes' count (the trees' fill: a tree's model of more than one sways)
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

/// the units' collection's (EEROUnits::vf0) list of what it drew this frame, at engine_display.pick_list: an entry
/// per object - its model, a transformation, the object (it takes a reference to it, 0x50F6E2) - the mouse's to pick
/// from [guess]: the entries hold what a pick test needs and nothing reads them in the drawing; hooks_functions.pick_listed
/// searches it
[c_type("ee_pick_list")]
unsafe struct EEPickList
{
    [offset(0x2A0), found(c)]
    EEPickEntry* entries;

    /// the entries' room (the constructor's 0x600; the add 0x50F6E2 drops an object past it - collect.c gives it more)
    [offset(0x2A4), found(c)]
    uint32 room;

    [offset(0x2A8), found(c)]
    uint32 count;

    /// Reset (the AoC decompilation's EEUnitPicker): emptied (0x50F5EF) and the map's viewport copied in - the
    /// collection starts it over each update before it walks the tiles
    [address((ee)0x0050F663, (aoc)0x00519F7C)]
    extern void reset();

    /// PickUnits: the player's objects in the rectangle (the selection box being drawn) into a list - it reads through
    /// the entries' objects; the units' update's call, which collect.c leaves out of an update run during a step
    [address((ee)0x0050F589, (aoc)0x00519EA2)]
    [call_site("units_pick_rect", (ee)0x00503D44, (aoc)0x0050EC7F)]
    extern void pick_units(void* rectangle, EEPlayer* player, int32 type, void* into);

    /// PickUnit: the object under a screen point, a reference taken on it for the caller; the units' update's call for
    /// the unit under the mouse, which collect.c leaves out of an update run during a step (answer: none)
    [address((ee)0x0050EC37, (aoc)0x00519540)]
    [call_site("units_pick_mouse", (ee)0x00503E7E, (aoc)0x0050EDB9)]
    extern EEUnit* pick_unit(void* point, int32 prefer_resources);
}

[c_type("ee_pick_entry"), size(0x50)]
unsafe struct EEPickEntry
{
    [offset(0x4C), found(c)]
    EEUnit* unit;
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

/// the engine's narrow string (0x10 each in GEModel.texture_names: LoadTexturesForScreen's step; its length at +8)
[c_type("ee_ustring"), size(0x10)]
unsafe struct UString
{
    /// its characters, 0 at the end
    [offset(0x4), found(c)]
    char8* text;
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

    /// GETerrainMesh's polygons (GETMPolygon, 0x30 each: three vertex pointers first, the clip codes the display DLLs'
    /// TransformToCamera reads at a vertex's +0x20), which UpdateVisibleTerrain makes for its region from two pools in the
    /// mesh, taken in turn and never checked: the ground's (12000 - every tile's two, two more for each of up to four
    /// neighbours' textures blended over it, four for a map edge's skirt) and the water's (5000 - a water tile's two).
    /// Each pool is followed by its vertex grid (101 x 101), so a region with more runs into the grid: the grid then
    /// overwrites the polygons (the 2017 update's constructor 0x100066A8, AoC's 0x100273C9)
    [value((ee)0xCEC, (aoc)0xD28)]
    static uint32 of_mesh_ground_polygons;

    [value((ee)0xE7170, (aoc)0xF1110)]
    static uint32 of_mesh_water_polygons;

    [value((ee)12000, (aoc)12000)]
    static uint32 mesh_ground_polygons;

    [value((ee)5000, (aoc)5000)]
    static uint32 mesh_water_polygons;

    /// GETerrainMesh: the index of the water's colour set among its colour sets (SetWaterColorSet: a corner under the water
    /// takes the slot height * of_mesh_water_scale * (slots - 1) of it, past the last the last)
    [value((ee)0x17C6C4, (aoc)0x1905D0)]
    static uint32 of_mesh_water_set;

    /// GETerrainMesh: 33 x 33 numbers, which of the water's groups a tile takes: the one at (x % 33) * 33 + y % 33
    /// (SetGlobalWaterState fills them at random, UpdateVisibleTerrain reads them) - tiles side by side at different
    /// frames of the animation
    [value((ee)0x17B5A8, (aoc)0x18F4B4)]
    static uint32 of_mesh_water_patches;

    /// GETerrainMesh: float, that scale (SetWaterColorSet's flds)
    [value((ee)0x17B580, (aoc)0x18F488)]
    static uint32 of_mesh_water_scale;

    [value((ee)0x30, (aoc)0x30)]
    static uint32 size_mesh_polygon;

    /// UpdateVisibleTerrain (exported by name): its lea eax, [mesh + of_mesh_ground_polygons] (6 bytes: 8D, a modrm
    /// [reg + disp32] into eax, the offset) that starts the ground's pool - the 2017 update's with edi, AoC's with esi
    [value((ee)0x3A1, (aoc)0x3A4)]
    static uint32 of_visible_ground_pool;

    /// and its lea eax, [mesh + of_mesh_water_polygons], the water's
    [value((ee)0x3AA, (aoc)0x3AD)]
    static uint32 of_visible_water_pool;

    /// GETerrainMesh: its polygons by texture - a vector (MSVC: the first element at +4, the end at +8) of groups
    /// (0x34 each; the group's own vector of polygon pointers at +4: the first at +8, the end at +0xC) - the ground's
    /// at +0x10, the water's (one per frame of its animation, UpdateWaterAnimation) at +0x30; what the display DLLs'
    /// TransformToCamera walks. Both builds
    [value((ee)0x10, (aoc)0x10)]
    static uint32 of_mesh_ground_groups;

    [value((ee)0x30, (aoc)0x30)]
    static uint32 of_mesh_water_groups;

    [value((ee)0x34, (aoc)0x34)]
    static uint32 size_mesh_group;

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

    /// GEViewport: its camera's transformation (a GETransformation, 4 rows - the eye in the fourth column), what
    /// GEViewport::SetCamera copies (2017 update 0x10005E48, AoC 0x1000E910 the same: then the look vector at +0x230, the
    /// inverse at +0xF0, the projections made again) - the camera the display DLLs' TransformToCamera puts the ground
    /// into its space with
    [value((ee)0xB0, (aoc)0xB0)]
    static uint32 of_viewport_camera;

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

    /// the frame's light, worked out by the frame's lighting (0x50130D) for the hour and handed to the device by the
    /// units' draw (0x504BA4, AoC 0x50FA01): 3 floats, the direction the light travels - the world's sun or moon vector
    /// (EEWorld +0x98C / +0x980) for the hour
    [address((ee)0x008D2A70, (aoc)0x008E9210)]
    static block light_direction;

    /// its ambient colour (3 floats), times light_ambient_intensity and light_fade
    [address((ee)0x00912564, (aoc)0x00928D04)]
    static block light_ambient;

    /// float
    [address((ee)0x009125A0, (aoc)0x00928D40)]
    static block light_ambient_intensity;

    /// the light's colour (3 floats), times light_intensity and light_fade
    [address((ee)0x00912570, (aoc)0x00928D10)]
    static block light_color;

    /// float
    [address((ee)0x009125A4, (aoc)0x00928D44)]
    static block light_intensity;

    /// float, 0..1: the picture's fade in and out, over every light
    [address((ee)0x008A7F68, (aoc)0x008BE368)]
    static block light_fade;

    /// byte: the units are drawn with that light; 0: an ambient colour alone
    [address((ee)0x00919462, (aoc)0x009308E3)]
    static block light_on;

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

    /// the units' collection's list of what it drew (EEPickList; its address the collection passes, 0x503F8D)
    [address((ee)0x009125D8, (aoc)0x00928D78)]
    static block pick_list;

    /// THE EFFECTS' QUEUES (the AoC decompilation's gEffectQueues; EEROEffects' update, QueueEffects EE 0x5010A9, AoC
    /// 0x50BDD2): five maps from a texture to the effects to draw with it, remade each full update - the old entries
    /// released, then of every map region under the view's rows each effect shown to the player asked its PrepareQueue
    /// (slot 11) and Queue(the queues) (slot 9: its view test, its draw state copied from what the steps wrote - mDrawAge ...
    /// -, its entry; 0 = queued, and a reference is taken at +4)
    [address((ee)0x008D2C78, (aoc)0x008E9418)]
    static block effect_queues;

    /// the map's table of effects by region (RTSRegionTable: +4 the regions, 0x10 bytes each with a vector of EEEffect*
    /// at +4 .. +8; +8 a region's size in tiles; +0x10 its width, +0x14 its height - a region at x * height + y)
    [value((ee)0x181720, (aoc)0x19563C)]
    static uint32 of_map_effect_regions;

    /// an effect's virtual Queue and PrepareQueue, its player (+0x20: whose alone it is when) its local-only byte (+0x2D) is 1
    [value((ee)9, (aoc)9)]
    static uint32 slot_effect_queue;

    [value((ee)11, (aoc)11)]
    static uint32 slot_effect_prepare;

    /// an effect's virtual Draw (EEROTerrain's draw calls it on the third queue's: EE 0x508390, AoC 0x5131A4)
    [value((ee)4, (aoc)4)]
    static uint32 slot_effect_draw;

    [value((ee)0x20, (aoc)0x20)]
    static uint32 of_effect_player;

    [value((ee)0x2D, (aoc)0x2D)]
    static uint32 of_effect_local_only;

    /// the collection's lock over pick_list (a TSCriticalSection*; the AoC decompilation's gUnitPickerLock): the
    /// update walks the tiles inside it
    [address((ee)0x00912874, (aoc)0x00929014)]
    static block picker_lock;

    /// the draw lists' sort maps (std::map heads, 12 bytes each): the objects' (+0), the objects' see-through (+0xC),
    /// a third (+0x18, emptied with them), the trees' (+0x24), the trees' see-through (+0x30), a sixth (+0x3C) - the
    /// collection's opacity (EEROUnits.opacity) chooses between the first two of each, the draw goes through them
    [address((ee)0x008D2CB8, (aoc)0x008E9458)]
    static block sort_maps;

    /// the collection's walk over the visible tiles (the update 0x503B83 from the picker's lock to its Leave; the same
    /// instructions in AoC): its first, mov ecx, [picker_lock] (6 bytes) - collect.c's patch -, and the update's next
    /// after the walk
    [address((ee)0x00503E9F, (aoc)0x0050EDDA)]
    static code units_walk;

    [address((ee)0x005041F4, (aoc)0x0050F12F)]
    static code units_walk_end;

    /// the simulation's loop (RTSPhysics::Main, EE 0x5532FB) takes the world's write lock for its steps through
    /// EEPhysics' vf5 (EE 0x546B0A, AoC 0x552F29: the lock in ecx, a jmp to TSReadWriteLock::WriteLock), its steps
    /// (vf8) under it, and lets go through vf10: the return address the lock sees (after its call [eax+0x14], EE
    /// 0x55340E, AoC 0x56151B) - the units' collection copies the world when the steps let go of it (collect.c's copy;
    /// the other writers - the interface's, the lobby's, a load - only make it stale)
    [address((ee)0x00553411, (aoc)0x00561522)]
    static code step_write_return;

    /// the update's frame (ebp less these) as the walk reads it, the same in AoC: the render object, the local player,
    /// the map, the objects with a physics part counted (float, for the music), the info its fills get (the AoC
    /// decompilation's: the unit under the mouse +0, the camera's yaw +4, the fighting units +8, the frame +0xC, the
    /// ambient sound's turn +0x10), the doppelgangers met (a set: a building on several tiles), the objects met
    [value((ee)0x4, (aoc)0x4)]
    static uint32 walk_this;

    [value((ee)0x1C, (aoc)0x1C)]
    static uint32 walk_player;

    [value((ee)0x24, (aoc)0x24)]
    static uint32 walk_map;

    [value((ee)0x28, (aoc)0x28)]
    static uint32 walk_complex;

    [value((ee)0x3C, (aoc)0x3C)]
    static uint32 walk_info;

    [value((ee)0x68, (aoc)0x68)]
    static uint32 walk_doppelgangers;

    [value((ee)0x88, (aoc)0x88)]
    static uint32 walk_met;

    /// the engine's lock over the graphic objects' users (a TSCriticalSection*): EEGraphicObject.acquire takes it
    [address((ee)0x008D1D50, (aoc)0x008E8430)]
    static block graphics_lock;

    /// the map's GEViewport (its rectangle is game.select_rect, +0x10) +0xF0: the world into the camera (a
    /// GETransformation, 4 rows: the inverse of the camera's transformation at +0xB0 - GEViewport::SetCamera, at the
    /// camera's step)
    [address((ee)0x008D2858, (aoc)0x008E8FF8)]
    static block viewport_view;

    /// its operands (mov ecx, viewport_view) in the units' draw (EEROUnits.draw): a world point to the screen's pixels
    /// (0x50DD81, AoC 0x518676: the selected units' health bars, the building's footprint lines, the goal markers, the
    /// waypoints' lines), the depth range of the building being placed (0x50A4A1) and of the objects drawn over the
    /// rest (0x50B9A4) by their depth in the camera. The fourth (0x53C193: a sound's pan and volume by its place on
    /// the screen) isn't the draw's
    [address((ee)0x0050DD99, (aoc)0x0051868E)]
    [address((ee)0x0050A5F5, (aoc)0x00514E10)]
    [address((ee)0x0050BA99, (aoc)0x0051638E)]
    static code viewport_view_ops;

    /// THE UNITS' OPACITY (EEROUnits.opacity, the AoC decompilation's GetUnitOpacity; collect.c's port) reads these.
    /// The "show hidden units" mode (int; gRenderEnvironment.mShowHiddenUnits): 1 an obstacle or a moved unit hidden
    /// behind something (EEUnit.update_hidden) drawn see-through, 2 every obstacle
    [address((ee)0x009125C8, (aoc)0x00928D68)]
    static block show_hidden;

    /// float constants of its code: a hidden one's opacity; 1 (the cloak's full opacity, the depth range's numerator);
    /// a cloaked unit's opacity to an enemy (kCloakedOpacity); 0; how near the front plane a land building starts to fade
    /// (times the depth range's inverse); a tile's middle (0.5)
    [address((ee)0x0082C2F0, (aoc)0x00841350)]
    static block hidden_opacity;

    [address((ee)0x008254DC, (aoc)0x0083948C)]
    static block float_one;

    [address((ee)0x0082C314, (aoc)0x00849B04)]
    static block cloaked_opacity;

    [address((ee)0x00824E64, (aoc)0x00838EDC)]
    static block float_zero;

    [address((ee)0x0082C30C, (aoc)0x0084136C)]
    static block front_fade;

    [address((ee)0x00824AD0, (aoc)0x00838AAC)]
    static block float_half;

    /// float: 1 less cloaked_opacity (kCloakedOpacityRange, made at the start), the cloak's range
    [address((ee)0x009128C0, (aoc)0x00964498)]
    static block cloaked_range;

    /// the GEViewport its land buildings fade by (the AoC decompilation's gViewport; not the map's, viewport_view's):
    /// its camera's inverse at +0xF0 (opacity_view_inverse), its front and rear clipping distances at +0x264 / +0x270
    /// (of_viewport_near, of_viewport_far)
    [address((ee)0x00919470, (aoc)0x009308F0)]
    static block opacity_viewport;

    [value((ee)0xF0, (aoc)0xF0)]
    static uint32 of_viewport_inverse;

    /// the exe's import slots of GETransformation::Apply (thiscall on the transformation: a GE3DPoint in, one out) and
    /// GETerrainMesh::GetMeshZ (thiscall on the mesh: x, y; the ground's height, a float on the FPU's stack)
    [import_slot("Low-Level Engine.dll", "?Apply@GETransformation@@QBEXABVGE3DPoint@@AAV2@@Z")]
    [address((ee)0x008231C4, (aoc)0x008371C0)]
    static block iat_transform_apply;

    [import_slot("Low-Level Engine.dll", "?GetMeshZ@GETerrainMesh@@QBEMMM@Z")]
    [address((ee)0x0082327C, (aoc)0x00837278)]
    static block iat_mesh_z;

    /// a unit's script state while it dies and rots (EEUnitAction.id; the AoC decompilation's kActionUnitRotting): the
    /// opacity is its rot_ms over its type's rot_time
    [value((ee)0x1772, (aoc)0x1772)]
    static uint32 state_rotting;

    /// EEUnit.hidden_flags' moved bit (kHiddenFlagMoved)
    [value((ee)0x10, (aoc)0x10)]
    static uint32 hidden_moved;

    /// THE UNITS' VIEW TEST (EEROUnits.drawn, the AoC decompilation's IsUnitDrawn; collect.c's port) reads these. The
    /// camera's yaw (float; gRenderEnvironment.mCameraYaw): an object without a complex unit (a remembered one, a
    /// tree) is tested turned to it
    [address((ee)0x009125C4, (aoc)0x00928D64)]
    static block camera_yaw;

    /// float 0.125: a tree's shift off its tile's middle by its id (id % 3 - 1, id / 3 % 3 - 1, times its size and this)
    [address((ee)0x0082B380, (aoc)0x0084014C)]
    static block tree_shift;

    /// a unit's script state once dead (EEUnitAction.id; kActionDead): not drawn
    [value((ee)0x1773, (aoc)0x1773)]
    static uint32 state_dead;

    /// the type of a path point (kUnitTypePathPoint): not drawn but as a complex unit's
    [value((ee)0xDF, (aoc)0xDF)]
    static uint32 type_path_point;

    /// the exe's import slots of Low-Level Engine.dll's GETransformation::SetOrientationYPR (thiscall: yaw, pitch, roll),
    /// its operator= (thiscall: the other; returns this), GEPhysicalModel::SetModel (thiscall: a GEModel) and its
    /// destructor - the view test's own physical model (0x4C bytes: a GETransformation, the model +0x40, the scale
    /// +0x44, the frame +0x48) and transformation, on its stack
    [import_slot("Low-Level Engine.dll", "?SetOrientationYPR@GETransformation@@QAEXMMM@Z")]
    [address((ee)0x00823408, (aoc)0x0083739C)]
    static block iat_set_orientation;

    [import_slot("Low-Level Engine.dll", "??4GETransformation@@QAEAAV0@ABV0@@Z")]
    [address((ee)0x008232F4, (aoc)0x0083738C)]
    static block iat_transform_assign;

    [import_slot("Low-Level Engine.dll", "?SetModel@GEPhysicalModel@@QAEXPAVGEModel@@@Z")]
    [address((ee)0x00823848, (aoc)0x00837818)]
    static block iat_set_model;

    [import_slot("Low-Level Engine.dll", "??1GEPhysicalModel@@QAE@XZ")]
    [address((ee)0x0082384C, (aoc)0x0083781C)]
    static block iat_model_free;

    [import_slot("Low-Level Engine.dll", "??0GETransformation@@QAE@XZ")]
    [address((ee)0x00823254, (aoc)0x00837250)]
    static block iat_transform_new;

    [value((ee)0x4C, (aoc)0x4C)]
    static uint32 size_physical_model;

    [value((ee)0x40, (aoc)0x40)]
    static uint32 of_physical_model_model;

    [value((ee)0x44, (aoc)0x44)]
    static uint32 of_physical_model_scale;

    [value((ee)0x48, (aoc)0x48)]
    static uint32 of_physical_model_frame;

    /// THE UNITS' FILL (EEROUnits' FillUnit, EE 0x502622, AoC 0x50D3AD; collect.c's port uc_fill) reads and writes
    /// these. The ambient sounds' (thiscall ambient_add / ambient_add_object on it, while the update's info asks: its
    /// turn, +0x10)
    [address((ee)0x009174E4, (aoc)0x0092F5D4)]
    static block ambient_sounds;

    /// a vector of unit ids (the first at +4, the end at +8): a unit in it gets its health bar drawn [guess: what
    /// adds them]
    [address((ee)0x008D2A90, (aoc)0x008E9230), guess]
    static block bar_ids;

    /// a vector of the records whose health bars are drawn (vector_push adds one)
    [address((ee)0x008D2D10, (aoc)0x008E94B0)]
    static block bar_records;

    /// the record of the local player's unit under the mouse (ui_unit), and of another player's (its owner's name
    /// in hover_name); show_labels: the stats' name into the record of the info's picked unit also when not the
    /// player's [guess]
    [address((ee)0x00912530, (aoc)0x00928CD0)]
    static block ui_record;

    [address((ee)0x009128A4, (aoc)0x00929044)]
    static block hover_record;

    [address((ee)0x00912890, (aoc)0x00929030)]
    static block hover_name;

    [address((ee)0x00912D60, (aoc)0x00929510), guess]
    /// a flag: the fill copies the stats' name into the record of the info's picked unit while it is set and the unit is not the player's
    static block show_labels;

    /// the selection circles' size (int): 0 the type's mark_size, 1 twice the sight, 2 twice the range
    [address((ee)0x009128D0, (aoc)0x0092906C)]
    static block circle_mode;

    /// byte: count the fighting units for the music (the update's info +8: one more for each unit whose goal's state
    /// is 2, 3 or 11)
    [address((ee)0x0091945F, (aoc)0x009308E0)]
    static block music_counts;

    /// float: a path marker is drawn at least this high
    [address((ee)0x009125B0, (aoc)0x00928D50)]
    static block marker_min_z;

    /// float constants of the fill's code: the tint's fourth value, a full bar (1), a path marker's colour step (by
    /// its target's id), the tilt's offset off the arc cosine
    [address((ee)0x008257CC, (aoc)0x00839784)]
    static block tint_alpha;

    [address((ee)0x00827B3C, (aoc)0x0083C324)]
    static block bar_full;

    [address((ee)0x0082C370, (aoc)0x008413C8)]
    static block marker_color_step;

    [address((ee)0x00825984, (aoc)0x0083C008)]
    static block tilt_offset;

    /// AoC's: how far its building sites of two types (0x302, 0x31F) are drawn sunk
    [address((ee)none, (aoc)0x008CC9FC)]
    static block sink_302;

    [address((ee)none, (aoc)0x008CCA00)]
    static block sink_31f;

    /// the gate's class the fill casts a gate's type to (__RTDynamicCast's type descriptors: from, to)
    [address((ee)0x008A0A98, (aoc)0x008B6700)]
    static block rtti_unit_class;

    [address((ee)0x008A0AB8, (aoc)0x008B6720)]
    static block rtti_gate_class;

    /// GETerrainMesh (the map's +0x1C): the rows of tiles it has (a count, then a row's first x and its count, 0xC
    /// each); EEMap: a row's tiles' light (0xC each: the first x, the data +8; 4 bytes a tile, the fill tests the
    /// fourth)
    [value((ee)0x17C6D0, (aoc)0x1905DC)]
    static uint32 of_mesh_rows_n;

    [value((ee)0x17B57C, (aoc)0x18F480)]
    static uint32 of_mesh_rows;

    [value((ee)0x17B598, (aoc)0x18F49C)]
    static uint32 of_map_light_rows;

    /// the player's selected group (EEPlayer.ui_group): its vf3, thiscall(group, unit) - whether the unit is in it (the
    /// fill draws the health bars of one that is)
    [value((ee)3, (aoc)3)]
    static uint32 slot_group_contains;

    /// EEComplexUnit.producing's nothing
    [value((ee)100000, (aoc)100000)]
    static uint32 producing_none;

    /// the exe's import slots of UWideString::operator=, GETransformation::Apply (a GEVector), UMArcCos and
    /// GETransformation::Scale (x, y, z)
    [import_slot("Low-Level Engine.dll", "??4UWideString@@QAEAAV0@ABV0@@Z")]
    [address((ee)0x008231D0, (aoc)0x008371CC)]
    static block iat_wstring_assign;

    [import_slot("Low-Level Engine.dll", "?Apply@GETransformation@@QBEXABVGEVector@@AAV2@@Z")]
    [address((ee)0x008233FC, (aoc)0x00837394)]
    static block iat_transform_apply_vector;

    [import_slot("Low-Level Engine.dll", "?UMArcCos@@YAMM@Z")]
    [address((ee)0x008237E4, (aoc)0x00837754)]
    static block iat_arc_cos;

    [import_slot("Low-Level Engine.dll", "?Scale@GETransformation@@QAEXMMM@Z")]
    [address((ee)0x008237DC, (aoc)0x008377B4)]
    static block iat_transform_scale;

    /// THE TREES' FILL (EEROUnits' FillObject; collect.c uc_fill_tree): the trees' health bars' records (a vector,
    /// vector_push_small adds one), the sway's clock (int), the scale's step by id, the sway's step
    [address((ee)0x008D2D20, (aoc)0x008E94C0)]
    static block tree_bar_records;

    [address((ee)0x009128E8, (aoc)0x00929084)]
    static block sway_clock;

    [address((ee)0x0082C2F4, (aoc)0x00841354)]
    static block tree_scale_step;

    [address((ee)0x0082C2E4, (aoc)0x00841344)]
    static block sway_step;

    /// the exe's import slot of GETransformation::SetPosition (x, y, z); UMCos's is game.iat_cos
    [import_slot("Low-Level Engine.dll", "?SetPosition@GETransformation@@QAEXMMM@Z")]
    [address((ee)0x00823414, (aoc)0x008373B0)]
    static block iat_set_position;

    /// EEUnit.resource_type's wood (EE's trees' fill hands only a wood tree to the ambient sounds)
    [value((ee)3, (aoc)3)]
    static uint32 resource_wood;

    /// EEUnit.resource's food (kResourceFood): an animal or a bush of the gaia player with food left is drawn on a
    /// tile the player hasn't in sight (its fog) as long as it has a complex unit and isn't a citizen
    [value((ee)2, (aoc)2)]
    static uint32 resource_food;

    /// THE DOPPELGANGERS' FILLS (EEROUnits.fill_doppelganger, fill_doppelganger_object; collect.c uc_fill_dopp,
    /// uc_fill_dopp_tree): the exe's import slot of GETransformation::MakeIdentity - a remembered building without a
    /// pose of its own starts from it
    [import_slot("Low-Level Engine.dll", "?MakeIdentity@GETransformation@@QAEXXZ")]
    [address((ee)0x008232F0, (aoc)0x0083772C)]
    static block iat_make_identity;

    /// THE GHOSTS' FILL (EEROUnits.fill_ghosts; collect.c uc_fill_ghosts): the placement ghosts - the world's vector of
    /// EEGhostUnit* (EEGhostUnit::Create, ghost_unit.create, fills it: a building being placed, an airport's runways,
    /// a granary's farms), their lock (a TSCriticalSection*), whether they changed since (a byte: the preview sets it,
    /// the fill makes records only then), the frame's vector of their records (0x420 each: an ee_draw_record, then
    /// float[12][12] the footprint's corner heights, uint8[12][12] its cells, the tile x, y; push_back
    /// ghost_record_push), the lift over the ground every record gets (a float, 0.001)
    [address((ee)0x00918CC0, (aoc)0x00931700)]
    static block ghost_units;

    [address((ee)0x00918E18, (aoc)0x00931858)]
    static block ghost_lock;

    [address((ee)0x00941444, (aoc)0x00963240)]
    static block ghosts_changed;

    [address((ee)0x008D2D00, (aoc)0x008E94A0)]
    static block ghost_records;

    [address((ee)0x00827AC4, (aoc)0x0083C2A4)]
    static block ghost_lift;

    /// a ghost record's size and its parts' offsets
    [value((ee)0x420, (aoc)0x420)]
    static uint32 size_ghost_record;

    [value((ee)0x148, (aoc)0x148)]
    static uint32 of_ghost_record_heights;

    [value((ee)0x388, (aoc)0x388)]
    static uint32 of_ghost_record_cells;

    [value((ee)0x418, (aoc)0x418)]
    static uint32 of_ghost_record_tile;
}

/// the game's functions engine_display.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class engine_display_functions
{
    /// thiscall(the collection's set of the doppelgangers met (walk_doppelgangers), found: its node, &the doppelganger):
    /// find - the set's head (+4) when not met (std::set; AoC's is a shared copy far off, 0x42B588). A set of pointers:
    /// the view test (EEROUnits.drawn) keeps the objects met (walk_met) in one too, with the same two functions
    [address((ee)0x005050BD, (aoc)0x0042B588), thiscall]
    static extern void* doppelganger_set_find(void* set, void** found, EEDoppelganger** d);

    /// the fill's helpers. thiscall(engine_display.ambient_sounds, a complex unit / an object): it into the ambient
    /// sounds' turn
    [address((ee)0x0051A4C5, (aoc)0x00524D63), thiscall]
    static extern void ambient_add(void* sounds, EEUnit* cu);

    [address((ee)0x00506249, (aoc)0x00524E80), thiscall]
    static extern void ambient_add_object(void* sounds, EEUnit* u);

    /// MSVC's vector: thiscall(v, n, value) resize (a 4-byte value); thiscall(v) clear; thiscall(v, &value) push_back
    [address((ee)0x004D0945, (aoc)0x0041D79B), thiscall]
    static extern void vector_resize(void* v, uint32 n, uint32 value);

    [address((ee)0x00526AFF, (aoc)0x005B5772), thiscall]
    static extern void vector_clear(void* v);

    [address((ee)0x00504F0A, (aoc)0x0050FDA9), thiscall]
    static extern void vector_push(void* v, void** value);

    /// the sort maps' insert: thiscall(map, found: 8 bytes, a pair {key, record})
    [address((ee)0x0044F431, (aoc)0x0051005E), thiscall]
    static extern void sort_map_insert(void* map, void** found, void** pair);

    /// an effects' queue (std::map from a texture to a deque of EEEffect*, 12 bytes: engine_display.effect_queues): made,
    /// emptied (the deques and nodes; the effects' references are the caller's)
    [address((ee)0x00521704, (aoc)0x0056D077), thiscall]
    static extern void effect_queue_new(void* queue);

    [address((ee)0x00504FC8, (aoc)0x0050FE93), thiscall]
    static extern void effect_queue_clear(void* queue);

    /// an object's reference given back (RTSObject: the count at +4, its slot 13 at the last): plain, not interlocked -
    /// inside the world's lock only
    [address((ee)0x004FCAD3, (aoc)0x00401DB4), thiscall]
    static extern void object_release(void* o);

    /// EEReleaseScene: the frame's lists, the effects' queues and the picker emptied (a game's end, a load): the graphics
    /// thread's two calls of it; collect_copy.c gives back what the simulation's effects' queues hold first
    [address((ee)0x005042FE, (aoc)0x0050F239), cdecl]
    [call_site("scene_release", (ee)0x004FED15, (aoc)0x00509828)]
    [call_site("scene_release", (ee)0x004FEE3B, (aoc)0x0050994E)]
    static extern void scene_release();

    /// thiscall(engine_display.pick_list, the unit, its record, its opacity): it into the list the mouse picks from
    [address((ee)0x0050F6E2, (aoc)0x00519FFB), thiscall]
    static extern void pick_list_add(void* list, EEUnit* u, void* record, float32 opacity);

    /// thiscall(map, float pos[3], float out[3]): out the point on the ground under pos (its z when off the mesh)
    [address((ee)0x004C0201, (aoc)0x004C0894), thiscall]
    static extern void ground_point(void* map, float32* pos, float32* @out);

    /// thiscall(a path marker's target's complex unit, x, y, float** at): where the marker at tile x, y is drawn on it
    [address((ee)0x005EDB5E, (aoc)0x005FBC3B), thiscall]
    static extern uint8 marker_place(EEUnit* target, int32 x, int32 y, float32** at);

    /// MSVC's __RTDynamicCast(object, 0, from, to, 0) and the gate's class's test of a complex unit (thiscall on the
    /// cast type) [guess: what it tests - drawn open]
    [address((ee)0x00683720, (aoc)0x0069D180), cdecl]
    static extern void* rt_dynamic_cast(void* o, int32 vf_delta, void* from, void* to, int32 is_ref);

    [address((ee)0x00414C14, (aoc)0x0041076F), thiscall, guess]
    static extern uint8 gate_drawn_open(void* gate_class, EEUnit* cu);

    /// AoC's: EERenderObject::DrawGroundCircle (thiscall on the collection: a transformation, a radius, a mode) and
    /// EEUnit::GetLineOfSight (thiscall: a float) - the AoC decompilation's names
    [address((ee)none, (aoc)0x0050BBA4), thiscall]
    static extern void ground_circle(void* collection, float32* at, float32 radius, int32 mode);

    [address((ee)none, (aoc)0x005F9DCB), thiscall]
    static extern float32 line_of_sight(EEUnit* cu);

    /// the trees' fill's: thiscall(engine_display.ambient_sounds, a tree) into the ambient sounds' turn (AoC's the
    /// object's, ambient_add_object); cdecl(tile): a tree on it (its objects' types' is_tree); push_back into
    /// tree_bar_records' vector type
    [address((ee)0x0051A5D5, (aoc)0x00524E80), thiscall]
    static extern void ambient_add_tree(void* sounds, EEUnit* u);

    [address((ee)0x00502200, (aoc)0x0050CF98), cdecl]
    static extern uint8 tile_has_tree(ee_tile* t);

    [address((ee)0x00504F36, (aoc)0x0050FDD5), thiscall]
    static extern void vector_push_small(void* v, void** value);

    /// thiscall(ghost_records, the record): push_back (a copy: its vectors and label copied)
    [address((ee)0x00504EAA, (aoc)0x0050FD49), thiscall]
    static extern void ghost_record_push(void* v, void* record);

    /// thiscall(the same set, found: 8 bytes - its node, then a byte: 1 inserted, 0 there before -, &the doppelganger):
    /// insert (ret 8; the pair std::set::insert returns, through the hidden pointer)
    [address((ee)0x00505007, (aoc)0x0050FED2), thiscall]
    static extern void* doppelganger_set_insert(void* set, void** found, EEDoppelganger** d);
}
