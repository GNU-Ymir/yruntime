#include <stdio.h>
#include "reflect.h"
#include "ystring.h"
#include <string.h>
#include "type.h"
#include "demangle.h"
#include "run.h"

#ifndef GC_THREADS
#define GC_THREADS
#include "gc.h"
#endif


void** __YRT_ELF_LOADER__ = 0;
void** __YRT_REFLECT_SYMBOL_TABLE__ = 0;
void** __YRT_INV_REFLECT_SYMBOL_TABLE__ = 0;


static struct ReflectSymbolTable __yrt_reflectSymbolTable__ =
{ .numberOfEntries = 0,
  .data = NULL };

static int __INDEX_TABLE_CONSTRUCTED__ = 0;

#if _WIN32

void _yrt_reflect_register_symbol_table (const char* moduleName, unsigned long long numberOfSymbols, struct ReflectSymbol* symbols) {
    unsigned long nbEntry = __yrt_reflectSymbolTable__.numberOfEntries;
    if (nbEntry == 0) {
	struct ReflectSymbolTableEntry * entry = (struct ReflectSymbolTableEntry*) malloc (sizeof (struct ReflectSymbolTableEntry));
	entry-> moduleName = moduleName; // The name is assumed to be in the text as it is called by a compiler function
	entry-> numberOfSymbols = numberOfSymbols;
	entry-> symbols = symbols;
	
	__yrt_reflectSymbolTable__.data = entry;
	__yrt_reflectSymbolTable__.numberOfEntries = 1;
    } else {
      struct ReflectSymbolTableEntry * newTable = (struct ReflectSymbolTableEntry*) malloc (sizeof (struct ReflectSymbolTableEntry) * (nbEntry + 1));
	memcpy (newTable, __yrt_reflectSymbolTable__.data, sizeof (struct ReflectSymbolTableEntry) * nbEntry);	
	newTable [nbEntry].moduleName = moduleName;
	newTable [nbEntry].numberOfSymbols = numberOfSymbols;
	newTable [nbEntry].symbols = symbols;

	free (__yrt_reflectSymbolTable__.data);
	__yrt_reflectSymbolTable__.data = newTable;
	__yrt_reflectSymbolTable__.numberOfEntries = nbEntry + 1;
    }
}

#else

void _yrt_reflect_update_index_table ();

void _yrt_reflect_update_index_table_with_elf_name (_yrt_c8_array_ elfPath);

#endif

int startsWith( const char * theString, const char * theBase ) {
    return strncmp( theString, theBase, strlen( theBase ) ) == 0;
}

struct ReflectSymbol _yrt_reflect_find_symbol_in_table_string (_ystring mangledName) {
    _yrt_c8_array_ array;
    array.len = mangledName.len;
    array.data = mangledName.data;
    
    return _yrt_reflect_find_symbol_in_table_array (array);
}

struct ReflectSymbol _yrt_reflect_find_symbol_in_table_array (_yrt_c8_array_ mangledName) {
#if _WIN32 
    if (!__INDEX_TABLE_CONSTRUCTED__) {
	_yrt_reflect_construct_index_table (__yrt_reflectSymbolTable__);
	__INDEX_TABLE_CONSTRUCTED__ = 1;	
    }
#else
    _yrt_reflect_update_index_table ();
#endif

    return _yrt_reflect_find_symbol_in_indexed_table (mangledName);   
}

struct ReflectSymbol _yrt_reflect_find_symbol_from_addr (void* addr) {
#if _WIN32
    if (!__INDEX_TABLE_CONSTRUCTED__) {
	_yrt_reflect_construct_index_table (__yrt_reflectSymbolTable__);
	__INDEX_TABLE_CONSTRUCTED__ = 1;
    }
#else
    _yrt_reflect_update_index_table ();    
#endif
    
    return _yrt_reflect_find_symbol_in_indexed_table_from_addr (addr);   
}


struct ReflectSymbol _yrt_reflect_find_symbol_from_addr_with_elf_name (void* addr, _yrt_c8_array_ filename) {
#if _WIN32
    if (!__INDEX_TABLE_CONSTRUCTED__) {
	_yrt_reflect_construct_index_table (__yrt_reflectSymbolTable__);
	__INDEX_TABLE_CONSTRUCTED__ = 1;
    }
#else
    _yrt_reflect_update_index_table_with_elf_name (filename);    
#endif
    
    return _yrt_reflect_find_symbol_in_indexed_table_from_addr (addr);   
}


/**
 * ================================================================================
 * ================================================================================
 * =========================   CLASS CONSTRUCTION     =============================
 * ================================================================================
 * ================================================================================
 */

void* _yrt_reflect_get_vtable (_yrt_c8_array_ mangledClassName, _yrt_c8_array_ className) {
    _yrt_c8_array_ name = _yrt_type_vtable_name (mangledClassName);

    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_array (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
    } else {
	_ystring name = str_create ("Could not find symbol in reflection table : vtable for ");
	name = str_fit (str_concat (name, str_create_len (className.data, className.len)));
	_yrt_c8_array_ arr;
	arr.data = name.data;
	arr.len = name.len;
	
	_yrt_throw_runtime_abort (arr);
    }

    return NULL;
}

void* _yrt_reflect_get_constructor_no_param (_yrt_c8_array_ mangledClassName, _yrt_c8_array_ className) {
    _yrt_c8_array_ name = _yrt_type_constructor_no_param_name (mangledClassName);

    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_array (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
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

    return NULL;
}

void* _yrt_reflect_create_class_from_name (_yrt_c8_array_ mangled) {
    void* vtable = _yrt_reflect_get_vtable (mangled, mangled);
    void*(*cstr)(void*) = (void*(*)(void*)) _yrt_reflect_get_constructor_no_param (mangled, mangled);

    void* classData = _yrt_alloc_class (vtable);
    return cstr (classData);
}

void* _yrt_reflect_create_class_from_name_no_construct (_yrt_c8_array_ className) {
    void* vtable = _yrt_reflect_get_vtable (className, className);    
    void* classData = _yrt_alloc_class (vtable);
    return classData;
}

void* _yrt_reflect_create_class_from_name_utf32 (_yrt_c32_array_ classNameUtf32) {
    _yrt_c8_array_ className = _yrt_to_utf8_array (classNameUtf32);
    return _yrt_reflect_create_class_from_name (className);
}

void* _yrt_reflect_create_class_from_name_no_construct_utf32 (_yrt_c32_array_ classNameUtf32) {
    _yrt_c8_array_ className = _yrt_to_utf8_array (classNameUtf32);
    return _yrt_reflect_create_class_from_name_no_construct (className);
}


/**
 * ================================================================================
 * ================================================================================
 * ==================================   FUNCTIONS     =============================
 * ================================================================================
 * ================================================================================
 */

void * _yrt_reflect_get_function (_yrt_c8_array_ mangle, _yrt_c8_array_ retName, _yrt_array_ paramNames) {
    // _yrt_c8_array_ mangle = _yrt_mangle_path (funcName);
    
    _ystring name = str_create_len (mangle.data, mangle.len);
    name = str_concat (str_create_len ("_Y", 2), name);
    name = str_concat (name, str_create_len ("F", 1));
    for (int i = 0 ; i < paramNames.len ; i++) {
	void* j = paramNames.data + i * sizeof (_yrt_c8_array_);
	_yrt_c8_array_ p_name = *(_yrt_c8_array_*)j;
	name = str_concat (name, str_create_len (p_name.data, p_name.len));
    }
    
    name = str_concat (name, str_create_len ("Z", 1));
    name = str_concat (name, str_create_len (retName.data, retName.len));

    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_string (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
    } else {
	_ystring name2 = str_create ("Could not find symbol in reflection table : symbol ");
	name2 = str_fit (str_concat (name2, str_create_len (name.data, name.len)));
	_yrt_c8_array_ arr;
	arr.data = name2.data;
	arr.len = name2.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void* _yrt_reflect_get_function_utf32 (_yrt_c32_array_ name, _yrt_c32_array_ _retType, _yrt_array_ _paramNames) {
    _yrt_c8_array_ funcName = _yrt_to_utf8_array (name);    
    _yrt_c8_array_ retType = _yrt_to_utf8_array (_retType);    
    _yrt_array_ params = _yrt_new_array (sizeof (_yrt_c8_array_*), _paramNames.len);
    for (int i = 0 ; i < _paramNames.len ; i++) {
	void* j = _paramNames.data + i * sizeof (_yrt_c32_array_);
    	_yrt_c8_array_ p_name = _yrt_to_utf8_array (*(_yrt_c32_array_*)j);
    	((_yrt_c8_array_**) params.data) [i] = (_yrt_c8_array_*) _yrt_dupl_any (&p_name, sizeof (_yrt_c8_array_));
    }
    return _yrt_reflect_get_function (funcName, retType, params);
}


/**
 * ================================================================================
 * ================================================================================
 * ==================================   METHODS   =================================
 * ================================================================================
 * ================================================================================
 */

void * _yrt_reflect_get_method (_yrt_c8_array_ mangleClassName, _yrt_c8_array_ _funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames) {
    _yrt_c8_array_ mangle = _yrt_mangle_path (_funcName);
    
    _ystring className = str_create_len (mangleClassName.data, mangleClassName.len);
    _ystring funcName = str_create_len (mangle.data, mangle.len);
    _ystring name = str_concat (str_concat (str_create_len ("_Y", 2), className), funcName);
    
    name = str_concat (name, str_create_len ("FP", 2));
    name = str_concat (name, str_from_int (className.len));
    name = str_concat (name, className);
    for (int i = 0 ; i < paramNames.len ; i++) {
	void* j = paramNames.data + i * sizeof (_yrt_c8_array_);
	_yrt_c8_array_ p_name = *(_yrt_c8_array_*)j;
	name = str_concat (name, str_create_len (p_name.data, p_name.len));
    }
    
    name = str_concat (name, str_create_len ("Z", 1));
    name = str_concat (name, str_create_len (retName.data, retName.len));
    
    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_string (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
    } else {
	_ystring name2 = str_create ("Could not find symbol in reflection table : symbol ");
	name2 = str_fit (str_concat (name2, str_create_len (name.data, name.len)));
	_yrt_c8_array_ arr;
	arr.data = name2.data;
	arr.len = name2.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void * _yrt_reflect_get_impl_method (_yrt_c8_array_ mangleClassName, _yrt_c8_array_ _funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames) {
    _yrt_c8_array_ mangle = _yrt_mangle_path (_funcName);
    
    _ystring className = str_create_len (mangleClassName.data, mangleClassName.len);
    _ystring funcName = str_create_len (mangle.data, mangle.len);
    _ystring name = str_concat (str_create_len ("_Y", 2), funcName);
    
    name = str_concat (name, str_create_len ("FP", 2));
    name = str_concat (name, str_from_int (className.len));
    name = str_concat (name, className);
    for (int i = 0 ; i < paramNames.len ; i++) {
	void* j = paramNames.data + i * sizeof (_yrt_c8_array_);
	_yrt_c8_array_ p_name = *(_yrt_c8_array_*)j;
	name = str_concat (name, str_create_len (p_name.data, p_name.len));
    }
    
    name = str_concat (name, str_create_len ("Z", 1));
    name = str_concat (name, str_create_len (retName.data, retName.len));
    
    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_string (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
    } else {
	_ystring name2 = str_create ("Could not find symbol in reflection table : symbol ");
	name2 = str_fit (str_concat (name2, str_create_len (name.data, name.len)));
	_yrt_c8_array_ arr;
	arr.data = name2.data;
	arr.len = name2.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void* _yrt_reflect_get_method_utf32 (_yrt_c32_array_ _className, _yrt_c32_array_ name, _yrt_c32_array_ _retType, _yrt_array_ _paramNames) {
    _yrt_c8_array_ className = _yrt_to_utf8_array (_className);
    _yrt_c8_array_ funcName = _yrt_to_utf8_array (name);    
    _yrt_c8_array_ retType = _yrt_to_utf8_array (_retType);    
    _yrt_array_ params = _yrt_new_array (sizeof (_yrt_c8_array_*), _paramNames.len);
    for (int i = 0 ; i < _paramNames.len ; i++) {
	void* j = _paramNames.data + i * sizeof (_yrt_c32_array_);
    	_yrt_c8_array_ p_name = _yrt_to_utf8_array (*(_yrt_c32_array_*)j);
    	((_yrt_c8_array_**) params.data) [i] = (_yrt_c8_array_*) _yrt_dupl_any (&p_name, sizeof (_yrt_c8_array_));
    }
    return _yrt_reflect_get_method (className, funcName, retType, params);
}


void * _yrt_reflect_get_method_mutable (_yrt_c8_array_ mangleClassName, _yrt_c8_array_ _funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames) {
    _yrt_c8_array_ mangle = _yrt_mangle_path (_funcName);
    
    _ystring className = str_create_len (mangleClassName.data, mangleClassName.len);
    _ystring funcName = str_create_len (mangle.data, mangle.len);
    _ystring name = str_concat (str_concat (str_create_len ("_Y", 2), className), funcName);
    
    name = str_concat (name, str_create_len ("FxP", 3));
    name = str_concat (name, str_from_int (className.len + 1)); // +1 because we add an x 
    name = str_concat (name, str_create_len ("x", 1));

    name = str_concat (name, className);

    for (int i = 0 ; i < paramNames.len ; i++) {
	void* j = paramNames.data + i * sizeof (_yrt_c8_array_);
	_yrt_c8_array_ p_name = *(_yrt_c8_array_*)j;
	name = str_concat (name, str_create_len (p_name.data, p_name.len));
    }
    
    name = str_concat (name, str_create_len ("Z", 1));
    name = str_concat (name, str_create_len (retName.data, retName.len));
    
    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_string (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
    } else {
	_ystring name2 = str_create ("Could not find symbol in reflection table : symbol ");
	name2 = str_fit (str_concat (name2, str_create_len (name.data, name.len)));
	_yrt_c8_array_ arr;
	arr.data = name2.data;
	arr.len = name2.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}

void * _yrt_reflect_get_impl_method_mutable (_yrt_c8_array_ mangleClassName, _yrt_c8_array_ _funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames) {
    _yrt_c8_array_ mangle = _yrt_mangle_path (_funcName);
    
    _ystring className = str_create_len (mangleClassName.data, mangleClassName.len);
    _ystring funcName = str_create_len (mangle.data, mangle.len);
    _ystring name = str_concat (str_create_len ("_Y", 2), funcName);
    
    name = str_concat (name, str_create_len ("FxP", 3));
    name = str_concat (name, str_from_int (className.len + 1)); // +1 because we add an x 
    name = str_concat (name, str_create_len ("x", 1));

    name = str_concat (name, className);

    for (int i = 0 ; i < paramNames.len ; i++) {
	void* j = paramNames.data + i * sizeof (_yrt_c8_array_);
	_yrt_c8_array_ p_name = *(_yrt_c8_array_*)j;
	name = str_concat (name, str_create_len (p_name.data, p_name.len));
    }
    
    name = str_concat (name, str_create_len ("Z", 1));
    name = str_concat (name, str_create_len (retName.data, retName.len));
    
    struct ReflectSymbol sym = _yrt_reflect_find_symbol_in_table_string (name);
    if (sym.ptr != NULL) {
	return sym.ptr;
    } else {
	_ystring name2 = str_create ("Could not find symbol in reflection table : symbol ");
	name2 = str_fit (str_concat (name2, str_create_len (name.data, name.len)));
	_yrt_c8_array_ arr;
	arr.data = name2.data;
	arr.len = name2.len;
	
	_yrt_throw_runtime_abort (arr);
    }
}


void* _yrt_reflect_get_method_mutable_utf32 (_yrt_c32_array_ _className, _yrt_c32_array_ name, _yrt_c32_array_ _retType, _yrt_array_ _paramNames) {
    _yrt_c8_array_ className = _yrt_to_utf8_array (_className);
    _yrt_c8_array_ funcName = _yrt_to_utf8_array (name);    
    _yrt_c8_array_ retType = _yrt_to_utf8_array (_retType);    
    _yrt_array_ params = _yrt_new_array (sizeof (_yrt_c8_array_*), _paramNames.len);
    for (int i = 0 ; i < _paramNames.len ; i++) {
	void* j = _paramNames.data + i * sizeof (_yrt_c32_array_);
    	_yrt_c8_array_ p_name = _yrt_to_utf8_array (*(_yrt_c32_array_*)j);
    	((_yrt_c8_array_**) params.data) [i] = (_yrt_c8_array_*) _yrt_dupl_any (&p_name, sizeof (_yrt_c8_array_));
    }
    return _yrt_reflect_get_method_mutable (className, funcName, retType, params);
}
