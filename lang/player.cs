// A PLAYER: me, them, player(3), game.players[3] - its values, its relations with another player (A.opinion(B)), a
// script's effects on it. Its text is its name; it equals only itself (lord == them). Nothing where a value doesn't
// apply (a relation with oneself, a war that isn't): a T? - checked before it is used (docs/scripts.md "Types").

[numbered]
public class player
{
    /// its name - the same as the player itself in a text: {them} = {them.name}
    public text name;
    /// its soldiers (all numbers of units are counted every 10 seconds)
    public int soldiers;
    /// its citizens
    public int citizens;
    /// its units: soldiers and citizens - to scale a number to the game's stage: population_limit, max_population
    public int population;
    /// its buildings
    public int buildings;
    /// its towers
    public int towers;
    /// its wall pieces
    public int walls;
    /// its units and buildings of a filter: unit.<identifier> (one type), kind.<kind> (the game's family, every
    /// epoch's: kind.towers, kind.citizen), or a list of them - any of them: them.count(kind.towers),
    /// me.count([kind.tank, kind.tank_strong]) (docs/units.md: the identifiers and kinds)
    [method] public extern int count(filter of);
    /// its units and buildings on the map now, by id - of a filter (as count's: p.units(kind.tank)) or all (p.units())
    [method, script] public extern unit[] units(filter of = default);
    /// its Town Centers and Capitols
    public int centers;
    /// its fighting strength (its soldiers' hit points, 30 a building) - to compare: them.strength > me.strength *
    /// 2
    public int strength;
    /// its place by strength among the players still in, 1 = the strongest
    public int rank;
    /// its epoch: the name in a text, 1 (Prehistoric) .. 15 (Space) as a number - compare it with epoch.middle etc.
    [mine] public epoch epoch;
    /// its colour: the name in a text (in the game's language), 1 (blue) .. 17 as a number - compare it with
    /// color.red etc.
    public color color;
    /// its civilization's name, as the game shows it (Karthago in a German game; an AI's personality may give it
    /// another: civilization=) - nothing when none is chosen
    public text? civilization;
    /// its civilization in the game's database, in English in every language (Carthage) - for conditions:
    /// them.civilization_en == "Carthage"
    public text? civilization_en;
    /// 1 while it is still in the game
    public bool alive;
    /// 1 for the human player at this PC
    public bool human;
    /// how many have capitulated to it
    public int vassals;
    /// how many are allied with it (both ways), still in - allies alone: mine
    [mine] public int allies;
    /// how many it treats as enemies, still in - enemies alone: mine
    [mine] public int enemies;
    /// its traits, a share: 0% .. 100% (50% without a personality) - aggression > 60%, {pride:%} in a text
    [mine] public float aggression;
    [mine] public float loyalty;
    [mine] public float pride;
    [mine] public float greed;
    [mine] public float forgiveness;
    [mine] public float caution;
    [mine] public float talk;
    /// its lord, the player it capitulated to - a player: them.lord.name, lord == them; nothing when it has none
    public player? lord;
    /// its stock - another's too (the AI's spies): them.gold; a script sets it: p.gold = 500, p.food += 100
    [set] public int food;
    [set] public int wood;
    [set] public int stone;
    [set] public int gold;
    [set] public int iron;
    /// the five resources together - to scale amounts: them.tribute(me) > me.stock / 2
    public int stock;
    /// -100..100: what A thinks of B - in a text in words: hostile, unfriendly, indifferent, friendly, trusting
    /// (round(opinion): the number). An AI's opinion; a human's of an AI is a guess from what the AI did to them
    /// (war, a betrayal, peace, tribute, a refusal, their losses), nothing of another human
    [relation] public extern float opinion(player b);
    /// 1 when A and B are enemies, both of them
    [relation] public extern bool at_war(player b);
    /// 1 when A and B are allied, both of them
    [relation] public extern bool allied(player b);
    /// 1 when A treats B as an enemy (whatever B does)
    [relation] public extern bool hostile(player b);
    /// how long the war of A and B has lasted: from both enemies until both are allied again (a peace offer doesn't
    /// end it; in peace_accept: the war just ended) - nothing without a war
    [relation] public extern int? war_minutes(player b);
    /// how long A and B have been allied - nothing when they are not
    [relation] public extern int? alliance_minutes(player b);
    /// how often A turned on B while they were allied (an alliance of 2 minutes or more: a peace broken at once is
    /// war, not betrayal) - (B, minutes): in the last minutes; in betrayed the betrayals before the one it is about
    [relation] public extern int betrayals(player b, float minutes = 0);
    /// how often A refused B's offers of peace or alliance - (B, minutes): in the last minutes
    [relation] public extern int refusals(player b, float minutes = 0);
    /// B's alliance offers A let run out, in a row - in offer_withdrawn those before the one withdrawn
    [relation] public extern int ignored(player b);
    /// resources A has paid B in this game - (B, minutes): in the last minutes, them.tribute(me, 15); in
    /// tribute_thanks / tribute_small what was paid before this tribute
    [relation] public extern int tribute(player b, float minutes = 0);
    /// A's soldiers within 20 tiles of B's base now, while B treats A as an enemy (an AI's base)
    [relation] public extern int near(player b);
    /// 1 when A's army is in B's town: its soldiers have stayed near B's base for a while (an AI's base)
    [relation] public extern bool in_town(player b);
    /// minutes since A's last attack wave set out against B - nothing if none (an AI's waves)
    [relation] public extern int? attack_minutes(player b);
    /// B's units and buildings A has destroyed - (B, minutes): in the last minutes (whole minutes, the current one
    /// counts); a filter last picks which, as count's: unit.<identifier>, kind.<kind>, a list of them - or one of
    /// mod/units.eeai's lists (units, army, airforce, marine, military, civilians, buildings): them.kills(me,
    /// units), me.kills(them, 10, kind.tank), them.kills(me, 20, buildings)
    [relation] public extern int kills(player b, float minutes = 0, filter of = default);
    /// a script's: p.win() - the player is victorious, as a scenario's Victory: the game is over, the others lose
    [effect] public extern void win();
    /// a script's: p.lose() - the player is defeated, as a scenario's Defeat
    [effect] public extern void lose();
    /// a script's: A.war(B) - A treats B as an enemy (one way, as the game's stances: B's towards A stays; an AI
    /// may change its mind later)
    [effect] public extern void war(player b);
    /// a script's: A.ally(B) - A treats B as an ally (one way)
    [effect] public extern void ally(player b);
    /// a script's: p.spawn(unit.inf10_doughboy, vec(40, 40), 10) - new units of one type at a place (2 tiles apart, in
    /// a square), as a scenario's Create makes them, at the game's next step: returned at once, the script goes on and
    /// holds only when it first uses one of them or asks the map (p.units(), game.unit(id)); a trigger's are there at
    /// once
    [effect, script] public extern unit[] spawn(unit_type type, vec2 at, int count = 1);
}
