[c_type("void")]
unsafe struct SEMixer
{
    [offset(0xB0), found(c)]
    block spim;
}

[c_type("ee_sample")]
unsafe struct SESample
{
    [offset(0x24), found(c)]
    int32 channel;
}

unsafe struct ee_sound_cat
{
    [offset(0x0), found(c)]
    float32 share;

    [offset(0x4), found(c)]
    int32 max;

    [offset(0x8), found(c)]
    int32 used;

    [offset(0xC), found(c)]
    int32 priority;
}

[c_type("ee_sound_node")]
unsafe struct SESMInstanceNode
{
    [offset(0xC), found(c)]
    int32 id;

    [offset(0x1C), found(c)]
    int32 channel;

    [offset(0x20), found(c)]
    SESample* sample;
}

[c_type("ee_sound_mgr")]
unsafe struct SESoundManager
{
    [offset(0x24), found(c)]
    SESMInstanceNode** slots;

    [offset(0x34), found(c)]
    uint8* paused_begin;

    [offset(0x54), found(c)]
    ee_sound_cat** cats_begin;

    [offset(0x70), found(c)]
    ee_mixer* mixer;

    [offset(0x88), found(c)]
    uint32 channels;
}

/// engine_sound's data of the game, places in its code and numbers
static unsafe class engine_sound
{
    /// SESoundManager::Play(id, vol, pan, pitch, bool queue, char *) -> id or -1
    [import_slot("Low-Level Engine.dll", "?Play@SESoundManager@@QAEKKMMM_NPAD@Z")]
    [address((ee)0x008232D0, (aoc)0x008372CC)]
    static block iat_snd_play;

    /// SESoundManager::PlayAndReturnReferencedNode(same) -> node or NULL
    [import_slot("Low-Level Engine.dll", "?PlayAndReturnReferencedNode@SESoundManager@@QAEPAVSESMInstanceNode@@KMMM_NPAD@Z")]
    [address((ee)0x008233EC, (aoc)0x00837380)]
    static block iat_snd_playref;

    [import_slot("Low-Level Engine.dll", "?SetMaxChannels@SESoundManager@@QAEJK@Z")]
    [address((ee)0x00823AD4, (aoc)0x00837AC4)]
    static block iat_snd_setmaxch;

    /// Miles Sound System Mixer.dll: its channels, a vector of 8 bytes each
    [value(0x52AC)]
    static uint32 rva_mix_channels;

    /// int: the most channels it gives
    [value(0x5320)]
    static uint32 rva_mix_max_channels;
}
