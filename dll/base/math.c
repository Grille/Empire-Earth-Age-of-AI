// FLOOR AND CEIL WITHOUT THE LIBRARY: its floorf and ceilf (the target has SSE2, not SSE4.1's roundss) switch the x87's
// rounding mode there and back with two fldcw at each call - a fully zoomed-out paused frame of 3000 units spent 19 %
// of the render thread in them (the shadows' rows widened toward the sun, render_shadows.c shr_hook_rows: ~10^5 calls a
// frame). Here by the conversion toward zero, one step down (up) when that passed x; the same results to the bit -
// the sign of a zero too (floorf(-0.0) and ceilf(-0.5) are -0), whole numbers past 2^23, infinities and NaN as given.
internal inline float floor_exact(float x) {
    if (!(fabsf(x) < 8388608.0f)) return x;
    float t = (float)(int32_t)x;
    if (t > x) t -= 1.0f;
    return t == 0.0f ? copysignf(0.0f, x) : t;
}
internal inline float ceil_exact(float x) {
    if (!(fabsf(x) < 8388608.0f)) return x;
    float t = (float)(int32_t)x;
    if (t < x) t += 1.0f;
    return t == 0.0f ? copysignf(0.0f, x) : t;
}
#define floorf(x) floor_exact(x)
#define ceilf(x) ceil_exact(x)

// log2 with the same bits on every PC (lockstep multiplayer: every PC must decide alike) - no library call, whose
// result can differ by a bit between CPUs
internal float det_log2f(float x) {
    if (!(x > 0)) return -1000.0f;
    union { float f; uint32_t u; } b = { x };
    int e = (int)((b.u >> 23) & 0xFF) - 127;
    b.u = (b.u & 0x007FFFFF) | 0x3F800000;
    float t = (b.f - 1.0f) / (b.f + 1.0f), t2 = t * t;
    return (float)e + 2.0f * t * (1.0f + t2 * (1.0f / 3 + t2 * (1.0f / 5 + t2 * (1.0f / 7)))) * 1.44269504f;
}
