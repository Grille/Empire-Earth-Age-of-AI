# a territories dump (the pipe's territories dump <file>) as a picture: each territory a colour (land warm, water blue),
# borders dark, trees darker, neither black, each middle a red dot, resource sites white (food), grey (stone), yellow
# (gold), brown (iron). usage: territories_png.py <dump> <png> [scale]
import sys, struct, zlib, colorsys
src, dst = sys.argv[1], sys.argv[2]
scale = int(sys.argv[3]) if len(sys.argv) > 3 else 3
b = open(src, 'rb').read()
nl = b.index(b'\n'); _, size, nt = b[:nl].split(); size = int(size); nt = int(nt)
p = nl + 1
lab = struct.unpack('<%dH' % (size * size), b[p:p + 2 * size * size]); p += 2 * size * size
cls = b[p:p + size * size]; p += size * size
lines = b[p:].decode('latin-1').splitlines()
mid = set(); sites = {}
SITE = {2: (255, 255, 255), 4: (120, 120, 120), 5: (255, 220, 0), 6: (160, 60, 0)}
for ln in lines:
    w = ln.split()
    if len(w) > 3 and w[2] == 'at':
        x, y = w[3].rstrip(':').split(','); mid.add((int(x), int(y)))
    if len(w) == 4 and w[0] == 'site': sites[(int(w[1]), int(w[2]))] = int(w[3])
def col(r, water):
    h = (r * 0.61803398875) % 1.0
    if water: c = colorsys.hsv_to_rgb(0.55 + 0.1 * h, 0.6, 0.45 + 0.4 * ((r * 7) % 5) / 4)
    else: c = colorsys.hsv_to_rgb((0.75 + 0.7 * h) % 1.0, 0.40, 0.95)   # no blues: 0.75 .. 1 and 0 .. 0.45
    return tuple(int(255 * v) for v in c)
rows = []
for y in range(size):
    row = bytearray()
    for x in range(size):
        i = y * size + x; r = lab[i]; k = cls[i]
        if (x, y) in mid: c = (255, 0, 0)
        elif (x, y) in sites: c = SITE.get(sites[(x, y)], (255, 0, 255))
        elif r == 0xFFFF: c = (0, 0, 0) if (k & 3) == 0 else (70, 70, 70)
        else:
            c = col(r, (k & 3) == 2)
            if k & 4: c = tuple(v * 6 // 10 for v in c)
            if (x + 1 < size and lab[i + 1] != r and lab[i + 1] != 0xFFFF and (cls[i + 1] & 3) == (k & 3)) or (y + 1 < size and lab[i + size] != r and lab[i + size] != 0xFFFF and (cls[i + size] & 3) == (k & 3)): c = (20, 20, 20)
        row += bytes(c) * scale
    for _ in range(scale): rows.append(b'\0' + bytes(row))
def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size * scale, size * scale, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(b''.join(rows), 6)) + chunk(b'IEND', b'')
open(dst, 'wb').write(png)
print(size, nt, size * scale)
