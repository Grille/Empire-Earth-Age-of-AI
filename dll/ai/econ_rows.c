// THE ECONOMY ROWS of the log: per AI every 10 s its stock, demand, gatherer targets and lists, the managers' timers
static void log_state(void *rm, const char *phase, uint32_t now) {
    void *pl = rm_player(rm);
    if (!g_log || !pl) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "%u,%d,%s,%d,%d", now, FIELD(pl, PL_INDEX, int), phase,
            FIELD(pl, PL_DIFFICULTY, int), FIELD(pl, PL_CHEAT, uint8_t));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%d", FIELD(pl, PL_STOCK + i * 4, int));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%d", FIELD(rm, RM_DEMAND + i * 4, int));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%u", FIELD(rm, RM_TARGET + i * 4, uint32_t));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%u", FIELD(rm, RM_LISTS + i * 0x14 + 0x10, uint32_t));
    fprintf(g_log, ",%d", FIELD(rm, RM_INITIALIZED, uint8_t));
    for (int i = 0; i < 5; i++) fprintf(g_log, ",%u", FIELD(rm, RM_TIMERS + i * 4, uint32_t));
    void *cm = FIELD(rm, RM_OWNER, void *);
    fprintf(g_log, ",%p", FIELD(cm, CM_PROJECT, void *));
    for (int i = 0; i < CM_TIMERS_N; i++) fprintf(g_log, ",%u", FIELD(cm, CM_TIMERS + i * 4, uint32_t));
    fputc(',', g_log);
    for (int i = 0; i < CM_FLAGS_LEN; i++) fprintf(g_log, "%02x", FIELD(cm, CM_FLAGS + i, uint8_t));
    fputc(',', g_log);
    for (int i = 0; i < PL_TOGGLES_LEN; i++) fprintf(g_log, "%02x", FIELD(pl, PL_TOGGLES + i, uint8_t));
    fprintf(g_log, ",%d\n", FIELD(FIELD(pl, PL_TECHTREE, void *), TT_EPOCH, int));
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

static void log_header(void) {
    fputs("time_ms,player,phase,difficulty,cheat", g_log);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",stock%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",demand%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",target%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",list%d", i);
    fputs(",rm_init,t_farm,t_20s,t_targets,t_assign,t_cleanup"
          ",cm_project,cm_t_epoch,cm_t_bcount,cm_t_milbldg,cm_t_build,cm_t_e817,cm_t_59s,cm_flags74"
          ",toggles_ef4,epoch\n", g_log);
}
