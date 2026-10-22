// VECTORS: a position, a direction or a tile as one value - vec2 / vec3 floats (positions in tiles, the game's
// world units; z up), ivec2 a tile (whole tiles, as the game's int pos[2]: the same layout, &p.x where it wants one).
// The names and the order of the parts are the text language's (vec(x, y [, z [, w]]): script/expr.c), so a value
// crosses between them as it is. Each operation is the arithmetic written out, in the order a hand-written
// a.x * b.x + a.y * b.y would do it: the same bits on every PC (lockstep multiplayer) - only + - * / and sqrtf
// (SSE, exact), nothing from the C runtime that could differ between Windows versions.

typedef struct { float x, y; } vec2;
typedef struct { float x, y, z; } vec3;
typedef struct { int x, y; } ivec2;

internal inline vec2 v2(float x, float y) { vec2 r = { x, y }; return r; }
internal inline vec3 v3(float x, float y, float z) { vec3 r = { x, y, z }; return r; }
internal inline ivec2 iv2(int x, int y) { ivec2 r = { x, y }; return r; }

internal inline vec2 vec2_add(vec2 a, vec2 b) { return v2(a.x + b.x, a.y + b.y); }
internal inline vec2 vec2_sub(vec2 a, vec2 b) { return v2(a.x - b.x, a.y - b.y); }
internal inline vec2 vec2_scale(vec2 a, float s) { return v2(a.x * s, a.y * s); }
internal inline float vec2_dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
internal inline float vec2_len2(vec2 a) { return a.x * a.x + a.y * a.y; }
internal inline float vec2_len(vec2 a) { return sqrtf(vec2_len2(a)); }
internal inline float vec2_dist2(vec2 a, vec2 b) { return vec2_len2(vec2_sub(a, b)); }

internal inline vec3 vec3_add(vec3 a, vec3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
internal inline vec3 vec3_sub(vec3 a, vec3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
internal inline vec3 vec3_scale(vec3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
internal inline vec3 vec3_div(vec3 a, float s) { return v3(a.x / s, a.y / s, a.z / s); }   // not * (1 / s): other bits
internal inline float vec3_dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
internal inline float vec3_len2(vec3 a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
internal inline float vec3_len(vec3 a) { return sqrtf(vec3_len2(a)); }
internal inline vec2 vec3_xy(vec3 a) { return v2(a.x, a.y); }   // on the ground
// the game's float p[3]: read into a value, written back
internal inline vec3 vec3_load(const float *p) { return v3(p[0], p[1], p[2]); }
internal inline void vec3_store(float *p, vec3 a) { p[0] = a.x; p[1] = a.y; p[2] = a.z; }

internal inline ivec2 ivec2_add(ivec2 a, ivec2 b) { return iv2(a.x + b.x, a.y + b.y); }
internal inline ivec2 ivec2_sub(ivec2 a, ivec2 b) { return iv2(a.x - b.x, a.y - b.y); }
internal inline int ivec2_eq(ivec2 a, ivec2 b) { return a.x == b.x && a.y == b.y; }
internal inline int ivec2_len2(ivec2 a) { return a.x * a.x + a.y * a.y; }
internal inline int ivec2_dist2(ivec2 a, ivec2 b) { int dx = a.x - b.x, dy = a.y - b.y; return dx * dx + dy * dy; }
internal inline vec2 ivec2_centre(ivec2 t) { return v2((float)t.x + 0.5f, (float)t.y + 0.5f); }   // a tile's middle
internal inline ivec2 vec2_tile(vec2 a) { return iv2((int)floorf(a.x), (int)floorf(a.y)); }       // the tile a point lies on
