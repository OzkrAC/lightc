/*
 * based.c — a plugin linked at a nonzero address, with -Ttext-segment.
 *
 * Its first PT_LOAD is at 0x200000, not 0, so the loader maps that address
 * at the start of its mapping, and every virtual address of the plugin is
 * the load bias, mapping minus 0x200000, away from where it is mapped:
 * symbol values, relocation offsets and RELATIVE addends alike.
 * plugin_value_ptr is an absolute pointer to an exported symbol, and table
 * a RELATIVE one.
 */

int plugin_add(int a, int b) { return a + b; }

int plugin_value = 7;
int *plugin_value_ptr = &plugin_value;
int plugin_read_value(void) { return *plugin_value_ptr; }

static int s_a(void) { return 11; }
static int (*table[1])(void) = { s_a };
int plugin_table(void) { return table[0](); }
