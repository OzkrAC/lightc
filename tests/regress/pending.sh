#!/bin/sh
# lightc — pending regress program runner
#
# Runs each PROGRAM, the path of a pending regress program such as
# build/regress/TIME-sleep-eintr, through tests/run.sh, then lists the
# programs by id, each with PASS or FAIL. ninja regress-pending runs this
# with the programs of the ids tests/regress/manifest marks pending.
#
# A pending program checks a defect that is not fixed yet, so it is meant to
# fail, and a FAIL does not fail this. One that passes says its defect looks
# fixed: the PR with the fix marks its id gating in the manifest. A PROGRAM
# that does not exist, as when it did not build, is listed as FAIL.
#
# Usage: tests/regress/pending.sh [PROGRAM...]
# Exit status: 0 whether the programs passed or failed. When tests/run.sh
# exits with another status than 0 or 1, as for a usage error (2) or when a
# signal stops it (128 plus the signal number), this stops at once and exits
# with that status.

set -u

if [ $# -gt 0 ]; then
    echo "Pending regress programs check defects not fixed yet, so tests/run.sh"
    echo "is expected to say FAIL for them below. That does not fail this run."
fi

results=
for program in "$@"; do
    sh tests/run.sh "$program"
    status=$?
    case $status in
        0) result=PASS ;;
        1) result=FAIL ;;
        *) exit "$status" ;;
    esac
    results="$results  ${program##*/}: $result
"
done

echo
echo "========================================"
if [ -n "$results" ]; then
    echo "Pending regress programs:"
    printf '%s' "$results"
else
    echo "No pending regress programs"
fi
echo "========================================"
exit 0
