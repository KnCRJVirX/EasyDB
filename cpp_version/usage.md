# EasyDB C++版本使用文档

## 命名空间

```cpp
namespace EDB {}
```
EasyDB使用声明、定义的数据类型、函数均在其中。

## 数据类型

### EasyDB支持存储的数据类型
| 枚举 | 值 | 数据类型定义 | 说明 |
|------|----|------------|------|
|`DataType::INT`|0|`using Int = int64_t;`|64位有符号整数|
|`DataType::REAL`|1|`using Real = double;;`|双精度浮点数|
|`DataType::BLOB`|2|`using Blob = std::shared_ptr<char[]>;`|二进制数据|
|`DataType::TEXT`|3|`using Text = std::string;`|字符串|

### 使用EasyDB开发时有关的数据类型

#### Data

##### 定义

```cpp
using Data = std::variant<Int, Real, Text, Blob>;
```

## 枚举

### DataType

#### 定义

```cpp
enum class DataType : int64_t
{
    ILLEGAL_TYPE = -1,
    INT = 0,
    REAL = 1,
    BLOB = 2,
    TEXT = 9
};
```

## 静态函数

### GetDataType

#### 语法

```cpp
static inline DataType GetDataType(const Data& data);
```

#### 参数

`data`

数据元素。

#### 返回值

数据类型枚举。

#### 注解

获取数据元素的数据类型。

### DataTypeEnum2String

#### 语法

```cpp
static inline const char* DataTypeEnum2String(DataType dType);
```

#### 参数

`dType`

数据类型枚举。

#### 返回值

类型名称的空终止字符串。

#### 注解

将数据类型枚举转换为空终止字符串。

### StatusCode2String

#### 语法

```cpp
static inline const char* StatusCode2String(int statusCode);
```

#### 参数

`statusCode`

调用EasyDB API返回的状态码。

#### 返回值

状态码名称的空终止字符串。

#### 注解

将状态码转换为字符串。

## 类

### EasyDB

#### 成员类型

| 成员类型 | 定义 |
|----------|------|
|TableType|`std::map<Data, std::vector<Data>>`|
|RowViewType|`ERowView`|
|iterator|`EasyDBIterator<EasyDB>`|
|const_iterator|`EasyDBIterator<EasyDB>`|

#### 成员函数

| 名称 | 描述 |
|-----|------|
|构造函数|构造一个EasyDB。|
|析构函数|不保存并关闭EasyDB。|
|EasyDB::operator=|不保存并关闭EasyDB。|
|EasyDB::create|创建一个EasyDB数据库。|
|EasyDB::open|打开一个EasyDB数据库。|
|EasyDB::close|关闭并保存数据库。|
|EasyDB::closeNotSave|关闭数据库但不保存。|
|EasyDB::save|保存数据库。|
|EasyDB::insert|插入新数据。|
|EasyDB::erase|删除数据。|
|EasyDB::update|更新数据。|
|EasyDB::at|获取数据。|
|EasyDB::find|查找数据。|
|EasyDB::begin|获取迭代器（开始）。|
|EasyDB::end|获取迭代器（结束）。|
|EasyDB::toColumnIndex|列名转列索引。|
|EasyDB::get_column_info|获取列信息。|
|EasyDB::size|获取行数。|
|EasyDB::get_row_count|获取行数。|
|EasyDB::is_open|检查数据库是否打开。|
|EasyDB::is_init|检查数据库是否初始化。|
|EasyDB::get_version|获取数据库版本。|
|EasyDB::get_table_name|获取表名。|
|EasyDB::set_table_name|设置表名。|
|EasyDB::get_primary_key_index|获取主键索引。|

#### 构造函数

```cpp
EasyDB();                               // 1
EasyDB(const std::string& dbFileName);  // 2
EasyDB(const EasyDB& other);            // 3
EasyDB(EasyDB&& other);                 // 4
```

(1) 空构造一个**未初始化**的EasyDB。
(2) 构造一个EasyDB并打开名为`dbFileName`的文件。
(3)(4) 使用`other`构造EasyDB。

#### 析构函数

##### 注解

不保存并关闭EasyDB。

#### EasyDB::operator=

```cpp
EasyDB& operator=(const EasyDB& other); // 1
EasyDB& operator=(EasyDB&& other);      // 2
```

(1) 拷贝赋值。

(2) 移动赋值。

##### 返回值

`*this`

#### EasyDB::create

```cpp
static int create(const std::string& filename, const std::string& tableName, std::vector<EColumn> tableHead, const std::string& primaryKeyColumnName);
```

##### 参数

`filename`

要创建的数据库保存的文件名。

`tableName`

要创建的数据库的表名。

`tableHead`

要创建的数据库的列信息，每一个`EColumn`元素表示一列的信息。

`primaryKeyColumnName`

主键名。

##### 返回值

如果成功，返回**SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **PRIMARY_KEY_NOT_IN_LIST** | 主键名不在传入的列名数组中 |
| **FILE_OPEN_ERROR** | 创建数据库文件时打开文件失败 |

##### 注解

创建一个EasyDB数据库并保存到名为`filename`的文件。

#### EasyDB::open

```cpp
int open(const std::string& dbFilePath);
```

##### 参数

`dbFilePath`
要打开的EasyDB数据库的文件路径。

##### 返回值

如果数据库打开成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NULL_PTR_ERROR** | 传入的指针类型参数含空指针 |
| **FILE_OPEN_ERROR** | 打开数据库文件失败 |
| **MAGIC_NUMBER_ERROR** | 魔数错误，打开的文件不是支持的EasyDB文件 |

##### 注解

打开名为`dbFilePath`的EasyDB数据库文件。
如果有已打开的数据库，会调用`EasyDB::closeNotSave()` 。

#### EasyDB::close

```cpp
int close();
```

##### 返回值

如果关闭并保存成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NOT_INITED** | 未初始化（未打开文件） |

##### 注解

关闭数据库并保存。

#### EasyDB::closeNotSave

```cpp
int closeNotSave();
```

##### 返回值

如果关闭成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NOT_INITED** | 未初始化（未打开文件） |

##### 注解

关闭数据库但不保存。

#### EasyDB::save

```cpp
int save();
```

##### 返回值

如果保存成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NOT_INITED** | 未初始化（未打开文件） |

##### 注解

将内存中的数据库内容保存到文件。

#### EasyDB::insert

```cpp
int insert(const std::unordered_map<std::string, Data>& row);
int insert(const std::vector<Data>& row);
```

##### 参数

`row`
要插入的新行数据，可以为列名到数据的映射，或数据的有序数组。

##### 返回值

如果插入成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NOT_INITED** | 未初始化（未打开文件） |
| **INVALID_ROW** | 插入的新行不合法（无主键，列数不匹配等） |
| **PRIMARY_KEY_NOT_UNIQUE** | 主键重复 |
| **DATA_TYPE_NOT_MATCH** | 数据类型不匹配 |
| **TOO_LONG_DATA** | 数据长度超过预设 |

##### 注解

插入一行新数据到数据库。

#### EasyDB::erase

```cpp
int erase(const Data& primaryKey);
```

##### 参数

`primaryKey`
要删除的行的主键数据。

##### 返回值

如果删除成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NOT_INITED** | 未初始化（未打开文件） |
| **KEY_NOT_FOUND** | 未找到匹配的行 |

##### 注解

根据主键删除一行数据。

#### EasyDB::update

```cpp
int update(const Data& primaryKey, int columnIndex, const Data& newData);
int update(const Data& primaryKey, const std::string& columnName, const Data& newData);
```

##### 参数

`primaryKey`
要修改的行的主键数据。

`columnIndex` / `columnName`
要修改的列索引或列名。

`newData`
新的数据。

##### 返回值

如果修改成功，返回 **SUCCESS**。
如果失败，返回下表的值。

| 返回值 | 说明 |
|--------|-----|
| **NOT_INITED** | 未初始化（未打开文件） |
| **COLUMN_INDEX_OUT_OF_RANGE** | 传入的列索引超出原来的列数 |
| **COLUMN_NOT_FOUND** | 找不到列名对应的列 |
| **KEY_NOT_FOUND** | 未找到匹配的行 |
| **DATA_TYPE_NOT_MATCH** | 数据类型不匹配 |
| **TOO_LONG_DATA** | 数据长度超过预设 |

##### 注解

根据主键修改指定列的数据。

#### EasyDB::at

```cpp
RowViewType at(const Data& primaryKey);
const Data& at(const Data& primaryKey, int columnIndex);
const Data& at(const Data& primaryKey, const std::string& columnName);
```

##### 参数

`primaryKey`
要查找的主键数据。

`columnIndex` / `columnName`
要获取的列索引或列名。

##### 返回值

返回对应行或单元格的数据。

##### 注解

根据主键和列索引/列名获取行或单元格数据。

#### EasyDB::find

```cpp
std::vector<RowViewType> find(int columnIndex, const Data& inKey);
std::vector<RowViewType> find(const std::string& columnName, const Data& inKey);
```

##### 参数

`columnIndex` / `columnName`
要查找的列索引或列名。

`inKey`
要查找的键值。

##### 返回值

返回所有匹配的行的视图。

##### 注解

查找指定列等于指定值的所有行。

#### EasyDB::begin

```cpp
iterator begin();
```

##### 返回值

返回数据库的起始迭代器。

##### 注解

STL风格遍历接口，指向第一行。

#### EasyDB::end

```cpp
iterator end();
```

##### 返回值

返回数据库的尾后迭代器。

##### 注解

STL风格遍历接口，指向末尾后一行。

#### EasyDB::get_table_name

```cpp
std::string get_table_name() const;
```

##### 返回值

返回表名。

##### 注解

获取当前表名。

#### EasyDB::set_table_name

```cpp
int set_table_name(const std::string& newTableName);
```

##### 参数

`newTableName`
新的表名。

##### 返回值

成功返回**SUCCESS**。

##### 注解

设置表名。

#### EasyDB::get_primary_key_index

```cpp
int get_primary_key_index() const;
```

##### 返回值

返回主键在表头中的索引。

##### 注解

获取主键列索引。

---

### ERowView

对EasyDB中的一行数据的只读视图。若该行数据被修改/删除，视图将失效。

#### 成员类型

| 成员类型 | 定义 |
|----------|------|
|DataType|`Data`|
|VectorType|`std::vector<Data>`|
|MapType|`std::unordered_map<std::string, Data>`|
|value_type|`Data`|
|size_type|`size_t`|
|reference|`const Data&`|
|const_reference|`const Data&`|
|pointer|`const Data*`|
|const_pointer|`const Data*`|
|iterator|`std::vector<Data>::const_iterator`|
|const_iterator|`std::vector<Data>::const_iterator`|

#### 成员函数

| 名称 | 描述 |
|-----|------|
|构造函数|构造一个ERowView。|
|begin|获取起始迭代器。|
|end|获取末尾迭代器。|
|at|按索引或列名获取数据。|
|operator[]|按索引或列名获取数据。|
|size|获取列数。|
|empty|判断是否为空。|
|front|获取首元素。|
|back|获取尾元素。|
|to_vector|转为vector。|
|to_map|转为map。|

#### 构造函数

```cpp
ERowView(const std::vector<Data>& row, const std::unordered_map<std::string, size_t>& colNameMap);
```

##### 参数

`row`
一行的数据。

`colNameMap`
列名到索引的映射。

##### 注解

构造一个只读的行视图对象。

#### begin

```cpp
iterator begin() const;
```

##### 返回值

返回指向行首的常迭代器。

##### 注解

用于STL风格遍历。

#### end

```cpp
iterator end() const;
```

##### 返回值

返回指向行尾的常迭代器。

##### 注解

用于STL风格遍历。

#### at

```cpp
const_reference at(size_t columnIndex) const;
const_reference at(const std::string& columnName) const;
```

##### 参数

`columnIndex`
列索引。

`columnName`
列名。

##### 返回值

返回指定单元格的数据常引用。

##### 注解

越界或列名不存在会抛出异常。

#### operator[]

```cpp
const_reference operator[](size_t columnIndex) const;
const_reference operator[](const std::string& columnName) const;
```

##### 参数

`columnIndex`
列索引。

`columnName`
列名。

##### 返回值

返回指定单元格的数据常引用。

##### 注解

列名不存在会抛出异常。

#### size

```cpp
size_type size() const;
```

##### 返回值

返回该行的列数。

##### 注解

等价于vector的size。

#### empty

```cpp
bool empty() const;
```

##### 返回值

若该行无数据返回true，否则返回false。

##### 注解

判断行是否为空。

#### front

```cpp
const_reference front() const;
```

##### 返回值

返回首单元格的数据常引用。

##### 注解

等价于vector的front。

#### back

```cpp
const_reference back() const;
```

##### 返回值

返回末单元格的数据常引用。

##### 注解

等价于vector的back。

#### to_vector

```cpp
VectorType to_vector() const;
```

##### 返回值

返回该行的所有数据组成的vector。

##### 注解

可用于整体拷贝行数据。

#### to_map

```cpp
MapType to_map() const;
```

##### 返回值

返回该行的列名到数据的映射。

##### 注解

便于按列名访问所有数据。

---

### EColumn

#### 成员类型

| 成员类型 | 定义 |
|----------|------|
|dType|`EDB::DataType`|
|columnName|`std::string`|
|columnSize|`size_t`|

#### 成员函数

| 名称 | 描述 |
|-----|------|
|构造函数|构造一个EColumn。|

#### 构造函数

```cpp
EColumn();
EColumn(DataType dType, const std::string& colName, int colSize);
```

##### 参数

`dType`
数据类型。

`colName`
列名。

`colSize`
列长度。

##### 注解

用于表头定义和元数据获取。默认构造类型为INT，长度为0。

---

### EasyDBIterator

#### 成员类型

| 成员类型 | 定义 |
|----------|------|
|RowViewType|`EasyDB::RowViewType`|
|InternalIterType|`EasyDB::TableType::iterator`|

#### 成员函数

| 名称 | 描述 |
|-----|------|
|构造函数|构造一个迭代器。|
|operator=|赋值。|
|operator++|前置/后置自增。|
|operator--|前置/后置自减。|
|operator==|判断相等。|
|operator!=|判断不等。|
|operator*|解引用，获取行视图。|

#### 构造函数

```cpp
EasyDBIterator();
EasyDBIterator(const InternalIterType& i, const EasyDB* db);
EasyDBIterator(const EasyDBIterator& other);
```

##### 参数

`i`
底层迭代器。

`db`
数据库指针。

`other`
拷贝构造。

##### 注解

用于遍历数据库所有行。

#### operator=

```cpp
EasyDBIterator& operator=(const EasyDBIterator& other);
```

##### 参数

- `other`：另一个迭代器。

##### 返回值

返回自身引用。

##### 注解

赋值操作。

#### operator++

```cpp
EasyDBIterator& operator++();
EasyDBIterator operator++(int);
```

##### 返回值

自增后的迭代器。

##### 注解

前置/后置自增，指向下一行。

#### operator--

```cpp
EasyDBIterator& operator--();
EasyDBIterator operator--(int);
```

##### 返回值

自减后的迭代器。

##### 注解

前置/后置自减，指向上一行。

#### operator==

```cpp
bool operator==(const EasyDBIterator& other);
```

##### 参数

- `other`：另一个迭代器。

##### 返回值

若相等返回true，否则返回false。

##### 注解

判断两个迭代器是否指向同一行。

#### operator!=

```cpp
bool operator!=(const EasyDBIterator& other);
```

##### 参数

- `other`：另一个迭代器。

##### 返回值

若不相等返回true，否则返回false。

##### 注解

判断两个迭代器是否指向不同的行。

#### operator*

```cpp
RowViewType operator*();
```

##### 返回值

返回当前行的只读视图。

##### 注解

解引用获取当前行数据。