#include <rt/memory/map.h>
#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <rt/concurrency/thread.h>

#include <string.h>
#include <stdlib.h>

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
    mp-> data = (_yrt_map_content_t*) GC_malloc (sizeof (_yrt_map_content_t));
    mp-> data-> len = 0;
    mp-> data-> minfo = info;
    mp-> data-> loaded = 0;
    mp-> data-> cap = 0;
    mp-> data-> entries = NULL;
}

void _yrt_dup_map (_yrt_map_t * result, _yrt_map_info_t * info, _yrt_map_t * old) {
    _yrt_map_empty (result, info);
    if (old == NULL || old-> data == NULL || old-> data-> len == 0 || old-> data-> cap == 0) {
        return;
    }

    result-> data-> cap = old-> data-> cap;
    result-> data-> entries = (_yrt_map_entry_t**) GC_malloc ((old-> data-> cap) * sizeof (_yrt_map_entry_t*));
    memset (result-> data-> entries, 0, (old-> data-> cap) * sizeof (_yrt_map_entry_t*));

    _yrt_map_copy_entries (result, old);
}

void _yrt_map_insert (_yrt_map_t * mp, uint8_t * key, uint8_t * value) {
    if (mp-> data-> cap == 0) {
        _yrt_map_fit (mp, 1);
    } else if ((mp-> data-> loaded * 100 / mp-> data-> cap) > MAP_MAX_LOADED_FACTOR) {
        _yrt_map_fit (mp, _yrt_next_pow2 (mp-> data-> cap + 1));
    }

    uint64_t hash = mp-> data-> minfo-> hash (key);
    _yrt_map_insert_no_resize (mp, hash, key, value);
}

void _yrt_map_insert_no_resize (_yrt_map_t * mp, uint64_t hash, uint8_t * key, uint8_t * value) {
    uint64_t index = hash % mp-> data-> cap;
    if (mp-> data-> entries [index] != NULL) {
        _yrt_map_entry_t * entry = mp-> data-> entries [index];
        if (_yrt_map_entry_insert (entry, hash, key, value, mp-> data-> minfo) == 1) {
            mp-> data-> len += 1;
        }

        return;
    }

    _yrt_map_create_entry (&(mp-> data-> entries [index]), hash, key, value, mp-> data-> minfo);
    mp-> data-> loaded += 1;
    mp-> data-> len += 1;
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
    if (mp-> data-> cap == 0) {
        return;
    }

    uint64_t hash = mp-> data-> minfo-> hash (key);
    uint64_t index = hash % mp-> data-> cap;
    if (mp-> data-> entries [index] == NULL) {
        return;
    }

    if (_yrt_map_erase_entry (&(mp-> data-> entries [index]), key, mp-> data-> minfo) == 1) {
        mp-> data-> len -= 1;
    }

    if (mp-> data-> entries [index] == NULL) {
        mp-> data-> loaded -= 1;
    }

    if (((mp-> data-> loaded * 100) / mp-> data-> cap) < MAP_MIN_LOADED_FACTOR) {
        _yrt_map_fit (mp, _yrt_next_pow2 (mp-> data-> loaded + 1));
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
    if (mp-> data-> cap == 0) {
        return NULL;
    }

    uint64_t hash = mp-> data-> minfo-> hash (key);
    uint64_t index = hash % mp-> data-> cap;
    if (mp-> data-> entries [index] == NULL) {
        return NULL;
    }

    return _yrt_map_find_entry (mp-> data-> entries [index], key, mp-> data-> minfo);
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

    _yrt_map_copy_entries (&result, mp);
    mp-> data-> loaded = data.loaded;
    mp-> data-> len = data.len;
    mp-> data-> cap = data.cap;
    mp-> data-> entries = data.entries;
}

void _yrt_map_copy_entries (_yrt_map_t * result, _yrt_map_t * old) {
    for (uint64_t i = 0 ; i < old-> data-> cap ; i++) {
        if (old-> data-> entries [i] != NULL) {
            _yrt_map_entry_t * head = old-> data-> entries [i];
            while (head != NULL) {
                uint64_t hash = head-> hash;
                uint8_t * key = ((uint8_t*) head) + sizeof (_yrt_map_entry_t);
                uint8_t * value = ((uint8_t*) head) + sizeof (_yrt_map_entry_t) + old-> data-> minfo-> keySize;

                _yrt_map_insert_no_resize (result, hash, key, value);
                head = head-> next;
            }
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
