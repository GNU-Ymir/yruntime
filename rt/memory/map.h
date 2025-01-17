#ifndef MAP_H_
#define MAP_H_

#include <rt/memory/types.h>

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ================================          DEFAULT MAP IMPL          ================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Create an empty map
 * @params:
 *    - mp: the map to init
 *    - info: the information about the types in the map
 */
void _yrt_map_empty (_yrt_map_t * mp, _yrt_map_info_t * info);

/**
 * Create a new map from a copy of an old map
 * @params:
 *    - result: the result map
 *    - info: the information about the types in the map
 *    - old: the old map to copy
 */
void _yrt_dup_map (_yrt_map_t * result, _yrt_map_info_t * info, _yrt_map_t *old);

/**
 * Insert an element into the map
 * @params:
 *    - mp: the map in which the insertion is made
 *    - key: the data of the key
 *    - value: the data of the value
 */
void _yrt_map_insert (_yrt_map_t * mp, uint8_t * key, uint8_t * value);

/**
 * Insert an element into the map without rebalancing
 * @params:
 *    - mp: the map in which the insertion is made
 *    - hash: the hash of the key
 *    - key: the data of the key
 *    - value: the data of the value
 */
void _yrt_map_insert_no_resize (_yrt_map_t * mp, uint64_t hash, uint8_t * key, uint8_t * value);
/**
 * Insert an element in the map entry linked list
 * @params:
 *    - en: the current head of the list
 *    - hash: the hash of the key
 *    - key: the data of the key
 *    - value: the data of the value
 * @returns: true if new entry, false if replaced
 */
uint8_t _yrt_map_entry_insert (_yrt_map_entry_t * en, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_t * minfo);

/**
 * Allocate a map entry
 */
void _yrt_map_create_entry (_yrt_map_entry_t ** en, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_t * minfo);

/**
 * Erase data from the map
 */
void _yrt_map_erase (_yrt_map_t * mp, uint8_t * key);

/**
 * Erase a key from the linked list
 */
uint8_t _yrt_map_erase_entry (_yrt_map_entry_t ** en, uint8_t * key, _yrt_map_info_t * minfo);

/**
 * Lookup wheter the key exists in the map
 * @params:
 *    - mp: the map
 *    - key: the key to find
 * @returns:
 *    - 0 if not found, 1 otherwise
 *    - value: the value
 */
uint8_t * _yrt_map_find (_yrt_map_t * mp, uint8_t * key);

/**
 * Lookup wheter the key exists in the linked list
 * @params:
 *    - en: the current node of the list
 *    - key: the key to find
 *    - valueSize: the size of the values
 * @returns:
 *    - 0 if not found, 1 otherwise
 *    - value: the value
 */
uint8_t * _yrt_map_find_entry (_yrt_map_entry_t * en, uint8_t * key, _yrt_map_info_t * minfo);

/**
 * Change the size of the map to decrease the complexity of access/insert or to decrease the amount of unused indexes
 */
void _yrt_map_fit (_yrt_map_t * mp, uint64_t newSize);

/**
 * Insert all the key-> values from old into result
 * @assume: result has allocated data array
 * @warning: result is not be resized
 */
void _yrt_map_copy_entries (_yrt_map_t * result, _yrt_map_t * old);



/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          MAP ITERATION          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Create a new iterator over a map
 */
_yrt_map_iterator_t * _yrt_map_iter_begin (_yrt_map_t * mp);

/**
 * Extract the key pointed by the current iteration
 */
uint8_t* _yrt_map_iter_key (_yrt_map_iterator_t * iter);

/**
 * Extract the value pointed by the current iteration
 */
uint8_t* _yrt_map_iter_val (_yrt_map_iterator_t * iter);

/**
 * Increment the iterator and go to the next element
 */
void _yrt_map_iter_next (_yrt_map_iterator_t * iter);

/**
 * Free the allocated iterator
 */
void _yrt_map_iter_del (_yrt_map_iterator_t * iter);

#endif // MAP_H_
