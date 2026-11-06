// THE PICTURE'S SHARPNESS ([display] antialias, anisotropy; the pipe's picture): the 3D scene's edges smoothed once a
// frame by FXAA (the walls', towers' and units' outlines were stairs), at the first of the interface's draws after it
// (rf_replay) - the interface's text and panels drawn over it afterwards stay sharp. The scene's part of the target is
// copied into a texture of its own and drawn back through the filter: the present's triangle over the viewport
// (present_d3d.c's vertex shader), every state the list binds set again after it. The anisotropy is the samplers'
// (rd3_mk_samp: the game's linear minifying with mip levels).
internal struct {
    int on, aniso;
    float edge;                     // the luma range an edge has at least, of its brightest (the pipe: picture edge)
    int ready;                      // 1 made, -1 failed
    ID3D11PixelShader *ps;
    ID3D11Buffer *cb;
    ID3D11Texture2D *tex;           // the scene's copy
    ID3D11ShaderResourceView *srv;
    UINT w, h;
    DXGI_FORMAT fmt;
    LONG frame;                     // the frame it ran in last (once a frame)
    LONG passes;
} g_fx = { .aniso = 1, .edge = 0.5f };

// FXAA in its simple form (Lottes's, the luma of the four diagonal neighbours): where their range is an edge's, along
// its direction two and four samples averaged, the four's unless they leave the neighbours' luma range
internal const char k_fx_shader[] =
    "Texture2D t : register(t0); SamplerState s : register(s0); cbuffer k : register(b0) { float4 k; };\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    "float luma(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }\n"
    "float3 at(float2 uv) { return t.SampleLevel(s, uv, 0).rgb; }\n"
    "float4 ps(V i) : SV_Target { float w, h; t.GetDimensions(w, h); float2 r = 1 / float2(w, h), uv = i.uv;"
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
    " float lb = luma(b); return float4(lb < lo || lb > hi ? a : b, 1); }\n";

internal int rd3_fxaa_init(void) {
    if (g_fx.ready) return g_fx.ready > 0;
    g_fx.ready = -1;
    ID3DBlob *code = d3d_compile(k_fx_shader, sizeof k_fx_shader - 1, "ps", "ps_4_0");
    HRESULT hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_fx.ps) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_BUFFER_DESC bd = { 16, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_fx.cb);
    if (FAILED(hr)) return rd_fail("the anti-aliasing's shader", hr), 0;
    g_fx.ready = 1;
    return 1;
}

// the scene on rtv within vp smoothed (the caller binds its states again)
internal void rd3_fxaa(ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp) {
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
    ID3D11DeviceContext_PSSetShader(x, g_fx.ps, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &g_fx.srv);
    ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_d3d.linear);
    float k[4] = { g_fx.edge, 0, 0, 0 };
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_fx.cb, 0, NULL, k, 0, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 0, 1, &g_fx.cb);
    ID3D11DeviceContext_Draw(x, 3, 0);
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &none);
    g_fx.passes++;
}

internal int g_rwl_free; internal LONG g_rwl_skips;   // render_3d.c: the frame during a step (below its lock hooks)
// the pipe: picture [aa on | off] [aniso <1..16>] [edge <0..1>] [steps on | off] [morph on | off] [text on | off] [margin <0..2>] [objects <0..0.5>]
internal void cmd_picture(char *out, const char *args) {
    char mode[16] = "", arg[16] = "";
    sscanf(args, "%15s %15s", mode, arg);
    if (!strcmp(mode, "aa")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_fx.on = arg[1] == 'n';
        else { REPLY("error: usage: picture aa on | off\n"); return; }
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
    } else if (!strcmp(mode, "objects")) {
        float m = (float)atof(arg);
        if (arg[0] && m >= 0 && m <= 0.5f) g_om_margin = m;
        else { REPLY("error: usage: picture objects <0..0.5>\n"); return; }
    } else if (!strcmp(mode, "morph")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_mo.on = arg[1] == 'n' && g_mo_smooth;
        else { REPLY("error: usage: picture morph on | off\n"); return; }
    } else if (!strcmp(mode, "text")) {   // display_text.c: the GDI text onto the target on the GPU, or by locks
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_text_layer = arg[1] == 'n';
        else { REPLY("error: usage: picture text on | off\n"); return; }
    } else if (!strcmp(mode, "steps")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_rwl_free = arg[1] == 'n';
        else { REPLY("error: usage: picture steps on | off\n"); return; }
    } else if (mode[0]) { REPLY("error: usage: picture [aa on | off] [aniso <1..16>] [edge <0..1>] [steps on | off] [morph on | off] [text on | off] [margin <0..2>] [objects <0..0.5>]\n"); return; }
    REPLY("picture: anti-aliasing %s (%s, %ld passes, an edge %.2f of its brightest), anisotropy %d; text on the target %s (frames %ld, text phases %ld, reads back %ld so far); frames drawn during a"
          " step %s (%ld); the ground's margin %.2f (the last region %ld x %ld tiles at %.2f, its top %.2f, its bottom %.2f, the depths %.2f .. %.2f, its near %.2f, worked out %ld, its polygons %lu and %lu on the water; the ground kept %d batches, %d vertices); the objects' margin %.2f (%ld passed by it)\n", g_fx.on ? "on" : "off",
          g_fx.ready > 0 ? "ready" : g_fx.ready < 0 ? "FAILED (the log says why)" : "not made yet",
          (long)g_fx.passes, (double)g_fx.edge, g_fx.aniso, g_text_layer ? "on the GPU" : "by locks", (long)g_rd_frames, (long)g_tdc_phases, (long)g_rd_reads, g_rwl_free ? "on" : "off", (long)g_rwl_skips,
          (double)g_gm_margin, g_gm_size[0] + 1, g_gm_size[1] + 1, (double)g_gm_k, (double)g_gm_kt, (double)g_gm_kb, (double)g_gm_depths[0], (double)g_gm_depths[1], (double)g_gm_near, g_gm_pred + 1, g_gm_polys[0], g_gm_polys[1], g_kg.nb, g_kg.nv, (double)g_om_margin, (long)g_om_passed);
}
