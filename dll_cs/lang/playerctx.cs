// THE NAMES A TEXT HAS of its own (a personality's line, a system text, a prompt): the players of the line - me (the
// AI that speaks), them (the one it speaks to or about; in a system line the reader), enemy (a third), lord (mine) -,
// the speaker's relations with them, and the game's values. A script has game and player(n) instead (game.cs). One
// line's context: a field is a name the C side fills (relations.c), a property 'x => me.x(them)' a relation of me with
// them under its own name - C# checks that player has it, lang_read.py takes only that form.
namespace ageofai.lang;

class playerctx
{
    /// the AI that speaks - its name in a text; me.soldiers ...
    player me;
    /// the player it speaks to or about; in a system line you, the local player
    player them;
    /// a third player: the enemy in an alliance offer, the ally or lord in some lines
    player enemy;
    /// my lord (me.lord), the player I capitulated to - in every line; nothing when I have none
    player? lord;
    /// what I think of them
    float opinion => me.opinion(them);
    /// 1 when we are enemies, both of us
    bool at_war => me.at_war(them);
    /// 1 when we are allied, both of us
    bool allied => me.allied(them);
    /// the game time in minutes
    int minutes;
    /// the players still in, me among them
    int players;
    /// the game's unit limit per player (its setting)
    int population_limit;
    /// the largest population of a player still in - kills > max_population / 2: many for this stage of the game
    int max_population;
    /// the game's language: de, en, fr, ...
    text language;
    /// 1 when the language model is told nothing of who plays the computer ([llm] llm_equal)
    bool llm_equal;
    /// 1 when the language model decides within the rules' limits ([llm] llm_constrained)
    bool llm_constrained;
}
