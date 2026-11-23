/*
 * weak.c — a plugin whose one import is weak.
 *
 * weak_thing is undefined and weak, and lc_library_open has no other
 * object to find it in, so the GLOB_DAT relocation of its GOT slot
 * resolves to 0 and the plugin loads: plugin_has_weak returns 0.
 */

extern int weak_thing __attribute__((weak));
int plugin_has_weak(void) { return &weak_thing != 0; }
