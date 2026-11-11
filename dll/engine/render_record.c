// THE RECORDING (the pipe's record; scripts/results/flicker.py reads it): the next N presented frames one after another,
// each as <name>_NNN.bmp with its depth <name>_NNN.depth (the depth buffer's float a pixel, 0..1, rows top down), and a
// line each in <name>.csv - the camera the map was drawn with (scene_vp, the viewport), whether the frame was drawn
// during a step (the last update's ground, water and fields drawn again: kept, their batches - not counted by role), the
// water's level and whether the shadows fade under it, the shadow map's fitting (shm, the casters' box, the maps drawn)
// and the draws by their shadow role - to find what changes from one frame to the next that shouldn't (the player, 2026-11-06: "shadows are still unstable, and farms flicker in visibility" - standing still
// too, worse while the camera moves). A single shot lands on a frame by chance; a flicker of one frame in twenty needs
// them all. The depth lets the script place each pixel in the world and find it again in the frames before and after
// while the camera moves (with pan dx dy: moved by that every frame, as the pipe's camera pan). The frames are copies
// on the graphics card while they are taken (the picture and the depth: 30 MB a frame at 2560 x 1440, at most REC_MAX),
// read back once the last is taken: the game keeps its pace meanwhile. Read back frame by frame they took 250 ms each -
// nearly every frame came after a step of the game, and whatever happens between two (a frame drawn from the last
// step's kept list, moved to the camera) never showed. Written all at once after the last, they held the render thread
// and with it the game 19 - 24 s for 120 frames (the player's game, 2026-11-11: "game frezzes for several seconds"): now
// each presented frame after the last maps one copy whose read back the frame before started (no wait for the card)
// and hands its rows to a thread of its own that makes the file - REC_QUEUE at most waiting (15 MB each).
#define REC_ROLES 11   // the shadow roles SH_NONE .. SH_TREE, and the game's shadows' decals (the farms' fields)
#define REC_MAX 120
internal struct {
    volatile LONG left;      // frames still to record (the pipe thread waits for 0)
    int n, depth, taken, written;
    int steps, flip;         // record steps: every second frame drawn as during a step (rec_step; 2 bare: without the
                             // game's update at all, as while the interface's lock is held - render_3d.c g_rwl_bare)
    ID3D11Texture2D *pic[REC_MAX], *dep[REC_MAX];   // the frames' copies, until written
    char base[MAX_PATH];     // the dump folder's path and the name: base_NNN.bmp
    float pan[2];
    FILE *csv;
    LONGLONG t0;
    LONG passes;             // the shadow maps drawn before the frame
    LONG kept;               // the kept batches drawn again before the frame (render_frame.c g_kg_drawn)
    int draws[REC_ROLES], verts[REC_ROLES];   // the draws and their vertices by role since the last presented frame
    volatile LONG writing;   // the frames taken being read back and written (rec_flush_frame); a new recording waits
    int next;                // the next copy to read back: picture i at 2 i, its depth at 2 i + 1
    ID3D11Texture2D *st;     // the copy being read back, into ...
    char st_path[MAX_PATH + 16];
    int st_depth;
} g_rec;

// THE WRITER: the rows read back into files, on a thread of its own
#define REC_QUEUE 3
typedef struct rec_item {
    struct rec_item *next;
    char path[MAX_PATH + 16];
    int depth, done;          // the depth raw, else a picture (B8G8R8A8 rows) as a 24-bit BMP; done: <name>.done, no rows
    UINT w, h;
    uint8_t *rows;            // w x 4 bytes a row, top down
} rec_item;
internal struct {
    CRITICAL_SECTION cs;
    HANDLE wake;
    rec_item *first, *last;
    volatile LONG queued;
    int on;
} g_rw;
internal void rec_write_item(const rec_item *it) {
    FILE *f = fopen(it->path, it->done ? "w" : "wb");
    if (!f) return;
    if (it->done) fprintf(f, "%d of %d frames taken, %d written\n", g_rec.taken, g_rec.n, g_rec.written);
    else if (it->depth) fwrite(it->rows, (size_t)it->w * 4, it->h, f);
    else {
        int w = (int)it->w, h = (int)it->h, stride = (w * 3 + 3) & ~3;
        BITMAPINFOHEADER bi = { sizeof bi, w, h, 1, 24, BI_RGB, (DWORD)(stride * h), 0, 0, 0, 0 };
        BITMAPFILEHEADER fh = { 0x4D42, (DWORD)(sizeof fh + sizeof bi + bi.biSizeImage), 0, 0, sizeof fh + sizeof bi };
        fwrite(&fh, sizeof fh, 1, f);
        fwrite(&bi, sizeof bi, 1, f);
        uint8_t *row = calloc((size_t)stride, 1);
        for (int y = h - 1; row && y >= 0; y--) {   // bottom up
            const uint8_t *src = it->rows + (size_t)y * (size_t)w * 4;
            for (int x = 0; x < w; x++) { row[x * 3] = src[x * 4]; row[x * 3 + 1] = src[x * 4 + 1]; row[x * 3 + 2] = src[x * 4 + 2]; }
            fwrite(row, (size_t)stride, 1, f);
        }
        free(row);
        g_rec.written++;
    }
    fclose(f);
}
internal DWORD WINAPI rec_writer(LPVOID unused) {
    (void)unused;
    for (;;) {
        WaitForSingleObject(g_rw.wake, INFINITE);
        for (;;) {
            EnterCriticalSection(&g_rw.cs);
            rec_item *it = g_rw.first;
            if (it) { g_rw.first = it->next; if (!g_rw.first) g_rw.last = NULL; }
            LeaveCriticalSection(&g_rw.cs);
            if (!it) break;
            rec_write_item(it);
            if (it->done) InterlockedExchange(&g_rec.writing, 0);
            free(it->rows);
            free(it);
            InterlockedDecrement(&g_rw.queued);
        }
    }
}
// an item to the writer (its rows its own from here); 0: no writer
internal int rec_queue(rec_item *it) {
    if (!g_rw.on) {
        InitializeCriticalSection(&g_rw.cs);
        g_rw.wake = CreateEventA(NULL, FALSE, FALSE, NULL);
        HANDLE t = g_rw.wake ? CreateThread(NULL, 0, rec_writer, NULL, 0, NULL) : NULL;
        if (!t) return 0;
        CloseHandle(t);
        g_rw.on = 1;
    }
    it->next = NULL;
    InterlockedIncrement(&g_rw.queued);
    EnterCriticalSection(&g_rw.cs);
    if (g_rw.last) g_rw.last->next = it; else g_rw.first = it;
    g_rw.last = it;
    LeaveCriticalSection(&g_rw.cs);
    SetEvent(g_rw.wake);
    return 1;
}

// record steps: while frames are left, every second frame skips the render objects' update as a frame drawn during a
// step does - a heavy game draws many such frames, a test's steps are too short for any: alternating with the frames
// drawn with an update, whatever such a frame draws otherwise is there for a frame and gone in the next (flicker.py)
internal int rec_step(void) {
    if (!g_rec.steps || g_rec.left <= 0) return 0;
    g_rec.flip ^= 1;
    return g_rec.flip ? g_rec.steps : 0;
}

// a draw's role counted for the frame (rd3_shadow_classify)
internal void rec_count(int role, DWORD nv) {
    if (role == SH_CARD) role = SH_AWAY;   // counted with what casts and isn't drawn, as ever
    if (role < 0 || role >= REC_ROLES - 1) return;
    g_rec.draws[role]++;
    g_rec.verts[role] += (int)nv;
    if (role == SH_DECAL && g_ro_cur == RO_SHADOWS) { g_rec.draws[REC_ROLES - 1]++; g_rec.verts[REC_ROLES - 1] += (int)nv; }
}

// a copy of a texture made on the card (to read back later): its kind but nothing bound, as fmt (0 its own)
internal ID3D11Texture2D *rec_copy(ID3D11Texture2D *src, DXGI_FORMAT fmt) {
    D3D11_TEXTURE2D_DESC td = { 0 };
    ID3D11Texture2D_GetDesc(src, &td);
    if (fmt) td.Format = fmt;
    td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = 0; td.CPUAccessFlags = 0; td.MiscFlags = 0;
    ID3D11Texture2D *t = NULL;
    if (FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &t))) return NULL;
    ID3D11DeviceContext_CopyResource(g_d3d.ctx, (ID3D11Resource *)t, (ID3D11Resource *)src);
    return t;
}

// a presented frame while the frames taken are written: the copy whose read back the frame before started mapped (not
// waited for: still on the card, the next frame tries again) and its rows to the writer, the next copy's read back started;
// after the last <name>.done (the picture B8G8R8A8 as a 24-bit BMP, the depth R32_FLOAT raw, rows top down)
#define REC_HANDED (2 * REC_MAX + 1)   // g_rec.next once all is with the writer
internal void rec_flush_frame(void) {
    if (!g_rec.writing || g_rec.next == REC_HANDED) return;
    ID3D11DeviceContext *x = g_d3d.ctx;
    if (g_rec.st) {
        if (g_rw.queued >= REC_QUEUE) return;   // the writer behind: its memory bounded
        D3D11_MAPPED_SUBRESOURCE ms;
        HRESULT hr = ID3D11DeviceContext_Map(x, (ID3D11Resource *)g_rec.st, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &ms);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) return;
        if (SUCCEEDED(hr)) {
            D3D11_TEXTURE2D_DESC td = { 0 };
            ID3D11Texture2D_GetDesc(g_rec.st, &td);
            rec_item *it = calloc(1, sizeof *it);
            uint8_t *rows = it ? malloc((size_t)td.Width * 4 * td.Height) : NULL;
            if (rows) {
                for (UINT y = 0; y < td.Height; y++) memcpy(rows + (size_t)y * td.Width * 4, (const uint8_t *)ms.pData + (size_t)y * ms.RowPitch, (size_t)td.Width * 4);
                lstrcpynA(it->path, g_rec.st_path, sizeof it->path);
                it->depth = g_rec.st_depth; it->w = td.Width; it->h = td.Height; it->rows = rows;
                if (!rec_queue(it)) { free(rows); free(it); }
            } else free(it);
            ID3D11DeviceContext_Unmap(x, (ID3D11Resource *)g_rec.st, 0);
        }
        g_rec.st->lpVtbl->Release(g_rec.st);
        g_rec.st = NULL;
    }
    while (!g_rec.st && g_rec.next < 2 * g_rec.taken) {
        int i = g_rec.next / 2, depth = g_rec.next & 1;
        ID3D11Texture2D **t = depth ? &g_rec.dep[i] : &g_rec.pic[i];
        g_rec.next++;
        if (!*t) continue;
        D3D11_TEXTURE2D_DESC td = { 0 };
        ID3D11Texture2D_GetDesc(*t, &td);
        td.Usage = D3D11_USAGE_STAGING; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if (SUCCEEDED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &g_rec.st))) {
            ID3D11DeviceContext_CopyResource(x, (ID3D11Resource *)g_rec.st, (ID3D11Resource *)*t);
            snprintf(g_rec.st_path, sizeof g_rec.st_path, "%s_%03d.%s", g_rec.base, i, depth ? "depth" : "bmp");
            g_rec.st_depth = depth;
        } else g_rec.st = NULL;
        (*t)->lpVtbl->Release(*t);
        *t = NULL;
    }
    if (!g_rec.st && g_rec.next >= 2 * g_rec.taken) {   // all handed over: the end, for whoever waits for the files
        rec_item *it = calloc(1, sizeof *it);
        if (it) {
            snprintf(it->path, sizeof it->path, "%s.done", g_rec.base);
            it->done = 1;
            if (rec_queue(it)) { g_rec.next = REC_HANDED; return; }   // the writer ends the writing
            free(it);
        }
        InterlockedExchange(&g_rec.writing, 0);
    }
}

// a presented frame (render_present, its list drawn): recorded while frames are left, the counts started over
internal void rec_frame(rd_surface *s) {
    if (g_rec.left > 0 && g_rec.csv) {
        int i = g_rec.n - (int)g_rec.left, w = (int)s->desc.dwWidth, h = (int)s->desc.dwHeight;
        g_rec.pic[i] = s->tex ? rec_copy(s->tex, 0) : NULL;
        g_rec.dep[i] = g_rec.depth && s->zbuf && s->zbuf->tex ? rec_copy(s->zbuf->tex, DXGI_FORMAT_R32_FLOAT) : NULL;
        int pic = g_rec.pic[i] != NULL, dep = g_rec.dep[i] != NULL;
        g_rec.taken = i + 1;
        LARGE_INTEGER now, f;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&f);
        FILE *c = g_rec.csv;
        fprintf(c, "%d,%.1f,%d,%d,%d,%d,%.1f,%.1f,%.1f,%.1f", i, (double)(now.QuadPart - g_rec.t0) * 1000.0 / (double)f.QuadPart,
                w, h, pic, dep, (double)g_rd3.scene_port.TopLeftX, (double)g_rd3.scene_port.TopLeftY,
                (double)g_rd3.scene_port.Width, (double)g_rd3.scene_port.Height);
        // the camera the picture has: the map's is the last update's, each draw moved to the camera's place now
        // (render_frame.c rf_shift: V_now V^-1 before view x projection) - the same moved here
        float vp[16], inv[16], to_now[16];
        memcpy(vp, g_rd3.scene_vp, sizeof vp);
        if (g_cam_now_ok) {
            rd3_rigid_inverse(inv, g_rd3.scene_view);
            rd3_mul(to_now, g_cam_view_now, inv);
            rd3_mul(vp, to_now, g_rd3.scene_vp);
        }
        for (int a = 0; a < 16; a++) fprintf(c, ",%.9g", (double)vp[a]);
        fprintf(c, ",%d,%ld", g_kg_skip + g_rwl_bare_now, (long)(g_kg_drawn - g_rec.kept));   // step: 1 during a step, 2 bare
        fprintf(c, ",%.4f,%d", (double)g_sh.shl[0], (int)g_sh.shl[2]);
        fprintf(c, ",%d,%ld,%d", g_sh.valid, (long)(g_sh.passes - g_rec.passes), g_sh.last_cast);
        for (int k = 0; k < 3; k++) fprintf(c, ",%.4f", (double)g_sh.last_lo[k]);
        for (int k = 0; k < 3; k++) fprintf(c, ",%.4f", (double)g_sh.last_hi[k]);
        for (int k = 0; k < 16; k++) fprintf(c, ",%.9g", (double)g_sh.shm[k]);
        for (int k = 0; k < REC_ROLES; k++) fprintf(c, ",%d,%d", g_rec.draws[k], g_rec.verts[k]);
        fputc('\n', c);
        if (g_rec.pan[0] != 0 || g_rec.pan[1] != 0) camera_pan(g_rec.pan[0], g_rec.pan[1]);
        if (!pic) g_rec.left = 1;   // the card's memory full: the frames so far
        if (g_rec.left == 1) {   // the last: the writing begins (rec_flush_frame, from the next frame on)
            fclose(c);
            g_rec.csv = NULL;
            g_rec.next = 0;
            g_rec.st = NULL;
            InterlockedExchange(&g_rec.writing, 1);
        }
        InterlockedDecrement(&g_rec.left);
    } else rec_flush_frame();
    g_rec.passes = g_sh.passes;
    g_rec.kept = g_kg_drawn;
    memset(g_rec.draws, 0, sizeof g_rec.draws);
    memset(g_rec.verts, 0, sizeof g_rec.verts);
}

// record <frames 3..120> [name] [pan <dx> <dy>] [steps | bare] [nodepth]: the next frames into the dump folder (above), the
// reply once they are written
internal void cmd_record(char *out, const char *args) {
    char copy[256], name[64] = "rec";
    int n = 0, depth = 1, steps = 0;
    float pan[2] = { 0, 0 };
    lstrcpynA(copy, args, sizeof copy);
    char *t = strtok(copy, " ");
    if (t) n = atoi(t);
    while (t && (t = strtok(NULL, " "))) {
        if (!strcmp(t, "nodepth")) depth = 0;
        else if (!strcmp(t, "steps")) steps = 1;
        else if (!strcmp(t, "bare")) steps = 2;
        else if (!strcmp(t, "pan")) {
            char *x = strtok(NULL, " "), *y = x ? strtok(NULL, " ") : NULL;
            char *ex = NULL, *ey = NULL;
            if (y) { pan[0] = strtof(x, &ex); pan[1] = strtof(y, &ey); }
            if (!y || *ex || *ey) { n = 0; break; }   // two numbers ("record 40 pan pan 0.05 0.05" took the name for one)
        } else lstrcpynA(name, t, sizeof name);
    }
    if (n < 3 || n > REC_MAX) { REPLY("error: usage: record <frames 3..120> [name] [pan <dx> <dy>] [steps | bare] [nodepth]\n"); return; }
    if (!g_rd_on) { REPLY("error: the recording needs the own renderer ([display] renderer = own)\n"); return; }
    if (g_rec.left > 0 || g_rec.writing) { REPLY("error: a recording is under way (or its files are being written)\n"); return; }
    CreateDirectoryA(g_dump_dir, NULL);
    char dir[MAX_PATH], path[MAX_PATH + 16];
    DWORD dn = GetFullPathNameA(g_dump_dir, sizeof dir, dir, NULL);
    if (!dn || dn >= sizeof dir || snprintf(g_rec.base, sizeof g_rec.base, "%s\\%s", dir, name) >= (int)sizeof g_rec.base) {
        REPLY("error: the recording's path is too long\n");
        return;
    }
    snprintf(path, sizeof path, "%s.done", g_rec.base);
    DeleteFileA(path);   // an earlier recording's
    snprintf(path, sizeof path, "%s.csv", g_rec.base);
    FILE *c = fopen(path, "w");
    if (!c) { REPLY("error: can't write %s\n", path); return; }
    fprintf(c, "frame,ms,w,h,picture,depth,port_x,port_y,port_w,port_h");
    for (int k = 0; k < 16; k++) fprintf(c, ",vp%d", k);
    fprintf(c, ",step,kept,water_level,water_fade,sh_valid,sh_maps,sh_cast,sh_lo_x,sh_lo_y,sh_lo_z,sh_hi_x,sh_hi_y,sh_hi_z");
    for (int k = 0; k < 16; k++) fprintf(c, ",shm%d", k);
    static const char *const roles[REC_ROLES] = { "none", "receive", "cast", "tint", "decal", "blob", "water", "away", "gone", "tree", "fields" };
    for (int k = 0; k < REC_ROLES; k++) fprintf(c, ",%s_draws,%s_verts", roles[k], roles[k]);
    fputc('\n', c);
    LARGE_INTEGER t0;
    QueryPerformanceCounter(&t0);
    RD_LOCK();
    g_rec.n = n;
    g_rec.depth = depth;
    g_rec.steps = steps;
    g_rec.flip = 0;
    g_rec.pan[0] = pan[0]; g_rec.pan[1] = pan[1];
    g_rec.csv = c;
    g_rec.t0 = t0.QuadPart;
    g_rec.taken = g_rec.written = 0;
    g_rec.left = n;
    RD_UNLOCK();
    // not waited for: the pipe's commands run on the game's thread, and waiting there held the simulation for the
    // whole recording - the frames drawn on, the game standing still (the player, 2026-11-06: "does recording pause
    // the game?"; the flicker they saw is only there while it runs). <name>.done says when the files are written.
    REPLY("ok: recording %d frames into %s_000.bmp ... and %s.csv; %s.done once they are written (a frame each, the game goes on)\n",
          n, g_rec.base, g_rec.base, g_rec.base);
}
