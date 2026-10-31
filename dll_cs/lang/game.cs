// THE GAME: a script's game - its time, the players, timers, the setup before the game starts and the players to be.
namespace ageofai.lang;

class game
{
    /// the game time in seconds (a script's time)
    float time;
    /// in whole minutes
    int minutes;
    /// every player, in or out: game.players[3] is player 3
    player[] players;
    /// game.player(3)
    [method]
    extern player player(int number);
    /// game.timer(20s, () => ...): every 20 s from now on (expr.c TIMER)
    [method]
    extern trigger timer(float every, function run);
    /// game.setup(() => ...): before the game starts (relations.c SETUP)
    [method]
    extern void setup(function run);
    /// the unit of an id (u.id) while it is on the map - null once it is gone
    [method]
    extern unit? unit(int id);
    /// the players to be, by number: game.slots[2]
    slot[] slots;
    /// game.slot(2)
    [method]
    extern slot slot(int number);
}
