// [sound]: the sound mixer (engine/sound.c)
[section(50, "Sound")]
class sound {
    /// Sound diagnostics into ageofai_log.csv: 1 = counts of dropped sounds once a game minute, 2 = every sound played. 0 = off
    [range(0, 2)] int log = 0;
    /// The "Max Number Of Sounds" slider goes to 200% = 32 channels, the sound mixer has 16 on current Windows: sounds on the
    /// missing channels stay silent (looping ones for good). 1 = hold the channels to what the mixer has (from 100% up: 16)
    bool clamp_channels = true;
    /// Sound channels at 100% (8..63; the slider gives 8 at 0%). The mixer's 16 are only a fallback - Windows no longer
    /// reports hardware voices; Miles mixes in software. 0 = keep the 16
    [range(0, 1000), game("the sound mixer's channels at 100%", "16", 0)] int channels = 32;
    /// The rate the sound is mixed at, in Hz (Miles Sound System): the game opens it at 22050. 44100 or 48000 = clearer
    /// high tones; the game's sounds are recorded at 22050 at most, so mostly cleaner mixing. 0 = the game's 22050
    [range(0, 48000), game("the sound's output rate (Hz)", "22050", 0)] int sample_rate = 44100;
}
