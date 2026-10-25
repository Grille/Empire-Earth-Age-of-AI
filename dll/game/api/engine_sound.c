// THE SOUND (the engine DLLs' sound manager, its categories, nodes, samples, the mixer): fields, functions, hooks.

// FIELDS
// the sound manager (SESoundManager): its channel slots +0x24 (nodes), the paused +0x34 .. +0x38, the categories +0x54
// .. +0x58, the mixer +0x70, the channel count +0x88
internal inline uint8_t *sound_mgr_get_paused_end(const ee_sound_mgr *sm) { return FIELD(sm, of_sound_mgr_paused_begin + 0x4, uint8_t *); }
internal inline ee_sound_cat **sound_mgr_get_cats_end(const ee_sound_mgr *sm) { return FIELD(sm, of_sound_mgr_cats_begin + 0x4, ee_sound_cat **); }
// a category: its share of the channels, its most channels, in use, priority
// a playing sound (SESMInstanceNode): its id +0xC, channel +0x1C, sample +0x20; a sample (SESample): its channel +0x24
// the mixer (SEMixer): the plugin's SPIM functions as its fields - the mod reads and replaces some
enum mixer_fn { MIXER_GET_MAX_CHANNELS, MIXER_PLAY_CHANNEL, MIXER_SET_CHANNEL_SAMPLE, MIXER_SET_SAMPLE_VOLUME };
// SPIMGetMaximumChannelCount, int (cdecl *)(int channel), int (cdecl *)(int channel, SESample *), SPIMSetSampleVolume
internal inline void **mixer_fn_at(ee_mixer *mx, enum mixer_fn k) {
    const uint8_t off[] = { 0x0, 0x20, 0x28, 0x40 };   // from +0xB0
    return (void **)FIELD_AT(mx, of_se_mixer_spim + off[k]);
}

// FUNCTIONS
// the sound manager's channel count set (as the options screen does: the original of sound.c's hook through its import)
internal inline int snd_set_max_channels(ee_sound_mgr *sm, int n) { return (*(int (__thiscall **)(ee_sound_mgr *, int))data_engine_sound_iat_snd_setmaxch)(sm, n); }
// the Miles mixer plugin's channels (Miles Sound System Mixer.dll at base: a vector of 8-byte entries) and its maximum
internal inline uint8_t *mix_channels(uintptr_t base, uint8_t **end) {
    uint8_t *v = *(uint8_t **)(base + value_engine_sound_rva_mix_channels);
    if (end) *end = v ? *(uint8_t **)(base + value_engine_sound_rva_mix_channels + 4) : NULL;
    return v;
}
internal inline int mix_max_channels(uintptr_t base) { return *(int *)(base + value_engine_sound_rva_mix_max_channels); }

// HOOKS
// the sound manager (sound.c, through the exe's imports of Low-Level Engine: the slots checked against the exports)
typedef uint32_t (__thiscall *snd_play_fn)(void *sm, uint32_t id, float vol, float pan, float pitch, int queue, void *name);
internal inline int patch_snd_play(snd_play_fn h, snd_play_fn *orig) {
    return iat_hook(data_engine_sound_iat_snd_play, "Low-Level Engine.dll", "?Play@SESoundManager@@QAEKKMMM_NPAD@Z", (void *)h, (void **)orig);
}
internal inline int patch_snd_playref(snd_play_fn h, snd_play_fn *orig) {
    return iat_hook(data_engine_sound_iat_snd_playref, "Low-Level Engine.dll", "?PlayAndReturnReferencedNode@SESoundManager@@QAEPAVSESMInstanceNode@@KMMM_NPAD@Z",
                    (void *)h, (void **)orig);
}
// the sound manager's channel limit (sound.c: the exe's import of SetMaxChannels) - the original into *orig
typedef int (__thiscall *set_max_ch_fn)(ee_sound_mgr *sm, uint32_t n);
internal inline int patch_snd_set_max_channels(set_max_ch_fn h, set_max_ch_fn *orig) {
    return iat_hook(data_engine_sound_iat_snd_setmaxch, "Low-Level Engine.dll", "?SetMaxChannels@SESoundManager@@QAEJK@Z", (void *)h, (void **)orig);
}
