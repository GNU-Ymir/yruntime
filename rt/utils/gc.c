#ifndef _GNU_SOURCE
#  define _GNU_SOURCE // dl_iterate_phdr
#endif

#include <rt/utils/gc.h>

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
// x86 uses TLS variant II: the static TLS block sits immediately below the thread pointer, and
// __builtin_thread_pointer gives us that pointer. Other targets lay TLS out differently(variant
// I puts it above the thread control block) and are left unhandled rather than guessed at.
#  define _YRT_TLS_ROOTS 1
#  include <link.h>
#endif

void _yrt_disable_GC () {
    GC_disable ();
}

void _yrt_enable_GC () {
    GC_enable ();
}

uint8_t _yrt_is_GC_enabled () {
    return !GC_is_disabled ();
}

#ifdef _YRT_TLS_ROOTS

// The size of the main executable's PT_TLS segment, rounded up to its alignment: the amount of
// static TLS every thread gets a private copy of. Read once from the program headers, it cannot
// change afterwards.
static size_t __TLS_BLOCK_SIZE__ = 0;
static int __TLS_BLOCK_SIZE_READ__ = 0;

static int _yrt_read_tls_phdr (struct dl_phdr_info * info, size_t size, void * data) {
    (void) size;

    // The main executable is the only object with an empty name. Shared objects are skipped on
    // purpose: their TLS is allocated on demand by __tls_get_addr and is not part of this block.
    if (info-> dlpi_name != NULL && info-> dlpi_name [0] != '\0') return 0;

    for (int i = 0 ; i < info-> dlpi_phnum ; i++) {
        const ElfW (Phdr) * ph = &info-> dlpi_phdr [i];
        if (ph-> p_type == PT_TLS) {
            size_t align = ph-> p_align != 0 ? ph-> p_align : 1;
            *((size_t*) data) = (ph-> p_memsz + align - 1) & ~(align - 1);
        }
    }

    return 1; // the main object was the one we wanted, stop walking
}

/**
 * Compute the bounds of the calling thread's static TLS block
 * @info: primed by _yrt_gc_add_tls_roots from _yrt_init_runtime, while still single threaded
 * */
static void _yrt_tls_bounds (char ** lo, char ** hi) {
    if (!__TLS_BLOCK_SIZE_READ__) {
        dl_iterate_phdr (&_yrt_read_tls_phdr, &__TLS_BLOCK_SIZE__);
        __TLS_BLOCK_SIZE_READ__ = 1;
    }

    char * tp = (char*) __builtin_thread_pointer ();
    *hi = tp;
    *lo = tp - __TLS_BLOCK_SIZE__;
}

void _yrt_gc_add_tls_roots () {
    char * lo, * hi;
    _yrt_tls_bounds (&lo, &hi);

    if (lo != hi) GC_add_roots (lo, hi);
}

void _yrt_gc_remove_tls_roots () {
    char * lo, * hi;
    _yrt_tls_bounds (&lo, &hi);

    if (lo != hi) GC_remove_roots (lo, hi);
}

#else

void _yrt_gc_add_tls_roots () {}
void _yrt_gc_remove_tls_roots () {}

#endif
