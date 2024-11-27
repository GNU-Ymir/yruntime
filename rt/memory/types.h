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

typedef struct _yrt_slice_blk_info_ {
	uint64_t len;
	uint64_t cap;
} _yrt_slice_blk_info_;

typedef struct _yrt_slice_ {
    uint64_t len;
    void * data;
    _yrt_slice_blk_info_ * blk_info;
} _yrt_slice_;

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          COPY MAP          ====================================
 * ====================================================================================================
 * ====================================================================================================
 */

typedef struct _yrt_dcopy_map_node_ {
    unsigned long len;
    unsigned long used;
    void** from;
    void** to;
}  _yrt_dcopy_map_node_ ;

typedef struct _yrt_map_entry_ {
    uint64_t hash;
    struct _yrt_map_entry_ * next;
} _yrt_map_entry_;


// The map info is a constant placed in the text
typedef struct _yrt_map_info_ {
    uint8_t (*cmp) (uint8_t*, uint8_t*);
    uint64_t (*hash) (uint8_t*);
    uint64_t keySize;
    uint64_t valueSize;
} _yrt_map_info_;

typedef struct _yrt_map_ {
    _yrt_map_info_ * minfo;
    _yrt_map_entry_ ** data;
    uint64_t allocLen; // the length of the data array
    uint64_t loaded; // The number of entries used
    uint64_t len; // The number of elements in the map
} _yrt_map_;

typedef struct _yrt_map_iterator_ {
    _yrt_map_ * mp;
    _yrt_map_entry_ * current;
    uint64_t rootIndex;
    uint8_t notEnd;
} _yrt_map_iterator_;

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ======================================          LAZY          ======================================
 * ====================================================================================================
 * ====================================================================================================
 */

typedef struct _yrt_lazy_closure_ {
    void * closure;
    void (*func) (void* closure, void*ret);
}  _yrt_lazy_closure_ ;

typedef struct _yrt_lazy_value_ {
    unsigned char set;
    void* data;
    uint32_t size;
    struct _yrt_lazy_closure_ closure;
} _yrt_lazy_value_;

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
    _yrt_slice_ inner;
    _yrt_slice_ name;
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


#endif // TYPES_H_
