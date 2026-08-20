#include <rt/memory/map.h>
#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <rt/concurrency/thread.h>

#include <string.h>
#include <stdlib.h>

#define MAP_MAX_LOADED_FACTOR 75
#define MAP_MIN_LOADED_FACTOR 40

// Entry count of the first slab allocated for a map (see _map_entry_alloc). Each
// subsequent slab for the same map doubles in size, up to MAP_SLAB_MAX_BYTES, so a map
// that only ever holds a handful of entries (e.g. one JSON object) does not pay for a
// slab sized for a map that ends up holding millions.
#define MAP_SLAB_INITIAL_ENTRIES 1
#define MAP_SLAB_MAX_BYTES 4096

// All entry nodes are aligned on this boundary within a slab, so hash/next/key/value
// fields keep the same alignment they would get from a standalone GC_malloc
#define MAP_ENTRY_ALIGN (sizeof (void*))

// cap is always 0 or a power of two (see _yrt_map_fit/_next_pow2), so bucket index
// computation can use a mask instead of a division
#define MAP_BUCKET_INDEX(hash, cap) ((hash) & ((cap) - 1))

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ================================          DEFAULT MAP IMPL          ================================
 * ====================================================================================================
 * ====================================================================================================
 */

void _yrt_map_empty (_yrt_map_t * mp, _yrt_map_info_t * info) {
    mp-> data = (_yrt_map_content_t*) GC_malloc (sizeof (_yrt_map_content_t));
    mp-> data-> len = 0;
    mp-> data-> minfo = info;
    mp-> data-> loaded = 0;
    mp-> data-> cap = 0;
    mp-> data-> entries = NULL;
    mp-> data-> slabCur = NULL;
    mp-> data-> slabEnd = NULL;
    mp-> data-> slabSize = 0;
}

/**
 * Carve `nodeSize` bytes for a new entry out of `data`'s bump-allocated slab,
 * allocating a fresh slab first if the current one does not have enough room left.
 * This turns what used to be one GC_malloc per entry into one GC_malloc per slab,
 * with slabs growing geometrically (like a vector) instead of a single fixed size,
 * so a map with few entries does not over-allocate.
 */
static uint8_t * _map_entry_alloc (_yrt_map_content_t * data, uint64_t nodeSize) {
    uint64_t stride = (nodeSize + MAP_ENTRY_ALIGN - 1) & ~(MAP_ENTRY_ALIGN - 1);

    if (data-> slabCur == NULL || (uint64_t) (data-> slabEnd - data-> slabCur) < stride) {
        uint64_t slabSize = data-> slabSize == 0 ? stride * MAP_SLAB_INITIAL_ENTRIES : data-> slabSize * 2;
        if (slabSize > MAP_SLAB_MAX_BYTES && stride <= MAP_SLAB_MAX_BYTES) {
            slabSize = MAP_SLAB_MAX_BYTES;
        }
        if (slabSize < stride) {
            slabSize = stride;
        }
        slabSize = ((slabSize + stride - 1) / stride) * stride;

        data-> slabCur = (uint8_t *) GC_malloc (slabSize);
        data-> slabEnd = data-> slabCur + slabSize;
        data-> slabSize = slabSize;
    }

    uint8_t * result = data-> slabCur;
    data-> slabCur += stride;

    return result;
}

void _yrt_dup_map (_yrt_map_t * result, _yrt_map_info_t * info, _yrt_map_t * old) {
    _yrt_map_empty (result, info);
    if (old == NULL || old-> data == NULL || old-> data-> len == 0 || old-> data-> cap == 0) {
        return;
    }

    result-> data-> cap = old-> data-> cap;
    result-> data-> entries = (_yrt_map_entry_t**) GC_malloc ((old-> data-> cap) * sizeof (_yrt_map_entry_t*));
    memset (result-> data-> entries, 0, (old-> data-> cap) * sizeof (_yrt_map_entry_t*));

    _map_copy_entries (result, old);
}

void _yrt_map_insert (_yrt_map_t * mp, uint8_t * key, uint8_t * value) {
    if (mp-> data-> cap == 0) {
        _map_fit (mp, 1);
    } else if ((mp-> data-> loaded * 100) > (MAP_MAX_LOADED_FACTOR * mp-> data-> cap)) {
        _map_fit (mp, _next_pow2 (mp-> data-> cap + 1));
    }

    uint64_t hash = mp-> data-> minfo-> hash (key);
    _map_insert_no_resize (mp, hash, key, value);
}

void _map_insert_no_resize (_yrt_map_t * mp, uint64_t hash, uint8_t * key, uint8_t * value) {
    uint64_t index = MAP_BUCKET_INDEX (hash, mp-> data-> cap);
    if (mp-> data-> entries [index] != NULL) {
        _yrt_map_entry_t * entry = mp-> data-> entries [index];
        if (_map_entry_insert (mp-> data, entry, hash, key, value) == 1) {
            mp-> data-> len += 1;
        }

        return;
    }

    _map_create_entry (mp-> data, &(mp-> data-> entries [index]), hash, key, value);
    mp-> data-> loaded += 1;
    mp-> data-> len += 1;
}

uint8_t _map_entry_insert (_yrt_map_content_t * data, _yrt_map_entry_t * mp, uint64_t hash, uint8_t * key, uint8_t * value) {
    _yrt_map_info_t * minfo = data-> minfo;
    uint8_t * keyEntry = ((uint8_t*) mp) + sizeof (_yrt_map_entry_t);
    if (mp-> hash == hash && minfo-> cmp (key, keyEntry) == 1) {
        uint8_t * valEntry = keyEntry + minfo-> keySize;
        memcpy (valEntry, value, minfo-> valueSize);
        return 0;
    }

    if (mp-> next != NULL) {
        return _map_entry_insert (data, mp-> next, hash, key, value);
    }

    _map_create_entry (data, &(mp-> next), hash, key, value);
    return 1;
}

void _map_create_entry (_yrt_map_content_t * data, _yrt_map_entry_t ** entry, uint64_t hash, uint8_t * key, uint8_t * value) {
    _yrt_map_info_t * minfo = data-> minfo;
    uint64_t nodeSize = sizeof (_yrt_map_entry_t) + (minfo-> keySize + minfo-> valueSize);
    uint8_t * newEntry = _map_entry_alloc (data, nodeSize);
    uint8_t * keyEntry = newEntry + sizeof (_yrt_map_entry_t);
    uint8_t * valueEntry = keyEntry + minfo-> keySize;

    ((_yrt_map_entry_t*) newEntry)-> hash = hash;
    ((_yrt_map_entry_t*) newEntry)-> next = NULL;

    memcpy (keyEntry, key, minfo-> keySize);
    memcpy (valueEntry, value, minfo-> valueSize);

    *entry = (_yrt_map_entry_t*) newEntry;
}

void _yrt_map_erase (_yrt_map_t * mp, uint8_t * key) {
    if (mp-> data-> cap == 0) {
        return;
    }

    uint64_t hash = mp-> data-> minfo-> hash (key);
    uint64_t index = MAP_BUCKET_INDEX (hash, mp-> data-> cap);
    if (mp-> data-> entries [index] == NULL) {
        return;
    }

    if (_map_erase_entry (&(mp-> data-> entries [index]), hash, key, mp-> data-> minfo) == 1) {
        mp-> data-> len -= 1;
    }

    if (mp-> data-> entries [index] == NULL) {
        mp-> data-> loaded -= 1;
    }

    if ((mp-> data-> loaded * 100) < (MAP_MIN_LOADED_FACTOR * mp-> data-> cap)) {
        _map_fit (mp, _next_pow2 (mp-> data-> loaded + 1));
    }
}

uint8_t _map_erase_entry (_yrt_map_entry_t ** en, uint64_t hash, uint8_t * key, _yrt_map_info_t * minfo) {
    uint8_t * keyEntry = ((uint8_t*) (*en)) + sizeof (_yrt_map_entry_t);
    if ((*en)-> hash == hash && minfo-> cmp (key, keyEntry) == 1) {
        *en = (*en)-> next;

        return 1;
    }

    if ((*en)-> next != NULL) {
        return _map_erase_entry (&(*en)-> next, hash, key, minfo);
    }

    return 0;
}

uint8_t * _yrt_map_find (_yrt_map_t * mp, uint8_t * key) {
    if (mp-> data-> cap == 0) {
        return NULL;
    }

    uint64_t hash = mp-> data-> minfo-> hash (key);
    uint64_t index = MAP_BUCKET_INDEX (hash, mp-> data-> cap);
    if (mp-> data-> entries [index] == NULL) {
        return NULL;
    }

    return _map_find_entry (mp-> data-> entries [index], hash, key, mp-> data-> minfo);
}

uint8_t * _map_find_entry (_yrt_map_entry_t * en, uint64_t hash, uint8_t * key, _yrt_map_info_t * minfo) {
    uint8_t * keyEntry = ((uint8_t*) en) + sizeof (_yrt_map_entry_t);
    if (en-> hash == hash && minfo-> cmp (key, keyEntry) == 1) {
        uint8_t * valueEntry = keyEntry + minfo-> keySize;
        return valueEntry;
    }

    if (en-> next == NULL) {
        return NULL;
    }

    return _map_find_entry (en-> next, hash, key, minfo);
}

void _map_fit (_yrt_map_t * mp, uint64_t newSize) {
    if (newSize == 0) {
        _yrt_map_empty (mp, mp-> data-> minfo);
        return;
    }

    _yrt_map_t result;
    _yrt_map_content_t data;

    result.data = &data;
    result.data-> cap = newSize;
    result.data-> entries = (_yrt_map_entry_t**) GC_malloc (newSize * sizeof (_yrt_map_entry_t*));
    memset (result.data-> entries, 0, newSize * sizeof (_yrt_map_entry_t*));

    result.data-> minfo = mp-> data-> minfo;
    result.data-> loaded = 0;
    result.data-> len = 0;

    _map_relink_entries (&result, mp);
    mp-> data-> loaded = data.loaded;
    mp-> data-> len = data.len;
    mp-> data-> cap = data.cap;
    mp-> data-> entries = data.entries;
}

void _map_copy_entries (_yrt_map_t * result, _yrt_map_t * old) {
    for (uint64_t i = 0 ; i < old-> data-> cap ; i++) {
        if (old-> data-> entries [i] != NULL) {
            _yrt_map_entry_t * head = old-> data-> entries [i];
            while (head != NULL) {
                uint64_t hash = head-> hash;
                uint8_t * key = ((uint8_t*) head) + sizeof (_yrt_map_entry_t);
                uint8_t * value = ((uint8_t*) head) + sizeof (_yrt_map_entry_t) + old-> data-> minfo-> keySize;

                _map_insert_no_resize (result, hash, key, value);
                head = head-> next;
            }
        }
    }
}

void _map_relink_entries (_yrt_map_t * result, _yrt_map_t * old) {
    for (uint64_t i = 0 ; i < old-> data-> cap ; i++) {
        _yrt_map_entry_t * head = old-> data-> entries [i];
        while (head != NULL) {
            _yrt_map_entry_t * next = head-> next;
            uint64_t index = MAP_BUCKET_INDEX (head-> hash, result-> data-> cap);

            if (result-> data-> entries [index] == NULL) {
                result-> data-> loaded += 1;
            }

            head-> next = result-> data-> entries [index];
            result-> data-> entries [index] = head;
            result-> data-> len += 1;

            head = next;
        }
    }
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          MAP ITERATION          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */


_yrt_map_iterator_t * _yrt_map_iter_begin (_yrt_map_t * mp) {
    if (mp == NULL || mp-> data == NULL || mp-> data-> cap == 0) {
        return NULL;
    }

    for (uint64_t i = 0 ; i < mp-> data-> cap ; i++) {
        if (mp-> data-> entries [i] != NULL) { // return the first allocated node found in the map
            // We allocate without the GC, since the iterator is necessarily cleaned at exit
            _yrt_map_iterator_t * result = (_yrt_map_iterator_t*) malloc (sizeof (_yrt_map_iterator_t));

            result-> mp = mp;
            result-> rootIndex = i;
            result-> current = mp-> data-> entries [i];
            result-> notEnd = 1;

            return result;
        }
    }

    // No node found
    return NULL;
}

uint8_t* _yrt_map_iter_key (_yrt_map_iterator_t * iter) {
    return ((uint8_t*) iter-> current) + sizeof (_yrt_map_entry_t);
}

uint8_t* _yrt_map_iter_val (_yrt_map_iterator_t * iter) {
    _yrt_map_info_t * minfo = iter-> mp-> data-> minfo;
    uint8_t * keyEntry = ((uint8_t*) iter-> current) + sizeof (_yrt_map_entry_t);
    return keyEntry + minfo-> keySize;
}

uint8_t _yrt_map_iter_end (_yrt_map_iterator_t * iter) {
    if (iter == NULL) return 0;

    return iter-> notEnd;
}

void _yrt_map_iter_next (_yrt_map_iterator_t * iter) {
    if (iter == NULL || iter-> current == NULL || !iter-> notEnd) return;

    if (iter-> current != NULL && iter-> current-> next != NULL) {
        iter-> current = iter-> current-> next;
        return;
    }

    // in case of refit the size of the map might have changed
    // And we don't want the index to overflow
    if (iter-> rootIndex < iter-> mp-> data-> cap) {
        for (uint64_t i = iter-> rootIndex + 1 ; i < iter-> mp-> data-> cap ; i++) {
            if (iter-> mp-> data-> entries [i] != NULL) {
                iter-> rootIndex = i;
                iter-> current = iter-> mp-> data-> entries [i];
                iter-> notEnd = 1;

                return;
            }
        }
    }

    iter-> current = NULL;
    iter-> rootIndex = iter-> mp-> data-> cap;
    iter-> notEnd = 0;
}

void _yrt_map_iter_del (_yrt_map_iterator_t * iter) {
    if (iter != NULL) {
        free (iter);
    }
}
