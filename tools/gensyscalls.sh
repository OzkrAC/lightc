#!/bin/sh
# lightc — syscall and errno table generator
#
# Reads syscalls.def and writes what is generated from it:
#
#   include/lightc/arch/x86_64/syscall_nr.h   the whole file
#   include/lightc/arch/aarch64/syscall_nr.h  the whole file
#   include/lightc/result.h                   the errno-enum and errno-names
#                                             regions
#   tests/abi_check.c                         the abi-syscalls and abi-errnos
#                                             regions
#
# A region is the lines between a line that starts with the comment
# "/* BEGIN GENERATED <region>" and the next one that starts with
# "/* END GENERATED <region>". This replaces those lines and keeps the rest of
# the file, which is written by hand. Each region must be there exactly once.
#
# A file whose content does not change is not rewritten, so ninja does not
# rebuild for it. When syscalls.def or a region marker is wrong, this names
# the line, writes no file and exits 1. It needs only a POSIX sh and awk.
#
# Usage, from any directory: sh tools/gensyscalls.sh
# Run it after editing syscalls.def and commit its output with the edit. CI
# runs the same command and fails when it changes any file.

set -eu

# CDPATH= keeps cd from looking up "tools/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."

# Byte semantics for awk's length() and regular expressions in every locale.
LC_ALL=C
export LC_ALL

DEF=syscalls.def

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# The awk program reads syscalls.def first. With what=x86_64 or
# what=aarch64 it then prints that arch's syscall_nr.h. With what=splice it
# copies a second file and fills in its regions, which must be exactly those
# named in "regions".
GEN='
function fail(msg) {
    print msg | "cat 1>&2"
    failed = 1
    exit 1
}

function is_number(s) {
    return s ~ /^(0|[1-9][0-9]*)$/
}

function lc_name(e) {
    return "LC_ERR_" substr(e, 2)
}

# --- syscalls.def ---

FNR == NR {
    where = FILENAME ":" FNR
    sub(/#.*/, "")
    if (NF == 0) next
    if ($1 == "syscall") {
        if (NF != 5) fail(where ": expected syscall NAME X86_64 AARCH64 ARGS")
        name = $2
        if (name !~ /^[a-z][a-z0-9_]*$/) fail(where ": " name " is not a syscall name")
        if (name in args) fail(where ": syscall " name " is listed twice")
        for (i = 1; i <= 2; i++) {
            arch = arches[i]
            nr = $(i + 2)
            if (nr != "-") {
                if (!is_number(nr)) fail(where ": the " arch " number of " name " is " nr ", not a number or -")
                if ((arch, nr) in owner) fail(where ": " name " has the " arch " number " nr " of " owner[arch, nr])
                owner[arch, nr] = name
            }
            number[arch, name] = nr
        }
        if ($3 == "-" && $4 == "-") fail(where ": " name " has a number on neither arch")
        if ($5 !~ /^[0-6]$/) fail(where ": " name " takes " $5 " arguments, not 0 to 6")
        args[name] = $5
        syscalls[++nsyscalls] = name
    } else if ($1 == "errno") {
        if (NF < 3) fail(where ": expected errno NAME VALUE DESCRIPTION or errno NAME OTHER")
        name = $2
        if (name !~ /^E[A-Z0-9]+$/) fail(where ": " name " is not an errno name")
        if (name in value) fail(where ": errno " name " is listed twice")
        v = $3
        if (is_number(v)) {
            if (v + 0 < 1 || v + 0 > 4095) fail(where ": " name " is " v ", not 1 to 4095")
            if (v in canonical) fail(where ": " name " has the value of " canonical[v] ": write errno " name " " canonical[v])
            if (NF < 4) fail(where ": errno " name " has no description")
            desc = $4
            for (i = 5; i <= NF; i++) desc = desc " " $i
            if (index(desc, "/*") || index(desc, "*/")) fail(where ": the description of " name " contains /* or */")
            canonical[v] = name
            other[name] = ""
        } else if (v ~ /^E[A-Z0-9]+$/) {
            if (!(v in value) || other[v] != "") fail(where ": " name " names " v ", which is not an errno with a value above it")
            if (NF != 3) fail(where ": errno " name " " v " takes no description")
            desc = "same as " v
            other[name] = v
            v = value[v]
        } else {
            fail(where ": the value of " name " is " v ", not a number or an errno name")
        }
        value[name] = v
        text[name] = desc
        errnos[++nerrnos] = name
        if (length(lc_name(name)) > width) width = length(lc_name(name))
    } else {
        fail(where ": a line starts with syscall or errno, not " $1)
    }
    next
}

# --- Generated text ---

# "a", "a or b", "a, b or c" for the n strings in words[1..n].
function or_list(n, words,    i, s) {
    s = words[1]
    for (i = 2; i <= n; i++) s = s (i == n ? " or " : ", ") words[i]
    return s
}

function emit_syscall_nr(arch,    guard, i, s, nr, w, max, at, missing, nmissing, fmt, n) {
    guard = "LIGHTC_SYSCALL_NR_" toupper(arch) "_H"
    w = 0
    max = -1
    nmissing = 0
    for (i = 1; i <= nsyscalls; i++) {
        s = syscalls[i]
        nr = number[arch, s]
        if (nr == "-") {
            missing[++nmissing] = "SYS_" s
            continue
        }
        at[nr] = s
        if (nr + 0 > max) max = nr + 0
        if (length(s) > w) w = length(s)
    }
    print "#ifndef " guard
    print "#define " guard
    print ""
    print "/*"
    print " * " arch " syscall numbers, from " source[arch] ","
    print " * in number order, each with the number of arguments it takes."
    if (nmissing > 0) print " * " arch " has no syscall for " or_list(nmissing, missing) "."
    print " *"
    print " * Generated from syscalls.def by tools/gensyscalls.sh: edit syscalls.def,"
    print " * not this file, then run sh tools/gensyscalls.sh."
    print " */"
    print ""
    fmt = "#define SYS_%-" w "s %-4s /* %s */\n"
    for (n = 0; n <= max; n++)
        if (n in at)
            printf fmt, at[n], n, args[at[n]] (args[at[n]] == 1 ? " arg" : " args")
    print ""
    print "#endif /* " guard " */"
}

function emit_errno_enum(    i, e, fmt) {
    fmt = "    %-" width "s = %-4s /* %s: %s */\n"
    for (i = 1; i <= nerrnos; i++) {
        e = errnos[i]
        printf fmt, lc_name(e), value[e] ",", e, text[e]
    }
}

function emit_errno_names(    i, e, fmt) {
    fmt = "    case %-" (width + 1) "s return \"%s\";\n"
    for (i = 1; i <= nerrnos; i++) {
        e = errnos[i]
        if (other[e] == "") printf fmt, lc_name(e) ":", e
    }
}

# Prints "#define MACRO(X)" and X(item) for items[1..n], continued over
# lines of at most 75 columns with the backslashes in column 77.
function emit_list(macro, n, items,    i, line) {
    line = "#define " macro "(X)"
    for (i = 1; i <= n; i++) {
        if (i == 1 || length(line " X(" items[i] ")") > 75) {
            printf "%-76s\\\n", line
            line = "   "
        }
        line = line " X(" items[i] ")"
    }
    print line
}

function emit_abi_syscalls(    i, s, both, nboth, only, nonly, j, arch, lacking) {
    nboth = 0
    for (i = 1; i <= nsyscalls; i++) {
        s = syscalls[i]
        if (number["x86_64", s] != "-" && number["aarch64", s] != "-") both[++nboth] = s
    }
    emit_list("SYSCALLS", nboth, both)
    for (j = 1; j <= 2; j++) {
        arch = arches[j]
        lacking = arches[3 - j]
        nonly = 0
        for (i = 1; i <= nsyscalls; i++) {
            s = syscalls[i]
            if (number[lacking, s] == "-") only[++nonly] = s
        }
        emit_list("SYSCALLS_" toupper(arch) "_ONLY", nonly, only)
    }
    for (j = 1; j <= 2; j++) {
        arch = arches[j]
        for (i = 1; i <= nsyscalls; i++) {
            s = syscalls[i]
            if (number[arch, s] != "-") continue
            print "#if defined(__" arch "__) && defined(__NR_" s ")"
            print "#error \"syscalls.def gives " s " no " arch " number, but " arch " has __NR_" s "\""
            print "#endif"
        }
    }
}

function emit_abi_errnos(    i, names) {
    for (i = 1; i <= nerrnos; i++) names[i] = substr(errnos[i], 2)
    emit_list("ERRNOS", nerrnos, names)
}

function emit(region) {
    if (region == "errno-enum") emit_errno_enum()
    else if (region == "errno-names") emit_errno_names()
    else if (region == "abi-syscalls") emit_abi_syscalls()
    else if (region == "abi-errnos") emit_abi_errnos()
}

# --- The file with regions ---

# The region a "/* KIND GENERATED region" line names, "?" when it names none,
# or "" for any other line.
function marker(kind,    s) {
    s = $0
    if (!sub("^[ \t]*/[*] " kind " GENERATED ", "", s)) return ""
    if (!match(s, /^[a-z][a-z-]*/)) return "?"
    return substr(s, 1, RLENGTH)
}

BEGIN {
    failed = 0
    nsyscalls = 0
    nerrnos = 0
    width = 0
    inside = ""
    arches[1] = "x86_64"
    arches[2] = "aarch64"
    source["x86_64"] = "arch/x86/entry/syscalls/syscall_64.tbl"
    source["aarch64"] = "include/uapi/asm-generic/unistd.h"
    nwanted = split(regions, list, " ")
    for (i = 1; i <= nwanted; i++) wanted[list[i]] = 1
}

{
    where = FILENAME ":" FNR
    starts = marker("BEGIN")
    ends = marker("END")
    if (inside != "") {
        if (starts != "") fail(where ": a region starts inside region " inside)
        if (ends == "") next
        if (ends != inside) fail(where ": this ends region " ends ", not region " inside)
        print
        inside = ""
        next
    }
    if (ends != "") fail(where ": region " ends " ends here but did not start")
    print
    if (starts == "") next
    if (!(starts in wanted)) fail(where ": this file has no region named " starts)
    if (starts in seen) fail(where ": region " starts " is here a second time")
    seen[starts] = 1
    inside = starts
    emit(starts)
}

END {
    if (failed) exit 1
    if (what == "x86_64" || what == "aarch64") {
        emit_syscall_nr(what)
        exit 0
    }
    if (inside != "") fail(FILENAME ": region " inside " does not end")
    for (i = 1; i <= nwanted; i++)
        if (!(list[i] in seen)) fail(FILENAME ": region " list[i] " is missing")
}
'

# generate OUTPUT WHAT [REGIONS]: writes what OUTPUT will hold to $work.
generate() {
    new="$work/$(echo "$1" | tr / _)"
    if [ "$2" = splice ]; then
        awk -v what=splice -v regions="$3" "$GEN" "$DEF" "$1" > "$new"
    else
        awk -v what="$2" "$GEN" "$DEF" > "$new"
    fi
}

generate include/lightc/arch/x86_64/syscall_nr.h x86_64
generate include/lightc/arch/aarch64/syscall_nr.h aarch64
generate include/lightc/result.h splice "errno-enum errno-names"
generate tests/abi_check.c splice "abi-syscalls abi-errnos"

# Every output was generated, so now replace the ones that changed.
for out in include/lightc/arch/x86_64/syscall_nr.h \
           include/lightc/arch/aarch64/syscall_nr.h \
           include/lightc/result.h \
           tests/abi_check.c; do
    new="$work/$(echo "$out" | tr / _)"
    if ! cmp -s "$new" "$out"; then
        cat "$new" > "$out"
        echo "updated $out"
    fi
done
