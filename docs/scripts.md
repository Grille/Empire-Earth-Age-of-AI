# Scripts (test scenarios)

A test run's `setup=<file>` (autotest, `run_test.ps1 -IniExtra "setup=scenarios/bombing.eeai"` - relative to the repository) is a script in
the text language of the personalities ([config/aimod_ai/README.md](../config/aimod_ai/README.md)): the same
expressions, plus statements, variables, functions and the pipe's commands. It runs from 2 s game time, top to
bottom; waiting (`wait`, `wait_until`, `every`) lets the game go on until that game time. The runner: `dll/test/scripts.c`.

```
# Bombing: an airport at minute 19.5, fighters and bombers at 20, every 20 s they attack P3's town
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

One a line; `#` starts a comment.

| Statement | |
|---|---|
| `let name = value` | a new variable (a name of letters, digits, `_`) - once in its block; known from there to the block's end |
| `name = value` | a variable a `let` made gets a new value; `name += value`, `-=`, `*=`, `/=`: `name = name + value` ... |
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
while player(3).towers > 0 {       # every minute until P3's towers are gone, at most 10 times
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
| `(a, b) => value`, `(i) => { ... }`, `() => ...` | a function: one value after `=>`, or statements in `{ }` with `return` |
| `player(3)` | player 3 - `let p3 = player(3)` then `p3.towers`, `p3.soldiers` ... (the values of `them.` in the README) |
| `vec(70, 50)` | a vector (2 to 4 numbers - a place in tiles): `let home = vec(70, 50)` then `home.x`, `step(home).x`, `home + vec(5, 0)`, `home * 2`, `distance(home, there)` (the README's "Vectors"); as a command's argument its numbers one each: `spawn(2, unit.inf02_spearman, home, 10)` is `spawn 2 24 70 50 10` |
| `{ x = 70, y = 50, units = 10 }` | a record (the README's "Records"): `let wave = { at = vec(70, 50), units = 10 }` then `wave.at`, `wave.units`, `has(wave, "units")`; a comma after each member |
| `time` | the game time now, in seconds |
| `stop` | what an `every` body returns to end it early |
| `"text \{x\}"` | `\{` `\}`: braces that are text, not a placeholder (also `\"` `\\` `\n`) |

## Functions

| Function | |
|---|---|
| `wait(10s)` | the script waits that long (game time) |
| `wait_until(t)` | the script waits until game time `t` (not at all if it is past) |
| `every(step, from, until, f)` | `f(run)` at `from`, `from + step` ... up to `until` (inclusive) - `run` 1, 2, 3 ...; `return stop` ends it |
| `player(n)` | player n as a value |
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
let row = (y) => spawn(3, unit.b_guard_tower_bronze, 51, y)
for i in 1..3 { row(40 + i * 3) }         # towers at 51,43 51,46 51,49
let checks = 0
every(10s, 50s, 2min, (run) => {
    objs(3, unit.b_guard_tower_bronze)
    checks += 1                            # the script's own: 3 after it
    return run == 3 ? stop                 # three times, then on
})
```

## Triggers

What the scenario editor's triggers do, in the language - checked every 500 ms of game time like the game's own,
also while the script waits and after it has ended:

```
let p2 = player(2)
let t = trigger({
    name = "wood for the poor",          # for the messages (else the statement's start)
    when = () => p2.wood < 100 and time > 10min,
    for = 5s,                            # true that long without a break first (default 0)
    loop = false,                        # fires once, then it is off (true: again whenever it holds)
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

## Several scripts side by side

`setup=a.eeai;b.eeai` (`;` - `run_test.ps1` splits its ini arguments at commas): each runs on its own, in turn, always
in the order given. A script waits only for itself: two things that repeat at different steps (a fight measured every
3 s, orders every 12 s) are two scripts (`scripts/duels.sh`).

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
