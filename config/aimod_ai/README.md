# The aimod_ai folder

This folder holds the words of eeaimod's computer players: their personalities, the system lines the mod shows
you, and what it tells a language model when the AI voice is on. Every file is plain text (UTF-8) with the
extension `.eeai`; this README is not read by the game. The files are read once, when Empire Earth starts - restart
the game after editing. The folder sits next to Empire Earth.exe; `ai_dir=` in aimod.ini's `[general]` reads it from
somewhere else instead (for example where you edit it: `ai_dir=_aimod\config\aimod_ai`, relative to the game folder
or absolute).

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
`ai_talk=0` in aimod.ini silences all lines, `ai_names=0` keeps the game's own player and civilization names.

```ini
[personality]
name = "Captain Ines"
name:de = "Kapitänin Ines"
civilization = "Free Harbours"
civilization:de = "Freie Häfen"
color = ["lightblue", "cyan"]
aggression = 40%
loyalty = 70%
character = "A sea captain: dry humour, proud of her fleet, keeps her word."

[vars]
friend = opinion > 30% ? "old friend" : "captain"

[lines]
greeting = ["Fair winds, {them}.", "The tide is with us today."]
war_declared = [
    them.betrayals(me) > 0 ? "Again, {them}? I should have known.",
    "You will regret this, {them}.",
]
peace_accept = [them.kills(me) > 100 ? "Fine. I will not forget my dead."] ?? "Peace it is, {friend}."

[lines:de]
greeting = ["Guten Wind, {them}.", "Die Flut ist heute mit uns."]
```

## The format

The same language runs the mod's test scenarios, with statements, variables and functions on top
([docs/scripts.md](../../docs/scripts.md) in the mod's sources) - a personality's file has keys and values only.

A file is made of **sections** - `[personality]`, `[lines]`, `[lines:de]` ... - and **keys**, one `key = value` per
line. A comment runs from `;` or `#` to the end of the line, also after a value (not inside a string). A value is
an **expression**; it may go over several lines while a `(`, `[` or `{` is open:

| Value | Meaning |
|---|---|
| `"text"` | A string. `\"` is a quote, `\\` a backslash, `\n` a line break, `\{` `\}` braces that are text (no placeholder). `'text'` works too. |
| `85`, `0.5` | A number |
| `["a", "b", "c"]` | A list. Said or used as one value, it is one of its items, at random; `len`, `first`, `item`, `contains` and `map_range` see all of them. A comma after the last item is fine. |
| `allied ? "Welcome back."` | A condition without `:` - nothing when it doesn't hold |
| `opinion > 30% ? "friend" : "stranger"` | One or the other |
| `a ?? b` | `a`, unless it is nothing: then `b` |

**Inside a string**, braces hold a placeholder or an expression: `"Hello, {them}!"`,
`"Hello, {opinion > 30% ? "friend" : "you"}!"`. The quotes inside the braces belong to the expression and don't end
the string.

**Long values over several lines.** A line break counts as a space while a `(`, `[` or `{` is open. A string itself
(and a `{...}` inside it) stays on one line - for a long text, put strings side by side in `( )`: they are one
string. That works for every key, also in `[prompt]` and `[system]`, which take strings only:

```ini
peace_offer = ("Peace, {them}? "
    "My walls have held so far - "
    "but I would rather build than fight.")

report = (
    allied ? "We stand together." :
    them.kills(me) > 10 ? (
        "You have killed {them.kills(me)} of mine. "
        "I have not forgotten."
    ) :
    "We are not friends - not yet."
)
```

Only inside `( )`: in a list, `["Hello" "Hi"]` stays a mistake (a missing comma), not one joined string. `+` joins
texts too (`"a " + "b"`), but adds when both sides are numbers. A line that starts with `[` at its very beginning is
a new section; indent the `[` of a list that starts a line of a value.

**A mistake** - a missing quote or bracket, an expression that doesn't parse (also in a string's `{placeholder}`), an
unknown key, situation or section - is written to the log (`aimod_econ.csv` next to the game, lines starting with
`# texts:`) with the file and line, and only that key is skipped. The files are checked when the game starts, every
line, not only those that happen to be said. A bad file never stops the game.

**A name nobody knows** (`them.soldirs`, `{foo}`) is found when the file is read too and logged with its file and
line, but the key is kept. Some mistakes can only show while the game runs: `"a" * 2`, a text and a number added or
compared, a division by 0, a player who isn't there used without a check (`lord.soldiers` without a lord). Nothing
is guessed then. The mistake is logged, and what it is in is left out: an item of a list (`[foo > 3 ? "Ha!",
"Hello."]` says "Hello."), else the whole line, which then falls back like a line with nothing to say. A broken `{placeholder}` in a system text or prompt writes nothing.

**Writing texts?** Set `strict_parsing=1` in aimod.ini (`[general]`): every mistake then comes up at once - those in
the files when the game starts, in a message box; those that only show while playing (`"a" * 2`, a player used who
isn't there) as a line in the game.

The VS Code extension in the mod's sources (`vscode/eeaimod-texts`) highlights these files and marks unknown keys,
situations and `epoch.` names.

## Lines

Each key of `[lines]` is a **situation**; its value is what the AI says then - usually a string or a list:

```ini
attack = "My riders are on their way."
greeting = ["Hello.", "Greetings, {them}.", "Who are you?"]
```

How a line is chosen:

1. The situation happens (the table below). Lines that aren't always said have a **chance** (scaled by the
   personality's `talk`: at talk=100 twice as often as at 50), a **cooldown** per AI, and need 3 seconds since the
   AI's last line.
2. The value is worked out: a list gives one of its items at random, conditions leave items out. The same text as
   last time is avoided when there is another.
3. **Nothing to say** - no key, `[]`, or a list whose conditions all fail - falls back to a more general situation
   (the fallback column), then to `computer.eeai`, else the AI stays silent.
4. Only those it is meant for hear it: everyone, the AI's allies, or the player it is said to (`{them}`).
5. With the AI voice on (`[llm]` in aimod.ini) a language model says the line in the AI's own words, from its
   `character`, its traits and the game as it stands; the chosen line is its example.

`[lines:de]` (or `:fr`, `:es`, `:it`, `:pl`, `:pt-br`, `:ru`, `:ko`, `:zh-cn`, `:zh-tw`) replaces `[lines]`
situation by situation in a game of that language; the language is detected from the game (or set with
`language=` in aimod.ini). The same works for `name:de` and `[vars:de]`. The included personalities are English only
while their lines still change; translations come with a release, in a format still to be designed (a language
needs conditions of its own for its grammar - one listener or several, gender - but the AI's decisions what to say
should stay the same in every language).

### Conditions and variety

```ini
; nothing unless the condition holds - a list leaves it out
greeting = [allied ? "Good to see you, ally.", "Hello, {them}."]

; one or the other
winning = opinion > 0 ? "Let us stay friends." : "You are next, {them}."

; the angry lines when they hate it, else the friendly ones
angry = ["Go away.", "You again?"]
tribute_thanks = [opinion < -50% ? angry] ?? ["Thank you!", "Most generous."]

; a list in a list: each of its items counts as one (here a third each)
greeting = ["Hello.", angry]

; now and then
epoch_up = [chance(30%) ? "Progress, finally.", "A new age begins."]

; one situation, told apart by how I stand with them: someone else has won
other_victory = lord == them ? "Long live {them}, my lord!" : allied ? "We won together, {them}!" : "Enjoy it while it lasts."
```

### Situations

<!-- situations: generated by scripts/gen_eeai_readme.py -->
| Situation | When - "you" is the AI | Heard by | Chance | Cooldown | Without a line |
|---|---|---|---|---|---|
| `greeting` | the game begins - a first word to everyone | everyone | 60% |  |  |
| `war_declared` | {them} has just declared war on you | {them} | always | 20 s |  |
| `betrayed` | {them}, your ally until now, has betrayed you and declared war | {them} | always | 20 s | `war_declared` |
| `peace_accept` | you accept {them}'s offer of peace (you are allied now) | {them} | always |  |  |
| `peace_refuse` | you refuse {them}'s offer of peace | {them} | always | 60 s |  |
| `alliance_offer` | you offer {them} an alliance against {enemy}, your common enemy | {them} | always |  |  |
| `offer_accepted` | {them} has accepted your alliance offer | {them} | always |  |  |
| `offer_withdrawn` | your alliance offer to {them} has expired unanswered | {them} | always |  |  |
| `betray` | you betray {them}, your ally, and declare war on them | {them} | always |  |  |
| `tribute_thanks` | {them} sent you a generous tribute | {them} | always | 20 s |  |
| `tribute_small` | {them} sent you a tribute - a small one | {them} | always | 60 s |  |
| `attack` | your army sets out to attack {them} | its allies | 40% | 300 s |  |
| `under_attack` | your town is under attack | its allies | 60% | 240 s |  |
| `building_lost` | you lost a building | its allies | 25% | 180 s |  |
| `epoch_up` | you have just reached the {epoch} age | everyone | 50% | 60 s |  |
| `enemy_ahead` | an enemy is a whole age ahead of you | its allies | 40% | 600 s |  |
| `winning` | you are winning against {them} | {them} | 40% | 600 s |  |
| `losing` | you are losing the war | its allies | 50% | 600 s |  |
| `desperate` | you are close to defeat | its allies | always |  |  |
| `enemy_defeated` | {them} has been defeated | everyone | 80% | 30 s |  |
| `ally_defeated` | your ally {them} has been defeated | its allies | 80% | 30 s |  |
| `defeated` | you have been defeated | everyone | always |  |  |
| `wonder_threat` | a wonder is threatened | everyone | always | 60 s |  |
| `wonder_help` | you send help for an ally's wonder | its allies | always | 60 s |  |
| `wonder_nohelp` | you have no help to give for an ally's wonder | its allies | always | 60 s |  |
| `help_land` | you send land troops to help an ally | its allies | always | 30 s |  |
| `help_air` | you send aircraft to help an ally | its allies | always | 30 s |  |
| `help_naval` | you send ships to help an ally | its allies | always | 30 s |  |
| `help_none` | you have no troops to send an ally who asked for help | its allies | always | 30 s |  |
| `surrender` | you give up the game | everyone | always |  | `defeated` |
| `capitulate` | you capitulate: {them} is beating you, and you offer to PAY them a tribute ({tribute}) for peace - you give, they receive | {them} | always |  |  |
| `capitulate_accepted` | {them} accepted your capitulation: you PAY them the tribute ({tribute}) and serve them from now on | {them} | always |  |  |
| `capitulate_refused` | {them} did not accept your capitulation: the war goes on | {them} | always |  |  |
| `attack_threat` | your army is about to attack {them} | {them} | 60% | 300 s |  |
| `all_submitted` | every other player has submitted to {them}, your lord | {them} | always |  |  |
| `other_victory` | {them} has won the game | everyone | always |  |  |
| `block_refuse` | you refuse {them}'s peace offer because {enemy} in their alliance cannot be at peace with your side | {them} | always | 60 s | `peace_refuse` |
| `block_refuse_own` | you refuse {them}'s peace offer because your own ally {enemy} won't make peace with them | {them} | always | 60 s | `block_refuse` |
| `vassal_grumble` | your lord {them} forces you to make peace with {enemy}, whom you hate | {them} | always | 60 s |  |
| `ask_lord` | {them} offers you peace, but you are the vassal of {enemy} and have no diplomacy of your own - they must talk to {enemy} | {them} | always | 60 s | `peace_refuse` |
<!-- /situations -->

## Expressions

| | |
|---|---|
| Numbers, texts | `85`, `0.5`, `70%` (the same as `0.7` - `%` only right after a number), `"text"`, `'text'` |
| Arithmetic | `+ - * /` on numbers; `+` also joins texts. A text and a number is a mistake: write `"Gold: {me.gold}"` or `"Gold: " + str(me.gold)`. Dividing by 0 is a mistake too; `me.strength * 150%` |
| Comparisons | `< <= > >= == !=` - numbers when both are numbers, else texts (case ignored). A text against a number (`"12" == 12`) is a mistake: logged, and only `!=` holds |
| Logic | `and or not` - only the words (`&&`, `\|\|` and `!` are a mistake; `!=` is a comparison) |
| Choice | `a ? b : c`, `a ? b` (nothing when `a` doesn't hold), `a ?? b` (`b` when `a` is nothing) |
| Lists | `[a, b, c]` - leaving out what is nothing, a list in a list giving its items; as one value (in a line, `+`, `<` ...) one of them at random |
| Functions | `chance(p)` - true with the probability p, 0 to 1: `chance(25%)` or `chance(0.25)` (`chance(25)` is a mistake); `rnd()` - a random number, 0 up to but not 1 (`rnd() < 25%` is `chance(25%)`); `rnd(n)` - 0 up to n; `min(a, b, ...)`, `max(a, b, ...)`, `abs(a)`, `round(a)`, `floor(a)` (down to a whole number: `floor(me.gold / 4)`), `log2(a)` (of a number above 0: `log2(2)` is 1, `log2(0.5)` is -1 - for ratios: `log2(them.strength / me.strength)`) |
| List functions | `len(l)` - how many items (a single value 1, nothing 0); `first(l)` - the first item, not a random one (`first([a ? "x", b ? "y", "z"])`: the first that holds); `item(l, i)` - the i-th, from 1 (nothing past the end); `contains(l, x)` - an item equal to x (as `==`); `map_range(x, [limits], [values])` - the value of the first limit x is below, the last value when it is below none: `map_range(ratio, [0.25, 0.67, 1.5, 4], ["much weaker", "weaker", "about as strong", "stronger", "much stronger"])`. A value that is no list counts as a list of one. |
| Text and number | `str(x)` - x as a text only (`str(epoch)` is "Copper Age", `str(12) + 1` is "121"); `number(x)` - x as a number: a value's number, or a text that is one (`number("12") + 1` is 13), else nothing (`number(t) ?? 0`). Rarely needed - for a text that holds a number |
| Grouping | `( )` |

- **True** is a number other than 0 or a text that isn't empty. **Nothing** is false, an empty text in a line, 0 in
  arithmetic, and a comparison with nothing is false (`war_minutes > 5` and `war_minutes < 5` are both false when
  there is no war).
- **Numbers and texts at once:** some values are both - the epoch is 3 and "Copper Age", an opinion is -62% (-0.62) and
  "hostile" (hostile, unfriendly, indifferent, friendly, trusting: `opinion1`..`opinion5` in `_system.eeai`). The
  number counts in arithmetic, in comparisons with a number and as true/false; the text counts in a line and next to
  texts (`"Age: " + epoch`). Arithmetic gives a plain number: `{round(opinion * 100)}` writes -62.
- **A string is always a text**, even when it looks like a number: `"12" + 1` is `"121"`, and `"12"` is no colour
  number. Write numbers without quotes.
- Only what is chosen is worked out: the other side of `? :`, `??`, `and`, `or` is not (no random draws, no
  errors from text that isn't said).
- Mistakes are never guessed around: an unknown name, a text where a number belongs, a division by 0 leave out the
  list item they are in, else the whole line (see "A name nobody knows" above). There are no loops and no
  assignments: a text can't hang the game.

### [vars]

`[vars]` names strings and expressions for the texts. A personality's own `[vars]` come first, then those of
`_system.eeai`; `_template.eeai` has its own for the prompts. `[vars:de]` replaces them in a German game.

```ini
[vars]
address = opinion > 30% ? "friend" : opinion < -30% ? "enemy" : "neighbour"
title = "my lord {them}"
angry = ["Go away.", "You again?"]
```

Then `{address}`, `{title}` in a line, or `angry` in an expression. A `[vars]` entry is worked out anew wherever it
is used: `"{angry} {angry}"` picks twice, maybe two different items, and `len(angry)` is 2. A function's parameter
is worked out once, when it is called: given a list, it keeps the list (`len(l)` inside) and the item it picked
(`{l}` is the same item every time).

**Functions:** a `[vars]` entry with parameters is worked out with the values it is given:

```ini
[vars]
losses(minutes) = them.kills(me, minutes)
fighting_report(minutes) = "you killed {me.kills(them, minutes, units.army)} of their soldiers and sank {me.kills(them, minutes, units.marine)} of their ships; they killed {them.kills(me, minutes, units.military)} of yours"

[lines]
under_attack = losses(10) > many_dead / 3 ? "{losses(10)} dead in ten minutes. Enough!"
```

- Inside the function its parameters are names like any other; outside it they don't exist. Its other names are the
  game's values and the `[vars]`, as everywhere (a `[vars]` entry it uses doesn't see its parameters).
- It is called with exactly as many values as it has parameters (`losses()` or `losses(1, 2)` is a mistake, found
  when the file is read). Up to 8 parameters; none is fine too: `greeting_line() = ...`.
- A parameter may have the name of a value - `minutes`, `them` - and then means the parameter inside its function.
- A function may call others; one that calls itself stops at the nesting limit (a mistake, never a hang).
- Lookup is as for the other `[vars]`: the personality's own first, then its includes, then `_system.eeai`.

### Shared [vars]: include

`[vars]` that several personalities share go into a file of their own, which each of them names before its first
section:

```ini
include = "_eeaimod_shared.eeai"
[personality]
...
```

or several: `include = ["_eeaimod_shared.eeai", "_knights.eeai"]`. Their `[vars]` (and `[vars:de]` in a German game)
come after the personality's own - a name it defines itself wins - in the order named; its `[decide]` likewise. An
included file holds only `[vars]` and `[decide]` and may include others (4 deep, no circles). Only a file name in this folder: no path.

Start a shared file's name with `_` (no personality is read from it), and give a pack of your own its own shared file
with its own name: packs dropped into this folder then never depend on each other. eeaimod's personalities share
`_eeaimod_shared.eeai`: `this_war`, `many_dead`, `big_tribute`, and the `[decide]` rules.

## The game's values

A line is said by one AI - **me** - to or about another player - **them**. Some lines also involve a
third player - **enemy** (`{enemy}`): the enemy in an alliance offer, the ally or lord in some lines. **lord** is
my lord - the player I capitulated to - in every line, and nothing while I have none. In `_system.eeai` `them` is
you, the local player.

Each of them is a player: in a text its name (`{me}`, `{them}`, `{enemy}`, `{lord}`), in a
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
greeting = [lord and lord.soldiers > 50 ? "My lord's army is mighty, {them}."] ?? "Greetings, {them}."
tribute_thanks = "{lord ?? me} thanks you, {them}."
```

A condition (`lord ? ...`, `lord and ...`, `not lord`), `??` and a comparison (`lord == them`) check it. Used without
a check - in a text (`{lord}`), a sum, a member (`lord.soldiers`) or a relation (`opinion(lord)`) - it is a
mistake: a line that names a lord nobody has makes no sense, so it is left out (the list item, else the line).

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
| `X.walls` | its wall pieces |
| `X.centers` | its Town Centers and Capitols |
| `X.strength` | its fighting strength (its soldiers' hit points, 30 a building) - to compare: them.strength &gt; me.strength * 2 |
| `X.rank` | its place by strength among the players still in, 1 = the strongest |
| `X.epoch` | its epoch: the name in a text, 1 (Prehistoric) .. 15 (Space) as a number - compare it with epoch.middle etc. |
| `X.color` | its colour: the name in a text (in the game's language), 1 (blue) .. 17 as a number - compare it with color.red etc. |
| `X.civilization` | its civilization's name, as the game shows it (Karthago in a German game; an AI's personality may give it another: civilization=) - nothing when none is chosen |
| `X.civilization_en` | its civilization in the game's database, in English in every language (Carthage) - for conditions: them.civilization_en == "Carthage" |
| `X.alive` | 1 while it is still in the game |
| `X.human` | 1 for the human player at this PC |
| `X.vassals` | how many have capitulated to it |
| `X.allies` | how many are allied with it (both ways), still in - allies alone: mine |
| `X.enemies` | how many it treats as enemies, still in - enemies alone: mine |
| `X.aggression`, `X.loyalty`, `X.pride`, `X.greed`, `X.forgiveness`, `X.caution`, `X.talk` | its traits, 0% to 100% (50% without a personality) - a number: `{aggression}` writes 0.6, `{aggression:%}` 60%; `chance(aggression)`: as likely as it is aggressive |
| `X.food`, `X.wood`, `X.stone`, `X.gold`, `X.iron` | its stock - another's too (the AI's spies): them.gold |
| `X.stock` | the five resources together - to scale amounts: them.tribute(me) &gt; me.stock / 2 |
| `X.lord` | its lord, the player it capitulated to - a player: them.lord.name, lord == them; nothing when it has none |

**Relations** of player A with player B - `me.opinion(them)`, `them.opinion(me)`, `enemy.at_war(them)`; without `A.` mine (`opinion(enemy)`). Those that count events - `tribute`, `refusals`, `betrayals`, `kills`, `razed` - take a time window as a second argument: `them.tribute(me, 15)` counts the last 15 minutes only (`16.5` too), without it the whole game:

| Relation | |
|---|---|
| `A.opinion(B)` | -100% to 100%: what A thinks of B, 0 indifferent - in a text in words: hostile, unfriendly, indifferent, friendly, trusting (round(opinion * 100): the number). An AI's opinion; a human's of an AI is a guess from what the AI did to them (war, a betrayal, peace, tribute, a refusal, their losses), nothing of another human |
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
| `A.in_town(B)` | 1 when A's army is in B's town: its soldiers have stayed near B's base for a while (an AI's base) |
| `A.attack_minutes(B)` | minutes since A's last attack wave set out against B - nothing if none (an AI's waves) |
| `A.kills(B)` | B's units A has destroyed - (B, minutes): in the last minutes (whole minutes, at most 60); a last argument picks which: units.all (the default), units.army (on land: also priests, heroes, siege), units.airforce, units.marine, units.military (the three), units.civilian (citizens, fishing boats) - them.kills(me, units.marine), me.kills(them, 10, units.military) |
| `A.razed(B)` | B's buildings A has destroyed - (B, minutes) as kills |

**The game:**

| Name | |
|---|---|
| `me` | the AI that speaks - its name in a text; me.soldiers ... |
| `them` | the player it speaks to or about; in a system line you, the local player |
| `enemy` | a third player: the enemy in an alliance offer, the ally or lord in some lines |
| `lord` | my lord (me.lord), the player I capitulated to - in every line; nothing when I have none |
| `opinion` | me.opinion(them): what I think of them |
| `at_war` | me.at_war(them) |
| `allied` | me.allied(them) |
| `minutes` | the game time in minutes |
| `players` | the players still in, me among them |
| `population_limit` | the game's unit limit per player (its setting) |
| `max_population` | the largest population of a player still in - kills &gt; max_population / 2: many for this stage of the game |
| `language` | the game's language: de, en, fr, ... |
| `offer_minutes` | how long an offer stands, in minutes ([all] offer_minutes in aimod.ini) |
| `llm_equal` | 1 when the language model is told nothing of who plays the computer ([llm] equal) |
| `llm_constrained` | 1 when the language model decides within the rules' limits ([llm] constrained) |
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
epoch_up = [epoch >= epoch.middle ? "Steel and stone!", "Another age, {them}."]
```

A player's colour works the same way: `them.color` is its name in a line (in the game's language - "dunkelrot" in a
German game) and a number in a comparison. Compare it with `color.blue`, `color.red`, `color.yellow`,
`color.magenta`, `color.orange`, `color.green`, `color.cyan`, `color.darkred`, `color.darkgrey`, `color.purple`,
`color.grey`, `color.silver`, `color.lightyellow`, `color.lime`, `color.lightblue`, `color.pink`, `color.darkgrey2`.
`them.civilization` is the name of its civilization as the game shows it - a text only, in the game's language.

```ini
greeting = them.color == color.red ? "Red banners, {them}? Mine are redder." : "Greetings, {them} of {them.civilization}."
```

Which units `kills` counts - its last argument, after the minutes if any:

| Name | Counts |
|---|---|
| `units.all` | every unit (the default) |
| `units.army` | on land - also priests, heroes, siege |
| `units.airforce` | aircraft, helicopters, balloons |
| `units.marine` | warships, submarines, transports, carriers |
| `units.military` | army, airforce and marine |
| `units.civilian` | citizens and fishing boats |

```ini
sea_war = them.kills(me, 10, units.marine) > 3 ? "Leave my ships alone, {them}!"
```

## Placeholders

In a line, `{...}` holds any expression: `{them}`, `{me}`, `{enemy}`, `{epoch}`, `{opinion}`, a `[vars]` name,
`{them.soldiers}`, `{round(me.strength / 100)}`. Personality lines also have one placeholder, `{tribute}`: what a
capitulation pays, by resource ("200 gold, 800 iron").

A number is written as it is, with up to 2 decimals: `{0.625}` writes 0.63 (a half rounds away from 0). How else, a
format after a `:` says - the last thing in the braces:

| | 0.625 | 1234.5 |
|---|---|---|
| `{x}` | 0.63 | 1234.5 |
| `{x:%}` - as a share | 63% | |
| `{x:.1%}` - a share with 1 decimal | 62.5% | |
| `{x:.0}`, `{x:.2}` - with that many decimals (0 to 6) | 1, 0.63 | 1235, 1234.50 |

`50%` in an expression is only another way to write 0.5 - a value never remembers how it was written: `{aggression}`
writes 0.6, `{aggression:%}` 60%. A choice in front of a format goes in parentheses - `{(opinion > 0 ? opinion : 0):%}`
- since `{opinion > 0 ? opinion : 0:%}` reads as if only the 0 were formatted (the same for `??`); without them it is
a mistake. A format on a text or a player, or one that is no format, is a mistake too.

`_system.eeai` and `_template.eeai` use the same expressions - `{me}` is the AI, `{them}` the one it talks to (in
`_system.eeai` you), `{them.epoch}`, `{me.attack_minutes(them)}` - and add a few texts only the mod has: `{stance}`,
`{terms}`, `{tribute}`, `{ally}` ... Each key's are listed above it in the file. None of them has the name of a
value: `{epoch}` is always the speaker's epoch, wherever it stands.

## [personality] and [style]

| Key | Value |
|---|---|
| `name`, `name:de`, ... | The player's name in the game (`name:de` in a German game) |
| `civilization`, `civilization:de`, ... | The name of its civilization as the game shows it, instead of the one it plays (that one stays in `X.civilization_en`) |
| `color` | Favoured colours on random maps, the first free one wins: `["blue", "cyan"]` - blue, red, yellow, magenta, orange, green, cyan, darkred, darkgrey, purple, grey, silver, lightyellow, lime, lightblue, pink (or the German names, or a number 1..17 as `them.color` has it: `color.red` or `2`, not `"2"`). An expression like any other, worked out when the game starts - before there are players, so only constants, `chance()`, `rnd()`, `language` and `[vars]`: `chance(50%) ? ["red", "orange"] : ["grey"]`, `language == "de" ? color.yellow : color.blue` |
| `aggression` | 0% to 100% - written with the %: `aggression = 60%` (50% when left out): betrays, refuses peace |
| `loyalty` | keeps its alliances; a vassal under 50% rebels against its lord once it has served 10 minutes and is clearly stronger than its lord's side (1 + 2 x (loyalty + 100% - pride) times: loyalty 35% and pride 80% 2.1 times) - with or without the language model |
| `pride` | how long it holds a grudge and refuses peace after a declaration of war (pride x 5 minutes: 60% 3 minutes), how long it fights on before it capitulates, how soon it rebels as a vassal |
| `greed` | what a tribute is worth to it |
| `forgiveness` | how fast a grudge fades |
| `caution` | capitulates earlier, pays more |
| `talk` | how often it says the lines that are not always said |
| `character` | For the AI voice: who this is, in a few words - the model plays it |

`[style]` is **experimental** and only read with `ai_style=1` in aimod.ini (off by default: every AI plays the same,
tested way). Its keys are aimod.ini settings that replace the difficulty's values for this AI. Each value is an
expression worked out once when the game starts, like `color` (constants, `chance()`, `rnd()`, `language`, `[vars]`):
shares and chances with `%` - `home_guard_share = 35%`, `attack_on_contact = 20%`, `attack_group_percent = 150%` (aimod.ini
writes these as whole percents: 35 is 0.35 there, 20, 150) - minutes and epochs as numbers: `first_attack_min = 15`,
`first_attack_max = chance(50%) ? 20 : 30`. A value out of its range (`attack_on_contact = 20` would be 2000%) is a
mistake: that key keeps the difficulty's value.

<!-- style: generated by scripts/gen_eeai_readme.py -->
`military_start_share`, `military_income_share`, `military_income_share_farms`, `epoch_min`, `epoch_max`, `first_attack_min`, `first_attack_max`, `attack_interval_min`, `attack_interval_max`, `attack_group_percent`, `attack_on_contact`, `attack_for_resources`, `attack_revenge`, `citizen_share`, `home_guard_share`
<!-- /style -->

## [decide]: what the AI wants

The AI's diplomacy decides by numbers worked out from expressions - the same ones as in its lines, with `me` the AI,
`them` the other player and `enemy` its strongest enemy (there may be none: check it, `enemy and ...`). eeaimod's
are in `_eeaimod_shared.eeai`; a personality that writes one of them in its own `[decide]` decides that its own way:

| Key | When | The AI ... |
|---|---|---|
| `peace_score` | someone offers peace, or an alliance is weighed (by either side) | accepts when it is above 0 (opinion is -100%..100%: a score on that scale) |
| `seek_ally` | every minute from minute 10, at war with a strongest enemy | looks for an ally against it when true |
| `ally_score` | for each of that enemy's other enemies (`them`) | asks the one with the highest score, if above -20% |

```ini
[decide]
; a fox: an alliance as soon as anyone gets ahead - and with the strongest when it can
seek_ally = enemy and enemy.strength > me.strength
ally_score = them.rank == 1 ? 100% : opinion
```

What stays the mod's: who can be asked at all (not someone whose army is in its town, not twice in a row too soon),
how long an offer stands, how it is answered - and no peace within pride x 5 minutes of a declaration of war. A
mistake in one (or a value that is no number) is logged once and the mod's built-in rule decides instead: the same
as the shipped expressions. `chance()` and random choices here draw from the game's own generator, so every PC of a
multiplayer game decides alike. Every PC must have the same files then.

## _system.eeai and _template.eeai

`_system.eeai` holds the lines the mod shows you itself: `[system]` in English, `[system:de]` (or another language)
replaces what it has in a game of that language. `_template.eeai` holds everything the language model is told, in
English (the model is told which language to answer in); its pieces are joined with a space. Both files list every
key with its built-in text and its placeholders. `[vars]` in `_system.eeai` serve the system lines and every
personality's lines; `[vars]` in `_template.eeai` serve the prompts only.

`_template.eeai` comes with `[vars]` of its own: the pieces of `game_state_other`, what the AI is told about the one it
talks to - `stance`, `power` (from `strength_ratio`), `town`, `attacks`, `fighting` and `fighting_recent` (with the
functions `destroyed(by, of, minutes)` and `fought(minutes)`, `recent_minutes = 20`) and `bond` (lord or vassal).
Change one by writing it in the file's `[vars]`; take one out of `game_state_other` by leaving its `{name}` out. A
function's parameter can be a player: `destroyed(me, them, 0)` uses `by.kills(of, ...)`. `texts dump force` writes
the built-in ones again and keeps your own `[vars]` below them.

## Trying things out

The control pipe (for developers, see `docs/testing.md` in the sources) has `texts eval P Q <expression>` - what an
expression gives with player P speaking to Q in the running game - and `texts line P <situation> [Q]` - the line P
would say now.
