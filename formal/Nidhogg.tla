------------------------------ MODULE Nidhogg ------------------------------
(***************************************************************************)
(* A model of the marking and the sweep of core::nidhogg, checked by TLC:  *)
(* threads loading, dropping, storing through the write barrier and        *)
(* allocating, while the collector runs its cycles. Safe states that no    *)
(* object reachable from the global, a stack or the heap was ever freed.   *)
(*                                                                         *)
(* An object has one pointer field. A store through the barrier is three   *)
(* steps: the test of _yrt_nh_marking, the body of the barrier (busy, so   *)
(* no handshake parks the thread inside it), and the store. A thread parks *)
(* between the body and the store only from leaveBusy, as _yrt_nh_wb lets  *)
(* it, and shades again; _yrt_nh_store_ptr never parks there, one of the   *)
(* behaviors checked. Threads are spawned, register and exit as the        *)
(* runtime has them do, and assist the collector from the grey buffers     *)
(* handed over. The toggles below put the defects back, so that the model  *)
(* is shown to catch them.                                                 *)
(***************************************************************************)
EXTENDS Naturals, FiniteSets, TLC

CONSTANTS
    Thr,              \* the threads
    Obj,              \* the objects of the heap, allocated or free
    Null,             \* the null pointer
    MaxHeld,          \* the most objects a thread holds at once
    LargeFixed,       \* FALSE: decide black before the object exists (allocLarge before the fix)
    AtomicTestStore,  \* FALSE: a handshake can park a thread between its test of the flag and its store,
                      \* outside the barrier body, as code testing the flag before _yrt_nh_wb lets it
    RedoBarrier,      \* FALSE: a thread parked from leaveBusy goes on to its store without shading again
    RaceFree,         \* FALSE: two threads can store into one slot at once (a data race of the program)
    ShadeOld,         \* FALSE: the barrier does not shade the overwritten pointer
    ShadeNew,         \* FALSE: the barrier does not shade the stored pointer
    Lifecycle,        \* FALSE: every thread is registered from the start and never exits
    ScanNewThreads,   \* FALSE: a flush round does not scan a thread registered since the SCAN round
    ExitHandsGrey,    \* FALSE: a thread exiting drops its grey buffer instead of handing it over
    Assists,          \* TRUE: grey buffers overflow into work, and threads assist from it
    AssistAfterAck,   \* FALSE: a thread assists once it reads the flag at 1, before the ACK round ended
    CountAssists      \* FALSE: terminate ignores the assists that took a buffer during its round

VARIABLES
    heap,       \* the pointer field of each object
    glob,       \* a pointer of a data segment, a root
    alloc,      \* TRUE iff the object is allocated
    mark,       \* the mark of each object
    stack,      \* the objects each thread holds, on its stack or in its registers
    tpc,        \* where each thread is in a multi-step operation
    tdst,       \* the slot a store is writing: an object, or "glob"
    tval,       \* the pointer a store writes
    tm,         \* what the barrier test read, or what a large allocation read as black
    tobj,       \* the object an allocation returns
    grey,       \* the grey buffer of each thread
    marking,    \* _yrt_nh_marking
    sweeping,   \* the end of the marking has advanced sweepGen, the sweep has not run yet
    markStack,  \* the mark stack of the collector
    req,        \* the collector asked the thread to park
    parked,     \* the thread is parked in the handler of the handshake
    cpc,        \* where the collector is in its cycle
    pending,    \* the threads the current round has not served yet
    found,      \* the current flush round found grey objects
    reg,        \* "none", "starting" (created, not registered yet) or "running" (registered)
    scanned,    \* the stack of the thread was scanned in the current cycle
    work,       \* the grey buffers handed over to the collector, by the threads that exited
    tchild,     \* the thread a thread spawned and waits for, until it registered
    copy,       \* the copy of the stack of each thread the round served, to scan
    copied,     \* the threads whose copy is not scanned yet
    assisted    \* an assist took a buffer since the last drain ended (`assists` changed)

vars == << heap, glob, alloc, mark, stack, tpc, tdst, tval, tm, tobj, grey, marking, sweeping,
           markStack, req, parked, cpc, pending, found, reg, scanned, work, tchild, copy, copied,
           assisted >>

LifeUnchanged == UNCHANGED << reg, scanned, work, tchild, copy, copied, assisted >>

NoAssist == UNCHANGED assisted

Ptr == Obj \cup {Null}

-----------------------------------------------------------------------------
(* Reachability *)

Roots == ({glob} \ {Null}) \cup UNION {stack[t] : t \in Thr}

RECURSIVE Closure(_, _)
Closure(S, n) ==
    IF n = 0 THEN S
    ELSE Closure(S \cup ({heap[o] : o \in S} \ {Null}), n - 1)

Reachable == Closure(Roots, Cardinality(Obj))

(* The property: nothing reachable was freed *)
Safe == \A o \in Reachable : alloc[o]

TypeOK ==
    /\ heap \in [Obj -> Ptr]
    /\ glob \in Ptr
    /\ alloc \in [Obj -> BOOLEAN]
    /\ mark \in [Obj -> BOOLEAN]
    /\ stack \in [Thr -> SUBSET Obj]
    /\ tpc \in [Thr -> {"idle", "barrier", "store", "decide", "large", "spawnwait", "assist"}]
    /\ grey \in [Thr -> SUBSET Obj]
    /\ markStack \in SUBSET Obj
    /\ cpc \in {"idle", "ack", "scan", "drain", "flush", "sweep"}
    /\ reg \in [Thr -> {"none", "starting", "running"}]
    /\ work \in SUBSET Obj
    /\ copy \in [Thr -> SUBSET Obj]
    /\ copied \in SUBSET Thr
    /\ assisted \in BOOLEAN

-----------------------------------------------------------------------------
(* Shading: mark a white allocated object, and put it where it will be scanned *)

White(p) == p # Null /\ alloc[p] /\ ~mark[p]

ShadeSet(S) == {p \in S : White(p)}

MarkAll(S) == [o \in Obj |-> mark[o] \/ o \in S]

-----------------------------------------------------------------------------
(* Threads *)

Held(t) == stack[t]

\* A thread parks at any step but inside the body of a barrier (a single step here) or an assist.
\* Between the test of compiled code and its store, two parks are possible: the one leaveBusy makes
\* once the body is done (tpc = "store" after a test that read 1), which RedoBarrier follows with the
\* body again, and a plain one, which AtomicTestStore rules out.
DeferredPark(t) == tpc[t] = "store" /\ tm[t]

PlainParkOK(t) ==
    /\ tpc[t] # "assist"
    /\ \/ tpc[t] \notin {"barrier", "store"}
       \/ ~AtomicTestStore
       \/ DeferredPark(t) /\ ~RedoBarrier

\* A handshake of an ACK round is only acknowledged: the thread goes on at once, and the collector
\* has nothing more to do on it
Park(t) ==
    /\ req[t] /\ ~parked[t]
    /\ \/ PlainParkOK(t) /\ UNCHANGED tpc
       \/ DeferredPark(t) /\ RedoBarrier /\ tpc' = [tpc EXCEPT ![t] = "barrier"]
    /\ IF cpc = "ack"
       THEN /\ req' = [req EXCEPT ![t] = FALSE]
            /\ pending' = pending \ {t}
            /\ UNCHANGED parked
       ELSE /\ parked' = [parked EXCEPT ![t] = TRUE]
            /\ UNCHANGED << req, pending >>
    /\ UNCHANGED << heap, glob, alloc, mark, stack, tdst, tval, tm, tobj, grey, marking,
                    sweeping, markStack, cpc, found >>
    /\ LifeUnchanged

Running(t) == reg[t] = "running" /\ ~parked[t]

ThreadVarsUnchanged ==
    /\ UNCHANGED << marking, sweeping, markStack, req, parked, cpc, pending, found >>
    /\ LifeUnchanged

Load(t) ==
    /\ Running(t) /\ tpc[t] = "idle"
    /\ Cardinality(stack[t]) < MaxHeld
    /\ \E src \in stack[t] \cup {"glob"} :
         LET p == IF src = "glob" THEN glob ELSE heap[src] IN
         /\ p # Null
         /\ stack' = [stack EXCEPT ![t] = @ \cup {p}]
    /\ UNCHANGED << heap, glob, alloc, mark, tpc, tdst, tval, tm, tobj, grey >>
    /\ ThreadVarsUnchanged

Drop(t) ==
    /\ Running(t) /\ tpc[t] = "idle"
    /\ \E o \in stack[t] : stack' = [stack EXCEPT ![t] = @ \ {o}]
    /\ UNCHANGED << heap, glob, alloc, mark, tpc, tdst, tval, tm, tobj, grey >>
    /\ ThreadVarsUnchanged

\* if (_yrt_nh_marking) ... : the test of compiled code. A program free of data races never has two
\* threads storing into one slot at once
StoreTest(t) ==
    /\ Running(t) /\ tpc[t] = "idle"
    /\ \E d \in stack[t] \cup {"glob"}, v \in stack[t] \cup {Null} :
         /\ RaceFree => \A u \in Thr \ {t} : ~(tpc[u] \in {"barrier", "store"} /\ tdst[u] = d)
         /\ tdst' = [tdst EXCEPT ![t] = d]
         /\ tval' = [tval EXCEPT ![t] = v]
    /\ tm' = [tm EXCEPT ![t] = marking]
    /\ tpc' = [tpc EXCEPT ![t] = "barrier"]
    /\ UNCHANGED << heap, glob, alloc, mark, stack, tobj, grey >>
    /\ ThreadVarsUnchanged

Old(t) == IF tdst[t] = "glob" THEN glob ELSE heap[tdst[t]]

\* _yrt_nh_wb: busy, it tests the flag again and shades both pointers into the grey buffer
Barrier(t) ==
    /\ Running(t) /\ tpc[t] = "barrier"
    /\ LET S == IF tm[t] /\ marking
                THEN (IF ShadeOld THEN {Old(t)} ELSE {}) \cup (IF ShadeNew THEN {tval[t]} ELSE {})
                ELSE {}
           G == ShadeSet(S) IN
       /\ mark' = MarkAll(G)
       /\ grey' = [grey EXCEPT ![t] = @ \cup G]
    /\ tpc' = [tpc EXCEPT ![t] = "store"]
    /\ UNCHANGED << heap, glob, alloc, stack, tdst, tval, tm, tobj >>
    /\ ThreadVarsUnchanged

Store(t) ==
    /\ Running(t) /\ tpc[t] = "store"
    /\ IF tdst[t] = "glob"
       THEN glob' = tval[t] /\ UNCHANGED heap
       ELSE heap' = [heap EXCEPT ![tdst[t]] = tval[t]] /\ UNCHANGED glob
    /\ tpc' = [tpc EXCEPT ![t] = "idle"]
    /\ tdst' = [tdst EXCEPT ![t] = Null]
    /\ tval' = [tval EXCEPT ![t] = Null]
    /\ tm' = [tm EXCEPT ![t] = FALSE]
    /\ UNCHANGED << alloc, mark, stack, tobj, grey >>
    /\ ThreadVarsUnchanged

\* a small object (and a large one since the fix): it exists, in the registers of the thread,
\* before the thread reads whether it is black
AllocObj(t) ==
    /\ Running(t) /\ tpc[t] = "idle"
    /\ Cardinality(stack[t]) < MaxHeld
    /\ \E o \in Obj :
         /\ ~alloc[o]
         /\ alloc' = [alloc EXCEPT ![o] = TRUE]
         /\ mark' = [mark EXCEPT ![o] = FALSE]
         /\ heap' = [heap EXCEPT ![o] = Null]
         /\ stack' = [stack EXCEPT ![t] = @ \cup {o}]
         /\ tobj' = [tobj EXCEPT ![t] = o]
    /\ tpc' = [tpc EXCEPT ![t] = "decide"]
    /\ UNCHANGED << glob, tdst, tval, tm, grey >>
    /\ ThreadVarsUnchanged

\* marking() || sweepGen != sweepGen (): black while the collector marks, or once the end of the
\* marking came across it before the sweep
Decide(t) ==
    /\ Running(t) /\ tpc[t] = "decide"
    /\ mark' = IF marking \/ sweeping THEN [mark EXCEPT ![tobj[t]] = TRUE] ELSE mark
    /\ tpc' = [tpc EXCEPT ![t] = "idle"]
    /\ tobj' = [tobj EXCEPT ![t] = Null]
    /\ UNCHANGED << heap, glob, alloc, stack, tdst, tval, tm, grey >>
    /\ ThreadVarsUnchanged

\* allocLarge before the fix: black read first, the object allocated later, under the heap lock
LargeRead(t) ==
    /\ ~LargeFixed
    /\ Running(t) /\ tpc[t] = "idle"
    /\ Cardinality(stack[t]) < MaxHeld
    /\ tm' = [tm EXCEPT ![t] = marking \/ sweeping]
    /\ tpc' = [tpc EXCEPT ![t] = "large"]
    /\ UNCHANGED << heap, glob, alloc, mark, stack, tdst, tval, tobj, grey >>
    /\ ThreadVarsUnchanged

LargeAlloc(t) ==
    /\ Running(t) /\ tpc[t] = "large"
    /\ \E o \in Obj :
         /\ ~alloc[o]
         /\ alloc' = [alloc EXCEPT ![o] = TRUE]
         /\ mark' = [mark EXCEPT ![o] = tm[t]]
         /\ heap' = [heap EXCEPT ![o] = Null]
         /\ stack' = [stack EXCEPT ![t] = @ \cup {o}]
    /\ tpc' = [tpc EXCEPT ![t] = "idle"]
    /\ tm' = [tm EXCEPT ![t] = FALSE]
    /\ UNCHANGED << glob, tdst, tval, tobj, grey >>
    /\ ThreadVarsUnchanged

InRound == cpc \in {"ack", "scan", "flush"}

Live == {t \in Thr : reg[t] = "running"}

UnchangedButLife == UNCHANGED << heap, glob, alloc, mark, tdst, tval, tm, tobj, marking, sweeping,
                                markStack, req, parked, cpc, pending, found, copy, copied, assisted >>

\* _yrt_i_thread_create: the child gets an object of the parent (the closure, the content of a
\* future), which the parent keeps on its stack until the child registered
Spawn(p, c) ==
    /\ Lifecycle
    /\ Running(p) /\ tpc[p] = "idle"
    /\ reg[c] = "none"
    /\ \E d \in stack[p] : stack' = [stack EXCEPT ![c] = {d}]
    /\ reg' = [reg EXCEPT ![c] = "starting"]
    /\ tpc' = [tpc EXCEPT ![p] = "spawnwait"]
    /\ tchild' = [tchild EXCEPT ![p] = c]
    /\ UNCHANGED << grey, scanned, work >>
    /\ UnchangedButLife

\* _yrt_nh_register_thread: under the lock of the threads, never during a round; the stack of the
\* thread is not scanned in this cycle yet
Register(c) ==
    /\ reg[c] = "starting"
    /\ ~InRound
    /\ reg' = [reg EXCEPT ![c] = "running"]
    /\ scanned' = [scanned EXCEPT ![c] = FALSE]
    /\ UNCHANGED << stack, tpc, grey, work, tchild >>
    /\ UnchangedButLife

\* sem_wait (&start.copied) returns once the child registered
SpawnDone(p) ==
    /\ Running(p) /\ tpc[p] = "spawnwait"
    /\ reg[tchild[p]] # "starting"
    /\ tpc' = [tpc EXCEPT ![p] = "idle"]
    /\ tchild' = [tchild EXCEPT ![p] = Null]
    /\ UNCHANGED << stack, grey, reg, scanned, work >>
    /\ UnchangedButLife

\* _yrt_i_nh_thread_exit: the grey buffer is handed over (flushGrey), then the thread leaves the list
\* under its lock, its stack no longer a root
Exit(t) ==
    /\ Lifecycle
    /\ Running(t) /\ tpc[t] = "idle"
    /\ ~InRound
    /\ work' = IF ExitHandsGrey THEN work \cup grey[t] ELSE work
    /\ grey' = [grey EXCEPT ![t] = {}]
    /\ stack' = [stack EXCEPT ![t] = {}]
    /\ reg' = [reg EXCEPT ![t] = "none"]
    /\ UNCHANGED << tpc, scanned, tchild >>
    /\ UnchangedButLife

\* flushGrey, a barrier or an assist filling the grey buffer: the buffer is handed over
Overflow(t) ==
    /\ Assists
    /\ Running(t) /\ grey[t] # {}
    /\ work' = work \cup grey[t]
    /\ grey' = [grey EXCEPT ![t] = {}]
    /\ UNCHANGED << heap, glob, alloc, mark, stack, tpc, tdst, tval, tm, tobj >>
    /\ UNCHANGED << marking, sweeping, markStack, req, parked, cpc, pending, found >>
    /\ UNCHANGED << reg, scanned, tchild, copy, copied, assisted >>

\* assist, busy from an allocation while the collector marks, once every thread acknowledged it
\* (assistOn): it takes a buffer handed over, counted by `assists`, and scans it in a later step,
\* no handshake parking it in between
AssistTake(t) ==
    /\ Assists
    /\ Running(t) /\ tpc[t] = "idle" /\ marking
    /\ AssistAfterAck => cpc \in {"scan", "drain", "flush"}
    /\ \E o \in work :
         /\ work' = work \ {o}
         /\ tobj' = [tobj EXCEPT ![t] = o]
    /\ tpc' = [tpc EXCEPT ![t] = "assist"]
    /\ assisted' = TRUE
    /\ UNCHANGED << heap, glob, alloc, mark, stack, tdst, tval, tm, grey >>
    /\ UNCHANGED << marking, sweeping, markStack, req, parked, cpc, pending, found >>
    /\ UNCHANGED << reg, scanned, tchild, copy, copied >>

AssistScan(t) ==
    /\ Running(t) /\ tpc[t] = "assist"
    /\ LET G == ShadeSet({heap[tobj[t]]}) IN
       /\ mark' = MarkAll(G)
       /\ grey' = [grey EXCEPT ![t] = @ \cup G]
    /\ tpc' = [tpc EXCEPT ![t] = "idle"]
    /\ tobj' = [tobj EXCEPT ![t] = Null]
    /\ UNCHANGED << heap, glob, alloc, stack, tdst, tval, tm >>
    /\ ThreadVarsUnchanged

ThreadStep(t) ==
    \/ Park(t) \/ Load(t) \/ Drop(t) \/ StoreTest(t) \/ Barrier(t) \/ Store(t)
    \/ AllocObj(t) \/ Decide(t) \/ LargeRead(t) \/ LargeAlloc(t)
    \/ (\E c \in Thr \ {t} : Spawn(t, c)) \/ Register(t) \/ SpawnDone(t) \/ Exit(t)
    \/ Overflow(t) \/ AssistTake(t) \/ AssistScan(t)

-----------------------------------------------------------------------------
(* The collector *)

CollectorThreadVarsUnchanged ==
    /\ UNCHANGED << heap, glob, alloc, stack, tpc, tdst, tval, tm, tobj >>
    /\ UNCHANGED << reg, tchild >>

CopiesUnchanged == UNCHANGED << copy, copied >>

\* reclaim, sweep the rest and clearAllMarks, then _yrt_nh_marking = 1 and the ACK round
Start ==
    /\ cpc = "idle"
    /\ mark' = [o \in Obj |-> FALSE]
    /\ marking' = TRUE
    /\ req' = [t \in Thr |-> t \in Live]
    /\ pending' = Live
    /\ scanned' = [t \in Thr |-> FALSE]
    /\ cpc' = "ack"
    /\ UNCHANGED << grey, sweeping, markStack, parked, found, work >>
    /\ CollectorThreadVarsUnchanged /\ CopiesUnchanged /\ NoAssist

\* the work of a round on a parked thread, served in the order the threads park: its stack is
\* copied (copyThread), to be scanned once it went
CopyStack(t) ==
    /\ copy' = [copy EXCEPT ![t] = Held(t)]
    /\ copied' = copied \cup {t}

Serve(t) ==
    /\ cpc \in {"scan", "flush"}
    /\ t \in pending /\ parked[t]
    /\ markStack' = markStack \cup grey[t]
    /\ grey' = [grey EXCEPT ![t] = {}]
    /\ CASE cpc = "scan" ->
              /\ CopyStack(t)
              /\ scanned' = [scanned EXCEPT ![t] = TRUE]
              /\ UNCHANGED found
         [] cpc = "flush" ->
              \* a thread registered since the SCAN round has its stack copied now, which counts
              \* as something found
              /\ IF scanned[t] \/ ~ScanNewThreads THEN CopiesUnchanged ELSE CopyStack(t)
              /\ found' = (found \/ grey[t] # {} \/ ~scanned[t])
              /\ scanned' = [scanned EXCEPT ![t] = TRUE]
    /\ parked' = [parked EXCEPT ![t] = FALSE]
    /\ req' = [req EXCEPT ![t] = FALSE]
    /\ pending' = pending \ {t}
    /\ UNCHANGED << mark, marking, sweeping, cpc, work >>
    /\ CollectorThreadVarsUnchanged /\ NoAssist

\* scanCopies: the copy of a stack is scanned, its thread running meanwhile
ScanCopy(t) ==
    /\ cpc \in {"scan", "flush"}
    /\ t \in copied
    /\ LET G == ShadeSet(copy[t]) IN
       /\ mark' = MarkAll(G)
       /\ markStack' = markStack \cup G
    /\ copy' = [copy EXCEPT ![t] = {}]
    /\ copied' = copied \ {t}
    /\ UNCHANGED << grey, marking, sweeping, req, parked, cpc, pending, found, scanned, work >>
    /\ CollectorThreadVarsUnchanged /\ NoAssist

\* the end of a round, once the copies of the stacks are scanned
RoundDone ==
    /\ cpc \in {"ack", "scan", "flush"}
    /\ pending = {} /\ copied = {}
    /\ CASE cpc = "ack" ->
              \* scanRoots and the data segments, then the SCAN round
              LET G == ShadeSet({glob}) IN
              /\ mark' = MarkAll(G)
              /\ markStack' = markStack \cup G
              /\ req' = [t \in Thr |-> t \in Live]
              /\ pending' = Live
              /\ cpc' = "scan"
              /\ UNCHANGED << found, marking, sweeping >>
         [] cpc = "scan" ->
              /\ cpc' = "drain"
              /\ UNCHANGED << mark, markStack, req, pending, found, marking, sweeping >>
         [] cpc = "flush" ->
              IF found \/ (CountAssists /\ assisted) \/ markStack # {} \/ work # {}
              THEN /\ cpc' = "drain"
                   /\ UNCHANGED << mark, markStack, req, pending, found, marking, sweeping >>
              ELSE \* the end of the marking: sweepGen += 2, then _yrt_nh_marking = 0
                   /\ sweeping' = TRUE
                   /\ marking' = FALSE
                   /\ cpc' = "sweep"
                   /\ UNCHANGED << mark, markStack, req, pending, found >>
    /\ UNCHANGED << grey, parked, scanned, work >>
    /\ CollectorThreadVarsUnchanged /\ CopiesUnchanged /\ NoAssist

\* scan an object of the mark stack
DrainOne ==
    /\ cpc = "drain"
    /\ \E o \in markStack :
         LET G == ShadeSet({heap[o]}) IN
         /\ mark' = MarkAll(G)
         /\ markStack' = (markStack \ {o}) \cup G
    /\ UNCHANGED << grey, marking, sweeping, req, parked, cpc, pending, found, scanned, work >>
    /\ CollectorThreadVarsUnchanged /\ CopiesUnchanged /\ NoAssist

\* takeWork: the buffers handed over go to the mark stack
TakeWork ==
    /\ cpc = "drain"
    /\ work # {}
    /\ markStack' = markStack \cup work
    /\ work' = {}
    /\ UNCHANGED << mark, grey, marking, sweeping, req, parked, cpc, pending, found, scanned >>
    /\ CollectorThreadVarsUnchanged /\ CopiesUnchanged /\ NoAssist

\* publishWork: an assist found no buffer, the collector hands objects of its mark stack over
Share ==
    /\ Assists
    /\ cpc = "drain"
    /\ \E o \in markStack :
         /\ markStack' = markStack \ {o}
         /\ work' = work \cup {o}
    /\ UNCHANGED << mark, grey, marking, sweeping, req, parked, cpc, pending, found, scanned >>
    /\ CollectorThreadVarsUnchanged /\ CopiesUnchanged /\ NoAssist

\* the mark stack is empty and no buffer is handed over: terminate reads `assists`, then a FLUSH round
DrainDone ==
    /\ cpc = "drain"
    /\ markStack = {} /\ work = {}
    /\ req' = [t \in Thr |-> t \in Live]
    /\ pending' = Live
    /\ found' = FALSE
    /\ assisted' = FALSE
    /\ cpc' = "flush"
    /\ UNCHANGED << mark, markStack, grey, marking, sweeping, parked, scanned, work >>
    /\ CollectorThreadVarsUnchanged /\ CopiesUnchanged

\* sweepAll: never while a thread is between an allocation and its decision, which holds the span
\* in its cache (a small object) or the lock of the heap (a large one)
Sweep ==
    /\ cpc = "sweep"
    /\ \A t \in Thr : tpc[t] # "decide"
    /\ alloc' = [o \in Obj |-> alloc[o] /\ mark[o]]
    /\ heap' = [o \in Obj |-> IF alloc[o] /\ mark[o] THEN heap[o] ELSE Null]
    /\ sweeping' = FALSE
    /\ cpc' = "idle"
    /\ mark' = [o \in Obj |-> FALSE]
    /\ UNCHANGED << glob, stack, tpc, tdst, tval, tm, tobj, grey, marking, markStack,
                    req, parked, pending, found >>
    /\ LifeUnchanged

CollectorStep ==
    Start \/ (\E t \in Thr : Serve(t) \/ ScanCopy(t)) \/ RoundDone \/ DrainOne \/ TakeWork
    \/ Share \/ DrainDone \/ Sweep

-----------------------------------------------------------------------------

Init ==
    /\ heap = [o \in Obj |-> Null]
    /\ glob = Null
    /\ alloc = [o \in Obj |-> FALSE]
    /\ mark = [o \in Obj |-> FALSE]
    /\ stack = [t \in Thr |-> {}]
    /\ tpc = [t \in Thr |-> "idle"]
    /\ tdst = [t \in Thr |-> Null]
    /\ tval = [t \in Thr |-> Null]
    /\ tm = [t \in Thr |-> FALSE]
    /\ tobj = [t \in Thr |-> Null]
    /\ grey = [t \in Thr |-> {}]
    /\ marking = FALSE
    /\ sweeping = FALSE
    /\ markStack = {}
    /\ req = [t \in Thr |-> FALSE]
    /\ parked = [t \in Thr |-> FALSE]
    /\ cpc = "idle"
    /\ pending = {}
    /\ found = FALSE
    \* one thread runs from the start, the others are spawned (all of them without Lifecycle)
    /\ \E t0 \in Thr : reg = [t \in Thr |-> IF ~Lifecycle \/ t = t0 THEN "running" ELSE "none"]
    /\ scanned = [t \in Thr |-> FALSE]
    /\ work = {}
    /\ tchild = [t \in Thr |-> Null]
    /\ copy = [t \in Thr |-> {}]
    /\ copied = {}
    /\ assisted = FALSE

Next == CollectorStep \/ (\E t \in Thr : ThreadStep(t))

Spec == Init /\ [][Next]_vars

Symmetry == Permutations(Obj) \cup Permutations(Thr)

=============================================================================
