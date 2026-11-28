/*
 * align.c — a plugin whose data is aligned beyond the page.
 *
 * big_aligned asks for 64 KiB, so ld gives its PT_LOAD a p_align of
 * 0x10000, and the compiler takes the alignment as given. The loader must
 * place the plugin so that the variable is 64 KiB-aligned at run time, on
 * 4 KiB pages too. plugin_aligned_first reads it, to show its bytes were
 * loaded where the plugin finds them.
 */

static char big_aligned[64] __attribute__((aligned(65536))) = { 1 };

void *plugin_aligned_addr(void) {
    void *p = big_aligned;
    __asm__("" : "+r"(p));   /* so the compiler cannot fold the low bits to 0 */
    return p;
}

int plugin_aligned_first(void) { return big_aligned[0]; }
