// THE EXE BUILD. Every address in the mod is the 1.04 exe of the "Difficulty Setting & Multiplayer Lobby" update
// (timestamp 0x594D0000): GOG, Steam (+ a signature and a DPI manifest) and the empireearth.eu "Vanilla" setup have the
// same code - only the resources and the signature differ. Another build would half work or crash (patch_call and
// patch_code32 check their bytes, much else doesn't, and the game's data is read at fixed addresses). NeoEE's exe is
// the same code with its imports elsewhere: it plays the 2017 update's column, its import slots found by name
// (builds.h, spec_import; docs/engine-notes.md "Exe builds"). Known by a hash (FNV-1a) of every section but .rsrc,
// read from the file: in memory another DLL may have patched the code already, and run_test.ps1 patches it before the
// game runs. An exe of no known hash plays a column whose checks all hold in it (build_try): the old retail 1.04 is
// the same compile in its copy protection, a no-CD crack of it that compile with the protection taken off and the
// import table rebuilt in an order of its own - each crack a file of its own, too many for the table. Not the code
// byte for byte, but 147 of the checks are pairs the code must hold in itself (a call of the function at the spec's
// address, a vtable's RTTI name and its slots), spread over the parts the mod uses: another compile fails hundreds.
// (the builds, their hashes and the hash: builds.h)
// the running build by its name ("ee", "aoc"): for code that differs by more than an address (AoC's rewrites)
internal int build_is(const char *name) { return g_exe_build >= 0 && !lstrcmpA(k_build_names[g_exe_build], name); }
// 1: the game in a build the mod knows and is ready for (its column selected: g_exe_build, the tables); 0: not the game (another
// program loading the folder's DLLs) or a known build that isn't ready (the player's game runs without the mod); -1:
// the game, another build (*hash its hash). A test run (run_test.ps1: EEAIMOD_UNREADY=1) plays a build that isn't
// ready too - how it gets ready.
internal int g_spec_missing;   // the spec's values the running build hasn't got (spec_log)
internal int spec_select(int build);   // gen/gen.c: the spec's values of the build
internal void spec_log(FILE *log);
internal int spec_check(void);   // gen/gen.c: what the spec says of the exe, looked at (the helpers below)
internal int spec_imports(void);   // gen/gen.c: the import slots found by name (spec_import)

// THE LOAD-TIME CHECK: the spec's values against the running exe, before anything is patched - a wrong value (a slip
// in the spec, a port gone wrong) shows in the start's log by name, not as a hook that fails to install or a crash. A
// value the build hasn't got (0, a slot NA) is spec_log's and not looked at.
internal int g_spec_checked, g_spec_bad;
internal char g_spec_bad_names[600];
internal int spec_fail(const char *what) {
    size_t n = strlen(g_spec_bad_names);
    if (n + strlen(what) + 6 < sizeof g_spec_bad_names) wsprintfA(g_spec_bad_names + n, " %s", what);
    else if (n + 5 < sizeof g_spec_bad_names && !strstr(g_spec_bad_names, " ...")) lstrcatA(g_spec_bad_names, " ...");
    g_spec_bad++;
    return 1;
}
internal int spec_in_image(uint32_t a, uint32_t n) { return a >= g_image_lo && a + n <= g_image_hi && a + n >= a; }
// a function or a place in the code: in the exe's code section
internal int spec_code(uint32_t a, const char *what) {
    if (!a) return 0;
    g_spec_checked++;
    return a >= g_code_lo && a < g_code_hi ? 0 : spec_fail(what);
}
// a place of the game's data: in the exe's image
internal int spec_data(uint32_t a, const char *what) {
    if (!a) return 0;
    g_spec_checked++;
    return spec_in_image(a, 1) ? 0 : spec_fail(what);
}
// a class's vtable: the RTTI before it names the class (its complete object locator's type descriptor, ".?AVName@@")
internal int spec_vtable(uint32_t vt, const char *rtti, const char *what) {
    if (!vt) return 0;
    g_spec_checked++;
    if (!spec_in_image(vt - 4, 4)) return spec_fail(what);
    uint32_t col = *(uint32_t *)(uintptr_t)(vt - 4);
    if (!spec_in_image(col, 0x10)) return spec_fail(what);
    uint32_t td = *(uint32_t *)(uintptr_t)(col + 0xC);
    if (!spec_in_image(td, 8 + (uint32_t)strlen(rtti) + 1)) return spec_fail(what);
    return strcmp((const char *)(uintptr_t)(td + 8), rtti) ? spec_fail(what) : 0;
}
// a class's own function in its vtable's slot
internal int spec_slot(uint32_t vt, uint32_t slot, uintptr_t fn, const char *what) {
    if (!vt || !fn || slot == 0xFFFFFFFFu) return 0;
    g_spec_checked++;
    if (!spec_in_image(vt + 4 * slot, 4)) return spec_fail(what);
    return *(uint32_t *)(uintptr_t)(vt + 4 * slot) == fn ? 0 : spec_fail(what);
}
// a call site: an E8 rel32 call of its function
internal int spec_call(uint32_t site, uintptr_t fn, const char *what) {
    if (!site || !fn) return 0;
    g_spec_checked++;
    if (site < g_code_lo || site + 5 > g_code_hi) return spec_fail(what);
    const uint8_t *p = (const uint8_t *)(uintptr_t)site;
    return p[0] == 0xE8 && site + 5 + *(const int32_t *)(p + 1) == fn ? 0 : spec_fail(what);
}

// THE IMPORT SLOTS ([import_slot]): found by their DLL and name in the exe file's import table - its name thunks (an
// import descriptor's OriginalFirstThunk, else its FirstThunk: NeoEE's are the slots themselves, overwritten in memory
// at the start), so the file and not the running image. The file is game_build's, read for the hash.
internal const uint8_t *g_exe_file;
internal uint32_t g_exe_file_size;
internal int g_spec_moved;   // import slots found elsewhere than the spec's [address] (another layout of the build)
internal const uint8_t *exe_file_at(uint32_t rva, uint32_t n) {   // a range of the image in the file, NULL: none
    const uint8_t *b = g_exe_file;
    uint32_t o = *(const uint32_t *)(b + 0x3C);
    const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(b + o);
    const IMAGE_SECTION_HEADER *s = (const IMAGE_SECTION_HEADER *)((const uint8_t *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++) {
        if ((const uint8_t *)(s + 1) > b + g_exe_file_size) return NULL;
        if (rva >= s->VirtualAddress && rva - s->VirtualAddress + n <= s->SizeOfRawData && rva - s->VirtualAddress + n >= n) {
            uint32_t f = s->PointerToRawData + (rva - s->VirtualAddress);
            return f <= g_exe_file_size && n <= g_exe_file_size - f ? b + f : NULL;
        }
    }
    return NULL;
}
internal int exe_file_str(uint32_t rva, const char *want) {   // the file's string at rva is want, its case aside
    size_t n = strlen(want);
    const char *p = (const char *)exe_file_at(rva, (uint32_t)n + 1);
    return p && p[n] == 0 && !_strnicmp(p, want, n);
}
internal uint32_t exe_import_slot(const char *dll, const char *name) {   // the slot's address, 0: not imported
    if (!g_exe_file) return 0;
    const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(g_exe_file + *(const uint32_t *)(g_exe_file + 0x3C));
    uint32_t dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    for (const IMAGE_IMPORT_DESCRIPTOR *d; (d = (const IMAGE_IMPORT_DESCRIPTOR *)exe_file_at(dir, sizeof *d)) && d->Name; dir += sizeof *d) {
        if (!exe_file_str(d->Name, dll)) continue;
        uint32_t thunks = d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk;
        for (uint32_t k = 0;; k++) {
            const uint32_t *t = (const uint32_t *)exe_file_at(thunks + 4 * k, 4);
            if (!t || !*t) break;
            if (!(*t & 0x80000000u) && exe_file_str(*t + 2, name)) return nt->OptionalHeader.ImageBase + d->FirstThunk + 4 * k;
        }
    }
    return 0;
}
internal int spec_import(uint32_t *slot, const char *dll, const char *name, const char *what) {
    if (!*slot) return 0;   // a slot the build hasn't got
    g_spec_checked++;
    uint32_t found = exe_import_slot(dll, name);
    if (!found) return spec_fail(what);
    if (found != *slot) { g_spec_moved++; *slot = found; }
    return 0;
}

// the spec's column col against the running exe: its values selected, its import slots looked up in the file, every
// check made (the counts start over: a column tried before leaves nothing)
internal void build_try(int col, const uint8_t *file) {
    g_spec_checked = g_spec_bad = g_spec_moved = 0;
    g_spec_bad_names[0] = 0;
    g_spec_missing = spec_select(col);
    g_exe_file = file;
    spec_imports();   // (before the checks: the slots their exe has)
    g_exe_file = NULL;
    spec_check();   // (its failures: g_spec_bad, api_log tells them)
}
internal uint32_t g_exe_hash;
internal int g_try_bad[ADDR_BUILDS], g_try_checked[ADDR_BUILDS];   // an exe of no known hash: each column's failures (build_unknown)
internal int game_build(uint32_t *hash) {
    char path[MAX_PATH], unready[4];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    const char *name = strrchr(path, '\\');
    uint8_t *file = exe_file_read(path, &g_exe_file_size);
    *hash = g_exe_hash = exe_hash(file, g_exe_file_size);
    int test = GetEnvironmentVariableA("EEAIMOD_UNREADY", unready, sizeof unready) == 1 && unready[0] == '1';
    for (int i = 0; i < EXE_BUILDS; i++)
        if (*hash == k_exe_builds[i].hash) {
            if (!k_exe_builds[i].ready && !test) { free(file); return 0; }
            g_exe_kind = i;
            g_exe_build = k_exe_builds[i].col;
            image_ranges();   // the exe's code and data sections (base/patch.c checks a patch against them)
            build_try(g_exe_build, file);
            free(file);
            return 1;
        }
    // no known file: the first column whose every check holds, of those ready (a row of it is) - a test run tries all
    if (file && *hash) {
        char closest[sizeof g_spec_bad_names] = "";   // the failures of the column nearest to holding (build_unknown)
        int best = -1;
        image_ranges();
        for (int c = 0; c < ADDR_BUILDS; c++) {
            int ready = test;
            for (int i = 0; i < EXE_BUILDS; i++) ready |= k_exe_builds[i].col == c && k_exe_builds[i].ready;
            if (!ready) continue;
            g_exe_build = c;
            build_try(c, file);
            g_try_bad[c] = g_spec_bad;
            g_try_checked[c] = g_spec_checked;
            if (!g_spec_bad && g_spec_checked) { free(file); return 1; }
            if (best < 0 || g_spec_bad < g_try_bad[best]) { best = c; lstrcpyA(closest, g_spec_bad_names); }
        }
        lstrcpyA(g_spec_bad_names, closest);
        g_exe_build = -1;
    }
    free(file);
    // a renamed copy of a known build counts above; a test run's exe is the game whatever its name
    return name && (!lstrcmpiA(name + 1, "Empire Earth.exe") || test) ? -1 : 0;
}
// the start's log: the build when it isn't ready, the addresses and fields it hasn't got (what uses them doesn't work)
internal void api_log(FILE *log) {
    if (!log) return;
    if (g_exe_kind < 0) fprintf(log, "# build: an exe of no known file (code %08X) - %s's code by the checks below\n", g_exe_hash, k_build_names[g_exe_build]);
    else if (!k_exe_builds[g_exe_kind].ready) fprintf(log, "# build: %s - not ready, a test run's\n", k_exe_builds[g_exe_kind].what);
    if (g_exe_kind >= 0 && k_exe_builds[g_exe_kind].col != g_exe_kind) fprintf(log, "# build: %s (%s's code)\n", k_exe_builds[g_exe_kind].what, k_build_names[g_exe_build]);
    if (g_spec_moved) fprintf(log, "# spec: %d import slots elsewhere than the spec's, found by name\n", g_spec_moved);
    spec_log(log);
    if (g_spec_bad) fprintf(log, "# spec: %d of %d checks against the exe fail:%s\n", g_spec_bad, g_spec_checked, g_spec_bad_names);
    else fprintf(log, "# spec: %d checks against the exe, all hold\n", g_spec_checked);
}
// Another build: nothing installed - the player is told once a start (a test run, which has nobody to click: only
// the log), and the game runs as it is
internal void build_unknown(uint32_t hash) {
    char dir[MAX_PATH], path[MAX_PATH + 32];
    // the engine loads the folder's DLLs several times (it frees those that are no render plugin): told once - the
    // process's environment outlives the DLL
    if (GetEnvironmentVariableA("EEAIMOD_BUILD_TOLD", dir, sizeof dir)) return;
    SetEnvironmentVariableA("EEAIMOD_BUILD_TOLD", "1");
    GetModuleFileNameA(NULL, dir, MAX_PATH);   // the game folder (the exe's)
    char *slash = strrchr(dir, '\\');
    if (slash) slash[1] = 0; else dir[0] = 0;
    slot_dir(dir);
    wsprintfA(path, "%saimod_econ.csv", dir);
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "# eeaimod: Empire Earth.exe is another build (code %08X) - nothing installed:", hash);
        for (int c = 0, n = 0; c < ADDR_BUILDS; c++)
            if (g_try_checked[c]) fprintf(f, "%s %d of %d checks of %s's code fail", n++ ? "," : "", g_try_bad[c], g_try_checked[c], k_build_names[c]);
        fprintf(f, " -%s\n", g_spec_bad_names);   // (the closest column's)
        fclose(f);
    }
    wsprintfA(path, "%saimod_autotest.ini", dir);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return;
    char msg[700];
    wsprintfA(msg, "The Empire Earth AI mod (eeaimod.dll) doesn't know this version of Empire Earth.exe (code %08X): "
                   "its code isn't the one the mod is made for, so the mod stays switched off - the game starts without it.\n\n"
                   "The mod works with Empire Earth 1.04: the GOG and Steam versions, the Vanilla setup from empireearth.eu, "
                   "NeoEE, and other copies of 1.04 with the same code (the retail game's without its copy protection). "
                   "Versions before 1.04 have their code in other places.\n\nDelete eeaimod.dll to stop this message.", hash);
    MessageBoxA(NULL, msg, "Empire Earth AI mod: unknown game version", MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
}
