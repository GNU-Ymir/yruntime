#include <rt/memory/map.h>
#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <rt/thread.h>

#include <string.h>

pthread_mutex_t _yrt_dcopy_map_mutex;
_yrt_dcopy_map_node_ _yrt_dcopy_head = {.len = 0, .used = 0, .from = NULL, .to = NULL};

#define MAP_MAX_LOADED_FACTOR 75
#define MAP_MIN_LOADED_FACTOR 40

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ================================          DEFAULT MAP IMPL          ================================
 * ====================================================================================================
 * ====================================================================================================
 */

void _yrt_map_empty (_yrt_map_ * mp, _yrt_map_info_ * info) {
    mp-> minfo = info;
    mp-> data = NULL;
    mp-> allocLen = 0;
    mp-> loaded = 0;
    mp-> len = 0;
}

void _yrt_map_insert (_yrt_map_ * mp, uint8_t * key, uint8_t * value) {
    if (mp-> allocLen == 0) {
        _yrt_map_fit (mp, 1);
    } else if ((mp-> loaded * 100 / mp-> allocLen) > MAP_MAX_LOADED_FACTOR) {
        _yrt_map_fit (mp, _yrt_next_pow2 (mp-> allocLen + 1));
    }

    uint64_t hash = mp-> minfo-> hash (key);
    _yrt_map_insert_no_resize (mp, hash, key, value);
}

void _yrt_map_insert_no_resize (_yrt_map_ * mp, uint64_t hash, uint8_t * key, uint8_t * value) {
    uint64_t index = hash % mp-> allocLen;
    if (mp-> data [index] != NULL) {
        _yrt_map_entry_ * entry = mp-> data [index];
        if (_yrt_map_entry_insert (entry, hash, key, value, mp-> minfo) == 1) {
            mp-> len += 1;
        }

        return;
    }

    _yrt_map_create_entry (&(mp-> data [index]), hash, key, value, mp-> minfo);
    mp-> loaded += 1;
    mp-> len += 1;
}

uint8_t _yrt_map_entry_insert (_yrt_map_entry_ * mp, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_ * minfo) {
    uint8_t * keyEntry = ((uint8_t*) mp) + sizeof (_yrt_map_entry_);
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

void _yrt_map_create_entry (_yrt_map_entry_ ** entry, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_ * minfo) {
    uint64_t nodeSize = sizeof (_yrt_map_entry_) + (minfo-> keySize + minfo-> valueSize);
    uint8_t * newEntry = (uint8_t *) GC_malloc (nodeSize);
    uint8_t * keyEntry = newEntry + sizeof (_yrt_map_entry_);
    uint8_t * valueEntry = keyEntry + minfo-> keySize;

    ((_yrt_map_entry_*) newEntry)-> hash = hash;
    ((_yrt_map_entry_*) newEntry)-> next = NULL;

    memcpy (keyEntry, key, minfo-> keySize);
    memcpy (valueEntry, value, minfo-> valueSize);

    *entry = (_yrt_map_entry_*) newEntry;
}

void _yrt_map_erase (_yrt_map_ * mp, uint8_t * key) {
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

uint8_t _yrt_map_erase_entry (_yrt_map_entry_ ** en, uint8_t * key, _yrt_map_info_ * minfo) {
    uint8_t * keyEntry = ((uint8_t*) (*en)) + sizeof (_yrt_map_entry_);
    if (minfo-> cmp (key, keyEntry) == 1) {
        *en = (*en)-> next;
        return 1;
    }

    if ((*en)-> next != NULL) {
        return _yrt_map_erase_entry (&(*en)-> next, key, minfo);
    }

    return 0;
}

uint8_t _yrt_map_find (_yrt_map_ * mp, uint8_t * key, uint8_t * value) {
    if (mp-> allocLen == 0) {
        return 0;
    }

    uint64_t hash = mp-> minfo-> hash (key);
    uint64_t index = hash % mp-> allocLen;
    if (mp-> data [index] == NULL) {
        return 0;
    }

    return _yrt_map_find_entry (mp-> data [index], key, value, mp-> minfo);
}

uint8_t _yrt_map_find_entry (_yrt_map_entry_ * en, uint8_t * key, uint8_t * value, _yrt_map_info_ * minfo) {
    uint8_t * keyEntry = ((uint8_t*) en) + sizeof (_yrt_map_entry_);
    if (minfo-> cmp (key, keyEntry) == 1) {
        uint8_t * valueEntry = keyEntry + minfo-> keySize;
        memcpy (value, valueEntry, minfo-> valueSize);

        return 1;
    }

    if (en-> next == NULL) {
        return 0;
    }

    return _yrt_map_find_entry (en-> next, key, value, minfo);
}

void _yrt_map_fit (_yrt_map_ * mp, uint64_t newSize) {
    if (newSize == 0) {
        mp-> data = NULL;
        mp-> loaded = 0;
        mp-> allocLen = 0;
        mp-> len = 0;

        return;
    }

    _yrt_map_ result;
    result.data = (_yrt_map_entry_**) GC_malloc (newSize * sizeof (_yrt_map_entry_*));
    memset (result.data, 0, newSize * sizeof (_yrt_map_entry_*));

    result.minfo = mp-> minfo;
    result.loaded = 0;
    result.len = 0;
    result.allocLen = newSize;

    for (uint64_t i = 0 ; i < mp-> allocLen ; i++) {
        if (mp-> data [i] != NULL) {
            _yrt_map_entry_ * head = mp-> data [i];
            while (head != NULL) {
                uint64_t hash = head-> hash;
                uint8_t * key = ((uint8_t*) head) + sizeof (_yrt_map_entry_);
                uint8_t * value = ((uint8_t*) head) + sizeof (_yrt_map_entry_) + mp-> minfo-> keySize;

                _yrt_map_insert_no_resize (&result, hash, key, value);
                head = head-> next;
            }
        }
    }

    *mp = result;
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
