// THE GAME'S OBJECTS' FIELDS are the spec's since 2026-10-25: spec/game/*.cs, a class per object of the game with its
// fields' offsets per build (and how each was found: [found]); their C is gen/ (scripts/build/gen_api.py) - of_unit_hp
// the offset, unit_get_hp(u) the accessor. The table here is empty, kept while raw/tables.c still builds the address
// rows' tables beside it.
#define NA 0xFFFFFFFFu   // a build hasn't got it, or it isn't known yet

#define FIELDS(O)
