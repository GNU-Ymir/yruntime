#ifndef MAP_H_
#define MAP_H_

#include <rt/memory/types.h>

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
char _yrt_dcopy_map_is_started ();

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
