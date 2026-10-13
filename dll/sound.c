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

static void sound_install(const char *aimod_ini, const char *autotest_ini) {
    // aimod.ini [sound] log = 1 (counts once a minute) | 2 (every play) - diagnostics in normal play too
    g_sound_log = GetPrivateProfileIntA("sound", "log", 0, aimod_ini);
    if (autotest_ini) g_sound_log = GetPrivateProfileIntA("autotest", "sound_log", g_sound_log, autotest_ini);
}
