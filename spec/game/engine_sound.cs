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

[c_prefix("sndcat")]
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

[c_type("ee_sound_node"), c_prefix("sndnode")]
unsafe struct SESMInstanceNode
{
    [offset(0xC), found(c)]
    int32 id;

    [offset(0x1C), found(c)]
    int32 channel;

    [offset(0x20), found(c)]
    SESample* sample;
}

[c_type("ee_sound_mgr"), c_prefix("sndmgr")]
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
