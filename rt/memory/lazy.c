#include <rt/memory/lazy.h>

#include <sched.h>

#define LAZY_UNSET 0
#define LAZY_SET 1
#define LAZY_INITIALIZING 2

typedef struct _yrt_i_lazy_entry_t {
    _yrt_lazy_value_t * value;
    struct _yrt_i_lazy_entry_t * next;
} _yrt_i_lazy_entry_t;

static __thread _yrt_i_lazy_entry_t * __YRT_LAZY_INITIALIZING__ = NULL;

static int _yrt_i_lazy_is_mine (_yrt_lazy_value_t * value) {
    for (_yrt_i_lazy_entry_t * it = __YRT_LAZY_INITIALIZING__ ; it != NULL ; it = it-> next) {
        if (it-> value == value) return 1;
    }

    return 0;
}

static void _yrt_i_lazy_release (_yrt_i_lazy_entry_t * entry) {
    __YRT_LAZY_INITIALIZING__ = entry-> next;
    if (entry-> value != NULL) {
        __atomic_store_n (&entry-> value-> set, LAZY_UNSET, __ATOMIC_RELEASE);
    }
}

void* _yrt_call_lazy (_yrt_lazy_value_t * value) {
    void* data = &value-> data;
    for (;;) {
        unsigned char state = __atomic_load_n (&value-> set, __ATOMIC_ACQUIRE);
        if (state == LAZY_SET) return data;

        if (state == LAZY_UNSET &&
            __atomic_compare_exchange_n (&value-> set, &state, LAZY_INITIALIZING, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
            _yrt_i_lazy_entry_t entry __attribute__ ((cleanup (_yrt_i_lazy_release))) = { value, __YRT_LAZY_INITIALIZING__ };
            __YRT_LAZY_INITIALIZING__ = &entry;

            value-> closure.func (value-> closure.closure, data);
            __atomic_store_n (&value-> set, LAZY_SET, __ATOMIC_RELEASE);
            entry.value = NULL;
            return data;
        }

        if (state == LAZY_INITIALIZING && _yrt_i_lazy_is_mine (value)) {
            value-> closure.func (value-> closure.closure, data);
            return data;
        }

        sched_yield ();
    }
}
