#include <stdio.h>
#include "src/uthash.h"

#ifndef INDEX
#define INDEX
#define NODE_NOT_EXIST -1

typedef struct SetNode
{
    void* data;
    UT_hash_handle hh;
}SetNode;

typedef struct IndexNode
{
    void* key;
    size_t keyLenth;
    SetNode* setHead;
    UT_hash_handle hh;
}IndexNode;


int IndexInsert(IndexNode** head, void* inKey, size_t keyLenth, void* data);
size_t IndexFind(IndexNode** head, void* inKey, size_t keyLenth, void** findResults, size_t len);
int IndexDel(IndexNode** head, void* inKey, size_t keyLenth, void* data_ptr);
int IndexClear(IndexNode** head);

#endif