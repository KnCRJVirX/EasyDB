#include "easydb.h"

int main(int argc, char const *argv[])
{
    size_t dataTypes[] = {EDB_TYPE_INT, EDB_TYPE_TEXT, EDB_TYPE_REAL};
    size_t dataLens[] = {0, 50, 0};
    char* columnNames[] = {"ID", "Name", "Balance"};
    edbCreate(".\\test.db", 3, dataTypes, dataLens, 1, columnNames);
    EasyDB db;


    edbOpen(".\\test.db", &db);
    printf("%s\t\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);

    edb_int newID = 55240888;                              //插入第一项测试数据
    char* newName = "Alice";
    double newBalance = 520.1314;
    void* newLine[] = {&newID, newName, &newBalance};
    edbInsert(&db, newLine);

    EDBRow* ptr;
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)      //遍历打印测试数据
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");
    edbClose(&db);                  //关闭数据库


    edbOpen(".\\test.db", &db);     //重新打开，以测试edbClose和edbOpen

    printf("%s\t\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)      //遍历打印测试数据
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");

    newID = 55230777;                                  //插入第二项测试数据
    newLine[1] = "Bob";
    newBalance = 114514.1919810;
    edbInsert(&db, newLine);
    edbClose(&db);


    edbOpen(".\\test.db", &db);

    printf("%s\t\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)      //遍历打印测试数据
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");

    ptr = db.head->next;                            //删除第一项测试数据
    edbNodeDelete(&db, ptr);

    printf("%s\t\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)      //遍历打印测试数据
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");
    edbClose(&db);                  //关闭数据库


    edbOpen(".\\test.db", &db);     //重新打开，以测试edbClose和edbOpen

    printf("%s\t\t%s\t%s\n", db.columnNames[0], db.columnNames[1], db.columnNames[2]);
    for (ptr = db.head->next; ptr != db.tail; ptr = ptr->next)      //遍历打印测试数据
    {
        printf("%d\t%s\t%lf\n", *(edb_int*)(ptr->data[0]), ptr->data[1], *(double*)(ptr->data[2]));
    }
    printf("\n");

    edbClose(&db);
    
    return 0;
}
