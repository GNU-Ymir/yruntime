#include <rt/memory/classes.h>

void _yrt_i_destruct_class (GC_PTR obj, GC_PTR x) {
    void* vtable = *((void**)obj); // vtable
    // the second element is the destructor
    void(*__dtor) (void*) = *(void(**)(void*)) ((void**) vtable + 1);
    (*__dtor) (obj);
}

void* _yrt_alloc_class_pending (void* vtable) {
    // the first element stored in the vtable is the typeinfo
    _yrt_type_info *ti = *((_yrt_type_info**) vtable);

    void* cl = GC_MALLOC (ti-> size);
    *((void**)cl) = vtable; // vtable
    *((void**)cl+1) = NULL; // monitor

    return cl;
}

void _yrt_commit_class (void* cl) {
    void* vtable = *((void**)cl);

    // the second element is the destructor
    void(*__dtor) () = *(void(**)()) ((void**) vtable + 1);

    // No need to finalize classes without destructors
    if ((*__dtor) != NULL) {
        // no_order: a class reachable from a reference cycle must still be
        // destructed. Ordered finalization refuses to finalize cycles (warning
        // "Finalization cycle involving 0x...", and the __dtor never runs).
        GC_register_finalizer_no_order (cl, _yrt_i_destruct_class, NULL, NULL, NULL);
    }
}

void* _yrt_alloc_class (void* vtable) {
    void* cl = _yrt_alloc_class_pending (vtable);
    _yrt_commit_class (cl);

    return cl;
}
