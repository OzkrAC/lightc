#!/bin/sh
# lightc — public header check
#
# Compiles each public header alone, as the one #include of a translation
# unit of its own, so a header that uses something it does not include
# fails. Each header is compiled with -ffreestanding -Wall -Wextra -Werror:
#
#   - as C23 (-std=c2x on compilers older than GCC 14) with -Wpedantic;
#   - as C17 without -Wpedantic, with a compiler that takes C23's
#     [[attribute]] syntax in C17 as an extension, as GCC 14 and clang do.
#
# The headers use that syntax, so a C17 program needs such a compiler:
# GCC 13 rejects [[...]] before C23. A compiler that rejects it gets only
# the C23 compile, and the script says so.
#
# The headers are every *.h under include/, found when this runs, so a new
# header is checked too, also in a new directory. The syscall_nr.h files
# under include/lightc/arch/ are left out: each only defines the SYS_*
# numbers of one arch, so alone it would be an empty translation unit,
# which -Wpedantic rejects. <lightc/syscall.h> includes the one for the
# compiler's arch, so give a compiler for each arch to check both.
#
# Usage, from any directory: sh ci/headers.sh [CC...]
# CC defaults to gcc. It exits 1 if any compile failed, after trying every
# header with every compiler.

set -eu

# CDPATH= keeps cd from looking up "ci/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."

[ $# -gt 0 ] || set -- gcc

headers=$(find include -name '*.h' ! -path 'include/lightc/arch/*/syscall_nr.h' | LC_ALL=C sort)
count=$(printf '%s\n' "$headers" | grep -c . || true)
if [ "$count" -eq 0 ]; then
    echo "FAIL: no headers under include/"
    exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

status=0
for cc in "$@"; do
    # The same test as configure's.
    if $cc -std=c23 -x c -E /dev/null >/dev/null 2>&1; then
        c23=-std=c23
    else
        c23=-std=c2x
    fi
    # Does $cc take [[attribute]] syntax in C17?
    printf '[[gnu::const]] int probe(void);\n' > "$work/probe.c"
    if $cc -std=c17 -ffreestanding -Wall -Wextra -Werror \
            -c "$work/probe.c" -o "$work/probe.o" >/dev/null 2>&1; then
        c17=-std=c17 compiles=2
    else
        c17= compiles=1
        echo "$cc: rejects [[attributes]] in C17, so it compiles the headers as C23 only"
    fi
    failed=0
    for header in $headers; do
        printf '#include <%s>\n' "${header#include/}" > "$work/tu.c"
        for std in "$c23 -Wpedantic" "$c17"; do
            if [ -z "$std" ]; then
                continue
            fi
            # $cc and $std are split into words, as configure splits CC.
            if ! $cc $std -ffreestanding -Wall -Wextra -Werror -Iinclude \
                    -c "$work/tu.c" -o "$work/tu.o" > "$work/out" 2>&1; then
                echo "FAIL: $cc $std: $header"
                cat "$work/out"
                failed=$((failed + 1))
            fi
        done
    done
    if [ "$failed" -eq 0 ]; then
        echo "$cc: all $count headers compile alone as $c23 -Wpedantic${c17:+ and as $c17}"
    else
        echo "$cc: $failed of $((count * compiles)) compiles of $count headers failed"
        status=1
    fi
done
exit "$status"
