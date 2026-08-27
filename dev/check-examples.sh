#!/bin/sh
# Compiles every code example found in midgard's doc comments.
#
# Each fenced block of an `@example:` (or any other) doc comment is extracted into a standalone
# .yr file by dev/extract-examples.awk and handed to `gyc -fsyntax-only`, so a signature change
# that invalidates an example is caught instead of shipping to the exported documentation.
#
# A block whose fence carries an info string other than `ymir`/`yr` (e.g. ```text) is skipped:
# that is the opt-out marker for blocks that are not meant to compile - pseudo code, fragments
# showing a compile error, shell transcripts, C declarations.
#
# Usage: dev/check-examples.sh [-j N] [-f SUBSTR] [-k] [-v]
#   -j N        compile N examples in parallel (default: number of cpus)
#   -f SUBSTR   only check examples coming from source paths containing SUBSTR
#   -k          keep the generated files and compiler logs on success
#   -v          print the full compiler output of each failure (default: first error only)
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
SRCDIR="$ROOT/midgard"
OUTDIR="$ROOT/.examples"
GYC=${GYC:-gyc}

JOBS=$( (nproc 2>/dev/null) || echo 4)
FILTER=""
KEEP=0
VERBOSE=0

while [ $# -gt 0 ]; do
    case "$1" in
        -j) JOBS=$2; shift 2 ;;
        -f) FILTER=$2; shift 2 ;;
        -k) KEEP=1; shift ;;
        -v) VERBOSE=1; shift ;;
        -h|--help) sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "check-examples: unknown option '$1'" >&2; exit 2 ;;
    esac
done

command -v "$GYC" > /dev/null 2>&1 || {
    echo "check-examples: '$GYC' not found in PATH" >&2
    exit 2
}

# The module path a reader would `use` to get at the symbols documented in $1. A submodule
# declared without `pub` in its parent (`mod ::stream;`) is not importable on its own, its
# content is reachable through the parent - walk up until a publicly declared module is found.
import_path () {
    rel=${1#"$SRCDIR"/}
    rel=${rel%.yr}
    case "$rel" in
        __lib__) echo ""; return ;;
        */__lib__) rel=${rel%/__lib__} ;;
    esac

    while [ -n "$rel" ]; do
        parent=${rel%/*}
        leaf=${rel##*/}
        [ "$parent" = "$rel" ] && break

        pfile="$SRCDIR/$parent.yr"
        [ -f "$pfile" ] || break
        grep -qE "^[[:space:]]*pub[[:space:]]+mod[[:space:]]+(::)?[[:space:]]*$leaf[[:space:]]*;" "$pfile" && break

        rel=$parent
    done

    echo "$rel" | sed 's|/|::|g'
}

rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"
MANIFEST="$OUTDIR/manifest.tsv"
: > "$MANIFEST"

for file in $(find "$SRCDIR" -name '*.yr' | sort); do
    case "$file" in
        *"$FILTER"*) ;;
        *) continue ;;
    esac
    awk -v OUTDIR="$OUTDIR" \
        -v IMPORT="$(import_path "$file")" \
        -v PREFIX="$(echo "${file#"$SRCDIR"/}" | sed 's|[/.]|_|g')" \
        -f "$SCRIPT_DIR/extract-examples.awk" "$file" >> "$MANIFEST"
done

CHECKED=$(grep -c '	checked$' "$MANIFEST" || true)
SKIPPED=$(grep -c '	skipped$' "$MANIFEST" || true)

echo "check-examples: $CHECKED example(s) to compile, $SKIPPED skipped, -j $JOBS"

# gyc treats an unused symbol as a fatal diagnostic (it reports it as a warning, then stops), so
# an example that leaves a value unused does not compile for a reader either - no leniency here,
# a failing exit status is a failing example.
COMPILE="$GYC -fsyntax-only -funittest -nostdinc -nomidgardlib -I'$SRCDIR' \"\$1\" > \"\$1.log\" 2>&1 || echo \"\$1\" >> '$OUTDIR/failed.txt'"

grep '	checked$' "$MANIFEST" | cut -f1 | \
    xargs -r -P "$JOBS" -I@ sh -c "$COMPILE" sh @ \
    || true

FAILED=0
if [ -f "$OUTDIR/failed.txt" ]; then
    sort -o "$OUTDIR/failed.txt" "$OUTDIR/failed.txt"
    FAILED=$(wc -l < "$OUTDIR/failed.txt" | tr -d ' ')

    while read -r gen; do
        loc=$(awk -F'\t' -v g="$gen" '$1 == g { printf "%s:%s", $2, $3 }' "$MANIFEST")
        echo ""
        echo "=== ${loc#"$ROOT"/} ($gen)"
        if [ "$VERBOSE" -eq 1 ]; then
            sed 's/\x1b\[[0-9;]*m//g' "$gen.log"
        else
            sed 's/\x1b\[[0-9;]*m//g' "$gen.log" | grep -E 'Error : |Warning : ' | head -3
        fi
    done < "$OUTDIR/failed.txt"
fi

echo ""
if [ "$FAILED" -eq 0 ]; then
    echo "check-examples: all $CHECKED example(s) compile"
    [ "$KEEP" -eq 1 ] || rm -rf "$OUTDIR"
    exit 0
fi

echo "check-examples: $FAILED of $CHECKED example(s) failed to compile"
echo "check-examples: generated sources kept in ${OUTDIR#"$ROOT"/}"
exit 1
