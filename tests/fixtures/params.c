// Parameterized test providers for tests/testrt_params.yr, with the generator ABI gyc emits:
// a (closure, func) pair, func storing the next parameter set in its out parameter.

#include <stdint.h>
#include <stddef.h>

typedef struct { void * closure; uint8_t (*func) (void *, void **); } testrt_gen_t;

static int64_t values [] = { 0, 10, 20 };
static int64_t next = 0;
static int64_t limit = 0;
static int64_t starts = 0;
static int64_t yielded = 0;

static uint8_t resume (void * closure, void ** out) {
  (void) closure;
  if (next >= limit) return 0;

  *out = &values [next++];
  yielded++;
  return 1;
}

static testrt_gen_t start (int64_t count) {
  next = 0;
  limit = count;
  starts++;

  testrt_gen_t gen = { NULL, resume };
  return gen;
}

// Yields 0, 10 and 20
testrt_gen_t testrt_params_counting () {
  return start (3);
}

// Yields nothing
testrt_gen_t testrt_params_empty () {
  return start (0);
}

// Yields 0, 10 and 20 the first time it is started, only 0 afterwards
testrt_gen_t testrt_params_shrinking () {
  return start (starts == 0 ? 3 : 1);
}

int64_t testrt_params_value (void * set) {
  return *(int64_t *) set;
}

int64_t testrt_params_starts () {
  return starts;
}

int64_t testrt_params_yielded () {
  return yielded;
}

void testrt_params_reset () {
  next = 0;
  limit = 0;
  starts = 0;
  yielded = 0;
}
