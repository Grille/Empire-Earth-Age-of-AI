// [autotest]: the test ini's own knobs (ageofai_autotest.ini, run_test.ps1): not in ageofai.ini
namespace ageofai.settings;

[section(0), title("Tests"), test]
class autotest
{
    bool aim_hits = true;
    [range(0, 1000000)]
    int aim_log = 0;
    bool order_log = false;
    /// the AI's fronts (ai/fronts.c: territory, border, each front's width, threat, value, share) every 5 minutes
    bool fronts_log = false;
    [range(100, 300)]
    int route_weight = 100;
    [range(-1, 7)]
    int formation_kind = -1;
    [num]
    int citizen_train_ms = 28000;
    [num]
    int citizen_queue = 0;
    bool navy_fleet_boats = true;
    bool navy_fleet_coast = true;
    [num]
    int navy_fleet_raid = 130;
    bool landing_flag = false;
    bool landing_knows_map = false;
    bool landing_trace = false;
    [num]
    int transport_log = 0;
}
