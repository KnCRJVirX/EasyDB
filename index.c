#include "index.h"

int IndexInsertINT(IndexINTNode** head, edb_int inKey, void* data)
{
    IndexINTNode* newNode = (IndexINTNode*)malloc(sizeof(IndexINTNode));
    newNode->key = inKey;
    newNode->data = data;
    newNode->next = NULL;
    IndexINTNode* findNode = NULL;
    HASH_FIND_INT(*head, &inKey, findNode);
    if (findNode == NULL)
    {
        HASH_ADD_INT(*head, key, newNode);
    }
    else
    {
        newNode->next = findNode->next;
        findNode->next = newNode;
    }
    return 0;
}

int IndexInsertTEXT(IndexTEXTNode** head, char* inKey, void* data)
{
    IndexTEXTNode* newNode = (IndexTEXTNode*)malloc(sizeof(IndexTEXTNode));
    newNode->key = inKey;
    newNode->data = data;
    newNode->next = NULL;
    IndexTEXTNode* findNode = NULL;
    HASH_FIND_STR(*head, inKey, findNode);
    if (findNode == NULL)
    {
        HASH_ADD_KEYPTR(hh, *head, newNode->key, strlen(newNode->key), newNode);
    }
    else
    {
        newNode->next = findNode->next;
        findNode->next = newNode;
    }
    return 0;
}

// int IndexInsert(IndexNode** head, void* inKey, size_t keyLenth, void* data)
// {
    
//     IndexNode* newNode = (IndexNode*)malloc(sizeof(IndexNode));
//     newNode->key = inKey;
//     newNode->data = data;
//     newNode->next = NULL;
//     IndexTEXTNode* findNode = NULL;
//     HASH_FIND_STR(*head, inKey, findNode);
//     if (findNode == NULL)
//     {
//         HASH_ADD_STR(*head, key, newNode);
//     }
//     else
//     {
//         findNode->next = newNode;
//     }
//     return 0;
// }

size_t IndexFindINT(IndexINTNode** head, edb_int inKey, void** findResults, size_t len)
{
    IndexINTNode* findNode = NULL;
    HASH_FIND_INT(*head, &inKey, findNode);
    size_t count = 0;
    if (findNode == NULL)
    {
        return 0;
    }
    else
    {
        if (findResults == NULL || len == 0) return 1;
        while (findNode != NULL && count < len)
        {
            findResults[count++] = findNode->data;
            findNode = findNode->next;
        }
        return count;
    }
    return 0;
}

size_t IndexFindTEXT(IndexTEXTNode** head, char* inKey, void** findResults, size_t len)
{
    IndexTEXTNode* findNode = NULL;
    HASH_FIND_STR(*head, inKey, findNode);
    size_t count = 0;
    if (findNode == NULL)
    {
        return 0;
    }
    else
    {
        if (findResults == NULL || len == 0) return 1;
        while (findNode != NULL && count < len)
        {
            findResults[count++] = findNode->data;
            findNode = findNode->next;
        }
        return count;
    }
    return 0;
}

int IndexDelINT(IndexINTNode** head, edb_int inKey, void* data_ptr)
{
    IndexINTNode* findNode = NULL;
    HASH_FIND_INT(*head, &inKey, findNode);
    if (findNode == NULL)
    {
        return NODE_NOT_EXIST;
    }
    else if (findNode->next == NULL)
    {
        HASH_DEL(*head, findNode);
        free(findNode);
        return 0;
    }
    else
    {
        if (findNode->data == data_ptr)
        {
            IndexINTNode* newNode = findNode->next;
            HASH_DEL(*head, findNode);
            HASH_ADD_INT(*head, key, newNode);
            return 0;
        }
        while (findNode->next != NULL)
        {
            if (findNode->next->data == data_ptr)
            {
                IndexINTNode* delNode = findNode->next;
                findNode->next = findNode->next->next;
                free(delNode);
                return 0;
            }
            findNode = findNode->next;
        }
    }
    return 0;
}

int IndexDelTEXT(IndexTEXTNode** head, char* inKey, void* data_ptr)
{
    IndexTEXTNode* findNode = NULL;
    HASH_FIND_STR(*head, inKey, findNode);
    if (findNode == NULL)
    {
        return NODE_NOT_EXIST;
    }
    else if (findNode->next == NULL)
    {
        HASH_DEL(*head, findNode);
        free(findNode);
        return 0;
    }
    else
    {
        if (findNode->data == data_ptr)
        {
            IndexTEXTNode* newNode = findNode->next;
            HASH_DEL(*head, findNode);
            HASH_ADD_KEYPTR(hh, *head, newNode->key, strlen(newNode->key), newNode);
            return 0;
        }
        while (findNode->next != NULL)
        {
            if (findNode->next->data == data_ptr)
            {
                IndexTEXTNode* delNode = findNode->next;
                findNode->next = findNode->next->next;
                free(delNode);
                return 0;
            }
            findNode = findNode->next;
        }
    }
    return 0;
}

int IndexClearINT(IndexINTNode** head)
{
    IndexINTNode *ptr1, *ptr2;
    IndexINTNode *pre, *ptr;
    HASH_ITER(hh, *head, ptr1, ptr2){
        HASH_DEL(*head, ptr1);
        ptr = pre = ptr1;
        while (ptr != NULL)
        {
            ptr = ptr->next;
            free(pre);
            pre = ptr;
        }
    }
    return 0;
}

int IndexClearTEXT(IndexTEXTNode** head)
{
    IndexTEXTNode *ptr1, *ptr2;
    IndexTEXTNode *pre, *ptr;
    HASH_ITER(hh, *head, ptr1, ptr2){
        HASH_DEL(*head, ptr1);
        ptr = pre = ptr1;
        while (ptr != NULL)
        {
            ptr = ptr->next;
            free(pre);
            pre = ptr;
        }
    }
    return 0;
}