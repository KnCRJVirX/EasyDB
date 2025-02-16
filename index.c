#include "index.h"

int IndexInsert(IndexNode** head, void* inKey, size_t keyLenth, void* data)
{
    SetNode* newSetNode = (SetNode*)malloc(sizeof(SetNode));
    newSetNode->data = data;
    IndexNode* findNode = NULL;
    HASH_FIND(hh, *head, inKey, keyLenth, findNode);
    if (findNode == NULL)
    {
        IndexNode* newIndexNode = (IndexNode*)malloc(sizeof(IndexNode));
        newIndexNode->key = inKey;
        newIndexNode->keyLenth = keyLenth;
        newIndexNode->data = NULL;
        HASH_ADD_KEYPTR(hh, newIndexNode->data, &newSetNode->data, sizeof(void*), newSetNode);
        HASH_ADD_KEYPTR(hh, *head, newIndexNode->key, newIndexNode->keyLenth, newIndexNode);
    }
    else
    {
        HASH_ADD_KEYPTR(hh, findNode->data, &newSetNode->data, sizeof(void*), newSetNode);
    }
    return 0;
}

size_t IndexFind(IndexNode** head, void* inKey, size_t keyLenth, void** findResults, size_t len)
{
    IndexNode* findNode = NULL;

    HASH_FIND(hh, *head, inKey, keyLenth, findNode);
    size_t count = 0;
    if (findNode == NULL)
    {
        return 0;
    }
    else
    {
        if (findResults == NULL || len == 0)
        {
            return HASH_COUNT(findNode->data);
        }
        SetNode *ptr1 = NULL, *ptr2 = NULL;
        HASH_ITER(hh, findNode->data, ptr1, ptr2){
            if (findResults && count < len)
            {
                findResults[count] = ptr1->data;
            }
            ++count;
        }
        return count;
    }
    return 0;
}

int IndexDel(IndexNode** head, void* inKey, size_t keyLenth, void* data_ptr)
{
    IndexNode* findNode = NULL;
    HASH_FIND(hh, *head, inKey, keyLenth, findNode);
    if (findNode == NULL)
    {
        return NODE_NOT_EXIST;
    }
    else
    {
        SetNode* findSetNode = NULL;
        HASH_FIND(hh, findNode->data, &data_ptr, sizeof(data_ptr), findSetNode);
        if (findSetNode == NULL)
        {
            return NODE_NOT_EXIST;
        }
        else
        {
            HASH_DEL(findNode->data, findSetNode);
            free(findSetNode);
        }
    }
    
    return 0;
}

int IndexClear(IndexNode** head)
{
    IndexNode *ptr1, *ptr2;
    HASH_ITER(hh, *head, ptr1, ptr2){
        SetNode *ptr3 = NULL, *ptr4 = NULL;
        HASH_ITER(hh, ptr1->data, ptr3, ptr4){
            HASH_DEL(ptr1->data, ptr3);
            free(ptr3);
        }
        HASH_DEL(*head, ptr1);
        free(ptr1);
    }
    return 0;
}