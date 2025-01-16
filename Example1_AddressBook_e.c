#include "易库.h"
#include <windows.h>
#define NAME_SIZE 50
#define CONTACT_SIZE 100
#define REMARKS_SIZE 200
#define SEARCH_RESULTS_MAX_COUNT 20

int main(int argc, char const *argv[])
{
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    易库 数据库;
    int retval;
    char *数据库文件名 = "Example1_AddressBook.db";
    char* colNames[] = {"Name", "Contact", "Remarks"};
    size_t dataTypes[] = {EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_TEXT};
    size_t dataSizes[] = {NAME_SIZE, CONTACT_SIZE, REMARKS_SIZE};

    if (argc > 1)
    {
        数据库文件名 = argv[1];
    }
    

    retval = 打开数据库(数据库文件名, &数据库);
    if (retval == 文件打开错误)
    {
        创建数据库(数据库文件名, 3, "Name", dataTypes, dataSizes, colNames);
        打开数据库(数据库文件名, &数据库);
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
            printf("%s | %s | %s\n", colNames[0], colNames[1], colNames[2]);
            for (void** it = edbIterBegin(&数据库); it != NULL; it = edbIterNext(&数据库))
            {
                printf("%s | %s | %s\n", it[0], it[1], it[2]);
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
            retval = edbInsert(&数据库, newRow);
            if (retval == PRIMARY_KEY_NOT_UNIQUE)
            {
                printf("Name should be unique!\n");
            }
            else if (retval == 操作成功完成)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "delline") || !strcmp(command, "dl")) //删
        {
            printf("Name: ");
            fgets(newName, NAME_SIZE, stdin);
            if (strchr(newName, '\n')) *(strchr(newName, '\n')) = 0;
            retval = edbDelete(&数据库, newName);
            if (retval == KEY_NOT_FOUND)
            {
                printf("Not found!\n");
            }
            else if (retval == 操作成功完成)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "findname") || !strcmp(command, "fn"))
        {
            printf("Name: ");
            fgets(newName, NAME_SIZE, stdin);
            if (strchr(newName, '\n')) *(strchr(newName, '\n')) = 0;
            retval = edbWhere(&数据库, "Name", newName, searchResults, SEARCH_RESULTS_MAX_COUNT, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else if (resultsCount > 0)
            {
                printf("%s | %s | %s\n", colNames[0], colNames[1], colNames[2]);
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%s | %s | %s\n", searchResults[i][0], searchResults[i][1], searchResults[i][2]);
                }
            }
        }
        else if (!strcmp(command, "findphonenumber") || !strcmp(command, "fpn"))
        {
            printf("Contact: ");
            fgets(newContact, CONTACT_SIZE, stdin);
            if (strchr(newContact, '\n')) *(strchr(newContact, '\n')) = 0;
            retval = edbWhere(&数据库, "Contact", newContact, searchResults, SEARCH_RESULTS_MAX_COUNT, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else if (resultsCount > 0)
            {
                printf("%s | %s | %s\n", colNames[0], colNames[1], colNames[2]);
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%s | %s | %s\n", searchResults[i][0], searchResults[i][1], searchResults[i][2]);
                }
            }
        }
        else if (!strcmp(command, "edit")) //改
        {
            printf("Name (You want to edit) : ");
            fgets(buf, NAME_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            retval = edbWhere(&数据库, "Name", buf, searchResults, 1, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
                continue;
            }
            else
            {
                printf("%s | %s | %s\n", searchResults[0][0], searchResults[0][1], searchResults[0][2]);
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

            if (strcmp(newName, "/"))
            {
                edbUpdate(&数据库, buf, "Name", newName);
            }
            if (strcmp(newContact, "/"))
            {
                edbUpdate(&数据库, buf, "Contact", newContact);
            }
            if (!strcmp(newRemarks, "/clear"))
            {
                newRemarks[0] = 0;
                edbUpdate(&数据库, buf, "Remarks", newRemarks);
            }
            else if (strcmp(newRemarks, "/"))
            {
                edbUpdate(&数据库, buf, "Remarks", newRemarks);
            }
        }    
        else if (!strcmp(command, "search") || !strcmp(command, "s"))
        {
            printf("Name: ");
            fgets(buf, NAME_SIZE, stdin);
            if (strchr(buf, '\n')) *(strchr(buf, '\n')) = 0;
            retval = edbSearch(&数据库, "Name", buf, searchResults, SEARCH_RESULTS_MAX_COUNT, &resultsCount);
            if (resultsCount == 0)
            {
                printf("Not found!\n");
            }
            else if (resultsCount > 0)
            {
                printf("%s | %s | %s\n", colNames[0], colNames[1], colNames[2]);
                for (size_t i = 0; i < resultsCount; i++)
                {
                    printf("%s | %s | %s\n", searchResults[i][0], searchResults[i][1], searchResults[i][2]);
                }
            }
        }
        else if (!strcmp(command, "cls"))
        {
            system("cls");
        }
        else if (!strcmp(command, "exit") || !strcmp(command, "quit"))
        {
            edbClose(&数据库);
            return 0;
        }
        else
        {
            printf("Unknown command!\n");
        } 
    }
    return 0;
}
