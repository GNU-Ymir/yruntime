#pragma once

struct VTable;
struct TypeInfo {    
    VTable* vtable;
    void* ig1;
    void* ig2;
};
  
struct VTable {
    TypeInfo* typeinfo;
    bool (*equals)(void*, TypeInfo);
};
