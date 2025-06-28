# EasyDB C版本使用文档

## API

### edbCreate

#### 语法

```c
int edbCreate(const char* filename, const char* tableName, size_t columnCount, char* primaryKeyColumnName, size_t dataTypes[], size_t dataSizes[], char* columnNames[]);
```

#### 参数

`filename`
要创建的数据库保存的文件名。

`tableName`
要创建的数据库的表名。

`columnCount`
要创建的数据库的列数。

`primaryKeyColumnName`
要创建的数据库的主键名。

`dataTypes`
存储每一列的数据类型的数组。数组中的每个元素可使用以下的一个值。

| 宏定义 | 值 | 含义 |
|--------|----|------|
| EDB_TYPE_INT | 0 | 64位有符号整数`int64_t` |
| EDB_TYPE_REAL | 1 | 双进度浮点数`double` |
| EDB_TYPE_BLOB | 2 | 二进制数据 |
| EDB_TYPE_TEXT | 9 | 空终止字符串`char*` |


`dataSizes`
存储每一列的数据长度的数组。只有`EDB_TYPE_BLOB`和`EDB_TYPE_TEXT`类型的列才会使用对应位置的数据长度，其他类型的列设置数据长度无效。

`columnNames`
存储每一列的列名的数组。列名为不超过4095字节的空终止字符串。

#### 返回值

如果数据库创建成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **PRIMARY_KEY_NOT_IN_LIST** | 主键名不在传入的列名数组中 |
| **FILE_OPEN_ERROR** | 创建数据库文件时打开文件失败 |

#### 注解
如果创建成功，则创建一个不含记录的数据库并保存到`filename`指定的文件中。

### edbOpen

#### 语法

```c
int edbOpen(const char* filename, EasyDB* db);
```

#### 参数

`filename`
要打开的数据库保存的文件名。

`db`
用于接收数据库打开的`EasyDB`结构的指针。

#### 返回值

如果数据库打开成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **FILE_OPEN_ERROR** | 打开数据库文件失败 |
| **MAGIC_NUMBER_ERROR** | 魔数错误，打开的文件不是支持的EasyDB文件 |

#### 注解

如果数据库文件打开成功，则将文件解析后填充`db`指向的`EasyDB`结构。

### edbCloseNotSave

#### 语法

```c
int edbCloseNotSave(EasyDB *db);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

#### 返回值

如果关闭成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |

#### 注解

关闭`db`指向的`EasyDB`结构中存储的数据库且不保存。

### edbClose

#### 语法

```c
int edbClose(EasyDB *db);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

#### 返回值

如果关闭并保存成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |

#### 注解

关闭`db`指向的`EasyDB`结构中存储的数据库并保存。

### edbSave

#### 语法

```c
int edbSave(EasyDB *db);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

#### 返回值

如果保存成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |

#### 注解

将内存中的数据库内容保存到文件。

### edbInsert

#### 语法

```c
int edbInsert(EasyDB *db, void* row[]);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`row`
要插入的新行数据，每一项为对应列的数据指针。

#### 返回值

如果插入成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **PRIMARY_KEY_NOT_UNIQUE** | 主键重复 |

#### 注解

插入一行新数据到数据库。

### edbDelete

#### 语法

```c
int edbDelete(EasyDB *db, void* primaryKey);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`primaryKey`
要删除的行的主键数据指针。

#### 返回值

如果删除成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **KEY_NOT_FOUND** | 未找到匹配的行 |

#### 注解

根据主键删除一行数据。

### edbWhere

#### 语法

```c
int edbWhere(EasyDB *db, char* columnName, void* inKey, void*** findResults, size_t maxResultNumber, size_t *resultsCount);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`columnName`
要查找的列名。

`inKey`
要查找的键值（指针）。

`findResults`
用于接收查找到的结果数组。一般以`void** res[]`形式声明。

`maxResultNumber`
最多查找的结果数量。

`resultsCount`
用于接收实际查找到的结果数量。

#### 返回值

如果查找成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **COLUMN_NOT_FOUND** | 找不到列名对应的列 |

#### 注解

查找指定列等于指定值的所有行。

### edbUpdate

#### 语法

```c
int edbUpdate(EasyDB *db, void* primaryKey, char* updateColumnName, void* newData);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`primaryKey`
要修改的行的主键数据指针。

`updateColumnName`
要修改的列名。

`newData`
新的数据指针。

#### 返回值

如果修改成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **COLUMN_NOT_FOUND** | 找不到列名对应的列 |
| **KEY_NOT_FOUND** | 未找到匹配的行 |

#### 注解

根据主键修改指定列的数据。

### edbGet

#### 语法

```c
void* edbGet(EasyDB *db, void* primaryKey, char* columnName);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`primaryKey`
要查找的主键数据指针。

`columnName`
要获取的列名。

#### 返回值

返回对应单元格的数据指针，未找到时返回NULL。

#### 注解

根据主键和列名获取单元格数据。

### edbCount

#### 语法

```c
size_t edbCount(EasyDB *db, char* columnName, void* inKey);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`columnName`
要计数的列名。

`inKey`
要计数的键值（指针）。

#### 返回值

返回匹配的项的个数。

#### 注解

统计指定列等于指定值的行数。

### edbSearch

#### 语法

```c
int edbSearch(EasyDB *db, char* columnName, char *keyWord, void*** findResults, size_t maxResultNumber, size_t *resultsCount);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`columnName`
要搜索的列名（必须为文本类型）。

`keyWord`
要搜索的关键词。

`findResults`
用于接收查找到的结果数组。

`maxResultNumber`
最多查找的结果数量。

`resultsCount`
用于接收实际查找到的结果数量。

#### 返回值

如果搜索成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **COLUMN_NOT_FOUND** | 找不到列名对应的列 |
| **NOT_TEXT_COLUMN** | 该列非文本类型 |

#### 注解

对文本列进行模糊搜索。

### edbDeleteByArray

#### 语法

```c
int edbDeleteByArray(EasyDB *db, void** deleteRows[], size_t arraySize);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`deleteRows`
要删除的行数据指针数组。

`arraySize`
要删除的行数。

#### 返回值

如果全部删除成功，返回 **SUCCESS**。
如果部分失败，返回最后一次失败的错误码。

#### 注解

批量删除多行数据。

### edbDeleteByKeyword

#### 语法

```c
int edbDeleteByKeyword(EasyDB *db, char* columnName, char *keyword);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`columnName`
要搜索的列名（必须为文本类型）。

`keyword`
要删除的关键词。

#### 返回值

如果全部删除成功，返回 **SUCCESS**。
如果部分失败，返回最后一次失败的错误码。

#### 注解

删除所有包含指定关键词的行。

### edbDeleteByKey

#### 语法

```c
int edbDeleteByKey(EasyDB *db, char* columnName, void* inKey);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`columnName`
要查找的列名。

`inKey`
要删除的键值（指针）。

#### 返回值

如果全部删除成功，返回 **SUCCESS**。
如果部分失败，返回最后一次失败的错误码。

#### 注解

删除所有指定列等于指定值的行。

### edbSort

#### 语法

```c
int edbSort(EasyDB *db, char* columnName, int (*compareFunc)(const void*, const void*));
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`columnName`
要排序的列名。

`compareFunc`
比较函数指针。

#### 返回值

如果排序成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **COLUMN_NOT_FOUND** | 找不到列名对应的列 |

#### 注解

对指定列进行排序。

### edbImportCSV

#### 语法

```c
int edbImportCSV(EasyDB* db, char* csvFileName);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`csvFileName`
要导入的CSV文件名。

#### 返回值

如果导入成功，返回 **SUCCESS**。

#### 注解

从CSV文件导入数据。

### edbExportCSV

#### 语法

```c
int edbExportCSV(EasyDB* db, char* csvFileName, bool withBOM);
```

#### 参数

`db`
已打开文件的`EasyDB`结构的指针。

`csvFileName`
要导出的CSV文件名。

`withBOM`
是否在文件头添加BOM。

#### 返回值

如果导出成功，返回 **SUCCESS**。

#### 注解

将数据导出为CSV文件。

### Easy User Management

#### easyLogin

##### 语法

```c
int easyLogin(EasyDB *db, char* userID, char* password, void*** retUserData);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。
- `userID`：用户ID（主键列）。
- `password`：明文密码。
- `retUserData`：用于接收用户数据的数组。

##### 返回值

| 返回值 | 说明 |
|--------|-----|
| **SUCCESS** | 登录成功 |
| **PASSWORD_WRONG** | 密码错误 |

##### 注解

用于用户登录，密码自动进行SHA256摘要比对。

#### easyAddUser

##### 语法

```c
int easyAddUser(EasyDB *db, void* newRow[]);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。
- `newRow`：新用户数据。

##### 返回值

| 返回值 | 说明 |
|--------|-----|
| **SUCCESS** | 添加成功 |
| **PRIMARY_KEY_NOT_UNIQUE** | 主键重复 |

##### 注解

添加新用户，密码字段会自动加密。

#### easyDeleteUser

##### 语法

```c
int easyDeleteUser(EasyDB *db, char* userID);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。
- `userID`：要删除的用户ID。

##### 返回值

| 返回值 | 说明 |
|--------|-----|
| **SUCCESS** | 删除成功 |
| **KEY_NOT_FOUND** | 未找到用户 |

##### 注解

根据用户ID删除用户。

#### easyResetPassword

##### 语法

```c
int easyResetPassword(EasyDB *db, char* userID, char* newPassword);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。
- `userID`：要重置密码的用户ID。
- `newPassword`：新密码。

##### 返回值

| 返回值 | 说明 |
|--------|-----|
| **SUCCESS** | 重置成功 |
| **KEY_NOT_FOUND** | 未找到用户 |

##### 注解

重置指定用户的密码，密码会自动加密。

### 其他辅助API

#### toColumnIndex

##### 语法

```c
long long toColumnIndex(EasyDB *db, char *columnName);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。
- `columnName`：列名。

##### 返回值

返回列索引，未找到返回 **COLUMN_NOT_FOUND**。

##### 注解

将列名转换为列索引。

#### edbIterBegin

##### 语法

```c
void* edbIterBegin(EasyDB *db);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。

##### 返回值

返回数据库首行的迭代器指针，若无数据返回NULL。

##### 注解

用于遍历数据库所有行，配合`edbIterNext`使用。

#### edbIterNext

##### 语法

```c
void** edbIterNext(EasyDB *db, void** pEdbIterator);
```

##### 参数

- `db`：已打开文件的`EasyDB`结构的指针。
- `pEdbIterator`：当前迭代器指针的地址。

##### 返回值

返回下一行的数据指针，遍历结束返回NULL。

##### 注解

用于遍历数据库所有行。

#### uuid

##### 语法

```c
char* uuid(char *UUID);
```

##### 参数

- `UUID`：用于接收生成的UUID字符串的缓冲区。

##### 返回值

返回UUID字符串指针。

##### 注解

生成一个UUID v4字符串。

#### sha256

##### 语法

```c
char* sha256(const char* input, char* SHA256);
```

##### 参数

- `input`：要加密的字符串。
- `SHA256`：用于接收SHA256摘要的缓冲区。

##### 返回值

返回SHA256摘要字符串指针。

##### 注解

对输入字符串进行SHA256摘要。

## 宏

### Int(x)

#### 定义

```c
#define Int(x) (*(edb_int*)(x))
```

#### 注解

将`void*`指针`x`转换为`edb_int`类型的整数。

#### 示例

```c
int64_t age = Int(edbGet(&db, "Alice", "Age"));
```

### Real(x)

#### 定义

```c
#define Real(x) (*(double*)(x))
```

#### 注解

将`void*`指针`x`转换为`double`类型的双精度浮点数。

#### 示例

```c
double balance = Real(edbGet(&db, "Alice", "Balance"));
```

### Text(x)

#### 定义

```c
#define Real(x) (*(double*)(x))
```

#### 注解

将`void*`指针`x`转换为`char*`类型的指针。

#### 示例

```c
char name[100];
strcpy(name, Text(edbGet(&db, "Alice", "Name")));
```

## 返回值/错误代码 对照表

| 宏定义 | 值 | 说明 |
|-------|----|------|
| **SUCCESS** | 0 | 操作成功完成 |
| **FILE_OPEN_ERROR** | -1 | 文件打开错误 |
| **MAGIC_NUMBER_ERROR** | -2 | 魔数错误（非EasyDB数据库文件） |
| **NULL_PTR_ERROR** | -3 | 空指针错误 |
| **PRIMARY_KEY_NOT_UNIQUE** | -4 | 主键重复 |
| **PRIMARY_KEY_TYPE_CANNOT_INDEX** | -5 | 已弃用 |
| **TYPE_CANNOT_INDEX** | -6 | 已弃用 |
| **KEY_NOT_FOUND** | -7 | 未找到匹配的行 |
| **COLUMN_INDEX_OUT_OF_RANGE** | -8 | 传入的列索引超出原来的列数 |
| **COLUMN_NOT_FOUND** | -9 | 找不到列名对应的列 |
| **EMPTY_TABLE** | -10 | 该表为空 |
| **NOT_TEXT_COLUMN** | -11 | 该列非文本类型（仅支持对文本类型的列进行搜索） |
| **PASSWORD_WRONG** | -12 | 密码错误（Easy User Management） |
| **PRIMARY_KEY_NOT_IN_LIST** | -13 | 主键名不在传入的列名列表中 |
