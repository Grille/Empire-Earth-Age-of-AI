// AIMOD_CONFIG.EXE: the mod's settings (aimod.ini) in a window - for players who'd rather not edit the file. One tree:
// the sections (rows.h SETTING_SECTIONS: General, Display ... the AI), their groups (SETTING_GROUPS), the settings
// (value, default; an AI setting [ai_base]'s, the medium AI's, and where easy or hard plays another), a master's children
// under it; for the one selected its help - the comment above its key in the shipped config/aimod.ini, built in
// (aimod_config.rc), so it is the text of this version - and an editor: on / off, a choice, or a text field. A value
// is checked as it is typed, by the DLL's own code (settings/settings.c setting_value, the language's x_const_eval:
// 35%, 2 * 60, min(...)), and its mistake told as the game would log it. Saving writes only the keys changed, each
// through WritePrivateProfileString, so the file's comments and order stay; an empty value removes the key (the
// default, or for a difficulty [ai_base]'s value). Keys in the file that are no setting of this version are listed
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
enum { ID_FILTER = 100, ID_LIST, ID_HELP, ID_NAME, ID_FILE, ID_SAVE, ID_CLOSE, ID_UNKNOWN, ID_REMOVE, ID_GAME, ID_GAME_TEXT,
       ID_GAME_APPLY, ID_GAME_RESTORE };

// THE PAGES: a section each (rows.h SETTING_SECTIONS by their order, the test ini's left out) - the tree's sections and
// the AI's difficulty columns; diff >= 0: a difficulty's own section, its rows those of [ai_base]
struct page { const char *title, *sec; int section, diff; };   // section: of k_setting_sections, -1 none
internal struct page g_page_of[SETTING_SECTION_N];
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
        for (int d = 0; d < 3 && t->kind == SECK_DIFF; d++) if (k_set_diff_sec[d] && !lstrcmpA(k_set_diff_sec[d], t->sec)) pg->diff = d;
    }
}
#define TEXT_CAP (MAX_PATH + 8)
#define PLAYER_SLOTS 8   // personality_P1 .. P8: the game's players (P0 is nature)

// THE ENTRIES: a key of a page - a row (a text row per player slot: slot) - with what the file says (has, text) and
// what it said when read (the changes since)
struct entry { int page, row, slot, has, had; char text[TEXT_CAP], was[TEXT_CAP]; };
internal struct entry g_e[1024];
internal int g_ne;
internal int g_all_of[SETTING_N];   // a row's entry on the [ai_base] page (what a difficulty's page falls back to)
internal int g_diff_of[SETTING_N][3];   // a row of [ai_base]: its entries on the difficulties' pages

internal char g_ini_path[MAX_PATH];
internal char *g_shipped;   // the shipped config/aimod.ini (the resource), 0-ended
internal char g_unknown[4096];   // "[sec] key" of the keys that are no setting, ", " between them
internal struct { char sec[16], key[64]; } g_unk[64];
internal int g_nunk, g_remove_unknown;

internal HWND g_wnd, g_filter, g_list, g_help, g_name, g_file_label, g_save, g_close, g_unk_label, g_remove, g_game,
              g_game_text, g_game_apply, g_game_restore;
internal HFONT g_font, g_bold, g_mono;   // g_mono: the help, aligned in the ini for a fixed-width font
internal int g_sel = -1, g_filling;   // g_sel: the selected setting's entry; g_filling: the controls set by the program
internal float g_dpi = 1;
internal RECT g_legend;   // where the colours' legend is painted (layout)

internal int dpi_px(int n) { return (int)(n * g_dpi + 0.5f); }   // n pixels at 96 dpi on this screen
internal const struct setting_row *row_of(const struct entry *e) { return &k_settings[e->row]; }
internal int page_kind(int p) {   // SECK_*
    return g_page_of[p].section < 0 ? -1 : k_setting_sections[g_page_of[p].section].kind;
}
internal const char *sec_of(const struct entry *e) { return g_page_of[e->page].sec; }
internal int on_diff_page(const struct entry *e) { return g_page_of[e->page].diff >= 0; }

// MASTERS AND CHILDREN: a CHILD row follows its master (row a) where the file doesn't set it; the master's entry shows
// its children under it when opened (g_open), and the rows SETTING_UNDER puts there (they keep their own values)
internal int g_open[SETTING_N];
internal int is_child(const struct entry *e) { return row_of(e)->kind == SETK_CHILD; }
internal int under_of(int row) {   // the master a row is shown under: a CHILD's, SETTING_UNDER's, else -1
    if (k_settings[row].kind == SETK_CHILD) return k_settings[row].a;
    for (int i = 0; i < SETTING_UNDER_N; i++) if (k_settings_under[i].row == row) return k_settings_under[i].master;
    return -1;
}
internal int is_master(int row) {
    for (int i = 0; i < SETTING_N; i++) if (under_of(i) == row) return 1;
    return 0;
}
// an entry without a value of its own takes another's: [ai_base]'s on a difficulty's page, a child its master's
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

// what the child comes to while it follows its master (as settings.c setting_follow): 1 on, 0 off; *word the master's
internal int child_follows(const struct entry *e, char *word, int cap) {
    int w = master_word(e, word, cap);
    return w == 0 ? row_of(e)->def : w == 1;
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

// text checked as row r's value: 0 a mistake (why: the game uses the default), CHECK_WARN outside the row's range (why:
// the game uses it anyway), else CHECK_OK; but for a mistake what it comes to (shown: 35%, on, rush)
enum { CHECK_MISTAKE, CHECK_OK, CHECK_WARN };
internal int check(const struct setting_row *r, const char *sec, const char *key, const char *text, char *why, char *shown, int cap) {
    static uint8_t val[TEXT_CAP];
    why[0] = shown[0] = 0;
    g_ini_why = why;
    g_ini_told = 0;
    setting_value(r, g_ini_path, sec, key, text, val);
    g_ini_why = NULL;
    if (g_ini_told == 2) return CHECK_MISTAKE;
    if (r->kind == SETK_FLAG || r->kind == SETK_CHILD) lstrcpynA(shown, *(int *)val ? "on" : "off", cap);
    else if (r->kind == SETK_STR_P) lstrcpynA(shown, (char *)val, cap);
    else setting_text(r, val, shown, cap);
    return g_ini_told == 1 ? CHECK_WARN : CHECK_OK;
}

// what the entry's own text comes to: CHECK_OK (none too), CHECK_WARN or CHECK_MISTAKE; an AI row: the worst of it and
// its difficulties'
internal int entry_level(const struct entry *e);

// the easy AI's own default of the entry's row (an entry of the easy page), NULL none
internal const char *easy_default(const struct entry *e) {
    if (g_page_of[e->page].diff != 0) return NULL;
    for (size_t i = 0; i < sizeof k_settings_easy / sizeof *k_settings_easy; i++)
        if (!lstrcmpA(k_settings_easy[i].key, row_of(e)->key)) return k_settings_easy[i].text;
    return NULL;
}

// what the entry comes to as shown in the list (*grey: not in the file - the default, [ai_base]'s or easy's own)
internal void shown_value(const struct entry *e, char *out, int cap, int *grey) {
    char why[MAX_PATH], key[64];
    key_of(e, key, sizeof key);
    *grey = !e->text[0];
    if (e->text[0]) {
        int c = check(row_of(e), sec_of(e), key, e->text, why, out, cap);
        if (c == CHECK_MISTAKE) snprintf(out, cap, "%s  (a mistake: the default instead)", e->text);
        else if (c == CHECK_WARN && strlen(out) + 28 < (size_t)cap) lstrcatA(out, "  (outside its usual range)");
        return;
    }
    if (on_diff_page(e)) {
        const struct entry *a = &g_e[g_all_of[e->row]];
        char v[TEXT_CAP];
        int g;
        const char *easy = easy_default(e);
        if (!a->text[0] && easy) {   // neither [ai_easy] nor [ai_base]: the easy AI's own (SETTINGS_EASY)
            check(row_of(e), sec_of(e), key, easy, why, v, sizeof v);
            lstrcpynA(out, v, cap);
            return;
        }
        shown_value(a, v, sizeof v, &g);
        lstrcpynA(out, v, cap);
        return;
    }
    if (is_child(e)) {
        char word[32];
        lstrcpynA(out, child_follows(e, word, sizeof word) ? "on" : "off", cap);   // grey: it follows (the status says whom)
        return;
    }
    char def[TEXT_CAP];
    default_text(row_of(e), def, sizeof def);
    if (!check(row_of(e), sec_of(e), key, def, why, out, cap)) lstrcpynA(out, "(the game's own)", cap);   // a default that leaves it: -1
}

// changed from the default: shown bold (a difficulty's page: any value of its own; a child: other than what it would
// follow under its master as it is set now)
internal int is_changed(const struct entry *e) {
    if (!e->text[0]) return 0;
    if (on_diff_page(e)) return 1;
    char def[TEXT_CAP], a[TEXT_CAP], b[TEXT_CAP], why[MAX_PATH], key[64];
    key_of(e, key, sizeof key);
    if (is_child(e)) {
        char word[32];
        if (!check(row_of(e), sec_of(e), key, e->text, why, a, sizeof a)) return 1;
        return strcmp(a, child_follows(e, word, sizeof word) ? "on" : "off") != 0;
    }
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
    memset(g_diff_of, 0xFF, sizeof g_diff_of);   // -1: no section (medium plays [ai_base])
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
                if (page_kind(p) == SECK_DIFF) g_diff_of[i][g_page_of[p].diff] = g_ne - 1;
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
    static char names[4096];   // and the sections that are none (an old [medium]: the medium AI plays [ai_base]), whole
    DWORD n = GetPrivateProfileSectionNamesA(names, sizeof names, g_ini_path);
    for (const char *sec = names; sec < names + n && *sec && g_nunk < 64; sec += strlen(sec) + 1) {
        if (setting_section_known(sec)) continue;
        lstrcpynA(g_unk[g_nunk].sec, sec, sizeof g_unk[0].sec);
        g_unk[g_nunk].key[0] = 0;
        size_t o = strlen(g_unknown);
        snprintf(g_unknown + o, sizeof g_unknown - o, "%s[%s] (the whole section)", g_nunk ? ", " : "", sec);
        g_nunk++;
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
        if (!WritePrivateProfileStringA(g_unk[i].sec, g_unk[i].key[0] ? g_unk[i].key : NULL, NULL, g_ini_path)) bad++;   // a section: all of it
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

// THE TREE: the sections (by their order; the difficulties' own are no sections here but part of the AI's value), in
// them the groups (rows.h SETTING_GROUPS), in those the settings, under a master its children - each a line of the
// list, a node that opens with its triangle. A line's lParam: an entry (>= 0), a section's page (-1 - page) or a group
// (-1000 - group).
enum { NODE_SECTION, NODE_GROUP, NODE_ENTRY };
#define COLS 3   // the columns: setting, value (an AI row: [ai_base]'s, then where easy or hard plays another), default
internal int g_group_of[SETTING_N];   // a row's group, -1 none
internal int g_sec_open[SETTING_SECTION_N + 1], g_grp_open[SETTING_GROUP_N + 1];
internal struct { int kind, id, level; } g_vis[2048];   // the lines shown, as in the list
internal int g_nvis, g_node = -1, g_game_mode;   // g_node: the selected line's lParam

internal LPARAM node_param(int kind, int id) { return kind == NODE_ENTRY ? id : kind == NODE_SECTION ? -1 - id : -1000 - id; }
internal int node_kind(LPARAM p) { return p >= 0 ? NODE_ENTRY : p > -1000 ? NODE_SECTION : NODE_GROUP; }
internal int node_id(LPARAM p) { return p >= 0 ? (int)p : p > -1000 ? (int)(-1 - p) : (int)(-1000 - p); }
internal int is_section_page(int p) { return g_page_of[p].section >= 0 && page_kind(p) != SECK_DIFF; }

internal void make_groups(void) {   // the rows in their order: each in the last group begun in its section
    int cur[SETTING_SECTION_N];
    for (int s = 0; s < SETTING_SECTION_N; s++) cur[s] = -1;
    for (int i = 0; i < SETTING_N; i++) {
        int s = 0;
        while (s < SETTING_SECTION_N - 1 && lstrcmpA(k_setting_sections[s].sec, k_settings[i].sec)) s++;
        for (int g = 0; g < SETTING_GROUP_N; g++) if (k_setting_groups[g].first == i) cur[s] = g;
        g_group_of[i] = cur[s];
    }
}

internal const struct entry *diff_entry(const struct entry *e, int d) {   // an AI row's difficulty d (easy, hard), NULL none
    return row_of(e)->ai && page_kind(e->page) == SECK_ALL && g_diff_of[e->row][d] >= 0 ? &g_e[g_diff_of[e->row][d]] : NULL;
}
internal int entry_level(const struct entry *e) {
    char why[MAX_PATH], shown[TEXT_CAP], key[64];
    int worst = CHECK_OK;
    for (int d = -1; d < 3; d++) {
        const struct entry *x = d < 0 ? e : diff_entry(e, d);
        if (!x || !x->text[0]) continue;
        key_of(x, key, sizeof key);
        int c = check(row_of(x), sec_of(x), key, x->text, why, shown, sizeof shown);
        if (c == CHECK_MISTAKE) return CHECK_MISTAKE;
        if (c == CHECK_WARN) worst = CHECK_WARN;
    }
    return worst;
}
internal int diff_set(const struct entry *e) {   // an AI row whose easy or hard has a value of its own in the file
    for (int d = 0; d < 3; d++) if (diff_entry(e, d) && diff_entry(e, d)->text[0]) return 1;
    return 0;
}

internal void fill_item(int item, LPARAM p) {
    char v[TEXT_CAP] = "", def[TEXT_CAP] = "", why[MAX_PATH], key[64];
    for (int c = 1; c < COLS; c++) ListView_SetItemText(g_list, item, c, "");
    if (node_kind(p) != NODE_ENTRY) return;
    const struct entry *e = &g_e[p];
    int grey;
    key_of(e, key, sizeof key);
    shown_value(e, v, sizeof v, &grey);
    if (is_child(e)) {   // what it comes to under its master as set now
        char word[32];
        lstrcpyA(def, child_follows(e, word, sizeof word) ? "on" : "off");
    } else {
        char d[TEXT_CAP];
        default_text(row_of(e), d, sizeof d);
        if (!check(row_of(e), sec_of(e), key, d, why, def, sizeof def)) lstrcpyA(def, "(the game's own)");
    }
    char base[TEXT_CAP];   // the difficulties that play another value than [ai_base]: 40% (easy: 30%, hard: 55%)
    lstrcpyA(base, v);
    size_t o = strlen(v);
    int n = 0;
    for (int d = 0; d < 3; d++) {
        const struct entry *de = diff_entry(e, d);
        char dv[TEXT_CAP];
        if (!de) continue;
        shown_value(de, dv, sizeof dv, &grey);
        if (!strcmp(dv, base) || o + 8 >= sizeof v) continue;
        o += (size_t)snprintf(v + o, sizeof v - o, "%s%s: %s", n++ ? ", " : " (", k_set_diff[d], dv);
        if (o >= sizeof v) o = sizeof v - 1;
    }
    if (n && o + 1 < sizeof v) lstrcpyA(v + o, ")");
    ListView_SetItemText(g_list, item, 1, v);
    ListView_SetItemText(g_list, item, COLS - 1, def);
}

internal void refill_items(void) {   // every line's values again (a value shows in others: [ai_base] in its difficulties, a master in its children)
    int n = (int)SendMessageA(g_list, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < n; i++) {
        LVITEMA it = { 0 };
        it.mask = LVIF_PARAM;
        it.iItem = i;
        SendMessageA(g_list, LVM_GETITEMA, 0, (LPARAM)&it);
        fill_item(i, it.lParam);
    }
    InvalidateRect(g_list, NULL, FALSE);
}

internal int matches(const struct entry *e, const char *filter) {
    if (!filter[0]) return 1;
    char key[64], help[4096], low[4096];
    key_of(e, key, sizeof key);
    help_of(sec_of(e), row_of(e)->key, row_of(e)->kind == SETK_STR_P, help, sizeof help);
    snprintf(low, sizeof low, "%s %s", key, help);
    CharLowerA(low);
    return strstr(low, filter) != NULL;
}

internal int item_of(LPARAM p) {
    LVFINDINFOA f = { 0 };
    f.flags = LVFI_PARAM;
    f.lParam = p;
    return (int)SendMessageA(g_list, LVM_FINDITEMA, (WPARAM)-1, (LPARAM)&f);
}

internal void show_editors(void);
internal void layout(void);
internal void repaint(void);

internal void add_line(int kind, int id, int level) {
    if (g_nvis < (int)(sizeof g_vis / sizeof *g_vis)) { g_vis[g_nvis].kind = kind; g_vis[g_nvis].id = id; g_vis[g_nvis].level = level; g_nvis++; }
}

// the lines: a closed node's lines left out; with a search, every setting it finds and the nodes above them (open)
internal void fill_list(LPARAM keep) {
    char filter[64];
    static uint8_t found[1024];
    GetWindowTextA(g_filter, filter, sizeof filter);
    CharLowerA(filter);
    for (int i = 0; i < g_ne; i++) found[i] = is_section_page(g_e[i].page) && matches(&g_e[i], filter);
    g_nvis = 0;
    for (int p = 0; p < g_npages; p++) {
        if (!is_section_page(p)) continue;
        int any = 0;
        for (int i = 0; i < g_ne; i++) if (g_e[i].page == p && found[i]) any = 1;
        if (!any) continue;
        add_line(NODE_SECTION, p, 0);
        if (!filter[0] && !g_sec_open[p]) continue;
        int group = -1;
        for (int i = 0; i < g_ne; i++) {
            if (g_e[i].page != p || !found[i]) continue;
            int gr = g_group_of[g_e[i].row];
            if (gr != group) { group = gr; if (gr >= 0) add_line(NODE_GROUP, gr, 1); }
            if (gr >= 0 && !filter[0] && !g_grp_open[gr]) continue;
            int master = under_of(g_e[i].row), shown = 0;   // shown: its master is a line too (a search may leave it out)
            if (master >= 0 && !filter[0] && !g_open[master]) continue;
            for (int j = 0; master >= 0 && j < g_ne; j++) if (g_e[j].page == p && g_e[j].row == master) shown = found[j];
            add_line(NODE_ENTRY, i, (gr >= 0) + 1 + shown);
        }
    }
    SendMessageA(g_list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_list);
    for (int n = 0; n < g_nvis; n++) {
        wchar_t text[128];
        char name[96];
        int o = 0, kind = g_vis[n].kind, id = g_vis[n].id, open = -1;   // open: a node's state, -1 none
        for (int k = 0; k < g_vis[n].level * 4; k++) text[o++] = ' ';
        if (kind == NODE_SECTION) { lstrcpynA(name, g_page_of[id].title, sizeof name); open = g_sec_open[id] || filter[0]; }
        else if (kind == NODE_GROUP) { lstrcpynA(name, k_setting_groups[id].title, sizeof name); open = g_grp_open[id] || filter[0]; }
        else {
            key_of(&g_e[id], name, sizeof name);
            if (is_master(g_e[id].row)) open = g_open[g_e[id].row] || filter[0];
        }
        if (open >= 0) { text[o++] = open ? 0x25BE : 0x25B8; text[o++] = ' '; }   // a triangle down / right
        else if (kind == NODE_ENTRY) { text[o++] = ' '; text[o++] = ' '; text[o++] = ' '; }
        MultiByteToWideChar(CP_ACP, 0, name, -1, text + o, 128 - o);
        LVITEMW it = { 0 };
        it.mask = LVIF_TEXT | LVIF_PARAM;
        it.iItem = n;
        it.pszText = text;
        it.lParam = node_param(kind, id);
        SendMessageW(g_list, LVM_INSERTITEMW, 0, (LPARAM)&it);
        fill_item(n, it.lParam);
    }
    SendMessageA(g_list, WM_SETREDRAW, TRUE, 0);
    int item = item_of(keep);
    if (item < 0) item = 0;
    g_node = g_nvis ? (int)node_param(g_vis[item].kind, g_vis[item].id) : 0;
    g_sel = -1;
    if (g_nvis) {
        ListView_SetItemState(g_list, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g_list, item, FALSE);
    } else show_editors();
}

// a node opened or closed (a section, a group, a master)
internal int is_node(LPARAM p) { return node_kind(p) != NODE_ENTRY || is_master(g_e[p].row); }
internal int *open_of(LPARAM p) {
    return node_kind(p) == NODE_SECTION ? &g_sec_open[node_id(p)] : node_kind(p) == NODE_GROUP ? &g_grp_open[node_id(p)] : &g_open[g_e[p].row];
}
internal void toggle(LPARAM p) {
    if (!is_node(p)) return;
    *open_of(p) ^= 1;
    fill_list(p);
}

// the line above p that holds it (the left arrow key), -1 none
internal int parent_item(int item) {
    if (item < 0 || item >= g_nvis) return -1;
    for (int i = item - 1; i >= 0; i--) if (g_vis[i].level < g_vis[item].level) return i;
    return -1;
}

// THE EDITORS: one for a plain setting; for an AI setting three - every difficulty's ([ai_base], the medium AI's) and
// easy's, hard's own - each a checkbox, a list or a text field, a button (Default / As [ai_base] / Follow) and what it
// comes to
#define EDITORS 3
enum { ED_CHECK, ED_COMBO, ED_EDIT, ED_BUTTON, ED_PARTS = 10 };
#define ID_ED 200   // set k's part: ID_ED + k * ED_PARTS + part
struct editor { HWND label, check, combo, edit, button, status; int entry, level; };   // level: its check (the status's colour)
internal struct editor g_ed[EDITORS];
internal int g_ned;

internal void show_status(int k) {
    const struct entry *e = &g_e[g_ed[k].entry];
    char why[MAX_PATH], shown[TEXT_CAP], msg[MAX_PATH + 64], key[64], word[32];
    key_of(e, key, sizeof key);
    g_ed[k].level = CHECK_OK;
    if (!e->text[0] && is_child(e)) {
        int on = child_follows(e, word, sizeof word);
        snprintf(msg, sizeof msg, "Not in the file: follows %s=%s - %s.", k_settings[row_of(e)->a].key, word, on ? "on" : "off");
    } else if (!e->text[0] && on_diff_page(e)) {
        int grey;
        shown_value(e, shown, sizeof shown, &grey);
        snprintf(msg, sizeof msg, "Not set: %s - %s.", easy_default(e) && !g_e[g_all_of[e->row]].text[0] ? "the easy AI's own default" : "as [ai_base]", shown);
    } else if (!e->text[0]) lstrcpyA(msg, "Not in the file: the default counts.");
    else {
        int c = check(row_of(e), sec_of(e), key, e->text, why, shown, sizeof shown);
        g_ed[k].level = c;
        if (c == CHECK_OK) snprintf(msg, sizeof msg, "= %s", shown);
        else if (c == CHECK_WARN) snprintf(msg, sizeof msg, "= %s - warning: %s; the game uses it anyway.", shown, why);
        else snprintf(msg, sizeof msg, "Mistake: %s - the game uses the default instead.", why);
    }
    SetWindowTextA(g_ed[k].status, msg);
    InvalidateRect(g_ed[k].status, NULL, TRUE);
}

// editor k for its entry: a checkbox (on / off), a list (a choice; for a difficulty on / off too, with "as [ai_base]" first;
// a child: follow its master, off, on), or a text field
internal void show_editor(int k) {
    struct editor *ed = &g_ed[k];
    struct entry *e = &g_e[ed->entry];
    const struct setting_row *r = row_of(e);
    int flag = r->kind == SETK_FLAG || r->kind == SETK_CHILD;
    int box = flag && !inherits(e), list = r->kind == SETK_CHOICE || (flag && inherits(e));
    ShowWindow(ed->label, SW_SHOW);
    ShowWindow(ed->check, box ? SW_SHOW : SW_HIDE);
    ShowWindow(ed->combo, list ? SW_SHOW : SW_HIDE);
    ShowWindow(ed->edit, !box && !list ? SW_SHOW : SW_HIDE);
    ShowWindow(ed->button, SW_SHOW);
    ShowWindow(ed->status, SW_SHOW);
    SetWindowTextA(ed->label, on_diff_page(e) ? g_page_of[e->page].title : r->ai ? "All" : "Value");
    SetWindowTextA(ed->button, on_diff_page(e) ? "As [ai_base]" : is_child(e) ? "Follow" : "Default");
    char def[TEXT_CAP];
    default_text(r, def, sizeof def);
    char bad[MAX_PATH], shown[TEXT_CAP], key[64];
    key_of(e, key, sizeof key);
    // a checkbox or a list shows what the game plays: for a mistake the default (the text field keeps the text, to mend)
    const char *v = e->text[0] && check(r, sec_of(e), key, e->text, bad, shown, sizeof shown) != CHECK_MISTAKE ? e->text : def;
    if (box) {
        double x;
        const char *why;
        SendMessageA(ed->check, BM_SETCHECK, x_const_eval(v, &x, &why) && x != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    } else if (list) {
        SendMessageA(ed->combo, CB_RESETCONTENT, 0, 0);
        int base = inherits(e);
        char follow[96];
        snprintf(follow, sizeof follow, "follow %s", is_child(e) ? k_settings[r->a].key : "");
        if (base) SendMessageA(ed->combo, CB_ADDSTRING, 0, (LPARAM)(on_diff_page(e) ? "as [ai_base]" : follow));
        int sel = base && !e->text[0] ? 0 : -1;
        if (flag) {
            SendMessageA(ed->combo, CB_ADDSTRING, 0, (LPARAM)"off (0)");
            SendMessageA(ed->combo, CB_ADDSTRING, 0, (LPARAM)"on (1)");
            if (sel < 0) { double x; const char *why; sel = base + (x_const_eval(v, &x, &why) && x != 0); }
        } else {
            char name[64];
            for (int i = 0; ; i++) {
                setting_choice_name(r->choices, i, name, sizeof name);
                if (!name[0]) break;
                SendMessageA(ed->combo, CB_ADDSTRING, 0, (LPARAM)name);
                if (sel < 0 && !lstrcmpiA(name, v)) sel = base + i;
            }
        }
        SendMessageA(ed->combo, CB_SETCURSEL, sel < 0 ? 0 : sel, 0);
    } else SetWindowTextA(ed->edit, e->text);
    show_status(k);
}

internal void hide_editor(int k) {
    HWND c[6] = { g_ed[k].label, g_ed[k].check, g_ed[k].combo, g_ed[k].edit, g_ed[k].button, g_ed[k].status };
    for (int i = 0; i < 6; i++) ShowWindow(c[i], SW_HIDE);
}

// the selected line's name, help and editors (a section or a group: what it holds)
internal void show_editors(void) {
    g_filling = 1;
    int old = g_ned;
    g_ned = 0;
    char title[160], help[4096];
    title[0] = help[0] = 0;
    if (g_nvis && node_kind(g_node) != NODE_ENTRY) {
        int kind = node_kind(g_node), id = node_id(g_node), n = 0;
        size_t o = 0;
        for (int i = 0; i < g_ne; i++) {
            if (kind == NODE_SECTION ? g_e[i].page != id : (!is_section_page(g_e[i].page) || g_group_of[g_e[i].row] != id)) continue;
            char key[64];
            key_of(&g_e[i], key, sizeof key);
            if (o + strlen(key) + 4 < sizeof help) o += (size_t)snprintf(help + o, sizeof help - o, "%s%s", n ? ", " : "", key);
            n++;
        }
        snprintf(title, sizeof title, "%s - %d settings (the triangle, a double click or the right arrow key opens it)",
                 kind == NODE_SECTION ? g_page_of[id].title : k_setting_groups[id].title, n);
    } else if (g_sel >= 0) {
        struct entry *e = &g_e[g_sel];
        char key[64];
        key_of(e, key, sizeof key);
        snprintf(title, sizeof title, "[%s] %s", sec_of(e), key);
        help_of(sec_of(e), row_of(e)->key, row_of(e)->kind == SETK_STR_P, help, sizeof help);
        g_ed[g_ned++].entry = g_sel;
        for (int d = 0; d < 3; d++) if (diff_entry(e, d)) g_ed[g_ned++].entry = g_diff_of[e->row][d];
    }
    SetWindowTextA(g_name, title);
    SetWindowTextA(g_help, help);
    for (int k = 0; k < EDITORS; k++) if (k < g_ned) show_editor(k); else hide_editor(k);
    if (g_ned != old) layout();
    else repaint();
    g_filling = 0;
}

// the player changed editor k (part: ED_*): its entry's text from it
internal void editor_changed(int k, int part) {
    if (g_filling || k >= g_ned) return;
    struct editor *ed = &g_ed[k];
    struct entry *e = &g_e[ed->entry];
    const struct setting_row *r = row_of(e);
    if (part == ED_CHECK) lstrcpyA(e->text, SendMessageA(ed->check, BM_GETCHECK, 0, 0) == BST_CHECKED ? "1" : "0");
    else if (part == ED_COMBO) {
        int i = (int)SendMessageA(ed->combo, CB_GETCURSEL, 0, 0) - inherits(e);
        if (i < 0) e->text[0] = 0;
        else if (r->kind == SETK_FLAG || r->kind == SETK_CHILD) lstrcpyA(e->text, i ? "1" : "0");
        else setting_choice_name(r->choices, i, e->text, sizeof e->text);
    } else if (part == ED_EDIT) GetWindowTextA(ed->edit, e->text, sizeof e->text);
    e->has = e->text[0] != 0;
    refill_items();
    for (int j = 0; j < g_ned; j++) show_status(j);   // [ai_base]'s value shows in the difficulties' "as [ai_base]"
    update_title();
}

internal void set_default(int k) {
    if (k >= g_ned) return;
    struct entry *e = &g_e[g_ed[k].entry];
    if (inherits(e) || !e->had) e->text[0] = 0;
    else default_text(row_of(e), e->text, sizeof e->text);   // a key the file has keeps its line, with the default
    e->has = e->text[0] != 0;
    g_filling = 1;
    show_editor(k);
    g_filling = 0;
    editor_changed(k, -1);
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

internal void layout(void) {
    RECT rc;
    GetClientRect(g_wnd, &rc);
    int w = rc.right, h = rc.bottom, m = dpi_px(8), x = m, rw = w - 2 * m, line = dpi_px(30);
    int unk = g_nunk ? dpi_px(40) : 0, bottom = dpi_px(32) + unk, help_h = dpi_px(110);
    int editor = dpi_px(22) + help_h + dpi_px(6) + (g_ned ? g_ned : 1) * line;
    MoveWindow(g_filter, x, m, dpi_px(300), dpi_px(24), FALSE);
    SetRect(&g_legend, x + dpi_px(320), m, w - m, m + dpi_px(24));   // beside the search
    int list_h = h - m - dpi_px(32) - editor - bottom - m;
    MoveWindow(g_list, x, m + dpi_px(32), rw, list_h, FALSE);
    int y = m + dpi_px(32) + list_h + m;
    MoveWindow(g_name, x, y, rw, dpi_px(20), FALSE);
    MoveWindow(g_help, x, y + dpi_px(22), rw, help_h, FALSE);
    for (int k = 0; k < EDITORS; k++) {   // a line each: its label, its control, the button, what it comes to
        struct editor *ed = &g_ed[k];
        int ey = y + dpi_px(22) + help_h + dpi_px(6) + k * line, ex = x + dpi_px(70);
        MoveWindow(ed->label, x, ey + dpi_px(4), dpi_px(66), dpi_px(20), FALSE);
        MoveWindow(ed->check, ex, ey, dpi_px(60), dpi_px(24), FALSE);
        MoveWindow(ed->combo, ex, ey, dpi_px(260), dpi_px(300), FALSE);
        MoveWindow(ed->edit, ex, ey, dpi_px(260), dpi_px(24), FALSE);
        int bx = ex + (IsWindowVisible(ed->check) ? dpi_px(60) : dpi_px(260)) + dpi_px(10);
        MoveWindow(ed->button, bx, ey - dpi_px(1), dpi_px(90), dpi_px(26), FALSE);
        MoveWindow(ed->status, bx + dpi_px(100), ey + dpi_px(4), w - m - bx - dpi_px(100), dpi_px(20), FALSE);
    }
    int by = h - dpi_px(32) - m / 2;
    if (unk) {
        MoveWindow(g_unk_label, m, by - unk, w - 3 * m - dpi_px(110), unk - dpi_px(4), FALSE);
        MoveWindow(g_remove, w - m - dpi_px(110), by - unk, dpi_px(110), dpi_px(26), FALSE);
    }
    MoveWindow(g_game, m, by, dpi_px(150), dpi_px(28), FALSE);
    MoveWindow(g_file_label, m + dpi_px(160), by + dpi_px(5), w - 4 * m - dpi_px(360), dpi_px(20), FALSE);
    MoveWindow(g_save, w - m - dpi_px(196), by, dpi_px(94), dpi_px(28), FALSE);
    MoveWindow(g_close, w - m - dpi_px(96), by, dpi_px(94), dpi_px(28), FALSE);
    int gh = h - 2 * m - bottom - dpi_px(40);   // the game page: its text, the buttons under it
    MoveWindow(g_game_text, x, m, rw, gh, FALSE);
    MoveWindow(g_game_apply, x, m + gh + dpi_px(8), dpi_px(140), dpi_px(28), FALSE);
    MoveWindow(g_game_restore, x + dpi_px(150), m + gh + dpi_px(8), dpi_px(200), dpi_px(28), FALSE);
    int cw = rw - dpi_px(24) - dpi_px(280);   // the setting's column, then the values' share the rest
    ListView_SetColumnWidth(g_list, 0, dpi_px(280));
    ListView_SetColumnWidth(g_list, 1, cw * 65 / 100);
    ListView_SetColumnWidth(g_list, COLS - 1, cw * 35 / 100);
    repaint();
}

internal HWND make_child(const char *cls, const char *text, DWORD style, int id) {
    HWND c = CreateWindowExA(!strcmp(cls, WC_EDITA) || !strcmp(cls, WC_LISTBOXA) ? WS_EX_CLIENTEDGE : 0, cls, text,
                             WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, g_wnd, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
    SendMessageA(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

internal void create_controls(void) {
    g_filter = make_child(WC_EDITA, "", ES_AUTOHSCROLL | WS_TABSTOP, ID_FILTER);
    SendMessageW(g_filter, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search every setting");
    g_list = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT |
                             LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, 0, 10, 10, g_wnd, (HMENU)ID_LIST, GetModuleHandleA(NULL), NULL);
    SendMessageA(g_list, WM_SETFONT, (WPARAM)g_font, TRUE);
    ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    const char *cols[COLS] = { "Setting", "Value", "Default" };
    for (int i = 0; i < COLS; i++) {
        LVCOLUMNA c = { 0 };
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (char *)cols[i];
        c.cx = dpi_px(120);
        SendMessageA(g_list, LVM_INSERTCOLUMNA, i, (LPARAM)&c);
    }
    g_name = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX, ID_NAME);
    SendMessageA(g_name, WM_SETFONT, (WPARAM)g_bold, TRUE);
    g_help = make_child(WC_EDITA, "", ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL, ID_HELP);
    SendMessageA(g_help, WM_SETFONT, (WPARAM)g_mono, TRUE);
    for (int k = 0; k < EDITORS; k++) {
        struct editor *ed = &g_ed[k];
        int id = ID_ED + k * ED_PARTS;
        ed->label = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX, 0);
        ed->check = make_child(WC_BUTTONA, "on", BS_AUTOCHECKBOX | WS_TABSTOP, id + ED_CHECK);
        ed->combo = make_child(WC_COMBOBOXA, "", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, id + ED_COMBO);
        ed->edit = make_child(WC_EDITA, "", ES_AUTOHSCROLL | WS_TABSTOP, id + ED_EDIT);
        ed->button = make_child(WC_BUTTONA, "Default", BS_PUSHBUTTON | WS_TABSTOP, id + ED_BUTTON);
        ed->status = make_child(WC_STATICA, "", SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS, 0);
        hide_editor(k);
    }
    g_game = make_child(WC_BUTTONA, "Game exe...", BS_PUSHBUTTON | WS_TABSTOP, ID_GAME);
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
}

// 1: may close (saved, discarded or nothing to save)
internal int ask_close(void) {
    if (!unsaved()) return 1;
    int a = MessageBoxA(g_wnd, "Save the changes to aimod.ini?", TITLE, MB_YESNOCANCEL | MB_ICONQUESTION);
    return a == IDNO || (a == IDYES && save_file());
}

// the settings, or the game page in their place (the Game exe button)
internal void show_mode(void) {
    int game = g_game_mode;
    HWND side[] = { g_filter, g_list, g_name, g_help };
    for (size_t i = 0; i < sizeof side / sizeof *side; i++) ShowWindow(side[i], game ? SW_HIDE : SW_SHOW);
    for (int k = 0; k < EDITORS; k++) if (game || k >= g_ned) hide_editor(k);
    ShowWindow(g_game_text, game ? SW_SHOW : SW_HIDE);
    ShowWindow(g_game_apply, game ? SW_SHOW : SW_HIDE);
    ShowWindow(g_game_restore, game ? SW_SHOW : SW_HIDE);
    SetWindowTextA(g_game, game ? "Back to the settings" : "Game exe...");
    if (game) game_show();
    else fill_list(g_node);
    repaint();
}

// THE COLOURS: the value cell tinted by its check - a warning (outside its usual range, the game uses it) yellow, a
// mistake (the game uses the default) red; the legend under the list says what each is
#define TINT_WARN RGB(255, 240, 180)
#define TINT_MISTAKE RGB(255, 205, 205)
#define TEXT_WARN RGB(150, 100, 0)
#define TEXT_MISTAKE RGB(190, 0, 0)

// a custom-drawn cell: changed bold (an AI row: easy or hard set too), not in the file grey (a section or group line
// bold); the default never; the value tinted by its check
internal LRESULT draw_cell(NMLVCUSTOMDRAW *cd) {
    LPARAM p = cd->nmcd.lItemlParam;
    int bold = 0, grey = 0;
    COLORREF bk = GetSysColor(COLOR_WINDOW);
    if (node_kind(p) != NODE_ENTRY) bold = 1;
    else if (cd->iSubItem < COLS - 1) {
        const struct entry *e = &g_e[p];
        bold = is_changed(e) || diff_set(e);
        grey = !e->text[0] && !diff_set(e);
        int level = cd->iSubItem == 1 ? entry_level(e) : CHECK_OK;
        if (level == CHECK_WARN) bk = TINT_WARN;
        else if (level == CHECK_MISTAKE) bk = TINT_MISTAKE;
    }
    SelectObject(cd->nmcd.hdc, bold ? g_bold : g_font);
    cd->clrText = GetSysColor(grey ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT);
    cd->clrTextBk = bk;
    return CDRF_NEWFONT;
}

// the legend beside the search (g_legend): a swatch and a word for each look of a value
internal void paint_legend(HDC dc) {
    static const struct { const char *text; int kind; } k[] = {
        { "Bold: changed", 1 }, { "Grey: not in your file, the default", 2 }, { "outside its usual range - used anyway", 3 },
        { "a mistake - the default instead", 4 },
    };
    HFONT old = (HFONT)SelectObject(dc, g_font);
    SetBkMode(dc, TRANSPARENT);
    int x = g_legend.left, y = g_legend.top, h = g_legend.bottom - g_legend.top;
    for (size_t i = 0; i < sizeof k / sizeof *k; i++) {
        SIZE sz;
        if (k[i].kind >= 3) {   // a tinted swatch
            RECT sw = { x, y + h / 2 - dpi_px(6), x + dpi_px(22), y + h / 2 + dpi_px(6) };
            HBRUSH b = CreateSolidBrush(k[i].kind == 3 ? TINT_WARN : TINT_MISTAKE);
            FillRect(dc, &sw, b);
            DeleteObject(b);
            x += dpi_px(28);
        }
        SelectObject(dc, k[i].kind == 1 ? g_bold : g_font);
        SetTextColor(dc, GetSysColor(k[i].kind == 2 ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
        const char *t = k[i].text;
        GetTextExtentPoint32A(dc, t, (int)strlen(t), &sz);
        TextOutA(dc, x, y + (h - sz.cy) / 2, t, (int)strlen(t));
        x += sz.cx + dpi_px(24);
    }
    SelectObject(dc, old);
}

internal LRESULT CALLBACK wnd_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE: if (g_list) layout(); return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(w, &ps);
        if (!g_game_mode) paint_legend(dc);
        EndPaint(w, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:   // an editor's status in its check's colour
        for (int k = 0; k < EDITORS; k++) {
            if ((HWND)lp != g_ed[k].status || g_ed[k].level == CHECK_OK) continue;
            SetTextColor((HDC)wp, g_ed[k].level == CHECK_WARN ? TEXT_WARN : TEXT_MISTAKE);
            SetBkColor((HDC)wp, GetSysColor(COLOR_BTNFACE));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
        break;
    case WM_GETMINMAXINFO: ((MINMAXINFO *)lp)->ptMinTrackSize.x = dpi_px(900); ((MINMAXINFO *)lp)->ptMinTrackSize.y = dpi_px(600); return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id >= ID_ED && id < ID_ED + EDITORS * ED_PARTS) {
            int k = (id - ID_ED) / ED_PARTS, part = (id - ID_ED) % ED_PARTS;
            if ((part == ED_CHECK && code == BN_CLICKED) || (part == ED_COMBO && code == CBN_SELCHANGE) || (part == ED_EDIT && code == EN_CHANGE))
                editor_changed(k, part);
            else if (part == ED_BUTTON && code == BN_CLICKED) set_default(k);
        } else if (id == ID_FILTER && code == EN_CHANGE) fill_list(g_node);
        else if (id == ID_GAME) { g_game_mode ^= 1; show_mode(); }
        else if (id == ID_GAME_APPLY) game_apply();
        else if (id == ID_GAME_RESTORE) game_restore();
        else if (id == ID_REMOVE) { g_remove_unknown = 1; show_unknown(); update_title(); }
        else if (id == ID_SAVE) {
            if (save_file()) { read_file(); show_unknown(); show_mode(); layout(); update_title(); }
        } else if (id == ID_CLOSE || id == IDCANCEL) SendMessageA(w, WM_CLOSE, 0, 0);
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)lp;
        if (n->idFrom != ID_LIST) break;
        if (n->code == LVN_ITEMCHANGED) {
            NMLISTVIEW *v = (NMLISTVIEW *)lp;
            if ((v->uChanged & LVIF_STATE) && (v->uNewState & LVIS_SELECTED)) {
                g_node = (int)v->lParam;
                g_sel = node_kind(v->lParam) == NODE_ENTRY ? (int)v->lParam : -1;
                show_editors();
            }
            return 0;
        }
        // a node opened or closed: a click on its triangle, a double click on it, the right / left arrow key (on a
        // line that is no open node the left key goes to the line that holds it)
        if (n->code == NM_CLICK || n->code == NM_DBLCLK) {
            NMITEMACTIVATE *a = (NMITEMACTIVATE *)lp;
            if (a->iItem >= 0 && a->iItem < g_nvis) {
                int tri = dpi_px(6) + g_vis[a->iItem].level * dpi_px(14) + dpi_px(16);   // about where the triangle ends
                if (n->code == NM_DBLCLK || a->ptAction.x < tri) toggle(g_node);
            }
            return 0;
        }
        if (n->code == LVN_KEYDOWN && g_nvis) {
            WORD k = ((NMLVKEYDOWN *)lp)->wVKey;
            int item = (int)SendMessageA(g_list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (k == VK_RIGHT && is_node(g_node) && !*open_of(g_node)) toggle(g_node);
            else if (k == VK_LEFT && is_node(g_node) && *open_of(g_node)) toggle(g_node);
            else if (k == VK_LEFT && parent_item(item) >= 0) {
                int up = parent_item(item);
                ListView_SetItemState(g_list, up, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(g_list, up, FALSE);
            }
            return 0;
        }
        if (n->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW *cd = (NMLVCUSTOMDRAW *)lp;
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) return CDRF_NOTIFYSUBITEMDRAW;
            if (cd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) return draw_cell(cd);
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
    make_groups();
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
                            dpi_px(1180), dpi_px(760), NULL, NULL, inst, NULL);
    create_controls();
    show_unknown();
    layout();
    fill_list(-1);
    ShowWindow(g_wnd, show);
    SetFocus(g_list);
    MSG m;
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        if (IsDialogMessageA(g_wnd, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
    return 0;
}
