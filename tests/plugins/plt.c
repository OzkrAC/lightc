/*
 * plt.c — a plugin whose functions call each other through its PLT.
 *
 * plt.so is built from this and plt_helper.c, with -fPIC as configure
 * builds every test plugin. plugin_call calls helper, a function of default
 * visibility of the other file, so the call goes through the PLT whatever
 * the compiler, though the plugin defines helper itself: it reads helper's
 * address from a GOT slot that an R_*_JUMP_SLOT relocation fills, and only
 * DT_JMPREL lists those. The plugin has the other relocations a loader
 * applies too: plugin_counter is read through the GOT (GLOB_DAT), table
 * holds two RELATIVE pointers, and counter_ptr an absolute one to an
 * exported symbol (R_X86_64_64, R_AARCH64_ABS64). The constructor sets
 * ctor_ran, and the destructor writes 5678 through dtor_flag once a host
 * has set it.
 */

int plugin_counter = 7;
int helper(int x);      /* in plt_helper.c: x * 3 */

int plugin_call(int x) { return helper(x) + 1; }
int plugin_read_counter(void) { return plugin_counter; }

static int s_a(void) { return 11; }
static int s_b(void) { return 22; }
int (*table[2])(void) = { s_a, s_b };
int *counter_ptr = &plugin_counter;
int plugin_table(int i) { return table[i](); }
int plugin_counter_via_ptr(void) { return *counter_ptr; }

int ctor_ran = 0;
__attribute__((constructor)) static void ctor(void) { ctor_ran = 1234; }
int *dtor_flag = 0;
__attribute__((destructor)) static void dtor(void) { if (dtor_flag) *dtor_flag = 5678; }
