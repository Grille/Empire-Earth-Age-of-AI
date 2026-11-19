// [ai_hard]: the hard AI's own values over [ai_base] (none shipped: only examples)
namespace ageofai.settings;

///
[section(100), title("Hard"), diff]
class ai_hard
{
    /// e.g. a harder opponent that pays less and keeps a bigger army:
    /// cost_percent=75%
    /// military_income_share=55%
    /// or the game's own hard, its units hitting 1.4 times as hard and yours 0.714 times:
    /// damage_to_humans=140%
    /// damage_to_ais=71%
}
