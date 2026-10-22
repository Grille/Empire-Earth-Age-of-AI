// Random-map test starts (autotest ini): turns a -loadgame start into a random-map start by rewriting the
// game setup handed to EEGameState::Start (0x52599B). Layout learned from a real menu start (2026-10-10):
//   16 player slots of 0x38 bytes: +0 type (1 human, 0 computer, 4 closed; 3 = unused in saved games),
//     +4 = 3, +8 UString name, +0x20 slot index, +0x24 colour, +0x28 team (0 = none), +0x2C starting
//     citizens, +0x34 flags (human 0x75010001, computer 0x77010000).
//   +0x3A8 UString map script name ("Continental"), +0x3D8 mode (0 random map), +0x3DC map size (0 Tiny, 1 Small,
//   2 Medium, 3 Large, 4 Huge, 5 Gigantic - the setup copy 0x524B.. clamps it below 6, default 1; 0x524E1F turns it into
//   42/58/75/105/160/220 and the map script's IsLargeMap etc. pick "<map> Large.rmv"), +0x3E0 start epoch (14 = random),
//   +0x3E4 end epoch (13 = Nano), +0x3E8 an option index (checked against the list at 0x8CF52C - NOT the map size: until
//   2026-10-20 map_size wrote it and every test map was Small), +0x3F0 unit limit, +0x3F4 wonders for victory, +0x3F8
//   difficulty (1 medium), +0x400 option bytes.
//   Map seed: world (0x91977C) +0x54.
// Ini keys: random_map=<script name>, map_scale (tiny|small|medium|large|huge|gigantic or 0..5, default small - the
//   real map size), map_size (the old key: the +0x3E8 index, kept so old test commands play the same maps), ai_count (1..7), start_epoch, seed, teams=ffa|pairs,
//   citizens, unit_limit (500), difficulty (easy|medium|hard or 0..2: the setup's +0x3F8 and 0x918C14, every AI's +0xBA4 - the
//   menu's AI difficulty; default medium). AI toggles at their first update: ai_cheat (0/1), ai_ignore (Ignorieren 0/1).
// VANILLA PLAYERS (game/players.c is_vanilla): vanilla_players=<digits>, and for them only vanilla_difficulty (as difficulty)
//   and vanilla_cheat (0/1, on top of ai_cheat). The game's cheat (EEComputerPlayer vf27 0x430FBD, AdjustCost): with
//   Cheat (+0xEFB) on, every price x pct / 100 - pct by difficulty, dbcpbehavior record 96 (easy +0xD48, hard +0xD4C) and
//   record 67 (medium +0xBB4). The difficulty itself scales the epoch timer (+0xD60/+0xD64), the attack times
//   (+0xD08/+0xD0C, +0xCEC/+0xCF0), the attack group size (+0xD30..) and the population cap (+0xDF0..), all read when
//   used - a player's +0xBA4 set at its first update counts from then on; its first epoch attempt and first attack,
//   drawn before with the setup's difficulty, are drawn again by the game's own code (0x46DE8C, 0x43DD2F). Logged per
//   AI at its first update: "# cheat: P3 (vanilla) difficulty 2, cheat 1: a price of 1000 costs 10".
// Included by eeaimod.c.

internal struct {
    char map[64];
    int size, scale, ais, start_epoch, seed, pairs, citizens, unit_limit;
    int ai_cheat, ai_ignore;   // -1 = leave as the game sets them
    int epoch_min, epoch_max;  // test-only epoch timer in minutes (0 = game setting)
    int difficulty;            // the setup's (every AI), 0..2
    int v_difficulty, v_cheat; // the vanilla players' own (-1: as the others)
} g_rm = { "", 0, 1, 3, 0, 0, 0, 5, 500, -1, -1, 0, 0, 1, -1, -1 };
internal const char *const k_rm_scale[6] = { "tiny", "small", "medium", "large", "huge", "gigantic" };

internal int difficulty_key(const char *ini, const char *key, int def) {   // easy|medium|hard or 0..2
    char v[16];
    GetPrivateProfileStringA("autotest", key, "", v, sizeof v, ini);
    if (!v[0]) return def;
    if (!lstrcmpiA(v, "easy")) return 0;
    if (!lstrcmpiA(v, "medium") || !lstrcmpiA(v, "moderate")) return 1;
    if (!lstrcmpiA(v, "hard")) return 2;
    int d = atoi(v);
    return d < 0 ? 0 : d > 2 ? 2 : d;
}


internal void randmap_load(const char *ini) {
    GetPrivateProfileStringA("autotest", "random_map", "", g_rm.map, sizeof g_rm.map, ini);
    g_rm.size = ini_int("autotest", "map_size", 0, ini);
    char sc[16];
    GetPrivateProfileStringA("autotest", "map_scale", "", sc, sizeof sc, ini);
    g_rm.scale = 1;
    for (int k = 0; k < 6; k++) if (!lstrcmpiA(sc, k_rm_scale[k])) g_rm.scale = k;
    if (sc[0] >= '0' && sc[0] <= '5' && !sc[1]) g_rm.scale = sc[0] - '0';
    g_rm.ais = ini_int("autotest", "ai_count", 3, ini);
    if (g_rm.ais < 1) g_rm.ais = 1;
    if (g_rm.ais > 7) g_rm.ais = 7;
    g_rm.start_epoch = ini_int("autotest", "start_epoch", 0, ini);
    g_rm.seed = ini_int("autotest", "seed", 0, ini);
    char t[16];
    GetPrivateProfileStringA("autotest", "teams", "ffa", t, sizeof t, ini);
    g_rm.pairs = !lstrcmpiA(t, "pairs");
    g_rm.citizens = ini_int("autotest", "citizens", 5, ini);
    g_rm.unit_limit = ini_int("autotest", "unit_limit", 500, ini);   // the game's, shared by all players
    g_rm.ai_cheat = ini_int("autotest", "ai_cheat", -1, ini);
    g_rm.ai_ignore = ini_int("autotest", "ai_ignore", -1, ini);
    g_rm.epoch_min = ini_int("autotest", "epoch_min", 0, ini);
    g_rm.epoch_max = ini_int("autotest", "epoch_max", g_rm.epoch_min, ini);
    g_rm.difficulty = difficulty_key(ini, "difficulty", 1);
    char vp[32];
    GetPrivateProfileStringA("autotest", "vanilla_players", "", vp, sizeof vp, ini);
    for (char *c = vp; *c; c++) if (*c >= '1' && *c <= '9') g_vanilla_mask |= 1u << (*c - '0');
    g_rm.v_difficulty = difficulty_key(ini, "vanilla_difficulty", -1);
    g_rm.v_cheat = ini_int("autotest", "vanilla_cheat", -1, ini);
}

// Called from hook_game_start before the original runs.
internal void randmap_rewrite(ee_setup *setup) {
    ustring_set(setup_map_name_at(setup), g_rm.map);
    setup_set_mode(setup, 0);
    for (int i = 0; i < 16; i++) {
        ee_setup_slot *slot = setup_slot_at(setup, i);
        int type = i == 0 ? 1 : i <= g_rm.ais ? 0 : 4;
        slot_set_type(slot, type);
        slot_set_unknown_4(slot, 3);
        slot_set_unknown_1c(slot, 0);
        slot_set_index(slot, i);
        slot_set_color(slot, i);
        // human alone on team 1; AIs free-for-all (team 0) or in pairs (2,2,3,3,...)
        slot_set_team(slot, type == 4 ? 0 : i == 0 ? 1 : g_rm.pairs ? 2 + (i - 1) / 2 : 0);
        slot_set_citizens(slot, g_rm.citizens);
        slot_set_unknown_30(slot, 0);
        slot_set_flags(slot, i == 0 ? 0x75010001u : 0x77010000u);
    }
    setup_set_unknown_3d4(setup, 1);
    setup_set_map_scale(setup, g_rm.scale);   // the map size (1 Small: what every test map was before map_scale)
    setup_set_start_epoch(setup, g_rm.start_epoch);
    setup_set_end_epoch(setup, 13);
    setup_set_map_size(setup, g_rm.size);
    setup_set_unknown_3ec(setup, 3);
    setup_set_unit_limit(setup, g_rm.unit_limit);
    setup_set_unknown_3f4(setup, 3);
    setup_set_difficulty(setup, g_rm.difficulty);
    // the AIs' difficulty: their set-up (0x431697) copies the global 0x918C14 (set by the game options messages,
    // EEMFullGameOptions 0x4B642F, which a load-game start never sends - the setup's +0x3F8 alone left medium)
    *difficulty_var() = g_rm.difficulty;
    setup_set_unknown_3fc(setup, 0);
    setup_set_flag(setup, 0, 1); setup_set_flag(setup, 1, 0); setup_set_flag(setup, 2, 0); setup_set_flag(setup, 3, 1);
    *game_type_var() = 0;   // game type: random map
    ee_world *world = game_world();
    if (world && g_rm.seed) world_set_seed(world, (unsigned)g_rm.seed);
    if (g_log) {
        ee_game_data *lim = game_data();   // the game's unit limits: default +0x2E8, the most +0x2EC (more: the default)
        fprintf(g_log, "# random map: \"%s\" size %d, %d AIs (%s), start epoch %d, seed %d, difficulty %d%s%s, unit limit %d (game default %d, max %d)\n",
                g_rm.map, g_rm.size, g_rm.ais, g_rm.pairs ? "pairs" : "ffa", g_rm.start_epoch, world ? (int)world_get_seed(world) : -1, g_rm.difficulty,
                g_rm.scale != 1 ? ", map scale " : "", g_rm.scale != 1 ? k_rm_scale[g_rm.scale] : "", g_rm.unit_limit,
                lim ? gamedata_get_unit_limit(lim) : -1, lim ? gamedata_get_unit_limit_max(lim) : -1);
        fflush(g_log);
    }
}

// EEPlayer::SetDiplomacy(other, stance, ?) 0x55B8E1, thiscall, ret 0xC;
// stance values: 0 = allied, 1 = enemy (verified: team mates have 0 toward each other, everyone else 1).
// Diplomacy array: player+0x9E4[index].

// Called once per AI player at its first update of a game.
internal void randmap_ai_toggles(ee_player *pl) {
    if (g_rm.ai_cheat >= 0) player_set_cheat(pl, (uint8_t)g_rm.ai_cheat);
    if (g_rm.ai_ignore >= 0) player_set_ignore(pl, (uint8_t)g_rm.ai_ignore);
    int vanilla = is_vanilla(pl), idx = player_get_index(pl);
    if (vanilla && g_rm.v_cheat >= 0) player_set_cheat(pl, (uint8_t)g_rm.v_cheat);
    if (vanilla && g_rm.v_difficulty >= 0 && g_rm.v_difficulty != player_get_difficulty(pl)) {
        player_set_difficulty(pl, g_rm.v_difficulty);
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        uint32_t now = game_time();
        if (cm) cm_set_epoch_time(cm, now, 0);   // the epoch attempt, drawn anew
        if (!player_get_first_attack_done(pl)) cp_schedule_attack(pl, now);   // the first attack
        if (g_log) fprintf(g_log, "# vanilla: P%d difficulty %d - first epoch attempt at %.1f min, first attack at %.1f min\n", idx,
                           g_rm.v_difficulty, cm ? cm_get_timer(cm, 0) / 60000.0 : -1.0, player_get_next_attack(pl) / 60000.0);
    }
    if (g_log) {   // the cheat as the game will apply it: AdjustCost (vf27) on a price of 1000
        int cost = cp_adjust_cost(pl, 1000);
        fprintf(g_log, "# cheat: P%d (%s) difficulty %d, cheat %d: a price of 1000 costs %d (pct easy %d, medium %.1f, hard %d)\n", idx,
                vanilla ? "vanilla" : "mod", player_get_difficulty(pl), player_get_cheat(pl), cost, player_get_cost_easy(pl),
                player_get_cost_medium(pl), player_get_cost_hard(pl));
    }
    if (g_rm.epoch_min > 0 && !vanilla) {   // the vanilla players keep the game's timer
        // Test-only: shorter epoch timer. SetNextEpochAdvanceTime (0x46DE8C) draws the delay between
        // player+0xD58 and +0xD5C minutes (x difficulty factor) into civ manager +0x58; the first one is
        // drawn before this hook runs, so reschedule it here (deterministic per player index).
        player_set_epoch_min(pl, g_rm.epoch_min);
        player_set_epoch_max(pl, g_rm.epoch_max);
        int span = g_rm.epoch_max - g_rm.epoch_min + 1;
        uint32_t delay = (uint32_t)(g_rm.epoch_min + (idx * 7) % (span > 0 ? span : 1)) * 60000u;
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        if (cm) cm_set_timer(cm, 0, game_time() + delay);
        if (g_log) fprintf(g_log, "# test epoch timer: P%d %d-%d min, first attempt at %.1f min\n", idx,
                           g_rm.epoch_min, g_rm.epoch_max, (game_time() + delay) / 60000.0);
    }
    if (!g_rm.map[0]) return;
    ee_player *human = game_players()[(int)local_player_index()];
    if (!human || human == pl) return;
    // The idle human slot: civilization counts as chosen (else the civ-selection timer opens a dialog that
    // freezes the game), and allied with every AI so the AIs fight each other and the game can't end in
    // the human's defeat.
    player_set_civ_chosen(human, 1);
    player_set_explore_all(human, 1);   // the watching human sees the whole map (AIs still have to explore)
    set_diplomacy(human, pl, 0, 0);
    set_diplomacy(pl, human, 0, 0);
    if (g_log) fprintf(g_log, "# random map: P%d allied with human P%d\n", player_get_index(pl), player_get_index(human));
}
