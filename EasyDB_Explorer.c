#include <stdio.h>
#include <windows.h>
#include "easydb.h"

int main(int argc, char const *argv[])
{
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    char* dbfilename;
    if (argc < 2)
    {
        printf("Usage: <This program> <EasyDB File>\n");
        return 0;
    }
    else
    {
        dbfilename = argv[1];
    }
    EasyDB db;
    if (edbOpen(dbfilename, &db) != SUCCESS)
    {
        printf("Cannot open the EasyDB file!\n");
        return 0;
    }
    
    char command[50];
    while (1)
    {
        printf("EasyDB Explorer>");
        scanf("%s", command);
        if (!strcmp(command, "show"))
        {
            if (db.version >= 1)
            {
                printf("Table name: \"%s\"\n", db.tableName);
            }
            for (size_t i = 0; i < db.columnCount - 1; i++)
            {
                if (i == db.primaryKeyIndex)
                {
                    printf("%s <Primary key> | ", db.columnNames[i]);
                }
                else
                {
                    printf("%s | ", db.columnNames[i]);
                }
            }
            if (db.columnCount - 1 == db.primaryKeyIndex)
            {
                printf("%s <Primary key>\n", db.columnNames[db.columnCount - 1]);
            }
            else
            {
                printf("%s\n", db.columnNames[db.columnCount - 1]);
            }
            for (void** it = edbIterBegin(&db); it != NULL ; it = edbIterNext(&db))
            {
                size_t j;
                for (j = 0; j < db.columnCount - 1; j++)
                {
                    switch (db.dataTypes[j])
                    {
                    case EDB_TYPE_INT:
                        printf("%d | ", Int(it[j]));
                        break;
                    case EDB_TYPE_TEXT:
                        printf("%s | ", Text(it[j]));
                        break;
                    case EDB_TYPE_REAL:
                        printf("%lf | ", Real(it[j]));
                        break;
                    default:
                        printf("[Not printable] | ");
                        break;
                    }
                }
                switch (db.dataTypes[j])
                {
                case EDB_TYPE_INT:
                    printf("%d\n", Int(it[j]));
                    break;
                case EDB_TYPE_TEXT:
                    printf("%s\n", Text(it[j]));
                    break;
                case EDB_TYPE_REAL:
                    printf("%lf\n", Real(it[j]));
                    break;
                default:
                    printf("[Not printable]\n");
                    break;
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
