// BIG SELECTIONS ([general] big_selection, default 1). Double-click (all units of a type on screen) and the
// selection box send a command message (0x4BD072, 0x138 bytes, ids 0x4E/0x4F/0x50) that carries the units found
// (0x50F589) packed into 256 bytes at +0x34, length at +0x134 (one byte); 0x4BD2C4 unpacks them (also when the
// message is carried out). The packer 0x4BD15A takes at most 63 units (local budget 0x3F) - with larger armies only
// a part got selected. Format: byte 0 = the player byte of the ids (player +0x45C, id bits 24..31), then runs of
// units sharing id bits 8..23: header 0x00|n (bits 8..23 zero), 0x40|n + 1 byte (bits 8..15), 0x80|n + 2 bytes
// (bits 8..23), n <= 63, then n bytes of id bits 0..7; an empty list is 0xFF. This packer writes the same format
// for as many units as fit into the 255 bytes the length byte allows (about 240, fewer with scattered ids), so the
// message stays readable for an unmodded game (multiplayer) and the game's own unpacker. The rest goes after it when
// the lasso sends it (EEALasso slot 1): messages of the kind that adds its ids to the selection (0x4F, the shift
// box's; a control box's toggles 0x50 - each id once, so the same) - one 0x4E and its adds select what one message
// with every id would, each a message an unmodded game knows. Each add plays its last unit's selection sound when
// carried out for the local player: those of the adds are left out (the first message's plays).
internal int g_big_selection = 1;
internal unsigned g_sel_last_in, g_sel_last_out;   // tests: units found / packed by the last pack
internal unsigned g_sel_last_msgs, g_sel_quieted;   // tests: messages the last big selection went as, sounds left out
internal uint32_t *g_sel_ids;                       // the last pack's ids, sorted; g_sel_at .. g_sel_n not packed yet
internal unsigned g_sel_room, g_sel_n, g_sel_at;
internal uint8_t *g_sel_rest_out;                   // the packed bytes of the message the rest belongs to
internal int g_sel_more, g_sel_quiet;               // packing an add from the rest / sending the adds

internal int sel_cmp(const void *a, const void *b) {
    uint32_t x = (*(const uint32_t *)a >> 8) & 0xFFFF, y = (*(const uint32_t *)b >> 8) & 0xFFFF;
    return x < y ? -1 : x > y;
}
internal unsigned __cdecl hook_sel_pack(uint8_t *list, uint32_t player_byte, uint8_t *out) {
    if (!g_big_selection)
        return sel_pack(list, player_byte, out);
    if (!g_sel_more) {   // a new selection: the ids found (an add's pack takes the rest instead)
        uint32_t *v = FIELD(list, 4, uint32_t *), *ve = FIELD(list, 8, uint32_t *);
        unsigned n = v ? (unsigned)(ve - v) : 0;
        if (n > g_sel_room) {
            uint32_t *p = realloc(g_sel_ids, n * sizeof *p);
            if (p) g_sel_ids = p, g_sel_room = n;
            else n = g_sel_room;
        }
        if (n) {
            memcpy(g_sel_ids, v, n * sizeof *v);
            qsort(g_sel_ids, n, sizeof g_sel_ids[0], sel_cmp);   // runs of the same bits 8..23 (the game groups them in a map)
        }
        g_sel_n = n;
        g_sel_at = 0;
        g_sel_last_in = n;
        g_sel_last_out = 0;
    }
    uint32_t *ids = g_sel_ids;
    unsigned n = g_sel_n, pos = 0, i = g_sel_at;
    out[pos++] = (uint8_t)player_byte;
    while (i < n) {
        uint32_t pre = (ids[i] >> 8) & 0xFFFF;
        unsigned run = 0;
        while (i + run < n && run < 63 && ((ids[i + run] >> 8) & 0xFFFF) == pre) run++;
        unsigned head = pre == 0 ? 1 : pre < 0x100 ? 2 : 3;
        if (pos + head + 1 > 255) break;
        if (pos + head + run > 255) run = 255 - pos - head;
        if (pre == 0) out[pos++] = (uint8_t)run;
        else if (pre < 0x100) { out[pos++] = (uint8_t)(0x40 | run); out[pos++] = (uint8_t)pre; }
        else { out[pos++] = (uint8_t)(0x80 | run); out[pos++] = (uint8_t)(pre >> 8); out[pos++] = (uint8_t)pre; }
        for (unsigned k = 0; k < run; k++) out[pos++] = (uint8_t)ids[i + k];
        i += run;
        g_sel_last_out += run;
    }
    if (pos == 1) out[pos++] = 0xFF;
    g_sel_at = i;
    g_sel_rest_out = i < n ? out : NULL;
    return pos & 0xFF;
}

// the lasso's send: its message, then the rest of a big selection as adds - made by the message's constructor as
// the lasso makes them (its rectangle and type; the pack takes the rest instead of what it finds) and sent the same way
internal void __thiscall hook_lasso_send(ee_lasso *self) {
    ee_select_msg *m = lasso_get_message(self);
    int more = m && g_sel_rest_out && select_msg_packed_at(m) == g_sel_rest_out;
    float rect[4];
    int type = 0, kind = 0;
    if (more) {
        memcpy(rect, select_msg_rect_at(m), sizeof rect);
        type = select_msg_get_select_kind(m);
        kind = select_msg_get_kind(m) == 0x50 ? 0x50 : 0x4F;
    }
    g_sel_rest_out = NULL;
    eea_lasso_send(self);
    if (!more) return;
    ee_server *net = game_server();
    g_sel_last_msgs = 1;
    while (net && g_sel_at < g_sel_n) {
        unsigned at = g_sel_at;
        g_sel_more = 1;
        ee_select_msg *c = select_msg_new(rect, type, kind);
        g_sel_more = 0;
        if (!c || g_sel_at == at) break;
        g_sel_quiet = 1;
        server_send_all(net, c);
        g_sel_quiet = 0;
        g_sel_last_msgs++;
    }
    g_sel_rest_out = NULL;
}
internal uint32_t __cdecl hook_selection_sound(ee_player *pl, void *sound) {
    if (g_sel_quiet) {
        g_sel_quieted++;
        return (uint32_t)-1;
    }
    return play_local(pl, sound);
}

// seltest T [x0 y0 x1 y1]: builds the double-click message for unit type T (the screen rectangle 0x8D2778, or the
// given one) for the local player without sending it: units found, packed, unpacked. Tests.
internal void cmd_seltest(char *out, const char *args) {
    if (atoi(args) == -1) {   // seltest -1: how many the local player has selected (a vdrag's box; its first argument is a type's)
        ee_player *pl = game_players()[local_player_index()];
        int *grp = pl ? player_get_selected_group(pl) : NULL;
        REPLY("selected %u (the last big selection: found %u, %u messages, %u sounds left out)\n", grp ? group_count(grp) : 0,
              g_sel_last_in, g_sel_last_msgs, g_sel_quieted);
        return;
    }
    int type = 0;
    float r[4];
    if (sscanf(args, "%d %f %f %f %f", &type, &r[0], &r[1], &r[2], &r[3]) < 5) memcpy(r, select_rect(), sizeof r);
    static __attribute__((aligned(8))) uint8_t mem[0x140];
    memset(mem, 0, sizeof mem);
    ee_select_msg *msg = (ee_select_msg *)mem;
    g_sel_last_in = g_sel_last_out = 0;
    select_msg_init(msg, r, type, 0x4E);
    g_sel_rest_out = NULL;   // never sent: no adds after it
    uint32_t *b = select_msg_get_ids_begin(msg), *e = select_msg_get_ids_end(msg);
    REPLY("rect %.0f %.0f %.0f %.0f: found %u, packed %u in %u bytes, unpacked %d (big_selection %d)", r[0], r[1], r[2],
          r[3], g_sel_last_in, g_sel_last_out, select_msg_get_flag(msg), b ? (int)(e - b) : 0, g_big_selection);
    // carried out as the message does (EEMSelectUnits::vf10 -> 0x55B3FB on the player): the selected group (+0x9BC)
    ee_player *pl = game_players()[local_player_index()];
    int *grp = pl ? player_get_selected_group(pl) : NULL;
    if (grp) {
        player_select(pl, select_msg_selection_at(msg));
        REPLY(", selected %u", group_count(grp));
    }
    REPLY("\n");
}

internal void select_install(void) {
    g_big_selection = g_set.big_selection;
    int of = 0;
    BOOL ok = patch_sel_pack(hook_sel_pack);
    int more = 0;
    if (g_big_selection) {   // the sounds first: the adds are sent only with them quiet
        int quiet = patch_selection_sound(hook_selection_sound, &of);
        more = quiet == of && patch_lasso_send(hook_lasso_send);
    }
    if (g_log) fprintf(g_log, "# selection: big_selection=%d (hook %s, the rest as adds %s)\n", g_big_selection,
                       ok ? "ok" : "FAILED", more ? "ok" : g_big_selection ? "FAILED" : "off");
}
