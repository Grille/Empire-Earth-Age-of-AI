# Empire Earth tuning mod

A runtime mod for **Empire Earth 1.04 with the Difficulty Setting & Multiplayer Lobby add-on** (the
last official base-game exe; not Art of Conquest). It fixes and tunes the game from inside: a DLL in the
game folder patches the running exe, and the game's files stay unchanged. The engine loads every DLL
in its folder, so dropping the file there is enough.

The current focus is the **computer player**: it should play fair, build a real economy and army, defend
itself, and feel like an opponent with a character. The tooling here works for any part of the game: hooks,
control pipe, automated tests and the engine notes.

The mod needs the 1.04 exe of the 2017 update: the **GOG** version, the **Steam Gold Edition**
(`steamapps\common\Empire Earth Gold Edition\Empire Earth` - the same code, only a signature and a DPI manifest are
added) or the **Vanilla** setup from empireearth.eu. Old CD versions, no-CD exes and NeoEE have their code in other
places: the mod recognizes the exe when the game starts and stays switched off with another one, with a message
(docs/engine-notes.md, Exe builds). Art of Conquest (`EE-AOC.exe`) isn't supported yet. The mod coexists with community DLL
patches such as *Empire Earth Tweaks* or *dreXmod*, which patch different places.

**Documentation**

| Document | Contents |
|---|---|
| [docs/ai-analysis.md](docs/ai-analysis.md) | How the original AI works, where it fails, what the mod changes, and all test results |
| [docs/engine-notes.md](docs/engine-notes.md) | Verified addresses and data layouts of the engine |
| [docs/testing.md](docs/testing.md) | Autotest mode, test scenarios, log format, control pipe commands |
| [config/aimod.ini](config/aimod.ini) | Every setting, explained |
| [config/aimod_ai/README.md](config/aimod_ai/README.md) | The personalities and texts: format, expressions, values, situations |
| [config/aimod_ai/_template.eeai](config/aimod_ai/_template.eeai) | An empty personality with every key explained, to copy |
| [config/aimod_ai/mod/](config/aimod_ai/mod/) | The mod's own texts: `computer.eeai` (every personality's base), `prompt.eeai` (what the mod tells the language model - the AI voice), `system.eeai` (the system lines the mod shows), `units.eeai` (the groups the texts count with: units, army, buildings ...) |

## Computer player

The original AI cheats: by default it pays about 1% of the real prices. With cheating switched off, it
can't run its economy, gets stuck in the early epochs, only trains soldiers for its next attack and leaves its
base undefended. This mod changes that. Every feature has its own setting in `aimod.ini`; the AI never gets
information a human couldn't have (it only uses enemy buildings it has seen).

**Economy**

| Feature | What it does |
|---|---|
| Fair prices | The AI pays what you pay (configurable per difficulty, e.g. a discount only on Hard). |
| Economy first | Before farms (Copper Age), the AI grows its workforce, counts the wild food it has found as its budget, saves for the next epochs and keeps only a token army. |
| Military budget | Soldiers are paid from a share of the starting stock and of the income, never from the epoch reserve; from the Copper Age on most of the income goes to the army. |
| Farm fields | The original AI left the fields around its granary as bare foundations (always on maps with fish, often on land), put nobody on finished ones and never replaced destroyed ones. It now builds them as far as its wood pays (keeping 50 wood per field for that), farms every one and lays out lost fields again. |
| More granaries | The original AI built a single granary (8 fields) all game; its food stayed short and spare citizens went to gold. It now adds granaries while it wants more food workers than its fields hold. |
| Citizens, towers and raids | Citizens no longer gather inside or walk past the range of enemy towers the AI has seen, nor go one by one to a field or mine where enemy soldiers stand; those working there are called back. The AI no longer plans its own towers and walls where the builders would walk into enemy fire. |
| Citizens fight back | Enemy citizens putting up a tower next to the AI's gatherers get attacked - the builders first, by its soldiers or its citizens - and in the early ages citizens beat off small raids when clearly stronger. |
| Paid upgrades | The original AI got all unit and tower upgrades of an epoch for free the moment it reached it. It now researches them at their buildings and pays, like a player. |
| Working build queue | The original build list started with a building that can't be placed, which blocked everything after it (granary, archery range, siege factory, university...). |
| Short ways to work | Citizens mine the free mine nearest to a drop site, move from oversupplied resources to short ones, and get the gathering upgrades the original AI rarely researched. |
| Epoch pacing | `classic` (the game's timer, at most one epoch ahead of the human), `timer` (the timer alone), `rush` (as soon as affordable), `follow` (never ahead of the human, prepared to follow at once) or `independent` (the default: the timer, and a few minutes after a human's advance the AIs behind follow). |

**Defence and army**

| Feature | What it does |
|---|---|
| Towers and walls | Towers at the front facing the enemy and at outlying mines instead of scattered around; a wall arc in front of them. |
| Home guard | The AI keeps an army at home that grows with its population cap (the original trained only for its next attack). |
| Muster | When an enemy army in its base is too strong to fight (twice its soldiers and towers), the AI stops sending every new soldier in alone: they gather beyond the base and strike together once they can win; citizens next to the enemy flee there too. |
| Unit choice | When the AI asks for a unit it can't pay for or train, it trains the strongest soldier it can afford. |
| Counter-siege | Soldiers near the base attack enemy siege weapons shelling it, when clearly stronger than the escort. |

**Attacks**

| Feature | What it does |
|---|---|
| Attack tuning | Fewer, bigger waves instead of a stream of small, wasted attacks. |
| Routing | Armies march around the enemy towers they know about and look for undefended targets instead of running into the tower line. |
| Siege | The AI builds siege weapons when it knows enemy defences, keeps them at firing range and breaks through walls. |

**Scenarios and gathering**

| Feature | What it does |
|---|---|
| Scenario fix | AIs in scenarios started from the menu play like in skirmish games (the original turns most of their planning off). |
| Safe gathering | Optional: citizens only gather near their base, on routes away from known enemies. |

## Diplomacy and personalities

The original AI has no diplomacy at all: declaring war on it changes nothing on its side, and it never makes
peace or alliances. With the mod:

- **War is answered:** whoever declares war on an AI gets war back, with a revenge attack and a taunt.
- **Peace and alliances:** an AI judges peace offers by its opinion of you, the strength of both sides, how
  recently you attacked it and its character. Tribute buys goodwill. AIs ally against a much stronger enemy,
  and a disloyal one may betray a weak ally. Nothing changes in scenarios or with locked teams.
- **Offers to you are messages:** an AI offering you an alliance (or peace inside your block) only says so; it
  stays at war until you set it to allied (2 minutes), and only then turns allied itself.
- **Stance lines** (`stance_lines`): only the AIs' stance changes towards you are shown, naming the AI ("Khan
  Temur is now your enemy"). The game's line for every change - also between AIs, without saying towards whom - is
  `stance_lines=2`; `0` shows none (the personalities still speak).
- **Alliance blocks:** everyone linked by alliances is one block, with peace inside it: two AIs allied with you
  (or with the same AI) don't fight each other. An AI at war with you that joins your block offers you peace.
  Whoever turns on a member breaks the block, and every other AI picks a side (`alliance_blocks`). Joining
  means peace with the whole block, so an AI refuses an alliance when it - or one of its allies - holds a grudge
  against one of yours (and says who stands in the way). Vassals have no say in their lord's alliances - they
  make peace as told, grumbling if they hold a grudge.
- **Capitulation:** when your army is in its base and the fight is lost, an AI offers peace and tribute
  ("set us allied"); accept within 2 minutes and it pays and stays loyal. Proud personalities hold out longer.
  A vassal has no diplomacy of its own: no offers, no betrayal, and whoever asks it for peace is sent to its lord.
  It still defends itself, and a lord who attacks his vassal sets it free.
- **Reputation:** breaking an alliance is seen by every AI - each thinks worse of the traitor, the more loyal it
  is (twice as much if the victim is its own ally). Attacking your own vassal is three times as bad, and right after
  accepting its capitulation (taking the tribute, then attacking) five times.
- **Victory over vassals:** vassals at war with each other would keep the game running for ever. Once every
  other player has capitulated to you, you are told so; play on as long as you like, and type `sieg` (or
  `victory`) in the chat to end the game as the winner. Resigning or exiting from the menu then counts as
  a victory too.
- **Ending the game yourself** (`auto_victory=0`): the game no longer ends on its own when one alliance is left;
  once nobody stands against you, a hint says so, and `sieg`/`victory`, Resign or Exit ends it as a victory.
- **Saved games:** the AIs remember across save and load - grudges, alliances, capitulations, pending offers
  (a small `.aimod` file next to each save; `save_state`).
- **Surrender:** an AI that has lost all its citizens and can't train new ones resigns (with a last line),
  instead of leaving its last buildings for you to hunt down (`surrender`, not in scenarios). It gives up to
  whoever destroyed the most of it lately, and doesn't capitulate or take sides in a war once it is beyond saving.
- **Personalities** (optional folder [`config/aimod_ai`](config/aimod_ai)): every AI gets a name, a favoured
  colour on random maps, character traits (aggression, loyalty, pride, greed, forgiveness, caution, talk) and
  its own lines for situations of the game: greetings, attacks, being attacked, new ages, winning and losing,
  diplomacy. Six personalities are included, in English (other languages: at the release, a file each). A saved
  game finds its personalities again.
  A file is a personality when it says so (`register_personality()`). `_template.eeai` is an empty personality
  with every key explained: copy it to a new name to make your own.
- **The mod's own texts** are in its subfolder `mod/`: `computer.eeai` is no personality but the base of every
  one, without being named: neutral lines for what must not go unsaid (war, peace, offers waiting for your answer,
  capitulation, victory), the diplomacy's rules and a few shared values. A personality without a line for such a
  situation uses its line, and AIs without any personality speak only these lines. `prompt.eeai` holds everything
  the mod tells the language model (the AI voice: who the AI is, the rules, the game as it stands, the situations,
  the tools) - every personality's last layer, so a personality can say any of it its own way
  (`prompt = { ... }`). `system.eeai` holds the system lines it shows you (offers waiting for your answer, chat
  hints, victory) in English. Placeholders in braces are filled in. The mod has no texts of its own: these files
  belong to it like the DLL - without them it says nothing (no personalities, no AI voice, no diplomacy of its own;
  a message at the start tells so).
- **The format** of all these files is the scripts' language ([docs/scripts.md](docs/scripts.md)), without the
  commands: the keys the mod reads in records - `personality = { ... }`, `lines = { greeting = ... }` - and names
  of the file's own outside them (`weakling = them.strength * 2 < me.strength`), each `name = value`, where a
  value is an expression - a `"string"` with `{placeholders}` and `{expressions}` in it, a
  `["list", "of", "strings"]` (one at random), conditions
  (`allied ? "Welcome back."`), fallbacks (`a ?? b`) - over the game as it stands: what the AI thinks of you, how
  strong each side is, how long the war has lasted, whether you betrayed it before, what you paid it, its epoch.
  Everything - the format, the expressions, every value, every situation - is in
  [`config/aimod_ai/README.md`](config/aimod_ai/README.md), which goes into the folder with the files. A mistake is
  logged with the file and line and only that key is skipped; `strict_parsing=1` in aimod.ini shows every mistake
  at once (for whoever writes texts).
- **Editing them in VS Code:** [`extensions/eeaimod-texts`](extensions/eeaimod-texts) highlights the `.eeai`
  files - records, keys, strings, placeholders, expressions; an unknown key, situation or epoch name shows as an
  error. `python scripts/build/pack_vscode.py`, then `code --install-extension local/work/eeaimod-texts.vsix`
  and reload the window. The grammar takes the names from the DLL's sources (`scripts/build/gen_vscode_grammar.py`).
- **Play style** (experimental, `ai_style=1`, off by default): a personality can also change how its AI plays
  (attack timing, army share, home guard). Off, every AI plays the same, tested way.

- **AI voice** (optional, `[llm] enabled=1`): a language model on your PC (LM Studio, Ollama, llama.cpp - any
  server with the OpenAI chat API) speaks for the AIs. Their lines come in their own words, knowing the game's
  state (who is at war with whom, who is stronger, what they think of you, how many soldiers, citizens and buildings each of you has destroyed of the other's - in the whole game and in the last 20 minutes), and they answer when you write to them
  in the chat - the game's chat filter decides who gets your line (everyone, allies, enemies or the players you
  chose); of those, the one you name answers ("Temur, ..." - a typo or the first letters will do), else the last one
  you heard, else the only one that got it. In single player a line nobody will answer gets a system line saying
  why (no AI named, the filter reaches none or not the one you named, the model can't be reached; with the AI voice
  off, once a game: the AIs don't read the chat). Words only: the model decides
  nothing, and an AI can't be talked into anything - it knows its real terms for peace (the tribute that would buy
  it, by the same rules that decide, or why no price would: its pride after your declaration, an ally's veto, a
  grudge too deep), so it can name its price and its words match what it will do. Tribute amounts and names must survive the rewording, else the fixed line is shown;
  without a server (or too slow, `timeout`) the fixed lines show as before. Only on your PC - in multiplayer each
  player's own model (or none). `character=` in a personality file adds a few words about it for the model.
  - **The model decides** (`decide=1`, single player, a model with tool calls): your peace offers are answered by
    the model, and in the chat it can make peace, declare war on you or send you a tribute - real deals ("pay me
    1000 and we have peace" holds if it says so). The rules only limit it: what isn't possible right now (its
    pride after your declaration, an ally's veto, being a vassal, war within 5 minutes of an alliance, more than a
    quarter of its stock as tribute) is never offered to it, and without an answer in time the rules decide. Its
    nature and the rules' verdict are its advice. Everything else (answering war, capitulations, alliances between
    the AIs, betrayal) stays with the rules.
  - **The AIs talk to each other** (`ai_talk=1`, single player): when one AI says something to another (a
    declaration of war, a betrayal, an alliance offer, a capitulation), they talk it over - a few messages, each
    pair with its own memory of what they told each other (and what one tells another, you don't learn - it may tell
    you something else). Words only, their decisions stay with the rules. One exchange at a time, at most one a minute
    (`ai_talk_gap`), your own conversations go to the model first; `ai_talk_show=1` lets you read along.
  - **The model decides between the AIs too** (`ai_decide=1`, single player): an AI's alliance offer or capitulation
    to another AI is answered by the other's model (accept or refuse - and haggle: "an alliance against Gunther?
    250 gold first"), and in their talks an AI can offer peace, declare war on an ally or send a tribute. The rules
    still limit what is possible (an ally's veto, a vassal, war only after 5 minutes of alliance) and decide when
    the model doesn't answer; answering war, betrayal and taking sides in a split alliance stay with the rules.
  - **Nobody is special** (`equal=1`): the model isn't told who is human - you are another leader like the AIs, and
    no AI is told it is played by the computer.

The AIs can also explain themselves in the chat (`chat=1`, a debug aid): attacks, new ages, towers, walls,
siege, home guard.

## Installing (players)

1. Copy `eeaimod.dll` into the Empire Earth folder (next to `Empire Earth.exe`).
2. Copy [`config/aimod.ini`](config/aimod.ini) into the same folder and adjust it. Every option is
   explained in the file. Settings can differ per difficulty (`[easy]`, `[medium]`, `[hard]` override
   `[all]`).
3. Optional: copy the folder [`config/aimod_ai`](config/aimod_ai) into the same folder for the AI
   personalities. Add or edit `.eeai` files there to make your own; they are read when Empire Earth starts.
4. Start the game normally. Changes to `aimod.ini` take effect when a game starts or a save is loaded.
   Instead of editing the file you can copy `aimod_config.exe` into the same folder and start it: every
   setting by section (General, Display, ... the AI for every difficulty and each one alone) with its
   explanation, its value and its default; a value is checked as you type, as the game reads it, and Save
   writes only what you changed, keeping the file's comments. It also lists keys in your file that this
   version no longer knows (old experiments, typing mistakes) and can remove them. Without an `aimod.ini` it
   offers to make one with the default settings. Its Game page shows which exe is next to it and can give the
   game 4 GB of memory instead of 2 (the Large Address Aware flag - big armies and maps need it; one bit in the
   exe's header, the game closed, the exe kept as `Empire Earth.exe.original` and put back by Restore).
5. Optional: copy `dinput.dll` into the same folder too. The game then keeps its own settings (graphics,
   sound, scroll speed, the last game's options - what the options screen sets) in `eesettings.ini` in the
   game folder instead of the Windows registry, so they travel with the folder; the first start copies the
   registry's values into it. It is a stand-in Windows loads at the start: the game's one DirectInput call
   goes on to the system's `dinput.dll`. If another program in the folder already has a `dinput.dll`, keep
   that one - everything works without ours, the settings then simply stay in the registry. Delete it to go
   back to the registry (the file is then ignored).

`aimod.ini` is required: without it Empire Earth shows an error and closes. To play the original game,
set `enabled=0` at the top of `aimod.ini` (the mod then installs nothing), or delete `eeaimod.dll` to
uninstall. A setting missing from an older `aimod.ini` takes its built-in default. The DLL writes a log to
`aimod_econ.csv` in the game folder.

## Display and interface

Settings in `aimod.ini`:

- `[display] mode=windowed`: native windowed mode. The engine still contains SSSI's own windowed path; the
  mod switches it on and fixes what broke on current Windows (presentation, crashes, terrain, editor).
  The window has the game resolution from the game settings (scaled down if it doesn't fit the screen).
  A DirectDraw wrapper (`ddraw.dll`) is still needed for the game to start on current Windows; which one is up
  to you, the mod works with both it was tested with:
  - [DDrawCompat](https://github.com/narzoul/DDrawCompat): fastest by far (2560x1440: ~4.5 ms per frame vs ~10).
    Put `CpuAffinity = all` into `DDrawCompat-Empire Earth.ini` next to the exe (its default runs the whole
    game on one CPU core). The window gets a classic (Windows 98 style) frame, which DDrawCompat draws itself.
  - dgVoodoo (DirectX pass-through is enough): modern window frame, slower.

  `[display] present=auto` picks the matching way to show the picture (DDrawCompat: its primary surface;
  otherwise Direct3D 11).
  The mouse follows the Windows cursor (your pointer speed), the window can be moved and switched like
  any other. `lock_mouse` keeps the cursor inside while the game is in front: `auto` (default) only in
  borderless, so edge scrolling doesn't slide onto a second monitor, while a window stays free; `1` always, `0` never.
- `[display] mode=borderless`: a frameless window over the whole monitor, the game image scaled to it
  (aspect kept). Alt+Enter switches between window and borderless. The window is DPI-aware (`dpi_aware`),
  so Windows display scaling doesn't blur it; `smooth=1` filters the scaled image.
- `[editor] resolution=aoc`: scenario editor at 1024x768 with The Art of Conquest's layout; `game` or any
  `WIDTHxHEIGHT` extends that layout to the resolution (map view fills the screen, panels at the bottom,
  minimap and file buttons at the bottom right).
- `[camera] scroll`: `game` (default) the build's own camera scrolling, `aoc` The Art of Conquest's acceleration
  in either build, `ee` Empire Earth's constant speed in either build; `smooth=1` (default) moves the camera every
  frame by the frame's time instead of the game's fixed timer steps (jitter) - in both builds.
- `[general] lead_targets=1` (default): ranged units and towers aim where a moving target will be when the
  projectile comes down, not where it stood when it was fired. Before, slow projectiles (arrows, catapults,
  cannon balls) missed nearly everything walking across their line of fire.
- `[general] spread_missiles=1` (default): AA towers and other missile launchers stop wasting missiles on an
  aircraft the missiles already on their way will destroy; they take another target in range or hold the shot.
  `lead_missiles=1` (off by default) makes homing missiles intercept instead of chase.
- Multiplayer (not tested yet): every player needs the same `eeaimod.dll`, the same `aimod.ini` and the same
  `aimod_ai` personalities. The game is lockstep - every PC simulates everything - so different settings make
  the games drift apart. The mod's decisions use only the game's own state, math and random numbers for this.
- `[general] skip_intro=1`: skip the intro movies.
- `[display] draw_limit=4096` (default): the game drew at most 512 units, buildings and missiles per frame - arrows
  in flight or stuck in the ground included (a 360-archer fight reached 445) - so in big fights, and the more so at
  high resolutions, the farthest objects blinked in and out. `0` keeps the 512.
- `[general] sight_counts=1` (default): every map tile counts how many of a player's units see it, but the game read
  only one byte of that count - a tile seen by more than 254 units took itself for unexplored, and in a big army
  random tiles with everything on them (units, trees) blinked out, then went to fog while units still stood there.
  Now the whole count is kept (up to 65000). `0` = the original.
- `[general] big_selection=1` (default): double-clicking a unit (all of its kind on screen) or dragging a box selected
  at most 63 units - with a large army only a part. Now up to about 240, as many as the game's selection message
  can carry; the message keeps its format, so it should stay compatible in multiplayer.
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
- `[general] improved_unit_movement=default`: the seven movement settings below (ship_routes, land_routes,
  ship_turns, land_turns, turn_fix, ship_collide, land_collide) as one. `default` gives each its own default (on
  what is measured and helps), `enabled` all of them on, `disabled` all off - the game's own movement. Each one set
  on its own wins over it; the shipped file shows them commented out, so they follow it.
- `[general] ship_routes=0` (default off while it is measured): the game plans a ship's route in pieces of 8-18
  tiles, each in the 8 tile directions only, so a route at any angle between them zigzags and a slow-turning ship
  keeps swinging its bow. With 1 a ship's route is planned whole at once, in straight lines wherever the water between
  two points is clear, around the shallows a big ship can't enter, and keeping to the middle of channels; a fleet
  doesn't get the game's chain of in-between stops through the map's sectors. The player's and the AI's ships alike
  (the 2017 update; not yet in Art of Conquest). In multiplayer every PC needs the same setting.
- `[general] land_routes=0` (default off while it is measured): the same for land units and hovercraft - a route
  planned whole, straight wherever the ground between two points is clear, around buildings, walls and forests by
  the game's own tile test, instead of pieces in the 8 tile directions. A search takes well under a millisecond.
  The 2017 update; in multiplayer every PC needs the same setting.
- `[general] land_widen=0` (default off while it is measured; with land_routes): a route pulled straight hugs
  everything it rounds, so units scrape each building corner, tree and cliff on the way. With 1 each corner of a
  land route is moved away from what it rounds, up to 1.5 tiles, till the unit has its own radius and half a tile
  of room - only where there is room (through a gap it keeps to the middle, so choke points are still taken) and
  the legs to it stay clear.
- `[general] ship_turns=0` (default off while it is measured): told to go somewhere more than 45 degrees off its
  bow - a new order while it sails, or a sharp corner - a ship braked to a crawl, turned almost on the spot and
  sped up again (a U-turn took 6 s nearly standing). With 1 it turns in an arc: at full speed towards a far point,
  slower and tighter towards a near one, and only where the circle it sweeps is free of land, shallows and other
  ships - otherwise it turns as before. It also looks ahead: the ship steers at a point sliding along its route
  two turning circles ahead, where the way there is clear, instead of at the next waypoint - a fast ship steered at
  a waypoint inside its turning circle, overshot and circled it - and rounds corners in arcs. Through a shift-queued point
  it sails on in an arc instead of braking for it, where the way on to the next point is clear. In multiplayer every
  PC needs the same setting.
- `[general] turn_fix=1` (default; 0 the original): Art of Conquest's fix of the turn, brought back. Ships,
  submarines, helicopters and the other units that turn with momentum braked their turn a step late and swung past
  the heading, then back (a unit turning the wrong way was even let turn on). With 1 they brake in time and stop on
  the heading, as in Art of Conquest. Tanks and hovercraft too (infantry and riders turn without momentum). A
  helicopter told four ways under way reversed its heading 19 times, now 7. In multiplayer every PC needs the same
  setting.
- `[general] land_turns=0` (default off while it is measured): land units and hovercraft steered at each tile centre
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
- `[general] land_collide=0` (default off while it is measured): the same for land units and hovercraft - circles as
  big as the game's size for them (a citizen 0.3 tiles, infantry 0.5, a tank 1), pushed apart gently instead of
  slowed and jumped back, a unit walking into another veers off and slides past, and a standing unit of the same
  player steps aside. Units at work (gathering, building) are walked through as in the game, so citizens at a mine
  don't wall it off. Units at rest drift apart too, so a crowd that stopped on one spot (after a melee fight, say)
  doesn't stay one blob. Tanks through a block of standing infantry: about a quarter of the overlap, the deepest 0.79 ->
  0.24 tiles. In multiplayer every PC needs the same setting.
- `[general] formations=0` (default, being worked on; 1 on): a group sent somewhere took up the formation's grid - only 5
  columns, every row and column as wide as the group's largest unit, units and places paired by distance alone - so a
  big army, worst with small and large units mixed, spread into a column dozens of tiles deep: 174 soldiers, riders and
  tanks sent 20 tiles stood half of them more than 27 tiles from the click. With it every formation (lines, wedges,
  circles) is laid out by its units' own sizes, its front row on the clicked point and facing the way the mouse was
  dragged with the right button (a plain click: the way it moves), like sizes in rows of their own, each unit keeping
  its place in the group: the army in after 46 s instead of 52, back in 57 instead of 79 (with the land movement
  settings). The ranks front to back: who fights up close (melee, tanks, the fighting heroes), the ranged soldiers and
  the healing heroes, siege and rams, then who doesn't fight (citizens, priests, medics) - in a circle formation they
  are in the middle. Off while it is tried in games. (It replaces `group_block`, a big group gathered as one block.)
- `[general] formation_march=0` (default, being tried; 1 on, with `formations`): a group sent somewhere marches at its
  slowest unit's speed, so riders and tanks stay with the infantry: a mixed army sent 40 tiles strung out to 14 tiles
  front to back, now about 10 - but they reach the goal all at once and the last is in about 15 s later.
- `[general] formation_follow=0` (default, being worked on; 1 on, with `formations`, land units): a group sent
  somewhere marches in its formation behind a leader - the unit in its middle - each unit steering for its place
  beside and behind it, a blocked one taking its own way for a while; near the goal each goes to its own spot.
  The formation goes at the slowest unit's speed, turning like a ship (no unit walks backwards in a turn), facing
  the way it goes and turning into its final facing at the goal (re-forming there when that is 45 degrees or more
  off); a shift-queued turn under 45 degrees keeps the formation.
- `[general] formation_catchup=200%` (with formation_follow): how fast a unit behind its place may go to keep it, in
  percent of its own top speed (100% never faster than its own) - near its place only (all of it within its size,
  none from four times that) or within 5 seconds of last being in it (a re-form or a new order on the march: no more
  than the boost it had then; none from rest - a group standing about takes its time to assemble). The formation
  goes at most 80% of each unit's speed so: at its slowest unit's top speed when all have 125% or more, slowed to
  80% of a unit's top speed while it has fallen behind.
- `[general] formation_collision=75%` (with formation_follow and land_collide): units marching in the same
  formation push each other off at this share of their size - they slip past each other to their places, which
  stay as far apart (100% as any two units). It fades in over the march's first 2 seconds, stays while the
  formation marches and for 10 seconds after it arrived, then fades out over 2 seconds; in those 10 seconds a unit
  standing in it that is pushed off its spot turns, walks back to it and turns to the formation's facing.
- `[general] goal_markers=1` (default; 0 the original): a move order drew its circle at the goal of only the group's
  first 15 units (20 in the other lines and wedges, every few in a circle formation). Now every unit's shows.
- `[sound] clamp_channels=1` (default): the "Max Number Of Sounds" slider goes to 200%, which asks for 32 sound
  channels, but the sound mixer has only 16 on current Windows. Sounds put on the 16 missing channels were silent,
  and a looping one (unit movement such as the U-boats', ambience) stayed stuck there and never played again -
  especially after changing the slider in a running game. The channels are now held to what the mixer has, so
  anything from 100% up gives the same working channels.
- `[sound] channels=32` (default): the mixer's 16 channels are only a fallback because Windows no longer reports
  hardware voices; Miles mixes in software and allows 63. The mod raises the maximum to this value, so the slider
  gives 8 (0%) to 32 (100% and up) channels: fewer sounds cut in big battles. `0` keeps the 16.

## Status

- **Confirmed in play:** scenario fix, the early economy (fair prices, economy first, budget), windowed and
  borderless mode, editor resolution, camera scrolling.
- **Tested in automated AI games and test scenarios, needs playtesting against a human:** build queue,
  mines, gathering upgrades, defences, home guard and army size, attack routing, siege, diplomacy and
  personalities. The numbers are in [docs/ai-analysis.md](docs/ai-analysis.md#test-results-so-far).
- **Experimental, off by default:** personality play styles (`ai_style`).
- **Planned:** an Art of Conquest port (the AI classes are the same, the addresses differ).

## Building

Requirements: Windows, any 32-bit MinGW C compiler. The project uses
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases) (`llvm-mingw-*-ucrt-x86_64.zip`) and a
POSIX shell (Git Bash).

```sh
git clone <this repo> eeaimod     # anywhere; inside the game folder (<game>/_aimod) needs no setting
cd eeaimod
# elsewhere: name the game folder (or set EE_GAME_DIR) - the tests and the install use it
echo "X:/Games/Empire Earth Gold/Empire Earth" > local/game_dir.txt
# unpack llvm-mingw to local/tools/llvm-mingw (or set CC=/path/to/i686-w64-mingw32-clang)
sh dll/build.sh            # -> dll/eeaimod.dll
sh scripts/build/install.sh      # the committed build into the game folder (reserves it: waits until no game runs)
```

The DLL is a single translation unit: `dll/eeaimod.c` includes the other `.c` files, one folder per part (Project layout).

## Development setup

Everything below is only needed for reverse engineering and automated tests.

| Tool | Where | Used for |
|---|---|---|
| llvm-mingw | `local/tools/llvm-mingw` | building the DLL |
| Python 3.12+ | `local/venv` (`python -m venv local/venv`, `local/venv/Scripts/pip install -r requirements.txt`) | analysis and test scripts |
| Ghidra 12 + JDK 21 | `local/tools/ghidra12`, `local/tools/jdk21` | decompiling the game |

None of these, and nothing derived from the game's files, are in the repository: they live in `local/` (git-ignored -
`work/` the test logs and builds, `tools/`, `venv/`, `games/ee/` and `games/aoc/` what is taken from each game).
Generate them locally:

```sh
# function names from the game's RTTI (vtables) -> local/work/names.tsv
mkdir -p local/work && local/venv/Scripts/python scripts/re/rtti_names.py local/work/names.tsv
# full decompilation to local/games/ee/decomp/<address>.c + local/games/ee/decomp/functions.tsv (takes a while)
cp "../Empire Earth.exe" local/work/EE.exe
local/tools/ghidra12/support/analyzeHeadless local/games/ee/ghidra_proj EE -import local/work/EE.exe \
    -scriptPath scripts/re/ghidra -postScript ExportDecomp.java "$(pwd)/local/games/ee/decomp"
# the spec's names, types, structures and enums into the project (before the decompilation above, so it reads with
# them; again after a change of spec/ - the spec's names win): local/work/spec_ee.json, then ApplySpec (aoc: AOC's project)
local/venv/Scripts/python scripts/re/spec_ghidra.py ee
local/tools/ghidra12/support/analyzeHeadless local/games/ee/ghidra_proj EE -process -noanalysis     -scriptPath scripts/re/ghidra -postScript ApplySpec.java "$(pwd)/local/work/spec_ee.json"
# the same for each engine DLL (scripts/re/port_dll.py; Art of Conquest's into local/games/aoc/decomp/dll/ from its folder)
local/tools/ghidra12/support/analyzeHeadless local/games/ee/ghidra_proj DLL_LowLevelEngine -import "../Low-Level Engine.dll" \
    -scriptPath scripts/re/ghidra -postScript ExportDecomp.java "$(pwd)/local/games/ee/decomp/dll/LowLevelEngine"
# string table of the German Language.dll -> local/games/ee/strings_de.json
local/venv/Scripts/python scripts/re/strtab.py "../Language.dll"
```

Helpers working on the dump and the exe:
- `scripts/re/fn.sh <hex>`: print a decompiled function.
- `scripts/re/refs.py <hex>`: list calls, jumps and pointers to an address.
- `scripts/re/disp.py <hex>`: find instructions using a displacement or offset.
- `scripts/re/pdis.py <hex>`: disassemble at an address.

[docs/engine-notes.md](docs/engine-notes.md) collects the verified addresses and data layouts.
The community's projects at [EE-modders](https://github.com/EE-modders) are a good second source (the game's
text in all languages, the DB file formats, other DLL mods).

## Testing

The DLL has an **autotest mode** and a **control pipe**, so AI games run unattended. Scenario and save
runs at the same speed are byte-identical, which makes A/B tests exact; random maps vary between runs, so
those are compared over several runs. See [docs/testing.md](docs/testing.md).

```sh
# one 75-minute AI-vs-AI game on a random map, logs to local/work/autotest_<stamp>_<tag>.csv
powershell -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 -Save newgame.ees -Tag _try \
    -IniExtra "random_map=Continental,ai_count=3,seed=101,avoid_towers=1"
# the 4-map suite for several variants, scored per variant
sh scripts/test/suite.sh 50 "base:" "smart:avoid_towers=1"
# talk to the running game
local/venv/Scripts/python scripts/test/eectl.py status | army 2 | diplo | persona | say 2 Hello | quit
```

## Project layout

```
dll/              the mod (C), one translation unit: eeaimod.c includes every part - the game start, the AI's update
                  hooks that call the parts, install, DllMain; persist.c the mod's state in saved games
  base/             the DLL's own helpers: log, patching, the pipe commands' REPLY, paths and the test slot, mistakes.c
                    the report of a mistake in the mod's files, math.c (the same bits on every PC), aimod.ini
                    (ini.c), watchpoints, headless_win.c test windows without focus or taskbar button (also in dinput.dll)
  game/             what the mod knows of the game: api/ the game's own code and objects, one module (api.c) - exe.c
                    the exe build, its addresses per build (gen/: the spec's C, generated; builds.h;
                    editor_layout.h generated), its objects typed (types.h), a file per object (unit.c, player.c,
                    attack.c ...: its fields by name, the game's functions on it, its hooks; per build:
                    spec/game/), game.c the globals, hooks.c the code patches; world.c
                    type data, game time, RTTI names, players.c the players (and the vanilla
                    ones, the colours), random.c the decisions' generator (alike on every PC), the unit types
                    (unit_ids.h, unitnames.c, unit_names.h generated) and roles.c what a type is,
                    grid.c the unit grid (iterator)
  engine/           fixes and features of the game itself: display.c / present_d3d.c windowed mode (display_text.c,
                    display_diag.c), camera.c scrolling, aim.c leading targets, sound.c, select.c, drawlist.c,
                    playername.c the menu name fix, scenario_ai.c the scenario AI fix
  ai/               the computer players: econ.c budget, epoch plan, build queue, workers, research; sites.c gathering
                    sites; attack.c attack routing, siege, unit choice; stats.c fight estimates; defence.c towers, walls, home guard; diplo.c
                    diplomacy; landing.c landings; navy.c / fleet.c the navy; settings.c aimod.ini's AI settings ...
  script/           expr.c the text language (parsed once into a tree; statements, functions) - it knows no game:
                    what it asks the program it runs in is script/host.h (eeaimod.c x_host_install sets the game's)
  game_script/      the game in the language: constants.c (epoch., resource., color., kind., unit.), relations.c
                    the players' values and relations (them.soldiers, them.kills(me) - from the AI's state)
  voice/            what the AI says: say.h the AI's one door to it (its lines, its personality's traits and
                    decisions, the language model), persona.c personalities, texts.c the text files, chat.c chat lines, llm.c the
                    language model
  test/             control.c the control pipe, testcmd.c / testopp.c test commands and the sea opponent, randmap.c
                    test-game setup, autotest.c, scripts.c the setup scripts (cmd_verbs.h, generated: the commands
                    they call), trace.c call tracing, ecolog.c the economy log
  settings/         rows.h every aimod.ini key as a row (kind, section, default, limits) and every section (order, kind,
                    title: aimod_config.exe's pages), settings.c reads them once into
                    g_set / g_set_ai (a value per AI difficulty) - the parts read those, never the file
  tool/             aimod_config.c: aimod_config.exe, the settings in a window (from base/, script/ and settings/ only:
                    the DLL's own rows and checks; the shipped aimod.ini built in as the help)
  dinput/           eesettings.c: dinput.dll, the game's settings in eesettings.ini instead of the registry
config/           aimod.ini and the aimod_ai personalities, shipped to players
tests/            what the tests run: scenarios/ the setup scripts (docs/scripts.md) that turn a save into a test
                  scenario (attacks, sieges, raids), probes/ the one-off setups of single measurements, config/
                  autotest.ini and the test configs, text_engine_cases.txt the text engine comparison's cases
scripts/          game_dir.py / .sh: where the game is (every script's)
  build/          the build's checks and generators (layers.py, style.py, spec_check.py, gen_api.py, persist_layout.py,
                  gen_cmd_verbs.py, gen_*.py for the docs and the grammar), code_same.py, install.sh
  test/           run_test.ps1 and the runners on top of it (suite.sh, scen.sh, h2h.sh, vanilla.sh, jobs.sh),
                  reserve.sh, eectl.py the control client, ee_mcp.py the MCP server, work_prune.ps1
  results/        the log readers and scorers (score.py, h2h.py, vanilla.py ...; logs.py opens the gzipped logs)
  re/             reverse engineering: the exe and dump helpers, port_*.py for Art of Conquest, gen_units.py +
                  dat_read.py (docs/units.md and dll/game/unit_names.h from the game data: the community DB editor's
                  field layouts and English strings, downloaded into local/games/ee/dats/), make_cpbehavior.py (a
                  tuned dbcpbehavior.dat; superseded by aimod.ini), ghidra/ the headless scripts that name and
                  decompile the exe (ExportDecomp) and put the spec into a project (ApplySpec, from spec_ghidra.py's file)
extensions/       eeaimod-texts: the VS Code extension for the .eeai files
local/            git-ignored: this machine's tools, test output and what is derived from the games (Setup)
docs/             engine notes, AI analysis, testing guide, units.md (every type: id, identifier, stats),
                  scripts.md (the setup scripts)
```

## License

Copyright (C) 2026 Paul Hirch.

This mod is free software under the [GNU General Public License v3.0](LICENSE): you may use, share and change
it, and anything you distribute that is built from it must be published under the same license, with its
source code.

Empire Earth and its files belong to their rights holders and are not part of this repository; files derived
from them (decompilation, extracted strings and tables) are kept out of git (see `.gitignore`). Addresses,
offsets and similar facts about the game, including those cross-checked with the community's GPL-3.0 projects
at [EE-modders](https://github.com/EE-modders), are documented in `docs/`.
