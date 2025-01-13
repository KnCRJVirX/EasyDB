#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "edbdef.h"

/*便利宏*/
#define Int(x) *(edb_int*)(x)   //将返回的void指针转换为整数指针并解引用
#define Real(x) *(double*)(x)   //将返回的void指针转换为浮点数指针并解引用
#define Text(x) (char*)(x)      //将返回的void指针转换为字符指针

/*内部API*/
int edbPrimaryKeyIndex(EasyDB *db, void* primaryKey, EDBRow** indexResult);
int edbNodeDelete(EasyDB *db, EDBRow* row);

/*文件读写API*/
int edbCreate(const char* filename, size_t columnCount, size_t primaryKeyIndex, size_t dataTypes[], size_t dataSizes[], char* columnNames[]);   //创建数据库
int edbOpen(const char* filename, EasyDB* db);                                                                                                  //打开数据库
int edbClose(EasyDB *db);                                                                                                                       //关闭数据库
int edbSave(EasyDB *db);                                                                                                                        //保存数据库

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

/*Easy User Management*/
/* userID所在的列必须是主键列，密码所在的列请将列名设为“password” */
int easyLogin(EasyDB *db, char* userID, char* password, void*** retUserData);
int easyAddUser(EasyDB *db, void* newRow[]);
int easyDeleteUser(EasyDB *db, char* userID);
int easyResetPassword(EasyDB *db, char* userID, char* newPassword);
char* uuid(char *UUID);                                                                                                                         //生成UUID v4
char* sha256(const char* input, char* SHA256);                                                                                                  //对输入的字符串进行sha256摘要