#!/bin/sh
# lightc — pkg-config consumer check
#
# Installs the configured build into a temporary DESTDIR and builds programs
# the way an outside project would, with only the flags from the installed
# lightc.pc:
#
#   - examples/hello.c, built in one step and with separate compile and link
#     steps, must be a static EXEC with no INTERP, GNU_STACK RW and no PT_TLS,
#     and must run and exit 0;
#   - a program with a thread_local variable must compile, then fail to link
#     with the linker script's "lightc: thread_local" message.
#
# Usage, from a configured tree (the built programs are run, so CC must
# target this machine; it defaults to gcc):
#   ./configure release && sh ci/consumer.sh

set -eu

cd "$(dirname "$0")/.."

CC=${CC:-gcc}

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

[ -f build.ninja ] || fail "no build.ninja; run ./configure first"
command -v pkg-config >/dev/null || fail "pkg-config not found"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# --- Install ---

DESTDIR="$work/dest" ninja install
pc=$(find "$work/dest" -path '*/lib/pkgconfig/lightc.pc')
[ -n "$pc" ] || fail "ninja install wrote no lib/pkgconfig/lightc.pc"
prefix=${pc%/lib/pkgconfig/lightc.pc}

# PKG_CONFIG_SYSROOT_DIR would rewrite -I and -L but not -T, so point prefix
# at the DESTDIR copy instead. pkg-config searches PKG_CONFIG_PATH before
# PKG_CONFIG_LIBDIR, so unsetting the one and pointing the other at the
# DESTDIR keeps any other lightc.pc out of the search.
unset PKG_CONFIG_PATH PKG_CONFIG_SYSROOT_DIR
PKG_CONFIG_LIBDIR=${pc%/lightc.pc}
export PKG_CONFIG_LIBDIR
cflags=$(pkg-config --define-variable=prefix="$prefix" --cflags lightc)
libs=$(pkg-config --define-variable=prefix="$prefix" --libs lightc)
echo "cflags: $cflags"
echo "libs:   $libs"

# Every path the flags name must exist in the DESTDIR install, so nothing
# is silently taken from an earlier install under the real prefix.
for flag in $cflags $libs; do
    case $flag in
        -I*|-L*|-T*)
            path=${flag#-?}
            case $path in
                "$prefix"/*) ;;
                *) fail "$flag is outside $prefix" ;;
            esac
            [ -e "$path" ] || fail "$flag: no such file or directory"
            ;;
    esac
done

# --- hello: static EXEC, no INTERP, GNU_STACK RW, no PT_TLS, exits 0 ---

check_hello() {
    exe=$1
    elf_type=$(readelf -h "$exe" | awk '$1 == "Type:" { print $2 }')
    [ "$elf_type" = EXEC ] || fail "$exe: ELF type is ${elf_type:-unknown}, want EXEC"
    phdrs=$(readelf -lW "$exe")
    if printf '%s\n' "$phdrs" | grep -q '^ *INTERP '; then
        fail "$exe has a PT_INTERP header"
    fi
    if printf '%s\n' "$phdrs" | grep -q '^ *TLS '; then
        fail "$exe has a PT_TLS header"
    fi
    stack=$(printf '%s\n' "$phdrs" | awk '$1 == "GNU_STACK" { print $7 }')
    [ "$stack" = RW ] || fail "$exe: GNU_STACK flags are '${stack:-missing}', want RW"

    status=0
    out=$("$exe") || status=$?
    [ "$status" -eq 0 ] || fail "$exe exited with status $status"
    case $out in
        "hello from lightc"*) ;;
        *) fail "$exe printed '$out'" ;;
    esac
    echo "ok: $exe"
}

# $cflags and $libs are unquoted on purpose: each is a list of words.
$CC $cflags examples/hello.c $libs -o "$work/hello" ||
    fail "hello did not build in one step"
check_hello "$work/hello"

$CC $cflags -c examples/hello.c -o "$work/hello-split.o" ||
    fail "hello did not compile with Cflags alone"
$CC "$work/hello-split.o" $libs -o "$work/hello-split" ||
    fail "hello did not link with Libs alone"
check_hello "$work/hello-split"

# --- thread_local: compiles, then the link fails with lightc's ASSERT ---

# thread_local is a keyword from C23 on (-std=c2x in GCC 13), not in GCC's
# default before GCC 15 (gnu17), so this program gets the -std configure picks.
if $CC -std=c23 -x c -E /dev/null >/dev/null 2>&1; then
    cstd=-std=c23
else
    cstd=-std=c2x
fi

cat > "$work/tls.c" << 'EOF'
#include <lightc/syscall.h>

static thread_local int counter = 5;

int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    return ++counter == 6 ? 0 : 1;
}
EOF

$CC $cstd $cflags -c "$work/tls.c" -o "$work/tls.o" ||
    fail "the thread_local program did not compile"
if $CC "$work/tls.o" $libs -o "$work/tls" 2> "$work/tls.log"; then
    fail "the thread_local program linked"
fi
if ! grep -q 'lightc: thread_local is not supported' "$work/tls.log"; then
    cat "$work/tls.log" >&2
    fail "the thread_local link failed without lightc's message"
fi
echo "ok: thread_local program fails to link:"
grep 'lightc: thread_local' "$work/tls.log"

echo "PASS: pkg-config consumer"
