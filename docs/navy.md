# Navy: ship roles, what the AI does at sea, baseline and plan

The playtester: on water and island maps a strong standing navy decides the game. Lose sea control and you
cannot expand or counter-attack. "Each ship has specific roles and counters." This file holds the research
(the ship data, the game's own AI), a baseline on island maps, and a staged design. Stage 0 (the food hole) and
stage 1 (the navy's money, docks and room) are done. Stage 2 (the fleet mix) is built but off by default: it brought
no head-to-head gain. Stage 3 (the mod leads the fleet) is built and off by default too: it wins the sea (about 2
warships a game net in head-to-head) but brings no gain on land yet. Stage 4 (a big fleet in task forces,
`navy_fleet=2`) beat the control AI (+18 kills - losses a game, more fish, fewer boats lost) but not the human-like sea
opponent. Stage 5 (mass under the towers before striking, the mines worked for the navy) makes `navy_fleet=2` the
default: +24 kills - losses against the control AI, and the sea exchange against the sea opponent positive on every
map.

The playtester's notes on this plan:
- **Farms are always needed.** "Fish is very useful but a limited resource; fishing boats have to go further and
  further out as the game goes, which makes them hard to protect." Fish is a bonus on top of farms, never the
  food base.
- **The x4-5 of catapults and cannons against battleships is an equalizer** for the battleships' hit points and
  attack, not a weakness of battleships alone. Frigates and galleys still do badly against cannons and artillery
  (long range, high attack, x1 against them), so the whole fleet must keep away from coastal siege and towers.

Tools:
- `navy types A B [all]` / `navy table` (pipe or setup). `scripts/navy_types.eeai` dumps every type and the hit
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
  - Every ship request went through the military budget (econ.c `military = !is_citizen_type`), fishing boats
    included. A refused ship is simply not trained. **Stage 1:** on water maps warships are paid from a navy
    account, fishing boats and a landing's transports as economy.
  - `unit_substitution` excluded family 4 only, so a refused submarine, transport, carrier or fishing boat
    became a land soldier after 90 s of saving (the baseline had fishing boat 109 failed 72 times, transports
    111/113 in the failed lists). **Fixed (stage 0):** every water family (4, 10, 18, 21, 22) is excluded.
  - landing.c:
    - Leads its own transports and escorts. It skips the sea, transport and air subgroups' updates and refuses
      their orders while it leads them (vf57).
    - Trains transports and, with `landing_bombard` (off by default), a bombardment fleet (`ld_war_type`:
      family 4 only, so no missile subs).
  - Nothing else touches warships (stage 1 pays for them, keeps 2-3 docks and holds room; it gives no orders).

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

- **Fishing boat** ([all] `fishing_boat`, default 1; dll/ai/navy.c `hook_fish_type`). Both callers of the game's pick
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

### Stage 1 - water-map detection and the navy account - DONE

Built (dll/ai/navy.c, econ.c; `[all] navy=1`, `navy_share=25%`):

- **Water map** (`navy_detect`, once per AI 10 s in): the home continent (area_id kind 2 at the build manager's
  base point) and every enemy's buildings - across the water when none stands on that continent. Strength = the
  share of enemies across the water: 1 islands, 0 land, between mixed. `# navy: P<n> map at 0.2 min: islands
  (strength 1.00) - 1 of 1 enemies across the water, water 25% of the map, ...; the game's water flag 1, +0xEFA 1`.
  The game's own flag (world 0x9183BC +0x181730 bit 0) agrees: 1 on Small/Large Islands, 0 on Continental (20%
  water, enemies on the same continent: "land").
- **Navy account:** navy_share % x strength of every wood, gold and iron income (hook_add_resource), at most 600
  of each.
  - Every other order (army, citizens, fishing boats) sees it as reserve: the army can't take it. Once it pays the
    warship the game last asked for (within 2 min), that ship's whole cost is held (the army took the iron down to
    the reserve again and again while the account held 44 of it).
  - Warship, submarine and carrier orders spend the account - one pool: a frigate's iron may come from the
    account's gold, the rest is charged to the others - or the stock beyond the hoard cap, never the military
    account.
  - Not held from the priority orders (siege, a landing's transports and bombardment fleet). With 1200 and the
    hold on them, a landing's transports waited for wood: landings sailed 4.0 -> 1.3 per AI (Small Islands e5).
  - Fishing boats and a landing's transports are economy (not charged to the military account).
  - `# navy: P<n> budget at N min:` every 5 minutes: share, account, what it paid, ship orders paid/refused by
    type, warships, docks (built, asked), fishing boats, the room held.
- **Docks** (`navy_docks`): once a minute fewer than 2 (on islands 3 while the account waits to pay a warship or
  12+ fishing boats share them), none unfinished or queued, buildable and payable -> one more through the build
  manager's own queue and placement (0x46802D(bm, 36, 1, 0)). Every requested dock was placed.
- **Room** (`navy_room_reserve`): with a finished dock, room for the missing warships (up to 4 x strength, no more
  than the account pays for of the type last asked for) is held from the army's orders (econ.c train_call, as
  `landing_room_reserve`).
- **What the game's AI buys** with it (paid per AI in the hour, A/B runs): e5 Middle Age Frigate 6.2-6.6, Galley
  2.0-2.9, Catapult Ship 1.0-1.3, Battleship 0.9-1.1; e9 Good Hope Frigate 5.0-7.5, U-Boat 1.7-2.4, Dreadnought
  1.6-2.1. Frigates first, by the RPS vote - and it still asks 300-750 times an hour for ships the account can't
  pay yet (iron).

Result (shipped config, 2 AIs, seed 101 map, 60 min, same DLL, `scripts/navy_ab.py` on the `# navy:` / `# battle:`
lines; per AI; std. error in docs/testing.md):

| navy 0 -> 1, both AIs, 20 seeds | Small Isl. e5 | Large Isl. e5 | Small Isl. e9 | Large Isl. e9 | Continental e5 (6) |
|---|---|---|---|---|---|
| warships most / end / average | 2.6/1.4/0.6 -> 3.6/1.4/1.1 | 2.6/1.2/0.6 -> 4.2/1.9/1.4 | 2.7/1.3/0.7 -> 3.8/2.0/1.1 | 3.8/1.8/0.9 -> 4.6/2.4/1.2 | 2.5/2.2/0.8 -> 1.3/1.2/0.4 |
| own / enemy warships in own waters | 0.2/0.3 -> 0.4/0.6 | 0.2/0.3 -> 0.6/0.8 | 0.2/0.3 -> 0.3/0.6 | 0.4/0.4 -> 0.5/0.6 | - |
| navy lost (= sunk) | 7.4 -> 13.4 | 8.0 -> 14.8 | 8.1 -> 9.5 | 10.6 -> 13.6 | 1.1 -> 0.9 |
| docks (end) | 2.0 -> 2.9 | 2.1 -> 3.1 | 1.4 -> 3.0 | 1.6 -> 3.2 | 1.0 -> 0.9 |
| fishing boats most / lost; food from fish | 17.3/5.8; 4418 -> 12.5/6.8; 3525 | 14.8/5.2; 3579 -> 13.2/6.5; 2850 | 16.3/7.5; 3352 -> 17.8/6.8; 3614 | 14.4/8.8; 2492 -> 16.2/8.6; 2017 | - |
| all food in the hour | 11317 -> 10933 | 13293 -> 12491 | 8690 -> 9398 | 9521 -> 11386 | 10820 -> 11605 |
| gold + iron income a minute | 148 -> 128 | 176 -> 177 | 173 -> 167 | 249 -> 245 | 136 -> 152 |
| soldiers (5-min lines) | 23.6 -> 21.7 | 28.2 -> 23.9 | 18.2 -> 15.6 | 18.7 -> 18.8 | 15.0 -> 14.4 |
| kills / losses | 76/84 -> 67/76 | 74/83 -> 75/90 | 55/59 -> 43/50 | 58/72 -> 57/70 | 109/95 -> 109/110 |
| landings sailed / ashore | 3.5/3.2 -> 1.9/1.6 | 2.2/2.1 -> 1.8/1.6 | 1.9/1.6 -> 1.5/1.2 | 1.8/1.5 -> 1.3/1.1 | 2.4/2.1 -> 1.9/1.8 |

With both AIs on, each holds about twice the warships, the waters fill on both sides and nobody wins them: twice
the ships are sunk, the enemy's warships take fishing boats, and the landings wait for the sea ("waits - on the
way N afloat", "waits for the warships") - the landing counts the enemy fleet, and no fleet clears the way (stage
3). Fewer landings, fewer kills; the army 2-4 soldiers smaller on Small Islands and Large e5. No game decided
either way, no crash (172 runs).

**Head-to-head** (one AI with the navy, `[autotest] variant_players=<p>, v_navy=1`, navy=0 for the other; 10 seeds
per side; the same slot with and without the navy, so the map's slot advantage cancels):

| per AI, the slot without -> with the navy | kills / losses | buildings razed / lost | food from fish, fishing boats lost | soldiers |
|---|---|---|---|---|
| Small e5 P3 | 101/95 -> 99/85 | 8.5/2.3 -> 6.5/0.8 | 4387, 6.2 -> 5566, 1.7 | 28.5 -> 29.1 |
| Small e5 P2 | 47/87 -> 62/76 | 0.8/2.1 -> 1.7/4.7 | 2351, 12.7 -> 3025, 8.8 | 21.4 -> 13.4 |
| Small e9 P3 | 35/60 -> 56/50 | 9.4/0.3 -> 4.9/0.2 | 3643, 4.6 -> 5928, 3.2 | 20.0 -> 22.0 |
| Small e9 P2 | 29/55 -> 39/35 | 0.1/2.2 -> 0/3.9 | 1982, 16.6 -> 2616, 5.2 | 10.7 -> 9.1 |
| Large e5 P3 | 79/140 -> 88/92 | 0.8/8.8 -> 3.3/2.7 | 1513, 18.3 -> 3959, 4.6 | 27.1 -> 27.9 |
| Large e5 P2 | 57/73 -> 92/52 | 1.8/1.5 -> 3.7/0.5 | 2666, 5.6 -> 3005, 3.3 | 25.2 -> 25.7 |
| Large e9 P3 | 68/84 -> 71/82 | 1.9/1.4 -> 1.6/7.1 | 2047, 9.3 -> 2830, 10.0 | 19.8 -> 17.9 |
| Large e9 P2 | 47/60 -> 49/55 | 4.5/0.3 -> 0.4/1.0 | 2212, 7.8 -> 2277, 4.4 | 19.1 -> 17.8 |

The AI with the navy has the better kill/loss ratio in every pairing (0.53-1.06 -> 0.82-1.75), more food from
fish in all 8 and fewer fishing boats lost in 7; its own warships in its waters 0.1-0.4 -> 0.3-0.7 on average. Its army is about as large, except Small e5 P2 (21 -> 13). Buildings lost are mixed (noisy: std. error
1-5). **navy_share 15** (Small/Large e5, 10 seeds per side): the same picture - kills/losses 105/94 and 73/67 on
Small, 106/102 and 83/62 on Large with the navy against 88/118, 60/87, 88/123, 66/85 without; soldiers 32.5, 19.1,
29.3, 27.3 (with 25: 29.1, 13.4, 27.9, 25.7). Not distinguishable from 25 at 10 seeds; 25 stays (the playtester's
figure).

**Default on**: it helps the AI that has it against one that hasn't (a human without a fleet), and on land maps
nothing changes. Against each other two navy AIs only trade ships until stages 2-3 give the fleet a job.

### Stage 2 - composition by roles - BUILT, OFF (`navy_mix=0`)

Built (dll/ai/navy.c `navy_pick`, the hook in econ.c `hook_train_units`; `[all] navy_mix`, `[autotest] v_navy_mix`):

- **Where:** a warship or submarine order the navy account pays (`navy_pays`) from one of the game's own training
  sites. The mod's own orders and a landing's orders are not touched. Carriers stay the game's choice. `req` points
  into the unit manager's request list, so it gets its own type back after the call. The game then counts the
  entry down as if its own type came (0x490AD6 subtracts what 0x49014F trained): no duplicate orders, as with the
  land unit substitution.
- **Roles from the hit table** (0x8D200C, categories c7/c9/c21/c47, family):
  - F: frigates (factor > 0 against c7, the submarines);
  - L: battleships (category c21);
  - A: galleys and catapult ships (x3 or more against c21);
  - S: attack submarines (family 10 that hits ships);
  - AA: cruisers (hit only aircraft).
  - Not used: the Naval Mine 386 (family 4, category 22) and the hero ships 268 (Agincourt, tech 9035, picked
    before the filter and never trained) and 611. Missile subs hit no ship and are left out too.
- **The wanted shares:**
  - Base mix: F 2 : L 1 : A 0.5. Once an attack sub is available (e10): S 1 in place of A.
  - Enemy ships seen (intel memory, 10 minutes; with honest_vision 0, what it sees now): + 0.5 F per galley or sub,
    + 0.25 L per frigate, + 0.75 A/S per battleship.
  - AA only with aircraft seen (a carrier counts as 3): 0.5 + aircraft / 8, at most 2.
  - L is scaled by the non-submarine share of the enemy fleet, and by 4 / (4 + n), where n is the seen enemy units
    and buildings within 4 tiles of the sea that hit battleships x3 or more (the playtester's equalizer).
- **Each order:**
  - Picks the role furthest below its share of the fleet: own warships, plus those paid in the last minute, plus 1.
  - If that role's type can't be paid now (stock and account), it takes the next role below its share that can.
  - Within a role, the type with the best factor x attack / (reload + 2 s) x hp / cost^2 against the role's target
    (Lanchester).
  - Only types available by their tech record (`ld_type_avail`).
  - A type the budget refused for 2 minutes is skipped for 1. A type paid for but not trained is skipped for 5.
- **Logs:**
  - `# navy: P<n> mix at N min:` every 5 minutes: the wanted shares, own fleet by role, the types, the enemy seen,
    what the game asked for and what was trained, and the changed / skipped / payable-next counts.
  - The 2-minute `# navy:` line has the warships, lost and sunk by role for every AI (diplo.c counts navy kills by
    type).
  - Score with `scripts/navy_ab.py [-mix]`.

What it did: about 45% of the ships it trained were another type than the game asked for. On Small Islands e5,
fewer galleys and more frigates and battleships.

**Head-to-head** (one AI with the mix, `navy_mix=0,v_navy_mix=1`, both with the navy account; the seat swaps every
game; per game the mix AI minus the other; std. error in brackets):

| | Small e5 | Small e9 | Large e5 | Large e9 | all |
|---|---|---|---|---|---|
| first version, 160 games (40 each): kills - losses | -8.2 (5.1) | -6.3 (3.9) | -15.8 (4.6) | -3.9 (4.4) | **-8.6 (2.3)** |
| warships sunk - lost | +0.1 | -0.2 | +0.2 | +1.8 | +0.5 (0.5) |
| ship orders paid | -0.8 | -0.4 | -0.5 | -2.5 | -1.1 (0.5) |
| next payable role, 120 games (30 each): kills - losses | +1.8 (6.0) | -14.5 (5.8) | -9.0 (6.2) | -8.1 (4.6) | **-7.5 (2.9)** |
| warships sunk - lost | +2.0 | -0.5 | +0.9 | -0.5 | +0.5 (0.5) |

Soldiers, buildings, food from fish and fishing boats lost: within noise in both versions.

The first version waited for its first choice and paid for a ship less an hour. Taking the next payable role
didn't change the result.

**Both AIs with the mix** (current version, Small Islands, 20 seeds each, 60 min; navy_mix 0 -> 1, per AI):

| | Small e5 | Small e9 |
|---|---|---|
| frigates / battleships / galleys / subs afloat (average) | 0.6/0.1/0.4/0 -> 0.7/0.3/0.2/0 | 0.5/0.2/0/0.3 -> 0.5/0.2/0/0.2 |
| lost by role F/L/A/S | 4.8/0.7/2.9/0 -> 6.1/1.7/0.8/0 | 4.7/1.0/0/0.7 -> 4.2/1.5/0/0.7 |
| navy lost (= sunk) | 12.4 -> 12.0 | 9.3 -> 8.9 |
| kills / losses | 71.7/81.3 -> 62.9/74.2 | 44.5/51.5 -> 41.7/51.2 |
| food from fish / all food | 3562/11308 -> 3264/10694 | 3354/8911 -> 4057/10951 |
| landings sailed | 1.8 -> 1.9 | 1.6 -> 1.6 |

All within noise (std. error: kills 3-5, food 300-700). 80 runs, no crash. With the first version (also 20 seeds)
e5 was the same picture: navy lost 13.2 -> 11.0, kills 63 -> 74.

**Why it stays off:** the AI with the mix loses about 8 units more per game than the one without (2.6-3 std.
errors, both versions). The sea exchange is even. The likely reasons:
- The fleets are tiny: 1-1.5 warships on average, 3-5 at most, so a mix hardly forms.
- The game's sea groups still decide where the ships go. The mix's battleships go where the game sends them and
  are lost more often (Large e5: 2.4 vs 0.8), against coasts where the x4.5 is real.
- The gold that battleships take is missed elsewhere (the mix AI spent +86 gold and -73 iron on ships).

Retested with stage 3 (below, `fm`): with the fleet led, the mix adds nothing either.

### Stage 3 - the mod leads the fleet - BUILT, OFF (`navy_fleet=0`)

Built (dll/ai/fleet.c; `[all] navy_fleet`, `[autotest] v_navy_fleet`; test keys `navy_fleet_boats`, `navy_fleet_coast`,
`navy_fleet_raid`):

- **Who:** every warship and submarine of an AI on a water map (navy_detect: enemies across the water) that is not a
  landing's. Carriers, transports and fishing boats stay the game's.
- **Taken from the game** as landing.c takes its units:
  - a sea subgroup holding a fleet ship is not updated (hook_asg_sea_update);
  - the subgroups' orders to a fleet ship (vf57) are refused in hook_asg_give_goal. The land subgroup's hand-off,
    which uses the goal after a refusal, keeps its exception; it never orders a fleet ship. The order hooks are now
    installed once (`asg_order_hooks`) for the landing and the fleet;
  - the naval maintenance (0x4779FA) deletes own units for room when it is short of "Ships to Maintain"
    (0x430D40 called from 0x477B17). That call is skipped while the fleet leads. In the runs it was short only once,
    because the fleet's ships count for it.
  - Only a few orders came through vf57 at all (0-12 a game, `refused`). The skipped sea updates were 400-3000 a
    game. The attack commander's groups stayed at 5-9 at the end of 60 minutes, so no frozen groups pile up.
- **Danger zones** (every 2 s):
  - what counts: enemy defence buildings with a ground attack, and land units that hit battleships x3 or more
    (catapults, cannons, artillery, elephants ...) or reach 7+ tiles, as long as they hit ships;
  - from where: seen now, and with honest_vision those remembered (buildings until their spot is looked at again,
    siege 5 minutes);
  - size: the range + 2 tiles.
  - No ship sits or sails inside a zone. A ship inside sails out, away from the zone's centre, or 45/90 degrees off
    that direction. A way that crosses a zone goes round a waypoint 8 or 16 tiles off the line, or is not taken.
  - A target must be 3 tiles clear of every zone, because the ship shooting it stands nearer to the zone. Without
    that margin, ships went in and out of a tower's reach hundreds of times a game.
- **Strength:**
  - each side's hit points x its damage a second (attack x 1000 / (reload + 2000)) x the hit table's factor against
    the other side's ships, spread over their hit points;
  - the ratio is sqrt(own hp x own dps / (enemy hp x enemy dps)).
- **Tasks:**
  - *support / escort:* while a landing sails or is ashore, the ships it didn't take stay off its beach, out of
    danger. While a landing gets its transports and enemy warships are known on its route, the fleet goes there
    and fights them if the ratio is at least 1. The landing still takes up to 6 warships as escorts (all of them,
    with these fleet sizes).
  - *defend:* enemy warships seen within 15 tiles of its buildings, fishing boats or guard point are attacked when
    the ratio is at least 0.8. Each ship takes the nearest one it can hit.
  - *hunt:* starts when the ratio against every enemy warship known (seen now, or remembered for 5 minutes) is 1.3
    or more and the fleet has 60% of its hit points. It gathers first (within 8 tiles, 60 s at most). Then it goes
    for, in this order:
    1. the enemy's warships;
    2. its fishing boats and transports;
    3. units and buildings on its coast;
    4. the water off its nearest known dock (its fishing grounds), skipped for 3 minutes once it has been there.

    A hunt or gather lasts 30 s at least. The fleet retreats to the guard point for at least 60 s when its hit
    points fall below half of what it had when the hunt began, or when the enemy near the fleet is stronger
    (ratio < 0.8).
  - *guard:* a point 5 tiles from the fishing boats (or the docks) toward the nearest known enemy building, on open
    water and out of danger.
  - Ships below 35% hit points go to the dock. The game repairs them there (`repaired`: 0-2800 hit points a game).
- **Fishing boats:** an enemy warship seen within 10 tiles of a boat, and no fleet within 20 tiles that wins
  (ratio of at least 1): the boat goes to its nearest dock.
- **Log:** `# navy: P<n> fleet at N min:` every minute and on a change of task. It shows the task and why, the
  fleet's position, target and guard point, its ships and hit points, the ratio against the enemy ships known and
  near, the danger zones, warships lost and sunk, fishing boats lost and sunk, and counters (boats pulled, flees,
  hunts, retreats, defends, escorts, hit points repaired, orders, refused, sea updates skipped, maintenance skipped,
  attack groups).

What the fleet did, Small/Large Islands e5/e9:
- From the 2-minute looks: guarding and defending at home about half the time, hunting about a quarter (mostly the
  enemy's fishing grounds and boats; few enemy fleets were ever stronger than 1.3 to 1), support for landings the
  rest.
- Fleets were still small: 1-3 warships, 8-10 in some e9 games late on.

**Head-to-head, seed-paired.**
- Setup: one AI leads its fleet (`v_navy_fleet=1`); both AIs have the navy account. Small/Large Islands e5/e9,
  seed 101 map, 60 min, game seeds 1-32 (`h2h.sh`: odd seeds P2, even seeds P3).
- Each fleet game is compared with the control game of the same seed (`v_navy_fleet=0`). The control games are
  identical between builds: checked at 3 seeds x 2 maps, and the code is unchanged with the fleet off.
- Two scores: *seat* is the fleet AI minus the same seat in the control game. *DiD* is (fleet AI - other) minus
  the same difference in the control game.
- Scored with `scripts/fleet_ab.py -strip 1 -ctl ctl -h2h fl,fm,fr`. Per game; std. error in brackets; 32 games
  per map:

| fleet AI, seat (DiD) | Small e5 | Small e9 | Large e5 | Large e9 | all 128 |
|---|---|---|---|---|---|
| warships sunk - lost (sea exchange) | +2.3 (+4.6) | +1.9 (+3.8) | +1.7 (+3.3) | +1.9 (+3.9) | **+1.9 (0.45) / DiD +3.9 (0.9)** |
| own warships lost | -1.9 | -0.9 | -0.9 | -1.8 | -1.4 (0.4) |
| warships afloat (average) | +0.0 | +0.2 | +0.1 | +0.3 | +0.2 (0.06) |
| kills - losses | +3.6 (+8.6) | -4.9 (-8.5) | -1.7 (-7.4) | **-14.9 (-33.5)** | -4.5 (3.4) / DiD -10.2 (6.2) |
| fishing boats lost / sunk | +0.6 / -1.7 | -1.8 / +0.4 | -1.1 / +0.8 | 0.0 / -4.1 | -0.6 (0.7) / -1.1 (0.8) |
| food from fish | -295 | +150 | +200 | -243 | -47 (143) |
| soldiers | +2.0 | -1.0 | +0.6 | -0.9 | +0.2 (0.5) |
| landings ashore | -0.4 | +0.1 | -0.1 | +0.4 | 0.0 (0.14) |

- **It wins the sea on every map.** It loses 1.4 warships fewer a game and sinks about 0.5 more.
- **The war on land is not better.** Kills - losses is -4.5 (3.4) overall. Large e9 is -15 (6.1), with 12 kills
  fewer: 4 of them enemy fishing boats, the rest probably what the game's sea groups shell on the enemy's coast
  when they roam freely. The fleet keeps out of the reach of towers and cannons and, against a known U-boat, has
  no ratio to hunt with (battleships can't hit submarines).
- The other maps are within noise.
- Fishing boats are not kept better. Pulling them to the dock changed nothing: `navy_fleet_boats=0` (64 games)
  gave fish -238 (227), like -230 with it.
- Variants, 128 games each:
  - With the fleet mix (`fm`, `v_navy_mix=1` too): sea +2.1 (0.46), kills - losses -6.3 (3.8);
  - Raids on boats and the coast already at ratio 1.0 (`navy_fleet_raid=100`): sea +1.7, kills - losses -4.9 (3.3),
    Large e9 again -14.8.

**Both AIs with the fleet** (navy_fleet 0 -> 1, seeds 1-16, 64 games, per AI against the same seed's control game):
- kills -4.7 (2.6) and losses -4.5 (2.8): a less bloody war;
- soldiers +1.8 (0.6);
- food from fish +288 (173), fishing boats lost -1.2 (0.8);
- warships afloat +0.15 (0.08), landings ashore +0.24 (0.13).

About 1000 runs in all, no crash, every game finished.

**Why it stays off:** the fleet does what it is meant to do at sea: it holds home waters, sinks more than it loses
and keeps out of tower and cannon range. But a 1-3 ship fleet's sea control doesn't turn into more kills or
buildings on land, and on Large Islands e9 the AI with it does worse than with the game's roaming sea groups. What
is missing:
- shelling the coast where it is safe while guarding, not only in a hunt;
- a way to handle known submarines (frigates only);
- escorts for the game's own transport attacks;
- landings planned around sea control (stage 4).

`navy_mix` stays 0 as well.

### Against a human-like navy (the sea opponent)

Measuring an AI's navy against another AI's is weak evidence: both have 1-3 warships, and nobody holds any water. The
playtester builds 20+ ships early on an island map and uses them. dll/test/testopp.c (test code, started by the setup line
in `scenarios/seaop.eeai`) plays that with the idle P1 against one AI: a scheduled build-up (medium: 3 ships at minute
5, 16 by 20, at most 22, losses replaced), raiders on the fishing boats and the coast, a main fleet that hunts the
AI's warships when it is 1.2 times stronger and retreats when weaker, a patrol on the sea lane against transports,
two landings with transports, honest sight (9 tiles around its own units). How to run it and the log: docs/testing.md
"Sea opponent". Score: `python scripts/seacontrol.py -ctl f0 <logs>`.

**Baseline** (HEAD 5868f55, medium opponent, 1 AI, seed 101 map, 60 min, game seeds 1-10 per cell; n0 = navy=0, f0 =
shipped (navy account, the game's sea groups), f1 = navy_fleet=1; per game, std. error in brackets):

| f0 (shipped) | Small e5 | Large e5 | Small e9 | Large e9 |
|---|---|---|---|---|
| AI warships afloat avg / most | 0.2 / 2.4 | 0.5 / 3.1 | 1.0 / 4.0 | 0.7 / 4.8 |
| AI warships sunk / opponent ships sunk by the AI | 6.0 / 7.1 | 13.8 / 5.6 | 11.7 / 18.4 | 12.0 / 9.8 |
| opponent warships afloat (avg) | 16.0 | 16.4 | 15.8 | 16.2 |
| AI fishing boats avg / lost; food from fish | 2.5 / 12.9; 450 | 1.3 / 14.9; 449 | 3.7 / 12.2; 782 | 0.8 / 10.3; 160 |
| AI citizens killed (by ships) | 25.0 (1.9) | 11.3 (7.8) | 11.7 (0.5) | 15.4 (0.9) |
| AI transports sunk; AI landings sailed / ashore | 1.8; 0.4 / 0 | 8.2; 0.3 / 0 | 5.6; 0.4 / 0 | 5.0; 0.4 / 0.2 |
| opponent landings ashore / stopped (of 2) | 1.6 / 0.4 | 1.6 / 0.4 | 1.1 / 0.9 | 1.8 / 0.2 |
| AI home waters held | 13% | 53% | 22% | 46% |
| AI buildings / towns on other islands | 1.5 / 0.4 | 0 | 4.0 / 1.0 | 0 |
| AI kills - losses; AI defeated | -32 (10); 2 of 10 | -59 (5); 0 | -45 (12); 1 | -44 (10); 1 |

AI against AI (stages 0-1) the AIs fished 2000-4500 an hour; here 160-780: the opponent holds its waters (on Large
Islands the AI's buildings stand farther from the sea, so "held" is higher), sinks its boats, transports and the few
warships it builds, and lands. The AI sails 0.3-0.4 landings a game; 2 of 40 got ashore.

**Passive against active** (paired by seed, variant minus f0):

| | Small e5 | Large e5 | Small e9 | Large e9 |
|---|---|---|---|---|
| n0: AI warships sunk | -4.8 (1.2) | -9.2 (1.5) | -3.2 (2.5) | -5.3 (2.0) |
| n0: opponent ships sunk by the AI | -6.0 (3.6) | -0.8 (2.4) | -5.5 (4.0) | -0.8 (4.8) |
| n0: AI home waters held | -0.5 | +10.2 (7.1) | -0.8 | +26.3 (5.4) |
| f1: sea exchange (opponent sunk - AI lost) | -3.2 (3.1) | -0.8 (2.4) | **-8.1 (2.1)** | -1.3 (4.2) |
| f1: food from fish | +238 (121) | -156 (72) | +27 (254) | +124 (174) |

- Without the navy account (n0) the AI builds fewer warships, loses fewer and sinks fewer of the opponent's: the
  metrics see the navy. With it (f0) its 1-5 warships are a cost against 16: about even in the sea exchange on
  Small Islands, -8 on Large e5.
- The mod's fleet (f1) is no better against this opponent; on Small e9 it sinks 7.8 fewer of its ships (it keeps
  out of the way of a stronger fleet). Fishing a little better on Small e5 (+238, 2 std. errors).
- **Levels** (f0, 5 seeds per map): weak - the AI fishes 860-2650, holds its waters 27-57%, wins 3 of 19 games;
  strong (19-22 ships) - fish 8-310, AI defeated in 10 of 20.

What a human-level AI navy would have to show here: more warships afloat when the opponent comes (the AI asks for
ships all game and is refused - section 3), fishing boats kept (fish food), transports through, and a sea exchange
above 0 against the main fleet.

### Stage 4 - a big fleet in task forces - BUILT, ON WITH STAGE 5 (`navy_fleet=2`)

The playtester (2026-10-19): "at least 20 ships on a low population game, way more with higher limits, split into
smaller fleets and task forces for proper map control"; "the fleet needs to go out and do things: hunt fishing boats and
shell citizens to disturb the economy, hunt enemy warships to prevent the enemy from massing a force at sea, patrol for
transports ... Any unit that just sits there is a huge opportunity cost"; "the fleet can't be scared off by a few subs -
that's what the frigates are for".

Built (dll/ai/navy.c, dll/ai/fleet.c, econ.c, diplo.c; `[all] navy_fleet=2`, `[autotest] v_navy_fleet=2`; keys
`navy_fleet_pop` 15, `navy_share_max` 50):

- **Size.** The target is `navy_fleet_pop` % of the AI's population cap x the map strength: 26 warships at the tests'
  cap of 176, 20 at a cap of 135.
  - Below the target the account's share grows from `navy_share` (25) toward `navy_share_max` (50) with what is missing.
  - A warship spends the account through the reserve (the epoch's and the landing's): it is the navy's money. Large
    e9 before: 524 frigate orders refused in an hour with the account full (W600 G600 I546).
  - Room held for the fleet and docks wanted (2 + target / 8, at most 5) follow the target.
  - The mod trains the missing warships itself, one every 10 s while the warships afloat plus those ordered in the
    last 90 s are below the target (`# navy: P<n> at N min: the fleet trains a warship (type, role)`). The game's
    own warship orders go on.
- **Composition.** The fleet mix (stage 2) chooses the type of the mod's orders and the game's. When the role most
  missing can't be paid, any role it can pay, the most missing first: iron comes in slowly on islands (e5: account W600
  G574 I41, 488 frigate orders refused), so battleships (wood + gold) fill in.
- **Task forces** (fleet.c `tf_*`), only while the fleet is clearly the stronger at sea (ratio 1.5 against every
  enemy warship known, 6+ ships; one fleet again under 1.1). Split forces of 1-3 ships lost to a concentrated enemy.
  - Sizes: an escort of a quarter while a landing gets ready or is under way; of the rest, a sixth home guard, a fifth
    raiders, a sixth patrol (from 6 ships), the rest hunters. 20 ships: home 3, raid 4, patrol 3, hunt 10.
  - Ships keep their force. A force over its size (one more is kept) lets the least fitting go; one under it takes
    the best fitting free ship. Fit by role: hunters battleships and submarines, then frigates; raiders and patrol
    frigates and galleys; home battleships, galleys, cruisers.
  - *Home guard*: the stage-3 decision without hunts (defend when the ratio against the ships at home is 0.8+).
  - *Hunters*: the enemy warship group (within 12 tiles of one) where they are 1.2+ stronger, near ones first -
    strike before they mass. All groups stronger: they hold. They help the home guard when it can't hold alone.
  - *Raiders*: transports (weight 4), fishing boats (3), citizens on the coast (3), docks (2), other buildings (1),
    soldiers (0.5), over (1 + distance / 25), where the enemy ships within 12 tiles are not stronger.
  - *Patrol*: transports seen within 16 tiles, then warships there it beats (else back off); otherwise waypoints on
    the sea lane to the nearest known enemy building (35% and 65% of the way, 10 tiles to each side half way).
  - *Escort*: the stage-3 support / route clearing, and covering the route while the landing gets its transports.
  - No target: the enemy's fishing grounds (off its docks, not cleared in the last 3 minutes), its shore, a sweep.
  - Retreat at half the hit points of the start or outmatched near (0.8), 45 s.
- **One fleet** (not split): the stage-3 decision on all ships. **Shelter** (fleet in being): against an enemy fleet
  known much stronger (ratio < 0.6, 3+ warships), or while losing at sea (4+ warships lost in 10 minutes and twice
  what it sank), every ship waits at the haven - water by an own tower near the sea, the one farthest from the
  enemy, else by such a dock. The mod trains none then, and the share is back to the base. Out again at 0.9.
- **Combined arms in the orders:** frigates take submarines first; raiders citizens first (soldiers last). Points the
  forces go to are 3 tiles clear of every danger zone (at the edge ships went in and out: 1900 flees an hour).
  Tried and dropped: ships that can't hit a submarine near them (battleships) keeping 8 tiles off - no gain.
- **Log:** `# navy: P<n> tf <home|hunt|raid|patrol|escort> at N min: task (why) | ships n F L A S AA hp | at -> to |
  ratio | kills W B T C Bd O lost | hunts retreats defends orders flees` on a change and every minute. The fleet line
  adds `target built refused | haven x,y sheltering/split/one fleet (n times) | forces ... | lost by force ... | by
  task ... | kills W B T C Bd O`. Kills are counted by the ships' targets when the unit dies (diplo.c hook_kill_stat
  -> fleet_kill, fleet_lost).
- `scripts/fleet_ab.py` columns `wmax` (most warships), `fcit` / `fkill` (citizens / everything the led fleet killed).

**Head-to-head against the control AI** (one AI `v_navy_fleet=2`, both with the navy account; Small/Large Islands
e5/e9, seed 101 map, 60 min, game seeds 1-32, each game paired with the control game of its seed; fleet_ab.py; std.
error in brackets; 128 games):

| fleet AI, seat (DiD) | Small e5 | Small e9 | Large e5 | Large e9 | all |
|---|---|---|---|---|---|
| kills - losses | +27.9 | +9.1 | +46.5 | -10.2 (8.2) | **+18.3 (4.2) / DiD +38.8 (7.8)** |
| losses | -22.6 | -3.4 | -34.4 | -2.9 | -15.8 (2.6) |
| warships afloat (average / most), fleet AI | 4.9 / 12.5 | 6.9 / 16.1 | 6.1 / 15.9 | 9.3 / 21.2 | +5.5 / +12.0 vs control |
| warships sunk - lost | +0.2 | -1.3 | +1.1 | -1.7 | -0.4 (0.65) |
| own fishing boats lost / enemy's sunk | -5.4 / +5.0 | -2.6 / +4.2 | -5.3 / +3.8 | -0.3 / -5.6 | -3.4 (0.9) / +1.9 (0.9) |
| food from fish | +1739 | +1776 | +1012 | -89 | +1110 (181) |
| enemy transports sunk | +3.5 | +2.3 | +1.3 | +0.3 | +1.8 (0.3) |
| citizens killed by the fleet (game) | 1.8 | 1.0 | 1.6 | 0.2 | 1.2 |
| soldiers | +0.6 | +3.7 | +2.0 | +8.3 | +3.7 (0.7) |
| buildings lost | -2.6 | -0.8 | -2.5 | -0.6 | -1.6 (0.5) |

- The mod trained 18-31 warships an hour per AI; the most afloat 12-21 (the target 26 is not reached: the islands'
  gold and iron - 110 a minute at e5 - pay a frigate a minute at best, and ships are lost).
- Earlier versions, 64-128 games each: everything split always +21 / +28 (5); with `navy_share_max=70%` +26 (6), more
  ships (+7.1 afloat) but fewer soldiers; the stage-3 fleet was -4.5 (3.4).
- Both AIs with it (8 seeds, Small e5 / Large e9): kills 62.6/62.2 -> 58.2/36.6, losses 76.3/72.8 -> 61.9/44.6,
  workers +4.5/+8.5. Continental: detection "land", games identical. No crash in about 1000 runs.

**Against the sea opponent** (docs/testing.md "Sea opponent"; 1 AI, navy_fleet 0 vs 2 paired by seed, 60 min;
seacontrol.py; medium 10 seeds, weak and strong 6):

| fleet AI - control | Small e5 | Small e9 | Large e5 | Large e9 |
|---|---|---|---|---|
| medium: kills - losses | +11.0 (11.8) | +26.7 (12.5) | +9.4 (9.8) | +24.0 (11.5) |
| medium: sea exchange (opp sunk - AI lost) | -4.4 (3.0) | -9.8 (2.9) | -4.0 (2.6) | -6.8 (4.0) |
| medium: fishing boats lost | -5.0 | -5.9 | -5.0 | -2.6 |
| weak: kills - losses | +6.3 (12.7) | -38.7 (25) | -16.7 (11.2) | -17.2 (14.2) |
| weak: sea exchange | -8.8 (3.7) | -16.3 (6.3) | -11.5 (2.6) | -11.5 (2.6) |
| strong: sea exchange (first shelter version) | -1.5 (1.0) | -8.8 (3.9) | -1.0 (2.1) | -4.8 (2.0) |

- The first version (always split, no shelter) lost 5-18 more warships a game than the control against the medium
  opponent: ships went out one by one and were sunk at the guard point (`lost by task`: guard 10-22 a game). The
  shelter and the split gate cut that to 2-8, and the games' kills - losses turned positive.
- The opponent's 16 warships come for free (44 an hour); the AI's 18-30 an hour are paid from 110-250 gold + iron a
  minute and arrive one by one, so it never holds 3+ at once there (0.4-1.2 afloat on average). Against the weak
  opponent (8-12 warships) it builds and loses more than the control without sinking more.

**Why it stayed off (stage 5 mends it):** it beats the control AI clearly (+18 kills - losses, +1100 fish food, fewer boats lost, more
soldiers) but not the sea opponent: against the weak one it loses the sea and the land exchange on 3 maps, against
medium the sea exchange is worse though the land exchange is better. What is missing:
- a fleet that grows unseen before it fights: the build-up comes one ship at a time to a sea the enemy holds;
- the gold/iron economy for 20+ warships (the metals' workers follow the army's refusals, not the navy's target);
- Large Islands e9 against the control: -10 (8).

### Stage 5 - mass before striking; the mines - BUILT, ON (`navy_fleet=2`, `navy_fleet_mass=1`, `navy_mines=1`)

The playtester: "massing a fleet under the protection of towers and land units is an important step"; the AI should
"exploit all resource nodes it has on its island quite quickly, and after that try to expand to other islands". Against
the sea opponent stage 4 lost because its ships came one at a time into a sea the opponent held (0.3-0.8 afloat on
average, 8-20 sunk a game, most at the guard point).

Built (dll/ai/fleet.c `fl_rally`, `fl_rally_tower`, fl_decide `FLD_MASS`; dll/ai/navy.c `navy_nodes`, `navy_mines`; econ.c
econ_targets calls navy_mines):

- **Massing** (`navy_fleet_mass`, replaces the shelter). While the fleet is weaker than the enemy fleet known (ratio
  under 1.0) or under its critical mass (a quarter of the fleet's target, at least 4; both with a margin of 3/4),
  every ship waits at the rally and the mod keeps training - the rally is where the fleet grows. It strikes as one
  force at ratio 1.25 with the critical mass; from there the stage-4 decision (one fleet, task forces only when
  clearly superior).
- **The rally**: the water tile of the fleet's sea inside the reach (range - 1) of the most own towers, then by a dock,
  far from the nearest known enemy building, near its tower; out of danger. Re-chosen every 20 s.
  - The fleet fights there whatever comes into the cover (towers' range + 2): at ratio 0.5 with towers covering, 0.8
    without. It never chases out of it (a target farther than the radius + 3 from the rally is dropped).
  - No tower covers the fleet's sea: the rally is the haven (by the dock farthest from the enemy), and a tower is
    queued by that dock (land within 5 tiles with the sea within 4, placeable; the best tower type it can build and
    pay; every 2 minutes while none is queued, 3 a game at most). 24 requests in 39 games against the sea opponent.
  - Land units at the rally are not counted yet.
- **The nodes** (`# nodes:` with the 5-minute budget line): the stone, gold and iron sites the AI knows on its home
  island - worked, gatherers of the slots (6 a mine), near (12 tiles) or far from a drop site - and elsewhere.
  - Small Islands e5 home: 1 stone, 2 gold, 2 iron mines (30 slots); Large Islands e9: 2 stone, 3 gold, 3 iron.
  - Every home mine lies near a drop site from minute ~10 (the game's own Settlements) - no Settlements needed for
    the home island. But the slots stay empty: 2-5 of the 12 iron slots worked at minute 20 on Small e5, the gold
    full only at minute 25, while 40+ woodcutters piled up 5000 wood.
  - Elsewhere (other islands, known): 2-5 stone, 4-7 gold, 4-6 iron mines. The game's AI expands there by itself
    now and then (towns on other islands: 0.4-1.4 at most on Small Islands, none on Large). Claiming them
    (Settlements by transport under fleet cover) is the next step, not built.
- **The mines** (`navy_mines`): on a water map, after the game's targets, gold and iron are wanted at the target + the
  workers for the navy's missing warships' metal within 10 minutes (stage 4: up to 6 missing, ~100 gold + 100 iron
  each, less the account and the stock beyond the reserve), and from minute 15 with 35+ workers every slot at the home
  island's mines near a drop site; never beyond the home island's slots. A pull of 2 every 10 s toward that while wood
  piles up (500 beyond its need) or food is 400 beyond its reserve and the mines take those sent; 4 back when food
  falls under its reserve + 100. Taken from wood first (half its target kept), then food (60% kept). Early on food
  and wood keep their priority by the donors' rule.

**Against the sea opponent** (1 AI, 60 min, paired by seed with `navy_fleet=0,navy_mines=0`; medium 10 seeds, weak and
strong 6 (Small e9 weak and Large e5 strong 14); seacontrol.py; std. error in brackets):

| stage 5 (fm) | Small e5 | Small e9 | Large e5 | Large e9 |
|---|---|---|---|---|
| medium: warships afloat avg / most (control) | 3.3 / 9.6 (0.2 / 2.4) | 6.0 / 13.3 (1.0 / 4.0) | 5.0 / 12.6 (0.4 / 2.6) | 7.6 / 19.3 (0.7 / 3.6) |
| medium: sea exchange (control) | **+24.9** (+1.1) | **+14.6** (+6.7) | **+2.8** (-7.9) | **+6.5** (-3.2) |
| medium: sea exchange, paired | +23.8 (4.8) | +7.9 (3.3) | +10.0 (4.2) | +9.7 (6.2) |
| medium: opponent warships sunk by the AI | 33.4 | 35.0 | 30.8 | 33.5 |
| medium: kills - losses, paired | +48.0 (9.0) | +47.1 (15.1) | +30.8 (11.0) | +33.9 (9.7) |
| medium: fishing boats lost / fish food, paired | -8.5 / +930 | -9.5 / +1483 | +1.6 / +436 | -0.8 / +363 |
| weak: sea exchange, paired | +6.2 (4.0) | -3.9 (4.1) | -0.3 (7.6) | +1.8 (3.9) |
| weak: kills - losses, paired | +47.2 (13.9) | +24.0 (12.9) | +58.3 (22.1) | +11.7 (12.9) |
| strong: sea exchange, paired | +17.8 (6.8) | +22.0 (5.1) | +21.6 (4.1) | +10.7 (5.4) |
| strong: kills - losses, paired | +12.3 (10.5) | +19.3 (24.3) | +11.6 (8.6) | +32.7 (31.7) |
| strong: AI defeated (control) | 3 of 6 (5) | 3 of 6 (5) | 0.5 of 13 (0.3 of 14) | 3 of 6 (4) |

- The opponent's fleet breaks itself on the rally: it comes for the AI's ships and meets towers and fleet together
  (its own rule keeps its targets out of the towers' reach + 4, but a fight that started outside drifts in). The
  sea exchange turned positive on every map at medium (stage 4: -3 to -12).
- Mines alone (`fi`: navy_fleet=0, navy_mines=1) changed nothing against it (sea exchange -0.8..+2.2, kills -
  losses +1..+6): the food stock stays near 0 under the raids, so the pull stays 0 (773 of 838 looks).

**Head-to-head against the control AI** (one AI `v_navy_fleet=2`, control `v_navy_fleet=0`, both with the navy account
and the mines; seed 101 maps, 60 min, seeds 1-32; fleet_ab.py; 123 games, 5 lost to start races):

| fleet AI, seat | Small e5 | Small e9 | Large e5 | Large e9 | all (stage 4) |
|---|---|---|---|---|---|
| kills - losses | +21.9 (6.2) | +5.8 (5.1) | +42.9 (8.3) | +24.7 (8.2) | **+23.7 (3.7)** (+18.3 (4.2)) |
| losses | -22.9 | -7.6 | -32.3 | -25.7 | -22.0 (2.8) (-15.8) |
| warships afloat avg / most vs control | +3.8 / +8.8 | +6.3 / +13.5 | +5.9 / +15.1 | +9.0 / +18.0 | +6.3 / +14.0 (+5.5 / +12.0) |
| sea exchange | -3.4 (1.1) | -1.6 (1.6) | +3.1 (1.4) | +2.1 (1.6) | +0.15 (0.75) (-0.4) |
| fishing boats lost / fish food | -4.4 / +1522 | -2.8 / +2300 | -4.6 / +688 | -5.3 / +80 | -4.3 / +1139 (-3.4 / +1110) |
| enemy transports sunk; soldiers | +3.3; -0.7 | +1.5; +4.0 | +4.0; +2.8 | +1.0; +13.0 | +2.4; +5.0 |

Large Islands e9, -10 (8) in stage 4, is now +25 (8).

**Mines, head-to-head** (navy_fleet=0 as before; one AI `v_navy_mines=1`, both else `navy_mines=0`; 128 games): kills -
losses +1.1 (1.7), gold +200 and iron +150 in the hour (+3-4%), everything else within noise. On by default: the
navy's supply path and the playtester's "work every node", at no cost seen.

**Default on**: `navy_fleet=2` beats the control AI (+24 kills - losses a game, every map positive) and does better than
`navy_fleet=0` against the sea opponent at every level on kills - losses (all 12 cells positive) and on the sea
exchange in 10 of 12 (weak Small e9 -3.9 (4.1), weak Large e5 -0.3 (7.6): within noise). Caveat: strong Large e5
AI defeated in half of 13 games against 0.3 of 14 (+0.2, std. error 0.2). Continental: detection "land", the games identical with and
without (2 seeds, 25 min). About 1000 runs, no crash.

Still missing: land units at the rally; expansion to other islands' mines under fleet cover; the patrol and raids
when the opponent fleet is out of sight but stronger (the fleet waits at the rally then).

### Ships to cover and to the docks (2026-10-21, dll/ai/fleet.c with dll/ai/cover.c `fall_back` 4, ON)

Measured (`# cvd`, base AI at hard, 40 games a map): on the big island maps two thirds of the value the mod lost was
warships, 5000-5700 of it in fights lost when they began (Tournament / Large Islands Huge, no cheat); the docks repaired
3500-5000 hit points a game - ships went at 35% and sailed again at 35%, and a ship with a target never went. SEA: the
fleet's retreat and a damaged force go to the rally under its towers (else the haven by a dock) instead of the open-
water guard point; a warship below half its hit points breaks off at once to the water by a dock (one an own tower
covers counts half the way) and stays until 80%. Alone (20 games a cell): Large Islands Huge without the cheat
kills - losses +21 (13) / +41 (13), warships +8 to +10, the docks' repairs 5000 -> 12900; with the cheat within the
noise. With 1 + 2 (fall_back 7, on): warships at the end +0 to +9.5 (2.0), head-to-head (Small Islands Large) warships
+8.1 (2.1), sunk +12.3 (4.3), ships lost -5.6 (2.5). docs/economy.md "Fall back to cover".

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

### Outposts on other islands (2026-10-20, dll/ai/expand.c, off)

The expansion to free islands (docs/economy.md "Other islands", `expand_islands`, off) sails one transport with citizens and a guard; it checks the sea on the way as the landing does (ld_sea_threat / ld_sea_risk against all its warships) and unloads at home at once when it is held: a transport waiting loaded at the shore for the fleet was sunk with everyone aboard in 9 of 10 cases. The fleet does not escort it (fleet.c's escort follows only the landing).

Large maps (2026-10-21, docs/economy.md "Large island maps"): the scouting trip of the empty transport (an island it has hardly looked at) checks the sea the same way first - without the check 2 of 3 scouting transports were sunk. FORWARD (`expand_islands` 64) asks for sea control before a forward base gets its barracks: `sea_held` (expand.c) - no enemy warship seen (intel.c, honest_vision) in the AI's own waters (navy_waters) within 5 minutes and its warships at least the enemy's it saw within 10 minutes. On the Large / Huge test maps it passed nearly every time: the enemy's warships had rarely been seen when the outpost stood (0 known in the games checked), so its own warships alone decided - a weak test of sea control; what decides at sea there (`# isl home_foe`, `# navy foes-home`) is in docs/economy.md.
