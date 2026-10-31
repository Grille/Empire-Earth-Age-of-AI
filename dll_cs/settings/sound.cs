// [sound]: the sound mixer (engine/sound.c)
namespace ageofai.settings;

///
[section(50)]
class sound
{
    enum sound_log_choice
    {
        /// none
        off,
        /// counts of dropped sounds once a game minute
        drops,
        /// also every sound played
        all,
    }

    enum sound_sample_rate_choice
    {
        /// the game's own
        [name("22050")]
        hz_22050,
        /// CD quality
        [name("44100")]
        hz_44100,
        /// what most sound cards run at - nothing for Windows to convert
        [name("48000")]
        hz_48000,
    }

    /// Sound diagnostics into ageofai_log.csv:
    sound_log_choice sound_log = sound_log_choice.off;
    /// The "Max Number Of Sounds" slider goes to 200% = 32 channels, the sound mixer has 16 on current Windows: sounds on the
    /// missing channels stay silent (looping ones for good). 1 = hold the channels to what the mixer has (from 100% up: 16)
    bool sound_clamp_channels = true;
    /// Sound channels at 100% (8..63; the slider gives 8 at 0%). The mixer's 16 are only a fallback - Windows no longer
    /// reports hardware voices; Miles mixes in software. 0 = keep the 16
    [range(0, 1000), game("the sound mixer's channels at 100%", "16", 0)]
    int sound_channels = 32;
    /// The rate the sound is mixed at, in Hz (Miles Sound System). The game's sounds are recorded at 22050 at most, so a
    /// higher rate mostly mixes them cleaner:
    [game("the sound's output rate (Hz)", "22050", 0)]
    sound_sample_rate_choice sound_sample_rate = sound_sample_rate_choice.hz_44100;
}
