#include "easydb.h"
#include "index.h"

int edbCreate(const char* filename, size_t columnCount, size_t dataType[], size_t dataLens[], size_t primaryKeyIndex, char* columnNames[])
{
    if (filename == NULL) return NULL_PTR_ERROR;
    if (dataType[primaryKeyIndex] != EDB_TYPE_INT && dataType[primaryKeyIndex] != EDB_TYPE_TEXT) return PRIMARY_KEY_TYPE_CANNOT_INDEX;
    
    FILE* dbfile = fopen64(filename, "wb+");
    if (dbfile == NULL) return FILE_OPEN_ERROR;

    int magicNum = EDB_MAGIC_NUMBER;
    fwrite(&magicNum, 4, 1, dbfile);

    int ver = EDB_VERSION;
    fwrite(&ver, 4, 1, dbfile);

    size_t emptyLineCount = 0;
    fwrite(&emptyLineCount, EDB_INT_SIZE, 1, dbfile);

    size_t lineLen = 0;
    for (size_t i = 0; i < columnCount; i++)
    {
        switch (dataType[i])
        {
        case EDB_TYPE_INT:
            lineLen += EDB_INT_SIZE;
            dataLens[i] = EDB_INT_SIZE;
            break;
        case EDB_TYPE_REAL:
            lineLen += EDB_REAL_SIZE;
            dataLens[i] = EDB_REAL_SIZE;
            break;
        case EDB_TYPE_TEXT:
            lineLen += dataLens[i];
            break;
        case EDB_TYPE_BLOB:
            lineLen += dataLens[i];
            break;
        default:
            break;
        }
    }
    fwrite(&lineLen, EDB_INT_SIZE, 1, dbfile);

    fwrite(&columnCount, EDB_INT_SIZE, 1, dbfile);
    fwrite(dataType, EDB_INT_SIZE, columnCount, dbfile);
    fwrite(dataLens, EDB_INT_SIZE, columnCount, dbfile);
    for (size_t i = 0; i < columnCount; i++)
    {
        fwrite(columnNames[i], sizeof(char), strlen(columnNames[i]) + 1, dbfile); //+1是为了把结束符也写入文件
    }
    fwrite(&primaryKeyIndex, EDB_INT_SIZE, 1, dbfile);
    fclose(dbfile);
    return 0;
}

int edbOpen(const char* filename, EasyDB* db)
{
    if (filename == NULL || db == NULL) return NULL_PTR_ERROR;
    
    strcpy(db->dbfilename, filename);
    FILE* dbfile = fopen64(db->dbfilename, "rb+");
    if (dbfile == NULL) return FILE_OPEN_ERROR;

    int readMagicNum;                                                       //魔数检查
    fread(&readMagicNum, 4, 1, dbfile);
    if (readMagicNum != EDB_MAGIC_NUMBER) return MAGIC_NUMBER_ERROR;

    int ver;                                                                //版本号检查 
    fread(&ver, 4, 1, dbfile);

    fread(&db->lineCount, EDB_INT_SIZE, 1, dbfile);                         //读取行数
    fread(&db->lineLen, EDB_INT_SIZE, 1, dbfile);                           //读取行长度
    fread(&db->columnCount, EDB_INT_SIZE, 1, dbfile);                         //读取每行数据个数

    db->dataType = (size_t*)malloc(db->columnCount * sizeof(size_t));         //读取一行中每个数据的类型
    fread(db->dataType, EDB_INT_SIZE, db->columnCount, dbfile);

    db->dataLens = (size_t*)malloc(db->columnCount * sizeof(size_t));         //读取一行中每个数据的长度
    fread(db->dataLens, EDB_INT_SIZE, db->columnCount, dbfile);

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
        } while (c != 0);
        db->columnNames[i] = (char*)malloc(colNameLen);
        strcpy(db->columnNames[i], colNameBuf);
    }

    fread(&db->primaryKey, EDB_INT_SIZE, 1, dbfile);                        //读取主键索引

    db->dataOffset = (size_t*)malloc(db->columnCount * sizeof(size_t));       //记录文件头长度
    size_t offset = 0;
    for (size_t i = 0; i < db->columnCount; i++)
    {
        db->dataOffset[i] = offset;
        switch (db->dataType[i])
        {
        case EDB_TYPE_INT:
            offset += EDB_INT_SIZE;
            break;
        case EDB_TYPE_REAL:
            offset += EDB_REAL_SIZE;
            break;
        case EDB_TYPE_TEXT:
            offset += db->dataLens[i];
            break;
        case EDB_TYPE_BLOB:
            offset += db->dataLens[i];
            break;
        default:
            break;
        }
    }

    db->dataFileOffset = ftello64(dbfile);                                  //记录数据开始的位置

    db->indexheads = calloc(db->columnCount, sizeof(void*));                 //初始化索引表头指针

    db->head = (EDBRow*)malloc(sizeof(EDBRow));
    db->head->prev = NULL;
    db->tail = (EDBRow*)malloc(sizeof(EDBRow));
    db->tail->next = NULL;
    EDBRow* ptr = db->head;
    EDBRow* pre = db->head;
    char rbuf[db->lineLen];
    size_t cur_id = 0;
    while (fread(rbuf, 1, db->lineLen, dbfile))
    {
        ptr->next = (EDBRow*)malloc(sizeof(EDBRow));
        ptr = ptr->next;
        ptr->prev = pre;
        ptr->id = cur_id++;
        ptr->data = (void**)malloc(db->columnCount * sizeof(void*));
        for (size_t i = 0; i < db->columnCount; i++)                          //读取一行中多个数据
        {
            ptr->data[i] = (void*)malloc(db->dataLens[i]);
            memcpy(ptr->data[i], rbuf + db->dataOffset[i], db->dataLens[i]);
            if (i != db->primaryKey)                                          //非主键的情况下，将数据插入索引
            {
                switch (db->dataType[i])
                {
                case EDB_TYPE_INT:
                    IndexAddINT((IndexINTNode**)&db->indexheads[i], *(edb_int*)ptr->data[i], ptr->data);
                    break;
                case EDB_TYPE_TEXT:
                    IndexAddTEXT((IndexTEXTNode**)&db->indexheads[i], (char*)ptr->data[i], ptr->data);
                    break;
                default:
                    break;
                }
            }
            
        }
        switch (db->dataType[db->primaryKey])
        {
        case EDB_TYPE_INT:
            IndexAddINT((IndexINTNode**)&db->indexheads[db->primaryKey], *(edb_int*)ptr->data[db->primaryKey], ptr->data);
            break;
        case EDB_TYPE_TEXT:
            IndexAddTEXT((IndexTEXTNode**)&db->indexheads[db->primaryKey], (char*)ptr->data[db->primaryKey], ptr->data);
            break;
        default:
            break;
        }
        pre = pre->next;
    }
    pre->next = db->tail;
    db->tail->prev = ptr;

    db->tmpptr = db->head;

    fclose(dbfile);
    return 0;
}

int edbInsert(EasyDB *db, void* row[])
{
    if (db == NULL || row == NULL) return NULL_PTR_ERROR;
    int retval = 0;
    switch (db->dataType[db->primaryKey])
    {
    case EDB_TYPE_INT:
        retval = IndexFindINT((IndexINTNode**)&db->indexheads[db->primaryKey], *(edb_int*)row[db->primaryKey], NULL, 0);
        if (retval) return PRIMARY_KEY_NOT_UNIQUE;
        break;
    case EDB_TYPE_TEXT:
        retval = IndexFindTEXT((IndexTEXTNode**)&db->indexheads[db->primaryKey], (char*)row[db->primaryKey], NULL, 0);
        if (retval) return PRIMARY_KEY_NOT_UNIQUE;
        break;
    default:
        break;
    }

    EDBRow* ptr = db->tail->prev;
    EDBRow* pre = db->tail->prev;
    ptr->next = (EDBRow*)malloc(sizeof(EDBRow));
    ptr = ptr->next;
    ptr->prev = pre;
    ptr->next = db->tail;
    ptr->id = (db->lineCount == 0) ? 0 : ptr->prev->id + 1;
    db->tail->prev = ptr;
    db->tmpptr = ptr;

    ptr->data = (void**)malloc(db->columnCount * sizeof(void*));
    for (size_t i = 0; i < db->columnCount; i++)
    {
        ptr->data[i] = (void*)calloc(db->dataLens[i], 1);
        if (db->dataType[i] >= EDB_TYPE_TEXT) strcpy(ptr->data[i], row[i]);
        else memcpy(ptr->data[i], row[i], db->dataLens[i]);
    }
    switch (db->dataType[db->primaryKey])
    {
        case EDB_TYPE_INT:
            IndexAddINT((IndexINTNode**)&db->indexheads[db->primaryKey], *(edb_int*)ptr->data[db->primaryKey], ptr->data);
            break;
        case EDB_TYPE_TEXT:
            IndexAddTEXT((IndexTEXTNode**)&db->indexheads[db->primaryKey], (char*)ptr->data[db->primaryKey], ptr->data);
            break;
        default:
            break;
    }
    db->lineCount += 1;
    return 0;
}

int edbDelete(EasyDB *db, EDBRow* row)
{
    if (row == NULL || db == NULL) return NULL_PTR_ERROR;

    EDBRow *pre = row->prev;
    EDBRow *next = row->next;

    pre->next = row->next;
    next->prev = row->prev;
    
    for (size_t i = 0; i < db->columnCount; i++)
    {
        switch (db->dataType[i])
        {
        case EDB_TYPE_INT:
            IndexDelINT((IndexINTNode**)&db->indexheads[i], *(edb_int*)row->data[i], row->data);
            break;
        case EDB_TYPE_TEXT:
            IndexDelTEXT((IndexTEXTNode**)&db->indexheads[i], (char*)row->data[i], row->data);
            break;
        default:
            break;
        }
    }
    
    for (size_t i = 0; i < db->columnCount; i++)
    {
        free(row->data[i]);
    }
    free(row->data);
    free(row);
    return 0;
}

int edbPrimaryKeyIndex(EasyDB *db, void* primaryKey, void*** indexResult)
{
    int retval = 0;
    void** findRes;
    switch (db->dataType[db->primaryKey])
    {
    case EDB_TYPE_INT:
        findRes = NULL;
        retval = IndexFindINT((IndexINTNode**)&db->indexheads[db->primaryKey], *(edb_int*)primaryKey, (void**)&findRes, 1);
        break;
    case EDB_TYPE_TEXT:
        findRes = NULL;
        retval = IndexFindTEXT((IndexTEXTNode**)&db->indexheads[db->primaryKey], (char*)primaryKey, (void**)&findRes, 1);
        break;
    default:
        return PRIMARY_KEY_TYPE_CANNOT_INDEX;
        break;
    }
    *indexResult = findRes;
    return 0;
}

int edbClose(EasyDB *db)
{
    char fileHead[db->dataFileOffset];
    FILE* dbfileReadHead = fopen64(db->dbfilename, "rb+");
    fread(fileHead, 1, db->dataFileOffset, dbfileReadHead);
    *(size_t*)&fileHead[4 + 4] = db->lineCount;

    FILE* dbfile = fopen64(db->dbfilename, "wb+");
    fwrite(fileHead, 1, db->dataFileOffset, dbfile);
    fsetpos64(dbfile, &db->dataFileOffset);
    EDBRow* ptr = db->head;
    EDBRow* pre = db->head;
    ptr = ptr->next;
    for (size_t i = 0; i < db->lineCount && ptr != db->tail; i++)
    {
        for (size_t j = 0; j < db->columnCount; j++)
        {
            fwrite(ptr->data[j], 1, db->dataLens[j], dbfile);
            free(ptr->data[j]);
        }
        free(ptr->data);
        ptr = ptr->next;
        free(pre);
        pre = ptr;
    }
    free(ptr);

    for (size_t i = 0; i < db->columnCount; i++) free(db->columnNames[i]);
    free(db->columnNames);
    free(db->dataType);
    free(db->dataOffset);
    free(db->dataLens);
    fclose(dbfile);
    return 0;
}