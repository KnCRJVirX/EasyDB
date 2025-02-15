#include <stdio.h>
#include "easydb.h"

#ifdef __WIN32
#include <windows.h>
#endif

int main(int argc, char const *argv[])
{
    #ifdef __WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif

    const char* dbfilename = "test02_EUM_interactive_test.db";
    const char* tablename = "Test02: Easy User Management interactive test";
    size_t dataTypes[]= {EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_INT};
    size_t dataSizes[] = {50, 100, 0};
    char* colNames[] = {"UserID", "password", "VIPLevel"};

    EasyDB db;
    int retval = edbOpen(dbfilename, &db);
    if (retval == FILE_OPEN_ERROR)
    {
        edbCreate(dbfilename, tablename, 3, "UserID", dataTypes, dataSizes, colNames);
        edbOpen(dbfilename, &db);
    }
    
    char command[50];
    char newUserID[50];
    char newPasswd[50];
    edb_int newVIPLevel;
    void* newRow[] = {newUserID, newPasswd, &newVIPLevel};

    while (1)
    {
        printf("Easy User Management>");
        scanf("%s", command);
        if (!strcmp(command, "adduser"))
        {
            scanf("%s %s %d", newUserID, newPasswd, &newVIPLevel);
            retval = easyAddUser(&db, newRow);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
            else if (retval == PRIMARY_KEY_NOT_UNIQUE)
            {
                printf("User ID should be unique!\n");
            }
        }
        else if (!strcmp(command, "login"))
        {
            scanf("%s %s", newUserID, newPasswd);
            void** userData;
            retval = easyLogin(&db, newUserID, newPasswd, &userData);
            if (retval == PASSWORD_WRONG || retval == KEY_NOT_FOUND)
            {
                printf("Wrong!\n");
            }
            else
            {
                printf("Login success! UserID: %s, VIP Level: %d\n", userData[0], Int(userData[2]));
            }
        }
        else if (!strcmp(command, "deluser"))
        {
            scanf("%s", newUserID);
            retval = easyDeleteUser(&db, newUserID);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "passwd"))
        {
            scanf("%s %s", newUserID, newPasswd);
            easyResetPassword(&db, newUserID, newPasswd);
            if (retval == SUCCESS)
            {
                printf("OK!\n");
            }
        }
        else if (!strcmp(command, "userlist"))
        {
            printf("UserID | password | VIP Level\n");
            for (void** it = edbIterBegin(&db); it != NULL; it = edbIterNext(&db))
            {
                printf("%s | %s | %d\n", it[0], it[1], Int(it[2]));
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
