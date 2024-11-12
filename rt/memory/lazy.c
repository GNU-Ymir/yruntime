#include <rt/memory/lazy.h>

#include <rt/utils/gc.h>

void* _yrt_call_lazy (_yrt_lazy_value_ * value) {
    if (!value-> set) {
        void * data = GC_malloc (value-> size);
        value-> closure.func (value-> closure.closure, data); // value-> data);
        value-> data = data;
        value-> set = 1;
    }

    return value-> data;
}
