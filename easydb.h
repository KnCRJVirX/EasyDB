#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "edbdef.h"

bool isExisted(int element, int array[], int array_len);
int compareInts(const void *a, const void *b);

// int read(int *line_count, struct person addressbook[BOOK_MAX_SIZE], char *filepath);
// int save(int line_count, struct person addressbook[BOOK_MAX_SIZE], char *filepath);

// int add(int *line_count, struct person addressbook[BOOK_MAX_SIZE]);
// int delline(int line_count, struct person addressbook[BOOK_MAX_SIZE]);
// int del(int line_count, struct person addressbook[BOOK_MAX_SIZE]);
// int findName(int line_count, struct person addressbook[BOOK_MAX_SIZE]);
// int findPhoneNumber(int line_count, struct person addressbook[BOOK_MAX_SIZE]);
// int edit(int line_count, struct person addressbook[BOOK_MAX_SIZE]);
// int search(int line_count, struct person addressbook[BOOK_MAX_SIZE], int index_list[BOOK_MAX_SIZE]);
// int randomData(int *line_count, struct person addressbook[BOOK_MAX_SIZE]);

int edbCreate(const char* filename, size_t columnCount, size_t dataTypes[], size_t dataLens[], size_t primaryKeyIndex, char* columnNames[]);
int edbOpen(const char* filename, EasyDB* db);
int edbClose(EasyDB *db);

int edbInsert(EasyDB *db, void* row[]);
int edbDelete(EasyDB *db, EDBRow* row);
int edbPrimaryKeyIndex(EasyDB *db, void* primaryKey, void*** indexResult);
size_t edbIndex(EasyDB* db, size_t colIndex, void* inKey, void*** indexResults, size_t maxResultNumber);