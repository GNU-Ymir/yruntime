#include "atomic.h"

uint32_t _yrt_atomic_load_u32 (uint32_t * p) {
    return __atomic_load_n (p, __ATOMIC_ACQUIRE);
}

uint64_t _yrt_atomic_load_u64 (uint64_t * p) {
    return __atomic_load_n (p, __ATOMIC_ACQUIRE);
}

void _yrt_atomic_store_u32 (uint32_t * p, uint32_t value) {
    __atomic_store_n (p, value, __ATOMIC_RELEASE);
}

void _yrt_atomic_store_u64 (uint64_t * p, uint64_t value) {
    __atomic_store_n (p, value, __ATOMIC_RELEASE);
}

uint8_t _yrt_atomic_cas_u32 (uint32_t * p, uint32_t * expected, uint32_t desired) {
    return __atomic_compare_exchange_n (p, expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

uint8_t _yrt_atomic_cas_u64 (uint64_t * p, uint64_t * expected, uint64_t desired) {
    return __atomic_compare_exchange_n (p, expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

uint32_t _yrt_atomic_add_u32 (uint32_t * p, uint32_t value) {
    return __atomic_add_fetch (p, value, __ATOMIC_ACQ_REL);
}

uint64_t _yrt_atomic_add_u64 (uint64_t * p, uint64_t value) {
    return __atomic_add_fetch (p, value, __ATOMIC_ACQ_REL);
}
