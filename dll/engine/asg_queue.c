// AN ATTACK SUBGROUP'S WAITING UNITS: units handed to an attack subgroup wait in its queue (subgroup +0x188, 16-byte
// entries, each holding a reference on its unit) until the subgroup's vf85 takes them in (0x45BD56 - land, sea and
// transport subgroups - and the air subgroups' copy in 0x4426CC): front (0x444760) into EDI, pop (0x482028: releases
// the queue's reference through 0x4FCAD3 and drops the entry), then a member record for EDI (0x455331 -> the record's
// constructor 0x461551: record +0x44 = the unit, its count + 1). A unit that died while it waited is held by nothing
// else any more: the pop gives it back to the unit pool (0x9413B0, vf13 0x5ED71E) and the record is made for the freed
// unit - its count 1 again, +0x2A set, on memory the pool hands out again at once (it is a stack: the next arrow or
// stone ball). The record releases that memory again later: the attack call-off's vf26 (0x45B4FB, every member of the
// attack at once: one more reference, the record's release, then its own) gives a unit still in the pool back a
// second time - the pool hands one memory out to two units and the x-sorted unit list loops (the stalls in 0x5310B3 /
// 0x62FE01) or breaks (the crashes at 0x53110D, 0x628D6F, 0x531131); a unit whose memory is out again is released
// under its new owner. Seen 2026-10-22 (calloff_storm, docs/testing.md "Continental stalls"): every record made for a
// freed unit came from here (2-8 a game), and the crash's broken unit was such a record's, given back twice by a
// call-off. Fix: after the pop, a unit back in the pool (+0x2A: set by the release's vf12 0x628BC6, cleared when the
// pool hands it out, 0x628CDD) is not made a member - EDI 0 takes the game's own branch for an empty entry. A unit
// still held elsewhere (dead or in a transport: mIsValid 0, its count above 1) joins as before.
internal unsigned g_asg_queue_freed;
__attribute__((used)) uintptr_t g_asg_queue_pop;   // game_asg_queue_pop()
__attribute__((used)) ee_unit *__cdecl asg_queue_unit(ee_unit *u) {
    if (!u || !unit_get_released(u)) return u;
    if (g_log && g_asg_queue_freed++ < 5) {
        ee_unit_type *td = unit_get_type_data(u);
        fprintf(g_log, "# engine fix: a unit (type %d) freed while it waited to join an attack subgroup - not made a member at %.1f min\n",
                td ? unit_type_get_id(td) : -1, game_time() / 60000.0);
    }
    return NULL;
}
__attribute__((naked, used)) void asg_queue_pop_thunk(void) {   // at the call of the pop: ECX the queue, EDI its front unit
    __asm__ volatile (
        "movl _g_asg_queue_pop, %eax\n\t"
        "call *%eax\n\t"                // the pop (thiscall, no arguments); EAX, ECX, EDX are free after it
        "pushl %edi\n\t"
        "call _asg_queue_unit\n\t"
        "addl $4, %esp\n\t"
        "movl %eax, %edi\n\t"           // the unit, or 0: the caller skips the record
        "ret\n\t");
}

internal void asg_queue_install(void) {
    g_asg_queue_pop = game_asg_queue_pop();
    int ok = patch_asg_queue_pop(asg_queue_pop_thunk, NULL);
    if (g_log) fprintf(g_log, "# engine fix: units freed while waiting to join an attack subgroup %d/2\n", ok);
}
