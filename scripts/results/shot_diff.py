"""Compares the shot pairs of scripts/test/render_compare.sh: <dir>/wrapper_<name>.bmp against <dir>/own_<name>.bmp.

Per pair: the mean difference (0..255 over the colour channels), the share of pixels that differ by more than 24 in
some channel, and the share of 16x16 blocks whose mean difference is over 8 (a scattered difference - moving units,
animations - touches few blocks; a systematic one - fog, filtering, an offset - most). Writes <dir>/diff_<name>.bmp
(the difference x4) and <dir>/table.txt.
usage: python scripts/results/shot_diff.py <dir>
"""
import os
import struct
import sys


def read_bmp(path):
    with open(path, 'rb') as f:
        data = f.read()
    off, = struct.unpack_from('<I', data, 10)
    w, h, _, bpp = struct.unpack_from('<iiHH', data, 18)
    bpp_bytes = bpp // 8
    stride = (w * bpp_bytes + 3) & ~3
    rows = []
    for y in range(abs(h)):
        src_y = abs(h) - 1 - y if h > 0 else y   # bottom-up unless the height is negative
        row = data[off + src_y * stride: off + src_y * stride + w * bpp_bytes]
        if bpp_bytes == 4:   # BGRX -> BGR
            row = bytes(b for i, b in enumerate(row) if i % 4 != 3)
        rows.append(row)
    return w, abs(h), rows


def write_bmp(path, w, h, rows):
    stride = (w * 3 + 3) & ~3
    pad = b'\0' * (stride - w * 3)
    body = b''.join(rows[h - 1 - y] + pad for y in range(h))
    head = struct.pack('<2sIHHI', b'BM', 54 + len(body), 0, 0, 54)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, 24, 0, len(body), 2835, 2835, 0, 0)
    with open(path, 'wb') as f:
        f.write(head + info + body)


def compare(a, b):
    w, h, ra = a
    w2, h2, rb = b
    if (w, h) != (w2, h2):
        return None
    total = 0
    big = 0
    block = 16
    bw, bh = (w + block - 1) // block, (h + block - 1) // block
    bsum = [0] * (bw * bh)
    diff_rows = []
    for y in range(h):
        x_row, y_row = ra[y], rb[y]
        d = bytes(abs(p - q) for p, q in zip(x_row, y_row))
        total += sum(d)
        by = (y // block) * bw
        for x in range(w):
            i = x * 3
            m = max(d[i], d[i + 1], d[i + 2])
            if m > 24:
                big += 1
            bsum[by + x // block] += d[i] + d[i + 1] + d[i + 2]
        diff_rows.append(bytes(min(255, v * 4) for v in d))
    n = w * h
    blocks = sum(1 for s in bsum if s / (block * block * 3) > 8)
    return total / (n * 3), 100.0 * big / n, 100.0 * blocks / len(bsum), diff_rows


def main():
    d = sys.argv[1]
    names = sorted(f[len('wrapper_'):] for f in os.listdir(d) if f.startswith('wrapper_') and f.endswith('.bmp'))
    lines = ['%-14s %8s %10s %10s' % ('shot', 'mean', 'pixels>24', 'blocks>8')]
    for name in names:
        own = os.path.join(d, 'own_' + name)
        if not os.path.exists(own):
            lines.append('%-14s missing in own' % name[:-4])
            continue
        a, b = read_bmp(os.path.join(d, 'wrapper_' + name)), read_bmp(own)
        r = compare(a, b)
        if r is None:
            lines.append('%-14s sizes differ: %dx%d / %dx%d' % (name[:-4], a[0], a[1], b[0], b[1]))
            continue
        mean, big, blocks, rows = r
        write_bmp(os.path.join(d, 'diff_' + name), a[0], a[1], rows)
        lines.append('%-14s %8.2f %9.2f%% %9.2f%%' % (name[:-4], mean, big, blocks))
        print(lines[-1], flush=True)
    with open(os.path.join(d, 'table.txt'), 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('\n'.join(lines))


if __name__ == '__main__':
    main()
