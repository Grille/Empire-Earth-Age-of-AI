// THE SETTINGS: the keys of ageofai.ini as C# declarations - a class per section (general.cs, display.cs ...), a field
// per key in the shipped file's order, its /// lines the comment above it in the file (the documentation the player
// reads, and ageofai_config.exe's help). Declared once here, read by scripts/build/settings_read.py: gen_settings.py
// (run by dll/build.sh) makes dll/settings/rows.h and the shipped config/ageofai.ini of it - both generated, never
// edited (one changed by hand stops the build: its change belongs here). What a setting does stays the code's (it
// reads g_set.key, g_set.section_key, g_set_ai[d].key - rows.h's header); a declaration is its name, kind, default,
// limits and doc. C# is only how they are written down: names and types checked (settings.csproj; dotnet build
// there). Nothing of it runs. A new setting: a field here, with its lines for the file, then the build.
//
// A SECTION is a class with [section(place)] - place: ageofai_config.exe's order, its page's title the class's name
// (camera: Camera) or [title("t")]; [all] the
// AI's rows ([ai_base]: a value per difficulty), [diff] a difficulty's own section ([ai_easy], [ai_hard]: its fields
// are [ai_base] keys with the difficulty's value, written bool, int or "text" - [ai_easy]'s are also what the easy AI
// gets where the player's file says nothing), [test] the test ini's own (not in ageofai.ini). Its fields in the
// file's order.
//
// A GROUP is a class inside a section (PascalCase, apart from the keys: CitizensUnderAttack): ageofai_config.exe's
// topic, its title the name's words (Citizens under attack; a word in capitals stays: AIChat - AI chat) or
// [title("t")]. Its keys in it; keys before the first group belong to none. Enums and structs stay in the section.
// A key's name stays the key alone - a group or section is only where it is shown.
//
// A KEY is a field; its type and attributes are its kind (rows.h's header tells each):
//   bool k = true;                               FLAG     (in the file 1 / 0)
//   [range(lo, hi)] int k = 4096;                INT      a whole number lo .. hi
//   [num] int k = 7;                             NUM      any whole number ([range] too: its bounds)
//   [pct(hi)] int k = 80;                        PCT      whole percents up to hi% (temperature=80%)
//   [share(lo, hi)] int k = 35;                  SHARE    a share kept as a float, in the file in percent
//   kind k = kind.auto;                          CHOICE   an enum of the section: its members the names, in order
//                                                         ([name("0")] a member whose name C# can't take)
//   [length(n)] string k = "";                   STR      a text shorter than n
//   [per_player, length(n)] string k_P = "";     STR_P    a text per player slot (k_P0 .. k_P15)
//   [child(nameof(master))] bool k = true;       CHILD    follows the master CHOICE when not set
//   k_fields k;                                  FLAGS    a struct of the section: bool fields and enum fields; its
//                                                         default all at their first ([on("a, b")]: those on)
//
// THE FILE, line for line: a key's /// lines are the file's lines above it - "/// text" the line "; text", an empty
// "///" a blank line (headings like "; --- Attacks ---" are lines as any other); several keys under one comment: the
// lines on the first, none on the others. The /// lines before a class are the file's lines before its [section]
// header (the banners), those after its last key the lines after it. [commented] the key written with a ';' before
// it (a value to try, not in force: ;diag=0), [commented("x")] with x instead of the default
// (;ai_dir=_aimod\config\personalities), [commented("x", "key_P2")] as another key (one slot of a per-player key).
//
// MORE ABOUT A KEY: [game("what", "original", keep)] it sets one of the game's own values (SETTING_GAME: the tool shows its key blue);
// [under(nameof(master))] the tool folds it under a master it doesn't follow (SETTING_UNDER); [also("section")] the tool
// lists it on that section's page too (SETTING_ALSO) - the key itself stays in its own section, once in the file.

using System;

[AttributeUsage(AttributeTargets.Class)] public sealed class section : Attribute { public section(int place) { } }
[AttributeUsage(AttributeTargets.Class)] public sealed class all : Attribute { }
[AttributeUsage(AttributeTargets.Class)] public sealed class diff : Attribute { }
[AttributeUsage(AttributeTargets.Class)] public sealed class test : Attribute { }
[AttributeUsage(AttributeTargets.Field)] public sealed class range : Attribute { public range(int lo, int hi) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class num : Attribute { }
[AttributeUsage(AttributeTargets.Field)] public sealed class pct : Attribute { public pct(int hi) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class share : Attribute { public share(int lo, int hi) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class length : Attribute { public length(int n) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class per_player : Attribute { }
[AttributeUsage(AttributeTargets.Field)] public sealed class child : Attribute { public child(string master) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class name : Attribute { public name(string text) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class commented : Attribute { public commented(string value = "", string key = "") { } }
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Field)] public sealed class title : Attribute { public title(string text) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class game : Attribute { public game(string what, string original, int keep) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class under : Attribute { public under(string master) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class also : Attribute { public also(string section) { } }
[AttributeUsage(AttributeTargets.Field)] public sealed class on : Attribute { public on(string fields) { } }

static class limits {
    public const int MAX_PATH = 260;   // windows.h's: a path's buffer
}
