#include <rt/memory/map.h>

#include <rt/utils/gc.h>
#include <rt/thread.h>


pthread_mutex_t _yrt_dcopy_map_mutex;
struct _yrt_dcopy_map_node _yrt_dcopy_head = {.len = 0, .used = 0, .from = NULL, .to = NULL};

char _yrt_dcopy_map_is_started () {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    char ret = (_yrt_dcopy_head.len != 0);
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
    return ret;
}

void _yrt_purge_dcopy_map () {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    _yrt_dcopy_head.len = 0;
    _yrt_dcopy_head.used = 0;
    _yrt_dcopy_head.from = NULL;
    _yrt_dcopy_head.to = NULL;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void _yrt_insert_dcopy_map (void * data, void * to) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    uint64_t index = _yrt_dcopy_head.used;
    if (index >= _yrt_dcopy_head.len) _yrt_dcopy_map_grow ();

    _yrt_dcopy_head.from [index] = data;
    _yrt_dcopy_head.to [index] = to;
    _yrt_dcopy_head.used += 1;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void* _yrt_find_dcopy_map (void * data) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    for (uint64_t i = 0 ; i < _yrt_dcopy_head.used ; i++) {
        if (_yrt_dcopy_head.from [i] == data) {
            _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
            return _yrt_dcopy_head.to [i];
        }
    }
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);

    return NULL;
}


void _yrt_dcopy_map_grow () {
    uint64_t len = _yrt_dcopy_head.len;
    if (len == 0) len = 10;
    else len = len * 2;

    void** from_res = (void**) GC_malloc (len * sizeof (void*));
    void** to_res = (void**) GC_malloc (len * sizeof (void*));

    for (uint64_t i = 0 ; i < _yrt_dcopy_head.len ; i++) {
        from_res [i] = _yrt_dcopy_head.from [i];
        to_res [i] = _yrt_dcopy_head.to [i];
    }

    _yrt_dcopy_head.len = len;
    _yrt_dcopy_head.from = from_res;
    _yrt_dcopy_head.to = to_res;
}
