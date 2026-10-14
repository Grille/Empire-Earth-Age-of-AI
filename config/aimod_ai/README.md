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
    betrayed_me > 0 ? "Again, {name}? I should have known.",
    "You will regret this, {name}.",
]
peace_accept = [regard < -50 ? "Fine. I will keep an eye on you."] ?? "Peace it is, {friend}."

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

**A mistake** - a missing quote or bracket, an unknown key, situation or section - is written to the log
(`aimod_econ.csv` next to the game, lines starting with `# texts:`) with the file and line, and only that key is
skipped. A placeholder that is not a valid expression stays in the text as written. A bad file never stops the game.

**Writing texts?** Set `strict_parsing=1` in aimod.ini (`[general]`): every mistake then comes up at once - those in
the files when the game starts, in a message box; those found while playing (an unknown name in a condition, a
broken expression, a placeholder nobody fills) as a line in the game.

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
epoch_up = [chance(30) ? "Progress, finally.", "A new age begins."]
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
| `lord_victory` | {name}, your lord, has won the game | everyone | always |  |  |
| `block_refuse` | you refuse {name}'s peace offer because {enemy} in their alliance cannot be at peace with your side | {name} | always | 60 s | `peace_refuse` |
| `block_refuse_own` | you refuse {name}'s peace offer because your own ally {enemy} won't make peace with them | {name} | always | 60 s | `block_refuse` |
| `vassal_grumble` | your lord {name} forces you to make peace with {enemy}, whom you hate | {name} | always | 60 s |  |
| `ask_lord` | {name} offers you peace, but you are the vassal of {enemy} and have no diplomacy of your own - they must talk to {enemy} | {name} | always | 60 s | `peace_refuse` |
<!-- /situations -->

## Expressions

| | |
|---|---|
| Numbers, texts | `85`, `0.5`, `"text"`, `'text'` |
| Arithmetic | `+ - * / %` - `+` joins texts unless both sides are numbers |
| Comparisons | `< <= > >= == !=` - numbers when both are numbers, else texts (case ignored) |
| Logic | `and or not` (also `&& \|\| !`) |
| Choice | `a ? b : c`, `a ? b` (nothing when `a` doesn't hold), `a ?? b` (`b` when `a` is nothing) |
| Lists | `[a, b, c]` - one of them at random, leaving out what is nothing |
| Functions | `chance(25)` (true 25% of the time), `min(a, b, ...)`, `max(a, b, ...)`, `abs(a)`, `round(a)` |
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
third player - **enemy** (`{enemy}`): the enemy in an alliance offer, the ally or lord in some lines. In
`_system.eeai` `them` is you, the local player.

Each of the three is a value whose text is the player's name (`{me}`, `{them}`, `{enemy}`, and `{name}` for
`{them}`), with members: `me.soldiers`, `them.strength`, `enemy.epoch`. A **relation** is between me and that
player: `them.opinion` is what I think of them, `them.regard` what they think of me, `enemy.at_war` whether I am at
war with the enemy. A player who isn't there - no third player in a greeting - is nothing, and so are its members.

Shortcuts: a relation without `them.` (`opinion` is `them.opinion`), and my own `epoch` and traits without `me.`
(`aggression` is `me.aggression`). Everything else needs its player: `me.soldiers`, `them.soldiers`.

<!-- values: generated by scripts/gen_eeai_readme.py -->
**Of each player** - `me.`, `them.`, `enemy.`:

| Member | |
|---|---|
| `X.name` | its name - the same as the player itself in a text: {them} = {them.name} |
| `X.soldiers` | its soldiers (all numbers of units are counted every 10 seconds) |
| `X.citizens` | its citizens |
| `X.buildings` | its buildings |
| `X.towers` | its towers |
| `X.centers` | its Town Centers and Capitols |
| `X.strength` | its fighting strength (its soldiers' hit points, 30 a building) - to compare: them.strength &gt; me.strength * 2 |
| `X.rank` | its place by strength among the players still in, 1 = the strongest |
| `X.epoch` | its epoch: the name in a text, 1 (Prehistoric) .. 15 (Space) as a number - compare it with epoch.middle etc. |
| `X.alive` | 1 while it is still in the game |
| `X.human` | 1 for the human player at this PC |
| `X.vassal` | 1 when it has capitulated to someone |
| `X.vassals` | how many have capitulated to it |
| `X.aggression`, `X.loyalty`, `X.pride`, `X.greed`, `X.forgiveness`, `X.caution`, `X.talk` | its traits, 0-100 (50 without a personality) |

**Only my own** - `me.` (another player's stock is hidden in the game):

| Member | |
|---|---|
| `me.food`, `me.wood`, `me.stone`, `me.gold`, `me.iron` | my stock |

**Relations** between me and the player - `them.` (or without it), `enemy.`:

| Member | |
|---|---|
| `X.opinion` | -100..100: what I think of them |
| `X.regard` | -100..100: what they think of me (an AI's opinion; 0 from a human) |
| `X.at_war` | 1 when we are enemies, both of us |
| `X.allied` | 1 when we are allied, both of us |
| `X.hostile` | 1 when they treat me as an enemy |
| `X.my_enemy` | 1 when I treat them as an enemy |
| `X.war_minutes` | how long our war has lasted - nothing when we are not at war |
| `X.alliance_minutes` | how long we have been allied - nothing when we are not |
| `X.my_lord` | 1 when they are my lord (I capitulated to them) |
| `X.my_vassal` | 1 when they capitulated to me |
| `X.betrayed_me` | how often they turned on me while we were allied |
| `X.betrayed_by_me` | how often I turned on them while we were allied |
| `X.refused` | how often they refused my offers of peace or alliance |
| `X.ignored` | my alliance offers they let run out, in a row |
| `X.tribute_got` | resources they have paid me in this game |
| `X.tribute_paid` | resources I have paid them |
| `X.at_my_base` | their soldiers within 20 tiles of my base now (an AI's base) |
| `X.attack_minutes` | minutes since their last attack wave set out against me - nothing if none (an AI's waves) |
| `X.my_attack_minutes` | minutes since my last attack wave against them - nothing if none |
| `X.kills` | their soldiers and citizens I have destroyed |
| `X.losses` | my soldiers and citizens they have destroyed |
| `X.recent_kills` | kills in the last 20 minutes |
| `X.recent_losses` | losses in the last 20 minutes |
| `X.razed` | their buildings I have destroyed |
| `X.my_razed` | my buildings they have destroyed |

**The game:**

| Name | |
|---|---|
| `me` | the AI that speaks - its name in a text; me.soldiers ... |
| `them` | the player it speaks to or about ({name}); in a system line you, the local player |
| `enemy` | a third player: the enemy in an alliance offer ({enemy}), the ally or lord in some lines |
| `minutes` | the game time in minutes |
| `players` | the players still in, me among them |
| `allies` | my allies still in |
| `enemies` | my enemies still in |
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
