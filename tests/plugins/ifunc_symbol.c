/*
 * ifunc_symbol.c — a plugin that exports an IFUNC symbol and uses it
 * nowhere.
 *
 * Nothing in the plugin refers to fn, so it has no relocation against it,
 * and it loads. fn's symbol has the value of resolve_fn, the resolver, so
 * a lookup that gave that address would have a caller of fn call the
 * resolver instead: the lookup must fail. plugin_plain is an ordinary
 * function, whose lookup works.
 */

static int impl(int x) { return x + 100; }

static int (*resolve_fn(void))(int) { return impl; }
int fn(int) __attribute__((ifunc("resolve_fn")));

int plugin_plain(int x) { return x + 1; }
