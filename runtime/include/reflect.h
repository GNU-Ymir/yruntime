#ifndef _Y_REFLECT_H_
#define _Y_REFLECT_H_

#include <elf.h>
#include <sys/mman.h>
#include "yarray.h"
#include "type.h"

/**
 * This structure contains the elf content of an executable
 * This will be used to find the vtables of each types, and then be able to construct them from string name
 */
typedef struct {
    size_t string_table_size;
    Elf64_Shdr* string_table;
    size_t elf_size;
    void* elf_data;
    Elf64_Ehdr* elf_header;
    void* symbol_table;
    size_t symbol_table_size;
} ELFContent;


/**
 * Read a file in elf format and return the information
 * @info: All the allocation are made using the garbage collector
 */
ELFContent _yrt_read_elf (FILE * elfFile);

/**
 * Clean the elf file 
 * if everything was done correctly only one or none ELFContent exists, this will delete it if it exists
 */
void _yrt_elf_clean ();

/**
 * Get the vtable address from its name
 */
_ytype_info  _yrt_reflect_vtable (const _yrt_c8_array_ name);

/** 
 * @return the path of the current executable
 * @info /proc/pid/exe
 * @returns: 
 *    - name: a buffer in which the filepath will be written
 */
const char* _yrt_get_executable_path (char* name);

#endif
