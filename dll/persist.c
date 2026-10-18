// SAVED GAMES: the mod's own state goes into a file next to the save. Included by eeaimod.c after diplo.c.
//
// The game saves its own AI managers (build, civilization, research, resource, unit, wall), the stances, names and
// colours; what the mod rebuilds by itself (income rates, defence layout, siege, threat) needs nothing. Lost without
// this file: opinions and grudges, alliance times, who capitulated to whom, pending offers and promised tribute, the
// "already said" flags of the situations, the army accounts of the budget, the language model's memories (what it
// and you, and the AIs among themselves, said - block llm_memory).
//
// Every save goes through 0x540ACA cdecl(UString *file, int, int, UString folder by value) -> 0 on success
// (save dialog EEASaveGame::vf1, autosave, quicksave, the network save message: 4 call sites, all redirected). After a
// successful save <game>\<folder><file without .ees>.aimod is written: a header with the .ees file's size and write
// time, then named blocks {tag, size, layout, bytes}. On a saved-game start (game start hook, mode 2) the matching
// .aimod is read after the new-game resets: only if size and write time still match the .ees (a save made later
// without the mod leaves a stale file behind), and only blocks whose tag, size and layout match (a newer DLL with
// other layouts just starts fresh for those). All times inside are game times, which the save restores.
// Layout: the size alone let a struct changed to the same size through (fields reordered, int <-> float) - its
// bytes became other values. Each block carries a hash of its variable's definition, the structs it names and the
// macros of its sizes (scripts/persist_layout.py -> persist_layout.h, run by build.sh). Files of format 1 (no
// layout word: the saves made before it) are read as they always were, blocks matched by tag and size only.
#define FN_SAVE_GAME 0x00540ACA
static int g_persist = 1;   // aimod.ini [general] save_state
static const struct { const char *tag; void *p; size_t n; } k_blocks[] = {
#define B(x) { #x, &x, sizeof x }
    B(g_diplo_seen), B(g_diplo_prev), B(g_diplo_due), B(g_war_since), B(g_ally_since), B(g_peace_due),
    B(g_peace_refused), B(g_offer_t), B(g_offer_last), B(g_opinion), B(g_war_betrayal), B(g_diplo_answers),
    B(g_pressure), B(g_offer_kind), B(g_promise), B(g_capit_last), B(g_lord), B(g_own_prev), B(g_ws), B(g_sit),
    B(g_no_citizens_since), B(g_bud), B(g_talk_last), B(g_talk_any), B(g_offer_ignored), B(g_escrow), B(g_lord_since),
    B(g_kills), B(g_kills_min), B(g_kills_min_t), B(g_attack_sent), B(g_hist), B(g_offer_vs), B(g_hlog), B(g_hum),
    B(g_nv_acc),   // navy.c: the navy account (the map kind is found again)
    B(g_intel),   // intel.c: what each AI remembers (honest_vision) - unit pointers dropped on load, buildings matched again
#undef B
};
static uint8_t *llm_mem_save(uint32_t *size);              // llm.c: the language model's memories (variable size)
static void llm_mem_load(const uint8_t *p, uint32_t size);
struct persist_head { char magic[8]; uint32_t version, game_time; uint64_t ees_size; FILETIME ees_time; };
#define PERSIST_FORMAT 2   // 1: blocks {tag[32], size, bytes}; 2: {tag[32], size, layout, bytes}
#define PERSIST_MAX_BLOCK (64u << 20)   // more is a broken file (the largest block, g_intel, is ~0.9 MB)
#include "persist_layout.h"
static uint32_t block_layout(const char *tag) {   // 0: no hash (llm_memory: its own format, checked as it is read)
    for (size_t i = 0; i < sizeof k_layout / sizeof *k_layout; i++) if (!strcmp(k_layout[i].tag, tag)) return k_layout[i].hash;
    return 0;
}

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
    lstrcpynA(dir, g_game_dir, MAX_PATH);   // not the config's folder: under autotest that can be _aimod\config
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
    struct persist_head h = { "EEAIMOD", PERSIST_FORMAT, game_time(), 0, { 0, 0 } };
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
        uint32_t n = (uint32_t)k_blocks[i].n, lay = block_layout(tag);
        fwrite(tag, sizeof tag, 1, f);
        fwrite(&n, sizeof n, 1, f);
        fwrite(&lay, sizeof lay, 1, f);
        fwrite(k_blocks[i].p, 1, n, f);
    }
    uint32_t mn;
    uint8_t *mp = llm_mem_save(&mn);
    if (mp) {
        char tag[32] = "llm_memory";
        uint32_t lay = 0;
        fwrite(tag, sizeof tag, 1, f);
        fwrite(&mn, sizeof mn, 1, f);
        fwrite(&lay, sizeof lay, 1, f);
        fwrite(mp, 1, mn, f);
        free(mp);
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
    int ok = fread(&h, sizeof h, 1, f) == 1 && !memcmp(h.magic, "EEAIMOD", 8);
    if (ok && h.version != 1 && h.version != PERSIST_FORMAT) {   // a newer mod's file: its blocks can't be read
        fclose(f);
        if (g_log) fprintf(g_log, "# persist: %s has format %u (this mod reads 1 and %d) - ignored, the AIs start fresh\n",
                           side, h.version, PERSIST_FORMAT);
        return;
    }
    int v1 = ok && h.version == 1;   // format 1 (saves made before): no layout word, blocks matched by tag and size
    ok = ok && ees_stamp(ees, &now.ees_size, &now.ees_time) && h.ees_size == now.ees_size &&
         !CompareFileTime(&h.ees_time, &now.ees_time);
    if (!ok) {
        fclose(f);
        if (g_log) fprintf(g_log, "# persist: %s doesn't belong to this save (made without the mod?) - ignored\n", side);
        return;
    }
    int got = 0, skipped = 0, broken = 0;
    char tag[32];
    uint32_t n, lay = 0;
    if (g_log) fprintf(g_log, "# persist: %s has format %u (blocks matched by tag, size%s)\n", side, h.version,
                       v1 ? " - no layout check" : " and layout");
    while (fread(tag, sizeof tag, 1, f) == 1 && fread(&n, sizeof n, 1, f) == 1 && (v1 || fread(&lay, sizeof lay, 1, f) == 1)) {
        tag[31] = 0;
        if (n > PERSIST_MAX_BLOCK) { broken = 1; break; }
        // read whole before it is used: a block cut short (a broken file) left half old, half new state
        uint8_t *mp = malloc(n ? n : 1);
        if (!mp || fread(mp, 1, n, f) != n) { free(mp); broken = 1; break; }
        size_t i = 0;
        while (i < sizeof k_blocks / sizeof *k_blocks && strcmp(k_blocks[i].tag, tag)) i++;
        if (!strcmp(tag, "llm_memory")) { llm_mem_load(mp, n); got++; }   // the model's memories: their own format
        else if (i < sizeof k_blocks / sizeof *k_blocks && k_blocks[i].n == n && (v1 || lay == block_layout(tag))) {
            memcpy(k_blocks[i].p, mp, n);
            got++;
        } else {
            skipped++;
            if (g_log) fprintf(g_log, "# persist: block %s skipped (%s)\n", tag, i == sizeof k_blocks / sizeof *k_blocks ?
                               "unknown" : k_blocks[i].n != n ? "another size" : "another layout");
        }
        free(mp);
    }
    fclose(f);
    if (broken && g_log) fprintf(g_log, "# persist: %s ends in a broken block - the rest left out\n", side);
    intel_loaded();
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
