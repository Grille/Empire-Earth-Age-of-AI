// FIX (scenario bug): the scenario/campaign branch of the game start (0x52599B) sets gamestate+0x411,
// which is copied to the settings byte 0x918819. Several AI routines skip their normal planning when it
// is set (apparently scenario authors were expected to script the AI), so in scenarios played from the
// menu the AI only gathers. Editor test-play and saved games don't set it, which is why they work.
// We clear the byte only while a computer player updates (EEComputerPlayer::Update 0x43F91A, called
// at 0x42F3CD for active AIs), so triggers, civ selection and the rest of the game still see a scenario.
internal int g_fix_scenario_ai = 1;    // ageofai.ini [fixes] scenario_ai=0 turns it off
internal uint8_t g_is_scenario;        // the flag as the game set it (it reads 0 inside the AI update)

internal int is_vanilla(const ee_player *pl);   // below
internal void __thiscall hook_cp_update(ee_player *pl, uint32_t now) {
    uint8_t *flag = scenario_flag_var();
    uint8_t saved = *flag;
    g_is_scenario = saved;
    if (g_fix_scenario_ai && !is_vanilla(pl)) *flag = 0;
    player_update(pl, now);
    *flag = saved;
}
