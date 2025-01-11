#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "src/uthash.h"
#include "index.h"
#include "easydb.h"
#define COMMAND_SIZE 50
#define BUF_SIZE 50
#define NAME_SIZE 30

int main(int argc, char const *argv[])
{
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    EasyDB db;
    const char* dbfilename = ".\\interactive_test.db";
    size_t dataTypes[] = {EDB_TYPE_INT, EDB_TYPE_TEXT, EDB_TYPE_REAL};
    size_t dataLenths[] = {0, NAME_SIZE, 0};
    char* colNames[] = {"ID", "Name", "Balance"};
    int retval = edbOpen(dbfilename, &db);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(dbfilename, 3, 0, dataTypes, dataLenths, colNames);
        edbOpen(dbfilename, &db);
    }
    
    char command[COMMAND_SIZE];
    char inColName[30];
    char buf[BUF_SIZE];
    edb_int tmpInputID;
    double tmpInputBalance;
    void** findResults[10];

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
            for (void** it = edbIterBegin(&db); it != NULL; it = edbIterNext(&db))      //遍历打印测试数据
            {
                printf("%d\t%-15s\t%lf\n", *(edb_int*)(it[0]), it[1], *(double*)(it[2]));
            }
            printf("\n");
        }
        else if (!strcmp(command, "select"))
        {
            scanf("%s", inColName);
            getchar();
            fgets(buf, BUF_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            columnIndex = columnNameToColumnIndex(&db, inColName);
            size_t resultsCount;
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                tmpInputID = atoll(buf);
                retval = edbWhere(&db, columnIndex, &tmpInputID, findResults, 10, &resultsCount);
                break;
            case EDB_TYPE_TEXT:
                retval = edbWhere(&db, columnIndex, buf, findResults, 10, &resultsCount);
                break;
            default:
                break;
            }
            if (resultsCount == 0)
            {
                printf("Not found!\n");
                continue;
            }
            for (size_t i = 0; i < resultsCount; i++)
            {
                printf("%d\t%-15s\t%lf\n", *(edb_int*)(findResults[i][0]), findResults[i][1], *(double*)(findResults[i][2]));
            }
        }
        else if (!strcmp(command, "delete"))
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
        else if (!strcmp(command, "search"))
        {
            char keyword[30];
            size_t resultsCount;
            getchar();
            fgets(keyword, NAME_SIZE, stdin);
            if (strchr(keyword, '\n')) *(strchr(keyword, '\n')) = 0;
            retval = edbSearch(&db, 1, keyword, findResults, 10, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else
            {
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%d\t%-15s\t%lf\n", *(edb_int*)(findResults[i][0]), findResults[i][1], *(double*)(findResults[i][2]));
                }
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
