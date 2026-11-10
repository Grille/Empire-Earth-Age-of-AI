// the scenario editor's UI: its form (EEUIFScenarioEditor, built in code - not a row of dbuiforms.dat), its panels
// (EEUIFSE*: the main buttons, the map, terrain, elevation and unit panels), its screens (EESE*: the trigger screen,
// the player screen, the instructions - EESETS* the trigger screen's conditions and effects, EESETSET* one per
// effect), its events (EESEE*) and actions (EESEA*: what a click or a key does on the map). Found as ui_forms.cs's
// (scripts/re/ui_classes.py); the layout the mod patches for the screen size: dll/game/api/editor_layout.h.

/// made in 0x5628EE
[size((ee)0x20, (aoc)0x20), vtable((ee)0x0082D594, (aoc)0x00842E44)]
unsafe struct EESEAChangeLoadText
{
}

/// made in 0x563D1B
[size((ee)0x10, (aoc)0x10), vtable((ee)0x0082D85C, (aoc)0x0084310C)]
unsafe struct EESEAChangeWorldPlayerCount
{
}

[size((ee)0x18, (aoc)0x18), vtable((ee)0x00830034, (aoc)0x00845904), c_type("ee_se_blank_map")]
unsafe struct EESEACreateBlankMap
{
    /// vf1: a blank map made (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x005955FE, (aoc)0x005A58BC)]
    [vtable_site("se_blank_map")]
    extern void run();

    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x005955C3, (aoc)0x005A5881)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[size((ee)0x14, (aoc)0x14), vtable((ee)0x00830070, (aoc)0x00845944), c_type("ee_se_random_map")]
unsafe struct EESEACreateRandomMap
{
    /// vf1: a random map made (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x00596426, (aoc)0x005A66E2)]
    [vtable_site("se_random_map")]
    extern void run();

    /// its constructor (2 arguments, not typed yet)
    [address((ee)0x005963F2, (aoc)0x005A66AE)]
    extern void* init(uint32 p1, uint32 p2);
}

[vtable((ee)0x00830080, (aoc)0x00845958), c_type("ee_se_delete")]
unsafe struct EESEADeleteUnit
{
    /// the unit's id, what its 1-argument constructor sets (-1: the selection)
    [offset(0xC), found(c)]
    int32 unit_id;

    /// that unit's owner's index
    [offset(0x10), found(c)]
    int32 player;

    /// vf1: units deleted (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x00596990, (aoc)0x005A6C4C)]
    [vtable_site("se_delete")]
    extern void run();

    /// its constructor (2 arguments, not typed yet)
    [address((ee)0x00596889, (aoc)0x005A6B45)]
    extern void* init(uint32 p1, uint32 p2);

    /// its constructor (1 argument, not typed yet)
    [address((ee)0x005968C5, (aoc)0x005A6B81)]
    extern void* init_2(uint32 p1);
}

[size((ee)0xC, (aoc)0xC), vtable((ee)0x00830090, (aoc)0x00845968)]
unsafe struct EESEAExitScenario
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0059718F, (aoc)0x005A7331)]
    extern void* init();
}

[size((ee)0x10, (aoc)0x10), vtable((ee)0x0082EE84, (aoc)0x008446A0)]
unsafe struct EESEALasso
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0058353C, (aoc)0x0059177D)]
    extern void* init();
}

/// made in 0x58A70E
[size((ee)0x10, (aoc)0x10), vtable((ee)0x0082EE70, (aoc)0x0084468C)]
unsafe struct EESEAMouseClick
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00583508, (aoc)0x0059176B)]
    extern void* init();
}

/// made in 0x59743E
[size((ee)0x10, (aoc)0x10), vtable((ee)0x008300A0, (aoc)0x0084597C), c_type("ee_se_move")]
unsafe struct EESEAMoveUnit
{
    /// 1: the units picked up (taken off the map, the ghost made), 0: put down where the ghost is
    [offset(0x1C), found(c)]
    uint8 lift;

    /// vf1: a unit moved (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x00597472, (aoc)0x005A7646)]
    [vtable_site("se_move")]
    extern void run();
}

[vtable((ee)0x00830058, (aoc)0x00845928)]
unsafe struct EESEAPaint
{
}

[size((ee)0x2C, (aoc)0x2C), vtable((ee)0x0082DFF0, (aoc)0x008438A4), c_type("ee_se_paint_area")]
unsafe struct EESEAPaintArea
{
    /// vf1: an area painted (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x005980DF, (aoc)0x005A8284)]
    [vtable_site("se_paint_area")]
    extern void run();

    /// its constructor (2 arguments, not typed yet)
    [address((ee)0x0056E07A, (aoc)0x0057D991)]
    extern void* init(uint32 p1, uint32 p2);
}

[@base(typeof(EESEAPaint)), size((ee)0x38, (aoc)0x38), vtable((ee)0x0082FD3C, (aoc)0x00845604), c_type("ee_se_paint_elevation")]
unsafe struct EESEAPaintElevation
{
    /// vf1: the ground raised or lowered (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x00598165, (aoc)0x005A830A)]
    [vtable_site("se_paint_elevation")]
    extern void run();
}

[@base(typeof(EESEAPaint)), size((ee)0x20, (aoc)0x20), vtable((ee)0x00830694, (aoc)0x008459DC), c_type("ee_se_paint_mountains")]
unsafe struct EESEAPaintMoutains
{
    /// vf1: cliffs painted (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x005984E0, (aoc)0x005A86FF)]
    [vtable_site("se_paint_mountains")]
    extern void run();
}

/// made in 0x5955FE, 0x59B234
[@base(typeof(EESEAPaint)), size((ee)0x28, (aoc)0x28), vtable((ee)0x00830044, (aoc)0x00845914), c_type("ee_se_paint_terrain")]
unsafe struct EESEAPaintTerrain
{
    /// vf1: the ground painted (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x00598CA8, (aoc)0x005A8E6C)]
    [vtable_site("se_paint_terrain")]
    extern void run();
}

[size((ee)0x1C, (aoc)0x1C), vtable((ee)0x0082F680, (aoc)0x00844F18), c_type("ee_se_place_gate")]
unsafe struct EESEAPlaceGate
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0058C8A5, (aoc)0x0059BA16)]
    extern void* init();

    /// vf1: the gate laid - build_gate (0x6022E3) for each, as the player's gate button does (walls.c: the joins)
    [slot(1), address((ee)0x00598E97, (aoc)0x005A9112)]
    [vtable_site("se_place_gate")]
    extern void run();
}

/// what the editor's Undo button queues (EEUIFSEUndo +0x104): the game's own takes the last action back by its vf2,
/// which is empty in every editor action (editor_undo.c does the undo instead)
[vtable((ee)0x0082FE98, (aoc)0x00845768), c_type("ee_se_undo")]
unsafe struct EEAUndo
{
    [slot(1), address((ee)0x00594C34, (aoc)0x005A4E8E)]
    [vtable_site("se_undo")]
    extern void run();
}

/// what the editor's Redo button queues
[vtable((ee)0x0082FDF0, (aoc)0x008456C0), c_type("ee_se_redo")]
unsafe struct EEARedo
{
    [slot(1), address((ee)0x00594B3B, (aoc)0x005A4DB3)]
    [vtable_site("se_redo")]
    extern void run();
}

[vtable((ee)0x0082F694, (aoc)0x00844F04), c_type("ee_se_place_unit")]
unsafe struct EESEAPlaceUnit
{
    /// vf1: the units placed, finished (walls.c: the joins beside a tower)
    [slot(1), address((ee)0x005994EF, (aoc)0x005A9650)]
    [vtable_site("se_place_unit")]
    extern void run();
}

[size((ee)0x1C, (aoc)0x1C), vtable((ee)0x0082F6A8, (aoc)0x00844F2C), c_type("ee_se_place_wall")]
unsafe struct EESEAPlaceWall
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0058C969, (aoc)0x0059BA78)]
    extern void* init();

    /// vf1: the wall laid - the game's own placing (0x629DDB: wall_piece, wall_joints), finished (walls.c: the joins)
    [slot(1), address((ee)0x00599F78, (aoc)0x005AA0D1)]
    [vtable_site("se_place_wall")]
    extern void run();
}

[vtable((ee)0x0082F888, (aoc)0x0084516C)]
unsafe struct EESEAQuickSaveLoad
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x0058ECA2, (aoc)0x0059F0B1)]
    extern void* init(uint32 p1);

}

/// made in 0x59F61B
[size((ee)0x10, (aoc)0x10), vtable((ee)0x008306BC, (aoc)0x00845FC0)]
unsafe struct EESEAQuickSaveScenario
{
}

/// made in 0x58702B
[size((ee)0x10, (aoc)0x10), vtable((ee)0x0082F080, (aoc)0x00844940), c_type("ee_se_rotate")]
unsafe struct EESEARotateUnits
{
    /// vf1: units turned (editor_undo.c: what it changes kept)
    [slot(1), address((ee)0x0059A63F, (aoc)0x005AA74F)]
    [vtable_site("se_rotate")]
    extern void run();
}

[size((ee)0xC, (aoc)0xC), vtable((ee)0x008300B8, (aoc)0x008459A4)]
unsafe struct EESEASaveScenario
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0059A6F2, (aoc)0x005AA802)]
    extern void* init();
}

/// made in 0x563603
[size((ee)0x1C, (aoc)0x1C), vtable((ee)0x0082D6DC, (aoc)0x00842F8C)]
unsafe struct EESEATechTree
{
}

[size((ee)0xC, (aoc)0xC), vtable((ee)0x008300DC, (aoc)0x008459C8)]
unsafe struct EESEATestScenario
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0059A99D, (aoc)0x005AAAAD)]
    extern void* init();
}

[@base(typeof(UIButton)), size((ee)0x104, (aoc)0x104), vtable((ee)0x0082D640, (aoc)0x00842EF0)]
unsafe struct EESEBSelect
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x0056346C, (aoc)0x00572DA4)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    [address((ee)0x0056354F, (aoc)0x00572E87), overrides(typeof(UIButton))]
    extern void button_down();

    [address((ee)0x005635D6, (aoc)0x00572F0E), overrides(typeof(UIButton))]
    extern void button_up();
}

[@base(typeof(UIComboBox)), size((ee)0xE0, (aoc)0xE0), vtable((ee)0x0082D890, (aoc)0x00843140)]
unsafe struct EESECBMaxPlayer
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x005656C0, (aoc)0x00575001)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DD08, (aoc)0x008435B8)]
unsafe struct EESEEAlphaSort
{
    [address((ee)0x00560005, (aoc)0x0056F93D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56D39A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DF44, (aoc)0x008437F8)]
unsafe struct EESEEAreaCheck
{
    [address((ee)0x0056019C, (aoc)0x0056FAF7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56D39A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DF30, (aoc)0x008437E4)]
unsafe struct EESEECenterCameraOnArea
{
    [address((ee)0x005601C5, (aoc)0x0056FB20), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56EA99
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E150, (aoc)0x00843AAC)]
unsafe struct EESEEChangeConditionType
{
    [address((ee)0x00560232, (aoc)0x0056FB8D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5731F8, 0x57467D, 0x57641E, 0x578A1F, 0x579E87, 0x57E26E, 0x57E942
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E428, (aoc)0x00843CE4)]
unsafe struct EESEEChangeEffectAction
{
    [address((ee)0x005602DF, (aoc)0x0056FC3A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x572A2A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E3A8, (aoc)0x00843C64)]
unsafe struct EESEEChangeEffectType
{
    [address((ee)0x005602C5, (aoc)0x0056FC20), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x569046
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082DB60, (aoc)0x00843410)]
unsafe struct EESEEChangeStartEpoch
{
    [address((ee)0x00568F75, (aoc)0x00578958), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x59F2B8
[@base(typeof(UIEvent)), size((ee)0x20, (aoc)0x20), vtable((ee)0x008306A8, (aoc)0x00845F98)]
unsafe struct EESEEChangeViewport
{
    [address((ee)0x0059F351, (aoc)0x005AF55A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56FDC1
[@base(typeof(UIEvent)), size((ee)0x1C, (aoc)0x1C), vtable((ee)0x0082E28C, (aoc)0x00843B44)]
unsafe struct EESEECheckDisplay
{
    [address((ee)0x00560464, (aoc)0x0056FDBF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x576ABF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E74C, (aoc)0x00844008)]
unsafe struct EESEEClearAddLine
{
    [address((ee)0x005768E4, (aoc)0x005864D0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56F4E7
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E254, (aoc)0x00843B0C)]
unsafe struct EESEEConditionPlayerResource
{
    [address((ee)0x0056024C, (aoc)0x0056FBA7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DCE0, (aoc)0x00843590)]
unsafe struct EESEEDecPriority
{
    [address((ee)0x0055FFEE, (aoc)0x0056F926), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DD24, (aoc)0x008435D4)]
unsafe struct EESEEDeleteEntry
{
    [address((ee)0x0055FFC0, (aoc)0x0056F8F8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56575B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082D934, (aoc)0x008431E4)]
unsafe struct EESEEDisplayEpoch
{
    [address((ee)0x005632FB, (aoc)0x00572C33), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DD38, (aoc)0x008435E8)]
unsafe struct EESEEDuplicateEntry
{
    [address((ee)0x0055FFA9, (aoc)0x0056F8E1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58ECA2
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F870, (aoc)0x00845154)]
unsafe struct EESEEExitScenario
{
    [address((ee)0x0059F550, (aoc)0x005AF7E2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x57B9B5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EA88, (aoc)0x00844348)]
unsafe struct EESEEFaceAreaObject
{
    [address((ee)0x0057B708, (aoc)0x0058B31F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x560EC7
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082D520, (aoc)0x00842DD0)]
unsafe struct EESEEFindText
{
    [address((ee)0x0059F0A0, (aoc)0x005AF2A9), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x574E5D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E678, (aoc)0x00843F34)]
unsafe struct EESEEGCPlace
{
    [address((ee)0x00574D46, (aoc)0x00584A41), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5672AC
[@base(typeof(UIESliderChange)), size((ee)0x24, (aoc)0x24), vtable((ee)0x0082DAA8, (aoc)0x00843358)]
unsafe struct EESEEGameUnitLimit
{
    [address((ee)0x0059F272, (aoc)0x005AF47B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x2C, (aoc)0x2C), vtable((ee)0x0082F8BC, (aoc)0x0084518C)]
unsafe struct EESEEHideUI
{
    [address((ee)0x0059F2B8, (aoc)0x005AF4C1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DCF4, (aoc)0x008435A4)]
unsafe struct EESEEIncPriority
{
    [address((ee)0x0055FFD7, (aoc)0x0056F90F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58ECA2
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F834, (aoc)0x00845118)]
unsafe struct EESEELoadScenario
{
    [address((ee)0x0059F396, (aoc)0x005AF59F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x57B9B5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EA9C, (aoc)0x0084435C)]
unsafe struct EESEEMoveAreaObject
{
    [address((ee)0x00577D4D, (aoc)0x0058B2DC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DD4C, (aoc)0x008435FC)]
unsafe struct EESEENewEntry
{
    [address((ee)0x0055FF92, (aoc)0x0056F8CA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x57ED81
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EB24, (aoc)0x008443E4)]
unsafe struct EESEEObjectCheck
{
    [address((ee)0x005600F7, (aoc)0x0056FA39), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x57ED81
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082ED38, (aoc)0x008445F8)]
unsafe struct EESEEObjectDetail
{
    [address((ee)0x0056005C, (aoc)0x0056F994), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x57ED81
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EBD0, (aoc)0x00844490)]
unsafe struct EESEEObjectUnitSelect
{
    [address((ee)0x00560115, (aoc)0x0056FA57), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x57584F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E698, (aoc)0x00843F54)]
unsafe struct EESEEPlace
{
    [address((ee)0x00575716, (aoc)0x0058547C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082F8D0, (aoc)0x008451A0)]
unsafe struct EESEEQuickLoad
{
    [address((ee)0x0059F385, (aoc)0x005AF58E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082F8E4, (aoc)0x008451B4)]
unsafe struct EESEEQuickSave
{
    [address((ee)0x0059F5E2, (aoc)0x005AF874), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58ECA2
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F848, (aoc)0x0084512C)]
unsafe struct EESEESaveScenario
{
    [address((ee)0x0059F4B8, (aoc)0x005AF74A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DE18, (aoc)0x008436C8)]
unsafe struct EESEESelectEntry
{
    [address((ee)0x0055FF7B, (aoc)0x0056F8B3), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E, 0x56F729
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DC1C, (aoc)0x008434CC)]
unsafe struct EESEESelectName
{
    [address((ee)0x00560045, (aoc)0x0056F97D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DC34, (aoc)0x008434E4)]
unsafe struct EESEEShowDebugIDs
{
    [address((ee)0x0056001C, (aoc)0x0056F954), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x56B75E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082DD60, (aoc)0x00843610)]
unsafe struct EESEEShowTab
{
    [address((ee)0x0055FF5A, (aoc)0x0056F892), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x2C, (aoc)0x2C), vtable((ee)0x0082E2A0, (aoc)0x00843B58)]
unsafe struct EESEESliderChange
{
    [address((ee)0x0056040F, (aoc)0x0056FD6A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58ECA2
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F85C, (aoc)0x00845140)]
unsafe struct EESEETestScenario
{
    [address((ee)0x0059F4EC, (aoc)0x005AF77E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x583705
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EEC0, (aoc)0x00844780)]
unsafe struct EESEETriggerChangeIndex
{
    [address((ee)0x0056037B, (aoc)0x0056FCD6), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x583705
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EEAC, (aoc)0x0084476C)]
unsafe struct EESEETriggerConditionTimer
{
    [address((ee)0x0056031D, (aoc)0x0056FC78), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x583705
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EE98, (aoc)0x00844758)]
unsafe struct EESEETriggerOperator
{
    [address((ee)0x005602F9, (aoc)0x0056FC54), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x583705
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082EED4, (aoc)0x00844794)]
unsafe struct EESEETriggerRemove
{
    [address((ee)0x005603EB, (aoc)0x0056FD46), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x577E65
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082E784, (aoc)0x00844040)]
unsafe struct EESEEUseAreaObject
{
    [address((ee)0x00577D4D, (aoc)0x0058B2DC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x573C90, 0x578379
[@base(typeof(UIEvent)), size((ee)0x18, (aoc)0x18), vtable((ee)0x0082E544, (aoc)0x00843E00)]
unsafe struct EESEEUseObjectArea
{
    [address((ee)0x00560520, (aoc)0x0056FE58), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5607B1
[size((ee)0x54, (aoc)0x54), vtable((ee)0x0082D300, (aoc)0x00842BB0)]
unsafe struct EESEIAExport
{
}

/// made in 0x560C6A
[size((ee)0x58, (aoc)0x58), vtable((ee)0x0082D310, (aoc)0x00842BC0)]
unsafe struct EESEIAImport
{
}

/// made in 0x560EC7
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082D324, (aoc)0x00842BD4)]
unsafe struct EESEIEExport
{
    [address((ee)0x00560838, (aoc)0x00570170), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x560EC7
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082D338, (aoc)0x00842BE8)]
unsafe struct EESEIEImport
{
    [address((ee)0x00560D00, (aoc)0x00570638), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(EEUIFSEMainGroup)), size((ee)0x4C, (aoc)0x4C), vtable((ee)0x0082D534, (aoc)0x00842DE4)]
unsafe struct EESEInstructions
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x00560EC7, (aoc)0x005707FF)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(UIButton)), size((ee)0x104, (aoc)0x104), vtable((ee)0x0082D5A4, (aoc)0x00842E54)]
unsafe struct EESEPSSetPlayers
{
    /// its constructor (4 arguments, not typed yet)
    [address((ee)0x00563312, (aoc)0x00572C4A)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4);

    [address((ee)0x005633D6, (aoc)0x00572D0E), overrides(typeof(UIButton))]
    extern void button_up();
}

[@base(typeof(EEUIFSEMainGroup)), size((ee)0x138, (aoc)0x138), vtable((ee)0x0082D82C, (aoc)0x008430DC)]
unsafe struct EESEPlayerScreen
{
    /// its constructor (4 arguments, not typed yet)
    [address((ee)0x005639DB, (aoc)0x00573313)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4);
}

[@base(typeof(UIRadioButton)), size((ee)0x114, (aoc)0x114), vtable((ee)0x0082D78C, (aoc)0x0084303C)]
unsafe struct EESERBUnitTypes
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x005638E0, (aoc)0x00573218)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

[@base(typeof(UIComboBox)), size((ee)0xE0, (aoc)0xE0), vtable((ee)0x0082EDCC, (aoc)0x008446B4)]
unsafe struct EESETSAComboBox
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x005815AC, (aoc)0x00591840)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

/// made in 0x57ED81
[@base(typeof(UIEditBox)), size((ee)0x1E0, (aoc)0x1E0), vtable((ee)0x0082ECA4, (aoc)0x00844564)]
unsafe struct EESETSAEditBox
{
}

/// made in 0x57ED81
[@base(typeof(EESEALasso)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082ED50, (aoc)0x00844610)]
unsafe struct EESETSALasso
{
}

/// made in 0x57ED81
[@base(typeof(EESEAMouseClick)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082ED64, (aoc)0x00844624)]
unsafe struct EESETSAMouseClick
{
}

[@base(typeof(EESETSBaseObject)), size((ee)0x38, (aoc)0x38), vtable((ee)0x0082DF58, (aoc)0x0084380C)]
unsafe struct EESETSArea
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056D39A, (aoc)0x0057CCF3)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[vtable((ee)0x0082DFA4, (aoc)0x00843858)]
unsafe struct EESETSBaseObject
{
}

[@base(typeof(UIComboBox)), size((ee)0xE0, (aoc)0xE0), vtable((ee)0x0082E004, (aoc)0x00843960)]
unsafe struct EESETSCComboBox
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x0056E938, (aoc)0x0057E33D)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

[@base(typeof(EESETSConditionType)), vtable((ee)0x0082E0D8, (aoc)0x00843A34)]
unsafe struct EESETSCTAlwaysTrue
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056EA99, (aoc)0x0057E49E)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

/// made in 0x56EA99
[@base(typeof(EESETSConditionType)), vtable((ee)0x0082E138, (aoc)0x00843A94)]
unsafe struct EESETSCTChatMessage
{
}

[@base(typeof(EESETSConditionType)), vtable((ee)0x0082E0C0, (aoc)0x00843A1C)]
unsafe struct EESETSCTDiplomacy
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056EA99, (aoc)0x0057E49E)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(EESETSConditionType)), vtable((ee)0x0082E0A8, (aoc)0x00843A04)]
unsafe struct EESETSCTGame
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056EA99, (aoc)0x0057E49E)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

/// made in 0x56EA99
[@base(typeof(EESETSConditionType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E0F0, (aoc)0x00843A4C)]
unsafe struct EESETSCTPlayerResearch
{
}

[@base(typeof(EESETSConditionType)), size((ee)0x8, (aoc)0x8), vtable((ee)0x0082E268, (aoc)0x00843B20)]
unsafe struct EESETSCTPlayerResource
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0056F4E7, (aoc)0x0057EE40)]
    extern void* init();
}

[@base(typeof(EESETSConditionType)), vtable((ee)0x0082E120, (aoc)0x00843A7C)]
unsafe struct EESETSCTTriggerAttribute
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056EA99, (aoc)0x0057E49E)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(EESETSBaseObject)), vtable((ee)0x0082E164, (aoc)0x00843AC0)]
unsafe struct EESETSCondition
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056EA99, (aoc)0x0057E49E)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[vtable((ee)0x0082E108, (aoc)0x00843A64)]
unsafe struct EESETSConditionType
{
}

[@base(typeof(UIComboBox)), size((ee)0xE0, (aoc)0xE0), vtable((ee)0x0082E2C4, (aoc)0x00843B80)]
unsafe struct EESETSEComboBox
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x00572846, (aoc)0x00582445)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

[@base(typeof(UIComboBox)), size((ee)0xE8, (aoc)0xE8), vtable((ee)0x0082E1B0, (aoc)0x008438BC)]
unsafe struct EESETSEScenarioComboBox
{
    /// its constructor (16 arguments, not typed yet)
    [address((ee)0x0056F403, (aoc)0x0057E291)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11, uint32 p12, uint32 p13, uint32 p14, uint32 p15, uint32 p16);

    [address((ee)0x0056B47B, (aoc)0x0057ADD4), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E4DC, (aoc)0x00843D98)]
unsafe struct EESETSETAI
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005731F8, (aoc)0x00582DDA)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E49C, (aoc)0x00843D58)]
unsafe struct EESETSETBuildObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005731F8, (aoc)0x00582DDA)]
    extern void* init();
}

/// made in 0x574037
[@base(typeof(UIEvent)), size((ee)0x18, (aoc)0x18), vtable((ee)0x0082E558, (aoc)0x00843E14)]
unsafe struct EESETSETCAMGModifyObject
{
    [address((ee)0x005739C3, (aoc)0x0058370D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(EESETSEffectType)), size((ee)0x24, (aoc)0x24), vtable((ee)0x0082E500, (aoc)0x00843DBC)]
unsafe struct EESETSETCreateAIMilitaryGroup
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0057384E, (aoc)0x0058347F)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E898, (aoc)0x00844154)]
unsafe struct EESETSETFreezeObject
{
}

[@base(typeof(EESETSEffectType)), size((ee)0x28, (aoc)0x28), vtable((ee)0x0082E658, (aoc)0x00843F14)]
unsafe struct EESETSETGame
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0057467D, (aoc)0x005843C7)]
    extern void* init();
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E578, (aoc)0x00843E34)]
unsafe struct EESETSETGameAmbientWeather
{
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E5F8, (aoc)0x00843EB4)]
unsafe struct EESETSETGameCalamity
{
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E618, (aoc)0x00843ED4)]
unsafe struct EESETSETGameCycleTime
{
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E638, (aoc)0x00843EF4)]
unsafe struct EESETSETGameDayNight
{
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E598, (aoc)0x00843E54)]
unsafe struct EESETSETGameEndCinematic
{
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E5D8, (aoc)0x00843E94)]
unsafe struct EESETSETGameGraphicEffect
{
}

/// made in 0x57467D
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E5B8, (aoc)0x00843E74)]
unsafe struct EESETSETGameStartCinematic
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E878, (aoc)0x00844134)]
unsafe struct EESETSETKillObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E8B8, (aoc)0x00844174)]
unsafe struct EESETSETLogObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

/// made in 0x577233
[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E764, (aoc)0x00844020)]
unsafe struct EESETSETMSDCameraScrollChange
{
    [address((ee)0x00576FC4, (aoc)0x00586BB0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E47C, (aoc)0x00843D38)]
unsafe struct EESETSETMaintainObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005731F8, (aoc)0x00582DDA)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), size((ee)0x1C, (aoc)0x1C), vtable((ee)0x0082E72C, (aoc)0x00843FE8)]
unsafe struct EESETSETMedia
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0057641E, (aoc)0x00585FED)]
    extern void* init();
}

/// made in 0x57641E
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E6EC, (aoc)0x00843FA8)]
unsafe struct EESETSETMediaChangeText
{
}

/// made in 0x57641E
[@base(typeof(EESETSEffectType)), size((ee)0x8, (aoc)0x8), vtable((ee)0x0082E70C, (aoc)0x00843FC8)]
unsafe struct EESETSETMediaPlayGraphic
{
}

/// made in 0x57641E
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E6CC, (aoc)0x00843F88)]
unsafe struct EESETSETMediaSendDialog
{
}

/// made in 0x57641E
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E6AC, (aoc)0x00843F68)]
unsafe struct EESETSETMediaSendMessage
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E91C, (aoc)0x008441D8)]
unsafe struct EESETSETMiniMapFlare
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

/// made in 0x5731F8
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E4BC, (aoc)0x00843D78)]
unsafe struct EESETSETModifyAIMilitaryTarget
{
}

/// made in 0x572A2A
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E388, (aoc)0x00843C44)]
unsafe struct EESETSETNoEffect
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E8F8, (aoc)0x008441B4)]
unsafe struct EESETSETObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

/// made in 0x578A1F
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E8D8, (aoc)0x00844194)]
unsafe struct EESETSETObjectCreate
{
}

/// made in 0x578A1F
[@base(typeof(EESETSEffectType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E818, (aoc)0x008440D4)]
unsafe struct EESETSETObjectToArea
{
}

/// made in 0x578A1F
[@base(typeof(EESETSEffectType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E838, (aoc)0x008440F4)]
unsafe struct EESETSETObjectToObject
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E97C, (aoc)0x00844238)]
unsafe struct EESETSETPCameraFadeIn
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E95C, (aoc)0x00844218)]
unsafe struct EESETSETPCameraFadeOut
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E93C, (aoc)0x008441F8)]
unsafe struct EESETSETPVisibility
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082EA5C, (aoc)0x00844318)]
unsafe struct EESETSETPlayer
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E99C, (aoc)0x00844258)]
unsafe struct EESETSETPlayerCivPoint
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082EA1C, (aoc)0x008442D8)]
unsafe struct EESETSETPlayerDiplomacy
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E9FC, (aoc)0x008442B8)]
unsafe struct EESETSETPlayerResource
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

/// made in 0x579E87
[@base(typeof(EESETSEffectType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E9DC, (aoc)0x00844298)]
unsafe struct EESETSETPlayerTechnology
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082EA3C, (aoc)0x008442F8)]
unsafe struct EESETSETPlayerVictory
{
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E9BC, (aoc)0x00844278)]
unsafe struct EESETSETPlayerView
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00579E87, (aoc)0x00589904)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E858, (aoc)0x00844114)]
unsafe struct EESETSETRemoveObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

/// made in 0x57CB17
[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082EAC4, (aoc)0x00844384)]
unsafe struct EESETSETSAAAttributeChange
{
    [address((ee)0x0057C8CC, (aoc)0x0058C4E3), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5731F8
[@base(typeof(EESETSEffectType)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082E43C, (aoc)0x00843CF8)]
unsafe struct EESETSETSetAIAttribute
{
}

[@base(typeof(EESETSEffectType)), size((ee)0x34, (aoc)0x34), vtable((ee)0x0082E520, (aoc)0x00843DDC)]
unsafe struct EESETSETSetFavoriteUnit
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005738C1, (aoc)0x005834F2)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E7D8, (aoc)0x00844094)]
unsafe struct EESETSETSetObjectAttribute
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E798, (aoc)0x00844054)]
unsafe struct EESETSETSetObjectClassAttribute
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E7B8, (aoc)0x00844074)]
unsafe struct EESETSETSetObjectName
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00578A1F, (aoc)0x0058849C)]
    extern void* init();
}

/// made in 0x578A1F
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E7F8, (aoc)0x008440B4)]
unsafe struct EESETSETSetObjectOwner
{
}

[@base(typeof(EESETSEffectType)), size((ee)0x8, (aoc)0x8), vtable((ee)0x0082EADC, (aoc)0x0084439C)]
unsafe struct EESETSETSetObjectStance
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x0057E26E, (aoc)0x0058E4A4)]
    extern void* init();
}

[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E45C, (aoc)0x00843D18)]
unsafe struct EESETSETToggleAI
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005731F8, (aoc)0x00582DDA)]
    extern void* init();
}

/// made in 0x572A2A
[@base(typeof(EESETSEffectType)), vtable((ee)0x0082E368, (aoc)0x00843C24)]
unsafe struct EESETSETTrigger
{
}

[@base(typeof(EESETSBaseObject)), size((ee)0x38, (aoc)0x38), vtable((ee)0x0082E3BC, (aoc)0x00843C78)]
unsafe struct EESETSEffect
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x00572A2A, (aoc)0x00582629)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[vtable((ee)0x0082E408, (aoc)0x00843CC4)]
unsafe struct EESETSEffectType
{
}

[@base(typeof(UILBString)), vtable((ee)0x0082EBE4, (aoc)0x008444A4)]
unsafe struct EESETSOUnitList
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0057ED81, (aoc)0x0058EFB7)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(EESETSBaseObject)), vtable((ee)0x0082ED78, (aoc)0x00844638)]
unsafe struct EESETSObject
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0057ED81, (aoc)0x0058EFB7)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(UICheckBox)), vtable((ee)0x0082DC48, (aoc)0x008434F8)]
unsafe struct EESETSShowIDsCheckBox
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056B75E, (aoc)0x0057B0B7)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(EESETSBaseObject)), size((ee)0xAC, (aoc)0xAC), vtable((ee)0x0082EEEC, (aoc)0x008447AC)]
unsafe struct EESETSTrigger
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x00583705, (aoc)0x0059393B)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(UIComboBox)), size((ee)0xE4, (aoc)0xE4), vtable((ee)0x0082EF38, (aoc)0x008447F8)]
unsafe struct EESETSTriggerUIComboBox
{
    /// its constructor (15 arguments, not typed yet)
    [address((ee)0x005859FF, (aoc)0x00595C35)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11, uint32 p12, uint32 p13, uint32 p14, uint32 p15);

    [address((ee)0x00585A7F, (aoc)0x00595CB5), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(UIComboBox)), size((ee)0xE8, (aoc)0xE8), vtable((ee)0x0082EFDC, (aoc)0x0084489C)]
unsafe struct EESETSUICBPlayer
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x00586C88, (aoc)0x00596EBE)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

[@base(typeof(UIRadioButton)), size((ee)0x114, (aoc)0x114), vtable((ee)0x0082D6EC, (aoc)0x00842F9C)]
unsafe struct EESETab
{
    /// its constructor (6 arguments, not typed yet)
    [address((ee)0x00563728, (aoc)0x00573060)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6);
}

[@base(typeof(EEUIFSEMainGroup)), vtable((ee)0x0082DEEC, (aoc)0x0084379C)]
unsafe struct EESETriggerScreen
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0056B75E, (aoc)0x0057B0B7)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(UIButton)), size((ee)0x108, (aoc)0x108), vtable((ee)0x0082D9E8, (aoc)0x00843298)]
unsafe struct EESEUIBDiplomacy
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x00566EE0, (aoc)0x00576818)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    [address((ee)0x00567277, (aoc)0x00576BAF), overrides(typeof(UIButton))]
    extern void button_up();
}

[@base(typeof(EEUIBuildGate)), vtable((ee)0x0082F608, (aoc)0x00844ECC)]
unsafe struct EESEUIBuildGate
{
}

[@base(typeof(EEUIBuildWall)), vtable((ee)0x0082F624, (aoc)0x00844EE8)]
unsafe struct EESEUIBuildWall
{
    /// its constructor (4 arguments, not typed yet)
    [address((ee)0x0058A70E, (aoc)0x0059A93E)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4);
}

[@base(typeof(EEUICCommandUnits)), size((ee)0xD4, (aoc)0xD4), vtable((ee)0x0082F090, (aoc)0x00844950)]
unsafe struct EESEUICCommandUnits
{
    /// its constructor (4 arguments, not typed yet)
    [address((ee)0x0058702B, (aoc)0x00597261)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4);

    [address((ee)0x005870B0, (aoc)0x005972E6), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x005870E0, (aoc)0x00597316), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(EEUICSelectUnits)), size((ee)0xD8, (aoc)0xD8), vtable((ee)0x0082F120, (aoc)0x008449E0)]
unsafe struct EESEUICTriggerSelectUnits
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x0058715D, (aoc)0x00597393)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    [address((ee)0x0058725B, (aoc)0x00597491), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x005871B2, (aoc)0x005973E8), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x005873EE, (aoc)0x00597624), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x005873FA, (aoc)0x00597630), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(UILBString)), size((ee)0x15C, (aoc)0x15C), vtable((ee)0x0082F1AC, (aoc)0x00844A6C)]
unsafe struct EESEUITechListBox
{
    /// its constructor (10 arguments, not typed yet)
    [address((ee)0x00587415, (aoc)0x0059764B)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10);

    [address((ee)0x005874E6, (aoc)0x0059771C), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x0058765F, (aoc)0x0059789F), overrides(typeof(UIControl))]
    extern int32 draw_polygons();
}

/// the terrain panel's colour list (EEUIFSETerrain.color_list, made in 0x59B710): an item a colour type, drawn as a
/// strip of its colours - an item a vector of GEColor (0x10: the allocator, first +4, end +8, 3 floats each)
[@base(typeof(UIControl)), size((ee)0x134, (aoc)0x134), vtable((ee)0x00830220, (aoc)0x00845B24)]
[c_type("ee_se_color_list")]
unsafe struct EESEUITerrainColorLB
{
    /// the selected item (0x59D123 reads it: valid while there are items and +0x133 is set)
    [offset(0x120), found(c)]
    int32 selected;

    /// vf34: an item more at the end, copied from item (a vector of GEColor)
    [slot(34), address((ee)0x0059CCC8, (aoc)0x005ACE34)]
    extern void add(void* item);

    /// vf36: the item selected - notify: the list's event (EEUIETerrainColorChange, 0x3EB) runs; scroll: into view
    [slot(36), address((ee)0x0059CD3A, (aoc)0x0065BEB7)]
    extern void select(uint32 index, uint8 notify, uint8 scroll);

    /// vf39: the item at index taken out
    [slot(39), address((ee)0x0059D083, (aoc)0x005AD12E)]
    extern void remove(uint32 index);

    /// vf42: the items
    [slot(42), address((ee)0x0059D165, (aoc)0x005AD428)]
    extern uint32 count();

    [address((ee)0x0059C1B1, (aoc)0x005AC31D), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x0059A9C4, (aoc)0x005AAAD4), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0059D111, (aoc)0x0065F3F4), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x0059D985, (aoc)0x005AD441), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0059DA20, (aoc)0x0065C4C4), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    /// a double click: the item under the mouse selected (with notify), then its event 0x3EC (none listens)
    [address((ee)0x0059D17E, (aoc)0x005AD4DC), overrides(typeof(UIControl))]
    [vtable_site("se_color_list_double_click")]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x00649012, (aoc)0x005AD5A8), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x006470AE, (aoc)0x005ADDD5), overrides(typeof(UIControl))]
    extern int32 mouse_wheel_turned(int32 amount);

    [address((ee)0x00648EC1, (aoc)0x005AD1F2), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x006490E8, (aoc)0x005ADE79), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0059D24A, (aoc)0x0065C61F), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x006470CC, (aoc)0x005AD327), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

[@base(typeof(UIControl)), size((ee)0x144, (aoc)0x144), vtable((ee)0x00830328, (aoc)0x00845C2C)]
unsafe struct EESEUITerrainTypeLB
{
    /// its constructor (7 arguments, not typed yet)
    [address((ee)0x0059C1E4, (aoc)0x005AC350)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7);

    [address((ee)0x0059ABE2, (aoc)0x005AACF2), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x0059ACD1, (aoc)0x005AADE1), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0059B092, (aoc)0x005AB1A2), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x0059D985, (aoc)0x005AD441), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0059DA20, (aoc)0x0065C4C4), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0059D17E, (aoc)0x005AD4DC), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x00649012, (aoc)0x005AD5A8), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x006470AE, (aoc)0x005ADDD5), overrides(typeof(UIControl))]
    extern int32 mouse_wheel_turned(int32 amount);

    [address((ee)0x00648EC1, (aoc)0x005AD1F2), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x006490E8, (aoc)0x005ADE79), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0059D24A, (aoc)0x0065C61F), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x006470CC, (aoc)0x005AD327), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// the editor's units panel (its main button 3: the panel factory 0x58EB4B makes it with the editor): the unit types'
/// list, its categories, the world player / player switch and the sort switches (engine/editor_search.c: a search box)
[@base(typeof(EEUIFSEMainGroup)), size((ee)0x140, (aoc)0x140), vtable((ee)0x0082F5D8, (aoc)0x00844E9C)]
[c_type("ee_se_unit")]
unsafe struct EESEUnit
{
    /// the unit types' list
    [offset(0x8C), found(c)]
    EEUIFSELBUnitList* list;

    /// the list's category, sort and units (the scenario's or all) as refill last built it: it builds the list anew
    /// only when one of the three has changed
    [offset(0x12C), found(c)]
    int32 shown_category;

    [offset(0x130), found(c)]
    int32 shown_sort;

    [offset(0x134), found(c)]
    int32 shown_units;

    /// a unit type of the list is being placed: set by the list's choice (0x58C025), cleared by a right click on the map
    /// and when the panel is left (0x58B9D7); read by the map's click (vf9), a click without it selects what is there
    [offset(0x13C), found(c)]
    uint8 placing;

    /// with placing: it is a wall's line that is being laid (the list's choice, the builder at +0x1C)
    [offset(0x13D), found(c)]
    uint8 placing_wall;

    /// with placing: it is a gate that is being placed (the list's choice, the builder at +0x3C)
    [offset(0x13E), found(c)]
    uint8 placing_gate;

    /// its constructor: the form's running control index (each control takes one), the editor's form, the labels'
    /// font, the form's +0x250
    [address((ee)0x0058A70E, (aoc)0x0059A93E)]
    [call_site("se_unit_init", (ee)0x0058EC1F, (aoc)0x0059F02E)]
    extern void* init(int32* index, UIForm* form, void* font, uint32 arg);

    /// the list built anew - every unit type of the category shown added (list.add) - when the category, the sort or
    /// the units have changed (shown_*): when the panel is opened and by the sort and category switches' event
    /// (EEUIFSEESortUnitList)
    [address((ee)0x0058C2F5, (aoc)0x0059C64B)]
    extern void refill();

    /// the list's chosen type taken up again when placing: the ghosts made anew (the player switch's event)
    [address((ee)0x0058BD07, (aoc)0x0059C05D)]
    extern void reselect();

    /// vf0: deleted - its controls with it (0x58B7E2), each by its own vf0
    [slot(0), address((ee)0x0058B7C6, (aoc)0x0059BB1C)]
    [vtable_site("se_unit_delete")]
    extern void* destroy(uint8 flags);

    /// vf1: every frame while the panel is the one shown (the ghosts' preview)
    [slot(1), address((ee)0x0058B9F4, (aoc)0x0059BD4A)]
    [vtable_site("se_unit_update")]
    extern void update();
}

[@base(typeof(UIButton)), size((ee)0x104, (aoc)0x104), vtable((ee)0x0082F524, (aoc)0x00844DE8)]
unsafe struct EEUIFSEBRemoveCargo
{
    /// its constructor (10 arguments, not typed yet)
    [address((ee)0x0058A672, (aoc)0x0059A8A2)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10);

    [address((ee)0x0058A650, (aoc)0x0059A880), overrides(typeof(UIButton))]
    extern void button_up();
}

[@base(typeof(UIButton)), size((ee)0x11C, (aoc)0x11C), vtable((ee)0x0082F48C, (aoc)0x00844D50)]
unsafe struct EEUIFSEBRotation
{
    /// its constructor (10 arguments, not typed yet)
    [address((ee)0x00589FDD, (aoc)0x0059A213)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10);

    [address((ee)0x0058A2E5, (aoc)0x0059A515), overrides(typeof(UIButton))]
    extern void button_up();

    [address((ee)0x0058A38A, (aoc)0x0059A5BA), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0058A39A, (aoc)0x0059A5CA), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x0058A4F8, (aoc)0x0059A728), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

[@base(typeof(UIRadioButton)), size((ee)0x114, (aoc)0x114), vtable((ee)0x0082F3EC, (aoc)0x00844CB0)]
unsafe struct EEUIFSEBUnitType
{
    /// its constructor (11 arguments, not typed yet)
    [address((ee)0x00589E7B, (aoc)0x0059A0B1)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11);
}

/// made in 0x59198C
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082FA84, (aoc)0x00845354)]
unsafe struct EEUIFSEEMapType
{
    [address((ee)0x0059196C, (aoc)0x005A1BC6), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UISlider)), size((ee)0xF8, (aoc)0xF8), vtable((ee)0x0082F944, (aoc)0x00845214)]
unsafe struct EEUIFSEESElevation
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x0059113D, (aoc)0x005A1397)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);
}

/// made in 0x58A70E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F5C4, (aoc)0x00844E88)]
unsafe struct EEUIFSEESortUnitList
{
    [address((ee)0x0058B784, (aoc)0x0059BADA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(EEUIFSEMainGroup)), size((ee)0x34, (aoc)0x34), vtable((ee)0x0082F914, (aoc)0x008451E4), c_type("ee_se_elevation")]
unsafe struct EEUIFSEElevation
{
    /// its radio buttons, the last under the others: the elevation mode's (ids 0x96, 0x97, 0xA0)
    [offset(0xC), found(c)]
    UIControl* smooth;

    /// its constructor, as the terrain panel's: the editor's form's (0x58EC75), index its control index (taken and counted on)
    [address((ee)0x00590734, (aoc)0x005A098E)]
    [call_site("se_elevation_init", (ee)0x0058EC75, (aoc)0x0059F084)]
    extern void* init(uint32 p1, uint32 p2, int32* index, UIForm* form, void* font);

    /// vf0: deleted with the editor
    [slot(0), address((ee)0x00591003, (aoc)0x005A125D)]
    [vtable_site("se_elevation_delete")]
    extern void* destroy(uint8 flags);
}

[@base(typeof(UILBString)), size((ee)0x14C, (aoc)0x14C), vtable((ee)0x0082F32C, (aoc)0x00844BF0)]
unsafe struct EEUIFSELBCargoList
{
    /// its constructor (10 arguments, not typed yet)
    [address((ee)0x00589C83, (aoc)0x00599EB9)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10);
}

/// the units panel's list of unit types (EESEUnit.list): their names, the type ids beside them (+0x140)
[@base(typeof(UILBString)), size((ee)0x154, (aoc)0x154), vtable((ee)0x0082F26C, (aoc)0x00844B30)]
[c_type("ee_se_unit_list")]
unsafe struct EEUIFSELBUnitList
{
    /// its constructor (10 arguments, not typed yet)
    [address((ee)0x005898C7, (aoc)0x00599AFD)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10);

    /// a unit type into the list at its place in the list's sort: its name (the type's name_text), or its epoch or
    /// family number and its name
    [address((ee)0x005899BC, (aoc)0x00599BF2)]
    [call_site("se_unit_list_add", (ee)0x0058C46B, (aoc)0x0059C7C5)]
    extern void add(int32 type);

    [address((ee)0x00589C4B, (aoc)0x00599E81), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

[@base(typeof(UIRadioButton)), vtable((ee)0x0082F9DC, (aoc)0x008452AC)]
unsafe struct EEUIFSEMainButton
{
    /// its constructor (10 arguments, not typed yet)
    [address((ee)0x00591225, (aoc)0x005A147F)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10);
}

[vtable((ee)0x0082D564, (aoc)0x00842E14)]
unsafe struct EEUIFSEMainGroup
{
}

[@base(typeof(EEUIFSEMainGroup)), size((ee)0x7C, (aoc)0x7C), vtable((ee)0x0082FBDC, (aoc)0x008454AC)]
unsafe struct EEUIFSEMap
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0059198C, (aoc)0x005A1BE6)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);
}

[@base(typeof(UIControl)), size((ee)0xC0, (aoc)0xC0), vtable((ee)0x0082FC0C, (aoc)0x008454DC)]
unsafe struct EEUIFSEPaint
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x00593CB6, (aoc)0x005A3F10)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x00593DDC, (aoc)0x005A4036), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00593DFA, (aoc)0x005A4054), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x00593F73, (aoc)0x005A41CD), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0059400B, (aoc)0x005A4265), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

[@base(typeof(EEUIFSEPaint)), size((ee)0xE4, (aoc)0xE4), vtable((ee)0x0082FC9C, (aoc)0x0084556C)]
unsafe struct EEUIFSEPaintElevation
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x00594216, (aoc)0x005A4470)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x0059447A, (aoc)0x005A475D), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0059449F, (aoc)0x005A4782), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x0059474E, (aoc)0x005A4A31), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x00594888, (aoc)0x005A4B6B), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(UIRadioButton)), size((ee)0x11C, (aoc)0x11C), vtable((ee)0x0082FD50, (aoc)0x00845620)]
unsafe struct EEUIFSERBBrush
{
    /// its constructor (9 arguments, not typed yet)
    [address((ee)0x005949A6, (aoc)0x005A4C00)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9);
}

[@base(typeof(UIButton)), vtable((ee)0x0082FE00, (aoc)0x008456D0)]
unsafe struct EEUIFSERedo
{
    /// its constructor (9 arguments, not typed yet)
    [address((ee)0x00594A61, (aoc)0x005A4CBB)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9);

    [address((ee)0x00594C16, (aoc)0x005A4D95), overrides(typeof(UIButton))]
    extern void button_up();
}

[@base(typeof(UIRadioButton)), size((ee)0x501E, (aoc)0x501E), vtable((ee)0x008303E8, (aoc)0x00845CEC)]
unsafe struct EEUIFSETRBPaintType
{
    /// its constructor (7 arguments, not typed yet)
    [address((ee)0x0059C31F, (aoc)0x005AC48B)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7);
}

/// the editor's terrain panel (its main button 2): the textures' list, the colours' list, the paint
[@base(typeof(EEUIFSEMainGroup)), size((ee)0x48, (aoc)0x48), vtable((ee)0x008302F8, (aoc)0x00845BFC)]
[c_type("ee_se_terrain")]
unsafe struct EEUIFSETerrain
{
    /// the colour types of the colour list, an item each in its order: a vector of ee_color_type pointers (MSVC: the
    /// first at +4, the end at +8) - the constructor's of every type in_editor; the paint (EEUIFSETerrainPaint vf34)
    /// passes the selected item's type's id
    [offset(0x14), found(c)]
    block color_types;

    /// the title over the colour list
    [offset(0x34), found(c)]
    UIControl* color_label;

    /// the colour list
    [offset(0x3C), found(c)]
    EESEUITerrainColorLB* color_list;

    /// the picture of the selected texture, right of the colour list
    [offset(0x40), found(c)]
    UIControl* preview;

    /// its constructor: the editor form's +0x234 and +0x244, the form's running control index (each control takes
    /// one), the editor's form, the labels' font - the lists filled from the game data
    [address((ee)0x0059B710, (aoc)0x005AB87C)]
    [call_site("se_terrain_init", (ee)0x0058EC4A, (aoc)0x0059F059)]
    extern void* init(uint32 p1, uint32 p2, int32* index, UIForm* form, void* font);

    /// vf0: deleted with the editor
    [slot(0), address((ee)0x0059C445, (aoc)0x005AC5B1)]
    [vtable_site("se_terrain_delete")]
    extern void* destroy(uint8 flags);
}

/// made in 0x59B710
[@base(typeof(EEUIFSEPaint)), size((ee)0xC0, (aoc)0xC0), vtable((ee)0x0083017C, (aoc)0x00845A80)]
unsafe struct EEUIFSETerrainPaint
{
}

[@base(typeof(UIButton)), size((ee)0x108, (aoc)0x108), vtable((ee)0x0082FEA8, (aoc)0x00845778)]
unsafe struct EEUIFSEUndo
{
    /// its constructor (9 arguments, not typed yet)
    [address((ee)0x00594B46, (aoc)0x005A4DBE)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9);

    [address((ee)0x00594C16, (aoc)0x005A4D95), overrides(typeof(UIButton))]
    extern void button_up();
}

/// the scenario editor's form: made by 0x65B9F5 (the operator new's 0x2B8), not a row of dbuiforms.dat; its open
/// (slot 18) lays out the panels (editor_layout.h's vf18)
[@base(typeof(UIForm)), size((ee)0x2B8, (aoc)0x2B8), c_type("ee_editor_form")]
[vtable((ee)0x0082F7B4, (aoc)0x00845084)]
unsafe struct EEUIFScenarioEditor
{
    /// its constructor: the panels, the map, the screens
    [address((ee)0x0058D3E1, (aoc)0x0059D615)]
    [call_site("editor_form", (ee)0x0065BA13, (aoc)0x00673E9B)]
    extern void* init(void* owner);

    /// its minimap (0x58D3E1 makes it; the form's copy_data marks it dirty when its picture has changed)
    [offset(0x238), found(c)]
    EEUIMiniMap* minimap;

    /// every frame the editor is on screen (walls.c: the editor's walls watched while it is)
    [address((ee)0x0058F0D0, (aoc)0x0059F501), overrides(typeof(UIForm))]
    [vtable_site("editor_frame")]
    extern int32 draw_background(void* rect);

    [address((ee)0x0058F997, (aoc)0x0059FDC8), overrides(typeof(UIForm))]
    extern int32 update_keyboard();

    [address((ee)0x0058FC52, (aoc)0x005A0083), overrides(typeof(UIForm))]
    extern int32 update_mouse();

    [address((ee)0x0058E26B, (aoc)0x0059E68A), overrides(typeof(UIForm))]
    extern void close();

    [address((ee)0x0058E30D, (aoc)0x0059E72C), overrides(typeof(UIForm))]
    extern int32 copy_data();

    [address((ee)0x0058F363, (aoc)0x0059F794), overrides(typeof(UIForm))]
    extern void open();
}

static unsafe class editor
{
    /// the listeners told of a change of the ground (a list; what EESEAPaintElevation's end tells, editor_functions.ground_changed)
    [address((ee)0x00918E1C, (aoc)0x0093185C)]
    static block ground_listeners;

    /// the units panel's group of the editor's form (UIForm::AddControl's second argument for each of its controls,
    /// 0x58A70E): shown while the panel is
    [value(6)]
    static uint32 group_units;

    /// the elevation panel's (0x590734)
    [value(2)]
    static uint32 group_elevation;

    /// the terrain panel's (0x59B710)
    [value(3)]
    static uint32 group_terrain;
}

/// the game's functions the editor's code wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class editor_functions
{
    /// the listeners (editor.ground_listeners) told the ground changed in x_lo .. x_hi, y_lo .. y_hi (the end of EESEAPaintElevation's vf1)
    [address((ee)0x004FC9C7, (aoc)0x005074C9), thiscall]
    static extern void ground_changed(void* list, int32 x_lo, int32 y_hi, int32 x_hi, int32 y_lo);

    /// the scenario in the file (an FSFileSpec) loaded over the running one, as the quick load does (1, 1, 1): 0 done
    [address((ee)0x00540997, (aoc)0x0054CF33), cdecl]
    [call_site("se_quick_read", (ee)0x005953F4, (aoc)0x005A5679)]    // EEAQuickSaveLoad's, its only caller
    static extern int32 scenario_read(void* spec, int32 a, int32 b, uint8 c);

    /// push_back on a vector of pointers (MSVC: the vector's allocator first, thiscall on it; the item by reference) -
    /// the terrain panel's constructor's for its colour types (0x59B7D3)
    [address((ee)0x0059C98E, (aoc)0x005ACAFA), thiscall]
    static extern void ptr_vector_push(void* vec, void** item);

    /// can a unit of the type stand on the tile: the terrain paint's objects (0x598B7A) ask it for each one it would
    /// place (players: the game's player list, p5, p6 0 there)
    [address((ee)0x00638D2A, (aoc)0x0064F1F8), cdecl]
    [call_site("ambient_can_place", (ee)0x00598C64, (aoc)0x005A8E28)]
    static extern int8 ambient_can_place(void* players, int32 type, int32 x, int32 y, int32 p5, int32 p6);
}
