#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "easydb.h"

int main(int argc, char const *argv[])
{
    const char* filename1 = ".\\test_primary_key_1.db";
    size_t dataType[] = {EDB_TYPE_INT, EDB_TYPE_TEXT, EDB_TYPE_REAL};
    size_t dataLenths[] = {0, 50, 0};
    char* colNames[] = {"ID", "Name", "Balance"};
    

    EasyDB db1;
    int retval;
    retval = edbOpen(filename1, &db1);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(filename1, 3, dataType, dataLenths, 0, colNames);
        edbOpen(filename1, &db1);
        size_t dataCount;
        printf("How many data you want to padding: ");
        scanf("%llu", &dataCount);

        long long newID = 0;
        char newName[50] = {0};
        int newNameLen;
        double newBalance = 0;
        void* newRow[] = {&newID, newName, &newBalance};
        srand(time(NULL));
        for (size_t i = 0; i < dataCount; i++)
        {
            newID = 10000000 + rand() % (100000000 - 10000000);
            newNameLen = 5 + rand() % 6;
            for (int j = 0; j < newNameLen; j++)
            { 
                newName[j] = 'A' + rand() % ('z' - 'A');
            }
            newName[newNameLen] = 0;
            newBalance = 10.0 + (double)(rand() % 10000000) / 1000;
            edbInsert(&db1, newRow);
        }
    }

    EDBRow* ptr;
    printf("%s\t\t%s\t\t%s\n", db1.columnNames[0], db1.columnNames[1], db1.columnNames[2]);
    for (ptr = db1.head->next; ptr != db1.tail; ptr = ptr->next)      //遍历打印测试数据
    {
        printf("%d\t%-10s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");

    char indexKey[50];
    long long indexID;
    while (1)
    {
        printf("Input the name to index (input quit to stop): ");
        scanf("%s", indexKey);
        if (!strcmp(indexKey, "quit")) break;
        indexID = atoll(indexKey);
        edbPrimaryKeyIndex(&db1, &indexID, &ptr);
        if (ptr == NULL)
        {
            printf("Cannot find!\n");
        }
        else
        {
            printf("%d\t%-10s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
        }
    }
    edbClose(&db1);

    return 0;
}
