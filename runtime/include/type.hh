#pragma once

struct VTable;
struct TypeInfo {    
    VTable* vtable;
    void* _vtable;
    unsigned long len;
    TypeInfo * c_o_a;
};
  
struct VTable {
    TypeInfo* typeinfo;
    bool (*equals)(void*, TypeInfo);
};
