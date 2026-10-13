// Sound diagnostics (autotest sound_log=1). The game's sound manager (gSoundManager 0x919848, Low-Level Engine.dll
// SESoundManager) plays through the Miles mixer plugin (Miles Sound System Mixer.dll); "Max Number Of Sounds
// (percent)" sets the channel count to percent x the mixer's maximum (min 8, exe 0x5c0000 area / 0x530000 area via
// SetMaxChannels). The mixer's maximum is DirectSound's free hardware buffers if 1..64, else 16 - on Windows since
// Vista always 16. The log counts per sound id the plays the game asks for and the ones the manager drops (Play
// returns -1 / PlayAndReturnReferencedNode NULL), and the mixer's refusals (SetChannelSample / PlayChannel != 0).
#define G_SOUND_MANAGER 0x00919848
#define IAT_SND_PLAY 0x008232D0      // SESoundManager::Play(id, vol, pan, pitch, bool queue, char *) -> id or -1
#define IAT_SND_PLAYREF 0x008233EC   // SESoundManager::PlayAndReturnReferencedNode(same) -> node or NULL
#define SM_MIXER 0x70                // SEMixer *; the mixer's SPIM function pointers are fields of it
#define SM_CHANNELS 0x88
#define MX_SET_CHANNEL_SAMPLE 0xD8   // int (cdecl *)(int channel, SESample *)
#define MX_PLAY_CHANNEL 0xD0         // int (cdecl *)(int channel)
#define SND_IDS 2048
#define IAT_SND_SETMAXCH 0x00823AD4

static int g_sound_log;
static uint32_t g_snd_req[SND_IDS], g_snd_drop[SND_IDS], g_snd_req_total, g_snd_drop_total, g_snd_zero_vol;
static uint32_t g_mix_set_fail[16], g_mix_play_fail[16], g_mix_set_total;
typedef uint32_t (__thiscall *snd_play_fn)(void *, uint32_t, float, float, float, int, void *);
static snd_play_fn g_snd_play, g_snd_playref;
typedef int (__cdecl *mix_set_fn)(int, void *);
typedef int (__cdecl *mix_play_fn)(int);
static mix_set_fn g_mix_set;
static mix_play_fn g_mix_play;
typedef int (__cdecl *mix_vol_fn)(void *, float);
static mix_vol_fn g_mix_vol;
static uintptr_t g_mss_mixer;                   // Miles Sound System Mixer.dll base: channel handles at +0x52AC
static int (__stdcall *g_ail_status)(void *);   // AIL_sample_status: 4 = playing
static int snd_status(int ch) {
    uint8_t *v = g_mss_mixer ? *(uint8_t **)(g_mss_mixer + 0x52AC) : NULL, *ve = v ? *(uint8_t **)(g_mss_mixer + 0x52B0) : NULL;
    if (!v || !g_ail_status || ch < 0 || v + ch * 8 >= ve) return -1;
    return g_ail_status(*(void **)(v + ch * 8));
}

// Why the manager dropped a play (Low-Level Engine.dll, thiscall(sm, id), relative to its Play export 0x1005B50B):
// 0x1005AC64 the sound's copies in use, 0x1005ABDD its copies, 0x1005AC31 its category's channels in use,
// 0x1005ABAA the category's maximum, 0x1005A9E8 the category.
static uintptr_t g_lle;
static uint32_t g_drop_copies, g_drop_cat, g_drop_chan, g_cat_drop[64];
static uint32_t lle_call(uint32_t rva, void *sm, uint32_t id) {
    return ((uint32_t (__thiscall *)(void *, uint32_t))(g_lle + rva))(sm, id);
}
static void snd_count(void *sm, uint32_t id, float vol, int failed) {
    g_snd_req_total++;
    if (vol == 0.0f) { g_snd_zero_vol++; if (failed) g_snd_drop_total++; return; }   // out of earshot: skipped
    if (id < SND_IDS) g_snd_req[id]++;
    if (!failed) return;
    g_snd_drop_total++;
    if (id < SND_IDS) g_snd_drop[id]++;
    if (!g_lle) return;
    uint32_t used = lle_call(0x5AC64, sm, id), copies = lle_call(0x5ABDD, sm, id);
    uint32_t cused = lle_call(0x5AC31, sm, id), cmax = lle_call(0x5ABAA, sm, id), cat = lle_call(0x5A9E8, sm, id);
    if (used >= copies) g_drop_copies++;
    else if (cused >= cmax) { g_drop_cat++; g_cat_drop[cat & 63]++; }
    else g_drop_chan++;
}
static uint32_t __fastcall hook_snd_play(void *sm, void *edx, uint32_t id, float vol, float pan, float pitch, int queue,
                                         void *name) {
    (void)edx;
    uint32_t r = g_snd_play(sm, id, vol, pan, pitch, queue, name);
    snd_count(sm, id, vol, r == 0xFFFFFFFFu);
    if (g_sound_log >= 2 && g_log)
        fprintf(g_log, "# snd: %.3f min play id %u vol %.3f pan %.2f pitch %.2f queue %d -> %d\n", game_time() / 60000.0, id, vol,
                pan, pitch, queue & 0xFF, (int)r);
    return r;
}
static uint32_t __fastcall hook_snd_playref(void *sm, void *edx, uint32_t id, float vol, float pan, float pitch,
                                            int queue, void *name) {
    (void)edx;
    uint32_t r = g_snd_playref(sm, id, vol, pan, pitch, queue, name);
    snd_count(sm, id, vol, r == 0);
    if (g_sound_log >= 2 && g_log)
        fprintf(g_log, "# snd: %.3f min playref id %u vol %.3f pan %.2f pitch %.2f queue %d -> node %08x channel %d, caller %06x\n",
                game_time() / 60000.0, id, vol, pan, pitch, queue & 0xFF, r, r ? FIELD((void *)(uintptr_t)r, 0x1C, int) : -9,
                (unsigned)(uintptr_t)__builtin_return_address(0));
    return r;
}
static int __cdecl hook_mix_set(int ch, void *sample) {
    int r = g_mix_set(ch, sample);
    g_mix_set_total++;
    if (g_sound_log >= 2 && g_log) fprintf(g_log, "# snd:   mixer channel %d sample %p -> %d\n", ch, sample, r);
    if (r) g_mix_set_fail[r & 15]++;
    return r;
}
static int __cdecl hook_mix_play(int ch) {
    int r = g_mix_play(ch);
    if (r) g_mix_play_fail[r & 15]++;
    if (g_sound_log >= 2 && g_log) fprintf(g_log, "# snd:   mixer start channel %d -> %d, Miles status %d\n", ch, r, snd_status(ch));
    return r;
}
static int __cdecl hook_mix_vol(void *sample, float vol) {
    int r = g_mix_vol(sample, vol);
    if (g_sound_log >= 2 && g_log && sample)
        fprintf(g_log, "# snd:   volume %.3f on channel %d\n", vol, FIELD(sample, 0x24, int));
    return r;
}

static void iat_swap(uintptr_t slot, void *fn, void **orig) {
    DWORD old;
    if (!VirtualProtect((void *)slot, 4, PAGE_READWRITE, &old)) return;
    *orig = *(void **)slot;
    *(void **)slot = fn;
    VirtualProtect((void *)slot, 4, old, &old);
}

// Called from the AI tick: hooks once the sound manager exists, then logs the counts once a game minute.
static void sound_tick(uint32_t now) {
    static int hooked;
    static uint32_t last;
    if (!g_sound_log || !g_log) return;
    uint8_t *sm = *(uint8_t **)G_SOUND_MANAGER;
    if (!sm) return;
    if (!hooked) {
        hooked = 1;
        iat_swap(IAT_SND_PLAY, (void *)hook_snd_play, (void **)&g_snd_play);
        g_lle = (uintptr_t)g_snd_play - 0x5B50B;
        uint8_t **cv = FIELD(sm, 0x54, uint8_t **), **ce = FIELD(sm, 0x58, uint8_t **);
        for (int i = 0; cv && cv + i < ce; i++)   // categories: share, max channels, in use, priority
            if (cv[i]) fprintf(g_log, "# sound: category %d: %.2f of the channels = %d, in use %d, priority %d\n", i,
                               FIELD(cv[i], 0, float), FIELD(cv[i], 4, int), FIELD(cv[i], 8, int), FIELD(cv[i], 0xC, int));
        iat_swap(IAT_SND_PLAYREF, (void *)hook_snd_playref, (void **)&g_snd_playref);
        uint8_t *mx = FIELD(sm, SM_MIXER, uint8_t *);
        if (mx) {
            g_mix_set = FIELD(mx, MX_SET_CHANNEL_SAMPLE, mix_set_fn);
            g_mix_play = FIELD(mx, MX_PLAY_CHANNEL, mix_play_fn);
            FIELD(mx, MX_SET_CHANNEL_SAMPLE, void *) = (void *)hook_mix_set;
            FIELD(mx, MX_PLAY_CHANNEL, void *) = (void *)hook_mix_play;
            g_mix_vol = FIELD(mx, 0xF0, mix_vol_fn);   // SPIMSetSampleVolume
            FIELD(mx, 0xF0, void *) = (void *)hook_mix_vol;
            g_mss_mixer = (uintptr_t)g_mix_set - 0x1874;   // SPIMSetChannelSample = base + 0x1874
            HMODULE mss = GetModuleHandleA("mss32.dll");
            if (mss) g_ail_status = (int (__stdcall *)(void *))GetProcAddress(mss, "_AIL_sample_status@4");
        }
        fprintf(g_log, "# sound: log on, %u channels\n", FIELD(sm, SM_CHANNELS, uint32_t));
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
static int g_clamp_channels = 1;
typedef int (__thiscall *set_max_ch_fn)(void *, uint32_t);
static set_max_ch_fn g_set_max_ch;
// MORE CHANNELS (aimod.ini [sound] channels, default 32; 0 = the mixer's own maximum). The mixer's maximum is 16
// only because Windows reports no hardware voices; Miles mixes in software and is opened for 64 samples
// (DIG_MIXER_CHANNELS), one of which the plugin keeps. The maximum (the global SPIMGetMaximumChannelCount reads,
// `mov eax, [addr]; ret`) is raised to `channels` (8..63) before the game's first SetMaxChannels, and the count the
// game passes - percent x the old maximum - is worked out again from the slider (0x9193B8, 1.0 = 100%): 100% =
// `channels`, 0% = 8, 200% is held to `channels` by clamp_channels.
static int g_sound_channels = 32;
#define G_SOUND_PERCENT 0x009193B8
static int __fastcall hook_set_max_channels(uint8_t *sm, void *edx, uint32_t n) {
    (void)edx;
    static int raised;
    uint8_t *mx = sm ? FIELD(sm, SM_MIXER, uint8_t *) : NULL;
    uint8_t *get_max = mx ? FIELD(mx, 0xB0, uint8_t *) : NULL;   // SPIMGetMaximumChannelCount
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
        float pct = *(float *)G_SOUND_PERCENT;
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

static void sound_install(const char *aimod_ini, const char *autotest_ini) {
    g_clamp_channels = GetPrivateProfileIntA("sound", "clamp_channels", 1, aimod_ini);
    g_sound_channels = GetPrivateProfileIntA("sound", "channels", 32, aimod_ini);
    if (autotest_ini) g_sound_channels = GetPrivateProfileIntA("autotest", "sound_channels", g_sound_channels, autotest_ini);
    if (autotest_ini) g_clamp_channels = GetPrivateProfileIntA("autotest", "clamp_channels", g_clamp_channels, autotest_ini);
    iat_swap(IAT_SND_SETMAXCH, (void *)hook_set_max_channels, (void **)&g_set_max_ch);
    if (g_log) fprintf(g_log, "# sound: clamp_channels=%d channels=%d (hook %d)\n", g_clamp_channels, g_sound_channels, g_set_max_ch != NULL);
    // aimod.ini [sound] log = 1 (counts once a minute) | 2 (every play) - diagnostics in normal play too
    g_sound_log = GetPrivateProfileIntA("sound", "log", 0, aimod_ini);
    if (autotest_ini) g_sound_log = GetPrivateProfileIntA("autotest", "sound_log", g_sound_log, autotest_ini);
}

// sndstate: the manager's channels (node, sound id, node channel, sample channel, Miles status), the mixer's
// channels, categories in use, the paused queue. sndchan N: SetMaxChannels(N) as the options screen does. Tests.
static void cmd_sndstate(char *out, const char *args) {
    (void)args;
    uint8_t *sm = *(uint8_t **)G_SOUND_MANAGER;
    if (!sm) { REPLY("error: no sound manager\n"); return; }
    uint8_t *mv = g_mss_mixer ? *(uint8_t **)(g_mss_mixer + 0x52AC) : NULL, *me = mv ? *(uint8_t **)(g_mss_mixer + 0x52B0) : NULL;
    REPLY("setting %.2f (0x9193B8), mixer maximum %d, ", *(float *)0x009193B8,
          g_mss_mixer ? *(int *)(g_mss_mixer + 0x5320) : -1);
    int nulls = 0;
    for (uint8_t *h = mv; h && h < me; h += 8) nulls += !*(void **)h;
    REPLY("handles missing %d, ", nulls);
    REPLY("channels %u, mixer channels %d, paused %d:", FIELD(sm, SM_CHANNELS, uint32_t), mv ? (int)((me - mv) / 8) : -1,
          (int)((FIELD(sm, 0x38, uint8_t *) - FIELD(sm, 0x34, uint8_t *)) / 4));
    uint8_t **slots = FIELD(sm, 0x24, uint8_t **);
    for (uint32_t i = 0; slots && i < FIELD(sm, SM_CHANNELS, uint32_t); i++) {
        uint8_t *n = slots[i], *smp = n ? FIELD(n, 0x20, uint8_t *) : NULL;
        if (n) REPLY(" [%u: id %d node ch %d sample ch %d status %d]", i, FIELD(n, 0xC, int), FIELD(n, 0x1C, int),
                     smp ? FIELD(smp, 0x24, int) : -9, snd_status((int)i));
    }
    uint8_t **cv = FIELD(sm, 0x54, uint8_t **), **ce = FIELD(sm, 0x58, uint8_t **);
    for (int i = 0; cv && cv + i < ce; i++)
        if (cv[i] && FIELD(cv[i], 8, int)) REPLY(" cat %d %d/%d", i, FIELD(cv[i], 8, int), FIELD(cv[i], 4, int));
    REPLY("\n");
}
static void cmd_sndchan(char *out, const char *args) {
    int n = atoi(args);
    uint8_t *sm = *(uint8_t **)G_SOUND_MANAGER;
    if (!sm || n < 1 || n > 64) { REPLY("error: usage sndchan N (1..64)\n"); return; }
    int r = (*(int (__thiscall **)(void *, int))IAT_SND_SETMAXCH)(sm, n);
    REPLY("ok: SetMaxChannels(%d) -> %d, now %u channels\n", n, r, FIELD(sm, SM_CHANNELS, uint32_t));
}
