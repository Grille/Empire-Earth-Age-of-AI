# Navy: ship roles, what the AI does at sea, baseline and plan

The playtester: on water and island maps a strong standing navy decides the game. Lose sea control and you
cannot expand or counter-attack. "Each ship has specific roles and counters." This file holds the research
(the ship data, the game's own AI), a baseline on island maps, and a staged design. Stage 0 (the food hole) is
done; the rest is not built yet - apart from the fishing boat choice, dll/navy.c only observes.

The playtester's notes on this plan:
- **Farms are always needed.** "Fish is very useful but a limited resource; fishing boats have to go further and
  further out as the game goes, which makes them hard to protect." Fish is a bonus on top of farms, never the
  food base.
- **The x4-5 of catapults and cannons against battleships is an equalizer** for the battleships' hit points and
  attack, not a weakness of battleships alone. Frigates and galleys still do badly against cannons and artillery
  (long range, high attack, x1 against them), so the whole fleet must keep away from coastal siege and towers.

Tools:
- `navy types A B [all]` / `navy table` (pipe or setup). `scripts/navy_types.setup` dumps every type and the hit
  table into the log.
- `scripts/navy.py types <log>` builds the role table below.
- `# navy:` log lines (autotest, every 2 minutes per AI) and `scripts/navy.py <logs>` for the baseline.

## 1. Ship roles from the game's data

**Damage model.** A hit does `attack x class table x hit factor - armour`, at least 1 (engine-notes.md, Ranged attacks):
- The **class table** 0x8D2050 is 1 almost everywhere.
- The **hit factor** is the table 0x8D200C [target category, type +0x4C4][weapon kind, type +0x4C8]. 0 means
  the unit cannot hit that target. The game's attack subgroups test exactly this (0x4504FD), and landing.c's
  `ld_factor` uses it.
- **Armour** is indexed by the attacker's damage class. Every ship has class 5 and armour 0 there, so armour
  plays no part at sea.
- So the **factor x attack / reload** is the whole story.

**Ship families** (+0x4B4):

| Family | Ships |
|---|---|
| 4 | warships |
| 10 | submarines |
| 18 | transports |
| 21 | carriers |
| 22 | fishing boats |

Fish (315) are family 4 with attack 0.

**Target categories:**
- c9: frigates, cruisers, transports and fishing boats
- c21: battleships
- c47: galleys / galleons
- c7: submarines
- c50: carriers
- c37: towers
- c3: other buildings
- c46: AA buildings

### Weapon kinds and their factors

Rows are weapon kinds, columns are the factor against each target category:

| Kind | Ships (epochs) | frig c9 | bship c21 | galley c47 | sub c7 | carrier c50 | tower c37 | bldg c3 | land | air |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 frigate | War Raft e2, Copper..Juggernaut Frigate e3-13 (also the heroes Golden Hind, Agincourt) | 1 | 1 | 1 | **1** | 1 | 0.4 | 0.45 | 0-2 (x2 archers, infantry with guns) | 0 |
| 3 galley | Copper..Royal Galley/Galleon e3-9 (no later one) | 1 | **3.8** | 1 | 0 | 1 | **2** | **3.95** | x2 almost everything | 0 |
| 34 battleship | Copper..Leviathan e3-13 | 1 | 1 | **0.25** | 0 | 1 | 0.4 | 0.35 | 0-1 | 0 |
| 6 catapult ship | Bronze e4, Middle Ages e6 (251, 263) | 1 | **4.5** | 1 | 0 | 1 | **3** | 1.5 | 0.7-1.4 | 0 |
| 18 attack sub | U-Boat e10, Nautilus e12, Hammerhead e14 | 1 | **3.5** | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| 51 missile sub | Trident e12, Triton e14 (range 24, attack 800-1000, 15 s reload) | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 0 | 0 |
| 12 cruiser | Gunboat e8, Cruiser e10, Sagittarian e13 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | **1.2-1.5** |
| carrier | Enterprise e11, Nexus e13 (no weapon) | | | | | | | | | |

Carrier fighters (e11+) do x1.9 to frigates, x3 to battleships and x1.5 to galleys and carriers.

### Costs and stats (one epoch line, e9)

| Ship | Cost | Hit points | Attack | Range | Speed | Reload |
|---|---|---|---|---|---|---|
| Frigate | 125 wood + 125 iron | 1060 | 51 | 8 | 0.85 | 2 s |
| Galleon | 125 wood + 125 iron | 1100 | 24 | 8 | 0.75 | 2 s |
| Battleship | 250 wood + 250 gold | 4525 | 200 (area 1) | 8 | 0.55 | 4 s |

- Attack subs cost 190 wood + 190 iron.
- Missile subs cost 600 gold + 600 iron.
- Cruisers cost 150 wood + 150 gold.
- Fishing boats cost 50 wood.
- Transports cost 250 wood.

So the game's **gold and iron are the navy's resources**, and wood is needed for everything.

### The triangle

Seconds for 1000 resources of the row to sink 1000 resources of the column. The time per shot is the reload
+ 2 s, as measured in duels. Battleship area damage is left out. Lower is better; `-` means it cannot hit.

| e5 | frigate | galley | battleship |
|---|---|---|---|
| Byzantine Frigate | 76 | **84** | 149 |
| Byzantine Galley | 178 | 196 | **91** |
| Byzantine Battleship | **53** | 235 | 104 |

| e10 | frigate | galleon | battleship | U-Boat |
|---|---|---|---|---|
| Good Hope Frigate | 98 | **80** | 209 | **48** |
| Royal Galleon (last galley) | 225 | 183 | 126 | - |
| Dreadnought | **70** | 230 | 150 | - |
| U-Boat | 126 | 103 | **77** | 62 |

The counters, every epoch:
- **Battleships beat frigates.** Battleships are also the line ships: the most hit points and area damage.
- **Galleys beat battleships** and bombard buildings and towers.
- **Frigates beat galleys** and are the only ships that hit submarines.

From e10:
- **Submarines beat battleships and galleys.** Battleships and galleys cannot hit them. Frigates beat
  submarines.
- Missile subs only bombard: towers and buildings from 24 tiles. They are fragile, and a frigate sinks
  1000 resources of them in 10 s.
- Only cruisers (from e8) hit aircraft from the sea. Frigates and battleships have factor 0 against air.

Bombardment:
- Per resource, galleys and catapult ships hurt towers most (x2 / x3).
- Battleships hit towers with x0.4, but with 200+ attack.
- landing.c's bombardment fleet already picks by factor >= 1 against the beach's tower, so galleys and catapult
  ships.

### Land and air against ships

- **Siege and artillery hit battleships x4.5**: catapult e4, trebuchet, the cannons e7-9, bombard,
  artillery, howitzer x5, Colossus and Paladin. So do War Elephants (x4.5), flamethrowers (x5), AT guns (x2)
  and every bomber (x4).
- **Against frigates and galleys the same units do x1.** The x4.5 against battleships is an equalizer for the
  battleships' hit points and attack (the playtester). x1 is no protection: cannons and artillery outrange ships
  and hit hard, and frigates and galleys have a quarter of a battleship's hit points.
- Every ranged land unit, every tower and most aircraft hit frigates and galleys x1. Dragoons and Carabineers
  do x0.44. AA units cannot.
- **Submarines are hit only by towers, frigates, attack subs and Sea King helicopters** (e12, e14).
- So **the whole fleet** must stay out of the reach of coastal siege, artillery and towers (the playtester:
  frigates and galleys still do badly against cannons and artillery). Battleships also out of bomber reach.
  Frigates and galleys may stand off a coast held only by infantry.

## 2. What the game's own AI does with ships

From the code, via a read-only research pass. Addresses are in the decomp.

- **Docks.**
  - The building plan 0x46CD18 builds docks (36) and Navy Yards (65) up to "Min military buildings"
    (+0xDD8.. per difficulty).
  - Both are set to 0 when world +0x181730 bit 1 is set (probably "no water").
  - +0xEFA (water map with >= 10 fish, 0x4307A7) turns the field passes into fishing: no field builders, fishing
    boats instead (0x48FB05).
- **Fishing boats.**
  - 0x46D209 picks the type by epoch: below 3 the Stone boat 108, below 9 the Bronze boat 109, below 14 the
    Modern boat 211. The Imperial boat 365 and the Digital boat 332 are never picked.
  - It orders one only while none of that type exists or is in production.
  - **The Modern boat is an e11 unit (tech 11090).** So in e9-e10 the AI asks for a boat it cannot build: no
    fishing at all. With +0xEFA it builds no fields either.
  - Baseline: food stops at minute ~30 (below).
- **Warships.**
  - The unit manager's 14 request lists are per unit epoch (tech id / 1000 - 1), not per kind.
  - **Unit choice.** With favourites (strategy +0x34 with +0xBAC naval), it takes a fixed favourite, else a
    random trainable candidate.
  - **Without favourites, an RPS vote.** 0x49352A / 0x495B40 add 3/2/1 to the counter classes of each enemy
    type (typedata +0x4D0/+0x4D4/+0x4D8). The naval part is weighted by "Naval Unit Weight" +0xBD0 (0.4) on
    water maps.
  - There is no counter against the enemy fleet as such.
  - The baseline shows the AI asking for frigates, galleys and battleships all game.
- **Standing fleet: "naval maintenance" 0x4779FA.**
  - It runs from the defence update 0x476428, every 8 s, with the Defend toggle +0xF00.
  - It counts own ships (typedata +0x5D4 bit 2, no fishing) against "Ships to Maintain": record 5,
    +0xCB0/+0xCB4/+0xCB8, roughly 0 on easy and 2-3 otherwise.
  - If it is short, it deletes own units for room (0x430D40, skipped with Ignorieren +0xF02), then opens a Defend
    group for the missing count against a random enemy. This repeats every record 118 = 4 minutes.
  - That is the only standing fleet: 2-3 ships.
- **Sea attack subgroups** (EECPASGSea, update 0x44ED11).
  - Goals come from 0x45462A:
    - With chance 0.25, a random unit of the target player in this subgroup's ocean or a neighbouring water area
      (mission 7).
    - Otherwise a goal from the pool (chance min(0.9, 0.5 + 0.05 n)).
    - Otherwise one of the player's water areas (mission 1), else home.
  - It does not seek the enemy fleet, the fishing boats, the transports or the docks.
  - States: 0x1776 attack the goal, 0x1772 wait at a place, 3000 combat, 0xBC1 -> 0x1777 retreat home.
  - Nothing escorts or protects the fishing boats, and fishing boats do not flee.
- **What the mod does today.**
  - Every ship request goes through the military budget (econ.c `military = !is_citizen_type`), fishing boats
    included. A refused ship is simply not trained.
  - `unit_substitution` excluded family 4 only, so a refused submarine, transport, carrier or fishing boat
    became a land soldier after 90 s of saving (the baseline had fishing boat 109 failed 72 times, transports
    111/113 in the failed lists). **Fixed (stage 0):** every water family (4, 10, 18, 21, 22) is excluded.
  - landing.c:
    - Leads its own transports and escorts. It skips the sea, transport and air subgroups' updates and refuses
      their orders while it leads them (vf57).
    - Trains transports and, with `landing_bombard` (off by default), a bombardment fleet (`ld_war_type`:
      family 4 only, so no missile subs).
  - Nothing else touches warships.

## 3. Baseline (shipped config, 2 AIs, seed 101 map, 6 game seeds, 60 minutes, 2026-10-18)

Run with `REPEAT=6 sh scripts/scen.sh 60 islands "random_map=<map>,map_size=0,ai_count=2,seed=101,start_epoch=<e>" "nv..:"`,
then `python scripts/navy.py work/autotest_*_suite20261018_214[1-5]_nv*.csv`. All 24 runs finished with no
crash and no game decided. Values are per AI averages:

| | Small Isl. e5 | Large Isl. e5 | Small Isl. e9 | Large Isl. e9 |
|---|---|---|---|---|
| docks (end) | 1.9 | 2.2 | 1.7 | 1.5 |
| fishing boats at 30 min / most | 10.3 / 17.3 | 8.6 / 12.4 | **0 / 0** | **0 / 0** |
| food from fish / all food (60 min) | 4340 / 10750 (40%) | 3250 / 14190 (23%) | 0 / **2750** | 0 / **2680** |
| warships + subs at 30 min / end / most | 0.9 / 1.8 / 2.8 | 0.4 / 1.2 / 2.4 | 0.5 / 2.1 / 3.2 | 0.8 / 2.2 / 4.2 |
| transports (end) | 4.3 | 3.5 | 2.3 | 1.1 |
| navy lost (ships, subs, transports) | 6.8 | 7.8 | 6.8 | 12.9 |
| fishing boats lost | 7.0 | 3.8 | 0 | 0 |
| own / enemy warships in own waters (average) | 0.2 / 0.3 | 0.2 / 0.2 | 0.3 / 0.4 | 0.5 / 0.4 |
| gold + iron income a minute | 145 | 193 | 161 | 236 |
| landings sailed / ashore (soldiers) | 3.4 / 3.0 (35) | 2.5 / 2.3 (30) | 2.1 / 1.6 (14) | 1.8 / 1.5 (14) |
| failed ship requests (in the top-4 failed lists) | 1720 | 1270 | 1260 | 940 |

**Warship types seen** (AIs that had one, out of 12):
- e5 start: Middle Age Frigate 7-11, Galley 7, Battleship 2-3, Catapult Ship 3 (Large).
- e9 start: Good Hope Frigate 10-12, Dreadnought 7-8, U-Boat 6-10.

What the baseline shows:
- **No navy.** At most 2-4 warships at once, usually 0-2. Nobody holds any water: own and enemy warships in an
  AI's waters average 0.2-0.5.
- **The AI asks for ships all game and is refused.** About 1000-1700 failed ship requests per AI per hour, by
  iron or gold, while the army takes the military account.
- **Fishing.**
  - At e5 fishing is 23-40% of all food.
  - The fishing boats die unprotected: 7 lost per AI on Small Islands.
  - In e9-e10 there is no fishing and no farming: food income stops at minute ~30, 2700 in the hour against
    10-14000 from e5.
  - Continental e9 has granaries and 20 farmers (`bombing_late` logs). This is the game's boat-type bug
    above, plus +0xEFA keeping the field builders away.
- Landings succeed without any fight for the sea, because the enemy has no fleet either.

## 4. Design: a standing navy

### Stage 0 - the e9-e10 food hole (economy, before any navy) - DONE

- **Fishing boat** ([all] `fishing_boat`, default 1; dll/navy.c `hook_fish_type`). Both callers of the game's pick
  0x46D209 (0x48F819 counts the boats, 0x48FB05 orders one) get the newest fishing boat (family 22, highest tech)
  the player has available by its tech record's test (landing.c `ld_type_avail`), every epoch: 108 (tech 2000),
  109 (4005), 365 (8090), 211 (11090), 332 (13098). None available: the game's pick. `# fishing:` log line when
  the choice changes (e9/e10 starts: "the game asks for boat 211, trains 365").
- **Farms on water maps** (econ.c `more_granaries`). +0xEFA keeps the game's own field builders away and its
  building plan counts the fields as wanted, so on islands it gets no granary from the plan. In e9 starts the
  build list brought none either, and the mod's field passes returned before `more_granaries` while the player
  had no field at all. Now the granary pass runs without fields, and the first granary comes once food lacks 2
  citizens (6 for each further one, as before). The mod's passes build and farm the fields on every map; the
  farmers stay at the game's food target, so fish income lowers the farms' share on its own.
- **Unit substitution** skips every water family (above).

Result (shipped config, 2 AIs, seed 101 map, 12 game seeds, 60 min; HEAD 325e4fb vs stage 0; per AI;
`scripts/navy.py`, score.py):

| | Small Isl. e5 | Large Isl. e5 | Small Isl. e9 | Large Isl. e9 | Continental e5 (6) |
|---|---|---|---|---|---|
| all food by min 20 / 40 / 60 | 2109/7618/11137 -> 2069/8018/12137 | 1861/7489/12373 -> 1794/7873/12852 | 1265/2566/2662 -> 1477/5072/8625 | 1085/2489/2604 -> 1249/5112/9587 | 1400/5871/10441 -> 1630/7044/11620 |
| food from fish (60 min) | 4424 -> 4517 | 2824 -> 3068 | 0 -> 3708 | 0 -> 2398 | 0 -> 0 |
| fishing boats at 30 min / most | 10.8/16.0 -> 10.2/15.6 | 9.7/14.4 -> 9.5/14.3 | 0/0 -> 9.5/18.0 | 0/0 -> 8.0/14.6 | - |
| granaries / citizens on food (end) | 1.8/14.3 -> 1.7/15.8 | 2.7/17.7 -> 2.9/14.3 | 0/0.2 -> 1.5/13.2 | 0/0.3 -> 1.7/13.5 | 2.7/15.0 -> 2.8/10.1 |
| workers at the end | 83.1 -> 81.1 | 85.5 -> 85.6 | 41.5 -> 79.7 | 40.8 -> 82.5 | 67.3 -> 70.0 |
| soldiers (5-min lines average) | 23.8 -> 23.1 | 25.6 -> 24.9 | 12.2 -> 16.6 | 12.4 -> 19.6 | 13.8 -> 13.4 |
| epoch | 5.42 -> 5.42 | 5.33 -> 5.38 | 9.42 -> 9.42 | 9.38 -> 9.25 | 5.42 -> 5.33 |
| kills / losses | 77/83 -> 73/89 | 96/107 -> 87/100 | 24/31 -> 56/62 | 38/45 -> 57/76 | 108/106 -> 113/108 |
| fishing boats lost | 6.0 -> 4.6 | 7.3 -> 7.7 | 0 -> 9.0 | 0 -> 10.0 | - |
| landings sailed / ashore | 3.2/2.9 -> 3.5/3.3 | 3.0/2.9 -> 3.0/2.8 | 1.4/1.1 -> 2.1/1.9 | 1.1/1.0 -> 1.6/1.3 | - |

54 runs each side, no crash. e9 islands: food x3.2-3.7, workers doubled, the army 35-60% larger, fish 25-43% of
the food. e5 and land within noise (std. error: workers ~1.5, kills 5-6, epoch 0.1). The fishing boats now die
unprotected in e9 too (9-10 per AI) - the playtester's point: farms first, the boats need the fleet of stage 3.

### Stage 1 - water-map detection and the navy account

- **Water map:** world +0x181730 bit 0, or more concretely: the enemy's buildings stand on another continent
  (area_id kind 2) reachable only over an ocean (kind 1), as landing.c already finds. Navy on only then. On
  mixed maps, scale it by the share of enemy buildings across water.
- **A navy account** next to the military account (econ.c budget): a share of gold, iron and wood income, e.g.
  25% on islands.
  - Ship requests (the game's and the mod's) pay from it, so the army no longer starves them, and the navy no
    longer takes from the army's share.
  - Fishing boats are paid as economy, not military.
  - Docks are kept at 2+ (one per coast near its fish) through the existing build-queue path (econ.c retry /
    0x62EF8A).
- The population room is held like `landing_room_reserve` does for the bombardment fleet.

### Stage 2 - composition by roles

- **The core every epoch:** frigates first. They hit everything afloat, are cheap (wood + iron) and are the
  only answer to submarines. Then battleships as the line ships, about 1 per 2 frigates. Use the existing
  type-availability test `ld_type_avail` and train only types the game can train now.
- **React to the seen enemy fleet** (intel: seen ships, remembered 10 minutes). For each enemy category, the
  type in reach with the best factor x attack / reload per cost, taken from the hit table at run time (no
  hardcoded roles, as landing.c does):
  - enemy battleships -> galleys (to e9), submarines (e10+), catapult ships (e4-6);
  - enemy galleys -> frigates;
  - enemy submarines -> frigates;
  - enemy aircraft over the water -> cruisers (e8+).
- **Mind the land counters.** Keep the whole fleet out of reach of the enemy's seen coastal siege, artillery and
  towers, battleships also of bombers (x4.5 / x4 against them; frigates and galleys do badly there too). Bombard
  with galleys and missile subs (towers x1 from 24 tiles) from outside that reach.
- **Size:** at least the enemy's seen fleet strength x 1.3 (sqrt(hp x attack) as landing.c), at least 4 warships
  from the epoch of docks on islands, and at most the navy account.

### Stage 3 - fleet tasks (a mod-led sea subgroup, as landing.c leads its units)

- **Guard:** home waters and the fishing grounds. A patrol point between the docks and the fish, and the
  enemy's ships within 15 tiles of own buildings or fishing boats attacked first. Fishing boats near enemy
  warships are pulled back to the dock (the user's "citizens stay safe" for boats).
- **Hunt** when stronger than the seen enemy fleet (1.3x): its fleet, then its fishing boats and transports, then
  its docks (galleys). Retreat at 50% strength or when a fight turns (the attack_retreat estimate).
- **Escort** landings: landing.c already takes escorts by the table. The standing fleet becomes its source, and
  the landing waits for sea control instead of building its own fleet.
- **Bombard:** the landing's beach towers with galleys or catapult ships and missile subs, out of siege reach.
- **Repair:** damaged ships go to the dock. The game's repair round sends citizens, check that it reaches ships.
  Losses are replaced from the account.

### Stage 4 - tie-in and measurement

- landing.c: plan landings only with sea control near the route, or with escorts from the standing fleet.
  `landing_bombard` uses the fleet instead of training on demand.
- **Success measures** (`# navy:` and navy.py):
  - sea control: own vs enemy warships in own and enemy waters, and the share of looks holding home waters;
  - fishing boats kept vs lost, and food from fish;
  - enemy fleet sunk vs own lost (resource-weighted);
  - landings ashore and enemy landings stopped;
  - games won within 90 minutes (2 AIs and the reference-style 3-AI maps).
- A/B: navy on vs off, both AIs, 20 seeds, Small/Large Islands e5/e9. Also one AI with and one without, for the
  head-to-head effect (`scripts/h2h.sh`).

### Risks

- **The engine's own subgroup orders.**
  - The game's sea subgroups, the transport manager's unload and the maintenance group will order the same
    ships. landing.c needed hooks on vf57 and the updates for that (transports carried soldiers off).
  - The standing fleet must own its ships the same way: `landing_leads`-like membership, the update skipped,
    vf57 refused.
  - Or keep the ships out of the game's groups altogether: the maintenance group 0x4779FA may delete units for
    room; disable it while the mod's navy runs.
- **Starving the army:** the navy account takes gold and iron the land war needs. On mixed maps keep the share
  small and measure land results too.
- **Population:** ships take military slots. The maintenance code deletes units for naval room. The mod must not
  push the army below what home defence needs.
- **Pathing:** fleets at sea near shores and between islands can stick. The transports' unload problems may also
  show up as patrol problems at narrow waters.
- **Honest vision:** react only to ships seen (intel), not to the floor of the human's units.
