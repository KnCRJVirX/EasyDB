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

    void** newRow = (void**)calloc(db.columnCount, sizeof(void*));
    for (size_t i = 0; i < db.columnCount; i++)
    {
        newRow[i] = calloc(1, db.dataSizes[i]);
    }
    
    long long columnIndex;

    while (1)
    {
        printf("Interactive Test>");
        scanf("%s", command);
        if (!strcmp(command, "insert"))
        {
            scanf("%s %s %lld", Text(newRow[toColumnIndex(&db, "UUID")]), Text(newRow[toColumnIndex(&db, "Name")]), &Int(newRow[toColumnIndex(&db, "ID")]));
            Int(newRow[toColumnIndex(&db, "isRandomData")]) = 0;
            edbInsert(&db, newRow);
        }
        else if (!strcmp(command, "show"))
        {
            printf("%-39s\t%-15s\t%-12s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2], db.columnNames[3]);
            void* eIterator = edbIterBegin(&db);
            void** it = NULL;
            while (it = edbIterNext(&db, &eIterator))
            {
                printf("%-39s\t%-15s\t%-12lld\t%lld\n", Text(it[0]), Text(it[1]), Int(it[2]), Int(it[3]));
            }
            printf("\n");
        }
        else if (!strcmp(command, "select"))
        {
            scanf("%s %s", inColName, buf);
            switch (db.dataTypes[toColumnIndex(&db, inColName)])
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
            scanf("%s", buf);
            retval = edbDelete(&db, buf);
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
            scanf("%s %s %s", Text(newRow[toColumnIndex(&db, "UUID")]), inColName, buf);
            columnIndex = toColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                edb_int newInt = atoll(buf);
                retval = edbUpdate(&db, Text(newRow[toColumnIndex(&db, "UUID")]), inColName, &newInt);
                break;
            case EDB_TYPE_TEXT:
                retval = edbUpdate(&db, Text(newRow[toColumnIndex(&db, "UUID")]), inColName, buf);
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
            scanf("%s", buf);
            retval = edbDeleteByKeyword(&db, "Name", buf);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "deletek"))
        {
            scanf("%s %s", inColName, buf);
            columnIndex = toColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                edb_int newInt = atoll(buf);
                retval = edbDeleteByKey(&db, inColName, &newInt);
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
            scanf("%s %s", Text(newRow[toColumnIndex(&db, "UUID")]), buf);
            void* res = edbGet(&db, Text(newRow[toColumnIndex(&db, "UUID")]), buf);
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
        else if (!strcmp(command, "addrandomdata"))
        {
            long long count = 0;
            scanf("%lld", &count);
            srand(time(NULL));
            Int(newRow[toColumnIndex(&db, "isRandomData")]) = 1;
            for (size_t i = 0; i < count; i++)
            {
                uuid(Text(newRow[toColumnIndex(&db, "UUID")]));
                int randLen = 5 + rand() % 6;
                for (int j = 0; j < randLen; j++)
                {
                    Text(newRow[toColumnIndex(&db, "Name")])[j] = 'A' + rand() % 26;
                }
                Text(newRow[toColumnIndex(&db, "Name")])[randLen] = 0;
                Int(newRow[toColumnIndex(&db, "ID")]) = 10000000 + rand() * 100 + rand();
                retval = edbInsert(&db, newRow);
                if (retval != SUCCESS)
                {
                    printf("Skip 1!\n");
                }
            }
        }
        else if (!strcmp(command, "sort"))
        {
            scanf("%s", inColName);
            edbSort(&db, inColName, NULL);
        }
        else if (!strcmp(command, "count"))
        {
            scanf("%s", inColName);
            getchar();
            fgets(buf, BUF_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            columnIndex = toColumnIndex(&db, inColName);
            switch (db.dataTypes[columnIndex])
            {
            case EDB_TYPE_INT:
                tmpInputID = atoll(buf);
                resultsCount = edbCount(&db, inColName, &tmpInputID);
                break;
            case EDB_TYPE_TEXT:
                resultsCount = edbCount(&db, inColName, buf);
                break;
            default:
                break;
            }
            if (resultsCount == 0)
            {
                printf("Not found!\n");
                continue;
            }
            printf("%lld\n", resultsCount);
        }
        else if (!strcmp(command, "quit"))
        {
            edbClose(&db);
            break;
        }
        
    }
    
    return 0;
}
