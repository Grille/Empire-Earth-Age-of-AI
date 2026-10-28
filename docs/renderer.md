# The mod's own renderer: the DirectX 7 the game uses

The plan (the player, 2026-10-27): the mod implements the DirectDraw 7 / Direct3D 7 calls the game makes itself, on
Direct3D 11, instead of a wrapper's whole DirectX 7 (DDrawCompat: screenshots, recording and the game's warning dialogs
fail - its own presentation window covers the game's; GOG's wrapper, dgVoodoo: half the frame rate, intro videos fail,
the ground decals z-fight). Owning the renderer is also where the frame time can be won: the game's cost is per call in
the DirectX 7 runtime and the driver (docs/testing.md, the 2026-10-11 profile).

## What the game calls

Measured by the probe (autotest `dx_probe=1`, dll/test/dxprobe.c) on 2026-10-27 with GOG's wrapper: a 10 and a
20-minute game (Continental 101), the intro videos and main menu, the scenario editor. The two display DLLs
(DX7HRDisplay.dll, DX7HRTnLDisplay.dll) are the only importers of ddraw.dll (`DirectDrawCreateEx`,
`DirectDrawEnumerateA/ExA`); everything else is a method of an object made from that. Per frame in the game: ~276
draws, 12.5k vertices, 13.5k indices, 409 SetRenderState, 207 SetTextureStageState, 165 SetTransform, 157
SetMaterial, 73 SetLight, 276 vertex buffer locks, 4 Blt. The menus are 2D (Blt and locks of CPU surfaces, a 3D frame
now and then), the editor the game's 3D at ~440 fps on an empty map.

**DirectDraw7**: QueryInterface (IID_IDirect3D7 only), CreateSurface, EnumDisplayModes, GetCaps, GetDisplayMode,
SetCooperativeLevel (NORMAL, NORMAL|MULTITHREADED), GetAvailableVidMem, AddRef / Release.
Surfaces it makes (DDSD flags caps caps2):

| kind | caps | use |
|---|---|---|
| primary | PRIMARYSURFACE 3DDEVICE VIDEOMEMORY (flags CAPS only) | the screen |
| render target | OFFSCREENPLAIN 3DDEVICE VIDEOMEMORY, 32 bit | the back buffer; locked by the mod (WAIT READONLY, every frame with a wrapper other than DDrawCompat: the D3D11 present reads it back) |
| z-buffer | ZBUFFER VIDEOMEMORY, 32 bit depth | attached to the render target (AddAttachedSurface) |
| CPU surfaces | OFFSCREENPLAIN SYSTEMMEMORY, 565 / XRGB8888 | the UI's software composition, Bink's frames (640x480): Lock, Blt to the back buffer |
| sprites | OFFSCREENPLAIN VIDEOMEMORY + CKSRCBLT, colour key 0xFF..0xFF | UI pieces blitted with KEYSRC |
| textures | TEXTURE, managed, static or dynamic hint | 2x2 .. 256x256 |
| mipmapped textures | TEXTURE MIPMAP COMPLEX, managed static, MIPMAPCOUNT | 8x8 .. 256x256, the full chain (GetAttachedSurface MIPMAP to walk it, Lock per level) |

Pixel formats: ARGB8888 (385 in a game), XRGB8888 (81), RGB565 (11), ARGB4444 (9), the depth format.

**Surface7**: Blt (DONOTWAIT with source and both rects; DONOTWAIT KEYSRC; COLORFILL), GetAttachedSurface (MIPMAP,
ZBUFFER), Lock (none / WAIT / WAIT READONLY), Unlock, SetColorKey (SRCBLT), AddAttachedSurface, GetPixelFormat
(binkw32), GetSurfaceDesc / GetCaps (the mod), AddRef / Release.

**Direct3D7**: EnumDevices, CreateDevice (HAL, T&L HAL), CreateVertexBuffer (WRITEONLY SYSTEMMEMORY, FVF 0x152, 4096
vertices - three of them).

**Device7**: GetCaps, EnumTextureFormats, BeginScene / EndScene, SetRenderTarget, Clear (ZBUFFER; TARGET once),
SetTransform (WORLD, VIEW, PROJECTION), SetViewport, SetMaterial, SetLight (one, DIRECTIONAL), LightEnable,
SetRenderState, DrawPrimitiveVB (triangle lists, line lists), DrawIndexedPrimitiveVB (triangle lists), SetTexture
(stage 0; stage 1 always NULL), SetTextureStageState.

**VertexBuffer7**: Lock (WRITEONLY with NOOVERWRITE, or DISCARDCONTENTS), Unlock. One vertex format for everything:
FVF 0x152 = position, normal, diffuse colour, one texture coordinate pair (36 bytes).

**Render states** (all values seen): ZENABLE 0/1, ZWRITEENABLE 0/1, ZFUNC LESS / LESSEQUAL (NEVER at the start),
ALPHATESTENABLE 0/1, ALPHAFUNC GREATER, ALPHAREF 0 / 127 / 229, ALPHABLENDENABLE 0/1 with SRCBLEND/DESTBLEND
ONE/ZERO or SRCALPHA/INVSRCALPHA, CULLMODE NONE / CW / CCW, FOGENABLE 0/1, FOGVERTEXMODE LINEAR, FOGCOLOR,
FOGSTART / FOGEND, LIGHTING 0/1, AMBIENT (a handful of colours), COLORVERTEX 0/1, DIFFUSE- and
AMBIENTMATERIALSOURCE material / colour 1 (specular and emissive: material), NORMALIZENORMALS 1, LOCALVIEWER 0,
SPECULARENABLE 0, CLIPPING 0/1 (toggled ~285 times a frame - a hint for the runtime, nothing to draw), FILLMODE
solid, SHADEMODE gouraud, DITHERENABLE, COLORKEYENABLE, ANTIALIAS 0, EXTENTS 0. No z-bias, no stencil, no
specular, no table fog.

**Texture stage 0** (stage 1: COLOROP / ALPHAOP DISABLE): COLOROP and ALPHAOP DISABLE, MODULATE, SELECTARG1 (the
editor), arguments TEXTURE / DIFFUSE; ADDRESS WRAP / CLAMP; MAG and MINFILTER POINT / LINEAR; MIPFILTER NONE / POINT /
LINEAR; MIPMAPLODBIAS 0 / -0.75 / -1; TEXCOORDINDEX 0.

So the 3D is one vertex shader (world-view-projection, one directional light with ambient and the material sources,
linear vertex fog) and one pixel shader (stage 0's three operations, alpha test, fog), with the depth / blend /
raster states as a few cached D3D11 state objects.

## How the mod would do it

- The display DLLs' `DirectDrawCreateEx` import returns the mod's objects (C structs with COM vtables) implementing
  the methods above; any other method logs once and returns DDERR_UNSUPPORTED, so a call the probe never saw shows up
  at once. Both display DLLs make the same calls.
- Direct3D 11 on the game window (the device and flip-model swap chain of present_d3d.c): the render target and depth
  buffer are D3D11 textures; textures upload at Unlock (mip levels each); CPU surfaces stay in memory and upload when
  blitted to the GPU; the back buffer reads back only when something locks it (the mod's own shot reads the D3D11
  texture instead).
- The game's vertex buffers become windows into one large per-frame upload buffer; states are compared before they are
  set, so the ~600 state calls per frame cost nothing unless a value changes.
- Screenshots and recording see a normal DXGI window; the warning dialogs paint (nothing covers the window).
- A setting (`[display] renderer`), default off until it plays the same picture: with game_seed the same game frame by
  frame, the mod's `shot` from DDrawCompat and from the own renderer compared automatically.

Steps: the objects and the 2D path (menus, Bink, the UI's blits, present); the 3D (states, shaders, lights, fog,
mipmaps); the picture compared and fixed (half-pixel offset, fog formula, texture formats, the decals' depth); then
the frame time.

## Status

2026-10-27, steps 1 and 2 (`[display] renderer = own`, default `wrapper`; dll/engine/render.c, render_3d.c): the
display DLLs' DirectDraw imports go to the mod's objects (every method the probe measured; any other logs
`# render: X.Y not implemented` once and returns E_NOTIMPL), surfaces in memory and on Direct3D 11, Blt / BltFast
(fills, copies, stretched, colour key), locks of the render target read back only the locked rect, the frame goes
into the window through present_d3d.c's swap chain without a read-back. Checked in headless slot runs: the intro
videos (Bink) and the main menu draw (shots), no call outside the measured list; the 3D is not drawn yet (the
menu's background is black, the device keeps every state and counts the draws). Open: opening the scenario editor
from the menu stopped the game (all threads waiting) once - with the 3D. The display DLL lists 800x600x16 among the
modes it needs: every mode is enumerated in 32 and 16 bits. Autotest `render_trace=N` logs the first N calls
(`# render call`, caller, thread); the pipe's `profile stacks` lists every thread's code addresses.

2026-10-27, step 3 (render_3d.c): the 3D draws. The menus' pictures and buttons are 3D too (textured quads; only
the cursor is a Blt). One vertex shader (world-view-projection, directional lights with ambient and the material
sources, linear / exp / exp2 vertex fog by view depth or range) and one pixel shader (stage 0's operations with
their arguments, alpha test, the colour key, fog); depth, blend, raster states and samplers made once per
combination and set only when they change, the constants uploaded only when changed; each draw's vertices and
indices copied into one upload buffer (4 MB / 1 MB, no-overwrite, discarded when full; an indexed draw's indices
count from its start vertex); half a pixel shift for Direct3D 7's pixel centres; Clear with rects writes depth
through a quad. Only FVF 0x152 and list / strip types are drawn (anything else logs once). Checked: the main menu
and a Continental game draw in full (terrain, water, trees, units, shadows, the interface). Not compared with
DDrawCompat's picture yet (step 4); the editor's stop not looked at again.

2026-10-27, step 4, the picture against DDrawCompat's (docs/testing.md `wrapper_dll`, scripts/test/render_compare.*):
the same game, 20 shots each. Two differences found and fixed: the half-pixel shift went the wrong way (everything one
pixel up-left - the interface matched to 0.7 of 255 once shifted back); the fog factor was clamped per vertex - the
driver interpolates it unclamped and clamps per pixel (the game's fog runs over a narrow depth band, 50 to 58, so
triangles cross its ends: the horizon band's fog was 19% off). After them the mean difference is 5.4-9.2 against
6.3-10.9 between two DDrawCompat runs of the same game (water, swaying trees and the ground's light patches move in
real time, paused too): within the noise. Interface, buildings, units and the decals on the ground (selection circles)
match exactly. Tried and dropped: colour interpolated without perspective (worse).

2026-10-28, batching (the player: 100 fps in a busy base). The game draws an object at a time: 5000 draws a frame in a
late base, 1100-2600 elsewhere. The minute's log line measures it: `# render: frames, draws (the last minute a frame:
draws in batches, ms in them, ms in Present)`. On a copy of the player's autosave (headless, paused at the same step,
the same view, 2580 draws a frame): 0.53 us a draw before, 0.43 after - 1.37 ms a frame to 1.10. Two changes: draws in
a row whose states match go to the GPU as one (2580 draws in 440 batches; the world matrix is the vertex's own, an index
into a ring of matrices read by the vertex shader; strips become lists; anything else that uses the context sends the
batch first - rd3_flush); and the state step is skipped when nothing but the world matrix changed (the setters note a
change only when the value differs - the game sets most states over and over to what they are). Fewer GPU calls alone
gained nothing: the cost was the mod's own per-draw work. The rest of a 10 ms frame is the game's own. Tried and
dropped: the matrices in a constant buffer discarded per batch (the frame rate fell to a third - the driver stalls).
Picture: the same as before (mean difference 0.05-0.46 at the same moment; trees and light patches move in real time).

## The debug overlay

2026-10-28 (the player: what the renderer could do - the AI's plans on the map). `[display] overlay = routes,danger`
(or `all`; the pipe: `overlay <layers | off>`) draws the AI's own data into the map's view: base/overlay.c keeps the
shapes (lines, circles, runs of tiles, in tiles; per layer and key, replaced when their producer draws again or
dropped after a time to live in game time) - the AI may use base/ only, not engine/ -, engine/render_overlay.c draws
them once a frame where the game turns from the map to the interface (its first draw without the depth test after a
draw with the map's camera, else at EndScene): with that camera (the world matrix places objects in tiles, one view
and projection serve the whole map), lines and circles as ribbons on the ground (tile heights bilinear, 0.06 above),
depth tested and not written (hills, buildings and trees hide them), blended, under the interface. Layers: `routes`
(attack.c route_overlay: each attack subgroup's route in its player's colour, red rings at waypoints in reach of known
defences, a ring at the goal; 3 game minutes) and `danger` (attack.c danger_overlay: each AI's danger map, a faint fill
where enemies reach, stronger where its fight is lost, the zones' edges strong - a fill alone tinted half the map;
every 5 game seconds). A layer off costs its producer one test. A new layer: its name a `field_bool(name)` in rows.h's
`OVERLAY_FIELDS` (the setting is a FLAGS row since caa7ea7: a name not there is a mistake in the ini), `overlay_layer("name")` once,
`overlay_on(layer)` before the work, `overlay_begin(layer, key, now, ttl)` ... `overlay_line / overlay_circle /
overlay_tiles` ... `overlay_end()`.

The setting's default is `auto` (2026-10-27, the player): the own renderer when the game folder has no ddraw.dll -
Windows' own DirectDraw crashes the game windowed (DX7HRTnLDisplay.dll+c7d8) - else the wrapper there; `wrapper` and
`own` choose either outright. With `own` a ddraw.dll in the folder is still loaded (the engine loads every DLL there,
the display DLLs import it) but never called; DDrawCompat hooks parts of Windows at its load, so keep it out then.
