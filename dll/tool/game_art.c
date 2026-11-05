// THE GAME'S ART: pictures read from the player's own Data\data.ssa when the program starts - nothing of the game's is
// shipped. data.ssa is an archive: "rass", a version, a word, the directory's end; then per file a name (its length
// with the 0 first), its first and last byte in the archive and its size. A file there starts with "PK01", its
// unpacked size and 4 bytes, then a PKWARE DCL ("implode") stream - unpacked by art_explode, a port of zlib's
// contrib/blast.c (Mark Adler, the zlib license; scripts/re/blast.py the same in Python). A texture (.sst) is a
// 15-byte header and a plain TGA; a full-screen picture is 256x256 tiles named <name>_<row>_<column>
// (docs/engine-notes.md). Any part missing or not as expected: no picture, and the program looks as without them.

struct art_entry { char name[64]; uint32_t first, last; };
internal struct art_entry *g_art_dir;
internal int g_art_n;
internal HANDLE g_art_file = INVALID_HANDLE_VALUE;

internal int art_read_at(uint32_t at, void *buf, uint32_t n) {
    OVERLAPPED o = { 0 };
    o.Offset = at;
    DWORD got = 0;
    return ReadFile(g_art_file, buf, n, &got, &o) && got == n;
}

internal void art_close(void) {   // the archive closed, its directory forgotten (another one opened next)
    if (g_art_file != INVALID_HANDLE_VALUE) CloseHandle(g_art_file);
    g_art_file = INVALID_HANDLE_VALUE;
    free(g_art_dir);
    g_art_dir = NULL;
    g_art_n = 0;
}

// the archive <game>\Data\data.ssa opened and its directory read: 1, else 0 (art_close after either)
internal int art_open(const char *game_dir) {
    char path[MAX_PATH];
    snprintf(path, sizeof path, "%s\\Data\\data.ssa", game_dir);
    g_art_file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (g_art_file == INVALID_HANDLE_VALUE) return 0;
    uint8_t head[16];
    uint32_t end;
    if (!art_read_at(0, head, 16) || memcmp(head, "rass", 4)) return 0;
    memcpy(&end, head + 12, 4);
    if (end < 16 || end > (64u << 20)) return 0;
    uint8_t *dir = malloc(end);
    if (!dir || !art_read_at(0, dir, end)) { free(dir); return 0; }
    int cap = 0;
    for (uint32_t p = 16; p + 4 <= end;) {
        uint32_t n;
        memcpy(&n, dir + p, 4);
        p += 4;
        if (!n || n > 300 || p + n + 12 > end) break;
        if (g_art_n == cap) {
            cap = cap ? cap * 2 : 1024;
            struct art_entry *more = realloc(g_art_dir, cap * sizeof *g_art_dir);
            if (!more) break;
            g_art_dir = more;
        }
        struct art_entry *e = &g_art_dir[g_art_n];
        if (n <= sizeof e->name) {
            memcpy(e->name, dir + p, n);
            e->name[n - 1] = 0;
            memcpy(&e->first, dir + p + n, 4);
            memcpy(&e->last, dir + p + n + 4, 4);
            g_art_n++;
        }
        p += n + 12;
    }
    free(dir);
    return g_art_n > 0;
}

// DCL: blast.c's tables and decoder
struct art_bits { const uint8_t *in, *end; uint32_t buf; int cnt, bad; };
internal int art_get(struct art_bits *s, int need) {
    uint32_t v = s->buf;
    while (s->cnt < need) {
        if (s->in >= s->end) { s->bad = 1; return 0; }
        v |= (uint32_t)*s->in++ << s->cnt;
        s->cnt += 8;
    }
    s->buf = v >> need;
    s->cnt -= need;
    return (int)(v & ((1u << need) - 1));
}
struct art_huff { short count[14], symbol[256]; };
internal void art_construct(struct art_huff *h, const uint8_t *rep, int n) {
    uint8_t length[256];
    int symbol = 0, offs[14];
    for (; n; n--, rep++)
        for (int left = (*rep >> 4) + 1; left && symbol < 256; left--) length[symbol++] = *rep & 15;
    memset(h->count, 0, sizeof h->count);
    for (int i = 0; i < symbol; i++) h->count[length[i]]++;
    offs[1] = 0;
    for (int len = 1; len < 13; len++) offs[len + 1] = offs[len] + h->count[len];
    for (int i = 0; i < symbol; i++) if (length[i]) h->symbol[offs[length[i]]++] = (short)i;
}
internal int art_decode(struct art_bits *s, const struct art_huff *h) {   // the codes come bit-inverted
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 14; len++) {
        code |= art_get(s, 1) ^ 1;
        int c = h->count[len];
        if (code < first + c) return h->symbol[index + (code - first)];
        index += c;
        first = (first + c) << 1;
        code <<= 1;
    }
    s->bad = 1;
    return 0;
}

// a DCL stream unpacked into out (cap bytes): the bytes written, -1 a broken stream
internal int art_explode(const uint8_t *in, uint32_t n, uint8_t *out, uint32_t cap) {
    static const uint8_t litlen[] = {
        11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8, 9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11,
        7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12, 8, 12, 5, 38, 5,
        38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27, 44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12,
        61, 12, 45, 44, 173 };
    static const uint8_t lenlen[] = { 2, 35, 36, 53, 38, 23 };
    static const uint8_t distlen[] = { 2, 20, 53, 230, 247, 151, 248 };
    static const short base[16] = { 3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264 };
    static const uint8_t extra[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    static struct art_huff lit, len, dist;
    static int made;
    if (!made) {
        art_construct(&lit, litlen, sizeof litlen);
        art_construct(&len, lenlen, sizeof lenlen);
        art_construct(&dist, distlen, sizeof distlen);
        made = 1;
    }
    struct art_bits s = { in, in + n, 0, 0, 0 };
    int coded = art_get(&s, 8), dict = art_get(&s, 8);
    if (coded > 1 || dict < 4 || dict > 6) return -1;
    uint32_t o = 0;
    while (!s.bad) {
        if (art_get(&s, 1)) {
            int sym = art_decode(&s, &len);
            int count = base[sym] + art_get(&s, extra[sym]);
            if (count == 519) return s.bad ? -1 : (int)o;   // the end
            int sh = count == 2 ? 2 : dict;
            uint32_t back = ((uint32_t)art_decode(&s, &dist) << sh) + (uint32_t)art_get(&s, sh) + 1;
            if (s.bad || back > o || o + (uint32_t)count > cap) return -1;
            for (; count; count--, o++) out[o] = out[o - back];
        } else {
            int b = coded ? art_decode(&s, &lit) : art_get(&s, 8);
            if (o >= cap) return -1;
            out[o++] = (uint8_t)b;
        }
    }
    return -1;
}

// the archive's file by name (textures\uicontrols.sst, any case), unpacked: malloc'd, *size its bytes; NULL none
internal uint8_t *art_load(const char *name, uint32_t *size) {
    for (int i = 0; i < g_art_n; i++) {
        if (lstrcmpiA(g_art_dir[i].name, name)) continue;
        uint32_t n = g_art_dir[i].last - g_art_dir[i].first + 1;
        if (g_art_dir[i].last < g_art_dir[i].first || n > (32u << 20)) return NULL;
        uint8_t *raw = malloc(n);
        if (!raw || !art_read_at(g_art_dir[i].first, raw, n)) { free(raw); return NULL; }
        if (n < 12 || memcmp(raw, "PK01", 4)) { *size = n; return raw; }
        uint32_t full;
        memcpy(&full, raw + 4, 4);
        uint8_t *out = full <= (32u << 20) ? malloc(full) : NULL;
        int got = out ? art_explode(raw + 12, n - 12, out, full) : -1;
        free(raw);
        if (got < 0) { free(out); return NULL; }
        *size = (uint32_t)got;
        return out;
    }
    return NULL;
}

// a texture as 32-bit pixels, top row first, alpha premultiplied (AlphaBlend's): malloc'd, NULL none
internal uint32_t *art_texture(const char *name, int *w, int *h) {
    char full[MAX_PATH];
    snprintf(full, sizeof full, "textures\\%s.sst", name);
    uint32_t n;
    uint8_t *b = art_load(full, &n);
    if (!b) return NULL;
    uint32_t *px = NULL;
    const uint8_t *t = b + 15;   // the TGA
    if (n > 15 + 18 && (t[2] == 2 || t[2] == 10) && (t[16] == 24 || t[16] == 32)) {
        int tw = t[12] | t[13] << 8, th = t[14] | t[15] << 8, bpp = t[16] / 8, top = t[17] & 0x20;
        const uint8_t *p = t + 18 + t[0], *end = b + n;
        px = tw > 0 && th > 0 && tw <= 4096 && th <= 4096 ? malloc((size_t)tw * th * 4) : NULL;
        for (int i = 0, run = 0, raw = 0; px && i < tw * th; i++) {
            if (t[2] == 10 && !run && !raw) {   // RLE: a packet's head
                if (p >= end) { free(px); px = NULL; break; }
                if (*p & 0x80) run = (*p++ & 0x7F) + 1; else raw = *p++ + 1;
            }
            if (p + bpp > end) { free(px); px = NULL; break; }
            uint32_t a = bpp == 4 ? p[3] : 255;
            uint32_t c = a << 24 | (p[2] * a / 255) << 16 | (p[1] * a / 255) << 8 | p[0] * a / 255;
            int y = i / tw, x = i % tw;
            px[(top ? y : th - 1 - y) * tw + x] = c;
            if (run) { if (--run) continue; } else if (raw) raw--;
            p += bpp;
        }
        *w = tw;
        *h = th;
    }
    free(b);
    return px;
}

internal HBITMAP art_bitmap(const uint32_t *px, int w, int h) {   // the pixels as a DIB section
    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void *bits;
    HBITMAP bm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (bm) memcpy(bits, px, (size_t)w * h * 4);
    return bm;
}

// a full-screen picture of rows x cols tiles (gamesettings: 3 x 4) as one bitmap, NULL a tile missing
internal HBITMAP art_picture(const char *name, int rows, int cols, int *w, int *h) {
    *w = cols * 256;
    *h = rows * 256;
    uint32_t *all = malloc((size_t)*w * *h * 4);
    for (int r = 0; all && r < rows; r++)
        for (int c = 0; c < cols; c++) {
            char tile[64];
            int tw, th;
            snprintf(tile, sizeof tile, "%s_%d_%d", name, r + 1, c + 1);
            uint32_t *t = art_texture(tile, &tw, &th);
            if (!t || tw != 256 || th != 256) { free(t); free(all); return NULL; }
            for (int y = 0; y < 256; y++) memcpy(all + (size_t)(r * 256 + y) * *w + c * 256, t + y * 256, 256 * 4);
            free(t);
        }
    HBITMAP bm = all ? art_bitmap(all, *w, *h) : NULL;
    free(all);
    return bm;
}
