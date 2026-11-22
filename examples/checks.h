/*
 * checks.h — how the examples that check their own results report them.
 *
 * Each check prints its name, then calls say_pass_fail(), which ends the
 * line with " PASS" or " FAIL" and counts the check, or say_skip(), which
 * ends it with " SKIP: " and the reason, for a check that cannot run on
 * this system or in this build. main ends with
 *
 *   return checks_summary();
 *
 * which prints the counts in the format of tests/test.h and returns 1 if a
 * check failed, 0 if none did. ninja smoke runs the examples through
 * tests/run.sh, which reads that summary and the exit status, as it does
 * for the test suites.
 */

#ifndef LIGHTC_EXAMPLES_CHECKS_H
#define LIGHTC_EXAMPLES_CHECKS_H

#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/print.h>

static int checks_passed  = 0;
static int checks_failed  = 0;
static int checks_skipped = 0;

static inline void checks_say(const char *text) {
    lc_print_string(STDOUT, text, lc_string_length(text));
}

static inline void say_pass_fail(bool passed) {
    if (passed) {
        checks_say(" PASS\n");
        checks_passed++;
    } else {
        checks_say(" FAIL\n");
        checks_failed++;
    }
}

static inline void say_skip(const char *reason) {
    checks_say(" SKIP: ");
    checks_say(reason);
    lc_print_newline(STDOUT);
    checks_skipped++;
}

static inline int checks_summary(void) {
    checks_say("\n========================================\n");
    checks_say("Tests run:     ");
    lc_print_signed(STDOUT, checks_passed + checks_failed + checks_skipped);
    checks_say("\nTests passed:  ");
    lc_print_signed(STDOUT, checks_passed);
    checks_say("\nTests failed:  ");
    lc_print_signed(STDOUT, checks_failed);
    checks_say("\nTests skipped: ");
    lc_print_signed(STDOUT, checks_skipped);
    checks_say("\n========================================\n");
    if (checks_failed > 0) {
        checks_say("RESULT: FAIL\n");
        return 1;
    }
    checks_say("RESULT: PASS\n");
    return 0;
}

#endif /* LIGHTC_EXAMPLES_CHECKS_H */
