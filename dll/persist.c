// SAVED GAMES: the mod's own state goes into a file next to the save. Included by eeaimod.c after diplo.c.
//
// The game saves its own AI managers (build, civilization, research, resource, unit, wall), the stances, names and
// colours; what the mod rebuilds by itself (income rates, defence layout, siege, threat) needs nothing. Lost without
// this file: opinions and grudges, alliance times, who capitulated to whom, pending offers and promised tribute, the
// "already said" flags of the situations, the army accounts of the budget.
//
// Every save goes through 0x540ACA cdecl(UString *file, int, int, UString folder by value) -> 0 on success
// (save dialog EEASaveGame::vf1, autosave, quicksave, the network save message: 4 call sites, all redirected). After a
// successful save <game>\<folder><file without .ees>.aimod is written: a header with the .ees file's size and write
// time, then named blocks {tag, size, bytes}. On a saved-game start (game start hook, mode 2) the matching .aimod is
// read after the new-game resets: only if size and write time still match the .ees (a save made later without the
// mod leaves a stale file behind), and only blocks whose tag and size match (a newer DLL with other layouts just
// starts fresh for those). All times inside are game times, which the save restores.
#define FN_SAVE_GAME 0x00540ACA
static char g_aimod_ini[MAX_PATH];   // eeaimod.c (defined after the includes)
static int g_persist = 1;   // aimod.ini [general] save_state
static const struct { const char *tag; void *p; size_t n; } k_blocks[] = {
#define B(x) { #x, &x, sizeof x }
    B(g_diplo_seen), B(g_diplo_prev), B(g_diplo_due), B(g_war_since), B(g_ally_since), B(g_peace_due),
    B(g_peace_refused), B(g_offer_t), B(g_offer_last), B(g_opinion), B(g_war_betrayal), B(g_diplo_answers),
    B(g_pressure), B(g_offer_kind), B(g_promise), B(g_capit_last), B(g_lord), B(g_own_prev), B(g_ws), B(g_sit),
    B(g_no_citizens_since), B(g_bud), B(g_talk_last), B(g_talk_any), B(g_offer_ignored), B(g_escrow), B(g_lord_since),
    B(g_kills), B(g_kills_min), B(g_kills_min_t), B(g_attack_sent), B(g_hist), B(g_offer_vs),
#undef B
};
struct persist_head { char magic[8]; uint32_t version, game_time; uint64_t ees_size; FILETIME ees_time; };

static int ees_stamp(const char *ees, uint64_t *size, FILETIME *t) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(ees, GetFileExInfoStandard, &a)) return 0;
    *size = ((uint64_t)a.nFileSizeHigh << 32) | a.nFileSizeLow;
    *t = a.ftLastWriteTime;
    return 1;
}
// <game dir>\<folder><file> with .ees, and the same with .aimod
static void persist_paths(const char *folder, const char *file, char *ees, char *side) {
    char dir[MAX_PATH];
    lstrcpynA(dir, g_aimod_ini, MAX_PATH);
    char *slash = strrchr(dir, '\\');
    if (slash) slash[1] = 0; else dir[0] = 0;
    wsprintfA(ees, "%s%s%s", dir, folder ? folder : "", file);
    size_t n = strlen(file);
    int has_ext = n > 4 && !lstrcmpiA(file + n - 4, ".ees");
    if (!has_ext) lstrcatA(ees, ".ees");
    lstrcpyA(side, ees);
    lstrcpyA(side + strlen(side) - 4, ".aimod");
}

static void persist_write(const char *folder, const char *file) {
    char ees[MAX_PATH * 2], side[MAX_PATH * 2];
    persist_paths(folder, file, ees, side);
    struct persist_head h = { "EEAIMOD", 1, game_time(), 0, { 0, 0 } };
    if (!ees_stamp(ees, &h.ees_size, &h.ees_time)) {
        if (g_log) fprintf(g_log, "# persist: saved game %s not found - no state file\n", ees);
        return;
    }
    FILE *f = fopen(side, "wb");
    if (!f) { if (g_log) fprintf(g_log, "# persist: can't write %s\n", side); return; }
    fwrite(&h, sizeof h, 1, f);
    for (size_t i = 0; i < sizeof k_blocks / sizeof *k_blocks; i++) {
        char tag[32] = { 0 };
        lstrcpynA(tag, k_blocks[i].tag, sizeof tag);
        uint32_t n = (uint32_t)k_blocks[i].n;
        fwrite(tag, sizeof tag, 1, f);
        fwrite(&n, sizeof n, 1, f);
        fwrite(k_blocks[i].p, 1, n, f);
    }
    fclose(f);
    if (g_log) { fprintf(g_log, "# persist: state saved with %s at %.1f min\n", side, h.game_time / 60000.0); fflush(g_log); }
    // the game deletes old autosaves: drop state files whose save is gone
    char pat[MAX_PATH * 2], *slash = strrchr(side, '\\');
    if (!slash) return;
    int dl = (int)(slash - side) + 1;   // (wsprintf has no %.*s)
    lstrcpynA(pat, side, dl + 1);
    lstrcatA(pat, "*.aimod");
    WIN32_FIND_DATAA fd;
    HANDLE fh = FindFirstFileA(pat, &fd);
    for (int more = fh != INVALID_HANDLE_VALUE; more; more = FindNextFileA(fh, &fd)) {
        char other[MAX_PATH * 2];
        lstrcpynA(other, side, dl + 1);
        lstrcatA(other, fd.cFileName);
        lstrcpyA(other + strlen(other) - 6, ".ees");
        if (GetFileAttributesA(other) == INVALID_FILE_ATTRIBUTES) {
            lstrcpyA(other + strlen(other) - 4, ".aimod");
            if (DeleteFileA(other) && g_log) fprintf(g_log, "# persist: removed %s (its save is gone)\n", fd.cFileName);
        }
    }
    if (fh != INVALID_HANDLE_VALUE) FindClose(fh);
}

static int __cdecl hook_save_game(void *file, int a, int b, struct ustring folder) {
    // the folder is passed by value and destroyed by the original: read both names first
    char fname[MAX_PATH] = "", dname[MAX_PATH] = "";
    const char *fp = file ? FIELD(file, 4, const char *) : NULL, *dp = FIELD(folder.raw, 4, const char *);
    if (fp) lstrcpynA(fname, fp, MAX_PATH);
    if (dp) lstrcpynA(dname, dp, MAX_PATH);
    int r = ((int (__cdecl *)(void *, int, int, struct ustring))FN_SAVE_GAME)(file, a, b, folder);
    if (g_log) fprintf(g_log, "# persist: game saved as \"%s\" in \"%s\" (result %d)\n", fname, dname, r);
    if (!r && g_persist && fname[0]) persist_write(dname, fname);   // 0 = saved (else the error dialog 0x65C6A9)
    return r;
}

// a saved game starts (game start hook, after the new-game resets): its state file, if it belongs to it
static void persist_load(const char *name) {
    if (!g_persist || !name || !name[0]) return;
    char ees[MAX_PATH * 2], side[MAX_PATH * 2];
    persist_paths("data\\Saved Games\\", name, ees, side);
    FILE *f = fopen(side, "rb");
    if (!f) { if (g_log) fprintf(g_log, "# persist: no state file for %s\n", name); return; }
    struct persist_head h, now;
    int ok = fread(&h, sizeof h, 1, f) == 1 && !memcmp(h.magic, "EEAIMOD", 8) && h.version == 1 &&
             ees_stamp(ees, &now.ees_size, &now.ees_time) && h.ees_size == now.ees_size &&
             !CompareFileTime(&h.ees_time, &now.ees_time);
    if (!ok) {
        fclose(f);
        if (g_log) fprintf(g_log, "# persist: %s doesn't belong to this save (made without the mod?) - ignored\n", side);
        return;
    }
    int got = 0, skipped = 0;
    char tag[32];
    uint32_t n;
    while (fread(tag, sizeof tag, 1, f) == 1 && fread(&n, sizeof n, 1, f) == 1) {
        tag[31] = 0;
        size_t i = 0;
        while (i < sizeof k_blocks / sizeof *k_blocks && strcmp(k_blocks[i].tag, tag)) i++;
        if (i < sizeof k_blocks / sizeof *k_blocks && k_blocks[i].n == n && fread(k_blocks[i].p, 1, n, f) == n) got++;
        else { skipped++; if (fseek(f, (long)n, SEEK_CUR)) break; }
    }
    fclose(f);
    if (g_log) fprintf(g_log, "# persist: state of %s restored (saved at %.1f min): %d blocks, %d skipped\n", name,
                       h.game_time / 60000.0, got, skipped);
}

// all call sites of the save function: E8 rel32 to it in .text
static void persist_install(const char *ini, const char *test_ini) {
    g_persist = GetPrivateProfileIntA("general", "save_state", 1, ini);
    if (test_ini) g_persist = GetPrivateProfileIntA("autotest", "save_state", g_persist, test_ini);
    uint8_t *base = (uint8_t *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    int sites = 0;
    for (int s = 0; s < nt->FileHeader.NumberOfSections; s++, sec++) {
        if (memcmp(sec->Name, ".text", 6)) continue;   // flagged as data like every section of this exe
        uint8_t *p = base + sec->VirtualAddress, *e = p + sec->Misc.VirtualSize - 5;
        for (; p < e; p++)
            if (*p == 0xE8 && (uintptr_t)p + 5 + *(int32_t *)(p + 1) == FN_SAVE_GAME)
                sites += patch_call((uintptr_t)p, FN_SAVE_GAME, (void *)hook_save_game);
    }
    if (g_log) fprintf(g_log, "# persist: save_state=%d, save hooks %d/4\n", g_persist, sites);
}
