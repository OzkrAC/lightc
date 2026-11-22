#!/bin/sh
# lightc — self-test of ci/abilint.sh
#
# Copies what ci/abilint.sh reads (syscalls.def, include/, src/, tests/,
# examples/ and bench/) to a scratch tree, where the lint must pass. Then
# each case below plants mistakes in a fresh copy, and the lint must exit 1
# with exactly the FAIL: lines expected:
#
#   arity    A new src/planted.c: a call whose N differs from syscalls.def,
#            written over three lines, and one in a macro over two; a call
#            of a syscall syscalls.def does not list; first arguments that
#            are not a SYS_ name; an lc_syscallN that is not called; and
#            wrong calls after a character literal '"' and after a string
#            literal holding \" and /*, so a literal that hid the code
#            after it would fail. Calls in comments and literals, correct
#            calls over lines and in a macro over lines, and a longer name
#            that ends in lc_syscall3 must not fail.
#   unlisted A macro added to <lightc/socket.h>, and a new src/planted.h
#            with an include guard, a constant, a function-like macro and
#            a constant defined after an #ifndef of its name: each macro
#            fails, the guard does not.
#   stale    LC_LOG_BUFFER_SIZE removed from <lightc/log.h>, so its entry
#            in LIGHTC_OWN names no macro, and LC_STATS added to
#            NO_UAPI_CHECK, so it is in two lists.
#   undef    The #undef of SA_RESTORER removed from tests/abi_check.c.
#
# Usage, from any directory: sh ci/abilint-test.sh
# It exits 1 after all the cases when any of them did not go as expected.

set -eu

# CDPATH= keeps cd from looking up "ci/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."
here=$(pwd)

LC_ALL=C
export LC_ALL

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

failed=0

# fresh: a copy of the tree in $work/tree, as the lint reads it.
fresh() {
    rm -rf "$work/tree"
    mkdir "$work/tree"
    cp -R syscalls.def include src tests examples bench "$work/tree"
}

# lint: runs the lint on the copy, its output in $work/out.
lint() {
    status=0
    sh "$here/ci/abilint.sh" "$work/tree" > "$work/out" 2>&1 || status=$?
}

# expect NAME: the lint must exit 1 with the FAIL: lines of $work/expected,
# in any order, and no other.
expect() {
    lint
    grep '^FAIL: ' "$work/out" | sort > "$work/actual" || true
    sort "$work/expected" > "$work/expected.sorted"
    if [ "$status" -eq 1 ] && cmp -s "$work/expected.sorted" "$work/actual"; then
        echo "ok: $1: the lint fails with the $(wc -l < "$work/actual" | tr -d ' ') FAIL: lines expected"
    else
        echo "FAIL: $1: exit status $status, and the FAIL: lines differ (-: expected, +: the lint's):"
        diff -u "$work/expected.sorted" "$work/actual" || true
        echo "The lint's output:"
        cat "$work/out"
        failed=$((failed + 1))
    fi
}

# The line of tests/abi_check.c in the copy that lists X(NAME.
list_line() {
    grep -n "X($1[,)]" "$work/tree/tests/abi_check.c" | sed 's/:.*//'
}

fresh
lint
if [ "$status" -eq 0 ]; then
    echo "ok: the tree as it is passes"
else
    echo "FAIL: the tree as it is fails the lint:"
    cat "$work/out"
    failed=$((failed + 1))
fi

# --- arity ---

fresh
cat > "$work/tree/src/planted.c" << 'EOF'
/* lc_syscall1(SYS_read, 0) in a comment is no call */
/*
 * lc_syscall4(SYS_futex, 0, 0, 0, 0) nor in a longer one
 */
static const char *text = "lc_syscall1(SYS_read, 0)";
static void planted(int64_t nr) {
    lc_syscall3(
        SYS_listen,
        0, 1);
    lc_syscall2(SYS_nosuch, 0, 0);
    lc_syscall1(nr);
    lc_syscall1(SYS_close + 0);
    lc_sysret (*call)(int64_t, int64_t) = lc_syscall1;
    char quote = '"'; lc_syscall1(SYS_read, 0);
    text = "\"/*"; lc_syscall1(SYS_write, 0);
    lc_syscall2(
        SYS_listen,
        0, 1);
    my_lc_syscall3(SYS_listen, 0, 1);
    lc_syscall1(SYS_close, 0); // lc_syscall2(SYS_close, 0, 0)
}
#define PLANTED_CLOSE(fd) lc_syscall2( \
        SYS_close, fd, 0)
#define PLANTED_GETPID() lc_syscall0( \
        SYS_getpid)
EOF
cat > "$work/expected" << 'EOF'
FAIL: src/planted.c:7: lc_syscall3(SYS_listen, ...): listen takes 2 arguments in syscalls.def, not 3
FAIL: src/planted.c:10: lc_syscall2(SYS_nosuch, ...): nosuch is not in syscalls.def
FAIL: src/planted.c:11: the first argument of lc_syscall1 is not a SYS_ name, so its arity cannot be checked
FAIL: src/planted.c:12: the first argument of lc_syscall1 is not a SYS_ name, so its arity cannot be checked
FAIL: src/planted.c:13: lc_syscall1 is not called here, so its arity cannot be checked
FAIL: src/planted.c:14: lc_syscall1(SYS_read, ...): read takes 3 arguments in syscalls.def, not 1
FAIL: src/planted.c:15: lc_syscall1(SYS_write, ...): write takes 3 arguments in syscalls.def, not 1
FAIL: src/planted.c:22: lc_syscall2(SYS_close, ...): close takes 1 argument in syscalls.def, not 2
EOF
expect arity

# --- unlisted ---

fresh
socket_h=$work/tree/include/lightc/socket.h
echo '#define LC_PLANTED_FLAG 0x40' >> "$socket_h"
socket_line=$(wc -l < "$socket_h" | tr -d ' ')
cat > "$work/tree/src/planted.h" << 'EOF'
#ifndef PLANTED_H
#define PLANTED_H

#define PLANTED_FLAG 0x10
#define PLANTED_FIELD(x) ((x) >> 4)

#ifndef PLANTED_FALLBACK
#define PLANTED_FALLBACK 0x20
#endif

#endif
EOF
cat > "$work/expected" << EOF
FAIL: include/lightc/socket.h:$socket_line: LC_PLANTED_FLAG is in no list of tests/abi_check.c: check it against the UAPI there, or add it to NO_UAPI_CHECK or LIGHTC_OWN with the reason
FAIL: src/planted.h:4: PLANTED_FLAG is in no list of tests/abi_check.c: check it against the UAPI there, or add it to NO_UAPI_CHECK or LIGHTC_OWN with the reason
FAIL: src/planted.h:5: PLANTED_FIELD is in no list of tests/abi_check.c: check it against the UAPI there, or add it to NO_UAPI_CHECK or LIGHTC_OWN with the reason
FAIL: src/planted.h:8: PLANTED_FALLBACK is in no list of tests/abi_check.c: check it against the UAPI there, or add it to NO_UAPI_CHECK or LIGHTC_OWN with the reason
EOF
expect unlisted

# --- stale ---

fresh
log_h=$work/tree/include/lightc/log.h
grep -v '^#define LC_LOG_BUFFER_SIZE ' "$log_h" > "$work/log.h"
cat "$work/log.h" > "$log_h"
abi=$work/tree/tests/abi_check.c
sed 's/X(STDIN) /X(STDIN) X(LC_STATS) /' "$abi" > "$work/abi_check.c"
cat "$work/abi_check.c" > "$abi"
cat > "$work/expected" << EOF
FAIL: tests/abi_check.c:$(list_line LC_LOG_BUFFER_SIZE): LC_LOG_BUFFER_SIZE is in LIGHTC_OWN, but no header under include/ or src/ defines it
FAIL: tests/abi_check.c:$(list_line LC_STATS | sed -n 2p): LC_STATS is in NO_UAPI_CHECK and in LIGHTC_OWN
EOF
expect stale

# --- undef ---

fresh
abi=$work/tree/tests/abi_check.c
grep -v '^#undef SA_RESTORER$' "$abi" > "$work/abi_check.c"
cat "$work/abi_check.c" > "$abi"
cat > "$work/expected" << EOF
FAIL: tests/abi_check.c:$(list_line SA_RESTORER): SA_RESTORER is in UAPI_CONSTANTS, but abi_check.c does not #undef it before it includes the lightc headers
EOF
expect undef

if [ "$failed" -gt 0 ]; then
    echo "FAIL: $failed of the cases did not go as expected"
    exit 1
fi
echo "abilint-test: every case went as expected"
