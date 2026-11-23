#!/bin/sh
# lightc — public API snapshot
#
# Writes the snapshot of lightc's public API that api/ holds: four files of
# one item per line, each line meaningful alone, so that a diff of two
# snapshots lists what changed between them.
#
#   symbols.txt         each global symbol liblightc.a defines, with its nm
#                       type: "T lc_heap_allocate"
#   decls.txt           each declaration of the public headers, after the
#                       header that makes it:
#                       "lightc/log.h: void lc_log_set_output(int32_t fd);"
#   layout-x86_64.txt   the size and alignment of each struct the public
#   layout-aarch64.txt  headers define, and the offset, size and type of each
#                       of its fields, on that arch:
#                       "lc_writer size 32 align 8"
#                       "lc_writer.fd offset 0 size 4: int32_t"
#
# The public headers are every *.h under include/ but the syscall_nr.h
# files, as in ci/headers.sh. The library's objects are those that the
# liblightc.a edge of configure lists, built from the sources that their cc
# and as edges name.
#
# symbols.txt: the objects are compiled for each arch, with configure's
# flags at -O0, and nm lists the symbols they define that are not local, so
# a static function never appears. The debug, release, tiny and ubsan
# builds define the same symbols, so -O0 gives those of every mode.
#
# decls.txt: one translation unit that includes every public header is
# preprocessed with -E, and only the text that the line markers attribute
# to a file under include/ is read, so nothing of the compiler's own
# <stdint.h>, <stddef.h>, <stdbool.h> or <stdatomic.h> gets in. That text
# is split into tokens and each declaration is printed from its tokens with
# fixed spacing, so the headers' own spacing, line breaks and comments
# change nothing. A function definition, such as a static inline function,
# is printed as its declaration. The members of a struct are left out, as
# "{ ... }", since the layout files give them per arch, and each enumerator
# has a line of its own, with its value:
#   "lightc/log.h: typedef enum { LC_LOG_INFO = 1, ... } lc_log_level;"
# _Static_assert declarations and macros are not in it.
#
# layout-ARCH.txt: a translation unit that defines a char array of
# sizeof, _Alignof or offsetof plus one bytes for each struct and field is
# compiled with -S, and the sizes are read from the .size directives of the
# assembly. Nothing is run, so both arches are measured on any host.
#
# This handles only the forms the headers use: struct and enum bodies in a
# typedef or after a tag, opaque structs, plain fields, arrays and
# _Atomic(T) fields, and enumerators whose value is an integer literal or
# the one before plus one. A union, a bit-field, a struct inside a struct, a
# function pointer field, an attribute between struct or enum and its body
# or anything else it cannot read stops it with the header and line, so
# nothing is left out unseen.
#
# Each file is generated with and without NDEBUG, which sets LC_STATS and
# with it the fields of lc_arena, and decls.txt and symbols.txt for both
# arches too. A line that not every one of these has ends with what has it:
#   "lc_arena size 24 align 8 # only with NDEBUG"
#   "T lc_foo # only on x86_64", "... # only on aarch64 without NDEBUG"
# The lines are sorted with LC_ALL=C: decls.txt by header, then by the name
# declared, symbols.txt by name, and the layout files by struct, each
# struct's line first, then its fields by offset.
#
# The compiler gives the layout's numbers, which the psABI fixes, and the
# symbol types. The text of decls.txt and of the field types is that of
# lightc's headers, but after the preprocessor, so a macro of the compiler's
# headers that a declaration uses would bring in the compiler's text. The
# headers use two: NULL, only in function bodies, which are left out, and
# bool, which GCC 13 and later leave a keyword in C23 mode, where older ones
# define it as _Bool. A _Bool in a declaration or field type stops the tool,
# so such a compiler fails rather than changing the snapshot. GCC 13 and
# GCC 14 write the same bytes.
#
# The compilers are CC_X86_64 and CC_AARCH64, by default the first of
# ARCH-linux-gnu-gcc and gcc that targets ARCH. nm is the one the compiler
# names with -print-prog-name=nm, else TARGET-nm, else nm.
#
# Usage, from any directory:
#   sh tools/api-snapshot.sh             write the snapshot to api/
#   sh tools/api-snapshot.sh -o DIR      write it to DIR
#   sh tools/api-snapshot.sh -c DIR      write it to DIR, then fail, with a
#                                        diff, when it differs from api/
#                                        (ninja api-check)
#   sh tools/api-snapshot.sh -a DIR [BASELINE]
#                                        write it to DIR, then fail when it
#                                        lacks a line of BASELINE, for any of
#                                        the variants that line holds, so it
#                                        passes when lines are only added
#                                        (ninja api-additive). BASELINE is a
#                                        directory of snapshot files,
#                                        api/vX.Y.Z of the newest release by
#                                        default. A line counts as a whole,
#                                        so renaming a parameter or adding
#                                        an attribute changes it too.
# Run it after an intended change to the API and commit api/ with the
# change. At a release, write the release's snapshot to api/vX.Y.Z with -o.
# It exits 1 when a check fails or when it cannot generate the snapshot.

set -eu

here=$(pwd)

# CDPATH= keeps cd from looking up "tools/.." in the caller's CDPATH.
CDPATH= cd -- "$(dirname -- "$0")/.."

# Byte semantics for awk, sort and the regular expressions in every locale.
LC_ALL=C
export LC_ALL

FILES="symbols.txt decls.txt layout-x86_64.txt layout-aarch64.txt"

usage() {
    echo "usage: sh tools/api-snapshot.sh [-o DIR | -c DIR | -a DIR [BASELINE]]" >&2
    exit 1
}

# A directory named on the command line, from the caller's directory.
from_caller() {
    case "$1" in
        /*) echo "$1" ;;
        *)  if [ "$here" = "$(pwd)" ]; then echo "$1"; else echo "$here/$1"; fi ;;
    esac
}

mode=write
out=api
baseline=
case $# in
    0) ;;
    2|3)
        case "$1" in
            -o|-c) [ $# -eq 2 ] || usage ;;
            -a)    [ $# -eq 2 ] || baseline=$(from_caller "$3") ;;
            *)     usage ;;
        esac
        mode=$1
        out=$(from_caller "$2")
        ;;
    *) usage ;;
esac

# The baseline of -a, by default the newest api/vX.Y.Z.
if [ "$mode" = -a ] && [ -z "$baseline" ]; then
    baseline=$(for d in api/v*; do
                   [ -d "$d" ] && printf '%s\n' "${d#api/}"
               done | awk -F . '
                   /^v[0-9]+\.[0-9]+\.[0-9]+$/ {
                       k = sprintf("%09d%09d%09d", substr($1, 2), $2, $3)
                       if (k > best) { best = k; name = $0 }
                   }
                   END { if (name != "") print "api/" name }')
    if [ -z "$baseline" ]; then
        echo "api-snapshot: api/ has no release baseline api/vX.Y.Z" >&2
        exit 1
    fi
fi
if [ "$mode" = -a ] && [ ! -d "$baseline" ]; then
    echo "api-snapshot: $baseline is not a directory" >&2
    exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# --- The compilers ---

# compiler_for ARCH: prints the compiler for ARCH.
compiler_for() {
    case "$1" in
        x86_64)  cc=${CC_X86_64:-} var=CC_X86_64 ;;
        aarch64) cc=${CC_AARCH64:-} var=CC_AARCH64 ;;
    esac
    if [ -n "$cc" ]; then
        # $cc is split into words, as configure splits CC.
        target=$($cc -dumpmachine 2>/dev/null) || target=
        if [ "${target%%-*}" != "$1" ]; then
            echo "api-snapshot: $var=$cc does not target $1" >&2
            exit 1
        fi
        echo "$cc"
        return
    fi
    for cc in "$1-linux-gnu-gcc" gcc; do
        if command -v "$cc" >/dev/null 2>&1 &&
                target=$("$cc" -dumpmachine 2>/dev/null) &&
                [ "${target%%-*}" = "$1" ]; then
            echo "$cc"
            return
        fi
    done
    echo "api-snapshot: no GCC for $1: install $1-linux-gnu-gcc or set $var" >&2
    exit 1
}

# nm_for CC: prints the nm for CC's target.
nm_for() {
    nm=$($1 -print-prog-name=nm 2>/dev/null) || nm=
    case "$nm" in
        */*) if [ -x "$nm" ]; then echo "$nm"; return; fi ;;
    esac
    target=$($1 -dumpmachine)
    if command -v "$target-nm" >/dev/null 2>&1; then
        echo "$target-nm"
        return
    fi
    echo nm
}

# --- The awk programs ---

# Reads the preprocessed headers and prints, a tab between fields:
#   decl HEADER NAME TEXT     a line of decls.txt: HEADER declares NAME
#   struct CNAME              a struct the layout files measure, as sizeof
#                             takes it: lc_writer, struct lc_scheduler
#   field CNAME NAME TYPE     one of its fields, in order
PARSE='
function fail(k, msg) {
    if (k) msg = tfile[k] ":" tline[k] ": " msg
    print "api-snapshot: " msg | "cat 1>&2"
    failed = 1
    exit 1
}

function is_ident(t) {
    return t ~ /^[A-Za-z_][A-Za-z_0-9]*$/
}

# "a struct", "an enum"
function article(w) {
    return (w ~ /^[aeiou]/ ? "an " : "a ") w
}

# --- Tokens ---

function tokenize(s,    n) {
    while (s != "") {
        if (match(s, /^[ \t\r\f]+/)) {
            s = substr(s, RLENGTH + 1)
            continue
        }
        if (match(s, /^[A-Za-z_][A-Za-z_0-9]*/) ||
                match(s, /^\.?[0-9]([0-9A-Za-z_.]|[eEpP][-+])*/) ||
                match(s, /^"([^"\\]|\\.)*"/) ||
                match(s, /^\047([^\047\\]|\\.)*\047/))
            n = RLENGTH
        else if (substr(s, 1, 3) in punct)
            n = 3
        else if (substr(s, 1, 2) in punct)
            n = 2
        else
            n = 1
        ntok++
        tok[ntok] = substr(s, 1, n)
        tfile[ntok] = file
        tline[ntok] = line
        s = substr(s, n + 1)
    }
}

# Is there a space between tokens p and t, where nx follows t?
function spaced(p, t, nx) {
    if (p == "(" || p == "[" || p == "*" || p == "::") return 0
    if (t == ")" || t == "]" || t == "," || t == ";" || t == "::") return 0
    if (t == "[") return p == ","
    if (t == "(") return !(is_ident(p) || p == ")" || p == "]") || (is_ident(p) && nx == "*")
    return 1
}

# Tokens from..to as text. The headers write bool, which a compiler whose
# <stdbool.h> defines it as a macro turns into _Bool.
function text(from, to,    k, s) {
    s = ""
    for (k = from; k <= to; k++) {
        if (tok[k] == "_Bool")
            fail(k, "a _Bool: write bool, and use a compiler that keeps bool a keyword, such as GCC 13 or later")
        if (k > from && spaced(tok[k - 1], tok[k], k < to ? tok[k + 1] : "")) s = s " "
        s = s tok[k]
    }
    return s
}

# The index of the } that closes the { at k.
function closing(k,    j, depth) {
    depth = 0
    for (j = k; j <= ntok; j++) {
        if (tok[j] == "{") depth++
        else if (tok[j] == "}" && --depth == 0) return j
    }
    fail(k, "a { that is not closed")
}

# The index of the first token from k that is not in a [[...]] attribute.
function skip_attributes(k, end,    depth) {
    while (k < end && tok[k] == "[" && tok[k + 1] == "[") {
        depth = 0
        for (; k < end; k++) {
            if (tok[k] == "[") depth++
            else if (tok[k] == "]" && --depth == 0) break
        }
        k++
    }
    return k
}

# Has from..to a comma outside parentheses and brackets?
function top_comma(from, to,    k, depth) {
    depth = 0
    for (k = from; k <= to; k++) {
        if (tok[k] == "(" || tok[k] == "[") depth++
        else if (tok[k] == ")" || tok[k] == "]") depth--
        else if (tok[k] == "," && depth == 0) return 1
    }
    return 0
}

# --- Declarations ---

function emit(k, name, s) {
    print "decl\t" substr(tfile[k], 9) "\t" name "\t" s
}

# The index of the name that from..to declares: the identifier before the
# first ( outside parentheses that follows an identifier, or the one inside
# it when it starts a declarator such as (*name); else the last identifier
# outside parentheses and brackets, but __attribute__ and the like.
function declarator_name(from, to,    k, depth, last, m) {
    depth = 0
    last = 0
    for (k = from; k <= to; k++) {
        if (tok[k] == "(" && depth == 0 && k > from && is_ident(tok[k - 1]) && !(tok[k - 1] in not_declarator)) {
            if (tok[k + 1] != "*") return k - 1
            m = k + 1
            while (m <= to && (tok[m] == "*" || tok[m] in qualifiers)) m++
            if (m <= to && is_ident(tok[m])) return m
            fail(k, "a declarator in parentheses with no name")
        }
        if (tok[k] == "(" || tok[k] == "[") depth++
        else if (tok[k] == ")" || tok[k] == "]") depth--
        else if (depth == 0 && is_ident(tok[k]) && !(tok[k] in not_declarator)) last = k
    }
    if (!last) fail(from, "a declaration with no name")
    return last
}

# "struct", "union" or "enum" when the { at k opens the body of one.
function aggregate(start, k) {
    if (k - 1 >= start && tok[k - 1] in aggregates) return tok[k - 1]
    if (k - 2 >= start && is_ident(tok[k - 1]) && tok[k - 2] in aggregates) return tok[k - 2]
    return ""
}

# Reads the declaration that starts at token start, and returns the index
# of the token after it.
function declaration(start,    k, t, depth, ob, cb, body, j) {
    depth = 0
    ob = 0
    for (k = start; k <= ntok; k++) {
        t = tok[k]
        if (t == "(" || t == "[") depth++
        else if (t == ")" || t == "]") depth--
        else if (t == "{") {
            body = closing(k)
            if (depth == 0 && aggregate(start, k) != "") {
                if (ob) fail(k, "a second struct, union or enum body in one declaration")
                ob = k
                cb = body
                k = body
                continue
            }
            # The { of struct __attribute__((packed)) { follows a ), as that
            # of a function body does.
            if (depth == 0)
                for (j = ob ? cb + 1 : start; j < k; j++)
                    if (tok[j] in aggregates && !(is_ident(tok[j + 1]) && !(tok[j + 1] in not_declarator)))
                        fail(j, article(tok[j]) " with something other than a tag before its body, such as an attribute")
            if (depth == 0 && !ob && k > start && tok[k - 1] == ")") {
                function_definition(start, k - 1)
                return body + 1
            }
            fail(k, "a { that opens neither a function body nor a struct, union or enum body")
        }
        else if (t == "}") fail(k, "a } with no { before it")
        else if (t == ";" && depth == 0) {
            finish(start, k, ob, cb)
            return k + 1
        }
    }
    fail(start, "a declaration with no ; after it")
}

function function_definition(from, to,    n) {
    n = declarator_name(from, to)
    if (tok[n + 1] != "(" || top_comma(from, to))
        fail(from, "a { after a declaration that does not declare one function")
    emit(from, tok[n], text(from, to) ";")
}

# The declaration start..end, where end is its ;, with the { at ob and
# the } at cb of the struct, union or enum body in it, if any (else ob is 0).
function finish(start, end, ob, cb,    first, n) {
    if (end == start) return
    first = skip_attributes(start, end)
    if (tok[first] == "_Static_assert" || tok[first] == "static_assert") return
    if (ob) {
        aggregate_declaration(start, end, ob, cb)
        return
    }
    if (top_comma(start, end - 1)) fail(start, "a declaration of more than one name")
    n = declarator_name(start, end - 1)
    emit(start, tok[n], text(start, end - 1) ";")
}

function aggregate_declaration(start, end, ob, cb,    kw, tag, at, name, cname, head, tail) {
    kw = aggregate(start, ob)
    if (tok[ob - 1] == kw) {
        at = ob - 1
        tag = ""
    } else {
        at = ob - 2
        tag = tok[ob - 1]
    }
    if (kw == "union") fail(ob, "a union: only struct and enum bodies are read")
    if (at == start + 1 && tok[start] == "typedef") {
        if (cb + 2 != end || !is_ident(tok[cb + 1]))
            fail(cb, "a typedef of " article(kw) " body that does not name one type")
        name = tok[cb + 1]
        cname = name
        head = "typedef " kw (tag != "" ? " " tag : "")
        tail = " " name ";"
    } else if (at == start) {
        if (cb + 1 != end) fail(cb, article(kw) " body with a declarator that is not a typedef name")
        if (tag == "" && kw == "struct") fail(ob, "a struct with neither a tag nor a typedef name")
        name = tag
        cname = kw " " tag
        head = kw (tag != "" ? " " tag : "")
        tail = ";"
    } else {
        fail(start, article(kw) " body after something other than typedef")
    }
    if (kw == "enum") {
        enumerators(start, ob + 1, cb - 1, head, tail)
        return
    }
    emit(start, name, head " { ... }" tail)
    print "struct\t" cname
    members(ob + 1, cb - 1, cname)
}

# --- Struct members ---

function members(from, to, cname,    a, k, t, depth) {
    a = from
    depth = 0
    for (k = from; k <= to; k++) {
        t = tok[k]
        if (t == "(" || t == "[") depth++
        else if (t == ")" || t == "]") depth--
        else if (t == "{") fail(k, "a struct, union or enum body inside a struct")
        else if (t == ":" && depth == 0) fail(k, "a bit-field")
        else if (t == ";" && depth == 0) {
            member(a, k - 1, cname)
            a = k + 1
        }
    }
    if (a <= to) fail(a, "a struct member with no ; after it")
}

# The index of the name of the member declarator from..to: the last
# identifier outside parentheses and brackets, with only [N] after it.
function member_name(from, to,    k, t, depth, n) {
    depth = 0
    n = 0
    for (k = from; k <= to; k++) {
        t = tok[k]
        if (t == "(" || t == "[") depth++
        else if (t == ")" || t == "]") depth--
        else if (depth == 0 && is_ident(t) && !(t in qualifiers)) n = k
    }
    if (!n) fail(from, "a struct member with no name")
    depth = 0
    for (k = n + 1; k <= to; k++) {
        t = tok[k]
        if (depth == 0 && t != "[")
            fail(k, "a struct member declarator other than *name or name[N], such as a function pointer")
        if (t == "[") depth++
        else if (t == "]") depth--
    }
    return n
}

# The member declaration a..b, of one or more fields: uint64_t x19, x20.
function member(a, b, cname,    k, t, depth, np, pfrom, pto, p, n, d, spec, type) {
    if (a > b) return
    if (tok[a] == "_Static_assert" || tok[a] == "static_assert") return
    np = 0
    depth = 0
    pfrom[1] = a
    for (k = a; k <= b; k++) {
        t = tok[k]
        if (t == "(" || t == "[") depth++
        else if (t == ")" || t == "]") depth--
        else if (t == "," && depth == 0) {
            pto[++np] = k - 1
            pfrom[np + 1] = k + 1
        }
    }
    pto[++np] = b
    for (p = 1; p <= np; p++) {
        n = member_name(pfrom[p], pto[p])
        # The *s and qualifiers before the name are the declarator.
        d = n
        while (d > pfrom[p] && (tok[d - 1] == "*" || tok[d - 1] in qualifiers)) d--
        if (p == 1) {
            # Qualifiers before the first * belong to the type: const char *.
            while (d < n && tok[d] in qualifiers) d++
            if (d == pfrom[p] || tok[d - 1] in aggregates) fail(n, "a struct member with no declarator")
            spec = text(pfrom[p], d - 1)
        } else if (d != pfrom[p]) {
            fail(pfrom[p], "a struct member declarator other than *name or name[N]")
        }
        type = spec
        if (d < n) type = type " " text(d, n - 1)
        if (n < pto[p]) type = type text(n + 1, pto[p])
        print "field\t" cname "\t" tok[n] "\t" type
    }
}

# --- Enumerators ---

# The value of the integer literal t, or "" when t is not one.
function int_value(t,    base, v, i) {
    sub(/[uUlL]+$/, "", t)
    if (t ~ /^[1-9][0-9]*$/) return t + 0
    if (t ~ /^0[xX][0-9A-Fa-f]+$/) {
        base = 16
        t = substr(t, 3)
    } else if (t ~ /^0[0-7]*$/) {
        base = 8
    } else {
        return ""
    }
    v = 0
    for (i = 1; i <= length(t); i++) v = v * base + index("0123456789abcdef", tolower(substr(t, i, 1))) - 1
    return v
}

function enumerators(start, from, to, head, tail,    k, t, a, depth, v, value) {
    value = 0
    a = from
    depth = 0
    for (k = from; k <= to + 1; k++) {
        if (k <= to) {
            t = tok[k]
            if (t == "(" || t == "[") depth++
            else if (t == ")" || t == "]") depth--
            if (t != "," || depth != 0) continue
        }
        # tok[a..k-1] is one enumerator.
        if (a == k) {
            if (k > to && a > from) break
            fail(a, "an empty enumerator")
        }
        if (!is_ident(tok[a])) fail(a, "an enumerator that does not start with its name")
        if (a == k - 1) {
            v = value
        } else {
            if (tok[a + 1] != "=") fail(a + 1, "an enumerator with something other than = after its name")
            if (a + 3 == k) v = int_value(tok[a + 2])
            else if (a + 4 == k && tok[a + 2] == "-") v = int_value(tok[a + 3])
            else v = ""
            if (v == "") fail(a + 2, "an enumerator whose value is not an integer literal")
            if (a + 4 == k) v = 0 - v
        }
        emit(start, tok[a], head " { " tok[a] " = " v ", ... }" tail)
        value = v + 1
        a = k + 1
    }
}

BEGIN {
    failed = 0
    ntok = 0
    ours = 0
    split("... <<= >>= -> ++ -- << >> <= >= == != && || :: ## += -= *= /= %= &= ^= |=", list, " ")
    for (i in list) punct[list[i]] = 1
    split("const volatile restrict _Atomic", list, " ")
    for (i in list) qualifiers[list[i]] = 1
    split("struct union enum", list, " ")
    for (i in list) aggregates[list[i]] = 1
    split("__attribute__ _Atomic _Alignas alignas typeof __typeof__ typeof_unqual", list, " ")
    for (i in list) not_declarator[list[i]] = 1
    split("sizeof _Alignof alignof _Static_assert static_assert", list, " ")
    for (i in list) not_declarator[list[i]] = 1
}

# A line marker: # LINE "FILE" FLAGS
/^# [0-9]+ "/ {
    file = $0
    sub(/^# [0-9]+ "/, "", file)
    sub(/".*$/, "", file)
    line = $2 + 0
    ours = (file ~ /^include\//)
    next
}

# #pragma and the like: what they could change, the compiler measures.
/^[ \t]*#/ {
    line++
    next
}

{
    if (ours) tokenize($0)
    line++
}

END {
    if (failed) exit 1
    for (i = 1; i <= ntok; ) i = declaration(i)
}
'

# Reads the struct and field lines of PARSE and prints a translation unit
# that defines a char array for each value the layout needs, sized by it
# plus one, and to the file map the line "N\tWHAT" for array lc_api_N.
LAYOUT_C='
function value(expr, what) {
    n++
    print "char lc_api_" n "[(" expr ") + 1];"
    print n "\t" what > map
}

BEGIN {
    FS = "\t"
    n = 0
}

$1 == "struct" {
    value("sizeof(" $2 ")", "struct\t" $2 "\tsize")
    value("_Alignof(" $2 ")", "struct\t" $2 "\talign")
}

$1 == "field" {
    value("offsetof(" $2 ", " $3 ")", "field\t" $2 "\t" $3 "\toffset")
    value("sizeof(((" $2 " *)0)->" $3 ")", "field\t" $2 "\t" $3 "\tsize\t" $4)
}
'

# Reads the map of LAYOUT_C, then the assembly, and prints the lines of a
# layout file, each after its sort key and a tab.
LAYOUT='
function fail(msg) {
    print "api-snapshot: " msg | "cat 1>&2"
    failed = 1
    exit 1
}

function key(cname) {
    sub(/^struct /, "", cname)
    return cname
}

FILENAME == ARGV[1] {
    record[++m] = $0
    next
}

/^[ \t]*\.size[ \t]/ {
    s = $0
    sub(/^[ \t]*\.size[ \t]+/, "", s)
    if (split(s, part, /,[ \t]*/) == 2 && part[1] ~ /^lc_api_[0-9]+$/)
        size[substr(part[1], 8) + 0] = part[2] - 1
}

END {
    if (failed) exit 1
    for (i = 1; i <= m; i++) {
        split(record[i], f, "\t")
        if (!((f[1] + 0) in size)) fail(ARGV[2] ": no .size for lc_api_" f[1])
        v = size[f[1] + 0]
        if (f[2] == "struct" && f[4] == "size") {
            struct_size = v
        } else if (f[2] == "struct") {
            print key(f[3]) " 0\t" f[3] " size " struct_size " align " v
        } else if (f[5] == "offset") {
            offset = v
        } else {
            print key(f[3]) " 1 " sprintf("%012d", offset) "\t" f[3] "." f[4] " offset " offset " size " v ": " f[6]
        }
    }
}
'

# Reads one "KEY\tLINE" file per variant, named in "variants" as
# ARCH/with or ARCH/without (NDEBUG) in the order of the files, and prints
# each line once, after its key and a tab, with " # only ..." after a line
# that not every variant has.
MERGE='
function tag(t,    i, j, b, n, a, s, nhave, first, same, archs_part, nd_part) {
    n = 0
    for (i = 1; i <= nfiles; i++) if ((t, i) in has) n++
    if (n == nfiles) return ""
    nhave = 0
    first = ""
    same = 1
    for (j = 1; j <= narchs; j++) {
        a = archs[j]
        s = ""
        for (b = 1; b <= nnds; b++)
            if ((a, nds[b]) in which && (t, which[a, nds[b]]) in has)
                s = s (s == "" ? "" : " and ") nds[b]
        set[a] = s
        if (s == "") continue
        nhave++
        if (first == "") first = s
        else if (s != first) same = 0
    }
    if (same) {
        archs_part = ""
        if (nhave < narchs)
            for (j = 1; j <= narchs; j++)
                if (set[archs[j]] != "") archs_part = archs_part (archs_part == "" ? "on " : " and ") archs[j]
        nd_part = first == all_nds ? "" : first
        return " # only " archs_part (archs_part != "" && nd_part != "" ? " " : "") nd_part
    }
    s = ""
    for (j = 1; j <= narchs; j++) {
        a = archs[j]
        if (set[a] != "") s = s (s == "" ? "" : ", ") "on " a (set[a] == all_nds ? "" : " " set[a])
    }
    return " # only " s
}

BEGIN {
    nfiles = ARGC - 1
    narchs = 0
    nnds = 0
    split(variants, v, " ")
    for (i = 1; i <= nfiles; i++) {
        file[ARGV[i]] = i
        split(v[i], p, "/")
        nd = p[2] " NDEBUG"
        if (!(p[1] in seen_arch)) {
            seen_arch[p[1]] = 1
            archs[++narchs] = p[1]
        }
        if (!(nd in seen_nd)) {
            seen_nd[nd] = 1
            nds[++nnds] = nd
            all_nds = all_nds (all_nds == "" ? "" : " and ") nd
        }
        which[p[1], nd] = i
    }
}

{
    t = substr($0, index($0, "\t") + 1)
    if (!(t in key)) key[t] = substr($0, 1, index($0, "\t") - 1)
    has[t, file[FILENAME]] = 1
}

END {
    for (t in key) print key[t] "\t" t tag(t)
}
'

# Reads a baseline snapshot file, then the same file of this tree, and fails
# when this tree lacks a line of the baseline for one of the variants the
# line holds: its arches, of those in "arches", and NDEBUG settings, as the
# " # only ..." of MERGE gives them. Prints each such line after "- ", then
# each line this tree added to the group of one of them after "+ ", where
# lines with the same first word are a group: the header in decls.txt, the
# struct in a layout file (without .field), and the name in symbols.txt,
# whose first word is the type.
ADDITIVE='
function bad(line) {
    print "api-snapshot: " FILENAME ": cannot read what has this line: " line | "cat 1>&2"
    failed = 1
    exit 1
}

# Records in have[TEXT, ARCH, ND] each variant the line holds, and returns
# its TEXT, the line without " # only ...".
function add(line, have,    at, t, nitems, items, k, nw, w, j, na, nn, al, nl, x, y, nd) {
    at = index(line, " # only ")
    if (!at) {
        for (x = 1; x <= nall; x++)
            for (y = 1; y <= 2; y++) have[line, all[x], nds[y]] = 1
        return line
    }
    t = substr(line, 1, at - 1)
    nitems = split(substr(line, at + 8), items, ", ")
    for (k = 1; k <= nitems; k++) {
        nw = split(items[k], w, " ")
        na = 0
        nn = 0
        j = 1
        if (w[1] == "on") {
            for (j = 2; j <= nw && w[j] != "with" && w[j] != "without"; j++) {
                if (w[j] == "and") continue
                if (!(w[j] in known_arch)) bad(line)
                al[++na] = w[j]
            }
        }
        for (; j <= nw; j++) {
            if (w[j] == "and") continue
            nd = w[j] " " w[j + 1]
            if (!(nd in known_nd)) bad(line)
            nl[++nn] = nd
            j++
        }
        if (na == 0 && nn == 0) bad(line)
        if (na == 0) for (x = 1; x <= nall; x++) al[++na] = all[x]
        if (nn == 0) {
            nl[1] = nds[1]
            nl[2] = nds[2]
            nn = 2
        }
        for (x = 1; x <= na; x++)
            for (y = 1; y <= nn; y++) have[t, al[x], nl[y]] = 1
    }
    return t
}

function group(t,    w, g) {
    split(t, w, " ")
    if (ARGV[2] ~ /symbols\.txt$/) return w[2]
    # A struct measured by its tag: "struct lc_scheduler size 192 align 8".
    g = w[1] == "struct" ? w[2] : w[1]
    sub(/\..*/, "", g)
    return g
}

# Does a hold a variant of text t that b lacks?
function lacks(t, a, b,    x, y) {
    for (x = 1; x <= nall; x++)
        for (y = 1; y <= 2; y++)
            if ((t, all[x], nds[y]) in a && !((t, all[x], nds[y]) in b)) return 1
    return 0
}

BEGIN {
    failed = 0
    nall = split(arches, all, " ")
    for (x = 1; x <= nall; x++) known_arch[all[x]] = 1
    nds[1] = "with NDEBUG"
    nds[2] = "without NDEBUG"
    known_nd[nds[1]] = 1
    known_nd[nds[2]] = 1
}

FILENAME == ARGV[1] {
    t = add($0, base)
    if (!(t in base_line)) border[++nb] = t
    base_line[t] = $0
    next
}

{
    t = add($0, cur)
    if (!(t in cur_line)) corder[++nc] = t
    cur_line[t] = $0
}

END {
    if (failed) exit 1
    removed = 0
    for (i = 1; i <= nb; i++) {
        if (!lacks(border[i], base, cur)) continue
        print "- " base_line[border[i]]
        changed[group(border[i])] = 1
        removed++
    }
    added = 0
    for (i = 1; i <= nc; i++) {
        if (!lacks(corder[i], cur, base)) continue
        if (group(corder[i]) in changed) print "+ " cur_line[corder[i]]
        added++
    }
    print removed " removed or changed, " added " added" > summary
    exit (removed ? 1 : 0)
}
'

# Reads configure and prints "RULE SOURCE OBJECT" for each object that its
# liblightc.a edge lists, from the cc or as edge that builds the object,
# with arch for $arch: "as arch/x86_64/thread.S thread_asm.o".
LIBRARY='
function fail(msg) {
    print "api-snapshot: configure: " msg | "cat 1>&2"
    failed = 1
    exit 1
}

BEGIN {
    failed = 0
    b = "\\$builddir/"
    nlib = 0
    prev = ""
}

# A variable of the edge on the line before.
/^[ \t]/ {
    if (prev != "") vars[prev] = 1
    next
}

{ prev = "" }

$1 == "build" && substr($2, 1, length(b)) == b && $2 ~ /\.o:$/ && ($3 == "cc" || $3 == "as") {
    o = substr($2, length(b) + 1)
    sub(/:$/, "", o)
    if (o in rule) fail("two edges build " o)
    if (NF != 4) fail("the edge of " o " has more than one input")
    rule[o] = $3
    source[o] = $4
    prev = o
}

$1 == "build" && $2 == b "liblightc.a:" {
    if ($3 != "ar" || nlib) fail("liblightc.a has another edge than one of ar")
    for (i = 4; i <= NF; i++) {
        if (substr($i, 1, length(b)) != b) fail("liblightc.a has an input outside $builddir: " $i)
        lib[++nlib] = substr($i, length(b) + 1)
    }
}

END {
    if (failed) exit 1
    if (!nlib) fail("no edge builds liblightc.a")
    for (i = 1; i <= nlib; i++) {
        o = lib[i]
        if (!(o in rule)) fail("no cc or as edge builds " o ", which liblightc.a lists")
        if (o in vars) fail("the edge of " o " sets a variable")
        s = source[o]
        gsub(/\\\$arch/, arch, s)
        if (index(s, "$")) fail("the source of " o " names a variable other than $arch")
        print rule[o] " " s " " o
    }
}
'

# --- Generate ---

headers=$(find include -name '*.h' ! -path 'include/lightc/arch/*/syscall_nr.h' | LC_ALL=C sort)
if [ -z "$headers" ]; then
    echo "api-snapshot: no headers under include/" >&2
    exit 1
fi
for header in $headers; do
    printf '#include <%s>\n' "${header#include/}"
done > "$work/headers.c"

tab=$(printf '\t')
variants="x86_64/with x86_64/without aarch64/with aarch64/without"

for arch in x86_64 aarch64; do
    cc=$(compiler_for "$arch")
    nm=$(nm_for "$cc")
    # The same tests and flags as configure's.
    if $cc -std=c23 -x c -E /dev/null >/dev/null 2>&1; then
        std=-std=c23
    else
        std=-std=c2x
    fi
    case "$arch" in
        aarch64) arch_flags=-mno-outline-atomics ;;
        *)       arch_flags= ;;
    esac
    awk -v arch="$arch" "$LIBRARY" configure > "$work/library.$arch"
    for nd in with without; do
        v=$arch-$nd
        if [ "$nd" = with ]; then defs=-DNDEBUG; else defs=; fi
        flags="$std -ffreestanding -fno-stack-protector $arch_flags -Iinclude $defs -w"

        # decls.txt
        $cc $flags -E "$work/headers.c" -o "$work/$v.i"
        awk "$PARSE" "$work/$v.i" > "$work/$v.parsed"
        awk -F "$tab" '$1 == "decl" { print $2 " " $3 "\t" $2 ": " $4 }' "$work/$v.parsed" > "$work/decls.$v"

        # layout-ARCH.txt
        {
            echo '#include <stddef.h>'
            cat "$work/headers.c"
            awk -v map="$work/$v.map" "$LAYOUT_C" "$work/$v.parsed"
        } > "$work/$v.layout.c"
        : >> "$work/$v.map"
        $cc $flags -fno-common -S "$work/$v.layout.c" -o "$work/$v.s"
        awk "$LAYOUT" "$work/$v.map" "$work/$v.s" > "$work/layout.$v"

        # symbols.txt. configure assembles the .S files without its C flags,
        # so without NDEBUG.
        mkdir "$work/$v.o"
        while read -r rule source object; do
            case "$rule" in
                cc) $cc $flags -O0 -c "$source" -o "$work/$v.o/$object" ;;
                as) $cc -Iinclude -c "$source" -o "$work/$v.o/$object" ;;
            esac
        done < "$work/library.$arch"
        if ! "$nm" -g --defined-only "$work/$v.o"/*.o > "$work/nm.$v"; then
            echo "api-snapshot: $nm failed on the $v objects" >&2
            exit 1
        fi
        awk 'NF == 3 { print $3 "\t" $2 " " $3 }' "$work/nm.$v" > "$work/symbols.$v"
        if [ ! -s "$work/symbols.$v" ]; then
            echo "api-snapshot: $nm lists no symbols in the $v objects" >&2
            exit 1
        fi
    done
done

# merge OUTPUT VARIANTS FILE...: writes the merged FILEs to OUTPUT, sorted
# by key, without it.
merge() {
    output=$1
    merged_variants=$2
    shift 2
    awk -v variants="$merged_variants" "$MERGE" "$@" | sort | cut -f 2- > "$output"
}

mkdir "$work/out"
merge "$work/out/symbols.txt" "$variants" \
    "$work/symbols.x86_64-with" "$work/symbols.x86_64-without" \
    "$work/symbols.aarch64-with" "$work/symbols.aarch64-without"
merge "$work/out/decls.txt" "$variants" \
    "$work/decls.x86_64-with" "$work/decls.x86_64-without" \
    "$work/decls.aarch64-with" "$work/decls.aarch64-without"
for arch in x86_64 aarch64; do
    merge "$work/out/layout-$arch.txt" "$arch/with $arch/without" \
        "$work/layout.$arch-with" "$work/layout.$arch-without"
done
for f in $FILES; do
    if [ ! -s "$work/out/$f" ]; then
        echo "api-snapshot: $f came out empty" >&2
        exit 1
    fi
done

# Write the files that changed.
mkdir -p "$out"
for f in $FILES; do
    if ! cmp -s "$work/out/$f" "$out/$f"; then
        cat "$work/out/$f" > "$out/$f"
        if [ "$mode" = write ]; then
            echo "updated $out/$f"
        fi
    fi
done

case "$mode" in
    -c)
        differ=0
        for f in $FILES; do
            if [ ! -f "api/$f" ]; then
                diff -u /dev/null "$out/$f" || true
                differ=1
            elif ! cmp -s "api/$f" "$out/$f"; then
                diff -u "api/$f" "$out/$f" || true
                differ=1
            fi
        done
        for f in api/*.txt; do
            [ -f "$f" ] || continue
            case " $FILES " in
                *" ${f#api/} "*) ;;
                *) echo "$f is not a file of the snapshot"; differ=1 ;;
            esac
        done
        if [ "$differ" -ne 0 ]; then
            echo "The public API differs from its snapshot in api/ (-: api/, +: this tree)."
            echo "If the change is intended, run sh tools/api-snapshot.sh and commit api/ with it."
            exit 1
        fi
        echo "api/ matches the public API of this tree."
        ;;
    -a)
        failed=0
        found=0
        for b in "$baseline"/*.txt; do
            [ -f "$b" ] || continue
            found=1
            f=${b##*/}
            case "$f" in
                layout-*.txt) arches=${f#layout-}; arches=${arches%.txt} ;;
                *)            arches="x86_64 aarch64" ;;
            esac
            if [ ! -f "$out/$f" ]; then
                echo "$f: $baseline has it, but this tree's snapshot has no such file"
                failed=1
                continue
            fi
            rm -f "$work/summary"
            if awk -v arches="$arches" -v summary="$work/summary" "$ADDITIVE" "$b" "$out/$f" > "$work/additive"; then
                echo "$f: $(cat "$work/summary")"
            else
                [ -s "$work/summary" ] || exit 1
                echo "$f: $(cat "$work/summary"):"
                cat "$work/additive"
                failed=1
            fi
        done
        if [ "$found" -eq 0 ]; then
            echo "api-snapshot: $baseline holds no snapshot file" >&2
            exit 1
        fi
        if [ "$failed" -ne 0 ]; then
            echo "The public API removes or changes lines of $baseline (-: removed or changed, +: added)."
            echo "Only additions keep it compatible with that release."
            exit 1
        fi
        echo "The public API keeps every line of $baseline."
        ;;
esac
