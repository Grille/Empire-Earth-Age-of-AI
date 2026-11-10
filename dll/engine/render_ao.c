// THE AMBIENT OCCLUSION ([display] ambient_occlusion, ambient_occlusion_radius, ambient_occlusion_strength; the pipe's
// picture ao, aoradius, aostrength, aoview): the units and buildings stood on the ground as if pasted on it - the game's
// light knows nothing of what is near a surface, and its shadow is a sun's. Where something stands close to a surface the
// picture is darkened, worked out from the scene's depth alone once a frame (Scalable Ambient Obscurance, McGuire's: each
// pixel's place in the view from its depth, samples on a spiral around it within a radius in tiles, each counted by how
// far it rises over the pixel's surface and gone at the radius, so that a thing far in front of what lies behind it leaves
// no halo). Then blurred along the two axes, not across a jump of the depth (the samples' spiral is turned per pixel:
// their pattern is noise, which the blur takes out), and multiplied into the picture. At the last of the units' batches
// drawn in their place (render_frame.c rf_replay): all that writes the depth is there, and the water's surface, the
// silhouettes and the interface come after it and stay as they are. A pixel of depth 0 is the panels' (render_fxaa.c), of
// depth 1 nothing drawn.
internal struct {
    int force, ready, fail, view;   // force: the pipe's on / off (-1: the setting); view: the occlusion alone on the screen
    float radius, strength;         // the pipe's (< 0: the settings')
    ID3D11PixelShader *ps[4];       // the occlusion, the blur, the blur onto the picture, that as the picture itself
    ID3D11Buffer *cb;
    ID3D11BlendState *mul;
    ID3D11Texture2D *tex[2];
    ID3D11RenderTargetView *rtv[2];
    ID3D11ShaderResourceView *srv[2];
    UINT w, h;
    LONG passes;
    ID3D11Query *q[3];              // what the pass takes on the graphics card: every few frames, read when it has come back
    int asked;
    float ms;                       // the last measured
} g_ao = { .force = -1, .radius = -1, .strength = -1 };

internal const char k_ao_shader[] =
    "#define RAO_N 12\n"
    "Texture2D<float> zt : register(t0); Texture2D<float> at : register(t1); cbuffer k : register(b0) { float4 kp; float4 kd; float4 ka; float4 ko; };\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    // kp: from the pixel to the view's x, y at depth 1 (x = (px * kp.x + kp.y) * z ...); kd: depth d = A + B / z;
    // ka: the radius in tiles, its square, what a tile is in pixels at depth 1, the strength; ko: the viewport's corner on the target
    "float zof(float d) { return kd.y / min(d - kd.x, -1e-7); }\n"
    "float3 at3(int2 p) { float z = zof(zt.Load(int3(p, 0))); return float3((p.x + 0.5 - ko.x) * kp.x + kp.y, (p.y + 0.5 - ko.y) * kp.z + kp.w, 1) * z; }\n"
    "float3 side(float3 m, int2 p, int2 s) { float3 a = at3(p + s), b = at3(p - s); float3 da = a - m, db = m - b; return abs(da.z) < abs(db.z) ? da : db; }\n"
    "float4 ao(V i) : SV_Target { int2 p = int2(i.p.xy) + int2(ko.xy); float d = zt.Load(int3(p, 0)); if (d <= 0 || d >= 1) return 1;"
    " float3 m = at3(p), n = normalize(cross(side(m, p, int2(0, 1)), side(m, p, int2(1, 0)))); if (n.z > 0) n = -n;"
    " float r = min(ka.z * ka.x / m.z, ko.w); if (r < 1) return 1;"
    // the spiral's turn per pixel (interleaved gradient noise, Jimenez's)
    " float ph = 6.2831853 * frac(52.9829189 * frac(dot(float2(p), float2(0.06711056, 0.00583715)))), s = 0;"
    " for (int j = 0; j < RAO_N; j++) { float a = (j + 0.5) / RAO_N, t = a * 6.2831853 * 3 + ph; int2 q = p + int2(float2(cos(t), sin(t)) * a * r);"
    "  float dq = zt.Load(int3(q, 0)); if (dq <= 0 || dq >= 1) continue;"
    "  float3 v = at3(q) - m; float vv = dot(v, v), f = max(ka.y - vv, 0);"
    "  s += f * f * f * max((dot(v, n) - 0.002 * m.z) / (vv + 0.01), 0); }"
    " return saturate(1 - s * 5 * ka.w * ka.x / (RAO_N * ka.y * ka.y * ka.y)); }\n"
    // the blur, 7 pixels along ko.z's axis (0: x, 1: y), weighed by the depth's nearness
    "float bl(int2 p, int2 s) { float z = zof(zt.Load(int3(p + int2(ko.xy), 0))), o = 0, w = 0;"
    " for (int j = -3; j <= 3; j++) { int2 q = p + s * j; float g = 1 - abs(j) * 0.2, zq = zof(zt.Load(int3(q + int2(ko.xy), 0))), e = g * max(1 - abs(zq - z) / (0.02 * z + 0.05), 0);"
    "  o += at.Load(int3(q, 0)) * e; w += e; }"
    " return w > 0 ? o / w : 1; }\n"
    "float4 bx(V i) : SV_Target { return bl(int2(i.p.xy), int2(1, 0)); }\n"
    // onto the picture, which it multiplies (the pixel's place is the target's there: ko.xy back off)
    "float4 by(V i) : SV_Target { int2 p = int2(i.p.xy); float d = zt.Load(int3(p, 0)); if (d <= 0 || d >= 1) discard; return bl(p - int2(ko.xy), int2(0, 1)); }\n"
    "float4 bv(V i) : SV_Target { int2 p = int2(i.p.xy); if (zt.Load(int3(p, 0)) <= 0) discard; float a = bl(p - int2(ko.xy), int2(0, 1)); return float4(a, a, a, 1); }\n";

internal int rao_init(void) {
    if (g_ao.ready) return g_ao.ready > 0;
    g_ao.ready = -1;
    static const char *const entry[4] = { "ao", "bx", "by", "bv" };
    HRESULT hr = S_OK;
    for (int i = 0; i < 4 && SUCCEEDED(hr); i++) {
        ID3DBlob *code = d3d_compile(k_ao_shader, sizeof k_ao_shader - 1, entry[i], "ps_4_0");
        hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_ao.ps[i]) : E_FAIL;
        D3D_RELEASE(code);
    }
    D3D11_BUFFER_DESC bd = { 64, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_ao.cb);
    D3D11_BLEND_DESC bl = { 0 };
    bl.RenderTarget[0].BlendEnable = TRUE;
    bl.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
    bl.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
    bl.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    bl.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    bl.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBlendState(g_d3d.dev, &bl, &g_ao.mul);
    if (FAILED(hr)) return rd_fail("the ambient occlusion's shaders", hr), 0;
    g_ao.ready = 1;
    return 1;
}
internal int rao_targets(UINT w, UINT h) {
    if (g_ao.tex[0] && g_ao.w == w && g_ao.h == h) return 1;
    for (int i = 0; i < 2; i++) { D3D_RELEASE(g_ao.srv[i]); D3D_RELEASE(g_ao.rtv[i]); D3D_RELEASE(g_ao.tex[i]); }
    HRESULT hr = S_OK;
    for (int i = 0; i < 2 && SUCCEEDED(hr); i++) {
        D3D11_TEXTURE2D_DESC td = { w, h, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, 0 };
        hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &g_ao.tex[i]);
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRenderTargetView(g_d3d.dev, (ID3D11Resource *)g_ao.tex[i], NULL, &g_ao.rtv[i]);
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_ao.tex[i], NULL, &g_ao.srv[i]);
    }
    if (FAILED(hr)) { g_ao.fail = 1; rd_fail("the ambient occlusion's pictures", hr); return 0; }
    g_ao.w = w; g_ao.h = h;
    return 1;
}
// the pass's time on the graphics card: asked for when the last answer is in (begin: 1 before the pass, 0 after it)
internal void rao_time(ID3D11DeviceContext *x, int begin) {
    if (!g_ao.q[0]) {
        D3D11_QUERY_DESC d[3] = { { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 }, { D3D11_QUERY_TIMESTAMP, 0 }, { D3D11_QUERY_TIMESTAMP, 0 } };
        for (int i = 0; i < 3; i++) ID3D11Device_CreateQuery(g_d3d.dev, &d[i], &g_ao.q[i]);
    }
    if (!g_ao.q[0] || !g_ao.q[1] || !g_ao.q[2]) return;
    if (begin) {
        if (g_ao.asked) {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj;
            UINT64 t0, t1;
            if (ID3D11DeviceContext_GetData(x, (ID3D11Asynchronous *)g_ao.q[0], &dj, sizeof dj, D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK
                || ID3D11DeviceContext_GetData(x, (ID3D11Asynchronous *)g_ao.q[1], &t0, sizeof t0, D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK
                || ID3D11DeviceContext_GetData(x, (ID3D11Asynchronous *)g_ao.q[2], &t1, sizeof t1, D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) return;
            if (!dj.Disjoint && dj.Frequency) g_ao.ms = (float)((double)(t1 - t0) * 1000 / (double)dj.Frequency);
            g_ao.asked = 0;
        }
        g_ao.asked = 1;
        ID3D11DeviceContext_Begin(x, (ID3D11Asynchronous *)g_ao.q[0]);
        ID3D11DeviceContext_End(x, (ID3D11Asynchronous *)g_ao.q[1]);
    } else if (g_ao.asked == 1) {
        g_ao.asked = 2;
        ID3D11DeviceContext_End(x, (ID3D11Asynchronous *)g_ao.q[2]);
        ID3D11DeviceContext_End(x, (ID3D11Asynchronous *)g_ao.q[0]);
    }
}
// the projection's numbers from a matrix view x projection whose view turns but doesn't scale: how much a view's x and y
// are on the screen, and the depth's A and B; 0: no perspective
internal int rao_projection(const float *m, float *sx, float *sy, float *a, float *b) {
    int k = 0;
    for (int j = 1; j < 3; j++) if (fabsf(m[j * 4 + 3]) > fabsf(m[k * 4 + 3])) k = j;
    if (fabsf(m[k * 4 + 3]) < 1e-6f) return 0;
    *sx = sqrtf(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
    *sy = sqrtf(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
    *a = m[k * 4 + 2] / m[k * 4 + 3];
    *b = m[14] - *a * m[15];
    return *sx > 1e-6f && *sy > 1e-6f && fabsf(*b) > 1e-9f;
}
internal int rao_on(void) { return !g_ao.fail && (g_ao.force < 0 ? g_set.ambient_occlusion : g_ao.force); }
// the scene so far on rtv within vp, with its depth dsv and its camera vpm, darkened (the caller binds its states again)
internal void rd3_ao(ID3D11DeviceContext *x, ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp, ID3D11DepthStencilView *dsv, const float *vpm) {
    float sx, sy, a, b;
    if (!rao_on() || !rtv || !dsv || !rd3_fxaa_init() || !rao_init() || !rao_projection(vpm, &sx, &sy, &a, &b)) return;
    ID3D11Resource *res = NULL;
    ID3D11Texture2D *tex = NULL;
    ID3D11View_GetResource((ID3D11View *)rtv, &res);
    if (!res || FAILED(ID3D11Resource_QueryInterface(res, &IID_ID3D11Texture2D, (void **)&tex))) { D3D_RELEASE(res); return; }
    D3D11_TEXTURE2D_DESC td;
    ID3D11Texture2D_GetDesc(tex, &td);
    ID3D11Texture2D_Release(tex);
    ID3D11Resource_Release(res);
    UINT x0 = vp->TopLeftX > 0 ? (UINT)vp->TopLeftX : 0, y0 = vp->TopLeftY > 0 ? (UINT)vp->TopLeftY : 0;
    UINT x1 = (UINT)(vp->TopLeftX + vp->Width), y1 = (UINT)(vp->TopLeftY + vp->Height);
    if (x1 > td.Width) x1 = td.Width;
    if (y1 > td.Height) y1 = td.Height;
    if (td.SampleDesc.Count > 1 || x1 <= x0 + 8 || y1 <= y0 + 8 || !rao_targets(x1 - x0, y1 - y0) || !rd3_fxaa_depth(x, dsv, &td)) return;
    rao_time(x, 1);
    float w = (float)(x1 - x0), h = (float)(y1 - y0);
    float r = g_ao.radius >= 0 ? g_ao.radius : (float)g_set.ambient_occlusion_radius;
    float st = g_ao.strength >= 0 ? g_ao.strength : (float)g_set.ambient_occlusion_strength;
    float k[16] = { 2 / (w * sx), -1 / sx, -2 / (h * sy), 1 / sy,   a, b, 0, 0,   r, r * r, sy * h / 2, st,   (float)x0, (float)y0, 0, 96 };
    D3D11_VIEWPORT v = { 0, 0, w, h, 0, 1 }, on = { (float)x0, (float)y0, w, h, 0, 1 };
    ID3D11ShaderResourceView *none[2] = { NULL, NULL };
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, NULL, 0);
    ID3D11DeviceContext_RSSetState(x, NULL);
    ID3D11DeviceContext_IASetInputLayout(x, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, g_d3d.vs, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 0, 1, &g_ao.cb);
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_ao.cb, 0, NULL, k, 0, 0);
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &g_fx.zsrv);
    // the occlusion
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_ao.rtv[0], NULL);
    ID3D11DeviceContext_RSSetViewports(x, 1, &v);
    ID3D11DeviceContext_PSSetShader(x, g_ao.ps[0], NULL, 0);
    ID3D11DeviceContext_Draw(x, 3, 0);
    // across
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_ao.rtv[1], NULL);
    ID3D11DeviceContext_PSSetShaderResources(x, 1, 1, &g_ao.srv[0]);
    ID3D11DeviceContext_PSSetShader(x, g_ao.ps[1], NULL, 0);
    ID3D11DeviceContext_Draw(x, 3, 0);
    ID3D11DeviceContext_PSSetShaderResources(x, 1, 1, &none[0]);
    // down, onto the picture
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &rtv, NULL);
    ID3D11DeviceContext_RSSetViewports(x, 1, &on);
    ID3D11DeviceContext_PSSetShaderResources(x, 1, 1, &g_ao.srv[1]);
    ID3D11DeviceContext_OMSetBlendState(x, g_ao.view ? NULL : g_ao.mul, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_PSSetShader(x, g_ao.ps[g_ao.view ? 3 : 2], NULL, 0);
    ID3D11DeviceContext_Draw(x, 3, 0);
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 2, none);
    rao_time(x, 0);
    g_ao.passes++;
}
// the pipe: picture ao on | off | view | setting, aoradius <tiles>, aostrength <0..4>
internal int rao_pipe(char *out, const char *mode, const char *arg) {
    if (!strcmp(mode, "ao")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) { g_ao.force = arg[1] == 'n'; g_ao.view = 0; }
        else if (!strcmp(arg, "view")) { g_ao.force = 1; g_ao.view = 1; }
        else if (arg[0]) { REPLY("error: usage: picture ao on | off | view\n"); return 1; }
    } else if (!strcmp(mode, "aoradius") || !strcmp(mode, "aostrength")) {
        float e = (float)atof(arg);
        if (!arg[0] || e < 0 || e > 16) { REPLY("error: usage: picture %s <0..16>\n", mode); return 1; }
        *(mode[2] == 'r' ? &g_ao.radius : &g_ao.strength) = e;
    } else return 0;
    REPLY("ambient occlusion %s%s, %ld passes, %.2f ms on the graphics card, radius %.2f tiles, strength %.2f\n", rao_on() ? "on" : "off", g_ao.view ? " (alone on the screen)" : "", (long)g_ao.passes, (double)g_ao.ms,
          (double)(g_ao.radius >= 0 ? g_ao.radius : (float)g_set.ambient_occlusion_radius), (double)(g_ao.strength >= 0 ? g_ao.strength : (float)g_set.ambient_occlusion_strength));
    return 1;
}
