// THE GAME: a script's game - its time, the players, timers, the setup before the game starts and the players to be.

public class game
{
    /// the game time in seconds (a script's time)
    public float time;
    /// in whole minutes
    public int minutes;
    /// every player, in or out: game.players[3] is player 3
    public player[] players;
    /// game.player(3)
    [method] public extern player player(int number);
    /// game.timer(20s, () => ...): every 20 s from now on (expr.c TIMER)
    [method] public extern trigger timer(float every, function run);
    /// game.setup(() => ...): before the game starts (relations.c SETUP)
    [method] public extern void setup(function run);
    /// the unit of an id (u.id) while it is on the map - null once it is gone
    [method] public extern unit? unit(int id);
    /// the players to be, by number: game.slots[2]
    public slot[] slots;
    /// game.slot(2)
    [method] public extern slot slot(int number);
}
