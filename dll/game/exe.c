// THE EXE BUILD. Every address in the mod is the 1.04 exe of the "Difficulty Setting & Multiplayer Lobby" update
// (timestamp 0x594D0000): GOG, Steam (+ a signature and a DPI manifest) and the empireearth.eu "Vanilla" setup have the
// same code - only the resources and the signature differ. Another build would half work or crash (patch_call and
// patch_code32 check their bytes, much else doesn't, and the game's data is read at fixed addresses): the old retail
// 1.04 with its copy protection, its no-CD cracks, NeoEE (the same code, but its imports elsewhere - docs/engine-notes.md
// "Exe builds"). Known by a hash (FNV-1a) of every section but .rsrc, read from the file: in memory another DLL may have
// patched the code already, and run_test.ps1 patches it before the game runs.
// (the builds and their hashes: addr.h k_exe_builds)
static uint32_t exe_build_hash(const char *path) {   // 0: not a readable PE file
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    uint32_t h = 0, size = GetFileSize(f, NULL);
    uint8_t *b = size != INVALID_FILE_SIZE && size > 0x400 && size < (64u << 20) ? malloc(size) : NULL;
    DWORD got = 0;
    if (b && ReadFile(f, b, size, &got, NULL) && got == size) {
        uint32_t o = *(uint32_t *)(b + 0x3C);
        if (o < size - 0xF8 && *(uint32_t *)(b + o) == 0x00004550) {
            IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(b + o);
            IMAGE_SECTION_HEADER *s = (IMAGE_SECTION_HEADER *)((uint8_t *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
            h = 2166136261u;
            for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++) {
                if ((uint8_t *)(s + 1) > b + size) { h = 0; break; }
                if (!memcmp(s->Name, ".rsrc", 6)) continue;
                uint32_t n = s->Misc.VirtualSize && s->Misc.VirtualSize < s->SizeOfRawData ? s->Misc.VirtualSize : s->SizeOfRawData;
                if (s->PointerToRawData > size || n > size - s->PointerToRawData) { h = 0; break; }
                for (const uint8_t *p = b + s->PointerToRawData, *e = p + n; p < e; p++) h = (h ^ *p) * 16777619u;
            }
        }
    }
    free(b);
    CloseHandle(f);
    return h;
}
// 1: the game in a build the mod knows and is ready for (its addresses selected: addr.h); 0: not the game (another
// program loading the folder's DLLs) or a known build that isn't ready (the player's game runs without the mod); -1:
// the game, another build (*hash its hash). A test run (run_test.ps1: EEAIMOD_UNREADY=1) plays a build that isn't
// ready too - how it gets ready.
static int game_build(uint32_t *hash) {
    char path[MAX_PATH], unready[4];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    const char *name = strrchr(path, '\\');
    *hash = exe_build_hash(path);
    int test = GetEnvironmentVariableA("EEAIMOD_UNREADY", unready, sizeof unready) == 1 && unready[0] == '1';
    for (int i = 0; i < ADDR_BUILDS; i++)
        if (*hash == k_exe_builds[i].hash && (k_exe_builds[i].ready || test)) {
            g_addr_missing = addr_select(i);
            g_of_missing = fields_select(i);
            return 1;
        }
    return name && !lstrcmpiA(name + 1, "Empire Earth.exe") ? -1 : 0;   // a renamed copy of a known build counts above
}
// Another build: nothing installed - the player is told once a start (a test run, which has nobody to click: only
// the log), and the game runs as it is
static void build_unknown(uint32_t hash) {
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
