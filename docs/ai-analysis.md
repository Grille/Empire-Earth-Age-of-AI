# How the original computer player works, where it fails, and what the mod changes

These are findings from the logs, tests and the decompiled code of Empire Earth 1.04 (+ DSML add-on). Addresses
are listed in [engine-notes.md](engine-notes.md). All tests ran AI-vs-AI with **Cheat off** (fair prices),
unless noted otherwise.

## Structure

Every computer player (EEComputerPlayer) has a civilization manager (`cm`). It drives these sub-managers:

| Manager | Job |
|---|---|
| resource manager | worker targets per resource, assigning citizens to sites |
| build manager | what to build and where (per-epoch build lists) |
| research manager | techs, including the techs an epoch advance requires |
| unit manager | 14 request lists of {unit type, count}; trains units in buildings |
| attack commander | attack groups and subgroups (EECPAttackGroup / EECPASGLand); march routes, formations |

Most behaviour is driven by an **AI settings table** (`dbcpbehavior.dat`, ~100 records with min/max/value
fields). It holds timers (epoch, first attack, attack interval), shares (citizens, army, defence) and chances
(instant attacks), plus the cheat percentage. The table is copied into player fields at game start.

## The cheat hides a broken economy

- **The cheat itself:** with Cheat on (the default), AdjustCost multiplies every price by about 1%. The AI can
  then afford almost anything, so none of the problems below show.
- **With Cheat off, the AI overspends on soldiers too early.** In 20 minutes, one AI spent 14,467 food and
  gathered 1,260. Rock Throwers (it wants 60 of them from minute 0) cost 6,240 of that, Clubmen 3,660,
  citizens 4,000.
- **It doesn't count its food.** Before farms (Copper Age), food is finite: berries and hunting. The AI doesn't
  budget it, gets stuck in the Prehistoric or Stone Age and starves.
- **Food search** (0x48BBA8):
  - rejects every site with "enemy near", which in practice rejects almost all berries;
  - aborts the whole search when it hits an unexplored site (a bug).
- **Leftover citizens** (0x48A1AE) are sent to *any* free mine on the map. In one test they mined iron under an
  enemy's towers 25 tiles away and the workforce was gone by minute 12.
- **Worker targets** (ComputeTargets) follow the share of outstanding demand. Blocked military requests inflate
  the food demand, so stone and gold starve.
- **Unit manager:**
  - It counts units "in production" against the population cap. The training call adds them even when the
    building queue refuses the unit for lack of resources; nothing ever subtracts them. A budget that refuses
    at the wrong moment therefore eats the cap until nothing can be trained. Our first budget did exactly
    that: about 99 of 125 slots filled with phantom units, so the AIs had 0–2 soldiers.
  - It also requests campaign heroes that can't be trained (Golden Hind, Court Jester, Grigor II). Harmless,
    they just fail.

## Epochs

- **The timer decides, not the economy.** The AI only tries to advance when its epoch timer fires: 50–70
  minutes per epoch, scaled by difficulty. It also never gets ahead of the highest human epoch unless the
  "Ignorieren" toggle is set.
- **Every vanilla AI reaches the Stone Age when its timer fires** (minute 52–71 in our suite), however good its
  economy is.
- **An advance needs requirement techs.** The research manager is asked for them once (cm+0x76 stays set). If
  that request falls through, the AI waits forever.

## Attacks

- **Waves are started by a timer:** first attack 20–40 min, then every 14–17 min.
- **Instant extra attacks** come on contact with an enemy (71%), to take resources (20%) and as revenge (10%).
  That makes many small waves.
- **Routes ignore defences in practice:**
  - The march is planned over path sectors about 10 tiles apart, computed once.
  - The danger term is the target player's influence: the sum of each unit type's value (tower 175, house 90),
    counted whether the AI has seen the units or not.
  - A tower line looks like ordinary base, so armies walk straight through it.
- **The goal is usually inside the defences** (the nearest building).
- **Without the cheat, armies are tiny anyway.** At minute 59 the AIs had 0–2 soldiers in our tests.

## Scenario bug

A scenario started from the menu sets a "scenario" flag (0x918819). Several AI routines then skip their planning
(attack timing, research, building), probably because scenario authors were expected to script the AI. In
scenarios the AI therefore only gathers. Editor test-play and saved games don't set the flag, which is why
they behave normally. The mod clears the flag only while an AI updates.

## What the mod does about it

| Problem | Fix (module, `aimod.ini` key) |
|---|---|
| cheating prices | `cost_percent` per difficulty |
| scenario AI asleep | clear the scenario flag during AI updates (`scenario_ai`) |
| pre-farm overspending | **copper plan** (`copper_plan`): grow to `min_workforce` workers if stock + 60% of the explored wild food covers the epoch costs, then save for Stone and Copper Age; military only from the excess |
| no budget at all | **budget** (`budget`, `military_start_share`, `military_income_share`): soldiers paid from an account (share of start stock + share of income); requests trimmed to what the account pays *before* the game queues them |
| epoch stuck on requirements | re-request the requirement techs after 5 min; advance as soon as affordable before farms |
| worker targets | before farms: targets by shortfall towards the next epoch cost, floors of 2 on food and wood |
| citizens to distant mines | `safe_sites` (optional): only sites within `site_max_distance` and away from known enemies |
| build queue blocked | `build_fix`: the random-map build list starts with an unplaceable "Single Player Settlement" (395) that took every build-manager round (~1000 failed placements a game) - nothing from the later queues was built (Granary, Archery Range, Stable, Siege Factory, University...). It is placed as a normal Settlement; an entry that fails 3 times in a row is dropped (the planner re-queues missing buildings every 2 min) |
| mines far from drop sites | `mine_choice`: the free, safe mine nearest to a Town Center/Capitol/Settlement instead of the first in zone order (citizens walked 35 tiles to iron next to unused mines) |
| workers stuck on old targets | `rebalance_workers`: gathering citizens move from a resource far above its target to a mine that is short (the game only reassigns idle citizens) |
| gold hoarded | `hoard_cap_mineral`: stone/gold/iron above the epoch reserve + 600 may go into the army (the 3000 cap kept 1000 gold idle while soldiers failed for gold) |
| no gathering upgrades | `gather_research`: the research manager only picks new techs while its random planner is switched on (the AI's "attention" pass enables 5 of 9 activities per period, research rarely), and its preference test for gathering upgrades misses most of them (effects on "all units"). In the Bronze Age Hafts, Hand Axe, Metal Working, Hardened Chisel were still unresearched. Now every available gathering upgrade is scheduled (one every 15 s) |
| scattered towers, useless walls | `defence_layout`, `defence_towers`, `defence_walls`: towers go to slots at the front facing the nearest enemy building the AI has seen (3 tiles apart, 10 tiles out) and at remote drop sites; the AI also queues towers itself while stone allows (the game's plan asked for none in 15 minutes); once 2 towers stand at a front, an 11-tile wall arc goes up 3 tiles in front of them (repaired when fewer than 8 pieces stand). The game's own wall manager (a ring, one segment at a time - in practice one stray piece with a gate) is left alone |
| towers shelled by catapults | `counter_siege`: enemy siege weapons (and rams) the AI sees within their range + 4 tiles of its buildings are attacked by its soldiers within 20 tiles (up to 5 each, idle or not fighting close by) - only when they are at least 2x + 3 the enemy soldiers within 8 tiles of the siege weapon |
| army too small for its stock | `unit_substitution`: the AI's unit choice ignores what it can pay for and train - heroes (Golden Hind, Grigor II, Court Jester, Sampson), priests, gold units, upgraded-away Clubmen/Rock Throwers: hundreds of failed requests per 5 min, 10-12 soldiers at minute 45 with 2900 food in the military account. A refused military request (not a ship, siege weapon or ram) trains the strongest land soldier (hit points x attack) the budget pays for and the game can train instead |
| many small waves | attack settings: interval, group size, instant-attack chances |
| marching through towers | `avoid_towers`: route steps inside the range of a *seen* enemy tower get a big cost |
| goal inside the defences | `attack_weak_spots`: retarget to a seen, undefended enemy building nearby |

## Test results so far

Random-map suite: Continental, Plains, Highlands, Mediterranean, fixed seeds, 2–4 AIs, Cheat off.

| Variant | Result |
|---|---|
| vanilla | Stone Age at minute 52–71 (timer), Copper Age: none by minute 75 |
| copper plan | Stone Age at minute 17–23, Copper Age for 9 of 12 AIs by minute 36–50, all AIs alive, ~39 workers |
| fewer/bigger waves (Continental, 75 min) | 7 waves instead of 10, about 9 more workers per AI, same epochs |
| avoid towers + weak spots (Continental) | routes through tower range: 1 of 18 instead of 6 of 22; 13 attacks retargeted |
| budget leak fixed (Continental, 75 min) | soldiers per AI peak at 15–21 instead of 0–6; kills 53/94/88 instead of 21/10/21; first buildings destroyed; economy unchanged (~55 workers, Copper Age at 30–45 min) |
| 4-map suite with real armies (75 min, 12 AIs): original routes vs avoid towers + weak spots | original: 3 of 12 AIs eliminated, per AI 72.7 kills / 73.7 losses / 6.8 buildings destroyed; smart: none eliminated, 53.5 / 59.9 / 3.3, more workers at the end (48.6 vs 37.2). In AI-vs-AI the smarter routes make attacks less deadly (detours, split targets); the two features are being tested separately |
| player-style defences (`pathtest.scn`: guard towers, walls, 3 lasers around a building in the open; the long way round to the main base is free), attacker at 10 min | original: goal inside the lasers, weak-spot search (25 tiles) finds nothing, defenders waiting next to the lasers draw the army in: 25 kills / 104 losses / 0 buildings, 0 soldiers left. Weak-spot radius 60 + leash (enemies in tower range are no targets) + margin 6: goal switched to the main base, long route, 42 / 64 / 2 buildings, 40 soldiers left; by minute 20 the defender is down to 4 soldiers and 10 citizens |
| 4-map suite x 5 runs (turbo, 60 AIs per variant): original vs leash + radius 60 with margin 3 / 6 | alive 97 / 93 / 92%, workers at the end 42.7 / 40.6 / 42.6, kills 58.0 / 57.0 / 51.9, razed 3.8 / 4.8 / 4.0 - all within noise (std. error ~3% alive, ~3 workers, ~4 kills). Single-run suites before (e.g. "less deadly attacks") were mostly noise. AI-vs-AI can only show that these features don't hurt: the AI doesn't build focused defences like a player. Margin 6 is the default |
| own scenario `towers_on_path` (tower cluster in front of the defender, spawned armies), 5 runs each | first try: with the new settings both economies often collapsed (alive 40% vs 100%): the weak-spot search also diverted marches that were no attack on a building (goals near the own base, "target kind 7") and picked wall segments as targets. Fixed: only goals with an enemy building within 4 tiles are retargeted, walls and gates are never a weak spot. Then: alive 100% (new) vs 70% (old). pathtest after the fix: 11 buildings destroyed by minute 10 instead of 2 |
| own scenario `wall_gap` (wall across the map, one gap under towers), 5 runs each | the route planner ignores walls (~10-tile sectors): the army marches to the wall, piles up next to the gap and is shot from behind the wall (39 -> 4 soldiers in 8 min, never through). Breach (attack a wall segment outside tower range along the crossed wall; the scorer, which rates walls below zero, is overridden for it): losses ~46 instead of ~55, the army survives longer, one run broke through (6 buildings razed); mostly the infantry can't break a bronze wall in time - the AI brings hardly any siege. AI-vs-AI suite (2 runs x 4 maps): no difference (breach triggered 96 times) |
| siege (`siege_towers`: 5 bronze guard towers, attacker with 6 catapults), 5 runs each | the AI never takes its siege weapons along (8 catapults stayed at home for 9 min). Siege controller: the catapults move to a firing position at range - 0.7 from the nearest outranged defence (or a breach wall), outside every other known defence, and fire; idle soldiers guard the position. Without: 0 of 5 games decided in 30 min. With: the attacker wins 3 of 5 (after 13.7, 21.2, 25.6 min), all catapults survive the tower fight. AI-vs-AI suite (2 runs x 4 maps): no significant difference |
| siege training, rams | normal games (Bronze start, 45-60 min): no AI trained a siege weapon; one sometimes built a Siege Factory, the Stone Thrower (range 5 = guard tower) was requested but failed for iron/gold. Now: knowing enemy defences, the AI queues a Siege Factory (build manager queue) and requests 2-4 siege weapons of its epoch with priority. Rams skip buildings guarded by enemy soldiers (held back several hundred times per game). Limit: the AI's economy has hardly any gold - factories and catapults mostly stay unaffordable (1 catapult in 50 min); with the resource cheat factories come, still few catapults. In pathtest the defender now shells the attacker's towers with its 4 scenario catapults |
| economy (4 maps x 2 seeds, 60 min, Prehistoric start, per AI) | measured: gold income ~2400 in 55 min; ~1000 failed placements of an unplaceable settlement blocked the build queue all game; iron mined 25-35 tiles from any drop site. build_fix: soldiers trained 43 -> 97, siege weapons 0.2 -> 2.2, kills 32 -> 46, army at 60 min 9 -> 17, workers at the end 44 -> 51, all AIs alive; income +10-15% (food 5100 -> 5900, iron 2900 -> 3400). mine_choice: mean drop distance of iron 7.9 -> 4.2 tiles, iron income +13%. Citizen share 0.55/0.65 (`citizen_share`): more workers (max 68/79 instead of 56), income +5-10%, no more soldiers - left at the game default. siege_towers with the economy fixes: the attacker wins 2 of 6 instead of 4 of 6 - the defender profits as well (24 instead of 17 workers, rebuilds) |
| gathering research, same suite | all four early upgrades researched soon after each epoch; food income +14% (5560 -> 6330), workers at the end 49 -> 53, kills 37 -> 45 (within noise for kills). Measured on the side: the rebalance moves ~10 citizens per AI and 10 min (about 1% of working time - target smoothing not worth it); epoch advances come 1.2 min after the planned attempt (the attention pass doesn't delay them) |
| defences (`assault`: the defender builds 15 min, then the attacker gets 43 units incl. 3 catapults; 8 runs each) | defender eliminated in 40 min: no layout 4 of 8, layout only 4, layout + own towers 5, layout + towers + walls 1 of 8 (buildings lost per AI 7.6 -> 4.4). The walls make the difference: the attacking army stops at them under tower fire. Without walls bronze towers fall to the catapults (range 7 > 5). Random-map suite: no cost to economy or army. One crash in the engine's damage effect (EEEDamagedEffect::vf19, a map lookup on a missing table) in ~40 runs with walls, not reproducible |
| counter-siege (6-8 runs per variant) | first version (sally when outnumbering the soldiers within 5 tiles by 2): siege_towers attacker wins 4 of 6 instead of 1 of 6 - the 14 defenders ran into the escorting army of 36 and the base fell. With 2x + 3 within 8 tiles: siege_towers 2 of 6 either way (fires in 2 of 6 games), assault never fires (the catapults come with the army), siege_raid (4 catapults, 4 escorts): fires 9-29 times a game, but the towers and spearmen kill those catapults anyway - no measurable gain yet, no harm |
| defending army (`assault`, normal game raid at 46 min) | the defender has no army to answer with: 2 soldiers at minute 15 in `assault` (Bronze start with 5 citizens, its few requests a Bronze Frigate and Short Swords - iron), 10-12 at minute 45 in a normal game. Unit substitution (4 maps x 2): army at 40/50/60 min 9.6/13.5/12.5 -> 13.7/15.9/17.0, 605 substitutions, economy unchanged |

Open points:
- The army budget fix (unit cap leak) still needs a 4-map suite.
- Attack effectiveness against a human player needs playtesting.

## Lessons for testing

- Scenario and save runs are byte-identical (same speed), so those A/B tests are exact. Random-map runs
  differ between runs even with the same seed: compare averages over several runs. Check several maps
  all the same: one map overfits.
- Tower-handling features need a map with player-style defences (`pathtest.scn`); AI-vs-AI suites only show
  whether they hurt.
- A short test epoch timer hides economy problems. Use the real timer for anything about pacing.
- Without the cheat, look at the soldier count before judging attacks. Tiny armies make every attack look
  pointless.
- The game's statistics (player+0x4A8: kills, losses, buildings destroyed) are the best outcome measure. They
  are in the log every 10 minutes.
