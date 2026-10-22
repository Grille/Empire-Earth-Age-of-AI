// THE SOUND (the engine DLLs' sound manager, its categories, nodes, samples, the mixer): fields, functions, hooks.

// FIELDS
// the sound manager (SESoundManager): its channel slots +0x24 (nodes), the paused +0x34 .. +0x38, the categories +0x54
// .. +0x58, the mixer +0x70, the channel count +0x88
internal inline uint8_t **sndmgr_get_slots(const void *sm) { return FIELD(sm, E_SOUNDMGR_SLOTS, uint8_t **); }
internal inline uint8_t *sndmgr_get_paused_begin(const void *sm) { return FIELD(sm, E_SOUNDMGR_PAUSED, uint8_t *); }
internal inline uint8_t *sndmgr_get_paused_end(const void *sm) { return FIELD(sm, E_SOUNDMGR_PAUSED + 0x4, uint8_t *); }
internal inline uint8_t **sndmgr_get_cats_begin(const void *sm) { return FIELD(sm, E_SOUNDMGR_CATS, uint8_t **); }
internal inline uint8_t **sndmgr_get_cats_end(const void *sm) { return FIELD(sm, E_SOUNDMGR_CATS + 0x4, uint8_t **); }
internal inline uint8_t *sndmgr_get_mixer(const void *sm) { return FIELD(sm, E_SOUNDMGR_MIXER, uint8_t *); }
internal inline uint32_t sndmgr_get_channels(const void *sm) { return FIELD(sm, E_SOUNDMGR_CHANNELS, uint32_t); }
// a category: its share of the channels, its most channels, in use, priority
internal inline float sndcat_get_share(const void *c) { return FIELD(c, E_SNDCAT_SHARE, float); }
internal inline int sndcat_get_max(const void *c) { return FIELD(c, E_SNDCAT_MAX, int); }
internal inline int sndcat_get_used(const void *c) { return FIELD(c, E_SNDCAT_USED, int); }
internal inline int sndcat_get_priority(const void *c) { return FIELD(c, E_SNDCAT_PRIORITY, int); }
// a playing sound (SESMInstanceNode): its id +0xC, channel +0x1C, sample +0x20; a sample (SESample): its channel +0x24
internal inline int sndnode_get_id(const void *n) { return FIELD(n, E_SNDNODE_ID, int); }
internal inline int sndnode_get_channel(const void *n) { return FIELD(n, E_SNDNODE_CHANNEL, int); }
internal inline uint8_t *sndnode_get_sample(const void *n) { return FIELD(n, E_SNDNODE_SAMPLE, uint8_t *); }
internal inline int sample_get_channel(const void *s) { return FIELD(s, E_SAMPLE_CHANNEL, int); }
// the mixer (SEMixer): the plugin's SPIM functions as its fields - the mod reads and replaces some
enum mixer_fn { MIXER_GET_MAX_CHANNELS, MIXER_PLAY_CHANNEL, MIXER_SET_CHANNEL_SAMPLE, MIXER_SET_SAMPLE_VOLUME };
// SPIMGetMaximumChannelCount, int (cdecl *)(int channel), int (cdecl *)(int channel, SESample *), SPIMSetSampleVolume
internal inline void **mixer_fn_at(void *mx, enum mixer_fn k) {
    const uint8_t off[] = { 0x0, 0x20, 0x28, 0x40 };   // from +0xB0
    return (void **)FIELD_AT(mx, E_MIXER_SPIM + off[k]);
}

// FUNCTIONS
internal inline uint8_t *sound_manager(void) { return *(uint8_t **)GA(G_SOUND_MANAGER); }
internal inline float sound_percent(void) { return *(float *)GA(G_SOUND_PERCENT); }
// the sound manager's channel count set (as the options screen does: the original of sound.c's hook through its import)
internal inline int snd_set_max_channels(void *sm, int n) { return (*(int (__thiscall **)(void *, int))GA(IAT_SND_SETMAXCH))(sm, n); }
// the Miles mixer plugin's channels (Miles Sound System Mixer.dll at base: a vector of 8-byte entries) and its maximum
internal inline uint8_t *mix_channels(uintptr_t base, uint8_t **end) {
    uint8_t *v = *(uint8_t **)(base + GA(RVA_MIX_CHANNELS));
    if (end) *end = v ? *(uint8_t **)(base + GA(RVA_MIX_CHANNELS) + 4) : NULL;
    return v;
}
internal inline int mix_max_channels(uintptr_t base) { return *(int *)(base + GA(RVA_MIX_MAX_CHANNELS)); }

// HOOKS
// the sound manager (sound.c, through the exe's imports of Low-Level Engine: the slots checked against the exports)
typedef uint32_t (__thiscall *snd_play_fn)(void *sm, uint32_t id, float vol, float pan, float pitch, int queue, void *name);
typedef uint32_t (__fastcall *snd_play_hook)(void *sm, void *edx, uint32_t id, float vol, float pan, float pitch, int queue, void *name);
internal inline int patch_snd_play(snd_play_hook h, snd_play_fn *orig) {
    return iat_hook(GA(IAT_SND_PLAY), "Low-Level Engine.dll", "?Play@SESoundManager@@QAEKKMMM_NPAD@Z", (void *)h, (void **)orig);
}
internal inline int patch_snd_playref(snd_play_hook h, snd_play_fn *orig) {
    return iat_hook(GA(IAT_SND_PLAYREF), "Low-Level Engine.dll", "?PlayAndReturnReferencedNode@SESoundManager@@QAEPAVSESMInstanceNode@@KMMM_NPAD@Z",
                    (void *)h, (void **)orig);
}
