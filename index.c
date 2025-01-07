#include "index.h"

int IndexAddINT(IndexINTNode** head, edb_int inKey, void* data)
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
        findNode->next = newNode;
    }
    return 0;
}

int IndexAddTEXT(IndexTEXTNode** head, char* inKey, void* data)
{
    IndexTEXTNode* newNode = (IndexTEXTNode*)malloc(sizeof(IndexTEXTNode));
    newNode->key = inKey;
    newNode->data = data;
    newNode->next = NULL;
    IndexTEXTNode* findNode = NULL;
    HASH_FIND_STR(*head, inKey, findNode);
    if (findNode == NULL)
    {
        HASH_ADD_STR(*head, key, newNode);
    }
    else
    {
        findNode->next = newNode;
    }
    return 0;
}

int IndexFindINT(IndexINTNode** head, edb_int inKey, void** findResult, size_t len)
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
        if (findResult == NULL || len == 0) return 1;
        while (findNode != NULL && count < len)
        {
            findResult[count++] = findNode->data;
            findNode = findNode->next;
        }
        return count;
    }
    return 0;
}

int IndexFindTEXT(IndexTEXTNode** head, char* inKey, void** findResult, size_t len)
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
        if (findResult == NULL || len == 0) return 1;
        while (findNode != NULL && count < len)
        {
            findResult[count++] = findNode->data;
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
            HASH_ADD_INT(*head, key, newNode);
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