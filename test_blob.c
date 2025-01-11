#include "easydb.h"

typedef struct Person
{
    char name[100];
    char contect[100];
    char remarks[200];
    int age;
}Person;


int main(int argc, char const *argv[])
{
    size_t dataTypes[] = {EDB_TYPE_BLOB};
    size_t dataSizes[] = {sizeof(Person)};
    char *colNames[] = {"AddressBook"};
    edbCreate(".\\test_blob.db", 1, dataTypes, dataSizes, 0, colNames);
    
    EasyDB db;
    edbOpen(".\\test_blob.db", &db);

    Person man1 = {"Alice", "alice@gmail.com", "A test data.", 18};
    Person man2 = {"Bob", "13888888888", "A test data, too.", 20};
    void* newLine[] = {&man1};
    edbInsert(&db, newLine);
    newLine[0] = &man2;
    edbInsert(&db, newLine);

    EDBRow* ptr;
    Person* abLine;
    printf("Name\t\tContact\t\t\tReamrks\t\t\tAge\n");
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)
    {
        abLine = (Person*)(ptr->data[0]);
        printf("%-10s\t%-20s\t%-20s\t%d\n", abLine->name, abLine->contect, abLine->remarks, abLine->age);
    }
    printf("\n");

    edbClose(&db);

    edbOpen(".\\test_blob.db", &db);
    printf("Name\t\tContact\t\t\tReamrks\t\t\tAge\n");
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)
    {
        abLine = (Person*)(ptr->data[0]);
        printf("%-10s\t%-20s\t%-20s\t%d\n", abLine->name, abLine->contect, abLine->remarks, abLine->age);
    }
    printf("\n");
    edbClose(&db);
    return 0;
}
