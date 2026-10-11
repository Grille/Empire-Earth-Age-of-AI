// Random-map test starts (autotest ini): turns a -loadgame start into a random-map start by rewriting the
// game setup handed to EEGameState::Start (0x52599B). Layout learned from a real menu start (2026-10-10):
//   16 player slots of 0x38 bytes: +0 type (1 human, 0 computer, 4 closed; 3 = unused in saved games),
//     +4 = 3, +8 UString name, +0x20 slot index, +0x24 colour, +0x28 team (0 = none), +0x2C starting
//     citizens, +0x34 flags (human 0x75010001, computer 0x77010000).
//   +0x3A8 UString map script name ("Continental"), +0x3D8 mode (0 random map), +0x3E0 start epoch
//   (14 = random), +0x3E4 end epoch (13 = Nano), +0x3E8 map size (0 small), +0x3F0 unit limit,
//   +0x3F4 wonders for victory, +0x3F8 difficulty (1 medium), +0x400 option bytes.
//   Map seed: world (0x91977C) +0x54.
// Ini keys: random_map=<script name>, map_size, ai_count (1..7), start_epoch, seed, teams=ffa|pairs,
//   citizens, unit_limit (500). AI toggles at their first update: ai_cheat (0/1), ai_ignore (Ignorieren 0/1).
// Included by eeaimod.c.

static struct {
    char map[64];
    int size, ais, start_epoch, seed, pairs, citizens, unit_limit;
    int ai_cheat, ai_ignore;   // -1 = leave as the game sets them
    int epoch_min, epoch_max;  // test-only epoch timer in minutes (0 = game setting)
} g_rm = { "", 0, 3, 0, 0, 0, 5, 500, -1, -1, 0, 0 };

typedef void *(__thiscall *UStrAssignFn)(void *ustr, const char *s);
#define IAT_USTRING_ASSIGN 0x00823228   // UString::operator=(const char *)

static void randmap_load(const char *ini) {
    GetPrivateProfileStringA("autotest", "random_map", "", g_rm.map, sizeof g_rm.map, ini);
    g_rm.size = GetPrivateProfileIntA("autotest", "map_size", 0, ini);
    g_rm.ais = GetPrivateProfileIntA("autotest", "ai_count", 3, ini);
    if (g_rm.ais < 1) g_rm.ais = 1;
    if (g_rm.ais > 7) g_rm.ais = 7;
    g_rm.start_epoch = GetPrivateProfileIntA("autotest", "start_epoch", 0, ini);
    g_rm.seed = GetPrivateProfileIntA("autotest", "seed", 0, ini);
    char t[16];
    GetPrivateProfileStringA("autotest", "teams", "ffa", t, sizeof t, ini);
    g_rm.pairs = !lstrcmpiA(t, "pairs");
    g_rm.citizens = GetPrivateProfileIntA("autotest", "citizens", 5, ini);
    g_rm.unit_limit = GetPrivateProfileIntA("autotest", "unit_limit", 500, ini);   // the game's, shared by all players
    g_rm.ai_cheat = GetPrivateProfileIntA("autotest", "ai_cheat", -1, ini);
    g_rm.ai_ignore = GetPrivateProfileIntA("autotest", "ai_ignore", -1, ini);
    g_rm.epoch_min = GetPrivateProfileIntA("autotest", "epoch_min", 0, ini);
    g_rm.epoch_max = GetPrivateProfileIntA("autotest", "epoch_max", g_rm.epoch_min, ini);
}

// Called from hook_game_start before the original runs.
static void randmap_rewrite(uint8_t *setup) {
    (*(UStrAssignFn *)IAT_USTRING_ASSIGN)(setup + 0x3A8, g_rm.map);
    *(int *)(setup + 0x3D8) = 0;
    for (int i = 0; i < 16; i++) {
        uint8_t *slot = setup + i * 0x38;
        int type = i == 0 ? 1 : i <= g_rm.ais ? 0 : 4;
        *(int *)(slot + 0x00) = type;
        *(int *)(slot + 0x04) = 3;
        *(int *)(slot + 0x1C) = 0;
        *(int *)(slot + 0x20) = i;
        *(int *)(slot + 0x24) = i;
        // human alone on team 1; AIs free-for-all (team 0) or in pairs (2,2,3,3,...)
        *(int *)(slot + 0x28) = type == 4 ? 0 : i == 0 ? 1 : g_rm.pairs ? 2 + (i - 1) / 2 : 0;
        *(int *)(slot + 0x2C) = g_rm.citizens;
        *(int *)(slot + 0x30) = 0;
        *(uint32_t *)(slot + 0x34) = i == 0 ? 0x75010001u : 0x77010000u;
    }
    *(int *)(setup + 0x3D4) = 1;
    *(int *)(setup + 0x3DC) = 1;
    *(int *)(setup + 0x3E0) = g_rm.start_epoch;
    *(int *)(setup + 0x3E4) = 13;
    *(int *)(setup + 0x3E8) = g_rm.size;
    *(int *)(setup + 0x3EC) = 3;
    *(int *)(setup + 0x3F0) = g_rm.unit_limit;
    *(int *)(setup + 0x3F4) = 3;
    *(int *)(setup + 0x3F8) = 1;
    *(int *)(setup + 0x3FC) = 0;
    setup[0x400] = 1; setup[0x401] = 0; setup[0x402] = 0; setup[0x403] = 1;
    *(int *)0x00918BF4 = 0;   // game type: random map
    uint8_t *world = *(uint8_t **)0x0091977C;
    if (world && g_rm.seed) *(int *)(world + 0x54) = g_rm.seed;
    if (g_log) {
        fprintf(g_log, "# random map: \"%s\" size %d, %d AIs (%s), start epoch %d, seed %d\n", g_rm.map, g_rm.size,
                g_rm.ais, g_rm.pairs ? "pairs" : "ffa", g_rm.start_epoch, world ? *(int *)(world + 0x54) : -1);
        fflush(g_log);
    }
}

// EEPlayer::SetDiplomacy(other, stance, ?) 0x55B8E1, thiscall, ret 0xC;
// stance values: 0 = allied, 1 = enemy (verified: team mates have 0 toward each other, everyone else 1).
// Diplomacy array: player+0x9E4[index].
#define FN_SET_DIPLOMACY 0x0055B8E1
#define G_PLAYERS        0x00918374
#define G_LOCAL_PLAYER   0x00918E84

// Called once per AI player at its first update of a game.
static void randmap_ai_toggles(void *pl) {
    if (g_rm.ai_cheat >= 0) FIELD(pl, PL_CHEAT, uint8_t) = (uint8_t)g_rm.ai_cheat;
    if (g_rm.ai_ignore >= 0) FIELD(pl, 0xF02, uint8_t) = (uint8_t)g_rm.ai_ignore;
    if (g_rm.epoch_min > 0) {
        // Test-only: shorter epoch timer. SetNextEpochAdvanceTime (0x46DE8C) draws the delay between
        // player+0xD58 and +0xD5C minutes (x difficulty factor) into civ manager +0x58; the first one is
        // drawn before this hook runs, so reschedule it here (deterministic per player index).
        FIELD(pl, 0xD58, int) = g_rm.epoch_min;
        FIELD(pl, 0xD5C, int) = g_rm.epoch_max;
        int idx = FIELD(pl, PL_INDEX, int), span = g_rm.epoch_max - g_rm.epoch_min + 1;
        uint32_t delay = (uint32_t)(g_rm.epoch_min + (idx * 7) % (span > 0 ? span : 1)) * 60000u;
        uint8_t *cm = FIELD(pl, 0xB90, uint8_t *);
        if (cm) FIELD(cm, 0x58, uint32_t) = game_time() + delay;
        if (g_log) fprintf(g_log, "# test epoch timer: P%d %d-%d min, first attempt at %.1f min\n", idx,
                           g_rm.epoch_min, g_rm.epoch_max, (game_time() + delay) / 60000.0);
    }
    if (!g_rm.map[0]) return;
    uint8_t *human = ((uint8_t **)G_PLAYERS)[*(int *)G_LOCAL_PLAYER];
    if (!human || human == pl) return;
    // The idle human slot: civilization counts as chosen (else the civ-selection timer opens a dialog that
    // freezes the game), and allied with every AI so the AIs fight each other and the game can't end in
    // the human's defeat.
    human[0xB46] = 1;
    human[0x465] = 1;   // the watching human sees the whole map (AIs still have to explore)
    void (__thiscall *set_dip)(void *, void *, int, int) = (void *)FN_SET_DIPLOMACY;
    set_dip(human, pl, 0, 0);
    set_dip(pl, human, 0, 0);
    if (g_log) fprintf(g_log, "# random map: P%d allied with human P%d\n", FIELD(pl, PL_INDEX, int), FIELD(human, PL_INDEX, int));
}
