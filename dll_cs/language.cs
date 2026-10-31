// THE LANGUAGE the mod's files share (.eeai and ageofai.ini alike: an ini value is read by the same expression parser,
// 35%, 2*25, a choice's name) - what both parts declare with: the language's types and the attributes both use. One
// project (dll_cs.csproj, dotnet build here checks it all), a namespace per part: ageofai.lang the objects a script or
// text sees (lang/language.cs: their format), ageofai.settings ageofai.ini's keys (settings/settings.cs: theirs). Both
// see this file's names; a part's own attributes stay in its namespace (settings' [game] beside the language's game).
// Nothing of it runs - scripts/build/lang_read.py and settings_read.py read the files.

using System;

namespace ageofai;

// the language's own types (lang/language.cs "TYPES": what each is) - int, float, bool and void are C#'s
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

// a name to show for a section, a group or a key (ageofai_config.exe; a section's or group's title else comes from its
// name - settings/settings.cs)
[AttributeUsage(AttributeTargets.Class | AttributeTargets.Field)] public sealed class title : Attribute { public title(string text) { } }

// a sanity bound lo .. hi of a whole number (a setting's: outside it a warning, the value used)
[AttributeUsage(AttributeTargets.Field)] public sealed class range : Attribute { public range(int lo, int hi) { } }
