// A UNIT: one on the map - a soldier, a citizen, a ship, a building - held by its id: the value stays when the unit is
// destroyed (alive false then, any other member a mistake: check u.alive). p.units(...), game.unit(id). Its text is
// its type's identifier; it equals only itself (the same id).

public class unit
{
    /// its number in the game - another unit never has it (game.unit(id) finds it again)
    public int id;
    /// while it is on the map: false once it is destroyed
    public bool alive;
    /// its type: unit.<identifier> - u.type == unit.inf10_doughboy (docs/units.md)
    public unit_type type;
    /// the player it belongs to - a script gives it to another: u.owner = p (as a scenario's Owner effect: the game
    /// makes it anew for p, with a new id - u follows it)
    [set] public player owner;
    /// where it is, in tiles: vec(x, y) - as spawn and cmd's commands take them
    public vec2 pos;
    /// its hit points now - a script sets them: u.hp = 100, u.hp -= 50 (0 or less: it vanishes, as a scenario's)
    [set] public int hp;
    /// its hit points when whole (its type's, with its owner's upgrades)
    public int max_hp;
    /// its ammunition - an aircraft's shots before it must land; a script sets it
    [set] public int ammo;
    /// its power - a priest's, a prophet's (what its spells cost); a script sets it
    [set] public int power;
    /// a script's: u.kill() - it dies (as a scenario's Kill effect: its death shown)
    [effect, script] public extern void kill();
    /// a script's: u.remove() - it vanishes from the map (as a scenario's Remove effect)
    [effect, script] public extern void remove();
}
