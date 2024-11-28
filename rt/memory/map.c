#include <rt/memory/map.h>
#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <rt/concurrency/thread.h>

#include <string.h>
#include <stdlib.h>

pthread_mutex_t _yrt_dcopy_map_mutex;
_yrt_dcopy_map_node_t _yrt_dcopy_head = {.len = 0, .used = 0, .from = NULL, .to = NULL};

#define MAP_MAX_LOADED_FACTOR 75
#define MAP_MIN_LOADED_FACTOR 40

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ================================          DEFAULT MAP IMPL          ================================
 * ====================================================================================================
 * ====================================================================================================
 */

void _yrt_map_empty (_yrt_map_t * mp, _yrt_map_info_t * info) {
    mp-> minfo = info;
    mp-> data = NULL;
    mp-> allocLen = 0;
    mp-> loaded = 0;
    mp-> len = 0;
}

void _yrt_dup_map (_yrt_map_t * result, _yrt_map_info_t * info, _yrt_map_t * old) {
    if (old == NULL || old-> data == NULL || old-> len == 0) {
        _yrt_map_empty (result, info);
    }

    result-> data = (_yrt_map_entry_t**) GC_malloc ((result-> allocLen) * sizeof (_yrt_map_entry_t*));
    memset (result-> data, 0, (result-> allocLen) * sizeof (_yrt_map_entry_t*));

    result-> minfo = info;
    result-> allocLen = old-> allocLen;
    result-> loaded = 0;
    result-> len = 0;

    _yrt_map_copy_entries (result, old);
}

void _yrt_map_insert (_yrt_map_t * mp, uint8_t * key, uint8_t * value) {
    if (mp-> allocLen == 0) {
        _yrt_map_fit (mp, 1);
    } else if ((mp-> loaded * 100 / mp-> allocLen) > MAP_MAX_LOADED_FACTOR) {
        _yrt_map_fit (mp, _yrt_next_pow2 (mp-> allocLen + 1));
    }

    uint64_t hash = mp-> minfo-> hash (key);
    _yrt_map_insert_no_resize (mp, hash, key, value);
}

void _yrt_map_insert_no_resize (_yrt_map_t * mp, uint64_t hash, uint8_t * key, uint8_t * value) {
    uint64_t index = hash % mp-> allocLen;
    if (mp-> data [index] != NULL) {
        _yrt_map_entry_t * entry = mp-> data [index];
        if (_yrt_map_entry_insert (entry, hash, key, value, mp-> minfo) == 1) {
            mp-> len += 1;
        }

        return;
    }

    _yrt_map_create_entry (&(mp-> data [index]), hash, key, value, mp-> minfo);
    mp-> loaded += 1;
    mp-> len += 1;
}

uint8_t _yrt_map_entry_insert (_yrt_map_entry_t * mp, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_t * minfo) {
    uint8_t * keyEntry = ((uint8_t*) mp) + sizeof (_yrt_map_entry_t);
    if (minfo-> cmp (key, keyEntry) == 1) {
        uint8_t * valEntry = keyEntry + minfo-> keySize;
        memcpy (valEntry, value, minfo-> valueSize);
        return 0;
    }

    if (mp-> next != NULL) {
        return _yrt_map_entry_insert (mp-> next, hash, key, value, minfo);
    }

    _yrt_map_create_entry (&(mp-> next), hash, key, value, minfo);
    return 1;
}

void _yrt_map_create_entry (_yrt_map_entry_t ** entry, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_t * minfo) {
    uint64_t nodeSize = sizeof (_yrt_map_entry_t) + (minfo-> keySize + minfo-> valueSize);
    uint8_t * newEntry = (uint8_t *) GC_malloc (nodeSize);
    uint8_t * keyEntry = newEntry + sizeof (_yrt_map_entry_t);
    uint8_t * valueEntry = keyEntry + minfo-> keySize;

    ((_yrt_map_entry_t*) newEntry)-> hash = hash;
    ((_yrt_map_entry_t*) newEntry)-> next = NULL;

    memcpy (keyEntry, key, minfo-> keySize);
    memcpy (valueEntry, value, minfo-> valueSize);

    *entry = (_yrt_map_entry_t*) newEntry;
}

void _yrt_map_erase (_yrt_map_t * mp, uint8_t * key) {
    if (mp-> allocLen == 0) {
        return;
    }

    uint64_t hash = mp-> minfo-> hash (key);
    uint64_t index = hash % mp-> allocLen;
    if (mp-> data [index] == NULL) {
        return;
    }

    if (_yrt_map_erase_entry (&(mp-> data [index]), key, mp-> minfo) == 1) {
        mp-> len -= 1;
    }

    if (mp-> data [index] == NULL) {
        mp-> loaded -= 1;
    }

    if (((mp-> loaded * 100) / mp-> allocLen) < MAP_MIN_LOADED_FACTOR) {
        _yrt_map_fit (mp, _yrt_next_pow2 (mp-> loaded + 1));
    }
}

uint8_t _yrt_map_erase_entry (_yrt_map_entry_t ** en, uint8_t * key, _yrt_map_info_t * minfo) {
    uint8_t * keyEntry = ((uint8_t*) (*en)) + sizeof (_yrt_map_entry_t);
    if (minfo-> cmp (key, keyEntry) == 1) {
        *en = (*en)-> next;

        return 1;
    }

    if ((*en)-> next != NULL) {
        return _yrt_map_erase_entry (&(*en)-> next, key, minfo);
    }

    return 0;
}

uint8_t * _yrt_map_find (_yrt_map_t * mp, uint8_t * key) {
    if (mp-> allocLen == 0) {
        return NULL;
    }

    uint64_t hash = mp-> minfo-> hash (key);
    uint64_t index = hash % mp-> allocLen;
    if (mp-> data [index] == NULL) {
        return NULL;
    }

    return _yrt_map_find_entry (mp-> data [index], key, mp-> minfo);
}

uint8_t * _yrt_map_find_entry (_yrt_map_entry_t * en, uint8_t * key, _yrt_map_info_t * minfo) {
    uint8_t * keyEntry = ((uint8_t*) en) + sizeof (_yrt_map_entry_t);
    if (minfo-> cmp (key, keyEntry) == 1) {
        uint8_t * valueEntry = keyEntry + minfo-> keySize;
        return valueEntry;
    }

    if (en-> next == NULL) {
        return NULL;
    }

    return _yrt_map_find_entry (en-> next, key, minfo);
}

void _yrt_map_fit (_yrt_map_t * mp, uint64_t newSize) {
    if (newSize == 0) {
        mp-> data = NULL;
        mp-> loaded = 0;
        mp-> allocLen = 0;
        mp-> len = 0;

        return;
    }

    _yrt_map_t result;
    result.data = (_yrt_map_entry_t**) GC_malloc (newSize * sizeof (_yrt_map_entry_t*));
    memset (result.data, 0, newSize * sizeof (_yrt_map_entry_t*));

    result.minfo = mp-> minfo;
    result.loaded = 0;
    result.len = 0;
    result.allocLen = newSize;

    _yrt_map_copy_entries (&result, mp);
    *mp = result;
}

void _yrt_map_copy_entries (_yrt_map_t * result, _yrt_map_t * old) {
    for (uint64_t i = 0 ; i < old-> allocLen ; i++) {
        if (old-> data [i] != NULL) {
            _yrt_map_entry_t * head = old-> data [i];
            while (head != NULL) {
                uint64_t hash = head-> hash;
                uint8_t * key = ((uint8_t*) head) + sizeof (_yrt_map_entry_t);
                uint8_t * value = ((uint8_t*) head) + sizeof (_yrt_map_entry_t) + old-> minfo-> keySize;

                _yrt_map_insert_no_resize (result, hash, key, value);
                head = head-> next;
            }
        }
    }
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          COPY MAP          ====================================
 * ====================================================================================================
 * ====================================================================================================
 */

uint8_t _yrt_dcopy_map_is_started () {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    char ret = (_yrt_dcopy_head.len != 0);
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
    return ret;
}

void _yrt_purge_dcopy_map () {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    _yrt_dcopy_head.len = 0;
    _yrt_dcopy_head.used = 0;
    _yrt_dcopy_head.from = NULL;
    _yrt_dcopy_head.to = NULL;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void _yrt_insert_dcopy_map (void * data, void * to) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    uint64_t index = _yrt_dcopy_head.used;
    if (index >= _yrt_dcopy_head.len) _yrt_dcopy_map_grow ();

    _yrt_dcopy_head.from [index] = data;
    _yrt_dcopy_head.to [index] = to;
    _yrt_dcopy_head.used += 1;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void* _yrt_find_dcopy_map (void * data) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    for (uint64_t i = 0 ; i < _yrt_dcopy_head.used ; i++) {
        if (_yrt_dcopy_head.from [i] == data) {
            _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
            return _yrt_dcopy_head.to [i];
        }
    }
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);

    return NULL;
}

void _yrt_dcopy_map_grow () {
    uint64_t len = _yrt_dcopy_head.len;
    if (len == 0) len = 10;
    else len = len * 2;

    void** from_res = (void**) GC_malloc (len * sizeof (void*));
    void** to_res = (void**) GC_malloc (len * sizeof (void*));

    for (uint64_t i = 0 ; i < _yrt_dcopy_head.len ; i++) {
        from_res [i] = _yrt_dcopy_head.from [i];
        to_res [i] = _yrt_dcopy_head.to [i];
    }

    _yrt_dcopy_head.len = len;
    _yrt_dcopy_head.from = from_res;
    _yrt_dcopy_head.to = to_res;
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          MAP ITERATION          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */


_yrt_map_iterator_t * _yrt_map_iter_begin (_yrt_map_t * mp) {
    if (mp == NULL || mp-> data == NULL || mp-> len == 0) {
        return NULL;
    }

    for (uint64_t i = 0 ; i < mp-> allocLen ; i++) {
        if (mp-> data [i] != NULL) { // return the first allocated node found in the map
            // We allocate without the GC, since the iterator is necessarily cleaned at exit
            _yrt_map_iterator_t * result = (_yrt_map_iterator_t*) malloc (sizeof (_yrt_map_iterator_t));

            result-> mp = mp;
            result-> rootIndex = i;
            result-> current = mp-> data [i];
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
    _yrt_map_info_t * minfo = iter-> mp-> minfo;
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
    if (iter-> rootIndex < iter-> mp-> allocLen) {
        for (uint64_t i = iter-> rootIndex + 1 ; i < iter-> mp-> allocLen ; i++) {
            if (iter-> mp-> data [i] != NULL) {
                iter-> rootIndex = i;
                iter-> current = iter-> mp-> data [i];
                iter-> notEnd = 1;

                return;
            }
        }
    }

    iter-> current = NULL;
    iter-> rootIndex = iter-> mp-> allocLen;
    iter-> notEnd = 0;
}

void _yrt_map_iter_del (_yrt_map_iterator_t * iter) {
    if (iter != NULL) {
        free (iter);
    }
}
