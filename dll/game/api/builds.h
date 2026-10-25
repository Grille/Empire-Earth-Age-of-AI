// THE EXE BUILDS THE MOD KNOWS (exe.c tells which one runs): the spec's values per build ((ee), (aoc): spec/environment.cs), in this order,
// and how a file is known as one (exe_build_hash). Art of Conquest's values: scripts/re/port_addrs.py. Not ready: the
// player's game runs without the mod, a test run plays it (game_build). The module's one file outside it may use
// (layers.py): the settings program tells the exe next to it by this too (tool/aimod_config.c, its Game page).
#define ADDR_BUILDS 2
internal const struct exe_build { uint32_t hash; const char *name; int ready; } k_exe_builds[ADDR_BUILDS] = {
    { 0x4FB6A479, "ee", 1 },    // the 2017 update (GOG, Steam, empireearth.eu Vanilla)
    { 0x0644BA54, "aoc", 0 },   // The Art of Conquest, EE-AOC.exe (GOG, Steam)
};
internal int g_exe_build = -1;   // the running build's column (-1: none known - the mod stays off)
internal uint32_t exe_build_hash(const char *path) {   // 0: not a readable PE file
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
