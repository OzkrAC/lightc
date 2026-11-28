#!/bin/sh
# lightc — checks and renames for the objects of the math sources
#
# The math sources (src/math_*.c) define the standard names that GCC calls
# when it cannot inline a builtin, such as floorf for __builtin_floorf. A
# builtin used in such a source must compile to an instruction there: if
# GCC falls back to a call, the call is to the function it is in, and
# recurses. configure runs the first form below on every math object with
# every build; the other two build the sweep tool.
#
#   sh tools/math-objects.sh calls STAMP OBJECT...
#       Fails, naming each, when an OBJECT has a relocation against a
#       function it defines itself, by its global name: a call that a
#       builtin fell back to. Writes STAMP when none has. The math sources
#       take no address of their own functions, so every such relocation
#       is a call.
#
#   sh tools/math-objects.sh names MAP OBJECT...
#       Writes MAP, objcopy's --redefine-syms file that renames each
#       function the OBJECTs define, but for the lc_ ones, to lightc_NAME:
#       "floorf lightc_floorf". tools/math-sweep.c then links them beside
#       the host's libm, whose floorf is the oracle.
#
#   sh tools/math-objects.sh rename MAP IN OUT
#       Copies IN to OUT with the names of MAP renamed, the references to
#       them too, and without the .lightc_abi section, whose reference to
#       the ABI tag only liblightc.a defines. Fails when OUT still refers
#       to a symbol that is not a lightc_ one, which the sweep would take
#       from the host's libc or libm: the oracle would then be compared
#       with itself.
#
# READELF and OBJCOPY name the tools (default readelf and objcopy); readelf
# reads the objects of either arch.

set -eu

READELF=${READELF:-readelf}
OBJCOPY=${OBJCOPY:-objcopy}

fail() {
    echo "math-objects: $*" >&2
    exit 1
}

# The global and weak functions OBJECT defines, one per line.
defined_functions() {
    "$READELF" -sW "$1" |
        awk '$4 == "FUNC" && ($5 == "GLOBAL" || $5 == "WEAK") && $7 != "UND" { print $8 }'
}

[ $# -ge 1 ] || fail "usage: math-objects.sh calls|names|rename ..."
command=$1
shift

case $command in
    calls)
        [ $# -ge 2 ] || fail "usage: math-objects.sh calls STAMP OBJECT..."
        stamp=$1
        shift
        bad=0
        for object in "$@"; do
            [ -f "$object" ] || fail "no $object"
            defined=$(defined_functions "$object")
            # A relocation line starts with its offset in hex; the
            # symbol's name is its fifth field, "floorf" in
            # "000000000000000f ... R_X86_64_PLT32 0000000000000000 floorf - 4".
            calls=$("$READELF" -rW "$object" | awk -v names="$defined" '
                BEGIN { n = split(names, list, "\n"); for (i = 1; i <= n; i++) own[list[i]] = 1 }
                /^[0-9a-f]+[ \t]/ && ($5 in own) { print $5 " (" $3 " at " $1 ")" }' | sort -u)
            if [ -n "$calls" ]; then
                echo "math-objects: $object calls a function it defines, which a builtin fell back to:" >&2
                printf '%s\n' "$calls" | sed 's/^/  /' >&2
                bad=1
            fi
        done
        [ "$bad" -eq 0 ] || exit 1
        : > "$stamp"
        ;;
    names)
        [ $# -ge 2 ] || fail "usage: math-objects.sh names MAP OBJECT..."
        map=$1
        shift
        : > "$map.tmp"
        for object in "$@"; do
            [ -f "$object" ] || fail "no $object"
            defined_functions "$object" | awk '$1 !~ /^lc_/ { print $1 " lightc_" $1 }' >> "$map.tmp"
        done
        sort -u "$map.tmp" > "$map"
        rm -f "$map.tmp"
        [ -s "$map" ] || fail "the objects define no function to rename"
        ;;
    rename)
        [ $# -eq 3 ] || fail "usage: math-objects.sh rename MAP IN OUT"
        "$OBJCOPY" --redefine-syms="$1" -R .lightc_abi --strip-unneeded "$2" "$3"
        others=$("$READELF" -sW "$3" | awk '$7 == "UND" && $8 != "" && $8 !~ /^lightc_/ { print $8 }')
        if [ -n "$others" ]; then
            rm -f "$3"
            fail "$2 refers to what lightc does not define, which the sweep would take from the host:" $others
        fi
        ;;
    *)
        fail "unknown command $command: use calls, names or rename"
        ;;
esac
