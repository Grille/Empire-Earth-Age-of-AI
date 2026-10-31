// THE NAMES A TEXT HAS of its own (a personality's line, a system text, a prompt): the players of the line - me (the
// AI that speaks), them (the one it speaks to or about; in a system line the reader), enemy (a third), lord (mine) -,
// the speaker's relations with them, and the game's values. A script has game and player(n) instead (game.cs).

static class names
{
    /// the AI that speaks - its name in a text; me.soldiers ...
    static player me;
    /// the player it speaks to or about; in a system line you, the local player
    static player them;
    /// a third player: the enemy in an alliance offer, the ally or lord in some lines
    static player enemy;
    /// my lord (me.lord), the player I capitulated to - in every line; nothing when I have none
    static player? lord;
    /// me.opinion(them): what I think of them
    [with_them] static float opinion;
    /// me.at_war(them)
    [with_them] static bool at_war;
    /// me.allied(them)
    [with_them] static bool allied;
    /// the game time in minutes
    static int minutes;
    /// the players still in, me among them
    static int players;
    /// the game's unit limit per player (its setting)
    static int population_limit;
    /// the largest population of a player still in - kills > max_population / 2: many for this stage of the game
    static int max_population;
    /// the game's language: de, en, fr, ...
    static text language;
    /// how long an offer stands, in minutes ([ai_base] offer_minutes in ageofai.ini)
    static float offer_minutes;
    /// 1 when the language model is told nothing of who plays the computer ([llm] equal)
    static bool llm_equal;
    /// 1 when the language model decides within the rules' limits ([llm] constrained)
    static bool llm_constrained;
}
