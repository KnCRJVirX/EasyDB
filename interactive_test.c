#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "src/uthash.h"
#include "index.h"
#include "easydb.h"

int main(int argc, char const *argv[])
{
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    EasyDB db;
    EDBRow *ptr;
    const char* dbfilename = ".\\interactive_test.db";
    size_t dataTypes[] = {EDB_TYPE_INT, EDB_TYPE_TEXT, EDB_TYPE_REAL};
    size_t dataLenths[] = {0, 30, 0};
    char* colNames[] = {"ID", "Name", "Balance"};
    int retval = edbOpen(dbfilename, &db);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(dbfilename, 3, 0, dataTypes, dataLenths, colNames);
        edbOpen(dbfilename, &db);
    }
    
    char command[50];
    char inColName[30];
    char buf[50];
    edb_int tmpInputID;
    double tmpInputBalance;
    void** indexResults[10];
    size_t findCount;

    edb_int newID;
    char newName[30];
    double newBalance;
    void* newRow[] = {&newID, newName, &newBalance};
    size_t columnIndex;

    // IndexTEXTNode *ptr1, *ptr2;
    // HASH_ITER(hh, (IndexTEXTNode*)db.indexheads[1], ptr1, ptr2){
    //     printf("%lld\t%-15s\t%lf\n", *(edb_int*)(((void**)ptr1->data)[0]), ((void**)ptr1->data)[2], *(double*)(((void**)ptr1->data)[2]));
    // }

    while (1)
    {
        printf("Interactive Test>");
        scanf("%s", command);
        if (!strcmp(command, "insert"))
        {
            scanf("%lld %s %lf", &newID, newName, &newBalance);
            edbInsert(&db, newRow);
        }
        else if (!strcmp(command, "show"))
        {
            printf("%s\t\t%s\t\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);
            for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)      //遍历打印测试数据
            {
                printf("%d\t%-15s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
            }
            printf("\n");
        }
        else if (!strcmp(command, "index") || !strcmp(command, "select"))
        {
            scanf("%s %s", inColName, buf);
            columnIndex = columnNameToColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                tmpInputID = atoll(buf);
                findCount = edbWhere(&db, columnIndex, &tmpInputID, indexResults, 10);
                break;
            case EDB_TYPE_TEXT:
                findCount = edbWhere(&db, columnIndex, buf, indexResults, 10);
                break;
            default:
                break;
            }
            if (findCount == 0)
            {
                printf("Not found!\n");
                continue;
            }
            for (size_t i = 0; i < findCount; i++)
            {
                printf("%d\t%-15s\t%lf\n", *(edb_int*)(indexResults[i][0]), indexResults[i][1], *(double*)(indexResults[i][2]));
            }
        }
        else if (!strcmp(command, "delete") || !strcmp(command, "del"))
        {
            scanf("%lld", &tmpInputID);
            retval = edbDelete(&db, &tmpInputID);
            if (retval == KEY_NOT_FOUND)
            {
                printf("Not found!\n");
            }
            else
            {
                printf("Success!\n");
            }
        }
        else if (!strcmp(command, "update"))
        {
            scanf("%lld %s %s", &tmpInputID, inColName, buf);
            columnIndex = columnNameToColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                edb_int newInputID;
                newInputID = atoll(buf);
                retval = edbUpdate(&db, &tmpInputID, columnIndex, &newInputID);
                break;
            case EDB_TYPE_TEXT:
                retval = edbUpdate(&db, &tmpInputID, columnIndex, buf);
                break;
            case EDB_TYPE_REAL:
                tmpInputBalance = atof(buf);
                retval = edbUpdate(&db, &tmpInputID, columnIndex, &tmpInputBalance);
            default:
                break;
            }
            if (retval == KEY_NOT_FOUND)
            {
                printf("Not found!\n");
            }
            
        }
        else if (!strcmp(command, "quit"))
        {
            edbClose(&db);
            break;
        }
        
    }
    
    return 0;
}
