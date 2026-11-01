#include <rt/memory/classes.h>

void _yrt_i_destruct_class (void * obj, void * x) {
    (void) x;
    void* vtable = *((void**)obj); // vtable
    // the second element is the destructor
    void(*__dtor) (void*) = *(void(**)(void*)) ((void**) vtable + 1);
    (*__dtor) (obj);
}

void* _yrt_alloc_class_pending (void* vtable) {
    // the first element stored in the vtable is the typeinfo
    _yrt_type_info *ti = *((_yrt_type_info**) vtable);

    // the vtable is in the static data, the barrier would have nothing to shade
    void* cl = _yrt_i_gc_alloc (ti-> size);
    *((void**)cl) = vtable;

    return cl;
}

void _yrt_commit_class (void* cl) {
    void* vtable = *((void**)cl);

    // the second element is the destructor
    void(*__dtor) () = *(void(**)()) ((void**) vtable + 1);

    // No need to finalize classes without destructors
    if ((*__dtor) != NULL) {
        // finalizers are unordered: a class reachable from a reference cycle is still destructed
        _yrt_nh_register_finalizer (cl, (void*) &_yrt_i_destruct_class);
    }
}

void* _yrt_alloc_class (void* vtable) {
    void* cl = _yrt_alloc_class_pending (vtable);
    _yrt_commit_class (cl);

    return cl;
}
