// The debug overlay's drawing ([display] overlay; base/overlay.c keeps the shapes): once a frame, where the game
// turns from the map to the interface (its first draw without the depth test after the map's camera, else at the
// scene's end), the shapes go into the map's view with that camera - lines and circles as ribbons on the ground
// (the tiles' heights, bilinear between their centres, a little above), tiles as quads; depth tested so hills and
// buildings hide them, not written; blended by their alpha; under the interface drawn after them.

#define RO_VERTS 131072   // 2 MB of 16-byte vertices: about 21000 ribbon quads a frame
typedef struct { float x, y, z; uint32_t argb; } ro_vertex;
internal const char k_ro_shader[] =
    "cbuffer o : register(b0) { row_major float4x4 vp; float4 px; };\n"
    "struct I { float3 p : POSITION; float4 c : COLOR; };\n"
    "struct O { float4 p : SV_Position; float4 c : COLOR; };\n"
    "O vs(I i) { O o; o.p = mul(float4(i.p, 1), vp); o.p.xy += px.xy * o.p.w; o.c = i.c.zyxw; return o; }\n"   // ARGB's bytes: B G R A
    "float4 ps(O i) : SV_Target { return i.c; }\n";
internal struct {
    int ready;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11InputLayout *inputs;
    ID3D11Buffer *cb, *vb;
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
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateInputLayout(g_d3d.dev, el, 2, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), &g_ro.inputs);
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_ro_shader, sizeof k_ro_shader - 1, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_ro.ps) : E_FAIL;
    D3D_RELEASE(code);
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
internal void ro_quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t argb) {
    if (g_ro.n + 6 > RO_VERTS) return;
    float px[4] = { x0, x1, x2, x3 }, py[4] = { y0, y1, y2, y3 };
    static const int order[6] = { 0, 1, 2, 0, 2, 3 };
    for (int k = 0; k < 6; k++) {
        ro_vertex *v = &g_ro.v[g_ro.n++];
        v->x = px[order[k]]; v->y = py[order[k]]; v->z = ro_height(v->x, v->y); v->argb = argb;
    }
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
    g_ro.n = 0;
    for (int i = 0; i < n; i++) {
        const ov_shape *s = &g_ro.shapes[i];
        if (s->kind == OV_LINE) ro_ribbon(s->a[0], s->a[1], s->a[2], s->a[3], s->width, s->argb);
        else if (s->kind == OV_CIRCLE) {
            int seg = (int)(s->a[2] * 6.0f);
            seg = seg < 16 ? 16 : seg > 96 ? 96 : seg;
            for (int k = 0; k < seg; k++) {
                float a0 = 6.2831853f * (float)k / (float)seg, a1 = 6.2831853f * (float)(k + 1) / (float)seg;
                ro_ribbon(s->a[0] + s->a[2] * cosf(a0), s->a[1] + s->a[2] * sinf(a0),
                          s->a[0] + s->a[2] * cosf(a1), s->a[1] + s->a[2] * sinf(a1), s->width, s->argb);
            }
        } else {   // a run of tiles: a quad per tile, on each one's own heights
            float y = s->a[1];
            for (int k = 0; k < (int)s->a[2]; k++) {
                float x = s->a[0] + (float)k;
                ro_quad(x, y, x + 1, y, x + 1, y + 1, x, y + 1, s->argb);
            }
        }
    }
    if (!g_ro.n) return;
    ID3D11DeviceContext *x = g_d3d.ctx;
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ID3D11DeviceContext_Map(x, (ID3D11Resource *)g_ro.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
    memcpy(ms.pData, g_ro.v, (size_t)g_ro.n * sizeof *g_ro.v);
    ID3D11DeviceContext_Unmap(x, (ID3D11Resource *)g_ro.vb, 0);
    struct { float vp[16], px[4]; } c;
    memcpy(c.vp, g_rd3.scene_vp, sizeof c.vp);
    c.px[0] = g_rd3.scene_port.Width > 0 ? 1.0f / g_rd3.scene_port.Width : 0;   // Direct3D 7's pixel centres, as the map's
    c.px[1] = g_rd3.scene_port.Height > 0 ? -1.0f / g_rd3.scene_port.Height : 0;
    c.px[2] = c.px[3] = 0;
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_ro.cb, 0, NULL, &c, 0, 0);
    ID3D11DepthStencilView *dsv = d->target->zbuf ? d->target->zbuf->dsv : NULL;
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &d->target->rtv, dsv);
    ID3D11DeviceContext_RSSetViewports(x, 1, &g_rd3.scene_port);
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
    ID3D11DeviceContext_PSSetShader(x, g_ro.ps, NULL, 0);
    ID3D11DeviceContext_Draw(x, (UINT)g_ro.n, 0);
    rd3_dirty();
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
