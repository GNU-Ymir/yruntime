#!/usr/bin/env bash
#
# Runs one CI shard's slice of the test suite. Executed *inside* the midgard build image, from
# /midgard, by the `shard` job of ci.yml.
#
#   usage: run-shard.sh <shard-name> <filter>...
#
# Each <filter> is a ./midgard_tests `-f` filter: a "::"-separated path of glob segments matching
# the *test name* with its "__test" segment dropped, and it must have exactly as many segments as
# that name ("config::*" matches config::__test::0, "algorithm::*::*" matches
# algorithm::sorting::__test::0, "algorithm::searching::*::*" matches
# algorithm::searching::find::__test::0). Only one filter can be passed per run, hence the loop.
#
# Every ./midgard_tests run leaves two things behind in the working directory: `.ymir_test_success`,
# the "name;true|false" map `--resume` reads, and one `.ymir_coverage_<pid>.json` per process,
# which the coverage report merges. Both are collected into /tmp/shard-out for the workflow to hand
# to the final `--resume -cov` job, which turns them back into a single whole-suite report.

set -uo pipefail

SHARD="${1:-}"
shift || true

if [ -z "$SHARD" ] || [ "$#" -eq 0 ]; then
    echo "usage: run-shard.sh <shard-name> <filter>..." >&2
    exit 2
fi

# The whole suite runs in a few seconds, so a shard is sequential unless JOBS says otherwise.
JOBS="${JOBS:-1}"

OUT="/tmp/shard-out"
rm -rf "$OUT"
mkdir -p "$OUT"

# A CI image is built from a clean checkout, but an image built from a working tree would carry a
# stale success map - which --resume below would read as "all of this already passed" and skip the
# entire shard.
rm -f .ymir_test_success .ymir_test_success_*.part .ymir_coverage_*.json

failed=""
count=0
for filter in "$@"; do
    before="$(wc -l < .ymir_test_success 2>/dev/null || echo 0)"

    echo "::group::[${SHARD}] ${filter} (-j ${JOBS})"
    code=0
    # --resume on every run, including the first: without it midgard_tests starts by deleting every
    # coverage file it finds in the working directory (resetCoverageUnlessResuming in the test
    # runner). The filters of a shard are disjoint, so nothing is ever skipped as "already passed".
    ./midgard_tests --resume -j "$JOBS" -f "$filter" || code=$?
    echo "::endgroup::"

    # This run's coverage dumps leave the working directory immediately, because at the end of
    # every *successful* run midgard_tests loads and merges every .ymir_coverage_* file it finds
    # there - `-cov` only decides whether the merged report is printed, never whether it is built.
    #
    # Their names cannot collide either: every shard runs as pid 1 in its own container, so the pid
    # they are named after would come back as .ymir_coverage_1.json from all of them. CoverageLoad
    # matches only on the ".ymir_coverage_" prefix and the ".json" suffix, so the shard name goes in
    # the middle.
    for cov in .ymir_coverage_*.json; do
        [ -e "$cov" ] || continue
        count=$((count + 1))
        mv "$cov" "${OUT}/.ymir_coverage_${SHARD}-${count}.json"
    done

    after="$(wc -l < .ymir_test_success 2>/dev/null || echo 0)"

    if [ "$code" -ne 0 ]; then
        failed="${failed} ${filter}"
    elif [ "$after" -le "$before" ]; then
        # A filter that matches nothing prints nothing and exits 0 - it is never a pass, always a
        # stale entry in the shard map below. Whatever it was meant to claim, if it still exists, is
        # run by the final --resume job instead, which reports it as unclaimed.
        echo "::warning::[${SHARD}] filter '${filter}' matched no test"
    fi
done

results=0
if [ -f .ymir_test_success ]; then
    cp .ymir_test_success "${OUT}/success-${SHARD}.txt"
    results="$(wc -l < "${OUT}/success-${SHARD}.txt")"
else
    : > "${OUT}/success-${SHARD}.txt"
fi

echo "[${SHARD}] ${results} test result(s), ${count} coverage file(s) collected"

if [ -n "$failed" ]; then
    echo "::error::[${SHARD}] failing filter(s):${failed}"
    exit 1
fi
