# Níðhöggr: why it never frees a reachable object

This is the correctness argument of `core::nidhogg`, written against the code: every step names
the function it relies on, so that a change to one of them shows which part of the argument it has
to keep true. It proves one safety property, and the liveness of the handshakes; it does not prove
that memory is reclaimed promptly.

**Theorem (safety).** An object the sweep of cycle *N* frees is unreachable when it is freed, and
stays so: no thread, root or object ever holds a pointer to it again.

The argument holds under the assumptions of [§1](#1-model-and-assumptions). [§7](#7-the-model-checked-by-tlc)
is a model of the same algorithm, `formal/Nidhogg.tla`, that TLC checks exhaustively on small
instances; it is what found the two defects this argument now accounts for, in `allocLarge` and in
the park of `leaveBusy`, and it shows the gap of [§8](#8-the-barrier-test-and-its-store), which the
stores through the barrier close.

## 1. Model and assumptions

**Objects.** An object is a slot of a span whose `alloc` byte is 1 (`findObject`). It is *marked*
when its `marks` byte is non zero. A word *points into* an object when it lies anywhere in it,
interior pointers included, one byte past its end too (`allocate` adds that byte).

**Roots.** The writable segments of the loaded objects (`_yrt_i_nh_data_segments`), the ranges the
runtime registers (`_yrt_nh_add_roots`), the objects whose finalizer is queued
(`scanFinalizerQueue`), and for every registered thread its stack from the address it parked at up
to its top, its registers, and its static TLS block (`scanThread`). An object is *reachable* when a
chain of pointers leads to it from a root.

**Colors during the marking of cycle *N*.** An object is *white* while unmarked, *grey* while
marked and in a set of objects to scan (the mark stack of the collector, a grey buffer of a thread,
a buffer handed over in `work`), *black* while marked and not grey. A thread is *grey* until its
stack is scanned in the cycle (`ROUND_SCAN`, or `ROUND_FLUSH` for a thread registered since),
*black* afterwards.

**Times of cycle *N*.**
- *M*: `_yrt_nh_marking` is set to 1, followed by a fence (`runCycle`).
- *a(T)*: thread *T* acknowledges the handshake of the `ROUND_ACK` round.
- *s(T)*: the collector scans *T*'s stack.
- *E*: the `ROUND_FLUSH` round after which `terminate` stops (the last one, after `finalize`).
- *G*: `sweepGen` is increased by 2, then `toUnswept`, `_yrt_nh_marking` is set to 0, `quiesce`
  (a `ROUND_ACK` round), and `sweepAll`.

Then *M* < *a(T)* < *s(T)* < *E* < *G* for every thread registered at *M*.

**Assumptions.**
- **A1 (no hidden pointers).** A collected pointer is stored only in memory the collector scans:
  collected objects, roots, stacks, registers and TLS blocks of registered threads. Memory from
  `malloc` holding such a pointer is registered as a root (the exception headers are). A thread the
  collector does not know holds no collected pointer.
- **A2 (barriers).** Every store of a pointer into memory other than the storing thread's own frame
  is made by `_yrt_nh_store_ptr`, `_yrt_nh_store` or `_yrt_nh_clear` (what compiled code and the
  helpers of `rt/utils/gc.h` call), or follows `_yrt_nh_wb` or `_yrt_nh_wb_range`, and memory
  copied into a new object goes through `_yrt_nh_shade_range` (as `_yrt_i_gc_copy_new` does).
- **A3 (registration).** A thread registers before it holds a collected pointer it did not load
  itself: the runtime registers a thread before running any Ymir code on it, and the spawning
  thread keeps the arguments on its own stack until then (`_yrt_i_thread_create`, `_thread_main`).
- **A4 (handshake signal).** A registered thread does not block the handshake signal for long, and
  its signal handler runs on its own stack.
- **A5 (test and store).** No handshake parks a thread between the read of `_yrt_nh_marking` that
  decides whether a store is shaded and that store, except the park of `leaveBusy` in `_yrt_nh_wb`
  and `_yrt_nh_wb_range`, after which they shade again. `_yrt_nh_store_ptr`, `_yrt_nh_store` and
  `_yrt_nh_clear` read the flag and store while the thread is busy (Lemma 1), so it holds for them;
  code testing the flag itself before calling `_yrt_nh_wb` breaks it,
  [§8](#8-the-barrier-test-and-its-store).
- **A6 (no data race on a slot).** Two threads never store into one slot at once: a program
  orders its stores to a shared field with a lock or an atomic. Without it, a barrier shades the
  value the slot held when it read it, not the one its store overwrites (`formal/Racy.cfg`).

## 2. The handshake is a rendezvous

**Lemma 1.** When the collector returns from waiting for thread *T* to park in handshake *k*
(`suspend`, or `isParked` in `round`), *T* has been in `_yrt_i_nh_handshake`, has published `sp`,
and has finished every write barrier and assist it started. Unless *k* is only to be acknowledged
(a `ROUND_ACK`), *T* stays there until the collector stores `go = k`. Everything *T* wrote before
parking is visible to the collector, everything the collector wrote before `req = k` is visible to
*T* from its park on, and everything it wrote before `go = k` is visible to *T* once it resumes.

*Proof.* Requests to one thread are serialized: the collector stores `req = k` only once it has
stored `go = k - 1` (in `letGo`, or in `request` when the signal could not be sent), so while *T*
handles request *k*, `go` is `k - 1` or `k`. The handler reads `seq = req` with acquire ordering,
which the collector stored with release ordering. It returns at once if `go == seq`, which means
request *k* is already served. Otherwise it reads `ack`, stored by `request` before `req`, then
stores `sp`, then `parked = seq` with release ordering, then wakes the futex `parks`. The collector
reads `parked` with acquire ordering, so `sp` and *T*'s earlier writes happen before what the
collector does next. An acknowledgment returns there: the collector does nothing to *T*, and what
the round needs of it, that *T* reads `_yrt_nh_marking` as the collector stored it before the
request and that no barrier of *T* started before is still running, holds from the park on.

Otherwise *T* then sleeps while `go == seq - 1`. That is the only value `go` can hold before the
collector releases *T*. It sleeps on the futex `releases`, which the collector increments after
each store of a `go` (`letGo`) and wakes once a pass is done (`wakeReleased`): *T* reads `releases`
before `go`, so a release falling between its read of `go` and its sleep has changed `releases`,
and the sleep returns at once. If a handshake nested in a park started from `leaveBusy` has
already pushed `go` past `seq`, the loop still ends: it waits for `go` to leave `seq - 1`, not for
`go == seq`, and waiting for equality was the deadlock found in this branch. The collector stores
`go` with release ordering, so its writes are visible to *T* after it resumes.

*T* never parks inside a barrier or an assist. `_yrt_i_nh_enter_busy` sets `busy`, a thread local,
then a signal fence. The handler runs on *T* itself, so it sees the stores of *T* in program order:
one that reads `busy == 1` only sets `deferred` and returns, and `_yrt_i_nh_leave_busy` parks *T* in
`_yrt_i_nh_park` once `busy` is 0 again. No other thread reads `busy`, so it needs no hardware
fence: `quiesce` waits for the barriers with a round, not by reading it. ∎

**Lemma 2 (liveness).** A round ends.

*Proof.* A thread is parked only between its request and its release, and the collector takes no
lock while one is parked: the work of `ROUND_SCAN`, `ROUND_FLUSH` and `ROUND_ACK` is lock free
(`scanThread`, `takeGrey`, `markPush`). A thread the collector waits for can be blocked by a parked
one: a barrier of *T* calling `link`, which takes `workLock`, while a parked *U* holds it. That is
why `round` never waits for one given thread. It waits until any thread in its window parks
(`awaitPark` on the futex `parks`), and serves every parked thread it finds, so *U* is released
and *T* can go on. A thread blocked in a lock, in `pace`, or in a system call takes the signal and
parks (A4). The handler wakes `parks` after storing `parked`, and `awaitPark` sleeps only while
`parks` still holds the value read before the check, so no wake-up is lost. `ROUND_RECLAIM` takes
list locks (`releaseCache`), so it parks one thread at a time, and it parks *T* only after reading
`inAlloc == 0`. ∎

## 3. What the barriers keep

Write *P(W)*, "*W* is protected", for: *W* is reachable from a grey object or from the stack of a
grey thread through white objects only.

**Invariant WT (weak tricolor), on [*s₀*, *G*], where *s₀* is the first stack scan of the
cycle.** Every white object pointed to by a black object, by a root other than a stack, or by the
stack or registers of a black thread, is protected.

**Lemma 3.** WT holds from *s₀* to *G*.

*Proof.* At *s₀* nothing is black except the objects allocated black since *M*, which are empty
apart from what was stored into them through a barrier, and the roots were scanned just before
(`scanRoots`, the data segments, `scanFinalizerQueue`). So WT holds. Each kind of step keeps it:

- **Shading** (`shade`) turns a white object grey. That only removes white objects and adds grey
  ones, which can only protect more.
- **Scanning an object or a stack** (`scanObject`, `scanThread`) shades every word of it before
  making it black, so no white object is left pointed to by it.
- **A thread loads a pointer to *W***, white, from a location *L* it already holds. If *L* is in a
  black object, *W* is protected by WT. If *L* is in a grey object, *W* is reachable from it. If
  *L* is in a white object *O*, *O* is held by the thread: a black thread's *O* is protected, so
  *W* is too through *O*, and a grey thread's *W* is reachable from its own stack.
- **A thread stores *V* into a slot holding *Y***, through a barrier (A2), during the marking
  (A5), *Y* being what the slot still holds when the store lands (A6). The barrier shades both *Y*
  and *V* (`_yrt_nh_store_ptr`, `_yrt_nh_wb`; `_yrt_nh_store` and `_yrt_nh_wb_range` shade both
  ranges). `_yrt_nh_store_ptr` and `_yrt_nh_store` store before the thread can park. If `leaveBusy`
  parked the thread once the shading of `_yrt_nh_wb` was done, the handshake may have started the
  marking the shading was not part of, so `_yrt_nh_wb` and `_yrt_nh_wb_range` shade again before
  they return. Shading *V* protects the new edge whatever the colors. Shading *Y* keeps every path
  that went through the overwritten edge: what was protected through *Y* is now protected by *Y*,
  which is grey. This is the barrier of Go's hybrid proposal, with the insertion half applied even
  from black stacks, so the argument of that proposal applies.
- **A copy into new memory** shades its source range first (`_yrt_nh_shade_range`), which is the
  barrier of every word it copies.
- **Dropping a pointer from a stack** is not seen by any barrier. A black stack protects nothing,
  so dropping from it cannot break a protection. A grey stack can protect *W* by holding it, but
  WT only concerns *W* if a black object, a root or a black stack also points to it. That pointer
  was stored through a barrier, which shaded *W*, or loaded, and then *W* is protected by its
  source (the previous step).
- **An allocation** creates either a black object (Lemma 4) or a white one held by a grey thread
  (Lemma 4). A white object held by a grey thread is protected by that stack.
- **A thread registering** (A3) has a grey stack until its first `ROUND_FLUSH`, and holds only
  what its parent still holds.
- **The collector's other work** keeps the marks: `clearAllMarks` runs before *M*, and nothing
  unmarks an object between *M* and *G*. ∎

## 4. Allocation

**Lemma 4.** An object allocated by thread *T* is marked, unless *T* is grey and holds it, or the
allocation came before *M* and its span is swept only after the marking of cycle *N*.

*Proof.* *Small objects* (`allocate`) come from a span cached by *T*. Only *T* allocates from it,
and no sweep touches it while it is cached: `reclaim` takes it back only when `inAlloc == 0`. *T*
sets the `alloc` byte (`spanAlloc`) and zeroes the object, and only then reads
`marking() || sweepGen != sweepGen()`. If that read gives 1, the object is marked. If it gives 0,
then either *M* is not visible to *T* yet, so *T* has not passed *a(T)* (after the handshake *T*
reads 1, Lemma 1), the object already exists and is held by *T*, and *s(T)* comes later and finds
it in *T*'s stack or registers. Or *G* has passed, and the span is swept at the end of the next
cycle (handled in [§6](#6-the-sweep)).

*Large objects* (`allocLarge`) get a span of their own, pushed onto `large`, which `sweepAll` walks
under the heap lock. The same decision is made after `spanAlloc` and **under the heap lock**, so
the same two cases apply, and the end-of-cycle sweep cannot run between the allocation and the
decision.

The proof is what exposed a defect of the code before this branch. `allocLarge` was given
`black = marking()`, read before taking the heap lock. A thread reading 0 and then waiting on that
lock could have its stack scanned (*s(T)*) before the object existed. The object then came out
white and held by a black thread, which WT does not allow, and the sweep at *G* freed it while it
was in use. Holding threads in `pace` released them all at once at the end of a cycle, so they all
competed for the heap lock as the next one began. That turned the race into one broken `large`
run in about ten. ∎

## 5. The end of the marking

**Lemma 5.** At the end of the last `ROUND_FLUSH` of cycle *N* (time *E*), no reachable object is
white.

*Proof.* `terminate` drains the mark stack and the buffers in `work` (`drain`), then runs a
`ROUND_FLUSH`. That round moves every thread's grey buffer onto the mark stack (`takeGrey`) and
scans every stack not scanned yet in the cycle, then drains again. The loop stops when nothing was
found: no buffer held an object, no stack was left to scan, no buffer was handed over, no assist
started (`assists` unchanged), and no thread unknown to the collector was in a barrier
(`foreignBusy`).

Suppose a thread shaded a white object after its own flush in that round. Take the first such
shade, at time *t*, of *W*. By Lemma 3, *W* was protected just before *t*. Every stack had already
been scanned (by `ROUND_SCAN`, or by this round for a thread registered since), so the protection
came from a grey object *Q*. *Q* became grey in a buffer, the mark stack, or `work`. If before the
round, *Q* was drained or found in a thread's buffer (that round then finds something, a
contradiction). If during the round, it came from a shade before the shading thread's flush (found)
or after it (earlier than *t*, against the choice of *t*). Assists move buffers out of `work`
(counted by `assists`), and threads in a barrier park only once done (Lemma 1), so no shade escapes
the count. Hence no thread shades a white object between its flush and *E*, no grey object exists
at *E*, and by WT no black object, root or stack points to a white object. Every reachable object
is therefore black. `finalize` then marks what queued finalizers point to and runs `terminate`
again, which ends the same way. ∎

## 6. The sweep

**Lemma 6.** Between *E* and the sweep of a span, no white object of that span becomes reachable,
and the sweep reads the marks of cycle *N*.

*Proof.* After *E*, every reachable object is black (Lemma 5), and a store can only copy pointers
to reachable objects, so the only new objects are allocations. Until `_yrt_nh_marking` is cleared
they are black (Lemma 4). After it is cleared, a thread has also read the new `sweepGen`: it is
increased with an acquire/release add and a fence before the release store of the flag, and the
flag is read with acquire ordering. So an object from a span not swept since *G* is marked
(`sweepGen != sweepGen()`). An object from a span already swept for the new generation is white,
but that span is swept next only at the end of cycle *N + 1*, after its own marking.

A span is swept when its generation is older than the current one: by `sweepAll` (after
`quiesce`, whose `ROUND_ACK` round reaches every thread out of its barriers once the flag is 0, so
that no barrier still reads the span), or by `refill`, both under the lock of its list. That
is right only with the marks of a finished marking, so between *G* and the `clearAllMarks` of cycle
*N + 1*, which requires that no span of an older generation is still in a list or a cache once those
marks are cleared:
- the spans in the lists at *G*: `toUnswept` moves them, under the lock of each list, after the
  generation advanced, and `sweepAll` sweeps them;
- the spans handed back to a list: `release` and `refill` read the generation **under the lock of
  the list**. A release either takes the lock before `toUnswept` and its span is moved, or after it
  and it reads the new generation. Read before the lock, a generation could predate *G*, and a span
  of the generation before would land in `avail` behind `toUnswept`. Nothing would sweep it until a
  `refill` popped it during the next marking, and that `refill` would sweep it with marks still
  being made, freeing what they had not reached yet. Threads exiting hand back their whole cache at
  once, which made this one break `tests/nidhogg/threads` in about one run out of twenty under
  contention;
- the spans in a cache: `ROUND_RECLAIM` of cycle *N + 1* takes them all back, and `sweepAll` sweeps
  those of the generation before, before `clearAllMarks`. A large
object is freed by `sweepAll` under the heap lock, only when its `sweepGen` is from the last
generation and its mark is 0, which by Lemma 4 cannot hold for one allocated during or after the
marking. ∎

*Proof of the theorem.* By Lemmas 5 and 6, an object the sweep frees was unreachable at *E* and
stayed white, so it was unreachable when freed. An unreachable object cannot become reachable: a
thread can only obtain a pointer by loading it from memory it reaches, or by allocating (A1).
Conservative scanning only adds marks, so a word that merely looks like a pointer keeps garbage
alive, and never frees anything. ∎

## 7. The model checked by TLC

`formal/Nidhogg.tla` models the threads and the collector at the grain at which they interleave.
Threads load, drop, allocate, and store through the barrier in three steps: the test of the flag,
the body of the barrier, and the store. A handshake parks a thread before the test, after the store,
or between the body and the store, after which the body runs again (`_yrt_nh_wb`). The stores of
`_yrt_nh_store_ptr` never park there, so they are one of the behaviors the model checks. The collector clears
the marks, sets the flag, and runs the `ROUND_ACK`, root and `ROUND_SCAN` rounds (serving the
threads in the order they park, a thread acknowledging the `ROUND_ACK` going on at once). It then drains and runs `ROUND_FLUSH` rounds until one finds
nothing, and sweeps. `Safe` states that every object reachable from the global, a stack or the heap
is allocated. TLC explores every interleaving of an instance. With `Lifecycle`, threads are also
spawned (the parent keeping what it hands over on its stack until the child registered), register
outside the rounds, and exit (their grey buffer handed over, their stack no longer a root). The
model leaves out the arithmetic (the page map, the size classes), the span lists and the lazy
sweeping (its sweep is one atomic step), assists, and finalizers. The race of `release` in Lemma 6
lived in that gap: the stress tests found it, and assertions in the collector located it.

| configuration | threads, objects | what it changes | result |
|---|---|---|---|
| `Small1T.cfg` | 1, 3 | nothing | holds: 801,579 states, the whole graph |
| `Small2T.cfg` | 2, 2 | nothing | holds: 1,713,169 states, the whole graph |
| `Nidhogg.cfg` | 2, 3 | nothing | holds: 133,420,992 states, the whole graph (21 minutes on 8 cores) |
| `Life2T.cfg` | 2, 2 | with `Lifecycle` | holds: 3,220,293 states, the whole graph |
| `Life3T.cfg` | 3, 2 | with `Lifecycle` | holds: 153,415,832 states, the whole graph (78 minutes on 8 cores) |
| `Life.cfg` | 2, 3 | with `Lifecycle` | holds: 255,134,049 states, the whole graph (57 minutes on 8 cores) |
| `NoScanNew.cfg` | 2, 2 | with `Lifecycle`, a flush round no longer scans a thread registered since the SCAN round | holds: 3,219,597 states, the whole graph |
| `LargeEarly.cfg` | 2, 2 | black decided before the object exists (`allocLarge` before the fix) | fails in 19 steps |
| `NoRedo.cfg` | 1, 2 | `leaveBusy` parks without shading again (the barrier before the fix) | fails in 29 steps |
| `TestStoreGap.cfg` | 2, 2 | a handshake between a test of the flag before `_yrt_nh_wb` and the store (A5 dropped) | fails in 23 steps |
| `Racy.cfg` | 2, 2 | two threads storing into one slot at once (A6 dropped) | fails in 34 steps |
| `NoShadeOld.cfg` | 1, 2 | the barrier without its deletion half | fails in 25 steps |
| `NoShadeNew.cfg` | 2, 2 | the barrier without its insertion half | fails in 23 steps |
| `ExitDropsGrey.cfg` | 2, 3 | with `Lifecycle`, a thread exiting drops its grey buffer | fails in 30 steps |

The failing configurations are what make the passing ones mean something: the model notices every
defect it is given, and each trace is a scenario the code allows, in the same steps.

`NoScanNew.cfg` is the exception, on purpose: it holds, so scanning the stack of a thread registered
during a marking is not needed. Such a thread holds only what it allocated, black, or loaded,
protected, and what its parent handed over, which the parent keeps until the child registered: a new
thread can start black, as a new goroutine does in Go. The flush round scans it anyway, at little cost.

To run a configuration:

```text
java -cp tla2tools.jar tlc2.TLC -workers auto -config formal/Small2T.cfg formal/Nidhogg.tla
```

TLC keeps its state files in `-metadir` (by default next to the model), several gigabytes for
`Nidhogg.cfg`. Point it at a disk, not a tmpfs.

**What the tests add.** The heap verification (`NIDHOGG_VERIFY=1`, `verifyMarks`) checks WT on the
real heap after every marking, with every thread parked: no marked object, root or data segment may
point to an unmarked one. The stress cases of ymir-dev (`tests/nidhogg/`) check the outcome, and
they cover what the model leaves out. `large` was the one that broke when Lemma 4 did not hold.

## 8. The barrier test and its store

A store testing `_yrt_nh_marking` itself, then calling `_yrt_nh_wb` and storing (what compiled code
did before `_yrt_nh_store_ptr`), can acknowledge `ROUND_ACK` between a test that read 0 and its
store. It goes on after *a(T)* and stores without a barrier. The same happens after a test that
read 1, if the barrier body ran before *M*: once the barrier has returned, a plain handshake (not
the park of `leaveBusy`, which shades again) leaves the store unshaded. Either way the store step
of Lemma 3 breaks:

- **Insertion.** *T* stores *V*, white and held only by *T*'s stack, into an object *B* allocated
  black by a thread that acknowledged before *T*. If *T* then drops *V* before *s(T)*, nothing
  protects it.
- **Deletion.** The overwritten *Y* is not shaded. If a thread already scanned loaded *Y* after its
  scan, *Y* ends up held by a black stack only.
- **Into a root.** The shortest case, found by TLC (`formal/TestStoreGap.cfg`): *T* reads 0 and is
  about to store *V* into a global. The roots are scanned while the global is still empty, *T*
  stores *V* without a barrier, then drops it before *s(T)*. *V* is now held only by a root that has
  already been scanned, and the sweep frees it.

Running the ACK round twice does not help: a thread preempted between the test and the store is
still there at the second round. So the test belongs to the barrier: the compiler makes a store of
a pointer out of the frame a call, `_yrt_nh_store_ptr(&L, t)`, or `_yrt_nh_store(&L, &t, size)` for
an aggregate, and the helpers of `rt/utils/gc.h` call the same functions. They set `busy`, test the
flag, shade, store, and leave `busy`, a handshake falling anywhere in between being deferred until
the store is done (Lemma 1). The store either lands before *a(T)*, part of the heap the marking
starts from, or after a test that read 1 in the same busy section as the shading. `busy` being
thread local, the call needs no hardware fence: it costs about 1 ns more per store than the test
inlined (82 million stores of a pointer into an array take 105 ms against 32 ms), which the
benchmarks of the workspace do not show beyond their noise. `_yrt_nh_wb` and `_yrt_nh_wb_range`
stay for code compiled by a gyc that tests the flag inline, which keeps the gap.
