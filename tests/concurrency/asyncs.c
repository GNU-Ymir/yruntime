#include <stdatomic.h>
#include <unistd.h>

#include <rt/utils/gc.h>
#include <rt/concurrency/async.h>
#include <rt/concurrency/future.h>
#include <rt/concurrency/thread.h>

typedef uint8_t (*resume_t) (void *, void *, _yrt_delegate_t);

static void call_waker (_yrt_delegate_t waker) {
  ((void (*) (void*)) waker.func) (waker.closure);
}

// A waker counting its calls and posting a semaphore at each of them
typedef struct counting_waker_t {
  sem_t sem;
  atomic_uint calls;
} counting_waker_t;

static void counting_wake (void * closure) {
  counting_waker_t * w = closure;
  atomic_fetch_add (&w-> calls, 1);
  sem_post (&w-> sem);
}

static _yrt_delegate_t counting_waker (counting_waker_t * w) {
  sem_init (&w-> sem, 0, 0);
  atomic_init (&w-> calls, 0);

  _yrt_delegate_t waker;
  waker.closure = w;
  waker.func = (void*) &counting_wake;

  return waker;
}

// The frame of the hand-written resume functions handed to _yrt_async_run
typedef struct test_frame_t {
  uint32_t resumes;
  atomic_uint woken;
  uint8_t spurious;
  _yrt_delegate_t waker;
  uint8_t hasThread;
  _yrt_thread_t thread;
  uint8_t hasFuture;
  _yrt_future_t future;
} test_frame_t;

static _yrt_delegate_t test_handle (resume_t resume) {
  _yrt_delegate_t handle;
  handle.closure = _yrt_i_gc_alloc (sizeof (test_frame_t));
  handle.func = (void*) resume;

  return handle;
}

// Pending twice, calling the waker before returning, then completed with 42
static uint8_t inline_resume (void * frame, void * out, _yrt_delegate_t waker) {
  test_frame_t * f = frame;
  f-> resumes += 1;
  if (f-> resumes < 3) {
    call_waker (waker);
    return 1;
  }

  *((int32_t*) out) = 42;
  return 0;
}

static void * wake_later (void * data) {
  test_frame_t * f = data;
  usleep (20000);
  atomic_store (&f-> woken, 1);
  call_waker (f-> waker);

  return NULL;
}

// Pending once, the waker being called 20ms later by another thread, then completed with 7
static uint8_t threaded_resume (void * frame, void * out, _yrt_delegate_t waker) {
  test_frame_t * f = frame;
  f-> resumes += 1;
  if (f-> resumes == 1) {
    _yrt_i_gc_store (&f-> waker, &waker, sizeof (waker));
    f-> hasThread = 1;
    _yrt_i_thread_create (&f-> thread, NULL, &wake_later, f);
    return 1;
  }

  if (!atomic_load (&f-> woken)) f-> spurious = 1;
  *((int32_t*) out) = 7;
  return 0;
}

static _yrt_future_t spawn_sleeping (uint32_t micros);

// Spawns a future finishing 20ms later at its first resume, pending until it finished, then
// completed with its value
static uint8_t awaiting_resume (void * frame, void * out, _yrt_delegate_t waker) {
  test_frame_t * f = frame;
  f-> resumes += 1;
  if (f-> resumes == 1) {
    _yrt_future_t future = spawn_sleeping (20000);
    f-> hasFuture = 1;
    _yrt_i_gc_store (&f-> future, &future, sizeof (future));
  }

  if (!_yrt_future_await (f-> future, waker)) return 1;

  *((int32_t*) out) = *((int32_t*) _yrt_wait_future (f-> future));
  return 0;
}

_yrt_delegate_t _yrt_test_async_inline () {
  return test_handle (&inline_resume);
}

_yrt_delegate_t _yrt_test_async_threaded () {
  return test_handle (&threaded_resume);
}

_yrt_delegate_t _yrt_test_async_awaiting () {
  return test_handle (&awaiting_resume);
}

// Releases the threads a handle started, and calls its stored waker once more, after the run returned
// @returns: the number of resumes, in `spurious` whether it was resumed before its thread woke it
uint32_t _yrt_test_async_finish (_yrt_delegate_t handle, uint8_t * spurious) {
  test_frame_t * f = handle.closure;
  if (f-> hasThread) _yrt_i_thread_join (f-> thread, NULL);
  if (f-> hasFuture) _yrt_i_thread_join (f-> future.id, NULL);
  if (f-> waker.func != NULL) call_waker (f-> waker);

  *spurious = f-> spurious;
  return f-> resumes;
}

static void sleeping_value (void * closure, void * ret) {
  usleep (*((uint32_t*) closure));
  *((int32_t*) ret) = 7;
}

static _yrt_future_t spawn_sleeping (uint32_t micros) {
  uint32_t * duration = _yrt_i_gc_alloc_atomic (sizeof (uint32_t));
  *duration = micros;

  _yrt_lazy_closure_t closure = { duration, &sleeping_value };
  return _yrt_spawn_future (closure, sizeof (int32_t));
}

// Awaits a future finishing 20ms later, then once more after it finished
// @returns: 0 on success, the number of the first failing check otherwise
uint32_t _yrt_test_future_await_wakes () {
  counting_waker_t w, late;
  _yrt_future_t f = spawn_sleeping (20000);

  if (_yrt_future_await (f, counting_waker (&w)) != 0) return 1;
  if (!_yrt_thread_sem_wait_timeout (&w.sem, 5, 0)) return 2;
  if (!_yrt_check_finished_future (f)) return 3;
  if (_yrt_future_await (f, counting_waker (&late)) != 1) return 4;
  if (*((int32_t*) _yrt_wait_future (f)) != 7) return 5;

  _yrt_i_thread_join (f.id, NULL);
  if (atomic_load (&w.calls) != 1) return 6;
  if (atomic_load (&late.calls) != 0) return 7;

  return 0;
}

// Awaits a future twice before it finishes, only the second waker is called
// @returns: 0 on success, the number of the first failing check otherwise
uint32_t _yrt_test_future_await_replaces () {
  counting_waker_t first, second;
  _yrt_future_t f = spawn_sleeping (20000);

  if (_yrt_future_await (f, counting_waker (&first)) != 0) return 1;
  if (_yrt_future_await (f, counting_waker (&second)) != 0) return 2;

  _yrt_i_thread_join (f.id, NULL);
  if (atomic_load (&first.calls) != 0) return 3;
  if (atomic_load (&second.calls) != 1) return 4;

  return 0;
}

static void nothing (void * closure) {
  (void) closure;
}

// Awaits `nb` futures finishing right away, racing their thread to the registration of the waker
// @returns: the number of awaits whose waker was registered and never called
uint32_t _yrt_test_future_await_race (uint32_t nb) {
  uint32_t lost = 0;

  for (uint32_t i = 0 ; i < nb ; i++) {
    counting_waker_t w;
    _yrt_lazy_closure_t closure = { NULL, (void (*) (void*, void*)) &nothing };
    _yrt_future_t f = _yrt_spawn_future (closure, 0);

    if (_yrt_future_await (f, counting_waker (&w)) == 0) {
      if (!_yrt_thread_sem_wait_timeout (&w.sem, 5, 0)) lost += 1;
    }

    _yrt_i_thread_join (f.id, NULL);
  }

  return lost;
}
