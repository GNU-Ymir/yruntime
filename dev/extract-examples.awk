# Extracts the fenced code blocks of midgard's doc comments (`/** ... */`) into standalone
# compilable .yr files, one per block, and prints a manifest line for each on stdout:
#
#     <generated file>\t<source file>\t<line of the opening fence>\t<checked|skipped>
#
# A bare ``` fence (or ```ymir / ```yr) is checked; a fence with any other info string is an
# opt-out marker for blocks that are not meant to compile (pseudo code, error fragments,
# shell transcripts, ...) and is only counted.
#
# Blocks are wrapped so that a bare sequence of statements is a valid program: top level
# declarations found in the block are hoisted out, the rest goes into a `__test` body (which,
# unlike `fn main`, needs no `throws` annotation). `use std::io;` and the documented module
# itself are injected, since doc examples elide the imports the reader is expected to have.
#
# Required variables: OUTDIR (where the .yr files are written), IMPORT (the module path a reader
# would `use` to reach the documented symbols, injected into every block), PREFIX (a filename
# stem identifying the source file).

function reset_block() {
    nlines = 0
    delete lines
}

function flush_line(l) {
    lines[nlines++] = l
}

function is_decl_start(l) {
    return l ~ /^(pub |prv |use |mod |in |fn |class |record |enum |trait |def |static |extern|import |aka |macro |union |lazy |template |__test|@)/
}

function count_braces(l,   i, c, n) {
    n = 0
    for (i = 1; i <= length(l); i++) {
        c = substr(l, i, 1)
        if (c == "{") n++
        else if (c == "}") n--
    }
    return n
}

function emit_block(src, startline,   i, out, l, depth, indecl, opened, ndecl, nbody, dl, bl) {
    out = sprintf("%s/%s_%d.yr", OUTDIR, PREFIX, startline)

    ndecl = 0; nbody = 0; indecl = 0; depth = 0; opened = 0
    for (i = 0; i < nlines; i++) {
        l = lines[i]
        if (!indecl && is_decl_start(l)) {
            indecl = 1; depth = 0; opened = 0
        }
        if (indecl) {
            dl[ndecl++] = l
            depth += count_braces(l)
            if (l ~ /\{/) opened = 1
            if ((opened && depth <= 0) || (!opened && l ~ /;[ \t]*(\/\/.*)?$/)) indecl = 0
        } else {
            bl[nbody++] = l
        }
    }

    printf("// extracted from %s:%d by dev/check-examples.sh - do not edit\n", src, startline) > out
    print "use std::io;" > out
    if (IMPORT != "" && IMPORT != "std::io") printf("use %s;\n", IMPORT) > out
    for (i = 0; i < ndecl; i++) print dl[i] > out
    print "" > out
    print "__test {" > out
    for (i = 0; i < nbody; i++) print "    " bl[i] > out
    print "}" > out
    close(out)

    printf("%s\t%s\t%d\tchecked\n", out, src, startline)
}

FNR == 1 { indoc = 0; incode = 0; reset_block() }

{
    line = $0

    if (!indoc) {
        if (line ~ /\/\*\*/) indoc = 1
        next
    }

    stripped = line
    sub(/^[ \t]*\*[ ]?/, "", stripped)

    if (line ~ /\*\//) {
        indoc = 0; incode = 0; reset_block()
        next
    }

    if (stripped ~ /^```/) {
        if (!incode) {
            info = stripped
            sub(/^`+[ \t]*/, "", info)
            gsub(/[ \t\r]+$/, "", info)
            incode = 1
            fence_line = FNR
            checked = (info == "" || info == "ymir" || info == "yr")
            reset_block()
        } else {
            incode = 0
            if (checked) emit_block(FILENAME, fence_line)
            else printf("-\t%s\t%d\tskipped\n", FILENAME, fence_line)
            reset_block()
        }
        next
    }

    if (incode) flush_line(stripped)
}
