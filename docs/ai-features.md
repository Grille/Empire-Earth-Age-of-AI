# The computer player: what the mod changes

The original AI cheats: by default it pays about 1% of the real prices. With cheating switched off, it
can't run its economy, gets stuck in the early epochs, only trains soldiers for its next attack and leaves its
base undefended. This mod changes that. Every feature has its own setting in `ageofai.ini`; the AI never gets
information a human couldn't have (it only uses enemy buildings it has seen).

**Where it is meant to play well.** A population of 200-400 per player is the target the AI is tuned and tested for
(the tests run at 300 per player); up to 1000 per player is expected to work well too. Below 100 per player it plays,
but isn't tuned for it - some features (the citizen shelf, the fortress reserve) then rarely or never act. The
total of the game's own population setting is misleading here: the per-player number is what counts.

## Economy

| Feature | What it does |
|---|---|
| Fair prices | The AI pays what you pay (configurable per difficulty, e.g. a discount only on Hard). |
| Economy first | Before farms (Copper Age), the AI grows its workforce, counts the wild food it has found as its budget, saves for the next epochs and keeps only a token army. |
| Military budget | Soldiers are paid from a share of the starting stock and of the income, never from the epoch reserve; from the Copper Age on most of the income goes to the army. |
| Farm fields | The original AI left the fields around its granary as bare foundations (always on maps with fish, often on land), put nobody on finished ones and never replaced destroyed ones. It now builds them as far as its wood pays (keeping 50 wood per field for that), farms every one and lays out lost fields again. |
| More granaries | The original AI built a single granary (8 fields) all game; its food stayed short and spare citizens went to gold. It now adds granaries while it wants more food workers than its fields hold. |
| Citizens, towers and raids | Citizens no longer gather inside or walk past the range of enemy towers the AI has seen, nor go one by one to a field or mine where enemy soldiers stand; those working there are called back. The AI no longer plans its own towers and walls where the builders would walk into enemy fire. |
| Ways round danger | (`danger_routes`) The AI keeps a map of where its enemies can reach - their defences it has seen, soldiers and warships it sees at their reach, where its citizens were hurt lately - and its units that can't fight (citizens, fishing boats, transports) take their way round it where there is one, through only where there is none. |
| No feeding a lost fight | (`avoid_lost_fights`) Where the AI sees an enemy force its soldiers nearby can't beat, its soldiers on their way somewhere go round it, its attack groups stop at its edge and pick no target inside it beyond their own reach - they wait there until enough have come to win, then go in together, instead of running into it a few at a time. |
| It remembers the enemy's army | (`fight_memory`) Its fight judgements - fall back, lost fights, the staging point - count the enemy soldiers it saw in the last 3 minutes where it saw them, not only those in sight at that moment: an army half in the fog no longer looks beatable until the AI's units are inside its reach. |
| One staging point | (`army_stage`) An attack's march from safety into the enemy's reach goes to one gathering point outside every danger on the way back to its base, instead of each attack group touring the enemy's buildings on its own. The army waits there and goes in together once it would win at the front; a group that wins at its target by itself still goes. |
| One army, ordered as one | (`mod_armies`, land maps) The AI's land soldiers are one force of the mod's instead of the game's attack commander's squads: it lives at the AI's front station, goes at the attack time as a whole when it beats the enemy at the nearest known enemy building, takes the next building while it wins - the target and the way to it chosen by the danger on the way (the enemy defences and shooters it has seen, and for 20 minutes where it last traded badly: going round a defended position wherever there is a way round) -, comes home as one when it is losing - before contact only on a clear deficit (under 0.7 of the enemy it sees ahead), in a fight once it has lost half again as many hit points as it took off the enemy over the last half minute - as a group, the fast waiting for the slow, and turning on the chasers it beats once it is clear of the enemy's base -, and answers a raid near home with the whole force. Against the commander (48 games each, 75 min): kills +30, losses -32, buildings lost 7 against 11 on Continental 1v1; losses -33, razed +7, buildings lost 12 against 20 with 4 AIs; buildings lost 7 against 10 on Mediterranean. The retreat as a group (96 games): soldiers lost on the way home 27 -> 7 a game, razed +2.3, buildings lost 4.6 against 6.9. The way round (24 games): razed +21 (se 12), soldiers +12 (6); on the player's save "choke wolf" with a push every 4 minutes (26 min) 45 buildings razed against 13, 540 units lost against 675. The retreat judged by the fight instead of the estimate (48 games, 4 AIs): kills -25 (se 11), losses +22 against the estimate's rule - on by the player's decision. |
| Stranded citizens fetched home | (`rescue_citizens`) Idle citizens left on land where their AI has no drop site - a transport put them off on an islet - are picked up by one of its transports and brought back to its main land. The game only gives a citizen work on its own land, so they would stand there for the rest of the game. |
| Walls where they help | (`game_walls`, `defence_walls`) The mod's AIs don't build the game's ring of walls around the base (one segment at a time, in practice a stray piece with a gate, its builders sent to the front one by one). They build one straight wall in front of their towers at each front: along the tiles or exactly diagonal, not a staircase, with a gate in its middle once the pieces there are finished - laid as the player lays one, over 5 finished pieces at once. |
| A shelf for idle citizens | (`shelve_citizens`) At its population limit with food, wood, gold and iron piled up, the AI puts citizens it doesn't need into fortresses (and builds more for them) so the room goes to soldiers, and trains no new citizens; they come out when a resource runs low or citizens are lost. |
| Production where it's needed | (`army_plan` 64) The AI adds a building of the kind that trains the soldiers it wants when those are busy, not only when all its production is - an AI training only cavalry had its stables full, its barracks idle and got no stable more. With resources piled up it may have more of them. |
| No crowds at the mines | (`mine_crowds`) A mine has room for 6 gatherers, but the game counts only those already working, so every search sent more citizens to a crowded mine - one AI had 86 on one gold mine while dozens of its mines stood empty. The citizens walking there or waiting count now, and those beyond the 6 go to the nearest free mine. |
| Settlement upgrades | (`garrison_upgrades`) Spare citizens - idle ones, and wood cutters while the wood stock is over 5000 - go into a Settlement by metal mines (5 make it a Town Center), then into the Town Center with the most metal mines near it (10 more: a Capitol, which takes up to 50). Each citizen inside raises the stone, gold and iron dropped there from mines within 4.5 tiles (+25% at 5, +50% at 15, +102% at 50); the citizens are used up. On the player's save at 114 min: 205 citizens in 10 minutes, Settlements 22 -> 11, Capitols 2 -> 9, the metal income of those minutes +10-15% with fewer miners. Head-to-head (24 games, 75 min): level; on by the player's decision (spare wood is a free gain). |
| Rebuilding a lost economy | (`recover_economy`) An AI that lost its economy late in the game (fewer than 40 workers) puts two thirds of its workers on food, lets its citizens spend the whole stock and trains no soldiers unless its base is attacked, until the economy is back. Before, a ruined AI spent the food it got on soldiers and stayed stuck. |
| The spoils of a resign | (`resign_spoils`) A player who leaves the game no longer leaves a frozen town behind. Its soldiers turn marauders: at war with everyone, they roam in bands hunting units and ignore buildings - they can't take a town, and houses to hit would only tie them down. Its buildings go to whoever beat it: everyone who did at least 20% of the damage it took in its last 10 minutes gets a share by that damage, the buildings nearest their own first, gates and airports whole. With nobody at 20% the buildings stay as they are. |
| Short queues | (`train_queue`) At most 3 units in each production building, counting the one in the making: the original queued every request however deep - 80 soldiers in one building are resources tied up for 40 minutes. The rest stays in the bank and pays for more production buildings, so it trains more at once. |
| Citizens fight back | Enemy citizens putting up a tower next to the AI's gatherers get attacked - the builders first, by its soldiers or its citizens - and in the early ages citizens beat off small raids when clearly stronger. |
| Paid upgrades | The original AI got all unit and tower upgrades of an epoch for free the moment it reached it. It now researches them at their buildings and pays, like a player. |
| Working build queue | The original build list started with a building that can't be placed, which blocked everything after it (granary, archery range, siege factory, university...). |
| Short ways to work | Citizens mine the free mine nearest to a drop site, move from oversupplied resources to short ones, and get the gathering upgrades the original AI rarely researched. |
| Epoch pacing | `classic` (the game's timer, at most one epoch ahead of the human), `timer` (the timer alone), `rush` (as soon as affordable), `follow` (never ahead of the human, prepared to follow at once) or `independent` (the default: the timer, and a few minutes after a human's advance the AIs behind follow). |

## Defence and army

| Feature | What it does |
|---|---|
| Towers and walls | Towers at the front facing the enemy and at outlying mines instead of scattered around; a wall arc in front of them. |
| Home guard | The AI keeps an army at home that grows with its population cap (the original trained only for its next attack). |
| Muster | When an enemy army in its base is too strong to fight (twice its soldiers and towers), the AI stops sending every new soldier in alone: they gather beyond the base and strike together once they can win; citizens next to the enemy flee there too. |
| Unit choice | When the AI asks for a unit it can't pay for or train, it trains the strongest soldier it can afford. |
| Counter-siege | Soldiers near the base attack enemy siege weapons shelling it, when clearly stronger than the escort. |

## Attacks

| Feature | What it does |
|---|---|
| Attack tuning | Fewer, bigger waves instead of a stream of small, wasted attacks. |
| Routing | Armies march around the enemy towers they know about and look for undefended targets instead of running into the tower line. |
| Siege | The AI builds siege weapons when it knows enemy defences, keeps them at firing range and breaks through walls. |

## Scenarios and gathering

| Feature | What it does |
|---|---|
| Scenario fix | AIs in scenarios started from the menu play like in skirmish games (the original turns most of their planning off). |
| Safe gathering | Optional: citizens only gather near their base, on routes away from known enemies. |

## More

This page covers the features players notice most; the full list of the AI's settings, with what each does and its
default, is `[ai_base]` in [config/ageofai.ini](../config/ageofai.ini). The navy and island play: [navy.md](navy.md);
the early economy against the base game's AI: [economy.md](economy.md); how the original AI works and the test
results: [ai-analysis.md](ai-analysis.md); the experiments that are off and why: [experiments.md](experiments.md).
Diplomacy and the personalities: [diplomacy.md](diplomacy.md).
