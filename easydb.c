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

    size_t emptyLineCount = 0;
    fwrite(&emptyLineCount, EDB_INT_SIZE, 1, dbfile);

    size_t lineSize = 0;
    for (size_t i = 0; i < columnCount; i++)
    {
        switch (dataTypes[i])
        {
        case EDB_TYPE_INT:
            lineSize += EDB_INT_SIZE;
            dataSizes[i] = EDB_INT_SIZE;
            break;
        case EDB_TYPE_REAL:
            lineSize += EDB_REAL_SIZE;
            dataSizes[i] = EDB_REAL_SIZE;
            break;
        case EDB_TYPE_TEXT:
            lineSize += dataSizes[i];
            break;
        case EDB_TYPE_BLOB:
            lineSize += dataSizes[i];
            break;
        default:
            break;
        }
    }
    fwrite(&lineSize, EDB_INT_SIZE, 1, dbfile);

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
        free(db->dbfilename);
        return FILE_OPEN_ERROR;
    }

    int readMagicNum;                                                       //魔数检查
    fread(&readMagicNum, 4, 1, dbfile);
    if (readMagicNum != EDB_MAGIC_NUMBER)
    {
        free(db->dbfilename);
        return MAGIC_NUMBER_ERROR;
    }

    fread(&db->version, 4, 1, dbfile);                                      //版本号检查

    fread(&db->rowCount, EDB_INT_SIZE, 1, dbfile);                          //读取行数
    fread(&db->lineSize, EDB_INT_SIZE, 1, dbfile);                           //读取行长度
    fread(&db->columnCount, EDB_INT_SIZE, 1, dbfile);                       //读取每行数据个数

    db->dataTypes = (size_t*)malloc(db->columnCount * sizeof(size_t));      //读取一行中每个数据的类型
    fread(db->dataTypes, EDB_INT_SIZE, db->columnCount, dbfile);

    db->dataSizes = (size_t*)malloc(db->columnCount * sizeof(size_t));       //读取一行中每个数据的长度
    fread(db->dataSizes, EDB_INT_SIZE, db->columnCount, dbfile);

    char colNameBuf[1024];                                                  //读取列名
    size_t colNameLen;
    db->columnNames = (char**)malloc(db->columnCount * sizeof(char*));
    for (size_t i = 0; i < db->columnCount; i++)
    {
        colNameLen = 0;
        char c;
        do
        {
            c = fgetc(dbfile);
            colNameBuf[colNameLen++] = c;
        } while (c != 0 && c < 1024);
        db->columnNames[i] = (char*)malloc(colNameLen);
        strcpy(db->columnNames[i], colNameBuf);
    }

    if (db->version >= 1)
    {
        db->tableName = (char*)malloc(1024 * sizeof(char));
        char c;
        int tableNameLen = 0;
        do
        {
            c = fgetc(dbfile);
            db->tableName[tableNameLen++] = c;
        } while (c != 0 && tableNameLen < 1024);
        
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

    db->dataFileOffset = ftell(dbfile);                                     //记录数据开始的位置

    db->indexheads = (IndexNode**)calloc(db->columnCount, sizeof(IndexNode*));                //初始化索引表头指针

    db->head = (EDBRow*)malloc(sizeof(EDBRow));
    db->head->prev = NULL;
    db->tail = (EDBRow*)malloc(sizeof(EDBRow));
    db->tail->next = NULL;
    EDBRow* ptr = db->head;
    EDBRow* pre = db->head;
    char rbuf[db->lineSize];
    size_t cur_id = 0;
    while (fread(rbuf, 1, db->lineSize, dbfile))
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

    fclose(dbfile);
    return SUCCESS;
}

int edbClose(EasyDB *db)
{
    if (db == NULL) return NULL_PTR_ERROR;
    
    char fileHead[db->dataFileOffset];
    FILE* dbfileReadHead = fopen(db->dbfilename, "rb+");
    fread(fileHead, 1, db->dataFileOffset, dbfileReadHead);
    *(size_t*)&fileHead[4 + 4] = db->rowCount;
    fclose(dbfileReadHead);

    FILE* dbfile = fopen(db->dbfilename, "wb+");
    fwrite(fileHead, 1, db->dataFileOffset, dbfile);
    EDBRow* ptr = db->head->next;
    EDBRow* pre = db->head->next;
    free(db->head);
    for (size_t i = 0; i < db->rowCount && ptr != db->tail; i++)
    {
        for (size_t j = 0; j < db->columnCount; j++)
        {
            fwrite(ptr->data[j], 1, db->dataSizes[j], dbfile);
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
    free(db->columnNames);
    free(db->dataTypes);
    free(db->dataOffset);
    free(db->dataSizes);
    free(db->indexheads);
    free(db->dbfilename);
    if (db->version >= 1)
    {
        free(db->tableName);
    }
    fclose(dbfile);
    return SUCCESS;
}

int edbSave(EasyDB *db)
{
    if (db == NULL) return NULL_PTR_ERROR;
    
    char fileHead[db->dataFileOffset];
    FILE* dbfileReadHead = fopen(db->dbfilename, "rb+");
    fread(fileHead, 1, db->dataFileOffset, dbfileReadHead);
    *(size_t*)&fileHead[4 + 4] = db->rowCount;
    fclose(dbfileReadHead);

    FILE* dbfile = fopen(db->dbfilename, "wb+");
    fwrite(fileHead, 1, db->dataFileOffset, dbfile);
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
    long long columnIndex = columnNameToColumnIndex(db, columnName);
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
    for (size_t i = 0; i < retval; i++)
    {
        findResults[i] = ((EDBRow*)(findResults[i]))->data;
    }
    if (resultsCount != NULL)
    {
        *resultsCount = retval;
    }
    
    return SUCCESS;
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
    long long updateColumnIndex = columnNameToColumnIndex(db, updateColumnName);
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


long long columnNameToColumnIndex(EasyDB *db, char *columnName)
{
    for (size_t i = 0; i < db->columnCount; i++)
    {
        if (!strcmp(db->columnNames[i], columnName))
        {
            return i;
        }
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

    long long columnIndex = columnNameToColumnIndex(db, columnName);
    if (columnIndex < 0) return NULL;

    return findResult->data[columnIndex];
}

int edbSearch(EasyDB *db, char* columnName, char *keyWord, void*** findResults, size_t maxResultNumber, size_t *resultsCount)
{
    if (db == NULL || keyWord == NULL || findResults == NULL) return NULL_PTR_ERROR;
    long long columnIndex = columnNameToColumnIndex(db, columnName);
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
    while (1)
    {
        edbSearch(db, columnName, keyword, searchResults, 1000, &resultsCount);
        if (resultsCount <= 0) break;
        edbDeleteByArray(db, searchResults, resultsCount);
    }
    return SUCCESS;
}

int edbDeleteByKey(EasyDB *db, char* columnName, void* inKey)
{
    if (db == NULL || inKey == NULL) return NULL_PTR_ERROR;

    void** findReshults[1000];
    size_t reshultsCount = 0;
    while (1)
    {
        edbWhere(db, columnName, inKey, findReshults, 1000, &reshultsCount);
        if (reshultsCount <= 0) break;
        edbDeleteByArray(db, findReshults, reshultsCount);
    }
    return SUCCESS;
}


char* uuid(char *UUID) 
{
    struct timeval now;
    gettimeofday(&now, NULL);
    srand((unsigned int)(now.tv_sec * 1000 + now.tv_usec));  // 设置随机数种子
    
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
    
    long long passwdColIndex = columnNameToColumnIndex(db, "password");
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

    long long passwdColIndex = columnNameToColumnIndex(db, "password");
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

    long long passwdColIndex = columnNameToColumnIndex(db, "password");
    if (passwdColIndex == COLUMN_NOT_FOUND) return COLUMN_NOT_FOUND;

    char newpasswd_sha256[70];
    sha256(newPassword, newpasswd_sha256);

    int retval = edbUpdate(db, userID, "password", newpasswd_sha256);
    return retval;
}