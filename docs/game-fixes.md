# Display, interface and engine fixes

What the mod changes in the game itself, for every player and the AI alike, and the `ageofai.ini` keys that switch
it. Each key's own explanation is in [config/ageofai.ini](../config/ageofai.ini) (and in `ageofai_config.exe`); this
page says what was wrong and what the fix does. A setting marked "in multiplayer every PC needs the same setting"
changes the simulation: in a lockstep game different values make the PCs' games drift apart (README "Multiplayer").

## Window and picture

- `[display] display_mode=windowed`: native windowed mode. The engine still contains SSSI's own windowed path; the
  mod switches it on and fixes what broke on current Windows (presentation, crashes, terrain, editor). The window
  has the game resolution from the game settings (scaled down if it doesn't fit the screen).
- `[display] display_mode=borderless`: a frameless window over the whole monitor, the game image scaled to it
  (aspect kept). Alt+Enter switches between window and borderless. The window is DPI-aware (`dpi_aware`), so
  Windows display scaling doesn't blur it; `smooth_scaling=1` filters the scaled image.
- `[display] renderer=auto` (both window modes): what draws the picture. Windows' own DirectDraw can't run the game
  in a window, so either a DirectDraw wrapper (`ddraw.dll` in the game folder) or the mod's own renderer on
  Direct3D 11 (`own`; screenshots, recording and the game's warning dialogs work with it - docs/renderer.md) does.
  `auto` takes the own renderer when the folder has no `ddraw.dll`, else the wrapper. Wrappers the mod was tested
  with:
  - [DDrawCompat](https://github.com/narzoul/DDrawCompat): fast (2560x1440: ~4.5 ms per frame vs ~10 with
    dgVoodoo). Put `CpuAffinity = all` into `DDrawCompat-Empire Earth.ini` next to the exe (its default runs the
    whole game on one CPU core). The window gets a classic (Windows 98 style) frame, which DDrawCompat draws itself.
  - dgVoodoo (DirectX pass-through is enough; GOG's wrapper): modern window frame, slower.

  With a wrapper, `[display] present=auto` picks the matching way to show the picture (DDrawCompat: its primary
  surface; otherwise Direct3D 11).
- The mouse follows the Windows cursor (your pointer speed), the window can be moved and switched like any other:
  another window in front takes the keyboard at once, the game cursor follows while you hover over the game in the
  background, and the click that brings it back counts as a click. `[display] lock_mouse` keeps the cursor inside
  while the game is in front: `auto` (default) only in borderless, so edge scrolling doesn't slide onto a second
  monitor, while a window stays free; `on` always, `off` never.
- `[display] draw_limit=4096` (default): the game drew at most 512 units, buildings and missiles per frame - arrows
  in flight or stuck in the ground included (a 360-archer fight reached 445) - so in big fights, and the more so at
  high resolutions, the farthest objects blinked in and out. `0` keeps the 512.
- `[display] frame_wait=auto` (default): the game's drawing thread slept at least a millisecond after every frame
  (about 15 ms on many PCs), which capped the frame rate. Now it only gives up its turn while the game draws (in a
  test about 290 -> 500-700 fps), and pauses as before once nothing has been drawn for 0.1 s (alt-tabbed,
  minimized), so it doesn't keep a CPU core busy then. `yield` gives up its turn always (tweaks.dll's
  SetSleepToZero), `game` is the original. Only the picture: the game itself runs on another thread.
- `[display] vertex_sysmem=1` (default): with hardware T&L the renderer kept its 3D models' vertex buffer in video
  memory, and every update of it made the graphics card finish its work first. The renderer without T&L already
  keeps it in system memory, and with T&L it's now the same (measured by DDrawCompat's author on a big map: 18 ->
  170 fps with DDrawCompat, 105 -> 315 without). dreXmod 3.4 does the same; `0` keeps video memory.
- `[general] skip_intro=1`: skip the intro movies.

## Scenario editor and camera

- `[editor] editor_resolution=aoc`: scenario editor at 1024x768 with The Art of Conquest's layout; `game` or any
  `WIDTHxHEIGHT` extends that layout to the resolution (map view fills the screen, panels at the bottom,
  minimap and file buttons at the bottom right).
- `[editor] editor_unit_search=1` (off by default until tried): a search box above the scenario editor's unit
  list. Type part of a name - as the list shows it, or the game's internal English name (`citizen` finds Bürger) -
  and the list keeps only the matching units of the category shown, as you type; several words must all match.
- `[editor] terrain_palette=1` (off by default until tried): the ground's colours are slots of a 256-colour table
  (`textures\ColorPalette.tga`), each terrain colour of the editor's list a run of them. The game's 54 colours hold
  255 slots, but 14 are the very shades of another in a place of their own (nine grass and sand colours one ramp);
  with this on they paint the first copy's slots - the ground looks exactly as before, in the original game too -
  and 96 slots are left free for a map's own colours (Art of Conquest: 28 of its 68 share, 96 free). A map's own
  colours travel in its file - a scenario, a saved game -, in a block after the game's own data that the original
  game doesn't read: there the ground on them shows the game's colours of those slots. The map's own colours come
  from the editor: the colour list ends in a strip of all hues - a click on it opens a dialog in the game's look (as
  ageofai_config.exe's) that makes a colour of the map's own (one
  colour, a ramp between two, or each slot its own, 1 to 16 slots, the colours from Windows' colour dialog), which
  follows the game's colours in the list and paints as they do; a double click on it changes or deletes it.
- `[camera] scroll`: `game` (default) the build's own camera scrolling, `aoc` The Art of Conquest's acceleration
  in either build, `ee` Empire Earth's constant speed in either build; `smooth=1` (default) moves the camera every
  frame by the frame's time instead of the game's fixed timer steps (jitter) - in both builds. While a text box
  has the keyboard (the editor's unit search, any of the game's edit boxes) the arrow keys move its cursor only,
  not the camera; a click anywhere outside the box (the map too) lets the keyboard go.

## Fights and sight

- `[general] lead_targets=1` (default): ranged units and towers aim where a moving target will be when the
  projectile comes down, not where it stood when it was fired. Before, slow projectiles (arrows, catapults,
  cannon balls) missed nearly everything walking across their line of fire.
- `[general] spread_missiles=1` (default): AA towers and other missile launchers stop wasting missiles on an
  aircraft the missiles already on their way will destroy; they take another target in range or hold the shot.
  `lead_missiles=1` (off by default) makes homing missiles intercept instead of chase.
- `[general] lob_each_shot=1` (default): archers, slingers, the pilum and the trebuchet shoot flat, and once a shot
  hit a wall in the way they lobbed every later shot, at 1.5 x their reload, until given a new order, even after the
  wall was gone. Now they look along the line to the target before each shot: a wall in the way gets the high arc,
  otherwise they shoot flat. The first shot over a wall no longer hits the wall either. Measured with
  tests/probes/wall_lob.eeai, 6 longbows against houses in a wall box: the same damage over the wall; after the
  near side is removed, 216 damage a minute instead of 144 (AoC 261 / 186). `0` = the original.
- `[patches] sight_counts=1` (default): every map tile counts how many of a player's units see it, but the game read
  only one byte of that count - a tile seen by more than 254 units took itself for unexplored, and in a big army
  random tiles with everything on them (units, trees) blinked out, then went to fog while units still stood there.
  Now the whole count is kept (up to 65000). `0` = the original.

## Walls

- `[patches] wall_slopes=1` (default): a wall piece stood flat at the height of one corner of its tile, so on a
  hill one end floated, the other sank into the ground, and neighbours at other heights stuck out of each other in
  steps. The game has sloped pieces, but each fits one steepness and they were hardly ever chosen. Now every piece
  leans along its length onto the ground under its two ends (its sides stay upright, as the game's sloped
  pieces do) with its bottom on the ground, the round posts on the ground at their tile's middle - the wall follows
  the hill in one line. Where the ground falls away beside or below a piece (a side slope, a post's lower corners)
  the wall's models reach further down instead of the piece being sunk, faced like the foundation of the tower of
  the wall's era (its stone or the player's colour), so its lower part stays in sight. Barbed wire, which has no
  such foundation, leans across a side slope as well (sheared: its stakes stay upright), its feet on the ground on
  both sides. Each piece also spans its tile exactly and reaches into a round post beside it: the
  game's pieces are longer than their spacing, so neighbours overlapped, their faces flickered, and on a slope their
  ends stuck out at every bend (the Copper Age wall and barbed wire keep their own length: the copper wall's stone is
  as long as the spacing, only its top reaches beyond, and the wire's coils are made to overlap). Walls built before
  are fitted when a game is loaded; a gate laid into a fitted wall keeps its own shape. In multiplayer every player
  needs the same setting. `0` = the original.
- `[patches] wall_joins=1` (default; only with `wall_slopes=1`): the game put a round post at the end of every
  stretch of wall it laid, where the ground stepped, and beside every joint - a wall drawn on from another had two
  posts side by side, walls crossing or meeting three or four in a cluster, and a post stood between a wall and the
  tower or gate it ran into. Now a post stands only where a wall ends, turns or branches: a wall runs on
  through, and into a tower or a gate. It is decided when a wall or a gate is built, as the game decides its posts;
  a destroyed wall's gap stays open. Both work in the scenario editor too: there the walls are fitted and joined
  as soon as they change - placed, deleted, moved, the ground under them painted. In multiplayer every player needs
  the same setting. `0` = the original.
- `[patches] wall_start_fit=1` (default; with `wall_slopes=1`): the walls a game starts with - a scenario's, made
  without the mod or with these settings off, and a game's saved before - are fitted and joined once at its start,
  as if built with them (a game saved with them on has nothing left to change). With `0` they stay as they were
  made; walls built during the game are fitted, and an old wall a new one is drawn onto is fitted with it. In the
  scenario editor `1` looks at every wall whenever walls change there, `0` only at those around the change. In
  multiplayer every player needs the same setting.
- `[patches] building_lean=1` (default): some buildings lean with the ground under them - the AA guns' type data
  says so (pitch and roll with the ground). The game leans a unit both ways only where it is put on the map whole -
  spawned, or the ground under it changed in the editor. A building laid out in a game never went through that: it
  stood level, its legs in the hill on one side and in the air on the other. The editor's own placing and moving
  lean it forward and back only (on a hill falling sideways one side sank in, the other floated) and its turning
  stands it level - only painting the ground under it put it right. Now a building laid out by a player or the AI
  leans as a placed one does and keeps it when finished; in the editor every such unit is stood onto the ground a
  moment after any change (with the walls' watch), and a game's start stands those a scenario saved half leaned.
  Ships and aircraft are left alone. Farms, mines, forage bushes and the ambient objects (debris, rocks, bones,
  wrecks, plants) stood level on any slope as well, half in the hill: they are now sheared onto the slope under
  them - their foot on the ground on every side, their upright parts (crop rows, stakes, lamps) still upright;
  trees and what doesn't stand on the ground (lily pads) stay as they were. Only their drawing changes: each frame
  the matrix they are drawn with (an ambient object or a mine has none of its own - the game makes one, level) is
  sheared by the ground under them (a farm), so it holds in the editor, in saved scenarios and in every game alike.
  A plane leaves a big object on a curved slope (4-tile debris over a ridge) floating at its ends, and a skewed
  matrix lights it darker: so an ambient object's or a mine's model is bent onto the ground as it is drawn instead -
  each vertex up or down to the ground under it (the highest within a quarter tile of the model's scale, so a big
  base triangle stays over a bulge), as the game lays a farm's ground mesh corner by corner (the renderer copies the
  vertices at every draw: they are put back after, the shared model unchanged). The smallest plants (a short draw record of their own) stay level. In multiplayer every player needs the same setting. `0` = the original.

## Selection, transports and building

- `[general] big_selection=1` (default): double-clicking a unit (all of its kind on screen) or dragging a box selected
  at most 63 units - with a large army only a part. Now every unit found: one message carries about 240, the rest
  goes after it as the messages a shift box sends (they add to the selection). The messages keep the game's own
  format and kinds, so they should stay compatible in multiplayer.
- `[general] transport_split=1` (default): units right-clicked onto a transport - a ship, a helicopter or a land
  transport (cargo truck, staff car, siege tower, Trojan Horse), mixed too, or a fortress. With transports of more
  than one type selected the game's command broke (it tried to load the clicked transport into another one, and
  nobody boarded); now the clicked one is the one loaded onto. The game sent every unit to the clicked transport
  alone - the first 10 boarded, the rest walked there anyway and were turned back at its side, and the other selected
  transports got nothing. Now the units are shared out: the clicked transport and every other selected one take the
  nearest units they can carry while they have room; units without room keep what they did (with one transport too).
  A ship gets a meeting point at the beach of its own (the game's was often the ship's own tile out at sea, so the
  units could not reach it, or the same spot for two ships). A yellow circle shows where each transport goes (the
  game's was a single one on the transport clicked).
- `[general] transport_unload_spread=1` (default): several loaded transports told to unload at one spot all headed
  for that one tile; each now gets its own tile there, apart by its size (ships 2 tiles, trucks and helicopters
  side by side), and they unload side by side - a yellow
  circle on each one's tile. Ships, helicopters and land transports alike, also mixed in one selection.
  Loading and unloading both first work out all the spots (the groups of units, the landing tiles), then match the
  transports to them so that the ways together are the shortest - so their ways don't cross.
  Both run inside the game's simulation like the original commands: in multiplayer every PC needs the same settings.
- `[general] build_retry=1` (default): citizens told to build together all walk to the free tile next to the site
  nearest them - after a move order, standing close, the same one - and push each other off it; the game makes a
  citizen that ends its walk off the site's edge, or finds no way round the others, stand idle there, so of five
  often only one or two began. Such a citizen of a human player is now sent again (up to three times, the game's
  own placing call: it picks the tile nearest to where it stands then). 0 = the original.

## Movement

- `[general] improved_unit_movement=default`: the movement settings below (ship_routes, land_routes, land_widen,
  share_routes, route_budget, ship_turns, land_turns, ship_collide, land_collide, formations, formation_march,
  formation_follow, formation_catchup_falloff, formation_wheel, and the factors formation_catchup, formation_reform, formation_wait,
  friendly_collision) as one. `default` gives each
  its own default (all on since 2026-10-26: the player's play), `file` each as set in the file (else its default),
  `default | file` a switch on when its default or the file has it on, `default & file` only when both do (a factor or
  number as the file sets it in both; 2026-11-05, the player's names in place of custom / enabled / disabled: the
  game's own movement is each switch set off with `file`). With `default` a key set below is logged (`# ini:`) and not
  used (improved_unit_movement=file with formation_follow=0: everything but the marching formations).
- `[general] ship_routes=1` (default): the game plans a ship's route in pieces of 8-18
  tiles, each in the 8 tile directions only, so a route at any angle between them zigzags and a slow-turning ship
  keeps swinging its bow. With 1 a ship's route is planned whole at once, in straight lines wherever the water between
  two points is clear, around the shallows a big ship can't enter, and keeping to the middle of channels; a fleet
  doesn't get the game's chain of in-between stops through the map's sectors. The player's and the AI's ships alike
  (the 2017 update; not yet in Art of Conquest). In multiplayer every PC needs the same setting.
- `[general] land_routes=1` (default): the same for land units and hovercraft - a route
  planned whole, straight wherever the ground between two points is clear, around buildings, walls and forests by
  the game's own tile test, instead of pieces in the 8 tile directions. A search takes well under a millisecond.
  The 2017 update; in multiplayer every PC needs the same setting.
- `[general] land_widen=1` (default; with land_routes): a route pulled straight hugs
  everything it rounds, so units scrape each building corner, tree and cliff on the way. With 1 each corner of a
  land route is moved away from what it rounds, up to 1.5 tiles, till the unit has its own radius and half a tile
  of room - only where there is room (through a gap it keeps to the middle, so choke points are still taken) and
  the legs to it stay clear.
- `[general] ship_turns=1` (default): told to go somewhere more than 45 degrees off its
  bow - a new order while it sails, or a sharp corner - a ship braked to a crawl, turned almost on the spot and
  sped up again (a U-turn took 6 s nearly standing). With 1 it turns in an arc: at full speed towards a far point,
  slower and tighter towards a near one, and only where the circle it sweeps is free of land, shallows and other
  ships - otherwise it turns as before. It also looks ahead: the ship steers at a point sliding along its route
  two turning circles ahead, where the way there is clear, instead of at the next waypoint - a fast ship steered at
  a waypoint inside its turning circle, overshot and circled it - and rounds corners in arcs. Through a shift-queued point
  it sails on in an arc instead of braking for it, where the way on to the next point is clear. In multiplayer every
  PC needs the same setting.
- `[patches] turn_fix=1` (default; 0 the original): Art of Conquest's fix of the turn, brought back. Ships,
  submarines, helicopters and the other units that turn with momentum braked their turn a step late and swung past
  the heading, then back (a unit turning the wrong way was even let turn on). With 1 they brake in time and stop on
  the heading, as in Art of Conquest. Tanks and hovercraft too (infantry and riders turn without momentum). A
  helicopter told four ways under way reversed its heading 19 times, now 7. In multiplayer every PC needs the same
  setting.
- `[general] land_turns=1` (default): land units and hovercraft steered at each tile centre
  of their route, so the heading jumped at every waypoint and a tank swung back and forth. With 1 they steer at a point
  sliding along the route, as ship_turns does for ships, and walk on through shift-queued points. A tank told four
  ways under way, with land_routes and turn_fix: 11 heading reversals and 837 degrees turned -> 5 and 680. In
  multiplayer every PC needs the same setting.
- `[general] ship_collide=1` (default; 0 the original): ships ran through each other, jumped back and got
  stuck on each other - the game tests two ships as thin lines along their headings (side by side or at a shallow
  angle they miss), a small ship next to a big one may not see it at all, and a hit halves the smaller one's speed
  and pushes it back in one jump. With 1 every ship has a hull (a capsule as long as the game's size for it, a third
  as wide), and ships whose hulls overlap are pushed apart gently, a little each step, the smaller one moving more; a
  ship heading into another veers off to starboard and slides past, and a standing ship of the same player is
  nudged aside instead of blocking. Submarines get the same among themselves (they pass under ships, as in the game). In a 16-ship fleet move through a channel the last
  ship arrived 50-70 s sooner and the ships stood about 70% less time short of their goal. The player's and the AI's
  ships alike; in multiplayer every PC needs the same setting.
- `[general] land_collide=1` (default): the same for land units and hovercraft - circles as
  big as the game's size for them (a citizen 0.3 tiles, infantry 0.5, a tank 1), pushed apart gently instead of
  slowed and jumped back, a unit walking into another veers off and slides past, and a standing unit of the same
  player steps aside. Units at work (gathering, building) are walked through as in the game, so citizens at a mine
  don't wall it off. Units at rest drift apart too, so a crowd that stopped on one spot (after a melee fight, say)
  doesn't stay one blob. Tanks through a block of standing infantry: about a quarter of the overlap, the deepest 0.79 ->
  0.24 tiles. In multiplayer every PC needs the same setting.
- `[general] formations=1` (default; 0 the game's grid): a group sent somewhere took up the formation's grid - only 5
  columns, every row and column as wide as the group's largest unit, units and places paired by distance alone - so a
  big army, worst with small and large units mixed, spread into a column dozens of tiles deep: 174 soldiers, riders and
  tanks sent 20 tiles stood half of them more than 27 tiles from the click. With it every formation (lines, wedges,
  circles) is laid out by its units' own sizes, its front row on the clicked point and facing the way the mouse was
  dragged with the right button (a plain click: the way it moves), like sizes in rows of their own, each unit keeping
  its place in the group: the army in after 46 s instead of 52, back in 57 instead of 79 (with the land movement
  settings). The ranks front to back: who fights up close (melee, tanks, the fighting heroes), the ranged soldiers and
  the healing heroes, siege and rams, then who doesn't fight (citizens, priests, medics). A circle is one ring of
  every unit round the point, as the game's own, each unit walking to its place by its own route.
  (It replaces `group_block`, a big group gathered as one block.)
- `[general] formation_march=1` (default, with `formations`): a group sent somewhere marches at its
  slowest unit's speed, so riders and tanks stay with the infantry: a mixed army sent 40 tiles strung out to 14 tiles
  front to back, now about 10 - but they reach the goal all at once and the last is in about 15 s later.
- `[general] formation_follow=1` (default, with `formations`; land units, or ships; 0 each on its own route): a group sent
  somewhere marches in its formation behind a leader - the unit in its middle - each unit steering for its place
  beside and behind it, a blocked one following the formation's middle line (the leader's route, through a gap the
  formation is too wide for), only then its own way, planned anew from where it stands; where the way is much narrower than
  the formation (a gap, a pass) it narrows for the while into a column - its ranks as many abreast as fit the gap by
  their own widths (a gap of 3 tiles takes two tanks or three soldiers), each rank as wide as the way ahead of it
  allows, so the head spreads out past the gap while the tail is still in it, given out by how far along each unit
  already is, so the ones nearest the gap go first and nobody crosses the body to a place beyond it, and a queue: each
  rank closes up behind the one ahead and moves when it moves, and the ones waiting for their turn creep on with it
  instead of standing till their place comes to them; a column still in the gap when its head reaches the
  goal keeps draining through it, each unit let go to its spot once it is past the narrow (before, the march ended
  there and the units left in the gap were the game's again: single file, and some stood for good, their orders given
  up); a unit whose move the game drops in the press at a gap's mouth (it stands idle, its order gone) is sent on to
  its spot at once, and when it was the one leading the march another leads on (before, the march ended there and the
  rest went in by their own routes, a large part of them stuck at the entry); an attack-move marches in formation
  too: its units look for enemies as they march (the game's own units look only at each waypoint of their route,
  and a unit steered at its place never arrives at one - a formation marched into an army and died without a shot),
  a unit that takes on an enemy fights as the game moves it and is steered back to its place when the fight is over,
  the formation holds while any of its units fights near it and moves on after (one chasing off is left to come
  after on its own), and when the unit whose route the formation follows is among the fighters another takes that
  over; in a jam the formation slows to what gets through, so it doesn't spill out thin behind it; a unit that is really stuck drops out
  and goes on alone; a place on ground the unit can't enter (a
  formation wider than the channel it passes) is drawn in towards the formation's middle, so it narrows there instead
  of losing its outer units; the formation keeps off an edge the leader's route hugs (a shore, a wood, a wall: the
  route is found for one unit) - it runs beside the route, by as much as gives every unit room, where the other side
  has it, and down the middle of a corridor narrower than itself; its rear ranks follow the path its head took, so a
  column snakes through a channel and round its corners instead of the whole body swinging as one, and it turns on
  its width, not its depth; a group whose route sets out another way than straight for the point forms up facing the
  route's first stretch and sets out along its leader's route from the first step, no turn on the spot; units march as one formation when the gaps between them are under 4 tiles (their sizes
  counted, so a fleet of large ships stays one); near the goal each goes to its own spot, and one the game gives up on
  short of it (its route cut by its detour round a blocker) walks back to it.
  The formation goes at the slowest unit's speed, turning like a ship (no unit walks backwards in a turn: it slows
  in a turn so its outermost units keep up, eased in and out over a few seconds - a bend in the route costs a little,
  only a sustained turn the whole), facing
  the way it goes to the goal, where each unit turns to the final facing on its spot (forming anew there when the turn would move its
  outermost unit further than `formation_wheel` tiles); a shift-queued turn within that keeps the formation. A fleet turns together: its formation swings to the
  new course at once and every ship turns on its own circle to its new place, instead of the whole fleet wheeling
  round in one wide arc; a ship ahead of its place slows for it instead of turning back, one beside it merges at a
  shallow angle. From rest the formation is laid over the group where it fits best along the way and sets out at
  once: a unit standing behind its place closes on it, one standing ahead of its place faces the way and waits for
  it to come up (the inner flank of a wheel) - nobody walks back through the formation, and the slowest units are
  in their places within half the formation's depth.
  Told to form within its own size (a right-button drag where it stands, to turn it) a formation
  takes its places without a march - each unit by its own route, as the game does; a formation that stands as it
  marched and turns within `formation_wheel` marches as it is then, no unit changing place, straight to the point and
  facing its final facing from the start, so it holds the formation on the way (turned further, a wedge placed over
  it facing back, it takes its places anew - every unit kept its offset and crossed the formation before). A click
  turns it to face the way to the point, as the game does;
  `formation_keep_facing=1` keeps its facing instead (in the game's view a click comes as a drag facing the way to
  the point, so a drag within 10 degrees of that way counts as a click here).
- `[general] formation_wheel=3` (tiles, with formation_follow): a formation turns as it stands - told to form where it
  stands facing another way, to face another way at its goal, or at a shift-queued turn - when the turn moves its
  outermost unit no further than this; turned further it forms anew, each unit to the nearest place. Not an angle: a
  squad of six (radius 1.5) wheels right round within 3 tiles, a wedge of a hundred (radius 6) breaks up at 30 degrees.
- `[general] formation_catchup=125%` (with formation_follow): how fast a unit behind its place may go to keep it, in
  percent of its own top speed (100% never faster than its own) - near its place only: all of it within its size,
  none from formation_catchup_falloff times that. Also how much the formation slows in a turn (its outermost unit
  keeps up within it). A ship above its top speed throws out its ripples no faster than at its top speed (the game's
  rate runs out at 108% of it: a ripple every frame, a solid white plume behind the stern).
- `[general] formation_catchup_falloff=4` (with formation_follow): how far behind its place, in multiples of its own
  size, a unit's catch-up has fallen off to nothing - from there on it walks at its own top speed and the formation
  slows for it (formation_wait).
- `[general] formation_reform=125%` (with formation_follow): how fast a unit may go to take its place when the
  formation forms - within 5 seconds of a march's start or of last being in its place (a re-form or a new order on the
  march), fading out, and no more than the catch-up it had at that moment (a straggler with none gets none; none from
  rest - a group standing about takes its time to assemble). The three were one key (formation_catchup) till
  2026-11-02; keeping the place and taking it are different manoeuvres (the player).
- `[general] formation_wait=100%` (with formation_follow): the formation goes at most this share of a straggler's
  speed - a unit fallen behind the formation's rear beyond its catch-up (formation_catchup_falloff times its size)
  has only its top speed, so at 80% the formation slows to 80% of that until the unit is within its catch-up of its
  place again (it catches up at the other 20%). A unit within the formation's body, or one held up for 2 seconds or
  more (a moment's bump in the crowd counts for nothing), slows it no
  further than it goes itself, and one not gaining on its place for 10 seconds (blocked, jostled in a crowd) is
  given up on - so the formation always either moves or waits for a straggler that is closing. Lower waits more for
  the stragglers (tighter, slower); 0% stands until they have caught up; 100% never slows below a straggler's
  speed, so it doesn't gain on the formation.
- `[general] friendly_collision=75%` (with land_collide / ship_collide): two units of one player or of allies push
  each other off at this share of their size - they slip past each other instead of getting stuck, and a
  formation's places stay as far apart (100% as any two units). Every friendly pair, moving or standing, so nothing
  switches and shoves them apart; a standing ally steps aside for a moving unit as one's own does. Until 10 seconds
  after the last unit of an arrived formation came to rest (a re-form in place too) a unit standing in it that is
  pushed off its spot turns, walks back to it and turns to the formation's facing.
- `[general] goal_markers=1` (default; 0 the original): a move order drew its circle at the goal of only the group's
  first 15 units (20 in the other lines and wedges, every few in a circle formation). Now every unit's shows.

## Sound

- `[sound] sound_clamp_channels=1` (default): the "Max Number Of Sounds" slider goes to 200%, which asks for 32 sound
  channels, but the sound mixer has only 16 on current Windows. Sounds put on the 16 missing channels were silent,
  and a looping one (unit movement such as the U-boats', ambience) stayed stuck there and never played again -
  especially after changing the slider in a running game. The channels are now held to what the mixer has, so
  anything from 100% up gives the same working channels.
- `[sound] sound_channels=32` (default): the mixer's 16 channels are only a fallback because Windows no longer reports
  hardware voices; Miles mixes in software and allows 63. The mod raises the maximum to this value, so the slider
  gives 8 (0%) to 32 (100% and up) channels: fewer sounds cut in big battles. `0` keeps the 16.
- `[sound] sound_sample_rate=44100` (default): the game mixed its sound at 22050 Hz; now at this rate (up to 48000).
  `0` keeps the 22050.

## Beside other DLL mods

`frame_wait`, `vertex_sysmem` and `sound_sample_rate` replace tweaks.dll's SetSleepToZero, VertexBufferSystemMem and
AudioSampleRate. With tweaks.dll in the game folder, switch those off in its tweaks.config; its map-size patches are
separate and stay with it. dreXmod 3.4's FixTnLVertexBuffer does what `vertex_sysmem` does (the mod finds it done
and leaves it).
