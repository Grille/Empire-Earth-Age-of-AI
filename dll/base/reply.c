// A PIPE COMMAND'S REPLY: every part has its own commands (cmd_<verb>(out, args), control.c execute dispatches them);
// REPLY appends to the reply text `out` (REPLY_CAP bytes, the control pipe's buffer).
#define REPLY_CAP 16384
static int out_printf(char *out, size_t cap, const char *fmt, ...) {
    size_t len = strlen(out);
    if (len >= cap - 1) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out + len, cap - len, fmt, ap);
    va_end(ap);
    return n;
}
#define REPLY(...) out_printf(out, REPLY_CAP, __VA_ARGS__)
