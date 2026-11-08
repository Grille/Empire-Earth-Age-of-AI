"""All buildings of the game as one OBJ with PNG textures, to look at or touch up in a 3D program.

    python scripts/re/cem_export.py <folder>

writes <folder>/buildings.obj, .mtl, .json (each model's place in the OBJ, its tag points) and <folder>/Textures/*.png
from the player's Data\\data.ssa. A model file (.cem, "SSMF"; GEModel::Load, Low-Level Engine.dll) is, in version 2:

  "SSMF", u16 version, u16 subversion; 7 u32: triangles, vertices, tag points, materials, frames, children, levels of
  detail; a string (u32 length with its 0, the bytes) the name, 3 floats the centre; per level of detail its triangles
  (a count, 3 u32 each - counted from the material's first vertex); per material a string, a u32 (the image of the
  texture it draws: GEMaterial::SetTextureIndex), per level of detail its first triangle and count, its first vertex
  and count, a string; the tag points' names; per frame a float (the radius), per vertex 3 floats position, 3 normal,
  2 texture, per tag point 3 floats, a 4 x 4 matrix (its place in its parent, by row), a box (6 floats); then each
  child, a whole model again. Version 1 (one building: bld_towncenter_10) is laid out otherwise: read_legacy.

The texture name in a model is the artist's and often names no file. The game's own is db\\dbgraphics.dat's, a record
per graphic with the model and its texture, set on all the model's materials and children (GEModel::SetTexture). A
texture (.sst; GETexture::Load) is 14 bytes (a byte, mip levels, u32 images, u32 width, height), then per image and
mip level a byte and a plain TGA.

Only the full level of detail and the first frame are written. Vertices at one place are one vertex of the OBJ (a 3D
program sees a connected mesh), texture coordinates and normals stay per corner. The game's z is up and the OBJ's y:
OBJ = (x + offset x, z, offset z - y), the .json has each model's offset. Texture v counts from the picture's top in
the game (GESurface::LoadTGA turns a bottom-up file over), from its bottom in an OBJ."""
import json, os, re, struct, sys, zlib

sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))   # scripts/: game_dir.py
from blast import explode
from game_dir import game_dir

GAP = 1.0   # between the models of the grid, in the game's units (a building's tile is about 1)
PLAYER_COLOR = 'player color'


class Reader:
    def __init__(self, data, pos=0):
        self.d, self.p = data, pos

    def take(self, fmt):
        v = struct.unpack_from('<' + fmt, self.d, self.p)
        self.p += struct.calcsize('<' + fmt)
        return v

    def u32(self):
        return self.take('I')[0]

    def skip(self, n):
        self.p += n

    def string(self):
        n = self.u32()
        s = self.d[self.p:self.p + n].split(b'\0')[0].decode('latin1')
        self.p += n
        return s


def read_archive(path):
    """data.ssa's directory as {lower-case name: (first byte, last byte)}, and the open file."""
    f = open(path, 'rb')
    n = struct.unpack('<I', f.read(16)[12:16])[0]
    idx = Reader(f.read(n))
    files = {}
    while idx.p < n - 4 and struct.unpack_from('<I', idx.d, idx.p)[0]:
        name = idx.string()
        first, last, _ = idx.take('III')
        files[name.lower()] = (first, last)
    return f, files


def read_file(f, entry):
    f.seek(entry[0])
    d = f.read(entry[1] - entry[0] + 1)
    return explode(d[12:]) if d[:4] == b'PK01' else d


def read_model(r):
    """A model and its children from r, its triangles by material in the model's own vertex numbers."""
    if r.d[r.p:r.p + 4] != b'SSMF':
        raise ValueError('no model at %d' % r.p)
    r.skip(4)
    version, sub = r.take('HH')
    return read_current(r) if version == 2 else read_legacy(r, sub)


def read_current(r):
    tris, verts, tags, mats, frames, children, lods = r.take('7I')
    name = r.string()
    r.take('3f')
    lists = [[r.take('3I') for _ in range(r.u32())] for _ in range(lods)]
    materials = []
    for _ in range(mats):
        m = {'name': r.string(), 'image': r.u32()}
        ranges = [r.take('II') for _ in range(lods)]
        vfirst, _ = r.take('II')
        m['model_texture'] = r.string()
        first, count = ranges[0]
        m['tris'] = [tuple(i + vfirst for i in t) for t in lists[0][first:first + count]]
        materials.append(m)
    tag_names = [r.string() for _ in range(tags)]
    model = None
    for k in range(frames):
        r.take('f')
        vs = [r.take('8f') for _ in range(verts)]
        tag_points = [r.take('3f') for _ in range(tags)]
        matrix = r.take('16f')
        r.take('6f')
        if k == 0:
            model = {'name': name, 'materials': materials, 'pos': [v[:3] for v in vs], 'normal': [v[3:6] for v in vs],
                     'uv': [v[6:8] for v in vs], 'tags': dict(zip(tag_names, tag_points)), 'matrix': matrix}
    model['children'] = [read_model(r) for _ in range(children)]
    return model


def read_legacy(r, sub):
    """Version 1 keeps points apart from vertices (a vertex: a point and a normal; a triangle's corner: a vertex and a
    texture place). Its normals are indices into a table the engine computes: not taken, the 3D program makes its own."""
    if sub != 3:
        raise ValueError('version 1.%d: no model of it seen' % sub)
    frames, mats, points, polys, groups, verts, tags, children = r.take('8I')
    name = r.string()
    r.take('3f')
    r.take('B')
    r.skip(4 * points)
    corners = []
    for _ in range(polys):
        tri = []
        for _ in range(3):
            vertex = r.u32()
            uv = r.take('2f')
            r.take('3f')   # a colour
            r.take('4f')
            tri.append((vertex, uv))
        corners.append(tri)
    player = set()
    for _ in range(groups):
        group = r.string()
        members = [r.u32() for _ in range(r.u32())]
        if group == PLAYER_COLOR:
            player.update(members)
    poly_lists = []   # (a material's image is taken to be its place among the materials: bld_towncenter_10 then looks right)
    for _ in range(mats):
        poly_lists.append([r.u32() for _ in range(r.u32())])
        if r.take('B')[0]:
            r.string()
            r.u32()
    vertex_point = []
    for _ in range(verts):
        vertex_point.append(r.u32())
        r.take('f')
    tag_names = [r.string() for _ in range(tags)]
    model = None
    for k in range(frames):
        r.take('f')
        pts = [r.take('3f') for _ in range(points)]
        r.skip(2 * verts)
        tag_points = [r.take('3f') for _ in range(tags)]
        matrix = r.take('16f')
        r.take('6f')
        if k == 0:
            pos, uv, materials = [], [], []
            for image, ps in enumerate(poly_lists):
                for wanted in (False, True):
                    out = []
                    for p in ps:
                        if (p in player) != wanted:
                            continue
                        base = len(pos)
                        for vertex, c_uv in corners[p]:
                            pos.append(pts[vertex_point[vertex]])
                            uv.append(c_uv)
                        out.append((base, base + 1, base + 2))
                    if out:
                        materials.append({'name': PLAYER_COLOR if wanted else '', 'image': image, 'model_texture': '', 'tris': out})
            model = {'name': name, 'materials': materials, 'pos': pos, 'normal': None, 'uv': uv,
                     'tags': dict(zip(tag_names, tag_points)), 'matrix': matrix}
    model['children'] = [read_model(r) for _ in range(children)]
    return model


IDENTITY = (1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)


def matmul(a, b):
    return tuple(sum(a[i * 4 + k] * b[k * 4 + j] for k in range(4)) for i in range(4) for j in range(4))


def apply(m, p, w=1.0):
    return tuple(m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3] * w for i in range(3))


def determinant3(m):
    return (m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) + m[2] * (m[4] * m[9] - m[5] * m[8]))


def parts(model, parent=None):
    """Each part of a model with the matrix that places it in the root's space (the root's own is not used)."""
    m = matmul(parent, model['matrix']) if parent else IDENTITY
    yield model, m
    for c in model['children']:
        yield from parts(c, m)


def read_graphics(f, files):
    """dbgraphics.dat as {model: [texture, ...]}, lower-case, the texture most records give first. The names are
    strings with a u32 length before them, the texture's right after the model's."""
    d = read_file(f, files['db\\dbgraphics.dat'])
    found = {}
    for m in re.finditer(rb'models\\', d, re.I):
        n = struct.unpack_from('<I', d, m.start() - 4)[0]
        if not 0 < n < 200:
            continue
        p = m.start() + n
        n2 = struct.unpack_from('<I', d, p)[0]
        tex = d[p + 4:p + 4 + n2].split(b'\0')[0].decode('latin1').lower() if 0 < n2 < 200 else ''
        if tex.startswith('textures\\'):
            model = d[m.start():p].split(b'\0')[0].decode('latin1').lower()[len('models\\'):]
            found.setdefault(model, {}).setdefault(tex[len('textures\\'):], 0)
            found[model][tex[len('textures\\'):]] += 1
    return {k: sorted(v, key=lambda t: -v[t]) for k, v in found.items()}


def tga_image(d, p):
    """The TGA at p as (width, height, channels, rows top first of RGB(A)) and the offset after it."""
    id_len, cmap, kind = d[p], d[p + 1], d[p + 2]
    w, h, bpp, desc = struct.unpack_from('<HHBB', d, p + 12)
    if kind != 2 or cmap or bpp not in (24, 32):
        raise ValueError('TGA type %d, %d bits' % (kind, bpp))
    ch = bpp // 8
    start = p + 18 + id_len
    rows = [bytearray(d[start + y * w * ch:start + (y + 1) * w * ch]) for y in range(h)]
    for row in rows:
        row[0::ch], row[2::ch] = row[2::ch], row[0::ch]   # BGR to RGB
    if not desc & 0x20:
        rows.reverse()
    if desc & 0x10:
        for i, row in enumerate(rows):
            rows[i] = b''.join(bytes(row[x * ch:x * ch + ch]) for x in range(w - 1, -1, -1))
    return (w, h, ch, rows), start + w * h * ch


def sst_images(d):
    """The full-size pictures of a texture, one per image of it (a file holds several images or mip levels)."""
    mips = d[1] or 1
    images = struct.unpack_from('<I', d, 2)[0]
    p, out = 14, []
    for _ in range(images):
        for level in range(mips):
            image, p2 = tga_image(d, p + 1)
            if level == 0:
                out.append(image)
            p = p2
    return out


def write_png(path, image):
    w, h, ch, rows = image

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body))

    raw = b''.join(b'\0' + bytes(r) for r in rows)
    with open(path, 'wb') as out:
        out.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6 if ch == 4 else 2, 0, 0, 0))
                  + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def safe(name):
    return re.sub(r'[^a-z0-9_.-]', '_', name.lower())


def main():
    out_dir = sys.argv[1]
    os.makedirs(os.path.join(out_dir, 'Textures'), exist_ok=True)
    f, files = read_archive(os.path.join(game_dir(), 'Data', 'data.ssa'))
    graphics = read_graphics(f, files)
    models = []
    for path in sorted(files):
        base = os.path.basename(path)
        if base.startswith('bld') and base.endswith('.cem'):
            models.append((base[:-4], read_model(Reader(read_file(f, files[path])))))

    # a row of the grid per building, its eras along the row; models without a texture of the game's in the last rows
    def row_of(name):
        return re.sub(r'_?\d+t?$', '', name)

    def texture_of(name):
        candidates = [t for t in graphics.get(name, []) if 'textures\\%s.sst' % t in files]
        return candidates[0] if candidates else None

    models.sort(key=lambda m: (texture_of(m[0]) is None, row_of(m[0]), m[0]))

    images, materials = {}, {}

    def material(texture, image, player):
        """The OBJ's material for an image of a texture, its PNG written once."""
        key = (texture, image, player)
        if key in materials:
            return materials[key]['name']
        name, png = 'no_texture', None
        if texture:
            if texture not in images:
                images[texture] = sst_images(read_file(f, files['textures\\%s.sst' % texture]))
                for i, picture in enumerate(images[texture]):
                    write_png(os.path.join(out_dir, 'Textures', '%s%s.png' % (safe(texture), '_image%d' % (i + 1) if i else '')), picture)
            i = image if image < len(images[texture]) else 0
            png = 'Textures/%s%s.png' % (safe(texture), '_image%d' % (i + 1) if i else '')
            name = safe(texture) + ('_image%d' % (i + 1) if i else '')
        if player:
            name += '__player_color'
        materials[key] = {'name': name, 'png': png}
        return name

    obj = ['# Empire Earth buildings, from the player\'s own data.ssa (scripts/re/cem_export.py); buildings.json places them',
           'mtllib buildings.mtl']
    info = {'axes': 'OBJ x = game x + offset[0], OBJ y = game z, OBJ z = offset[1] - game y; OBJ v = 1 - game v',
            'detail_levels': 'only the full one', 'models': {}}
    counts = [0, 0, 0]   # the OBJ's v, vt, vn so far
    x_cursor, z_cursor, row_depth, last_row = 0.0, 0.0, 0.0, None
    triangles = 0

    for name, model in models:
        all_parts = []
        for part, m in parts(model):
            all_parts.append((part, m, [apply(m, p) for p in part['pos']]))
        xs = [p[0] for _, _, ps in all_parts for p in ps]
        ys = [p[1] for _, _, ps in all_parts for p in ps]
        row = row_of(name)
        if row != last_row:
            if last_row is not None:
                z_cursor += row_depth + GAP
            x_cursor, row_depth, last_row = 0.0, 0.0, row
        depth = max(ys) - min(ys)
        ox = round(x_cursor - min(xs), 3)
        oz = round(z_cursor + max(ys), 3)
        x_cursor += max(xs) - min(xs) + GAP
        row_depth = max(row_depth, depth)
        texture = texture_of(name)
        record = {'file': 'models\\%s.cem' % name, 'offset': [ox, oz], 'texture': texture,
                  'other_textures': [t for t in graphics.get(name, []) if t != texture], 'parts': []}

        used_names = set()
        for part, m, world in all_parts:
            object_name = name if part is model else '%s/%s' % (name, safe(part['name']))
            while object_name in used_names:
                object_name += '_'
            used_names.add(object_name)
            obj.append('o ' + object_name)
            v_seen, vt_seen, vn_seen = {}, {}, {}
            flip = determinant3(m) < 0

            def index(table, key, line, kind):
                if key not in table:
                    obj.append(line)
                    counts[kind] += 1
                    table[key] = counts[kind]
                return table[key]

            for mat in part['materials']:
                if not mat['tris']:
                    continue
                obj.append('usemtl ' + material(texture, mat['image'], mat['name'] == PLAYER_COLOR))
                for tri in mat['tris']:
                    corners = []
                    for i in tri:
                        p = world[i]
                        pos = (p[0] + ox, p[2], oz - p[1])
                        k = tuple(round(c, 6) for c in pos)
                        v = index(v_seen, k, 'v %.6f %.6f %.6f' % k, 0)
                        u, w = part['uv'][i]
                        k = (round(u, 6), round(1 - w, 6))
                        vt = index(vt_seen, k, 'vt %.6f %.6f' % k, 1)
                        if part['normal']:
                            n = apply(m, part['normal'][i], 0.0)
                            k = tuple(round(c, 5) for c in (n[0], n[2], -n[1]))
                            vn = index(vn_seen, k, 'vn %.5f %.5f %.5f' % k, 2)
                            corners.append('%d/%d/%d' % (v, vt, vn))
                        else:
                            corners.append('%d/%d' % (v, vt))
                    if flip:
                        corners.reverse()
                    obj.append('f ' + ' '.join(corners))
                    triangles += 1
            record['parts'].append({'object': object_name, 'name': part['name'], 'tags': part['tags']})
        info['models'][name] = record

    mtl = []
    for m in materials.values():
        if m.get('written'):
            continue
        same = [o for o in materials.values() if o['name'] == m['name']]
        for o in same:
            o['written'] = True
        mtl += ['newmtl ' + m['name'], 'Ka 1 1 1', 'Ks 0 0 0', 'illum 1']
        mtl += ['Kd 1 1 1', 'map_Kd ' + m['png']] if m['png'] else ['Kd 0.8 0 0.8']
        mtl.append('')
    for name, text in (('buildings.obj', obj), ('buildings.mtl', mtl)):
        with open(os.path.join(out_dir, name), 'w', encoding='ascii', newline='\n') as out:
            out.write('\n'.join(text) + '\n')
    with open(os.path.join(out_dir, 'buildings.json'), 'w', encoding='utf-8') as out:
        json.dump(info, out, indent=1)

    none = [n for n, r in info['models'].items() if not r['texture']]
    print('%d models, %d triangles, %d vertices, %d materials, %d textures' % (len(models), triangles, counts[0], len({m['name'] for m in materials.values()}), len(images)))
    print('no texture of the game\'s:', len(none), none)
    print('more than one texture in dbgraphics:', sum(1 for r in info['models'].values() if r['other_textures']))


if __name__ == '__main__':
    main()
