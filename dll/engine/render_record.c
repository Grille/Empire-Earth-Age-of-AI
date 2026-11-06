// THE RECORDING (the pipe's record; scripts/results/flicker.py reads it): the next N presented frames one after another,
// each as <name>_NNN.bmp with its depth <name>_NNN.depth (the depth buffer's float a pixel, 0..1, rows top down), and a
// line each in <name>.csv - the camera the map was drawn with (scene_vp, the viewport), whether the frame was drawn
// during a step (the last update's ground, water and fields drawn again: kept, their batches - not counted by role), the
// shadow map's fitting (shm, the casters' box, the maps drawn) and the draws by their shadow role - to find what changes from one frame to the next
// that shouldn't (the player, 2026-11-06: "shadows are still unstable, and farms flicker in visibility" - standing still
// too, worse while the camera moves). A single shot lands on a frame by chance; a flicker of one frame in twenty needs
// them all. The depth lets the script place each pixel in the world and find it again in the frames before and after
// while the camera moves (with pan dx dy: moved by that every frame, as the pipe's camera pan). The frames are copies
// on the graphics card while they are taken (the picture and the depth: 30 MB a frame at 2560 x 1440, at most REC_MAX),
// read back and written once the last is taken: the game keeps its pace meanwhile. Read back frame by frame they took
// 250 ms each - nearly every frame came after a step of the game, and whatever happens between two (a frame drawn from
// the last step's kept list, moved to the camera) never showed.
#define REC_ROLES 11   // the shadow roles SH_NONE .. SH_TREE, and the game's shadows' decals (the farms' fields)
#define REC_MAX 120
internal struct {
    volatile LONG left;      // frames still to record (the pipe thread waits for 0)
    int n, depth, taken, written;
    ID3D11Texture2D *pic[REC_MAX], *dep[REC_MAX];   // the frames' copies, until written
    char base[MAX_PATH];     // the dump folder's path and the name: base_NNN.bmp
    float pan[2];
    FILE *csv;
    LONGLONG t0;
    LONG passes;             // the shadow maps drawn before the frame
    LONG kept;               // the kept batches drawn again before the frame (render_frame.c g_kg_drawn)
    int draws[REC_ROLES], verts[REC_ROLES];   // the draws and their vertices by role since the last presented frame
} g_rec;

// a draw's role counted for the frame (rd3_shadow_classify)
internal void rec_count(int role, DWORD nv) {
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

// a copy read back into path: the picture (B8G8R8A8) as a 24-bit BMP, the depth (R32_FLOAT) raw, rows top down
internal int rec_write(ID3D11Texture2D *t, const char *path, int depth) {
    D3D11_TEXTURE2D_DESC td = { 0 };
    ID3D11Texture2D_GetDesc(t, &td);
    td.Usage = D3D11_USAGE_STAGING; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D *st = NULL;
    if (FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &st))) return 0;
    ID3D11DeviceContext_CopyResource(g_d3d.ctx, (ID3D11Resource *)st, (ID3D11Resource *)t);
    D3D11_MAPPED_SUBRESOURCE ms;
    int mapped = SUCCEEDED(ID3D11DeviceContext_Map(g_d3d.ctx, (ID3D11Resource *)st, 0, D3D11_MAP_READ, 0, &ms));
    FILE *f = mapped ? fopen(path, "wb") : NULL;
    if (f && depth)
        for (UINT y = 0; y < td.Height; y++) fwrite((uint8_t *)ms.pData + (size_t)y * ms.RowPitch, 4, td.Width, f);
    else if (f) {
        int w = (int)td.Width, h = (int)td.Height, stride = (w * 3 + 3) & ~3;
        BITMAPINFOHEADER bi = { sizeof bi, w, h, 1, 24, BI_RGB, (DWORD)(stride * h), 0, 0, 0, 0 };
        BITMAPFILEHEADER fh = { 0x4D42, (DWORD)(sizeof fh + sizeof bi + bi.biSizeImage), 0, 0, sizeof fh + sizeof bi };
        fwrite(&fh, sizeof fh, 1, f);
        fwrite(&bi, sizeof bi, 1, f);
        uint8_t *row = calloc((size_t)stride, 1);
        for (int y = h - 1; row && y >= 0; y--) {   // bottom up
            const uint8_t *src = (const uint8_t *)ms.pData + (size_t)y * ms.RowPitch;
            for (int x = 0; x < w; x++) { row[x * 3] = src[x * 4]; row[x * 3 + 1] = src[x * 4 + 1]; row[x * 3 + 2] = src[x * 4 + 2]; }
            fwrite(row, (size_t)stride, 1, f);
        }
        free(row);
    }
    if (f) fclose(f);
    if (mapped) ID3D11DeviceContext_Unmap(g_d3d.ctx, (ID3D11Resource *)st, 0);
    st->lpVtbl->Release(st);
    return f != NULL;
}

// the frames taken into their files, the copies released
internal void rec_flush(void) {
    char path[MAX_PATH + 16];
    for (int i = 0; i < g_rec.taken; i++) {
        snprintf(path, sizeof path, "%s_%03d.bmp", g_rec.base, i);
        if (g_rec.pic[i] && rec_write(g_rec.pic[i], path, 0)) g_rec.written++;
        snprintf(path, sizeof path, "%s_%03d.depth", g_rec.base, i);
        if (g_rec.dep[i]) rec_write(g_rec.dep[i], path, 1);
        if (g_rec.pic[i]) g_rec.pic[i]->lpVtbl->Release(g_rec.pic[i]);
        if (g_rec.dep[i]) g_rec.dep[i]->lpVtbl->Release(g_rec.dep[i]);
        g_rec.pic[i] = g_rec.dep[i] = NULL;
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
        fprintf(c, ",%d,%ld", g_kg_skip, (long)(g_kg_drawn - g_rec.kept));
        fprintf(c, ",%d,%ld,%d", g_sh.valid, (long)(g_sh.passes - g_rec.passes), g_sh.last_cast);
        for (int k = 0; k < 3; k++) fprintf(c, ",%.4f", (double)g_sh.last_lo[k]);
        for (int k = 0; k < 3; k++) fprintf(c, ",%.4f", (double)g_sh.last_hi[k]);
        for (int k = 0; k < 16; k++) fprintf(c, ",%.9g", (double)g_sh.shm[k]);
        for (int k = 0; k < REC_ROLES; k++) fprintf(c, ",%d,%d", g_rec.draws[k], g_rec.verts[k]);
        fputc('\n', c);
        if (g_rec.pan[0] != 0 || g_rec.pan[1] != 0) camera_pan(g_rec.pan[0], g_rec.pan[1]);
        if (!pic) g_rec.left = 1;   // the card's memory full: the frames so far
        if (g_rec.left == 1) { fclose(c); g_rec.csv = NULL; rec_flush(); }
        InterlockedDecrement(&g_rec.left);
    }
    g_rec.passes = g_sh.passes;
    g_rec.kept = g_kg_drawn;
    memset(g_rec.draws, 0, sizeof g_rec.draws);
    memset(g_rec.verts, 0, sizeof g_rec.verts);
}

// record <frames 3..120> [name] [pan <dx> <dy>] [nodepth]: the next frames into the dump folder (above), the reply once
// they are written
internal void cmd_record(char *out, const char *args) {
    char copy[256], name[64] = "rec";
    int n = 0, depth = 1;
    float pan[2] = { 0, 0 };
    lstrcpynA(copy, args, sizeof copy);
    char *t = strtok(copy, " ");
    if (t) n = atoi(t);
    while (t && (t = strtok(NULL, " "))) {
        if (!strcmp(t, "nodepth")) depth = 0;
        else if (!strcmp(t, "pan")) {
            char *x = strtok(NULL, " "), *y = x ? strtok(NULL, " ") : NULL;
            char *ex = NULL, *ey = NULL;
            if (y) { pan[0] = strtof(x, &ex); pan[1] = strtof(y, &ey); }
            if (!y || *ex || *ey) { n = 0; break; }   // two numbers ("record 40 pan pan 0.05 0.05" took the name for one)
        } else lstrcpynA(name, t, sizeof name);
    }
    if (n < 3 || n > REC_MAX) { REPLY("error: usage: record <frames 3..120> [name] [pan <dx> <dy>] [nodepth]\n"); return; }
    if (!g_rd_on) { REPLY("error: the recording needs the own renderer ([display] renderer = own)\n"); return; }
    if (g_rec.left > 0) { REPLY("error: a recording is under way\n"); return; }
    CreateDirectoryA(g_dump_dir, NULL);
    char dir[MAX_PATH], path[MAX_PATH + 16];
    DWORD dn = GetFullPathNameA(g_dump_dir, sizeof dir, dir, NULL);
    if (!dn || dn >= sizeof dir || snprintf(g_rec.base, sizeof g_rec.base, "%s\\%s", dir, name) >= (int)sizeof g_rec.base) {
        REPLY("error: the recording's path is too long\n");
        return;
    }
    snprintf(path, sizeof path, "%s.csv", g_rec.base);
    FILE *c = fopen(path, "w");
    if (!c) { REPLY("error: can't write %s\n", path); return; }
    fprintf(c, "frame,ms,w,h,picture,depth,port_x,port_y,port_w,port_h");
    for (int k = 0; k < 16; k++) fprintf(c, ",vp%d", k);
    fprintf(c, ",step,kept,sh_valid,sh_maps,sh_cast,sh_lo_x,sh_lo_y,sh_lo_z,sh_hi_x,sh_hi_y,sh_hi_z");
    for (int k = 0; k < 16; k++) fprintf(c, ",shm%d", k);
    static const char *const roles[REC_ROLES] = { "none", "receive", "cast", "tint", "decal", "blob", "water", "away", "gone", "tree", "fields" };
    for (int k = 0; k < REC_ROLES; k++) fprintf(c, ",%s_draws,%s_verts", roles[k], roles[k]);
    fputc('\n', c);
    LARGE_INTEGER t0;
    QueryPerformanceCounter(&t0);
    RD_LOCK();
    g_rec.n = n;
    g_rec.depth = depth;
    g_rec.pan[0] = pan[0]; g_rec.pan[1] = pan[1];
    g_rec.csv = c;
    g_rec.t0 = t0.QuadPart;
    g_rec.taken = g_rec.written = 0;
    g_rec.left = n;
    RD_UNLOCK();
    int waited = 0;
    while (g_rec.left > 0 && waited < 20000 + n * 1000) { Sleep(20); waited += 20; }
    if (g_rec.left > 0) {
        RD_LOCK();
        REPLY("error: only %d of %d frames presented in %d s\n", n - (int)g_rec.left, n, waited / 1000);
        g_rec.left = 0;
        if (g_rec.csv) { fclose(g_rec.csv); g_rec.csv = NULL; }
        rec_flush();
        RD_UNLOCK();
        return;
    }
    REPLY("%s: %d of %d frames taken, %d written: %s_000.bmp ..., %s.csv\n", g_rec.written == n ? "ok" : "error", g_rec.taken, n,
          g_rec.written, g_rec.base, g_rec.base);
}
