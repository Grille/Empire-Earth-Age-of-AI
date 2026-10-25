// AN AI ZONE (ee_zone: a town or resource zone).

// FIELDS: its resource sites, whether it has a drop site, active (player_get_zones_begin(pl))
internal inline void **zone_get_sites_end(const ee_zone *z) { return FIELD(z, of_zone_sites_begin + 0x4, void **); }

// FUNCTIONS
// a zone's site of kind (2: a resource site)
