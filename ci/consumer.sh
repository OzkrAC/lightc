#!/bin/sh
# lightc — pkg-config consumer check
#
# Installs the configured build into a temporary DESTDIR and builds programs
# the way an outside project would, with only the flags from the installed
# lightc.pc:
#
#   - examples/hello.c, built in one step and with separate compile and link
#     steps, must be a static EXEC with no INTERP, must pass ci/elfcheck.sh
#     (GNU_STACK RW, no PT_TLS, no RWX header, text and data on pages of
#     their own), and must run and exit 0;
#   - a program with a thread_local variable must compile, then fail to link
#     with the linker script's "lightc: thread_local" message;
#   - the ABI guard of <lightc/version.h> and the installed <lightc/config.h>:
#     hello must fail to link against a liblightc.a whose ABI tag is another
#     minor version's, naming the tag it lacks; tests/regress/ARENA-abi-lcstats.c,
#     which flips NDEBUG, must build and pass; a -DLC_STATS other than the
#     library's must stop at config.h's #error; and a -fPIC plugin compiled
#     with the installed headers must load and run.
#
# Usage, from a configured tree (the built programs are run, so CC must
# target this machine; it defaults to gcc):
#   ./configure release && sh ci/consumer.sh

set -eu

# CDPATH= keeps cd from looking up "ci/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."

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

# --- hello: static EXEC, no INTERP, passes ci/elfcheck.sh, exits 0 ---

# ci/elfcheck.sh checks the program headers: GNU_STACK, TLS, RWX and the
# pages of the segments. It checks a PIE as it checks an EXEC, so the ELF
# type and the INTERP header, which mark a binary built some other way
# than lightc.pc says, are checked here.
check_hello() {
    exe=$1
    elf_type=$(readelf -h "$exe" | awk '$1 == "Type:" { print $2 }')
    [ "$elf_type" = EXEC ] || fail "$exe: ELF type is ${elf_type:-unknown}, want EXEC"
    if readelf -lW "$exe" | grep -q '^ *INTERP '; then
        fail "$exe has a PT_INTERP header"
    fi
    sh ci/elfcheck.sh "$exe" || fail "$exe did not pass ci/elfcheck.sh"

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

# --- The ABI tag: another minor version's library does not link ---

version=$(pkg-config --modversion lightc)
major=${version%%.*}
minor=${version#*.}
minor=${minor%%.*}
tag="lc_abi_${major}_${minor}"
lib=$(find "$prefix" -name liblightc.a)
[ -n "$lib" ] || fail "no liblightc.a under $prefix"
# A copy of the library whose version.o defines the tag of minor version 999.
cp "$lib" "$work/other.a"
"${AR:-ar}" d "$work/other.a" version.o || fail "the installed liblightc.a has no version.o"
printf '__asm__(".globl lc_abi_%s_999\\n\\t.set lc_abi_%s_999, 0");\n' "$major" "$major" > "$work/other_version.c"
$CC -c "$work/other_version.c" -o "$work/version.o" || fail "the other version.o did not compile"
"${AR:-ar}" r "$work/other.a" "$work/version.o" 2>/dev/null
other_libs=$(printf '%s\n' $libs | sed "s|^-llightc\$|$work/other.a|")
if $CC "$work/hello-split.o" $other_libs -o "$work/hello-other" 2> "$work/other.log"; then
    fail "hello linked against a liblightc.a of another minor version"
fi
grep -q "undefined reference to .$tag'" "$work/other.log" || {
    cat "$work/other.log" >&2
    fail "the link against another minor version failed without naming $tag"
}
echo "ok: hello does not link against a liblightc.a of minor version 999 ($tag missing)"

# --- A program built with the other NDEBUG agrees on lc_arena ---

# It flips NDEBUG against its flags, which have none here.
$CC $cflags -Itests tests/regress/ARENA-abi-lcstats.c $libs -o "$work/arena" ||
    fail "ARENA-abi-lcstats did not build against the install"
"$work/arena" > "$work/arena.log" || {
    cat "$work/arena.log" >&2
    fail "ARENA-abi-lcstats failed against the install"
}
echo "ok: ARENA-abi-lcstats passes against the install"

# --- config.h: a -DLC_STATS other than the library's stops the build ---

config=$(find "$prefix" -path '*/include/lightc/config.h')
[ -n "$config" ] || fail "ninja install wrote no include/lightc/config.h"
stats=$(awk '$1 == "#define" && $2 == "LC_STATS" { print $3 }' "$config")
case $stats in
    0) other=1 ;;
    1) other=0 ;;
    *) fail "$config does not define LC_STATS as 0 or 1" ;;
esac
if $CC $cflags -DLC_STATS=$other -c examples/hello.c -o "$work/stats.o" 2> "$work/stats.log"; then
    fail "-DLC_STATS=$other compiled against an install built with LC_STATS=$stats"
fi
grep -q "lightc: this install was built with LC_STATS=$stats" "$work/stats.log" || {
    cat "$work/stats.log" >&2
    fail "-DLC_STATS=$other failed without config.h's message"
}
$CC $cflags -DLC_STATS=$stats -c examples/hello.c -o "$work/stats.o" ||
    fail "-DLC_STATS=$stats, the library's own, did not compile"
echo "ok: -DLC_STATS=$other stops at config.h, and -DLC_STATS=$stats builds"

# --- A -fPIC plugin compiled with lightc's headers loads ---

# A plugin is linked with no liblightc.a, so it uses only the headers'
# inline functions.
cat > "$work/plugin.c" << 'EOF'
#include <lightc/result.h>
#include <lightc/socket.h>

int plugin_swap(int value) {
    lc_result r = lc_ok(lc_network_to_host_16((uint16_t)value));
    return lc_is_ok(r) ? (int)r.value : -1;
}
EOF
cat > "$work/host.c" << 'EOF'
#include <lightc/library.h>

int main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc < 2) return 2;
    lc_result_ptr lib = lc_library_open(argv[1]);
    if (lc_ptr_is_err(lib)) return 3;
    lc_result_ptr sym = lc_library_find_symbol(lib.value, "plugin_swap");
    if (lc_ptr_is_err(sym)) return 4;
    int (*swap)(int);
    __builtin_memcpy(&swap, &sym.value, sizeof swap);
    int n = swap(0x0102);
    lc_library_close(lib.value);
    return n == 0x0201 ? 0 : 5;
}
EOF
$CC $cflags -fPIC -shared -nostdlib "$work/plugin.c" -o "$work/plugin.so" ||
    fail "the plugin did not build with the installed headers"
$CC $cflags "$work/host.c" $libs -o "$work/host" || fail "the plugin's host did not build"
status=0
"$work/host" "$work/plugin.so" || status=$?
[ "$status" -eq 0 ] || fail "the plugin did not load and run: status $status"
echo "ok: a -fPIC plugin built with the installed headers loads"

echo "PASS: pkg-config consumer"
