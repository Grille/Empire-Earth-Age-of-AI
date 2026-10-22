// AN AI ZONE (ee_zone: a town or resource zone).

// FIELDS: its resource sites, whether it has a drop site, active (player_get_zones_begin(pl))
internal inline void **zone_get_sites_begin(const ee_zone *z) { return FIELD(z, ZONE_SITES, void **); }   // +0x8 .. +0xC its sites (ee_unit *)
internal inline void **zone_get_sites_end(const ee_zone *z) { return FIELD(z, ZONE_SITES + 0x4, void **); }
internal inline int zone_get_drop(const ee_zone *z) { return FIELD(z, ZONE_DROP, int); }                 // +0x14 a drop site serves it
internal inline uint8_t zone_get_active(const ee_zone *z) { return FIELD(z, ZONE_ACTIVE, uint8_t); }       // +0x18

// FUNCTIONS
// a zone's site of kind (2: a resource site)
internal inline ee_unit *zone_site(ee_zone *z, int kind) { return (ee_unit *)GF(FN_ZONE_SITE)(z, kind); }
