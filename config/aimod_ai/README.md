# The aimod_ai folder

This folder holds the words of eeaimod's computer players: their personalities, the system lines the mod shows
you, and what it tells a language model when the AI voice is on. Every file is plain text (UTF-8) with the
extension `.eeai`; this README is not read by the game. The files are read once, when Empire Earth starts - restart
the game after editing.

| File | What it is |
|---|---|
| `eroberer.eeai`, `haendler.eeai`, ... | One personality each: a name, colours, character traits and the lines it says |
| `computer.eeai` | No personality but the fallback: neutral lines for what must not go unsaid (war, peace, offers, capitulation, victory) |
| `_persona.eeai` | An empty personality with every key, to copy |
| `_system.eeai` | The system lines the mod shows you (white, from nobody in particular) |
| `_template.eeai` | What the mod tells the language model (`[llm]` in aimod.ini): who the AI is, the game as it stands, the situations, the tools |

Files whose names start with `_` are no personalities. A key left out of `_system.eeai` or `_template.eeai` keeps
the built-in text; delete either file to get every built-in text back.

## A new personality

Copy `_persona.eeai` to a new name in this folder, for example `seefahrer.eeai`, and fill it in. Every AI of a game
gets a personality at random, each one once before any repeats; on random maps its favoured colour comes first. To
give a player a particular one, set `personality_P2=seefahrer` (the file name without `.eeai`) in aimod.ini.
`ai_talk=0` in aimod.ini silences all lines, `ai_names=0` keeps the game's own player names.

```ini
[personality]
name = "Captain Ines"
name:de = "Kapitänin Ines"
color = ["lightblue", "cyan"]
aggression = 40
loyalty = 70
character = "A sea captain: dry humour, proud of her fleet, keeps her word."

[vars]
friend = opinion > 30 ? "old friend" : "captain"

[lines]
greeting = ["Fair winds, {name}.", "The tide is with us today."]
war_declared = [
    them.betrayals(me) > 0 ? "Again, {name}? I should have known.",
    "You will regret this, {name}.",
]
peace_accept = [them.kills(me) > 100 ? "Fine. I will not forget my dead."] ?? "Peace it is, {friend}."

[lines:de]
greeting = ["Guten Wind, {name}.", "Die Flut ist heute mit uns."]
```

## The format

A file is made of **sections** - `[personality]`, `[lines]`, `[lines:de]` ... - and **keys**, one `key = value` per
line. A comment runs from `;` or `#` to the end of the line, also after a value (not inside a string). A value is
an **expression**; it may go over several lines while a `(`, `[` or `{` is open:

| Value | Meaning |
|---|---|
| `"text"` | A string. `\"` is a quote, `\\` a backslash, `\n` a line break. `'text'` works too. |
| `85`, `0.5` | A number |
| `["a", "b", "c"]` | A list: one of its items, at random. A comma after the last item is fine. |
| `allied ? "Welcome back."` | A condition without `:` - nothing when it doesn't hold |
| `opinion > 30 ? "friend" : "stranger"` | One or the other |
| `a ?? b` | `a`, unless it is nothing: then `b` |

**Inside a string**, braces hold a placeholder or an expression: `"Hello, {name}!"`,
`"Hello, {opinion > 30 ? "friend" : "you"}!"`. The quotes inside the braces belong to the expression and don't end
the string.

**A mistake** - a missing quote or bracket, an expression that doesn't parse (also in a string's `{placeholder}`), an
unknown key, situation or section - is written to the log (`aimod_econ.csv` next to the game, lines starting with
`# texts:`) with the file and line, and only that key is skipped. The files are checked when the game starts, every
line, not only those that happen to be said. A bad file never stops the game.

**Writing texts?** Set `strict_parsing=1` in aimod.ini (`[general]`): every mistake then comes up at once - those in
the files when the game starts, in a message box; those that only show while playing (an unknown name, a
placeholder nobody fills, a player used who isn't there) as a line in the game.

The VS Code extension in the mod's sources (`vscode/eeaimod-texts`) highlights these files and marks unknown keys,
situations and `epoch.` names.

## Lines

Each key of `[lines]` is a **situation**; its value is what the AI says then - usually a string or a list:

```ini
attack = "My riders are on their way."
greeting = ["Hello.", "Greetings, {name}.", "Who are you?"]
```

How a line is chosen:

1. The situation happens (the table below). Lines that aren't always said have a **chance** (scaled by the
   personality's `talk`: at talk=100 twice as often as at 50), a **cooldown** per AI, and need 3 seconds since the
   AI's last line.
2. The value is worked out: a list gives one of its items at random, conditions leave items out. The same text as
   last time is avoided when there is another.
3. **Nothing to say** - no key, `[]`, or a list whose conditions all fail - falls back to a more general situation
   (the fallback column), then to `computer.eeai`, else the AI stays silent.
4. Only those it is meant for hear it: everyone, the AI's allies, or the player it is said to (`{name}`).
5. With the AI voice on (`[llm]` in aimod.ini) a language model says the line in the AI's own words, from its
   `character`, its traits and the game as it stands; the chosen line is its example.

`[lines:de]` (or `:fr`, `:es`, `:it`, `:pl`, `:pt-br`, `:ru`, `:ko`, `:zh-cn`, `:zh-tw`) replaces `[lines]`
situation by situation in a game of that language; the language is detected from the game (or set with
`language=` in aimod.ini). The same works for `name:de` and `[vars:de]`.

### Conditions and variety

```ini
; nothing unless the condition holds - a list leaves it out
greeting = [allied ? "Good to see you, ally.", "Hello, {name}."]

; one or the other
winning = opinion > 0 ? "Let us stay friends." : "You are next, {name}."

; the angry lines when they hate it, else the friendly ones
angry = ["Go away.", "You again?"]
tribute_thanks = [opinion < -50 ? angry] ?? ["Thank you!", "Most generous."]

; a list in a list: each of its items counts as one (here a third each)
greeting = ["Hello.", angry]

; now and then
epoch_up = [chance(30%) ? "Progress, finally.", "A new age begins."]

; one situation, told apart by how I stand with them: someone else has won
other_victory = lord == them ? "Long live {name}, my lord!" : allied ? "We won together, {name}!" : "Enjoy it while it lasts."
```

### Situations

<!-- situations: generated by scripts/gen_eeai_readme.py -->
| Situation | When - "you" is the AI | Heard by | Chance | Cooldown | Without a line |
|---|---|---|---|---|---|
| `greeting` | the game begins - a first word to everyone | everyone | 60% |  |  |
| `war_declared` | {name} has just declared war on you | {name} | always | 20 s |  |
| `betrayed` | {name}, your ally until now, has betrayed you and declared war | {name} | always | 20 s | `war_declared` |
| `peace_accept` | you accept {name}'s offer of peace (you are allied now) | {name} | always |  |  |
| `peace_refuse` | you refuse {name}'s offer of peace | {name} | always | 60 s |  |
| `alliance_offer` | you offer {name} an alliance against {enemy}, your common enemy | {name} | always |  |  |
| `offer_accepted` | {name} has accepted your alliance offer | {name} | always |  |  |
| `offer_withdrawn` | your alliance offer to {name} has expired unanswered | {name} | always |  |  |
| `betray` | you betray {name}, your ally, and declare war on them | {name} | always |  |  |
| `tribute_thanks` | {name} sent you a generous tribute | {name} | always | 20 s |  |
| `tribute_small` | {name} sent you a tribute - a small one | {name} | always | 60 s |  |
| `attack` | your army sets out to attack {name} | its allies | 40% | 300 s |  |
| `under_attack` | your town is under attack | its allies | 60% | 240 s |  |
| `building_lost` | you lost a building | its allies | 25% | 180 s |  |
| `epoch_up` | you have just reached the {epoch} age | everyone | 50% | 60 s |  |
| `enemy_ahead` | an enemy is a whole age ahead of you | its allies | 40% | 600 s |  |
| `winning` | you are winning against {name} | {name} | 40% | 600 s |  |
| `losing` | you are losing the war | its allies | 50% | 600 s |  |
| `desperate` | you are close to defeat | its allies | always |  |  |
| `enemy_defeated` | {name} has been defeated | everyone | 80% | 30 s |  |
| `ally_defeated` | your ally {name} has been defeated | its allies | 80% | 30 s |  |
| `defeated` | you have been defeated | everyone | always |  |  |
| `wonder_threat` | a wonder is threatened | everyone | always | 60 s |  |
| `wonder_help` | you send help for an ally's wonder | its allies | always | 60 s |  |
| `wonder_nohelp` | you have no help to give for an ally's wonder | its allies | always | 60 s |  |
| `help_land` | you send land troops to help an ally | its allies | always | 30 s |  |
| `help_air` | you send aircraft to help an ally | its allies | always | 30 s |  |
| `help_naval` | you send ships to help an ally | its allies | always | 30 s |  |
| `help_none` | you have no troops to send an ally who asked for help | its allies | always | 30 s |  |
| `surrender` | you give up the game | everyone | always |  | `defeated` |
| `capitulate` | you capitulate: {name} is beating you, and you offer to PAY them a tribute ({tribute}) for peace - you give, they receive | {name} | always |  |  |
| `capitulate_accepted` | {name} accepted your capitulation: you PAY them the tribute ({tribute}) and serve them from now on | {name} | always |  |  |
| `capitulate_refused` | {name} did not accept your capitulation: the war goes on | {name} | always |  |  |
| `attack_threat` | your army is about to attack {name} | {name} | 60% | 300 s |  |
| `all_submitted` | every other player has submitted to {name}, your lord | {name} | always |  |  |
| `other_victory` | {name} has won the game | everyone | always |  |  |
| `block_refuse` | you refuse {name}'s peace offer because {enemy} in their alliance cannot be at peace with your side | {name} | always | 60 s | `peace_refuse` |
| `block_refuse_own` | you refuse {name}'s peace offer because your own ally {enemy} won't make peace with them | {name} | always | 60 s | `block_refuse` |
| `vassal_grumble` | your lord {name} forces you to make peace with {enemy}, whom you hate | {name} | always | 60 s |  |
| `ask_lord` | {name} offers you peace, but you are the vassal of {enemy} and have no diplomacy of your own - they must talk to {enemy} | {name} | always | 60 s | `peace_refuse` |
<!-- /situations -->

## Expressions

| | |
|---|---|
| Numbers, texts | `85`, `0.5`, `70%` (the same as `0.7` - `%` only right after a number), `"text"`, `'text'` |
| Arithmetic | `+ - * /` - `+` joins texts unless both sides are numbers; `me.strength * 150%` |
| Comparisons | `< <= > >= == !=` - numbers when both are numbers, else texts (case ignored) |
| Logic | `and or not` - only the words (`&&`, `\|\|` and `!` are a mistake; `!=` is a comparison) |
| Choice | `a ? b : c`, `a ? b` (nothing when `a` doesn't hold), `a ?? b` (`b` when `a` is nothing) |
| Lists | `[a, b, c]` - one of them at random, leaving out what is nothing |
| Functions | `chance(p)` - true with the probability p, 0 to 1: `chance(25%)` or `chance(0.25)` (`chance(25)` is a mistake); `rnd()` - a random number, 0 up to but not 1 (`rnd() < 25%` is `chance(25%)`); `rnd(n)` - 0 up to n; `min(a, b, ...)`, `max(a, b, ...)`, `abs(a)`, `round(a)` |
| Grouping | `( )` |

- **True** is a number other than 0 or a text that isn't empty. **Nothing** is false, an empty text in a line, 0 in
  arithmetic, and a comparison with nothing is false (`war_minutes > 5` and `war_minutes < 5` are both false when
  there is no war).
- **Numbers and texts at once:** some values are both - the epoch is 3 and "Copper Age", a placeholder "12" is 12
  and "12". The number counts in arithmetic, in comparisons with a number and as true/false; the text counts in a
  line and next to texts (`"Age: " + epoch`).
- Only what is chosen is worked out: the other side of `? :`, `??`, `and`, `or` is not (no random draws, no
  errors from text that isn't said).
- An unknown name counts as 0 and is logged. There are no loops and no assignments: a text can't hang the game.

### [vars]

`[vars]` names strings and expressions for the texts. A personality's own `[vars]` come first, then those of
`_system.eeai`; `_template.eeai` has its own for the prompts. `[vars:de]` replaces them in a German game.

```ini
[vars]
address = opinion > 30 ? "friend" : opinion < -30 ? "enemy" : "neighbour"
title = "my lord {name}"
angry = ["Go away.", "You again?"]
```

Then `{address}`, `{title}` in a line, or `angry` in an expression.

## The game's values

A line is said by one AI - **me** - to or about another player - **them** (`{name}`). Some lines also involve a
third player - **enemy** (`{enemy}`): the enemy in an alliance offer, the ally or lord in some lines. **lord** is
my lord - the player I capitulated to - in every line, and nothing while I have none. In `_system.eeai` `them` is
you, the local player.

Each of them is a player: in a text its name (`{me}`, `{them}`, `{enemy}`, `{lord}`, and `{name}` for `{them}`), in a
comparison itself - `lord == them` holds when the one I talk to is my lord. A player has members - `me.soldiers`,
`them.strength`, `enemy.epoch`, `them.lord` (a player again: `them.lord.name`, `them.lord == me`) - and
**relations**, which name the other player: `me.opinion(them)` is what I think of them, `them.opinion(me)` what they
think of me, `enemy.at_war(them)` whether the enemy is at war with them. A human has no opinion the game could
read: `them.opinion(me)` of a human is a guess from what I did to them - war (a betrayal most), peace, tribute, a
refused offer, their losses - so a line built on it should sound like my impression ("You must hate me by now"),
not a fact.

A player who isn't there - no third player in a greeting, no lord - is nothing. **Check it before you use it:**

```ini
under_attack = lord ? "My lord {lord} will hear of this!" : "To arms!"
greeting = [lord and lord.soldiers > 50 ? "My lord's army is mighty, {name}."] ?? "Greetings, {name}."
tribute_thanks = "{lord ?? me} thanks you, {name}."
```

A condition (`lord ? ...`, `lord and ...`, `not lord`), `??` and a comparison (`lord == them`) check it. Used without
a check - in a text (`{lord}`), a sum, a member (`lord.soldiers`) or a relation (`opinion(lord)`) - it gives
nothing, and with `strict_parsing=1` it is a mistake: a line that names a lord nobody has is a bug in the line.

Shortcuts: my own `epoch`, traits, `allies` and `enemies` without `me.` (`aggression` is `me.aggression`), a relation of mine without
`me.` (`opinion(enemy)` is `me.opinion(enemy)`), and `opinion`, `at_war`, `allied` alone: mine with them
(`me.opinion(them)`). Everything else needs its players: `them.soldiers`, `me.kills(them)`.

<!-- values: generated by scripts/gen_eeai_readme.py -->
**Of each player** - `me.`, `them.`, `enemy.`, `lord.`:

| Member | |
|---|---|
| `X.name` | its name - the same as the player itself in a text: {them} = {them.name} |
| `X.soldiers` | its soldiers (all numbers of units are counted every 10 seconds) |
| `X.citizens` | its citizens |
| `X.population` | its units: soldiers and citizens - to scale a number to the game's stage: population_limit, max_population |
| `X.buildings` | its buildings |
| `X.towers` | its towers |
| `X.centers` | its Town Centers and Capitols |
| `X.strength` | its fighting strength (its soldiers' hit points, 30 a building) - to compare: them.strength &gt; me.strength * 2 |
| `X.rank` | its place by strength among the players still in, 1 = the strongest |
| `X.epoch` | its epoch: the name in a text, 1 (Prehistoric) .. 15 (Space) as a number - compare it with epoch.middle etc. |
| `X.alive` | 1 while it is still in the game |
| `X.human` | 1 for the human player at this PC |
| `X.vassals` | how many have capitulated to it |
| `X.allies` | how many are allied with it (both ways), still in - allies alone: mine |
| `X.enemies` | how many it treats as enemies, still in - enemies alone: mine |
| `X.aggression`, `X.loyalty`, `X.pride`, `X.greed`, `X.forgiveness`, `X.caution`, `X.talk` | its traits, 0-100 (50 without a personality) |
| `X.food`, `X.wood`, `X.stone`, `X.gold`, `X.iron` | its stock - another's too (the AI's spies): them.gold |
| `X.stock` | the five resources together - to scale amounts: them.tribute(me) &gt; me.stock / 2 |
| `X.lord` | its lord, the player it capitulated to - a player: them.lord.name, lord == them; nothing when it has none |

**Relations** of player A with player B - `me.opinion(them)`, `them.opinion(me)`, `enemy.at_war(them)`; without `A.` mine (`opinion(enemy)`). Those that count events - `tribute`, `refusals`, `betrayals`, `kills`, `razed` - take a time window as a second argument: `them.tribute(me, 15)` counts the last 15 minutes only (`16.5` too), without it the whole game:

| Relation | |
|---|---|
| `A.opinion(B)` | -100..100: what A thinks of B - an AI's opinion; a human's of an AI is a guess from what the AI did to them (war, a betrayal, peace, tribute, a refusal, their losses), nothing of another human |
| `A.at_war(B)` | 1 when A and B are enemies, both of them |
| `A.allied(B)` | 1 when A and B are allied, both of them |
| `A.hostile(B)` | 1 when A treats B as an enemy (whatever B does) |
| `A.war_minutes(B)` | how long the war of A and B has lasted: from both enemies until both are allied again (a peace offer doesn't end it; in peace_accept: the war just ended) - nothing without a war |
| `A.alliance_minutes(B)` | how long A and B have been allied - nothing when they are not |
| `A.betrayals(B)` | how often A turned on B while they were allied (an alliance of 2 minutes or more: a peace broken at once is war, not betrayal) - (B, minutes): in the last minutes |
| `A.refusals(B)` | how often A refused B's offers of peace or alliance - (B, minutes): in the last minutes |
| `A.ignored(B)` | B's alliance offers A let run out, in a row |
| `A.tribute(B)` | resources A has paid B in this game - (B, minutes): in the last minutes, them.tribute(me, 15) |
| `A.near(B)` | A's soldiers within 20 tiles of B's base now, while B treats A as an enemy (an AI's base) |
| `A.attack_minutes(B)` | minutes since A's last attack wave set out against B - nothing if none (an AI's waves) |
| `A.kills(B)` | B's soldiers and citizens A has destroyed - (B, minutes): in the last minutes (whole minutes, at most 60) |
| `A.razed(B)` | B's buildings A has destroyed - (B, minutes) as kills |

**The game:**

| Name | |
|---|---|
| `me` | the AI that speaks - its name in a text; me.soldiers ... |
| `them` | the player it speaks to or about ({name}); in a system line you, the local player |
| `enemy` | a third player: the enemy in an alliance offer ({enemy}), the ally or lord in some lines |
| `lord` | my lord (me.lord), the player I capitulated to - in every line; nothing when I have none |
| `opinion` | me.opinion(them): what I think of them |
| `at_war` | me.at_war(them) |
| `allied` | me.allied(them) |
| `minutes` | the game time in minutes |
| `players` | the players still in, me among them |
| `population_limit` | the game's unit limit per player (its setting) |
| `max_population` | the largest population of a player still in - kills &gt; max_population / 2: many for this stage of the game |
| `language` | the game's language: de, en, fr, ... |
<!-- /values -->

### Epochs

`epoch` is the epoch's name in a line - in the game's language - and its number 1..15 in a comparison. Compare it
with these names, the same in every language (a name in quotes would only match in one):

<!-- epochs: generated by scripts/gen_eeai_readme.py -->
| Name | Number | Age |
|---|---|---|
| `epoch.prehistoric` | 1 | Prehistoric |
| `epoch.stone` | 2 | Stone |
| `epoch.copper` | 3 | Copper |
| `epoch.bronze` | 4 | Bronze |
| `epoch.dark` | 5 | Dark |
| `epoch.middle` | 6 | Middle |
| `epoch.renaissance` | 7 | Renaissance |
| `epoch.imperial` | 8 | Imperial |
| `epoch.industrial` | 9 | Industrial |
| `epoch.atomic_wwi` | 10 | Atomic - WWI |
| `epoch.atomic_wwii` | 11 | Atomic - WWII |
| `epoch.atomic_modern` | 12 | Atomic - Modern |
| `epoch.digital` | 13 | Digital |
| `epoch.nano` | 14 | Nano |
| `epoch.space` | 15 | Space |
<!-- /epochs -->

```ini
epoch_up = [epoch >= epoch.middle ? "Steel and stone!", "Another age, {name}."]
```

## Placeholders

In a line, `{...}` holds a placeholder or any expression: `{name}`, `{me}`, `{enemy}`, `{epoch}`, a `[vars]` name,
`{them.soldiers}`, `{round(me.strength / 100)}`. Personality lines also have `{tribute}`: what a capitulation pays,
by resource ("200 gold, 800 iron").

`_system.eeai` and `_template.eeai` pass their own placeholders - `{ai}`, `{lord}`, `{minutes}`, ... - each key's
are listed above it in the file.

## [personality] and [style]

| Key | Value |
|---|---|
| `name`, `name:de`, ... | The player's name in the game (`name:de` in a German game) |
| `color` | Favoured colours on random maps, the first free one wins: `["blue", "cyan"]` - blue, red, yellow, magenta, orange, green, cyan, darkred, darkgrey, purple, grey, silver, lightyellow, lime, lightblue, pink (or the German names, or "0".."16") |
| `aggression` | 0-100 (50 when left out): betrays, refuses peace |
| `loyalty` | keeps its alliances |
| `pride` | how long it holds a grudge and refuses peace after a declaration of war (pride x 3 s), how long it fights on before it capitulates |
| `greed` | what a tribute is worth to it |
| `forgiveness` | how fast a grudge fades |
| `caution` | capitulates earlier, pays more |
| `talk` | how often it says the lines that are not always said |
| `character` | For the AI voice: who this is, in a few words - the model plays it |

`[style]` is **experimental** and only read with `ai_style=1` in aimod.ini (off by default: every AI plays the same,
tested way). Its keys are aimod.ini settings that replace the difficulty's values for this AI:

<!-- style: generated by scripts/gen_eeai_readme.py -->
`military_start_share`, `military_income_share`, `military_income_share_farms`, `epoch_min`, `epoch_max`, `first_attack_min`, `first_attack_max`, `attack_interval_min`, `attack_interval_max`, `attack_group_percent`, `attack_on_contact`, `attack_for_resources`, `attack_revenge`, `citizen_share`, `home_guard_share`
<!-- /style -->

## _system.eeai and _template.eeai

`_system.eeai` holds the lines the mod shows you itself: `[system]` in English, `[system:de]` (or another language)
replaces what it has in a game of that language. `_template.eeai` holds everything the language model is told, in
English (the model is told which language to answer in); its pieces are joined with a space. Both files list every
key with its built-in text and its placeholders. `[vars]` in `_system.eeai` serve the system lines and every
personality's lines; `[vars]` in `_template.eeai` serve the prompts only.

## Trying things out

The control pipe (for developers, see `docs/testing.md` in the sources) has `texts eval P Q <expression>` - what an
expression gives with player P speaking to Q in the running game - and `texts line P <situation> [Q]` - the line P
would say now.
