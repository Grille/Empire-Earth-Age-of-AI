// Windowed present through Direct3D 11 (ageofai.ini [display] present = d3d, the default; gdi = the CPU path).
// The game's finished frame (the locked back buffer of its DirectDraw device) is uploaded to a texture and drawn
// into the window by a flip-model swap chain: scaling and filtering on the GPU, no GDI composition (which held
// a visible 2560x1440 window at 60 fps on a 120 Hz screen). Bars around the image are cleared black.
// vsync = 1 (default since 2026-11-18) waits for the screen; 0 presents immediately, tearing allowed where Windows supports it
// (borderless). Everything runs on the render thread; any failure falls back to GDI for good (the own renderer
// has no GDI path: d3d_fail_window).
// d3d11.dll and d3dcompiler_47.dll are loaded from the system directory (the game folder may hold wrappers).
// The device lives on its own (d3d_device) apart from the window's swap chain (d3d_window): the mod's own renderer
// (render.c, [display] renderer = own) draws with the same device before there is a window and shows its render
// target with d3d_draw_srv - no read-back of the frame.
#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_5.h>

typedef HRESULT (WINAPI *D3D11CreateDeviceFn)(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *,
                                              UINT, UINT, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
typedef HRESULT (WINAPI *D3DCompileFn)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *, ID3DInclude *, LPCSTR, LPCSTR,
                                       UINT, UINT, ID3DBlob **, ID3DBlob **);

internal int g_present_d3d = 1, g_vsync;
internal struct {
    int failed, tearing, bw, bh, tw, th;
    DXGI_FORMAT tfmt;
    D3D_FEATURE_LEVEL fl;
    HWND hwnd;
    D3DCompileFn compile;
    ID3D11Device *dev;
    ID3D11DeviceContext *ctx;
    IDXGISwapChain1 *sc;
    ID3D11RenderTargetView *rtv;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11SamplerState *linear, *point;
    ID3D11PixelShader *ps_crt;   // [display] scaling = crt (below); NULL: its shader didn't compile - linear
    ID3D11Buffer *crt_k;
    ID3D11Texture2D *tex;
    ID3D11ShaderResourceView *srv;
    char err[160];
} g_d3d;

#define D3D_RELEASE(p) do { if (p) { ((IUnknown *)(p))->lpVtbl->Release((IUnknown *)(p)); (p) = NULL; } } while (0)

internal void d3d_release_window(void) {
    if (g_d3d.ctx) ID3D11DeviceContext_OMSetRenderTargets(g_d3d.ctx, 0, NULL, NULL);
    D3D_RELEASE(g_d3d.rtv); D3D_RELEASE(g_d3d.sc);
    g_d3d.hwnd = NULL;
    g_d3d.bw = g_d3d.bh = 0;
}

internal void d3d_release(void) {
    if (g_d3d.ctx) ID3D11DeviceContext_ClearState(g_d3d.ctx);
    d3d_release_window();
    D3D_RELEASE(g_d3d.srv); D3D_RELEASE(g_d3d.tex); D3D_RELEASE(g_d3d.linear); D3D_RELEASE(g_d3d.point);
    D3D_RELEASE(g_d3d.vs); D3D_RELEASE(g_d3d.ps); D3D_RELEASE(g_d3d.ps_crt); D3D_RELEASE(g_d3d.crt_k);
    D3D_RELEASE(g_d3d.ctx); D3D_RELEASE(g_d3d.dev);
    g_d3d.tw = g_d3d.th = 0;
}

internal int d3d_fail(const char *what, HRESULT hr) {
    if (hr) snprintf(g_d3d.err, sizeof g_d3d.err, "%s failed (hr %08lx)", what, (unsigned long)hr);
    else snprintf(g_d3d.err, sizeof g_d3d.err, "%s failed", what);
    if (g_log) { log_locked("# display: Direct3D present: %s - %s\n", g_d3d.err, g_rd_on ? "the own renderer has no device" : "using GDI"); }
    d3d_release();
    g_d3d.failed = 1;
    return 0;
}

// a failure of the window's swap chain (making it, resizing it, presenting): with the own renderer (render.c) every
// surface, texture and state of the game lives on this device and nothing of it checks for one - releasing the device
// there left a NULL context for its next draw (a removed device's Present, a TDR, was a crash). Then only the window
// goes; the frames are no longer shown (no GDI path for that renderer), and a removed device's calls fail harmlessly.
internal int d3d_fail_window(const char *what, HRESULT hr) {
    if (!g_rd_on) return d3d_fail(what, hr);
    snprintf(g_d3d.err, sizeof g_d3d.err, "%s failed (hr %08lx)", what, (unsigned long)hr);
    if (g_log) { log_locked("# display: Direct3D present: %s - the own renderer's frames are no longer shown\n", g_d3d.err); }
    d3d_release_window();
    g_d3d.failed = 1;
    return 0;
}

internal const char k_shader[] =
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    "V vs(uint id : SV_VertexID) { V o; o.uv = float2((id << 1) & 2, id & 2);"
    " o.p = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1); return o; }\n"
    "float4 ps(V i) : SV_Target { return float4(t.Sample(s, i.uv).rgb, 1); }\n";

// [display] scaling = crt (the player, 2026-11-24: "silly idea, how about a crt mode?"): the picture as a tube monitor
// showed it, the last step into the window instead of the plain copy - so the game's picture, shots and records stay
// as they are (the pipe's "shot <name> window" reads the window's). It has to show at the screen's own resolution too
// (the player's game is 2560x1440 on such a screen), so nothing of it hangs on the scale but the scan lines' count:
// the game's lines where each gets 3 pixels of the window or more, else a line every 3 pixels over the picture (a
// tube of a third of the window's lines; fewer pixels a line and the gaps beat against the pixel rows).
//   the glass: the picture's place pushed outwards with the square of the other axis' distance from the middle, so
//     its edges bow inwards at the corners and touch the window's at their middles; black beyond, the corners darker
//   a scan line: the beam's spot across the line a bell, wider the brighter its colour (a bright line closes its
//     gaps, a dark one is thin), worked in linear light (the colours squared, the root at the end)
//   the stripes: the tube's red, green and blue phosphor stripes, one of the three on each pixel column of the window
//   the glow: the picture's light spread over two rings round each place and added (the glass scatters it)
// and the brightness the gaps and stripes take is given back by a gain (what was at full brightness clips).
// k: the picture's size and a texel's; the window's view size and the scan lines' count; scan lines, stripes, bulge, glow
internal const char k_crt_shader[] =
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "cbuffer k : register(b0) { float4 ksrc; float4 kout; float4 kp; };\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    "float3 lit(float2 uv) { float3 c = t.SampleLevel(s, uv, 0).rgb; return c * c; }\n"
    "float4 ps(V i) : SV_Target {\n"
    " float2 c = i.uv * 2 - 1, q = c * c;\n"
    " c *= 1 + kp.z * float2(0.07 * q.y, 0.10 * q.x);\n"
    " float2 uv = c * 0.5 + 0.5, e = (1 - abs(c)) * 0.5 * kout.xy;\n"   // e: pixels of the window to the picture's edge
    " float in_glass = saturate(e.x + 0.5) * saturate(e.y + 0.5);\n"
    " float2 px = 1 / kout.xy;\n"
    " float3 col = lit(uv) * 0.5 + (lit(uv + float2(px.x * 0.6, 0)) + lit(uv - float2(px.x * 0.6, 0))) * 0.25;\n"   // the spot's width along the line
    " if (kp.w > 0) {\n"
    "  float3 g = 0;\n"
    "  for (int n = 0; n < 8; n++) {\n"
    "   float a = n * 0.7853982; float2 d = float2(cos(a), sin(a)) * px;\n"
    "   g += lit(uv + d * 3.5) + lit(uv + d.yx * float2(-9, 9)) * 0.6;\n"
    "  }\n"
    "  col += g * (kp.w * 0.35 / 12.8);\n"
    " }\n"
    " float d = frac(uv.y * kout.z) - 0.5;\n"
    " float3 w = lerp(0.24, 0.60, saturate(sqrt(col)));\n"
    " col *= lerp(1, exp(-d * d / (2 * w * w)), kp.x);\n"
    " float3 m = 1 - kp.y; int col3 = (int)i.p.x % 3;\n"
    " if (col3 == 0) m.r = 1; else if (col3 == 1) m.g = 1; else m.b = 1;\n"
    " col *= m * (1 + 0.55 * kp.x) * 3 / (3 - 2 * kp.y);\n"
    " col *= 1 - 0.30 * kp.z * q.x * q.y - 0.10 * kp.z * (q.x + q.y) * 0.5;\n"
    " return float4(sqrt(saturate(col)) * in_glass, 1);\n"
    "}\n";
internal struct { int on; float k[4]; } g_crt;   // display.c sets it from [display] scaling and crt_*; the pipe: picture crt

// the pipe: picture crt [on | off | <scan lines> <stripes> <bulge> <glow>] (each 0..1)
internal int crt_pipe(char *out, const char *mode, const char *args) {
    if (strcmp(mode, "crt")) return 0;
    char arg[16] = "";
    float k[4];
    int n = sscanf(args, "%*s %f %f %f %f", &k[0], &k[1], &k[2], &k[3]);
    sscanf(args, "%*s %15s", arg);
    if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_crt.on = arg[1] == 'n';
    else if (n == 4 && k[0] >= 0 && k[0] <= 1 && k[1] >= 0 && k[1] <= 1 && k[2] >= 0 && k[2] <= 1 && k[3] >= 0 && k[3] <= 1) memcpy(g_crt.k, k, sizeof k);
    else if (arg[0]) { REPLY("error: usage: picture crt [on | off | <scan lines> <stripes> <bulge> <glow>] (each 0..1)\n"); return 1; }
    REPLY("crt %s%s: scan lines %.2f, stripes %.2f, bulge %.2f, glow %.2f\n", g_crt.on ? "on" : "off",
          g_crt.on && g_d3d.dev && !g_d3d.ps_crt ? " (no shader: linear)" : "", (double)g_crt.k[0], (double)g_crt.k[1], (double)g_crt.k[2], (double)g_crt.k[3]);
    return 1;
}

// a shader of src's entry point for target (vs_4_0, ps_4_0); NULL after logging the compiler's message
internal ID3DBlob *d3d_compile(const char *src, size_t n, const char *entry, const char *target) {
    ID3DBlob *code = NULL, *errs = NULL;
    HRESULT hr = g_d3d.compile(src, n, "aimod", NULL, NULL, entry, target, 0, 0, &code, &errs);
    if (FAILED(hr) && g_log)
        log_locked("# display: shader %s: %s\n", entry, errs ? (const char *)ID3D10Blob_GetBufferPointer(errs) : "failed");
    D3D_RELEASE(errs);
    return SUCCEEDED(hr) ? code : NULL;
}

internal int d3d_rtv(void) {
    ID3D11Texture2D *bb = NULL;
    HRESULT hr = IDXGISwapChain1_GetBuffer(g_d3d.sc, 0, &IID_ID3D11Texture2D, (void **)&bb);
    if (FAILED(hr)) return d3d_fail_window("GetBuffer", hr);
    hr = ID3D11Device_CreateRenderTargetView(g_d3d.dev, (ID3D11Resource *)bb, NULL, &g_d3d.rtv);
    ID3D11Texture2D_Release(bb);
    return FAILED(hr) ? d3d_fail_window("CreateRenderTargetView", hr) : 1;
}

// the device, its context and the present's shaders and samplers
internal int d3d_device(void) {
    if (g_d3d.dev) return 1;
    if (g_d3d.failed) return 0;
    char path[MAX_PATH];
    GetSystemDirectoryA(path, sizeof path);
    lstrcatA(path, "\\d3d11.dll");
    HMODULE m = LoadLibraryA(path);
    GetSystemDirectoryA(path, sizeof path);
    lstrcatA(path, "\\d3dcompiler_47.dll");
    HMODULE mc = LoadLibraryA(path);
    D3D11CreateDeviceFn create = m ? (D3D11CreateDeviceFn)(void *)GetProcAddress(m, "D3D11CreateDevice") : NULL;
    g_d3d.compile = mc ? (D3DCompileFn)(void *)GetProcAddress(mc, "D3DCompile") : NULL;
    if (!create || !g_d3d.compile) return d3d_fail("loading d3d11.dll / d3dcompiler_47.dll", 0);
    static const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = create(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, 3, D3D11_SDK_VERSION,
                        &g_d3d.dev, &g_d3d.fl, &g_d3d.ctx);
    if (FAILED(hr)) return d3d_fail("D3D11CreateDevice", hr);
    ID3DBlob *code = d3d_compile(k_shader, sizeof k_shader - 1, "vs", "vs_4_0");
    hr = code ? ID3D11Device_CreateVertexShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_d3d.vs) : E_FAIL;
    D3D_RELEASE(code);
    if (FAILED(hr)) return d3d_fail("vertex shader", hr);
    code = d3d_compile(k_shader, sizeof k_shader - 1, "ps", "ps_4_0");
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_d3d.ps) : E_FAIL;
    D3D_RELEASE(code);
    if (FAILED(hr)) return d3d_fail("pixel shader", hr);
    code = d3d_compile(k_crt_shader, sizeof k_crt_shader - 1, "ps", "ps_4_0");   // without it the plain copy: no failure
    D3D11_BUFFER_DESC kd = { 48, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (code && (FAILED(ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_d3d.ps_crt)) ||
                 FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &kd, NULL, &g_d3d.crt_k)))) D3D_RELEASE(g_d3d.ps_crt);
    D3D_RELEASE(code);
    D3D11_SAMPLER_DESC smp = { 0 };
    smp.AddressU = smp.AddressV = smp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    smp.MaxLOD = D3D11_FLOAT32_MAX;
    smp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    hr = ID3D11Device_CreateSamplerState(g_d3d.dev, &smp, &g_d3d.linear);
    smp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(g_d3d.dev, &smp, &g_d3d.point);
    if (FAILED(hr)) return d3d_fail("CreateSamplerState", hr);
    return 1;
}

// the window's flip-model swap chain (the device first)
internal int d3d_window(HWND hwnd, int w, int h) {
    if (!d3d_device()) return 0;
    d3d_release_window();
    IDXGIDevice *xd = NULL;
    IDXGIAdapter *ad = NULL;
    IDXGIFactory2 *f2 = NULL;
    HRESULT hr = ID3D11Device_QueryInterface(g_d3d.dev, &IID_IDXGIDevice, (void **)&xd);
    if (SUCCEEDED(hr)) hr = IDXGIDevice_GetAdapter(xd, &ad);
    if (SUCCEEDED(hr)) hr = IDXGIAdapter_GetParent(ad, &IID_IDXGIFactory2, (void **)&f2);
    D3D_RELEASE(ad); D3D_RELEASE(xd);
    if (FAILED(hr)) return d3d_fail_window("getting the DXGI factory", hr);
    IDXGIFactory5 *f5 = NULL;
    BOOL tear = FALSE;
    if (SUCCEEDED(IDXGIFactory2_QueryInterface(f2, &IID_IDXGIFactory5, (void **)&f5))) {
        if (FAILED(IDXGIFactory5_CheckFeatureSupport(f5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tear, sizeof tear))) tear = FALSE;
        IDXGIFactory5_Release(f5);
    }
    g_d3d.tearing = tear != FALSE;
    DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
    sd.Width = (UINT)w; sd.Height = (UINT)h;
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sd.Flags = g_d3d.tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    hr = IDXGIFactory2_CreateSwapChainForHwnd(f2, (IUnknown *)g_d3d.dev, hwnd, &sd, NULL, NULL, &g_d3d.sc);
    if (SUCCEEDED(hr)) IDXGIFactory2_MakeWindowAssociation(f2, hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    IDXGIFactory2_Release(f2);
    if (FAILED(hr)) return d3d_fail_window("CreateSwapChainForHwnd", hr);
    if (!d3d_rtv()) return 0;
    g_d3d.hwnd = hwnd;
    g_d3d.bw = w; g_d3d.bh = h;
    if (g_log) { log_locked("# display: Direct3D 11 present (feature level %x, tearing %s)\n", (unsigned)g_d3d.fl, g_d3d.tearing ? "yes" : "no"); }
    return 1;
}

// the swap chain for this window at its client size; 0: no Direct3D present
internal int d3d_window_ready(HWND hwnd, const RECT *cr) {
    if (!g_present_d3d || g_d3d.failed || cr->right < 1 || cr->bottom < 1) return 0;
    if (g_d3d.hwnd != hwnd && !d3d_window(hwnd, cr->right, cr->bottom)) return 0;
    if (g_d3d.bw != cr->right || g_d3d.bh != cr->bottom) {   // window size changed
        ID3D11DeviceContext_OMSetRenderTargets(g_d3d.ctx, 0, NULL, NULL);
        D3D_RELEASE(g_d3d.rtv);
        HRESULT hr = IDXGISwapChain1_ResizeBuffers(g_d3d.sc, 0, (UINT)cr->right, (UINT)cr->bottom, DXGI_FORMAT_UNKNOWN,
                                                   g_d3d.tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
        if (FAILED(hr)) return d3d_fail_window("ResizeBuffers", hr);
        if (!d3d_rtv()) return 0;
        g_d3d.bw = cr->right; g_d3d.bh = cr->bottom;
    }
    return 1;
}

// draws a frame of sw x sh (its texture's view) into the view rect of the window and presents
internal LONGLONG g_d3d_present_qpc;   // time inside Present
internal int g_present_no_wait;   // this frame doesn't wait for the screen, vsync or not (render.c render_present)
internal int d3d_draw_srv(ID3D11ShaderResourceView *srv, int sw, int sh, const RECT *cr, const RECT *v) {
    ID3D11DeviceContext *c = g_d3d.ctx;
    if (v->left > 0 || v->top > 0 || v->right < cr->right || v->bottom < cr->bottom) {
        static const float black[4] = { 0, 0, 0, 1 };
        ID3D11DeviceContext_ClearRenderTargetView(c, g_d3d.rtv, black);
    }
    D3D11_VIEWPORT vp = { (float)v->left, (float)v->top, (float)(v->right - v->left), (float)(v->bottom - v->top), 0.0f, 1.0f };
    int scaled = v->right - v->left != sw || v->bottom - v->top != sh;
    ID3D11DeviceContext_OMSetRenderTargets(c, 1, &g_d3d.rtv, NULL);
    ID3D11DeviceContext_OMSetBlendState(c, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(c, NULL, 0);
    ID3D11DeviceContext_RSSetState(c, NULL);
    ID3D11DeviceContext_RSSetViewports(c, 1, &vp);
    ID3D11DeviceContext_IASetInputLayout(c, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(c, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(c, g_d3d.vs, NULL, 0);
    int crt = g_crt.on && g_d3d.ps_crt;
    if (crt) {
        float vw = vp.Width, vh = vp.Height, lines = (float)sh < vh / 3 ? (float)sh : floorf(vh / 3);
        float k[12] = { (float)sw, (float)sh, 1.0f / (float)sw, 1.0f / (float)sh, vw, vh, lines, 0, g_crt.k[0], g_crt.k[1], g_crt.k[2], g_crt.k[3] };
        ID3D11DeviceContext_UpdateSubresource(c, (ID3D11Resource *)g_d3d.crt_k, 0, NULL, k, 0, 0);
        ID3D11DeviceContext_PSSetConstantBuffers(c, 0, 1, &g_d3d.crt_k);
    }
    ID3D11DeviceContext_PSSetShader(c, crt ? g_d3d.ps_crt : g_d3d.ps, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 1, &srv);
    ID3D11DeviceContext_PSSetSamplers(c, 0, 1, crt || (scaled && g_smooth) ? &g_d3d.linear : &g_d3d.point);
    ID3D11DeviceContext_Draw(c, 3, 0);
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 1, &none);   // the frame may be a render target again next
    if (g_shot_req == 2) {   // the pipe's "shot <name> window": what the window gets - scaled, bars, crt
        ID3D11Texture2D *bb = NULL, *st = NULL;
        D3D11_TEXTURE2D_DESC td = { (UINT)g_d3d.bw, (UINT)g_d3d.bh, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, { 1, 0 }, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
        D3D11_MAPPED_SUBRESOURCE ms;
        if (SUCCEEDED(IDXGISwapChain1_GetBuffer(g_d3d.sc, 0, &IID_ID3D11Texture2D, (void **)&bb)) &&
            SUCCEEDED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &st))) {
            ID3D11DeviceContext_CopyResource(c, (ID3D11Resource *)st, (ID3D11Resource *)bb);
            if (SUCCEEDED(ID3D11DeviceContext_Map(c, (ID3D11Resource *)st, 0, D3D11_MAP_READ, 0, &ms))) {
                DDSURFACEDESC2 d = { .dwSize = sizeof d, .dwWidth = td.Width, .dwHeight = td.Height, .lPitch = (LONG)ms.RowPitch, .lpSurface = ms.pData };
                d.ddpfPixelFormat.dwRGBBitCount = 32;
                d.ddpfPixelFormat.dwRBitMask = 0xFF0000, d.ddpfPixelFormat.dwGBitMask = 0xFF00, d.ddpfPixelFormat.dwBBitMask = 0xFF;
                write_desc_bmp(&d, g_shot_path);
                ID3D11DeviceContext_Unmap(c, (ID3D11Resource *)st, 0);
            }
        }
        D3D_RELEASE(st); D3D_RELEASE(bb);
        g_shot_req = 0;
        InterlockedIncrement(&g_shot_done);
    }
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    int sync = g_vsync && !g_present_no_wait;
    HRESULT hr = IDXGISwapChain1_Present(g_d3d.sc, sync ? 1 : 0, !sync && g_d3d.tearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
    QueryPerformanceCounter(&t1);
    g_d3d_present_qpc += t1.QuadPart - t0.QuadPart;   // the own renderer's minute line (render_3d.c): the wait for the GPU shows here
    if (FAILED(hr)) return d3d_fail_window("Present", hr);
    if (g_ct_left) {   // camera.c camtrace: this present's times, and the last one the screen showed
        DXGI_FRAME_STATISTICS st;
        g_ct_p0 = t0.QuadPart; g_ct_p1 = t1.QuadPart;
        if (FAILED(IDXGISwapChain1_GetLastPresentCount(g_d3d.sc, &g_ct_pc))) g_ct_pc = 0;
        if (SUCCEEDED(IDXGISwapChain1_GetFrameStatistics(g_d3d.sc, &st))) {
            g_ct_st_pc = st.PresentCount; g_ct_st_sync = st.SyncRefreshCount; g_ct_st_qpc = st.SyncQPCTime.QuadPart;
        } else g_ct_st_pc = g_ct_st_sync = 0, g_ct_st_qpc = 0;
    }
    return 1;
}

// draws the locked frame into the window; 0 = not done (GDI draws it)
internal int present_d3d(const DDSURFACEDESC2 *d, HWND hwnd, const RECT *cr, const RECT *v) {
    if (!d3d_window_ready(hwnd, cr)) return 0;
    int sw = (int)d->dwWidth, sh = (int)d->dwHeight, bpp = (int)d->ddpfPixelFormat.dwRGBBitCount;
    DXGI_FORMAT fmt = bpp == 32 && d->ddpfPixelFormat.dwRBitMask == 0xFF0000 ? DXGI_FORMAT_B8G8R8X8_UNORM
                    : bpp == 16 && d->ddpfPixelFormat.dwRBitMask == 0xF800 ? DXGI_FORMAT_B5G6R5_UNORM : DXGI_FORMAT_UNKNOWN;
    if (fmt == DXGI_FORMAT_UNKNOWN) return 0;   // unusual pixel format: GDI
    if (g_d3d.tw != sw || g_d3d.th != sh || g_d3d.tfmt != fmt) {
        D3D_RELEASE(g_d3d.srv); D3D_RELEASE(g_d3d.tex);
        D3D11_TEXTURE2D_DESC td = { 0 };
        td.Width = (UINT)sw; td.Height = (UINT)sh; td.MipLevels = 1; td.ArraySize = 1; td.Format = fmt;
        td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &g_d3d.tex);
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_d3d.tex, NULL, &g_d3d.srv);
        if (FAILED(hr)) return d3d_fail("frame texture", hr);
        g_d3d.tw = sw; g_d3d.th = sh; g_d3d.tfmt = fmt;
    }
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_d3d.tex, 0, NULL, d->lpSurface, (UINT)d->lPitch, 0);
    return d3d_draw_srv(g_d3d.srv, sw, sh, cr, v);
}
