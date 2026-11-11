// The debug overlay's drawing ([display] overlay; base/overlay.c keeps the shapes): once a frame, where the game
// turns from the map to the interface (its first draw without the depth test after the map's camera, else at the
// scene's end), the shapes go into the map's view with that camera - lines and circles as ribbons on the ground
// (the tiles' heights, bilinear between their centres, a little above), tiles as quads, rings as the game's command
// circle (below); depth tested so hills and buildings hide them, not written; blended by their alpha; under the
// interface drawn after them.

#define RO_VERTS 131072   // 3 MB of 24-byte vertices: about 21000 ribbon quads a frame
typedef struct { float x, y, z; uint32_t argb; float u, v; } ro_vertex;
internal const char k_ro_shader[] =
    "cbuffer o : register(b0) { row_major float4x4 vp; float4 px; };\n"
    "Texture2D tx : register(t0); SamplerState sm : register(s0);\n"
    "struct I { float3 p : POSITION; float4 c : COLOR; float2 t : TEXCOORD; };\n"
    "struct O { float4 p : SV_Position; float4 c : COLOR; float2 t : TEXCOORD; };\n"
    "O vs(I i) { O o; o.p = mul(float4(i.p, 1), vp); o.p.xy += px.xy * o.p.w; o.c = i.c.zyxw; o.t = i.t; return o; }\n"   // ARGB's bytes: B G R A
    "float4 ps(O i) : SV_Target { return i.c; }\n"
    "float4 pst(O i) : SV_Target { return i.c * tx.Sample(sm, i.t); }\n";
internal struct {
    int ready;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps, *pst;
    ID3D11InputLayout *inputs;
    ID3D11Buffer *cb, *vb;
    ID3D11SamplerState *samp;
    ro_vertex *v;
    int n;
    ov_shape shapes[OV_SHAPES];
    // the map, read once a frame
    ee_tile **tiles;
    int size;
} g_ro;

internal int ro_init(void) {
    if (g_ro.ready) return g_ro.ready > 0;
    g_ro.ready = -1;
    if (!g_d3d.dev || !(g_ro.v = malloc(RO_VERTS * sizeof *g_ro.v))) return 0;
    ID3DBlob *code = d3d_compile(k_ro_shader, sizeof k_ro_shader - 1, "vs", "vs_4_0");
    HRESULT hr = code ? ID3D11Device_CreateVertexShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_ro.vs) : E_FAIL;
    static const D3D11_INPUT_ELEMENT_DESC el[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateInputLayout(g_d3d.dev, el, 3, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), &g_ro.inputs);
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_ro_shader, sizeof k_ro_shader - 1, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_ro.ps) : E_FAIL;
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_ro_shader, sizeof k_ro_shader - 1, "pst", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_ro.pst) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_SAMPLER_DESC sd = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
                              0, 1, D3D11_COMPARISON_NEVER, { 0, 0, 0, 0 }, 0, D3D11_FLOAT32_MAX };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(g_d3d.dev, &sd, &g_ro.samp);
    D3D11_BUFFER_DESC bd = { 80, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_ro.cb);
    D3D11_BUFFER_DESC ud = { RO_VERTS * sizeof(ro_vertex), D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &ud, NULL, &g_ro.vb);
    if (FAILED(hr)) return rd_fail("the overlay", hr), 0;
    g_ro.ready = 1;
    return 1;
}

// the ground's height at a point (tiles): bilinear between the tile centres, water at 0
internal float ro_tile_h(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= g_ro.size) x = g_ro.size - 1;
    if (y >= g_ro.size) y = g_ro.size - 1;
    ee_tile *t = g_ro.tiles[y * g_ro.size + x];
    float h = t ? tile_get_height(t) : 0.0f;
    return h > 0 ? h : 0;
}
internal float ro_height(float x, float y) {
    float fx = x - 0.5f, fy = y - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    float ax = fx - (float)ix, ay = fy - (float)iy;
    float h0 = ro_tile_h(ix, iy) * (1 - ax) + ro_tile_h(ix + 1, iy) * ax;
    float h1 = ro_tile_h(ix, iy + 1) * (1 - ax) + ro_tile_h(ix + 1, iy + 1) * ax;
    return h0 * (1 - ay) + h1 * ay + 0.06f;   // a little above: no fight with the ground's own depth
}
internal void ro_vtx(float x, float y, uint32_t argb, float u, float v) {
    ro_vertex *o = &g_ro.v[g_ro.n++];
    o->x = x; o->y = y; o->z = ro_height(x, y); o->argb = argb; o->u = u; o->v = v;
}
internal void ro_quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t argb) {
    if (g_ro.n + 6 > RO_VERTS) return;
    float px[4] = { x0, x1, x2, x3 }, py[4] = { y0, y1, y2, y3 };
    static const int order[6] = { 0, 1, 2, 0, 2, 3 };
    for (int k = 0; k < 6; k++) ro_vtx(px[order[k]], py[order[k]], argb, 0, 0);
}
// a ribbon along a -> b, w tiles wide, in pieces of half a tile (on the ground between them)
internal void ro_ribbon(float ax, float ay, float bx, float by, float w, uint32_t argb) {
    float dx = bx - ax, dy = by - ay, len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-4f) return;
    float nx = -dy / len * w * 0.5f, ny = dx / len * w * 0.5f;
    int pieces = (int)(len / 0.5f) + 1;
    for (int i = 0; i < pieces; i++) {
        float t0 = (float)i / (float)pieces, t1 = (float)(i + 1) / (float)pieces;
        float x0 = ax + dx * t0, y0 = ay + dy * t0, x1 = ax + dx * t1, y1 = ay + dy * t1;
        ro_quad(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, argb);
    }
}
internal void ro_circle(float x, float y, float r, float w, uint32_t argb) {
    int seg = (int)(r * 6.0f);
    seg = seg < 16 ? 16 : seg > 96 ? 96 : seg;
    for (int k = 0; k < seg; k++) {
        float a0 = 6.2831853f * (float)k / (float)seg, a1 = 6.2831853f * (float)(k + 1) / (float)seg;
        ro_ribbon(x + r * cosf(a0), y + r * sinf(a0), x + r * cosf(a1), y + r * sinf(a1), w, argb);
    }
}

// THE GAME'S COMMAND CIRCLE (the player, 2026-11-11: "can you use the games own circle texture?"): an OV_RING is drawn
// as the game draws a command's circle (EEE3DCommandConfirmation::vf4) - its texture (ui.circle_texture: a white ring
// lit from one side) tinted by the shape's colour, on a square that the game turns with the camera, so the ring's light
// stays where it is on the screen: here the square's sides along the screen's right and up on the ground at its
// middle, turned by RO_RING_TURN. In cells of about RO_RING_CELL tiles, each corner on the ground's height. Without the
// texture (another renderer's surface, none loaded) a plain circle of the ring's middle.
#define RO_RING_MID 0.88f    // the texture's ring between 0.79 and 0.97 of the square's half (its alpha): the middle
#define RO_RING_WIDTH 0.07f  // the plain circle's width without it
#define RO_RING_CELL 0.5f
#define RO_RING_CELLS 16
#define RO_RING_TURN 0.0f    // radians, from the screen's up
// the screen's right and up on the ground at (x, y) by the view x projection vp (unit vectors, at right angles; 0: none)
internal int ro_screen_axes(const float *vp, float x, float y, float *right, float *up) {
    float z = ro_height(x, y), s[3][2];
    const float d[3][2] = { { 0, 0 }, { 0.5f, 0 }, { 0, 0.5f } };
    for (int k = 0; k < 3; k++) {
        float px = x + d[k][0], py = y + d[k][1];
        float cx = px * vp[0] + py * vp[4] + z * vp[8] + vp[12], cy = px * vp[1] + py * vp[5] + z * vp[9] + vp[13];
        float cw = px * vp[3] + py * vp[7] + z * vp[11] + vp[15];
        if (!(cw > 1e-6f)) return 0;
        s[k][0] = cx / cw; s[k][1] = cy / cw;
    }
    // the screen's change for a step along x (a) and along y (b); the ground's step that goes straight up: J^-1 (0, 1)
    float ax = s[1][0] - s[0][0], ay = s[1][1] - s[0][1], bx = s[2][0] - s[0][0], by = s[2][1] - s[0][1];
    float det = ax * by - bx * ay;
    if (fabsf(det) < 1e-12f) return 0;
    float ux = -bx / det, uy = ax / det, n = sqrtf(ux * ux + uy * uy);
    if (!(n > 0)) return 0;
    ux /= n; uy /= n;
    float rx = uy, ry = -ux;   // at right angles, the way that goes right on the screen
    if ((ax * rx + bx * ry) < 0) rx = -rx, ry = -ry;
    float c = cosf(RO_RING_TURN), sn = sinf(RO_RING_TURN);
    right[0] = rx * c + ux * sn; right[1] = ry * c + uy * sn;
    up[0] = ux * c - rx * sn; up[1] = uy * c - ry * sn;
    return 1;
}
internal void ro_ring(const float *vp, float x, float y, float r, uint32_t argb) {
    float rt[2], up[2], h = r / RO_RING_MID;
    if (!ro_screen_axes(vp, x, y, rt, up)) return;
    int n = (int)ceilf(2.0f * h / RO_RING_CELL);
    n = n < 1 ? 1 : n > RO_RING_CELLS ? RO_RING_CELLS : n;
    if (g_ro.n + n * n * 6 > RO_VERTS) return;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            static const int ci[6] = { 0, 1, 1, 0, 1, 0 }, cj[6] = { 0, 0, 1, 0, 1, 1 };
            for (int k = 0; k < 6; k++) {
                float u = (float)(i + ci[k]) / (float)n, v = (float)(j + cj[k]) / (float)n;
                float a = h * (2 * u - 1), b = h * (1 - 2 * v);   // v 0: the picture's top, the screen's up
                ro_vtx(x + rt[0] * a + up[0] * b, y + rt[1] * a + up[1] * b, argb, u, v);
            }
        }
}

// the overlay into the target (the map's view of this frame), once
internal void rd3_overlay(rd_device *d) {
    g_rd3.overlay_done = 1;
    ee_map *map = game_map();
    if (!g_ov.on || !g_rd3.scene_seen || !map || !d->target || !d->target->rtv) return;
    int n = overlay_copy(g_ro.shapes, OV_SHAPES, game_time());
    if (!n || !ro_init()) return;
    g_ro.tiles = map_get_tiles(map);
    g_ro.size = (int)map_get_size(map);
    if (!g_ro.tiles || g_ro.size <= 0) return;
    // the camera now, as the map's own draws have it (render_frame.c rf_shift: this frame's depths, then V_now V^-1 before
    // view x projection) - with the map's view alone, the last camera step's, the lines lagged up to a step's scrolling
    // behind the ground under them and jumped back at each step (the player, 2026-11-11: "the line jitter when i move the
    // camera")
    struct { float vp[16], px[4]; } c;
    memcpy(c.vp, g_rd3.scene_vp, sizeof c.vp);
    if (g_cam_now_ok) {
        float inv[16], to_now[16], vp[16];
        rf_depth_vpm(c.vp);
        rd3_rigid_inverse(inv, g_rd3.scene_view);
        rd3_mul(to_now, g_cam_view_now, inv);
        rd3_mul(vp, to_now, c.vp);
        memcpy(c.vp, vp, sizeof vp);
    }
    // the rings' texture: the own renderer's surface of the game's (rd_is_surface: not another renderer's)
    void *dds = circle_texture_dds();
    ID3D11ShaderResourceView *ring = rd_is_surface(dds) ? rd_gpu_texture((rd_surface *)dds, 0) : NULL;
    g_ro.n = 0;
    for (int i = 0; i < n; i++) {
        const ov_shape *s = &g_ro.shapes[i];
        if (s->kind == OV_LINE) ro_ribbon(s->a[0], s->a[1], s->a[2], s->a[3], s->width, s->argb);
        else if (s->kind == OV_CIRCLE) ro_circle(s->a[0], s->a[1], s->a[2], s->width, s->argb);
        else if (s->kind == OV_RING) { if (!ring) ro_circle(s->a[0], s->a[1], s->a[2] * RO_RING_MID, RO_RING_WIDTH, s->argb); }
        else {   // a run of tiles: a quad per tile, on each one's own heights
            float y = s->a[1];
            for (int k = 0; k < (int)s->a[2]; k++) {
                float x = s->a[0] + (float)k;
                ro_quad(x, y, x + 1, y, x + 1, y + 1, x, y + 1, s->argb);
            }
        }
    }
    int plain = g_ro.n;
    if (ring)
        for (int i = 0; i < n; i++)
            if (g_ro.shapes[i].kind == OV_RING) ro_ring(c.vp, g_ro.shapes[i].a[0], g_ro.shapes[i].a[1], g_ro.shapes[i].a[2], g_ro.shapes[i].argb);
    if (!g_ro.n) return;
    rd3_submit();
    ID3D11DeviceContext *x = g_d3d.ctx;
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ID3D11DeviceContext_Map(x, (ID3D11Resource *)g_ro.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
    memcpy(ms.pData, g_ro.v, (size_t)g_ro.n * sizeof *g_ro.v);
    ID3D11DeviceContext_Unmap(x, (ID3D11Resource *)g_ro.vb, 0);
    D3D11_VIEWPORT port = rs_port(&g_rd3.scene_port, d->target->rtv);
    c.px[0] = port.Width > 0 ? 1.0f / port.Width : 0;   // Direct3D 7's pixel centres, as the map's
    c.px[1] = port.Height > 0 ? -1.0f / port.Height : 0;
    c.px[2] = c.px[3] = 0;
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_ro.cb, 0, NULL, &c, 0, 0);
    ID3D11DepthStencilView *dsv = d->target->zbuf ? d->target->zbuf->dsv : NULL;
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &d->target->rtv, dsv);
    ID3D11DeviceContext_RSSetViewports(x, 1, &port);
    // alpha blended (SRCALPHA / INVSRCALPHA), depth tested LESSEQUAL and not written, no culling
    ID3D11DeviceContext_OMSetBlendState(x, rd3_cache(g_rd3.blends, &g_rd3.n_blend, 32, 1 | D3DBLEND_SRCALPHA << 1 | D3DBLEND_INVSRCALPHA << 5, rd3_mk_blend), NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, rd3_cache(g_rd3.depths, &g_rd3.n_depth, 32, 1 | D3DCMP_LESSEQUAL << 2, rd3_mk_depth), 0);
    ID3D11DeviceContext_RSSetState(x, rd3_cache(g_rd3.rasters, &g_rd3.n_raster, 8, D3DCULL_NONE, rd3_mk_raster));
    ID3D11DeviceContext_IASetInputLayout(x, g_ro.inputs);
    UINT stride = sizeof(ro_vertex), off = 0;
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_ro.vb, &stride, &off);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, g_ro.vs, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 0, 1, &g_ro.cb);
    if (plain) {
        ID3D11DeviceContext_PSSetShader(x, g_ro.ps, NULL, 0);
        ID3D11DeviceContext_Draw(x, (UINT)plain, 0);
    }
    if (g_ro.n > plain) {
        ID3D11DeviceContext_PSSetShader(x, g_ro.pst, NULL, 0);
        ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &ring);
        ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_ro.samp);
        ID3D11DeviceContext_Draw(x, (UINT)(g_ro.n - plain), (UINT)plain);
    }
    g_rd3.bound = 0;   // the game's next draw binds all its states again (its texture, its sampler)
}
// before a draw: the map is done when the game draws without the depth test after it
internal void rd3_overlay_before(rd_device *d) {
    if (g_rd3.scene_seen && !g_rd3.overlay_done && !d->rs[D3DRENDERSTATE_ZENABLE]) rd3_overlay(d);
}
// at the scene's end: drawn if the interface came without such a draw; the next frame looks again
internal void rd3_overlay_frame_end(rd_device *d) {
    if (g_rd3.scene_seen && !g_rd3.overlay_done) rd3_overlay(d);
    g_rd3.scene_seen = g_rd3.overlay_done = 0;
}
