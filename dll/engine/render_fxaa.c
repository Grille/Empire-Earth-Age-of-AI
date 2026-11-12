// THE PICTURE'S SHARPNESS ([display] antialias, anisotropy; the pipe's picture): the 3D scene's edges smoothed once a
// frame by FXAA (the walls', towers' and units' outlines were stairs), at the first of the interface's draws after it
// (rf_replay) - the interface's text and panels drawn over it afterwards stay sharp. The game keeps the 3D out from under
// the unit panel and the minimap with a depth of 0 (EEROUserInterface's clear) and draws those before the 3D and only when
// they change: what is there at the filter is last frames' pixels, and each frame's pass smoothed them further until the
// panel was drawn anew (the player: "it gets blured, builds up, and than resets"). So a pixel of depth 0 is left alone. The scene's part of the target is
// copied into a texture of its own and drawn back through the filter: the present's triangle over the viewport
// (present_d3d.c's vertex shader), every state the list binds set again after it. The anisotropy is the samplers'
// (rd3_mk_samp: the game's linear minifying with mip levels).
internal struct {
    int on, aniso;
    float edge;                     // the luma range an edge has at least, of its brightest (the pipe: picture edge)
    int depth;                      // [display] antialias_depth: the depth says where the edges are, the fuller filter (fxd)
    float jump, contrast, sub;      // an outline's depth jump (of its depth), the luma range enough there, the blur of what is thinner than a pixel (the pipe: picture aajump, aacontrast, aasub)
    float za;                       // the projection's depth term (rw_za)
    int dfail;                      // that shader or its copy could not be made: the simple form goes on
    ID3D11PixelShader *dps;
    ID3D11Texture2D *ztex;          // the scene's depth
    ID3D11ShaderResourceView *zsrv;
    UINT zw, zh;
    int ready;                      // 1 made, -1 failed
    ID3D11PixelShader *ps;
    ID3D11Buffer *cb;
    ID3D11Texture2D *tex;           // the scene's copy
    ID3D11Texture2D *bars;          // the target as it was outside the scene's viewport (rd3_bars), at the same places
    UINT bw, bh;
    DXGI_FORMAT bfmt;
    int bfail;
    ID3D11ShaderResourceView *srv;
    UINT w, h;
    DXGI_FORMAT fmt;
    LONG frame;                     // the frame it ran in last (once a frame)
    LONG passes;
} g_fx = { .aniso = 1, .edge = 0.5f, .jump = 0.02f, .contrast = 0.05f, .sub = 0.5f };

// FXAA in its simple form (Lottes's, the luma of the four diagonal neighbours): where their range is an edge's, along
// its direction two and four samples averaged, the four's unless they leave the neighbours' luma range
internal const char k_fx_shader[] =
    "Texture2D t : register(t0); Texture2D<float> zt : register(t1); SamplerState s : register(s0); cbuffer k : register(b0) { float4 k; float4 kz; };\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    "float luma(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }\n"
    "float3 at(float2 uv) { return t.SampleLevel(s, uv, 0).rgb; }\n"
    "float4 ps(V i) : SV_Target { if (kz.y > 0 && zt.Load(int3(i.p.xy, 0)) <= 0) discard; float w, h; t.GetDimensions(w, h); float2 r = 1 / float2(w, h), uv = i.uv;"
    " float3 m = at(uv); float nw = luma(at(uv + float2(-1, -1) * r)), ne = luma(at(uv + float2(1, -1) * r)),"
    " sw = luma(at(uv + float2(-1, 1) * r)), se = luma(at(uv + float2(1, 1) * r)), lm = luma(m);"
    " float lo = min(lm, min(min(nw, ne), min(sw, se))), hi = max(lm, max(max(nw, ne), max(sw, se)));"
    // not an edge: the textures' own grain left alone (the grass blurred - the first shots)
    " if (hi - lo < max(0.0833, hi * k.x)) return float4(m, 1);"
    " float2 d = float2(-((nw + ne) - (sw + se)), (nw + sw) - (ne + se));"
    " float red = max((nw + ne + sw + se) * 0.03125, 1.0 / 128);"
    " d = clamp(d / (min(abs(d.x), abs(d.y)) + red), -8, 8) * r;"
    " float3 a = 0.5 * (at(uv + d * (1.0 / 3 - 0.5)) + at(uv + d * (2.0 / 3 - 0.5)));"
    " float3 b = a * 0.5 + 0.25 * (at(uv - d * 0.5) + at(uv + d * 0.5));"
    " float lb = luma(b); return float4(lb < lo || lb > hi ? a : b, 1); }\n"
    // [display] antialias_depth: FXAA with the edge's two ends looked for, and the depth deciding what is an edge. The
    // depth of a flat surface changes evenly from pixel to pixel at any slant: what isn't even across a pixel, against
    // the distance there (depth = A + B / the view's distance), is an outline or a fold. There a little contrast is
    // enough; everywhere else, the textures' own grain, as much as the simple form wants
    "float lu(float2 uv) { return luma(at(uv)); }\n"
    "float4 fxd(V i) : SV_Target { float w, h; t.GetDimensions(w, h); float2 r = 1 / float2(w, h), uv = i.uv; float3 m = at(uv);"
    " float lm = luma(m), ln = lu(uv + float2(0, -1) * r), ls = lu(uv + float2(0, 1) * r), le = lu(uv + float2(1, 0) * r), lw = lu(uv + float2(-1, 0) * r);"
    " float lo = min(lm, min(min(ln, ls), min(le, lw))), hi = max(lm, max(max(ln, ls), max(le, lw))), rg = hi - lo;"
    " int2 p = int2(i.p.xy); float zm = zt.Load(int3(p, 0)); if (zm <= 0) discard;"
    " float dj = (abs(zt.Load(int3(p + int2(-1, -1), 0)) + zt.Load(int3(p + int2(1, 1), 0)) - 2 * zm)"
    "  + abs(zt.Load(int3(p + int2(1, -1), 0)) + zt.Load(int3(p + int2(-1, 1), 0)) - 2 * zm)) / max(abs(zm - kz.x), 1e-6);"
    " if (rg < (dj > k.y ? k.z : max(0.0833, hi * k.x))) return float4(m, 1);"
    " float nw = lu(uv + float2(-1, -1) * r), ne = lu(uv + float2(1, -1) * r), sw = lu(uv + float2(-1, 1) * r), se = lu(uv + float2(1, 1) * r);"
    " bool hz = abs(nw + sw - 2 * lw) + 2 * abs(ln + ls - 2 * lm) + abs(ne + se - 2 * le) >= abs(nw + ne - 2 * ln) + 2 * abs(lw + le - 2 * lm) + abs(sw + se - 2 * ls);"
    " float l1 = hz ? ln : lw, l2 = hz ? ls : le, g1 = abs(l1 - lm), g2 = abs(l2 - lm), len = hz ? r.y : r.x;"
    " bool st = g1 >= g2; float gr = 0.25 * max(g1, g2), av = 0.5 * ((st ? l1 : l2) + lm); if (st) len = -len;"
    " float2 pe = uv + (hz ? float2(0, len) : float2(len, 0)) * 0.5, o = hz ? float2(r.x, 0) : float2(0, r.y), a = pe - o, b = pe + o;"
    " float e1 = lu(a) - av, e2 = lu(b) - av; bool d1 = abs(e1) >= gr, d2 = abs(e2) >= gr;"
    " static const float sz[8] = { 1.5, 2, 2, 2, 2, 4, 8, 8 };"
    " [loop] for (int ki = 0; ki < 8 && !(d1 && d2); ki++) {"
    "  if (!d1) { a -= o * sz[ki]; e1 = lu(a) - av; d1 = abs(e1) >= gr; }"
    "  if (!d2) { b += o * sz[ki]; e2 = lu(b) - av; d2 = abs(e2) >= gr; } }"
    " float da = hz ? uv.x - a.x : uv.y - a.y, db = hz ? b.x - uv.x : b.y - uv.y, near = min(da, db);"
    // the end the pixel is nearer to says how far along the stair it lies: moved over the edge by that, if that end
    // turns the way the pixel's own side of the edge does
    " float off = ((da < db ? e1 : e2) < 0) != (lm < av) ? 0.5 - near / (da + db) : 0;"
    " float sb = saturate(abs((2 * (ln + ls + le + lw) + nw + ne + sw + se) / 12 - lm) / rg), sc = (-2 * sb + 3) * sb * sb;"
    " float2 q = uv + (hz ? float2(0, len) : float2(len, 0)) * max(off, sc * sc * k.w); return float4(at(q), 1); }\n";

internal int rd3_fxaa_init(void) {
    if (g_fx.ready) return g_fx.ready > 0;
    g_fx.ready = -1;
    ID3DBlob *code = d3d_compile(k_fx_shader, sizeof k_fx_shader - 1, "ps", "ps_4_0");
    HRESULT hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_fx.ps) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_BUFFER_DESC bd = { 32, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_fx.cb);
    if (FAILED(hr)) return rd_fail("the anti-aliasing's shader", hr), 0;
    g_fx.ready = 1;
    return 1;
}
// the scene's depth copied (a depth target is copied whole: the panels' pixels are told by it) and, when asked for, the second shader made; 0: no copy
internal int rd3_fxaa_depth(ID3D11DeviceContext *x, ID3D11DepthStencilView *dsv, const D3D11_TEXTURE2D_DESC *target) {
    if (g_fx.dfail || !dsv) return 0;
    HRESULT hr = S_OK;
    if (g_fx.depth && !g_fx.dps) {
        ID3DBlob *code = d3d_compile(k_fx_shader, sizeof k_fx_shader - 1, "fxd", "ps_4_0");
        hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_fx.dps) : E_FAIL;
        D3D_RELEASE(code);
    }
    ID3D11Resource *zr = NULL;
    ID3D11Texture2D *zt = NULL;
    int ok = 0;
    ID3D11View_GetResource((ID3D11View *)dsv, &zr);
    if (SUCCEEDED(hr) && zr && SUCCEEDED(ID3D11Resource_QueryInterface(zr, &IID_ID3D11Texture2D, (void **)&zt))) {
        D3D11_TEXTURE2D_DESC zd;
        ID3D11Texture2D_GetDesc(zt, &zd);
        if (zd.SampleDesc.Count == 1 && zd.Width == target->Width && zd.Height == target->Height && zd.Format == DXGI_FORMAT_D32_FLOAT) {
            if (!g_fx.ztex || g_fx.zw != zd.Width || g_fx.zh != zd.Height) {
                D3D_RELEASE(g_fx.zsrv); D3D_RELEASE(g_fx.ztex);
                D3D11_TEXTURE2D_DESC nd = { zd.Width, zd.Height, 1, 1, DXGI_FORMAT_R32_TYPELESS, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
                D3D11_SHADER_RESOURCE_VIEW_DESC sv = { .Format = DXGI_FORMAT_R32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D, .Texture2D = { 0, 1 } };
                hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &nd, NULL, &g_fx.ztex);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_fx.ztex, &sv, &g_fx.zsrv);
                g_fx.zw = zd.Width; g_fx.zh = zd.Height;
            }
            if (SUCCEEDED(hr)) { ID3D11DeviceContext_CopySubresourceRegion(x, (ID3D11Resource *)g_fx.ztex, 0, 0, 0, 0, zr, 0, NULL); ok = 1; }
        }
    }
    D3D_RELEASE(zt); D3D_RELEASE(zr);
    if (FAILED(hr)) { g_fx.dfail = 1; rd_fail("the anti-aliasing by the depth", hr); return 0; }
    return ok;
}

// What the 3D phase writes outside its viewport is put back after it (back: 0 keep, 1 put back). The interface there is only
// drawn when it changes (the dirty rectangles: display.c), so a stray pixel stayed: an order's lines drawn in a frame during a
// step (render_frame.c rf_ground_lines) have the interface's viewport, the whole screen - they ran over the bars and filled
// them with lines (the player: "the lines of shift write into the interface and tay there").
internal void rd3_bars(ID3D11DeviceContext *x, ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp, int back) {
    if (!rtv || g_fx.bfail) return;
    ID3D11Resource *res = NULL;
    ID3D11Texture2D *tex = NULL;
    ID3D11View_GetResource((ID3D11View *)rtv, &res);
    if (!res || FAILED(ID3D11Resource_QueryInterface(res, &IID_ID3D11Texture2D, (void **)&tex))) { D3D_RELEASE(res); return; }
    D3D11_TEXTURE2D_DESC td;
    ID3D11Texture2D_GetDesc(tex, &td);
    D3D11_RENDER_TARGET_VIEW_DESC rd;
    ID3D11RenderTargetView_GetDesc(rtv, &rd);
    int ok = td.SampleDesc.Count == 1;
    if (ok && (!g_fx.bars || g_fx.bw != td.Width || g_fx.bh != td.Height || g_fx.bfmt != td.Format)) {
        D3D_RELEASE(g_fx.bars);
        D3D11_TEXTURE2D_DESC cd = { td.Width, td.Height, 1, 1, td.Format, { 1, 0 }, D3D11_USAGE_DEFAULT, 0, 0, 0 };
        HRESULT hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &cd, NULL, &g_fx.bars);
        if (FAILED(hr)) { g_fx.bfail = 1; ok = 0; rd_fail("the interface's bars' copy", hr); }
        g_fx.bw = td.Width; g_fx.bh = td.Height; g_fx.bfmt = td.Format;
    }
    if (ok) {
        UINT x0 = vp->TopLeftX > 0 ? (UINT)vp->TopLeftX : 0, y0 = vp->TopLeftY > 0 ? (UINT)vp->TopLeftY : 0;
        UINT x1 = (UINT)(vp->TopLeftX + vp->Width), y1 = (UINT)(vp->TopLeftY + vp->Height);
        if (x1 > td.Width) x1 = td.Width;
        if (y1 > td.Height) y1 = td.Height;
        const D3D11_BOX box[4] = { { 0, 0, 0, td.Width, y0, 1 }, { 0, y1, 0, td.Width, td.Height, 1 }, { 0, y0, 0, x0, y1, 1 }, { x1, y0, 0, td.Width, y1, 1 } };
        UINT mip = rd.Texture2D.MipSlice;
        for (int i = 0; i < 4; i++) {
            if (box[i].left >= box[i].right || box[i].top >= box[i].bottom) continue;
            if (back) ID3D11DeviceContext_CopySubresourceRegion(x, res, mip, box[i].left, box[i].top, 0, (ID3D11Resource *)g_fx.bars, 0, &box[i]);
            else ID3D11DeviceContext_CopySubresourceRegion(x, (ID3D11Resource *)g_fx.bars, 0, box[i].left, box[i].top, 0, res, mip, &box[i]);
        }
    }
    ID3D11Texture2D_Release(tex);
    ID3D11Resource_Release(res);
}

// the scene on rtv within vp smoothed (the caller binds its states again); dsv, vpm: its depth, its camera
internal void rd3_fxaa(ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp, ID3D11DepthStencilView *dsv, const float *vpm) {
    if (!g_fx.on || !rtv || g_fx.frame == g_rd_frames || !rd3_fxaa_init()) return;
    g_fx.frame = g_rd_frames;
    ID3D11Resource *res = NULL;
    ID3D11View_GetResource((ID3D11View *)rtv, &res);
    ID3D11Texture2D *tex = NULL;
    if (!res || FAILED(ID3D11Resource_QueryInterface(res, &IID_ID3D11Texture2D, (void **)&tex))) { D3D_RELEASE(res); return; }
    D3D11_TEXTURE2D_DESC td;
    ID3D11Texture2D_GetDesc(tex, &td);
    D3D11_RENDER_TARGET_VIEW_DESC rd;
    ID3D11RenderTargetView_GetDesc(rtv, &rd);
    // the viewport's pixels on the target
    UINT x0 = vp->TopLeftX > 0 ? (UINT)vp->TopLeftX : 0, y0 = vp->TopLeftY > 0 ? (UINT)vp->TopLeftY : 0;
    UINT x1 = (UINT)(vp->TopLeftX + vp->Width), y1 = (UINT)(vp->TopLeftY + vp->Height);
    if (x1 > td.Width) x1 = td.Width;
    if (y1 > td.Height) y1 = td.Height;
    if (td.SampleDesc.Count > 1 || x1 <= x0 + 8 || y1 <= y0 + 8) { ID3D11Texture2D_Release(tex); ID3D11Resource_Release(res); return; }
    UINT w = x1 - x0, h = y1 - y0;
    if (!g_fx.tex || g_fx.w != w || g_fx.h != h || g_fx.fmt != rd.Format) {
        D3D_RELEASE(g_fx.srv);
        D3D_RELEASE(g_fx.tex);
        D3D11_TEXTURE2D_DESC cd = { w, h, 1, 1, rd.Format, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
        HRESULT hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &cd, NULL, &g_fx.tex);
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_fx.tex, NULL, &g_fx.srv);
        if (FAILED(hr)) {
            rd_fail("the anti-aliasing's copy", hr);
            D3D_RELEASE(g_fx.srv); D3D_RELEASE(g_fx.tex);
            g_fx.on = 0;
            ID3D11Texture2D_Release(tex); ID3D11Resource_Release(res);
            return;
        }
        g_fx.w = w; g_fx.h = h; g_fx.fmt = rd.Format;
    }
    ID3D11DeviceContext *x = g_d3d.ctx;
    D3D11_BOX box = { x0, y0, 0, x1, y1, 1 };
    ID3D11DeviceContext_CopySubresourceRegion(x, (ID3D11Resource *)g_fx.tex, 0, 0, 0, 0, res, rd.Texture2D.MipSlice, &box);
    int have_z = rd3_fxaa_depth(x, dsv, &td), by_depth = have_z && g_fx.depth && g_fx.dps;
    rw_za(vpm, &g_fx.za);
    ID3D11Texture2D_Release(tex);
    ID3D11Resource_Release(res);
    D3D11_VIEWPORT v = { (float)x0, (float)y0, (float)w, (float)h, 0, 1 };
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &rtv, NULL);
    ID3D11DeviceContext_RSSetViewports(x, 1, &v);
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, NULL, 0);
    ID3D11DeviceContext_RSSetState(x, NULL);
    ID3D11DeviceContext_IASetInputLayout(x, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, g_d3d.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, by_depth ? g_fx.dps : g_fx.ps, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &g_fx.srv);
    if (have_z) ID3D11DeviceContext_PSSetShaderResources(x, 1, 1, &g_fx.zsrv);
    ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_d3d.linear);
    float k[8] = { g_fx.edge, g_fx.jump, g_fx.contrast, g_fx.sub, g_fx.za, (float)have_z, 0, 0 };
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_fx.cb, 0, NULL, k, 0, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 0, 1, &g_fx.cb);
    ID3D11DeviceContext_Draw(x, 3, 0);
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &none);
    if (have_z) ID3D11DeviceContext_PSSetShaderResources(x, 1, 1, &none);
    g_fx.passes++;
}

#include "render_ao.c"        // the darkening where things stand close ([display] ambient_occlusion)

internal int ud_pipe(char *out, const char *mode, const char *arg);   // render_units.c
internal int g_rwl_free, g_rwl_handoff_after; internal LONG g_rwl_skips;   // render_3d.c: the frame during a step (below its lock hooks)
internal int collect_anim_mode(int on), collect_own(int on), collect_opacity(int how), collect_drawn(int how), collect_fill_mode(int how), collect_ghost(int type, float x, float y), collect_copy_mode(int how), collect_smooth_mode(int how), collect_during_mode(int on, int ms), collect_effects_mode(int how);
internal LONG g_uc_walks;   // collect.c: the units' walk, its fills
internal volatile int g_cp_on;   // collect_copy.c: the world's copy the walk reads
extern volatile int g_uc_own; internal volatile int g_uc_opacity, g_uc_drawn, g_uc_fill;
// the pipe: picture [aa on | off] [aadepth on | off] [aajump | aacontrast | aasub <0..1>] [aniso <1..16>] [edge <0..1>] [steps on | off] [handoff <ms>] [morph on | off] [text on | off] [margin <0..2>] [fog <0..0.95>] [objects <0..0.5>] [depths on | off] [collect own | game | opacity | drawn game | check | own | fill game | check | own | ghost <type> <x> <y> | copy off | on | check]
internal void cmd_picture(char *out, const char *args) {
    char mode[16] = "", arg[16] = "";
    sscanf(args, "%15s %15s", mode, arg);
    if (rao_pipe(out, mode, arg)) return;   // render_ao.c
    if (rgb_pipe(out, mode, arg)) return;   // render_ground.c
    if (ud_pipe(out, mode, arg)) return;    // render_units.c
    if (!strcmp(mode, "aa")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_fx.on = arg[1] == 'n';
        else { REPLY("error: usage: picture aa on | off\n"); return; }
    } else if (!strcmp(mode, "inst")) {   // render_instance.c
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_in_force = arg[1] == 'n';
        else { REPLY("instances: %s, parts drawn so %ld, models left to the display DLL %ld, trees' shadow cards made %ld\n", (g_in_force < 0 ? g_set.unit_instancing : g_in_force) ? "on" : "off", (long)g_in_parts, (long)g_in_fell, (long)g_in_cards); return; }
    } else if (!strcmp(mode, "sil")) {   // render_silhouette.c
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_sl.force = arg[1] == 'n';
        else { REPLY("error: usage: picture sil on | off\n"); return; }
    } else if (!strcmp(mode, "silfade")) {   // render_silhouette.c: picture silfade <tiles behind the face it starts> <where it is gone>; gone <= start: no fade
        float a = -1, b = -1;
        if (sscanf(args, "%*s %f %f", &a, &b) != 2 || a < 0 || b < 0) { REPLY("error: usage: picture silfade <tiles it starts> <tiles it is gone>\n"); return; }
        g_sl.fs = a; g_sl.fe = b;
        REPLY("the silhouettes fade from %.1f to %.1f tiles behind the face\n", (double)a, (double)b);
        return;
    } else if (!strcmp(mode, "aadepth")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_fx.depth = arg[1] == 'n';
        else { REPLY("error: usage: picture aadepth on | off\n"); return; }
    } else if (!strcmp(mode, "aajump") || !strcmp(mode, "aacontrast") || !strcmp(mode, "aasub")) {
        float e = (float)atof(arg);
        if (!arg[0] || e < 0 || e > 1) { REPLY("error: usage: picture %s <0..1>\n", mode); return; }
        *(mode[2] == 'j' ? &g_fx.jump : mode[2] == 'c' ? &g_fx.contrast : &g_fx.sub) = e;
    } else if (!strcmp(mode, "aniso")) {
        int n = atoi(arg);
        if (n >= 1 && n <= 16) g_fx.aniso = n;
        else { REPLY("error: usage: picture aniso <1..16>\n"); return; }
    } else if (!strcmp(mode, "edge")) {
        float e = (float)atof(arg);
        if (e > 0 && e < 1) g_fx.edge = e;
        else { REPLY("error: usage: picture edge <0..1>\n"); return; }
    } else if (!strcmp(mode, "camtrace")) {
        char dx[16] = "", dy[16] = "";
        sscanf(args, "%*s %*s %15s %15s", dx, dy);
        g_kg_trace_dx = (float)atof(dx);
        g_kg_trace_dy = (float)atof(dy);
        g_kg_trace = atoi(arg);
    } else if (!strcmp(mode, "margin")) {
        float m = (float)atof(arg);
        if (arg[0] && m >= 0 && m <= 2) g_gm_margin = m;
        else { REPLY("error: usage: picture margin <0..2>\n"); return; }
    } else if (!strcmp(mode, "fog")) {   // render_frame.c rf_fog_start: where the distance's fog starts, of its end
        float s = (float)atof(arg);
        if (arg[0] && s >= 0 && s <= 0.95f) g_rf_fog_start = s;
        else { REPLY("error: usage: picture fog <0..0.95> (of the fog's end; 0 the game's)\n"); return; }
    } else if (!strcmp(mode, "handoff")) {
        int ms = atoi(arg);
        if (arg[0] && ms >= 0 && ms <= 1000) g_rwl_handoff_after = ms;
        else { REPLY("error: usage: picture handoff <0..1000 ms>\n"); return; }
    } else if (!strcmp(mode, "depths")) {   // render_frame.c: the frame's own near and far depths
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_dz.off = arg[1] != 'n';
        else {
            REPLY("depths: the frame's own %s; frames with them %ld (nearer %ld, further %ld); the last: near %.2f (the step's %.2f), far %.2f (%.2f)\n",
                  g_dz.off ? "off" : "on", g_dz.frames, g_dz.nearer, g_dz.further, (double)g_dz.n, (double)g_dz.n0, (double)g_dz.f, (double)g_dz.f0);
            return;
        }
    } else if (!strcmp(mode, "objects")) {
        float m = (float)atof(arg);
        if (arg[0] && m >= 0 && m <= 0.5f) g_om_margin = m;
        else { REPLY("error: usage: picture objects <0..0.5>\n"); return; }
    } else if (!strcmp(mode, "ground") && !strcmp(arg, "raise")) {   // a test: the ground raised dz (1) at x, y (5 x 5 corners) as the game changes it, through its imports - below 0 a lake
        int x = 0, y = 0;
        float dz = 1;
        if (sscanf(args, "%*s %*s %d %d %f", &x, &y, &dz) < 2 || !g_rg.mesh || x < 3 || y < 3 || x + 3 >= g_rg.size || y + 3 >= g_rg.size) { REPLY("error: usage: picture ground raise <x> <y> [dz]\n"); return; }
        for (int j = -2; j <= 2; j++) for (int i = -2; i <= 2; i++) mesh_set_z(g_rg.mesh, (unsigned long)(x + i), (unsigned long)(y + j), g_rg.z(g_rg.mesh, (unsigned long)(x + i), (unsigned long)(y + j)) + dz);
        for (int j = -3; j <= 3; j++) for (int i = -3; i <= 3; i++) mesh_compute_normal(g_rg.mesh, (unsigned long)(x + i), (unsigned long)(y + j));
        REPLY("ground raised at %d %d; chunks made again so far %ld\n", x, y, g_rg.remade);
        return;
    } else if (!strcmp(mode, "ground")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_rg.on = arg[1] == 'n';
        else { REPLY("error: usage: picture ground on | off\n"); return; }
        REPLY("ground from the map %s: %ld of the chunks' draws last frame, %ld triangles, %d textures\n", g_rg.on ? "on" : "off", g_rg.drawn, g_rg.tris, g_rg.nmat);
        return;
    } else if (!strcmp(mode, "morph")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_mo.on = arg[1] == 'n' && g_mo_smooth;
        else { REPLY("error: usage: picture morph on | off\n"); return; }
    } else if (!strcmp(mode, "text")) {   // display_text.c: the GDI text onto the target on the GPU, or by locks
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_text_layer = arg[1] == 'n';
        else { REPLY("error: usage: picture text on | off\n"); return; }
    } else if (!strcmp(mode, "steps")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_rwl_free = arg[1] == 'n';
        else { REPLY("error: usage: picture steps on | off\n"); return; }
    } else if (!strcmp(mode, "collect") && !strcmp(arg, "ghost")) {   // collect.c: a placement ghost for the fill's check
        int type = -2;
        float x = 0, y = 0;
        if (sscanf(args, "%*s %*s %d %f %f", &type, &x, &y) != 3 || type < -1) { REPLY("error: usage: picture collect ghost <type | -1> <x> <y>\n"); return; }
        if (collect_ghost(type, x, y) < 0) { REPLY("error: the units' walk isn't hooked (the log: # collect)\n"); return; }
    } else if (!strcmp(mode, "collect") && !strcmp(arg, "effects")) {   // collect_copy.c: the effects' queues the simulation makes
        char how[16] = "";
        sscanf(args, "%*s %*s %15s", how);
        int m = !strcmp(how, "off") ? 0 : !strcmp(how, "on") ? 1 : !strcmp(how, "check") ? 2 : -1;
        if (m < 0) { REPLY("error: usage: picture collect effects off | on | check\n"); return; }
        if (collect_effects_mode(m) < 0) { REPLY("error: the units' walk isn't hooked (the log: # collect)\n"); return; }
    } else if (!strcmp(mode, "collect") && !strcmp(arg, "during")) {   // collect.c: the units' update during a step
        char how[16] = "";
        int ms = 250;
        sscanf(args, "%*s %*s %15s %d", how, &ms);
        int m = !strcmp(how, "off") ? 0 : !strcmp(how, "on") ? 1 : -1;
        if (m < 0 || ms < 0) { REPLY("error: usage: picture collect during off | on [hand-off ms]\n"); return; }
        if (collect_during_mode(m, ms) < 0) { REPLY("error: the units' walk isn't hooked (the log: # collect)\n"); return; }
    } else if (!strcmp(mode, "collect") && !strcmp(arg, "anim")) {   // collect.c: the animation between steps
        char how[16] = "";
        sscanf(args, "%*s %*s %15s", how);
        if (strcmp(how, "on") && strcmp(how, "off")) { REPLY("error: usage: picture collect anim on | off\n"); return; }
        collect_anim_mode(how[1] == 'n');
    } else if (!strcmp(mode, "collect") && !strcmp(arg, "smooth")) {   // collect.c: the moving between steps
        char how[16] = "";
        sscanf(args, "%*s %*s %15s", how);
        int m = !strcmp(how, "off") ? 0 : !strcmp(how, "on") ? 1 : !strcmp(how, "probe") ? 2 : -1;
        if (m < 0) { REPLY("error: usage: picture collect smooth off | on | probe\n"); return; }
        if (collect_smooth_mode(m) < 0) { REPLY("error: the units' walk isn't hooked (the log: # collect)\n"); return; }
    } else if (!strcmp(mode, "collect") && !strcmp(arg, "copy")) {   // collect_copy.c: the world's copy the walk reads
        char how[16] = "";
        sscanf(args, "%*s %*s %15s", how);
        int m = !strcmp(how, "off") ? 0 : !strcmp(how, "on") ? 1 : !strcmp(how, "check") ? 2 : -1;
        if (m < 0) { REPLY("error: usage: picture collect copy off | on | check\n"); return; }
        if (collect_copy_mode(m) < 0) { REPLY("error: the units' walk isn't hooked (the log: # collect)\n"); return; }
    } else if (!strcmp(mode, "collect")) {   // collect.c: the units' walk the mod's own or the game's
        char how[16] = "";
        sscanf(args, "%*s %*s %15s", how);
        int fill = !strcmp(arg, "opacity") ? 1 : !strcmp(arg, "drawn") ? 2 : !strcmp(arg, "fill") ? 3 : 0;
        int bad = !fill ? strcmp(arg, "own") && strcmp(arg, "game")
                                         : strcmp(how, "game") && strcmp(how, "check") && strcmp(how, "own");
        if (bad) { REPLY("error: usage: picture collect own | game | opacity | drawn game | check | own | fill game | check | own\n"); return; }
        int m = how[0] == 'g' ? 0 : how[0] == 'c' ? 1 : 2;
        int r = !fill ? collect_own(arg[0] == 'o') : fill == 1 ? collect_opacity(m) : fill == 2 ? collect_drawn(m) : collect_fill_mode(m);
        if (r < 0) { REPLY("error: the units' walk isn't hooked (the log: # collect)\n"); return; }
    } else if (mode[0]) { REPLY("error: usage: picture [aa on | off] [aadepth on | off] [aajump | aacontrast | aasub <0..1>] [aniso <1..16>] [edge <0..1>] [steps on | off] [handoff <ms>] [morph on | off] [text on | off] [margin <0..2>] [objects <0..0.5>] [depths on | off] [collect own | game | opacity | drawn game | check | own | fill game | check | own | ghost <type> <x> <y> | copy off | on | check]\n"); return; }
    REPLY("picture: anti-aliasing %s (%s, %ld passes, an edge %.2f of its brightest; by the depth %s, jump %.3f, contrast %.3f, sub %.2f), anisotropy %d; text on the target %s (frames %ld, text phases %ld, reads back %ld so far); frames drawn during a"
          " step %s (%ld; the simulation waits for one after %d ms without); the ground's margin %.2f (the last region %ld x %ld tiles at %.2f, its top %.2f, its bottom %.2f, the depths %.2f .. %.2f, its near %.2f (its camera pulled back %.1f for a zoom out), worked out %ld, its polygons %lu and %lu on the water; the ground kept %d batches, %d vertices); the objects' margin %.2f (%ld passed by it); the units' walk %s (%ld walks of the mod's), its opacity %s, its view test %s, its fill %s, its copy of the world %s\n", g_fx.on ? "on" : "off",
          g_fx.ready > 0 ? "ready" : g_fx.ready < 0 ? "FAILED (the log says why)" : "not made yet",
          (long)g_fx.passes, (double)g_fx.edge, g_fx.dfail ? "FAILED" : g_fx.depth ? "on" : "off", (double)g_fx.jump, (double)g_fx.contrast, (double)g_fx.sub, g_fx.aniso, g_text_layer ? "on the GPU" : "by locks", (long)g_rd_frames, (long)g_tdc_phases, (long)g_rd_reads, g_rwl_free ? "on" : "off", (long)g_rwl_skips, g_rwl_handoff_after,
          (double)g_gm_margin, g_gm_size[0] + 1, g_gm_size[1] + 1, (double)g_gm_k, (double)g_gm_kt, (double)g_gm_kb, (double)g_gm_depths[0], (double)g_gm_depths[1], (double)g_gm_near, (double)g_gm_ride, g_gm_pred + 1, g_gm_polys[0], g_gm_polys[1], g_kg.nb, g_kg.nv, (double)g_om_margin, (long)g_om_passed,
          g_uc_own ? "the mod's" : "the game's", (long)g_uc_walks,
          g_uc_opacity == 2 ? "the mod's" : g_uc_opacity == 1 ? "the game's, the mod's held against it" : "the game's",
          g_uc_drawn == 2 ? "the mod's" : g_uc_drawn == 1 ? "the game's, the mod's held against it" : "the game's",
          g_uc_fill == 2 ? "the mod's" : g_uc_fill == 1 ? "the game's, the trees', doppelgangers' and ghosts' held against the mod's" : "the game's",
          g_cp_on == 2 ? "the simulation's, held against the frame's" : g_cp_on ? "the simulation's" : "the frame's own");
}
