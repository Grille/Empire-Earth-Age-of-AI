// THE GAME'S OBJECTS: the offsets of the fields the mod reads in them (the 2017 build's; another build's layouts
// would come here as addr.h's addresses did) and FIELD, the read itself.
// EECPResourceManager layout (from decompiled ctor/serializer 0x489F76 / 0x48C5B2)
#define RM_OWNER        0x130  // -> object whose +0x4C is the EEComputerPlayer
#define RM_LISTS        0x0A4  // 7 unit lists, stride 0x14, count at +0x10
#define RM_DEMAND       0x134  // int[7] outstanding resource need
#define RM_TARGET       0x150  // uint[7] desired gatherers
#define RM_TIMERS       0x16C  // uint[5] next-run game times (ms)
#define RM_INITIALIZED  0x187  // byte, timers are only reset while this is 0
// EEPlayer / EEComputerPlayer
#define PL_INDEX        0x45C
#define PL_STOCK        0xAF0  // int[] resources, indexed by resource type
#define PL_DIFFICULTY   0xBA4
#define PL_CHEAT        0xEFB
#define PL_TOGGLES      0xEF4  // AI toggle bytes 0xEF4..0xF05 (Aktiv, Cheat, Angreifen, ...)
#define PL_TOGGLES_LEN  18
#define PL_TECHTREE     0x9D4  // -> tech tree; +0x4E4 = current epoch index (0 = Prehistoric)
#define TT_EPOCH        0x4E4
// EECPCivilizationManager (= rm->owner; main AI update 0x46E2C7)
#define CM_PROJECT      0x50   // current construction project; new projects only start when NULL
#define CM_TIMERS       0x58   // uint[6]: next epoch, bldg-count check, mil bldgs (31s), build (0x46E51E), 0x46E817, 59s
#define CM_TIMERS_N     6
#define CM_FLAGS        0x74   // bytes 0x74..0x7C
#define CM_SETUP_DONE   0x75   // one-time initial setup (0x46E2C7 top) runs while this is 0
#define CM_BUILD_MGR    0x3C   // EECPBuildManager    (Update 0x468540)
#define CM_RESEARCH_MGR 0x40   // EECPResearchManager (Update 0x486A74)
#define BM_INITIALIZED  0x148  // build lists initialised by 0x465E4F while 0
#define RS_INITIALIZED  0x118  // research schedule initialised while 0
#define CM_FLAGS_LEN    9

#define FIELD(p, off, T) (*(T *)((uint8_t *)(p) + (off)))
