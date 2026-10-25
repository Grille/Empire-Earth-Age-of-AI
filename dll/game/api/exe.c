// THE EXE BUILD. Every address in the mod is the 1.04 exe of the "Difficulty Setting & Multiplayer Lobby" update
// (timestamp 0x594D0000): GOG, Steam (+ a signature and a DPI manifest) and the empireearth.eu "Vanilla" setup have the
// same code - only the resources and the signature differ. Another build would half work or crash (patch_call and
// patch_code32 check their bytes, much else doesn't, and the game's data is read at fixed addresses): the old retail
// 1.04 with its copy protection, its no-CD cracks, NeoEE (the same code, but its imports elsewhere - docs/engine-notes.md
// "Exe builds"). Known by a hash (FNV-1a) of every section but .rsrc, read from the file: in memory another DLL may have
// patched the code already, and run_test.ps1 patches it before the game runs.
// (the builds, their hashes and the hash: builds.h)
// the running build by its name ("ee", "aoc"): for code that differs by more than an address (AoC's rewrites)
internal int build_is(const char *name) { return g_exe_build >= 0 && !lstrcmpA(k_exe_builds[g_exe_build].name, name); }
// 1: the game in a build the mod knows and is ready for (its column selected: g_exe_build, the tables); 0: not the game (another
// program loading the folder's DLLs) or a known build that isn't ready (the player's game runs without the mod); -1:
// the game, another build (*hash its hash). A test run (run_test.ps1: EEAIMOD_UNREADY=1) plays a build that isn't
// ready too - how it gets ready.
internal int g_spec_missing;   // the spec's values the running build hasn't got (spec_log)
internal int spec_select(int build);   // gen/gen.c: the spec's values of the build
internal void spec_log(FILE *log);
internal int spec_check(void);   // gen/gen.c: what the spec says of the exe, looked at (the helpers below)

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

internal int game_build(uint32_t *hash) {
    char path[MAX_PATH], unready[4];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    const char *name = strrchr(path, '\\');
    *hash = exe_build_hash(path);
    int test = GetEnvironmentVariableA("EEAIMOD_UNREADY", unready, sizeof unready) == 1 && unready[0] == '1';
    for (int i = 0; i < ADDR_BUILDS; i++)
        if (*hash == k_exe_builds[i].hash && (k_exe_builds[i].ready || test)) {
            g_exe_build = i;
            image_ranges();   // the exe's code and data sections (base/patch.c checks a patch against them)
            g_spec_missing = spec_select(i);
            spec_check();   // (its failures: g_spec_bad, api_log tells them)
            return 1;
        }
    return name && !lstrcmpiA(name + 1, "Empire Earth.exe") ? -1 : 0;   // a renamed copy of a known build counts above
}
// the start's log: the build when it isn't ready, the addresses and fields it hasn't got (what uses them doesn't work)
internal void api_log(FILE *log) {
    if (!log) return;
    if (!k_exe_builds[g_exe_build].ready) fprintf(log, "# build: %s - not ready, a test run's\n", k_exe_builds[g_exe_build].name);
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
    if (f) { fprintf(f, "# eeaimod: Empire Earth.exe is another build (code %08X) - nothing installed\n", hash); fclose(f); }
    wsprintfA(path, "%saimod_autotest.ini", dir);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return;
    char msg[700];
    wsprintfA(msg, "The Empire Earth AI mod (eeaimod.dll) doesn't know this version of Empire Earth.exe (code %08X), "
                   "so it stays switched off - the game starts without it.\n\n"
                   "The mod works with Empire Earth 1.04 with the 2017 update (\"Difficulty Setting & Multiplayer Lobby\"): "
                   "the GOG and Steam versions and the Vanilla setup from empireearth.eu. Old CD versions, no-CD exes and "
                   "NeoEE have their code in other places.\n\nDelete eeaimod.dll to stop this message.", hash);
    MessageBoxA(NULL, msg, "Empire Earth AI mod: unknown game version", MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
}
