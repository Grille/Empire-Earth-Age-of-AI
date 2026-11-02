// THE GAME'S SETTINGS IN A FILE (the Game page; ageofai_config.exe --settings-file on|off): the game reads its settings
// from the registry before the engine's DLL scan loads ageofai.dll, so the mod can only keep them in a file of its
// own (<game>\ageofai\eesettings.ini, dll/engine/game_settings.c) when Windows loads it earlier - as an import of
// Low-Level Engine.dll, the engine file that reads them. This adds that import: a section of its own (.ageofai) at
// the file's end with the engine's import table copied, one entry more - ageofai.dll!ageofai_settings - and the
// header pointed at it; nothing of the engine's code or data moves. The file is copied first to
// <engine>.original (Restore and "off" put it back), the game closed, the header's checksum made anew. Undone by a
// game's "verify files" too (Steam, GOG): the mod then simply loads late again, the settings in the registry. With
// the import, ageofai.dll must stay: Windows won't start the engine without it (README "Installing").
#define LLE_FILE "Low-Level Engine.dll"
#define LLE_IMPORT "ageofai.dll"
#define LLE_FN "ageofai_settings"
internal uint32_t pe_align(uint32_t v, uint32_t a) { return a ? (v + a - 1) / a * a : v; }
// the file's headers when it is a 32-bit PE with its sections in it, else NULL
internal IMAGE_NT_HEADERS32 *pe_nt(uint8_t *b, uint32_t size) {
    uint32_t o = size >= 0x40 ? *(uint32_t *)(b + 0x3C) : 0;
    if (!o || o > size - sizeof(IMAGE_NT_HEADERS32) || *(uint32_t *)(b + o) != IMAGE_NT_SIGNATURE) return NULL;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(b + o);
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return NULL;
    uint8_t *sec_end = (uint8_t *)(IMAGE_FIRST_SECTION(nt) + nt->FileHeader.NumberOfSections);
    return sec_end <= b + size ? nt : NULL;
}
internal uint8_t *pe_at(uint8_t *b, uint32_t size, IMAGE_NT_HEADERS32 *nt, uint32_t rva) {   // rva in the file, NULL: none
    IMAGE_SECTION_HEADER *s = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++)
        if (rva >= s->VirtualAddress && rva < s->VirtualAddress + s->SizeOfRawData) {
            uint32_t o = s->PointerToRawData + (rva - s->VirtualAddress);
            return o < size ? b + o : NULL;
        }
    return NULL;
}
// the import descriptors' count (without the closing empty one), -1 not readable; *has: one of them is dll's
internal int pe_imports(uint8_t *b, uint32_t size, IMAGE_NT_HEADERS32 *nt, const char *dll, int *has) {
    IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)pe_at(b, size, nt, dd.VirtualAddress);
    int n = 0;
    *has = 0;
    if (!d) return -1;
    for (; (uint8_t *)(d + 1) <= b + size && d->Name; d++, n++) {
        const char *name = (const char *)pe_at(b, size, nt, d->Name);
        if (name && !lstrcmpiA(name, dll)) *has = 1;
    }
    return (uint8_t *)(d + 1) <= b + size ? n : -1;
}
// 1 the import is there, 0 not, -1 the file isn't there or no engine file this can read
internal int lle_state(const char *path) {
    uint32_t size = 0;
    uint8_t *b = exe_file_read(path, &size);
    IMAGE_NT_HEADERS32 *nt = b ? pe_nt(b, size) : NULL;
    int has = 0, n = nt ? pe_imports(b, size, nt, LLE_IMPORT, &has) : -1;
    free(b);
    return n < 0 ? -1 : has;
}
// the file with the import added (malloc'd, *out its size), NULL: it can't take one (*why)
internal uint8_t *lle_with_import(uint8_t *b, uint32_t size, uint32_t *out, const char **why) {
    IMAGE_NT_HEADERS32 *nt = pe_nt(b, size);
    int has, nd = nt ? pe_imports(b, size, nt, LLE_IMPORT, &has) : -1;
    if (nd < 0) { *why = "not a 32-bit program file with an import table"; return NULL; }
    if (has) { *why = "the import is there already"; return NULL; }
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    int ns = nt->FileHeader.NumberOfSections;
    uint32_t hdr_end = (uint32_t)((uint8_t *)(sec + ns + 1) - b), first_raw = size, va_end = 0, raw_end = 0;
    for (int i = 0; i < ns; i++) {
        if (sec[i].PointerToRawData && sec[i].PointerToRawData < first_raw) first_raw = sec[i].PointerToRawData;
        uint32_t v = sec[i].VirtualAddress + (sec[i].Misc.VirtualSize > sec[i].SizeOfRawData ? sec[i].Misc.VirtualSize : sec[i].SizeOfRawData);
        if (v > va_end) va_end = v;
        if (sec[i].PointerToRawData + sec[i].SizeOfRawData > raw_end) raw_end = sec[i].PointerToRawData + sec[i].SizeOfRawData;
    }
    if (hdr_end > first_raw || hdr_end > nt->OptionalHeader.SizeOfHeaders) { *why = "no room for a section header"; return NULL; }
    // the section: the descriptors (the old ones, the new, the closing empty one), the DLL's name, the function's
    // hint/name, its name thunks and its import address slots (each two: the entry and the end)
    uint32_t fa = nt->OptionalHeader.FileAlignment, sa = nt->OptionalHeader.SectionAlignment;
    uint32_t o_desc = 0, o_name = (uint32_t)(nd + 2) * sizeof(IMAGE_IMPORT_DESCRIPTOR), o_hint = pe_align(o_name + sizeof LLE_IMPORT, 2);
    uint32_t o_oft = pe_align(o_hint + 2 + sizeof LLE_FN, 4), o_iat = o_oft + 8, vsize = o_iat + 8;
    uint32_t va = pe_align(va_end, sa), raw = pe_align(size > raw_end ? size : raw_end, fa), rsize = pe_align(vsize, fa);
    uint8_t *n = calloc(1, raw + rsize);
    if (!n) { *why = "out of memory"; return NULL; }
    memcpy(n, b, size);
    nt = (IMAGE_NT_HEADERS32 *)(n + ((uint8_t *)nt - b));
    sec = IMAGE_FIRST_SECTION(nt);
    uint8_t *s = n + raw;
    memcpy(s + o_desc, pe_at(n, size, nt, nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress),
           (size_t)nd * sizeof(IMAGE_IMPORT_DESCRIPTOR));
    IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(s + o_desc) + nd;
    d->OriginalFirstThunk = va + o_oft;
    d->Name = va + o_name;
    d->FirstThunk = va + o_iat;
    memcpy(s + o_name, LLE_IMPORT, sizeof LLE_IMPORT);
    memcpy(s + o_hint + 2, LLE_FN, sizeof LLE_FN);
    *(uint32_t *)(s + o_oft) = *(uint32_t *)(s + o_iat) = va + o_hint;
    IMAGE_SECTION_HEADER *ns_h = &sec[ns];
    memset(ns_h, 0, sizeof *ns_h);
    memcpy(ns_h->Name, ".ageofai", 8);
    ns_h->Misc.VirtualSize = vsize;
    ns_h->VirtualAddress = va;
    ns_h->SizeOfRawData = rsize;
    ns_h->PointerToRawData = raw;
    ns_h->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;   // the loader writes its slots
    nt->FileHeader.NumberOfSections++;
    nt->OptionalHeader.SizeOfImage = pe_align(va + vsize, sa);
    nt->OptionalHeader.SizeOfInitializedData += rsize;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = va;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (uint32_t)(nd + 2) * sizeof(IMAGE_IMPORT_DESCRIPTOR);
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;   // bound to the old table: none now
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
    DWORD header_sum, sum;
    if (CheckSumMappedFile(n, raw + rsize, &header_sum, &sum)) nt->OptionalHeader.CheckSum = sum;
    *out = raw + rsize;
    return n;
}
// the import added to the engine file of folder dir (the game's): 0 done, else why not (a sentence)
internal const char *lle_patch(const char *dir) {
    char path[MAX_PATH], backup[MAX_PATH + 16];
    snprintf(path, sizeof path, "%s%s", dir, LLE_FILE);
    snprintf(backup, sizeof backup, "%s.original", path);
    int st = lle_state(path);
    if (st < 0) return "Low-Level Engine.dll isn't there or can't be read.";
    if (st > 0) return NULL;
    HANDLE f = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return GetLastError() == ERROR_SHARING_VIOLATION ? "The game is running - close Empire Earth first."
                                                         : "Low-Level Engine.dll can't be written (a folder that needs administrator rights?).";
    CloseHandle(f);
    if (GetFileAttributesA(backup) == INVALID_FILE_ATTRIBUTES && !CopyFileA(path, backup, TRUE))
        return "The backup of Low-Level Engine.dll couldn't be made - nothing changed.";
    uint32_t size = 0, nsize = 0;
    uint8_t *b = exe_file_read(path, &size);
    const char *why = "Low-Level Engine.dll couldn't be read - nothing changed.";
    uint8_t *n = b ? lle_with_import(b, size, &nsize, &why) : NULL;
    free(b);
    if (!n) return why;
    f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    DWORD put = 0;
    int ok = f != INVALID_HANDLE_VALUE && WriteFile(f, n, nsize, &put, NULL) && put == nsize;
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
    free(n);
    if (!ok || lle_state(path) != 1) {
        CopyFileA(backup, path, FALSE);
        return "Writing Low-Level Engine.dll failed - the backup was put back.";
    }
    return NULL;
}
// the import taken out again: the backup put back (it is the file before the import); 0 done, else why not
internal const char *lle_unpatch(const char *dir) {
    char path[MAX_PATH], backup[MAX_PATH + 16];
    snprintf(path, sizeof path, "%s%s", dir, LLE_FILE);
    snprintf(backup, sizeof backup, "%s.original", path);
    if (lle_state(path) == 0) { DeleteFileA(backup); return NULL; }   // (a "verify files" did it already)
    if (lle_state(backup) != 0) return "No backup of Low-Level Engine.dll without the import (Low-Level Engine.dll.original) - "
                                       "the game's \"verify files\" (Steam, GOG) puts the original back.";
    if (!CopyFileA(backup, path, FALSE))
        return GetLastError() == ERROR_SHARING_VIOLATION ? "The game is running - close Empire Earth first."
                                                         : "Low-Level Engine.dll can't be written - nothing changed.";
    DeleteFileA(backup);
    return NULL;
}
// the mod's dinput.dll of before (the settings' first way: a stand-in for the system's DirectInput) - gone with the
// import in its place: 1 one was there and is gone
internal int old_dinput_remove(const char *dir) {
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%sdinput.dll", dir);
    uint32_t size = 0;
    uint8_t *b = exe_file_read(path, &size);
    int ours = 0;
    for (uint32_t i = 0; b && i + 15 <= size && !ours; i++) ours = !memcmp(b + i, "EEAimodSettings", 15);
    free(b);
    return ours && DeleteFileA(path);
}
