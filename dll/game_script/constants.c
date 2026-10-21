// THE GAME'S CONSTANTS in the text language: epoch.copper, resource.gold, color.red, kind.towers, unit.citizen
// (script/host.h constant) - the language itself has none.
// epoch.<name>: an epoch's number, in every language the same (the names the game's own are in another language)
static const char *const k_epoch_const[15] = { "prehistoric", "stone", "copper", "bronze", "dark", "middle", "renaissance",
    "imperial", "industrial", "atomic_wwi", "atomic_wwii", "atomic_modern", "digital", "nano", "space" };
// resource.<name>: a resource's number 1..5 (food, wood, stone, gold, iron - the game's order), its text its name;
// resource.all 0 (give: each of them)
static const char *const k_resource_const[5] = { "food", "wood", "stone", "gold", "iron" };
// 0: no constant, 1: one, 2: one that stays the same - kept in the parsed tree (kind.towers, unit.citizen once the
// game's types are there: filters must be quick, AI code)
static int ee_constant(const char *name, struct xval *v) {
    if (!_strnicmp(name, "color.", 6)) {   // color.red: a colour's number 1..17, as X.color has it
        int k = isalpha((unsigned char)name[6]) ? color_index(name + 6) : -1;
        if (k < 0) return 0;
        xnum(v, k + 1);
        return 1;
    }
    if (!_strnicmp(name, "resource.", 9)) {   // resource.gold: 4 (and "gold"); resource.all: 0 - each of them
        if (!lstrcmpiA(name + 9, "all")) { xdual(v, 0, "all"); return 1; }
        for (int k = 0; k < 5; k++) if (!lstrcmpiA(name + 9, k_resource_const[k])) { xdual(v, k + 1, k_resource_const[k]); return 1; }
        return 0;
    }
    if (!_strnicmp(name, "kind.", 5)) {   // kind.towers: one of the game's families (docs/units.md) - its number and "kind.towers"
        for (int k = 0; k < UNIT_KINDS; k++)
            if (k_kind_ident[k] && !lstrcmpiA(name + 5, k_kind_ident[k])) {
                static char text[UNIT_KINDS][48];   // "kind.towers" as it stays (a constant's text in its node: xe)
                if (!text[k][0]) wsprintfA(text[k], "kind.%s", k_kind_ident[k]);
                xdual_kept(v, k, text[k]);
                return 2;
            }
        return 0;
    }
    if (!_strnicmp(name, "unit.", 5)) {   // unit.b_guard_tower_bronze: a unit or building type - its number and identifier
        int t = unit_type_named(name + 5, (int)strlen(name + 5));
        if (t < 0 && !g_unit_types_n && g_x_check_file) {   // the game hasn't loaded its data yet: checked when it has
            unit_name_later(name + 5, g_x_check_file, g_x_check_line);
            xdual(v, -1, name + 5);
            return 1;
        }
        if (t < 0) return 0;
        xdual_kept(v, t, unit_ident(t));   // the game's text, as it stays
        return 2;
    }
    if (_strnicmp(name, "epoch.", 6)) return 0;
    for (int e = 0; e < 15; e++) if (!lstrcmpiA(name + 6, k_epoch_const[e])) { xnum(v, e + 1); return 1; }
    return 0;
}
