// THE WATER BY ITS DEPTH ([display] water_look, the player 2026-11-08: "keep the look but with subtle upgrades"). The
// game draws the surface at one alpha over a seabed as sharp in the deep as in the shallows. The surface's shader
// knows the depth under it (the hills' map holds the ground, render_shadows.c): the surface lets less of the seabed
// through the deeper it is, the water's own colour its fog, more in the shallows, and thins out to nothing at the
// shore instead of ending in a hard line. The seabed's colour is the game's, painted by depth into its vertices
// (turquoise shallows, dark deeps) - that is the water's look, and a surface made thicker with the depth covered it,
// the sea one flat blue. So the deep takes away the seabed's texture detail only, toward the texture's mean. Tried
// and dropped: the ground darkened by its height over the water - the land on an island map lies within a few
// hundredths of the level, whole tiles of it went dark.
// Needs the hills' map: dynamic shadows, the hills on.
// The fog: the game fogs the seabed to the sky's colour by its own distance and the half see-through surface over it
// by its own. From a low camera the seabed lies much farther along the line of sight than the surface in front of it,
// the fog's band is narrow (8 tiles), so a far seabed turned pale behind a near ridge still dark, and the two showed
// through the surface (the player, 2026-11-08: "the jaring skycolored fog underwater"). What lies under the level now
// takes the fog of the place its line of sight enters the water; with the two fogs alike, the surface over the seabed
// comes out as one fog over both. Tried first: no fog on what lies under the level and a surface more opaque in the fog
// - a hard line where the water's far end meets the haze and orange streaks on beaches, wherever ground under the level
// has no surface over it.
// The way through (the player, 2026-11-08: "taking into acount the distance one is curently looking trough the water to the
// face they are seeing", "what the editor calls -20 is a good depth/distance to be fully opaque"; "make it one color (use the
// depest of the underwater colors), elsewise things can stand out in a weired way", "shift it to a frambuffer step", "void
// (the world edge and out of render distance) should be seen as fully waterfogged"): a step over the finished picture.
// Before the surface is drawn, the picture so far and its depth are copied, and the surface's triangles are drawn once
// without blending, each pixel what lies behind it (from the copy) toward the deepest of the game's colours for the ground
// under the water, by the way from the surface to it, wholly after [display] water_opaque_depth; with nothing behind it,
// wholly. That colour goes by the surface's own vertices - the light, and black at what isn't explored - and takes the
// surface's fog. The surface is then drawn over that as ever (made more opaque by the way through it washed out the sea,
// "i didnt mean to make the water surface opaque"). The editor's number is the height over half a height step
// (EEUIFSEPaintElevation: 0.204 tiles). Refraction would be the copy read a little aside. Before: each face under the water
// faded by itself with the table by depth, and its own idea of the light (which flickered at the edge of what is explored).
// the map's deepest underwater colour is a vertex colour, multiplied with the ground's texture where the game draws it:
// raw as the fog it was too bright (the player, 2026-11-12: "the color is never used raw ingame, but multiplied with
// ground textures etc."; "the deepest underwater colour * 0.75 maybe?"). A colour the player sets stays as set
#define RW_MAP_FOG 0.75f
internal struct {
    int on;          // -1: as the ini says
    float fog;       // the depth (tiles) by which 63% of the way from the shallows' alpha to the deep's is done
    float shallow;   // the surface's alpha times this at no depth
    float deep;      // and at great depth
    float edge;      // the depth (tiles) over which the surface thins out to nothing at the shore
    float sea;       // how much of the seabed's texture detail the deep takes away (0..1)
    float seadepth;  // the depth (tiles) by which 63% of that is gone
    int haze;        // what lies under the water fogged from where the line of sight enters it
    float opaque;    // the way through the water that hides all (the editor's height units; 0 none, -1 as the ini says)
    float tint;      // the colour under the water times this
    float fc[3];     // the deepest of the game's colours for the ground under the water (rw_table), times RW_MAP_FOG
    float own[3];    // [display] water_fog_color, the colour under the water instead of that
    int owned;       // 1: own is in use, 0: fc, -1: the ini not read yet (rw_colour)
    ee_map *tmap;    // the map those are of
    unsigned tn, tfirst, tlast;   // how many steps, the first's and the last's colour
    float za;        // the projection's depth term (rw_depth)
    ID3D11PixelShader *fps;
    ID3D11DepthStencilState *fdepth;
    int late;        // see-through batches around the surface by their height (rd3_water_late)
    int away;        // what lies off the screen in the mirrored picture too (rw_role)
    int stage;       // this list's surface (rd3_water_pass): 0 each batch's step with it, 1 only the steps, 2 only the batches
    int ffail, have, cw, ch, zw, zh;   // the step's shader could not be made; this list's copies are there; their sizes
    DXGI_FORMAT cfmt;
    long copied;     // the list they are of, + 1
    ID3D11Texture2D *ctex, *ztex;
    ID3D11ShaderResourceView *csrv, *zsrv;
    int mirror;      // the reflections, -1: as the ini says
    float strength;  // how much of the surface a reflection takes
    float wobble;    // how far the water's ripples move it (of the picture)
    float full;      // the depth (tiles) from which it has its whole strength; toward the shore it fades to none
    int failed, in_pass, drawn;   // its target could not be made; being drawn; one drawn
    LONG frame;      // the frame it was started for (rd3_water_reflect: a frame's later parts draw into it)
    int w, h;        // the target's size
    float rfb[4];    // the picture's place on the screen: its x, y; 1 / its width, height
    ID3D11Texture2D *tex, *dtex;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    ID3D11ShaderResourceView *srv;
    ID3D11RasterizerState *raster;
    void *rsrc[8];                   // the list's raster states (render_3d.c rd3_mk_raster: one a cull mode) ...
    ID3D11RasterizerState *rmir[8];  // ... and the mirrored picture's for them (rw_raster)
    int nr;
} g_wa = { .on = -1, .fog = 0.35f, .shallow = 0.75f, .deep = 1.0f, .edge = 0.05f, .sea = 0.75f, .seadepth = 0.35f, .haze = 1, .opaque = -1, .tint = 1, .owned = -1, .late = 1, .away = 1, .mirror = -1, .strength = 0.35f, .wobble = 0.012f, .full = 0.4f };

// the game's colours for the depths, once a map: the mesh's water colour set, a list of slots of its colour table, first the
// shallowest (GETerrainMesh::SetWaterColorSet) - the last one's the colour under the water
internal void rw_table(void) {
    ee_map *map = game_map();
    if (!map || map == g_wa.tmap) return;
    void *mesh = map_mesh_at(map);
    unsigned ns = 0, n = 0, set = mesh_water_set(mesh);
    uint8_t *sets = mesh_color_sets(mesh, &ns);
    uint32_t *slots = sets && set < ns ? color_set_slots(sets, set, &n) : NULL;
    if (!slots || !n) return;   // the map's setup not done yet
    for (int last = 0; last < 2; last++) {
        const float *c = mesh_color(mesh, slots[last ? n - 1 : 0]);
        uint32_t px = 0;
        for (int a = 0; a < 3; a++) {
            float v = fminf(1, fmaxf(0, c[a]));
            px |= (uint32_t)(v * 255 + 0.5f) << ((2 - a) * 8);
            if (last) g_wa.fc[a] = v * RW_MAP_FOG;
        }
        if (last) g_wa.tlast = px; else g_wa.tfirst = px;
    }
    g_wa.tmap = map;
    g_wa.tn = n;
}
// the camera's height for the vertex shader's fog under the water (the ground's camera space's origin in the world); so low
// it is off
internal float rd3_water_camera(void) {
    int on = (g_wa.on < 0 ? g_set.water_look : g_wa.on) && g_wa.haze && g_sh.water_on;
    return on ? g_sh.gw[14] : -1e9f;
}
internal int rw_own_colour(int r, int g, int b) {
    if (r < 0 || g < 0 || b < 0 || r > 255 || g > 255 || b > 255) return 0;
    g_wa.own[0] = (float)r / 255; g_wa.own[1] = (float)g / 255; g_wa.own[2] = (float)b / 255;
    g_wa.owned = 1;
    return 1;
}
// the colour under the water: the player's ([display] water_fog_color, a colour setting: 0xRRGGBB, -1 none), else the map's deepest
internal const float *rw_colour(void) {
    if (g_wa.owned < 0) {
        int c = g_set.water_fog_color;
        g_wa.owned = 0;
        if (c >= 0) rw_own_colour(c >> 16 & 255, c >> 8 & 255, c & 255);
    }
    return g_wa.owned ? g_wa.own : g_wa.fc;
}
// 1 / the way through the water that hides all, 0: no such step now
internal float rw_opaque(void) {
    ee_map *m = game_map();
    float units = g_wa.opaque < 0 ? (float)g_set.water_opaque_depth : g_wa.opaque;
    // without a map the draws don't know their place in the world (rd3_shadow_vc)
    int on = (g_wa.on < 0 ? g_set.water_look : g_wa.on) && g_sh.water_on && (g_sh.valid || (g_sh.hill_ok && !g_sh.nohills));
    return on && m && units > 0 && g_wa.tn ? 1 / (units * map_get_height_step(m) * 0.5f) : 0;
}
internal void rw_caustics_pc(rd3_pcb *pc, int on);
// THE WAVES' BENDING (the player, 2026-11-12: "can we do refraction for underwater and diortion for the surface
// reflections?"): the surface's slope from the caustics' waves (one sea: the light on the seabed and its bending come
// from the same waves), turned from a move on the map into one on the screen by how the map's place changes from pixel to
// pixel. The refraction ([display] water_refraction) is the water's step reading its copy of the picture aside - only
// what lies behind the surface there too (a hull or the shore in front stays put), none at the shore, all where the line
// of sight runs a quarter tile through the water. The reflections ([display] water_reflection_ripples) read the
// mirrored picture aside by the same slope; the water texture's own wobble stays under it. Before, only that wobble: the
// texture's colours as a slope, a faint shimmer that didn't follow the waves
#define RW_SLOPE 0.5f      // the waves' slope (render_3d.c cwg) at a middling place
#define RW_REFRACT 0.06f   // tiles the refraction moves at that slope at 100%
#define RW_RIPPLES 0.06f   // tiles the reflections move at that slope at 100%
internal float g_rw_refract = -1, g_rw_ripples = -1;   // the pipe's, -1 the settings'
internal void rd3_water_pc(rd3_pcb *pc, float level, int water_seen) {
    // without a map the draws don't know their place in the world (rd3_shadow_vc)
    int on = (g_wa.on < 0 ? g_set.water_look : g_wa.on) && water_seen && (g_sh.valid || (g_sh.hill_ok && !g_sh.nohills));
    pc->wat[0] = on ? 1 / g_wa.fog : 0;
    pc->wat[1] = g_wa.shallow;
    pc->wat[2] = g_wa.deep;
    pc->wat[3] = 1 / g_wa.edge;
    pc->sea[0] = on ? g_wa.sea : 0;
    pc->sea[1] = 1 / g_wa.seadepth;
    pc->sea[2] = level;
    {
        memcpy(pc->wop, g_sh.gw + 12, 12);
        pc->wop[3] = rw_opaque();
        for (int a = 0; a < 3; a++) pc->wfc[a] = rw_colour()[a] * g_wa.tint;
        pc->wfc[3] = g_wa.za;
    }
    pc->rfa[0] = g_wa.strength;
    pc->rfa[1] = g_wa.wobble;
    pc->rfa[2] = g_wa.drawn && water_seen ? 1.0f : 0.0f;
    pc->rfa[3] = 1 / g_wa.full;
    memcpy(pc->rfb, g_wa.rfb, sizeof pc->rfb);
    rw_caustics_pc(pc, on);
    // the waves' bending (render_3d.c wslope, wpix): the refraction only with the water's step, the reflections' by
    // themselves - their slope is about RW_SLOPE where it is middling
    pc->cat[1] = on && rw_opaque() > 0 ? (g_rw_refract >= 0 ? g_rw_refract : g_set.water_refraction) * RW_REFRACT / RW_SLOPE : 0.0f;
    pc->cat[2] = (g_rw_ripples >= 0 ? g_rw_ripples : g_set.water_reflection_ripples) * RW_RIPPLES / RW_SLOPE;
}

// THE REFLECTIONS ([display] water_reflections): before the list is drawn, what stands in the world - the units,
// buildings and trees on the screen - is drawn again mirrored about the water's level into a picture of half the size, and the surface takes a part of it where
// something was drawn, less the shallower the water (the player, 2026-11-08, a tower's image lying on the sand under
// the shallows: none at the shore), each thing with the fog it is seen through itself - the surface's own fog is the
// nearer water's, and a palm on the horizon that the fog had all but hidden popped in and out in full colour (the player,
// 2026-11-08). The ground isn't: the land lies within hundredths of the level on most maps. A list without
// casters leaves the last picture (a frame's list is drawn in parts).
// The caustics ([display] water_caustics, the player 2026-11-10: "could our water throw casutics? (a cheap
// version)"): the pixel shader lights the seabed and what stands under the level (a hull, a wading unit) with what the waves
// focus there, nothing drawn on its own - render_3d.c's caus, by every draw's constants (rd3_pcb cau, cat). Their clock is
// render_ground.c's (the ground's draw moves it on once a frame). The pipe's water caustics <0..2> over the setting.
#define RW_CAUSTICS_SCALE 1.1f   // the waves' pattern units a tile: its cells about a tile across ([display] water_caustics_size 100%)
#define RW_CAUSTICS_WIND 0.0f    // the way the waves run (radians from the map's x): along an axis, the lines run diagonally on the screen (along the screen's own axes they read as streaks)
internal float g_rw_caustics = -1;   // the pipe's, -1 the setting's
internal float g_rw_cobj = -1;       // the pipe's water caustics_objects, -1 the setting's
internal double g_rw_ctime;          // their time: the wobble's half pace times [display] water_caustics_speed
internal void rw_caustics_pc(rd3_pcb *pc, int on) {
    pc->cau[0] = on ? (g_rw_caustics >= 0 ? g_rw_caustics : g_set.water_caustics) : 0.0f;
    pc->cau[1] = RW_CAUSTICS_SCALE / (g_set.water_caustics_size > 0.01f ? g_set.water_caustics_size : 1.0f);   // [display] water_caustics_size
    pc->cau[2] = cosf(RW_CAUSTICS_WIND);
    pc->cau[3] = sinf(RW_CAUSTICS_WIND);
    pc->cat[0] = (float)g_rw_ctime;
    pc->cat[3] = g_rw_cobj >= 0 ? g_rw_cobj : g_set.water_caustics_objects;   // on what stands in the water: times this ([display] water_caustics_objects)
}
internal void rd3_water_bind(ID3D11DeviceContext *x) {
    ID3D11ShaderResourceView *srv = g_wa.drawn && !g_wa.in_pass ? g_wa.srv : NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 6, 1, &srv);
}
// Not a tree's card (SH_CARD): a stand-in for the shadow. What lies off the screen is in the list for the shadows' sake (the rows
// widened, render_shadows.c) and counts here too: mirrored, an object beyond the top of the screen comes down onto it (the
// player, 2026-11-10: "the culling loses reflections taht are just out of screen")
internal int rw_role(int role) { return role == SH_CAST || role == SH_TINT || role == SH_TREE || (g_wa.away && (role == SH_AWAY || role == SH_GONE)); }
// what the water mirrors: those, and the ground from the map (the player, 2026-11-09: "terrain dosnt apear in water reflections yet")
internal int rw_mirrored(const rf_cmd *c) { return (rw_role(c->role) || (c->gvb && c->ro == RO_TERRAIN)) && c->topo == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; }
internal int rw_target(int w, int h) {
    if (g_wa.tex && g_wa.w == w && g_wa.h == h) return 1;
    if (g_wa.failed) return 0;
    D3D_RELEASE(g_wa.srv); D3D_RELEASE(g_wa.rtv); D3D_RELEASE(g_wa.dsv); D3D_RELEASE(g_wa.tex); D3D_RELEASE(g_wa.dtex);
    ID3D11Device *dv = g_d3d.dev;
    D3D11_TEXTURE2D_DESC td = { (UINT)w, (UINT)h, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT,
                                D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    D3D11_TEXTURE2D_DESC dd = td;
    dd.Format = DXGI_FORMAT_D32_FLOAT; dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    HRESULT hr = ID3D11Device_CreateTexture2D(dv, &td, NULL, &g_wa.tex);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateTexture2D(dv, &dd, NULL, &g_wa.dtex);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRenderTargetView(dv, (ID3D11Resource *)g_wa.tex, NULL, &g_wa.rtv);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(dv, (ID3D11Resource *)g_wa.dtex, NULL, &g_wa.dsv);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_wa.tex, NULL, &g_wa.srv);
    if (SUCCEEDED(hr) && !g_wa.raster) {
        D3D11_RASTERIZER_DESC rs = { D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, 0, 0, 0, TRUE, FALSE, FALSE, FALSE };   // mirrored: the faces turn
        hr = ID3D11Device_CreateRasterizerState(dv, &rs, &g_wa.raster);
    }
    if (FAILED(hr)) { g_wa.failed = 1; g_wa.drawn = 0; rd_fail("the water's reflections", hr); return 0; }
    g_wa.w = w; g_wa.h = h;
    return 1;
}
// the projection's depth term (a picture's depth d of a view's depth z is A + B / z: the row of the matrix that gives w is
// the view's axis, the one that gives z that axis times A); a matrix without a perspective leaves *a
internal void rw_za(const float *m, float *a) {
    int k = 0;
    for (int j = 1; j < 3; j++) if (fabsf(m[j * 4 + 3]) > fabsf(m[k * 4 + 3])) k = j;
    if (fabsf(m[k * 4 + 3]) > 1e-6f) *a = m[k * 4 + 2] / m[k * 4 + 3];
}
// of the list's water
internal void rw_depth(void) {
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (c->role != SH_WATER) continue;
        rd3_vcb vc = g_rf.vcs[c->vc];
        rd3_shadow_vc(&vc);
        rf_shift(&vc, c->ro);
        rw_za(vc.vpm, &g_wa.za);
        return;
    }
}
// the picture so far and its depth copied; 0: they can't be (a target of several samples, sizes that differ)
internal int rw_copy(ID3D11DeviceContext *x, ID3D11RenderTargetView *rtv, ID3D11DepthStencilView *dsv) {
    if (!rtv || !dsv) return 0;
    ID3D11Resource *cr = NULL, *zr = NULL;
    ID3D11Texture2D *ct = NULL, *zt = NULL;
    int ok = 0;
    ID3D11View_GetResource((ID3D11View *)rtv, &cr);
    ID3D11View_GetResource((ID3D11View *)dsv, &zr);
    if (cr && zr && SUCCEEDED(ID3D11Resource_QueryInterface(cr, &IID_ID3D11Texture2D, (void **)&ct)) && SUCCEEDED(ID3D11Resource_QueryInterface(zr, &IID_ID3D11Texture2D, (void **)&zt))) {
        D3D11_TEXTURE2D_DESC cd, zd;
        ID3D11Texture2D_GetDesc(ct, &cd);
        ID3D11Texture2D_GetDesc(zt, &zd);
        if (cd.SampleDesc.Count == 1 && zd.SampleDesc.Count == 1 && cd.Width == zd.Width && cd.Height == zd.Height && zd.Format == DXGI_FORMAT_D32_FLOAT) {
            HRESULT hr = S_OK;
            if (!g_wa.ctex || g_wa.cw != (int)cd.Width || g_wa.ch != (int)cd.Height || g_wa.cfmt != cd.Format) {
                D3D_RELEASE(g_wa.csrv); D3D_RELEASE(g_wa.ctex);
                D3D11_TEXTURE2D_DESC nd = { cd.Width, cd.Height, 1, 1, cd.Format, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
                hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &nd, NULL, &g_wa.ctex);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_wa.ctex, NULL, &g_wa.csrv);
                g_wa.cw = (int)cd.Width; g_wa.ch = (int)cd.Height; g_wa.cfmt = cd.Format;
            }
            if (SUCCEEDED(hr) && (!g_wa.ztex || g_wa.zw != (int)zd.Width || g_wa.zh != (int)zd.Height)) {
                D3D_RELEASE(g_wa.zsrv); D3D_RELEASE(g_wa.ztex);
                D3D11_TEXTURE2D_DESC nd = { zd.Width, zd.Height, 1, 1, DXGI_FORMAT_R32_TYPELESS, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
                D3D11_SHADER_RESOURCE_VIEW_DESC sv = { .Format = DXGI_FORMAT_R32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D, .Texture2D = { 0, 1 } };
                hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &nd, NULL, &g_wa.ztex);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_wa.ztex, &sv, &g_wa.zsrv);
                g_wa.zw = (int)zd.Width; g_wa.zh = (int)zd.Height;
            }
            if (FAILED(hr)) { g_wa.ffail = 1; rd_fail("the water's copy of the picture", hr); }
            else {
                ID3D11DeviceContext_CopySubresourceRegion(x, (ID3D11Resource *)g_wa.ctex, 0, 0, 0, 0, cr, 0, NULL);
                ID3D11DeviceContext_CopySubresourceRegion(x, (ID3D11Resource *)g_wa.ztex, 0, 0, 0, 0, zr, 0, NULL);
                ok = 1;
            }
        }
    }
    D3D_RELEASE(ct); D3D_RELEASE(zt); D3D_RELEASE(cr); D3D_RELEASE(zr);
    return ok;
}
// What is see-through and writes no depth leaves nothing in the depth the step reads: a move's green circle on the seabed
// went under the fog with the seabed (the player, 2026-11-09: "the navigation confirmation (green circle) is hidden by the
// fog"), smoke over the sea would have been fogged by the seabed under it. And the surface writes no depth either, so the
// order alone says which of the two is in front: the game's move circle (the ground's pass) lay under the surface although it
// is drawn at the level, the smoke (the water's own pass, after the surface) over it from wherever it started (the player,
// 2026-11-10: "id like them to draw behind the water surface when they are under it"). So such a batch is drawn in two
// parts by the height of each pixel, the part under the level after the surface's step and before the surface, the rest
// after the surface (rf_replay); -1: the list has no surface or the order is the game's
internal int rd3_water_last(void) {
    g_wa.stage = 0;
    if (!g_wa.late || g_wa.ffail || rw_opaque() <= 0) return -1;
    for (int i = g_rf.n - 1; i >= 0; i--) if (g_rf.cmd[i].role == SH_WATER) return i;
    return -1;
}
// A ship's wake is a decal of the units' pass (SH_DECAL, see-through, no depth written: the rings a ship leaves, 6 indices
// each) lying at the level, and "under" starts 0.05 below it so that a move's circle stays over the surface - the rings came
// out over the surface (the player, 2026-11-11: "the wave circle behind ships is suposed to render under the water surface, not
// over it"). Such a decal's split lies RF_HIGH_SPLIT higher (render_frame.c rf_water_split): the rings under the surface, as
// a decal on land is drawn where it was - no surface over it
internal int rd3_water_high(const rf_cmd *c) { return c->ro == RO_UNITS && c->role == SH_DECAL; }
internal int rd3_water_first(void) {
    for (int i = 0; i < g_rf.n; i++) if (g_rf.cmd[i].role == SH_WATER) return i;
    return -1;
}
// the step of the whole surface, from here on its batches without it: what lies under the level comes between the two
internal void rd3_water_steps(ID3D11DeviceContext *x, int lastw, rf_cmd *b, int *first) {
    g_wa.stage = 1;
    for (int j = 0; j <= lastw; j++) if (g_rf.cmd[j].role == SH_WATER) rf_draw(x, &g_rf.cmd[j], b, first);
    g_wa.stage = 2;
}
// The ground's pass: only what is tested with "less" (the game's move circle), not what lies under everything at "less or
// equal", the sky's plane 200 tiles up among it
internal int rd3_water_late(const rf_cmd *c) {
    static void *lb, *ld;
    static int late, less;
    if ((c->ro != RO_UNITS && c->ro != RO_WATER && c->ro != RO_TERRAIN) || c->role == SH_WATER || !c->blend || !c->depth) return 0;
    if (c->blend != lb || c->depth != ld) {
        D3D11_BLEND_DESC bd;
        D3D11_DEPTH_STENCIL_DESC dd;
        ID3D11BlendState_GetDesc((ID3D11BlendState *)c->blend, &bd);
        ID3D11DepthStencilState_GetDesc((ID3D11DepthStencilState *)c->depth, &dd);
        lb = c->blend; ld = c->depth;
        late = bd.RenderTarget[0].BlendEnable && dd.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ZERO;
        less = dd.DepthFunc == D3D11_COMPARISON_LESS;
    }
    return late && (c->ro != RO_TERRAIN || less);
}
// the water's step (render_frame.c rf_replay, each batch before it is drawn): a batch of the surface first over the copy;
// no depth written, its overlaps come out alike
internal void rw_step(ID3D11DeviceContext *x, const rf_cmd *c) {
    if (g_wa.ffail || rw_opaque() <= 0) return;
    if (!g_wa.fps) {
        ID3DBlob *code = d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "wfog", "ps_4_0");
        HRESULT hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_wa.fps) : E_FAIL;
        D3D_RELEASE(code);
        D3D11_DEPTH_STENCIL_DESC ds = { .DepthEnable = TRUE, .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO, .DepthFunc = D3D11_COMPARISON_LESS_EQUAL };
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilState(g_d3d.dev, &ds, &g_wa.fdepth);
        if (FAILED(hr)) { g_wa.ffail = 1; rd_fail("the water's step over the picture", hr); return; }
    }
    if (g_wa.copied != g_rf.replays + 1) {
        g_wa.copied = g_rf.replays + 1;
        g_wa.have = rw_copy(x, c->rtv, c->dsv);
    }
    if (!g_wa.have) return;
    ID3D11ShaderResourceView *srv[2] = { g_wa.csrv, g_wa.zsrv };
    ID3D11DeviceContext_PSSetShaderResources(x, 8, 2, srv);
    ID3D11DeviceContext_PSSetShader(x, g_wa.fps, NULL, 0);
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, g_wa.fdepth, 0);
    if (c->gvb) rg_replay(x, c);
    else rf_call(x, c);
    ID3D11DeviceContext_PSSetShader(x, g_rd3.ps, NULL, 0);
    ID3D11DeviceContext_OMSetBlendState(x, c->blend, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, c->depth, 0);
    srv[0] = srv[1] = NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 8, 2, srv);
}
// a batch about to be drawn; 1: only its step was wanted
// A batch's raster state in the mirrored picture: its own culling with the faces' turn reversed, as the mirror reverses
// it. With none culled (as at first) both sheets of a sail were drawn - the lit front and the back turned from the
// sun, one on the other - and they fought over the depth: stripes and specks on the mirrored sails, others each frame
// (the player, 2026-11-11). A state that can't be made: none culled, as before.
internal ID3D11RasterizerState *rw_raster(void *r) {
    for (int k = 0; k < g_wa.nr; k++) if (g_wa.rsrc[k] == r) return g_wa.rmir[k];
    D3D11_RASTERIZER_DESC rs = { D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, 0, 0, 0, TRUE, FALSE, FALSE, FALSE };
    if (r) ID3D11RasterizerState_GetDesc((ID3D11RasterizerState *)r, &rs);
    rs.FrontCounterClockwise = !rs.FrontCounterClockwise;
    ID3D11RasterizerState *o = NULL;
    if (FAILED(ID3D11Device_CreateRasterizerState(g_d3d.dev, &rs, &o)) || !o) o = g_wa.raster;
    if (g_wa.nr < 8) { g_wa.rsrc[g_wa.nr] = r; g_wa.rmir[g_wa.nr++] = o; }
    return o;
}
internal int rd3_water_pass(ID3D11DeviceContext *x, const rf_cmd *c) {
    if (c->role != SH_WATER) return 0;
    if (g_wa.stage != 2) rw_step(x, c);
    return g_wa.stage == 1;
}
internal void rd3_water_reflect(void) {
    rw_table();
    rw_depth();
    int on = g_wa.mirror < 0 ? g_set.water_reflections : g_wa.mirror;
    if (!on || !g_sh.on || g_sh.ready <= 0 || !g_sh.water_on) { g_wa.drawn = 0; return; }
    const rf_cmd *fc = NULL;
    for (int i = 0; i < g_rf.n && !fc; i++) if (rw_mirrored(&g_rf.cmd[i])) fc = &g_rf.cmd[i];
    if (!fc) return;
    D3D11_VIEWPORT port = rs_port(&fc->vport, fc->rtv);
    int w = (int)port.Width / 2, h = (int)port.Height / 2;
    if (w < 1 || h < 1 || !rw_target(w, h)) return;
    g_wa.rfb[0] = port.TopLeftX; g_wa.rfb[1] = port.TopLeftY;
    g_wa.rfb[2] = 1 / port.Width; g_wa.rfb[3] = 1 / port.Height;
    ID3D11DeviceContext *x = g_d3d.ctx;
    static const float clear[4] = { 0, 0, 0, 0 };
    g_wa.in_pass = 1;
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, NULL);
    rf_bind(x);
    // started anew only at the frame's first part: a list drawn in parts (a texture changed under it, a pool full) had
    // the ground alone in its first and the water in the next, and the mirrored picture made again from that part had no
    // ground - the hills gone from the water for one frame, twice a second in the player's game (2026-11-11: "the water
    // reflection flickers"; a test game: 10 frames of 1400 drawn in parts so). A later part draws its own into it.
    if (g_wa.frame != g_rd_frames) {
        g_wa.frame = g_rd_frames;
        ID3D11DeviceContext_ClearRenderTargetView(x, g_wa.rtv, clear);
        ID3D11DeviceContext_ClearDepthStencilView(x, g_wa.dsv, D3D11_CLEAR_DEPTH, 1, 0);
    }
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_wa.rtv, g_wa.dsv);
    D3D11_VIEWPORT vp = { 0, 0, (FLOAT)w, (FLOAT)h, 0, 1 };
    ID3D11DeviceContext_RSSetViewports(x, 1, &vp);
    ID3D11DeviceContext_RSSetState(x, g_wa.raster);
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    float mirror[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 2 * g_sh.level, 1 };
    // A batch the soft edges split (render_frame.c rf_edges: blended, alpha tested) by its solid part alone: the rest is
    // half see-through and the main picture blends it in faintly after the solid, but this picture is drawn unblended and
    // its alpha is the fog's - a ship's rigging, its ropes and the shrouds' nets thinner than a pixel, came out as dark
    // specks all over the mirrored sails, others at each frame as the ship rocks (the player, 2026-11-11: "the shadows
    // on the sails in the reflection flicker"). Whatever the setting: unblended, the soft part is never right here.
    rf_water_part(x, 0);   // all of a draw's pixels: the slot still holds the last frame's last part
    int edges = !g_fe.fail, lvc = -1, lpc = -1, lro = -1;
    ID3D11RasterizerState *lr = g_wa.raster;
    void *ld = NULL, *ls = NULL;
    ID3D11ShaderResourceView *lt = (ID3D11ShaderResourceView *)1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (!rw_mirrored(c)) continue;
        if (edges) rf_edges_part(x, c->soft ? 1 : 0);
        ID3D11RasterizerState *rr = rw_raster(c->raster);
        if (rr != lr) { lr = rr; ID3D11DeviceContext_RSSetState(x, rr); }
        if (c->depth != ld) { ld = c->depth; ID3D11DeviceContext_OMSetDepthStencilState(x, c->depth, 0); }
        if (c->srv != lt) { lt = c->srv; ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &lt); }
        if (c->samp != ls) { ls = c->samp; ID3D11DeviceContext_PSSetSamplers(x, 0, 1, (ID3D11SamplerState **)&c->samp); }
        if (c->vc != lvc || c->ro != lro) {
            float t[16];
            rd3_vcb vc = g_rf.vcs[c->vc];
            rd3_shadow_vc(&vc);
            rf_shift(&vc, c->ro);
            rd3_mul(t, mirror, vc.vpm);
            memcpy(vc.vpm, t, sizeof t);
            vc.shp[0] = 0;   // no shadows in it
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.vcb, 0, NULL, &vc, 0, 0);
            lvc = c->vc; lro = c->ro;
        }
        if (c->pc != lpc) {
            rd3_pcb pc = g_rf.pcs[c->pc];
            rd3_shadow_pc(&pc);
            pc.shd[0] = 0;
            pc.rfa[2] = 0;
            pc.sea[3] = 1;    // what lies under the level left out, alpha 1
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.pcb, 0, NULL, &pc, 0, 0);
            lpc = c->pc;
        }
        if (c->gvb) rg_replay(x, c);
        else rf_call(x, c);
    }
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, NULL);
    g_wa.in_pass = 0;
    g_wa.drawn = 1;
}

// the pipe: water [on | off] | fog <depth> | shallow <times> | deep <times> | edge <depth> | sea <0..1> | seadepth <depth>
// | haze on | off | late on | off | away on | off (what lies off the screen in the mirrored picture) | edges on | off (not the water's: the units' soft edges, render_frame.c rf_edges) | color <r g b> | map | opaque <the editor's height units, 0 none> | tint <times the colour under the water> | mirror on | off | strength <0..1> | wobble <of the picture> | full <depth> | refract <0..4> | ripples <0..4> | blend on | off (the animation's frames) | still on | off (no surface movement, no animation)
internal int g_rw_blend = 1;   // the pipe's water blend on | off: the animation's frames blended (render_ground.c rg_water_blend)
// the pipe's water still on | off (the player, 2026-11-13: "please add a command that completly disables the waters vertex
// movement and texture animation"): the surface's vertices at the level, each water group one frame of the animation
// (render_ground.c rg_water_frame, rg_draw_now); the caustics, the refraction and the reflections' ripples go on
internal int g_rw_still;
internal void cmd_water(char *out, const char *args) {
    char mode[32] = "";
    float v = 0;
    int n = sscanf(args, "%31s %f", mode, &v);
    if (n >= 1 && !strcmp(mode, "on")) g_wa.on = 1;
    else if (n >= 1 && !strcmp(mode, "off")) g_wa.on = 0;
    else if (n == 2 && !strcmp(mode, "fog") && v > 0) g_wa.fog = v;
    else if (n == 2 && !strcmp(mode, "shallow") && v >= 0) g_wa.shallow = v;
    else if (n == 2 && !strcmp(mode, "deep") && v >= 0) g_wa.deep = v;
    else if (n == 2 && !strcmp(mode, "edge") && v > 0) g_wa.edge = v;
    else if (n == 2 && !strcmp(mode, "sea") && v >= 0 && v <= 1) g_wa.sea = v;
    else if (n == 2 && !strcmp(mode, "seadepth") && v > 0) g_wa.seadepth = v;
    else if (n == 2 && !strcmp(mode, "caustics") && v >= 0 && v <= 2) g_rw_caustics = v;
    else if (n == 2 && !strcmp(mode, "objects") && v >= 0 && v <= 8) g_rw_cobj = v;
    else if (n == 2 && !strcmp(mode, "refract") && v >= 0 && v <= 4) g_rw_refract = v;
    else if (n == 2 && !strcmp(mode, "ripples") && v >= 0 && v <= 4) g_rw_ripples = v;
    else if (n == 2 && !strcmp(mode, "strength") && v >= 0 && v <= 1) g_wa.strength = v;
    else if (n == 2 && !strcmp(mode, "wobble") && v >= 0) g_wa.wobble = v;
    else if (n == 2 && !strcmp(mode, "full") && v > 0) g_wa.full = v;
    else if (n == 2 && !strcmp(mode, "opaque") && v >= 0) g_wa.opaque = v;
    else if (n == 2 && !strcmp(mode, "tint") && v >= 0) g_wa.tint = v;
    else if (!strcmp(mode, "haze")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_wa.haze = 1; else if (!strcmp(a, "off")) g_wa.haze = 0; }
    else if (!strcmp(mode, "color")) {
        int r, g, b;
        char a[8] = "";
        if (sscanf(args, "%*s %d %d %d", &r, &g, &b) == 3) { if (!rw_own_colour(r, g, b)) { REPLY("error: a colour's numbers are 0 .. 255\n"); return; } }
        else if (sscanf(args, "%*s %7s", a) == 1 && !strcmp(a, "map")) { rw_colour(); g_wa.owned = 0; }
        else { REPLY("error: usage: water color <red green blue> | map\n"); return; }
    }
    else if (!strcmp(mode, "late")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_wa.late = 1; else if (!strcmp(a, "off")) g_wa.late = 0; }
    else if (!strcmp(mode, "away")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_wa.away = 1; else if (!strcmp(a, "off")) g_wa.away = 0; }
    else if (!strcmp(mode, "edges")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_fe.force = 1; else if (!strcmp(a, "off")) g_fe.force = 0; }
    else if (!strcmp(mode, "still")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_rw_still = 1; else if (!strcmp(a, "off")) g_rw_still = 0; }
    else if (!strcmp(mode, "blend")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_rw_blend = 1; else if (!strcmp(a, "off")) g_rw_blend = 0; }
    else if (!strcmp(mode, "mirror")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_wa.mirror = 1; else if (!strcmp(a, "off")) g_wa.mirror = 0; }
    else if (n >= 1) { REPLY("error: usage: water [on | off] | fog <depth> | shallow <times> | deep <times> | edge <depth> | sea <0..1> | seadepth <depth> | caustics <0..2> | objects <0..8> | refract <0..4> | ripples <0..4> | haze on | off | late on | off | edges on | off | color <r g b> | map | opaque <editor units> | tint <times> | mirror on | off | strength <0..1> | wobble <of the picture> | full <depth> | blend on | off | still on | off\n"); return; }
    const float *wc = rw_colour();
    REPLY("water: camera %.2f over the level %.2f, the ground down to %.2f; %s, fog %.2f, shallow %.2f, deep %.2f, edge %.2f, sea %.2f by %.2f, fog over the water %s, opaque after %.0f, %u depth colours %06x .. %06x, under the water %.0f %.0f %.0f (%s), the step %s, depth term %.4f; reflections %s, strength %.2f, wobble %.3f, full from %.2f; refraction %.2f, ripples %.2f; the animation's frames %s%s\n",
          (double)(g_sh.gw[14] - g_sh.level), (double)g_sh.level, (double)g_sh.glow, (g_wa.on < 0 ? g_set.water_look : g_wa.on) ? "on" : "off", (double)g_wa.fog, (double)g_wa.shallow, (double)g_wa.deep,
          (double)g_wa.edge, (double)g_wa.sea, (double)g_wa.seadepth, g_wa.haze ? "on" : "off", (double)(g_wa.opaque < 0 ? (float)g_set.water_opaque_depth : g_wa.opaque),
          g_wa.tn, g_wa.tfirst & 0xFFFFFF, g_wa.tlast & 0xFFFFFF, (double)(wc[0] * 255), (double)(wc[1] * 255), (double)(wc[2] * 255), g_wa.owned ? "the setting" : "the map's deepest", g_wa.ffail ? "failed" : g_wa.have ? "drawn" : "not drawn", (double)g_wa.za,
          (g_wa.mirror < 0 ? g_set.water_reflections : g_wa.mirror) ? (g_wa.drawn ? "on" : "on, nothing drawn") : "off", (double)g_wa.strength, (double)g_wa.wobble, (double)g_wa.full,
          (double)(g_rw_refract >= 0 ? g_rw_refract : g_set.water_refraction), (double)(g_rw_ripples >= 0 ? g_rw_ripples : g_set.water_reflection_ripples), g_rw_blend ? "blended" : "stepped",
          g_rw_still ? "; still: no surface movement, no animation" : "");
}
