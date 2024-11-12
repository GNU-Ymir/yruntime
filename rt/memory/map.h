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
 */
void _yrt_map_empty (_yrt_map_ * mp, uint64_t (*hash) (uint8_t*), uint8_t (*cmp) (uint8_t*, uint8_t*), uint64_t keySize, uint64_t valueSize);

/**
 * Insert an element into the map
 * @params:
 *    - mp: the map in which the insertion is made
 *    - key: the data of the key
 *    - value: the data of the value
 */
void _yrt_map_insert (_yrt_map_ * mp, uint8_t * key, uint8_t * value);

/**
 * Insert an element into the map without rebalancing
 * @params:
 *    - mp: the map in which the insertion is made
 *    - hash: the hash of the key
 *    - key: the data of the key
 *    - value: the data of the value
 */
void _yrt_map_insert_no_resize (_yrt_map_ * mp, uint64_t hash, uint8_t * key, uint8_t * value);
/**
 * Insert an element in the map entry linked list
 * @params:
 *    - en: the current head of the list
 *    - hash: the hash of the key
 *    - key: the data of the key
 *    - value: the data of the value
 * @returns: true if new entry, false if replaced
 */
uint8_t _yrt_map_entry_insert (_yrt_map_entry_ * en, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_ * minfo);

/**
 * Allocate a map entry
 */
void _yrt_map_create_entry (_yrt_map_entry_ ** en, uint64_t hash, uint8_t * key, uint8_t * value, _yrt_map_info_ * minfo);

/**
 * Erase data from the map
 */
void _yrt_map_erase (_yrt_map_ * mp, uint8_t * key);

/**
 * Erase a key from the linked list
 */
uint8_t _yrt_map_erase_entry (_yrt_map_entry_ ** en, uint8_t * key, _yrt_map_info_ * minfo);

/**
 * Lookup wheter the key exists in the map
 * @params:
 *    - mp: the map
 *    - key: the key to find
 * @returns:
 *    - 0 if not found, 1 otherwise
 *    - value: the value
 */
uint8_t _yrt_map_find (_yrt_map_ * mp, uint8_t * key, uint8_t * value);

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
uint8_t _yrt_map_find_entry (_yrt_map_entry_ * en, uint8_t * key, uint8_t * value, _yrt_map_info_ * minfo);

/**
 * Change the size of the map to decrease the complexity of access/insert or to decrease the amount of unused indexes
 */
void _yrt_map_fit (_yrt_map_ * mp, uint64_t newSize);

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          COPY MAP          ====================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * A dcopy map is started if it contains at least one element
 * @return: true if the deep copy map has been started, false otherwise
 */
uint8_t _yrt_dcopy_map_is_started ();

/**
 * Purge the map containing all the current deep copy information
 */
void _yrt_purge_dcopy_map ();

/**
 * Search in the map if the element has already been deep copied, if true, then simply returns the pointer to that element
 * @params:
 *   - data: the pointer to an element
 * @return: a pointer either null, or the an element
 */
void* _yrt_find_dcopy_map (void* data);

/**
 * Insert an element that has been deep copied in the map
 * @params:
 * - data: the old pointer
 * - value: the copied pointer
 */
void _yrt_insert_dcopy_map (void* data, void* value);

/**
 * Make the copy map grow in size
 *  */
void _yrt_dcopy_map_grow ();



#endif // MAP_H_
