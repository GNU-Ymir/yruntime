#pragma once

struct VTable;
struct TypeInfo {
    VTable* vtable;
    void* ig1;
    void* ig2;
};
  
struct VTable {
    bool (*equals)(void*, TypeInfo);
};
