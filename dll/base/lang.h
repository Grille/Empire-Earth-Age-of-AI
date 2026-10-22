// THE C THE CODE IS WRITTEN IN: words for what C says unclearly (included before every part - eeaimod.c)

// internal: a function or variable at file level that only this DLL sees (C's static there: internal linkage - the
// DLL is one translation unit, so it is every part's, but no other module's). C gives static a second meaning
// inside a function - a variable that keeps its value between calls - and there it stays static. The build holds
// the line (scripts/style.py): static at file level, internal inside a function are refused.
#define internal static

// a function that takes a printf format: the compiler checks its calls' arguments against the format, as it does
// printf's (a %s given an int would crash, a %d given a float prints garbage - varargs pass it as a double)
#define PRINTF_LIKE(fmt, args) __attribute__((format(printf, fmt, args)))
