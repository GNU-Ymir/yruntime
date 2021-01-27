#include <dlfcn.h>
#include <stdio.h>
#include "../include/reflect.h"
#include "../include/ystring.h"
#include <unistd.h>     // getpid
#include <string.h>
#include "../include/type.h"
#include "../include/demangle.h"

#ifndef GC_THREADS
#define GC_THREADS
#include <gc/gc.h>
#endif

void _yrt_throw_runtime_abort (_yrt_c8_array_ str);

ELFContent _yrt_elf_empty () {
    ELFContent ret;
    ret.string_table_size = 0;
    ret.string_table = NULL;
    ret.elf_size = 0;
    ret.elf_data = NULL;
    ret.elf_header = NULL;
    ret.symbol_table = NULL;
    ret.symbol_table_size = 0;
    return ret;
}

ELFContent _yrt_elf_read (FILE * elfFile) {
    ELFContent elf = _yrt_elf_empty ();
    
    if (fseek (elfFile, 0, SEEK_END)) {
	_ystring str = str_create_len ("Could not read ELF file", 23);
	_yrt_c8_array_ arr;
	arr.data = str.data;
	arr.len = str.len;
	
	_yrt_throw_runtime_abort (arr); // make the program abort
    }

    // Copy the elf file in program space
    elf.elf_size = ftell (elfFile);
    elf.elf_data = mmap (NULL, elf.elf_size, PROT_READ, MAP_PRIVATE, fileno (elfFile), 0);
    if (elf.elf_data == NULL) {
	_ystring str = str_create_len ("Could not read ELF file", 23);
	_yrt_c8_array_ arr;
	arr.data = str.data;
	arr.len = str.len;
	
	_yrt_throw_runtime_abort (arr); // make the program abort
    }

    // Load and verify elf header
    elf.elf_header = (Elf64_Ehdr*) elf.elf_data;
    const uint8_t magic_header[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};    
    if (memcmp (elf.elf_header, magic_header, sizeof (magic_header)) != 0) {
	_ystring str = str_create_len ("Could not read ELF file, wrong format", 37);
	_yrt_c8_array_ arr;
	arr.data = str.data;
	arr.len = str.len;
	
	_yrt_throw_runtime_abort (arr); // make the program abort
    }

    int nb_str_tabs = 0;
    
    // Locate the symbol table
    for (uint16_t i = 0; i < elf.elf_header-> e_shnum; i++) {
	size_t offset = elf.elf_header-> e_shoff + (i * (elf.elf_header-> e_shentsize));
	Elf64_Shdr * section_header = (Elf64_Shdr*) (elf.elf_data + offset);
	switch (section_header-> sh_type) {
	case SHT_SYMTAB :
	    elf.symbol_table = elf.elf_data + section_header-> sh_offset;
	    elf.symbol_table_size = section_header-> sh_size;
	    break;
	
	case SHT_STRTAB:
	    if (nb_str_tabs + 1 > elf.string_table_size) {
		elf.string_table_size += 1;
		Elf64_Shdr** aux = GC_malloc (elf.string_table_size * sizeof (Elf64_Shdr*));
		for (int j = 0 ; j < elf.string_table_size - 1; j++)
		    aux [j] = elf.string_table [j];
		elf.string_table = aux;
	    }
	    
	    elf.string_table [nb_str_tabs] = section_header;	    
	    nb_str_tabs += 1;	    
	    break;
	
	default: break;
	}
    }
    return elf;
}

_ystring _yrt_elf_name_of_symbol (ELFContent elf, Elf64_Sym* symbol) {
    if (symbol-> st_name != 0) {
	char * name = NULL;
	for (int i = 0 ; i < elf.string_table_size; i++) {
	    void * table_start = elf.elf_data + elf.string_table [i]-> sh_offset;
	    size_t table_size = elf.string_table [i]-> sh_size;
	    if (symbol-> st_name  < table_size) {
		name = (char*) table_start + symbol-> st_name;
		return str_copy (name);
	    }
	}     
    }
    return str_empty ();
}

int _yrt_elf_name_of_symbol_is (ELFContent elf, Elf64_Sym* symbol, const char * name) {
    if (symbol-> st_name != 0) {
	char * fname = NULL;
	for (int i = 0 ; i < elf.string_table_size; i++) {
	    void * table_start = elf.elf_data + elf.string_table [i]-> sh_offset;
	    size_t table_size = elf.string_table [i]-> sh_size;
	    if (symbol-> st_name  < table_size) {
		fname = (char*) table_start + symbol-> st_name;
		return strcmp (fname, name) == 0;
	    }
	}     
    }
    return -1;
}

void _yrt_elf_print_symbol_table (ELFContent elf) {
    Elf64_Sym** symbols = elf.symbol_table;
    for (uint16_t i = 0 ; i * sizeof (Elf64_Sym) < elf.symbol_table_size; i++) {
	size_t offset = i * sizeof (Elf64_Sym);
	Elf64_Sym* symbol = (Elf64_Sym*) (elf.symbol_table + offset);
	switch (ELF64_ST_TYPE (symbol-> st_info)) {
	case STT_NOTYPE: printf ("STT_NOTYPE "); break;
        case STT_OBJECT: printf ("STT_OBJECT "); break;
        case STT_FUNC: printf ("STT_FUNC "); break;
        case STT_SECTION: printf ("STT_SECTION "); break;
        case STT_FILE: printf ("STT_FILE "); break;
        case STT_COMMON: printf ("STT_COMMON "); break;
        case STT_TLS: printf ("STT_TLS "); break;
        case STT_NUM: printf ("STT_NUM "); break;
        case STT_LOOS: printf ("STT_LOOS "); break;
        case STT_HIOS: printf ("STT_HIOS "); break;
        case STT_LOPROC: printf ("STT_LOPROC "); break;
        case STT_HIPROC: printf ("STT_HIPROC "); break;
        default:
            printf ("%d ", ELF64_ST_TYPE(symbol->st_info));
        }
    
	_ystring name = _yrt_elf_name_of_symbol (elf, symbol);
	if (name.len != 0) {
	    for (int i = 0 ; i < name.len ; i++) {
		putchar (name.data [i]);
	    }
	}
	printf (" %d \n", i);	
    }    
}

Elf64_Sym* _yrt_elf_find_function_in_table (ELFContent elf, const char * name) {
    Elf64_Sym** symbols = elf.symbol_table;
    for (uint16_t i = 0 ; i * sizeof (Elf64_Sym) < elf.symbol_table_size; i++) {
	size_t offset = i * sizeof (Elf64_Sym);
	Elf64_Sym* symbol = (Elf64_Sym*) (elf.symbol_table + offset);
	if (ELF64_ST_TYPE (symbol-> st_info) == STT_FUNC) {
	    if (_yrt_elf_name_of_symbol_is (elf, symbol, name)) { 
		return symbol;
	    }
	}
    }    
    return NULL;
}

Elf64_Sym* _yrt_elf_find_object_in_table (ELFContent elf, const char * name) {
    Elf64_Sym** symbols = elf.symbol_table;
    for (uint16_t i = 0 ; i * sizeof (Elf64_Sym) < elf.symbol_table_size; i++) {
	size_t offset = i * sizeof (Elf64_Sym);
	Elf64_Sym* symbol = (Elf64_Sym*) (elf.symbol_table + offset);
	if (ELF64_ST_TYPE (symbol-> st_info) == STT_OBJECT) {
	    if (_yrt_elf_name_of_symbol_is (elf, symbol, name)) { 
		return symbol;
	    }
	}
    }    
    return NULL;
}

void _yrt_elf_close (FILE * file) {
    fclose (file);
}

const char* _yrt_get_executable_path (char * filepath) {
    pid_t pid = getpid();
    sprintf(filepath, "/proc/%d/exe", pid);
    return filepath;
}

const char* _yrt_elf_call_function (Elf64_Sym* symbol) {
    void (*func) (void) = (void(*)()) symbol-> st_value;
    func ();
}

_ytype_info _yrt_elf_get_typeinfo (ELFContent elf, _yrt_c8_array_ mangledClassName, _yrt_c8_array_ className) {
    _yrt_c8_array_ name = _yrt_type_typeinfo_name (mangledClassName);
    Elf64_Sym * sym = _yrt_elf_find_object_in_table (elf, name.data);
    if (sym != NULL) {
	return *(_ytype_info*) (sym-> st_value);
    } else {
	_ystring name = str_create ("Could not find in ELF file : typeinfo for ");
	name = str_fit (str_concat (name, str_create_len (className.data, className.len)));
	_yrt_c8_array_ arr;
	arr.data = name.data;
	arr.len = name.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void* _yrt_elf_get_vtable (ELFContent elf, _yrt_c8_array_ mangledClassName, _yrt_c8_array_ className) {
    _yrt_c8_array_ name = _yrt_type_vtable_name (mangledClassName);
    Elf64_Sym * sym = _yrt_elf_find_object_in_table (elf, name.data);
    if (sym != NULL) {
	return (void*) sym-> st_value;
    } else {
	_ystring name = str_create ("Could not find in ELF file : vtable for ");
	name = str_fit (str_concat (name, str_create_len (className.data, className.len)));
	_yrt_c8_array_ arr;
	arr.data = name.data;
	arr.len = name.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void* _yrt_elf_get_constructor_no_param (ELFContent elf, _yrt_c8_array_ mangledClassName, _yrt_c8_array_ className) {
    _yrt_c8_array_ name = _yrt_type_constructor_no_param_name (mangledClassName);
    Elf64_Sym * sym = _yrt_elf_find_function_in_table (elf, name.data);    
    if (sym != NULL) {
	return (void*) sym-> st_value;
    } else {
	_ystring name = str_create_len ("Class ", 6);
	name = str_concat (name, str_create_len (className.data, className.len));
	name = str_concat (name, str_from_char ('\0'));
	name = str_fit (str_concat (name, str_create_len (" has no default constructor", 27)));
	_yrt_c8_array_ arr;
	arr.data = name.data;
	arr.len = name.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void* _yrt_create_class_from_name (_yrt_c8_array_ className) {
    char path [64];
    FILE * file = fopen (_yrt_get_executable_path (path), "r");
    ELFContent elf = _yrt_elf_read (file);
    
    _yrt_c8_array_ mangled = _yrt_mangle_class_name (className);
    _ytype_info info = _yrt_elf_get_typeinfo (elf, mangled, className);
    void* vtable = _yrt_elf_get_vtable (elf, mangled, className);
    void*(*cstr)(void*) = (void*(*)(void*)) _yrt_elf_get_constructor_no_param (elf, mangled, className);

    void* classData = _yrt_alloc_class (info.size);
    *((void**)classData) = vtable;
    return cstr (classData);
}

void* _yrt_create_class_from_name_utf32 (_yrt_c32_array_ classNameUtf32) {
    _yrt_c8_array_ className = _yrt_to_utf8_array (classNameUtf32);
    return _yrt_create_class_from_name (className);
}
