"""Pictures of an AI's base walls from a run's log: every `ringmap P r` reply (a setup script's
`ringmap(2, 50)`, or the pipe's) becomes a PNG, a tile 6 pixels, x right and y down as the game's tiles.

usage: ringmap.py <log (.csv or .csv.gz)> <out folder>   -> <out>/ringmap_<n>_P<p>_<min>.png

Colours: walkable ground light, water blue, other ground dark grey, trees green, mines/resources gold, the AI's wall
black (going up: grey), its gates red, towers purple, Town Center orange, other buildings tan, other players'
buildings and walls light red, something else in the way brown, ring tiles still open yellow, tower spots of the ring a purple dot.
"""
import os
import re
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(__file__))
from logs import open_log  # noqa: E402

COL = {
    '.': (226, 220, 196), '~': (70, 110, 190), '^': (90, 90, 90), 't': (40, 120, 40), 'm': (220, 180, 40),
    'W': (20, 20, 20), 'w': (140, 140, 140), 'G': (210, 30, 30), 'T': (130, 50, 170), 'C': (240, 130, 20),
    'B': (170, 130, 80), 'E': (240, 150, 150), ',': (250, 240, 60), 'x': (110, 80, 60),
}
PX = 6


def png(path, w, h, pixels):
    raw = b''.join(b'\x00' + bytes(v for p in row for v in p) for row in pixels)

    def chunk(t, d):
        c = struct.pack('>I', len(d)) + t + d
        return c + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def draw(rows, x0, y0, slots, gates, path):
    w = len(rows[0]) * PX
    h = len(rows) * PX
    pix = [[(0, 0, 0)] * w for _ in range(h)]
    for ty, row in enumerate(rows):
        for tx, c in enumerate(row):
            col = COL.get(c, (255, 0, 255))
            for py in range(PX):
                line = pix[ty * PX + py]
                for px in range(PX):
                    line[tx * PX + px] = col
    for sx, sy in slots:   # a dot in the middle of the spot
        tx, ty = sx - x0, sy - y0
        if 0 <= tx < len(rows[0]) and 0 <= ty < len(rows):
            for py in range(2, PX - 2):
                for px in range(2, PX - 2):
                    pix[ty * PX + py][tx * PX + px] = (130, 50, 170)
    for gx, gy in gates:   # a planned gate's middle: a red frame
        tx, ty = gx - x0, gy - y0
        if 0 <= tx < len(rows[0]) and 0 <= ty < len(rows):
            for i in range(PX):
                for a, b in ((0, i), (PX - 1, i), (i, 0), (i, PX - 1)):
                    pix[ty * PX + a][tx * PX + b] = (210, 30, 30)
    png(path, w, h, pix)


def main():
    log, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    head = re.compile(r'P(\d+) ringmap (-?\d+),(-?\d+) (\d+)')
    n = 0
    minute = 0.0
    cur = None
    with open_log(log) as f:
        for line in f:
            m = re.match(r'^(\d+),', line)
            if m:
                minute = int(m.group(1)) / 60000.0
            if cur is None:
                h = head.search(line) if 'ringmap' in line else None
                if h:
                    cur = dict(p=int(h.group(1)), x0=int(h.group(2)), y0=int(h.group(3)), w=int(h.group(4)), rows=[], slots=[], gates=[])
                continue
            body = line[1:].strip() if line.startswith('#') else None
            if body is None:
                continue
            if len(cur['rows']) < cur['w']:
                cur['rows'].append(body)
                continue
            s = re.match(r'(slot|gate) (-?\d+),(-?\d+)', body)
            if s:
                cur['slots' if s.group(1) == 'slot' else 'gates'].append((int(s.group(2)), int(s.group(3))))
                continue
            if body.startswith('x type'):   # what the brown tiles hold
                print('  P%d %s' % (cur['p'], body))
                continue
            n += 1
            path = os.path.join(out, 'ringmap_%d_P%d_%.0f.png' % (n, cur['p'], minute))
            draw(cur['rows'], cur['x0'], cur['y0'], cur['slots'], cur['gates'], path)
            print(path)
            cur = None
            h = head.search(line) if 'ringmap' in line else None
            if h:
                cur = dict(p=int(h.group(1)), x0=int(h.group(2)), y0=int(h.group(3)), w=int(h.group(4)), rows=[], slots=[], gates=[])
    if cur and len(cur['rows']) == cur['w']:
        n += 1
        path = os.path.join(out, 'ringmap_%d_P%d_%.0f.png' % (n, cur['p'], minute))
        draw(cur['rows'], cur['x0'], cur['y0'], cur['slots'], cur['gates'], path)
        print(path)


if __name__ == '__main__':
    main()
