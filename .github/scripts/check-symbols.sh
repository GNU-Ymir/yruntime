#!/usr/bin/env bash
#
# Fails if a built archive exports a symbol outside the prefixes the runtime owns. Executed
# *inside* the midgard build image, from /midgard, by the `symbols` job of ci.yml.
#
#   usage: check-symbols.sh [archive]...      (defaults to every libgymidgard_*.a at the root)
#
# libgymidgard_*.a is a static archive linked into every Ymir program, so every external symbol it
# defines lands in that program's global namespace. Anything the runtime defines under a bare name
# (str_create, selfId, installHandler, _exc_init, ...) therefore either collides with the user's own
# C code or, through archive link order, silently answers a call that was never meant for it -
# MID-62, where a rename out of the _yrt_ namespace broke gymir's binding without a single warning.
#
# The prefixes allowed below, and nothing else:
#   _yrt_     the public C ABI: what compiled code, the bindings and etc::runtime::* call
#   _yrt_i_   runtime-internal helpers, shared between rt/*.c but called from nowhere else
#   _Y        mangled Ymir symbols emitted by gyc
#   __        the compiler/ABI reserved namespace (__gyc_personality_v0, __YRT_DEBUG__, ...)
#   DW.ref.   unwind-table references, compiler-generated
#   main      the entry point gyc synthesizes into the -funittest archive

set -uo pipefail

ARCHIVES=("$@")
if [ "${#ARCHIVES[@]}" -eq 0 ]; then
    mapfile -t ARCHIVES < <(ls libgymidgard_*.a 2>/dev/null)
fi

if [ "${#ARCHIVES[@]}" -eq 0 ]; then
    echo "::error::no archive to check - was 'gyllir build' run?" >&2
    exit 2
fi

status=0
for archive in "${ARCHIVES[@]}"; do
    if [ ! -f "$archive" ]; then
        echo "::error::$archive does not exist" >&2
        status=1
        continue
    fi

    # nm's second column is the symbol type; an uppercase letter means external, and 'U' means
    # undefined (a symbol this archive *calls*, whose name is somebody else's problem).
    bad="$(nm -g --defined-only "$archive" \
               | awk '$2 ~ /^[A-TV-Z]$/ { print $3 }' \
               | grep -vE '^(_Y|_yrt_|__|DW\.ref\.|main$)' \
               | sort -u)"

    if [ -n "$bad" ]; then
        count="$(printf '%s\n' "$bad" | wc -l)"
        echo "::group::${archive}: ${count} symbol(s) outside the runtime's prefixes"
        printf '%s\n' "$bad"
        echo "::endgroup::"
        echo "::error::${archive} exports ${count} symbol(s) outside the runtime's prefixes - see .github/scripts/check-symbols.sh"
        status=1
    else
        echo "[ok] ${archive}"
    fi
done

exit "$status"
