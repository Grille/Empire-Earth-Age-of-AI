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

global using static how;
global using static values;

public static class values
{
    public const uint32 none = 0xFFFFFFFFu;
}

// the builds (builds.h): a value cast to one is that build's
public enum ee : uint { }
public enum aoc : uint { }

// how a field's value was found ([found]; fields.h's letters)
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