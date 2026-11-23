/*
 * relr.c — a plugin linked with -z pack-relative-relocs.
 *
 * ld then moves every relative relocation into DT_RELR, packed, and
 * DT_RELA keeps only the others. The relative ones here are the
 * constructor's entry in .init_array and the two pointers of table, so a
 * loader that skips DT_RELR calls the constructor at its link-time address.
 * The constructor sets ctor_ran to 1234.
 *
 * words holds 136 more relative pointers, one after another. ld packs a
 * run of them into an address entry, for the first, and a bitmap for each
 * 63 that follow, so they take three bitmaps: plugin_word(i) is i only
 * when the loader steps from one bitmap's 63 words to the next's.
 */

static int s_a(void) { return 11; }
static int s_b(void) { return 22; }
static int (*table[2])(void) = { s_a, s_b };
int plugin_table(int i) { return table[i](); }

int ctor_ran = 0;
__attribute__((constructor)) static void ctor(void) { ctor_ran = 1234; }

#define WORD(i)    &slots[i]
#define WORDS4(i)  WORD(i), WORD(i + 1), WORD(i + 2), WORD(i + 3)
#define WORDS16(i) WORDS4(i), WORDS4(i + 4), WORDS4(i + 8), WORDS4(i + 12)
#define WORDS64(i) WORDS16(i), WORDS16(i + 16), WORDS16(i + 32), WORDS16(i + 48)

static int slots[136];
static int *const words[136] = { WORDS64(0), WORDS64(64), WORDS4(128), WORDS4(132) };
int plugin_word_count(void) { return (int)(sizeof(words) / sizeof(words[0])); }
int plugin_word(int i) {
    return (int)(((__UINTPTR_TYPE__)words[i] - (__UINTPTR_TYPE__)&slots[0]) / sizeof(slots[0]));
}
