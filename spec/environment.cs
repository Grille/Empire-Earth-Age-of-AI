global using int8 = sbyte;
global using uint8 = byte;
global using int16 = short;
global using uint16 = ushort;
global using int32 = int;
global using uint32 = uint;
global using int64 = long;
global using uint64 = ulong;
global using float32 = float;
global using float64 = double;
global using char8 = sbyte;   // C's char: a character or a byte the game's code reads as char (int8 the same size)

global using static how;
global using static values;

public static class values
{
    public const uint32 none = 0xFFFFFFFFu;
}

// the builds (builds.h): a value cast to one is that build's
public enum ee : uint { }
public enum aoc : uint { }

// how a field's value was found ([found])
public enum how
{
    /// read in its class's own code - the strongest
    c,
    /// between two fields of the same kind that have the same shift from EE to AoC
    b,
    /// the code's reads agree with a neighbouring field's shift
    v,
    /// kept: no read of this object says otherwise
    s,
    /// by hand - its reason in found(h, "why")
    h,
}

// THE RUNTIME LIBRARY'S LAYOUTS: the code walks them itself (game/api's helpers)
public struct deque { }    // std::deque: +4 block map, +8 its size, +0xC the front's index, +0x10 the count
public struct vector { }   // std::vector: begin, end, capacity end
// the game's strings in an object (a field of these: its address, the helpers of game.c read and set it): UString and
// UWideString (8-bit characters despite its name), 16 bytes each, the text at +4 (a UWideString a function takes:
// types.h's struct wstring)
[c_type("struct wstring")] public struct wstring { }
// a structure inside an object whose parts are plain numbers from its start (the code adds them: the matrix's,
// a list's begin and end) - its address only
public struct block { }
// a place in the game's code (a call, an operand, a jump the mod patches): its address only
public struct code { }
// a class's vtable in the game's data (its functions' addresses, slot by slot): its address only
public struct vftable { }

// VALUES OF MORE THAN ONE NUMBER: their C types (base/), read and written whole
[c_type("vec2")] public struct vec2 { float32 x, y; }
[c_type("vec3")] public struct vec3 { float32 x, y, z; }
[c_type("ivec2")] public struct ivec2 { int32 x, y; }
// the game's string (UString) by value, as some of its functions take it: 16 bytes (types.h)
[c_type("struct ustring")] public struct ustring { }
// a string the game only reads (C's const char *)
[c_type("const char *")] public struct cstr { }