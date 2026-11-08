// THE SETTINGS: the keys of ageofai.ini as C# declarations - a class per section (general.cs, display.cs ...), a field
// per key in the shipped file's order, its /// lines the comment above it in the file (the documentation the player
// reads, and ageofai_config.exe's help). Declared once here, read by scripts/build/settings_read.py: gen_settings.py
// (run by build.sh) makes dll/settings/rows.h and the shipped config/ageofai.ini of it - both generated, never
// edited (one changed by hand stops the build: its change belongs here). What a setting does stays the code's (it
// reads g_set.key, g_set.section_key, g_set_ai[d].key - rows.h's header); a declaration is its name, kind, default,
// limits and doc. C# is only how they are written down: names and types checked (dll_cs/dll_cs.csproj; dotnet build
// in dll_cs/; the types and the attributes shared with the language: dll_cs/language.cs). Nothing of it runs. A new setting: a field here, with its lines for the file, then the build.
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
// [title("t")]. Its keys in it; keys before the first group belong to none.
// An enum or struct stands directly above the key it is the type of, before that key's /// lines (in its group too):
// a key and its values read together. Its name is one of the section's (a second one in another group is a mistake).
// A key's name stays the key alone - a group or section is only where it is shown.
//
// A MASTER is a group with [master]: one key of its own (the class's name: ImprovedUnitMovement improved_unit_movement,
// its /// lines the class's) over every key in it, which are bool, factor or int only:
//   default        = each its shipped default (a key set in the file is told and not used)
//   file           = each as the file sets it, else its default
//   default | file = a bool on when its default or the file's is on, a factor or int as file
//   default & file = a bool on when its default and the file's are on, a factor or int as file
// (scripts/build/settings_read.py MASTER_VALUES; settings.c setting_member)
//
// A KEY is a field; its type and attributes are its kind (rows.h's header tells each):
//   bool k = true;                               FLAG     (in the file 1 / 0)
//   [range(lo, hi)] int k = 4096;                INT      a whole number lo .. hi (outside: a warning, used)
//   [range(lo, hi, error: true)] int k = 32;     LIMIT    a whole number lo .. hi, nothing else (outside: a mistake)
//   int k = 7;                                   NUM      any whole number (a level, a bit mask, a count)
//   [range(0.25, 1)] share k = 0.35;             SHARE    a share (dll_cs/language.cs), in the file in percent: 35%
//   kind k = kind.auto;                          CHOICE   an enum of the section: its members the names, in order
//                                                         ([name("0")] a member whose name C# can't take)
//   [length(n)] string k = "";                   STR      a text shorter than n
//   [per_player, length(n)] string k_P = "";     STR_P    a text per player slot (k_P0 .. k_P15)
//   [range(1, 3)] factor k = 1.25;               FACTOR   a multiplier, 1 the game's own (dll_cs/language.cs), in percent
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
// A VALUE'S OWN TEXT: an enum (CHOICE) or a struct (FLAGS) written as a block, a member a line, its /// lines above
// it - the member says what it does, not the key. The file gets the key's /// lines, then a line per member
// "  name = text" (the names padded, the text flowed to the file's width under itself; gen_settings.py values_doc):
//     enum sound_log_choice              sound_log=        ; Sound diagnostics into ageofai_log.csv:
//     {                                                    ;   off   = none
//         /// none                                         ;   drops = counts of dropped sounds once a game minute
//         off,                                             ;   all   = also every sound played
//         ...
// Every member of such a block has its text; one written on one line ({ a, b }) has none - its key tells them.
//
// A NAME FOR A VALUE: const string aoc = "1024x768"; in a section's class - a text key of that section may be set to the
// name, or have it as its default (string k = aoc;), and the code reads the text (settings.c setting_named, rows.h
// SETTING_NAMED): the file and ageofai_config.exe show the name. It has to fit every text key of the section.
//
// MORE ABOUT A KEY: [game("what", "original", keep)] it sets one of the game's own values (SETTING_GAME: the tool shows its key blue);
// [also("section")] the tool
// lists it on that section's page too (SETTING_ALSO) - the key itself stays in its own section, once in the file.
// [target(aoc)] (or ee) it matters in that build only - the other has it of its own or lacks what it changes
// (SETTING_TARGET: the tool greys it out next to the other build's exe).

global using static ageofai.settings.build;   // [target(aoc)]: the builds by their names
using System;

namespace ageofai.settings;

[AttributeUsage(AttributeTargets.Class)]
public sealed class section : Attribute { public section(int place) { } }
[AttributeUsage(AttributeTargets.Class)]
public sealed class all : Attribute { }
[AttributeUsage(AttributeTargets.Class)]
public sealed class diff : Attribute { }
[AttributeUsage(AttributeTargets.Class)]
public sealed class test : Attribute { }
[AttributeUsage(AttributeTargets.Field)]
public sealed class length : Attribute { public length(int n) { } }
[AttributeUsage(AttributeTargets.Field)]
public sealed class per_player : Attribute { }
[AttributeUsage(AttributeTargets.Field)]
public sealed class name : Attribute { public name(string text) { } }
[AttributeUsage(AttributeTargets.Field)]
public sealed class commented : Attribute { public commented(string value = "", string key = "") { } }
[AttributeUsage(AttributeTargets.Field)]
public sealed class game : Attribute { public game(string what, string original, int keep) { } }
[AttributeUsage(AttributeTargets.Class)]
public sealed class master : Attribute { }
[AttributeUsage(AttributeTargets.Field)]
public sealed class also : Attribute { public also(string section) { } }
[AttributeUsage(AttributeTargets.Field)]
public sealed class on : Attribute { public on(string fields) { } }
[AttributeUsage(AttributeTargets.Field)]
public sealed class target : Attribute { public target(build b) { } }

// the game's builds: Empire Earth (the 2017 update) and The Art of Conquest
public enum build { ee, aoc }

static class limits {
    public const int MAX_PATH = 260;   // windows.h's: a path's buffer
}
