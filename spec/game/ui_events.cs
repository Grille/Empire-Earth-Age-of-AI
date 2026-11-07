// the exe's UI events (UIEvent's, ui.cs): a button's or a key's handler, a form's open or close - the DLL runs them,
// execute what they do. Each one's event ids (dbuicontrolevents.dat's rows and names; the creator 0x65A1A5's ids are
// dbuiformevents.dat's: the names agree) and the event
// creator that makes it (ui.event_creators) by scripts/re/ui_classes.py --events, which runs each creator for every
// id; the others are made by their forms or the game (a hotkey: UIForm::AddHotKey). Their names: EEUIE then the
// screen - MG the game screen (MGOE its options), MSE the main menu, MS the multiplayer screen (the 2017 update only),
// SP the single-player menu, SL save and load, LE the setup screen (the lobby), GSE the game settings, PGS the
// post-game summary, DS diplomacy, CB the civilization builder, CE the chat and the campaign editor, CCC / CSS / PCE
// the campaigns, SI the scenario intro, WN WON.net, HK the hotkeys.

/// control event 113 "DS Adjust Attack Sound One" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835100, (aoc)0x0084B1D4)]
unsafe struct EEDSUIAdjustAttackSoundOne
{
    [address((ee)0x0063E0D2, (aoc)0x0065475B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 114 "DS Adjust Attack Sound Two" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008350EC, (aoc)0x0084B1C0)]
unsafe struct EEDSUIAdjustAttackSoundTwo
{
    [address((ee)0x0063E0EF, (aoc)0x00654778), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 107 "DS Adjust Command Sound One" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835178, (aoc)0x0084B24C)]
unsafe struct EEDSUIAdjustCommandSoundOne
{
    [address((ee)0x0063E025, (aoc)0x006546AE), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 108 "DS Adjust Command Sound Two" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835164, (aoc)0x0084B238)]
unsafe struct EEDSUIAdjustCommandSoundTwo
{
    [address((ee)0x0063E042, (aoc)0x006546CB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 109 "DS Adjust Created Sound" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835150, (aoc)0x0084B224)]
unsafe struct EEDSUIAdjustCreatedSound
{
    [address((ee)0x0063E05F, (aoc)0x006546E8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 110 "DS Adjust Dying Sound" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083513C, (aoc)0x0084B210)]
unsafe struct EEDSUIAdjustDyingSound
{
    [address((ee)0x0063E07B, (aoc)0x00654704), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 111 "DS Adjust Selected Sound One" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835128, (aoc)0x0084B1FC)]
unsafe struct EEDSUIAdjustSelectedSoundOne
{
    [address((ee)0x0063E098, (aoc)0x00654721), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 112 "DS Adjust Selected Sound Two" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835114, (aoc)0x0084B1E8)]
unsafe struct EEDSUIAdjustSelectedSoundTwo
{
    [address((ee)0x0063E0B5, (aoc)0x0065473E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 115 "DS Adjust Walk Sound One" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008350D8, (aoc)0x0084B1AC)]
unsafe struct EEDSUIAdjustWalkSoundOne
{
    [address((ee)0x0063E10C, (aoc)0x00654795), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 116 "DS Adjust Walk Sound Two" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008350C4, (aoc)0x0084B198)]
unsafe struct EEDSUIAdjustWalkSoundTwo
{
    [address((ee)0x0063E129, (aoc)0x006547B2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 117 "DS Hide Debug Sound Panel" (dbuicontrolevents.dat), made by the event creator 0x65A81E
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008350B0, (aoc)0x0084B184)]
unsafe struct EEDSUIHideDebugSoundPanel
{
    [address((ee)0x0063E146, (aoc)0x006547CF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 51 "CB Accept Change Name", 119 "CB Accept Change Name" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835290, (aoc)0x0084B364)]
unsafe struct EEUIECBAcceptChangeName
{
    [address((ee)0x00665518, (aoc)0x0067CD6A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 60 "CB Accept changes" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008352E0, (aoc)0x0084B3B4)]
unsafe struct EEUIECBAcceptChanges
{
    [address((ee)0x006656F1, (aoc)0x0067CF48), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 53 "CB Accept Load Civilization", 120 "CB Accept Load Civilization" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008352A4, (aoc)0x0084B378)]
unsafe struct EEUIECBAcceptLoadCivilization
{
    [address((ee)0x00665587, (aoc)0x0067CDD9), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 54 "CB Save Civ Enter Pressed", 121 "CB Accept Save Civilization" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008352B8, (aoc)0x0084B38C)]
unsafe struct EEUIECBAcceptSaveCivilization
{
    [address((ee)0x00665626, (aoc)0x0067CE78), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 62 "CB Cancel New Civ Dialog" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008353BC, (aoc)0x0084B490)]
unsafe struct EEUIECBCancelChangeName
{
    [address((ee)0x0066561E, (aoc)0x0067CE70), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 61 "CB Cancel changes" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008352F4, (aoc)0x0084B3C8)]
unsafe struct EEUIECBCancelChanges
{
    [address((ee)0x006656E6, (aoc)0x0067CF3D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 66 "CB Cancel Load Civ Dialog" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835394, (aoc)0x0084B468)]
unsafe struct EEUIECBCancelLoadCivDialog
{
    [address((ee)0x0066561E, (aoc)0x0067CE70), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 64 "CB Cancel Save Civ Dialog" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083536C, (aoc)0x0084B440)]
unsafe struct EEUIECBCancelSaveCivDialog
{
    [address((ee)0x0066561E, (aoc)0x0067CE70), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x668967
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835ED8, (aoc)0x0084BFA8)]
unsafe struct EEUIECBCleanUp
{
    [address((ee)0x00668C0C, (aoc)0x00680E8E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 105 "CB Display Delete Civ Dialog (in load)", 118 "CB Display Delete Civ Dialog (in save)" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083527C, (aoc)0x0084B350)]
unsafe struct EEUIECBDisplayDeleteCivDialog
{
    [address((ee)0x00665734, (aoc)0x0067CF8B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 56 "CB Potential upgrade selected" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835344, (aoc)0x0084B418)]
unsafe struct EEUIECBPotentialUpgradeSelected
{
    [address((ee)0x006656B3, (aoc)0x0067CF05), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 55 "CB Purchased upgrade selected" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835358, (aoc)0x0084B42C)]
unsafe struct EEUIECBPurchasedUpgradeSelected
{
    [address((ee)0x00665695, (aoc)0x0067CEE7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 52 "CB Save Game Name Selected" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008353D0, (aoc)0x0084B4A4)]
unsafe struct EEUIECBSaveNameSelected
{
    [address((ee)0x0066568D, (aoc)0x0067CEDF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 123 "CB Show Change Name Dialog" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008352CC, (aoc)0x0084B3A0)]
unsafe struct EEUIECBShowChangeNameDialog
{
    [address((ee)0x00665510, (aoc)0x0067CD62), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 65 "CB Show Load Civ Dialog" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008353A8, (aoc)0x0084B47C)]
unsafe struct EEUIECBShowLoadCivDialog
{
    [address((ee)0x0066557F, (aoc)0x0067CDD1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x668967
[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x00835EEC, (aoc)0x0084BFBC)]
unsafe struct EEUIECBShowOrHideButtons
{
    [address((ee)0x00668C29, (aoc)0x00680EAB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 63 "CB Show Save Civ Dialog" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835380, (aoc)0x0084B454)]
unsafe struct EEUIECBShowSaveCivDialog
{
    [address((ee)0x00665616, (aoc)0x0067CE68), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 59 "CB Update controls" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835308, (aoc)0x0084B3DC)]
unsafe struct EEUIECBUpdateControls
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 57 "CB Upgrade purchased" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835330, (aoc)0x0084B404)]
unsafe struct EEUIECBUpgradePurchaseAttempt
{
    [address((ee)0x006656C0, (aoc)0x0067CF12), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 58 "CB Upgrade returned" (dbuicontrolevents.dat), made by the event creator 0x65AC0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083531C, (aoc)0x0084B3F0)]
unsafe struct EEUIECBUpgradeReturned
{
    [address((ee)0x006656D4, (aoc)0x0067CF26), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C335A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008309B8, (aoc)0x008462AC)]
unsafe struct EEUIECCCECancel
{
    [address((ee)0x005C225C, (aoc)0x005D2AA1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D1EF6
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083100C, (aoc)0x00846908)]
unsafe struct EEUIECCCEFormClose
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D1EF6
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831020, (aoc)0x0084691C)]
unsafe struct EEUIECCCEFormOpen
{
    [address((ee)0x005C335A, (aoc)0x005D3A77), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C335A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008309CC, (aoc)0x008462C0)]
unsafe struct EEUIECCCEOk
{
    [address((ee)0x005C388E, (aoc)0x005D403A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C335A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008309E0, (aoc)0x008462D4)]
unsafe struct EEUIECCCESelectCampaign
{
    [address((ee)0x005C36B4, (aoc)0x005D3E60), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 190 "CE: Add Scenario" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x00835268, (aoc)0x0084B33C)]
unsafe struct EEUIECEAddScenario
{
    [address((ee)0x005BF93B, (aoc)0x005D0095), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 191 "CE: Delete Campaign" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x00835254, (aoc)0x0084B328)]
unsafe struct EEUIECECampaignDelete
{
    [address((ee)0x005BFF2A, (aoc)0x005D0684), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 192 "CE: Campaign Name Changed" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x00835240, (aoc)0x0084B314)]
unsafe struct EEUIECECampaignNameChanged
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 202 "CE: Campaign New" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x0083518C, (aoc)0x0084B260)]
unsafe struct EEUIECECampaignNew
{
    [address((ee)0x005BFFC8, (aoc)0x005D0722), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 193 "CE: Campaign Save" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x0083522C, (aoc)0x0084B300)]
unsafe struct EEUIECECampaignSave
{
    [address((ee)0x005BFAC3, (aoc)0x005D021D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5BFAD2
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00830890, (aoc)0x00846184)]
unsafe struct EEUIECECampaignSaveContinued
{
    [address((ee)0x005BFB4E, (aoc)0x005D02A8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 194 "CE: Campaign Selected" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x00835218, (aoc)0x0084B2EC)]
unsafe struct EEUIECECampaignSelected
{
    [address((ee)0x005BFBF0, (aoc)0x005D034A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form events 5 "Chat MLB Pressed", 6 "Chat MRB Pressed" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835010, (aoc)0x0084B0BC)]
unsafe struct EEUIECECancel
{
    [address((ee)0x005C29BF, (aoc)0x005D31DF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C2496
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0083094C, (aoc)0x00846240)]
unsafe struct EEUIECEChangeSendMessageType
{
    [address((ee)0x005C23B7, (aoc)0x005D2B02), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C2496
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00830960, (aoc)0x00846254)]
unsafe struct EEUIECEChangeUserChatType
{
    [address((ee)0x005C23EB, (aoc)0x005D2B63), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C2496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830938, (aoc)0x0084622C)]
unsafe struct EEUIECEChatKeyboardFocus
{
    [address((ee)0x005C27F7, (aoc)0x005D2F9A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F34, (aoc)0x0084AFE0)]
unsafe struct EEUIECEClose
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 4 "Chat Form Close" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835024, (aoc)0x0084B0D0)]
unsafe struct EEUIECECloseChat
{
    [address((ee)0x005C2487, (aoc)0x005D2BFF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C2496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830974, (aoc)0x00846268)]
unsafe struct EEUIECEEscape
{
    [address((ee)0x005C2867, (aoc)0x005D300A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 198 "CE: Main Menu" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x008351C8, (aoc)0x0084B29C)]
unsafe struct EEUIECEMainScreen
{
    [address((ee)0x005C005B, (aoc)0x005D07B5), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 196 "CE: Move Scenario Down" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x008351F0, (aoc)0x0084B2C4)]
unsafe struct EEUIECEMoveScenarioDown
{
    [address((ee)0x005BFA61, (aoc)0x005D01BB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 195 "CE: Move Scenario Up" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x00835204, (aoc)0x0084B2D8)]
unsafe struct EEUIECEMoveScenarioUp
{
    [address((ee)0x005BF9FD, (aoc)0x005D0157), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F48, (aoc)0x0084AFF4)]
unsafe struct EEUIECEOpen
{
    [address((ee)0x005BF7BF, (aoc)0x005CFF19), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 3 "Chat Form Open" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835038, (aoc)0x0084B0E4)]
unsafe struct EEUIECEOpenChat
{
    [address((ee)0x005C2496, (aoc)0x005D2C0E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 200 "CE: Password entered", 201 "CE: Password OK button hit" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x008351A0, (aoc)0x0084B274)]
unsafe struct EEUIECEPasswordEntered
{
    [address((ee)0x005BFED9, (aoc)0x005D0633), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 197 "CE: Remove Scenario" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x008351DC, (aoc)0x0084B2B0)]
unsafe struct EEUIECERemoveScenario
{
    [address((ee)0x005BF99C, (aoc)0x005D00F6), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 199 "CE: Start Scenario Editor" (dbuicontrolevents.dat), made by the event creator 0x65A9E6
[@base(typeof(UIEvent)), vtable((ee)0x008351B4, (aoc)0x0084B288)]
unsafe struct EEUIECEScenarioEditorOpen
{
    [address((ee)0x005C0103, (aoc)0x005D085D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C2496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830988, (aoc)0x0084627C)]
unsafe struct EEUIECESendMessage
{
    [address((ee)0x005C28D6, (aoc)0x005D3079), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 213 "Chat Log Close" (dbuicontrolevents.dat), made by the event creator 0x65ABE0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008309A4, (aoc)0x00846298)]
unsafe struct EEUIECLECloseChatLog
{
    [address((ee)0x005C30DB, (aoc)0x005D3826), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F0C, (aoc)0x0084AFB8)]
unsafe struct EEUIECLEMousePressed
{
    [address((ee)0x005C30F6, (aoc)0x005D3841), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F20, (aoc)0x0084AFCC)]
unsafe struct EEUIECLEOpenChatLog
{
    [address((ee)0x005C2E1A, (aoc)0x005D3565), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C100C
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008308B0, (aoc)0x008461A4)]
unsafe struct EEUIECSSEBack
{
    [address((ee)0x005C225C, (aoc)0x005D2AA1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C388E, 0x5D1AAB, 0x5D3143
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008309F4, (aoc)0x008462E8)]
unsafe struct EEUIECSSEFormClose
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C388E, 0x5D1AAB, 0x5D3143
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830A08, (aoc)0x008462FC)]
unsafe struct EEUIECSSEFormOpen
{
    [address((ee)0x005C100C, (aoc)0x005D1766), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C100C
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830900, (aoc)0x008461F4)]
unsafe struct EEUIECSSESelectCampaign
{
    [address((ee)0x005C1BF1, (aoc)0x005D2445), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C100C
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008308D8, (aoc)0x008461CC)]
unsafe struct EEUIECSSESelectDifficultyLevel
{
    [address((ee)0x005C1E9B, (aoc)0x005D26EF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C100C
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008308EC, (aoc)0x008461E0)]
unsafe struct EEUIECSSESelectScenario
{
    [address((ee)0x005C1C78, (aoc)0x005D24CC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C100C
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008308C4, (aoc)0x008461B8)]
unsafe struct EEUIECSSEStartScenario
{
    [address((ee)0x005C1F05, (aoc)0x005D274A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 15 "Cancel Load" (dbuicontrolevents.dat), made by the event creator 0x65A46D
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00835074, (aoc)0x0084B120)]
unsafe struct EEUIECancel
{
    [address((ee)0x0065CC90, (aoc)0x006724AF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830AA4, (aoc)0x00846398)]
unsafe struct EEUIEDSAlliedCheckBox
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830A40, (aoc)0x00846334)]
unsafe struct EEUIEDSCancel
{
    [address((ee)0x005C5DFA, (aoc)0x005D674A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830A54, (aoc)0x00846348)]
unsafe struct EEUIEDSClearTribute
{
    [address((ee)0x005C5CA8, (aoc)0x005D65F8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830AB8, (aoc)0x008463AC)]
unsafe struct EEUIEDSCustomChatCheckBox
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830A68, (aoc)0x0084635C)]
unsafe struct EEUIEDSOKButton
{
    [address((ee)0x005C5995, (aoc)0x005D622F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F5C, (aoc)0x0084B008)]
unsafe struct EEUIEDSOpen
{
    /// the diplomacy screen (form 27) filled when it opens: from control 19 on a row of 11 per other player (by index,
    /// the local one left out; 15 rows) - its colour, the chat check box, its name, its civilization, its stance
    /// towards you (a UITextLabel: string 0x7349 allied, 0x734A enemy), your Allied check box, five tribute boxes;
    /// a row without a player, or one out of the game, hidden
    [address((ee)0x005C3A3A, (aoc)0x005D4165), overrides(typeof(UIEvent))]
    [vtable_site("diplomacy_open")]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830A90, (aoc)0x00846384)]
unsafe struct EEUIEDSTributeLClick
{
    [address((ee)0x005C5737, (aoc)0x005D5FD1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C3A3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830A7C, (aoc)0x00846370)]
unsafe struct EEUIEDSTributeRClick
{
    [address((ee)0x005C5879, (aoc)0x005D6113), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1, 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F764, (aoc)0x00845030)]
unsafe struct EEUIEDisableMouseMove
{
    [address((ee)0x005D5DA5, (aoc)0x005E6711), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 240 "EE Editor Sub Menu: Civ Builder Button" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008354C0, (aoc)0x0084B594)]
unsafe struct EEUIEEEECivBuilder
{
    [address((ee)0x005C6105, (aoc)0x005D6845), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 189 "EE Editor Sub Menu: Start Campaign Editor" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008354AC, (aoc)0x0084B580)]
unsafe struct EEUIEEEEEditor
{
    [address((ee)0x005C60AD, (aoc)0x005D67D8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 188 "EE Editor Sub Menu: Main Menu" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835498, (aoc)0x0084B56C)]
unsafe struct EEUIEEEEMainMenu
{
    [address((ee)0x005D88C0, (aoc)0x005D6811), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 29, 187 "EE Editor Sub Menu: Start Scenario Editor" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008354FC, (aoc)0x0084B5D0)]
unsafe struct EEUIEEEEScenarioEditor
{
    [address((ee)0x005C60E6, (aoc)0x005D6826), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 186 "Main Screen: Open EE Editor Sub Menu Screen" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835484, (aoc)0x0084B558)]
unsafe struct EEUIEEEESubMenuOpen
{
    [address((ee)0x005C6074, (aoc)0x005D679F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x590734
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F900, (aoc)0x008451D0)]
unsafe struct EEUIEElevationTool
{
    [address((ee)0x00590605, (aoc)0x005A085F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1, 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F750, (aoc)0x0084501C)]
unsafe struct EEUIEEnableMouseMove
{
    [address((ee)0x005D5DBF, (aoc)0x005E672B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 241 "SRM Set Formation Short Line", 242 "SRM Set Formation Long Line", 243 "SRM Set Formation Loose Line", 244 "SRM Set Formation Staggered Line", 245 "SRM Set Formation Deep Wedge", 246 "SRM Set Formation Shallow Wedge", 247 "SRM Set Formation Inward Circle", 248 "SRM Set Formation Outward Circle" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x008353E4, (aoc)0x0084B4B8)]
unsafe struct EEUIEFormationChange
{
    [address((ee)0x005C6146, (aoc)0x005D6886), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 220 "Game Settings Model Detail Radio High", 221 "Game Settings Model Detail Radio Med", 222 "Game Settings Model Detail Radio Low", 223 "Game Settings Effects Detail High", 224 "Game Settings Effects Detail Med", 225 "Game Settings Effects Detail Low", 226 "Game Settings Hi-Res Filter Checkbox Check", 227 "Game Settings Hi-Res Filter Checkbox Uncheck", 228 "Game Settings Hi-Res Lighting Checkbox Check", 229 "Game Settings Hi-Res Lighting Checkbox Uncheck", 230 "Game Settings Hi-Res Textures Checkbox Check", 231 "Game Settings Hi-Res Textures Checkbox Uncheck", 232 "Game Settings Smooth Animations Checkbox Check", 233 "Game Settings Smooth Animations Checkbox Uncheck", 234 "Game Settings Shadows Checkbox Check", 235 "Game Settings Shadows Checkbox Uncheck", 236 "Game Settings Clouds Checkbox Check", 237 "Game Settings Clouds Checkbox Uncheck", 238 "Game Settings Weather Effects Checkbox Check", 239 "Game Settings Weather Effects Checkbox Uncheck", 250 "Game Settings AntiAlias Checkbox Check", 251 "Game Settings AntiAlias Checkbox Uncheck" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E1C, (aoc)0x0084AEC8)]
unsafe struct EEUIEGSEAdvancedGraphicsSettingsChanged
{
    [address((ee)0x005C715F, (aoc)0x005D791D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 49 "Game Settings Audio Tab" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834EBC, (aoc)0x0084AF68)]
unsafe struct EEUIEGSEAudioTab
{
    [address((ee)0x005C6482, (aoc)0x005D6BC0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 46 "Game Settings Cancel" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E94, (aoc)0x0084AF40)]
unsafe struct EEUIEGSECancel
{
    [address((ee)0x005C64D1, (aoc)0x005D6C0F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 214 "Game Settings GQ radio 1 Lowest", 215 "Game Settings GQ radio 2", 216 "Game Settings GQ radio 3 Middle", 217 "Game Settings GQ radio 4", 218 "Game Settings GQ radio 5 Highest" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E30, (aoc)0x0084AEDC)]
unsafe struct EEUIEGSEGraphicsQuality
{
    [address((ee)0x005C70DE, (aoc)0x005D789C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C66F6
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830B4C, (aoc)0x00846448)]
unsafe struct EEUIEGSEInGameOk
{
    [address((ee)0x005C658C, (aoc)0x005D6CCA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0A97
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830EA4, (aoc)0x008467A0)]
unsafe struct EEUIEGSEInGameOpen
{
    [address((ee)0x005C66F6, (aoc)0x005D6E34), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 50 "Game Settings Interface Tab" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834ED0, (aoc)0x0084AF7C)]
unsafe struct EEUIEGSEInterfaceTab
{
    [address((ee)0x005C6813, (aoc)0x005D6F7C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 76 "Game Settings Music Quality High", 77 "Game Settings Music Quality Low", 78 "Game Settings Music Off" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E58, (aoc)0x0084AF04)]
unsafe struct EEUIEGSEMusicQualityRadio
{
    [address((ee)0x005C6862, (aoc)0x005D6FCB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 74 "Game Settings Music Volume" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E80, (aoc)0x0084AF2C)]
unsafe struct EEUIEGSEMusicVolumeSlider
{
    [address((ee)0x005C68B7, (aoc)0x005D7020), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C6943
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830B60, (aoc)0x0084645C)]
unsafe struct EEUIEGSEOutOfGameOk
{
    [address((ee)0x005C6919, (aoc)0x005D7082), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D1567
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830FBC, (aoc)0x008468B8)]
unsafe struct EEUIEGSEOutOfGameOpen
{
    [address((ee)0x005C6943, (aoc)0x005D70AC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 73 "Game Settings Rendering System" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834EF8, (aoc)0x0084AFA4)]
unsafe struct EEUIEGSERenderingSystemListBox
{
    [address((ee)0x005C69B0, (aoc)0x005D7144), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 47 "Game Settings Restore Default" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834EA8, (aoc)0x0084AF54)]
unsafe struct EEUIEGSERestoreDefault
{
    [address((ee)0x005C6ECB, (aoc)0x005D7689), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 75 "Game Settings Sound Volume" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E44, (aoc)0x0084AEF0)]
unsafe struct EEUIEGSESoundVolumeSlider
{
    [address((ee)0x005C6F19, (aoc)0x005D76D7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 84 "Game Settings Test Sound Volume" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834E6C, (aoc)0x0084AF18)]
unsafe struct EEUIEGSETestSoundVolume
{
    [address((ee)0x005C6F4B, (aoc)0x005D7709), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 72 "Game Settings Video Card Combo Box" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834EE4, (aoc)0x0084AF90)]
unsafe struct EEUIEGSEVideoCardComboBox
{
    [address((ee)0x005C6FBD, (aoc)0x005D777B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 48 "Game Settings Video Tab" (dbuicontrolevents.dat), made by the event creator 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830B74, (aoc)0x00846470)]
unsafe struct EEUIEGSEVideoTab
{
    [address((ee)0x005C7083, (aoc)0x005D7841), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x0082F898, (aoc)0x008450D8)]
unsafe struct EEUIEGrabScroll
{
    [address((ee)0x005C8F00, (aoc)0x005D99FF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00834960, (aoc)0x0084A9FC)]
unsafe struct EEUIEHKAddSelectionToGroup
{
    [address((ee)0x005D8A13, (aoc)0x005E93DF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0083494C, (aoc)0x0084A9E8)]
unsafe struct EEUIEHKCreateGroup
{
    [address((ee)0x005D8A72, (aoc)0x005E946B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834820, (aoc)0x0084A8D0)]
unsafe struct EEUIEHKPlayerEvent
{
    [address((ee)0x005D8AF8, (aoc)0x005E951E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D8B36
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008311F0, (aoc)0x00846AF0)]
unsafe struct EEUIEHKSelectGroup
{
    [address((ee)0x005D8B8B, (aoc)0x005E95B1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 138 "SRM Idle Citizen", 139 "SRM Idle Military" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x00834870, (aoc)0x00842B28)]
unsafe struct EEUIEHKUnitSelection
{
    [address((ee)0x005D8BF0, (aoc)0x005E964A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1, 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082F778, (aoc)0x00845044)]
unsafe struct EEUIEHotKeyZoom
{
    [address((ee)0x005C913C, (aoc)0x005D9C3B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x402BB8
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00824C58, (aoc)0x00838C2C)]
unsafe struct EEUIELEActivateMultiPlayerClient
{
    [address((ee)0x005CEB65, (aoc)0x005DF874), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x402BB8
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00824C44, (aoc)0x00838C18)]
unsafe struct EEUIELEActivateMultiPlayerServer
{
    [address((ee)0x005CE4BD, (aoc)0x005DF1CC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D817A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008311B8, (aoc)0x00846AB4)]
unsafe struct EEUIELEActivateSinglePlayer
{
    [address((ee)0x005CECD9, (aoc)0x005DF9E8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CE4BD, 0x5CEB65, 0x5CECD9
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830E10, (aoc)0x0084670C)]
unsafe struct EEUIELECancel
{
    [address((ee)0x005C9A0B, (aoc)0x005DA4DB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C9383, 0x5C9685
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00830BAC, (aoc)0x008464A8)]
unsafe struct EEUIELECancelLoad
{
    [address((ee)0x005C92D0, (aoc)0x005D9DCF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CAFBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830C6C, (aoc)0x00846568)]
unsafe struct EEUIELEChangeCitizenBonus
{
    [address((ee)0x005C992B, (aoc)0x005DA42A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CAD73, 0x5CAFBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830C58, (aoc)0x00846554)]
unsafe struct EEUIELEChangePlayerType
{
    [address((ee)0x005CA51D, (aoc)0x005DAFB8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CAD73
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830C08, (aoc)0x00846504)]
unsafe struct EEUIELEChangeReady
{
    [address((ee)0x005CA8BD, (aoc)0x005DB358), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CAD73, 0x5CCABC
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830C1C, (aoc)0x00846518)]
unsafe struct EEUIELEChangeTeam
{
    [address((ee)0x005CA44A, (aoc)0x005DAF10), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830C80, (aoc)0x0084657C)]
unsafe struct EEUIELEChangeVictory
{
    [address((ee)0x005CAC08, (aoc)0x005DB63C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x20, (aoc)0x20), vtable((ee)0x00830E24, (aoc)0x00846720)]
unsafe struct EEUIELEChat
{
    [address((ee)0x005C9AD2, (aoc)0x005DA5A2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CE4BD, 0x5CEB65
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830DE8, (aoc)0x008466E4)]
unsafe struct EEUIELEChatKeyboardFocus
{
    [address((ee)0x005CE67F, (aoc)0x005DF38E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CAD73
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00830C30, (aoc)0x0084652C)]
unsafe struct EEUIELEDecChangeColor
{
    [address((ee)0x005CA33E, (aoc)0x005DAE32), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D70, (aoc)0x0084666C)]
unsafe struct EEUIELEEpochEnd
{
    [address((ee)0x005CF8E8, (aoc)0x005E05BE), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830C94, (aoc)0x00846590)]
unsafe struct EEUIELEGameCheatCodes
{
    [address((ee)0x005CF0E9, (aoc)0x005DFDF8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D20, (aoc)0x0084661C)]
unsafe struct EEUIELEGameDifficulty
{
    [address((ee)0x005CF5D6, (aoc)0x005E02E5), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D84, (aoc)0x00846680)]
unsafe struct EEUIELEGameEpochStart
{
    [address((ee)0x005CF994, (aoc)0x005E066A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830CA8, (aoc)0x008465A4)]
unsafe struct EEUIELEGameLockSpeed
{
    [address((ee)0x005CF199, (aoc)0x005DFEA8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830CBC, (aoc)0x008465B8)]
unsafe struct EEUIELEGameLockTeams
{
    [address((ee)0x005CF039, (aoc)0x005DFD48), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830DC0, (aoc)0x008466BC)]
unsafe struct EEUIELEGameMapSize
{
    [address((ee)0x005CF249, (aoc)0x005DFF58), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830DAC, (aoc)0x008466A8)]
unsafe struct EEUIELEGameMapType
{
    [address((ee)0x005CFB5B, (aoc)0x005E0831), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830CF8, (aoc)0x008465F4)]
unsafe struct EEUIELEGameRecordGames
{
    [address((ee)0x005CEF89, (aoc)0x005DFC98), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D98, (aoc)0x00846694)]
unsafe struct EEUIELEGameResources
{
    [address((ee)0x005CFAAD, (aoc)0x005E0783), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830CE4, (aoc)0x008465E0)]
unsafe struct EEUIELEGameRevealMap
{
    [address((ee)0x005CEE29, (aoc)0x005DFB38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D0C, (aoc)0x00846608)]
unsafe struct EEUIELEGameSpeed
{
    [address((ee)0x005CF53A, (aoc)0x005E0249), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CB0A4
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830CD0, (aoc)0x008465CC)]
unsafe struct EEUIELEGameTeamIslands
{
    [address((ee)0x005CEED9, (aoc)0x005DFBE8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830DD4, (aoc)0x008466D0)]
unsafe struct EEUIELEGameType
{
    [address((ee)0x005CF2E5, (aoc)0x005DFFF4), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D5C, (aoc)0x00846658)]
unsafe struct EEUIELEGameUnitLimit
{
    [address((ee)0x005CF7DB, (aoc)0x005E04EA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D34, (aoc)0x00846630)]
unsafe struct EEUIELEGameVariant
{
    [address((ee)0x005CF682, (aoc)0x005E0391), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830D48, (aoc)0x00846644)]
unsafe struct EEUIELEGameWondersForVictory
{
    [address((ee)0x005CF73F, (aoc)0x005E044E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CAD73
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00830C44, (aoc)0x00846540)]
unsafe struct EEUIELEIncChangeColor
{
    [address((ee)0x005CA237, (aoc)0x005DAD59), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C9383
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830B98, (aoc)0x00846494)]
unsafe struct EEUIELELoadSavedGame
{
    [address((ee)0x005C931A, (aoc)0x005D9E19), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5C9685
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830BC0, (aoc)0x008464BC)]
unsafe struct EEUIELELoadScenarioGame
{
    [address((ee)0x005C961C, (aoc)0x005DA11B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CF2E5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830E38, (aoc)0x00846734)]
unsafe struct EEUIELEOpenLoadSavedGame
{
    [address((ee)0x005C9383, (aoc)0x005D9E82), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CF2E5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830E4C, (aoc)0x00846748)]
unsafe struct EEUIELEOpenLoadScenarioGame
{
    [address((ee)0x005C9685, (aoc)0x005DA184), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5CE4BD, 0x5CECD9
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830DFC, (aoc)0x008466F8)]
unsafe struct EEUIELEStart
{
    [address((ee)0x005C9B91, (aoc)0x005DA6B3), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F6EC, (aoc)0x00844FB8)]
unsafe struct EEUIELoadScenarioTriggersOnly
{
    [address((ee)0x0058D34E, (aoc)0x0059D582), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 27 "SRM Temp Box Load Unit" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835470, (aoc)0x0084B544)]
unsafe struct EEUIELoadUnit
{
    [address((ee)0x0065CAF9, (aoc)0x00673345), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x00834834, (aoc)0x0084A8E4)]
unsafe struct EEUIEMGAttackPlayer
{
    [address((ee)0x0064A0B9, (aoc)0x006606B2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64F409
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008349C8, (aoc)0x0084AA64)]
unsafe struct EEUIEMGCancelUIMode
{
    [address((ee)0x00649862, (aoc)0x0065FDB3), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834898, (aoc)0x0084A934)]
unsafe struct EEUIEMGCancelUpdateState
{
    [address((ee)0x00649FEE, (aoc)0x006605BA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x008348AC, (aoc)0x0084A948)]
unsafe struct EEUIEMGChangeGameSpeed
{
    [address((ee)0x0064A2A3, (aoc)0x006608F4), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 25 "SRM Main Game Chat" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834884, (aoc)0x0084A920)]
unsafe struct EEUIEMGDisplayChat
{
    [address((ee)0x00649A13, (aoc)0x0065FF64), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 24 "SRM Main Game Civ Builder", 211 "SRM Large Civ Builder Alert" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083540C, (aoc)0x0084B4E0)]
unsafe struct EEUIEMGDisplayCivBuilder
{
    [address((ee)0x00649A05, (aoc)0x0065FF56), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 23 "SRM Main Game Diplomacy" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083545C, (aoc)0x0084B530)]
unsafe struct EEUIEMGDisplayDiplomacy
{
    [address((ee)0x00649A41, (aoc)0x0065FF92), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 26 "SRM Main Game Options" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008347D0, (aoc)0x0084A880)]
unsafe struct EEUIEMGDisplayOptions
{
    [address((ee)0x00649A6F, (aoc)0x0065FFC0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 122 "SRM Game Info (Scenario Introduction)", 212 "SRM Large Scen Briefing Alert" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008353F8, (aoc)0x0084B4CC)]
unsafe struct EEUIEMGDisplayScenarioIntro
{
    [address((ee)0x00649AD5, (aoc)0x005E136E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 22 "SRM Main Game Statistics" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835448, (aoc)0x0084B51C)]
unsafe struct EEUIEMGDisplayStatistics
{
    [address((ee)0x006499E0, (aoc)0x0065FF31), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1, 0x5D16D3
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0082F6C4, (aoc)0x00844F90)]
unsafe struct EEUIEMGDumpLoadedMedia
{
    [address((ee)0x006498FC, (aoc)0x0065FE4D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 143 "SRM Chat Dialog Finished" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835434, (aoc)0x0084B508)]
unsafe struct EEUIEMGEndScenarioDialog
{
    [address((ee)0x006499D2, (aoc)0x0065FF23), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083480C, (aoc)0x0084A8BC)]
unsafe struct EEUIEMGEnterScenarioEditor
{
    [address((ee)0x00649C49, (aoc)0x00660215), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008348D4, (aoc)0x0084A970)]
unsafe struct EEUIEMGEnterShiftMode
{
    [address((ee)0x00649ADF, (aoc)0x00660051), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008348E8, (aoc)0x0084A984)]
unsafe struct EEUIEMGExitShiftMode
{
    [address((ee)0x00649B18, (aoc)0x006600B7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 204 "SRM Flare" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834848, (aoc)0x0084A8F8)]
unsafe struct EEUIEMGFlareMode
{
    [address((ee)0x00649B51, (aoc)0x006600F0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0083485C, (aoc)0x0084A90C)]
unsafe struct EEUIEMGFollowUnit
{
    [address((ee)0x0064A1B3, (aoc)0x006607E0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x651A19
[@base(typeof(UIEvent)), size((ee)0x14, (aoc)0x14), vtable((ee)0x00834C00, (aoc)0x0084ACA0)]
unsafe struct EEUIEMGHotKeyStatusButton
{
    [address((ee)0x00649C0E, (aoc)0x006601DA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x008347E4, (aoc)0x0084A894)]
unsafe struct EEUIEMGInGameLoadAndSave
{
    [address((ee)0x00649D51, (aoc)0x0066031D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 140 "SRM Fighter Rally Timer", 141 "SRM Bomber Rally Timer", 142 "SRM Atomic Rally Timer" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00835420, (aoc)0x0084B4F4)]
unsafe struct EEUIEMGIncrementRallyTimer
{
    [address((ee)0x00649B8E, (aoc)0x0066012D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x008348C0, (aoc)0x0084A95C)]
unsafe struct EEUIEMGKillUnits
{
    [address((ee)0x0064A084, (aoc)0x00660650), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830FA8, (aoc)0x008468A4)]
unsafe struct EEUIEMGOECloseOptions
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830EB8, (aoc)0x008467B4)]
unsafe struct EEUIEMGOEExitGame
{
    [address((ee)0x005D0AFB, (aoc)0x005E1407), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F44, (aoc)0x00846840)]
unsafe struct EEUIEMGOEGameBriefing
{
    [address((ee)0x00649AD5, (aoc)0x005E136E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F30, (aoc)0x0084682C)]
unsafe struct EEUIEMGOEGameSettings
{
    [address((ee)0x005D0A97, (aoc)0x005E1378), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F08, (aoc)0x00846804)]
unsafe struct EEUIEMGOELoadSavedGame
{
    [address((ee)0x005D0715, (aoc)0x005E0FEC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00830EF4, (aoc)0x008467F0)]
unsafe struct EEUIEMGOELoadScenario
{
    [address((ee)0x005D0937, (aoc)0x005E120E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x649A6F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834578, (aoc)0x0084A624)]
unsafe struct EEUIEMGOEOpenOptionsScreen
{
    [address((ee)0x005D0BBF, (aoc)0x005E14CB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F6C, (aoc)0x00846868)]
unsafe struct EEUIEMGOEPauseGame
{
    [address((ee)0x005D0890, (aoc)0x005E1167), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F58, (aoc)0x00846854)]
unsafe struct EEUIEMGOEPreviousMessages
{
    [address((ee)0x005D0918, (aoc)0x005E11EF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F80, (aoc)0x0084687C)]
unsafe struct EEUIEMGOEResign
{
    [address((ee)0x005D0578, (aoc)0x005E0E2B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830EE0, (aoc)0x008467DC)]
unsafe struct EEUIEMGOERestartGame
{
    [address((ee)0x005D0602, (aoc)0x005E0ED9), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830ECC, (aoc)0x008467C8)]
unsafe struct EEUIEMGOEReturnToEditor
{
    [address((ee)0x005D0661, (aoc)0x005E0F38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F94, (aoc)0x00846890)]
unsafe struct EEUIEMGOEReturnToGame
{
    [address((ee)0x005D0568, (aoc)0x005E0E1B), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D0BBF
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830F1C, (aoc)0x00846818)]
unsafe struct EEUIEMGOESaveGame
{
    [address((ee)0x005D07B3, (aoc)0x005E108A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x4C3B92, 0x64C8B0, 0x659A3B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008297D0, (aoc)0x0083E43C)]
unsafe struct EEUIEMGPauseGame
{
    [address((ee)0x00649F9A, (aoc)0x00660566), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 249 "SRM Display Chat Log" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834910, (aoc)0x0084A9AC)]
unsafe struct EEUIEMGShowPreviousMessages
{
    [address((ee)0x0064A58D, (aoc)0x00660C08), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008347BC, (aoc)0x0084A86C)]
unsafe struct EEUIEMGToggleGameClock
{
    [address((ee)0x0064A050, (aoc)0x0066061C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008347F8, (aoc)0x0084A8A8)]
unsafe struct EEUIEMGTogglePerspective
{
    [address((ee)0x0064A20D, (aoc)0x0066085E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 185 "SRM Hidden Unit Toggle" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F6D8, (aoc)0x00844FA4)]
unsafe struct EEUIEMGToggleShowHiddenUnits
{
    [address((ee)0x0064A102, (aoc)0x0066072F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 205 "SRM Set Scout Behavior", 206 "SRM Set Stand Ground Behavior", 207 "SRM Set Guard Behavior", 209 "SRM Set Aggressive Behavior" (dbuicontrolevents.dat), made by the event creator 0x65AF3A
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00834938, (aoc)0x0084A9D4)]
unsafe struct EEUIEMGUnitBehavior
{
    [address((ee)0x0064A394, (aoc)0x00660A12), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B18, (aoc)none)]
unsafe struct EEUIEMSCloseMultiplayerScreen
{
    [address((ee)0x00410A74, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B40, (aoc)none)]
unsafe struct EEUIEMSCreateAccount
{
    [address((ee)0x004109B3, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 31 "Main Screen Option Exit Game" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835510, (aoc)0x0084B5E4)]
unsafe struct EEUIEMSEExitGame
{
    [address((ee)0x005D15CC, (aoc)0x005E1ED8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 32 "Main Screen Learning Scenarios Button" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835524, (aoc)0x0084B5F8)]
unsafe struct EEUIEMSEGameSettings
{
    [address((ee)0x005D1567, (aoc)0x005E1E48), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 30 "Main Screen Option Multi Player Button" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082C74C, (aoc)0x00841EEC)]
unsafe struct EEUIEMSEMultiPlayer
{
    [address((ee)0x005D1653, (aoc)0x005E1F8A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 1 "Main Screen Open" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083504C, (aoc)0x0084B0F8)]
unsafe struct EEUIEMSEOpen
{
    [address((ee)0x005D16D3, (aoc)0x005E2035), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 28 "Main Screen Option Single Player Button" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008354E8, (aoc)0x0084B5BC)]
unsafe struct EEUIEMSESinglePlayer
{
    [address((ee)0x005D15DC, (aoc)0x005E1EE8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 33 "Main Screen Test Button" (dbuicontrolevents.dat), made by the event creator 0x65B496
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008354D4, (aoc)0x0084B5A8)]
unsafe struct EEUIEMSETestButton
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B2C, (aoc)none)]
unsafe struct EEUIEMSEmailMeMyPassword
{
    [address((ee)0x00410B98, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)none), vtable((ee)0x00825B00, (aoc)none)]
unsafe struct EEUIEMSFormUpdate
{
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B90, (aoc)none)]
unsafe struct EEUIEMSHostGame
{
    [address((ee)0x00410377, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B7C, (aoc)none)]
unsafe struct EEUIEMSJoinByIP
{
    [address((ee)0x00410477, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x4108B1, 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825AD8, (aoc)none)]
unsafe struct EEUIEMSJoinGame
{
    [address((ee)0x0041021A, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825BA4, (aoc)none)]
unsafe struct EEUIEMSMainMenu
{
    [address((ee)0x0041093F, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830FE4, (aoc)0x008468E0)]
unsafe struct EEUIEMSOpenMultiplayerScreen
{
    [address((ee)0x00410C0F, (aoc)0x005E2176), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B68, (aoc)none)]
unsafe struct EEUIEMSPlaySierra
{
    [address((ee)0x004105A0, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x410C0F
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825B54, (aoc)none)]
unsafe struct EEUIEMSShowIP
{
    [address((ee)0x00410952, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x10, (aoc)none), vtable((ee)0x00825BB8, (aoc)none)]
unsafe struct EEUIEMSStartGame
{
    [address((ee)0x0040FF2A, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F7A0, (aoc)0x0084506C)]
unsafe struct EEUIENewPlayer
{
    [address((ee)0x0058CA05, (aoc)0x0059CC39), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58E79A
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F81C, (aoc)0x00845100)]
unsafe struct EEUIEOpenCivBuilder
{
    [address((ee)0x0058EB06, (aoc)0x0059EF15), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 85 "PC Back to Single Player" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008355C4, (aoc)0x0084B60C)]
unsafe struct EEUIEPCEBackToSinglePlayer
{
    [address((ee)0x005D1A73, (aoc)0x005E244A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 86 "PC Campaign I" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008355B0, (aoc)0x0084B620)]
unsafe struct EEUIEPCECampaign1
{
    [address((ee)0x005D1A8D, (aoc)0x005E2464), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 87 "PC Campaign II" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083559C, (aoc)0x0084B634)]
unsafe struct EEUIEPCECampaign2
{
    [address((ee)0x005D1BC2, (aoc)0x005E2599), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 88 "PC Campaign III" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835588, (aoc)0x0084B648)]
unsafe struct EEUIEPCECampaign3
{
    [address((ee)0x005D1BE0, (aoc)0x005E25B7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 89 "PC Campaign IV" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00835574, (aoc)none)]
unsafe struct EEUIEPCECampaign4
{
    [address((ee)0x005D1BFE, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 92 "PC Custom Campaign" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083554C, (aoc)0x0084B670)]
unsafe struct EEUIEPCECustomCampaign
{
    [address((ee)0x005D1EF6, (aoc)0x005E2866), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D80D5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008311A4, (aoc)0x00846AA0)]
unsafe struct EEUIEPCEFormOpen
{
    [address((ee)0x005D1CA0, (aoc)0x005E264F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 93 "PC Learning Campaigns" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835538, (aoc)0x0084B684)]
unsafe struct EEUIEPCELearningCampaign
{
    [address((ee)0x005D1F6A, (aoc)0x005E2930), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00830FF8, (aoc)0x008468F4)]
unsafe struct EEUIEPCEMouseOver
{
    [address((ee)0x005D1C1C, (aoc)0x005E25D5), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x0083110C, (aoc)0x00846A08)]
unsafe struct EEUIEPDCSCancel
{
    [address((ee)0x005D88C0, (aoc)0x005D6811), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x008310D0, (aoc)0x008469CC)]
unsafe struct EEUIEPDCSClearCivStats
{
    [address((ee)0x005D56DA, (aoc)0x005E60A2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x66062B, 0x6677DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008359C0, (aoc)0x0084BA5C)]
unsafe struct EEUIEPDCSFormClose
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x66062B, 0x6677DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008359D4, (aoc)0x0084BA70)]
unsafe struct EEUIEPDCSFormOpen
{
    [address((ee)0x005D54E7, (aoc)0x005E5E2E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x1C, (aoc)0x1C), vtable((ee)0x00831120, (aoc)0x00846A1C)]
unsafe struct EEUIEPDCSLoadCiv
{
    [address((ee)0x005D59DC, (aoc)0x005E63C9), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x008310F8, (aoc)0x008469F4)]
unsafe struct EEUIEPDCSOk
{
    [address((ee)0x005D5B00, (aoc)0x005E64ED), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x008310E4, (aoc)0x008469E0)]
unsafe struct EEUIEPDCSSelectCiv
{
    [address((ee)0x005D571F, (aoc)0x005E610C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F70, (aoc)0x0084B01C)]
unsafe struct EEUIEPGSEClose
{
    [address((ee)0x005D4DD8, (aoc)0x005E5711), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 151 "PGS Exit to Main Menu" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835614, (aoc)0x0084B6D4)]
unsafe struct EEUIEPGSEExit
{
    [address((ee)0x005D3143, (aoc)0x005E3A88), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 184 "PGS Next Graph" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008355D8, (aoc)0x0084B698)]
unsafe struct EEUIEPGSENextGraph
{
    [address((ee)0x005D4EC1, (aoc)0x005E57FA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F84, (aoc)0x0084B030)]
unsafe struct EEUIEPGSEOpen
{
    [address((ee)0x005D342A, (aoc)0x005E3D6F), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 152 "PGS Player 1 Check Box", 153 "PGS Player 2 Check Box", 154 "PGS Player 3 Check Box", 155 "PGS Player 4 Check Box", 156 "PGS Player 5 Check Box", 157 "PGS Player 6 Check Box", 158 "PGS Player 7 Check Box", 159 "PGS Player 8 Check Box", 160 "PGS Player 9 Check Box", 161 "PGS Player 10 Check Box", 162 "PGS Player 11 Check Box", 163 "PGS Player 12 Check Box", 164 "PGS Player 13 Check Box", 165 "PGS Player 14 Check Box", 166 "PGS Player 15 Check Box", 167 "PGS Player 16 Check Box" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00835600, (aoc)0x0084B6C0)]
unsafe struct EEUIEPGSEPlayerChecked
{
    [address((ee)0x005D3350, (aoc)0x005E3C95), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 168 "PGS Player 1 UnCheck Box", 169 "PGS Player 2 UnCheck Box", 170 "PGS Player 3 UnCheck Box", 171 "PGS Player 4 UnCheck Box", 172 "PGS Player 5 UnCheck Box", 173 "PGS Player 6 UnCheck Box", 174 "PGS Player 7 UnCheck Box", 175 "PGS Player 8 UnCheck Box", 176 "PGS Player 9 UnCheck Box", 177 "PGS Player 10 UnCheck Box", 178 "PGS Player 11 UnCheck Box", 179 "PGS Player 12 UnCheck Box", 180 "PGS Player 13 UnCheck Box", 181 "PGS Player 14 UnCheck Box", 182 "PGS Player 15 UnCheck Box", 183 "PGS Player 16 UnCheck Box" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x008355EC, (aoc)0x0084B6AC)]
unsafe struct EEUIEPGSEPlayerUnChecked
{
    [address((ee)0x005D340E, (aoc)0x005E3D53), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 147 "PGS Tab - Economy" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835664, (aoc)0x0084B724)]
unsafe struct EEUIEPGSETabEconomy
{
    [address((ee)0x005D2DE7, (aoc)0x005E372C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 148 "PGS Tab - Epoch" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835650, (aoc)0x0084B710)]
unsafe struct EEUIEPGSETabEpoch
{
    [address((ee)0x005D2EBE, (aoc)0x005E3803), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 146 "PGS Tab - Military" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835678, (aoc)0x0084B738)]
unsafe struct EEUIEPGSETabMilitary
{
    [address((ee)0x005D2D10, (aoc)0x005E3655), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 150 "PGS Tab - Misc" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835628, (aoc)0x0084B6E8)]
unsafe struct EEUIEPGSETabMisc
{
    [address((ee)0x005D306C, (aoc)0x005E39B1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 149 "PGS Tab - Society" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083563C, (aoc)0x0084B6FC)]
unsafe struct EEUIEPGSETabSociety
{
    [address((ee)0x005D2F95, (aoc)0x005E38DA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 144 "PGS Tab - Summary" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008356A0, (aoc)0x0084B760)]
unsafe struct EEUIEPGSETabSummary
{
    [address((ee)0x005D29C0, (aoc)0x005E3305), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 145 "PGS Tab - Timeline" (dbuicontrolevents.dat), made by the event creator 0x65B7DA
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083568C, (aoc)0x0084B74C)]
unsafe struct EEUIEPGSETabTimeLine
{
    [address((ee)0x005D2C27, (aoc)0x005E356C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x4B4F9C
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008291B4, (aoc)0x0083E32C)]
unsafe struct EEUIEProcessTimerExpired
{
    [address((ee)0x004B50B0, (aoc)0x004BD11C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F714, (aoc)0x00844FE0)]
unsafe struct EEUIEReturnToHomeRotation
{
    [address((ee)0x0058CA2A, (aoc)0x0059CC5E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F73C, (aoc)0x00845008)]
unsafe struct EEUIERevolveCamera
{
    [address((ee)0x005D5DD9, (aoc)0x005E6745), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1, 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F728, (aoc)0x00844FF4)]
unsafe struct EEUIERotateCamera
{
    [address((ee)0x005D5E71, (aoc)0x005E67DD), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 103 "SL Load Cancel" (dbuicontrolevents.dat), made by the event creator 0x65BBE6
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083572C, (aoc)0x0084B7EC)]
unsafe struct EEUIESCLECancel
{
    [address((ee)0x005D7F52, (aoc)0x005E88BE), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 101 "SL Load Game", 102 "SL Load Load Game By DbClick" (dbuicontrolevents.dat), made by the event creator 0x65BBE6
[@base(typeof(UIEvent)), size((ee)0x30, (aoc)0x30), vtable((ee)0x00835808, (aoc)0x0084B800)]
unsafe struct EEUIESCLELoadFile
{
    [address((ee)0x005D7D4A, (aoc)0x005E86B6), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834FC0, (aoc)0x0084B06C)]
unsafe struct EEUIESCLEOpen
{
    [address((ee)0x005D7F7A, (aoc)0x005E88E6), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x2C, (aoc)0x2C), vtable((ee)0x00831190, (aoc)0x00846A8C)]
unsafe struct EEUIESCLLoadScreen
{
    [address((ee)0x005D7BCE, (aoc)0x005E853A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 94 "SI Back" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835718, (aoc)0x0084B7D8)]
unsafe struct EEUIESIEBack
{
    [address((ee)0x005D6D55, (aoc)0x005E76C1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 95 "SI Briefing" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831154, (aoc)0x00846A50)]
unsafe struct EEUIESIEBriefing
{
    [address((ee)0x005D6DD5, (aoc)0x005E7741), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 10 "Scenario Intro Close" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834FD4, (aoc)0x0084B080)]
unsafe struct EEUIESIEClose
{
    [address((ee)0x005D6EFE, (aoc)0x005E786A), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 96 "SI Hints" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835704, (aoc)0x0084B7C4)]
unsafe struct EEUIESIEHints
{
    [address((ee)0x005D6F7E, (aoc)0x005E78EA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 97 "SI History" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008356F0, (aoc)0x0084B7B0)]
unsafe struct EEUIESIEHistory
{
    [address((ee)0x005D704E, (aoc)0x005E79BA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 98 "SI Map" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008356DC, (aoc)0x0084B79C)]
unsafe struct EEUIESIEMap
{
    [address((ee)0x005D7129, (aoc)0x005E7A95), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 99 "SI Movie" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083117C, (aoc)0x00846A78)]
unsafe struct EEUIESIEMovie
{
    [address((ee)0x005D717B, (aoc)0x005E7AE7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 7 "Scenario Intro Open" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834FE8, (aoc)0x0084B094)]
unsafe struct EEUIESIEOpen
{
    [address((ee)0x005D73F9, (aoc)0x005E7D65), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D717B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831140, (aoc)0x00846A3C)]
unsafe struct EEUIESIESoundOver
{
    [address((ee)0x005D7226, (aoc)0x005E7B92), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 100 "SI Start" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831168, (aoc)0x00846A64)]
unsafe struct EEUIESIEStart
{
    [address((ee)0x005D77DB, (aoc)0x005E8147), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 20 "Save Game Change Focused File" (dbuicontrolevents.dat), made by the event creator 0x65A46D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835060, (aoc)0x0084B10C)]
unsafe struct EEUIESLDisplayNewFile
{
    [address((ee)0x005D6810, (aoc)0x005E717C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659CDD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834DD0, (aoc)0x0084AE7C)]
unsafe struct EEUIESLEActivateForm
{
    [address((ee)0x005D5F20, (aoc)0x005E688C), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 16 "Cancel Save" (dbuicontrolevents.dat), made by the event creator 0x65A46D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835088, (aoc)0x0084B134)]
unsafe struct EEUIESLECancelSave
{
    [address((ee)0x005C225C, (aoc)0x005D2AA1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 11 "Load Delete", 12 "Save Delete", 44 "SP_Load Delete", 104 "SL Load Delete" (dbuicontrolevents.dat), made by the event creator 0x65BBE6
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x0083509C, (aoc)0x0084B148)]
unsafe struct EEUIESLEDeleteFile
{
    [address((ee)0x005D5F39, (aoc)0x005E68A5), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 13 "Load Game", 17 "Load Game By DbClick" (dbuicontrolevents.dat), made by the event creator 0x65A46D
[@base(typeof(UIEvent)), size((ee)0x30, (aoc)0x30), vtable((ee)0x008357CC, (aoc)0x0084B15C)]
unsafe struct EEUIESLELoadFile
{
    [address((ee)0x005D5F89, (aoc)0x005E68F5), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 14 "Save Game", 18 "Save Game by Enter", 19 "Save Game By DblClick" (dbuicontrolevents.dat), made by the event creator 0x65A46D
[@base(typeof(UIEvent)), size((ee)0x30, (aoc)0x30), vtable((ee)0x008357E0, (aoc)0x0084B170)]
unsafe struct EEUIESLESaveFile
{
    [address((ee)0x005D60CD, (aoc)0x005E6A39), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x2C, (aoc)0x2C), vtable((ee)0x008306CC, (aoc)0x00845FAC)]
unsafe struct EEUIESLLoadScreen
{
    [address((ee)0x005D6427, (aoc)0x005E6D93), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x2C, (aoc)0x2C), vtable((ee)0x008300C8, (aoc)0x008459B4)]
unsafe struct EEUIESLSaveScreen
{
    [address((ee)0x005D663F, (aoc)0x005E6FAB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 35 "Single Player Campaign" (dbuicontrolevents.dat), made by the event creator 0x65BCBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835740, (aoc)0x0084B814)]
unsafe struct EEUIESPECampaign
{
    [address((ee)0x005D80D5, (aoc)0x005E8A41), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 36 "Single Player Civ Campaign" (dbuicontrolevents.dat), made by the event creator 0x65BCBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835754, (aoc)0x0084B828)]
unsafe struct EEUIESPECivCampaign
{
    [address((ee)0x005D812B, (aoc)0x005E8B10), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 37 "Single Player Game Settings" (dbuicontrolevents.dat), made by the event creator 0x65BCBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835768, (aoc)0x0084B83C)]
unsafe struct EEUIESPELearningScenarios
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 38 "Single Player Load Game", 90 "PC Play Saved Game" (dbuicontrolevents.dat), made by the event creator 0x65B684
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835560, (aoc)0x0084B65C)]
unsafe struct EEUIESPELoadGame
{
    [address((ee)0x005D8156, (aoc)0x005E8B44), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 39 "Single Player Main Menu" (dbuicontrolevents.dat), made by the event creator 0x65BCBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083577C, (aoc)0x0084B850)]
unsafe struct EEUIESPEMainMenu
{
    [address((ee)0x005D88C0, (aoc)0x005E8B68), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D15DC
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00830FD0, (aoc)0x008468CC)]
unsafe struct EEUIESPEOpen
{
    [address((ee)0x005D80B8, (aoc)0x005E8A24), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 40 "Single Player Random Map" (dbuicontrolevents.dat), made by the event creator 0x65BCBD
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00835790, (aoc)0x0084B864)]
unsafe struct EEUIESPERandomMap
{
    [address((ee)0x005D817A, (aoc)0x005E8B86), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 43 "SP_Load Cancel" (dbuicontrolevents.dat), made by the event creator 0x65BDB8
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008357A4, (aoc)0x0084B878)]
unsafe struct EEUIESPLECancel
{
    [address((ee)0x005D88C0, (aoc)0x005D6811), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 41 "SP_Load Load Game", 42 "SP_Load Load Game By DbClick" (dbuicontrolevents.dat), made by the event creator 0x65BDB8
[@base(typeof(UIEvent)), size((ee)0x30, (aoc)0x30), vtable((ee)0x008357F4, (aoc)0x0084B88C)]
unsafe struct EEUIESPLELoadFile
{
    [address((ee)0x005D862D, (aoc)0x005E900E), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659F31
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834FAC, (aoc)0x0084B058)]
unsafe struct EEUIESPLEOpen
{
    [address((ee)0x005D88D5, (aoc)0x005E92A1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), size((ee)0x2C, (aoc)0x2C), vtable((ee)0x008311CC, (aoc)0x00846AC8)]
unsafe struct EEUIESPLLoadScreen
{
    [address((ee)0x005D849C, (aoc)0x005E8E7D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 9 "SS Form Open" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834F98, (aoc)0x0084B044)]
unsafe struct EEUIESSEOpen
{
    [address((ee)0x005D12AC, (aoc)0x005E1B8D), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F700, (aoc)0x00844FCC)]
unsafe struct EEUIESaveScenarioTriggersOnly
{
    [address((ee)0x0058CA8C, (aoc)0x0059CCC0), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x64C8B0
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x00834924, (aoc)0x0084A9C0)]
unsafe struct EEUIESelectClosestAircraft
{
    [address((ee)0x005D9090, (aoc)0x005E9BEA), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x59B710
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083020C, (aoc)0x00845B10)]
unsafe struct EEUIETerrainColorChange
{
    [address((ee)0x0059B350, (aoc)0x005AB4BC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x59B710
[@base(typeof(UIEvent)), size((ee)0x10, (aoc)0x10), vtable((ee)0x008302E0, (aoc)0x00845BE4)]
unsafe struct EEUIETerrainTypeChange
{
    [address((ee)0x0059B56B, (aoc)0x005AB6D7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00831258, (aoc)0x00846B58)]
unsafe struct EEUIEWNAGActivateForm
{
    [address((ee)0x005D94FE, (aoc)0x005EA027), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D9D5B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008312E4, (aoc)0x00846BE4)]
unsafe struct EEUIEWNAGBack
{
    [address((ee)0x005D95A8, (aoc)0x005EA0D1), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00831280, (aoc)0x00846B80)]
unsafe struct EEUIEWNAGCheckPassword
{
    [address((ee)0x005D95C6, (aoc)0x005EA0EF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D9D5B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083126C, (aoc)0x00846B6C)]
unsafe struct EEUIEWNAGClickGameFilter
{
    [address((ee)0x005D966A, (aoc)0x005EA193), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00831230, (aoc)0x00846B30)]
unsafe struct EEUIEWNAGCloseForm
{
    [address((ee)0x005D967E, (aoc)0x005EA1A7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x00831244, (aoc)0x00846B44)]
unsafe struct EEUIEWNAGDeactivateForm
{
    [address((ee)0x005D9695, (aoc)0x005EA1BE), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D9D5B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831294, (aoc)0x00846B94)]
unsafe struct EEUIEWNAGDoubleclickUser
{
    [address((ee)0x005D96C2, (aoc)0x005EA1EB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x008312A8, (aoc)0x00846BA8)]
unsafe struct EEUIEWNAGHostOrJoinGame
{
    [address((ee)0x005D97D2, (aoc)0x005EA2FB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x4C8A90
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082986C, (aoc)0x0083E4D0)]
unsafe struct EEUIEWNAGOpenScreen
{
    [address((ee)0x005D9D5B, (aoc)0x005EA884), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x0083121C, (aoc)0x00846B1C)]
unsafe struct EEUIEWNAGSendChat
{
    [address((ee)0x005D9B94, (aoc)0x005EA6BD), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x008312D0, (aoc)0x00846BD0)]
unsafe struct EEUIEWNAGToggleHostScreen
{
    [address((ee)0x005D9C1A, (aoc)0x005EA743), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

[@base(typeof(UIEvent)), vtable((ee)0x008312BC, (aoc)0x00846BBC)]
unsafe struct EEUIEWNAGToggleJoinScreen
{
    [address((ee)0x005D9CDB, (aoc)0x005EA804), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5D94FE
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831208, (aoc)0x00846B08)]
unsafe struct EEUIEWNAGUpdateForm
{
    [address((ee)0x005D94C4, (aoc)0x005E9FED), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA79D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831348, (aoc)0x00846C48)]
unsafe struct EEUIEWNARActivateForm
{
    [address((ee)0x005DA530, (aoc)0x005EB075), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA79D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831384, (aoc)0x00846C84)]
unsafe struct EEUIEWNARBack
{
    [address((ee)0x0041093F, (aoc)0x005EB7E8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA79D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831320, (aoc)0x00846C20)]
unsafe struct EEUIEWNARClickUpdateAccount
{
    [address((ee)0x005DA6A7, (aoc)0x005EB1EC), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA79D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831334, (aoc)0x00846C34)]
unsafe struct EEUIEWNARDeactivateForm
{
    [address((ee)0x005DA612, (aoc)0x005EB157), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA79D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083135C, (aoc)0x00846C5C)]
unsafe struct EEUIEWNARDoubleClickRoom
{
    [address((ee)0x005DA62D, (aoc)0x005EB172), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA79D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00831370, (aoc)0x00846C70)]
unsafe struct EEUIEWNARJoinRoom
{
    [address((ee)0x005DA773, (aoc)0x005EB2B8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x4C65BB
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00829850, (aoc)0x0083E4B4)]
unsafe struct EEUIEWNAROpenScreen
{
    [address((ee)0x005DA79D, (aoc)0x005EB2E2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA530
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008312F8, (aoc)0x00846BF8)]
unsafe struct EEUIEWNARUpdateForm
{
    [address((ee)0x005DA486, (aoc)0x005EAFCB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DAC07
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x008313AC, (aoc)none)]
unsafe struct EEUIEWNCUACancel
{
    [address((ee)0x0041093F, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x4109B3
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00825AEC, (aoc)none)]
unsafe struct EEUIEWNCUAOpenScreen
{
    [address((ee)0x005DAC07, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DAC07
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)none), vtable((ee)0x00831398, (aoc)none)]
unsafe struct EEUIEWNCUASubmit
{
    [address((ee)0x005DAA9D, (aoc)none), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DAE0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008313D4, (aoc)0x00846CAC)]
unsafe struct EEUIEWNMUACancel
{
    [address((ee)0x0041093F, (aoc)0x005EB7E8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DA6A7
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0083130C, (aoc)0x00846C0C)]
unsafe struct EEUIEWNMUAOpenScreen
{
    [address((ee)0x005DAE0D, (aoc)0x005EB704), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DAE0D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008313C0, (aoc)0x00846C98)]
unsafe struct EEUIEWNMUASubmit
{
    [address((ee)0x005DACEB, (aoc)0x005EB5E2), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control event 106 "WNS Cancel" (dbuicontrolevents.dat), made by the event creator 0x65C5EE
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008357B8, (aoc)0x0084B8A0)]
unsafe struct EEUIEWNSCancel
{
    [address((ee)0x0063E146, (aoc)0x006547CF), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// form event 8 "WNS Form Open" (dbuiformevents.dat), made by the event creator 0x65A1A5
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834FFC, (aoc)0x0084B0A8)]
unsafe struct EEUIEWNSOpen
{
    [address((ee)0x005DB309, (aoc)0x005EBC13), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DAF9D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008313FC, (aoc)0x00846CD4)]
unsafe struct EEUIEWNSSCancel
{
    [address((ee)0x0041093F, (aoc)0x005EB7E8), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x5DAF9D
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008313E8, (aoc)0x00846CC0)]
unsafe struct EEUIEWNSSOk
{
    [address((ee)0x005DAEF1, (aoc)0x005EB7FB), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x660248
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008359AC, (aoc)0x0084BA48)]
unsafe struct EEUIEWNSSOpenScreen
{
    [address((ee)0x005DAF9D, (aoc)0x005EB8A7), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x58D3E1
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x0082F78C, (aoc)0x00845058)]
unsafe struct EEUIEWheelZoom
{
    /// the game's form's wheel event (form +0x150, UIForm::SetMouseWheelMoveEvent), run by the form's input on the
    /// game thread: with screen_ready, and the camera's view not 3 or a cinematic on, zoom_to(scroll_speed - the mouse's
    /// Z axis x 0.05) clamped to 0 .. 1 (both builds; the axis GetNormalizedValue, vtable slot 2: the wheel's delta /
    /// its granularity - notches); else the camera put back
    [address((ee)0x005D9417, (aoc)0x005E9F40), overrides(typeof(UIEvent))]
    [vtable_site("wheel_zoom_execute")]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x659A3B
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x00834D88, (aoc)0x0084AE68)]
unsafe struct EEUIEndCinematicModeEvent
{
    [address((ee)0x00658785, (aoc)0x0066FB93), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 125 "SI Map Mouse Out", 127 "SI Briefing Mouse Out", 129 "SI History Mouse Out", 131 "SI Hints Mouse Out", 133 "SI Movie Mouse Out", 135 "SI Start Mouse Out", 137 "SI Back Mouse Out" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008356B4, (aoc)0x0084B774)]
unsafe struct EEUISIEMouseOutButton
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// control events 124 "SI Map Mouse Over", 126 "SI Briefing Mouse Over", 128 "SI History Mouse Over", 130 "SI Hints Mouse Over", 132 "SI Movie Mouse Over", 134 "SI Start Mouse Over", 136 "SI Back Mouse Over" (dbuicontrolevents.dat), made by the event creator 0x65BA24
[@base(typeof(UIEvent)), size((ee)0xC, (aoc)0xC), vtable((ee)0x008356C8, (aoc)0x0084B788)]
unsafe struct EEUISIEMouseOverButton
{
    [address((ee)0x00628E89, (aoc)0x0067CF38), overrides(typeof(UIEvent))]
    extern uint32 execute(UIForm* form, uint32 arg);
}

/// made in 0x569046, 0x57BCE0, 0x57CD55, 0x590734, 0x5C7C40
[@base(typeof(UIEvent)), size((ee)0x24, (aoc)0x24), vtable((ee)0x0082DA94, (aoc)0x00843344)]
unsafe struct UIESliderChange
{
}
