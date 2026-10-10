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

Open points:
- The army budget fix (unit cap leak) still needs a 4-map suite.
- Attack effectiveness against a human player needs playtesting.

## Lessons for testing

- Runs at the same speed (up to 7.5) are byte-identical, so A/B tests are exact. Check several maps all the
  same: one map overfits.
- A short test epoch timer hides economy problems. Use the real timer for anything about pacing.
- Without the cheat, look at the soldier count before judging attacks. Tiny armies make every attack look
  pointless.
- The game's statistics (player+0x4A8: kills, losses, buildings destroyed) are the best outcome measure. They
  are in the log every 10 minutes.
