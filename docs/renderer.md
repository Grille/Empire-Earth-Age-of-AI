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

2026-11-04, the frame recorded (the player, after the shadows' workarounds: "take control of these renderer steps ...
fighting the game's antique renderer seems a bad idea"). engine/render_frame.c: a batch is no longer drawn when it ends
but goes into the frame's list with the states it was collected under (targets, viewport, blend, depth, raster,
texture, sampler; the vertex and pixel constants as snapshots shared by a run of batches) and its place in the upload
buffers; the list is drawn at the scene's end (rf_scene_end), or earlier where something needs the context as the game
left it (rd3_submit: a texture uploaded that the list draws with, a surface freed, a read back or a lock's write, the
2D path, a Clear, the overlay, the present). The mod's own passes go in with the whole frame known - the shadows' maps
from this frame's casters before anything is drawn. The upload buffers (48 / 16 / 4 MB) are a ring written without
overwrite, so a recorded batch's data stays where it is; the ring starts over at a scene's end when less than a quarter
is left, within one only if full (the list drawn first). The minute's line tells it: `# render: the frame's list drawn
N times a frame, M of them inside the scene ..., the ring full K times`. Measured on "choke wolf" running at speed 1:
1.00 lists a frame, 0.00 inside the scene, the ring never full, 1253 draws in 621 batches, 1.25 ms in them. Picture with
the game's shadows: the same as the build before where nothing moves (a terrain patch identical to the pixel; the info
panel and the counters differ with the game's state).

## The frame and the simulation's lock

The graphics thread draws a frame through RTSGraphics.frame: update_objects (every render object's update), the
clears, draw_objects inside the scene, show. EEGraphics (the game's class) overrides update_objects (0x4FED2D, AoC
0x509840): it takes the world's read lock (Low-Level Engine's TSReadWriteLock, with the interface's critical section)
around the update - the frame's only read of the world under the lock; the draws draw from what the update left. The
simulation's loop (RTSPhysics::Main 0x5532FB) takes the write lock once a pass for its step. A frame that comes during
a step waited for the rest of it: the camera stood, then jumped (the player's late game, 2026-11-03: 100-250 frames a
minute 8 ms or more, the holds 30-90 ms). The lock: a writer enters a critical section and waits on an auto-reset event
the last reader sets again; a first reader waits on the same event.
The minute's lines: `frames waiting 8 ms or more for the lock` (the holds that long by where the game took the lock) and
`the frame's read holds of the world` (how long the frame keeps it - what the simulation waits for a frame; 0.5-1.3 ms
on the reference save; frames drawn during a step).

The frame during a step (`[display] draw_during_steps`, default on; the pipe: `picture steps on | off`; render_3d.c
rd_hook_graphics_update, render_frame.c), 2026-11-05: while the simulation holds the write lock the game's update runs
with the world's lock passed over, and the render objects' updates that read the units are skipped - the ground's,
the units', their shadows' and the effects' (spec: their update slots; the effects' read a unit and crashed when it
ran). The frame is drawn from the last update's state - the units as they stood, the camera and the cursor moving on.
The interface's update runs (UIManager::CopyData, under the interface's own lock, and RedrawAll before it - outside
the world's lock in the game too): skipped at first, the chat lines and buttons over the 3D view were missing from such
frames and flickered while moving (the player); with it two chat lines in all of six shots while scrolling, two thirds
of the frames drawn during a step. The water's (its animation) runs too.
A simulation that is behind runs its steps back to back, the lock free only for a moment between them - no frame began
in it, every frame was drawn during a step and the units stood for minutes (10844 units at speed 2, the player
2026-11-06). So a frame drawn during a step hands off: the simulation's next write lock waits until a frame has
updated from the step's result (at most 50 ms; g_rwl_want, g_rwl_taken) - every step's result is drawn and no frame
waits for a step. On the 8000 save at speed 4: 62 fps with 9 updates a second, the simulation waiting about 16 ms a
hand-off (about 7 % less game time than frames that waited for a step every 100 ms).
- The ground's draw draws nothing without its update (the clear's sky blue where the ground was; units, trees and
  buildings were there). Its update alone took everything else with it - it makes the visible region the units' draw
  uses. So the ground batches of the last frame drawn with an update are kept on the CPU as they are flushed (vertices,
  indices, world matrices, states, constants: render_frame.c rf_keep_ground) and a frame during a step draws them again
  in the ground's place (rf_ground_again). First the frame before that one was drawn (two buffers swapped at the
  update): one frame's camera move off the units' - while scrolling the ground jumped against the debris on it, up and
  down on hills, and showed a row or two of sky at the edge (the player).
- The game's shadows' render object draws the farms' fields, and draws nothing without its update either: every frame
  drawn during a step showed the farms without their fields (the player, 2026-11-06: "farms flicker in visibility" -
  standing still too). Its batches (the fields alone: the blob shadows aren't drawn) are kept and drawn again the same
  way (g_ks). Found with the pipe's record (below, testing.md): the frame drawn during a step 59 kept batches and no
  field, with it 147 - the 88 fields of every other frame - and the field in the picture.
- The game's view is made in the update too: the units of such a frame are drawn from the camera of the last update.
  The camera's move since (its look-at, the height with it - a hill's worth up or down otherwise) is applied as the
  list is drawn (rf_shift): the units' constants by the world's shift, the ground's and the water's (drawn in the
  camera's space) by the shift seen through the camera's axes. The vertices stay where they were placed, the shadows'
  maps with them.
- The camera's place (rf_shift, every frame, not only those during a step): the camera's move sets its transformation
  (spec EECameraControl.transformation, the eye at +0xC/+0x1C/+0x2C) at once, but the view the game's draws are made
  with follows only at the next simulation step. The player's game, the pipe's `picture camtrace N [dx]` (each frame's
  look-at, its view's eye, the camera's eye; `# camtrace2`): while scrolling the view's eye moved 0.25-0.7 tiles at a
  time, every other frame or so - the first version moved frames during a step by the look-at since the update, the
  frames between steps kept the view's place, and the picture jumped between the two (the player: "all problems are
  still there"; the headless tests scrolled slowly and their views kept up). Now each 3D draw is moved by the camera's
  eye now less the eye it was made with - a view's own (its translation), the ground's the eye at its frame's update.
  A still camera: nothing moved (the same picture as before).
- The ground's view is no identity but swaps y and z (1 0 0 / 0 0 1 / 0 1 0; its world matrix the identity): its
  vertices are the camera's space with y and z exchanged. The shift taken in the camera's space as it was moved the
  ground in depth when scrolling up or down the screen - the ground jumped, far more than sideways (the player). Now the
  shift goes back through the draw's own view. The trace's ground vertices, through that view and the units' camera
  back into the world: whole tiles at height 0 (86 139 0, 100 110 0); a 3-tile pan up the screen during a step against
  the frame after it: the towers, the farm, the trees in the same places.
- The zoom (the player: "zooming in and out still cause heavy camera jitter"): EECameraControl.zoom_to (0x51F2F3, AoC
  0x529EB2; the pipe's `camera zoom <z>`) starts it, the camera's step (0x51EE75, in the simulation's step) turns and
  moves the transformation there over half a second - the eye from 121.4 75.0 24.2 to 114.5 81.9 5.7 at zoom 0.6, not
  along the line of sight: the camera tilts. Moving the eye alone left the turn out. Now each draw's view is replaced
  by the camera's whole view, (M^T)^-1 A with the game's axes A = M^T V taken from a frame whose view has the
  transformation's eye (rf_camera_axes); the ground through its own view to the camera of its update and on to the one
  now. Zooming in and out under 12 ms steps: nine shots through the turn, the trees, buildings and fields on their
  ground. Zooming out, a frame during a step shows the sky blue in a corner the last update's ground doesn't reach
  (the frames between the steps have it).
- The camera itself, made on the render thread (the player, the 8000-unit save: "it feels like before again, a bit
  sluggish", zoom, arrows, edge and middle mouse all - the cursor smooth at 120 fps). Two faults: the axes were never
  taken - rf_camera_axes ran at the scene's end after the overlay's frame end had cleared scene_seen, so from 91c9476 on
  nothing was corrected (the shots lined up because every draw kept the game's own stale view); it now runs before it.
  And the camera's transformation is not the move's: the move (camera.c's scrolling, every frame on the render thread)
  only sets a ride from where the camera stands (+0x34/+0x40, by its blend) to the new place with snap set; the
  camera's step (0x51EE75, in the simulation thread's loop between its steps) makes the transformation by blend
  (make_transformation 0x51D2F2, AoC 0x527EB1) and the device's view from it. In that save the transformation stood
  for up to six frames, then jumped (picture camtrace: the look-at 0.035 tiles a frame, the eye 0 0 0 0.10 0 0 ...).
  Now rf_camera_frame makes it each frame as the step would - at 1 with snap, a zoom's ride its blend moved on by
  blend_rate x the time since the step last changed it, never back within a ride - and every draw is corrected to it.
  Checked on a copy of the 8000 save: scrolling, the eye 0.035 a frame every frame; zoom 0.6, the eye down from 14.6 to
  6.2 in even frames (the game's own blend lagging behind); shots during a scroll under 60 ms steps, the trees and
  units on their ground.
- Beyond the last update's ground a scroll or zoom out during a long step showed the sky blue (the player, the
  8000-unit save: "i can move the camera so far over the edge that half the screen is sky"). The ground's margin
  (`[display] ground_margin`, default 100%; the pipe: `picture margin <0..2>`, its reply the region's size, the size
  worked out before and the kept ground's vertices; render_frame.c): the game cuts the ground to the screen three
  times, each widened by 1 + the margin for its call -
  - the region (GETerrainMesh::UpdateVisibleTerrain, its call in the ground's update hooked through the import: the
    tiles under the frustum through the viewport's rectangle, its far plane's corners too - 48 x 49 tiles on that save,
    81 x 81 at the margin): the rectangle widened about its centre. The mesh's arrays are 101 x 101 tiles, unchecked:
    the region's size is worked out first from the same corners (the engine's DenormalizePerspectivePoint) and the
    factor taken back to keep it within 88 (estimated from the last region it went to 95 on a zoom out between two
    updates); worked out and made agreed in every case on that save, AoC's came out above the engine's (72 against 66);
  - the clip codes (the display DLLs' TransformToCamera, their vtables at their load - every update): the left and
    right planes stored in the viewport (their normals' depth part times the factor), the top and bottom it makes in
    the call from the field of view (to 2 atan(k tan(fov / 2))). With the region alone the draws stayed the screen's
    (rprobe: 30 ground draws, 3900 vertices at any margin); with the side planes the width grew but the depth stayed
    17 - 37 (the far plane at 50: the far depth widened too only shrank the factor through the region's limit);
  - the vertex buffers: a material's draw writes all of its vertices into the display DLL's 2442 from the start,
    whatever their count - the grass's went past it with the margin (written beyond our memory, the draw dropped):
    each buffer has room for 65536 now (render_3d.c RD_VB_ROOM), a draw up to as many. And LockVB starts the buffer
    over by that room (its immediate patched at each display DLL's load, spec engine_display.of_lock_vb_room): it
    subtracts its place from 2442 unsigned, so after a ground draw of more (2997 on the 8000-unit save) the place
    was never started over and grew with every model until a copy wrote past our memory - the player's crash,
    DX7HRTnLDisplay.dll's CopyAndSmoothStdVertices, 5 game seconds after loading that save (the minute's line
    counts the draws of more than 2442 by render object).
  On that save, a scroll of 4.5 tiles during a 2.5 s step: the sky at the right and top before, ground to every edge
  with it (17400 vertices kept against 3900; the units and trees out there come with the next update); 12.7 tiles
  (more than a screen) still leave a strip. The ground's draw 0.3 -> 0.6 ms a frame, the frames a minute the same.
  AoC (Continental, the same steps): hooked in both display DLLs, the right side's sky filled.
  Toward the camera (2026-11-06, the player: "the ground should extend 1 or two tiles more under the camera ...
  zooming out or moving down still exposes void"). The region is the tiles in the hull of the eight corners
  (GetTileRowsInVolume), narrowing toward the camera to the near ones, at the viewport's near depth - which the game
  sets about at the last region's nearest ground (19.3 at zoom 1, 0.1 at zoom 0.5): the region ended under the screen's
  bottom edge. Now for the region's call the near depth is the viewport's / (1 + the margin) less 12 x the margin
  (below 0 the corners behind the eye, mirrored: -8.6 at zoom 1, -12 at 0.5), the rectangle's bottom 1 + twice the
  margin, and at the 88-tile cap the top gives way first, then the sides, the near end, the bottom last (the reply
  shows each). And the clip codes no longer go by the region's factors: by them the planes still cut into it - the
  sides, through the eye, as lines on the ground from the camera's foot (triangles of sky in the bottom corners after
  a zoom out), the bottom and the near depth level across the screen (a pan toward the bottom) - so the planes are
  widened by 4 and in the T&L DLL the near depth is the region's (at least 0.05; the software DLL projects the vertices
  itself, its near stays). The 8000-unit save in slot 24, 2560 x 1440, 3 s steps (`stepload`), against the installed
  build: a zoom out from 0.5 to 1 - the two triangles before, ground to every edge now; a pan of 0.03 tiles a frame
  toward the bottom - the ground there in both, to the map's edge; the region 84 / 89 tiles at zoom 1 / 0.5, the kept
  ground 17700 -> 21100 vertices, the ground's draw 0.11 -> 0.13 ms a frame.
- The objects' margin (`[display] object_margin`, default 10%; the pipe: `picture objects <0..0.5>`, its reply how
  many passed by it): the units, trees and buildings beyond the screen stayed away in a frame during a step after a
  scroll - the ground there bare (the player: "one extra tile before trees and units vanish would be good"). The game
  tests each object twice against the viewport's four stored side planes (the units' collection's
  GEPhysicalModel::Intersects, the display DLLs' GERasterizer::IsPerspectiveModelVisible); one that fails is tested
  again with the planes widened by 1 + the margin (render_shadows.c's hooks of both, render_frame.c rf_object_again).
  The 8000-unit save, a 3-tile pan during a step: the units at the right edge reach about 70 px further; the units'
  draw 1.35 -> 1.55 ms a frame.
- The water during a step (AoC, after a long move: dark and sandy squares on the water, pieces of water over the
  map's edge; the same at margin 0): a batch takes the render object current when it is flushed, which is at the next
  draw with other states - the ground's last batch went in with the units' first, the water's last with the
  interface's. So both were missing from what is kept (rf_keep_ground) and the water's last was not moved with the
  camera (rf_shift leaves the interface alone): drawn where the camera had been. Each render object's batch is now
  flushed at its draw's end (RO_DRAW). The water is kept and drawn again like the ground, and during a step the kept
  ground replaces the game's ground draw - in AoC that draws its last lists again (`rprobe step`: the same 73 draws
  as the frame before), the ground came twice. Checked on that AoC map, a 4.5-tile pan during a 2 s step: none of
  either, no hole; the 2017 update's 8000-unit save the same, its pond whole.
Checked on the reference save (slot 21, speed 2.5, the pipe's stepload 9 - each step held 9 ms, the simulation 410 ms
a second): frames waiting 8 ms or more 260-383 a minute off, 0-2 on; frames a minute 7300-8700 off, 11200-13300 on;
an 842 ms step no wait. Shots in steps held 600-4000 ms: complete frames (six over 3 s of one step the same); a 3-tile
pan during the step against the frame after it: the farm, towers, trees and ground in the same places.

Tried and dropped, 2026-11-05 (the player: "is it possible to decouple the rendering from the simulation lock?"): the
loop runs up to two steps under one hold, so the lock was let go between them while a frame waited (the step's vtable
slot). It never came up: the loop runs a second step only when its timer was late, and its step count is taken from
the time since the timer's wait ended, so even 25 ms steps (the pipe's stepload, longer than the 22 ms pass) ran one
step a hold - 2275 holds a minute of 25 ms each, one frame let in in 10 minutes of the reference save. The holds the
frames wait for are single steps. What came of it: the frame during a step (above).

## The units' animation on the graphics card

`[display] gpu_animation` (default on; the pipe: `picture morph on | off`; render_morph.c), 2026-11-06. EE animates a
model by morphing: every frame of its animation is a whole vertex array (spec GEModel.vertices, frames alike share
one), and the T&L DLL's DrawModelHierarchy draws each material by copying its run of the frame into the vertex buffer
- blended with the next frame's by the model's fraction (CopyAndSmoothStdVertices: position and normal; the texture
coordinates the first frame's; plain CopyStdVertices at fraction 0) - for every unit, every frame. The light is the
card's already (hardware T&L). The renderer then copied the vertices into its batch and uploaded them again. On the
8000-unit save, paused: 4566 unit draws a frame, 296 000 vertices, of 33 different parts (the soldiers 1385 units of
three parts) in 12-15 poses each; the profile had the drawing thread half in the DLL's model draws, a fifth in the
upload.

The two copy functions are hooked (the T&L DLL's only - the software DLL has the same, its own): they note what they
would copy and copy nothing. The draw that follows takes the note: each run goes once into a pool on the GPU (64 MB,
the 32-byte vertices as they are, two float4 each; known by its address, checked by its first and last vertex - a
model's memory reused - and started over when full), and the draw becomes a batch entry in the world matrices' table
(the two runs' places, the fraction, its matrix's slot) and indices that name the entry and the vertex in the run (top
bit set; the vertex buffer isn't read for them). Both vertex shaders (the picture's, the shadows' map) blend the two
runs (RD3_MORPH_HLSL). Such batches hold blended draws only; their indices are moved to the entries' place at the
upload. The shadows' role needs only the draw's box: each run's is kept with it. The copy is made after all where the
vertices are read on the CPU - a tree's shadow card (the tree itself still the GPU's), the probe, a draw without depth
- and for a model building_lean bends for its draw (walls.c; its arrays aren't its frames then).

Measured 2026-11-06 on the 8000-unit save, paused, in one game (picture morph on / off, 120 recorded frames each,
twice): 51.6 / 43.6 and 53.2 / 43.2 fps. Running: walking units drawn as before; two shots on / off differ no more than
two shots in a row. Left on the CPU per frame there: about 1700 trees' cards (their copies), and the DLL's own work per
draw - a SetTransform, SetMaterial, the vertex buffer's lock, the draw call - 2800 draws a frame: instancing (a draw
for all units of a part and frame) is the next step.

The material per draw (2026-11-06, the next step). Over the 8000-unit save's army (camera on it, running) the frame
had 19 100 draws in 12 300 batches - batching merged almost nothing - and the renderer's work for them was 12 ms of a
27 ms frame (37 fps); the profile: a seventh in the upload of each small batch (a Map of three rings each), a twelfth
in building and comparing the vertex constants. One frame's unit draws by the probe: 16 553 draws in 24 states, the
state changed 15 286 times - all but 21 of them the material: a model's parts alternate between white and the
player's colour (its banner parts). The material (diffuse, ambient, emissive) is now each draw's own like its world
matrix: the batch's table holds an entry of 8 rows a draw (rb_world: the matrix, the material, a blended draw's runs),
the vertex shader reads it there, and SetMaterial notes no change of the states. Measured the same way: 647 batches a
frame, 6.0 ms in the draws, 55 fps (3282 frames in the minute against 2227). Picture: a shot of each build paused at the
same step differs from the other no more than two shots of one build (the units and trees animate while paused:
42-44 % of the pixels either way). That was the instancing's gain without a draw call of its own per part: the batches
are few now, the game's own work per draw (the T&L DLL's model draw, its visibility test of each model - a ninth of
the drawing thread) is what is left.

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
every 5 game seconds). A layer off costs its producer one test. A new layer: its name a bool field of `overlay_fields` in
`dll_cs/settings/display.cs` (rows.h's `OVERLAY_FIELDS` is made of it) (the setting is a FLAGS row since caa7ea7: a name not there is a mistake in the ini), `overlay_layer("name")` once,
`overlay_on(layer)` before the work, `overlay_begin(layer, key, now, ttl)` ... `overlay_line / overlay_circle /
overlay_tiles` ... `overlay_end()`.

The setting's default is `auto` (2026-10-27, the player): the own renderer when the game folder has no ddraw.dll -
Windows' own DirectDraw crashes the game windowed (DX7HRTnLDisplay.dll+c7d8) - else the wrapper there; `wrapper` and
`own` choose either outright. With `own` a ddraw.dll in the folder is still loaded (the engine loads every DLL there,
the display DLLs import it) but never called; DDrawCompat hooks parts of Windows at its load, so keep it out then.

## The picture's sharpness

engine/render_fxaa.c, 2026-11-05 (the player: "start with them" - of the ideas after the shadows):
- Anisotropic filtering (`[display] anisotropy`, 1..16, default 16; the pipe: `picture aniso <n>`): the game's samplers
  with linear minifying and mip levels are anisotropic (rd3_mk_samp, the level in the sampler key's bits 20..24). The
  ground at the camera's slant kept its grass far into the picture instead of a blur.
- FXAA (`[display] antialias`, default on; the pipe: `picture aa on | off`, `picture edge <0..1>`): the simple form
  (Lottes's four diagonal neighbours), once a frame at the first of the interface's draws after the 3D scene's
  (rf_replay) - the scene's viewport copied into a texture and drawn back through the filter, the interface's text and
  panels drawn over it after. A pixel is an edge where its neighbours' luma range is at least half of the brightest's
  (picture edge 0.5): at 0.25 the grass texture's own grain counted, 42% of a patch of grass changed and it looked
  blurred; at 0.35 9%, at 0.5 0.1% - the wall's stair-stepped top edge still smoothed (24% of its pixels changed).
  Checked on "choke wolf" in the same paused frame, pixel counts by scripts and zoomed shots.

## Shadows

The player, 2026-11-04: dynamic shadows, cast on the terrain only - the models are low-poly and shading on them
would show it (on everything since 2026-11-05, below). `[display] shadows = dynamic` (default `game`; the pipe: `shadows game | dynamic [strength]`, the reply
tells the last map's casters, ground batches, box, sun and shadow colour), engine/render_shadows.c:
- Each draw gets a role: the terrain's receive (not the sky drawn with it); in the units' render object the lit draws
  that don't lie flat cast (trees - alpha tested at 200 or more - without the tint below), the textured ones lying flat
  (decals: roads, craters) receive; in the game's shadows' render object (EEROGroundShadows) the blobs (depth not
  written) are not drawn, the farms' fields (depth written, alpha tested at 127) receive; the water's surface (its
  render object's draws in the ground's camera space) receives.
- Trees cast a card (rd3_shadow_card). The game's camera never turns, and its trees are curved cards made for it (the
  player, 2026-11-05: "there no real 3d shapes, they are curved billboards to sell the illusion better, but from the
  side or behind they break" - static objects' back faces are left out too). Each piece of a tree draw (its triangles
  joined by shared vertices: a crown, a card) is laid flat along the camera's view - its width across the screen and
  its height kept, the depth dropped - and stood up facing the sun about its own middle; made each draw from the
  vertices as the game animated them (the wind's wave), drawn into the map only (SH_AWAY), the tree itself drawn without
  casting (off the screen not at all: SH_GONE). A tree: alpha tested at 200 or more (palms' crowns 229, their trunks
  127 - the trunks cast as they are). The pipe: `shadows cards on | off`. Three tries came before, the same day, each
  turning the tree as a whole in the shadow pass: every vertex laid out across the sun's rays at its distance from the
  trunk (a palm's crown a solid blob), the model's x axis turned across them (the game doesn't turn trees - their world
  matrices are plain scales - and many cast narrow), the widest of 12 directions of the draw's vertices (flipped with
  the wave where two were nearly as wide, and a multi-tree object's crowns came off their trunks: turned about its
  origin). The alpha test of the shadow pass reads a tree's texture at most one mip level down: the map has about 60
  texels a tile, fewer than the texture over a crown, and at the level that fits a palm's fronds' alpha was averaged
  away - only the crown's dense middle passed 229, a smooth blob. Checked on "choke wolf" with trees spawned (firs,
  cedars, spruces, dead pines, maples, palms, the multi-palm 356), cards on and off in the same paused frame: palms cast
  their crowns, dead pines their sparse width instead of a line, the multi-palm's crowns beside their trunks.
- Shadows on everything (the player, 2026-11-05: "how hard would it be to make the shadows affect everything"): the
  casters and the trees receive too, per pixel - the same 3x3 comparisons as the ground's, with a bias of 0.15 tiles
  against their own shape in the map. The vertex shader works out each vertex's colour without the lights' diffuse
  part (as much as the strength takes) and hands over its ratio to the lit colour; a shadow multiplies with that. A face
  turned from the sun has no diffuse part to lose, so it isn't darkened twice. A tree drawn while its card casts is
  SH_TREE: it receives its card's shadow as well, its far half shaded. The pipe: `shadows all on | off` (on). Checked
  on "choke wolf" (the sun at 30 and at 20 degrees, turned 150): towers shade the bunker and wall beside them, the
  gate's wall its own; units in a wall's shadow darker; trees shaded on their side away from the sun; no acne on the
  buildings' lit faces.
- One map a frame needs the frame's list whole (2026-11-05, the player on the 8000-unit save: "on the main mass of
  the archers the shadows repeatedly load in like scanlines"). The map is drawn from the list's casters before the list
  (rd3_shadow_frame), and an upload ring running full draws the list so far: over the archers (3 million vertices a
  frame) twice a frame, the ground in the first part with the casters recorded before the cut, the rows drawn after
  it without their shadows on the ground - a cut that moved every frame. A ring full inside a frame now grows (twice,
  up to 8 times its first 48 / 16 / 4 MB; the log's "upload ring full inside a frame"), and a ring starts over at the
  frame's end when less is left than the frame used. Checked there, 2560 x 1440: the vertices' ring 96 MB, the
  indices' 32 MB after one cut each, then the list drawn once a frame (before: 3.29 times, 2762 cuts a minute in the
  player's game), two shots a second apart with the same shadows over the mass.
- The ground's shadow by the light it faces (2026-11-05). The game re-shades the terrain's vertices as the sun moves
  (measured with the pipe's `shadows hour <0..24>`, which sets the time of day as the daylight-hour effect does -
  EEWorld.set_time_of_day: the hillsides' light and dark sides swap between 10 and 15 o'clock, the ground's mean
  colour 135 at 7, 238 at noon, 100 at 18, bluish at night). The terrain's map from the sun (what the hills hide) kept
  the casters' shadows off the slopes it hid - from the shadows' sun, never lower than 25 degrees, while the evening's
  baked shading is the lower light's: a tower's shadow on a hill broke off on its far slope, lit in the picture (the
  player's shot). Now a shadow leaves a ground vertex the ambient part of the game's light as its normal faces it (the
  units' ambient and sun colour, as on level ground before): a slope turned away loses nothing, and the map isn't read
  (gone with the hills' shadows, below). Checked: "choke wolf" at 8 o'clock (the light 24 degrees), towers and trees
  spawned on the hill, the installed build and this one side by side: the long shadows run on over the far slope
  instead of breaking off.
- The hills' shadows (2026-11-05, shadow_hills). A map over the whole map of the height below which the hills hide the
  sun: each texel's ground (from the tile's corners: ground_corner, once a map), then one sweep from the sun's side,
  each texel the higher of its ground and the texel before it toward the sun lowered by the sun's slope over the step
  (that texel between two of the line before - interpolated, so a shadow's edge softens with its length). 4 texels a
  tile up to a 256-tile map, 2 up to 512, at most 1024 a side. Made again when the sun has turned by a third of a degree,
  on a thread of its own (0.5 ms at 600 a side, 1.1 at 840; 10 ms in the frame before its loop was simplified) and
  uploaded with the next frame. The sun is the light's own height, not the casters' floor of 25 degrees: the map covers
  everything, no reach to keep short - and the ground's shading is the light's (the casters' floor left the hills of
  "choke wolf", gentler than 25 degrees, with hardly a shadow). Everything that receives compares its world height with
  it: in the shadow below it, full 0.75 tiles under it (0.25 before 2026-11-05 - the player: the ground cut level for
  a building site in a slope was a pit whose rim, a few tenths of a tile high, laid a full shadow over the site in the
  evening light; now such a rim gives under half, a hill's deep shadow stays full, its edge a wider soft band), a bias
  of 0.05; the darker of it and the casters' shadow - the ground, decals, water, and with shadows all on the units,
  buildings and trees (a tower's foot in a hill's shadow, its top in the sun). The terrain's second depth map is gone.
  The pipe: `shadows hills on | off`, `shadows hillsoft <tiles>`; the reply tells the map's size and its last making. Checked: "choke wolf" at 8 and 17 o'clock (the light 24 and 12 degrees), hills off
  and on in the same paused frame: at 17 the hill's eastern side and the plain behind it in shadow, a tower and the
  trees standing there darkened with it; a 210-tile random map (840 texels) made in 1.1 ms.
- Casters off the screen (the player, 2026-11-05: with the day's cycle a low sun casts long shadows, and they dropped
  out as soon as the building's or unit's bounds left the screen - his AoC shot: towers' shadows of 3-4 tiles). The
  game culls twice: its units' render object collects the objects on the terrain mesh's visible tile rows that pass
  GEPhysicalModel::Intersects against the view (EEROUnits.draw: GetNumVisibleTileRows / GetVisibleTileRow, five
  Intersects calls - spec engine_display), and the display DLLs draw a model only where
  GERasterizer::IsPerspectiveModelVisible / IsOrthographicModelVisible pass (DX7Rasterizer::DrawModelHierarchy, their
  imports from the engine; the second flag they write is the D3D clipping state). The mod widens the collection's rows
  toward the sun by the longest shadow (the tallest caster's height - measured each map, at least 2 tiles - over the
  sun's slope, at most 24 tiles; tiles the map hasn't got left out: the game's tile test would read them), as rows of
  their own after the game's, on which every object passes Intersects; the display DLLs' test passes every model in
  the units' render object. A draw wholly off the screen (its box's corners all beyond one side) is SH_AWAY: drawn into
  the casters' map only, not in the frame. Only with the draw list moved (draw_limit over 512). The sun for the shadows
  is never lower than 25 degrees (the pipe: `shadows sun <degrees>`; the shadows' colour keeps the light's true height):
  shorter evening shadows, and a shorter reach. For looking at it without the day's cycle the pipe sets the sun's
  height and turns its direction (`shadows height <deg>`, `shadows turn <deg>`), and `shadows reach on | off`.
  Checked 2026-11-05 on "choke wolf" (sun at 15 degrees, 60,88 - 64,92): the towers beyond the left edge cast their
  long shadows over the fields with the reach on, none with it off; the collection 271 -> 413 objects at 56,84, about
  200 casting draws off the screen; the frame's units part 1.14 ms with the reach off, 1.38 on (a
  minute each, paused, the sun at 15 degrees: the longest reach).
  A caster on a hill casts farther by its height over the ground its shadow falls on (the player's shot, 2026-11-05:
  a tower on a hill, its shadow gone with it off the screen - the reach was the tallest caster on level ground). Since
  then the reach is as far as a caster as tall as the tallest, on the highest ground of the blocks that far toward the
  sun, casts onto the lowest ground of the screen's rows (the ground's highest and lowest in blocks of 8 tiles, made
  with the hills' map once a map; each half tile of the sweep checks the blocks the swept rows cross). The whole map's
  height range instead put every hilly map at 24 tiles. Checked: "choke wolf" at 17 o'clock (casters' sun 25 degrees),
  a tower on the hill at 64,86, the camera at 74,100 (the tower off the screen): its shadow across the slope, which
  the build before didn't draw (reach 6.6 -> 21 tiles, 167 -> 475 casting draws off the screen, the units part 2.52 ->
  3.25 ms); on level ground at 40,60 no change (1.54 / 1.60 ms).
  The game's own rows kept their test against the view: an object there whose box had just left the picture lost its
  shadow at the screen's edge (the player, 2026-11-05: "the unit and building shadow still vanishes the second their
  exact bounding leave the image"). While the rows are widened every object on all of them passes now. "choke wolf" at
  17 o'clock: casting draws off the screen 558 -> 669 at 74,100 and 18 -> 117 at 40,60, the units part 3.44 -> 3.95
  and 2.23 -> 2.54 ms (two games side by side - they drifted apart, so no picture of the same edge).
- Dusk and dawn (2026-11-05, shadow_dusk). With the pipe's `shadows hour` stepped through the day: the light sets
  at about 17:45 (12 degrees at 17, 6 at 17:30), the mod took no light under 3 degrees - the shadows stayed, from the
  last sun's direction - and at about 20 the light is the moon's, 24 degrees high from the other side (44 at 22, the
  shadows as dark as at 16), the shadows turning at once. Now a shadow's share is a smooth step from none with the light
  2 degrees over the horizon to all at 12, a light under 3 degrees counts as 0, and the share moves there within
  1.5 s; a light turned by more than 20 degrees at once starts it from 0. Checked: hours 12 to 8 the next morning on
  "choke wolf": 1.00 down to 17, 0.36 at 17:30, 0 at 18 and 19, back to 1 with the moon at 20.
  By moonlight (19 o'clock to 6 - the game's light bluish, 0.49 0.49 0.90 at 22; the sun's warm, full red) a shadow
  takes 75% (the player: "moon should probably be a bit softer 75% maybe"; the pipe: `shadows moon <0..1>`); the hour
  is the world's time of day (spec EEWorld.daytime, +0xB74 in both builds). At 22 a shadow's colour 0.73 instead of
  0.64, the day's unchanged.
- Under the water a shadow fades out with the depth (the player, 2026-11-05: none under water, ideally fading): gone
  0.8 under the level (the pipe: `shadows fade <depth>`). The level is the water's vertices' mean height each frame (in
  the world through the units' camera); measured on "choke wolf": the water at 0.00, the deepest seabed at -0.84 (rprobe
  logs each draw's view translation and its heights in the world now) - a fade to -2 the player asked about would leave
  60% of a shadow on the deepest seabed. The water is drawn half see-through (vertex alpha 0x80): a shadow on deep water
  is half of one on land, on the shallows by the beach nearly all. Both the seabed under it and the surface took the
  shadow, the shallows twice: too strong (the player, 2026-11-05). The surface takes half of one since (it shows the
  sky's light too; the pipe: `shadows water <0..1>`).
- The frame is recorded (above, "the frame recorded"): before its list is drawn, its casters are drawn from the sun
  into a 4096x4096 depth map fitted to their box, its ground into a second one (what the hills hide from the sun) -
  this frame's, from the batches where they lie in the upload buffers (no copies). The terrain, the decals and the
  fields then receive in the main pixel shader (3x3 comparisons; a layer of the terrain blends SRCALPHA/INVSRCALPHA, so
  each layer times the shadow is the ground times it).
- The ground's vertices are the game's own transform into the camera's space (its view only swaps y and z - measured:
  ground vertex (-0.7, 31.2, -6.8) with the camera at tile 73,86), so their place in the world needs the camera the
  units are drawn with - later in the frame, known when the list is drawn: the terrain's constants get the map's
  matrix through it then (rd3_shadow_vc).
- What came before (2026-11-04, the same day): the frame drawn as the game sent it, the ground darkened at the units'
  first draw with a copy of its batches into a depth of its own, the frame multiplied by a picture of the shadow, the
  panels' depth clears copied into it, the map a frame behind. Each fix a guess at the game's order, and the player's
  shots found the next: the panel turned black where a selected building's info sat, the fields vanished (the game's
  shadows' render object was skipped whole - it draws the fields too). With the frame recorded all of it is gone.
- A unit (not a tree) standing in a shadow: its sun light scaled by the map at its origin (four texels), counting only
  what is nearer the sun than its own size - its own body must not shade it. Off since 2026-11-05 (the player: "it
  mostly comes off as bugged flicker" - a unit darkening and brightening as it moves through shadows; the pipe's
  `shadows tint on` brings it back).
- The player's shots of the first version, 2026-11-04, and what came of them: (1) shadows on ground the hills hide from
  the sun - the terrain's map: where it hides a point the casters' shadow is left out (the hill's shading is the
  game's, baked into the ground; no hill shadows of their own); (2) black over the info panel (the multiply above);
  buildings drawn see-through (no depth written) were left out of the casters - wrong: the game draws a building
  see-through while units stand behind it, and its shadow came and went with that (the player's shot, 2026-11-05:
  "they should always see the real building"); now every lit draw that doesn't lie flat casts, see-through or not
  (the see-through ones: material alpha 0.35 or 0.60, alpha tested at 7 or 127); (3) no shadows at some camera spots
  - the sun had been taken from the trees' light, nearly overhead (0.01, 0.11, -0.99), which some other objects are lit
  by too: a light that steep is no sun.
- The shadow's colour: the units' ambient light over ambient + sun on level ground, 80% of the way (strength 0.8).
- Checked 2026-11-04 on "choke wolf" (WWI) and a Continental game at 8 min (epoch 4): towers, bunkers, AA guns,
  vehicles, biplanes (on the hillside), the town centre, giraffes' legs and trees cast their shapes; recorded, the fields
  are drawn and tanks' shadows lie on them, the info panel is clean, a building under construction see-through over the
  shadowed ground. The frame's units part 1.35 ms (running at speed 1; the copying version 2.57, the game's shadows
  1.6-1.8). The water: ships' shadows on its surface, fading on the deep seabed (shots at 73,86 with fade 0.8, without
  fading and with the game's shadows). Open: the player's verdict; the units' tint not checked in a picture yet.

What the game draws (the pipe's `rprobe`: the next frame's draws logged one by one with their render object, states,
light, material, texture, world matrix and their vertices' extent and flatness; `rprobe skip <parts>` leaves render
objects out of the picture). Probed 2026-11-04 in a Continental game at 8 min (epoch 4) and on the player's save
"choke wolf" (WWI, 153 min, five camera spots, 4744 draws):
- The game's shadows (EEROGroundShadows) are a blob a unit: one quad lying on the ground, a soft dark texture of
  16x16 to 256x256 per kind of object (18 textures in the WWI view: 628 draws of one 16x16 - the soldiers -, every tree
  the same 32x32), scaled to the object, turned with it, unlit (light off, ambient white), blended, depth tested and not
  written. Faint: in the WWI view they are hardly seen beside the units.
- The terrain is drawn unlit (LIGHTING 0): its shading - the hillsides' - is baked into the vertex colours. A shadow
  there multiplies the colour.
- The units' light: one directional, dir (0.08, 0.75, -0.65) (z is up: about 40 degrees above the horizon), white,
  ambient 0xff657899. Trees (alpha test 229, no culling) are lit by another, nearly overhead: (0.01, 0.11, -0.99) - the
  sun for the shadows is the units' light.
- No upright flat sprites: of 2882 unit draws 2429 are solid meshes; the flat ones lie on the ground (decals - 277
  unlit 128x128, 56 untextured, a few lit) - those must not cast. 3 nearly flat upright draws of 56 vertices (a
  0.12 x 0.04 x 0.06 part scaled 2x). Buildings under construction are drawn translucent (alpha test 7).
