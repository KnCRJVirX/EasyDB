#include <stdio.h>
#include "edbdef.h"
#include "src/uthash.h"

#ifndef INDEX
#define INDEX
#define NODE_NOT_EXIST -1

typedef struct IndexINTNode
{
    long long key;
    void* data;
    struct IndexINTNode* next;
    UT_hash_handle hh;
}IndexINTNode;

typedef struct IndexTEXTNode
{
    char* key;
    void* data;
    struct IndexTEXTNode* next;
    UT_hash_handle hh;
}IndexTEXTNode;

typedef struct IndexNode
{
    void* key;
    void* data;
    struct IndexNode* next;
    UT_hash_handle hh;
}IndexNode;

int IndexInsertINT(IndexINTNode** head, edb_int inKey, void* data);
int IndexInsertTEXT(IndexTEXTNode** head, char* inKey, void* data);
// int IndexInsert(IndexNode** head, void* inKey, size_t keyLenth, void* data);
size_t IndexFindINT(IndexINTNode** head, edb_int inKey, void** findResult, size_t len);
size_t IndexFindTEXT(IndexTEXTNode** head, char* inKey, void** findResult, size_t len);
int IndexDelINT(IndexINTNode** head, edb_int inKey, void* data_ptr);
int IndexDelTEXT(IndexTEXTNode** head, char* inKey, void* data_ptr);

#endif