// THE DATA FILES' SEARCH (Low-Level Engine.dll's FSPath, [data_game_file_path]): its functions are the DLL's exports,
// found by name (each build ships its own DLL; the names are the same). FindFile(name, FSFileSpec *&, FSFileSpec &)
// is the one every lookup ends in (the other overloads, GetFileArchiveStatus, the texture, model and sound managers):
// the path's directories in the order added, then its archives - 0 found, 3 not.

typedef long (__thiscall *fs_find_fn)(void *path, const void *name, void **found, void *spec);
internal fs_find_fn g_fs_find;   // the original (the trampoline once hooked)
internal inline void *fs_main_path(void) { return *(void **)data_game_file_path; }   // NULL before the game set it up

internal void *fs_export(const char *name) {
    HMODULE m = GetModuleHandleA("Low-Level Engine.dll");
    return m ? (void *)GetProcAddress(m, name) : NULL;
}
internal int fs_dir_count(void *path) {
    typedef unsigned long (__thiscall *fn)(void *);
    fn f = (fn)fs_export("?GetDirectoryCount@FSPath@@QBEKXZ");
    return f ? (int)f(path) : -1;
}
internal int fs_archive_count(void *path) {
    typedef unsigned long (__thiscall *fn)(void *);
    fn f = (fn)fs_export("?GetArchiveCount@FSPath@@QBEKXZ");
    return f ? (int)f(path) : -1;
}

// A new path of the main path's directory d and archive a (either -1: none) - the engine's own calls: the directory's
// FSFileSpec copied out (GetDirectory) and added, the archive added by reference (AddArchive copies it and opens the
// copy: an index of its own). The objects are the engine's, allocated here with room to spare: an FSPath is 0x28 bytes,
// an FSFileSpec 0x48. NULL when an export is missing.
internal void *fs_path_of(void *main, int d, int a) {
    typedef void *(__thiscall *ctor_fn)(void *);
    typedef void (__thiscall *dtor_fn)(void *);
    typedef long (__thiscall *get_dir_fn)(void *, unsigned long, void *);
    typedef void *(__thiscall *get_arc_fn)(void *, unsigned long);
    typedef void (__thiscall *add_fn)(void *, const void *, uint8_t);   // the bool: at the end (0: in front)
    ctor_fn path_ctor = (ctor_fn)fs_export("??0FSPath@@QAE@XZ"), spec_ctor = (ctor_fn)fs_export("??0FSFileSpec@@QAE@XZ");
    dtor_fn spec_dtor = (dtor_fn)fs_export("??1FSFileSpec@@UAE@XZ");
    get_dir_fn get_dir = (get_dir_fn)fs_export("?GetDirectory@FSPath@@QBEJKAAVFSFileSpec@@@Z");
    get_arc_fn get_arc = (get_arc_fn)fs_export("?GetArchive@FSPath@@QBEABVFSArchive@@K@Z");
    add_fn add_dir = (add_fn)fs_export("?AddDirectory@FSPath@@QAEXABVFSFileSpec@@_N@Z");
    add_fn add_arc = (add_fn)fs_export("?AddArchive@FSPath@@QAEXABVFSArchive@@_N@Z");
    if (!path_ctor || !spec_ctor || !spec_dtor || !get_dir || !get_arc || !add_dir || !add_arc) return NULL;
    void *path = calloc(1, 0x80);
    if (!path) return NULL;
    path_ctor(path);
    if (d >= 0) {
        uint8_t spec[0x100] = { 0 };
        spec_ctor(spec);
        if (!get_dir(main, (unsigned long)d, spec)) add_dir(path, spec, 1);
        spec_dtor(spec);
    }
    if (a >= 0) add_arc(path, get_arc(main, (unsigned long)a), 1);
    return path;
}

// the lookup hooked: hook(path, name, found, spec) as FindFile, the original through g_fs_find. 0: the DLL isn't
// loaded or its FindFile doesn't start as both builds' do (push ebp; mov ebp, esp; sub esp, 0x50)
internal int patch_fs_find(fs_find_fn hook) {
    static const uint8_t prologue[6] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x50 };
    void *orig = NULL;
    uintptr_t f = (uintptr_t)fs_export("?FindFile@FSPath@@QAEJABVUString@@AAPAVFSFileSpec@@AAV3@@Z");
    if (!patch_entry(f, prologue, 6, (void *)hook, &orig)) return 0;
    g_fs_find = (fs_find_fn)orig;
    return 1;
}
