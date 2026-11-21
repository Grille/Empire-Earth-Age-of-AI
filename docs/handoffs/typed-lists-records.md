# Handoff: the .eeai language's types - lists and records typed (2026-11-15)

For the session that picks this up. The player's plan, agreed step by step on 2026-11-15 ("its probably needed to
keep it clean ... right now record and list are types that hold a somewhat unclear mix of types"; "a list of list
should still be possible: the type is list<list<int>> then, list<list<int>> gets spliced in, list<int> is an item -
the wonders of static typing"; "yes, sounds good" to both open points below).

## Done and committed (all installed)

| Commit | What |
|---|---|
| d2190e50 "Structs and refs" + e1ce1a2e | `struct`, value types, sets in place, `ref` parameters and locals, list item sets |
| 373d6b5a | what a function gives back: `name(a: int): float { ... }` (required of a declared function / method with statements that returns a value), told from `=> value` |
| 56c584f2 | `func<A, B, R>` types; the table of types that hold types (expr.c THE TYPES THAT HOLD TYPES: XCT_WRAP / XCT_FN / XCT_REC); types are 15 bits (host.h), `X_TBASE`, `xt_elem`, `xt_list_of`; `list<list<int>>` and `list<int?>` can be WRITTEN |

docs/scripts.md ("Structs", "Refs", "Types", "Functions and their names") and docs/testing.md ("The language's own
tests") describe them. Tests: `tests/lang/*.eeai` with `.expect` (the build runs each dry and compares),
`tests/probes/structs.eeai` (in a game).

## Sequences and lists: DONE 2026-11-20 (docs/scripts.md "Sequences and lists")

The player 2026-11-20: today's list is no list ("list probably should be a handle type to a dynamic array which also
allows null"), `[]` and its splicing "just the sequence init syntax ... a compiler feature for constructing sequence
types, not bound to a specific type", `seq<T>` the value's name. So: `seq<T>` the value (what the step below made),
`list<T>` a handle (expr.c LISTS), `[...]` built as the type wanted. Next: dicts on the same footing (any value type,
a literal), then records typed.

### Dicts of any value: DONE 2026-11-20 (docs/scripts.md "Dicts")

The player 2026-11-20: "do the typed dicts next". Done as planned below: `dict<K, V>` a type of the check
(XCT_DICT), V any type (a non-plain one an owned `xv` in the entry), the sweep frees dicts too, saves format 6. The
decisions taken on the way: a list or a dict as V can't be the default (one table for every missing key) - `null`
with `V?` is; `for x in d` and methods of a dict are mistakes (no order, no keys() yet); `a == b` of dicts works now
(it was a run-time mistake before). Next: records typed, then the dict literal.

What it took (the plan as it was, read off the code, expr.c DICTS):
- **The check has no dict type.** `d[k]` is X_ANY when the script is read; xdict_fit checks a value at run time
  against the four plain value types. First a compound type `dict<K, V>` (XCT_DICT, t[0] K, t[1] V) - x_type_named
  for lets, parameters, fields; x_stype(XN_DICT) that type; XN_INDEX of one V; XN_SETI checks K and V (op numeric);
  `for` over a dict stays a mistake. XN_DICT's n is `kt * 4096 + vt` now - a compound V's base passes 4096: keep the
  type there and split it in the compiler (XI_DICT a: `kt << 16 | vt`). The parser reads V with xps_type_scan (it
  stops at the closing `>`).
- **A value that isn't plain** (a struct, a seq, a list, a vector): kept as an owned xval in the entry (`struct xval
  *xv` beside vn / vs - xo_copy / xo_free, as a list's items), copied out when read (xvm_copy), checked by the check
  only (it can't be told at run time). A plain V stays as it is, so the personalities' dict fields (persona.c
  reads and writes vn / vs itself) don't change - a personality's dict of another V is a mistake (texts hold no
  structs or lists).
- `d[k].x = 5` stays a mistake (an entry is no place: `let v = d[k]`, change it, `d[k] = v`); `d[k].add(x)` of a
  dict<K, list<T>> works as it is (a handle).
- **The sweep:** xhl_mark goes into a dict's non-plain values (a `mark` on struct xdict, the same generation).
- **Saves:** a dict's entries are written before the code and the lists are read back, so a value that names a
  struct type or a list can't be read there. A dict of a non-plain V: its kt / vt / count early, its default and
  entries (xs_put) at the end after the lists' items; xs_note goes into them (xs_dict noting a dict once - it has
  no "new" flag yet). Format 6.
- The literal (`{ "a" = 1 }` or entries in `[...]`) after records are typed - `{ }` is the record's.

## Step 2, lists strictly typed: DONE 2026-11-20 (committed - docs/scripts.md "Lists")

What follows is the plan as it was parked; the decisions taken when it was finished: a list holds no null
(`list<T?>` is no type, a row that may be null is a mistake), an item of a list of lists whose type can't be told
is a mistake, `contains` and `==` of structs compare rows item for item. Left as it is: `a ?? b` of a value never
null is no mistake.

## (the parked plan)

`local/work/keep/lists_step/lists_step.patch` is the working diff of `dll/script/expr.c` against 56c584f2
(`git apply` it; `s9a.py` beside it is the same change as an edit script, run from the repo root). It compiles free
of warnings in the checker, `tests/lang/structs.eeai` still gives its lines, and the draft test
(`lists.eeai` beside the patch) gave the right lines on its first run:

```
flat 3 2 rows 2 5 more 4 0      set 9 60 3      total 89      ref 50      mixed 2.5
```

What the patch does:

- **The rule** (x_check `XN_LIST`, scripts only; texts untouched): a list's items are of one type T. An item is a T
  (one item), a `list<T>` (spliced in) or a `T?` (left out when null). The type WANTED where the list is written
  (`g_x_want`, set by `X_WANT` at a let's written type, an assignment, a struct's field, a list's item set, a
  return, a `=> [..]` of a written result, a call's argument - `x_arg_want`, a field's default) says what T is: for
  `list<list<int>>` a `list<int>` is one item and a `list<list<int>>` is spliced. No type wanted: flat, as ever;
  items of different types are a mistake then. The list's type is kept on its node (`n->n`; x_stype reads it).
- **A list as an item** (A LIST AS AN ITEM, top of expr.c): boxed - a record of the type `g_x_boxty`, its items the
  record's values, no names - so nothing splices it. The check marks the item (`xnode.sk |= 4`; the struct marks
  are `sk & 3` now) and the compiler emits `XI_ITEM` with b 1, or `XI_BOX` for `rows[2] = row`. Unboxed where an
  item is read: `xop_index`, `XI_FORL_NEXT`, first / item / map_range / pick, `xref_get`, `xe_ident`'s view read.
  Refs through rows: `xli_n` / `xli_at`, `xref_build` rebuilds a row as a row. Saved as a record with type -1.
- An empty row is a box of none; an empty list as a value of its own is still "nothing" at run time - the check keeps
  `list<T>` apart from null statically (`l == null` of a plain list is already a mistake).

Still to do for step 2:

1. `tests/lang/lists.eeai` + `.expect` from the draft, more cases: a list written as an argument / a field / a
   return, `first(rows)`, `pick`, a row of one item, `list<int?>`, lists of structs with list fields, rows across
   `wait` and `script state` (a probe like tests/probes/structs.eeai), 200000 rounds with the memory cleared.
2. The mistakes (a file each, as `neg.py` beside the patch does): mixed items with no type, an item of another
   type with one, a `list<int>` where a `list<list<int>>` is wanted, `rows[2] = 5`.
3. Run the whole build: the stricter rule may stop an older script (mixed lists) - fix the script, not the rule.
4. Decide with the player or leave: `a ?? b` of a value that is never null is not a mistake yet; a null row in a
   list of rows becomes an empty row (none and empty are not told apart at run time).
5. `contains(rows, row)` compares a box with a list: not handled.
6. docs/scripts.md ("Values": `[...]`, "Types"), docs/testing.md, then the checks below and commit.

## After it

3. **Records typed from how they are written**: DONE 2026-11-20 (the player: "continue" after the dicts) - expr.c
   x_rec_type: a script's record literal is an `XCT_REC` of its members sorted by name (`record` alone stays any
   record, a text's records too); a member it hasn't, a member set, records compared, `r.f(...)` are mistakes when
   the file is read. Left: a record type can't be written by its members (`record { a: int }`) - a struct is that.
   Then the dict literal: DONE 2026-11-20 - the player: `{ }` should mean no specific type, "its just a key:value
   collection that get interpreted as a record or dict or something else". So `{ ... }` is what the place it goes
   into is (expr.c x_check_rec, X_WANT as `[...]` has): a dict (a name the text key, `[key]` any; the default its V's
   own, or `dict<K, V>(default, { ... })`), a struct by its fields' names, else a record. Left: a `{ }` through
   `?:` / `??` gets no type wanted (a record there).
   Entries with a colon since 2026-11-21 - the player: "most languages use : for dict instead of =": `{ a: 1,
   [k]: 2 }`, a personality's records `name: "x",` (the file's top keeps `name = value`, a block's statements
   theirs); `{ a = 1 }` is a mistake. Every .eeai converted by scripts/build/eeai_colons.py.
4. `dict<K, V>` with any value type (structs).
5. The player's own generics (`struct pair<A, B>`) - only if scripts ever define their own containers.

## How each step was checked (do the same)

- `sh build.sh` free of warnings in a clean copy (`b5.sh <tag>`: a `git archive` of HEAD in
  `local/work/builds/vm5` plus the working tree's engine files; `c5.sh <file>`: the checker alone, then the file
  run dry). A clean checkout gives the `.expect` files CR LF - the build's compare leaves that aside.
- `verify5.sh <tag>`: old engine (`vm5/base5.dll`) against new in games - the five probes' `# setup:` lines the
  same, and the text corpus. **The corpus game (seed 101, game_ai=1) has two outcomes from the same seed, whichever
  build plays it** (md5 4b95af18... and 6ddd8bb9..., P4's food 72 or 76 in 27 rows): each file must be one of the
  two, byte for byte. (My first reading - "it depends on the DLL's layout" - was wrong and is corrected in
  docs/testing.md.) That non-repeatable game is somebody's bug in the AI code from minute 3 on (P4's citizens), not
  looked for.
- `st5.sh <tag>`: tests/probes/structs.eeai in three games, one with `script state` called while it waits.
- Commit only your own paths (`git commit --only -- <paths>`), then `sh scripts/build/install_wait.sh`.
