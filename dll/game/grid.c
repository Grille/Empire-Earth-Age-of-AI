// UNIT GRID: the map's unit grid (map +G_UNIT_GRID_OFS: +4 cells, 16 bytes each {?, first, last, end} - a vector
// of unit pointers; +8 cell size in tiles, +0x10 columns, +0x14 rows; cell (gx, gy) at gx * rows + gy). A big building
// stands in every cell it covers; a unit's own cell is the one of its position (+0x1C / +0x20 tiles).
// Included by eeaimod.c after control.c (G_MAP, G_UNIT_GRID_OFS).

// Iterator over the cells covering tiles x-r..x+r, y-r..y+r (or the whole grid): the units with a type, cell by cell
// (columns outer, rows inner), each cell's vector as it stands (its end read at every step). once = 1: each unit once -
// in the cell of its own position, or, when that lies outside the area, the area's cell nearest to it (a big building
// stands in every cell it covers, up to 4 - buildings and towers, families 2 and 29; every other unit in one cell, and
// every unit in the cell of its own position: gridcheck, Continental and Small Islands, 4 looks each). The same units
// as once = 0, which visits every entry (a big building once per cell: counted, its strength summed 2-4 times - the
// 36 loops it replaced did that, except those with their own own-cell test). A footprint is a rectangle of cells
// holding the own cell, so the nearest area cell to the own cell is one of it. break / continue / return in the loop
// body work as in a plain loop:
//     for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) { uint8_t *u = it.u, *td = it.td; ... }
struct grid_it {
    uint8_t *cells, *cell, **b;   // the grid's cells, the current cell, the next entry in it
    uint8_t *u, *td;              // the unit found and its type
    int cs, ny, once;
    int cx, cy;                   // the current cell
    int gx, gy, x0, x1, y0, y1;   // the next cell, the area's columns and rows
};
static struct grid_it grid_it_area(int x0, int y0, int x1, int y1, int once) {   // cells x0..x1, y0..y1 (clamped)
    struct grid_it it = { 0 };
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    it.gx = 1; it.x1 = 0;   // nothing unless set below
    if (!grid) return it;
    int nx = FIELD(grid, 0x10, int);
    it.cells = FIELD(grid, 4, uint8_t *);
    it.cs = FIELD(grid, 8, int);
    it.ny = FIELD(grid, 0x14, int);
    if (!it.cells || it.cs <= 0) return it;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= nx) x1 = nx - 1;
    if (y1 >= it.ny) y1 = it.ny - 1;
    if (y0 > y1) return it;
    it.gx = x0; it.x0 = x0; it.x1 = x1; it.gy = y0; it.y0 = y0; it.y1 = y1; it.once = once;
    return it;
}
// the cells covering tiles x-r..x+r, y-r..y+r. C division truncates toward 0: (x - r) / cs is cell 0 for -1..-(cs-1)
// as well, below that negative and left out by the clamp - as the hand-written loops' `cx < 0` test did
static struct grid_it grid_it_box(int x, int y, int r, int once) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    int cs = grid ? FIELD(grid, 8, int) : 0;
    if (cs <= 0) return grid_it_area(1, 1, 0, 0, once);
    return grid_it_area((x - r) / cs, (y - r) / cs, (x + r) / cs, (y + r) / cs, once);
}
static struct grid_it grid_it_all(int once) { return grid_it_area(0, 0, 0x7FFFFFFF, 0x7FFFFFFF, once); }
static int grid_it_next(struct grid_it *it) {
    for (;;) {
        while (it->b && it->b < FIELD(it->cell, 8, uint8_t **)) {
            uint8_t *u = *it->b++, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td) continue;
            if (it->once) {   // only in its own cell, or the area's cell nearest to it
                int ox = FIELD(u, 0x1C, int) / it->cs, oy = FIELD(u, 0x20, int) / it->cs;
                ox = ox < it->x0 ? it->x0 : ox > it->x1 ? it->x1 : ox;
                oy = oy < it->y0 ? it->y0 : oy > it->y1 ? it->y1 : oy;
                if (ox != it->cx || oy != it->cy) continue;
            }
            it->u = u; it->td = td;
            return 1;
        }
        if (it->gx > it->x1) return 0;
        it->cx = it->gx; it->cy = it->gy;
        it->cell = it->cells + (it->cx * it->ny + it->cy) * 16;
        it->b = FIELD(it->cell, 4, uint8_t **);
        if (++it->gy > it->y1) { it->gy = it->y0; it->gx++; }
    }
}

// Callback over every unit of the unit grid (each unit once: only in the cell of its own position).
static void grid_each(void (*fn)(uint8_t *u, void *ctx), void *ctx) {
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) fn(it.u, ctx);
}
// the same within the cells covering tiles x-r..x+r, y-r..y+r: each unit once (a big building stands in several cells -
// counted by hand, an AA building counted 2-4 times); fn checks the distance
static void grid_box(int x, int y, int r, void (*fn)(uint8_t *u, void *ctx), void *ctx) {
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) fn(it.u, ctx);
}
