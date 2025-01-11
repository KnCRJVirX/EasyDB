#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "edbdef.h"

/*内部API*/
int edbPrimaryKeyIndex(EasyDB *db, void* primaryKey, EDBRow** indexResult);
int edbNodeDelete(EasyDB *db, EDBRow* row);

/*文件读写API*/
int edbCreate(const char* filename, size_t columnCount, size_t primaryKeyIndex, size_t dataTypes[], size_t dataSizes[], char* columnNames[]);   //创建数据库
int edbOpen(const char* filename, EasyDB* db);                                                                                                  //打开数据库
int edbClose(EasyDB *db);                                                                                                                       //关闭数据库

/*数据库操作基础API*/
int edbInsert(EasyDB *db, void* row[]);                                                                                                         //插入
int edbDelete(EasyDB *db, void* primaryKey);                                                                                                    //删除
int edbWhere(EasyDB *db, size_t columnIndex, void* inKey, void*** findResults, size_t maxResultNumber, size_t *resultsCount);                   //查找
int edbUpdate(EasyDB *db, void* primaryKey, size_t updateColumnIndex, void* newData);                                                           //修改

/*便利API*/
void** edbIterBegin(EasyDB *db);                                                                                                                //数据库遍历（返回一个指向第一行数据的指针）
void** edbIterNext(EasyDB *db);                                                                                                                 //返回指向下一行数据的指针
int edbSearch(EasyDB *db, size_t columnIndex, char *keyWord, void*** findResults, size_t maxResultNumber, size_t *resultsCount);                //数据库文本搜索（慢）
long long columnNameToColumnIndex(EasyDB *db, char *columnName);                                                                                //将列名转换为列索引

/*Easy user management*/
void** easyLogin(EasyDB *db, size_t userIDColumn, size_t passwordColumn, char* userID, char* password);
int easyAddUser(EasyDB *db, size_t userIDColumn, size_t passwordColumn, char* userID, char* password);
int easyDeleteUser(EasyDB *db, size_t userIDColumn, size_t passwordColumn, char* userID, char* password);
int easyResetPassword(EasyDB *db, size_t userIDColumn, size_t passwordColumn, char* userID, char* newPassword);