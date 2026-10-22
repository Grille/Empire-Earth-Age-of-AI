// THE GAME'S OBJECTS: FIELD, a read at an offset - the accessors with the running build's offset (a field's
// name, raw/tables.c: only inside game/api/), elsewhere the runtime library's layouts (deque, vector, UString) and raw dumps.
// counts of indexed fields (player.c, ai_managers.c)
#define PL_TOGGLES_LEN  18             // the AI toggle bytes 0xEF4..0xF05 (player_get_toggle)
#define CM_TIMERS_N     6              // cm_get_timer
#define CM_FLAGS_LEN    9              // cm_get_flag: bytes 0x74..0x7C

#define FIELD(p, off, T) (*(T *)((uint8_t *)(p) + (off)))
#define FIELD_AT(p, off) ((uint8_t *)(p) + (off))   // the field's address: an array or structure inside the object
