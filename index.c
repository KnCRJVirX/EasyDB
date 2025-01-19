#include "index.h"

int IndexInsert(IndexNode** head, void* inKey, size_t keyLenth, void* data)
{
    
    IndexNode* newNode = (IndexNode*)malloc(sizeof(IndexNode));
    newNode->key = inKey;
    newNode->keyLenth = keyLenth;
    newNode->data = data;
    newNode->next = NULL;
    IndexNode* findNode = NULL;
    HASH_FIND(hh, *head, newNode->key, keyLenth, findNode);
    if (findNode == NULL)
    {
        HASH_ADD_KEYPTR(hh, *head, newNode->key, keyLenth, newNode);
    }
    else
    {
        newNode->next = findNode->next;
        findNode->next = newNode;
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
        while (findNode != NULL && (count < len || len == 0))
        {
            if (findResults != NULL)
            {
                findResults[count++] = findNode->data;
            }
            findNode = findNode->next;
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
            IndexNode* newNode = findNode->next;
            HASH_DEL(*head, findNode);
            free(findNode);
            HASH_ADD_KEYPTR(hh, *head, newNode->key, newNode->keyLenth, newNode);
            return 0;
        }
        while (findNode->next != NULL)
        {
            if (findNode->next->data == data_ptr)
            {
                IndexNode* delNode = findNode->next;
                findNode->next = findNode->next->next;
                free(delNode);
                return 0;
            }
            findNode = findNode->next;
        }
    }
    return 0;
}

int IndexClear(IndexNode** head)
{
    IndexNode *ptr1, *ptr2;
    IndexNode *pre, *ptr;
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