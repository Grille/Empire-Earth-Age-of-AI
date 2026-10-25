// AIMOD_CONFIG.EXE: the mod's settings (aimod.ini) in a window - for players who'd rather not edit the file. A page per
// section (rows.h SETTING_SECTIONS: General, Display ... the AI's [all] and its three difficulties), the page's
// settings in a list (value, default), and for the one selected its help - the comment above its key in the shipped config/aimod.ini, built in
// (aimod_config.rc), so it is the text of this version - and an editor: on / off, a choice, or a text field. A value
// is checked as it is typed, by the DLL's own code (settings/settings.c setting_value, the language's x_const_eval:
// 35%, 2 * 60, min(...)), and its mistake told as the game would log it. Saving writes only the keys changed, each
// through WritePrivateProfileString, so the file's comments and order stay; an empty value removes the key (the
// default, or on a difficulty's page [all]'s value). Keys in the file that are no setting of this version are listed
// and can be removed. Its own translation unit (dll/build.sh), from base/, script/ and settings/ only (layers.py).
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <imagehlp.h>   // CheckSumMappedFile: the exe header's checksum

#include "base/lang.h"
#include "base/log.c"
#include "base/math.c"
#include "base/mistakes.c"
#include "base/ini.c"
#include "script/expr.c"
#include "settings/settings.c"
#include "game/api/builds.h"   // the exe builds the mod knows, and how a file is known as one

#define TITLE "Empire Earth: Age of AI - settings"
enum { ID_PAGES = 100, ID_FILTER, ID_LIST, ID_HELP, ID_NAME, ID_CHECK, ID_COMBO, ID_EDIT, ID_DEFAULT, ID_STATUS, ID_FILE,
       ID_SAVE, ID_CLOSE, ID_UNKNOWN, ID_REMOVE, ID_GAME_TEXT, ID_GAME_APPLY, ID_GAME_RESTORE };

// THE PAGES: a section each (rows.h SETTING_SECTIONS by their order, the test ini's left out), then GAME_PAGE: the exe
// (no settings: its 4 GB flag); diff >= 0: a difficulty's own section, its rows those of [all]
#define GAME_PAGE -2
struct page { const char *title, *sec; int section, diff; };   // section: of k_setting_sections, -1 none
internal struct page g_page_of[SETTING_SECTION_N + 1];
internal int g_npages;

internal void make_pages(void) {
    g_npages = 0;
    for (int s = 0; s < SETTING_SECTION_N; s++) {
        const struct setting_section *t = &k_setting_sections[s];
        if (t->kind == SECK_TEST) continue;
        int at = g_npages++;   // sorted in by order
        while (at > 0 && k_setting_sections[g_page_of[at - 1].section].order > t->order) {
            g_page_of[at] = g_page_of[at - 1];
            at--;
        }
        struct page *pg = &g_page_of[at];
        pg->title = t->title;
        pg->sec = t->sec;
        pg->section = s;
        pg->diff = -1;
        for (int d = 0; d < 3 && t->kind == SECK_DIFF; d++) if (!lstrcmpA(k_set_diff[d], t->sec)) pg->diff = d;
    }
    g_page_of[g_npages++] = (struct page){ "Game (exe)", "", -1, GAME_PAGE };
}
#define TEXT_CAP (MAX_PATH + 8)
#define PLAYER_SLOTS 8   // personality_P1 .. P8: the game's players (P0 is nature)

// THE ENTRIES: a key of a page - a row (a text row per player slot: slot) - with what the file says (has, text) and
// what it said when read (the changes since)
struct entry { int page, row, slot, has, had; char text[TEXT_CAP], was[TEXT_CAP]; };
internal struct entry g_e[1024];
internal int g_ne;
internal int g_all_of[SETTING_N];   // a row's entry on the [all] page (what a difficulty's page falls back to)

internal char g_ini_path[MAX_PATH];
internal char *g_shipped;   // the shipped config/aimod.ini (the resource), 0-ended
internal char g_unknown[4096];   // "[sec] key" of the keys that are no setting, ", " between them
internal struct { char sec[16], key[64]; } g_unk[64];
internal int g_nunk, g_remove_unknown;

internal HWND g_wnd, g_pages, g_filter, g_list, g_help, g_name, g_check, g_combo, g_edit, g_default, g_status, g_file_label,
              g_save, g_close, g_unk_label, g_remove, g_game_text, g_game_apply, g_game_restore;
internal HFONT g_font, g_bold, g_mono;   // g_mono: the help, aligned in the ini for a fixed-width font
internal int g_page, g_sel = -1, g_filling;   // g_filling: the controls set by the program, not the player
internal float g_dpi = 1;

internal int dpi_px(int n) { return (int)(n * g_dpi + 0.5f); }   // n pixels at 96 dpi on this screen
internal const struct setting_row *row_of(const struct entry *e) { return &k_settings[e->row]; }
internal int page_kind(int p) {   // SECK_*, -1 the game page
    return g_page_of[p].section < 0 ? -1 : k_setting_sections[g_page_of[p].section].kind;
}
internal const char *sec_of(const struct entry *e) { return g_page_of[e->page].sec; }
internal int on_diff_page(const struct entry *e) { return g_page_of[e->page].diff >= 0; }

// MASTERS AND CHILDREN: a CHILD row follows its master (row a) where the file doesn't set it; the master's entry shows
// its children under it when opened (g_open)
internal int g_open[SETTING_N];
internal int is_child(const struct entry *e) { return row_of(e)->kind == SETK_CHILD; }
internal int is_master(int row) {
    for (int i = 0; i < SETTING_N; i++) if (k_settings[i].kind == SETK_CHILD && k_settings[i].a == row) return 1;
    return 0;
}
// an entry without a value of its own takes another's: [all]'s on a difficulty's page, a child its master's
internal int inherits(const struct entry *e) { return on_diff_page(e) || is_child(e); }

// the child's master on its page: 0 default, 1 enabled, 2 disabled (its text, else its default); *name: the word
internal int master_word(const struct entry *e, char *name, int cap) {
    const struct setting_row *m = &k_settings[row_of(e)->a];
    const char *text = m->def_s;
    for (int i = 0; i < g_ne; i++)
        if (g_e[i].page == e->page && g_e[i].row == row_of(e)->a && g_e[i].text[0]) text = g_e[i].text;
    int w = setting_choice(m->choices, text);
    if (w < 0) w = 0;   // a mistake: the DLL keeps the master's default
    setting_choice_name(m->choices, w, name, cap);
    return w;
}

internal void key_of(const struct entry *e, char *out, int cap) {
    if (e->slot >= 0) snprintf(out, cap, "%s%d", row_of(e)->key, e->slot);
    else lstrcpynA(out, row_of(e)->key, cap);
}

// the row's default as the file would write it
internal void default_text(const struct setting_row *r, char *out, int cap) {
    switch (r->kind) {
    case SETK_PCT: case SETK_SHARE: snprintf(out, cap, "%d%%", r->def); break;
    case SETK_CHOICE: case SETK_STR: case SETK_STR_P: lstrcpynA(out, r->def_s, cap); break;
    default: snprintf(out, cap, "%d", r->def);
    }
}

// text checked as row r's value: 0 a mistake (why), else what it comes to (shown: 35%, on, rush)
internal int check(const struct setting_row *r, const char *sec, const char *key, const char *text, char *why, char *shown, int cap) {
    static uint8_t val[TEXT_CAP];
    why[0] = shown[0] = 0;
    g_ini_why = why;
    setting_value(r, g_ini_path, sec, key, text, val);
    g_ini_why = NULL;
    if (why[0]) return 0;
    if (r->kind == SETK_FLAG || r->kind == SETK_CHILD) lstrcpynA(shown, *(int *)val ? "on" : "off", cap);
    else if (r->kind == SETK_STR_P) lstrcpynA(shown, (char *)val, cap);
    else setting_text(r, val, shown, cap);
    return 1;
}

// the easy AI's own default of the entry's row (an entry of the easy page), NULL none
internal const char *easy_default(const struct entry *e) {
    if (g_page_of[e->page].diff != 0) return NULL;
    for (size_t i = 0; i < sizeof k_settings_easy / sizeof *k_settings_easy; i++)
        if (!lstrcmpA(k_settings_easy[i].key, row_of(e)->key)) return k_settings_easy[i].text;
    return NULL;
}

// what the entry comes to as shown in the list (*grey: not in the file - the default, or [all]'s)
internal void shown_value(const struct entry *e, char *out, int cap, int *grey) {
    char why[MAX_PATH], key[64];
    key_of(e, key, sizeof key);
    *grey = !e->text[0];
    if (e->text[0]) {
        if (!check(row_of(e), sec_of(e), key, e->text, why, out, cap)) snprintf(out, cap, "%s  (a mistake)", e->text);
        return;
    }
    if (on_diff_page(e)) {
        const struct entry *a = &g_e[g_all_of[e->row]];
        char v[TEXT_CAP];
        int g;
        const char *easy = easy_default(e);
        if (!a->text[0] && easy) {   // neither [easy] nor [all]: the easy AI's own (SETTINGS_EASY)
            check(row_of(e), sec_of(e), key, easy, why, v, sizeof v);
            snprintf(out, cap, "easy's default: %s", v);
            return;
        }
        shown_value(a, v, sizeof v, &g);
        snprintf(out, cap, "as [all]: %s", v);
        return;
    }
    if (is_child(e)) {   // as settings.c setting_follow
        char word[32];
        int w = master_word(e, word, sizeof word), on = w == 0 ? row_of(e)->def : w == 1;
        snprintf(out, cap, "%s (follows %s=%s)", on ? "on" : "off", k_settings[row_of(e)->a].key, word);
        return;
    }
    char def[TEXT_CAP];
    default_text(row_of(e), def, sizeof def);
    if (!check(row_of(e), sec_of(e), key, def, why, out, cap)) lstrcpynA(out, "(the game's own)", cap);   // a default that leaves it: -1
}

// changed from the default: shown bold (a difficulty's page, a child: any value of its own)
internal int is_changed(const struct entry *e) {
    if (!e->text[0]) return 0;
    if (inherits(e)) return 1;
    char def[TEXT_CAP], a[TEXT_CAP], b[TEXT_CAP], why[MAX_PATH], key[64];
    key_of(e, key, sizeof key);
    default_text(row_of(e), def, sizeof def);
    if (!check(row_of(e), sec_of(e), key, e->text, why, a, sizeof a)) return 1;
    check(row_of(e), sec_of(e), key, def, why, b, sizeof b);
    return strcmp(a, b) != 0;
}

internal int unsaved(void) {
    for (int i = 0; i < g_ne; i++) if (g_e[i].has != g_e[i].had || strcmp(g_e[i].text, g_e[i].was)) return 1;
    return g_remove_unknown && g_nunk;
}
internal void update_title(void) { SetWindowTextA(g_wnd, unsaved() ? TITLE " *" : TITLE); }

// THE FILE
internal void read_file(void) {
    g_ne = 0;
    for (int p = 0; p < g_npages; p++)
        for (int i = 0; i < SETTING_N; i++) {
            const struct setting_row *r = &k_settings[i];
            if (g_page_of[p].diff >= 0 ? !r->ai : lstrcmpA(r->sec, g_page_of[p].sec)) continue;
            for (int s = r->kind == SETK_STR_P ? 1 : -1; s <= (r->kind == SETK_STR_P ? PLAYER_SLOTS : -1); s++) {
                struct entry *e = &g_e[g_ne++];
                e->page = p; e->row = i; e->slot = s;
                char key[64];
                key_of(e, key, sizeof key);
                e->has = e->had = ini_raw(g_page_of[p].sec, key, g_ini_path, e->text, sizeof e->text);
                if (!e->has) e->text[0] = 0;
                lstrcpyA(e->was, e->text);
                if (page_kind(p) == SECK_ALL) g_all_of[i] = g_ne - 1;
            }
        }
    g_nunk = 0;
    g_unknown[0] = 0;
    g_remove_unknown = 0;
    for (int p = 0; p < g_npages; p++) {
        if (g_page_of[p].section < 0) continue;
        const char *sec = g_page_of[p].sec;
        static char keys[16384];
        DWORD n = GetPrivateProfileStringA(sec, NULL, "", keys, sizeof keys, g_ini_path);
        for (const char *k = keys; k < keys + n && *k; k += strlen(k) + 1) {
            if (setting_known(sec, k) || g_nunk >= 64) continue;
            lstrcpynA(g_unk[g_nunk].sec, sec, sizeof g_unk[0].sec);
            lstrcpynA(g_unk[g_nunk].key, k, sizeof g_unk[0].key);
            size_t o = strlen(g_unknown);
            snprintf(g_unknown + o, sizeof g_unknown - o, "%s[%s] %s", g_nunk ? ", " : "", sec, k);
            g_nunk++;
        }
    }
}

internal int save_file(void) {
    int bad = 0;
    for (int i = 0; i < g_ne; i++) {
        struct entry *e = &g_e[i];
        if (e->has == e->had && !strcmp(e->text, e->was)) continue;
        char key[64];
        key_of(e, key, sizeof key);
        if (!WritePrivateProfileStringA(sec_of(e), key, e->text[0] ? e->text : NULL, g_ini_path)) bad++;
    }
    for (int i = 0; g_remove_unknown && i < g_nunk; i++)
        if (!WritePrivateProfileStringA(g_unk[i].sec, g_unk[i].key, NULL, g_ini_path)) bad++;
    WritePrivateProfileStringA(NULL, NULL, NULL, g_ini_path);   // flushed
    if (bad) {
        char msg[MAX_PATH + 96];
        snprintf(msg, sizeof msg, "%d setting(s) could not be written to\n%s\n(is the file read-only?)", bad, g_ini_path);
        MessageBoxA(g_wnd, msg, TITLE, MB_OK | MB_ICONERROR);
        return 0;
    }
    return 1;
}

// a line that sets a key, or shows one commented out (;key=...): its key's length, 0 none
internal int key_line(const char *l) {
    if (*l == ';') l++;
    int n = 0;
    while (isalnum((uint8_t)l[n]) || l[n] == '_') n++;
    return n && l[n] == '=' ? n : 0;
}

// THE HELP: the comment lines above key in section sec of the shipped file - above the key's line (key= or the example
// ;key=; slots: key0 .. key15), past the keys right above it that share the comment (chat=, chat_seconds=)
internal void help_of(const char *sec, const char *key, int slots, char *out, int cap) {
    out[0] = 0;
    if (!g_shipped) return;
    static const char *lines[4096];
    int n = 0, in = 0, at = -1;
    size_t kl = strlen(key), sl = strlen(sec);
    for (const char *s = g_shipped; *s && n < 4096; ) {
        const char *l = lines[n] = s;
        const char *e = strchr(s, '\n');
        s = e ? e + 1 : s + strlen(s);
        if (l[0] == '[') in = !_strnicmp(l + 1, sec, sl) && l[1 + sl] == ']';
        else if (in && at < 0 && key_line(l)) {
            const char *k = l[0] == ';' ? l + 1 : l;
            size_t d = kl;
            while (slots && isdigit((uint8_t)k[d])) d++;
            if (!_strnicmp(k, key, kl) && k[d] == '=' && (!slots || d > kl)) at = n;
        }
        n++;
    }
    if (at < 0) return;
    int first = at, top;
    while (first > 0 && key_line(lines[first - 1])) first--;   // the keys right above that share the comment
    for (top = first; top > 0; top--) {
        const char *l = lines[top - 1];
        if (l[0] != ';' || l[1] == '-' || l[1] == '=' || key_line(l)) break;   // a banner, a key, not a comment
    }
    size_t o = 0;
    for (int i = top; i < first && o + 4 < (size_t)cap; i++) {
        const char *l = lines[i] + 1;
        if (*l == ' ') l++;
        size_t len = strcspn(l, "\r\n");
        if (len > (size_t)cap - o - 3) len = (size_t)cap - o - 3;
        memcpy(out + o, l, len);
        o += len;
        out[o++] = '\r';
        out[o++] = '\n';
    }
    out[o] = 0;
    if (!out[0]) lstrcpynA(out, "(no description in config/aimod.ini)", cap);
}

// THE GAME PAGE: the exe next to the program (Empire Earth.exe, else The Art of Conquest's EE-AOC.exe) - which build
// (by builds.h's hash of its code, as the DLL tells it), and its 4 GB flag (Large Address Aware, bit 0x20 of the PE
// header's characteristics): Windows gives a 32-bit program 2 GB of memory unless its exe says it can use more, and
// reads that from the file when the program starts - no DLL can set it later, so this is the one change to the exe the
// mod asks for (everything else it patches in memory while the game runs). Set only on a build the mod knows, the game
// closed, the exe copied first to <exe>.original (kept: Restore puts it back); the header's checksum made anew. The
// code hash leaves the header out, so the mod knows the build afterwards as before.
internal char g_exe[MAX_PATH], g_exe_backup[MAX_PATH + 16];
internal const char *g_exe_name;

internal void exe_find(void) {
    static const char *const names[2] = { "Empire Earth.exe", "EE-AOC.exe" };
    char dir[MAX_PATH];
    lstrcpynA(dir, g_ini_path, sizeof dir);
    char *s = strrchr(dir, '\\');
    if (s) s[1] = 0; else dir[0] = 0;
    g_exe_name = NULL;
    for (int i = 0; i < 2 && !g_exe_name; i++) {
        snprintf(g_exe, sizeof g_exe, "%s%s", dir, names[i]);
        if (GetFileAttributesA(g_exe) != INVALID_FILE_ATTRIBUTES) g_exe_name = names[i];
    }
    snprintf(g_exe_backup, sizeof g_exe_backup, "%s.original", g_exe);
}

// the 4 GB flag of the exe at path: 1 set, 0 not, -1 not a readable PE file
internal int exe_laa(const char *path) {
    uint8_t b[4096];
    DWORD got = 0;
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return -1;
    ReadFile(f, b, sizeof b, &got, NULL);
    CloseHandle(f);
    uint32_t o = got >= 0x40 ? *(uint32_t *)(b + 0x3C) : 0;
    if (!o || o > got - sizeof(IMAGE_NT_HEADERS32) || *(uint32_t *)(b + o) != IMAGE_NT_SIGNATURE) return -1;
    return (((IMAGE_NT_HEADERS32 *)(b + o))->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
}

internal int exe_build_of(uint32_t hash) {   // its index in k_exe_builds, -1 none
    for (int i = 0; i < ADDR_BUILDS; i++) if (k_exe_builds[i].hash == hash) return i;
    return -1;
}

internal void game_show(void) {
    char t[3072];
    exe_find();
    if (!g_exe_name) {
        snprintf(t, sizeof t, "No Empire Earth.exe (or EE-AOC.exe) next to this program (%s).\r\n\r\n"
                 "aimod_config.exe belongs in the game folder, next to the game and aimod.ini.", g_ini_path);
        SetWindowTextA(g_game_text, t);
        EnableWindow(g_game_apply, FALSE);
        EnableWindow(g_game_restore, FALSE);
        return;
    }
    uint32_t hash = exe_build_hash(g_exe);
    g_exe_build = exe_build_of(hash);
    int laa = exe_laa(g_exe), backup = GetFileAttributesA(g_exe_backup) != INVALID_FILE_ATTRIBUTES;
    const char *build = g_exe_build < 0 ? NULL : !lstrcmpA(k_exe_builds[g_exe_build].name, "ee")
                        ? "the 2017 update (1.04: GOG, Steam, empireearth.eu) - the one the mod is made for"
                        : !lstrcmpA(k_exe_builds[g_exe_build].name, "aoc") ? "The Art of Conquest - the mod plays it in test runs only, so far"
                        : k_exe_builds[g_exe_build].name;
    char ver[160];
    if (build) lstrcpynA(ver, build, sizeof ver);
    else snprintf(ver, sizeof ver, "not one the mod knows (code hash %08X): the mod stays off, and this page leaves it alone", (unsigned)hash);
    snprintf(t, sizeof t,
             "%s\r\n%s\r\n\r\n"
             "Version:  %s\r\n\r\n"
             "Memory:   %s\r\n"
             "          A 32-bit game gets 2 GB of memory unless its exe says it can use more; with this flag\r\n"
             "          (Large Address Aware) 64-bit Windows gives it 4 GB. Big armies, big maps and the mod's higher\r\n"
             "          draw limits need it - without it a long game can run out of memory and crash. Windows reads\r\n"
             "          the flag from the file when the game starts, so the mod can't set it while running: setting\r\n"
             "          it changes one bit in the exe's header (and the header's checksum); the game's code stays as\r\n"
             "          it is, and the mod still knows the build.\r\n\r\n"
             "Backup:   %s\r\n",
             g_exe_name, g_exe, ver,
             laa < 0 ? "the file can't be read" : laa ? "4 GB - the flag is set" : "2 GB - the flag is not set",
             backup ? "the exe as it was before this page changed it is kept as " : "none (made before a change)");
    if (backup) {
        size_t o = strlen(t) - 2;
        snprintf(t + o, sizeof t - o, "%s - Restore puts it back\r\n", strrchr(g_exe_backup, '\\') + 1);
    }
    SetWindowTextA(g_game_text, t);
    EnableWindow(g_game_apply, laa == 0 && g_exe_build >= 0);
    EnableWindow(g_game_restore, backup);
}

internal void game_message(const char *what, UINT icon) { MessageBoxA(g_wnd, what, TITLE, MB_OK | icon); }

// the 4 GB flag set: the exe copied first, the file opened alone (the game closed), the bit and the checksum written
internal void game_apply(void) {
    uint32_t hash = exe_build_hash(g_exe);
    if (exe_build_of(hash) < 0) return;
    // the file alone (the game closed) and writable - checked first, then the backup made (CopyFile opens it itself),
    // then opened alone again for the change
    HANDLE f = CreateFileA(g_exe, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
    else {
        game_message(GetLastError() == ERROR_SHARING_VIOLATION ? "The game is running - close Empire Earth first."
                     : "The exe can't be written (a folder that needs administrator rights?).", MB_ICONWARNING);
        return;
    }
    if (GetFileAttributesA(g_exe_backup) == INVALID_FILE_ATTRIBUTES && !CopyFileA(g_exe, g_exe_backup, TRUE)) {
        game_message("The backup of the exe couldn't be made - nothing changed.", MB_ICONERROR);
        return;
    }
    f = CreateFileA(g_exe, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    DWORD size = f != INVALID_HANDLE_VALUE ? GetFileSize(f, NULL) : 0, got = 0;
    uint8_t *b = size != INVALID_FILE_SIZE && size > 0x1000 && size < (64u << 20) ? malloc(size) : NULL;
    int ok = b && ReadFile(f, b, size, &got, NULL) && got == size;
    if (!ok) {
        if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
        free(b);
        game_message("The exe couldn't be read - nothing changed.", MB_ICONERROR);
        return;
    }
    DWORD header_sum, sum;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(b + *(uint32_t *)(b + 0x3C));
    nt->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    if (CheckSumMappedFile(b, size, &header_sum, &sum)) nt->OptionalHeader.CheckSum = sum;
    DWORD put = 0;
    ok = SetFilePointer(f, 0, NULL, FILE_BEGIN) == 0 && WriteFile(f, b, size, &put, NULL) && put == size;
    CloseHandle(f);
    free(b);
    if (!ok || exe_laa(g_exe) != 1 || exe_build_hash(g_exe) != hash) {
        CopyFileA(g_exe_backup, g_exe, FALSE);
        game_message("Writing the exe failed - the backup was put back.", MB_ICONERROR);
    } else game_message("Done: the game gets 4 GB of memory from its next start.", MB_ICONINFORMATION);
    game_show();
}

internal void game_restore(void) {
    char q[MAX_PATH * 2 + 96];
    snprintf(q, sizeof q, "Put the original exe back?\r\n\r\n%s\r\nreplaces\r\n%s", g_exe_backup, g_exe);
    if (MessageBoxA(g_wnd, q, TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    if (!CopyFileA(g_exe_backup, g_exe, FALSE)) {
        game_message(GetLastError() == ERROR_SHARING_VIOLATION ? "The game is running - close Empire Earth first."
                     : "The exe can't be written - nothing changed.", MB_ICONWARNING);
        return;
    }
    DeleteFileA(g_exe_backup);
    game_show();
}

// THE WINDOW
internal void fill_item(int item, const struct entry *e) {
    char key[64], v[TEXT_CAP], def[TEXT_CAP], why[MAX_PATH];
    int grey;
    key_of(e, key, sizeof key);
    shown_value(e, v, sizeof v, &grey);
    if (on_diff_page(e)) lstrcpyA(def, "[all]");
    else if (is_child(e)) lstrcpyA(def, "follow");
    else {
        char d[TEXT_CAP];
        default_text(row_of(e), d, sizeof d);
        if (!check(row_of(e), sec_of(e), key, d, why, def, sizeof def)) lstrcpyA(def, "(the game's own)");
    }
    ListView_SetItemText(g_list, item, 1, v);
    ListView_SetItemText(g_list, item, 2, def);
}

internal int matches(const struct entry *e, const char *filter) {
    if (!filter[0]) return 1;
    char key[64], help[4096], low[4096];
    key_of(e, key, sizeof key);
    help_of(on_diff_page(e) ? "all" : sec_of(e), row_of(e)->key, row_of(e)->kind == SETK_STR_P, help, sizeof help);
    snprintf(low, sizeof low, "%s %s", key, help);
    CharLowerA(low);
    return strstr(low, filter) != NULL;
}

internal void show_editor(void);
internal void place_default(void);
internal void repaint(void);
internal int item_of(int entry);
// the page's entries in the list (those the search finds; a child while its master is open, or found by the search),
// keep: the entry selected afterwards (-1 the first)
internal void fill_list(int keep) {
    char filter[64];
    GetWindowTextA(g_filter, filter, sizeof filter);
    CharLowerA(filter);
    SendMessageA(g_list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_list);
    int n = 0;
    for (int i = 0; i < g_ne; i++) {
        if (g_e[i].page != g_page || !matches(&g_e[i], filter)) continue;
        if (is_child(&g_e[i]) && !filter[0] && !g_open[row_of(&g_e[i])->a]) continue;
        char key[64];
        wchar_t text[80];
        key_of(&g_e[i], key, sizeof key);
        int o = 0;
        if (is_master(g_e[i].row)) { text[o++] = g_open[g_e[i].row] ? 0x25BE : 0x25B8; text[o++] = ' '; }   // a triangle down / right
        else if (is_child(&g_e[i])) for (int k = 0; k < 6; k++) text[o++] = ' ';
        MultiByteToWideChar(CP_ACP, 0, key, -1, text + o, 80 - o);
        LVITEMW it = { 0 };
        it.mask = LVIF_TEXT | LVIF_PARAM;
        it.iItem = n;
        it.pszText = text;
        it.lParam = i;
        SendMessageW(g_list, LVM_INSERTITEMW, 0, (LPARAM)&it);
        fill_item(n++, &g_e[i]);
    }
    SendMessageA(g_list, WM_SETREDRAW, TRUE, 0);
    g_sel = -1;
    int item = keep >= 0 ? item_of(keep) : -1;
    if (item < 0) item = 0;
    if (n) {
        ListView_SetItemState(g_list, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_list, item, FALSE);
    } else show_editor();
}

// a master's children shown or hidden
internal void toggle(int entry) {
    if (entry < 0 || !is_master(g_e[entry].row)) return;
    g_open[g_e[entry].row] ^= 1;
    fill_list(entry);
}

internal int item_of(int entry) {
    LVFINDINFOA f = { 0 };
    f.flags = LVFI_PARAM;
    f.lParam = entry;
    return (int)SendMessageA(g_list, LVM_FINDITEMA, (WPARAM)-1, (LPARAM)&f);
}

internal void show_status(const struct entry *e) {
    char why[MAX_PATH], shown[TEXT_CAP], msg[MAX_PATH + 64], key[64];
    key_of(e, key, sizeof key);
    if (!e->text[0] && is_child(e)) snprintf(msg, sizeof msg, "Not in the file: follows %s.", k_settings[row_of(e)->a].key);
    else if (!e->text[0]) lstrcpyA(msg, on_diff_page(e) ? "Not set here: [all]'s value counts." : "Not in the file: the default counts.");
    else if (check(row_of(e), sec_of(e), key, e->text, why, shown, sizeof shown)) snprintf(msg, sizeof msg, "= %s", shown);
    else snprintf(msg, sizeof msg, "Mistake: %s - the game would use the default.", why);
    SetWindowTextA(g_status, msg);
}

// the editor for the selected entry: a checkbox (on / off), a list (a choice; on a difficulty's page on / off too, with
// "as [all]" first; a child: follow its master, off, on), or a text field
internal void show_editor(void) {
    g_filling = 1;
    int has = g_sel >= 0;
    struct entry *e = has ? &g_e[g_sel] : NULL;
    const struct setting_row *r = has ? row_of(e) : NULL;
    int flag = has && (r->kind == SETK_FLAG || r->kind == SETK_CHILD);
    int box = flag && !inherits(e), list = has && (r->kind == SETK_CHOICE || (flag && inherits(e)));
    ShowWindow(g_check, box ? SW_SHOW : SW_HIDE);
    ShowWindow(g_combo, list ? SW_SHOW : SW_HIDE);
    ShowWindow(g_edit, has && !box && !list ? SW_SHOW : SW_HIDE);
    EnableWindow(g_default, has);
    if (!has) {
        SetWindowTextA(g_name, "");
        SetWindowTextA(g_help, "");
        SetWindowTextA(g_status, "");
        repaint();
        g_filling = 0;
        return;
    }
    char key[64], help[4096], title[128];
    key_of(e, key, sizeof key);
    snprintf(title, sizeof title, "[%s] %s", sec_of(e), key);
    SetWindowTextA(g_name, title);
    help_of(on_diff_page(e) ? "all" : sec_of(e), r->key, r->kind == SETK_STR_P, help, sizeof help);
    SetWindowTextA(g_help, help);
    SetWindowTextA(g_default, on_diff_page(e) ? "As [all]" : is_child(e) ? "Follow" : "Default");
    char def[TEXT_CAP];
    default_text(r, def, sizeof def);
    const char *v = e->text[0] ? e->text : def;
    if (box) {
        double x;
        const char *why;
        SendMessageA(g_check, BM_SETCHECK, x_const_eval(v, &x, &why) && x != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    } else if (list) {
        SendMessageA(g_combo, CB_RESETCONTENT, 0, 0);
        int base = inherits(e);
        char follow[96];
        snprintf(follow, sizeof follow, "follow %s", is_child(e) ? k_settings[r->a].key : "");
        if (base) SendMessageA(g_combo, CB_ADDSTRING, 0, (LPARAM)(on_diff_page(e) ? "as [all]" : follow));
        int sel = base && !e->text[0] ? 0 : -1;
        if (flag) {
            SendMessageA(g_combo, CB_ADDSTRING, 0, (LPARAM)"off (0)");
            SendMessageA(g_combo, CB_ADDSTRING, 0, (LPARAM)"on (1)");
            if (sel < 0) { double x; const char *why; sel = base + (x_const_eval(v, &x, &why) && x != 0); }
        } else {
            char name[64];
            for (int i = 0; ; i++) {
                setting_choice_name(r->choices, i, name, sizeof name);
                if (!name[0]) break;
                SendMessageA(g_combo, CB_ADDSTRING, 0, (LPARAM)name);
                if (sel < 0 && !lstrcmpiA(name, v)) sel = base + i;
            }
        }
        SendMessageA(g_combo, CB_SETCURSEL, sel < 0 ? 0 : sel, 0);
    } else SetWindowTextA(g_edit, e->text);
    show_status(e);
    place_default();
    repaint();
    g_filling = 0;
}

// the player changed the editor: the entry's text from it
internal void editor_changed(int id) {
    if (g_filling || g_sel < 0) return;
    struct entry *e = &g_e[g_sel];
    const struct setting_row *r = row_of(e);
    if (id == ID_CHECK) lstrcpyA(e->text, SendMessageA(g_check, BM_GETCHECK, 0, 0) == BST_CHECKED ? "1" : "0");
    else if (id == ID_COMBO) {
        int i = (int)SendMessageA(g_combo, CB_GETCURSEL, 0, 0) - inherits(e);
        if (i < 0) e->text[0] = 0;
        else if (r->kind == SETK_FLAG || r->kind == SETK_CHILD) lstrcpyA(e->text, i ? "1" : "0");
        else setting_choice_name(r->choices, i, e->text, sizeof e->text);
    } else GetWindowTextA(g_edit, e->text, sizeof e->text);
    e->has = e->text[0] != 0;
    int item = item_of(g_sel);
    if (item >= 0) { fill_item(item, e); ListView_RedrawItems(g_list, item, item); }
    if (!lstrcmpA(sec_of(e), "all"))   // the difficulties' pages show it where they fall back to it
        for (int i = 0; i < g_ne; i++) if (g_e[i].row == e->row && on_diff_page(&g_e[i]) && item_of(i) >= 0) fill_item(item_of(i), &g_e[i]);
    if (is_master(e->row))   // its children show what they follow
        for (int i = 0; i < g_ne; i++)
            if (g_e[i].page == e->page && is_child(&g_e[i]) && row_of(&g_e[i])->a == e->row && item_of(i) >= 0) fill_item(item_of(i), &g_e[i]);
    show_status(e);
    update_title();
}

internal void set_default(void) {
    if (g_sel < 0) return;
    struct entry *e = &g_e[g_sel];
    if (inherits(e) || !e->had) e->text[0] = 0;
    else default_text(row_of(e), e->text, sizeof e->text);   // a key the file has keeps its line, with the default
    e->has = e->text[0] != 0;
    show_editor();
    editor_changed(-1);
    show_editor();
}

internal void show_unknown(void) {
    char msg[sizeof g_unknown + 96];
    if (!g_nunk) msg[0] = 0;
    else if (g_remove_unknown) snprintf(msg, sizeof msg, "Removed when saved: %s", g_unknown);
    else snprintf(msg, sizeof msg, "In your file but no setting of this version (no longer used, or misspelt): %s", g_unknown);
    SetWindowTextA(g_unk_label, msg);
    ShowWindow(g_unk_label, g_nunk ? SW_SHOW : SW_HIDE);
    ShowWindow(g_remove, g_nunk && !g_remove_unknown ? SW_SHOW : SW_HIDE);
}

// the whole window painted again, the controls too: the window doesn't paint under its controls (WS_CLIPCHILDREN, no
// flicker), so where one moved away from or was hidden (a resize, the editor switching from a checkbox to a text
// field) nothing else would paint it - the old picture stayed
internal void repaint(void) { RedrawWindow(g_wnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN); }

// the Default button right of the editor shown
internal void place_default(void) {
    RECT r;
    HWND c = IsWindowVisible(g_check) ? g_check : IsWindowVisible(g_combo) ? g_combo : g_edit;
    GetWindowRect(c, &r);
    MapWindowPoints(NULL, g_wnd, (POINT *)&r, 2);
    MoveWindow(g_default, r.right + dpi_px(10), r.top - dpi_px(1), dpi_px(90), dpi_px(26), FALSE);
}

internal void layout(void) {
    RECT rc;
    GetClientRect(g_wnd, &rc);
    int w = rc.right, h = rc.bottom, m = dpi_px(8), left = dpi_px(170), x = m + left + m, rw = w - x - m;
    int unk = g_nunk ? dpi_px(40) : 0, bottom = dpi_px(32) + unk, editor = dpi_px(200);
    MoveWindow(g_pages, m, m, left, h - 2 * m - bottom, FALSE);
    MoveWindow(g_filter, x, m, dpi_px(260), dpi_px(24), FALSE);
    int list_h = h - m - dpi_px(32) - editor - bottom - m;
    MoveWindow(g_list, x, m + dpi_px(32), rw, list_h, FALSE);
    int y = m + dpi_px(32) + list_h + m;
    MoveWindow(g_name, x, y, rw, dpi_px(20), FALSE);
    MoveWindow(g_help, x, y + dpi_px(22), rw, editor - dpi_px(22) - dpi_px(64), FALSE);
    int ey = y + editor - dpi_px(56);
    MoveWindow(g_check, x, ey, dpi_px(60), dpi_px(24), FALSE);
    MoveWindow(g_combo, x, ey, dpi_px(260), dpi_px(300), FALSE);
    MoveWindow(g_edit, x, ey, dpi_px(360), dpi_px(24), FALSE);
    place_default();
    MoveWindow(g_status, x, ey + dpi_px(30), rw, dpi_px(20), FALSE);
    int by = h - dpi_px(32) - m / 2;
    if (unk) {
        MoveWindow(g_unk_label, m, by - unk, w - 3 * m - dpi_px(110), unk - dpi_px(4), FALSE);
        MoveWindow(g_remove, w - m - dpi_px(110), by - unk, dpi_px(110), dpi_px(26), FALSE);
    }
    MoveWindow(g_file_label, m, by + dpi_px(5), w - 3 * m - dpi_px(200), dpi_px(20), FALSE);
    MoveWindow(g_save, w - m - dpi_px(196), by, dpi_px(94), dpi_px(28), FALSE);
    MoveWindow(g_close, w - m - dpi_px(96), by, dpi_px(94), dpi_px(28), FALSE);
    int gh = h - 2 * m - bottom - dpi_px(40);   // the game page: its text, the buttons under it
    MoveWindow(g_game_text, x, m, rw, gh, FALSE);
    MoveWindow(g_game_apply, x, m + gh + dpi_px(8), dpi_px(140), dpi_px(28), FALSE);
    MoveWindow(g_game_restore, x + dpi_px(150), m + gh + dpi_px(8), dpi_px(200), dpi_px(28), FALSE);
    ListView_SetColumnWidth(g_list, 0, dpi_px(240));
    ListView_SetColumnWidth(g_list, 1, (rw - dpi_px(260)) * 6 / 10);
    ListView_SetColumnWidth(g_list, 2, (rw - dpi_px(260)) * 4 / 10);
    repaint();
}

internal HWND make_child(const char *cls, const char *text, DWORD style, int id) {
    HWND c = CreateWindowExA(!strcmp(cls, WC_EDITA) || !strcmp(cls, WC_LISTBOXA) ? WS_EX_CLIENTEDGE : 0, cls, text,
                             WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, g_wnd, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
    SendMessageA(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

internal void create_controls(void) {
    g_pages = make_child(WC_LISTBOXA, "", LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP, ID_PAGES);
    for (int p = 0; p < g_npages; p++) SendMessageA(g_pages, LB_ADDSTRING, 0, (LPARAM)g_page_of[p].title);
    g_filter = make_child(WC_EDITA, "", ES_AUTOHSCROLL | WS_TABSTOP, ID_FILTER);
    SendMessageW(g_filter, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search this page");
    g_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT |
                             LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, 0, 10, 10, g_wnd, (HMENU)ID_LIST, GetModuleHandleA(NULL), NULL);
    SendMessageA(g_list, WM_SETFONT, (WPARAM)g_font, TRUE);
    ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    const char *cols[3] = { "Setting", "Value", "Default" };
    for (int i = 0; i < 3; i++) {
        LVCOLUMNA c = { 0 };
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (char *)cols[i];
        c.cx = dpi_px(150);
        SendMessageA(g_list, LVM_INSERTCOLUMNA, i, (LPARAM)&c);
    }
    g_name = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX, ID_NAME);
    SendMessageA(g_name, WM_SETFONT, (WPARAM)g_bold, TRUE);
    g_help = make_child(WC_EDITA, "", ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL, ID_HELP);
    SendMessageA(g_help, WM_SETFONT, (WPARAM)g_mono, TRUE);
    g_check = make_child(WC_BUTTONA, "on", BS_AUTOCHECKBOX | WS_TABSTOP, ID_CHECK);
    g_combo = make_child(WC_COMBOBOXA, "", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_COMBO);
    g_edit = make_child(WC_EDITA, "", ES_AUTOHSCROLL | WS_TABSTOP, ID_EDIT);
    g_default = make_child(WC_BUTTONA, "Default", BS_PUSHBUTTON | WS_TABSTOP, ID_DEFAULT);
    g_status = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX, ID_STATUS);
    g_file_label = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX | SS_PATHELLIPSIS, ID_FILE);
    g_save = make_child(WC_BUTTONA, "Save", BS_DEFPUSHBUTTON | WS_TABSTOP, ID_SAVE);
    g_close = make_child(WC_BUTTONA, "Close", BS_PUSHBUTTON | WS_TABSTOP, ID_CLOSE);
    g_unk_label = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX, ID_UNKNOWN);
    g_remove = make_child(WC_BUTTONA, "Remove them", BS_PUSHBUTTON | WS_TABSTOP, ID_REMOVE);
    g_game_text = make_child(WC_EDITA, "", ES_MULTILINE | ES_READONLY | WS_VSCROLL, ID_GAME_TEXT);
    SendMessageA(g_game_text, WM_SETFONT, (WPARAM)g_mono, TRUE);
    g_game_apply = make_child(WC_BUTTONA, "Allow 4 GB", BS_PUSHBUTTON | WS_TABSTOP, ID_GAME_APPLY);
    g_game_restore = make_child(WC_BUTTONA, "Restore the original exe", BS_PUSHBUTTON | WS_TABSTOP, ID_GAME_RESTORE);
    ShowWindow(g_game_text, SW_HIDE);
    ShowWindow(g_game_apply, SW_HIDE);
    ShowWindow(g_game_restore, SW_HIDE);
    char f[MAX_PATH + 16];
    snprintf(f, sizeof f, "File: %s", g_ini_path);
    SetWindowTextA(g_file_label, f);
    SendMessageA(g_pages, LB_SETCURSEL, 0, 0);
}

// 1: may close (saved, discarded or nothing to save)
internal int ask_close(void) {
    if (!unsaved()) return 1;
    int a = MessageBoxA(g_wnd, "Save the changes to aimod.ini?", TITLE, MB_YESNOCANCEL | MB_ICONQUESTION);
    return a == IDNO || (a == IDYES && save_file());
}

// the page g_page shown: its settings, or the game page
internal void show_page(void) {
    int game = g_page_of[g_page].diff == GAME_PAGE;
    HWND list_side[] = { g_filter, g_list, g_name, g_help, g_default, g_status, g_check, g_combo, g_edit };
    for (size_t i = 0; i < sizeof list_side / sizeof *list_side; i++) if (game) ShowWindow(list_side[i], SW_HIDE);
    if (!game) for (int i = 0; i < 6; i++) ShowWindow(list_side[i], SW_SHOW);
    ShowWindow(g_game_text, game ? SW_SHOW : SW_HIDE);
    ShowWindow(g_game_apply, game ? SW_SHOW : SW_HIDE);
    ShowWindow(g_game_restore, game ? SW_SHOW : SW_HIDE);
    if (game) game_show();
    else fill_list(-1);
    repaint();
}

internal LRESULT CALLBACK wnd_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE: if (g_list) layout(); return 0;
    case WM_GETMINMAXINFO: ((MINMAXINFO *)lp)->ptMinTrackSize.x = dpi_px(820); ((MINMAXINFO *)lp)->ptMinTrackSize.y = dpi_px(560); return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == ID_PAGES && code == LBN_SELCHANGE) {
            g_page = (int)SendMessageA(g_pages, LB_GETCURSEL, 0, 0);
            show_page();
        } else if (id == ID_FILTER && code == EN_CHANGE) fill_list(-1);
        else if ((id == ID_CHECK && code == BN_CLICKED) || (id == ID_COMBO && code == CBN_SELCHANGE) || (id == ID_EDIT && code == EN_CHANGE))
            editor_changed(id);
        else if (id == ID_DEFAULT) set_default();
        else if (id == ID_GAME_APPLY) game_apply();
        else if (id == ID_GAME_RESTORE) game_restore();
        else if (id == ID_REMOVE) { g_remove_unknown = 1; show_unknown(); update_title(); }
        else if (id == ID_SAVE) {
            if (save_file()) { read_file(); show_page(); show_unknown(); layout(); update_title(); }
        } else if (id == ID_CLOSE || id == IDCANCEL) SendMessageA(w, WM_CLOSE, 0, 0);
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)lp;
        if (n->idFrom != ID_LIST) break;
        if (n->code == LVN_ITEMCHANGED) {
            NMLISTVIEW *v = (NMLISTVIEW *)lp;
            if ((v->uChanged & LVIF_STATE) && (v->uNewState & LVIS_SELECTED)) { g_sel = (int)v->lParam; show_editor(); }
            return 0;
        }
        // a master opened or closed: a click on its triangle, a double click on it, the right / left arrow key (on a
        // child the left key goes to its master)
        if (n->code == NM_CLICK || n->code == NM_DBLCLK) {
            NMITEMACTIVATE *a = (NMITEMACTIVATE *)lp;
            if (a->iItem >= 0 && g_sel >= 0 && (n->code == NM_DBLCLK || a->ptAction.x < dpi_px(22))) toggle(g_sel);
            return 0;
        }
        if (n->code == LVN_KEYDOWN && g_sel >= 0) {
            WORD k = ((NMLVKEYDOWN *)lp)->wVKey;
            const struct entry *e = &g_e[g_sel];
            if (is_master(e->row) && ((k == VK_RIGHT && !g_open[e->row]) || (k == VK_LEFT && g_open[e->row]))) toggle(g_sel);
            else if (k == VK_LEFT && is_child(e))
                for (int i = 0; i < g_ne; i++) if (g_e[i].page == e->page && g_e[i].row == row_of(e)->a) toggle(i);
            return 0;
        }
        if (n->code == NM_CUSTOMDRAW) {   // changed: bold; not in the file: grey
            NMLVCUSTOMDRAW *cd = (NMLVCUSTOMDRAW *)lp;
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                const struct entry *e = &g_e[cd->nmcd.lItemlParam];
                if (is_changed(e)) SelectObject(cd->nmcd.hdc, g_bold);
                if (!e->text[0]) cd->clrText = GetSysColor(COLOR_GRAYTEXT);
                return CDRF_NEWFONT;
            }
        }
        break;
    }
    case WM_CLOSE: if (ask_close()) DestroyWindow(w); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

// the shipped aimod.ini (resource 101), 0-ended
internal char *shipped_text(void) {
    HRSRC r = FindResourceA(NULL, MAKEINTRESOURCEA(101), MAKEINTRESOURCEA(10));   // RT_RCDATA
    HGLOBAL g = r ? LoadResource(NULL, r) : NULL;
    DWORD n = r ? SizeofResource(NULL, r) : 0;
    if (!g || !n) return NULL;
    char *t = (char *)malloc(n + 1);
    memcpy(t, LockResource(g), n);
    t[n] = 0;
    return t;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    (void)prev;
    g_ini_eval = x_const_eval;
    g_shipped = shipped_text();
    // the file: the one named on the command line, else aimod.ini next to this program (the game folder)
    if (cmd && cmd[0]) {
        lstrcpynA(g_ini_path, cmd[0] == '"' ? cmd + 1 : cmd, sizeof g_ini_path);
        char *q = strchr(g_ini_path, '"');
        if (q) *q = 0;
    } else {
        GetModuleFileNameA(NULL, g_ini_path, sizeof g_ini_path);
        char *s = strrchr(g_ini_path, '\\');
        lstrcpyA(s ? s + 1 : g_ini_path, "aimod.ini");
    }
    if (GetFileAttributesA(g_ini_path) == INVALID_FILE_ATTRIBUTES) {
        char msg[MAX_PATH + 160];
        snprintf(msg, sizeof msg, "%s\nisn't there. Make it with the mod's default settings?\n\n(aimod_config.exe belongs next to "
                 "Empire Earth.exe, where the mod reads aimod.ini.)", g_ini_path);
        if (!g_shipped || MessageBoxA(NULL, msg, TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return 1;
        FILE *f = fopen(g_ini_path, "wb");
        if (!f || fwrite(g_shipped, 1, strlen(g_shipped), f) != strlen(g_shipped)) {
            if (f) fclose(f);
            MessageBoxA(NULL, "The file could not be written.", TITLE, MB_OK | MB_ICONERROR);
            return 1;
        }
        fclose(f);
    }
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    HDC dc = GetDC(NULL);
    g_dpi = GetDeviceCaps(dc, LOGPIXELSY) / 96.0f;
    ReleaseDC(NULL, dc);
    NONCLIENTMETRICSA ncm;
    memset(&ncm, 0, sizeof ncm);
    ncm.cbSize = sizeof ncm;
    SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0);
    g_font = CreateFontIndirectA(&ncm.lfMessageFont);
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    g_bold = CreateFontIndirectA(&ncm.lfMessageFont);
    g_mono = CreateFontA(ncm.lfMessageFont.lfHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
    make_pages();
    read_file();

    WNDCLASSA wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;   // a resize paints the whole window
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIconA(inst, MAKEINTRESOURCEA(1));
    wc.lpszClassName = "AimodConfig";
    RegisterClassA(&wc);
    g_wnd = CreateWindowExA(0, wc.lpszClassName, TITLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                            dpi_px(1040), dpi_px(720), NULL, NULL, inst, NULL);
    create_controls();
    show_unknown();
    layout();
    fill_list(-1);
    ShowWindow(g_wnd, show);
    SetFocus(g_pages);
    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        if (IsDialogMessageA(g_wnd, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
    return 0;
}
