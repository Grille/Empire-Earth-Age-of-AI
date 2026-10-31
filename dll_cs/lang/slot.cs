// A SLOT: a player to be, in a script's game.setup(() => ...) - game.slots[2], game.slot(2).

[numbered]
class slot
{
    /// the player it becomes
    int number;
    /// a human's (else a computer's)
    bool human;
    /// its personality (a computer's): persona.fuchs - its file
    [set] text? persona;
    /// its colour: color.red, or (a computer's) rgb(40, 200, 120)
    [set] any color;
}
