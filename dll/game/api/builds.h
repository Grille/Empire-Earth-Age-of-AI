// THE EXE BUILDS THE MOD KNOWS (exe.c tells which one runs): the spec's values per build ((ee), (aoc): spec/environment.cs), in this order,
// and how a file is known as one (exe_hash). Art of Conquest's values: scripts/re/port_addrs.py. Not ready: the
// player's game runs without the mod, a test run plays it (game_build). The module's one file outside it may use
// (layers.py): the settings program tells the exe next to it by this too (tool/ageofai_config.c, its Game page).
// An exe of another build's code with its import table elsewhere (NeoEE's: the same compile in the copy protection's
// layout) plays that build's column: its import slots are found by name ([import_slot], exe.c's spec_import). A file
// of no row here plays a ready column whose checks all hold in it (exe.c's build_try: the retail 1.04's cracks).
#define ADDR_BUILDS 2   // the spec's columns
internal const char *const k_build_names[ADDR_BUILDS] = { "ee", "aoc" };
#define EXE_BUILDS 4
internal const struct exe_build { uint32_t hash; int col; int ready; const char *what; } k_exe_builds[EXE_BUILDS] = {
    { 0x4FB6A479, 0, 1, "the 2017 update" },           // GOG, Steam, empireearth.eu Vanilla
    { 0x0644BA54, 1, 0, "The Art of Conquest" },       // EE-AOC.exe (GOG, Steam)
    { 0x62BC693E, 0, 1, "NeoEE" },                     // NeoEE's Empire Earth.exe (its installer of 2026)
    { 0xA4FF1307, 1, 0, "NeoEE's Art of Conquest" },   // NeoEE's EE-AOC.exe
};
internal int g_exe_build = -1;   // the running build's column (-1: none known - the mod stays off)
internal int g_exe_kind = -1;    // its exe: k_exe_builds' row
// the exe file whole (malloc'd, *size its size), NULL: not readable or of no sensible size
internal uint8_t *exe_file_read(const char *path, uint32_t *size) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    *size = GetFileSize(f, NULL);
    uint8_t *b = *size != INVALID_FILE_SIZE && *size > 0x400 && *size < (64u << 20) ? malloc(*size) : NULL;
    DWORD got = 0;
    if (b && !(ReadFile(f, b, *size, &got, NULL) && got == *size)) { free(b); b = NULL; }
    CloseHandle(f);
    return b;
}
// a file's build hash: FNV-1a over its sections but .rsrc (0: not a PE file)
internal uint32_t exe_hash(const uint8_t *b, uint32_t size) {
    uint32_t h = 0;
    if (b) {
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
    return h;
}
