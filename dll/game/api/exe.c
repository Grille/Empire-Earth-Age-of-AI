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
internal int game_build(uint32_t *hash) {
    char path[MAX_PATH], unready[4];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    const char *name = strrchr(path, '\\');
    *hash = exe_build_hash(path);
    int test = GetEnvironmentVariableA("EEAIMOD_UNREADY", unready, sizeof unready) == 1 && unready[0] == '1';
    for (int i = 0; i < ADDR_BUILDS; i++)
        if (*hash == k_exe_builds[i].hash && (k_exe_builds[i].ready || test)) {
            g_exe_build = i;
            g_addr_missing = addr_select();
            g_of_missing = fields_select();
            g_spec_missing = spec_select(i);
            return 1;
        }
    return name && !lstrcmpiA(name + 1, "Empire Earth.exe") ? -1 : 0;   // a renamed copy of a known build counts above
}
// the start's log: the build when it isn't ready, the addresses and fields it hasn't got (what uses them doesn't work)
internal void api_log(FILE *log) {
    if (!log) return;
    if (!k_exe_builds[g_exe_build].ready) fprintf(log, "# build: %s - not ready, a test run's\n", k_exe_builds[g_exe_build].name);
    addr_log(log);
    fields_log(log);
    spec_log(log);
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
