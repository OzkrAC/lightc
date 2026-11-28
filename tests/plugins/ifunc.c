/*
 * ifunc.c — a plugin with IFUNC symbols that it uses itself.
 *
 * fn is an exported IFUNC: its symbol's type is STT_GNU_IFUNC, and its
 * value the address of resolve_fn, which a dynamic linker calls to get
 * fn's address. The plugin calls fn and stores its address, which gives
 * relocations against that symbol, and stores the address of hid, a hidden
 * IFUNC, which gives an IRELATIVE relocation. The loader calls no
 * resolver, so it must refuse the plugin.
 */

static int impl(int x) { return x + 100; }

static int (*resolve_fn(void))(int) { return impl; }
int fn(int) __attribute__((ifunc("resolve_fn")));

static int (*resolve_hid(void))(int) { return impl; }
__attribute__((visibility("hidden"))) int hid(int) __attribute__((ifunc("resolve_hid")));

int plugin_call_fn(int x) { return fn(x); }
int (*plugin_fn_ptr)(int) = fn;
int (*plugin_hid_ptr)(int) = hid;
