# Scripts (test scenarios)

A test run's `setup=<file>` (autotest, `run_test.ps1 -IniExtra "setup=tests/scenarios/bombing.eeai"` - relative to the repository) is a script in
the text language of the personalities ([config/aimod_ai/README.md](../config/aimod_ai/README.md)): the same
expressions, plus statements, variables, functions and the pipe's commands. It runs from 2 s game time, top to
bottom; waiting (`wait`, `wait_until`, `every`) lets the game go on until that game time. The runner: `dll/test/scripts.c`.

```
// Bombing: an airport at minute 19.5, fighters and bombers at 20, every 20 s they attack P3's town
let fighter = unit.a_strafe_fighter10_fokker_dr_1
defence(2, 0)
wait_until(19.5min)
spawn(2, unit.b_airport, 92, 56, 1)
wait_until(20min)
spawn(2, fighter, 92, 60, 25)
spawn(2, unit.a_bomber_11_b_17, 94, 62, 4)
every(0.33min, 20.10min, 33.96min, (run) => {
    stats("engage", 2, 92, 58, 60, fighter, 40, 40)
    stats("engage", 2, 92, 58, 60, unit.a_bomber_11_b_17, 40, 40)
})
```

## Statements

One a line; `//` starts a comment, as in the game's own scripts (.rmv, .tai) - a `#` is a mistake.

| Statement | |
|---|---|
| `let name = value` | a new variable (a name of letters, digits, `_`) - once in its block; known from there to the block's end; `let name: float = value` with its type ("Types") |
| `name = value` | a variable a `let` made gets a new value; `name += value`, `-=`, `*=`, `/=`: `name = name + value` ... |
| `p.gold = value`, `p.gold += value` | an object's member that may be set ("Objects") |
| `call(...)`, any expression | worked out, its value not kept |
| `if c { ... } else if d { ... } else { ... }` | the first block whose condition holds (`else` on the `}`'s line or the next) |
| `while c { ... }` | the block again and again while `c` holds |
| `for i in 1..5 { ... }` | the block for every whole number from 1 to 5 (both in; none when the second is smaller); `for p in [2, 3, 5]`: for each item of a list |
| `break`, `continue` | in a loop: out of it, or on to its next round |
| `return value`, `return` | in a function's `{ }` body: what it hands back (nothing without one) |

A line may go on while a `(`, `[` or `{` is open; a block's `{` is on its statement's line. A condition is true as in
an expression (a number not 0, a text not empty, a list, a player). A `let` in a block (an `if`'s, a loop's) is that
block's own; a name is never made twice where it is seen - a `let` or a function's parameter of a name of the code
around, of the game (`epoch`) or of the script's functions (`wait`) is a mistake (a pipe command's name may be a
variable: `let wave = {...}`).

A loop that never waits holds the game: a script runs on the game's thread, and the game only goes on while it waits.
A million rounds of its loops without a `wait` (counted, not timed - every PC stops at the same one) and the script
stops: `a loop (line 12) ran 1000000 rounds without waiting`.

```
let row = 40
while row < 50 {
    spawn(3, unit.b_guard_tower_bronze, 51, row)
    row += 3
}
for p in 2..5 {
    for t in 2..5 {
        if p == t { continue }
        calloff(p, t)
    }
}
let waves = 0
while player(3).towers > 0 {       // every minute until P3's towers are gone, at most 10 times
    wait(1min)
    waves += 1
    if waves > 10 { break }
    spawn(2, unit.inf02_spearman, 70, 50, 10)
}
```

## Values

Everything the personalities' expressions have (numbers, `70%`, texts with `{placeholders}`, lists, `a ? b : c`,
`a ?? b`, `and or not`, `min`, `chance` ...), and:

| Value | |
|---|---|
| `20s`, `19.5min` | a time - a number of seconds (`19.5min` is 1170, as `70%` is 0.7) |
| `unit.b_guard_tower_bronze` | a unit or building type - its number and its identifier ([units.md](units.md)) |
| `kind.towers` | one of the game's families - every type of it, whatever the epoch ([units.md](units.md)): `p3.count(kind.towers)` |
| `resource.gold`, `resource.all` | a resource: food, wood, stone, gold, iron (1..5), all (0) - `give(2, 500, resource.gold)` |
| `(a: int, b: float) => value`, `(i: int) => { ... }`, `() => ...` | a function: one value after `=>`, or statements in `{ }` with `return` - its parameters with their types ("Types") |
| `player(3)`, `game` | objects - "Objects" below |
| `l[2]` | a list's item, from 1 (`l[0]`, one past its end: a mistake) |
| `vec(70, 50)` | a vector (2 to 4 numbers - a place in tiles): `let home = vec(70, 50)` then `home.x`, `step(home).x`, `home + vec(5, 0)`, `home * 2`, `distance(home, there)` (the README's "Vectors"); as a command's argument its numbers one each: `spawn(2, unit.inf02_spearman, home, 10)` is `spawn 2 24 70 50 10` |
| `{ x = 70, y = 50, units = 10 }` | a record (the README's "Records"): `let wave = { at = vec(70, 50), units = 10 }` then `wave.at`, `wave.units`, `has(wave, "units")`; a comma after each member |
| `time` | the game time now, in seconds |
| `stop` | what an `every` body returns to end it early |
| `"text \{x\}"` | `\{` `\}`: braces that are text, not a placeholder (also `\"` `\\` `\n`) |

## Types

Every value has a type the check works out when the file is read - nothing to write for it: a variable has its
`let`'s. `int` (`5`, `p.gold`, `game.minutes`), `float` (`5.0`, `.72`, `70%`, `20s`, `game.time`, an opinion), `bool`
(`true`, `a > b`, `p.alive`), `text`, `vec2` .. `vec4`, `epoch`, `color`, `unit type` (`unit.x`, `kind.x`), lists of
one of them, records, functions, triggers, and the game's objects (player, game, slot).

- An int goes where a float belongs, never the other way: `let n = 5` then `n = n * 1.5` is a mistake - `int(...)`
  (toward 0), `round`, `floor`, `ceil` make an int. `/` always gives a float (`7 / 2` is 3.5).
- `+ - * /` take numbers (and vectors): `epoch + 1`, `"a" * 2`, `p.name - 1` are mistakes.
- Compared are values of one type - an int with a float too: `p.epoch >= epoch.middle`, but `p.epoch == 5` is a
  mistake (`int(p.epoch) == 5`).
- `p.gold = 10.5` (a float in an int's place), `l[1.5]`, `v.z` of a `vec2`, `t.foo` of a trigger: mistakes.
- What may be none has a type with `?`: `p.lord` (a `player?`), `p.civilization`, `p.war_minutes`, `first(l)`,
  `item(l, i)`, `number(t)`. It is checked before it is used - `p.lord != null` (or `== null`), then a member, a sum,
  a place of a plain type (`let q: player = ...`); `??` gives a value instead (`p.war_minutes ?? 0`). The check holds
  where it guards: in `if p.lord != null { ... }` (and `else` of `== null`), right of `and` / `or`, in `?:`, and below
  an `if p.lord == null { return }` (also `break`, `continue`). Not in a function made there - it runs later - and not
  after the name is set anew. A first(l).name of what may be null goes into a variable first (`let f = first(l)`).
  `null` is "none there": `let q: player? = null`; it is compared only with `==` and `!=`, and only with what may be
  none (`p != null` of a plain `player` is a mistake).

Written where it can't be told - a function's parameters: `let f = (pos: vec3, id: int) => ...`. A function handed to
`every` has its `run` from there (`(run) => ...` is an int; `(run: text)` a mistake), the others take none. A `let`
may say its own: `let n: float = 5` (then `n = n * 1.5` fits), `let names: [text] = []`, `let l: player? = p.lord`.
The types by name: `int`, `float`, `bool`, `text`, `vec2` .. `vec4`, `epoch`, `color`, `unit_type`, `function`,
`record`, `trigger`, `player`, `game`, `slot`; `[T]` a list of them, `T?` one or null. A call of a function a `let`
made is checked against its parameters: their count and types (`f(2.5)` for an `x: int` is a mistake).

Where a type still can't be told (a record's member, what a function gives back), the value is checked when it is
worked out, as before.

## Objects

The game's things are objects, each of a kind: a value (`p.towers`) or a method (`p.opinion(q)`) by name. Its text
is its name (`"{p}"`), it equals only itself (`game.players[2] == player(2)`).

| Kind | |
|---|---|
| `game` | the game: `game.time` (seconds), `game.minutes`, `game.players` (every player, in or out - `game.players[3]` is player 3), `game.player(3)`, `game.timer(20s, f)` ("Timers" below), `game.setup(f)`, `game.slots` ("Before the game starts") |
| slot | a player to be, in `game.setup`: `s.number`, `s.human`, `s.persona`, `s.color` (the last two set) |
| player | `player(3)`, `game.player(3)`, `game.players[3]`, `me`/`them` in the texts: every value of `them.` in the README (`p.towers`, `p.epoch`, `p.lord` - a player again), its relations as methods (`p.opinion(q)`, `p.kills(q, 10)`), `p.count(kind.towers)`; as a command's argument its number (`spawn(p, ...)`). What it does below |

```
let p2 = game.player(2)
for p in game.players {
    if p.alive and p != p2 and p2.at_war(p) {
        ping("{p2} fights {p}: {p.soldiers} soldiers, {p2.kills(p, 10)} killed in 10 minutes")
    }
}
```

The check knows what a variable is from its `let` (`let p = player(2)`, a `for` over `game.players`, `game.players[1]`):
a member its kind hasn't (`p.towerz`), a method without its `( )` or with the wrong number of arguments, a member of
a list (`game.players.towers`), another kind put into it (`p = game`) - each a mistake when the file is read, with
the members it has. A function's parameter is of no kind known: its members are checked when it runs.

A player does things - a script's, a text changes nothing:

| | |
|---|---|
| `p.gold = 500`, `p.food += 100` | its stock (`food`, `wood`, `stone`, `gold`, `iron`): a whole number, 0 or more (`round(p.wood * 1.5)`) - written directly, as a scenario's Set-Attribute does |
| `p.war(q)`, `p.ally(q)` | p treats q as an enemy / an ally - one way, as the game's stances are (q's towards p stays: `q.war(p)` too for a war of both); an AI may change its mind later |
| `p.win()`, `p.lose()` | as a scenario's Victory / Defeat, at the end of the game's step: a win is the game's end, the others lose |

Each is logged (`# setup: player 2.gold = 500`); in a dry run it is only written to `<out>`, as a command is. A
player out of the game, a stock of `10.5` or `-5`, `p.war(p)` stop the script; `p.name = ...` (not to be set) is a
mistake when the file is read.

## Functions

| Function | |
|---|---|
| `wait(10s)` | the script waits that long (game time) |
| `wait_until(t)` | the script waits until game time `t` (not at all if it is past) |
| `every(step, from, until, f)` | `f(run)` at `from`, `from + step` ... up to `until` (inclusive) - `run` 1, 2, 3 ...; `return stop` ends it |
| `player(n)` | player n as a value (as `game.player(n)`) |
| `cmd("texts eval 3 1 them.kills(me)")` | a command as written (for a text with spaces, quotes or braces) |
| any pipe command | `spawn(2, unit.inf02_spearman, 70, 50, 10)`, `stats("engage", ...)`, `count(3)`, `save("zz_name")`: the command `spawn 2 24 70 50 10` - numbers as numbers, a type its number, a player its number, a text as it is. Its value is the reply's first line; a reply `error ...` is a mistake. Every command is logged `# setup: <command> -> <reply>` |

The commands: [testing.md](testing.md) "Control pipe" (`dll/test/cmd_verbs.h`, generated from control.c, lists those a
script may call).

## Functions and their names

A function sees its parameters, its own `let`s, and the names of the code around where it was made - those it reads
and sets (`n += 1` in a function changes the `n` around it: the code around sees the new value). Everything runs in
order, one thing after another: a function called later still finds the names around it as they are then - also a
`let` further down (a function made before it may call it). A function can be passed on (`every(10s, 0, 1min, row)`),
not handed back out of the function it was made in (nor put into a variable of the code around).

```
let row = (y: int) => spawn(3, unit.b_guard_tower_bronze, 51, y)
for i in 1..3 { row(40 + i * 3) }         // towers at 51,43 51,46 51,49
let checks = 0
every(10s, 50s, 2min, (run) => {
    objs(3, unit.b_guard_tower_bronze)
    checks += 1                            // the script's own: 3 after it
    return run == 3 ? stop                 // three times, then on
})
```

## Triggers

What the scenario editor's triggers do, in the language - checked every 500 ms of game time like the game's own,
also while the script waits and after it has ended:

```
let p2 = player(2)
let t = trigger({
    name = "wood for the poor",          // for the messages (else the statement's start)
    when = () => p2.wood < 100 and time > 10min,
    for = 5s,                            // true that long without a break first (default 0)
    loop = false,                        // fires once, then it is off (true: again whenever it holds)
    run = () => {
        give(2, 500, resource.wood)
        after(10s, () => chat(2, "Don't waste it."))
    },
})
let other = trigger({ when = () => t.fired, run = () => disable(t) })
```

| | |
|---|---|
| `trigger({ ... })` | `when` and `run` are functions of no arguments; `loop`, `for`, `name` may be left out. Its value is the trigger: `t.on`, `t.fired`, `t.name` |
| `enable(t)`, `disable(t)` | switch it on (its `for` starts again) or off |
| `after(10s, f)` | `f()` that much later - a trigger's `run` can't `wait` (that would hold up every trigger), its delays are `after`'s |

The triggers are checked in the order they were made, each `run` right after its `when` held, so a trigger made
after `t` sees `t.fired` in the same check. A mistake in a trigger's function turns that trigger off (logged
`# texts: trigger <name>: when stopped - ...`); the script and the other triggers go on. The functions read and set
the script's own names as they are when they run; the names of a function they were made in are a copy made with
the trigger (that function has returned): as they were then, set in the copy - the trigger's own from then on. A dry run (`script dryrun`) has the triggers' turns on the way to where the script waits:
end a trigger test with `wait_until(...)`.

## Timers

`game.timer(step, f)` runs `f()` every `step` of game time (0.5s or more) from now + `step` on, while the script goes
on (it doesn't wait) - also after it has ended. `f` returning `stop` ends it; its value is a trigger's: `t.on`,
`t.fired`, `disable(t)`, `enable(t)` (again a step from then). A timer runs at its time - at the next of the
simulation's safe points, about every 0.5 s - in the order of their times together with the `after`'s, not on the
triggers' 500 ms beat; one that fell behind runs once for every step it missed.

```
let counter = 0
game.timer(20s, () => {
    counter += 1
    if counter >= 10 {
        return stop                    // ten times, then no more
    }
    spawn(2, unit.inf02_spearman, 70, 50, counter)
})
```

`every(step, from, until, f)` is the waiting form: the script itself goes from one run to the next.

## Before the game starts

`game.setup(() => { ... })`, a statement at the script's top, runs before the game starts - when the game makes its
players, before the personalities and colours are planned - in a new game only (a saved game's ran when it began).
The rest of the script runs from 2 s as always; the setup's statement is nothing there. Its function sees none of the
script's names (they don't exist yet) - only its own `let`s, and the players to be:

| | |
|---|---|
| `game.slots`, `game.slot(2)` | the players to be by number (the human and computer slots in order) - only here; `game.players` after |
| `s.number`, `s.human` | the player it becomes; 1: a human's |
| `s.persona = persona.fuchs` | a computer's personality: `persona.<its file>` (aimod_ai\fuchs.eeai) - an unknown one a mistake when the file is read |
| `s.color = color.red`, `= rgb(40, 200, 120)` | its colour: one of the game's, or (a computer's) its own |

```
game.setup(() => {
    game.slot(2).persona = persona.fuchs
    game.slot(3).color = rgb(40, 200, 120)
    for s in game.slots {
        if not s.human and s.number > 3 {
            s.persona = persona.haendler
        }
    }
})
```

A mistake there (a human's personality, a vanilla player's, a scenario game - personalities are planned in random-map
games) ends that script before the game starts: `# setup: <file>: game.setup stopped - ...`. Test runs read their
scripts at the game's start now (setup=...); a dry run leaves the setup out.

## Several scripts side by side

`setup=a.eeai;b.eeai` (`;` - `run_test.ps1` splits its ini arguments at commas): each runs on its own, in turn, always
in the order given. A script waits only for itself: two things that repeat at different steps (a fight measured every
3 s, orders every 12 s) are two scripts (`scripts/test/duels.sh`).

## Mistakes

A script is checked when it is read: its syntax, a name or a function nobody has, a function's number of arguments, a
variable of the code around changed in a function - in its texts' `{placeholders}` too, which see the same names as
the code around them (`"{row(3)}"` calls the script's function `row`). Then it doesn't run at all:
`# setup: <file>:<line>: <what> - not run`. A mistake while it runs (a command's error, `"a" * 2`, a player that isn't
there) ends it: `# setup: <file>:<line>: stopped - <what>`. A script that ends logs `# setup: <file>: done`.

## A dry run

`script dryrun <file> <out>` (a pipe command, or `script("dryrun", ...)` in a script) runs a script without the game:
waiting moves the clock (from 2 s), every command goes into `<out>` as `<ms> <command>` instead of being run (its reply
is "ok"). The setup files of before 2026-10-20 (`@<minutes> <command>` lines) were converted this way: every converted
file issued the same 905 commands at the same times (to 1 ms) as the old runner would have.
