// THE GAME'S OBJECTS: OF, a field's offset in the running build (raw/fields.h), FIELD, the read itself, and the few
// names left for fields not moved over to objects.c's accessors yet (their kinds' turn comes).
#define OF(k, x) (g_of[OF_##k##_##x])   // the running build's offset of the field the 2017 update has at x in a k
// counts of objects.c's indexed fields
#define PL_TOGGLES_LEN  18             // the AI toggle bytes 0xEF4..0xF05 (player_get_toggle)
#define CM_TIMERS_N     6              // cm_get_timer
#define CM_FLAGS_LEN    9              // cm_get_flag: bytes 0x74..0x7C
// the tech tree (player_get_tech_tree): current epoch index (0 = Prehistoric)
#define TT_EPOCH        OF(TT, 0x4E4)

#define FIELD(p, off, T) (*(T *)((uint8_t *)(p) + (off)))
#define FIELD_AT(p, off) ((uint8_t *)(p) + (off))   // the field's address: an array or structure inside the object
#define G_UNIT_GRID_OFS OF(MAP, 0x181724)    // map +: unit grid (see attack.c)
