/*
 * versioned.c — a plugin with symbol versions.
 *
 * It defines foo twice, foo@V1 and the default foo@@V2, of the versions
 * tests/plugins/versioned.map declares, so it has DT_VERSYM. A lookup of
 * foo by its name alone cannot tell the two apart, so the loader must
 * refuse the plugin.
 */

int foo_v1(void) { return 1; }
int foo_v2(void) { return 2; }

__asm__(".symver foo_v1, foo@V1");
__asm__(".symver foo_v2, foo@@V2");
