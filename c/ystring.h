#ifndef _Y_STRING_H_
#define _Y_STRING_H_

#include <stdint.h>

/**
 * A string is a buffer of chars
 * The allocated size if generally larger that the used once
 * Thus, we can add element in the string without allocated each time 
 * All allocation are made with GC_malloc
 */
typedef struct {
    char * data;
    uint64_t len;
    uint64_t capacity;    
} _ystring;

/**
 * Make the capacity of the string grow
 * @params: 
 *    - self: the old string to make grow
 * @return: the new string
 */
_ystring str_grow (_ystring self, unsigned long min);

/**
 * Concatenate two strings
 * @params: 
 *    - left: the left operand of the concatenation
 *    - right: the right operand of the concatenation
 * @return: the concatenation is made on the left operand
 */
_ystring str_concat (_ystring left, _ystring right);

/**
 * Concatenate two strings
 * @params: 
 *    - left: the left operand of the concatenation
 *    - right: the right operand of the concatenation
 * @return: the concatenation is made on the left operand
 */
_ystring str_concat_c_str (_ystring left, const char *right);

/**
 * Transform a int value into a string
 */
_ystring str_from_int (int value);


/**
 * Transform a int value into a string
 */
_ystring str_from_char (char value);


/**
 * Transform a ptr value into a string
 */
_ystring str_from_ptr (void* value);

/**
 * Create a _ystring pointer to data
 */
_ystring str_create (const char * data);


/**
 * Create a _ystring pointer to data
 */
_ystring str_create_len (const char * data, unsigned long len);


/**
 * Create a copy of the data
 */
_ystring str_copy_len (const char *data, unsigned long len);

/**
 * Create a copy of the data
 * @warning: the data is assumed to be null terminated
 */
_ystring str_copy (const char * data);


/**
 * Create a copy where capacity is equals to the length
 * @params: 
 *    - self: an old string
 * @warning: self is not deallocated
 */
_ystring str_fit (_ystring self);

/**
 * Create an empty string
 */
_ystring str_empty ();

#endif
