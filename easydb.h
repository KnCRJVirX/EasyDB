#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifndef EASYDB
#define EASYDB

/*类型与结构体定义*/
typedef int64_t edb_int;
typedef double edb_real;

#define EDB_MAGIC_NUMBER 0xDBCEEA00     //魔数
#define EDB_VERSION 0                   //版本号

#define EDB_INT_SIZE 8  //整数类型长度
#define EDB_CHAR_SIZE 1 //字符类型长度
#define EDB_REAL_SIZE 8 //浮点数类型长度

#define EDB_TYPE_INT 0      //整数类型
#define EDB_TYPE_REAL 1     //实数（浮点数）类型
#define EDB_TYPE_BLOB 2     //二进制类型
#define EDB_TYPE_TEXT 9     //文本类型

#define SUCCESS 0                           //操作成功完成
#define FILE_OPEN_ERROR -1                  //文件打开错误
#define MAGIC_NUMBER_ERROR -2               //魔数错误（非EasyDB数据库文件）
#define NULL_PTR_ERROR -3                   //空指针错误
#define PRIMARY_KEY_NOT_UNIQUE -4           //主键重复
#define PRIMARY_KEY_TYPE_CANNOT_INDEX -5    //仅支持整数和文本的索引，主键必须是整数或文本类型
#define TYPE_CANNOT_INDEX -6                //仅支持整数和文本的索引
#define KEY_NOT_FOUND -7                    //未找到匹配的行
#define COLUMN_INDEX_OUT_OF_RANGE -8        //传入的列索引超出原来的列数
#define COLUMN_NOT_FOUND -9                 //找不到列名对应的列
#define EMPTY_TABLE -10                     //该表为空
#define NOT_TEXT_COLUMN -11                 //该列非文本类型（仅支持对文本类型的列进行搜索）
#define PASSWORD_WRONG -12                  //密码错误（Easy User Management）
#define PRIMARY_KEY_NOT_IN_LIST -13         //主键名不在传入的列名列表中

typedef struct ListNode
{
    size_t id;
    void** data;
    struct ListNode *prev;
    struct ListNode *next;
}ListNode;
typedef ListNode EDBRow;

typedef struct EasyDatabase
{
    char dbfilename[256];
    EDBRow* head;
    EDBRow* tail;
    EDBRow* tmpptr;
    size_t rowCount;
    size_t lineSize;                //一行的长度
    size_t columnCount;             //每行有几个数据
    size_t primaryKeyIndex;         //主键在一行中的索引
    size_t *dataOffset;             //每个数据相比行首的偏移量
    size_t *dataTypes;              //每个数据的类型，>=9即为TEXT类型，数值即为TEXT+长度
    size_t *dataSizes;              //每个数据的长度
    char** columnNames;             //列名
    fpos_t dataFileOffset;          //数据在文件中开始的位置
    void** indexheads;              //索引表头指针存储
}EasyDatabase;
typedef EasyDatabase EasyDB;


/*便利宏*/
#define Int(x) *(edb_int*)(x)   //将返回的void指针转换为整数指针并解引用
#define Real(x) *(double*)(x)   //将返回的void指针转换为浮点数指针并解引用
#define Text(x) (char*)(x)      //将返回的void指针转换为字符指针

/*内部API*/
int edbPrimaryKeyIndex(EasyDB *db, void* primaryKey, EDBRow** indexResult);
int edbNodeDelete(EasyDB *db, EDBRow* row);

/* 文件读写API */
/* 创建数据库文件(文件名, 列数, 主键所在的列名, 每列的数据类型的数组, 每列的数据大小的数组（仅TEXT和BLOB类型需要，别的列为0即可）, 每列的列名数组) */
int edbCreate(const char* filename, size_t columnCount, char* primaryKeyColumnName, size_t dataTypes[], size_t dataSizes[], char* columnNames[]);   //创建数据库
/* 打开数据库(文件名, &EasyDB结构体类型变量) */
int edbOpen(const char* filename, EasyDB* db);                                                                                                      //打开数据库
/* 关闭数据库(&EasyDB结构体类型变量) */
int edbClose(EasyDB *db);                                                                                                                           //关闭数据库
/* 保存数据库(&EasyDB结构体类型变量) */
int edbSave(EasyDB *db);                                                                                                                            //保存数据库

/* 数据库操作基础API */
/* 插入(&EasyDB结构体类型变量, 新行) */
int edbInsert(EasyDB *db, void* row[]);                                                                                                         //插入
/* 删除(&EasyDB结构体类型变量, 要删除的行的主键（指针）) */
int edbDelete(EasyDB *db, void* primaryKey);                                                                                                    //删除
/* 查找(&EasyDB结构体类型变量, 查找的列名, 查找的键即内容（指针）, 用来容纳结果的数组, 最多查找的个数, &用来接收查找到的个数的变量) */
int edbWhere(EasyDB *db, char* columnName, void* inKey, void*** findResults, size_t maxResultNumber, size_t *resultsCount);                     //查找
/* 修改(&EasyDB结构体类型变量, 要修改的行的主键（指针）, 要修改的列名, 新的内容（指针）) */
int edbUpdate(EasyDB *db, void* primaryKey, char* updateColumnName, void* newData);                                                             //修改

/*便利API*/
long long columnNameToColumnIndex(EasyDB *db, char *columnName);                                                                                //将列名转换为列索引
void** edbIterBegin(EasyDB *db);                                                                                                                //数据库遍历（返回一个指向第一行数据的指针）
void** edbIterNext(EasyDB *db);                                                                                                                 //返回指向下一行数据的指针
int edbSearch(EasyDB *db, char* columnName, char *keyWord, void*** findResults, size_t maxResultNumber, size_t *resultsCount);                  //数据库文本搜索（慢）
int edbDeleteByArray(EasyDB *db, void** deleteRows[], size_t arraySize);                                                                        //使用搜索得到的数组删除
int edbDeleteByKeyword(EasyDB *db, char* columnName, char *keyword, size_t maxDeleteCount);                                                     //使用关键词删除

/* Easy User Management */
/* userID所在的列必须是主键列，密码所在的列请将列名设为“password” */
/* 使用此系列功能处理的用户密码将自动进行SHA256摘要（加密）*/
int easyLogin(EasyDB *db, char* userID, char* password, void*** retUserData);                                                                   //登录
int easyAddUser(EasyDB *db, void* newRow[]);                                                                                                    //新增用户
int easyDeleteUser(EasyDB *db, char* userID);                                                                                                   //删除用户
int easyResetPassword(EasyDB *db, char* userID, char* newPassword);                                                                             //重置用户密码
char* uuid(char *UUID);                                                                                                                         //生成UUID v4
char* sha256(const char* input, char* SHA256);                                                                                                  //对输入的字符串进行sha256摘要

#endif