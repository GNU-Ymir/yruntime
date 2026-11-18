#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <stdint.h>

/**
 * @returns: the value at `p`, read with acquire ordering
 */
uint32_t _yrt_atomic_load_u32 (uint32_t * p);
uint64_t _yrt_atomic_load_u64 (uint64_t * p);

/**
 * Write `value` at `p` with release ordering
 */
void _yrt_atomic_store_u32 (uint32_t * p, uint32_t value);
void _yrt_atomic_store_u64 (uint64_t * p, uint64_t value);

/**
 * Replace the value at `p` by `desired` if it is `*expected`, with acquire-release ordering, or write the value
 * found in `*expected` with acquire ordering otherwise
 * @returns: 1 iif the value was replaced
 */
uint8_t _yrt_atomic_cas_u32 (uint32_t * p, uint32_t * expected, uint32_t desired);
uint8_t _yrt_atomic_cas_u64 (uint64_t * p, uint64_t * expected, uint64_t desired);

/**
 * Add `value` to the value at `p`, with acquire-release ordering
 * @returns: the value after the addition
 */
uint32_t _yrt_atomic_add_u32 (uint32_t * p, uint32_t value);
uint64_t _yrt_atomic_add_u64 (uint64_t * p, uint64_t value);

#endif // ATOMIC_H_
