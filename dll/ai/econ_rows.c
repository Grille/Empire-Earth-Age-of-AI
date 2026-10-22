// THE ECONOMY ROWS of the log: per AI every 10 s its stock, demand, gatherer targets and lists, the managers' timers
internal void log_state(void *rm, const char *phase, uint32_t now) {
    ee_player *pl = rm_player(rm);
    if (!g_log || !pl) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "%u,%d,%s,%d,%d", now, player_get_index(pl), phase,
            player_get_difficulty(pl), player_get_cheat(pl));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%d", player_get_stock(pl, i));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%d", rm_get_demand(rm, i));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%u", rm_get_target(rm, i));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%u", FIELD(rm_list_at(rm, i), 0x10, uint32_t));
    fprintf(g_log, ",%d", rm_get_initialized(rm));
    for (int i = 0; i < 5; i++) fprintf(g_log, ",%u", rm_get_timer(rm, i));
    void *cm = rm_get_owner(rm);
    fprintf(g_log, ",%p", cm_get_project(cm));
    for (int i = 0; i < CM_TIMERS_N; i++) fprintf(g_log, ",%u", cm_get_timer(cm, i));
    fputc(',', g_log);
    for (int i = 0; i < CM_FLAGS_LEN; i++) fprintf(g_log, "%02x", cm_get_flag(cm, i));
    fputc(',', g_log);
    for (int i = 0; i < PL_TOGGLES_LEN; i++) fprintf(g_log, "%02x", player_get_toggle(pl, i));
    fprintf(g_log, ",%d\n", FIELD(((void *)player_get_tech_tree(pl)), TT_EPOCH, int));
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

internal void log_header(void) {
    fputs("time_ms,player,phase,difficulty,cheat", g_log);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",stock%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",demand%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",target%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",list%d", i);
    fputs(",rm_init,t_farm,t_20s,t_targets,t_assign,t_cleanup"
          ",cm_project,cm_t_epoch,cm_t_bcount,cm_t_milbldg,cm_t_build,cm_t_e817,cm_t_59s,cm_flags74"
          ",toggles_ef4,epoch\n", g_log);
}
