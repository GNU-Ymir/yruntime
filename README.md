# Midgard

Midgard is the standard library for [GNU-Ymir](https://github.com/GNU-Ymir) (`gyc`), covering
everything under the `std`, `core`, and `etc` module roots — collections, algorithms, I/O,
filesystem, networking, concurrency (threads/actors/futures), formatting, config parsing, and
low-level runtime/C bindings.

It is one component of a larger Ymir toolchain checkout: `gyc` (the GCC-based Ymir compiler)
builds and links against Midgard, and the `midgard` sources are installed as system-wide Ymir
includes.

## Getting started

Building and testing this repo requires a `gyc` toolchain matching the version this repo
targets (`YMIR_VERSION` at the repo root) already installed on the machine — this repo does not
build `gyc` itself. If you don't already have a matching `gyc` on `PATH` (check with `gyc
--version`), bootstrap one:

```sh
./dev/init-dev-env.sh
```

This downloads the `gyc` release matching `YMIR_VERSION` from the
[gymir releases](https://github.com/GNU-Ymir/gymir/releases), installs it system-wide (expects
a Debian/Ubuntu-derived system with `apt`/`sudo`), and runs `./install` (see
[Installing](#installing)) so this repo's own `.yr` sources are where that `gyc` looks for
them. It's safe to re-run any time `YMIR_VERSION` changes.

Once `gyc` is available, see [Building](#building) below to compile Midgard itself and
[Running tests](#running-tests) to check your setup.

## Layout

- `midgard/` — the library sources, organized as three top-level modules:
  - `std/` — general-purpose standard library (`algorithm`, `collection`, `io`, `fs`, `net`,
    `concurrency`, `format`, `config`, `conv`, `time`, `stream`, `syntax`, ...).
  - `core/` — language-support runtime types (`exception`, `types`, `reflect`, `atom`, `math`,
    `concurrency`).
  - `etc/` — bindings to the C runtime and other externals (`etc::c`, `etc::runtime::*`).
- `rt/` — the C runtime backing Midgard (memory/GC, exceptions/stacktraces, threading,
  low-level utils). Compiled directly into each Midgard static library target (`c-sources` in
  `gyllir.toml`), rather than as a standalone runtime library.
- `tests/` — the Ymir test suite (`__test { ... }` blocks), mirroring the `std`/`core` module
  layout.
- `test-rt/` — the unit-test runner and its support library (`utils::runner`, `utils::args`,
  `utils::filters`, `utils::coverage::*`): test registration, CLI argument parsing, filtering,
  and code coverage collection/reporting.

## Building

Requires a working `gyc` toolchain (looked up as `gyc` on `PATH`, `compiler = "gyc"` in
`gyllir.toml`) and `gyllir` itself.

```sh
gyllir build
```

Midgard's own version lives in `VERSION` (also mirrored, and occasionally ahead of it while a
release is in flight, in `gyllir.toml`'s own `version` field); the ymir bootstrap version this
library is *compiled with* is a separate value, in `YMIR_VERSION` at the repo root (neither is
hardcoded). Intermediate objects land under `.target/`; this produces at the repo root:

- `libgymidgard_release.a` / `libgymidgard_debug.a` — the compiled Midgard library (release and
  debug builds).
- `libgymidgard_debug_unit.a` / `libgymidgard_tests.a` — the unit-test-instrumented library build
  and the test-runner support library.
- `midgard_tests` — the compiled Midgard test suite binary.

### Installing

```sh
sudo ./install          # installs the .yr sources as system Ymir includes
```

`install` copies `midgard/**/*.yr` into `/usr/include/ymir/<midgardShortVersion>` and the `gyc`
internal include path (the version component from `VERSION`, GCC major from `YMIR_VERSION`), so
other Ymir projects can `use std::...` / `use core::...` against this library. There is currently
no equivalent step for installing the built static libraries system-wide — copy
`libgymidgard_*.a` to `/usr/lib/` by hand if you need that.

## Running tests

```sh
./midgard_tests
```

Useful flags (see `test-rt/utils/args.yr`):

- `-f, --filter <substr>` — only run tests whose name contains `<substr>`.
- `-sf, --stop-first` — stop after the first failing test.
- `--resume` — re-run only tests that failed (or didn't run) last time.
- `-cov, --coverage` — print a branch/line coverage report after the run.
- `-ct, --call-tree` — print a call-tree report after the run.
- `-m, --missing` — list the uncovered lines (and half taken branches) of each file, as line
  ranges, under its entry of the coverage report (implies `-cov`).

## Contributing

- Every change is tracked by a [Linear](https://linear.app) issue in the **Ymir** team (key
  `YMI`) — open one before starting work if it doesn't already exist.
- Name the branch after the issue: `YMI-<issue_number>-<short-description>` (e.g.
  `YMI-34-coverage-by-files`).
- Name the pull request the same way, prefixed with the issue key (e.g.
  `YMI-34: Coverage by files`), so it's traceable back to the Linear issue.

## License

GPLv3 — see [LICENSE.txt](LICENSE.txt).
