#include "easydb.h"
#include "index.h"

int edbCreate(const char* filename, const char* tableName, size_t columnCount, char* primaryKeyColumnName, size_t dataTypes[], size_t dataSizes[], char* columnNames[])
{
    if (filename == NULL) return NULL_PTR_ERROR;
    size_t primaryKeyIndex = -1;
    for (size_t i = 0; i < columnCount; i++)
    {
        if (!strcmp(columnNames[i], primaryKeyColumnName))
        {
            primaryKeyIndex = i;
            break;
        }
    }
    if (primaryKeyIndex == -1) return PRIMARY_KEY_NOT_IN_LIST;
    
    FILE* dbfile = fopen(filename, "wb+");
    if (dbfile == NULL) return FILE_OPEN_ERROR;

    int magicNum = EDB_MAGIC_NUMBER;
    fwrite(&magicNum, 4, 1, dbfile);

    int ver = EDB_VERSION;
    fwrite(&ver, 4, 1, dbfile);

    size_t emptyRowCount = 0;
    fwrite(&emptyRowCount, EDB_INT_SIZE, 1, dbfile);

    size_t rowSize = 0;
    for (size_t i = 0; i < columnCount; i++)
    {
        switch (dataTypes[i])
        {
        case EDB_TYPE_INT:
            rowSize += EDB_INT_SIZE;
            dataSizes[i] = EDB_INT_SIZE;
            break;
        case EDB_TYPE_REAL:
            rowSize += EDB_REAL_SIZE;
            dataSizes[i] = EDB_REAL_SIZE;
            break;
        case EDB_TYPE_TEXT:
            rowSize += dataSizes[i];
            break;
        case EDB_TYPE_BLOB:
            rowSize += dataSizes[i];
            break;
        default:
            break;
        }
    }
    fwrite(&rowSize, EDB_INT_SIZE, 1, dbfile);

    fwrite(&columnCount, EDB_INT_SIZE, 1, dbfile);
    fwrite(dataTypes, EDB_INT_SIZE, columnCount, dbfile);
    fwrite(dataSizes, EDB_INT_SIZE, columnCount, dbfile);
    for (size_t i = 0; i < columnCount; i++)
    {
        fwrite(columnNames[i], sizeof(char), strlen(columnNames[i]) + 1, dbfile); //+1是为了把结束符也写入文件
    }
    fwrite(tableName, sizeof(char), strlen(tableName) + 1, dbfile);
    fwrite(&primaryKeyIndex, EDB_INT_SIZE, 1, dbfile);
    fclose(dbfile);
    return SUCCESS;
}

int edbOpen(const char* filename, EasyDB* db)
{
    if (filename == NULL || db == NULL) return NULL_PTR_ERROR;
    
    db->dbfilename = (char*)malloc((strlen(filename) + 10) * sizeof(char));
    strcpy(db->dbfilename, filename);
    FILE* dbfile = fopen(db->dbfilename, "rb+");
    if (dbfile == NULL)
    {
        free(db->dbfilename); db->dbfilename = NULL;
        return FILE_OPEN_ERROR;
    }

    int readMagicNum;                                                       //魔数检查
    fread(&readMagicNum, 4, 1, dbfile);
    if (readMagicNum != EDB_MAGIC_NUMBER)
    {
        free(db->dbfilename); db->dbfilename = NULL;
        return MAGIC_NUMBER_ERROR;
    }

    fread(&db->version, 4, 1, dbfile);                                      //版本号检查

    fread(&db->rowCount, EDB_INT_SIZE, 1, dbfile);                          //读取行数
    fread(&db->rowSize, EDB_INT_SIZE, 1, dbfile);                           //读取行长度
    fread(&db->columnCount, EDB_INT_SIZE, 1, dbfile);                       //读取每行数据个数

    db->dataTypes = (size_t*)malloc(db->columnCount * sizeof(size_t));      //读取一行中每个数据的类型
    fread(db->dataTypes, EDB_INT_SIZE, db->columnCount, dbfile);

    db->dataSizes = (size_t*)malloc(db->columnCount * sizeof(size_t));       //读取一行中每个数据的长度
    fread(db->dataSizes, EDB_INT_SIZE, db->columnCount, dbfile);

    char colNameBuf[4096];                                                  //读取列名
    size_t colNameLen;
    db->columnNames = (char**)malloc(db->columnCount * sizeof(char*));
    db->colNameIndexHead = NULL;
    for (size_t i = 0; i < db->columnCount; i++)
    {
        colNameLen = 0;
        char c;
        do
        {
            c = fgetc(dbfile);
            colNameBuf[colNameLen++] = c;
        } while (c != 0 && c < 4096);
        db->columnNames[i] = (char*)malloc(colNameLen);
        strcpy(db->columnNames[i], colNameBuf);
        // 将列名和列索引插入哈希表
        IndexInsert(&db->colNameIndexHead, db->columnNames[i], strlen(db->columnNames[i]), (void*)i);
    }

    if (db->version >= 1)
    {
        db->tableName = (char*)malloc(4096 * sizeof(char));
        char c;
        int tableNameLen = 0;
        do
        {
            c = fgetc(dbfile);
            db->tableName[tableNameLen++] = c;
        } while (c != 0 && tableNameLen < 4096);
        
    }

    fread(&db->primaryKeyIndex, EDB_INT_SIZE, 1, dbfile);                        //读取主键索引

    db->dataOffset = (size_t*)malloc(db->columnCount * sizeof(size_t));
    size_t offset = 0;
    for (size_t i = 0; i < db->columnCount; i++)
    {
        db->dataOffset[i] = offset;
        switch (db->dataTypes[i])
        {
        case EDB_TYPE_INT:
            offset += EDB_INT_SIZE;
            break;
        case EDB_TYPE_REAL:
            offset += EDB_REAL_SIZE;
            break;
        case EDB_TYPE_TEXT:
            offset += db->dataSizes[i];
            break;
        case EDB_TYPE_BLOB:
            offset += db->dataSizes[i];
            break;
        default:
            break;
        }
    }

    db->dataFileOffset = ftell(dbfile); //记录数据开始的位置

    db->indexheads = (IndexNode**)calloc(db->columnCount, sizeof(IndexNode*));  //初始化索引表头指针

    db->head = (EDBRow*)malloc(sizeof(EDBRow));
    db->head->prev = NULL;
    db->tail = (EDBRow*)malloc(sizeof(EDBRow));
    db->tail->next = NULL;
    EDBRow* ptr = db->head;
    EDBRow* pre = db->head;
    char* rbuf = (char*)calloc(db->rowSize, sizeof(char));
    size_t cur_id = 0;
    while (fread(rbuf, 1, db->rowSize, dbfile))
    {
        ptr->next = (EDBRow*)malloc(sizeof(EDBRow));
        ptr = ptr->next;
        ptr->prev = pre;
        ptr->id = cur_id++;
        ptr->data = (void**)malloc(db->columnCount * sizeof(void*));
        for (size_t i = 0; i < db->columnCount; i++)                          //读取一行中多个数据
        {
            ptr->data[i] = (void*)malloc(db->dataSizes[i]);
            memcpy(ptr->data[i], rbuf + db->dataOffset[i], db->dataSizes[i]);
            if (i != db->primaryKeyIndex)                                     //非主键的情况下，将数据插入索引
            {
                switch (db->dataTypes[i])
                {
                case EDB_TYPE_TEXT:
                    IndexInsert(&db->indexheads[i], ptr->data[i], strlen(ptr->data[i]), ptr);
                    break;
                default:
                    IndexInsert(&db->indexheads[i], ptr->data[i], db->dataSizes[i], ptr);
                    break;
                }
            }
            
        }
        size_t primaryKeyIndex = db->primaryKeyIndex;
        void* primaryKeyData = ptr->data[primaryKeyIndex];
        switch (db->dataTypes[db->primaryKeyIndex])
        {
        case EDB_TYPE_TEXT:
            IndexInsert(&db->indexheads[primaryKeyIndex], primaryKeyData, strlen(primaryKeyData), ptr);
            break;
        default:
            IndexInsert(&db->indexheads[primaryKeyIndex], primaryKeyData, db->dataSizes[primaryKeyIndex], ptr);
            break;
        }
        pre = pre->next;
    }
    pre->next = db->tail;
    db->tail->prev = ptr;

    db->tmpptr = db->head;

    srand(time(NULL));
    free(rbuf);
    fclose(dbfile);
    return SUCCESS;
}

int edbCloseNotSave(EasyDB *db)
{
    if (db == NULL) return NULL_PTR_ERROR;
    
    EDBRow* ptr = db->head->next;
    EDBRow* pre = db->head->next;
    free(db->head); db->head = NULL;
    for (size_t i = 0; i < db->rowCount && ptr != db->tail; i++)
    {
        for (size_t j = 0; j < db->columnCount; j++)
        {
            free(ptr->data[j]);
        }
        free(ptr->data);
        ptr = ptr->next;
        free(pre);
        pre = ptr;
    }
    free(ptr);

    for (size_t i = 0; i < db->columnCount; i++)
    {
        IndexClear(&db->indexheads[i]);
    }
    
    for (size_t i = 0; i < db->columnCount; i++) free(db->columnNames[i]);
    free(db->columnNames); db->columnNames = NULL;
    free(db->dataTypes); db->dataTypes = NULL;
    free(db->dataOffset); db->dataOffset = NULL;
    free(db->dataSizes); db->dataSizes = NULL;
    free(db->indexheads); db->indexheads = NULL;
    free(db->dbfilename); db->dbfilename = NULL;
    if (db->version >= 1)
    {
        free(db->tableName); db->tableName = NULL;
    }
    memset(db, 0, sizeof(EasyDB));
    return SUCCESS;
}

int edbClose(EasyDB *db)
{
    if (db == NULL) return NULL_PTR_ERROR;
    
    edbSave(db);
    
    EDBRow* ptr = db->head->next;
    EDBRow* pre = db->head->next;
    free(db->head); db->head = NULL;
    for (size_t i = 0; i < db->rowCount && ptr != db->tail; i++)
    {
        for (size_t j = 0; j < db->columnCount; j++)
        {
            free(ptr->data[j]);
        }
        free(ptr->data);
        ptr = ptr->next;
        free(pre);
        pre = ptr;
    }
    free(ptr);

    for (size_t i = 0; i < db->columnCount; i++)
    {
        IndexClear(&db->indexheads[i]);
    }
    IndexClear(&db->colNameIndexHead);
    
    for (size_t i = 0; i < db->columnCount; i++) free(db->columnNames[i]);
    free(db->columnNames); db->columnNames = NULL;
    free(db->dataTypes); db->dataTypes = NULL;
    free(db->dataOffset); db->dataOffset = NULL;
    free(db->dataSizes); db->dataSizes = NULL;
    free(db->indexheads); db->indexheads = NULL;
    free(db->dbfilename); db->dbfilename = NULL;
    if (db->version >= 1)
    {
        free(db->tableName); db->tableName = NULL;
    }
    memset(db, 0, sizeof(EasyDB));
    return SUCCESS;
}

int edbSave(EasyDB *db)
{
    if (db == NULL) return NULL_PTR_ERROR;

    FILE* dbfile = fopen(db->dbfilename, "wb+");

    // 写文件头
    int magicNum = EDB_MAGIC_NUMBER;
    fwrite(&magicNum, 4, 1, dbfile);
    fwrite(&db->version, 4, 1, dbfile);
    fwrite(&db->rowCount, 8, 1, dbfile);
    fwrite(&db->rowSize, 8, 1, dbfile);
    fwrite(&db->columnCount, 8, 1, dbfile);
    fwrite(db->dataTypes, 8, db->columnCount, dbfile);
    fwrite(db->dataSizes, 8, db->columnCount, dbfile);
    for (size_t i = 0; i < db->columnCount; i++)
    {
        fwrite(db->columnNames[i], 1, strlen(db->columnNames[i]) + 1, dbfile);
    }
    if (db->version >= 1)
    {
        fwrite(db->tableName, 1, strlen(db->tableName) + 1, dbfile);
    }
    fwrite(&db->primaryKeyIndex, 8, 1, dbfile);

    // 记录内容
    EDBRow* ptr = db->head->next;
    for (size_t i = 0; i < db->rowCount && ptr != db->tail; i++)
    {
        for (size_t j = 0; j < db->columnCount; j++)
        {
            fwrite(ptr->data[j], 1, db->dataSizes[j], dbfile);
        }
        ptr = ptr->next;
    }
    fclose(dbfile);
    return SUCCESS;
}


int edbInsert(EasyDB *db, void* row[])
{
    if (db == NULL || row == NULL) return NULL_PTR_ERROR;
    int retval = 0;
    size_t primaryKeyIndex = db->primaryKeyIndex;
    switch (db->dataTypes[db->primaryKeyIndex])
    {
    case EDB_TYPE_TEXT:
        retval = IndexFind(&db->indexheads[primaryKeyIndex], row[primaryKeyIndex], strlen(row[primaryKeyIndex]), NULL, 0);
        break;
    default:
        retval = IndexFind(&db->indexheads[primaryKeyIndex], row[primaryKeyIndex], db->dataSizes[primaryKeyIndex], NULL, 0);
        break;
    }
    if (retval) return PRIMARY_KEY_NOT_UNIQUE;

    EDBRow* ptr = db->tail->prev;
    EDBRow* pre = db->tail->prev;
    ptr->next = (EDBRow*)malloc(sizeof(EDBRow));
    ptr = ptr->next;
    ptr->prev = pre;
    ptr->next = db->tail;
    ptr->id = (db->rowCount == 0) ? 0 : ptr->prev->id + 1;
    db->tail->prev = ptr;
    db->tmpptr = ptr;

    ptr->data = (void**)malloc(db->columnCount * sizeof(void*));
    for (size_t i = 0; i < db->columnCount; i++)
    {
        ptr->data[i] = (void*)calloc(db->dataSizes[i], 1);
        if (row[i] == NULL) continue;
        if (db->dataTypes[i] == EDB_TYPE_TEXT)
        {
            strncpy(ptr->data[i], row[i], db->dataSizes[i]);
            ((char*)ptr->data[i])[db->dataSizes[i] - 1] = 0;
        }
        else memcpy(ptr->data[i], row[i], db->dataSizes[i]);
        if (i != db->primaryKeyIndex)                                          //非主键的情况下，将数据插入索引
        {
            switch (db->dataTypes[i])
            {
            case EDB_TYPE_TEXT:
                IndexInsert(&db->indexheads[i], ptr->data[i], strlen(ptr->data[i]), ptr);
                break;
            default:
                IndexInsert(&db->indexheads[i], ptr->data[i], db->dataSizes[i], ptr);
                break;
            }
        }
    }
    void* primaryKeyData = ptr->data[primaryKeyIndex];
    switch (db->dataTypes[db->primaryKeyIndex])
    {
    case EDB_TYPE_TEXT:
        IndexInsert(&db->indexheads[primaryKeyIndex], primaryKeyData, strlen(primaryKeyData), ptr);
        break;
    default:
        IndexInsert(&db->indexheads[primaryKeyIndex], primaryKeyData, db->dataSizes[primaryKeyIndex], ptr);
        break;
    }
    db->rowCount += 1;
    return SUCCESS;
}

int edbNodeDelete(EasyDB *db, EDBRow* row)
{
    if (row == NULL || db == NULL) return NULL_PTR_ERROR;

    EDBRow *pre = row->prev;
    EDBRow *next = row->next;

    pre->next = row->next;
    next->prev = row->prev;
    
    for (size_t i = 0; i < db->columnCount; i++)
    {
        switch (db->dataTypes[i])
        {
        case EDB_TYPE_TEXT:
            IndexDel(&db->indexheads[i], row->data[i], strlen(row->data[i]), row);
            break;
        default:
            IndexDel(&db->indexheads[i], row->data[i], db->dataSizes[i], row);
            break;
        }
    }
    
    for (size_t i = 0; i < db->columnCount; i++)
    {
        free(row->data[i]);
    }
    free(row->data);
    free(row);
    return SUCCESS;
}

int edbPrimaryKeyIndex(EasyDB *db, void* primaryKey, EDBRow** indexResult)
{
    int retval = 0;
    EDBRow* findRes = NULL;
    size_t primaryKeyIndex = db->primaryKeyIndex;
    switch (db->dataTypes[primaryKeyIndex])
    {
    case EDB_TYPE_TEXT:
        retval = IndexFind(&db->indexheads[primaryKeyIndex], primaryKey, strlen(primaryKey), (void**)&findRes, 1);
        break;
    default:
        retval = IndexFind(&db->indexheads[primaryKeyIndex], primaryKey, db->dataSizes[primaryKeyIndex], (void**)&findRes, 1);
        break;
    }
    *indexResult = findRes;
    return SUCCESS;
}

int edbWhere(EasyDB *db, char* columnName, void* inKey, void*** findResults, size_t maxResultNumber, size_t *resultsCount)
{   
    if (db == NULL || inKey == NULL) return NULL_PTR_ERROR;
    long long columnIndex = toColumnIndex(db, columnName);
    if (columnIndex < 0) return COLUMN_NOT_FOUND;
    
    size_t retval = 0;
    switch (db->dataTypes[columnIndex])
    {
    case EDB_TYPE_TEXT:
        retval = IndexFind(&db->indexheads[columnIndex], inKey, strlen(inKey), (void**)findResults, maxResultNumber);
        break;
    default:
        retval = IndexFind(&db->indexheads[columnIndex], inKey, db->dataSizes[columnIndex], (void**)findResults, maxResultNumber);
        break;
    }
    if (findResults)
    {
        for (size_t i = 0; i < retval; i++)
        {
            findResults[i] = ((EDBRow*)(findResults[i]))->data;
        }
    }
    if (resultsCount)
    {
        *resultsCount = retval;
    }
    
    return SUCCESS;
}

size_t edbCount(EasyDB *db, char* columnName, void* inKey)
{
    if (db == NULL || columnName == NULL || inKey == NULL)
    {
        return NULL_PTR_ERROR;
    }
    
    int colIndex = toColumnIndex(db, columnName);
    return IndexFind(&db->indexheads[colIndex], inKey, db->dataSizes[colIndex], NULL, 0);
}

int edbDelete(EasyDB *db, void* primaryKey)
{
    if (db == NULL || primaryKey == NULL) return NULL_PTR_ERROR;
    
    EDBRow *findRes = NULL;
    edbPrimaryKeyIndex(db, primaryKey, &findRes);

    if (findRes == NULL) return KEY_NOT_FOUND;

    edbNodeDelete(db, findRes);
    db->rowCount -= 1;
    return SUCCESS;
}

int edbUpdate(EasyDB *db, void* primaryKey, char* updateColumnName, void* newData)
{
    if (db == NULL || primaryKey == NULL) return NULL_PTR_ERROR;
    long long updateColumnIndex = toColumnIndex(db, updateColumnName);
    if (updateColumnIndex < 0) return COLUMN_NOT_FOUND;

    EDBRow *findRes = NULL;
    edbPrimaryKeyIndex(db, primaryKey, &findRes);

    if (findRes == NULL) return KEY_NOT_FOUND;

    if (updateColumnIndex == db->primaryKeyIndex)
    {
        EDBRow *tmp = NULL;
        edbPrimaryKeyIndex(db, newData, &tmp);
        if (tmp) return PRIMARY_KEY_NOT_UNIQUE;
    }

    switch (db->dataTypes[updateColumnIndex])
    {
    case EDB_TYPE_TEXT:
        IndexDel(&db->indexheads[updateColumnIndex], findRes->data[updateColumnIndex], strlen(findRes->data[updateColumnIndex]), findRes);
        strncpy(findRes->data[updateColumnIndex], newData, db->dataSizes[updateColumnIndex]);
        ((char**)(findRes->data))[updateColumnIndex][db->dataSizes[updateColumnIndex] - 1] = 0;
        IndexInsert(&db->indexheads[updateColumnIndex], findRes->data[updateColumnIndex], strlen(findRes->data[updateColumnIndex]), findRes);
        break;
    default:
        IndexDel(&db->indexheads[updateColumnIndex], findRes->data[updateColumnIndex], db->dataSizes[updateColumnIndex], findRes);
        memcpy(findRes->data[updateColumnIndex], newData, db->dataSizes[updateColumnIndex]);
        IndexInsert(&db->indexheads[updateColumnIndex], findRes->data[updateColumnIndex], db->dataSizes[updateColumnIndex], findRes);
        break;
    }
    return SUCCESS;
}


long long toColumnIndex(EasyDB *db, char *columnName)
{
    size_t result = 0;
    int retval = IndexFind(&db->colNameIndexHead, columnName, strlen(columnName), (void**)&result, 1);
    if (retval > 0)
    {
        return result;
    }
    return COLUMN_NOT_FOUND;
}

void** edbIterBegin(EasyDB *db)
{
    if (db->rowCount == 0 || db->head == NULL || db->head == db->tail) return NULL;

    EDBRow *ptr = db->head->next;
    db->tmpptr = ptr;
    return ptr->data;
}

void** edbIterNext(EasyDB *db)
{
    if (db->tmpptr->next == db->tail) return NULL;
    
    db->tmpptr = db->tmpptr->next;
    return db->tmpptr->data;
}

void* edbGet(EasyDB *db, void* primaryKey, char* columnName)
{
    if (db == NULL || primaryKey == NULL || columnName == NULL) return NULL;
    
    EDBRow* findResult = NULL;
    int retval = edbPrimaryKeyIndex(db, primaryKey, &findResult);
    if (findResult == NULL) return NULL;

    long long columnIndex = toColumnIndex(db, columnName);
    if (columnIndex < 0) return NULL;

    return findResult->data[columnIndex];
}

int edbSearch(EasyDB *db, char* columnName, char *keyWord, void*** findResults, size_t maxResultNumber, size_t *resultsCount)
{
    if (db == NULL || keyWord == NULL || findResults == NULL) return NULL_PTR_ERROR;
    long long columnIndex = toColumnIndex(db, columnName);
    if (columnIndex < 0) return COLUMN_NOT_FOUND;
    if (db->dataTypes[columnIndex] != EDB_TYPE_TEXT) return NOT_TEXT_COLUMN;
    
    size_t curResultsCount = 0;
    for (void** it = edbIterBegin(db); it != NULL; it = edbIterNext(db))
    {
        if (strstr((char*)it[columnIndex], keyWord))
        {
            findResults[curResultsCount++] = it;
        }
    }
    *resultsCount = curResultsCount;
    return SUCCESS;
}

int edbDeleteByArray(EasyDB *db, void** deleteRows[], size_t arraySize)
{
    if (db == NULL || deleteRows == NULL) return NULL_PTR_ERROR;
    
    int retvalRecv = 0;
    int retval = SUCCESS;
    for (size_t i = 0; i < arraySize; i++)
    {
        retvalRecv = edbDelete(db, deleteRows[i][db->primaryKeyIndex]);
        if (retvalRecv == KEY_NOT_FOUND) retval = KEY_NOT_FOUND;
    }
    return retval;
}

int edbDeleteByKeyword(EasyDB *db, char* columnName, char *keyword)
{
    if (db == NULL || keyword == NULL) return NULL_PTR_ERROR;

    void** searchResults[1000];
    size_t resultsCount = 0;
    int retval = 0;
    while (1)
    {
        retval = edbSearch(db, columnName, keyword, searchResults, 1000, &resultsCount);
        if (retval != SUCCESS) return retval;
        if (resultsCount <= 0) break;
        edbDeleteByArray(db, searchResults, resultsCount);
    }
    return SUCCESS;
}

int edbDeleteByKey(EasyDB *db, char* columnName, void* inKey)
{
    if (db == NULL || inKey == NULL) return NULL_PTR_ERROR;

    void** findReshults[1000];
    size_t resultsCount = 0;
    int retval = 0;
    while (1)
    {
        retval = edbWhere(db, columnName, inKey, findReshults, 1000, &resultsCount);
        if (retval != SUCCESS) return retval;
        if (resultsCount <= 0) break;
        edbDeleteByArray(db, findReshults, resultsCount);
    }
    return SUCCESS;
}

int edbDefaultCompareInts(const void *a, const void *b){return *(int*)a - *(int*)b;}
int edbDefaultCompareDoubles(const void *a, const void *b)
{
    if (*(double*)a - *(double*)b < 0) return -1;
    else if (*(double*)a - *(double*)b > 0) return 1;
    return 0;
}

int merge(EDBRow* l1head, EDBRow* l2head, int columnIndex, int (*compareFunc)(const void*, const void*), EDBRow** sortedHead, EDBRow** sortedTail)
{
    EDBRow tmp;
    EDBRow* ptr = &tmp;
    while (l1head != NULL && l2head!= NULL)
    {
        if (compareFunc(l1head->data[columnIndex], l2head->data[columnIndex]) < 0)
        {
            ptr->next = l1head;
            l1head->prev = ptr;
            l1head = l1head->next;
        }
        else
        {
            ptr->next = l2head;
            l2head->prev = ptr;
            l2head = l2head->next;
        }
        ptr = ptr->next;
    }
    if (l1head)
    {
        ptr->next = l1head;
        l1head->prev = ptr;
    }
    if (l2head)
    {
        ptr->next = l2head;
        l2head->prev = ptr;
    }
    while (ptr->next != NULL)
    {
        ptr = ptr->next;
    }
    *sortedTail = ptr;
    
    ptr = tmp.next;
    ptr->prev = NULL;
    *sortedHead = ptr;
    return 0;
}

EDBRow* findMiddle(EDBRow* head, EDBRow* tail)
{
    while (head != tail && head->next != tail)
    {
        head = head->next;
        tail = tail->prev;
    }
    return head;
}

int* mergeSort(EDBRow* head, EDBRow* tail, int columnIndex, int (*compareFunc)(const void*, const void*), EDBRow** sortedHead, EDBRow** sortedTail)
{
    if (head == NULL || head->next == NULL)
    {
        *sortedHead = head;
        *sortedTail = head;
        return 0;
    }
    
    EDBRow* mid = findMiddle(head, tail);
    EDBRow* rightHead = mid->next;
    rightHead->prev = NULL;
    mid->next = NULL;

    EDBRow *lHead, *lTail, *rHead, *rTail;
    mergeSort(head, mid, columnIndex, compareFunc, &lHead, &lTail);
    mergeSort(rightHead, tail, columnIndex, compareFunc, &rHead, &rTail);
    
    merge(lHead, rHead, columnIndex, compareFunc, sortedHead, sortedTail);
    return 0;
}

int edbSort(EasyDB *db, char* columnName, int (*compareFunc)(const void*, const void*))
{
    if (db == NULL || columnName == NULL)
    {
        return NULL_PTR_ERROR;
    }
    if (db->rowCount == 0)
    {
        return EMPTY_TABLE;
    }
    
    long long colIndex = toColumnIndex(db, columnName);
    if (compareFunc == NULL)
    {
        switch (db->dataTypes[colIndex])
        {
        case EDB_TYPE_INT:
            compareFunc = edbDefaultCompareInts;
            break;
        case EDB_TYPE_REAL:
            compareFunc = edbDefaultCompareDoubles;
            break;
        case EDB_TYPE_TEXT:
            compareFunc = (int(*)(const void *, const void *))strcmp;
            break;
        default:
            break;
        }
    }
    EDBRow* pHead = db->head;
    EDBRow* pTail = db->tail;

    EDBRow* realHead = pHead->next;
    EDBRow* realTail = pTail->prev;

    realHead->prev = NULL;
    realTail->next = NULL;

    EDBRow *sHead, *sTail;
    mergeSort(pHead->next, pTail->prev, colIndex, compareFunc, &sHead, &sTail);

    pHead->next = sHead;
    sHead->prev = pHead;
    pTail->prev = sTail;
    sTail->next = pTail;
    return 0;
}

char* uuid(char *UUID) 
{   
    unsigned char random_bytes[16];
    int i;

    // 使用随机数生成 16 字节的随机数
    for (i = 0; i < 16; ++i) {
        random_bytes[i] = rand() % 256;  // 随机生成一个字节
    }

    // 设置版本号为 4（UUID v4）
    random_bytes[6] = (random_bytes[6] & 0x0F) | 0x40;  // 版本号，4表示v4

    // 设置变异位（8-11位为0b10xx）
    random_bytes[8] = (random_bytes[8] & 0x3F) | 0x80;  // 变异位，要求最高位为1

    // 将字节转换为字符串
    sprintf(UUID, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x",
            random_bytes[0], random_bytes[1], random_bytes[2], random_bytes[3],
            random_bytes[4], random_bytes[5], random_bytes[6], random_bytes[7],
            random_bytes[8], random_bytes[9], random_bytes[10], random_bytes[11],
            random_bytes[12], random_bytes[13]);
    return UUID;
}

#define SHA256_ROTL(a,b) (((a>>(32-b))&(0x7fffffff>>(31-b)))|(a<<b))
#define SHA256_SR(a,b) ((a>>b)&(0x7fffffff>>(b-1)))
#define SHA256_Ch(x,y,z) ((x&y)^((~x)&z))
#define SHA256_Maj(x,y,z) ((x&y)^(x&z)^(y&z))
#define SHA256_E0(x) (SHA256_ROTL(x,30)^SHA256_ROTL(x,19)^SHA256_ROTL(x,10))
#define SHA256_E1(x) (SHA256_ROTL(x,26)^SHA256_ROTL(x,21)^SHA256_ROTL(x,7))
#define SHA256_O0(x) (SHA256_ROTL(x,25)^SHA256_ROTL(x,14)^SHA256_SR(x,3))
#define SHA256_O1(x) (SHA256_ROTL(x,15)^SHA256_ROTL(x,13)^SHA256_SR(x,10))
char* sha256(const char* input, char* SHA256)
{
    char *pp, *ppend;
    size_t length = strlen(input);
    unsigned int l, i, W[64], T1, T2, A, B, C, D, E, F, G, H, H0, H1, H2, H3, H4, H5, H6, H7;
    H0 = 0x6a09e667, H1 = 0xbb67ae85, H2 = 0x3c6ef372, H3 = 0xa54ff53a;
    H4 = 0x510e527f, H5 = 0x9b05688c, H6 = 0x1f83d9ab, H7 = 0x5be0cd19;
    unsigned long K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };
    l = length + ((length % 64 >= 56) ? (128 - length % 64) : (64 - length % 64));
    if (!(pp = (char*)malloc((unsigned long)l))) return 0;
    for (i = 0; i < length; pp[i + 3 - 2 * (i % 4)] = input[i], i++);
    for (pp[i + 3 - 2 * (i % 4)] = 128, i++; i < l; pp[i + 3 - 2 * (i % 4)] = 0, i++);
    *((long*)(pp + l - 4)) = length << 3;
    *((long*)(pp + l - 8)) = length >> 29;
    for (ppend = pp + l; pp < ppend; pp += 64){
        for (i = 0; i < 16; W[i] = ((long*)pp)[i], i++);
        for (i = 16; i < 64; W[i] = (SHA256_O1(W[i - 2]) + W[i - 7] + SHA256_O0(W[i - 15]) + W[i - 16]), i++);
        A = H0, B = H1, C = H2, D = H3, E = H4, F = H5, G = H6, H = H7;
        for (i = 0; i < 64; i++){
            T1 = H + SHA256_E1(E) + SHA256_Ch(E, F, G) + K[i] + W[i];
            T2 = SHA256_E0(A) + SHA256_Maj(A, B, C);
            H = G, G = F, F = E, E = D + T1, D = C, C = B, B = A, A = T1 + T2;
        }
        H0 += A, H1 += B, H2 += C, H3 += D, H4 += E, H5 += F, H6 += G, H7 += H;
    }
    free(pp - l);
    sprintf(SHA256, "%08x%08x%08x%08x%08x%08x%08x%08x", H0, H1, H2, H3, H4, H5, H6, H7);
    return SHA256;
}

int easyLogin(EasyDB *db, char* userID, char* password, void*** retUserData)
{
    *retUserData = NULL;
    if (db == NULL || userID == NULL || password == NULL) return NULL_PTR_ERROR;
    
    long long passwdColIndex = toColumnIndex(db, "password");
    if (passwdColIndex == COLUMN_NOT_FOUND) return COLUMN_NOT_FOUND;
    
    EDBRow* user = NULL;
    edbPrimaryKeyIndex(db, userID, &user);
    if (user == NULL) return KEY_NOT_FOUND;

    char passwd_sha256[70];
    sha256(password, passwd_sha256);

    void **userData = user->data;
    if (!strcmp(userData[passwdColIndex], passwd_sha256))
    {
        *retUserData = userData;
        return SUCCESS;
    }
    else
    {
        return PASSWORD_WRONG;
    }
}

int easyAddUser(EasyDB *db, void* newRow[])
{
    if (db == NULL || newRow == NULL) return NULL_PTR_ERROR;

    long long passwdColIndex = toColumnIndex(db, "password");
    if (passwdColIndex == COLUMN_NOT_FOUND) return COLUMN_NOT_FOUND;

    int retval = edbInsert(db, newRow);
    if (retval != SUCCESS) return retval;

    char *password = newRow[passwdColIndex];
    char passwd_sha256[70] = {0};
    sha256(password, passwd_sha256);

    retval = edbUpdate(db, newRow[db->primaryKeyIndex], "password", passwd_sha256);
    if (retval != SUCCESS) return retval;

    return SUCCESS;
}

int easyDeleteUser(EasyDB *db, char* userID)
{
    if (db == NULL || userID == NULL) return NULL_PTR_ERROR;
    int retval = edbDelete(db, userID);
    return retval;
}

int easyResetPassword(EasyDB *db, char* userID, char* newPassword)
{
    if (db == NULL || userID == NULL || newPassword == NULL) return NULL_PTR_ERROR;

    long long passwdColIndex = toColumnIndex(db, "password");
    if (passwdColIndex == COLUMN_NOT_FOUND) return COLUMN_NOT_FOUND;

    char newpasswd_sha256[70];
    sha256(newPassword, newpasswd_sha256);

    int retval = edbUpdate(db, userID, "password", newpasswd_sha256);
    return retval;
}

int edbImportCSV(EasyDB* db, char* csvFileName)
{
    if (db == NULL || csvFileName == NULL)
    {
        return NULL_PTR_ERROR;
    }
    
    FILE* csvFile = fopen(csvFileName, "r");
    if (csvFile == NULL)
    {
        return FILE_OPEN_ERROR;
    }

    char* rowBuf = (char*)malloc(db->rowSize * sizeof(char) * 2);
    fgets(rowBuf, db->rowSize, csvFile);    // 跳过表头
    int retval = 0;

    void** newRow = (void**)calloc(db->columnCount, sizeof(void*));
    for (size_t i = 0; i < db->columnCount; i++)
    {
        newRow[i] = calloc(db->dataSizes[i], 1);
    }
    
    bool isInQuot = 0;
    while (fgets(rowBuf, db->rowSize, csvFile) != NULL)
    {
        if (strchr(rowBuf, '\n')) *(strchr(rowBuf, '\n')) = 0;    // 移除换行符

        for (size_t i = 0; i < strlen(rowBuf); i++)
        {
            if (rowBuf[i] == '\"' && isInQuot) isInQuot = 0;
            else if (rowBuf[i] == '\"' && !isInQuot) isInQuot = 1;
            else if (rowBuf[i] == ',' && isInQuot) rowBuf[i] = '\x1F';  // 在双引号中的逗号替换为不可见字符
        }

        char* token = strtok(rowBuf, ",");  // 逗号分隔
        size_t cnt = 0;

        for (size_t i = 0; i < db->columnCount; i++)    // 清空上一行的内容
        {
            memset(newRow[i], 0, db->dataSizes[i]);
        }

        do
        {
            switch (db->dataTypes[cnt])
            {
            case EDB_TYPE_INT:
                sscanf(token, "%lld", newRow[cnt]);
                break;
            case EDB_TYPE_REAL:
                sscanf(token, "%lf", newRow[cnt]);
                break;
            case EDB_TYPE_TEXT:
                // 替换回逗号
                while (strchr(token, '\x1F'))
                {
                    *(strchr(token, '\x1F')) = ',';
                }
                
                // 移除头尾双引号
                if (token[0] == '\"' && token[strlen(token) - 1] == '\"')
                {
                    token[strlen(token) - 1] = 0;
                    ++token;
                }
                
                strcpy(newRow[cnt], token);
                break;
            default:
                break;
            }

            token = strtok(NULL, ",");
            ++cnt;
        } while (token != NULL && cnt < db->columnCount);
        edbInsert(db, newRow);
    }

    free(rowBuf);
    for (size_t i = 0; i < db->columnCount; i++)
    {
        free(newRow[i]);
    }
    free(newRow);
    fclose(csvFile);
    return SUCCESS;
}

int edbExportCSV(EasyDB* db, char* csvFileName, bool withBOM)
{
    if (db == NULL || csvFileName == NULL)
    {
        return NULL_PTR_ERROR;
    }
    
    FILE* csvFile = fopen(csvFileName, "w");
    if (csvFile == NULL)
    {
        return FILE_OPEN_ERROR;
    }

    if (withBOM)    // 写入BOM标识
    {
        fputs("\xEF\xBB\xBF", csvFile);
    }

    for (size_t i = 0; i < db->columnCount - 1; i++)    // 先打出表头
    {
        fprintf(csvFile, "%s,", db->columnNames[i]);
    }
    fprintf(csvFile, "%s\n", db->columnNames[db->columnCount - 1]);

    for (void** it = edbIterBegin(db); it != NULL; it = edbIterNext(db))
    {
        size_t i;
        for (i = 0; i < db->columnCount - 1; i++)
        {
            switch (db->dataTypes[i])
            {
            case EDB_TYPE_INT:
                fprintf(csvFile, "%lld,", Int(it[i]));
                break;
            case EDB_TYPE_REAL:
                fprintf(csvFile, "%lf,", Real(it[i]));
                break;
            case EDB_TYPE_TEXT:
                if (strchr(Text(it[i]), ','))   // 若含有逗号，则写入双引号
                {
                    fputc('\"', csvFile);
                    fputs(Text(it[i]), csvFile);
                    fputc('\"', csvFile);
                }
                else
                {
                    fputs(Text(it[i]), csvFile);
                }
                fputc(',', csvFile);
            default:
                break;
            }
        }
        i = db->columnCount - 1;
        switch (db->dataTypes[i])   // 最后一个数据不加逗号
        {
        case EDB_TYPE_INT:
            fprintf(csvFile, "%lld", Int(it[i]));
            break;
        case EDB_TYPE_REAL:
            fprintf(csvFile, "%lf", Real(it[i]));
            break;
        case EDB_TYPE_TEXT:
            fputs(Text(it[i]), csvFile);
        default:
            break;
        }
        fputc('\n', csvFile);       // 写入换行符
    }
    
    fclose(csvFile);
    return SUCCESS;
}