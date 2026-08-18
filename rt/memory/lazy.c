#include <rt/memory/lazy.h>

void* _yrt_call_lazy (_yrt_lazy_value_t * value) {
    void* data = &value-> data;
    if (!value-> set) {
        value-> closure.func (value-> closure.closure, data);
        value-> set = 1;
    }

    return data;
}
