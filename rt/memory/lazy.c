#include <rt/memory/lazy.h>

#include <rt/utils/gc.h>

void* _yrt_call_lazy (struct _yrt_lazy_value * value) {
    if (!value-> set) {
        void * data = GC_malloc (value-> size);
        value-> closure.func (value-> closure.closure, data); // value-> data);
        value-> data = data;
        value-> set = 1;
    }

    return value-> data;
}
