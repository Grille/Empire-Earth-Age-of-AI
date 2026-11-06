"""Finds a flicker in a recording of the pipe's record (dll/engine/render_record.c): <dir>/<name>_NNN.bmp, .depth and
<name>.csv - frames one after another.

The pictures: each frame against the one before and the one after, those two moved to its camera first (every pixel
of frame i placed in the world through its depth and the frame's camera, and looked up where frames i-1 and i+1 drew
that place - a pixel hidden there, by its depth, is left out). A flicker is a pixel that differs from both by more
than --diff the same way (brighter than both, or darker) while those two agree: there for a frame and gone, or gone
for a frame. Something that moves differs as well - a unit walking, its place in frame i is ground in both others:
a recording of a paused game has none of that. Left out: what stays on the screen while the camera moves (the
cursor, the interface), and single pixels - a thin edge's sparkle as it shifts a little (a pixel counts with 5 of the 9
around it). Per frame the pixels that flicker, and their 16x16 blocks; for the
frames with the most, <dir>/<name>_flicker_NNN.bmp: frame i-1 moved, frame i, frame i+1 moved, and frame i with the
flickering pixels red (cut to the region that has them, --crop 0 the whole).

The numbers: the shadow map's fitting frame by frame (its texel's size in tiles across and along, and how far a fixed
place of the world moved over the map, in texels: the edges of every shadow shift by that when the map is fitted
anew - a fraction of a texel is a shimmer), the maps drawn, and the draws by their shadow role - the farms' fields
("fields": the game's shadows' decals) among them, a field missing for a frame shows there; and which frames were drawn
during a step of the game (the last update's ground, water and fields drawn again).

usage: local/venv/Scripts/python.exe scripts/results/flicker.py <dir> [name] [--diff 24] [--worst 4] [--crop 1]
       (numpy: local/venv/Scripts/python.exe -m pip install numpy)
"""
import argparse
import csv
import os
import struct
import sys

import numpy as np

SH_SIZE = 4096   # render_shadows.c's map, texels a side


def read_bmp(path):
    with open(path, 'rb') as f:
        data = f.read()
    off, = struct.unpack_from('<I', data, 10)
    w, h, _, bpp = struct.unpack_from('<iiHH', data, 18)
    if bpp != 24:
        sys.exit(f'{path}: {bpp} bits a pixel, the recording writes 24')
    stride = (w * 3 + 3) & ~3
    a = np.frombuffer(data, np.uint8, stride * abs(h), off).reshape(abs(h), stride)[:, :w * 3].reshape(abs(h), w, 3)
    return a[::-1] if h > 0 else a   # bottom-up unless the height is negative: rows top down, BGR


def write_bmp(path, img):
    h, w, _ = img.shape
    stride = (w * 3 + 3) & ~3
    rows = np.zeros((h, stride), np.uint8)
    rows[:, :w * 3] = img[::-1].reshape(h, w * 3)
    with open(path, 'wb') as f:
        f.write(struct.pack('<2sIHHI', b'BM', 54 + rows.size, 0, 0, 54))
        f.write(struct.pack('<IiiHHIIiiII', 40, w, h, 1, 24, 0, rows.size, 0, 0, 0, 0))
        f.write(rows.tobytes())


def luma(img):
    return img[..., 2] * 0.299 + img[..., 1] * 0.587 + img[..., 0] * 0.114   # BGR


class Frame:
    def __init__(self, row, base):
        self.i = int(row['frame'])
        self.row = row
        self.w, self.h = int(row['w']), int(row['h'])
        self.vp = np.array([float(row[f'vp{k}']) for k in range(16)]).reshape(4, 4)   # row vectors: world @ vp = clip
        self.port = [float(row[k]) for k in ('port_x', 'port_y', 'port_w', 'port_h')]
        self.base = f'{base}_{self.i:03d}'
        self._img = self._depth = None

    @property
    def img(self):
        if self._img is None:
            self._img = read_bmp(self.base + '.bmp').astype(np.float32)
        return self._img

    @property
    def depth(self):
        if self._depth is None and int(self.row['depth']):
            self._depth = np.fromfile(self.base + '.depth', np.float32).reshape(self.h, self.w)
        return self._depth

    def drop(self):
        self._img = self._depth = None


def to_world(f):
    """Frame f's pixels as places in the world (h x w x 3) and where the map is (depth below 1 inside the port)."""
    ys, xs = np.mgrid[0:f.h, 0:f.w].astype(np.float64)
    px, py, pw, ph = f.port
    nx = (xs + 0.5 - px) / pw * 2 - 1
    ny = 1 - (ys + 0.5 - py) / ph * 2
    z = f.depth.astype(np.float64)
    clip = np.stack([nx, ny, z, np.ones_like(z)], -1)
    p = clip @ np.linalg.inv(f.vp)
    world = p[..., :3] / p[..., 3:4]
    ok = (z < 1) & (nx >= -1) & (nx <= 1) & (ny >= -1) & (ny <= 1)
    return world, ok


def moved_to(src, world, ok):
    """Frame src's picture looked up at the places world (another frame's pixels): the colours, and where it holds."""
    p = np.concatenate([world, np.ones(world.shape[:2] + (1,))], -1) @ src.vp
    w = p[..., 3]
    good = ok & (w > 1e-6)
    w = np.where(good, w, 1)
    nx, ny, nz = p[..., 0] / w, p[..., 1] / w, p[..., 2] / w
    px, py, pw, ph = src.port
    x = np.floor((nx + 1) / 2 * pw + px).astype(np.int64)
    y = np.floor((1 - ny) / 2 * ph + py).astype(np.int64)
    good &= (x >= 0) & (x < src.w) & (y >= 0) & (y < src.h) & (nx >= -1) & (nx <= 1) & (ny >= -1) & (ny <= 1)
    x = np.clip(x, 0, src.w - 1)
    y = np.clip(y, 0, src.h - 1)
    if src.depth is not None:   # hidden there: something nearer drawn over the place
        good &= np.abs(src.depth[y, x] - nz) < 2e-4 + 0.002 * np.abs(nz)
    return src.img[y, x], good


def same_camera(a, b):
    return np.allclose(a.vp, b.vp, rtol=0, atol=1e-7) and a.port == b.port


def flicker_mask(prev, cur, nxt, diff):
    """The pixels of cur that differ from both neighbours by more than diff the same way while those two agree."""
    if same_camera(prev, cur) and same_camera(nxt, cur):
        a, b = prev.img, nxt.img
        good = np.ones(cur.img.shape[:2], bool)
    else:
        if cur.depth is None or prev.depth is None or nxt.depth is None:
            sys.exit('the camera moved, but the recording has no depth (record ... nodepth): nothing to move frames by')
        world, ok = to_world(cur)
        a, ga = moved_to(prev, world, ok)
        b, gb = moved_to(nxt, world, ok)
        good = ga & gb
        # what stays on the screen while the camera moves - the cursor, the minimap's frame, the interface over the
        # map: its depth is the map's under it, so moved it is somewhere else; it matches both frames unmoved instead
        lc = luma(cur.img)
        good &= ~((np.abs(lc - luma(prev.img)) < diff / 2) & (np.abs(lc - luma(nxt.img)) < diff / 2))
    la, lc, lb = luma(a), luma(cur.img), luma(b)
    da, db = lc - la, lc - lb
    m = good & (da * db > 0) & (np.minimum(np.abs(da), np.abs(db)) > diff) & (np.abs(la - lb) < diff / 2)
    # a thing there or gone is a patch: a pixel counts with at least 5 of the 9 around it (itself among them) - a
    # thin edge's sparkle (a pixel's colour shifting as the edge crosses it a little elsewhere) is a line of single ones
    n = np.zeros(m.shape, np.int32)
    p = np.pad(m, 1).astype(np.int32)
    for dy in range(3):
        for dx in range(3):
            n += p[dy:dy + m.shape[0], dx:dx + m.shape[1]]
    m &= n >= 5
    return m, a, b, good


def blocks(mask, n=16):
    h, w = mask.shape
    hb, wb = h // n, w // n
    c = mask[:hb * n, :wb * n].reshape(hb, n, wb, n).sum((1, 3))
    return c


def shadow_table(frames):
    """The shadow map's fitting frame by frame: texel size across / along (tiles), the drift of a fixed place."""
    out = []
    ref = None
    for f in frames:
        r = f.row
        if not int(r['sh_valid']):
            out.append(None)
            continue
        m = np.array([float(r[f'shm{k}']) for k in range(16)]).reshape(4, 4)
        tx = 1 / (np.linalg.norm(m[:3, 0]) * SH_SIZE)   # u = p . r / wx + ...: a texel across is wx / 4096 tiles
        ty = 1 / (np.linalg.norm(m[:3, 1]) * SH_SIZE)
        if ref is None:   # a place in the middle of the casters' box, kept for the whole recording
            ref = np.array([(float(r[f'sh_lo_{a}']) + float(r[f'sh_hi_{a}'])) / 2 for a in 'xyz'] + [1])
        uv = (ref @ m)[:2] * SH_SIZE
        out.append((tx, ty, uv))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('dir')
    ap.add_argument('name', nargs='?', default='rec')
    ap.add_argument('--diff', type=float, default=24)
    ap.add_argument('--worst', type=int, default=4)
    ap.add_argument('--crop', type=int, default=1)
    a = ap.parse_args()
    base = os.path.join(a.dir, a.name)
    with open(base + '.csv', newline='') as f:
        frames = [Frame(r, base) for r in csv.DictReader(f)]
    frames = [f for f in frames if int(f.row['picture'])]
    if len(frames) < 3:
        sys.exit(f'{base}.csv: {len(frames)} frames with a picture, 3 needed')
    moved = sum(not same_camera(frames[k], frames[k + 1]) for k in range(len(frames) - 1))
    span = float(frames[-1].row['ms']) - float(frames[0].row['ms'])
    print(f'{base}: {len(frames)} frames, {frames[0].w}x{frames[0].h}, {span / max(1, len(frames) - 1):.0f} ms a frame,'
          f' the camera moved between {moved} of {len(frames) - 1}')

    # the shadow map
    sh = shadow_table(frames)
    print('\nthe shadow map (texel: its size in tiles across / along; drift: a fixed place over the map since the frame'
          ' before, in texels; maps: drawn for the frame):')
    print(' frame   texel across   along    drift x    drift y  maps  casters')
    prev = None
    drifts = []
    for f, s in zip(frames, sh):
        r = f.row
        if s is None:
            print(f' {f.i:5d}   no map')
            prev = None
            continue
        tx, ty, uv = s
        d = uv - prev if prev is not None else np.zeros(2)
        if prev is not None:
            drifts.append(d)
        print(f' {f.i:5d}   {tx:12.4f} {ty:7.4f} {d[0]:10.3f} {d[1]:10.3f} {r["sh_maps"]:>5} {r["sh_cast"]:>8}')
        prev = uv
    if drifts:
        d = np.abs(np.array(drifts))
        frac = np.abs(d - np.round(d))
        moving = (d > 1e-3).any(1)
        print(f' the map moved under a fixed place in {moving.sum()} of {len(d)} frames; the drift\'s part of a texel'
              f' (a shimmer of every shadow edge): mean {frac.mean():.3f}, at most {frac.max():.3f}')

    # the frames drawn during a step: the last update's ground, water and fields drawn again (kept: their batches, not
    # counted by role below)
    steps = [f.i for f in frames if int(f.row.get('step', 0))]
    if 'step' in frames[0].row:
        print(f'\nframes drawn during a step: {len(steps)} of {len(frames)}' + (f' ({" ".join(map(str, steps))})' if steps else '') +
              (f'; kept batches drawn again in them: {min(int(f.row["kept"]) for f in frames if int(f.row["step"]))} - '
               f'{max(int(f.row["kept"]) for f in frames if int(f.row["step"]))}' if steps else ''))

    # the draws by role
    roles = [k[:-6] for k in frames[0].row if k.endswith('_draws')]
    print('\nthe draws by shadow role (draws / vertices a frame: least - most; a role whose count changes is listed'
          ' frame by frame below):')
    changing = []
    for role in roles:
        dr = [int(f.row[f'{role}_draws']) for f in frames]
        vs = [int(f.row[f'{role}_verts']) for f in frames]
        if max(dr) == 0:
            continue
        print(f' {role:8s} {min(dr):6d} - {max(dr):6d}   {min(vs):8d} - {max(vs):8d}')
        if role in ('fields', 'decal', 'receive', 'water') and (min(dr) != max(dr) or min(vs) != max(vs)):
            changing.append(role)
    for role in changing:
        print(f' {role} frame by frame (draws/vertices): ' +
              ' '.join(f'{f.row[role + "_draws"]}/{f.row[role + "_verts"]}' for f in frames))

    # the pictures
    print(f'\nflickering pixels (differ from both neighbours by more than {a.diff:.0f} the same way, the neighbours'
          f' within {a.diff / 2:.0f}):')
    print(' frame   pixels  blocks  compared  where (x0 y0 - x1 y1)')
    found = []
    for k in range(1, len(frames) - 1):
        p, c, n = frames[k - 1], frames[k], frames[k + 1]
        m, ma, mb, good = flicker_mask(p, c, n, a.diff)
        bl = blocks(m)
        nb = int((bl >= 8).sum())   # blocks with at least 8 flickering pixels: a scattered pixel isn't one
        ys, xs = np.nonzero(m)
        where = f'{xs.min()} {ys.min()} - {xs.max()} {ys.max()}' if len(xs) else ''
        print(f' {c.i:5d} {int(m.sum()):8d} {nb:7d} {good.mean() * 100:8.1f}%  {where}')
        found.append((nb, int(m.sum()), k))
        frames[k - 1].drop()
    found.sort(key=lambda t: (-t[0], -t[1]))
    for nb, px, k in found[:a.worst]:   # their pictures made again: kept for every frame they'd fill the memory
        if not px:
            break
        c = frames[k]
        m, ma, mb, _ = flicker_mask(frames[k - 1], c, frames[k + 1], a.diff)
        cur = c.img
        mark = cur.copy()
        mark[m] = (0, 0, 255)
        y0, y1, x0, x1 = 0, c.h, 0, c.w
        if a.crop:
            bl = blocks(m) >= 8
            ys, xs = np.nonzero(bl if bl.any() else blocks(m) > 0)
            y0, y1 = max(0, ys.min() * 16 - 64), min(c.h, ys.max() * 16 + 80)
            x0, x1 = max(0, xs.min() * 16 - 64), min(c.w, xs.max() * 16 + 80)
        strip = np.concatenate([ma[y0:y1, x0:x1], cur[y0:y1, x0:x1], mb[y0:y1, x0:x1], mark[y0:y1, x0:x1]], 1)
        path = f'{base}_flicker_{c.i:03d}.bmp'
        write_bmp(path, np.clip(strip, 0, 255).astype(np.uint8))
        print(f' frame {c.i}: {px} pixels, {nb} blocks -> {path} (before moved, the frame, after moved, red: the flicker;'
              f' x {x0} - {x1}, y {y0} - {y1})')


if __name__ == '__main__':
    main()
