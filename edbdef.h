#include <stdio.h>
#include <stdint.h>
#include "src/uthash.h"
#ifndef EDBDEF
#define EDBDEF

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
#define PRIMARY_KEY_TYPE_CANNOT_INDEX -5    //仅支持整数和文本的索引
#define TYPE_CANNOT_INDEX -6                //仅支持整数和文本的索引
#define KEY_NOT_FOUND -7                    //未找到匹配的行
#define COLUMN_INDEX_OUT_OF_RANGE -8        //传入的列索引超出原来的列数
#define COLUMN_NOT_FOUND -9                 //找不到列名对应的列
#define EMPTY_TABLE -10                     //该表为空
#define NOT_TEXT_COLUMN -11                 //该列非文本类型（仅支持对文本类型的列进行搜索）
#define PASSWORD_WRONG -12                  //密码错误（Easy User Management）

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
    size_t lineSize;                 //一行的长度
    size_t columnCount;             //每行有几个数据
    size_t primaryKey;              //主键在一行中的索引
    size_t *dataOffset;             //每个数据相比行首的偏移量
    size_t *dataTypes;              //每个数据的类型，>=9即为TEXT类型，数值即为TEXT+长度
    size_t *dataSizes;               //每个数据的长度
    char** columnNames;             //列名
    long long dataFileOffset;       //数据在文件中开始的位置
    void** indexheads;              //索引表头指针存储
}EasyDatabase;
typedef EasyDatabase EasyDB;
#endif