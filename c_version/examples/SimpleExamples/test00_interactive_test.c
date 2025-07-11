#include <stdio.h>
#include <string.h>
#include "easydb.h"
#define COMMAND_SIZE 50
#define BUF_SIZE 50
#define NAME_SIZE 30

#ifdef __WIN32
#include <windows.h>
#endif

int cmpInts(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}

int cmpDoubles(const void *a, const void *b)
{
    if (*(double*)a - *(double*)b < 0) return -1;
    else if (*(double*)a - *(double*)b > 0) return 1;
    return 0;
}

int main(int argc, char const *argv[])
{
    #ifdef __WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif
    
    EasyDB db;
    const char* dbfilename = "test00_interactive_test.db";
    const char* tablename = "Test00: Interactive test";
    size_t dataTypes[] = {EDB_TYPE_INT, EDB_TYPE_TEXT, EDB_TYPE_REAL};
    size_t dataLenths[] = {0, NAME_SIZE, 0};
    char* colNames[] = {"ID", "Name", "Balance"};
    int retval = edbOpen(dbfilename, &db);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(dbfilename, tablename, 3, "ID", dataTypes, dataLenths, colNames);
        edbOpen(dbfilename, &db);
    }
    
    char command[COMMAND_SIZE];
    char inColName[30];
    char buf[BUF_SIZE];
    edb_int tmpInputID;
    double tmpInputBalance;
    void** findResults[10];
    size_t resultsCount = 0;

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
            void* eIterator = edbIterBegin(&db);
            void** it = NULL;
            while (it = edbIterNext(&db, &eIterator))
            {
                printf("%-10d\t%-15s\t%lf\n", Int(it[0]), it[1], Real(it[2]));
            }
            printf("\n");
        }
        else if (!strcmp(command, "select"))
        {
            scanf("%s", inColName);
            getchar();
            fgets(buf, BUF_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            columnIndex = toColumnIndex(&db, inColName);
            size_t resultsCount;
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                tmpInputID = atoll(buf);
                retval = edbWhere(&db, "ID", &tmpInputID, findResults, 10, &resultsCount);
                break;
            case EDB_TYPE_TEXT:
                retval = edbWhere(&db, "Name", buf, findResults, 10, &resultsCount);
                break;
            case EDB_TYPE_REAL:
                tmpInputBalance = atof(buf);
                retval = edbWhere(&db, "Balance", &tmpInputBalance, findResults, 10, &resultsCount);
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
                printf("%-10d\t%-15s\t%lf\n", Int(findResults[i][0]), findResults[i][1], Real(findResults[i][2]));
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
            columnIndex = toColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                edb_int newInputID;
                newInputID = atoll(buf);
                retval = edbUpdate(&db, &tmpInputID, "ID", &newInputID);
                break;
            case EDB_TYPE_TEXT:
                retval = edbUpdate(&db, &tmpInputID, "Name", buf);
                break;
            case EDB_TYPE_REAL:
                tmpInputBalance = atof(buf);
                retval = edbUpdate(&db, &tmpInputID, "Balance", &tmpInputBalance);
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
            getchar();
            fgets(keyword, NAME_SIZE, stdin);
            if (strchr(keyword, '\n')) *(strchr(keyword, '\n')) = 0;
            retval = edbSearch(&db, "Name", keyword, findResults, 10, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else
            {
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%-10d\t%-15s\t%lf\n", Int(findResults[i][0]), findResults[i][1], Real(findResults[i][2]));
                }
            }
        }
        else if (!strcmp(command, "deletethem"))
        {
            retval = edbDeleteByArray(&db, findResults, resultsCount);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "deletekw"))
        {
            scanf("%s", newName);
            retval = edbDeleteByKeyword(&db, "Name", newName);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "save"))
        {
            retval = edbSave(&db);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "get"))
        {
            scanf("%d %s", &tmpInputID, buf);
            void* res = edbGet(&db, &tmpInputID, buf);
            if (res == NULL)
            {
                printf("NULL\n");
            }
            else if (db.dataTypes[toColumnIndex(&db, buf)] == EDB_TYPE_INT)
            {
                printf("%d\n", Int(res));
            }
            else if (db.dataTypes[toColumnIndex(&db, buf)] == EDB_TYPE_TEXT)
            {
                printf("%s\n", Text(res));
            }
            else if (db.dataTypes[toColumnIndex(&db, buf)] == EDB_TYPE_REAL)
            {
                printf("%lf\n", Real(res));
            }
        }
        else if (!strcmp(command, "sort"))
        {
            scanf("%s", buf);
            if (db.dataTypes[toColumnIndex(&db, buf)] == EDB_TYPE_INT)
            {
                retval = edbSort(&db, buf, cmpInts);
            }
            else if (db.dataTypes[toColumnIndex(&db, buf)] == EDB_TYPE_REAL)
            {
                retval = edbSort(&db, buf, cmpDoubles);
            }
            else if (db.dataTypes[toColumnIndex(&db, buf)] == EDB_TYPE_TEXT)
            {
                retval = edbSort(&db, buf, (int(*)(const void*, const void*))strcmp);
            }
            if (retval == SUCCESS)
            {
                printf("OK!\n");
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
