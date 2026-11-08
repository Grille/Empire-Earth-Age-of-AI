// THE LANGUAGE the mod's files share (.eeai and ageofai.ini alike: an ini value is read by the same expression parser,
// 35%, 2*25, a choice's name) - what both parts declare with: the language's types and the attributes both use. One
// project (dll_cs.csproj, dotnet build here checks it all), a namespace per part: ageofai.lang the objects a script or
// text sees (lang/language.cs: their format), ageofai.settings ageofai.ini's keys (settings/settings.cs: theirs). Both
// see this file's names; a part's own attributes stay in its namespace (settings' [game] beside the language's game).
// Nothing of it runs - scripts/build/lang_read.py and settings_read.py read the files.

global using static ageofai.language_functions;
using System;

namespace ageofai;

// what a setting's declaration writes as the language writes it (static readonly vec2 aoc = vec(1024, 768))
static class language_functions { public static vec2 vec(double x, double y) => default; }

// the language's own types (lang/language.cs "TYPES": what each is). int, float, bool and void are written with C#'s
// words because the language has the same names (a script's x: int) - but they are the language's, not C#'s:
//   bool   true / false; in ageofai.ini also 1 / 0 (the file writes 1 / 0), and a bool only for a 0 / 1 key - true
//          for a number is a mistake there (ini.c ini_eval_as), not a 1
//   int    a whole number, read as an expression (epoch_min=2*25); a fraction is a mistake, never rounded
//   float  a number with decimals - the game's double
//   share  below: a float written in percent
struct text { }
struct epoch { }
struct color { }
struct unit_type { }
struct vec2 { }
struct vec3 { }
struct vec4 { }
struct trigger { }
struct function { }
struct @record { }   // (@: C#'s own word)
struct filter { }
struct any { }

// a share: a fraction, in a file written in percent (ageofai.ini's home_guard_share=35%, a script's 70%: both 0.35 /
// 0.7) - above 100% too where the key says so (cost_percent=150%). Declared as the fraction (share navy_share = 0.25),
// its bounds a [range] in fractions too; how the code keeps it is the code's
struct share
{
    public static implicit operator share(double value) => default;
}

// a factor: a multiplier whose 1 (100%) is the game's own - in a file written in percent like a share
// (friendly_collision=75%); a [master] group's disabled sets it to 1
struct factor
{
    public static implicit operator factor(double value) => default;
}

// a name to show for a section, a group or a key (ageofai_config.exe; a section's or group's title else comes from its
// name - settings/settings.cs)
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Field)]
public sealed class title : Attribute { public title(string text) { } }

// a sanity bound lo .. hi of a whole number or a share (a setting's: outside it a warning, the value used); error: true
// a whole number's hard limit - the code can't take a value outside it (a mistake: told, the default instead)
[AttributeUsage(AttributeTargets.Field)]
public sealed class range : Attribute
{
    public range(int lo, int hi, bool error = false) { }
    public range(double lo, double hi) { }
}
