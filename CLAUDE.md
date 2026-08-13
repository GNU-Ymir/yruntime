# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this
repository.

## Issue tracking (Linear)

Issues for this repo are tracked in Linear, team **Ymir** (key `YMI`, id
`43b9b28b-ad88-40b2-a3b7-e7ba31ad62fc`), workspace `ymir-bootstrap`. There is no Linear MCP tool
installed in this environment — read/write issues via the Linear GraphQL API directly
(`https://api.linear.app/graphql`), authenticated with the `$LINEAR_API_KEY` environment
variable (passed as-is in the `Authorization` header, no `Bearer` prefix). That variable lives in
the user's interactive shell — a fresh subprocess may not inherit it; check with
`[ -z "$LINEAR_API_KEY" ]` before use, and never print the key itself.

- Create an issue:
  ```bash
  curl -s -X POST https://api.linear.app/graphql \
    -H "Content-Type: application/json" \
    -H "Authorization: $LINEAR_API_KEY" \
    -d '{"query": "mutation IssueCreate($input: IssueCreateInput!) { issueCreate(input: $input) { success issue { id identifier url } } }", "variables": {"input": {"teamId": "43b9b28b-ad88-40b2-a3b7-e7ba31ad62fc", "title": "...", "description": "..."}}}'
  ```
- Read/search issues (e.g. by team):
  ```bash
  curl -s -X POST https://api.linear.app/graphql \
    -H "Content-Type: application/json" \
    -H "Authorization: $LINEAR_API_KEY" \
    -d '{"query": "query { team(id: \"43b9b28b-ad88-40b2-a3b7-e7ba31ad62fc\") { issues { nodes { id identifier title state { name } url } } } }"}'
  ```
- Look up a single issue by identifier (e.g. `YMI-35`) via the `issue(id: "...")` query, or
  `issueUpdate(id: ..., input: {...})` to change state/assignee/etc.
- Team discovery (only needed if working across other Linear workspaces/teams):
  `query { teams { nodes { id key name } } }`.

## Pull request titles

PR titles must read `[YMI-XXX][kind] Log` — `YMI-XXX` is the Linear issue, and `[kind]` is
optional and defaults to a feature. Known kinds: `feat`/`feature`, `fix`, `perf`, `refactor`,
`doc(s)`, `test(s)`, `chore`/`ci`/`build`/`style`, `breaking`. This is not cosmetic: the release
notes are generated from these titles by `.github/scripts/changelog.sh`, one entry per merged PR
(the commits inside a PR are never listed), grouped by kind. **A PR whose title does not follow
the format — no issue key, or a kind outside that list — is left out of the release notes
entirely**; the skip is logged on stderr by the release job, but the change goes unannounced.

The branch currently checked out is generally named `YMI-<issue_number>-<short-description>`
(e.g. `YMI-34-coverage-by-files`) — the `YMI-<issue_number>` part is the Linear issue key, so it
can be used to look up the issue this branch's work is tracked against (`issue(id: "YMI-34")` or
by matching `identifier` in a team issue list).

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
  For arrays/slices this works natively. **For classes, `copy` only works if the class
  implements `core::types::Copiable` (`fn deepCopy(self)-> dmut(typeof self);`)** — attempting
  `copy someClassInstance` on a class that doesn't implement it fails with "no copy exists for
  type ...". `copy` is also how objects are constructed in the first place, e.g.
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

Build system is plain CMake (no `gyllir` here, unlike the compiler frontend repo). The Ymir
compiler is looked up as `gyc` on `PATH` (`CMAKE_YMIR_COMPILER` in `CMakeLists.txt`).

`YMIR_VERSION` at the repo root (`YMIR_BOOTSTRAP_VERSION`/`GCC_VERSION`) is the single source
of truth for the ymir bootstrap version this library is built against; CMake reads it to derive
`YMIR_SHORT_VERSION` (major.minor, e.g. `1.1`), which is what's baked into the static lib names
and install include dir below — it is not hardcoded in `CMakeLists.txt`/`install` anymore.
`VERSION` at the repo root holds midgard's own (separate) version number, used by the release
workflows.

- Build: `mkdir -p .build && cd .build && cmake .. && make`.
- This builds four CMake targets in dependency order: `lib_release`/`lib_debug` (compile
  `midgard/__lib__.yr` via `gyc -c`), `lib_tests` (compiles `test-rt/__lib__.yr`), then the
  static libs (`gymidgard-release_<ymirShortVersion>`, `gymidgard-debug_<ymirShortVersion>`,
  `gymidgard-tests_<ymirShortVersion>`, `runtime`) that bundle the `.o` plus the C sources under
  `core/*.c`, `std/*.c`, `rt/*.c`, then finally `midgard_tests`, the compiled test binary.
- Run tests: `.build/midgard_tests` (add `-f <substr>` to filter, `-sf` to stop on first
  failure, `--resume` to re-run only previously-failed tests, `-cov` for a coverage report,
  `-ct` for a call-tree report, `-m` to list each file's uncovered lines under the coverage
  report — see `test-rt/utils/args.yr`).
- Install system-wide: `sudo make install` (libs to `/usr/lib/`) and `sudo ./install` (copies
  `midgard/**/*.yr` into `/usr/include/ymir/<ymirShortVersion>` and the `gyc` internal include
  dir, both derived from `YMIR_VERSION`).
- `.build/.ymir_test_success` caches per-test pass/fail state, consumed by `--resume`.

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
  (`dwarf`, `elf`, `env`, `errno`, `exception`, `files`, `gc`, `memory`, `rand`, `threads` — thin
  Ymir wrappers around `rt/`).

### `rt/` — the C runtime

Backs the language runtime independent of Ymir compilation: `rt/memory` (GC, allocation,
class/type info, printing), `rt/except` (exception raising, stack unwinding, DWARF-based
stacktraces), `rt/concurrency` (threads, futures), `rt/utils` (demangling, env, errno, files,
math, string helpers), `rt/run.c` (process entry glue). Built once as `libruntime.a` and also
folded into both `gymidgard-*` static libs.

### `tests/` — the test suite

Mirrors the `std`/`core` layout (`tests/algorithm/`, `tests/fs/`, `tests/concurrency/`, ...).
Each file uses `__test { ... }` blocks with `assert(...)`; there's no separate golden-file
comparison like the compiler frontend's test suite — assertions are the pass/fail signal.
`tests/__lib__.yr` is the aggregate root compiled into `midgard_tests`.

### `test-rt/` — the test runner and coverage system

`test-rt/__lib__.yr` wires the `_yrt_register_unittest_impl` / `_yrt_run_unittests_impl`
extern hooks (called by compiler-generated `__test` glue) into `utils::runner::UnittestLauncher`,
and wires `_yrt_unittest_coverage_hit_{branch,enter,exit}` into a global `utils::coverage::tree`
CoverageTree singleton.

- `utils::args` — CLI parsing (`TestRunnerArgument`, built on `std::config::ArgumentParser`).
- `utils::filters` — include/exclude test-name filtering used by the runner.
- `utils::colors` — terminal color helpers for pass/fail/coverage output.
- `utils::runner` — `UnittestLauncher`: registers tests, runs them (respecting filters,
  stop-first, resume-from-`.ymir_test_success`, and `-j`/`--jobs` parallelism across
  `utils::worker::TestWorker` subprocesses), and drives coverage/call-tree reporting.
- `utils::worker` — `TestWorker` (fork/waitpid/exit wrapper, no `execvp`, unlike
  `std::concurrency::process::SubProcess`) and `TestChunks::split` (splits a test list into `-j`
  contiguous chunks); see "Parallel test execution" below.
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

### Coverage/call-tree feature, if you're extending it

The pipeline is: `gyc`-instrumented test binary hits the three `_yrt_unittest_coverage_hit_*`
externs during execution → `CoverageTree` in `test-rt/__lib__.yr` accumulates hits. At the start
of `UnittestLauncher::run`, unless `--resume` was passed, every existing `.ymir_coverage_*.json`
is deleted (`resetCoverageUnlessResuming`) so a plain run always starts from a clean slate; with
`--resume`, old files are left in place so this run's coverage adds to them instead of replacing
them. After running the tests, this process's tree is written to its own
`.ymir_coverage_<pid>.json` (`storeCoverage` → `CoverageStore::storePid`). If none of the tests
failed, every `.ymir_coverage_*.json` still on disk (this run's, plus any kept from `--resume`)
is loaded and merged into one `CoverageReport` (`reportCoverage` →
`CoverageLoad::listPidFiles`/`loadAll` → `CoverageConv::fromConfigs`/`merge`), which
`utils::coverage::format` prints on `--coverage`/`--call-tree`. When touching this path, check
that `store`/`load`/`conv` stay in sync with `CoverageTree`'s field set and the JSON shape they
produce, that `conv`'s merge logic still dedupes by (file, func) rather than double-counting
hits/call-counts across files, and that generated/compiler-synthesized functions (e.g.
`nextCtors`, lambdas) are still filtered out where expected (see commit `54babf5`).

### Parallel test execution (`-j`/`--jobs`), if you're extending it

`UnittestLauncher::run` defaults to `-j 1` (sequential, in-process, via `runList`) — every other
code path described above is unaffected. When `-j N` (`N > 1`) is passed and there's more than
one test to run, `runParallel` splits the filtered/sorted test list into `N` chunks
(`TestChunks::split`) and calls `TestWorker::fork()` once per chunk. `fork()` (not `execvp`) is
deliberate: by the time `run()` executes, every `__test` has already been registered into
`UnittestLauncher::_tests` (compiler-generated glue calls `_yrt_register_unittest_impl` before
`_yrt_run_unittests_impl`), so a forked child's copy-on-write memory already has the full test
registry — no re-exec, no IPC needed to hand it work. Each worker runs its chunk with the same
`runList` the sequential path uses, then persists its results (`storeCoverage`, unchanged —
already pid-keyed and multi-process-safe; `TestFilters::dumpSuccessFragment`, which writes to its
own `.ymir_test_success_<pid>.part` instead of the shared `.ymir_test_success`, since concurrent
writers would otherwise clobber each other) and terminates via `TestWorker::exit` (this call never
returns — a worker must never fall through and re-enter the parent's fork loop for the remaining
chunks). The parent `TestWorker::wait`s every child, then `TestFilters::mergeFragments` folds
every fragment into the run's `already` map and deletes the fragment files, before the unchanged
single `TestFilters::dumpSuccessFile`/`reportCoverage` calls at the tail of `run()`. Known,
accepted simplifications: `--stop-first` only stops a worker's own chunk early (no cross-process
cancellation of siblings), and worker stdout/stderr is inherited/unpiped and therefore interleaves
across workers. `TestWorker::fork` flushes stdout right before forking to avoid a real fork+stdio
artifact: buffered-but-unflushed output gets printed a second time, independently, by both the
parent and the child otherwise.
