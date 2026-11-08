// THE WATER BY ITS DEPTH ([display] water_look, the player 2026-11-08: "keep the look but with subtle upgrades"). The
// game draws the surface at one alpha over a seabed as sharp in the deep as in the shallows. The surface's shader
// knows the depth under it (the hills' map holds the ground, render_shadows.c): the surface lets less of the seabed
// through the deeper it is, the water's own colour its fog, more in the shallows, and thins out to nothing at the
// shore instead of ending in a hard line. The seabed's colour is the game's, painted by depth into its vertices
// (turquoise shallows, dark deeps) - that is the water's look, and a surface made thicker with the depth covered it,
// the sea one flat blue. So the deep takes away the seabed's texture detail only, toward the texture's mean. Tried
// and dropped: the ground darkened by its height over the water - the land on an island map lies within a few
// hundredths of the level, whole tiles of it went dark.
// Needs the hills' map: dynamic shadows, the hills on.
internal struct {
    int on;          // -1: as the ini says
    float fog;       // the depth (tiles) by which 63% of the way from the shallows' alpha to the deep's is done
    float shallow;   // the surface's alpha times this at no depth
    float deep;      // and at great depth
    float edge;      // the depth (tiles) over which the surface thins out to nothing at the shore
    float sea;       // how much of the seabed's texture detail the deep takes away (0..1)
    float seadepth;  // the depth (tiles) by which 63% of that is gone
} g_wa = { .on = -1, .fog = 0.35f, .shallow = 0.75f, .deep = 1.0f, .edge = 0.05f, .sea = 0.75f, .seadepth = 0.35f };

internal void rd3_water_pc(rd3_pcb *pc, float level, int water_seen) {
    int on = (g_wa.on < 0 ? g_set.water_look : g_wa.on) && water_seen;
    pc->wat[0] = on ? 1 / g_wa.fog : 0;
    pc->wat[1] = g_wa.shallow;
    pc->wat[2] = g_wa.deep;
    pc->wat[3] = 1 / g_wa.edge;
    pc->sea[0] = on ? g_wa.sea : 0;
    pc->sea[1] = 1 / g_wa.seadepth;
    pc->sea[2] = level;
}

// the pipe: water [on | off] | fog <depth> | shallow <times> | deep <times> | edge <depth> | sea <0..1> | seadepth <depth>
internal void cmd_water(char *out, const char *args) {
    char mode[32] = "";
    float v = 0;
    int n = sscanf(args, "%31s %f", mode, &v);
    if (n >= 1 && !strcmp(mode, "on")) g_wa.on = 1;
    else if (n >= 1 && !strcmp(mode, "off")) g_wa.on = 0;
    else if (n == 2 && !strcmp(mode, "fog") && v > 0) g_wa.fog = v;
    else if (n == 2 && !strcmp(mode, "shallow") && v >= 0) g_wa.shallow = v;
    else if (n == 2 && !strcmp(mode, "deep") && v >= 0) g_wa.deep = v;
    else if (n == 2 && !strcmp(mode, "edge") && v > 0) g_wa.edge = v;
    else if (n == 2 && !strcmp(mode, "sea") && v >= 0 && v <= 1) g_wa.sea = v;
    else if (n == 2 && !strcmp(mode, "seadepth") && v > 0) g_wa.seadepth = v;
    else if (n >= 1) { REPLY("error: usage: water [on | off] | fog <depth> | shallow <times> | deep <times> | edge <depth> | sea <0..1> | seadepth <depth>\n"); return; }
    REPLY("water: %s, fog %.2f, shallow %.2f, deep %.2f, edge %.2f, sea %.2f by %.2f\n",
          (g_wa.on < 0 ? g_set.water_look : g_wa.on) ? "on" : "off", (double)g_wa.fog, (double)g_wa.shallow, (double)g_wa.deep,
          (double)g_wa.edge, (double)g_wa.sea, (double)g_wa.seadepth);
}
