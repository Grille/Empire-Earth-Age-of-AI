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
  it: in the shadow below it, a soft edge of 0.25 tiles, a bias of 0.05; the darker of it and the casters' shadow - the
  ground, decals, water, and with shadows all on the units, buildings and trees (a tower's foot in a hill's shadow, its
  top in the sun). The terrain's second depth map is gone. The pipe: `shadows hills on | off`, the reply tells the
  map's size and its last making. Checked: "choke wolf" at 8 and 17 o'clock (the light 24 and 12 degrees), hills off
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
- Under the water a shadow fades out with the depth (the player, 2026-11-05: none under water, ideally fading): gone
  0.8 under the level (the pipe: `shadows fade <depth>`). The level is the water's vertices' mean height each frame (in
  the world through the units' camera); measured on "choke wolf": the water at 0.00, the deepest seabed at -0.84 (rprobe
  logs each draw's view translation and its heights in the world now) - a fade to -2 the player asked about would leave
  60% of a shadow on the deepest seabed. The water is drawn half see-through (vertex alpha 0x80): a shadow on deep water
  is half of one on land, on the shallows by the beach nearly all.
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
