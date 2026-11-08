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
// The way through: the water is thicker the longer the line of sight runs in it (the depth times the way per depth, the
// camera's distance over its height above the water), opaque after [display] water_opaque_depth - the player, 2026-11-08:
// "customicable by tile height, what the editor calls -20 is a good depth/distance to be fully opaque". The editor's
// number is the height over half a height step (EEUIFSEPaintElevation: 0.204 tiles). From the usual camera the sea's
// depths are far from that, the look stays; a low camera looks along the water and no longer through to its ridges.
internal struct {
    int on;          // -1: as the ini says
    float fog;       // the depth (tiles) by which 63% of the way from the shallows' alpha to the deep's is done
    float shallow;   // the surface's alpha times this at no depth
    float deep;      // and at great depth
    float edge;      // the depth (tiles) over which the surface thins out to nothing at the shore
    float sea;       // how much of the seabed's texture detail the deep takes away (0..1)
    float seadepth;  // the depth (tiles) by which 63% of that is gone
    int haze;        // what lies under the water fogged from where the line of sight enters it
    float opaque;    // the way through the water that is opaque (the editor's height units; 0 none, -1 as the ini says)
    int mirror;      // the reflections, -1: as the ini says
    float strength;  // how much of the surface a reflection takes
    float wobble;    // how far the water's ripples move it (of the picture)
    float full;      // the depth (tiles) from which it has its whole strength; toward the shore it fades to none
    int failed, in_pass, drawn;   // its target could not be made; being drawn; one drawn
    int w, h;        // the target's size
    float rfb[4];    // the picture's place on the screen: its x, y; 1 / its width, height
    ID3D11Texture2D *tex, *dtex;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    ID3D11ShaderResourceView *srv;
    ID3D11RasterizerState *raster;
} g_wa = { .on = -1, .fog = 0.35f, .shallow = 0.75f, .deep = 1.0f, .edge = 0.05f, .sea = 0.75f, .seadepth = 0.35f, .haze = 1, .opaque = -1, .mirror = -1, .strength = 0.35f, .wobble = 0.012f, .full = 0.4f };

// the camera's height for the vertex shader's fog under the water and way through it (the ground's camera space's origin
// in the world); so low it is off
internal float rd3_water_camera(void) {
    int on = (g_wa.on < 0 ? g_set.water_look : g_wa.on) && g_wa.haze && g_sh.water_on;
    return on ? g_sh.gw[14] : -1e9f;
}
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
        ee_map *m = game_map();
        float units = g_wa.opaque < 0 ? (float)g_set.water_opaque_depth : g_wa.opaque;
        pc->wop[0] = on && m && units > 0 ? 1 / (units * map_get_height_step(m) * 0.5f) : 0;
    }
    pc->rfa[0] = g_wa.strength;
    pc->rfa[1] = g_wa.wobble;
    pc->rfa[2] = g_wa.drawn && water_seen ? 1.0f : 0.0f;
    pc->rfa[3] = 1 / g_wa.full;
    memcpy(pc->rfb, g_wa.rfb, sizeof pc->rfb);
}

// THE REFLECTIONS ([display] water_reflections): before the list is drawn, what stands in the world - the units,
// buildings and trees on the screen - is drawn again mirrored about the water's level into a picture of half the size, and the surface takes a part of it where
// something was drawn, less the shallower the water (the player, 2026-11-08, a tower's image lying on the sand under
// the shallows: none at the shore), each thing with the fog it is seen through itself - the surface's own fog is the
// nearer water's, and a palm on the horizon that the fog had all but hidden popped in and out in full colour (the player,
// 2026-11-08). The ground isn't: the land lies within hundredths of the level on most maps. A list without
// casters leaves the last picture (a frame's list is drawn in parts).
internal void rd3_water_bind(ID3D11DeviceContext *x) {
    ID3D11ShaderResourceView *srv = g_wa.drawn && !g_wa.in_pass ? g_wa.srv : NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 6, 1, &srv);
}
internal int rw_role(int role) { return role == SH_CAST || role == SH_TINT || role == SH_TREE; }   // not SH_AWAY: a tree's stand-in card among them
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
internal void rd3_water_reflect(void) {
    int on = g_wa.mirror < 0 ? g_set.water_reflections : g_wa.mirror;
    if (!on || !g_sh.on || g_sh.ready <= 0 || !g_sh.water_on) { g_wa.drawn = 0; return; }
    const rf_cmd *fc = NULL;
    for (int i = 0; i < g_rf.n && !fc; i++)
        if (rw_role(g_rf.cmd[i].role) && g_rf.cmd[i].topo == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) fc = &g_rf.cmd[i];
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
    ID3D11DeviceContext_ClearRenderTargetView(x, g_wa.rtv, clear);
    ID3D11DeviceContext_ClearDepthStencilView(x, g_wa.dsv, D3D11_CLEAR_DEPTH, 1, 0);
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_wa.rtv, g_wa.dsv);
    D3D11_VIEWPORT vp = { 0, 0, (FLOAT)w, (FLOAT)h, 0, 1 };
    ID3D11DeviceContext_RSSetViewports(x, 1, &vp);
    ID3D11DeviceContext_RSSetState(x, g_wa.raster);
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    float mirror[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 2 * g_sh.level, 1 };
    int lvc = -1, lpc = -1, lro = -1;
    void *ld = NULL, *ls = NULL;
    ID3D11ShaderResourceView *lt = (ID3D11ShaderResourceView *)1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (!rw_role(c->role) || c->topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) continue;
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
        ID3D11DeviceContext_DrawIndexed(x, c->n, c->at, c->base);
    }
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, NULL);
    g_wa.in_pass = 0;
    g_wa.drawn = 1;
}

// the pipe: water [on | off] | fog <depth> | shallow <times> | deep <times> | edge <depth> | sea <0..1> | seadepth <depth>
// | haze on | off | opaque <the editor's height units, 0 none> | mirror on | off | strength <0..1> | wobble <of the picture> | full <depth>
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
    else if (n == 2 && !strcmp(mode, "strength") && v >= 0 && v <= 1) g_wa.strength = v;
    else if (n == 2 && !strcmp(mode, "wobble") && v >= 0) g_wa.wobble = v;
    else if (n == 2 && !strcmp(mode, "full") && v > 0) g_wa.full = v;
    else if (n == 2 && !strcmp(mode, "opaque") && v >= 0) g_wa.opaque = v;
    else if (!strcmp(mode, "haze")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_wa.haze = 1; else if (!strcmp(a, "off")) g_wa.haze = 0; }
    else if (!strcmp(mode, "mirror")) { char a[8] = ""; sscanf(args, "%*s %7s", a); if (!strcmp(a, "on")) g_wa.mirror = 1; else if (!strcmp(a, "off")) g_wa.mirror = 0; }
    else if (n >= 1) { REPLY("error: usage: water [on | off] | fog <depth> | shallow <times> | deep <times> | edge <depth> | sea <0..1> | seadepth <depth> | haze on | off | opaque <editor units> | mirror on | off | strength <0..1> | wobble <of the picture> | full <depth>\n"); return; }
    REPLY("water: camera %.2f over the level %.2f, the ground down to %.2f; %s, fog %.2f, shallow %.2f, deep %.2f, edge %.2f, sea %.2f by %.2f, fog over the water %s, opaque after %.0f; reflections %s, strength %.2f, wobble %.3f, full from %.2f\n",
          (double)(g_sh.gw[14] - g_sh.level), (double)g_sh.level, (double)g_sh.glow, (g_wa.on < 0 ? g_set.water_look : g_wa.on) ? "on" : "off", (double)g_wa.fog, (double)g_wa.shallow, (double)g_wa.deep,
          (double)g_wa.edge, (double)g_wa.sea, (double)g_wa.seadepth, g_wa.haze ? "on" : "off", (double)(g_wa.opaque < 0 ? (float)g_set.water_opaque_depth : g_wa.opaque),
          (g_wa.mirror < 0 ? g_set.water_reflections : g_wa.mirror) ? (g_wa.drawn ? "on" : "on, nothing drawn") : "off", (double)g_wa.strength, (double)g_wa.wobble, (double)g_wa.full);
}
