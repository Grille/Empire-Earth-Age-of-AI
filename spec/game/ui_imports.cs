// Low-Level Engine.dll's UI functions the exe imports (UIForm, UIControl and its kinds, UIManager ...): their import
// slots, by the DLL's (mangled) names - found again by name at load (spec_import). The tool to change a form once
// it is built: UIForm::GetControlById, a control's SetRectangle / SetText, UIManager::GetFormByID / Push / Pop.
// Called through the slot with a typedef of the hand-written code (dll/game/api/ui.c: ui_form_add_hotkey).
// scripts/re/ui_classes.py's classes use them; game.cs and ui.cs hold the ones the mod had before.

/// the import slots: iat_ + the class without UI + the function (a template's argument after the class; overloads
/// numbered by their names' order)
static unsafe class ui_imports
{
    [import_slot("Low-Level Engine.dll", "?DrawBackground@UIBFull@@UAEJABVGERectangle@@@Z")]
    [address((ee)0x00824424, (aoc)0x00838424)]
    static block iat_b_full_draw_background;

    [import_slot("Low-Level Engine.dll", "?SetBitmapTexture@UIBFull@@UAEXPAVGETexture@@KK@Z")]
    [address((ee)0x0082423C, (aoc)0x00838250)]
    static block iat_b_full_set_bitmap_texture;

    [import_slot("Low-Level Engine.dll", "?SetNumberColumnsAndRows@UIBFull@@QAEXKK@Z")]
    [address((ee)0x00824428, (aoc)0x00838428)]
    static block iat_b_full_set_number_columns_and_rows;

    [import_slot("Low-Level Engine.dll", "?UnloadBackground@UIBFull@@UAEJXZ")]
    [address((ee)0x00824240, (aoc)0x00838254)]
    static block iat_b_full_unload_background;

    [import_slot("Low-Level Engine.dll", "?Close@UIBackground@@UAEXXZ")]
    [address((ee)0x0082424C, (aoc)0x00838260)]
    static block iat_background_close;

    [import_slot("Low-Level Engine.dll", "?Open@UIBackground@@UAEXXZ")]
    [address((ee)0x00824248, (aoc)0x0083825C)]
    static block iat_background_open;

    [import_slot("Low-Level Engine.dll", "?RelayoutBackground@UIBackground@@UAEXXZ")]
    [address((ee)0x00824244, (aoc)0x00838258)]
    static block iat_background_relayout_background;

    [import_slot("Low-Level Engine.dll", "?SetRectangle@UIBackground@@QAEXABVGERectangle@@@Z")]
    [address((ee)0x008242DC, (aoc)0x008382F8)]
    static block iat_background_set_rectangle;

    [import_slot("Low-Level Engine.dll", "?SetVisibility@UIBackground@@QAEX_N@Z")]
    [address((ee)0x00824278, (aoc)0x00838290)]
    static block iat_background_set_visibility;

    [import_slot("Low-Level Engine.dll", "?DrawStrectchedBackground@UIBitmapBackground@@UAEJABVGERectangle@@0@Z")]
    [address((ee)0x00824408, (aoc)0x00838408)]
    static block iat_bitmap_background_draw_strectched_background;

    [import_slot("Low-Level Engine.dll", "?LoadBackground@UIBitmapBackground@@UAEJXZ")]
    [address((ee)0x00824410, (aoc)0x00838410)]
    static block iat_bitmap_background_load_background;

    [import_slot("Low-Level Engine.dll", "?SetBitmapTexture@UIBitmapBackground@@QAEXPAVGETexture@@@Z")]
    [address((ee)0x00824468, (aoc)0x00838468)]
    static block iat_bitmap_background_set_bitmap_texture;

    [import_slot("Low-Level Engine.dll", "?UnloadBackground@UIBitmapBackground@@UAEJXZ")]
    [address((ee)0x0082440C, (aoc)0x0083840C)]
    static block iat_bitmap_background_unload_background;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIBitmap@@UAEJXZ")]
    [address((ee)0x008241B4, (aoc)0x008381C4)]
    static block iat_bitmap_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIBitmap@@UAEJXZ")]
    [address((ee)0x008241B0, (aoc)0x008381C0)]
    static block iat_bitmap_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIBitmap@@UAEJXZ")]
    [address((ee)0x008241AC, (aoc)0x008381BC)]
    static block iat_bitmap_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIBitmap@@MAEJXZ")]
    [address((ee)0x008241A8, (aoc)0x008381B8)]
    static block iat_bitmap_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIBitmap@@MAEJXZ")]
    [address((ee)0x008241A4, (aoc)0x008381B4)]
    static block iat_bitmap_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIBitmap@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x008241A0, (aoc)0x008381B0)]
    static block iat_bitmap_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIBitmap@@MAEJXZ")]
    [address((ee)0x0082419C, (aoc)0x008381AC)]
    static block iat_bitmap_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIBitmap@@MAEJXZ")]
    [address((ee)0x00824198, (aoc)0x008381A8)]
    static block iat_bitmap_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetBitmap@UIBitmap@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x008242B8, (aoc)0x008382D4)]
    static block iat_bitmap_set_bitmap;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIBitmap@@MAEXXZ")]
    [address((ee)0x00824190, (aoc)0x008381A0)]
    static block iat_bitmap_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIBitmap@@MAEXXZ")]
    [address((ee)0x00824194, (aoc)0x008381A4)]
    static block iat_bitmap_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIBitmap@@UAEJXZ")]
    [address((ee)0x0082418C, (aoc)0x0083819C)]
    static block iat_bitmap_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?SetIsBlinking@UIBlinkingButton@@QAEX_N@Z")]
    [address((ee)0x00823BB8, (aoc)0x00837BA0)]
    static block iat_blinking_button_set_is_blinking;

    [import_slot("Low-Level Engine.dll", "?ButtonDown@UIButton@@MAEXXZ")]
    [address((ee)0x00823C0C, (aoc)0x00837C08)]
    static block iat_button_button_down;

    [import_slot("Low-Level Engine.dll", "?ButtonUp@UIButton@@MAEXXZ")]
    [address((ee)0x00823C08, (aoc)0x00837C04)]
    static block iat_button_button_up;

    [import_slot("Low-Level Engine.dll", "?ChangeFont@UIButton@@UAEXPAVGEFont@@@Z")]
    [address((ee)0x00823C40, (aoc)0x00837C3C)]
    static block iat_button_change_font;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIButton@@UAEJXZ")]
    [address((ee)0x00823C3C, (aoc)0x00837C38)]
    static block iat_button_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIButton@@UAEJXZ")]
    [address((ee)0x00823C38, (aoc)0x00837C34)]
    static block iat_button_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIButton@@UAEJXZ")]
    [address((ee)0x00823C34, (aoc)0x00837C30)]
    static block iat_button_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIButton@@MAEJXZ")]
    [address((ee)0x00823C30, (aoc)0x00837C2C)]
    static block iat_button_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIButton@@MAEJXZ")]
    [address((ee)0x00823C2C, (aoc)0x00837C28)]
    static block iat_button_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIButton@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823C28, (aoc)0x00837C24)]
    static block iat_button_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseOver@UIButton@@MAEXXZ")]
    [address((ee)0x00823C04, (aoc)0x00837C00)]
    static block iat_button_mouse_over;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIButton@@MAEJXZ")]
    [address((ee)0x00823C24, (aoc)0x00837C20)]
    static block iat_button_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIButton@@MAEJXZ")]
    [address((ee)0x00823C20, (aoc)0x00837C1C)]
    static block iat_button_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetFocusedBitmap@UIButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823E68, (aoc)0x00837E74)]
    static block iat_button_set_focused_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIButton@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823C1C, (aoc)0x00837C18)]
    static block iat_button_set_location;

    [import_slot("Low-Level Engine.dll", "?SetNormalBitmap@UIButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823E64, (aoc)0x00837E70)]
    static block iat_button_set_normal_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetPressedBitmap@UIButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823E6C, (aoc)0x00837E78)]
    static block iat_button_set_pressed_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetText@UIButton@@QAEXABVUWideString@@PAVGEFont@@@Z")]
    [address((ee)0x00823334, (aoc)0x00837D38)]
    static block iat_button_set_text;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIButton@@MAEXXZ")]
    [address((ee)0x00823C14, (aoc)0x00837C10)]
    static block iat_button_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIButton@@MAEXXZ")]
    [address((ee)0x00823C18, (aoc)0x00837C14)]
    static block iat_button_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIButton@@UAEJXZ")]
    [address((ee)0x00823C10, (aoc)0x00837C0C)]
    static block iat_button_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?ButtonDown@UICheckBox@@MAEXXZ")]
    [address((ee)0x00823E24, (aoc)0x00837E2C)]
    static block iat_check_box_button_down;

    [import_slot("Low-Level Engine.dll", "?ButtonUp@UICheckBox@@MAEXXZ")]
    [address((ee)0x00823E20, (aoc)0x00837E28)]
    static block iat_check_box_button_up;

    [import_slot("Low-Level Engine.dll", "?ChangeWidth@UICheckBox@@QAEXK@Z")]
    [address((ee)0x00823F80, (aoc)0x00837F98)]
    static block iat_check_box_change_width;

    [import_slot("Low-Level Engine.dll", "?CopyData@UICheckBox@@UAEJXZ")]
    [address((ee)0x00823E54, (aoc)0x00837E5C)]
    static block iat_check_box_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UICheckBox@@UAEJXZ")]
    [address((ee)0x00823E50, (aoc)0x00837E58)]
    static block iat_check_box_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UICheckBox@@UAEJXZ")]
    [address((ee)0x00823E4C, (aoc)0x00837E54)]
    static block iat_check_box_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetType@UICheckBox@@UBEKXZ")]
    [address((ee)0x00823E48, (aoc)0x00837E50)]
    static block iat_check_box_get_type;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UICheckBox@@MAEJXZ")]
    [address((ee)0x00823E44, (aoc)0x00837E4C)]
    static block iat_check_box_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UICheckBox@@MAEJXZ")]
    [address((ee)0x00823E40, (aoc)0x00837E48)]
    static block iat_check_box_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UICheckBox@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823E3C, (aoc)0x00837E44)]
    static block iat_check_box_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseOver@UICheckBox@@MAEXXZ")]
    [address((ee)0x00823E1C, (aoc)0x00837E24)]
    static block iat_check_box_mouse_over;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UICheckBox@@MAEJXZ")]
    [address((ee)0x00823E38, (aoc)0x00837E40)]
    static block iat_check_box_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UICheckBox@@MAEJXZ")]
    [address((ee)0x00823E34, (aoc)0x00837E3C)]
    static block iat_check_box_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetChecked@UICheckBox@@QAEX_N@Z")]
    [address((ee)0x00823344, (aoc)0x00837628)]
    static block iat_check_box_set_checked;

    [import_slot("Low-Level Engine.dll", "?SetDisabled@UICheckBox@@QAEX_N@Z")]
    [address((ee)0x0082427C, (aoc)0x00838294)]
    static block iat_check_box_set_disabled;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UICheckBox@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823E30, (aoc)0x00837E38)]
    static block iat_check_box_set_location;

    [import_slot("Low-Level Engine.dll", "?SetText@UICheckBox@@QAEXABVUWideString@@PAVGEFont@@@Z")]
    [address((ee)0x00823F7C, (aoc)0x00837F94)]
    static block iat_check_box_set_text;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UICheckBox@@MAEXXZ")]
    [address((ee)0x00823E28, (aoc)0x00837E30)]
    static block iat_check_box_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UICheckBox@@MAEXXZ")]
    [address((ee)0x00823E2C, (aoc)0x00837E34)]
    static block iat_check_box_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?AddString@UIComboBox@@MAEJXZ")]
    [address((ee)0x00823DC8, (aoc)0x00837DD0)]
    static block iat_combo_box_add_string;

    [import_slot("Low-Level Engine.dll", "?AppendLine@UIComboBox@@QAEXAAVUWideString@@@Z")]
    [address((ee)0x00823318, (aoc)0x00837BF8)]
    static block iat_combo_box_append_line;

    [import_slot("Low-Level Engine.dll", "?AppendStringArray@UIComboBox@@QAEXAAV?$vector@VUWideString@@V?$allocator@VUWideString@@@std@@@std@@@Z")]
    [address((ee)0x00823ED0, (aoc)0x00837EDC)]
    static block iat_combo_box_append_string_array;

    [import_slot("Low-Level Engine.dll", "?ChangeSelection@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DB4, (aoc)0x00837DB8)]
    static block iat_combo_box_change_selection;

    [import_slot("Low-Level Engine.dll", "?ChangeWidth@UIComboBox@@QAEXK@Z")]
    [address((ee)0x00823320, (aoc)0x00837F88)]
    static block iat_combo_box_change_width;

    [import_slot("Low-Level Engine.dll", "?CloseListBox@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DC4, (aoc)0x00837DCC)]
    static block iat_combo_box_close_list_box;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIComboBox@@UAEJXZ")]
    [address((ee)0x00823E0C, (aoc)0x00837E14)]
    static block iat_combo_box_copy_data;

    [import_slot("Low-Level Engine.dll", "?DeleteAllEntries@UIComboBox@@QAEXXZ")]
    [address((ee)0x0082331C, (aoc)0x00837BFC)]
    static block iat_combo_box_delete_all_entries;

    [import_slot("Low-Level Engine.dll", "?DeleteLine@UIComboBox@@QAEXK@Z")]
    [address((ee)0x008242BC, (aoc)0x008382D8)]
    static block iat_combo_box_delete_line;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIComboBox@@UAEJXZ")]
    [address((ee)0x00823E08, (aoc)0x00837E10)]
    static block iat_combo_box_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIComboBox@@UAEJXZ")]
    [address((ee)0x00823E04, (aoc)0x00837E0C)]
    static block iat_combo_box_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetGEText@UIComboBox@@QBEPAVGEText@@XZ")]
    [address((ee)0x00823D44, (aoc)0x00837D44)]
    static block iat_combo_box_get_ge_text;

    [import_slot("Low-Level Engine.dll", "?HideControl@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823E00, (aoc)0x00837E08)]
    static block iat_combo_box_hide_control;

    [import_slot("Low-Level Engine.dll", "?InsertLine@UIComboBox@@QAEXABVUWideString@@K@Z")]
    [address((ee)0x00824230, (aoc)0x00838244)]
    static block iat_combo_box_insert_line;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIComboBox@@MAEJXZ")]
    [address((ee)0x00823DFC, (aoc)0x00837E04)]
    static block iat_combo_box_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIComboBox@@MAEJXZ")]
    [address((ee)0x00823DF8, (aoc)0x00837E00)]
    static block iat_combo_box_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIComboBox@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823DF4, (aoc)0x00837DFC)]
    static block iat_combo_box_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIComboBox@@MAEJXZ")]
    [address((ee)0x00823DF0, (aoc)0x00837DF8)]
    static block iat_combo_box_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIComboBox@@MAEJXZ")]
    [address((ee)0x00823DEC, (aoc)0x00837DF4)]
    static block iat_combo_box_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseWheelTurned@UIComboBox@@UAEJJ@Z")]
    [address((ee)0x00823DE8, (aoc)0x00837DF0)]
    static block iat_combo_box_mouse_wheel_turned;

    [import_slot("Low-Level Engine.dll", "?OpenListBox@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DC0, (aoc)0x00837DC8)]
    static block iat_combo_box_open_list_box;

    [import_slot("Low-Level Engine.dll", "?ProcessDown@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DBC, (aoc)0x00837DC4)]
    static block iat_combo_box_process_down;

    [import_slot("Low-Level Engine.dll", "?ProcessUp@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DB8, (aoc)0x00837DC0)]
    static block iat_combo_box_process_up;

    [import_slot("Low-Level Engine.dll", "?SetBgColor@UIComboBox@@UAEXABVGEColor@@@Z")]
    [address((ee)0x00823DE4, (aoc)0x00837DEC)]
    static block iat_combo_box_set_bg_color;

    [import_slot("Low-Level Engine.dll", "?SetBgOpacity@UIComboBox@@UAEXM@Z")]
    [address((ee)0x00823DE0, (aoc)0x00837DE8)]
    static block iat_combo_box_set_bg_opacity;

    [import_slot("Low-Level Engine.dll", "?SetCurrentSelection@UIComboBox@@QAEXK@Z")]
    [address((ee)0x00823314, (aoc)0x00837BF4)]
    static block iat_combo_box_set_current_selection;

    [import_slot("Low-Level Engine.dll", "?SetListBoxAlwaysDirty@UIComboBox@@QAEX_N@Z")]
    [address((ee)0x00823FD0, (aoc)0x00837FE4)]
    static block iat_combo_box_set_list_box_always_dirty;

    [import_slot("Low-Level Engine.dll", "?SetListBoxSelectedBgColor@UIComboBox@@QAEXABVGEColor@@@Z")]
    [address((ee)0x00823D40, (aoc)0x00837D40)]
    static block iat_combo_box_set_list_box_selected_bg_color;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIComboBox@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823DDC, (aoc)0x00837DE4)]
    static block iat_combo_box_set_location;

    [import_slot("Low-Level Engine.dll", "?SetTextBoxBgOpacity@UIComboBox@@QAEXM@Z")]
    [address((ee)0x00823FD4, (aoc)0x00837FE8)]
    static block iat_combo_box_set_text_box_bg_opacity;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DD4, (aoc)0x00837DDC)]
    static block iat_combo_box_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIComboBox@@MAEXXZ")]
    [address((ee)0x00823DD8, (aoc)0x00837DE0)]
    static block iat_combo_box_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIComboBox@@UAEJXZ")]
    [address((ee)0x00823DD0, (aoc)0x00837DD8)]
    static block iat_combo_box_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?UpdateTranslatedKey@UIComboBox@@UAEJPBDF@Z")]
    [address((ee)0x00823DCC, (aoc)0x00837DD4)]
    static block iat_combo_box_update_translated_key;

    [import_slot("Low-Level Engine.dll", "?AddIMEResultString@UIControl@@UAEXABVUWideString@@@Z")]
    [address((ee)0x00823D38, (aoc)0x00837D34)]
    static block iat_control_add_ime_result_string;

    [import_slot("Low-Level Engine.dll", "?CaptureKeyboard@UIControl@@IAEXXZ")]
    [address((ee)0x00824000, (aoc)0x00838014)]
    static block iat_control_capture_keyboard;

    [import_slot("Low-Level Engine.dll", "?CaptureMouse@UIControl@@IAEXXZ")]
    [address((ee)0x008240BC, (aoc)0x008380CC)]
    static block iat_control_capture_mouse;

    [import_slot("Low-Level Engine.dll", "?ChangeFont@UIControl@@UAEXPAVGEFont@@@Z")]
    [address((ee)0x00823E10, (aoc)0x00837E18)]
    static block iat_control_change_font;

    [import_slot("Low-Level Engine.dll", "?ChangeIMECompositionString@UIControl@@UAEJABVUWideString@@ABVUString@@K@Z")]
    [address((ee)0x00823D34, (aoc)0x00837D30)]
    static block iat_control_change_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?CloseIMECompositionString@UIControl@@UAEJXZ")]
    [address((ee)0x00823D2C, (aoc)0x00837D28)]
    static block iat_control_close_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?ControlKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x00824450, (aoc)0x00838450)]
    static block iat_control_control_key_pressed;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIControl@@UAEJXZ")]
    [address((ee)0x008240B0, (aoc)0x008380C0)]
    static block iat_control_copy_data;

    [import_slot("Low-Level Engine.dll", "?DownKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x008241DC, (aoc)0x008381EC)]
    static block iat_control_down_key_pressed;

    [import_slot("Low-Level Engine.dll", "?DrawBorder@UIControl@@MAEJABVGERectangle@@K@Z")]
    [address((ee)0x00823CB8, (aoc)0x00837CB4)]
    static block iat_control_draw_border;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIControl@@UAEJXZ")]
    [address((ee)0x008240AC, (aoc)0x008380BC)]
    static block iat_control_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIControl@@UAEJXZ")]
    [address((ee)0x00823EA4, (aoc)0x00837EB0)]
    static block iat_control_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?EndKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x008241D0, (aoc)0x008381E0)]
    static block iat_control_end_key_pressed;

    [import_slot("Low-Level Engine.dll", "?EnterKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x008241CC, (aoc)0x008381DC)]
    static block iat_control_enter_key_pressed;

    [import_slot("Low-Level Engine.dll", "?FireTriggerEvent@UIControl@@QAEXK@Z")]
    [address((ee)0x00823F94, (aoc)0x00837FAC)]
    static block iat_control_fire_trigger_event;

    [import_slot("Low-Level Engine.dll", "?GetHasMouseCapture@UIControl@@QBE_NXZ")]
    [address((ee)0x008240C8, (aoc)0x008380D8)]
    static block iat_control_get_has_mouse_capture;

    [import_slot("Low-Level Engine.dll", "?GetHeight@UIControl@@QBEKXZ")]
    [address((ee)0x008240B8, (aoc)0x008380C8)]
    static block iat_control_get_height;

    [import_slot("Low-Level Engine.dll", "?GetHelpString@UIControl@@UBE_NAAVUWideString@@@Z")]
    [address((ee)0x00823D1C, (aoc)0x00837D18)]
    static block iat_control_get_help_string;

    [import_slot("Low-Level Engine.dll", "?GetIMEEnabled@UIControl@@UBE_NXZ")]
    [address((ee)0x00823D18, (aoc)0x00837D14)]
    static block iat_control_get_ime_enabled;

    [import_slot("Low-Level Engine.dll", "?GetLeftMouseButtonDown@UIControl@@QBE_NXZ")]
    [address((ee)0x008240C4, (aoc)0x008380D4)]
    static block iat_control_get_left_mouse_button_down;

    [import_slot("Low-Level Engine.dll", "?GetMouseLocation@UIControl@@IBEABVGE2DPoint@@XZ")]
    [address((ee)0x00823FF0, (aoc)0x00838004)]
    static block iat_control_get_mouse_location;

    [import_slot("Low-Level Engine.dll", "?GetRasterizer@UIControl@@QBEPAVGERasterizer@@XZ")]
    [address((ee)0x00823FF8, (aoc)0x0083800C)]
    static block iat_control_get_rasterizer;

    [import_slot("Low-Level Engine.dll", "?GetRightMouseButtonDown@UIControl@@QBE_NXZ")]
    [address((ee)0x008240C0, (aoc)0x008380D0)]
    static block iat_control_get_right_mouse_button_down;

    [import_slot("Low-Level Engine.dll", "?GetScreenIndex@UIControl@@QBEKXZ")]
    [address((ee)0x00823FF4, (aoc)0x00838008)]
    static block iat_control_get_screen_index;

    [import_slot("Low-Level Engine.dll", "?GetTabable@UIControl@@UBE_NXZ")]
    [address((ee)0x00823C88, (aoc)0x00837C84)]
    static block iat_control_get_tabable;

    [import_slot("Low-Level Engine.dll", "?GetTriggerEvent@UIControl@@QAEPAVUIEvent@@K@Z")]
    [address((ee)0x00823310, (aoc)0x00837EF4)]
    static block iat_control_get_trigger_event;

    [import_slot("Low-Level Engine.dll", "?GetType@UIControl@@UBEKXZ")]
    [address((ee)0x00823D10, (aoc)0x00837D0C)]
    static block iat_control_get_type;

    [import_slot("Low-Level Engine.dll", "?GetWidth@UIControl@@QBEKXZ")]
    [address((ee)0x00823F90, (aoc)0x00837FA8)]
    static block iat_control_get_width;

    [import_slot("Low-Level Engine.dll", "?HideControl@UIControl@@UAEXXZ")]
    [address((ee)0x00823D0C, (aoc)0x00837D08)]
    static block iat_control_hide_control;

    [import_slot("Low-Level Engine.dll", "?HomeKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x008241D4, (aoc)0x008381E4)]
    static block iat_control_home_key_pressed;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDoublePressed@UIControl@@UAEJXZ")]
    [address((ee)0x00823D00, (aoc)0x00837CFC)]
    static block iat_control_mouse_l_button_double_pressed;

    [import_slot("Low-Level Engine.dll", "?MouseWheelTurned@UIControl@@UAEJJ@Z")]
    [address((ee)0x00823C6C, (aoc)0x00837C68)]
    static block iat_control_mouse_wheel_turned;

    [import_slot("Low-Level Engine.dll", "?OpenIMECompositionString@UIControl@@UAEJXZ")]
    [address((ee)0x00823CEC, (aoc)0x00837CE8)]
    static block iat_control_open_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?ReleaseCapturedKeyboard@UIControl@@UAEXXZ")]
    [address((ee)0x00823CE8, (aoc)0x00837CE4)]
    static block iat_control_release_captured_keyboard;

    [import_slot("Low-Level Engine.dll", "?ReleaseCapturedMouse@UIControl@@UAEXXZ")]
    [address((ee)0x00823CE4, (aoc)0x00837CE0)]
    static block iat_control_release_captured_mouse;

    [import_slot("Low-Level Engine.dll", "?sEnterKey@UIControl@@1PAVISEntity@@A")]
    [address((ee)0x00823FFC, (aoc)0x00838010)]
    static block iat_control_s_enter_key;

    [import_slot("Low-Level Engine.dll", "?SetBgColor@UIControl@@UAEXABVGEColor@@@Z")]
    [address((ee)0x00823CE0, (aoc)0x00837CDC)]
    static block iat_control_set_bg_color;

    [import_slot("Low-Level Engine.dll", "?SetBgOpacity@UIControl@@UAEXM@Z")]
    [address((ee)0x00823CDC, (aoc)0x00837CD8)]
    static block iat_control_set_bg_opacity;

    [import_slot("Low-Level Engine.dll", "?SetBgRectangle@UIControl@@IAEXABVGERectangle@@@Z")]
    [address((ee)0x008241C8, (aoc)0x008381D8)]
    static block iat_control_set_bg_rectangle;

    [import_slot("Low-Level Engine.dll", "?SetBorderColors@UIControl@@SAXABVGEColor@@0@Z")]
    [address((ee)0x00823A9C, (aoc)0x00837A8C)]
    static block iat_control_set_border_colors;

    [import_slot("Low-Level Engine.dll", "?SetDirty@UIControl@@UAEX_N@Z")]
    [address((ee)0x00823CD8, (aoc)0x00837CD4)]
    static block iat_control_set_dirty;

    [import_slot("Low-Level Engine.dll", "?SetDisplayMode@UIControl@@UAEXK@Z")]
    [address((ee)0x00823CD4, (aoc)0x00837CD0)]
    static block iat_control_set_display_mode;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIControl@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823FEC, (aoc)0x00838000)]
    static block iat_control_set_location;

    [import_slot("Low-Level Engine.dll", "?SetRectangle@UIControl@@UAEXABVGERectangle@@@Z")]
    [address((ee)0x00823C5C, (aoc)0x00837C58)]
    static block iat_control_set_rectangle;

    [import_slot("Low-Level Engine.dll", "?SetTriggerEvent@UIControl@@QAEXKPAVUIEvent@@@Z")]
    [address((ee)0x0082330C, (aoc)0x00837D48)]
    static block iat_control_set_trigger_event;

    [import_slot("Low-Level Engine.dll", "?ShiftKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x00824434, (aoc)0x00838434)]
    static block iat_control_shift_key_pressed;

    [import_slot("Low-Level Engine.dll", "?UpKeyPressed@UIControl@@SA_NXZ")]
    [address((ee)0x008241D8, (aoc)0x008381E8)]
    static block iat_control_up_key_pressed;

    [import_slot("Low-Level Engine.dll", "?UpdateMouse@UIControl@@QAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00824444, (aoc)0x00838444)]
    static block iat_control_update_mouse;

    [import_slot("Low-Level Engine.dll", "?UpdateTranslatedKey@UIControl@@UAEJPBDF@Z")]
    [address((ee)0x00823CBC, (aoc)0x00837CB8)]
    static block iat_control_update_translated_key;

    [import_slot("Low-Level Engine.dll", "?CreateBitmap@UIControlsTable@@QBEPAVGEBitmap@@PAVUIManager@@ABVGERectangle@@ABVUString@@_N1@Z")]
    [address((ee)0x00824430, (aoc)0x00838430)]
    static block iat_controls_table_create_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetupFont@UIControlsTable@@QAEXPAVUIManager@@ABVGERectangle@@_N@Z")]
    [address((ee)0x008244D8, (aoc)0x008384D8)]
    static block iat_controls_table_setup_font;

    [import_slot("Low-Level Engine.dll", "?SetupGEText@UIControlsTable@@QBEXPAVGEText@@@Z")]
    [address((ee)0x00824438, (aoc)0x00838438)]
    static block iat_controls_table_setup_ge_text;

    [import_slot("Low-Level Engine.dll", "?SetupStringArray@UIControlsTable@@QBEXPAVUIManager@@AAV?$vector@VUWideString@@V?$allocator@VUWideString@@@std@@@std@@@Z")]
    [address((ee)0x008244DC, (aoc)0x008384DC)]
    static block iat_controls_table_setup_string_array;

    [import_slot("Low-Level Engine.dll", "?ProcessEvent@UIEPolledEvent@@UAEJPAVUIForm@@PAVUIEntity@@@Z")]
    [address((ee)0x00823340, (aoc)none)]
    static block iat_e_polled_event_process_event;

    [import_slot("Low-Level Engine.dll", "?AddString@UIESliderChange@@QAEXMVUWideString@@@Z")]
    [address((ee)0x00823F98, (aoc)0x00837FB0)]
    static block iat_e_slider_change_add_string;

    [import_slot("Low-Level Engine.dll", "?ProcessEvent@UIESliderChange@@UAEJPAVUIForm@@PAVUIEntity@@@Z")]
    [address((ee)0x00823EB0, (aoc)0x00837EBC)]
    static block iat_e_slider_change_process_event;

    [import_slot("Low-Level Engine.dll", "?AddIMEResultString@UIEditBox@@UAEXABVUWideString@@@Z")]
    [address((ee)0x00823CA8, (aoc)0x00837CA4)]
    static block iat_edit_box_add_ime_result_string;

    [import_slot("Low-Level Engine.dll", "?ButtonDown@UIEditBox@@MAEXXZ")]
    [address((ee)0x00823C48, (aoc)0x00837C44)]
    static block iat_edit_box_button_down;

    [import_slot("Low-Level Engine.dll", "?ChangeFont@UIEditBox@@UAEXPAVGEFont@@@Z")]
    [address((ee)0x00823CA0, (aoc)0x00837C9C)]
    static block iat_edit_box_change_font;

    [import_slot("Low-Level Engine.dll", "?ChangeIMECompositionString@UIEditBox@@UAEJABVUWideString@@ABVUString@@K@Z")]
    [address((ee)0x00823CA4, (aoc)0x00837CA0)]
    static block iat_edit_box_change_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?ChangeWidth@UIEditBox@@QAEXK@Z")]
    [address((ee)0x00823F8C, (aoc)0x00837FA4)]
    static block iat_edit_box_change_width;

    [import_slot("Low-Level Engine.dll", "?CloseIMECompositionString@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C9C, (aoc)0x00837C98)]
    static block iat_edit_box_close_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C98, (aoc)0x00837C94)]
    static block iat_edit_box_copy_data;

    [import_slot("Low-Level Engine.dll", "?DeleteText@UIEditBox@@QAEXXZ")]
    [address((ee)0x008242B4, (aoc)0x008382CC)]
    static block iat_edit_box_delete_text;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C94, (aoc)0x00837C90)]
    static block iat_edit_box_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C90, (aoc)0x00837C8C)]
    static block iat_edit_box_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetAllText@UIEditBox@@QBEXAAVUWideString@@@Z")]
    [address((ee)0x00823D64, (aoc)0x00837D68)]
    static block iat_edit_box_get_all_text;

    [import_slot("Low-Level Engine.dll", "?GetIMEEnabled@UIEditBox@@UBE_NXZ")]
    [address((ee)0x00823C8C, (aoc)0x00837C88)]
    static block iat_edit_box_get_ime_enabled;

    [import_slot("Low-Level Engine.dll", "?GetLineCount@UIEditBox@@QBEKXZ")]
    [address((ee)0x00824224, (aoc)0x00838238)]
    static block iat_edit_box_get_line_count;

    [import_slot("Low-Level Engine.dll", "?GetTextAsInt@UIEditBox@@QBEJK@Z")]
    [address((ee)0x00823F74, (aoc)0x00837F8C)]
    static block iat_edit_box_get_text_as_int;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDoublePressed@UIEditBox@@MAEJXZ")]
    [address((ee)0x00823C7C, (aoc)0x00837C78)]
    static block iat_edit_box_mouse_l_button_double_pressed;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIEditBox@@MAEJXZ")]
    [address((ee)0x00823C84, (aoc)0x00837C80)]
    static block iat_edit_box_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIEditBox@@MAEJXZ")]
    [address((ee)0x00823C80, (aoc)0x00837C7C)]
    static block iat_edit_box_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIEditBox@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823C78, (aoc)0x00837C74)]
    static block iat_edit_box_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIEditBox@@MAEJXZ")]
    [address((ee)0x00823C74, (aoc)0x00837C70)]
    static block iat_edit_box_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIEditBox@@MAEJXZ")]
    [address((ee)0x00823C70, (aoc)0x00837C6C)]
    static block iat_edit_box_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?OpenIMECompositionString@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C68, (aoc)0x00837C64)]
    static block iat_edit_box_open_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?ProcessEnterKey@UIEditBox@@MAEXXZ")]
    [address((ee)0x00823C44, (aoc)0x00837C40)]
    static block iat_edit_box_process_enter_key;

    [import_slot("Low-Level Engine.dll", "?ReleaseCapturedKeyboard@UIEditBox@@UAEXXZ")]
    [address((ee)0x00823C64, (aoc)0x00837C60)]
    static block iat_edit_box_release_captured_keyboard;

    [import_slot("Low-Level Engine.dll", "?SelectText@UIEditBox@@QAEXXZ")]
    [address((ee)0x00824574, (aoc)0x00838570)]
    static block iat_edit_box_select_text;

    [import_slot("Low-Level Engine.dll", "?SetCaretLocation@UIEditBox@@QAEXKK@Z")]
    [address((ee)0x00823F68, (aoc)0x00837F7C)]
    static block iat_edit_box_set_caret_location;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIEditBox@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823C60, (aoc)0x00837C5C)]
    static block iat_edit_box_set_location;

    [import_slot("Low-Level Engine.dll", "?SetText@UIEditBox@@QAEXABVUWideString@@@Z")]
    [address((ee)0x00823350, (aoc)0x00837624)]
    static block iat_edit_box_set_text;

    [import_slot("Low-Level Engine.dll", "?SetText@UIEditBox@@QAEXK@Z")]
    [address((ee)0x00823F78, (aoc)0x00837F90)]
    static block iat_edit_box_set_text_2;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIEditBox@@MAEXXZ")]
    [address((ee)0x00823C54, (aoc)0x00837C50)]
    static block iat_edit_box_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIEditBox@@MAEXXZ")]
    [address((ee)0x00823C58, (aoc)0x00837C54)]
    static block iat_edit_box_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C50, (aoc)0x00837C4C)]
    static block iat_edit_box_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?UpdateTranslatedKey@UIEditBox@@UAEJPBDF@Z")]
    [address((ee)0x00823C4C, (aoc)0x00837C48)]
    static block iat_edit_box_update_translated_key;

    [import_slot("Low-Level Engine.dll", "?Disable@UIEntityGroup@@QAEXXZ")]
    [address((ee)0x00823330, (aoc)0x00837DBC)]
    static block iat_entity_group_disable;

    [import_slot("Low-Level Engine.dll", "?Enable@UIEntityGroup@@QAEXXZ")]
    [address((ee)0x00823338, (aoc)0x00837E68)]
    static block iat_entity_group_enable;

    [import_slot("Low-Level Engine.dll", "?RemoveEntity@UIEntityGroup@@AAEJPAVUIEntity@@@Z")]
    [address((ee)0x00824370, (aoc)0x0083838C)]
    static block iat_entity_group_remove_entity;

    [import_slot("Low-Level Engine.dll", "?InitializeEvent@UIEvent@@MAEJXZ")]
    [address((ee)0x0082320C, (aoc)0x00837208)]
    static block iat_event_initialize_event;

    [import_slot("Low-Level Engine.dll", "?UninitializeEvent@UIEvent@@MAEJXZ")]
    [address((ee)0x00823208, (aoc)0x00837204)]
    static block iat_event_uninitialize_event;

    [import_slot("Low-Level Engine.dll", "?CreateEntries@UIFileComboBox@@QAEXK@Z")]
    [address((ee)0x00824234, (aoc)0x00838248)]
    static block iat_file_combo_box_create_entries;

    [import_slot("Low-Level Engine.dll", "?SetSearchPaths@UIFileComboBox@@QAEXPAVFSPath@@@Z")]
    [address((ee)0x00824238, (aoc)0x0083824C)]
    static block iat_file_combo_box_set_search_paths;

    [import_slot("Low-Level Engine.dll", "?CreateEntries@UIFileListBox@@QAEXK@Z")]
    [address((ee)0x00824290, (aoc)0x008382A8)]
    static block iat_file_list_box_create_entries;

    [import_slot("Low-Level Engine.dll", "?DeleteSelectedFile@UIFileListBox@@QAEJXZ")]
    [address((ee)0x00824374, (aoc)0x00838390)]
    static block iat_file_list_box_delete_selected_file;

    [import_slot("Low-Level Engine.dll", "?SetSearchPaths@UIFileListBox@@QAEXPAVFSPath@@@Z")]
    [address((ee)0x00824294, (aoc)0x008382AC)]
    static block iat_file_list_box_set_search_paths;

    [import_slot("Low-Level Engine.dll", "?CreateClosestFont@UIFontTable@@SAPAVGEFont@@KK@Z")]
    [address((ee)0x00824448, (aoc)0x0083844C)]
    static block iat_font_table_create_closest_font;

    [import_slot("Low-Level Engine.dll", "?FindOrCreateFont@UIFontTable@@SAPAVGEFont@@ABVUString@@KKK@Z")]
    [address((ee)0x00823AA4, (aoc)0x00837A94)]
    static block iat_font_table_find_or_create_font;

    [import_slot("Low-Level Engine.dll", "?Activate@UIForm@@UAEXXZ")]
    [address((ee)0x00824038, (aoc)0x0083804C)]
    static block iat_form_activate;

    [import_slot("Low-Level Engine.dll", "?AddControl@UIForm@@QAEXPAVUIControl@@K@Z")]
    [address((ee)0x00823D4C, (aoc)0x00837D50)]
    static block iat_form_add_control;

    [import_slot("Low-Level Engine.dll", "?AddHotKey@UIForm@@QAEXABVUIHotKey@@K@Z")]
    [address((ee)0x00824054, (aoc)0x00838064)]
    static block iat_form_add_hot_key;

    [import_slot("Low-Level Engine.dll", "?AddIMEResultString@UIForm@@UAEXABVUWideString@@@Z")]
    [address((ee)0x00824034, (aoc)0x00838048)]
    static block iat_form_add_ime_result_string;

    [import_slot("Low-Level Engine.dll", "?ChangeIMECompositionString@UIForm@@UAEJABVUWideString@@ABVUString@@K@Z")]
    [address((ee)0x00824030, (aoc)0x00838044)]
    static block iat_form_change_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?Close@UIForm@@MAEXXZ")]
    [address((ee)0x00824078, (aoc)0x00838088)]
    static block iat_form_close;

    [import_slot("Low-Level Engine.dll", "?CloseIMECompositionString@UIForm@@UAEJXZ")]
    [address((ee)0x0082402C, (aoc)0x00838040)]
    static block iat_form_close_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIForm@@MAEJXZ")]
    [address((ee)0x0082407C, (aoc)0x0083808C)]
    static block iat_form_copy_data;

    [import_slot("Low-Level Engine.dll", "?Deactivate@UIForm@@UAEXXZ")]
    [address((ee)0x00824028, (aoc)0x0083803C)]
    static block iat_form_deactivate;

    [import_slot("Low-Level Engine.dll", "?DrawBackground@UIForm@@MAEJABVGERectangle@@@Z")]
    [address((ee)0x0082445C, (aoc)0x0083845C)]
    static block iat_form_draw_background;

    [import_slot("Low-Level Engine.dll", "?DrawBackground@UIForm@@QAEJPAVGEBitmap@@ABVGERectangle@@11@Z")]
    [address((ee)0x00823F5C, (aoc)0x00837F70)]
    static block iat_form_draw_background_2;

    [import_slot("Low-Level Engine.dll", "?DrawClippedText@UIForm@@QAEJABVGERectangle@@PAVGEText@@PAVUIControl@@@Z")]
    [address((ee)0x008237A8, (aoc)0x00837780)]
    static block iat_form_draw_clipped_text;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIForm@@MAEJXZ")]
    [address((ee)0x0082409C, (aoc)0x008380AC)]
    static block iat_form_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIForm@@MAEJXZ")]
    [address((ee)0x00824010, (aoc)0x00838024)]
    static block iat_form_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetBackgroundByID@UIForm@@QBEPAVUIBackground@@K@Z")]
    [address((ee)0x0082425C, (aoc)0x00838270)]
    static block iat_form_get_background_by_id;

    [import_slot("Low-Level Engine.dll", "?GetControlById@UIForm@@QAEPAVUIControl@@K@Z")]
    [address((ee)0x00823868, (aoc)0x0083783C)]
    static block iat_form_get_control_by_id;

    [import_slot("Low-Level Engine.dll", "?GetEntityGroupById@UIForm@@QBEPAVUIEntityGroup@@K@Z")]
    [address((ee)0x00823F64, (aoc)0x00837F78)]
    static block iat_form_get_entity_group_by_id;

    [import_slot("Low-Level Engine.dll", "?Open@UIForm@@MAEXXZ")]
    [address((ee)0x008240A0, (aoc)0x008380B0)]
    static block iat_form_open;

    [import_slot("Low-Level Engine.dll", "?OpenIMECompositionString@UIForm@@UAEJXZ")]
    [address((ee)0x00824024, (aoc)0x00838038)]
    static block iat_form_open_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?RedrawIntersectControls@UIForm@@QAEXABVGERectangle@@@Z")]
    [address((ee)0x0082444C, (aoc)0x00838448)]
    static block iat_form_redraw_intersect_controls;

    [import_slot("Low-Level Engine.dll", "?RelayoutForm@UIForm@@UAEXABVGERectangle@@@Z")]
    [address((ee)0x00824020, (aoc)0x00838034)]
    static block iat_form_relayout_form;

    [import_slot("Low-Level Engine.dll", "?RemoveAllHotKeys@UIForm@@QAEXXZ")]
    [address((ee)0x00824264, (aoc)0x00838278)]
    static block iat_form_remove_all_hot_keys;

    [import_slot("Low-Level Engine.dll", "?RemoveAllHotKeysForAGroup@UIForm@@QAEXK@Z")]
    [address((ee)0x0082448C, (aoc)0x0083848C)]
    static block iat_form_remove_all_hot_keys_for_a_group;

    [import_slot("Low-Level Engine.dll", "?RemoveBackground@UIForm@@QAEXPAVUIBackground@@@Z")]
    [address((ee)0x00824258, (aoc)0x0083826C)]
    static block iat_form_remove_background;

    [import_slot("Low-Level Engine.dll", "?SetAllControlsDirty@UIForm@@QAEXXZ")]
    [address((ee)0x00823F58, (aoc)0x00837F6C)]
    static block iat_form_set_all_controls_dirty;

    [import_slot("Low-Level Engine.dll", "?SetFormActivateEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824388, (aoc)0x008383A8)]
    static block iat_form_set_form_activate_event;

    [import_slot("Low-Level Engine.dll", "?SetFormCloseEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x0082335C, (aoc)0x0083828C)]
    static block iat_form_set_form_close_event;

    [import_slot("Low-Level Engine.dll", "?SetFormDeactivateEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824384, (aoc)0x008383A4)]
    static block iat_form_set_form_deactivate_event;

    [import_slot("Low-Level Engine.dll", "?SetFormKeyboardCapturedEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824268, (aoc)0x0083827C)]
    static block iat_form_set_form_keyboard_captured_event;

    [import_slot("Low-Level Engine.dll", "?SetFormManager@UIForm@@QAEXPAVUIFormManager@@@Z")]
    [address((ee)0x00824270, (aoc)0x00838284)]
    static block iat_form_set_form_manager;

    [import_slot("Low-Level Engine.dll", "?SetFormOpenEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00823214, (aoc)0x00837210)]
    static block iat_form_set_form_open_event;

    [import_slot("Low-Level Engine.dll", "?SetFormUpdateEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00823354, (aoc)0x0083839C)]
    static block iat_form_set_form_update_event;

    [import_slot("Low-Level Engine.dll", "?SetHelpTextPopupLocation@UIForm@@UAEXXZ")]
    [address((ee)0x0082401C, (aoc)0x00838030)]
    static block iat_form_set_help_text_popup_location;

    [import_slot("Low-Level Engine.dll", "?SetHotKeyGroupEnabled@UIForm@@QAEXK_N@Z")]
    [address((ee)0x0082447C, (aoc)0x0083847C)]
    static block iat_form_set_hot_key_group_enabled;

    [import_slot("Low-Level Engine.dll", "?SetMouseMiddleButtonPressedEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824044, (aoc)0x00838058)]
    static block iat_form_set_mouse_middle_button_pressed_event;

    [import_slot("Low-Level Engine.dll", "?SetMouseMiddleButtonUnpressedEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824040, (aoc)0x00838054)]
    static block iat_form_set_mouse_middle_button_unpressed_event;

    [import_slot("Low-Level Engine.dll", "?SetMouseMoveEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824288, (aoc)0x008382A0)]
    static block iat_form_set_mouse_move_event;

    [import_slot("Low-Level Engine.dll", "?SetMouseWheelMoveEvent@UIForm@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824060, (aoc)0x00838070)]
    static block iat_form_set_mouse_wheel_move_event;

    [import_slot("Low-Level Engine.dll", "?SetRectangle@UIForm@@MAEXABVGERectangle@@@Z")]
    [address((ee)0x0082400C, (aoc)0x00838020)]
    static block iat_form_set_rectangle;

    [import_slot("Low-Level Engine.dll", "?SetTabStop@UIForm@@QAE_NK@Z")]
    [address((ee)0x0082332C, (aoc)0x008381F0)]
    static block iat_form_set_tab_stop;

    [import_slot("Low-Level Engine.dll", "?SetTabbing@UIForm@@QAEX_N@Z")]
    [address((ee)0x00824380, (aoc)0x008383A0)]
    static block iat_form_set_tabbing;

    [import_slot("Low-Level Engine.dll", "?UpdateHelpString@UIForm@@QAEXXZ")]
    [address((ee)0x008232D4, (aoc)0x008372D0)]
    static block iat_form_update_help_string;

    [import_slot("Low-Level Engine.dll", "?UpdateInformation@UIForm@@UAEXXZ")]
    [address((ee)0x00824018, (aoc)0x0083802C)]
    static block iat_form_update_information;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIForm@@UAEJXZ")]
    [address((ee)0x008240A4, (aoc)0x008380B4)]
    static block iat_form_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?UpdateMouse@UIForm@@UAEJXZ")]
    [address((ee)0x008240A8, (aoc)0x008380B8)]
    static block iat_form_update_mouse;

    [import_slot("Low-Level Engine.dll", "?UpdateTranslatedKey@UIForm@@UAEJPBDF@Z")]
    [address((ee)0x00824014, (aoc)0x00838028)]
    static block iat_form_update_translated_key;

    [import_slot("Low-Level Engine.dll", "?DrawBackground@UIFullBackground@@UAEJABVGERectangle@@@Z")]
    [address((ee)0x00824084, (aoc)0x00838094)]
    static block iat_full_background_draw_background;

    [import_slot("Low-Level Engine.dll", "?LoadBackground@UIFullBackground@@UAEJABUGETextureDescription@@@Z")]
    [address((ee)0x00824088, (aoc)0x00838098)]
    static block iat_full_background_load_background;

    [import_slot("Low-Level Engine.dll", "?UnloadBackground@UIFullBackground@@UAEJXZ")]
    [address((ee)0x00824080, (aoc)0x00838090)]
    static block iat_full_background_unload_background;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIGraph@@UAEJXZ")]
    [address((ee)0x00824538, (aoc)0x00838538)]
    static block iat_graph_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawGridText@UIGraph@@UAEJXZ")]
    [address((ee)0x00824508, (aoc)0x00838508)]
    static block iat_graph_draw_grid_text;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIGraph@@UAEJXZ")]
    [address((ee)0x00824534, (aoc)0x00838534)]
    static block iat_graph_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?DrawTitleText@UIGraph@@UAEJXZ")]
    [address((ee)0x00824504, (aoc)0x00838504)]
    static block iat_graph_draw_title_text;

    [import_slot("Low-Level Engine.dll", "?DrawXAxisText@UIGraph@@UAEJXZ")]
    [address((ee)0x00824510, (aoc)0x00838510)]
    static block iat_graph_draw_x_axis_text;

    [import_slot("Low-Level Engine.dll", "?DrawYAxisText@UIGraph@@UAEJXZ")]
    [address((ee)0x0082450C, (aoc)0x0083850C)]
    static block iat_graph_draw_y_axis_text;

    [import_slot("Low-Level Engine.dll", "?GetGraph@UIGraph@@UBEABVUGraph@@XZ")]
    [address((ee)0x00824500, (aoc)0x00838500)]
    static block iat_graph_get_graph;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIGraph@@UAEJXZ")]
    [address((ee)0x00824530, (aoc)0x00838530)]
    static block iat_graph_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIGraph@@UAEJXZ")]
    [address((ee)0x0082452C, (aoc)0x0083852C)]
    static block iat_graph_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIGraph@@UAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00824528, (aoc)0x00838528)]
    static block iat_graph_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIGraph@@UAEJXZ")]
    [address((ee)0x00824524, (aoc)0x00838524)]
    static block iat_graph_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIGraph@@UAEJXZ")]
    [address((ee)0x00824520, (aoc)0x00838520)]
    static block iat_graph_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetFont@UIGraph@@UAEXPAVGEFont@@@Z")]
    [address((ee)0x008244FC, (aoc)0x008384FC)]
    static block iat_graph_set_font;

    [import_slot("Low-Level Engine.dll", "?SetFontColor@UIGraph@@QAEXABVGEColor@@@Z")]
    [address((ee)0x008244D4, (aoc)0x008384D4)]
    static block iat_graph_set_font_color;

    [import_slot("Low-Level Engine.dll", "?SetGraph@UIGraph@@UAEXABVUGraph@@@Z")]
    [address((ee)0x008244F8, (aoc)0x008384F8)]
    static block iat_graph_set_graph;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIGraph@@UAEXXZ")]
    [address((ee)0x00824518, (aoc)0x00838518)]
    static block iat_graph_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIGraph@@UAEXXZ")]
    [address((ee)0x0082451C, (aoc)0x0083851C)]
    static block iat_graph_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIGraph@@UAEJXZ")]
    [address((ee)0x00824514, (aoc)0x00838514)]
    static block iat_graph_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?UpdateMouse@UIGraph@@UAEJABVGE2DPoint@@@Z")]
    [address((ee)0x008244F4, (aoc)0x008384F4)]
    static block iat_graph_update_mouse;

    [import_slot("Low-Level Engine.dll", "?ZoomIn@UIGraph@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x008244F0, (aoc)0x008384F0)]
    static block iat_graph_zoom_in;

    [import_slot("Low-Level Engine.dll", "?ZoomOut@UIGraph@@UAEXXZ")]
    [address((ee)0x008244EC, (aoc)0x008384EC)]
    static block iat_graph_zoom_out;

    [import_slot("Low-Level Engine.dll", "?CalculateKeyCount@UIHotKey@@AAEXXZ")]
    [address((ee)0x0082405C, (aoc)0x0083806C)]
    static block iat_hot_key_calculate_key_count;

    [import_slot("Low-Level Engine.dll", "?GetHotKeyCount@UIHotKey@@SAKXZ")]
    [address((ee)0x0082449C, (aoc)0x0083849C)]
    static block iat_hot_key_get_hot_key_count;

    [import_slot("Low-Level Engine.dll", "?GetHotKeyName@UIHotKey@@QAEXAAVUWideString@@@Z")]
    [address((ee)0x008244A8, (aoc)0x008384A8)]
    static block iat_hot_key_get_hot_key_name;

    [import_slot("Low-Level Engine.dll", "?sHotKey@UIHotKey@@0V?$vector@PAVUIHotKey@@V?$allocator@PAVUIHotKey@@@std@@@std@@A")]
    [address((ee)0x00824498, (aoc)0x00838498)]
    static block iat_hot_key_s_hot_key;

    [import_slot("Low-Level Engine.dll", "?SetJustPressedEvent@UIHotKey@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824050, (aoc)0x00838060)]
    static block iat_hot_key_set_just_pressed_event;

    [import_slot("Low-Level Engine.dll", "?SetJustUnpressedEvent@UIHotKey@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x0082404C, (aoc)0x0083805C)]
    static block iat_hot_key_set_just_unpressed_event;

    [import_slot("Low-Level Engine.dll", "?SetModifier@UIHotKey@@QAEXKE@Z")]
    [address((ee)0x00824490, (aoc)0x00838490)]
    static block iat_hot_key_set_modifier;

    [import_slot("Low-Level Engine.dll", "?SetPressedEvent@UIHotKey@@QAEXPAVUIEvent@@@Z")]
    [address((ee)0x00824058, (aoc)0x00838068)]
    static block iat_hot_key_set_pressed_event;

    [import_slot("Low-Level Engine.dll", "?CalculateRectangle@UILayoutUtility@@QBEXABVGERectangle@@AAV2@_N@Z")]
    [address((ee)0x008240D4, (aoc)0x008380E4)]
    static block iat_layout_utility_calculate_rectangle;

    [import_slot("Low-Level Engine.dll", "?CopyData@UILBString@@UAEJXZ")]
    [address((ee)0x00823F50, (aoc)0x00837F64)]
    static block iat_lb_string_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UILBString@@UAEJXZ")]
    [address((ee)0x00823F48, (aoc)0x00837F5C)]
    static block iat_lb_string_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?UpdateTranslatedKey@UILBString@@UAEJPBDF@Z")]
    [address((ee)0x00823F18, (aoc)0x00837F2C)]
    static block iat_lb_string_update_translated_key;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UILineGraph@@UAEJXZ")]
    [address((ee)0x00824544, (aoc)0x00838544)]
    static block iat_line_graph_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawSeries@UILineGraph@@UAEJABVUGraphSeries@@@Z")]
    [address((ee)0x00824548, (aoc)0x00838548)]
    static block iat_line_graph_draw_series;

    [import_slot("Low-Level Engine.dll", "?AppendLine@?$UIListBox@VUWideString@@@@UAEXABVUWideString@@@Z")]
    [address((ee)0x00823F14, (aoc)0x00837F28)]
    static block iat_list_box_u_wide_string_append_line;

    [import_slot("Low-Level Engine.dll", "?AppendStringArray@?$UIListBox@VUWideString@@@@UAEXAAV?$vector@VUWideString@@V?$allocator@VUWideString@@@std@@@std@@@Z")]
    [address((ee)0x00823F10, (aoc)0x00837F24)]
    static block iat_list_box_u_wide_string_append_string_array;

    [import_slot("Low-Level Engine.dll", "?ButtonDown@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823EFC, (aoc)0x00837F10)]
    static block iat_list_box_u_wide_string_button_down;

    [import_slot("Low-Level Engine.dll", "?ButtonUp@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823EF8, (aoc)0x00837F0C)]
    static block iat_list_box_u_wide_string_button_up;

    [import_slot("Low-Level Engine.dll", "?ChangeSelectionIndex@?$UIListBox@VUWideString@@@@UAEXK_N0@Z")]
    [address((ee)0x00823F0C, (aoc)0x00837F20)]
    static block iat_list_box_u_wide_string_change_selection_index;

    [import_slot("Low-Level Engine.dll", "?ClearSelection@?$UIListBox@VUWideString@@@@UAEXXZ")]
    [address((ee)0x00823F08, (aoc)0x00837F1C)]
    static block iat_list_box_u_wide_string_clear_selection;

    [import_slot("Low-Level Engine.dll", "?DeleteAllEntries@?$UIListBox@VUWideString@@@@UAEXXZ")]
    [address((ee)0x00823F04, (aoc)0x00837F18)]
    static block iat_list_box_u_wide_string_delete_all_entries;

    [import_slot("Low-Level Engine.dll", "?DeleteLine@?$UIListBox@VUWideString@@@@UAEXK@Z")]
    [address((ee)0x00823F00, (aoc)0x00837F14)]
    static block iat_list_box_u_wide_string_delete_line;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@?$UIListBox@VUWideString@@@@UAEJXZ")]
    [address((ee)0x00823F4C, (aoc)0x00837F60)]
    static block iat_list_box_u_wide_string_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawScrollBarText@?$UIListBox@VUWideString@@@@IAEJXZ")]
    [address((ee)0x0082456C, (aoc)0x0083856C)]
    static block iat_list_box_u_wide_string_draw_scroll_bar_text;

    [import_slot("Low-Level Engine.dll", "?FindString@?$UIListBox@VUWideString@@@@QBE_NABVUWideString@@AAK@Z")]
    [address((ee)0x0082428C, (aoc)0x008382A4)]
    static block iat_list_box_u_wide_string_find_string;

    [import_slot("Low-Level Engine.dll", "?GetCurrentSelection@?$UIListBox@VUWideString@@@@QBEJAAK@Z")]
    [address((ee)0x008231D4, (aoc)0x008371D0)]
    static block iat_list_box_u_wide_string_get_current_selection;

    [import_slot("Low-Level Engine.dll", "?GetDisplayLine@?$UIListBox@VUWideString@@@@QBEKXZ")]
    [address((ee)0x008231D8, (aoc)0x008371D4)]
    static block iat_list_box_u_wide_string_get_display_line;

    [import_slot("Low-Level Engine.dll", "?GetLineCount@?$UIListBox@VUWideString@@@@QBEKXZ")]
    [address((ee)0x008231DC, (aoc)0x008371D8)]
    static block iat_list_box_u_wide_string_get_line_count;

    [import_slot("Low-Level Engine.dll", "?GetNumLines@?$UIListBox@VUWideString@@@@MBEKXZ")]
    [address((ee)0x00823EF4, (aoc)0x00837F08)]
    static block iat_list_box_u_wide_string_get_num_lines;

    [import_slot("Low-Level Engine.dll", "?InsertLine@?$UIListBox@VUWideString@@@@QAEXABVUWideString@@KAAV?$UILBNoSort@VUWideString@@@@@Z")]
    [address((ee)0x00823F88, (aoc)0x00837FA0)]
    static block iat_list_box_u_wide_string_insert_line;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDoublePressed@?$UIListBox@VUWideString@@@@MAEJXZ")]
    [address((ee)0x00823F3C, (aoc)0x00837F50)]
    static block iat_list_box_u_wide_string_mouse_l_button_double_pressed;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@?$UIListBox@VUWideString@@@@MAEJXZ")]
    [address((ee)0x00823F44, (aoc)0x00837F58)]
    static block iat_list_box_u_wide_string_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@?$UIListBox@VUWideString@@@@MAEJXZ")]
    [address((ee)0x00823F40, (aoc)0x00837F54)]
    static block iat_list_box_u_wide_string_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@?$UIListBox@VUWideString@@@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823F38, (aoc)0x00837F4C)]
    static block iat_list_box_u_wide_string_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@?$UIListBox@VUWideString@@@@MAEJXZ")]
    [address((ee)0x00823F34, (aoc)0x00837F48)]
    static block iat_list_box_u_wide_string_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@?$UIListBox@VUWideString@@@@MAEJXZ")]
    [address((ee)0x00823F30, (aoc)0x00837F44)]
    static block iat_list_box_u_wide_string_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseWheelTurned@?$UIListBox@VUWideString@@@@UAEJJ@Z")]
    [address((ee)0x00823F2C, (aoc)0x00837F40)]
    static block iat_list_box_u_wide_string_mouse_wheel_turned;

    [import_slot("Low-Level Engine.dll", "?ProcessDown@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823EF0, (aoc)0x00837F04)]
    static block iat_list_box_u_wide_string_process_down;

    [import_slot("Low-Level Engine.dll", "?ProcessEnd@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823EEC, (aoc)0x00837F00)]
    static block iat_list_box_u_wide_string_process_end;

    [import_slot("Low-Level Engine.dll", "?ProcessHome@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823EE8, (aoc)0x00837EFC)]
    static block iat_list_box_u_wide_string_process_home;

    [import_slot("Low-Level Engine.dll", "?ProcessUp@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823EE4, (aoc)0x00837EF8)]
    static block iat_list_box_u_wide_string_process_up;

    [import_slot("Low-Level Engine.dll", "?SetLocation@?$UIListBox@VUWideString@@@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823F28, (aoc)0x00837F3C)]
    static block iat_list_box_u_wide_string_set_location;

    [import_slot("Low-Level Engine.dll", "?SetScrollBarBgColor@?$UIListBox@VUWideString@@@@QAEXABVGEColor@@@Z")]
    [address((ee)0x00824568, (aoc)0x00838568)]
    static block iat_list_box_u_wide_string_set_scroll_bar_bg_color;

    [import_slot("Low-Level Engine.dll", "?SetScrollBarBgOpacity@?$UIListBox@VUWideString@@@@QAEXM@Z")]
    [address((ee)0x00823E5C, (aoc)0x00837E64)]
    static block iat_list_box_u_wide_string_set_scroll_bar_bg_opacity;

    [import_slot("Low-Level Engine.dll", "?Sort@?$UIListBox@VUWideString@@@@QAEXXZ")]
    [address((ee)0x00824570, (aoc)none)]
    static block iat_list_box_u_wide_string_sort;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823F20, (aoc)0x00837F34)]
    static block iat_list_box_u_wide_string_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@?$UIListBox@VUWideString@@@@MAEXXZ")]
    [address((ee)0x00823F24, (aoc)0x00837F38)]
    static block iat_list_box_u_wide_string_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@?$UIListBox@VUWideString@@@@UAEJXZ")]
    [address((ee)0x00823F1C, (aoc)0x00837F30)]
    static block iat_list_box_u_wide_string_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?AddDirtyRectangle@UIManager@@QAEXABVGERectangle@@PAVUIForm@@@Z")]
    [address((ee)0x00824008, (aoc)0x0083801C)]
    static block iat_manager_add_dirty_rectangle;

    [import_slot("Low-Level Engine.dll", "?AddIMEResultString@UIManager@@QAEXABVUWideString@@@Z")]
    [address((ee)0x00824554, (aoc)0x00838554)]
    static block iat_manager_add_ime_result_string;

    [import_slot("Low-Level Engine.dll", "?ChangeIMECandidateWindow@UIManager@@UAEJABUUIIMECandidateList@@@Z")]
    [address((ee)0x008244C4, (aoc)0x008384C4)]
    static block iat_manager_change_ime_candidate_window;

    [import_slot("Low-Level Engine.dll", "?ChangeIMECompositionString@UIManager@@UAEJABVUWideString@@ABVUString@@K@Z")]
    [address((ee)0x008244C0, (aoc)0x008384C0)]
    static block iat_manager_change_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?CloseIMECandidateWindow@UIManager@@UAEJXZ")]
    [address((ee)0x008244BC, (aoc)0x008384BC)]
    static block iat_manager_close_ime_candidate_window;

    [import_slot("Low-Level Engine.dll", "?CloseIMECompositionString@UIManager@@UAEJXZ")]
    [address((ee)0x008244B8, (aoc)0x008384B8)]
    static block iat_manager_close_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIManager@@QAEJXZ")]
    [address((ee)0x008238B0, (aoc)0x0083788C)]
    static block iat_manager_copy_data;

    [import_slot("Low-Level Engine.dll", "?CreateForm@UIManager@@UAEPAVUIForm@@ABUUIDbDataForms@@@Z")]
    [address((ee)0x008244E0, (aoc)0x008384E0)]
    static block iat_manager_create_form;

    [import_slot("Low-Level Engine.dll", "?CreateIMECandidateWindow@UIManager@@QAEXPAVGEFont@@@Z")]
    [address((ee)0x0082455C, (aoc)0x0083855C)]
    static block iat_manager_create_ime_candidate_window;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIManager@@QAEJXZ")]
    [address((ee)0x008238B4, (aoc)0x00837890)]
    static block iat_manager_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIManager@@QAEJXZ")]
    [address((ee)0x008238B8, (aoc)0x00837894)]
    static block iat_manager_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?IsFormOnStack@UIManager@@QAE_NPAVUIForm@@@Z")]
    [address((ee)0x00824558, (aoc)0x00838558)]
    static block iat_manager_is_form_on_stack;

    [import_slot("Low-Level Engine.dll", "?IsFormVisible@UIManager@@QAE_NPAVUIForm@@@Z")]
    [address((ee)0x00823450, (aoc)0x008373EC)]
    static block iat_manager_is_form_visible;

    [import_slot("Low-Level Engine.dll", "?LoadDatabase@UIManager@@QAEXPAVFSPath@@ABVUString@@@Z")]
    [address((ee)0x008244C8, (aoc)0x008384C8)]
    static block iat_manager_load_database;

    [import_slot("Low-Level Engine.dll", "?OpenIMECandidateWindow@UIManager@@UAEJXZ")]
    [address((ee)0x008244B4, (aoc)0x008384B4)]
    static block iat_manager_open_ime_candidate_window;

    [import_slot("Low-Level Engine.dll", "?OpenIMECompositionString@UIManager@@UAEJXZ")]
    [address((ee)0x008244B0, (aoc)0x008384B0)]
    static block iat_manager_open_ime_composition_string;

    [import_slot("Low-Level Engine.dll", "?Pop@UIManager@@QAEJXZ")]
    [address((ee)0x008231A0, (aoc)0x0083719C)]
    static block iat_manager_pop;

    [import_slot("Low-Level Engine.dll", "?PopTo@UIManager@@QAEJPAVUIForm@@@Z")]
    [address((ee)0x00823670, (aoc)0x00837614)]
    static block iat_manager_pop_to;

    [import_slot("Low-Level Engine.dll", "?Push@UIManager@@QAEJPAVUIForm@@@Z")]
    [address((ee)0x0082328C, (aoc)0x00837288)]
    static block iat_manager_push;

    [import_slot("Low-Level Engine.dll", "?RedrawAll@UIManager@@QAEXXZ")]
    [address((ee)0x00823804, (aoc)0x008377D4)]
    static block iat_manager_redraw_all;

    [import_slot("Low-Level Engine.dll", "?RelayoutAllForms@UIManager@@QAEXXZ")]
    [address((ee)0x0082454C, (aoc)0x0083854C)]
    static block iat_manager_relayout_all_forms;

    [import_slot("Low-Level Engine.dll", "?sLMouseButton@UIManager@@0PAVISEntity@@A")]
    [address((ee)0x00823F70, (aoc)0x00837F84)]
    static block iat_manager_s_l_mouse_button;

    [import_slot("Low-Level Engine.dll", "?sRMouseButton@UIManager@@0PAVISEntity@@A")]
    [address((ee)0x00823FE4, (aoc)0x00837FF8)]
    static block iat_manager_s_r_mouse_button;

    [import_slot("Low-Level Engine.dll", "?SetMouseLeftHanded@UIManager@@SAX_N@Z")]
    [address((ee)0x00823AA0, (aoc)0x00837A90)]
    static block iat_manager_set_mouse_left_handed;

    [import_slot("Low-Level Engine.dll", "?SetMusicVolume@UIManager@@QAEXM@Z")]
    [address((ee)0x00824280, (aoc)0x00838298)]
    static block iat_manager_set_music_volume;

    [import_slot("Low-Level Engine.dll", "?SwapForm@UIManager@@QAEJPAVUIForm@@@Z")]
    [address((ee)0x00823598, (aoc)0x0083753C)]
    static block iat_manager_swap_form;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIManager@@MAEJXZ")]
    [address((ee)0x008244E4, (aoc)0x008384E4)]
    static block iat_manager_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?UpdateMouse@UIManager@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x008244E8, (aoc)0x008384E8)]
    static block iat_manager_update_mouse;

    [import_slot("Low-Level Engine.dll", "?UpdateTranslatedKey@UIManager@@QAEJPBDF@Z")]
    [address((ee)0x00824564, (aoc)0x00838564)]
    static block iat_manager_update_translated_key;

    [import_slot("Low-Level Engine.dll", "?UpdateUI@UIManager@@UAEJABVGE2DPoint@@@Z")]
    [address((ee)0x008244AC, (aoc)0x008384AC)]
    static block iat_manager_update_ui;

    [import_slot("Low-Level Engine.dll", "?ButtonUp@UIRadioButton@@MAEXXZ")]
    [address((ee)0x00823D6C, (aoc)0x00837D70)]
    static block iat_radio_button_button_up;

    [import_slot("Low-Level Engine.dll", "?Check@UIRadioButton@@UAEXXZ")]
    [address((ee)0x00823DB0, (aoc)0x00837DB4)]
    static block iat_radio_button_check;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIRadioButton@@UAEJXZ")]
    [address((ee)0x00823D94, (aoc)0x00837D98)]
    static block iat_radio_button_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIRadioButton@@UAEJXZ")]
    [address((ee)0x00823D90, (aoc)0x00837D94)]
    static block iat_radio_button_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIRadioButton@@UAEJXZ")]
    [address((ee)0x00823D8C, (aoc)0x00837D90)]
    static block iat_radio_button_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetTabable@UIRadioButton@@UBE_NXZ")]
    [address((ee)0x00823D88, (aoc)0x00837D8C)]
    static block iat_radio_button_get_tabable;

    [import_slot("Low-Level Engine.dll", "?GetType@UIRadioButton@@UBEKXZ")]
    [address((ee)0x00823D84, (aoc)0x00837D88)]
    static block iat_radio_button_get_type;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIRadioButton@@MAEJXZ")]
    [address((ee)0x00823D80, (aoc)0x00837D84)]
    static block iat_radio_button_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIRadioButton@@MAEJXZ")]
    [address((ee)0x00823D7C, (aoc)0x00837D80)]
    static block iat_radio_button_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?SetDownCheckedBitmap@UIRadioButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823DA4, (aoc)0x00837DA8)]
    static block iat_radio_button_set_down_checked_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetDownUncheckedBitmap@UIRadioButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823D9C, (aoc)0x00837DA0)]
    static block iat_radio_button_set_down_unchecked_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIRadioButton@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823D78, (aoc)0x00837D7C)]
    static block iat_radio_button_set_location;

    [import_slot("Low-Level Engine.dll", "?SetText@UIRadioButton@@QAEXABVUWideString@@PAVGEFont@@@Z")]
    [address((ee)0x00823F84, (aoc)0x00837F9C)]
    static block iat_radio_button_set_text;

    [import_slot("Low-Level Engine.dll", "?SetUpCheckedBitmap@UIRadioButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823DA0, (aoc)0x00837DA4)]
    static block iat_radio_button_set_up_checked_bitmap;

    [import_slot("Low-Level Engine.dll", "?SetUpUncheckedBitmap@UIRadioButton@@QAEXPAVGEBitmap@@@Z")]
    [address((ee)0x00823D98, (aoc)0x00837D9C)]
    static block iat_radio_button_set_up_unchecked_bitmap;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIRadioButton@@MAEXXZ")]
    [address((ee)0x00823D70, (aoc)0x00837D74)]
    static block iat_radio_button_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIRadioButton@@MAEXXZ")]
    [address((ee)0x00823D74, (aoc)0x00837D78)]
    static block iat_radio_button_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UnCheck@UIRadioButton@@MAEXXZ")]
    [address((ee)0x00823D68, (aoc)0x00837D6C)]
    static block iat_radio_button_un_check;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIScrollBar@@UAEJXZ")]
    [address((ee)0x00824218, (aoc)0x0083822C)]
    static block iat_scroll_bar_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIScrollBar@@UAEJXZ")]
    [address((ee)0x00824214, (aoc)0x00838228)]
    static block iat_scroll_bar_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIScrollBar@@UAEJXZ")]
    [address((ee)0x00824210, (aoc)0x00838224)]
    static block iat_scroll_bar_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetTabable@UIScrollBar@@UBE_NXZ")]
    [address((ee)0x0082420C, (aoc)0x00838220)]
    static block iat_scroll_bar_get_tabable;

    [import_slot("Low-Level Engine.dll", "?IncrementBy@UIScrollBar@@QAEXJ@Z")]
    [address((ee)0x008241C4, (aoc)0x008381D4)]
    static block iat_scroll_bar_increment_by;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIScrollBar@@UAEJXZ")]
    [address((ee)0x00824208, (aoc)0x0083821C)]
    static block iat_scroll_bar_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIScrollBar@@UAEJXZ")]
    [address((ee)0x00824204, (aoc)0x00838218)]
    static block iat_scroll_bar_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIScrollBar@@UAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00824200, (aoc)0x00838214)]
    static block iat_scroll_bar_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIScrollBar@@UAEJXZ")]
    [address((ee)0x008241FC, (aoc)0x00838210)]
    static block iat_scroll_bar_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIScrollBar@@UAEJXZ")]
    [address((ee)0x008241F8, (aoc)0x0083820C)]
    static block iat_scroll_bar_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetBgColor@UIScrollBar@@UAEXABVGEColor@@@Z")]
    [address((ee)0x008241F4, (aoc)0x00838208)]
    static block iat_scroll_bar_set_bg_color;

    [import_slot("Low-Level Engine.dll", "?SetBgOpacity@UIScrollBar@@UAEXM@Z")]
    [address((ee)0x008241F0, (aoc)0x00838204)]
    static block iat_scroll_bar_set_bg_opacity;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIScrollBar@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x008241EC, (aoc)0x00838200)]
    static block iat_scroll_bar_set_location;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIScrollBar@@UAEXXZ")]
    [address((ee)0x008241E4, (aoc)0x008381F8)]
    static block iat_scroll_bar_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIScrollBar@@UAEXXZ")]
    [address((ee)0x008241E8, (aoc)0x008381FC)]
    static block iat_scroll_bar_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIScrollBar@@UAEJXZ")]
    [address((ee)0x008241E0, (aoc)0x008381F4)]
    static block iat_scroll_bar_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?ButtonDown@UISlider@@MAEXXZ")]
    [address((ee)0x00823E78, (aoc)0x00837E84)]
    static block iat_slider_button_down;

    [import_slot("Low-Level Engine.dll", "?ButtonUp@UISlider@@MAEXXZ")]
    [address((ee)0x00823E74, (aoc)0x00837E80)]
    static block iat_slider_button_up;

    [import_slot("Low-Level Engine.dll", "?ChangeRange@UISlider@@MAEXK@Z")]
    [address((ee)0x00823E70, (aoc)0x00837E7C)]
    static block iat_slider_change_range;

    [import_slot("Low-Level Engine.dll", "?CopyData@UISlider@@UAEJXZ")]
    [address((ee)0x00823EAC, (aoc)0x00837EB8)]
    static block iat_slider_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UISlider@@UAEJXZ")]
    [address((ee)0x00823EA8, (aoc)0x00837EB4)]
    static block iat_slider_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?GetTabable@UISlider@@UBE_NXZ")]
    [address((ee)0x00823EA0, (aoc)0x00837EAC)]
    static block iat_slider_get_tabable;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UISlider@@MAEJXZ")]
    [address((ee)0x00823E9C, (aoc)0x00837EA8)]
    static block iat_slider_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UISlider@@MAEJXZ")]
    [address((ee)0x00823E98, (aoc)0x00837EA4)]
    static block iat_slider_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UISlider@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823E94, (aoc)0x00837EA0)]
    static block iat_slider_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UISlider@@MAEJXZ")]
    [address((ee)0x00823E90, (aoc)0x00837E9C)]
    static block iat_slider_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UISlider@@MAEJXZ")]
    [address((ee)0x00823E8C, (aoc)0x00837E98)]
    static block iat_slider_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetCurrentRangeValue@UISlider@@QAEXJ@Z")]
    [address((ee)0x008245B8, (aoc)0x00837D54)]
    static block iat_slider_set_current_range_value;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UISlider@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823E88, (aoc)0x00837E94)]
    static block iat_slider_set_location;

    [import_slot("Low-Level Engine.dll", "?SetRange@UISlider@@QAEXKK@Z")]
    [address((ee)0x00823EBC, (aoc)0x00837EC8)]
    static block iat_slider_set_range;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UISlider@@MAEXXZ")]
    [address((ee)0x00823E80, (aoc)0x00837E8C)]
    static block iat_slider_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UISlider@@MAEXXZ")]
    [address((ee)0x00823E84, (aoc)0x00837E90)]
    static block iat_slider_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UISlider@@UAEJXZ")]
    [address((ee)0x00823E7C, (aoc)0x00837E88)]
    static block iat_slider_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?ChangeFont@UIStatusBar@@UAEXPAVGEFont@@@Z")]
    [address((ee)0x00824324, (aoc)0x00838340)]
    static block iat_status_bar_change_font;

    [import_slot("Low-Level Engine.dll", "?CopyData@UIStatusBar@@UAEJXZ")]
    [address((ee)0x00824320, (aoc)0x0083833C)]
    static block iat_status_bar_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UIStatusBar@@UAEJXZ")]
    [address((ee)0x0082431C, (aoc)0x00838338)]
    static block iat_status_bar_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UIStatusBar@@UAEJXZ")]
    [address((ee)0x00824318, (aoc)0x00838334)]
    static block iat_status_bar_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetTabable@UIStatusBar@@UBE_NXZ")]
    [address((ee)0x00824314, (aoc)0x00838330)]
    static block iat_status_bar_get_tabable;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UIStatusBar@@MAEJXZ")]
    [address((ee)0x00824310, (aoc)0x0083832C)]
    static block iat_status_bar_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UIStatusBar@@MAEJXZ")]
    [address((ee)0x0082430C, (aoc)0x00838328)]
    static block iat_status_bar_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UIStatusBar@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00824308, (aoc)0x00838324)]
    static block iat_status_bar_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UIStatusBar@@MAEJXZ")]
    [address((ee)0x00824304, (aoc)0x00838320)]
    static block iat_status_bar_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UIStatusBar@@MAEJXZ")]
    [address((ee)0x00824300, (aoc)0x0083831C)]
    static block iat_status_bar_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?SetCurrentRangeValue@UIStatusBar@@QAEXJ@Z")]
    [address((ee)0x008242E0, (aoc)0x008382FC)]
    static block iat_status_bar_set_current_range_value;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UIStatusBar@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x008242FC, (aoc)0x00838318)]
    static block iat_status_bar_set_location;

    [import_slot("Low-Level Engine.dll", "?SetRange@UIStatusBar@@QAEXKK@Z")]
    [address((ee)0x0082432C, (aoc)0x00838348)]
    static block iat_status_bar_set_range;

    [import_slot("Low-Level Engine.dll", "?SetText@UIStatusBar@@QAEXABVUWideString@@PAVGEFont@@@Z")]
    [address((ee)0x00824334, (aoc)0x00838350)]
    static block iat_status_bar_set_text;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UIStatusBar@@MAEXXZ")]
    [address((ee)0x008242F4, (aoc)0x00838310)]
    static block iat_status_bar_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UIStatusBar@@MAEXXZ")]
    [address((ee)0x008242F8, (aoc)0x00838314)]
    static block iat_status_bar_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIStatusBar@@UAEJXZ")]
    [address((ee)0x008242F0, (aoc)0x0083830C)]
    static block iat_status_bar_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?AppendText@UITextLabel@@UAEXABVUWideString@@@Z")]
    [address((ee)0x00823CB4, (aoc)0x00837CB0)]
    static block iat_text_label_append_text;

    [import_slot("Low-Level Engine.dll", "?ChangeFont@UITextLabel@@UAEXPAVGEFont@@@Z")]
    [address((ee)0x00823D30, (aoc)0x00837D2C)]
    static block iat_text_label_change_font;

    [import_slot("Low-Level Engine.dll", "?CheckScrollBar@UITextLabel@@MAEXXZ")]
    [address((ee)0x00823CAC, (aoc)0x00837CA8)]
    static block iat_text_label_check_scroll_bar;

    [import_slot("Low-Level Engine.dll", "?CopyData@UITextLabel@@UAEJXZ")]
    [address((ee)0x00823D28, (aoc)0x00837D24)]
    static block iat_text_label_copy_data;

    [import_slot("Low-Level Engine.dll", "?DeleteArray@UITextLabel@@QAEXXZ")]
    [address((ee)0x00824250, (aoc)0x00838264)]
    static block iat_text_label_delete_array;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UITextLabel@@UAEJXZ")]
    [address((ee)0x00823D24, (aoc)0x00837D20)]
    static block iat_text_label_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UITextLabel@@UAEJXZ")]
    [address((ee)0x00823D20, (aoc)0x00837D1C)]
    static block iat_text_label_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetLineCount@UITextLabel@@QBEKXZ")]
    [address((ee)0x00824274, (aoc)0x00838288)]
    static block iat_text_label_get_line_count;

    [import_slot("Low-Level Engine.dll", "?GetTabable@UITextLabel@@UBE_NXZ")]
    [address((ee)0x00823D14, (aoc)0x00837D10)]
    static block iat_text_label_get_tabable;

    [import_slot("Low-Level Engine.dll", "?GetTextAsInt@UITextLabel@@QBEJK@Z")]
    [address((ee)0x00823F9C, (aoc)0x00837FB4)]
    static block iat_text_label_get_text_as_int;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonDown@UITextLabel@@MAEJXZ")]
    [address((ee)0x00823D08, (aoc)0x00837D04)]
    static block iat_text_label_mouse_l_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseLButtonUp@UITextLabel@@MAEJXZ")]
    [address((ee)0x00823D04, (aoc)0x00837D00)]
    static block iat_text_label_mouse_l_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseMove@UITextLabel@@MAEJABVGE2DPoint@@@Z")]
    [address((ee)0x00823CFC, (aoc)0x00837CF8)]
    static block iat_text_label_mouse_move;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonDown@UITextLabel@@MAEJXZ")]
    [address((ee)0x00823CF8, (aoc)0x00837CF4)]
    static block iat_text_label_mouse_r_button_down;

    [import_slot("Low-Level Engine.dll", "?MouseRButtonUp@UITextLabel@@MAEJXZ")]
    [address((ee)0x00823CF4, (aoc)0x00837CF0)]
    static block iat_text_label_mouse_r_button_up;

    [import_slot("Low-Level Engine.dll", "?MouseWheelTurned@UITextLabel@@UAEJJ@Z")]
    [address((ee)0x00823CF0, (aoc)0x00837CEC)]
    static block iat_text_label_mouse_wheel_turned;

    [import_slot("Low-Level Engine.dll", "?RecomputeDisplayRange@UITextLabel@@UAEXXZ")]
    [address((ee)0x00823CB0, (aoc)0x00837CAC)]
    static block iat_text_label_recompute_display_range;

    [import_slot("Low-Level Engine.dll", "?SetFitRectToText@UITextLabel@@QAEX_N@Z")]
    [address((ee)0x00824064, (aoc)0x00838074)]
    static block iat_text_label_set_fit_rect_to_text;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UITextLabel@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823CD0, (aoc)0x00837CCC)]
    static block iat_text_label_set_location;

    [import_slot("Low-Level Engine.dll", "?SetRectangle@UITextLabel@@UAEXABVGERectangle@@@Z")]
    [address((ee)0x00823CCC, (aoc)0x00837CC8)]
    static block iat_text_label_set_rectangle;

    [import_slot("Low-Level Engine.dll", "?SetScrollBarBgColor@UITextLabel@@QAEXABVGEColor@@@Z")]
    [address((ee)0x00824458, (aoc)0x00838458)]
    static block iat_text_label_set_scroll_bar_bg_color;

    [import_slot("Low-Level Engine.dll", "?SetScrollBarBgOpacity@UITextLabel@@QAEXM@Z")]
    [address((ee)0x00824454, (aoc)0x00838454)]
    static block iat_text_label_set_scroll_bar_bg_opacity;

    [import_slot("Low-Level Engine.dll", "?SetText@UITextLabel@@AAEXABVUWideString@@_N@Z")]
    [address((ee)0x008232DC, (aoc)0x008372E0)]
    static block iat_text_label_set_text;

    [import_slot("Low-Level Engine.dll", "?SetText@UITextLabel@@QAEXJ@Z")]
    [address((ee)0x00823EB8, (aoc)0x00837EC4)]
    static block iat_text_label_set_text_2;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UITextLabel@@MAEXXZ")]
    [address((ee)0x00823CC4, (aoc)0x00837CC0)]
    static block iat_text_label_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UITextLabel@@MAEXXZ")]
    [address((ee)0x00823CC8, (aoc)0x00837CC4)]
    static block iat_text_label_tabbed_to;

    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UITextLabel@@UAEJXZ")]
    [address((ee)0x00823CC0, (aoc)0x00837CBC)]
    static block iat_text_label_update_keyboard;

    [import_slot("Low-Level Engine.dll", "?PauseScrolling@UITLScrolling@@QAEXXZ")]
    [address((ee)0x00824474, (aoc)0x00838474)]
    static block iat_tl_scrolling_pause_scrolling;

    [import_slot("Low-Level Engine.dll", "?ResumeScrolling@UITLScrolling@@QAEXXZ")]
    [address((ee)0x00824478, (aoc)0x00838478)]
    static block iat_tl_scrolling_resume_scrolling;

    [import_slot("Low-Level Engine.dll", "?ScrollText@UITLScrolling@@QAEXKK@Z")]
    [address((ee)0x0082446C, (aoc)0x0083846C)]
    static block iat_tl_scrolling_scroll_text;

    [import_slot("Low-Level Engine.dll", "?StopScrolling@UITLScrolling@@QAEXXZ")]
    [address((ee)0x00824480, (aoc)0x00838480)]
    static block iat_tl_scrolling_stop_scrolling;

    [import_slot("Low-Level Engine.dll", "?ButtonUp@UITriStateCheckBox@@MAEXXZ")]
    [address((ee)0x00823FA0, (aoc)0x00837FB8)]
    static block iat_tri_state_check_box_button_up;

    [import_slot("Low-Level Engine.dll", "?CopyData@UITriStateCheckBox@@UAEJXZ")]
    [address((ee)0x00823FBC, (aoc)0x00837FD4)]
    static block iat_tri_state_check_box_copy_data;

    [import_slot("Low-Level Engine.dll", "?DrawPolygons@UITriStateCheckBox@@UAEJXZ")]
    [address((ee)0x00823FB8, (aoc)0x00837FD0)]
    static block iat_tri_state_check_box_draw_polygons;

    [import_slot("Low-Level Engine.dll", "?DrawTextA@UITriStateCheckBox@@UAEJXZ")]
    [address((ee)0x00823FB4, (aoc)0x00837FCC)]
    static block iat_tri_state_check_box_draw_text_a;

    [import_slot("Low-Level Engine.dll", "?GetType@UITriStateCheckBox@@UBEKXZ")]
    [address((ee)0x00823FB0, (aoc)0x00837FC8)]
    static block iat_tri_state_check_box_get_type;

    [import_slot("Low-Level Engine.dll", "?SetCheckState@UITriStateCheckBox@@QAEXK@Z")]
    [address((ee)0x00823FE0, (aoc)0x00837FF4)]
    static block iat_tri_state_check_box_set_check_state;

    [import_slot("Low-Level Engine.dll", "?SetLocation@UITriStateCheckBox@@UAEXABVGE2DPoint@@@Z")]
    [address((ee)0x00823FAC, (aoc)0x00837FC4)]
    static block iat_tri_state_check_box_set_location;

    [import_slot("Low-Level Engine.dll", "?TabbedFrom@UITriStateCheckBox@@MAEXXZ")]
    [address((ee)0x00823FA4, (aoc)0x00837FBC)]
    static block iat_tri_state_check_box_tabbed_from;

    [import_slot("Low-Level Engine.dll", "?TabbedTo@UITriStateCheckBox@@MAEXXZ")]
    [address((ee)0x00823FA8, (aoc)0x00837FC0)]
    static block iat_tri_state_check_box_tabbed_to;
}
