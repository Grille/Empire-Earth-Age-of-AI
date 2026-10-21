// log2 with the same bits on every PC (lockstep multiplayer: every PC must decide alike) - no library call, whose
// result can differ by a bit between CPUs
static float det_log2f(float x) {
    if (!(x > 0)) return -1000.0f;
    union { float f; uint32_t u; } b = { x };
    int e = (int)((b.u >> 23) & 0xFF) - 127;
    b.u = (b.u & 0x007FFFFF) | 0x3F800000;
    float t = (b.f - 1.0f) / (b.f + 1.0f), t2 = t * t;
    return (float)e + 2.0f * t * (1.0f + t2 * (1.0f / 3 + t2 * (1.0f / 5 + t2 * (1.0f / 7)))) * 1.44269504f;
}
