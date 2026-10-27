// Windowed present through Direct3D 11 (ageofai.ini [display] present = d3d, the default; gdi = the CPU path).
// The game's finished frame (the locked back buffer of its DirectDraw device) is uploaded to a texture and drawn
// into the window by a flip-model swap chain: scaling and filtering on the GPU, no GDI composition (which held
// a visible 2560x1440 window at 60 fps on a 120 Hz screen). Bars around the image are cleared black.
// vsync = 1 waits for the screen; 0 (default) presents immediately, tearing allowed where Windows supports it
// (borderless). Everything runs on the render thread; any failure falls back to GDI for good.
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
    D3D_RELEASE(g_d3d.vs); D3D_RELEASE(g_d3d.ps);
    D3D_RELEASE(g_d3d.ctx); D3D_RELEASE(g_d3d.dev);
    g_d3d.tw = g_d3d.th = 0;
}

internal int d3d_fail(const char *what, HRESULT hr) {
    snprintf(g_d3d.err, sizeof g_d3d.err, "%s failed (hr %08lx)", what, (unsigned long)hr);
    if (g_log) { log_locked("# display: Direct3D present: %s - using GDI\n", g_d3d.err); }
    d3d_release();
    g_d3d.failed = 1;
    return 0;
}

internal const char k_shader[] =
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    "V vs(uint id : SV_VertexID) { V o; o.uv = float2((id << 1) & 2, id & 2);"
    " o.p = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1); return o; }\n"
    "float4 ps(V i) : SV_Target { return float4(t.Sample(s, i.uv).rgb, 1); }\n";

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
    if (FAILED(hr)) return d3d_fail("GetBuffer", hr);
    hr = ID3D11Device_CreateRenderTargetView(g_d3d.dev, (ID3D11Resource *)bb, NULL, &g_d3d.rtv);
    ID3D11Texture2D_Release(bb);
    return FAILED(hr) ? d3d_fail("CreateRenderTargetView", hr) : 1;
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
    if (FAILED(hr)) return d3d_fail("getting the DXGI factory", hr);
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
    if (FAILED(hr)) return d3d_fail("CreateSwapChainForHwnd", hr);
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
        if (FAILED(hr)) return d3d_fail("ResizeBuffers", hr);
        if (!d3d_rtv()) return 0;
        g_d3d.bw = cr->right; g_d3d.bh = cr->bottom;
    }
    return 1;
}

// draws a frame of sw x sh (its texture's view) into the view rect of the window and presents
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
    ID3D11DeviceContext_PSSetShader(c, g_d3d.ps, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 1, &srv);
    ID3D11DeviceContext_PSSetSamplers(c, 0, 1, scaled && g_smooth ? &g_d3d.linear : &g_d3d.point);
    ID3D11DeviceContext_Draw(c, 3, 0);
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 1, &none);   // the frame may be a render target again next
    HRESULT hr = IDXGISwapChain1_Present(g_d3d.sc, g_vsync ? 1 : 0, !g_vsync && g_d3d.tearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
    if (FAILED(hr)) return d3d_fail("Present", hr);
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
