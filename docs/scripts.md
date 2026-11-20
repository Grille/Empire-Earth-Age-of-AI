# Scripts (test scenarios)

A test run's `setup=<file>` (autotest, `run_test.py -IniExtra "setup=tests/scenarios/bombing.eeai"` - relative to the repository) is a script in
the text language of the personalities ([config/personalities/README.md](../config/personalities/README.md)): the same
expressions, plus statements, variables, functions and the pipe's commands. It runs from the game's first step (until 2026-11-18:
from 2 s), top to bottom; waiting (`wait`, `wait_until`, `every`) lets the game go on until that game time. The runner: `dll/test/scripts.c`.
A script is compiled to instructions when it is read and run by a machine of its own (`dll/script/expr.c` SCRIPTS
COMPILED): its turns come between the simulation's steps, where the pipe's commands run - a spawned unit is there at
once - and a wait is that machine stopping where it is. The texts (a personality's lines, names, functions and
handlers, the prompts, the settings' numbers) run on the same engine since 2026-11-15: each source compiled when it is
first worked out, all of them on one machine of their own that never waits - there is no second evaluator.

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
| `a.x = value`, `l[2] = value`, `l[2].x += value` | a struct's field, a list's item - set in the place it is in ("Structs") |
| `struct name { ... }` | a type of the script's own, at its top only ("Structs") |
| `ref q = l[2]` | a second name for a place ("Refs") |
| `call(...)`, any expression | worked out, its value not kept |
| `if c { ... } else if d { ... } else { ... }` | the first block whose condition holds (`else` on the `}`'s line or the next) |
| `while c { ... }` | the block again and again while `c` holds |
| `for i in 1..5 { ... }` | the block for every whole number from 1 to 5 (both in; none when the second is smaller); `for p in [2, 3, 5]`: for each item of a list |
| `break`, `continue` | in a loop: out of it, or on to its next round |
| `return value`, `return` | in a function's `{ }` body: what it hands back (nothing without one) |
| `name(a: int) => value`, `name(a: int): float { ... }` | a function declared, at the script's top only: known in the whole file (used above where it is written too), never set anew; what it gives back written after its parameters ("Functions and their names") |

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
| `1920x1080` | a size: `vec(1920, 1080)` |
| `vec(70, 50)` | a vector (2 to 4 numbers - a place in tiles): `let home = vec(70, 50)` then `home.x`, `step(home).x`, `home + vec(5, 0)`, `home * 2`, `distance(home, there)` (the README's "Vectors"); as a command's argument its numbers one each: `spawn(2, unit.inf02_spearman, home, 10)` is `spawn 2 24 70 50 10` |
| `dict<player, int>(0)` | a dict: an entry per key, the default (here 0) for every key without one - `let kills = dict<player, int>(0)`, `kills[p] += 1`, `kills[p]` (never missing: the default). Keys: `int`, `text` or an object (`player` ...); values: any type (`dict<text, point>`, `dict<player, seq<int>>` - "Dicts"). A dict is one table: `let k = kills` is the same one, `k[p] = 5` is seen in `kills` too. (A seq is a value, a list is shared as a dict is - "Sequences and lists") |
| `{ x = 70, y = 50, units = 10 }` | a record (the README's "Records"): `let wave = { at = vec(70, 50), units = 10 }` then `wave.at`, `wave.units`, `has(wave, "units")`; a comma after each member |
| `time` | the game time now, in seconds |
| `stop` | what an `every` body returns to end it early |
| `"text \{x\}"` | `\{` `\}`: braces that are text, not a placeholder (also `\"` `\\` `\n`) |

## Types

Every value has a type the check works out when the file is read - nothing to write for it: a variable has its
`let`'s. `int` (`5`, `p.gold`, `game.minutes`), `float` (`5.0`, `.72`, `70%`, `20s`, `game.time`, an opinion), `bool`
(`true`, `a > b`, `p.alive`), `text`, `vec2` .. `vec4`, `epoch`, `color`, `unit type` (`unit.x`, `kind.x`), lists of
one of them, records, functions, triggers, and the game's objects (player, game, slot, unit).

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
may say its own: `let n: float = 5` (then `n = n * 1.5` fits), `let names: seq<text> = []`, `let l: player? = p.lord`.
The types by name: `int`, `float`, `bool`, `text`, `vec2` .. `vec4`, `epoch`, `color`, `unit_type`, `function`,
`record`, `trigger`, `player`, `game`, `slot`, a struct of the script ("Structs"); `seq<T>` a sequence of them, `list<T>` a list ("Sequences and lists"), `dict<K, V>` a dict ("Dicts"), `T?` one or null - types are written with `< >`, `[ ]` is for a list as a value (`[a, b]`, `[]`) and for indexing (`l[1]`). A call of a function a `let`
made is checked against its parameters: their count and types (`f(2.5)` for an `x: int` is a mistake).

**A function's type** says its parameters and what it gives back: `func<int, text, point>` takes an int and a text
and gives a point (the last is the result; `void`: nothing - `func<int, void>`, `func<void>`). A function written or
declared has such a type from its code, so it is checked where it goes, and a variable or parameter of one is called
like any function - its arguments checked, its result's type known:

```
apply(f: func<int, int>, v: int) => f(f(v))
twice(n: int) => n * 2
let a = apply(twice, 4)                    // 16
let b = apply((n: int) => n + 1, 4)        // 6
let c = apply((t: text) => 1, 4)           // a mistake: a func<text, int> can't go in
let op: func<int, int> = twice
op = (n: int) => n * n                     // set anew: still a func<int, int>
struct rule { name: text  test: func<int, bool> }
```

The parameters' types are the very ones (no int for a float there); the result may be an int where a float is
wanted, anything where `void` is. `function` alone is any function - nothing known of it: what it takes and gives is
checked when it is called. A function with a `ref` parameter has no such type (it goes by its name only). A value
of another type called (`let n = 5` then `n(3)`) is a mistake.

**A record's type** is its members' names and types as written: `let w = { at = vec(70, 50), units = 10 }` is a
`record { at, units }` whose `at` is a `vec2` - so `w.unitz` is a mistake when the file is read, `w.units + 1` an
int, `let n: int = w.at.x` a mistake (a float). Written in another order the members are the same type (`a = { tick
= 1, name = "x" }` into a `{ name = "", tick = 0 }`); other members, or a member of another type, are another type
(a mistake there). A record's members aren't set (`w.units = 5` - a struct's fields are, or a new record goes in its
place), it is compared with nothing (its members are), and its function member is called from a variable (`let f
= r.f`, then `f(...)`). `record` written as a type is any record: what it holds is checked when it is worked out
(`has(r, "units")` first); a seq of records of different members is a `seq<record>` written. A record of more than
16 members, and the personalities' records, stay that way too.

Where a type still can't be told (a member of a record written as `record`, what a function held in a variable
gives back when it wrote none), the value is checked when it is worked out, as before.

## Sequences and lists

Two kinds hold items of one type T:

| | `seq<T>`, a sequence | `list<T>`, a list |
|---|---|---|
| is | a value, as a number is | a handle: one table, shared by every value that holds it (as a dict) |
| `let b = a` | a copy - `b[1] = 0` leaves `a` alone | the same list - `b.add(5)` is seen in `a` |
| made | `[a, b]` where nothing else is wanted | `list<int>()`, `list<int>([1, 2])`, `list<int>(s)`, or `[...]` where a list goes |
| grows | made anew: `s = [s, x]` | `l.add(x)`, `l.insert(i, x)`, `l.remove_at(i)`, `l.clear()` |
| an item set | in its place: `s[2] = 5` changes the variable `s` | in the list: `l[2] = 5`, `l[2].x = 5` - whatever holds it |
| null items | none: a null is left out (`seq<int?>` is no type) | where T may be one: `list<int?>` keeps them |
| in a `{placeholder}` | one of its items, at random (the texts' rule) | all of them: `[1, 2, 3]` |

Both: `l[i]` (from 1), `len(l)`, `first(l)`, `item(l, i)`, `contains(l, x)`, `pick(l)`, `for x in l { ... }`.
`l.index_of(x)` is a list's: the number of the first item equal to x, null when none (`l.index_of(x) ?? 0`).
`a == b` of lists: the same list. A list's items are read as they are at that moment (a `for` over a list goes on
with what is there each round).

**`[...]` makes a sequence of any kind** - a seq, or a list where a list goes (a `let`'s written type, a variable's,
a parameter's, a struct's field, a function's result, an item set, `list<T>(...)`); with nothing wanted, a seq. Each
item written is one of three:

| Item | |
|---|---|
| a `T` | one item |
| a `seq<T>` or a `list<T>` | its items, in its place - `let a = [0, 1]` then `[a, 2]` is `[0, 1, 2]` |
| a `T?` (T no `?` itself) | one item, or left out when it is null (`[first(l), 5]`) |

Anything else is a mistake when the script is read (`[1, "a"]`, a point among ints). The type wanted says what an
item is - so a sequence of sequences is its type written:

```
let a: list<int> = [10, 20, 30, 10, [10]]      // a list of 5
let b = list<int>([5, s, a])                   // a new list: 5, s's items, a's
let copy: list<int> = [a]                      // a new list of a's items - let same = a is the same list
let rows: list<list<int>> = [a, [1, 2]]        // two lists: a itself and a new one
let more: list<list<int>> = [rows, [7]]        // rows' two lists, then a new one
rows[2].add(20)
let grid: seq<seq<int>> = [[1, 2], [3]]        // a seq of seqs: rows, each a value
grid[2][1] = 30                                // an item of a row, set in place
let opt: list<int?> = [1, null, first(s)]      // nulls kept
```

- Without a type written a sequence in a sequence is spliced, as ever (`let x = [a, a]` is four ints). In a seq of
  seqs, a row that may be null is a mistake (`rowmaybe() ?? []`), and so is an item whose type the check can't tell
  (a row, or rows to splice?) - into a variable with its type first. An empty seq as a row is a row.
- A `for`'s variable is the loop's copy of a seq's row (`r[1] = 2` there is a mistake); a list's item is the list's
  own (`for r in rows { r.add(9) }` adds to each).
- A struct's `list<T>` field is a new empty list in each struct made, and the copies of that struct share it.
- A list nobody holds any more is freed; one a function made inside a function can't hold that function (one of the
  script's top it can). A list goes into a saved game with everything that holds it.

The personalities' lines keep the texts' rules: a line's sequence may mix what it holds, one in it is spliced, and
they have no lists.

## Dicts

`dict<K, V>(default)` makes a dict: an entry per key, `default` for every key without one - `d[k]` is never missing.
K is an `int`, a `text` or an object (`player` ...: by its number, as `==` tells them apart); V any type. Its type is
written as made, `dict<K, V>`, for a `let`, a parameter, a field, a function's result:

```
let kills = dict<player, int>(0)
kills[p] += 1                                  // += -= *= /=: of number values
let home = dict<player, vec2>(vec(0, 0))
let spots = dict<text, point>(point(0, 0))
let pt = spots["gate"]                         // a copy - pt.x = 5 changes pt alone
pt.x = 5
spots["gate"] = pt                             // set whole
let paths = dict<int, seq<int>>([])
paths[1] = [paths[1], 7]                       // a seq: a value, spliced where a seq is built
let squads = dict<text, list<unit>?>(null)     // lists: null the default (one list would be every key's)
let sq = squads["north"] ?? list<unit>()
sq.add(u)                                      // a list read out is the list itself: the dict's
squads["north"] = sq
struct book {
    seen: dict<text, int> = dict<text, int>(0) // a new dict in each book made, shared by its copies
}
```

- A key and a value are checked when the file is read: `kills["a"]`, `spots["a"] = 5`, `kills[maybe]` of a `player?`
  are mistakes. A dict goes only where its very types do (a `dict<int, int>` is no `dict<int, float>`: it is shared).
- What `d[k]` gives is a copy of the entry - so `d[k].x = 5`, `d[k][1] = 5`, a struct's method that changes it, `ref
  r = d[k]` are mistakes (no place: read it into a variable, change it, set it back). A list or a dict in it is a
  handle: `d[k].add(x)` of a `dict<K, list<T>?>` that holds one adds to it (`let l = d[k]`, `l != null` first).
- A list or a dict as V: its default would be one table for every key without an entry - a mistake. `null` is (V
  written with `?`), and each key gets its own where it is new.
- A dict has no order of its entries: `for x in d` is a mistake - a list of the keys beside it goes through them.
  No methods (`d.add(...)` is a mistake); `a == b` is the same dict.
- A dict nobody holds any more is freed (as a list); one a function made inside a function can't be held in one. A
  dict goes into a saved game with everything that holds it.
- A personality's dict fields hold an `int`, a `float`, a `bool` or a `text` (the texts' rules: no structs, no lists).

## Structs

A type of the script's own: fields with their types, and methods. Declared at the script's top, known in the whole
file (above where it is written too).

```
struct point {
    x: int
    y: int = 5                  // its default

    add(value: int) {           // a method: its struct's fields are this.x, this.y
        this.x += value
        this.y += value
    }
    sum() => this.x + this.y
}
struct squad {
    name: text
    pos: point
    strength: float = 1.5
    units: list<unit>                // a list: shared by the struct's copies
    target: point?
}

let a = point()                 // x 0, y 5
let b = point(3)                // x 3, y 5
let c = point(0, 1)
let s = squad("north")          // pos point(0, 5), strength 1.5, no units, no target
```

- **Made** by its name: its fields in order, the last ones may be left out. A field left out has its default - the
  declared one (`y: int = 5`: a value that needs none of the script's names), else `0`, `false`, `""`, an empty list,
  `null` for a `T?`, a struct's own defaults. A field of a type with none (`player`, `unit`, `vec2` ...) is given, or
  declared with one, or a `T?`.
- **A value**, as a number is: `let b = a` is a copy (`b.x = 9` leaves `a` alone), a function gets a copy, a list
  holds copies. `a == b` when every field is the same (`!=`; no `<`); in a text `point(3, 5)`.
- **Set in its place**: `a.x = 5`, `a.x += 1`, `s.pos.x = 7` - the place is a variable, a field of one, or a list's
  item: `l[2].x = 5`, `l[2] = point(1, 1)`, `squads[1].units[3] = u` (a list's items are set the same way:
  `let n: seq<int> = [1, 2]` then `n[1] += 5`). A value that is in no place - a call's result, a seq written out -
  can't be set: the change would be dropped (`point(1, 2).x = 5` is a mistake). So is a `for`'s variable: it is the
  loop's copy of the item (`for i in 1..len(l) { l[i].x = 0 }` changes the list's).
- **Methods**: `a.add(2)` changes `a` - its `this` is the place it is called on, so a method that changes its struct
  is called on a place only (`point(1, 2).add(1)`: a mistake; `point(1, 2).sum()` is fine). `this = point()` sets
  the whole struct. A method sees the script's top like any declared function, may `wait`, and may be called in a
  `{placeholder}` when it changes nothing (`"{a.sum()}"`; a struct is made in a statement, not in a placeholder).
- A struct holds no struct of its own type directly (`next: node` would never end): `next: node?` or `kids:
  seq<node>` does.
- The check must know that a variable is a struct to set its fields or call its methods: it does from `let a =
  point(...)`, a parameter's type, a field's, a list's (`let l: seq<point> = [...]`), and from what a function or
  method gives back (`make(n: int): point { ... }` - "Functions and their names").
- Functions set the variables around them, a timer's function too - no ref needed:
  `let total = point()` then `game.timer(20s, () => { total.add(1) })`.

Not there: inheritance, operators of its own (a vector has them), private fields.

## Refs

A second name for a place - a variable, a field of one, a list's item - so that a function can change what it is
handed, and a long way is written once.

```
shift(ref p: point, by: int) {          // a parameter that is a place, not a copy
    p.x += by
}
shift(ref a, 2)                         // ref at the call too: the reader sees a may change
shift(ref squads[2].pos, 2)

ref q = squads[2]                       // a local: q is squads[2] from here on
q.strength = 50
q.pos.add(1)
ref n: int = total.x                    // of any type; its type, when written, is the place's own
n = n + 1                               // total.x is one more
```

- Read, a ref is the value at its place; set (`q = ...`, `q.x = ...`), the place gets it. Nothing else can be done
  with one: it is in no list, record or field, no function hands one back, there is no null ref and no arithmetic.
- A ref parameter takes `ref <place>` and nothing else; a plain parameter takes no `ref`. It goes only to functions
  the check knows - the script's declared ones, a `let`'s function not set anew, a struct's methods.
- A function made where a ref is doesn't see it (`() => q.x` inside the code that has `q`: a mistake) - it may run
  when the place is gone. `let v = q` copies the value for it. A method's `this` is such a ref.
- A ref to a list's item is the item's number, taken when the ref is made: if the list is set anew with fewer items,
  using the ref is a mistake told then (`a ref's place is gone`).
- A ref is no pointer: it is its variable and the numbers of the way from it, so it survives a `wait` and a saved
  game like any value.

## Objects

The game's things are objects, each of a kind: a value (`p.towers`) or a method (`p.opinion(q)`) by name. Its text
is its name (`"{p}"`), it equals only itself (`game.players[2] == player(2)`). Every kind's members with their types,
arguments and docs: `dll_cs/lang/player.cs`, `game.cs`, `slot.cs` (the language's objects declared once, as C#: the check and
the README read them).

| Kind | |
|---|---|
| `game` | the game: `game.time` (seconds), `game.minutes`, `game.players` (every player, in or out - `game.players[3]` is player 3), `game.player(3)`, `game.timer(20s, f)` ("Timers" below), `game.setup(f)`, `game.slots` ("Before the game starts") |
| slot | a player to be, in `game.setup`: `s.number`, `s.human`, `s.persona`, `s.color` (the last two set) |
| player | `player(3)`, `game.player(3)`, `game.players[3]`, `me`/`them` in the texts: every value of `them.` in the README (`p.towers`, `p.epoch`, `p.lord` - a player again), its relations as methods (`p.opinion(q)`, `p.kills(q, 10)`), `p.count(kind.towers)`; as a command's argument its number (`spawn(p, ...)`). What it does below |
| unit | one on the map - a soldier, a citizen, a ship, a building: `p.units()` (all of a player's, by id), `p.units(kind.tank)` (of a filter, as count's), `game.unit(id)` (`null` once it is gone). `u.id`, `u.alive`, `u.type` (`== unit.inf10_doughboy`), `u.owner`, `u.pos` (a `vec2` in tiles), `u.hp`, `u.max_hp`, `u.ammo`, `u.power`, `u.stance` (the stance buttons': 0 scout ... 1 stand ground ... 5 aggressive - set as `u.stance = 1`). Held by its id, not as the unit itself: it stays a value when the unit is destroyed - `u.alive` is false then, and any other member is a mistake (check `u.alive` after a wait). Read when used, never kept: `u.hp` is the hit points now. tests/probes/units.eeai |
| spawning | `let men = p.spawn(unit.inf10_doughboy, vec(40, 40), 10)` - up to 100 new units of one type at a place (a square, 2 tiles apart), returned at once. They are made as a scenario's Create effect makes them - in the simulation, on every PC alike, no message - at the game's next step, so the script doesn't wait there: spawns one after another are made together. It holds - waits for that step - only when it first uses one of them (a member, `==`) or asks the map (`p.units()`, `game.unit(id)`). A trigger runs at that step already: its spawned units are there at once. One whose owner left the game is never made (`alive` false). A dry run makes no units - reading a spawned one stops it. tests/probes/spawn.eeai |
| a unit's effects | `u.hp = 100`, `u.hp -= 50` (0 or less: it vanishes), `u.ammo = 2`, `u.power = 50`, `u.owner = player(3)`, `u.kill()` (it dies), `u.remove()` (it vanishes) - as a scenario's Unit-Attribute, Owner, Kill and Remove effects do them, at the game's next step like spawning: the script goes on and holds only when it next reads a unit or asks the map; a trigger's are done at once. A unit given away is made anew for its new owner, with another id - the value `u` follows it. tests/probes/unit_effects.eeai |

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
| `p.unit_limit = 300`, `p.map_revealed = true`, `p.flare(vec(40, 40))` | its unit limit (the game's setting and its own share, as a scenario's Pop Cap sets it), the whole map seen without fog (Visibility-On / Off), a flare on its minimap (Minimap-Flare, on the map's tiles) - at once, read back the same: `p.unit_limit` is the game's own limit for p. tests/probes/player_effects.eeai |
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

**Declared functions:** at the script's top, `name(a: int) => value` (one value) or `name(a: int) { ... }`
(statements, `return` for a value) declares a function - made before anything else runs, so it can be used above
where it is written, and declared ones call each other in any order. Named without `( )` it is its value, to pass on
(`every(10s, 0, 1min, check)`, `let f = check`). It is never set anew (`check = ...` is a mistake: only a `let`'s
variable changes), and inside a block (an `if`'s, a function's) it is a mistake - there it is
`let name = (a: int) => ...`. Its parameters take their types as a function's do.

```
let n = twice(4)                           // 8 - declared further down
twice(x: int) => x * 2
spawn_row(y: int) {
    for i in 1..3 { spawn(3, unit.b_guard_tower_bronze, 51, y + i * 3) }
}
```

**What a function gives back** has a type like any value. Of one value (`twice(x: int) => x * 2`) it is that
value's - nothing to write. A declared function or a struct's method with statements that hands a value back writes
it after its parameters: `name(a: int): float { ... }` (`return <value>` in one without is a mistake). Then every
`return` hands back a value of that type, and no way through the function ends without one - unless the type is one
or null (`int?`: ending without a `return`, or `return null`, is none). One that hands nothing back writes nothing.
A function as a value may write it too (`(a: int): int => a + 1`, `(a: int): point => { ... }`); written for one
value, the value must fit.

```
half(n: int): float {
    return n / 2
}
first_over(l: seq<int>, over: int): int? {
    for v in l {
        if v > over { return v }
    }
    return null
}
make(n: int): point {
    if n < 0 { return point(0, 0) }
    return point(n, n)
}
let m = make(3)                            // a point: m.x = 10, m.add(1) - no type written at the let
let h: int = half(5)                       // a mistake: a float can't go in
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
The rest of the script runs from the game's start as always; the setup's statement is nothing there. Its function sees none of the
script's names (they don't exist yet) - only its own `let`s, and the players to be:

| | |
|---|---|
| `game.slots`, `game.slot(2)` | the players to be by number (the human and computer slots in order) - only here; `game.players` after |
| `s.number`, `s.human` | the player it becomes; 1: a human's |
| `s.persona = persona.fuchs` | a computer's personality: `persona.<its file>` (personalities\fuchs.eeai) - an unknown one a mistake when the file is read |
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

Only a skirmish gives its computer players personalities by itself (since 2026-11-16): a scenario's and a campaign's
keep their own names and have none - no personality's lines, name or civilization - unless a script gives one, with the
`persona` command: `persona(2, "fuchs")` (player 2 is personalities\fuchs.eeai from now on: its name, civilization and
lines), in setup.eeai's `on_scenario()` / `on_campaign()` or a scenario's own script. A saved game keeps who had which.

## setup.eeai: every game's script

`setup.eeai` beside `ageofai.ini` (`<game>\ageofai\setup.eeai`; the shipped one is `config/setup.eeai`, installed once
and then the player's own) is a script of every game started on that PC - not a test's only. The mod reads it anew at
each game's start and calls its hooks, functions declared by these names:

```
let waves = 0                        // outside the hooks: runs first, in every game - a name all hooks share

on_game() {                          // every new game, before the game type's own
    player(2).gold += 500
}
on_skirmish() {                      // a random-map game
    game.timer(5min, () => {
        waves += 1
        spawn(2, unit.inf02_spearman, 70, 50, waves * 5)
    })
}
on_scenario() { }                    // a scenario (the scenario editor's test play too)
on_campaign() { }                    // a campaign's scenario
```

A hook takes nothing and runs statements (`on_game() { ... }`); one that isn't needed is left out. Another `on_...`
name at the file's top is a mistake - a hook's name mistyped would never run. The file's top runs first, then
`on_game()`, then the game type's hook when `on_game()` has ended (its waits too) - from the game's start, as a script:
waits, triggers, timers, the players' members, the pipe's commands. Its `game.setup` blocks run before the game starts
as any script's, in every kind of game - and the slots' personalities and colours are a random-map game's, so one
there ends the script in a scenario.

Its turns come between the simulation's steps (where the pipe's commands run), several a game second, in games without
a computer player too - as every script's: a test's `setup=` scripts run beside it, after it in each turn.

Not run: a multiplayer game (each PC would run its own file - a desync), the scenario editor itself. In a saved game
that is loaded the hooks aren't called again - they ran when the game began, and the save carries on what they
started ("Saved games" below). A mistake is told on the screen and in the log (`# setup: <file>:<line>: ...`, `# texts: setup.eeai:<line>:
...`); `ageofai_check.exe` checks the file without the game (in the game folder without arguments: the personalities
and `setup.eeai`). Test runs read the `setup.eeai` beside their `ageofai.ini` - the shipped one, whose hooks are
empty, unless `CONFIG=` names another folder's.

## Saved games

A save carries its scripts whole (the block `scripts` of the save's `.ageofai` file, dll/test/scripts.c). A script is
compiled to instructions and run by a machine of its own (dll/script/expr.c SCRIPTS COMPILED), whose state is plain
data - and that is what the save holds: the instructions (a save runs its own: it depends neither on what `setup.eeai`
says later nor on what a later version of the mod would compile of it), the functions the script is in with their
parameters and `let`s, the values it was working out, where it stands; with them the triggers and their conditions'
state, the timers at their times, the `after`'s still to come, the dicts, the spawned units' handles, the structs
(their types with the instructions, their values as they are) and the refs, and the source for the messages. The loaded game goes on with them as they were: `# persist: scripts restored: 1 (1 going on), 3
triggers and timers, 1 after's ...`.

A script that waits (`wait`, `wait_until`, `every`) goes on where it stood, at the time it waited for - anywhere: in
a statement of its own, in a loop, inside functions, in the middle of an expression (`let x = 9999 + f() + l[2]`
where `f` waits). Nothing is worked out a second time.

Not saved: a function of a text (a personality's) held in a script's variable (told in the log). A script that saves
the game in the middle of its own turn (not the pipe's `save`, which is done after it) doesn't go on after the load -
told; what it started does. A test with its own `setup=` scripts keeps to them - the save's are left out there.

`script state` (a pipe command) saves the scripts and reads them back at once, as a save and its load do; `script
state save <file>` / `script state load <file>` (a path without spaces) carry them from one game to another.

## Several scripts side by side

`setup=a.eeai;b.eeai` (`;` - `run_test.py` splits its ini arguments at commas): each runs on its own, in turn, always
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
waiting moves the clock (from 0), every command goes into `<out>` as `<ms> <command>` instead of being run (its reply
is "ok"). The setup files of before 2026-10-20 (`@<minutes> <command>` lines) were converted this way: every converted
file issued the same 905 commands at the same times (to 1 ms) as the old runner would have.
