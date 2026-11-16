// QUEUE REFUNDS ([patches] queue_refund; the player 2026-11-16, of the handover's AI that paid for 73 citizens and got
// none: "shouldnt the game atleast have refuses and refunded the order? the ai being able to pay for nothing seems like
// a big hidden bug"). A building's production start (hooks.production_queue_empty) takes each order at its queue's
// front it can't make off the queue and refunds it - but it tests for an empty queue between the two and leaves
// without the last one's refund. The test sent to queue_refund_thunk: the same test, and for an empty queue the order
// just taken off refunded (EEPlayer.refund_tech 1.0, as the loop's own), then on to where the game's je went. Measured
// in a test game (the handover from epoch 0 before its fix, its citizens obsolete): 5 ordered for 250 food, 200 back.
internal uintptr_t g_qr_exit, g_qr_next;   // the loop's end, its refund (the queue not empty)
__attribute__((used)) internal uintptr_t g_qr_go;   // where the thunk goes now (read only by it)
__attribute__((used)) void __stdcall queue_refund_check(ee_unit *b, ee_tech *node) {
    int *qb = unit_get_queue_begin(b), *qe = unit_get_queue_end(b);
    int empty = !qb || (unsigned)((uint8_t *)qe - (uint8_t *)qb) < (unsigned)production_queue_entry();
    g_qr_go = empty ? g_qr_exit : g_qr_next;
    ee_player *pl = empty ? unit_get_owner(b) : NULL;
    ee_unit_type *td = unit_get_type_data(b);
    if (!pl || !node || !td) return;
    player_refund_tech(pl, node, unit_type_get_id(td), 1.0f);
    static unsigned told;   // rare in a game as the game plays it: the first ones told
    ee_tech_data *d = tech_get_data(node);
    if (g_log && told++ < 20)
        log_locked("# queue refund: at %.1f min P%d's last order of its queue (tech %d) at a building of type %d refunded\n",
                   game_time() / 60000.0, player_get_index(pl), d ? tech_data_get_id(d) : -1, unit_type_get_id(td));
}
// at the test: the building in esi, the node in ebx, the queue in edi; called (the return dropped) - every register
// kept, on to g_qr_go
__attribute__((naked, used)) void queue_refund_thunk(void) {
    __asm__ volatile (
        "addl $4, %esp\n\t"
        "pushal\n\t"
        "pushl %ebx\n\t"
        "pushl %esi\n\t"
        "call _queue_refund_check@8\n\t"
        "popal\n\t"
        "jmp *_g_qr_go\n\t");
}

internal void queue_refund_install(void) {
    if (!g_set.queue_refund) { if (g_log) fprintf(g_log, "# queue refund: off (the game's own)\n"); return; }
    int ok = patch_queue_empty(queue_refund_thunk, &g_qr_exit, &g_qr_next);
    if (g_log) fprintf(g_log, "# queue refund: the last order of a queue refunded too - %s\n", ok ? "patched" : "NOT patched (the code differs)");
}
