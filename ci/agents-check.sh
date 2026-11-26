#!/bin/sh
# lightc — check that AGENTS.md names only what exists
#
# AGENTS.md tells people and agents how to work on lightc, so a name in it
# that no longer exists sends them the wrong way. This reads its code: the
# text between backticks and the lines of its fenced blocks, without their
# # comments. Each word there is checked by what it looks like:
#
#   a path            api/index.md, tests/regress/<id>.c: the file or
#                     directory exists, a <...> standing for any name
#   a header          <lightc/io.h>: include/lightc/io.h exists
#   a name of lightc  lc_result, lio_read: api/decls.txt declares it
#   a macro           LIGHTC_TEST_ALLOW_SKIP, LC_ERR_AGAIN: a file under
#                     include/, src/, tests/, ci/ or configure has it
#   a regress id      HEAP-tid-reuse-tls: tests/regress/manifest lists it
#   a suite           test_async: tests/test_async.c exists
#   ninja TARGET...   configure writes a build edge for each target
#   ./configure MODE  configure has the mode
#
# A word with a * or a placeholder in capitals, such as lc_syscallN or
# SYS_NAME, is not checked. Run it from anywhere; it exits 1 on a name that
# does not exist, and prints each one with its line.

set -eu
CDPATH= cd -- "$(dirname -- "$0")/.."
LC_ALL=C
export LC_ALL

file=AGENTS.md
[ -f "$file" ] || { echo "agents-check: no $file" >&2; exit 1; }
[ -f api/decls.txt ] || { echo "agents-check: no api/decls.txt" >&2; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM

# "LINE<TAB>CODE" for each code span and each line of a fenced block.
awk '
/^```/ { fence = !fence; next }
fence {
    line = $0
    sub(/[ \t]#.*$/, "", line)
    if (line !~ /^[ \t]*#/) print NR "\t" line
    next
}
{
    s = $0
    while (match(s, /`[^`]+`/)) {
        print NR "\t" substr(s, RSTART + 1, RLENGTH - 2)
        s = substr(s, RSTART + RLENGTH)
    }
}' "$file" > "$work/code"

# The words of each, one per line, after the kind of check each needs:
# "KIND<TAB>LINE<TAB>WORD".
awk -F '\t' '
function check(kind, w) { print kind "\t" line "\t" w }
{
    line = $1
    n = split($2, w, /[ \t]+/)
    for (i = 1; i <= n; i++) {
        x = w[i]
        gsub(/[,;:()]+$/, "", x)
        gsub(/^[(]+/, "", x)
        if (x == "") continue
        if (x == "ninja") {
            for (j = i + 1; j <= n && w[j] ~ /^[a-z][a-z0-9-]*$/; j++) check("target", w[j])
            i = j - 1
            continue
        }
        if (x == "./configure") {
            if (i < n && w[i + 1] ~ /^[a-z]+$/) { check("mode", w[i + 1]); i++ }
            continue
        }
        if (x ~ /\*/ || x ~ /[a-z]N$/ || x ~ /_NAME$/) continue
        if (x ~ /^<[a-z]+\/[a-z0-9_]+\.h>$/) { check("header", substr(x, 2, length(x) - 2)); continue }
        if (x ~ /^</) continue
        if (x ~ /^-/) continue
        if (x ~ /\// && x ~ /^[A-Za-z0-9_.<>\/-]+$/ && x !~ /^[0-9.]+-/) { check("path", x); continue }
        if (x ~ /\.(c|h|sh|def|md|txt|tsv|ld|in)$/ && x ~ /^[A-Za-z0-9_.-]+$/) { check("path", x); continue }
        if (x ~ /^(lc|lio)_[a-z0-9_]+$/) { check("name", x); continue }
        if (x ~ /^(LIGHTC|LC)_[A-Z0-9_]+$/) { check("macro", x); continue }
        if (x ~ /^[A-Z]+-[a-z0-9]+(-[a-z0-9]+)*$/) { check("regress", x); continue }
        if (x ~ /^test_[a-z0-9_]+$/) { check("suite", x); continue }
    }
}' "$work/code" > "$work/words"

failed=0
count=0
tab=$(printf '\t')
while IFS="$tab" read -r kind line word; do
    count=$((count + 1))
    ok=1
    case "$kind" in
        path)
            pattern=$(printf '%s' "$word" | sed 's/<[^>]*>/*/g')
            found=0
            for p in $pattern; do [ -e "$p" ] && found=1; done
            [ "$found" -eq 1 ] || ok=0
            ;;
        header) [ -f "include/$word" ] || ok=0 ;;
        name)   grep -q -E "[^A-Za-z0-9_]$word([^A-Za-z0-9_]|$)" api/decls.txt || ok=0 ;;
        macro)  grep -r -q -w -- "$word" include src tests ci configure || ok=0 ;;
        regress) grep -q -E "^$word[ \t]" tests/regress/manifest || ok=0 ;;
        suite)  [ -f "tests/$word.c" ] || ok=0 ;;
        target) grep -q -F "build $word:" configure || ok=0 ;;
        mode)   grep -q -E "^    $word\)" configure || ok=0 ;;
    esac
    if [ "$ok" -eq 0 ]; then
        echo "$file:$line: $kind $word does not exist"
        failed=1
    fi
done < "$work/words"

if [ "$failed" -ne 0 ]; then
    echo "$file names something that does not exist: fix the name, or the file."
    exit 1
fi
echo "$file: all $count names it checks exist."
