// Sound diagnostics (autotest sound_log=1). The game's sound manager (gSoundManager 0x919848, Low-Level Engine.dll
// SESoundManager) plays through the Miles mixer plugin (Miles Sound System Mixer.dll); "Max Number Of Sounds
// (percent)" sets the channel count to percent x the mixer's maximum (min 8, exe 0x5c0000 area / 0x530000 area via
// SetMaxChannels). The mixer's maximum is DirectSound's free hardware buffers if 1..64, else 16 - on Windows since
// Vista always 16. The log counts per sound id the plays the game asks for and the ones the manager drops (Play
// returns -1 / PlayAndReturnReferencedNode NULL), and the mixer's refusals (SetChannelSample / PlayChannel != 0).
#define LLE_DLL "Low-Level Engine.dll"   // the sound manager's imports (by name, the slots checked against it)
#define SM_MIXER OF(E_SOUNDMGR, 0x70)            // SEMixer *; the mixer's SPIM function pointers are fields of it
#define SM_CHANNELS OF(E_SOUNDMGR, 0x88)
#define MX_SET_CHANNEL_SAMPLE OF(E_MIXER, 0xD8)  // int (cdecl *)(int channel, SESample *)
#define MX_PLAY_CHANNEL OF(E_MIXER, 0xD0)        // int (cdecl *)(int channel)
#define SND_IDS 2048

internal int g_sound_log;
internal uint32_t g_snd_req[SND_IDS], g_snd_drop[SND_IDS], g_snd_req_total, g_snd_drop_total, g_snd_zero_vol;
internal uint32_t g_mix_set_fail[16], g_mix_play_fail[16], g_mix_set_total;
typedef uint32_t (__thiscall *snd_play_fn)(void *, uint32_t, float, float, float, int, void *);
internal snd_play_fn g_snd_play, g_snd_playref;
typedef int (__cdecl *mix_set_fn)(int, void *);
typedef int (__cdecl *mix_play_fn)(int);
internal mix_set_fn g_mix_set;
internal mix_play_fn g_mix_play;
typedef int (__cdecl *mix_vol_fn)(void *, float);
internal mix_vol_fn g_mix_vol;
internal uintptr_t g_mss_mixer;                   // Miles Sound System Mixer.dll base: its channels at RVA_MIX_CHANNELS
internal int (__stdcall *g_ail_status)(void *);   // AIL_sample_status: 4 = playing
internal int snd_status(int ch) {
    uint8_t *v = g_mss_mixer ? *(uint8_t **)(g_mss_mixer + GA(RVA_MIX_CHANNELS)) : NULL, *ve = v ? *(uint8_t **)(g_mss_mixer + GA(RVA_MIX_CHANNELS) + 4) : NULL;
    if (!v || !g_ail_status || ch < 0 || v + ch * 8 >= ve) return -1;
    return g_ail_status(*(void **)(v + ch * 8));
}

// Why the manager dropped a play (Low-Level Engine.dll's SESoundManager, thiscall(sm, id), exported by name - each
// build has them elsewhere): the sound's copies in use and its copies, its category's channels in use and maximum,
// the category.
enum { SM_INSTANCES, SM_MAX_INSTANCES, SM_CAT_INSTANCES, SM_MAX_CAT_INSTANCES, SM_CATEGORY, SM_QUERIES };
internal const char *const k_sm_query[SM_QUERIES] = {
    "?GetNumberOfInstances@SESoundManager@@AAEKK@Z", "?GetMaxInstances@SESoundManager@@AAEKK@Z",
    "?GetNumberOfCategoryInstances@SESoundManager@@AAEKK@Z", "?GetMaxCategoryInstances@SESoundManager@@AAEKK@Z",
    "?GetCategory@SESoundManager@@AAEKK@Z",
};
typedef uint32_t (__thiscall *sm_query_fn)(void *, uint32_t);
internal sm_query_fn g_sm_query[SM_QUERIES];   // all or none
internal uint32_t g_drop_copies, g_drop_cat, g_drop_chan, g_cat_drop[64];
internal void snd_count(void *sm, uint32_t id, float vol, int failed) {
    g_snd_req_total++;
    if (vol == 0.0f) { g_snd_zero_vol++; if (failed) g_snd_drop_total++; return; }   // out of earshot: skipped
    if (id < SND_IDS) g_snd_req[id]++;
    if (!failed) return;
    g_snd_drop_total++;
    if (id < SND_IDS) g_snd_drop[id]++;
    if (!g_sm_query[0]) return;
    uint32_t used = g_sm_query[SM_INSTANCES](sm, id), copies = g_sm_query[SM_MAX_INSTANCES](sm, id);
    uint32_t cused = g_sm_query[SM_CAT_INSTANCES](sm, id), cmax = g_sm_query[SM_MAX_CAT_INSTANCES](sm, id);
    uint32_t cat = g_sm_query[SM_CATEGORY](sm, id);
    if (used >= copies) g_drop_copies++;
    else if (cused >= cmax) { g_drop_cat++; g_cat_drop[cat & 63]++; }
    else g_drop_chan++;
}
internal uint32_t __fastcall hook_snd_play(void *sm, void *edx, uint32_t id, float vol, float pan, float pitch, int queue,
                                         void *name) {
    (void)edx;
    uint32_t r = g_snd_play(sm, id, vol, pan, pitch, queue, name);
    snd_count(sm, id, vol, r == 0xFFFFFFFFu);
    if (g_sound_log >= 2 && g_log)
        log_locked("# snd: %.3f min play id %u vol %.3f pan %.2f pitch %.2f queue %d -> %d\n", game_time() / 60000.0, id, vol,
                pan, pitch, queue & 0xFF, (int)r);
    return r;
}
internal uint32_t __fastcall hook_snd_playref(void *sm, void *edx, uint32_t id, float vol, float pan, float pitch,
                                            int queue, void *name) {
    (void)edx;
    uint32_t r = g_snd_playref(sm, id, vol, pan, pitch, queue, name);
    snd_count(sm, id, vol, r == 0);
    if (g_sound_log >= 2 && g_log)
        log_locked("# snd: %.3f min playref id %u vol %.3f pan %.2f pitch %.2f queue %d -> node %08x channel %d, caller %06x\n",
                game_time() / 60000.0, id, vol, pan, pitch, queue & 0xFF, r, r ? FIELD((void *)(uintptr_t)r, OF(E_SNDNODE, 0x1C), int) : -9,
                (unsigned)(uintptr_t)__builtin_return_address(0));
    return r;
}
internal int __cdecl hook_mix_set(int ch, void *sample) {
    int r = g_mix_set(ch, sample);
    g_mix_set_total++;
    if (g_sound_log >= 2 && g_log) log_locked("# snd:   mixer channel %d sample %p -> %d\n", ch, sample, r);
    if (r) g_mix_set_fail[r & 15]++;
    return r;
}
internal int __cdecl hook_mix_play(int ch) {
    int r = g_mix_play(ch);
    if (r) g_mix_play_fail[r & 15]++;
    if (g_sound_log >= 2 && g_log) log_locked("# snd:   mixer start channel %d -> %d, Miles status %d\n", ch, r, snd_status(ch));
    return r;
}
internal int __cdecl hook_mix_vol(void *sample, float vol) {
    int r = g_mix_vol(sample, vol);
    if (g_sound_log >= 2 && g_log && sample)
        log_locked("# snd:   volume %.3f on channel %d\n", vol, FIELD(sample, OF(E_SAMPLE, 0x24), int));
    return r;
}

// Called from the AI tick: hooks once the sound manager exists, then logs the counts once a game minute.
internal void sound_tick(uint32_t now) {
    static int hooked;
    static uint32_t last;
    if (!g_sound_log || !g_log) return;
    uint8_t *sm = *(uint8_t **)GA(G_SOUND_MANAGER);
    if (!sm) return;
    if (!hooked) {
        hooked = 1;
        int a = iat_hook(GA(IAT_SND_PLAY), LLE_DLL, "?Play@SESoundManager@@QAEKKMMM_NPAD@Z", (void *)hook_snd_play, (void **)&g_snd_play);
        HMODULE lle = GetModuleHandleA(LLE_DLL);
        int q = 0;
        for (int i = 0; lle && i < SM_QUERIES; i++) q += (g_sm_query[i] = (sm_query_fn)(void *)GetProcAddress(lle, k_sm_query[i])) != NULL;
        if (q != SM_QUERIES) memset(g_sm_query, 0, sizeof g_sm_query);
        uint8_t **cv = FIELD(sm, OF(E_SOUNDMGR, 0x54), uint8_t **), **ce = FIELD(sm, OF(E_SOUNDMGR, 0x58), uint8_t **);
        for (int i = 0; cv && cv + i < ce; i++)   // categories: share, max channels, in use, priority
            if (cv[i]) fprintf(g_log, "# sound: category %d: %.2f of the channels = %d, in use %d, priority %d\n", i,
                               FIELD(cv[i], OF(E_SNDCAT, 0x0), float), FIELD(cv[i], OF(E_SNDCAT, 0x4), int), FIELD(cv[i], OF(E_SNDCAT, 0x8), int), FIELD(cv[i], OF(E_SNDCAT, 0xC), int));
        int b = iat_hook(GA(IAT_SND_PLAYREF), LLE_DLL, "?PlayAndReturnReferencedNode@SESoundManager@@QAEPAVSESMInstanceNode@@KMMM_NPAD@Z",
                         (void *)hook_snd_playref, (void **)&g_snd_playref);
        if (!a || !b) fprintf(g_log, "# sound: play hooks %d/2 - FAILED (exe differs)\n", a + b);
        uint8_t *mx = FIELD(sm, SM_MIXER, uint8_t *);
        if (mx) {
            g_mix_set = FIELD(mx, MX_SET_CHANNEL_SAMPLE, mix_set_fn);
            g_mix_play = FIELD(mx, MX_PLAY_CHANNEL, mix_play_fn);
            FIELD(mx, MX_SET_CHANNEL_SAMPLE, void *) = (void *)hook_mix_set;
            FIELD(mx, MX_PLAY_CHANNEL, void *) = (void *)hook_mix_play;
            g_mix_vol = FIELD(mx, OF(E_MIXER, 0xF0), mix_vol_fn);   // SPIMSetSampleVolume
            FIELD(mx, OF(E_MIXER, 0xF0), void *) = (void *)hook_mix_vol;
            g_mss_mixer = (uintptr_t)GetModuleHandleA("Miles Sound System Mixer.dll");
            HMODULE mss = GetModuleHandleA("mss32.dll");
            if (mss) g_ail_status = (int (__stdcall *)(void *))GetProcAddress(mss, "_AIL_sample_status@4");
        }
        fprintf(g_log, "# sound: log on, %u channels, why a play was dropped: %s\n", FIELD(sm, SM_CHANNELS, uint32_t),
                g_sm_query[0] ? "known" : "not known (the manager's queries not exported)");
        last = now;
        return;
    }
    if (now - last < 60000) return;
    last = now;
    char top[512] = "";
    int o = 0;
    static uint8_t listed[SND_IDS];
    memset(listed, 0, sizeof listed);
    for (int k = 0; k < 12; k++) {   // the sound ids with the most drops
        uint32_t best = 0, bi = 0;
        for (uint32_t i = 0; i < SND_IDS; i++)
            if (!listed[i] && g_snd_drop[i] > best) best = g_snd_drop[i], bi = i;
        if (!best) break;
        listed[bi] = 1;
        o += snprintf(top + o, sizeof top - o, " %u:%u/%u", bi, best, g_snd_req[bi]);
    }
    uint32_t set_fail = 0;
    for (int i = 0; i < 16; i++) set_fail += g_mix_set_fail[i];
    char cats[256] = "";
    int co = 0;
    for (int i = 0; i < 64; i++) if (g_cat_drop[i]) co += snprintf(cats + co, sizeof cats - co, " %d:%u", i, g_cat_drop[i]);
    fprintf(g_log, "# sound: %.1f min: audible drops: sound's copies all busy %u, category full %u (%s ), no channel %u\n",
            now / 60000.0, g_drop_copies, g_drop_cat, cats, g_drop_chan);
    fprintf(g_log, "# sound: %.1f min, %u channels: %u plays, %u dropped (%u at volume 0); mixer refused %u of %u"
                   " (busy %u, sample on another channel %u), start failed %u; most dropped (id:dropped/asked):%s\n",
            now / 60000.0, FIELD(sm, SM_CHANNELS, uint32_t), g_snd_req_total, g_snd_drop_total, g_snd_zero_vol,
            set_fail, g_mix_set_total, g_mix_set_fail[5], g_mix_set_fail[13], g_mix_play_fail[0] + g_mix_play_fail[1]
            + g_mix_play_fail[2] + g_mix_play_fail[10], top);
}

// CHANNEL FIX (aimod.ini [sound] clamp_channels, default on). The options slider "Max Number Of Sounds" goes to 200%:
// channels = round(percent x the mixer's maximum), so 200% asks for 32 while the Miles mixer creates at most its
// maximum (16 on Windows since Vista, see above). The manager then plays on channels 16..31 that don't exist: the
// mixer refuses the sample (error 10), nothing sounds, and the node stays "playing" there - a looping sound (unit
// movement, ambience) keeps its copy for good, so that sound never plays again. The paused queue resumes from the
// highest channel down, and a change in a running game pauses every looping sound, so there it shows at once.
// SetMaxChannels (exe IAT 0x823AD4) is held to the mixer's maximum: 200% = 100% = 16 channels.
internal int g_clamp_channels = 1;
typedef int (__thiscall *set_max_ch_fn)(void *, uint32_t);
internal set_max_ch_fn g_set_max_ch;
// MORE CHANNELS (aimod.ini [sound] channels, default 32; 0 = the mixer's own maximum). The mixer's maximum is 16
// only because Windows reports no hardware voices; Miles mixes in software and is opened for 64 samples
// (DIG_MIXER_CHANNELS), one of which the plugin keeps. The maximum (the global SPIMGetMaximumChannelCount reads,
// `mov eax, [addr]; ret`) is raised to `channels` (8..63) before the game's first SetMaxChannels, and the count the
// game passes - percent x the old maximum - is worked out again from the slider (0x9193B8, 1.0 = 100%): 100% =
// `channels`, 0% = 8, 200% is held to `channels` by clamp_channels.
internal int g_sound_channels = 32;
internal int __fastcall hook_set_max_channels(uint8_t *sm, void *edx, uint32_t n) {
    (void)edx;
    static int raised;
    uint8_t *mx = sm ? FIELD(sm, SM_MIXER, uint8_t *) : NULL;
    uint8_t *get_max = mx ? FIELD(mx, OF(E_MIXER, 0xB0), uint8_t *) : NULL;   // SPIMGetMaximumChannelCount
    if (!raised && get_max && g_sound_channels >= 8 && g_sound_channels <= 63 && get_max[0] == 0xA1 && get_max[5] == 0xC3) {
        raised = 1;
        uint32_t *gmax = *(uint32_t **)(get_max + 1);
        DWORD old;
        if (VirtualProtect(gmax, 4, PAGE_READWRITE, &old)) {
            uint32_t was = *gmax;
            *gmax = (uint32_t)g_sound_channels;
            VirtualProtect(gmax, 4, old, &old);
            if (g_log) fprintf(g_log, "# sound: mixer maximum %u -> %d channels\n", was, g_sound_channels);
        }
    }
    if (raised) {   // the game's count used the old maximum: percent x the new one, at least 8 as the game does
        float pct = *(float *)GA(G_SOUND_PERCENT);
        int want = (int)(pct * (float)g_sound_channels + 0.5f);
        n = (uint32_t)(want < 8 ? 8 : want);
    }
    uint32_t max = get_max ? ((uint32_t (__cdecl *)(void))(void *)get_max)() : 0;
    if (g_clamp_channels && max && n > max) {
        if (g_log) fprintf(g_log, "# sound: %u channels asked for, the mixer has %u - using %u\n", n, max, max);
        n = max;
    }
    return g_set_max_ch(sm, n);
}

internal void sound_install(const char *aimod_ini, const char *autotest_ini) {
    g_clamp_channels = ini_int("sound", "clamp_channels", 1, aimod_ini);
    g_sound_channels = ini_int("sound", "channels", 32, aimod_ini);
    if (autotest_ini) g_sound_channels = ini_int("autotest", "sound_channels", g_sound_channels, autotest_ini);
    if (autotest_ini) g_clamp_channels = ini_int("autotest", "clamp_channels", g_clamp_channels, autotest_ini);
    int ok = iat_hook(GA(IAT_SND_SETMAXCH), LLE_DLL, "?SetMaxChannels@SESoundManager@@QAEJK@Z", (void *)hook_set_max_channels, (void **)&g_set_max_ch);
    if (g_log) fprintf(g_log, "# sound: clamp_channels=%d channels=%d (hook %s)\n", g_clamp_channels, g_sound_channels, ok ? "ok" : "FAILED");
    // aimod.ini [sound] log = 1 (counts once a minute) | 2 (every play) - diagnostics in normal play too
    g_sound_log = ini_int("sound", "log", 0, aimod_ini);
    if (autotest_ini) g_sound_log = ini_int("autotest", "sound_log", g_sound_log, autotest_ini);
}

// sndstate: the manager's channels (node, sound id, node channel, sample channel, Miles status), the mixer's
// channels, categories in use, the paused queue. sndchan N: SetMaxChannels(N) as the options screen does. Tests.
internal void cmd_sndstate(char *out, const char *args) {
    (void)args;
    uint8_t *sm = *(uint8_t **)GA(G_SOUND_MANAGER);
    if (!sm) { REPLY("error: no sound manager\n"); return; }
    uint8_t *mv = g_mss_mixer ? *(uint8_t **)(g_mss_mixer + GA(RVA_MIX_CHANNELS)) : NULL, *me = mv ? *(uint8_t **)(g_mss_mixer + GA(RVA_MIX_CHANNELS) + 4) : NULL;
    REPLY("setting %.2f (0x9193B8), mixer maximum %d, ", *(float *)GA(G_SOUND_PERCENT),
          g_mss_mixer ? *(int *)(g_mss_mixer + GA(RVA_MIX_MAX_CHANNELS)) : -1);
    int nulls = 0;
    for (uint8_t *h = mv; h && h < me; h += 8) nulls += !*(void **)h;
    REPLY("handles missing %d, ", nulls);
    REPLY("channels %u, mixer channels %d, paused %d:", FIELD(sm, SM_CHANNELS, uint32_t), mv ? (int)((me - mv) / 8) : -1,
          (int)((FIELD(sm, OF(E_SOUNDMGR, 0x38), uint8_t *) - FIELD(sm, OF(E_SOUNDMGR, 0x34), uint8_t *)) / 4));
    uint8_t **slots = FIELD(sm, OF(E_SOUNDMGR, 0x24), uint8_t **);
    for (uint32_t i = 0; slots && i < FIELD(sm, SM_CHANNELS, uint32_t); i++) {
        uint8_t *n = slots[i], *smp = n ? FIELD(n, OF(E_SNDNODE, 0x20), uint8_t *) : NULL;
        if (n) REPLY(" [%u: id %d node ch %d sample ch %d status %d]", i, FIELD(n, OF(E_SNDNODE, 0xC), int), FIELD(n, OF(E_SNDNODE, 0x1C), int),
                     smp ? FIELD(smp, OF(E_SAMPLE, 0x24), int) : -9, snd_status((int)i));
    }
    uint8_t **cv = FIELD(sm, OF(E_SOUNDMGR, 0x54), uint8_t **), **ce = FIELD(sm, OF(E_SOUNDMGR, 0x58), uint8_t **);
    for (int i = 0; cv && cv + i < ce; i++)
        if (cv[i] && FIELD(cv[i], OF(E_SNDCAT, 0x8), int)) REPLY(" cat %d %d/%d", i, FIELD(cv[i], OF(E_SNDCAT, 0x8), int), FIELD(cv[i], OF(E_SNDCAT, 0x4), int));
    REPLY("\n");
}
internal void cmd_sndchan(char *out, const char *args) {
    int n = atoi(args);
    uint8_t *sm = *(uint8_t **)GA(G_SOUND_MANAGER);
    if (!sm || n < 1 || n > 64) { REPLY("error: usage sndchan N (1..64)\n"); return; }
    int r = (*(int (__thiscall **)(void *, int))GA(IAT_SND_SETMAXCH))(sm, n);
    REPLY("ok: SetMaxChannels(%d) -> %d, now %u channels\n", n, r, FIELD(sm, SM_CHANNELS, uint32_t));
}
