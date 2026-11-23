/*
 * page.c — a plugin linked for a given page size, twice.
 *
 * page64k.so is linked with -z max-page-size=0x10000 and -z
 * common-page-size=0x10000, so no two of its PT_LOADs share a 64 KiB page
 * and its PT_GNU_RELRO ends on a 64 KiB boundary: it loads with any page
 * size up to 64 KiB, and each of its segments, RELRO among them, gets its
 * own protection. Its RW segment starts inside a 64 KiB page, as ld lays
 * out the data of a plugin. page4k.so is linked for 4 KiB pages, so its
 * segments, all in its first 16 KiB, share one page wherever pages are
 * larger.
 *
 * plugin_const_table is a const pointer that a RELATIVE relocation fills,
 * so it is in .data.rel.ro, inside PT_GNU_RELRO, and plugin_data is
 * ordinary writable data. plugin_one_address gives the address of one(),
 * which its code computes from the program counter, not from the table:
 * the value the relocated table must hold.
 */

int plugin_add(int a, int b) { return a + b; }

static int one(void) { return 1; }
int (*const plugin_const_table[1])(void) = { one };
int (*plugin_one_address(void))(void) { return one; }

int plugin_data = 7;
