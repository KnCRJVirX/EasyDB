#include "easydb.h"

int main(int argc, char const *argv[])
{
    size_t dataType[] = {EDB_TYPE_INT, EDB_TYPE_TEXT, EDB_TYPE_REAL};
    size_t dataLens[] = {0, 50, 0};
    char* columnNames[] = {"ID", "Name", "Balance"};
    edbCreate(".\\test.db", 3, dataType, dataLens, 1, columnNames);
    EasyDB db;
    edbOpen(".\\test.db", &db);
    printf("%s\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);

    edb_int newID = 1;
    char* newName = "Alice";
    double newBalance = 520.1314;
    void* newLine[] = {&newID, newName, &newBalance};
    edbInsert(&db, newLine);

    EDBRow* ptr;
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");
    edbClose(&db);

    edbOpen(".\\test.db", &db);
    printf("%s\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);

    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");

    newID = 2;
    newLine[1] = "Bob";
    newBalance = 114514.1919810;
    edbInsert(&db, newLine);
    edbClose(&db);

    edbOpen(".\\test.db", &db);
    printf("%s\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);

    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");
    edbClose(&db);
    
    return 0;
}
