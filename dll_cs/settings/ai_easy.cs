// [ai_easy]: the easy AI's own values over [ai_base] - its fields are SETTINGS_EASY too: what the easy AI gets where
// the player's [ai_easy] and [ai_base] say nothing
///
///
[section(80), title("Easy"), diff]
class ai_easy
{
    /// the population limit follows the humans, and the AI keeps to your epoch (see [ai_base])
    bool pop_follow_human = true;
    string epoch_pacing = "classic";
    /// examples - remove the ';' to use:
    /// cost_percent=100%
}
