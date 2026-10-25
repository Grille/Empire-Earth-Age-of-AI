// THE LANGUAGE'S OBJECTS: what a .eeai script or text sees of the game, as C# declarations - a class per kind of object
// (player.cs, game.cs, slot.cs), names.cs the names a text has of its own (me, them, minutes ...), this file the
// language's own types and the attributes. Declared once here, read by scripts/build/lang_read.py: gen_lang.py makes
// the C tables of it (dll/game_script/lang.h - the check's types and arguments, the runtime's names and docs),
// gen_eeai_readme.py the README's tables, gen_vscode_grammar.py the editor's names. What a member does stays the
// hand-written code's (game_script/relations.c x_member, x_relation ...): a declaration is its name, type, arguments
// and doc. The language is its own (docs/scripts.md) - C# is only how its objects are written down: valid C# so that
// an editor reads them, names and types checked (lang.csproj; dotnet build there). Nothing of it runs.
//
// A MEMBER is a field (p.towers: a value) or a method (p.opinion(q)): its parameters are the arguments, one with a
// default may be left out (them.tribute(me) or them.tribute(me, 15)). Its /// lines are its doc (the README's text: a
// member without one shares the one before - the traits, the stocks). The order is the README's.
//
// TYPES: the language's, by its names - int, float (a number with decimals; the game's double), bool, text, vec2 ..
// vec4, epoch, color, unit_type, trigger, function, record; a kind of object (player, game, slot); T? one or none
// (null: lord without one), T[] a list of them; void: a method that gives nothing (an effect). filter: what count and
// kills take (unit.<identifier>, kind.<kind> or a list of them); any: a value the check doesn't type (a colour or an
// rgb(...)).

using System;

public struct text { }
public struct epoch { }
public struct color { }
public struct unit_type { }
public struct vec2 { }
public struct vec3 { }
public struct vec4 { }
public struct trigger { }
public struct function { }
public struct @record { }   // (@: C#'s own word)
public struct filter { }
public struct any { }

// a relation of player A with player B: A.opinion(B) - its first parameter the other player; without A. in a text
// mine (opinion(enemy) is me.opinion(enemy))
[AttributeUsage(AttributeTargets.Method)]
public sealed class relation : Attribute { }

// a script's effect: it changes the game (p.win(), A.war(B)) - a text changes nothing
[AttributeUsage(AttributeTargets.Method)]
public sealed class effect : Attribute { }

// a method that is neither: p.count(filter), game.player(3)
[AttributeUsage(AttributeTargets.Method)]
public sealed class method : Attribute { }

// a value a script may set: p.gold = 500, slot.persona = persona.fuchs
[AttributeUsage(AttributeTargets.Field)]
public sealed class set : Attribute { }

// a player's value a text has alone, as the speaker's: epoch is me.epoch
[AttributeUsage(AttributeTargets.Field)]
public sealed class mine : Attribute { }

// a text's name of its own that is the speaker's relation with them: opinion is me.opinion(them)
[AttributeUsage(AttributeTargets.Field)]
public sealed class with_them : Attribute { }

// a kind whose objects a command takes by number (spawn(p, ...): its number)
[AttributeUsage(AttributeTargets.Class)]
public sealed class numbered : Attribute { }
