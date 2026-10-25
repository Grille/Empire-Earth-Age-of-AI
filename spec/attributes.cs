// THE SPEC: what the mod knows of the game, as C# declarations - game/: one file per object of the game (unit.cs ...),
// game.cs its global state, stubs.cs the objects not described yet; the spec's own: this file its attributes,
// environment.cs its types and build names. It describes the game only: what is where, of what type, in which build.
// What the mod makes of it (its accessors, wrappers, hooks) is the generator's (plain C, dll/game/api/gen/) and the
// hand-written code's on top. The files are valid C# so that an editor reads them: names, types and attributes
// checked, go to definition (spec.csproj; dotnet build there checks them).
//
// NAMES: a class is the game's own name (EEComplexUnit: what Ghidra and the decompilations call it). Its C type is the
// name in lower case with _ between the words (EEGhostUnit ee_ghost_unit, RTSUnit rts_unit), else [c_type] says it;
// the C functions' prefix is that type without ee_ (ghost_unit_get_player). A member's name is the C side's as it
// stands - EEComplexUnit's add_waypoint is unit_add_waypoint, with the game's parameters; a hand-written wrapper over
// it (a parameter fixed, a value converted) takes another name. The game's functions of no class go into a static
// class of the file of their subject, its name their prefix (unit.cs's class unit: unit_aim); a class's struct holds
// only what acts on its object (scripts/build/spec_check.py), game.cs the game's global state.
//
// VALUES: a build's is cast to the build, (ee) the 2017 update, (aoc) Art of Conquest, in builds.h's order - an enum
// each, so the compiler holds them apart: (aoc) where (ee) belongs, or one build twice, doesn't build. One plain value
// is every build's; (aoc)none: the build hasn't got it, or it isn't known yet. An attribute given more than once is a
// list (its sites, in this order). Another build: its enum, and a parameter more in every constructor below.
//
// TYPES: the sizes by name, the same in C and here (environment.cs's aliases) - int8 uint8 int16 uint16 int32 uint32
// int64 uint64 float32 float64, nint / nuint (32 bits: intptr_t / uintptr_t), void* and pointers; the game's objects
// their structs (EEComplexUnit*), the runtime library's layouts environment.cs's. C#'s own names (int, char: 16 bits,
// byte ...) are not used in a member (scripts/build/spec_check.py, run by the build), so a type never means one thing
// here and another in C.

using System;

// WHERE

// a field of the object: its offset
[AttributeUsage(AttributeTargets.Field)]
public sealed class offset : Attribute
{
    public offset(uint all) { }
    public offset(ee ee, aoc aoc) { }
}

// a function or data in the game's exe: its address (in a [dll] class: relative to the DLL's base)
[AttributeUsage(AttributeTargets.Method | AttributeTargets.Field, AllowMultiple = true)]
public sealed class address : Attribute
{
    public address(uint all) { }
    public address(ee ee, aoc aoc) { }
}

// a virtual function: its slot in the object's vtable (called through the object's own; a class may have gained one);
// with [address] the declaring class's own function in it, a derived class's own one [overrides]
[AttributeUsage(AttributeTargets.Method)]
public sealed class slot : Attribute
{
    public slot(uint all) { }
    public slot(ee ee, aoc aoc) { }
}

// a function the game imports: its import slot's address (the function read from the slot when called)
[AttributeUsage(AttributeTargets.Method)]
public sealed class import : Attribute
{
    public import(uint all) { }
    public import(ee ee, aoc aoc) { }
}

// a number of the game's that is no address: a virtual function's slot index, an object's size, a place relative to
// an engine DLL's base (each build ships its own DLLs: the mod adds the DLL's base)
[AttributeUsage(AttributeTargets.Field)]
public sealed class value : Attribute
{
    public value(uint all) { }
    public value(ee ee, aoc aoc) { }
}

// a class of an engine DLL: its members' addresses relative to the DLL's base (each build ships its own DLLs)
[AttributeUsage(AttributeTargets.Class)]
public sealed class dll : Attribute
{
    public dll(string file) { }
}

// THE CLASSES: a struct per class of the game, its members where the game declares them (a derived class's own in its
// struct, the base's in the base's). C# structs don't inherit: [base] says the struct starts with the base's.

// the class this one derives from: its members first, at their offsets
[AttributeUsage(AttributeTargets.Struct)]
public sealed class @base : Attribute
{
    public @base(Type cls) { }
}

// the object's size in bytes (its operator new): every field inside it, a derived class's after its base's size
[AttributeUsage(AttributeTargets.Struct)]
public sealed class size : Attribute
{
    public size(uint all) { }
    public size(ee ee, aoc aoc) { }
}

// the class's C type where it isn't its name in C's spelling: classes the C side holds as one type (ee_unit for
// RTSUnit, EEUnit and EEComplexUnit: unit_get_speed), and the C names that came before the spec (EEUnitGoal ee_goal);
// "void" where the C side has no type for it yet (its accessors take void *). An enum's: the C enum that holds its
// values (enum sm_player_spec)
[AttributeUsage(AttributeTargets.Struct | AttributeTargets.Enum)]
public sealed class c_type : Attribute
{
    public c_type(string name) { }
}

// a static class's C prefix: "" for the one whose functions keep names of their own (explored_by); an enum's: its
// constants' (SM_CAT: SM_CAT_OBJECT, gen/enums.c); a class's struct has none - its prefix is its C type without ee_
// (the player, 2026-10-25: one name for type and functions)
[AttributeUsage(AttributeTargets.Struct | AttributeTargets.Class | AttributeTargets.Enum)]
public sealed class c_prefix : Attribute
{
    public c_prefix(string name) { }
}

// HOW THE GAME CALLS IT

// a function's calling convention other than thiscall (an instance method is thiscall, the object its this)
[AttributeUsage(AttributeTargets.Method)] public sealed class cdecl : Attribute { }
[AttributeUsage(AttributeTargets.Method)] public sealed class stdcall : Attribute { }
// a static class's function that is a method of a class the spec doesn't describe yet: thiscall, its first parameter
// the object
[AttributeUsage(AttributeTargets.Method)] public sealed class thiscall : Attribute { }

// a call of this function in the game's code (a call instruction there), by a name of its own: the place a hook goes
[AttributeUsage(AttributeTargets.Method, AllowMultiple = true)]
public sealed class call_site : Attribute
{
    public call_site(string name, uint all) { }
    public call_site(string name, ee ee, aoc aoc) { }
}

// the class's vtable: its address
[AttributeUsage(AttributeTargets.Struct)]
public sealed class vtable : Attribute
{
    public vtable(uint all) { }
    public vtable(ee ee, aoc aoc) { }
}

// this class's own function in a virtual function's slot - the slot is the base's ([slot] on the method of the same
// name and parameters there), its address this class's function
[AttributeUsage(AttributeTargets.Method)]
public sealed class overrides : Attribute
{
    public overrides(Type cls) { }
}

// the slot of an [overrides] function (or of a [slot] with [address], the class's own) in its class's vtable, by a
// name of its own: the place a hook goes (the patch checks the slot holds the function's address)
[AttributeUsage(AttributeTargets.Method)]
public sealed class vtable_site : Attribute
{
    public vtable_site(string name) { }
}

// CODE: what the game's code has at a place the mod changes in place - the bytes from the address + from (an
// operand's instruction before it), or the address of a member (nameof) as a 32-bit operand
[AttributeUsage(AttributeTargets.Field)]
public sealed class expect : Attribute
{
    public expect(int from, params byte[] bytes) { }
    public expect(string member) { }
}

// HOW IT WAS FOUND (a field's) - c its class's code, b between two fields of the same shift, v the
// code's reads agree with a neighbour's shift, s kept: no read says otherwise, h by hand (its reason)
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Method)]
public sealed class found : Attribute
{
    public found(how how) { }
    public found(how how, string why) { }
}

// WHAT IT IS: its name and type an educated guess from what the code does with it, not confirmed - its doc comment
// says the evidence (scripts/build/spec_check.py); the generated C and Ghidra's comment say "a guess", and
// scripts/re/spec_gaps.py guesses lists them with the mod's uses. Removed when a read or a test confirms it; a name
// that turns out wrong is renamed. (Where it is, [offset] and [found], is the reads' and holds either way.)
[AttributeUsage(AttributeTargets.Field | AttributeTargets.Method)]
public sealed class guess : Attribute { }


