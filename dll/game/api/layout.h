// THE GAME'S OBJECTS: names for some of their fields (their offsets per build: raw/fields.h) and FIELD, the read itself.
#define OF(k, x) (g_of[OF_##k##_##x])   // the running build's offset of the field the 2017 update has at x in a k
// EECPResourceManager layout (from decompiled ctor/serializer 0x489F76 / 0x48C5B2)
#define RM_OWNER        OF(RM, 0x130)  // -> object whose +0x4C is the EEComputerPlayer
#define RM_LISTS        OF(RM, 0xA4)  // 7 unit lists, stride 0x14, count at +0x10
#define RM_DEMAND       OF(RM, 0x134)  // int[7] outstanding resource need
#define RM_TARGET       OF(RM, 0x150)  // uint[7] desired gatherers
#define RM_TIMERS       OF(RM, 0x16C)  // uint[5] next-run game times (ms)
#define RM_INITIALIZED  OF(RM, 0x187)  // byte, timers are only reset while this is 0
// EEPlayer / EEComputerPlayer: its fields by name in objects.c (player_get_index(pl) ...)
#define PL_TOGGLES_LEN  18             // the AI toggle bytes 0xEF4..0xF05 (player_get_toggle)
// the tech tree (player_get_tech_tree): current epoch index (0 = Prehistoric)
#define TT_EPOCH        OF(TT, 0x4E4)
// EECPCivilizationManager (= rm->owner; main AI update 0x46E2C7)
#define CM_PROJECT      OF(CM, 0x50)   // current construction project; new projects only start when NULL
#define CM_TIMERS       OF(CM, 0x58)   // uint[6]: next epoch, bldg-count check, mil bldgs (31s), build (0x46E51E), 0x46E817, 59s
#define CM_TIMERS_N     6
#define CM_FLAGS        OF(CM, 0x74)   // bytes 0x74..0x7C
#define CM_SETUP_DONE   OF(CM, 0x75)   // one-time initial setup (0x46E2C7 top) runs while this is 0
#define CM_BUILD_MGR    OF(CM, 0x3C)   // EECPBuildManager    (Update 0x468540)
#define CM_RESEARCH_MGR OF(CM, 0x40)   // EECPResearchManager (Update 0x486A74)
#define BM_INITIALIZED  OF(BM, 0x148)  // build lists initialised by 0x465E4F while 0
#define RS_INITIALIZED  OF(RS, 0x118)  // research schedule initialised while 0
#define CM_FLAGS_LEN    9

#define FIELD(p, off, T) (*(T *)((uint8_t *)(p) + (off)))
#define FIELD_AT(p, off) ((uint8_t *)(p) + (off))   // the field's address: an array or structure inside the object
#define G_UNIT_GRID_OFS OF(MAP, 0x181724)    // map +: unit grid (see attack.c)
