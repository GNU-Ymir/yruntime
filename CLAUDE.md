# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this
repository.

## Issue tracking (Plane)

Project **Midgard** (key `MID`), workspace `ymir-bootstrap`. Use the plane MCP tools.

Work items used to live in Linear under a single `YMI` key shared by every repository, which
is why older PR titles and branch names here all read `YMI-`. Each repository now has its own
Plane project, and new work for this one goes under `MID`.

## Pull request titles

PR titles must read `[MID-XXX][kind] Log` — `MID-XXX` is the Plane work item, and `[kind]` is
optional and defaults to a feature. Known kinds: `feat`/`feature`, `fix`, `perf`, `refactor`,
`doc(s)`, `test(s)`, `chore`/`ci`/`build`/`style`, `breaking`. This is not cosmetic: the release
notes are generated from these titles by `.github/scripts/changelog.sh`, one entry per merged PR
(the commits inside a PR are never listed), grouped by kind. **A PR whose title does not follow
the format — no issue key, or a kind outside that list — is left out of the release notes
entirely**; the skip is logged on stderr by the release job, but the change goes unannounced.

The leading tag is also the only thing tying a PR to Plane. `.github/workflows/plane-sync.yml`
takes the work item from it and nowhere else — a body merely mentioning another item cannot
attach the PR to it — then attaches the PR as a link, comments on open, close and merge, and
moves the item to In Progress when the PR opens and to Done when it merges. A title without a
valid tag is skipped in silence, so it costs both the release note and the tracker update.

The branch currently checked out is generally named `MID-<issue_number>-<short-description>`
(e.g. `MID-60-plane-ci`) — the `MID-<issue_number>` part is the Plane work item key, so it
can be used to look up the item this branch's work is tracked against.

## Policy

Consice comment:
- only describe what the functions do, not what the current work is adding
- don't write comment inside a code unless it's absolutely necessary for understanding
- a comment of more than 3 lines is generally too verbose

Commit policy:
- split work in logical commits
- rewrite history when a new commit it modifying something that was introduced by another commit of the same branch
- There's no need for tests to pass, and code to compile between commits as long as the last commit of the branch compiles and test succeed
- don't add co-authors
- commit message are just one line long


## What this is

Midgard is the standard library for GNU-Ymir (`gyc`), written in Ymir (`.yr`) with a small C
runtime underneath. It provides the `std`, `core`, and `etc` module trees that every Ymir
program compiled with `gyc` links against. This repo is a sibling of the `gyc`/`gcc` compiler
checkout, not a standalone app — changes here affect what `use std::...` / `use core::...`
resolves to for all Ymir code on the system once installed.

## Ymir's memory model: `dmut` / `alias` / `copy`

Ymir is garbage-collected, so `dmut`/`alias` are **not** an ownership or lifetime system like
Rust's borrow checker — there's no dangling-pointer problem for them to solve, the GC already
handles that. What they actually are is an access-privilege annotation enforced at the point a
mutable handle is *created*: writing `alias a` at a call site or `let` is how you tell (and
prove to the compiler) that the callee/new binding is being granted write access to `a`'s data.
The payoff is readability — anyone reading a function signature or a call site can tell from
`dmut`/`alias` alone which arguments the callee might mutate as a side effect, without reading
the callee's body. If you don't want the callee to be able to mutate your data (or don't want to
grant that privilege at all), pass a `copy` instead. The rules below were verified empirically
against the installed `gyc`, not just inferred from docs.

- **`dmut T`** marks a binding/parameter as giving *deeply mutable* access to `T` (e.g.
  `let dmut a = copy [1, 2, 3];`). A plain `let a = ...` (no `dmut`) is a read-only view: you
  can still read through it even if the underlying value is mutated elsewhere (see `alias`
  below) — it is not a snapshot/copy.
- **Assigning or passing a `dmut` value to another `dmut` binding/parameter without saying so
  explicitly is a compile error**, because it would silently hand out a second mutable handle to
  the same memory:
  ```
  let dmut a = copy [1, 2, 3];
  let dmut b = a;   // Error: "discard the constant property is prohibited" /
                     // "implicit alias of type ... is prohibited, it implicitly
                     //  gives up on borrowings of mutable values"
  ```
  The fix is the explicit `alias` keyword at the use site:
  ```
  let dmut b = alias a;   // OK — b and a now share the same underlying array
  b[0] = 42;               // a[0] is also 42 after this
  ```
  The same rule applies at call sites: a function parameter declared `dmut` requires the
  argument to be written `foo(alias a)`, not `foo(a)` (`foo(a)` fails the same way as the
  `let` case above). This applies uniformly to arrays/slices **and classes** — the error for a
  class is the same "implicit alias ... is prohibited" message, just with a class type instead
  of an array type.
- **You cannot manufacture mutability out of nothing.** Both `let dmut b = a;` and
  `let dmut b = alias a;` are errors if `a` itself is not `dmut` (errors: "discard the constant
  property is prohibited ... from `[i32]` to `mut [mut i32]`", or "value of type `[i32]` is not
  a lvalue" for the `alias` form). Narrowing the other direction — binding a `dmut` value to a
  non-`dmut` name, or passing it to a non-`dmut` parameter — is always fine and needs no `alias`:
  ```
  let dmut a = copy [1, 2, 3];
  let b = a;         // OK, no alias needed; b is a read-only view of the same array
  a[0] = 9;
  println(b);        // prints [9, 2, 3] — b is not a copy, mutation is visible through it
  ```
- **`copy` allocates a genuinely independent value**, breaking aliasing: `let dmut b = copy a;`
  gives `b` its own storage, so mutating `b` afterwards does not affect `a` (and vice versa).
  For arrays/slices this works natively. **Copying an existing class instance is spelled `dcopy`,
  not `copy`, and requires the class to implement `core::types::Copiable`
  (`fn deepCopy(self)-> dmut(typeof self);`)** — `dcopy` calls that method, and `copy
  someClassInstance` fails with "no copy exists for type ..." whether or not the trait is
  implemented. `copy` is still how objects are constructed in the first place, e.g.
  `copy TaskPool(nbThreads-> 12u64)`, `copy [1, 2, 3]`.
- **`alias`/`dmut` do not make any exclusivity guarantee** — because that isn't their job. A
  plain (non-`dmut`) parameter or binding is not a frozen snapshot; it's still a live view of
  the same GC'd memory, so it can change if *anything else* holding a `dmut` handle to that
  memory mutates it. E.g. `bar(a, alias a)` compiles and runs fine, and inside `bar` the plain
  parameter observes the mutation made through the `dmut` one, because they're the same
  underlying array. The rule to internalize: `dmut`/`alias` tell you who is *granted the
  privilege* to mutate, not whether the data is otherwise stable — if you need a value that
  genuinely won't change under you, that's what `copy` is for.
### `ref`: rebinding the caller's storage slot, vs. `dmut`/`alias` mutating its pointee

`dmut`/`alias` only grant write access *through the pointer a compound value already carries*
(an array/slice's backing buffer, a class's fields) — they never let a callee replace what the
caller's own variable is bound to. `ref` is the separate, orthogonal mechanism for that: it
passes the address of the caller's storage slot itself. Confirmed by compiling with
`-fdump-ymir` and reading the lowered `.ydump-yil` output (dumps `.ydump-decls.{1,2}`,
`.ydump-sem`, `.ydump-yil` next to the source file):

- `fn foo(ref mut x: i32) {...}` lowers `x`'s parameter type to a raw pointer (`P_8 *(i32)`),
  `x = 42` inside `foo` lowers to `*x(#1) = 42`, and a call `foo(ref a)` lowers to
  `foo(&a(#1))` — i.e. `ref` is literally pass-by-address, and scalars (`i32`, ...) can *only*
  get write-back this way, since (unlike an array/class) a scalar has no inner pointer for
  `dmut`/`alias` to grant access to. Confirmed directly: `fn foo(dmut x: i32)` is rejected
  outright with "a parameter cannot be mutable, if it is not a reference or does not borrow
  mutable data" — that's the compiler's own description of the two mechanisms.
- Just like `alias`, a **mutable `ref` requires the explicit `ref` keyword at the call site**:
  `foo(a)` against a `ref mut x: i32` parameter fails ("cannot pass value of type mut i32 as
  ref"); `foo(ref a)` is required. And just like `alias`, **the argument must already be a
  mutable (`dmut`) lvalue** — `foo(ref a)` where `a` is plain `let a = 1;` fails with "left
  operand of type i32 is immutable".
- A **read-only `ref x: i32` (no `mut`) needs no `ref` at the call site** — `foo(a)` compiles,
  and the YIL dump shows the compiler silently inserts the address-of (`&a(#1)`) for you. Same
  pattern as `dmut`→plain narrowing being implicit: no `ref`/`alias` keyword is required unless
  the operation would hand out new write privilege.
- **`ref` and `dmut`/`alias` compose on compound types, and this is where the difference between
  "mutate the pointee" and "rebind the slot" becomes concrete:**
  ```
  fn mutateContents (dmut x : [i32]) { x[0] = 9; }         // x is a local copy of the slice
                                                              // header (len, ptr); x[0]=9 writes
                                                              // through the shared ptr
  fn reseat         (ref dmut x : [i32]) { x = copy [9, 9, 9]; }  // x is a pointer to the
                                                                    // caller's own slice-header
                                                                    // slot; assigning x replaces
                                                                    // what the caller's variable
                                                                    // itself points to
  let dmut a = copy [1, 2, 3];
  mutateContents (alias a);       // a is now [9, 2, 3]  — same backing array, contents changed
  reseat (ref a);                 // a is now [9, 9, 9]  — a's slice header was replaced outright
  ```
  Without `ref`, reassigning the parameter inside the callee (`x = copy [...]`) only rebinds the
  callee's local copy of the slice header and is invisible to the caller — `alias` alone can
  never do what `ref` does here.

- Heap-allocated values (`copy [...]`, `copy SomeClass(...)`) are GC-managed, so returning an
  `alias` of a local out of a function is safe and does not dangle — the GC keeps the backing
  storage alive as long as any handle (aliased or not) references it.

## Build / run / test

Build system is `gyllir` (`gyllir.toml` at the repo root), driven by targets that each invoke the
Ymir compiler, looked up as `gyc` on `PATH` (`compiler = "gyc"` in `gyllir.toml`). CMake
(`CMakeLists.txt`) has been removed — do not reintroduce a `.build/`/`cmake ..`/`make` flow.

The tree targets the gyc named by `YMIR_BOOTSTRAP_VERSION` in `YMIR_VERSION`, which may not be
released yet; an older gyc on `PATH` is not expected to compile it (1.5.3, for instance, lacks the
`__key` macro rule `std::config::json` uses and still emits the slice ABI of parameterized tests).
Until that release is installed, build with the in-development compiler: edit `compiler` in
`gyllir.toml` to an absolute path (`compiler = "/home/emile/ymir/gcc/gcc-install/bin/gyc"`), and
**`gyllir clean` first**: `.target/` is keyed by target and build kind only, never by compiler,
so switching without a clean relinks objects the other compiler produced. gyllir has no env-var or
`--compiler` override — the toml key is the only knob. Keep gyc's unused-import check,
`Warning[E3038] : no symbol is resolved through the use of ...`, at zero.
Note that it only credits a `use` when a symbol is reached through the *shortened* path, so
`use std::env;` pairs with `setVar(...)`, not with `std::env::setVar(...)`.

Two separate versions live at the repo root, and mixing them up is the classic bug here:

- `VERSION` — midgard's own release (also the git tag each release is cut under), and the version
  `install` derives `MIDGARD_SHORT_VERSION` (major.minor, e.g. `1.2`) from for the install include
  dir below. It has to be this one: `gyc` resolves `include/ymir/<ver>` from the midgard release
  *it* was built against. Note `gyllir.toml` also carries its own `version` field, tracking the
  in-progress release (e.g. `1.3.1` while `VERSION` still reads `1.3.0`) — the two are expected to
  diverge between a version bump commit and the matching release, not a bug to "fix" on sight.
- `YMIR_VERSION` (`YMIR_BOOTSTRAP_VERSION`/`GYLLIR_VERSION`/`GCC_VERSION`) — the gyc/gyllir release
  this library is *built with*, used to fetch those compilers' `.deb`s in CI/the `Dockerfile`. It
  runs ahead of `VERSION` and must never leak into an artifact name.

- Build: `gyllir build`. Builds every target in `gyllir.toml` in dependency order: `gymidgard_debug`
  and `gymidgard_release` (compile `midgard/__lib__.yr`), `gymidgard_debug_unit` (same, with
  `-funittest`), `gymidgard_tests` (compiles `test-rt/__lib__.yr`) — each bundling the C sources
  under `rt/*.c` (`c-sources` in its `gyllir.toml` target) alongside its own `.yr` object code —
  then `midgard_tests`, the compiled test binary, linked against `gymidgard_debug_unit` and
  `gymidgard_tests`. Intermediate objects land under `.target/`; the final artifacts
  (`libgymidgard_debug.a`, `libgymidgard_release.a`, `libgymidgard_tests.a`, `libgymidgard_debug_unit.a`,
  `midgard_tests`) are copied to the repo root — unversioned names, unlike the old CMake
  `gymidgard-*_<midgardShortVersion>` scheme.
- Run tests: `./midgard_tests` (add `-f <pattern>` to filter, `-sf` to stop on first failure,
  `--resume` to re-run only previously-failed tests, `-cov` for a coverage report, `-ct` for a
  call-tree report, `-m` to list each file's uncovered lines under the coverage report, `-d` to
  list the slowest tests once the run is over, `-l` to list the tests `-f`/`--resume`
  select without running them, `-j N` to run on N worker threads — see
  `test-rt/utils/args.yr`). `-f` is **not** a substring match: the pattern is a `::`-separated
  path of glob segments (`*` the only wildcard, matched within one segment), and it must have
  exactly as many segments as the test name. So `-f rand` selects nothing, `-f "rand::*"` runs
  that module, and a test one level deeper needs the matching depth — `-f "algorithm::*"` selects
  nothing while `-f "algorithm::*::*"` or `-f "algorithm::sorting::*"` works. `-l` lists what a
  pattern selects without running it (`test-rt/utils/filters.yr`).
- `sudo ./install` copies `midgard/**/*.yr` into `/usr/include/ymir/<midgardShortVersion>` and the
  `gyc` internal include dir (version component from `VERSION`, GCC major from `YMIR_VERSION`).
  There is currently no equivalent step for installing the built static libs system-wide (the old
  `sudo make install` rule went away with `CMakeLists.txt`) — copy `libgymidgard_*.a` to
  `/usr/lib/` by hand if you need that, until this is scripted.
- `./.ymir_test_success` (repo root, not `.build/`) caches per-test pass/fail state, consumed by
  `--resume`.
- `dev/check-examples.sh` compiles the code examples of the doc comments (`-j N` for parallelism,
  `-f SUBSTR` to restrict to source paths matching `SUBSTR`, `-k` to keep the generated files,
  `-v` for the full compiler output of a failure). It runs as its own CI job — see "Doc example
  checking" below.

## Architecture

### `midgard/` — the library itself

Three top-level modules declared in `midgard/__lib__.yr`:

- `std` (`midgard/std.yr`): general-purpose stdlib — `algorithm`, `any`, `box`, `char`,
  `concurrency`, `config`, `conv`, `env`, `format`, `fs`, `io`, `math`, `net`, `rand`, `stream`,
  `syntax`, `time`, `traits`, `unit`.
- `core` (`midgard/core.yr`): language-support types — `atom`, `exception`, `concurrency`,
  `math`, `types`, `reflect`. `core::exception::Exception` is the root of the exception
  hierarchy; in `DEBUG_LIB` builds it captures a stack trace at construction.
- `etc` (`midgard/etc.yr`): externals — `etc::c` (raw C bindings) and `etc::runtime::*`
  (`dwarf`, `elf`, `env`, `errno`, `exception`, `files`, `gc`, `memory`, `threads` — thin
  Ymir wrappers around `rt/`).

### `rt/` — the C runtime

Backs the language runtime independent of Ymir compilation: `rt/memory` (GC, allocation,
class/type info, printing), `rt/except` (exception raising, stack unwinding, DWARF-based
stacktraces), `rt/concurrency` (threads, futures), `rt/utils` (demangling, env, errno, files,
math, string helpers), `rt/run.c` (process entry glue). Built once as `libruntime.a` and also
folded into both `gymidgard-*` static libs.

**Every external symbol `rt/` defines must carry a prefix the runtime owns** — `_yrt_` for the
public C ABI (what compiled code, `etc::runtime::*` and gymir's bindings call) and `_yrt_i_` for
helpers shared between `rt/*.c` but called from nowhere else. Nothing else is available: these
objects are archived into `libgymidgard_*.a`, which is linked into every Ymir program, so a helper
named `str_create` or `installHandler` either collides with the user's own C or, through archive
link order, silently answers a call meant for theirs. `static` is not an escape hatch for most of
them — they are used across translation units — and hidden visibility does nothing in a static
archive. `.github/scripts/check-symbols.sh` (the `symbols` CI job) runs `nm -g --defined-only` over
every archive and fails on anything outside `_yrt_*`, `_Y*`, `__*`, `DW.ref.*` and `main`; run it
after touching `rt/`. MID-62 is what it exists to prevent: YMI-141 dropped the `_yrt_` prefix from
76 helpers it took to be internal, one of which (`_yrt_exc_init`) gymir's
`binding/binding/parser.yr` actually calls, and the breakage shipped in a patch release as a bare
link failure. **A rename inside `_yrt_*` is an ABI change** — it belongs in a minor release with a
release note, and anything gymir calls has to be changed there in the same breath.

### `tests/` — the test suite

Mirrors the `std`/`core` layout (`tests/algorithm/`, `tests/fs/`, `tests/concurrency/`, ...).
Each file uses `__test { ... }` blocks with `assert(...)`; there's no separate golden-file
comparison like the compiler frontend's test suite — assertions are the pass/fail signal.
`tests/__lib__.yr` is the aggregate root compiled into `midgard_tests`.

### `test-rt/` — the test runner and coverage system

`test-rt/__lib__.yr` wires the `_yrt_register_unittest_impl` /
`_yrt_register_parameterized_unittest_impl` / `_yrt_run_unittests_impl` extern hooks (called by
compiler-generated `__test` glue) into `utils::runner::UnittestLauncher`, and wires
`_yrt_unittest_coverage_hit_{branch,enter,exit}` into the calling thread's
`utils::coverage::tree` CoverageTree (`tree::threadTree`). The tree is per-thread because the
enter/exit stack it maintains is one thread's call stack; every tree handed out is registered in a
process-global list, and `tree::mergedTree` folds them together at store time, so what ran on a
`TaskPool` worker or an actor loop is not lost with the thread that ran it.

- `utils::args` — CLI parsing (`TestRunnerArgument`, built on `std::config::ArgumentParser`).
- `utils::filters` — include/exclude test-name filtering used by the runner, plus reading and
  writing the `.ymir_test_success` file behind `--resume`.
- `utils::colors` — terminal color helpers for pass/fail/coverage output.
- `utils::runner` — `UnittestLauncher`: registers tests (a parameterized `__test` as its two
  frames: a provider returning a generator of pointers to boxed parameter sets, and the test
  taking one such pointer). A parameterized test is never drained up front: `forEachCase`
  resumes its generator (`_yrt_next_parameter_set` in `test-rt/run.c`) while the run goes on,
  names the k-th yielded set `module::test[k]`, and applies `-f`/`--resume` to each case as it
  comes (`TestFilters::select`) before running it or handing it to the pool. So `-l` lists such a
  test as `module::test[*]`, and `TestFilters::mayPassCaseFilter` only decides whether its
  generator is worth starting. `[k]` filters and `--resume` rely on the generator yielding the
  same sets in the same order on every run. This ABI pairs with gyc's YMI-110: a midgard and a
  gyc from different sides of it do not work together. It runs them (respecting filters,
  stop-first, resume-from-`.ymir_test_success`, and `-j`/`--jobs` parallelism across a
  `TaskPool` of worker threads), and drives coverage/call-tree reporting.
- `utils::coverage::tree` — `CoverageTree`/`CoverageInfo`: in-memory record of branch/enter/exit
  hits per function, keyed by ELF/DWARF frame info from `etc::runtime::elf`/`dwarf`.
- `utils::coverage::list` — per-function hit-list bookkeeping backing the tree.
- `utils::coverage::store` — JSON-encodes a `CoverageTree` into a `&Config`
  (`CoverageStore::toConfig`), and writes it to this process's own coverage file
  `.ymir_coverage_<pid>.json` (`CoverageStore::storePid`/`PID_FILE_PREFIX`/`PID_FILE_SUFFIX`),
  so multiple runs never clobber each other's coverage. `CoverageStore::removePidFiles` deletes
  a given list of these files.
- `utils::coverage::load` — `CoverageLoad::listPidFiles` finds every `.ymir_coverage_*.json` in
  the cwd, `CoverageLoad::load`/`loadAll` parse one/many of them back into `&Config`.
- `utils::coverage::conv` — `CoverageConv::fromConfig(s)`/`merge`: converts the `&Config`(s)
  produced by `store`/`load` into plain `CoverageReport`/`CoverageFunctionEntry`/`CoverageCaller`/
  `CoverageCallee` records (merging several reports' hit counts/call counts together when more
  than one coverage file is loaded), so downstream code never has to touch `&Config` directly.
- `utils::coverage::format` — renders a `CoverageReport` as a human-readable report, grouped by
  file (most recent work on this repo has been here — see `git log --oneline` for the
  progression from "load/store as JSON" through "format module" to "group by files" to "convert
  Config to a plain record before formatting" to "persist/merge coverage across pid files").

### Doc example checking (`dev/check-examples.sh`), if you're extending it

Almost every public symbol carries an `@example:` doc comment, and those blocks are what the
exported HTML documentation shows, so they have to compile. `dev/extract-examples.awk` pulls every
fenced block out of every `/** ... */` comment in `midgard/**/*.yr` and writes each one as a
standalone `.yr` file under `.examples/` (gitignored), which `dev/check-examples.sh` then hands to
`gyc -fsyntax-only -funittest -nostdinc -nomidgardlib -I midgard` — the working tree, never the
installed midgard. Compiling is enough; the examples are not run, several of them open sockets,
spawn processes or write files.

Because an example is a fragment rather than a whole program, the extractor wraps it: top level
declarations found in the block (`use`, `fn`, `class`, `record`, ...) are hoisted out and the rest
goes into a `__test` body, which - unlike `fn main` - needs no `throws` annotation. The module the
documented symbol lives in is injected on top (walking up to the closest publicly declared parent,
since a submodule declared `mod ::stream;` is not importable on its own), plus `use std::io;` when
the block calls print/println - neither when the block imports it already. Because gyc credits a
`use` only for a symbol reached through the shortened path, an example made only of operator or
UFCS calls gets the injected module reported unused (E3038); `check-examples.sh` then drops that
line - it is marked `// injected` - and compiles the example again before calling it broken.
Everything else an example needs it must import itself, the way a reader copying it would have to.

A bare ```` ``` ```` fence (or ```` ```ymir ````/```` ```yr ````) is compiled; any other info
string is the opt-out marker for a block that is not meant to compile - a grammar in
`std/format.yr`, the internals of the private `std::syntax::tokenizer::node` module. Use
```` ```text ```` for those, and keep them rare: a skipped block is a block free to rot again.

Note that `gyc` stops compiling on an unused symbol (it prints it as a `Warning`, then terminates),
so an example that declares a value it never uses does not compile for a reader either - `println`
it rather than dropping the diagnostic.

### Coverage/call-tree feature, if you're extending it

The pipeline is: `gyc`-instrumented test binary hits the three `_yrt_unittest_coverage_hit_*`
externs during execution → the calling thread's `CoverageTree` accumulates hits. At the start
of `UnittestLauncher::run`, unless `--resume` was passed, every existing `.ymir_coverage_*.json`
is deleted (`resetCoverageUnlessResuming`) so a plain run always starts from a clean slate; with
`--resume`, old files are left in place so this run's coverage adds to them instead of replacing
them. After running the tests, every thread's tree is folded into one (`tree::mergedTree`) and
written to this process's own `.ymir_coverage_<pid>.json` (`storeCoverage` →
`CoverageStore::storePid`).

The denominator is not a source scan: `CoverageConv::rootReport` walks the linked binary's ELF
symbol table for `__COI__` records — one per function the compiler instrumented — and seeds each
at zero hits, which is what makes a never-called function show up at 0%. That makes the link order
load-bearing. The compiler emits a function's `__COI__` record *and* its hit calls in the module
that declares it, but a library template re-instantiated while compiling `tests/` is emitted a
second time, hookless, in `midgard_tests.a`; both copies are weak, the linker keeps the first group
it sees, and the hookless one shadowing the instrumented one reports 0% forever however many tests
exercise it. gyllir therefore passes the dependency archives `--whole-archive` ahead of the target's
own object when `-funittest` is in play (`Gyllir/src/gyllir/repo/builder.yr`, `createLinkOptions` →
`isInstrumented`). To check a build, compare the `__COI__` symbols against the bodies that actually
call `_yrt_unittest_coverage_hit_enter` — anything in the first set and not the second is
uncoverable, and the count should be zero. If none of the tests
failed *and* `--coverage`/`--call-tree` was passed, every `.ymir_coverage_*.json` still on disk
(this run's, plus any kept from `--resume`) is loaded and merged into one `CoverageReport`
(`reportCoverage` → `CoverageLoad::listPidFiles`/`loadAll` → `CoverageConv::fromConfigs`/`merge`),
which `utils::coverage::format` prints. That flag guard is not cosmetic: the merge goes through
the slow JSON parser and costs ~35% of a plain run's wall time, and `CoverageFormat::report`
prints nothing without one of the two flags anyway. When touching this path, check
that `store`/`load`/`conv` stay in sync with `CoverageTree`'s field set and the JSON shape they
produce, that `conv`'s merge logic still dedupes by (file, func) rather than double-counting
hits/call-counts across files, and that generated/compiler-synthesized functions (e.g.
`nextCtors`, lambdas) are still filtered out where expected (see commit `54babf5`).

### Parallel test execution (`-j`/`--jobs`), if you're extending it

`UnittestLauncher::run` defaults to `-j 1` (sequential, on the main thread, via `runList`). With
`-j N` (`N > 1`) and more than one test to run, `runParallel` runs them on a
`std::concurrency::task::TaskPool` of N threads, in the one test process:

- The master walks the tests (`forEachCase`, resuming parameterized generators as it goes) and
  submits one task per case, closing over its name and delegate. A worker never touches the registry: `runOne(name, dg)` runs the test on the
  calling thread and returns `(ok, micros)`, and the task sends `(name, ok, micros)` to a
  `std::concurrency::mail::MailBox`.
- Once `pool:.join()` returns (every task has finished), the master drains that mailbox into
  `already`/`durations`, and everything downstream (success file, `-d`, coverage) is unchanged.
  Coverage needs no special handling: each thread fills its own `CoverageTree`, and
  `tree::mergedTree` folds them all together when the run stores its coverage.
- `--stop-first` is a shared flag: a failing task sets it, tasks not started yet skip their test,
  the ones already running finish normally.

Things that will bite you here:

- **Tests run concurrently in one address space**, so anything process-global they touch (env,
  cwd, files at fixed paths, stdout) has to be safe to share — MID-79 is the pass that made the
  runtime and the suite so. A test that creates threads must release them (`TaskPool::dispose`,
  `ActorSystem::join`, `.value` on a `spawn`): a leaked pool stays parked in the process for
  the rest of the run.
- **A test that crashes the process takes the whole run down.** There is no worker process to
  lose and attribute the crash to; the segfault handler prints the trace of the faulty thread.
- **Stack trace resolution is serialized** (`__YRT_STACK_TRACE_MUTEX__` in
  `rt/except/stacktrace.c`, around the shared `__ELF_LOADER__`/`__DWARF_LOADER__`). The DWARF
  loader keeps its parsed compile units for the whole process — loading one sorts its line table
  under coverage hooks, and costs seconds. The tests formatting exceptions (`errors::*`,
  `config::args::errorsToStream`, ...) queue on that lock and dominate a `-j 8` run's tail.

Known, accepted simplification: every worker prints to the same stdout, so the `[RUN]`/`[SUCCESS]`
lines of concurrent tests interleave (each `println` stays whole).
