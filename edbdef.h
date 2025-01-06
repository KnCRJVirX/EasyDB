#include <stdio.h>
#include <stdint.h>
#include "src/uthash.h"
#ifndef EDBDEF
#define EDBDEF

typedef long long edb_int;
typedef double edb_real;

#define EDB_MAGIC_NUMBER 0xDBCEEA00     //魔数
#define EDB_VERSION 0                   //版本号

#define EDB_INT_SIZE 8  //整数类型长度
#define EDB_CHAR_SIZE 1 //字符类型长度
#define EDB_REAL_SIZE 8 //浮点数类型长度

#define EDB_TYPE_INT 0
#define EDB_TYPE_REAL 1
#define EDB_TYPE_BLOB 2
#define EDB_TYPE_TEXT 9

#define FILE_OPEN_ERROR -1          //文件打开错误
#define MAGIC_NUMBER_ERROR -2       //魔数错误（非EasyDB数据库文件）
#define NULL_PTR_ERROR -3           //空指针错误
#define PRIMARY_KEY_NOT_UNIQUE -4   //主键重复

typedef struct ListNode
{
    size_t id;
    void* primaryKey;
    void** data;
    struct ListNode *prev;
    struct ListNode *next;
    UT_hash_handle hh;
}ListNode;
typedef ListNode EDBRow;

typedef struct EasyDatabase
{
    char dbfilename[256];
    EDBRow* head;
    EDBRow* tail;
    EDBRow* tmpptr;
    size_t lineCount;
    size_t lineLen;         //一行的长度
    size_t dataCount;       //每行有几个数据
    size_t primaryKey;      //主键在一行中的索引
    size_t *dataOffset;     //每个数据相比行首的偏移量
    size_t *dataType;       //每个数据的类型，>=9即为TEXT类型，数值即为TEXT+长度
    size_t *dataLens;       //每个数据的长度
    char** columnNames;     //列名
    long long dataFileOffset;       //数据在文件中开始的位置
    void* indexhead;
}EasyDatabase;
typedef EasyDatabase EasyDB;
#endif