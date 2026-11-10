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
Asked after every step a frame came during, the hand-off held the simulation 1000-1500 times a minute in the player's
450-unit WW1 game (2026-11-07, their log: 4.5-10.8 s of every minute) - the frames come every few ms, so nearly every
step had one, while the frames between the steps would have updated by themselves. Now it is asked only once the
picture has gone `[display] step_handoff_ms` (default 50; the pipe: `picture handoff <ms>`, 0 as before) without an
update. The farm save at speed 2.5, two minutes: 720 hand-offs a minute -> 3-5, the game's pace the same (it kept up
either way). The 8000 save at speed 4, its steps back to back and each longer than 50 ms: still a hand-off a step (510
-> 528 a minute), 176 -> 191 game seconds in two minutes. What the simulation still waits for there is a frame's
draw to end.
What the picture costs the simulation now (2026-11-07; the minute's line has the other threads' write locks and their
wait for a frame's read or claim beside the hand-offs), a minute each: the farm save at speed 2.5 (it keeps up) - a
hand-off once, 0 ms, 2625-2728 write locks waiting 285-292 ms for the frames' reads (0.5 %), the update's read hold
about 0.6 ms; the 8000 save at speed 4 (behind) - 342-348 hand-offs, 4.9-5.4 s (about 15 ms each: the rest of the
frame being drawn, then its update), the read waits 63-84 ms, a read hold 2-5 ms; the player's 450-unit game with the
threshold (their log): 49-59 hand-offs, 0.5-0.6 s. A buffer of the units' records the simulation fills itself (the
units' update, 0x503B83, moved to the end of its step, the frames drawing from the other half) is not the gain it
seemed: the simulation would do the update's work itself every step - more than it waits now where the game keeps
up (the farm save's 0.6 ms a step against its 0.3 s a minute). Where it is behind, the hand-off's 15 ms against the
update's 2-5 ms is the gain left. A unit's pose is the simulation's: the update copies it from the unit's physics
object (0x502622: its frame +0x178, next frame +0x308, the fraction between them +0x23C - the record's +0x12C, +0x130,
+0x104, given to the model by the draw - and its transformation +0x130), which no write touches while the game is
paused (the pipe's watch on a soldier's fraction: none in 2 s; the poses of the click list's units alike a second
apart). What the update moves by the clock is the trees' sway: 40 frames recorded paused, each updated, differ in
87 000-150 000 pixels, all of them trees and the chat's text; 40 inside steps, none updated, in 2300-2900.
Two waits were left behind a step (2026-11-07, the player with the pipe's `stepload 1000`: "the mouse sidescroll lags
for one second once the sim takes control again ... one mouse click and it freezes once the next simstep comes"). The
pipe's `profile stacks` (now reading each stack page by page - one read past its end got nothing - and showing only
the threads with the game's or the mod's code) on the 8000 save, the mouse at the screen's edge:
- The interface's lock. EEGraphics.update_objects (0x4FED2D) enters the UI manager's critical section (UIManager +4,
  spec UIManager.lock) and takes the world's read lock inside it; the game's main thread (its WinMain, 0x535CA7) takes
  the world's write lock inside the same section for the player's input. During a step the main thread held it waiting
  for the step, the frame waited for the main thread. Now the frame takes it first without waiting (TryEnter on its
  CRITICAL_SECTION, the engine's own count kept: game/api ui.c ui_lock_try): while another thread holds it and a writer
  is in or waiting, the frame goes without the game's update at all - drawn as during a step, the interface's draws of
  its last update drawn again (the minute's line: frames "without the game's update"). Drawing the interface as it
  was made the command buttons flicker (the player): each form's DrawPolygons draws the list its CopyData (the
  interface's update) made for the frame and empties it, so such a frame had the panels but not the buttons, the
  cancel button while placing a building, parts of the minimap (the pipe's `record bare`: 26000 pixels a frame). Now
  the interface's batches are kept as the ground's are (render_frame.c g_ku), recorded in every frame whose interface
  was updated, each holding its surface: drawn again, its pixels go up to the card if they changed since (a text the
  game wrote anew after the update was drawn empty).
- The frame's decision. A frame read the world when no write was held at its start; a step's write lock taken between
  that and its read made the read wait for the whole step. Now a writer counts itself before it takes the lock (the
  frame's own not) and a frame claims its read before it looks: one of the two sees the other - the frame is drawn as
  during a step, or the writer waits for the frame's read to be in (a few ms, at most 100).
Measured on the 8000 save, `stepload 1000`, two minutes of the mouse at the left and right edges and a left and a right
click every few seconds, both builds at once: frames waiting 8 ms or more for the lock 10 and 9 a minute (the longest
2000 ms), a frame 48 ms (35 of them waits) before; none, a frame 13 ms with it (2209 and 2607 frames of the minutes
without the game's update). A shot 0.25 s after a right click in a step: the interface, the health bars and the minimap
drawn. AoC: the interface's lock at the same place (0x50989F), a short game normal.
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
- The soft edges of the units' textures (`unit_soft_edges`, 2026-11-09): most of the units' draws are blended, write the depth and are alpha tested at 127, so a texel half see-through writes the depth like a solid one, and what lies behind it and is drawn later fails there - the ground showed through a silo along a ladder's rails. `rf_edges` draws such a batch in two parts by the texture's alpha: at 250 or more in the list's order, the rest without writing the depth after the list's last such batch. Trees (tested at 200 or more) stay in one part: their edge is a narrow band and a forest would be drawn twice. The dark age's temple's texture (bld_temple_04t) is painted at alpha 238 and never reaches solid, so all of it counted as edge and its floor came out over its roof: `config/texfix.eeai` (installed once beside ageofai.ini, the player's own after that) lists it by name, `fixalpha = ["bld_temple_04t"]`, and the texture's alpha is scaled to reach 255 when it goes to the graphics card (`tf_hook_load`: the name is only known in the engine's GETextureManager::LoadTexture, the surfaces made inside that call are the texture's). By name and not by the pixels' look: a survey of data.ssa finds this one texture of 2235 images, but in Art of Conquest amb_carbon_3t looks the same and may be meant to be see-through. The game's files stay as they are. Multisampled alpha to coverage would need a multisampled target for the whole picture, and the water's step, FXAA and the read-backs all take a plain one.
- Silhouettes (`unit_silhouettes`, 2026-11-09; the pipe: `picture sil on | off`; render_silhouette.c; needs the mod's walk, units_smooth): a building or a tree no longer goes see-through for what stands behind it; a unit behind one is drawn over it in its player's colour, lit as it is (the brightest channel of its vertex colour): the units go solid into a picture and a depth of their own, and that picture over the scene at 0.55 as one, so that a model's nearer parts cover its farther ones. Only where a building is what the pixel shows and the unit lies behind it: the list's buildings are drawn alone into a depth of their own (the depth buffer can't say who wrote it, and has no stencil), and the unit's pixel compares itself with that and that with the picture's depth, so a unit's own front, another unit, hills and trees show nothing. The fill tags each record (`sil_tag`), the game's draw of a record is hooked (`ro_units_draw_record`'s three calls) so that a batch holds one kind, and the player's colour is the entry's free number.
- Units as instances (`unit_instancing`, 2026-11-09, default off; the pipe: `picture inst on | off`, nothing: its counts; render_instance.c): a unit's or a building's record no longer goes through the display DLL part by part. DrawModelHierarchy is hooked: the same walk of the hierarchy, the view's test and the detail by the engine's own functions, each material straight into the batch. A model's triangles are in a pool on the GPU once, and a batch's draws of one list are one DrawIndexedInstanced: the index's top bits say that the entry is the instance's, the instance stream (slot 1, 0 to 2^19) counts from where the batch's entries lie, so they are uploaded list by list. Trees come in by their own record draw (the hook of draw_tree), and their shadow cards are kept (in_card_of): the same for every tree of a model's frame, made anew when the sun or the camera has moved, two frames' cards blended like two frames, drawn from the pool at the tree's place in a batch of their own. Left to the DLL as a whole: what isn't a record of a unit or a tree, a model bent for the draw (building_lean), vertices with colours of their own, a pushed transformation, the probe. 1295 objects paused at 2560x1440, the same game, on / off: the records' draw 1.21 / 1.73 ms a frame, about 210 / 160 frames a second. What is left is spread over the view's test, the shadows' classifying and the batch's states, a part at a time.
- The lines from an airport to its targets (EEE3DMarchingLine, 2026-11-09) are effects of the third queue, which the ground's pass draws, in pixels. Kept with the ground, a frame during a step showed them where the last update's camera had put them, the ground moving on under them. They aren't kept now: such a frame draws them itself, from the queues (`rf_ground_lines`).
- What the units' draw places in pixels itself (2026-11-07, the player: "the healthbars of selected units dont draw
  based on camera position"): the selected units' health bars are drawn in 2D mode (DX7Rasterizer::Set2DMode: view
  identity, an orthographic projection, depth 0 - `rprobe` shows them as 45 x 3.75 quads at pixel places, the units'
  render object), each at a point the game took to the screen itself (0x50DD81, AoC 0x518676: also the building's
  footprint lines, the goal markers, the waypoints' lines) through the map's viewport's world into the camera
  (GEViewport +0xF0, the inverse of the camera's transformation at +0xB0, made at the camera's step). No view to
  correct, they stood while the units moved under them with the camera - a step's scrolling behind; without a depth
  they can't be taken back into the world either. So that transformation's operands in the units' draw (spec
  engine_display.viewport_view_ops: that function's, and the depth ranges of the building being placed and of the
  objects drawn over the rest) point at the renderer's own (render_frame.c rf_view_now): each frame the inverse of the
  camera's transformation now, the one the 3D draws are corrected to - the viewport's own while there is none. The sound's
  place on the screen (0x53C193, the fourth) keeps the game's. Checked on the 8000 save, the army selected, 4 s steps
  (`stepload`), a 1-tile pan landing in a step, both builds the same scene: the bars about 110 px to the right of their
  units before, each over its ringed unit now; 4 s on, after the step, over their units in both; AoC: the three
  places patched, the spec's checks hold.
- The ground from the map itself (always, with the own renderer; the pipe, for comparing while the game's path still exists: `picture ground on | off`; render_ground.c, 2026-11-09, the player: "raw game map data straight to gpu"): the whole map on the GPU in chunks of 32 tiles, made once through the mesh's exported functions (no offsets of its) and drawn each frame with the camera as it is now, instead of the game's region of 101 x 101 tiles at most, remade at every update. The recipe is UpdateVisibleTerrain's: a quad's corners belong to the four cells around them, the lowest texture by address is the base, the others are blended over it by corner; the colour is the table's times DX7Rasterizer::Light's light clamped to 1, taken while Light runs (the device's lights are off by the time the ground is drawn); the fog of war (UpdateFogOfWar: the sight number as stored, 2 and more 0.25 a cell, 1 0.125) a texture of the cells and the seabed's wobble (TransformToCamera: deeper than -0.5, sin(x y + phase) times the strength) both in the vertex shader. A frame during a step draws it with the states and the projection of the last frame that drew it (the current viewport then is the interface's). The end of the game's draw sets the display state's "3D drawn" (+0x6D): without it Set2DMode leaves the 3D matrices and the interface fails the depth test. On the 8000-unit save: 42372 triangles made in 15 ms; against the game's ground, paused, the same to the level at every place measured but what moves by the clock (trees, the water); both builds. Not yet: the water surface (the game's, its region still that of the mesh), the map's edge (the game's skirts and black beyond it; sky here), a chunk made again when the ground changes under a building.
- The water's surface from the map too (the same setting, 2026-11-09): per chunk a second vertex buffer. A tile `GetIsWaterTile` calls water is a quad at the level with one whole texture of the animation, its frame from the table of 33 x 33 (`of_mesh_water_patches`), read from the group's texture each frame, so `UpdateWaterAnimation` goes on driving it; alpha 0x80, 0 at a corner the bits of `GetWaterInfo` call shore (2 A, 4 B, 8 D, 0x10 C). A vertex holds the ground's height under it, and the vertex shader moves it as `TransformToCamera` does: sin(x y + phase) times the surface's strength, over land half of (sin - 1).
  The lights are the vertex shader's since then (the game's lights in a constant buffer, the table's colour and the normal in the vertex): they change in play, and a whole map made again for it cost 15 ms on 200 x 200 and 100 ms on 440 x 440, each time. A chunk is now made once a map.
  With dynamic shadows the map's ground and water take the game's roles (`SH_RECEIVE`, `SH_WATER`) in the world's space: `rd3_shadow_vc` tells them by the place in their view, the game's having none. Before, the map's ground got no shadows from the casters at all. Checked: the player's island save and the 8000-unit save, paused, dynamic shadows and the water's look on, against the game's from one camera: the ground black in the difference picture, what differs is the water's animation frame and the trees' sway; a 2.5 s step panning over open sea, the furthest zoom; Art of Conquest on Small Islands.
  The black round the map too, once a map (`rg_skirt_build`): the game's `Generate*EdgePolygons` from the boundary out and down to the level within 5 tiles, flat beyond, wedges at the four corners, as far as any view. Art of Conquest's maps with space tiles (`GetIsSpaceMap`) stay the game's ground: not done here.
  The ground's changes: the exe's imports of SetZ, ComputeVertexNormal and ComputeShorelines mark the chunks a corner reaches, the next frame that draws the ground makes those again (`rg_rebuild`); the scenario editor's paths are its own, so what it looks at is made again five times a second and the rest a chunk at a time (`rg_editor`). The pipe: `picture ground raise <x> <y>`.
  Past the mesh's arrays (`camera_distance_max` up to 1000, the own renderer only; without it 42): when the frustum's region would be over the 88 tiles the arrays can take (`rf_hook_terrain_visible`), the rows the units and trees are collected from are made by the mod, as `GetTileRowsInVolume` does, from the true frustum (`vr_make`) - the effects' update, which finds what lies on the ground (a farm's fields, roads, craters) by reading the mesh's vector of rows itself, has them in that vector through its call (`vr_effects_update`) -, and the game's update is given a nearer far depth and if need be a smaller rectangle so that its region fits (`rf_region_fit`): what is left of its use is the highlights on the ground, near the camera's foot. The far depth grows 1.6 tiles per tile back with the own renderer, to the top of the screen.
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
  A zoom out (2026-11-07, the player: "when zooming out the sky is still often visible - do you consider that the
  flat zoom out moves many tiles backward first before gaining real height?"): the ride from zoom 0.2 to 1 takes the
  eye about 24 tiles back over the ground before it climbs, and every frame drawn during a step showed the ground made
  for the eye of the last update - the bottom third of the screen the sky (the 8000-unit save, 300 ms steps). A region
  reaching further behind did nothing: TransformToCamera marks each vertex against the view's planes (projected: the
  near depth, the rectangle's sides) and leaves out a triangle all of whose corners share a mark - all through the
  eye, so the ground behind it is always left out. Now while a ride takes the eye away from the look-at, the ground's
  update runs with the viewport's camera pulled back over the ground to where the ride ends (its turn and height
  kept, the far plane longer by as much; GEViewport::SetCamera, set back after; spec engine_display.of_viewport_camera):
  the region, the marks and the ground's camera space all of that camera, and the correction of the ground and the
  water starts from the camera TransformToCamera ran with (render_frame.c rf_pull_begin, g_kg_view_ground) - unless
  the pulled camera's region at the screen's own rectangle is beyond the mesh's arrays. The same save, 100 ms steps:
  the sky at the bottom up to the first update after the zoom began, none after it (without: to frame 39 of 87);
  without steps, the frames updated during the ride with the trees, units and shadows on their ground. A zoom begun
  and ended within one step keeps the last ground until that step ends: nothing knew of the ride when it was made.
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
  The water's pass is its surface and then the fifth queue's effects (the flames, the smoke, the pieces a dying mech
  throws, the ships' ripples): kept whole, those stood through every frame during a step and jumped at the next real
  one (2026-11-10: at a step load of 30 ms every frame but one in 30 is drawn during a step, and none of them drew an
  effect). Since then a real frame notes how many of the water's kept batches are its surface (render_ground.c rg_hook,
  after the game's DrawWater: g_kw_surface), and a frame during a step draws only those again, then the game's water
  draw with its surface left out (g_rw_fx_only) - its effects anew, from the simulation's queues - when the queues in
  use are the simulation's (collect_copy.c collect_fx_sims: they hold their effects, a step can't free them); else
  all of it kept, as before.
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

## The units' collection

The aim (the player, 2026-11-07: "it means bringing a large chunk of the game under our own control again"): the
units' draw lists made by the mod's own code from a copy of the world the simulation makes at each step's end, so the
frame never reads the world and the simulation never waits for it - and with that the units drawn with each frame's
camera (none missing at the screen's edges while it moves), their movement between two steps' places, no fixed limits,
and the lists made the way the instancing wants them. In steps, each checked against the game's own lists.

What the game does: the units' update (EEROUnits::vf0 0x503B83, AoC 0x50EABE; the AoC decompilation's names below)
after its setup (the lists emptied, the selection's pick, the unit under the mouse, the camera's yaw, the frame's
number) walks the terrain mesh's visible tile rows inside the picker's lock (gUnitPickerLock, 0x912874): on each
explored tile every object and the parts it is made of (unit +0x30, its +0x1CC first, then +0x1D0), each the view's
test lets through (IsUnitDrawn 0x5008B8) and the picker (the list the mouse picks from) hasn't got yet into a record -
a tree (type +0x619, mIsTree) into the trees' list (FillObject 0x501DFF), anything else into the objects' (FillUnit
0x502622), the sort map by its opacity (GetUnitOpacity 0x5044D7); then, on a tile out of the player's sight, the
buildings and trees it remembers there (the tile's doppelgangers, +0x1C: FillDoppelgangerUnit 0x5035AD,
FillDoppelgangerObject 0x502224; one on several tiles once). After it the records are finished (0x501C29) and the
music's measure taken. A unit's pose is the simulation's (above, "The frame and the simulation's lock"); the trees'
records hold their sway by the clock (+0x28).

Step 1, the walk (collect.c, 2026-11-07; the pipe: `picture collect own | game`, the game's by default; `drawcount
hash` prints the lists the next update made): the update's first instruction of the walk (mov ecx, [picker_lock],
spec engine_display.units_walk) calls a thunk that runs the mod's walk and goes on after the game's (units_walk_end),
the game's fills still making the records; the walk's locals are the update's (its frame), and its calls at the
game's call sites go to what the sites call now (walls.c's record fill, drawlist.c's search of the picker, the
shadows' rows). AoC's walk is the same instructions but for its addresses and three of the type's fields. Checked,
paused (the lists the same every update): the 8000 save at two places, 7198 and 5210 objects, and an AoC Continental
game at two places - the counts, the picker, the six sort maps' sizes and the objects' records (but their vectors)
the same with either walk; the trees' records too but their sway. The save's player sees the whole map (no
doppelgangers anywhere in a sweep of it), so also as its player 2 (game.local_player poked): at two places with 14 and
415 doppelgangers drawn, the same. Running a minute at speed 3 with the mod's walk.
The picker's room (2026-11-07, the player: "the limited selection is a known limitation"): the list the mouse picks
from (EEUnitPicker, engine_display.pick_list: the walk adds every object it draws) has room for 0x600 - its
constructor's array, +0x2A4 its room, and its add (0x50F6E2) drops any more: in the 8000 save it held 1536 of 7198
objects drawn, the rest couldn't be clicked or boxed. collect.c gives it room for all the draw lists hold (drawlist.c's
limits, objects and trees: 65536 at draw_limit 16384) before the first walk: an array made as its constructor makes
one (the game's new, the count before the entries for its destructor's delete[], each a GETransformation), swapped in
once inside its lock with the list emptied (game/api pick_list_grow; the old array left - a reader may hold it). The
log: `# collect: the picker's room 1536 -> N`; the 8000 save's picker then held 8342. A box (vdrag 200 150 2400 1150,
`seltest -1`) over the save's crowd at 120 120 - 5291 objects, the player's own soldiers most of them: 0 selected
before (the picker full of others), 109 after. The 109 were the selection message's limit (255 packed bytes): the
rest now goes after it as messages that add to the selection (select.c; docs/engine-notes.md) - the same box 286 of
286 found, in 3 messages.

Step 2, the fills (done, 2026-11-08): each of the game's fills in C as two halves - what it reads of the world into a compact
record of the object (in step 3 the simulation's, at its step's end, for every object) and the draw record made of
that (the frame's: the camera's parts - GetUnitOpacity fades what is near the camera, IsUnitDrawn tests the model
against the view). Both halves in the frame first, each fill held against the game's by `drawcount hash`.

The opacity ported (collect.c uc_opacity, 2026-11-07; the pipe: `picture collect opacity game | check | own` - check
runs both on every object the mod's walk lists, keeps the game's and logs the differences with `drawcount hash`;
docs/testing.md). GetUnitOpacity (EE 0x5044D7, AoC 0x50F412) is the object's opacity into its record and whether it is
drawn see-through: "show hidden units" (1: an obstacle or a moved unit hidden behind something, by its update_hidden;
2: every obstacle), a dying unit fading by its rot_ms over its type's rot_time, a cloakable unit by its own opacity -
dimmed to an enemy to at most 0.5 -, and a land building fading as it comes within half a unit (front_fade) of the
front clipping plane of a viewport of its own (0x919470, not the map's). AoC moved the rot and cloak part into a
function (0x63CEE2) and takes it also for an object already see-through while it is cloaked (mIsCloaked, +0x346 - EE
has none), and reads the rot time of the complex unit's type where EE reads the part's. Ported from the disassembly with
the FPU's long doubles between the code's own float stores, so the result is the game's bit for bit whatever the
render thread's precision is: held against the game's, 0 of 8.4 million objects differed in the 8000 save (trees
see-through by show-hidden among them) and 0 of 915 000 on an EE Continental map, 0 of 430 000 in AoC. The near fade
never fires at the game's own constant (its window is half a unit before the plane, and objects behind the plane
aren't drawn); with it poked to 2 .. 40, 3927 (EE) and 2990 (AoC) land buildings faded, all alike. (On a Continental
map at the closest zoom, 0, buildings do fade by the camera: 7967 in EE, 8158 in AoC, all alike too.)

The view test ported (collect.c uc_drawn, 2026-11-07; the pipe: `picture collect drawn game | check | own`). IsUnitDrawn
(EE 0x5008B8, AoC 0x50B3B3 - the same code but for addresses and offsets) decides whether the walk lists an object: on a
tile in the player's sight, or a gaia animal or bush with food left (it shows in the fog), or the free camera; a scale
not 0; room left in its list (its cmp's operand, drawlist.c's limit); not dead; met once when it stands on several
tiles (the update's set); and its model against the view - a complex unit's own physical model, else a physical model
made on the stack at the tile's middle turned to the camera's yaw (a remembered building; a tree shifted off its middle
by its id). Its three calls of GEPhysicalModel::Intersects go to what their sites call now (render_shadows.c's casters
off the screen). A path point is never drawn (the game asks a function that returns false in both builds), a flare
neither, a foundation only for its owner. The check runs ours first with the met set only looked in, then the game's,
which adds to it. Held against the game's, 0 differ: the 8000 save 3.0 million objects (2.99 million drawn), as its
player 2 4.8 million (all in the fog: only the remembered ones drawn, 448 of them), an EE Continental game 4.8 million,
AoC 3.8 million. Found on the way: the set's insert answers a pair (its node and whether it was new, 8 bytes), and the
walk of step 1 gave it room for the node alone - 4 bytes of collect_walk's stack overwritten for each remembered
building on several tiles; now room for both.

The fill ported (collect.c uc_fill, 2026-11-07; the pipe: `picture collect fill game | own`). FillUnit (EE 0x502622,
AoC 0x50D3AD; 1246 instructions, AoC's 1289) makes an object's draw record (spec ee_draw_record, 0x148 bytes): its
type's, owner's and complex unit's values; the health bars (its own hit points, then each unit inside's for its owner
and allies) of a selected unit or one in the list of ids at 0x8D2A90 - and the record into the health bars' list -, a
power bar, the selection circle by the circle mode; its graphics' parts and two lists of the complex unit into the
record's vectors; the name under the mouse (and its owner's for another player's unit); its pose (the complex unit's
transformation, frame, next frame and fraction, turned up to the camera for a type that faces it); one without a
complex unit at its tile's middle turned to the camera's yaw, a path marker on its target (coloured by the target's
id); a building site scaled up by its hit points with its finished building's graphics beside; the ground's height
under it; then into the picker, the sort map by its graphics' key and, with a complex unit, the third map. The vectors'
resize, clear and push, the strings' operator=, the picker's add, the maps' insert, the ambient sounds' turn are the
game's own functions. AoC's code differs in four places, ported by build: a ground circle for its type 0x2F0 when
selected, the circle's sight from GetLineOfSight, two kinds of building sites sunk into the ground, the tile's light
byte tested by its low 7 bits. walls.c's hook of the fill's site calls collect_fill (the game's fill or ours), then
shears its farms. No check mode - the fill changes the game's lists -: held against the game's in a paused game, the
lists' print with either fill alike - the records' bytes and their vectors' and labels' contents, the picker, the sort
maps, the health bars' list: the 8000 save at three places (7264, 5299, 344 objects; 3677 units selected, 4395 health
bars), as its player 2 in the fog, an EE and an AoC Continental game with their start's units selected - each then
30 s at speed 3 with the mod's fill.

The trees' fill ported (collect.c uc_fill_tree, 2026-11-08; the same switch: `picture collect fill game | check | own`).
FillObject (EE 0x501DFF, AoC 0x50CBA3: 352 instructions, the same but for addresses and offsets; EE hands only a wood
tree to the ambient sounds) makes a tree's small record (spec ee_draw_small_record, 0x3C bytes): at its tile's middle
shifted by its id (id % 3 - 1 and id / 3 % 3 - 1 times its size and 0.125), the ground's height there, turned to the
camera's yaw; its hit points; its scale varied by its id (1 + id % 11 times a step); its type's two bar values when a
diagonal neighbour tile has no tree (the forest's edge); its selection circle and the trees' health bars' list while it
is the player's selected unit; its sway in the wind - (1 + cos(the tilt offset less ((the clock + id x 8) % 360 - 180)
times a step)) / 2, for a model of more than one frame; then the sort map, the picker (its transformation) and the sixth
sort map. The game also makes a physical model on its stack it never uses (left out). The sway follows a clock that
runs while the game is paused, so the paused prints differ in it; fill check holds the trees against the game's as
they are filled - the record as it was, the game's fill, ours into a copy without its calls that change the game's
lists, every byte compared: 0 differ of 88 683 trees in the 8000 save, 132 736 on an EE Continental map, 51 832 in AoC;
the paused prints alike in every other dword, the objects' as before.

The doppelgangers' fills ported (collect.c uc_fill_dopp, uc_fill_dopp_tree, 2026-11-08; the same switch). A building or
tree the player remembers on a tile out of sight is a doppelganger (spec EEDoppelganger: one of an object without a
complex unit, EEComplexDoppelganger one with - what it was when last seen); the fills ask it only its virtual
functions (anim, finished, frame, graphics, dead, has_transform, transform, scale, hp: vf6, 8-15). FillDoppelgangerUnit
(EE 0x5035AD, AoC 0x50E3C8) puts a building at its own pose or its tile's middle (turned to the camera's yaw - twice -
by a type that faces it), tests it against the view (calls_units_view's fifth site) and with room left makes its
record: its type's values, opaque, fogged, a building site with its finished building's graphics and its height scaled
by its hit points, the ground's height under it; then the picker and the sort maps by its graphics' first key (not its
animation's). A quirk kept: the finished building's graphics come from the record's player as the record had it
(the fill never sets it - the last record in that place). FillDoppelgangerObject (EE 0x502224, AoC 0x50CFBC) is FillObject for a remembered tree, at
the walk's tile and the type's unknown_42c for its scale, its hit points the doppelganger's; the next frame for its sway
when its model has one more. Both builds the same code but for addresses and offsets and the three differences the
other fills have (the graphics acquired through their vtable, the light byte's 7 bits, every tree to the ambient
sounds). fill check holds them against the game's: the record as it was, the game's fill, ours dry into the copy,
every byte and whether each drew it compared (a record left as it was could match by chance): the 8000 save as its
player 2 (all its 1237 remembered in the fog) 1 081 608 at seven places, an EE Continental game as its player 2
after 5 minutes 2 589 482 (1 439 430 drawn), AoC's 3 025 463 (1 559 686 drawn): 0 differ; the paused prints alike
with the game's fills and ours (a first run as player 2 found the ground's height on a lit tile wrong: the light's
tile had been read with x and y swapped - ours, not the game's).

The ghosts' fill ported (collect.c uc_fill_ghosts, 2026-11-08; the same switch; hooked at its call in the update, EE
0x5041F7, AoC 0x50F132, so with either walk). What the plan called the records' finish (0x501C29) is the AoC
decompilation's FillGhostRecords (AoC 0x50C952): the placement ghosts - the building the player is placing under the
mouse, green or red, an airport's three runways and a granary's farms with it (spec EEGhostUnit, the world's
engine_display.ghost_units, made by EEGhostUnit::Create and moved by the preview) - get a record each (0x420: an
ee_draw_record, then the footprint's 12 x 12 corner heights and cells, the tile) when they changed, inside their lock,
pushed into a vector the frame draws them from (EEROUnits::DrawPlacement). Its model, scale (its player's stats of its
type), frame, animation, can_place, its type's and player's values, the footprint, the pose lifted 0.001 over the
ground (AoC: the types 0x302 and 0x31F by their heights more - the same heights its fill sinks their building sites
by). One record on the game's stack serves them all and the record's constructor leaves the circle, the bars, the
ground's height, the opacity, state, serial and player as the stack had them - in the game's fill too; DrawPlacement
sets the opacity itself and reads none of the rest, so fill check compares the dwords the constructor or the fill
sets (uc_ghost_set). Checked with ghosts the pipe makes (`picture collect ghost <type> <x> <y>`: the game's Create
and preview, as the build menu and the mouse; -1 only moves them): a house, an airport, a granary and barracks at the
camera, each moved four times, 0 differ - EE 136 132 records in 36 614 fills, AoC 123 361 in 32 758; the space types'
lift not met.

Step 3, the world's copy (collect_copy.c, 2026-11-08; the pipe: `picture collect copy off | on | check`, off by
default). The fills read the world no more: what they read of it - per tile of a region its sight for the player, the
objects on it in the walk's order (an object of parts from its first part on), the buildings and trees remembered there;
per object its values, its complex unit's (its physical model byte for byte: pose, model, scale, frame - the view test
asks Intersects of the copy), the units inside's bars, its part values and two lists, its goal's value, a path marker's
target place; per doppelganger its virtual functions' answers - is copied into a copy of the region (uc_copy), and the
walk and every fill read that, the rest the game's data (types, graphics, players' colours and names, the ground) or
the frame's (the camera, the selection, the unit under the mouse, the interface's settings). The halves are the copy
(cp_obj, cp_dopp: the extract) and the fills (the build). With the copy off the walk makes it of its view itself at
its start, under the frame's read lock. With it on the simulation makes it when its loop lets go of the world's write
lock after its steps (render_3d.c rd_hook_write_unlock, the lock's caller the loop's: spec engine_display.
step_write_return) - of the frame's last view and 4 tiles round it -, and the walk takes it when it is for its view,
player, free camera and show-hidden mode; any other writer (the interface's, the lobby's, a load: 70 places in the exe
take the lock) only makes it stale, and the walk then copies the world itself (it holds the read lock). So whenever the
walk reads a copy, the world is as it was copied: the objects' pointers in it are as good as the game's own walk's (the
picker, the met sets, the ambient sounds, the selection's groups take them; the fills never read through them). Three
copies: the newest, the one the walk reads, one being made. update_hidden ("show hidden units" 1) writes to the unit:
the copy asks it, for an obstacle or a moved unit of the region, where the game asks it for those it draws (the
simulation calls it itself when a unit moves; it reads only the game time and the map).
Checked (copy check: at every walk the steps' copy against one made of the world then - every tile, object,
doppelganger and list byte for byte, but update_hidden's state, which the copy's own call changes), 0 differ:
the 8000 save 817 walks (4.2 million objects), as its player 2 917 (4.8 million), an EE Continental game as
player 2 after 5 minutes 19 124 (14.4 million), AoC 20 412 (13.8 million). With the copy on the fills held against the game's beside it: the view test 0 differ of 1.8 million objects
(the 8000 save), 9.3 million (EE Continental, player 2 after 5 minutes), 6.5 million (AoC); the trees', doppelgangers'
(3.5 million, 2.3 million) and ghosts' fills 0 differ; the opacity 1, 6 and 8 differ of millions - update_hidden asked
twice (the copy, then the game's call a step's time later in the check; ours alone asks it once). Paused, the prints of
the game's walk and the mod's from the steps' copy alike in the objects' records, their vectors and labels, the picker
and the sort maps (the 8000 save as players 1 and 2, EE and AoC Continental). Each game then 30 s at speed 3 with all
of the mod's. The walk took the steps' copy at most walks; other writers made it stale 0-4 times in a game. What it
costs the simulation: 3-8 ms a step in the 8000 save (3800-11 000 objects in the region), 0.6-0.8 ms on a Continental
map (900) - off by default till it is measured against what it buys (step 4: the units drawn during the steps, moved
between them). Since 2026-11-08 2.1-2.4 ms (the 8000 save, 6200 objects) and 0.24 ms (Continental, 775): cycle counts
per part of the copy found the time in build_is, which compared the build's name with lstrcmpA (the user's locale,
about 100 ns) 4-6 times an object - strcmp now. A raw copy (memcpy of each object and its complex unit, no fill's
reading) measured beside it is no gain: of the view 1.6 ms, of the whole map 2.9 ms (the 8000 save: 7.4 MB, 12 000
objects) against 2.2 ms, and it would leave out what the objects point to (goals, the two lists, the units inside, a
tile's sight), which the frame couldn't follow while a step runs. The region is the bounding box of the view's rows:
copying only the rows and their margin is the next saving (the box holds about twice the objects the rows do).

Step 4, the moving between steps (collect.c uc_smooth, collect_copy.c cp_pose_swap, 2026-11-08; `[display] units_smooth`,
default off, which turns the mod's walk, fills and the steps' copy on with it; the pipe: `picture collect smooth off |
on | probe`). The frame draws a moving object at a game time of its own - the pieces' clock (`pt_clock`: it runs on
with the real time at the rate the game time goes, drawn toward a step and a half behind the newest step, never past
it) - between the two of its last three poses whose game times hold it; at or past its newest (a pause) it stands as the
game has it. The poses are kept with their game times by the object's pointer in a table of the simulation's (a copy of
the same game time as the last - Art of Conquest's writes without a step - replaces the newest; a frame's own copy reads
them, never writes; an object not in the last copy has none to come from). Until 2026-11-10 the way was the real time
since the last copy over the time between the last two: the units stood whenever a step came late, and until the next
one when two came at once (their copies 0.2 ms apart) - 12 frames at a time in a recording with a step load of 30 ms -
and a frame's own copy (after another writer) drew them as they stood. The record's transformation is blended number by number where it differs (an unchanged -0
stays -0: a still object byte for byte); not across a jump of more than 4 tiles (a unit set down, teleported), and a
turn of more than about 25 degrees (the first column's dot product under 0.9) only moves, because blended matrices
shrink on the way. The picker takes the blended pose - what is clicked is where it is drawn; the view test keeps the
copy's. The animation goes the same way (`uc_anim_between`, 2026-11-09; the pipe: `picture collect anim on | off`): a step moves a walking soldier about 3 of its 15 frames, four poses a pace, so the frame draws frame + fraction blended between the two copies by the same alpha, the same animation only, running on by at most half its length. An animation may hold each pose for several of its model's frames (a dying artillery piece for 2: frames 7 and 8 share a vertex array, 9 and 10 the next; next_frame = frame + 2): the fraction is then the way from one pose to the next, so the place is counted in poses (`uc_anim_between`) and the drawn frame is the pose's first, its next frame the next pose's. Counted in frames, half of each pose's time blended between two identical arrays and stood still. What a dying mech or machine throws (a Minotaur's torso, shards) is no part of its model but an effect, EEE3DParticle in the fifth queue, moved once a step (by the game time gone since its last move, stamped with that time: moved_ms) and drawn from three plain things (the place, the angles, the fade): `pt_hook_draw` (collect.c) draws them at a game time of their own (`pt_clock`): it runs on with the real time at the rate the game time goes (measured over half a second of steps, collect_copy.c `cp_clock_moved`) and is drawn toward a step and a half behind the newest game time, never past it, so a late step or two at once only bend its pace; a piece keeps its last four moves and is blended between the two whose stamps hold that time, seen new at the draw, for it isn't in the copies of the world. Blended by the real time between its changes (39d8ee4) it kept the steps' unevenness, and at the units' time (8c70134) it stood one to three frames whenever a step came late (at the normal speed a step is about 23 ms, up to two run at once); a frame during a step still shows the kept picture of the last real one. The flames and smoke (EEE2DParticle, sprites turned to the camera) are made and moved the same way and blended the same (`pt_hook_draw2d`: their place, turn and fade - the size and alpha); the game makes a new one in the memory of one just gone, so a piece whose fade went down is a new one (blended from the old one's moves it was drawn grown and swept across the gap). The table holds 16384, looked up over 8 slots. A model also moves by a transformation per frame (GEModel frame_transforms), which the engine's PrepareTransforms takes from the frame alone: a part that hangs on a model flies by it - the Minotaur's top thrown off in its death rises and turns half over in frames 13 to 20 - and stood a frame and jumped to the next (at a quarter of the game's speed a tile, then still for a second). The exe calls PrepareTransforms through one import slot; `mt_hook_prepare` (collect.c) does the same with the frame's transformation blended to the next one's (the turn by its quaternion, the scale and the place straight), at the place the frame, next frame and fraction give, the fraction counted from the pose's first frame (the frames before it that share its vertices): the game's own values of a frame (14 at 0.55) and the walk's (13 at 0.55) are the same place. Where nothing is blended it is the engine's to the last bit (checked against it on 200 000 calls). Past the last frame the model blends toward what the game's next frame says, the first in a loop, itself in an animation that ends (2026-11-10, the player: "the death animation of attilery ... stutters, and at the end it reverts to the begining"). probe keeps the poses and counts the steps on the screen of one moving
object, nothing moved: the frames where it didn't move and how even its steps are (rms over mean, 1 is even). A moving
unit's step per frame on Continental at speed 1 (about 8 frames a step), probe -> on: EE 88-93% of the frames without
a move -> under 1%, unevenness 3.1-4.1 -> 1.3-1.7; AoC 87-90% -> 0, 3.1-3.6 -> 1.6-1.9. Paused (time up), the game's
walk and the mod's alike in the objects' records with it off, probed and on.

Step 4, the units' update during a step (collect.c collect_during_begin, render_3d.c rd_hook_ro_units_update,
2026-11-08; the pipe: `picture collect during off | on [hand-off ms]`; `[display] units_smooth` turns it on, hand-off
250). A frame during a step ran no units' update: the units stood till the step's end. It now runs from the steps'
newest copy, which needs nothing of the world - the game's update with what would read the live world left out: the
ground's pass (the region stays the last one's: the ground isn't updated either), the two picks (they read through the
picker's objects, and the mouse's takes a reference on one; spec: units_ground_pass, units_pick_rect, units_pick_mouse),
the picker's reset and adds (a running step may free an object of the copy, and the add would reference it: the
picker stays the last full update's, so a click picks from that), the ambient sounds; an object is drawn once by a
stamp of the walk in the copy (the picker's `listed` no more), the unit under the mouse is the last full update's
(only compared), the selection's group the copy's (uc_obj.in_group, asked for the player's own units when the copy is
made). All the fills must be ours (the game's read the world); not with the free camera. Only beside the simulation's
own step: every other writer (the interface's, a load, which may free the map) counts itself before it takes the lock
and waits for such an update to end, and the update doesn't begin while one is counted (g_rwl_nonstep, g_uc_in_during:
one of the two sees the other). The ground and the shadows' update still wait for a full update, so the
hand-off stays, only later: the picture goes 250 ms (the pipe's ms) without a full update first, not while the camera
moves (the ground must follow it). Checked: with the steps held 150 ms (stepload), speed 1, AoC Continental, a moving
unit in 25 s moved in 89 and 93 frames without it, in 16 164 and 18 955 with it; 41 551 such updates in AoC and 8435 in
the 8000 save at stepload 200 under selection boxes, clicks, orders and the mouse over units without a fault; the
picture of such a frame (11 000 units; a selection's bars and panel) as the game's own when paused beside it. What it
does not buy: speed. The 8000 save at speed 4 alternating a minute each, the game's walk against this: 79.2 / 60.0 /
58.6 game seconds against 76.2 / 56.0 / 62.1 - the hand-offs' wait falls from 6-8 s a minute to 3.5-5 s, and the
copy (4.3 ms a step of 11 000 objects there) costs what that saves. Its gain is the picture: units that keep moving
through long steps.

Step 4, the effects (collect_copy.c collect_fx_step, 2026-11-08; the pipe: `picture collect effects off | on | check`;
`[display] units_smooth` turns it on). The frame draws the effects from five queues (engine_display.effect_queues) that
EEROEffects' update remakes: every effect under the view's rows is asked its PrepareQueue and Queue, and Queue is the
game's own buffer - it copies what the steps wrote (age, frame, transformation) into the fields the draw reads, and
reads the unit the effect belongs to, which is why that update crashed when run beside a step (a step can clear the
effect's unit). A frame during a step skipped it and the effects stood. Now the simulation makes the queues at a step's
end, inside the world's lock, into queues of its own (the game's rows and order; Queue takes the queues as its
argument), and a frame during a step takes them in place of the ones it drew. References are plain counts, so they
are only taken and given back inside the lock: the queues a frame replaces are retired for the simulation to release
at its next step's end; a full update finds them or the game's own in place and releases them under its read lock as
ever. Queue's writes to the draw fields can meet a draw of the same effect (a torn number for a frame; no memory is
moved). At the scene's release (EEReleaseScene, spec scene_release) what the simulation's queues hold is given back
first, and queues of another map are never drawn or released. Checked (check: after a full update, when no other writer
came and the camera is as when they were made, the simulation's queues against the game's, entry by entry): the 8000
save 730 updates, 0 differ; EE Continental 25 475, 0 differ; AoC Continental 24 428, 1 differs (right after the test
moved the camera: 4 effects against 0 - the camera was set and the rows not yet remade, it seems; not proven). Each
then with the steps held 200 ms and at speed 5 for a minute, without a fault, 170-2900 queues made and taken by frames.
It costs the simulation 0.05-0.13 ms a step (up to 600 effects). Not run: a load or an end to the menu with it on (the
pipe's quit ends the process; nothing in the tests reaches the scene's release; the player tried both: they work).
A finished effect is the catch (the player's picture, 2026-11-08: muzzle flashes standing on their last frame for
good). Its Step ends it and the manager gives back its two references, but it leaves its map region only when the last
one goes. The game's update releases the old queues before it makes the new, so one only they held is gone by then;
the simulation makes the new ones while the frame still holds the old, and queued such an effect again, for ever. It
now passes over an effect nothing but the queues in use holds (its references against a set of those, the frame's and
the retired). The check had not seen it: the simulation's own references kept the dead effects in the regions, so the
game's update queued them too. The same EE Continental game, the old build against this: 488 effects queued a step at
speed 5 against 12 (641 finished ones passed over); the check 36 901 updates, 2 differ (9 effects against 1, a test
camera jump as above), the 8000 save 850, 0 differ.

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

## The interface's text without reading the picture back

2026-11-06 (the 8000-unit save ran at either about 3200 or about 1900 frames a minute, by game, whatever the build).
The minute's line now breaks the frame into its parts - `# render: a frame N ms: the render objects' draws, the
update's read holds, the lists to the GPU, Present, the rest; reads back from the GPU N a frame, N ms`. The slow games'
rest was 12-15 ms against 1, and they read the picture back from the graphics card 8 times a frame (6-9 ms in the
reads); the text log (pipe `textlog`): those games drew the epoch's name and the resource numbers with GDI every frame,
the fast ones never. In some games the interface redraws its bars every frame (why: below), and each text phase
(display_text.c's own text DC) locked the target twice, to load the tiles under its text and to store them: a read
back that waits for the card to finish the frame so far, so the card's time and the frame's added up.

Why some games redraw (2026-11-07): every frame EEGraphics (0x4FED2D) calls UIManager::RedrawAll unless the
rasterizer says the screen keeps its back buffer (GetDirtyRectangleSupport: a byte of the display DLL's screen object,
+0x71). DX7Rasterizer::ActivateTheScreen sets it - yes but for two old cards -, DX7TnLRasterizer's own (the T&L DLL,
EE's and AoC's alike) never does, and the screen object comes from operator new: the byte was the heap's left-over.
Found with the exe's calls of RedrawAll hooked (a slow game: 2486 a minute, all from there; the fast ones none) and the
byte read in running games (00 in the slow one, 03 in the fast one - a value no code writes). [patches]
interface_redraw (display.c dirty_rects_loaded): the T&L rasterizer's vtable slot of it answers yes, as DX7Rasterizer
does for any other card. Three pairs of games with it: the byte 00 in three of the six, all at 56-58 fps and 0.05 text
phases a frame; the resource bar's population count, shot 10 s apart, redrawn clean. The pipe's picture now reports
the frames, text phases and reads back so far (two replies a few seconds apart: the rates).

The T&L rasterizer always (2026-11-07, the player: "the game defaults to non T&L and most won't know"). The video
options offer Direct3D Hardware (the game's default: its DLL transformed and lit the vertices on the CPU) and Direct3D
Hardware TnL (the card did). The own renderer's vertex shader does it either way; what is left between them is each
DLL's own code, and the renderer's work is made for the T&L DLL (the units' animation on the card hooks only its
copies). So while the own renderer draws, the display's start (0x5392E9, AoC 0x5444D0) takes the T&L rasterizer: its
one comparison of a rasterizer's name with the setting's (spec game.settings_rasterizer_name, through the exe's import
of UString's operator==) answers by the candidate's DLL, and the setting takes its name (display.c hook_ustr_eq). With
the setting on Direct3D Hardware: EE and AoC log the T&L rasterizer taken, the 8000 save's units blended on the card
(16 110 draws a frame), the picture as before. A wrapper's games keep the player's choice.

Now (picture text on | off, the pipe; on): on the own renderer's target nothing is locked. Each tile the text touches
starts black in the text DC's DIB and white in a second one, each text output is drawn on both (the second DC given
the first's font, colours, alignment, origin, position and clipping), and at the phase's end the box of its tiles goes
to the renderer (render.c rd_text_layer): one texture with both, drawn onto the target with a second pixel shader
output as the target's factor - GDI's text over a background x is b + x * (w - b), for any colour, fill and smoothing
that blends linearly. Measured in one game redrawing its bars every frame (paused, the mouse placed on the resource bar
by vmouse, shot, resumed): 33.4 ms a frame with the locks (8 reads, 9.0 ms), 23.3 without (no reads) - the frames of
a game that doesn't redraw them took 17-19 ms (a rest of 0.4 ms), so the redrawing still costs about 5 ms of the
game's own. Picture: the two bars, paused, against the locks' - at most 1 apart, at 578 and 137 of their pixels (the
text's smoothed edges).

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
- Anisotropic filtering (`[display] anisotropy`, off, x2, x4, x8 or x16, default x16; the pipe: `picture aniso <n>`): the game's samplers
  with linear minifying and mip levels are anisotropic (rd3_mk_samp, the level in the sampler key's bits 20..24). The
  ground at the camera's slant kept its grass far into the picture instead of a blur.
- FXAA (`[display] antialias`, default on; the pipe: `picture aa on | off`, `picture edge <0..1>`): the simple form
  (Lottes's four diagonal neighbours), once a frame at the first of the interface's draws after the 3D scene's
  (rf_replay) - the scene's viewport copied into a texture and drawn back through the filter, the interface's text and
  panels drawn over it after. A pixel is an edge where its neighbours' luma range is at least half of the brightest's
  (picture edge 0.5): at 0.25 the grass texture's own grain counted, 42% of a patch of grass changed and it looked
  blurred; at 0.35 9%, at 0.5 0.1% - the wall's stair-stepped top edge still smoothed (24% of its pixels changed).
  Checked on "choke wolf" in the same paused frame, pixel counts by scripts and zoomed shots.

- The silhouettes fade with the distance behind the face they appear on (2026-11-10, the player: "is it possible to fade silluetes outt with distance to the face they apear on?"; `[display] silhouette_fade_start` 3, `silhouette_fade_end` 12 tiles; the pipe: `picture silfade <start> <end>`): per pixel, the unit's depth and the building's there turned back into distances along the view. The face is the one the pixel shows, so a unit just behind a large building is measured from its front.
- Cloaked units (2026-11-10, the player: "hidden unist have shadows, so the shadows give them away"): a cloakable unit below full opacity casts no shadow, is not mirrored in the water and has no silhouette behind a building; the fill tags its record (collect.c, SIL_CLOAKED), so this holds with the mod's own fill (`units_smooth`), as the silhouettes do.
- The interface and the filter (2026-11-09, the player: "the panel is blurry all the time ... it gets blured, builds up, and than resets again", "the lines of shift write into the interface and tay there"). EEROUserInterface's clear (0x50DF0D) sets the depth to 0 under the unit panel and the minimap (at 2560 x 1440: 0,1032 - 435,1302 and 2118,1032 - 2560,1302), so the 3D doesn't draw there and what was drawn stays; the scene's viewport holds those rectangles and the filter went over them every frame. A pixel of depth 0 is left alone now, in both forms of the filter. The filter runs after the list's last 3D batch; it used to look for the first batch that is no longer 3D, which the silhouettes' unbinding hid when no water followed them: on land no frame was filtered. What the 3D phase writes outside its viewport is put back after it (`rd3_bars`): the bars are only drawn when they change, and the lines of orders drawn in a frame during a step come with the interface's whole-screen viewport.

## Ambient occlusion

engine/render_ao.c, 2026-11-10 (`[display] ambient_occlusion`, default off until the player has seen it, `ambient_occlusion_radius` 60% = 0.6 tiles, `ambient_occlusion_strength` 100%; the pipe: `picture ao on | off | view` - view shows it alone -, `aoradius`, `aostrength`, and `picture ao` tells the passes and the last time on the graphics card): the picture darkened where things stand close, so that units and buildings stand on the ground instead of lying over it. Scalable Ambient Obscurance (McGuire's) from the scene's depth, full size, 12 samples on a spiral turned per pixel, blurred along both axes without crossing a jump of the depth, multiplied into the picture. A sample's weight is gone at the radius, so a soldier far in front of the slope behind him leaves no halo. The view's place comes from the depth by the projection of a batch of the units (`d = A + B / z`, the ground's view is another one), the radius in tiles. It runs after the last of the units' batches drawn in their place (rf_replay, before the silhouettes): all that writes the depth is there, and the water's surface, what is held back for after it, the silhouettes and the interface stay as they are; what was drawn before it and writes no depth (over land a selection ring, a health bar) is darkened with what lies under it. A pixel of depth 0 is the panels', of depth 1 nothing. The ground doesn't darken itself (the player, the same day: "is it possible to stop teh terrain from casiting ao on itself?"; the pipe's `picture aoself on` for the comparison): the depth buffer can't say who wrote a pixel, but the ground is drawn first, so the depth is copied after its last batch (`rd3_ao_ground`, only the map's own ground: Art of Conquest's space maps darken themselves as before), and a pixel and a sample that both still equal that copy, within 0.005 tiles, are ground on ground and skipped. A unit or a building still darkens the ground, and the ground the unit. A tolerance of 0.03 tiles took a wall's lowest pixels for ground, a bright line at its foot. Only a sample that would darken the pixel reads the copy: every sample reading it took the pass from 0.40 to 0.82 ms, this way 0.43. Checked headless on a copy of "choke wolf" (2560 x 1440, dynamic shadows; water look, reflections and soft edges on in part of it), the same paused frame off / on / alone: the bunkers' and tanks' feet, the wheat's rows, nothing on the water's surface, smoke or fire, the panels whole; the closest zoom (0.45) the ground without blotches; a frame during a 1.5 s step with a 3-tile pan. Close up the radius in pixels reaches its limit (a 15th of the picture's height) and is less than the radius in tiles: the weights go by the radius really sampled, or all the samples lie near the pixel at the full weight of the larger one (the player's close-up, the same day: the steps and the columns' feet black, "in horizontal view up close it way to intemce"; zoom 0 on the same save, old against new against none: an AA gun and the wheat nearly black, then close to none; the usual zoom unchanged). The pass takes 0.40 - 0.52 ms a frame on an RTX 3080 (a timestamp query; its cost is per pixel). Art of Conquest not run; nothing in it depends on the build.

## The ground's borders

render_ground.c `rgb_height`, 2026-11-10 (`[display] ground_blend`, default off until the player has seen it, `ground_blend_noise` 50%, `ground_blend_soft` 25%; the pipe: `picture gb on | off`, `gbnoise`, `gbsoft`, `gbscale`; the player: a height from the colours for the blending, normalized per texture): a layer of the ground fades between a quad's corners, a soft gradient along the tile grid. With this its high texels come first, its low ones last. A texture's height is its brightness a little blurred (twice three texels each way, wrapping) and ranked, the share of its texels darker than this one, so that at any fade that share of every texture shows, bright sand and dark rock alike; made on the CPU once per texture when a frame first wants it, an R8 texture with mip levels (44 textures in 11.5 ms on "choke wolf"). The shader mixes it with two octaves of noise over the map, because the textures repeat and their heights alone would follow the grid; `a = saturate((a (1 + soft) - (1 - h)) / soft)` keeps 0 at 0 and 1 at 1 whatever the height, so that neighbouring tiles still meet. The colour too goes by the texture (the player's desert, the same day: "the cliffs sorounded by yellow tinted dessert sand have that white border"): a point of the map has one cell, so one texture and one colour of the table; the desert's sand is nearly white and made yellow by that colour, and the colour between the points was the mix of a cliff's and the sand's, near white next to the cliff. The old fade hid it, a ragged border puts sand there. So each pass takes the colour of its own corners only (`o->w`, 1 where the corner's cell has the texture; the vertex shader's `gt`, the table's colour times that and that, and `gl`, the light and the fog of war as before; the pixel shader `gt.rgb / gt.a * gl`). Only with `ground_blend`: the game's look stays otherwise. Under the level (the vertex's height, a band of 0.005 below 0) both are the game's, the fade and the colour: the seabed's depth is painted in its colours, and a ragged border showed through the water as streaks (the player, the same day: "anything under 0 to fallback to the games blending"). A band of 0.05 above the level took the borders of island land too, which lies within hundredths of it. Each pass sees only its own texture and not the one it covers; comparing both would need all of them in one texture array. Only a layer's pixels in their fade pay for it (`gnd.x == 1`, the pixel shader's b3 and t13 are bound in `rg_replay`). Checked: "choke wolf" at the usual zoom and at 0.4, grass against rock and beach sand, no seams along the tiles, a frame during a step. Not measured: the frame's cost. Not run: snow, Art of Conquest, the editor.

## Shadows

The player, 2026-11-04: dynamic shadows, cast on the terrain only - the models are low-poly and shading on them
would show it (on everything since 2026-11-05, below). `[display] shadows = dynamic` (default `game`; the pipe: `shadows game | dynamic [strength]`, the reply
tells the last map's casters, ground batches, box, sun and shadow colour), engine/render_shadows.c:
- FXAA by the depth (`[display] antialias_depth`, default off; the pipe: `picture aadepth on | off`, `aajump`, `aacontrast`, `aasub`, 2026-11-09): the filter of Lottes's quality form (the edge followed to its two ends, what is thinner than a pixel blurred by half) with the threshold from the depth, as SMAA's predication does it. The scene's depth is copied at the filter. A flat surface's depth changes evenly across the screen at any slant, so what isn't even across a pixel against the distance there is an outline or a fold: there a luma range of 0.05 is an edge, elsewhere what the simple form wants (half of the brightest). The choke wolf copy at the usual zoom, pixels changed against no filter: brick 14.3% with the simple form, 4.5% with this, wheat 6.5% and 2.9%. A jump of 0.005 or 0.002 instead of 0.02 changed these hardly (tanks 4.1% to 4.8%): where something meets what it stands on the depth is continuous, and it stays the colour's to see. The water and what writes no depth (the soft edges) have only the colour.
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

## The water (2026-11-08)

`[display] water_look` and `water_reflections` (dll/engine/render_water.c, the pipe's `water`; both need dynamic shadows, the first the hills too).
The game draws the surface at one alpha over the seabed, and paints the depth into the seabed's vertices (turquoise shallows, dark deeps): that colour is the look, so it stays.
- By the depth: the hills' map holds the ground (its ceiling less its second number), so the surface's shader knows the depth under a pixel. The surface is thinner in the shallows and thins out to nothing over the last 0.05 at the shore; the seabed's texture goes toward its last mip level with the depth.
- Tried and dropped: the surface thicker with the depth (the sea went one flat blue with more white caps); the ground darkened at the waterline by its height (land on an island map lies within hundredths of the level: whole tiles went dark).
- Reflections: the units, buildings and trees on the screen drawn again mirrored about the level into a picture of half the size before the list, the surface takes 35% of it (moved by its ripples), none at the shore and all from 0.4 of depth on. Tree cards (SH_CARD) are left out: they are stand-ins for the shadow. What lies off the screen is in it too (2026-11-10, the player: "the culling loses reflections taht are just out of screen"): mirrored, an object beyond the top of the screen comes down onto it. Such objects were in the list already, for the shadows (SH_AWAY, a tree's SH_GONE: the collection's rows and the tests of the view passed, `shr_hook_rows`), only this pass left them out; the pipe's `water away off` for the comparison. The ground from the map is mirrored too (what the list holds of it; what lies under the level is left out, like anything else).
- A ship's wake under the surface (2026-11-11, the player: "the wave circle behind ships is suposed to render under the water surface, not over it"): the rings are a see-through decal of the units' pass (SH_DECAL, 6 indices a ring - found by logging the see-through batches while ships moved: 132, 138, 144 indices), split by height like the other see-through batches, and lying at the level they were all "not under" (the split 0.05 below the level, for a move's circle) - drawn after the surface. A decal of the units' pass splits 0.1 higher (render_frame.c rf_water_split, wsp.z; render_water.c rd3_water_high): the rings come before the surface, which covers them as it covers the seabed.
- The seabed's wobble can be switched off (2026-11-10, `[display] seabed_wobble`, default on as the game draws it; the player: "add an option to disable the terrain wobble underwater that tries to fake refraction"): the ground deeper than -0.5 moves by sin(x y + phase) times the rasterizer's seabed strength (`TransformToCamera`, render_3d.c's `ground`); off, render_ground.c hands the ground's draw a strength of 0. The surface's own movement stays.
- The water by the game time (2026-11-10, `[display] water_game_time`, default off; the player: "water surface and caustics should probably respect game time?", "add it as an option"): the game moves its water by the real time, paused or not - the wobble (the exe's ground update 0x503A7F hands TransformToCamera the `TMGetTime` gone) and the animation's frames (`UpdateWaterAnimation`, `TMGetTime`); paused, with the game's own ground and water drawn (`picture ground off`), 37% of the water's pixels changed in 2 s. With the setting render_ground.c advances the wobble's phase (and so the caustics) by the game time - the real time at the rate the steps measure (collect_copy.c `g_cp_rate`), none while paused, else the game's speed setting - and draws water group j with the texture of group j + ours - the game's frame counter (`of_mesh_water_frame`): the groups hold the animation's frames in turn, so that shows frame ours + j, ours counted in the game's ms a frame (`of_mesh_water_frame_ms`). The game's own animation goes on underneath, unchanged.
- Caustics (2026-11-10, the player: "could our water throw casutics? (a cheap version)"; `[display] water_caustics`, default 60%, the pipe's `water caustics <0..2>`; their size and pace `water_caustics_size`, `water_caustics_speed` (factors, 100% as before), 2026-11-11 - the pace a clock of their own, `gcc.x` of the ground's constants, at 100% the wobble's half pace as before): the light the surface's waves focus onto the seabed, drawn by the ground's pixel shader on the ground under the level at its own place (the player: "instead of bothering the water shader, should this maybe be part of the terrain renderer? blur and fogging come than for free later"): the water's step fogs it by the way through the water as it fogs the seabed, the shadows on the seabed darken it, and whatever the step does to the seabed later (refraction, blur) does it to the caustics too. The surface covers about half of the seabed (its vertices at alpha 128, the water look's 0.75 to 1 over that), and what the surface drew before showed at that same share - so the same strength. Hulls under the water no longer catch it. The pattern ("to uniform", "in realithy they move in one direction, not random cells"): eight waves within 0.66 of a radian of the wind's way (render_water.c RW_CAUSTICS_WIND, along the map's x: along the screen's axes the lines read as streaks), each as curved as the others and as fast as deep water's waves of its length - all run downwind. A ray of sunlight moves by the depth times the slope where it enters; `cwl` finds the place it entered by three steps back and takes the inverse of how much the rays round it are squeezed (the determinant): a line where they fold. The light moved, not added - over the mean on the lines, under it in the cells; red bent a little less than blue. The depth sets the focus: soft at the waterline, sharp from half a tile down, never past 0.35 (the steps miss beyond: blots); less the deeper (exp of -0.8 the depth). None at the waterline, by the shallower of the ground's own depth and the hills' map's - the surface thins out by the hills' map, and by its own depth alone a line lay on sand the surface no longer covered (a white streak on the beach). Gone where a line is thinner than a pixel. Prototyped in numpy against the rays themselves counted on the seabed, and as the tilted camera sees the pattern. Cost: 32 sines and cosines and a hills' map read a pixel of the seabed (not in the mirrored picture). Tried: drawn by the surface - first of the point straight below it ("very obvius that they are projected from the water surface" in a low view), then of the point behind it from the water step's copy of the depth; on the seabed at first, by the cells (the surface seemed to hide it; why is not known - drawn as now it shows); contour lines of two noises (loose wavy lines); a net of moving cells bent by a sine, then bent and broken by noise ("too uniform"); a second, smaller net over the first (cracked glass); only the light over the mean (a few long folds, no web).
- The fog: what lies under the level takes the fog of the place its line of sight enters the water (the game's own again toward the fog's end). Tried first: no fog of its own under the level and a surface more opaque in the fog - a hard line at the water's far end and orange streaks on beaches, where ground under the level has no surface over it.
- The way through: `[display] water_opaque_depth` (default 20, the editor's height numbers: a height over half a height step, 0.204 tiles), a step over the picture. Before the surface's first batch of a list the target and its depth (D32_FLOAT, whole subresources: a depth buffer can't be copied in part) are copied, and each batch of the surface is drawn once more first, unblended, without writing depth: its pixel is the copy's colour toward the deepest of the map's underwater colours (the last of the mesh's water colour set) by the way from the surface to what lies behind it - the two depths' ratio along the line of sight, `d = A + B / z`, A from the list's matrix (`rw_depth`), B falls out - wholly at that way, and wholly where nothing was drawn behind it (depth 1: the edge of the world, past the game's draw distance). The colour is multiplied by the surface's vertices (the light, and black at what isn't explored) and takes the surface's fog. The surface goes over it as ever. Tried first, each face under the water fading by itself: the table by depth made things stand out (the player: one colour), and a light of its own from the vertices flickered at the edge of what is explored. What is see-through and writes no depth (a move's green circle on the seabed, smoke) leaves nothing in the depth the step reads, so those batches of the units' pass are drawn after the list's last water batch (`rd3_water_late`; the pipe's `water late off` for the comparison). `[display] water_fog_color` (red green blue, 0 to 255; the pipe's `water color`) puts a colour of its own in place of the map's deepest; empty, the map's.
- In front of the surface or behind it (2026-11-10, the player: "the confirmation cicles are drawn behind the water surface, as do the healthbasr of selected units, and on smoke etc, id like them to draw behind the water surface when they are under it"). The surface writes no depth, so the order of the draws alone decided: a move's circle (the ground's pass, at the level) and the health bars (the units' pass, in pixels) came before it and lay under it, smoke (the water's own pass, after the mesh) lay over it from wherever it started. While the water's step runs (`rd3_water_last`; the pipe's `water late off`: the game's order), rf_replay draws the whole surface's step first, then what is see-through and writes no depth in the part under the level (more than 0.05 below it, per pixel: `wsp` in the shader, its own constant buffer b5), then the surface, then the part above it. What the 3D passes draw in pixels (health bars, the lines of orders) comes after the surface and the water's effects. The circles on the ground (a move's, the selected units') are drawn at depth 0 by the viewport's depth range and rely on what comes later to cover them: drawn after the buildings a move's circle lay over the building in front of it, so over the land they stay in their place (the hills' map says which pixel is over the water).
