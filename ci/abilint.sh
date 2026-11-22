#!/bin/sh
# lightc — kernel ABI lint
#
# Two checks of the sources that the compile of tests/abi_check.c against
# the UAPI headers cannot make:
#
#   arity   Each call lc_syscallN(SYS_name, ...) has the N that syscalls.def
#           gives name as its ARGS, and name is in syscalls.def. A wrong N
#           compiles, since lc_syscallN takes N plain integers whatever the
#           syscall, and the kernel then takes the arguments the call left
#           out from whatever the registers hold. The calls checked are
#           those in every .c and .h file under include/, src/, tests/,
#           examples/ and bench/; arch/ only defines lc_syscall0 to 6.
#           Comments, string literals and character literals are skipped,
#           and a call may span lines, in a macro too: a backslash at the
#           end of a line is a space. Only the prototypes in
#           <lightc/syscall.h>, "lc_syscallN(int64_t nr)", are not calls.
#           Any other lc_syscallN that is not a call whose first argument is
#           a SYS_ name followed by "," or ")" fails, as the lint cannot
#           tell which syscall it makes.
#   macros  Each macro that a header under include/ or src/ defines is named
#           in exactly one of the lists of tests/abi_check.c: those it
#           compares with the UAPI value (UAPI_CONSTANTS,
#           UAPI_CONSTANTS_X86_64, UAPI_RENAMED, UAPI_FUNCTIONS), and
#           NO_UAPI_CHECK and LIGHTC_OWN, which give the reason a macro has
#           no such check. So a new constant fails here until it gets a
#           check or a reason. Include guards, an #ifndef NAME and a
#           #define NAME with no value on the next line, are left out, and
#           so are the syscall_nr.h files under include/lightc/arch/:
#           tools/gensyscalls.sh generates them from syscalls.def, and
#           abi_check.c checks each SYS_* they define. Each name in those
#           lists must be a macro of those headers, and each name that
#           abi_check.c captures under its own name (UAPI_CONSTANTS,
#           UAPI_CONSTANTS_X86_64, UAPI_FUNCTIONS) must be #undef'd there,
#           or lightc's definition could be skipped or clash with the UAPI's.
#
# Each failure is a line "FAIL: FILE:LINE: what is wrong". It runs both
# checks before it exits 1 when either failed, and it also fails when it
# finds no call or no macro, so a wrong tree cannot pass. It needs only a
# POSIX sh and awk, and awk reads no hex.
#
# Usage, from any directory: sh ci/abilint.sh [ROOT]
# ROOT is the tree to check, by default the one this script is in.

set -eu

if [ $# -gt 0 ]; then
    root=$1
else
    root=$(dirname -- "$0")/..
fi
# CDPATH= keeps cd from looking the directory up in the caller's CDPATH.
CDPATH= cd -- "$root"

# Byte semantics for awk's length() and regular expressions in every locale.
LC_ALL=C
export LC_ALL

for f in syscalls.def tests/abi_check.c; do
    [ -f "$f" ] || { echo "FAIL: $f: no such file"; exit 1; }
done

# The C sources and the headers, in a stable order. No path has a space.
sources=$(find include src tests examples bench -name '*.[ch]' 2>/dev/null | sort)
headers=$(find include src -name '*.h' ! -path 'include/lightc/arch/*/syscall_nr.h' 2>/dev/null | sort)

# strip(line) returns the line without its comments and with each string or
# character literal emptied, so that what is left is code. A comment that
# does not end on its line sets in_comment, which the next line continues.
# A literal that does not end on its line ends there.
STRIP='
BEGIN {
    quote = sprintf("%c", 39)
    special = "/[*]|//|\"|" quote
}

function strip(s,    out, c, tok) {
    out = ""
    while (s != "") {
        if (in_comment) {
            c = index(s, "*/")
            if (c == 0)
                return out
            s = substr(s, c + 2)
            in_comment = 0
            out = out " "
            continue
        }
        if (!match(s, special))
            return out s
        out = out substr(s, 1, RSTART - 1)
        tok = substr(s, RSTART, RLENGTH)
        s = substr(s, RSTART + RLENGTH)
        if (tok == "//")
            return out
        if (tok == "/*") {
            in_comment = 1
            continue
        }
        while (s != "") {
            c = substr(s, 1, 1)
            s = substr(s, 2)
            if (c == "\\")
                s = substr(s, 2)
            else if (c == tok)
                break
        }
        out = out tok tok
    }
    return out
}

function fail(msg) {
    print "FAIL: " msg
    failures++
}
'

# Reads syscalls.def, then the sources. Each source is kept, stripped, in
# text, and scanned for calls once it has been read whole.
ARITY='
NR == FNR {
    sub(/#.*/, "")
    if ($1 == "syscall")
        args[$2] = $5
    next
}

FNR == 1 {
    scan()
    file = FILENAME
    text = ""
    in_comment = 0
}

# A backslash-newline, which joins the lines of a macro, is a space, and
# the newline stays to count the lines.
{
    code = strip($0)
    sub(/\\[ \t]*$/, " ", code)
    text = text code "\n"
}

# The calls in text, the stripped lines of file.
function scan(    rest, line, pre, prev, n, s, ident, c, where, name, found, noun) {
    rest = text
    line = 1
    prev = ""
    found = 0
    while (match(rest, /lc_syscall[0-6]/)) {
        pre = substr(rest, 1, RSTART - 1)
        if (RSTART > 1)
            prev = substr(pre, RSTART - 1, 1)
        line += gsub(/\n/, "", pre)
        n = substr(rest, RSTART + 10, 1)
        rest = substr(rest, RSTART + 11)
        # Part of a longer name, such as my_lc_syscall3 or lc_syscall30.
        if (prev ~ /[A-Za-z0-9_]/ || rest ~ /^[A-Za-z0-9_]/) {
            prev = n
            continue
        }
        prev = n
        where = file ":" line ": "
        # The first argument is near: a window keeps this linear.
        s = substr(rest, 1, 300)
        sub(/^[ \t\n]*/, "", s)
        if (substr(s, 1, 1) != "(") {
            fail(where "lc_syscall" n " is not called here, so its arity cannot be checked")
            continue
        }
        s = substr(s, 2)
        sub(/^[ \t\n]*/, "", s)
        ident = ""
        if (match(s, /^[A-Za-z_][A-Za-z0-9_]*/)) {
            ident = substr(s, 1, RLENGTH)
            s = substr(s, RLENGTH + 1)
        }
        sub(/^[ \t\n]*/, "", s)
        if (ident == "int64_t")
            continue
        calls++
        found = 1
        c = substr(s, 1, 1)
        if (ident !~ /^SYS_[a-z0-9_]+$/ || (c != "," && c != ")")) {
            fail(where "the first argument of lc_syscall" n " is not a SYS_ name, so its arity cannot be checked")
            continue
        }
        name = substr(ident, 5)
        if (!(name in args)) {
            fail(where "lc_syscall" n "(" ident ", ...): " name " is not in syscalls.def")
            continue
        }
        if (args[name] != n) {
            noun = args[name] == "1" ? "argument" : "arguments"
            fail(where "lc_syscall" n "(" ident ", ...): " name " takes " args[name] " " noun " in syscalls.def, not " n)
            continue
        }
    }
    files_with_calls += found
}

END {
    scan()
    if (calls == 0) {
        fail("found no lc_syscallN call in include/, src/, tests/, examples/ or bench/")
    } else if (failures == 0) {
        print "arity: the " calls " lc_syscallN calls in " files_with_calls " files have the arity of syscalls.def"
    } else {
        print "arity: " failures " of the " calls " lc_syscallN calls in " files_with_calls " files failed"
    }
    exit (failures > 0 ? 1 : 0)
}
'

# Reads tests/abi_check.c, then the headers.
MACROS='
BEGIN {
    nlists = split("UAPI_CONSTANTS UAPI_CONSTANTS_X86_64 UAPI_RENAMED UAPI_FUNCTIONS NO_UAPI_CHECK LIGHTC_OWN", lists, " ")
    for (i = 1; i <= nlists; i++)
        known[lists[i]] = 1
    same_name["UAPI_CONSTANTS"] = 1
    same_name["UAPI_CONSTANTS_X86_64"] = 1
    same_name["UAPI_FUNCTIONS"] = 1
}

FNR == 1 {
    in_comment = 0
    inside = ""
    after_ifndef = ""
    if (NR != FNR)
        headers++
}

# tests/abi_check.c: the names in its lists, and its #undefs. A list is a
# "#define LIST(X)" line and its continuation lines, which end in "\".
NR == FNR {
    raw = $0
    s = strip($0)
    if (inside == "") {
        if (match(s, /^[ \t]*#[ \t]*define[ \t]+[A-Z0-9_]+\(X\)/)) {
            name = substr(s, RSTART, RLENGTH)
            sub(/^[ \t]*#[ \t]*define[ \t]+/, "", name)
            sub(/\(X\)$/, "", name)
            if (name in known) {
                inside = name
                have_list[name] = 1
                s = substr(s, RSTART + RLENGTH)
            }
        } else if (match(s, /^[ \t]*#[ \t]*undef[ \t]+[A-Za-z_][A-Za-z0-9_]*/)) {
            name = substr(s, RSTART, RLENGTH)
            sub(/^[ \t]*#[ \t]*undef[ \t]+/, "", name)
            undefined[name] = 1
        }
    }
    if (inside != "") {
        # The space lets an X( at the start of s match too.
        s = " " s
        while (match(s, /[^A-Za-z0-9_]X\([ \t]*[A-Za-z_][A-Za-z0-9_]*/)) {
            name = substr(s, RSTART, RLENGTH)
            sub(/^.*X\([ \t]*/, "", name)
            s = substr(s, RSTART + RLENGTH)
            if (name in list) {
                fail(FILENAME ":" FNR ": " name " is in " list[name] " and in " inside)
                continue
            }
            list[name] = inside
            listed_at[name] = FILENAME ":" FNR
            listed[++nlisted] = name
        }
        if (raw !~ /\\$/)
            inside = ""
    }
    next
}

# A header: each #define but an include guard.
{
    s = strip($0)
    if (match(s, /^[ \t]*#[ \t]*define[ \t]+[A-Za-z_][A-Za-z0-9_]*/)) {
        name = substr(s, RSTART, RLENGTH)
        sub(/^[ \t]*#[ \t]*define[ \t]+/, "", name)
        if (name == after_ifndef && substr(s, RSTART + RLENGTH) ~ /^[ \t]*$/) {
            after_ifndef = ""
            next
        }
        macros++
        if (!(name in defined_at)) {
            defined_at[name] = FILENAME ":" FNR
            defined[++ndefined] = name
        }
    }
    after_ifndef = ""
    if (match(s, /^[ \t]*#[ \t]*ifndef[ \t]+[A-Za-z_][A-Za-z0-9_]*/)) {
        after_ifndef = substr(s, RSTART, RLENGTH)
        sub(/^[ \t]*#[ \t]*ifndef[ \t]+/, "", after_ifndef)
    }
}

END {
    for (i = 1; i <= nlists; i++)
        if (!(lists[i] in have_list))
            fail("tests/abi_check.c: it has no list " lists[i] "(X)")
    for (i = 1; i <= ndefined; i++) {
        name = defined[i]
        if (!(name in list))
            fail(defined_at[name] ": " name " is in no list of tests/abi_check.c: check it against the UAPI there, or add it to NO_UAPI_CHECK or LIGHTC_OWN with the reason")
    }
    for (i = 1; i <= nlisted; i++) {
        name = listed[i]
        if (!(name in defined_at))
            fail(listed_at[name] ": " name " is in " list[name] ", but no header under include/ or src/ defines it")
        else if ((list[name] in same_name) && !(name in undefined))
            fail(listed_at[name] ": " name " is in " list[name] ", but abi_check.c does not #undef it before it includes the lightc headers")
    }
    if (macros == 0) {
        fail("found no macro in the headers under include/ and src/")
    } else if (failures == 0) {
        print "macros: each of the " ndefined " macros of " headers " headers is in one list of tests/abi_check.c"
    } else {
        noun = failures == 1 ? "failure" : "failures"
        print "macros: " failures " " noun " in the " ndefined " macros of " headers " headers and the lists of tests/abi_check.c"
    }
    exit (failures > 0 ? 1 : 0)
}
'

# $sources and $headers are split into words: no path has a space.
status=0
awk "$STRIP$ARITY" syscalls.def $sources || status=1
awk "$STRIP$MACROS" tests/abi_check.c $headers || status=1
exit $status
