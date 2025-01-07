#include <stdio.h>
#include "edbdef.h"
#include "src/uthash.h"

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

int IndexAddINT(IndexINTNode** head, edb_int inKey, void* data);
int IndexAddTEXT(IndexTEXTNode** head, char* inKey, void* data);
int IndexFindINT(IndexINTNode** head, edb_int inKey, void** findResult, size_t len);
int IndexFindTEXT(IndexTEXTNode** head, char* inKey, void** findResult, size_t len);
int IndexDelINT(IndexINTNode** head, edb_int inKey, void* data_ptr);
int IndexDelTEXT(IndexTEXTNode** head, char* inKey, void* data_ptr);