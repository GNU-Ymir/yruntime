#ifndef _Y_REFLECT_H_
#define _Y_REFLECT_H_

#include "yarray.h"
#include "type.h"

/**
 * 
 */
enum ReflectSymbolType {
    FUNCTION = 0,
    VTABLE
};

/**
 * A symbol in the symbol table of reflection info
 */
struct ReflectSymbol {
    // The type of symbol (function, vtable, etc.)
    enum ReflectSymbolType type;
    
    // The pointer to that symbol
    void * ptr;

    // The name of the symbol
    char* name;

    // The name of the file that defined that symbol (warning: set only in debug mode)
    char* locFile;

    // The line location of the symbol in the source file (warning: set only in debug mode)
    unsigned int locLine;
};

/**
 * This is the content of a symbol table entry
 */
struct ReflectSymbolTableEntry {
    // The name of the symbol table 
    const char* moduleName;

    // The number of registered symbols
    unsigned long long numberOfSymbols;

    // The list of symbols
    struct ReflectSymbol* symbols;
};

/**
 * A symbol table
 */
struct ReflectSymbolTable {
    // The number of sub symbol tables
    unsigned long numberOfEntries;

    // The data to the sub symbol tables
    struct ReflectSymbolTableEntry * data;
};


/**
 * Function called by the compiled program to register a symbol table
 * @params:
 *     - moduleName: the module registering the symbol table (all inner symbols should start with the module name)
 *     - nbSymbols: the number of symbols in the table
 *     - symbolTable: the content of the symbol table
 */
void _yrt_reflect_register_symbol_table (const char* moduleName, unsigned long long nbSymbols, struct ReflectSymbol* symbolTable);

/**
 * Find a symbol in the reflect module symbol table whose name is exactly mangledName
 * @returns: the symbol or NULL if not found
 */
struct ReflectSymbol* _yrt_reflect_find_symbol_in_module (_ystring mangledName, struct ReflectSymbolTableEntry entry);

/**
 * Find a symbol in the reflect symbol table whose name is exactly mangledName
 * @returns: the symbol or NULL if not found
 */
struct ReflectSymbol* _yrt_reflect_find_symbol_in_table (_ystring mangledName);


/**
 * Find a symbol in the reflect symbol table whose name is exactly mangledName
 * @returns: the symbol or NULL if not found
 */
struct ReflectSymbol* _yrt_reflect_find_symbol_in_table_array (_yrt_c8_array_ mangledName);


/**
 * Find a symbol in the reflect module symbol table whose name is exactly mangledName
 * @returns: the symbol or NULL if not found
 */
struct ReflectSymbol* _yrt_reflect_find_symbol_in_module_array (_yrt_c8_array_ mangledName, struct ReflectSymbolTableEntry entry);


/**
 * Search for the vtable of the class in the registered symbol tables
 * @params:
 *    - mangledClassName: the name of the class mangled
 *    - className: the same name but demangled (for error throwing)
 * @returns: the pointer to the vtable
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_vtable (_yrt_c8_array_ mangledClassName, _yrt_c8_array_ className);

/**
 * Search for a constructor with no parameters for the class
  * @params:
 *    - mangledClassName: the name of the class mangled
 *    - className: the same name but demangled (for error throwing)
 * @returns: the pointer to the constructor
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_constructor_no_param (_yrt_c8_array_ mangledClassName, _yrt_c8_array_ className);


/**
 * Create and construct a class form its mangled nam
 * @params:
 *    - mangledName: the mangledName of the class to construct
 * @returns: a pointer to the constructed class
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_create_class_from_name (_yrt_c8_array_ mangledName);


/**
 * Create and construct a class form its mangled name (in utf32)
 * @params:
 *    - mangledName: the mangledName of the class to construct
 * @returns: a pointer to the constructed class
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_create_class_from_name_utf32 (_yrt_c32_array_ mangledName);

/**
 * Create a class but does not construct it (not calling the constructor)
 * @params:
 *   - mangledName: the mangled name of the class
 * @returns: a pointer to the class (not constructed)
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_create_class_from_name_no_construct (_yrt_c8_array_ mangledName);


/**
 * Create a class but does not construct it (not calling the constructor) with a name in utf32
 * @params:
 *   - mangledName: the mangled name of the class in utf32
 * @returns: a pointer to the class (not constructed)
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_create_class_from_name_no_construct_utf32 (_yrt_c32_array_ mangledName);


/**
 * Find a function in the reflect table
 * @params:
 *   - funcName: the name of the function (unmangled)
 *   - retName: the return type (mangled)
 *   - paramNames: the type of the parameters (mangled)
 * @returns: the pointer to the function
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_function (_yrt_c8_array_ funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames);


/**
 * Find a function in the reflect table in utf32
 * @params:
 *   - funcName: the name of the function (unmangled) in utf32
 *   - retName: the return type (mangled) in utf32
 *   - paramNames: the type of the parameters (mangled) in utf32
 * @returns: the pointer to the function
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_function_utf32 (_yrt_c32_array_ funcName, _yrt_c32_array_ retName, _yrt_array_ paramNames);


/**
 * Find a immutable method in reflect table
 * @params:
 *   - mangledClassName: the mangle name of the class
 *   - funcName: the name of the function (not mangled)
 *   - retName: the mangled name of the return type
 *   - paramNames: the mangled names of the parameters
 * @returns: the pointer to the method
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_method (_yrt_c8_array_ mangleClassName, _yrt_c8_array_ _funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames);

/**
 * Find a immutable method in reflect table in utf32
 * @params:
 *   - mangledClassName: the mangle name of the class
 *   - funcName: the name of the function (not mangled)
 *   - retName: the mangled name of the return type
 *   - paramNames: the mangled names of the parameters
 * @returns: the pointer to the method
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_method_utf32 (_yrt_c32_array_ mangleClassName, _yrt_c32_array_ _funcName, _yrt_c32_array_ retName, _yrt_array_ paramNames);


/**
 * Find a mutable method in reflect table
 * @params:
 *   - mangledClassName: the mangle name of the class
 *   - funcName: the name of the function (not mangled)
 *   - retName: the mangled name of the return type
 *   - paramNames: the mangled names of the parameters
 * @returns: the pointer to the method
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_method_mutable (_yrt_c8_array_ mangleClassName, _yrt_c8_array_ _funcName, _yrt_c8_array_ retName, _yrt_array_ paramNames);

/**
 * Find a mutable method in reflect table in utf32
 * @params:
 *   - mangledClassName: the mangle name of the class
 *   - funcName: the name of the function (not mangled)
 *   - retName: the mangled name of the return type
 *   - paramNames: the mangled names of the parameters
 * @returns: the pointer to the method
 * @throws: _yrt_throw_runtime_abort
 */
void* _yrt_reflect_get_method_mutable_utf32 (_yrt_c32_array_ mangleClassName, _yrt_c32_array_ _funcName, _yrt_c32_array_ retName, _yrt_array_ paramNames);


#endif
