#include <stdatomic.h>
#include <unistd.h>

#include <rt/utils/gc.h>
#include <rt/concurrency/async.h>
#include <rt/concurrency/thread.h>

typedef uint8_t (*resume_t) (void *, void *, _yrt_delegate_t);

static void call_waker (_yrt_delegate_t waker) {
  ((void (*) (void*)) waker.func) (waker.closure);
}

// The frame of the hand-written resume functions handed to _yrt_async_run
typedef struct test_frame_t {
  uint32_t resumes;
  atomic_uint woken;
  uint8_t spurious;
  _yrt_delegate_t waker;
  uint8_t hasThread;
  _yrt_thread_t thread;
} test_frame_t;

static _yrt_delegate_t test_handle (resume_t resume) {
  _yrt_delegate_t handle;
  handle.closure = GC_malloc (sizeof (test_frame_t));
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
    f-> waker = waker;
    f-> hasThread = 1;
    _yrt_i_thread_create (&f-> thread, NULL, &wake_later, f);
    return 1;
  }

  if (!atomic_load (&f-> woken)) f-> spurious = 1;
  *((int32_t*) out) = 7;
  return 0;
}

_yrt_delegate_t _yrt_test_async_inline () {
  return test_handle (&inline_resume);
}

_yrt_delegate_t _yrt_test_async_threaded () {
  return test_handle (&threaded_resume);
}

// Releases the threads a handle started, and calls its stored waker once more, after the run returned
// @returns: the number of resumes, in `spurious` whether it was resumed before its thread woke it
uint32_t _yrt_test_async_finish (_yrt_delegate_t handle, uint8_t * spurious) {
  test_frame_t * f = handle.closure;
  if (f-> hasThread) _yrt_i_thread_join (f-> thread, NULL);
  if (f-> waker.func != NULL) call_waker (f-> waker);

  *spurious = f-> spurious;
  return f-> resumes;
}
