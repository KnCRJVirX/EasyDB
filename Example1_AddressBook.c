#include "easydb.h"
#ifdef _WIN32
#include <windows.h>
#endif
#define NAME_SIZE 50
#define CONTACT_SIZE 100
#define REMARKS_SIZE 200
#define SEARCH_RESULTS_MAX_COUNT 20

int main(int argc, char const *argv[])
{
    #ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif

    EasyDB db;
    int retval;
    char *dbfilename = "Example1_AddressBook.db";
    char* colNames[] = {"Name", "Contact", "Remarks"};
    size_t dataTypes[] = {EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_TEXT};
    size_t dataSizes[] = {NAME_SIZE, CONTACT_SIZE, REMARKS_SIZE};

    if (argc > 1)
    {
        dbfilename = argv[1];
    }
    

    retval = edbOpen(dbfilename, &db);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(dbfilename, 3, "Name", dataTypes, dataSizes, colNames);
        edbOpen(dbfilename, &db);
    }

    char buf[200];
    char newName[50], newContact[100], newRemarks[200];
    void *newRow[] = {newName, newContact, newRemarks};
    char command[50];
    void **searchResults[SEARCH_RESULTS_MAX_COUNT];
    size_t resultsCount;
    while (1)
    {
        printf("AddressBook>");
        fgets(command, 50, stdin);
        if (strchr(command, '\n')) *(strchr(command, '\n')) = 0;

        if (!strcmp(command, "list") || !strcmp(command, "ls"))
        {
            printf("%s\t\t%s\t\t%s\n", colNames[0], colNames[1], colNames[2]);
            for (void** it = edbIterBegin(&db); it != NULL; it = edbIterNext(&db))
            {
                printf("%-15s\t%-15s\t%s\n", Text(it[0]), Text(it[1]), Text(it[2]));
            }
        }
        else if (!strcmp(command, "add")) //增
        {
            printf("Name: ");
            fgets(newName, NAME_SIZE, stdin);
            if (strchr(newName, '\n')) *(strchr(newName, '\n')) = 0;
            printf("Contact: ");
            fgets(newContact, CONTACT_SIZE, stdin);
            if (strchr(newContact, '\n')) *(strchr(newContact, '\n')) = 0;
            printf("Remarks (Input '/' to set empty) : ");
            fgets(newRemarks, REMARKS_SIZE, stdin);
            if (strchr(newRemarks, '\n')) *(strchr(newRemarks, '\n')) = 0;
            if (!strcmp(newRemarks, "/")) newRemarks[0] = 0;
            retval = edbInsert(&db, newRow);
            if (retval == PRIMARY_KEY_NOT_UNIQUE)
            {
                printf("Name should be unique!\n");
            }
            else if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "delline") || !strcmp(command, "dl")) //删
        {
            printf("Name: ");
            fgets(newName, NAME_SIZE, stdin);
            if (strchr(newName, '\n')) *(strchr(newName, '\n')) = 0;
            retval = edbDelete(&db, newName);
            if (retval == KEY_NOT_FOUND)
            {
                printf("Not found!\n");
            }
            else if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "findname") || !strcmp(command, "fn"))
        {
            printf("Name: ");
            fgets(newName, NAME_SIZE, stdin);
            if (strchr(newName, '\n')) *(strchr(newName, '\n')) = 0;
            retval = edbWhere(&db, "Name", newName, searchResults, SEARCH_RESULTS_MAX_COUNT, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else if (resultsCount > 0)
            {
                printf("%s\t\t%s\t\t%s\n", colNames[0], colNames[1], colNames[2]);
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%-15s\t%-15s\t%s\n", Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
                }
            }
        }
        else if (!strcmp(command, "findphonenumber") || !strcmp(command, "fpn"))
        {
            printf("Contact: ");
            fgets(newContact, CONTACT_SIZE, stdin);
            if (strchr(newContact, '\n')) *(strchr(newContact, '\n')) = 0;
            retval = edbWhere(&db, "Contact", newContact, searchResults, SEARCH_RESULTS_MAX_COUNT, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else if (resultsCount > 0)
            {
                printf("%s\t\t%s\t\t%s\n", colNames[0], colNames[1], colNames[2]);
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%-15s\t%-15s\t%s\n", Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
                }
            }
        }
        else if (!strcmp(command, "edit")) //改
        {
            printf("Name (You want to edit) : ");
            fgets(buf, NAME_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            retval = edbWhere(&db, "Name", buf, searchResults, 1, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
                continue;
            }
            else
            {
                printf("%s\t\t%s\t\t%s\n", colNames[0], colNames[1], colNames[2]);
                printf("%-15s\t%-15s\t%s\n", Text(searchResults[0][0]), Text(searchResults[0][1]), Text(searchResults[0][2]));
            }

            printf("New Name (Input '/' to keep): ");
            fgets(newName, NAME_SIZE, stdin);
            if (strchr(newName, '\n')) *(strchr(newName, '\n')) = 0;

            printf("New Contact (Input '/' to keep): ");
            fgets(newContact, CONTACT_SIZE, stdin);
            if (strchr(newContact, '\n')) *(strchr(newContact, '\n')) = 0;

            printf("New Remarks (Input '/' to keep, input '/clear' to set empty) : ");
            fgets(newRemarks, REMARKS_SIZE, stdin);
            if (strchr(newRemarks, '\n')) *(strchr(newRemarks, '\n')) = 0;

            if (strcmp(newContact, "/"))
            {
                edbUpdate(&db, buf, "Contact", newContact);
            }
            if (!strcmp(newRemarks, "/clear"))
            {
                newRemarks[0] = 0;
                edbUpdate(&db, buf, "Remarks", newRemarks);
            }
            else if (strcmp(newRemarks, "/"))
            {
                edbUpdate(&db, buf, "Remarks", newRemarks);
            }
            if (strcmp(newName, "/"))
            {
                edbUpdate(&db, buf, "Name", newName);
            }
        }    
        else if (!strcmp(command, "search") || !strcmp(command, "s"))
        {
            printf("Name: ");
            fgets(buf, NAME_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            retval = edbSearch(&db, "Name", buf, searchResults, SEARCH_RESULTS_MAX_COUNT, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else if (resultsCount > 0)
            {
                printf("%s\t\t%s\t\t%s\n", colNames[0], colNames[1], colNames[2]);
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%-15s\t%-15s\t%s\n", Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
                }
            }
        }
        else if (!strcmp(command, "cls"))
        {
            system("cls");
        }
        else if (!strcmp(command, "exit") || !strcmp(command, "quit"))
        {
            edbClose(&db);
            return 0;
        }
        else
        {
            printf("Unknown command!\n");
        } 
    }
    return 0;
}
