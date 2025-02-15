#include <stdio.h>
#include <string.h>
#include "easydb.h"
#define COMMAND_SIZE 50
#define BUF_SIZE 50
#define TEXT_SIZE 40

#ifdef __WIN32
#include <windows.h>
#endif

int main(int argc, char const *argv[])
{
    #ifdef __WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif
    
    EasyDB db;
    const char* dbfilename = "test04_hugedata.db";
    size_t dataTypes[] = {EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_INT, EDB_TYPE_INT};
    size_t dataLenths[] = {TEXT_SIZE, TEXT_SIZE, 0, 0};
    char* colNames[] = {"UUID", "Name", "ID", "isRandomData"};
    int retval = edbOpen(dbfilename, &db);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(dbfilename, "04_HugedataTest", 4, "UUID", dataTypes, dataLenths, colNames);
        edbOpen(dbfilename, &db);
    }
    
    char command[COMMAND_SIZE];
    char inColName[30];
    char buf[BUF_SIZE];
    edb_int tmpInputID;
    void** findResults[1000];
    size_t resultsCount = 0;

    char newUUID[TEXT_SIZE];
    char newName[TEXT_SIZE];
    edb_int newID;
    edb_int newIsRandomData;
    void* newRow[] = {newUUID, newName, &newID, &newIsRandomData};
    long long columnIndex;

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
            scanf("%s %s %lld", newUUID, newName, &newID);
            newIsRandomData = 0;
            edbInsert(&db, newRow);
        }
        else if (!strcmp(command, "show"))
        {
            printf("%-39s\t%-15s\t%-12s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2], db.columnNames[3]);
            for (void** it = edbIterBegin(&db); it != NULL; it = edbIterNext(&db))          //遍历打印测试数据
            {
                printf("%-39s\t%-15s\t%-12lld\t%lld\n", Text(it[0]), Text(it[1]), Int(it[2]), Int(it[3]));
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
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                tmpInputID = atoll(buf);
                retval = edbWhere(&db, inColName, &tmpInputID, findResults, 1000, &resultsCount);
                break;
            case EDB_TYPE_TEXT:
                retval = edbWhere(&db, inColName, buf, findResults, 1000, &resultsCount);
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
                printf("%-39s\t%-15s\t%-12lld\t%lld\n", Text(findResults[i][0]), Text(findResults[i][1]), Int(findResults[i][2]), Int(findResults[i][3]));
            }
        }
        else if (!strcmp(command, "delete"))
        {
            scanf("%s", newUUID);
            retval = edbDelete(&db, newUUID);
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
            scanf("%s %s %s", newUUID, inColName, buf);
            columnIndex = columnNameToColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                newID = atoll(buf);
                retval = edbUpdate(&db, newUUID, inColName, &newID);
                break;
            case EDB_TYPE_TEXT:
                retval = edbUpdate(&db, newUUID, inColName, buf);
                break;
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
            fgets(keyword, TEXT_SIZE, stdin);
            if (strchr(keyword, '\n')) *(strchr(keyword, '\n')) = 0;
            retval = edbSearch(&db, "Name", keyword, findResults, 1000, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else
            {
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%-39s\t%-15s\t%-12lld\t%lld\n", Text(findResults[i][0]), Text(findResults[i][1]), Int(findResults[i][2]), Int(findResults[i][3]));
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
        else if (!strcmp(command, "deletek"))
        {
            scanf("%s %s", inColName, buf);
            columnIndex = columnNameToColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                newID = atoll(buf);
                retval = edbDeleteByKey(&db, inColName, &newID);
                break;
            case EDB_TYPE_TEXT:
                retval = edbDeleteByKey(&db, inColName, buf);
            default:
                break;
            }
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
            scanf("%s %s", newUUID, buf);
            void* res = edbGet(&db, newUUID, buf);
            if (res == NULL)
            {
                printf("NULL\n");
            }
            else if (db.dataTypes[columnNameToColumnIndex(&db, buf)] == EDB_TYPE_INT)
            {
                printf("%d\n", Int(res));
            }
            else if (db.dataTypes[columnNameToColumnIndex(&db, buf)] == EDB_TYPE_TEXT)
            {
                printf("%s\n", Text(res));
            }
            else if (db.dataTypes[columnNameToColumnIndex(&db, buf)] == EDB_TYPE_REAL)
            {
                printf("%lf\n", Real(res));
            }
        }
        else if (!strcmp(command, "addrandomdata"))
        {
            long long count = 0;
            scanf("%lld", &count);
            srand(time(NULL));
            newIsRandomData = 1;
            for (size_t i = 0; i < count; i++)
            {
                uuid(newUUID);
                int randLen = 5 + rand() % 6;
                for (int j = 0; j < randLen; j++)
                {
                    newName[j] = 'A' + rand() % 26;
                }
                newName[randLen] = 0;
                newID = 10000000 + rand() * 100 + rand();
                retval = edbInsert(&db, newRow);
                if (retval != SUCCESS)
                {
                    printf("Skip 1!\n");
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
