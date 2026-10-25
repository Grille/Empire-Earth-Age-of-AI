// A GROUP'S FORMATION (EEFormation and its kinds: EEFLine, EEFShortLine, EEFWedge, EEFCircle ...; a group's +0x34):
// where a group move puts each unit. The group's dispatch (0x5DB641) sets the formation's centre to the clicked point
// and asks it for the slots (FN_FORMATION_SLOTS), then pairs units and slots and gives each unit a location goal at
// its slot. docs/engine-notes.md "Group moves".

// +0x58 its kind: FORMATION_SHORT_LINE ... (the game's factory numbers them so)
enum { FORMATION_SHORT_LINE, FORMATION_LONG_LINE, FORMATION_LOOSE_LINE, FORMATION_STAGGERED_LINE, FORMATION_DEEP_WEDGE,
       FORMATION_SHALLOW_WEDGE, FORMATION_INWARD_CIRCLE, FORMATION_OUTWARD_CIRCLE };
// a formation's slots: the vector FN_FORMATION_SLOTS fills (begin +4, end +8 - the runtime library's), SIZE_FORMATION_SLOT each
internal inline int formation_slot_count(const void *v) {
    uint8_t *b = FIELD(v, 4, uint8_t *), *e = FIELD(v, 8, uint8_t *);
    return b && e > b ? (int)((e - b) / GA(SIZE_FORMATION_SLOT)) : 0;
}
internal inline void *formation_slot(void *v, int i) { return FIELD(v, 4, uint8_t *) + i * GA(SIZE_FORMATION_SLOT); }
internal inline vec3 fslot_get_pos(const void *s) { return FIELD(s, of_formation_slot_pos, vec3); }
internal inline void fslot_set_pos(void *s, vec3 p) { FIELD(s, of_formation_slot_pos, vec3) = p; }
internal inline ee_unit *fslot_get_unit(const void *s) { return FIELD(s, of_formation_slot_unit, ee_unit *); }
// +0x10 the facing the unit takes there - kept as the game wrote it (copied between slots, not read)
internal inline uint32_t fslot_get_facing(const void *s) { return FIELD(s, of_formation_slot_facing, uint32_t); }
internal inline void fslot_set_facing(void *s, uint32_t v) { FIELD(s, of_formation_slot_facing, uint32_t) = v; }

// the slots of a group's move (slot 1 of every formation's vtable) - the hook calls formation_slots for the game's
typedef void (__thiscall *formation_slots_hook)(void *formation, void *group, void *slots);
internal inline void formation_slots(void *formation, void *group, void *slots) { GF(FN_FORMATION_SLOTS)(formation, group, slots); }
internal inline int patch_formation_slots(formation_slots_hook h) {   // every formation's vtable: their count
    return patch_vtables_with(GA(FN_FORMATION_SLOTS), GA(SLOT_FORMATION_SLOTS), GA(SLOT_FORMATION_SLOTS), GA(FN_FORMATION_SLOTS), (void *)h);
}
