// The graphs of the statistics screen after a game (Low-Level Engine.dll).

/// a point of a line: the time, the value (U2DPoint<long>)
[size(0x8), c_type("ee_graph_point")]
unsafe struct U2DPoint
{
    [offset(0x0), found(c)]
    int32 x;

    [offset(0x4), found(c)]
    int32 y;
}

/// a line of a graph: its points in order of x (AddDataPoint sorts after each), the extremes of them (RecomputeMinMaxValues)
[size(0x30), c_type("ee_graph_series")]
unsafe struct UGraphSeries
{
    [offset(0x4), found(c)]
    U2DPoint* points_begin;

    [offset(0x8), found(c)]
    U2DPoint* points_end;

    [offset(0xC), found(c)]
    U2DPoint* points_cap;

    [offset(0x1C), found(c)]
    int32 min_x;

    [offset(0x20), found(c)]
    int32 max_x;

    [offset(0x24), found(c)]
    int32 min_y;

    [offset(0x28), found(c)]
    int32 max_y;
}

/// a graph: its lines (a vector of UGraphSeries by value), the extremes over them
[size(0x5C), c_type("ee_graph")]
unsafe struct UGraph
{
    [offset(0x4), found(c)]
    UGraphSeries* series_begin;

    [offset(0x8), found(c)]
    UGraphSeries* series_end;

    [offset(0x4C), found(c)]
    int32 min_x;

    [offset(0x50), found(c)]
    int32 max_x;

    [offset(0x54), found(c)]
    int32 min_y;

    [offset(0x58), found(c)]
    int32 max_y;
}

/// graph's data of the game, places in its code and numbers
static unsafe class graph
{
    /// UGraphSeries::AddDataPoint(const U2DPoint<long> &): the point replaces the last if of the same x, then the line is sorted and its extremes made anew
    [import_slot("Low-Level Engine.dll", "?AddDataPoint@UGraphSeries@@QAEXABV?$U2DPoint@J@@@Z")]
    [address((ee)0x00824358, (aoc)0x00838374)]
    static block iat_add_point;

    /// UGraph::PruneGraph(): of the points of one x only that of the highest y stays
    [import_slot("Low-Level Engine.dll", "?PruneGraph@UGraph@@QAEXXZ")]
    [address((ee)0x00824360, (aoc)0x00838378)]
    static block iat_prune;

    /// UGraph::CombineGraph(const UGraph &other, UGraph &out): the lines of both added up, into new lines of out
    [import_slot("Low-Level Engine.dll", "?CombineGraph@UGraph@@QAEXABV1@AAV1@@Z")]
    [address((ee)0x0082434C, (aoc)0x00838368)]
    static block iat_combine;

    /// UGraph::AddSeries(const UGraphSeries &): a copy of the line appended, the graph's extremes widened by it
    [import_slot("Low-Level Engine.dll", "?AddSeries@UGraph@@QAEXABVUGraphSeries@@@Z")]
    [address((ee)0x00824354, (aoc)0x00838370)]
    static block iat_add_series;

    /// UGraphSeries::UGraphSeries(): no points, white, shown
    [import_slot("Low-Level Engine.dll", "??0UGraphSeries@@QAE@XZ")]
    [address((ee)0x00824364, (aoc)0x00838380)]
    static block iat_series_ctor;
}
