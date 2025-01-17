#ifndef TYPES_H_
#define TYPES_H_

#include <stdint.h>

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          ARRAYS/SLICES          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

typedef struct _yrt_slice_blk_info_t {
	uint64_t len;
	uint64_t cap;
} _yrt_slice_blk_info_t;

typedef struct _yrt_slice_t {
    uint64_t len;
    void * data;
    _yrt_slice_blk_info_t * blk_info;
} _yrt_slice_t;

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          COPY MAP          ====================================
 * ====================================================================================================
 * ====================================================================================================
 */

typedef struct _yrt_dcopy_map_node_t {
    unsigned long len;
    unsigned long used;
    void** from;
    void** to;
}  _yrt_dcopy_map_node_t ;

typedef struct _yrt_map_entry_t {
    uint64_t hash;
    struct _yrt_map_entry_t * next;
} _yrt_map_entry_t;


// The map info is a constant placed in the text
typedef struct _yrt_map_info_t {
    uint8_t (*cmp) (uint8_t*, uint8_t*);
    uint64_t (*hash) (uint8_t*);
    uint64_t keySize;
    uint64_t valueSize;
} _yrt_map_info_t;

typedef struct _yrt_map_entry_slice_t {
    _yrt_map_entry_t ** entries;
    uint64_t len;
} _yrt_map_entry_slice_t;

typedef struct _yrt_map_t {
    _yrt_map_info_t * minfo;
    _yrt_map_entry_slice_t * data;
    uint64_t loaded; // The number of entries used
    uint64_t len; // The number of elements in the map
} _yrt_map_t;

typedef struct _yrt_map_iterator_t {
    _yrt_map_t * mp;
    _yrt_map_entry_t * current;
    uint64_t rootIndex;
    uint8_t notEnd;
} _yrt_map_iterator_t;

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ======================================          LAZY          ======================================
 * ====================================================================================================
 * ====================================================================================================
 */

typedef struct _yrt_lazy_closure_t {
    void * closure;
    void (*func) (void* closure, void*ret);
}  _yrt_lazy_closure_t ;

typedef struct _yrt_lazy_value_t {
    unsigned char set;
    void* data;
    uint32_t size;
    struct _yrt_lazy_closure_t closure;
} _yrt_lazy_value_t;


/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          TYPEINFO          ====================================
 * ====================================================================================================
 * ====================================================================================================
 */

typedef struct _yrt_type_info {
    unsigned int id;
    unsigned long long size;
    _yrt_slice_t inner;
    _yrt_slice_t name;
}  _yrt_type_info;

enum _yrt_type_ids {
    ARRAY        = 1,
    BOOL_        = 2,
    CHAR_        = 3,
    CLOSURE      = 4,
    FLOAT_       = 5,
    FUNC_PTR     = 6,
    SIGNED_INT   = 7,
    UNSIGNED_INT = 8,
    POINTER      = 9,
    SLICE        = 10,
    STRUCT       = 11,
    TUPLE        = 12,
    OBJECT       = 13,
    VOID_         = 14,
};

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          REFLECT          =====================================
 * ====================================================================================================
 * ====================================================================================================
 */

enum _yrt_reflect_type {
    FUNCTION = 1,
    VTABLE   = 2,
    NONE     = 3,
};

typedef struct _yrt_reflect_symbol_t {
    enum _yrt_reflect_type type;
    void* ptr;
    uint64_t size;
    struct _yrt_slice_t name;
} _yrt_reflect_symbol_t;


#endif // TYPES_H_
