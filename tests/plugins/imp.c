/*
 * imp.c — a plugin that imports symbols from its host.
 *
 * host_value and host_func are undefined and not weak: GLOB_DAT and
 * JUMP_SLOT relocations against them need a definition from another
 * object, and lc_library_open searches none. weak_thing is undefined and
 * weak, which a loader resolves to 0.
 */

extern int host_value;
extern int host_func(int);
extern int weak_thing __attribute__((weak));
int plugin_uses_host_value(void) { return host_value; }
int plugin_uses_host_func(int x) { return host_func(x); }
int plugin_has_weak(void) { return &weak_thing != 0; }
