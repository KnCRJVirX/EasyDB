#ifndef EASYDB
#define EASYDB

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <variant>
#include <utility>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <any>

#include <cstring>
#include <stdint.h>

#define EDB_MAGIC_NUMBER 0xDBCEEA00     //魔数
#define EDB_VERSION 1

#define EDB_INT_SIZE 8      //整数类型长度
#define EDB_CHAR_SIZE 1     //字符类型长度
#define EDB_REAL_SIZE 8     //浮点数类型长度

#define EDB_TYPE_INT 0      //整数类型
#define EDB_TYPE_REAL 1     //实数（浮点数）类型
#define EDB_TYPE_TEXT 9     //文本类型

#define SUCCESS 0                           //操作成功完成
#define FILE_OPEN_ERROR -1                  //文件打开错误
#define MAGIC_NUMBER_ERROR -2               //魔数错误（非EasyDB数据库文件）
#define NULL_PTR_ERROR -3                   //空指针错误
#define PRIMARY_KEY_NOT_UNIQUE -4           //主键重复
#define PRIMARY_KEY_TYPE_CANNOT_INDEX -5    //已弃用
#define TYPE_CANNOT_INDEX -6                //已弃用
#define KEY_NOT_FOUND -7                    //未找到匹配的行
#define COLUMN_INDEX_OUT_OF_RANGE -8        //传入的列索引超出原来的列数
#define COLUMN_NOT_FOUND -9                 //找不到列名对应的列
#define EMPTY_TABLE -10                     //该表为空
#define NOT_TEXT_COLUMN -11                 //该列非文本类型（仅支持对文本类型的列进行搜索）
#define PASSWORD_WRONG -12                  //密码错误（Easy User Management）
#define PRIMARY_KEY_NOT_IN_LIST -13         //主键名不在传入的列名列表中
#define NOT_INITED -14                      //未初始化（未打开文件）
#define INVALID_ROW -15                     //插入的新行不合法（无主键，列数不匹配等）
#define DATA_TYPE_NOT_MATCH -16             //数据类型不匹配
#define UNKNOWN_ERROR -17                   //未知错误
#define TOO_LONG_DATA -18                   //数据长度超过预设
#define COLUMN_NAME_NOT_UNIQUE -19          //列名不唯一

#define IF_INIT(pEasyDB) if((pEasyDB)->is_init())
#define IF_NOT_INIT(pEasyDB) if(!((pEasyDB)->is_init()))
#define IF_NOT_INIT_RETURN_ERR_CODE do { IF_NOT_INIT(this) return NOT_INITED; } while(0);
#define IF_NOT_INIT_THROW_ERROR(ERR_STRING) do { IF_NOT_INIT(this) throw EDB::not_inited((ERR_STRING)); } while(0)

#define IF_DATA_TYPE_NOT_MATCH(COLINDEX, DATAREF) if ((this->dataTypes[(COLINDEX)] == EDB::DataType::INT && (DATAREF).index() != 0) || \
                                                        (this->dataTypes[(COLINDEX)] == EDB::DataType::REAL&& (DATAREF).index() != 1) || \
                                                        (this->dataTypes[(COLINDEX)] == EDB::DataType::TEXT && (DATAREF).index() != 2) || \
                                                        (this->dataTypes[(COLINDEX)] == EDB::DataType::BLOB && (DATAREF).index() != 3))
#define IF_DATA_IS_TEXT_AND_TOO_LONG(COLINDEX, DATAREF) if (this->dataTypes[(COLINDEX)] == EDB::DataType::TEXT && \
                                                        std::get<EDB::Text>((DATAREF)).length() >= this->dataSizes[(COLINDEX)])

namespace EDB {
    class EasyDB;
    class ERowView;

    // 存储数据类型
    using Int = int64_t;
    using Real = double;
    using Text = std::string;
    using Blob = std::shared_ptr<char[]>;
    using Data = std::variant<Int, Real, Text, Blob>;

    // 异常类
    class column_not_found : public std::out_of_range{
        using std::out_of_range::out_of_range;
    };
    class key_not_found : public std::out_of_range{
        using std::out_of_range::out_of_range;
    };
    class not_inited : public std::logic_error{
        using std::logic_error::logic_error;
    };
    class type_not_match : public std::invalid_argument {
        using std::invalid_argument::invalid_argument;
    };

    // 数据类型枚举
    enum class DataType : int64_t {
        ILLEGAL_TYPE = -1,
        INT = 0,
        REAL = 1,
        BLOB = 2,
        TEXT = 9
    };

    // 兼容EasyDB的数据对象基类
    class DataObject {
        public:
            virtual int edb_dump(EasyDB& edb) noexcept = 0;
            virtual int edb_load(ERowView rowView) noexcept = 0;
    };

    // 列描述
    class EColumn {
        public:
            DataType dType;
            std::string columnName;
            size_t columnSize;
            EColumn() : dType(DataType::INT), columnSize(0) {};
            EColumn(DataType dTy, const std::string& colName, int colSize) : dType(dTy), columnName(colName), columnSize(colSize) {};
    };

    struct _VariantHash {
        std::size_t operator()(const Data& v) const {
            return std::visit([](const auto& val) { return std::hash<std::decay_t<decltype(val)>>{}(val); }, v) ^
                   std::hash<size_t>{}(v.index()); // 加上类型索引以区分不同类型
        }
    };

    static inline DataType GetDataType(const Data& data) {
        switch (data.index()) {
        case 0:     return DataType::INT;
        case 1:     return DataType::REAL;
        case 2:     return DataType::TEXT;
        case 3:     return DataType::BLOB;
        default:    break;
        }
        return DataType::ILLEGAL_TYPE;
    }

    static inline const char* DataTypeEnum2String(DataType dType) {
        switch (dType) {
        case DataType::ILLEGAL_TYPE:    return "ILLEGAL_TYPE";
        case DataType::INT:             return "INT";
        case DataType::REAL:            return "REAL";
        case DataType::TEXT:            return "TEXT";
        case DataType::BLOB:            return "BLOB";
        default:                        break;
        }
        return "UNKNOWN TYPE";
    }

    static inline const char* StatusCode2String(int statusCode) {
        switch (statusCode) {
        case SUCCESS:                           return "SUCCESS";
        case FILE_OPEN_ERROR:                   return "FILE_OPEN_ERROR";
        case MAGIC_NUMBER_ERROR:                return "MAGIC_NUMBER_ERROR";
        case NULL_PTR_ERROR:                    return "NULL_PTR_ERROR";
        case PRIMARY_KEY_NOT_UNIQUE:            return "PRIMARY_KEY_NOT_UNIQUE";
        case PRIMARY_KEY_TYPE_CANNOT_INDEX:     return "PRIMARY_KEY_TYPE_CANNOT_INDEX";
        case TYPE_CANNOT_INDEX:                 return "TYPE_CANNOT_INDEX";
        case KEY_NOT_FOUND:                     return "KEY_NOT_FOUND";
        case COLUMN_INDEX_OUT_OF_RANGE:         return "COLUMN_INDEX_OUT_OF_RANGE";
        case COLUMN_NOT_FOUND:                  return "COLUMN_NOT_FOUND";
        case EMPTY_TABLE:                       return "EMPTY_TABLE";
        case NOT_TEXT_COLUMN:                   return "NOT_TEXT_COLUMN";
        case PASSWORD_WRONG:                    return "PASSWORD_WRONG";
        case PRIMARY_KEY_NOT_IN_LIST:           return "PRIMARY_KEY_NOT_IN_LIST";
        case NOT_INITED:                        return "NOT_INITED";
        case INVALID_ROW:                       return "INVALID_ROW";
        case DATA_TYPE_NOT_MATCH:               return "DATA_TYPE_NOT_MATCH";
        case UNKNOWN_ERROR:                     return "UNKNOWN_ERROR";
        case TOO_LONG_DATA:                     return "TOO_LONG_DATA";
        case COLUMN_NAME_NOT_UNIQUE:            return "COLUMN_NAME_NOT_UNIQUE";
        default:                                return "UNKNOWN_STATUS_CODE";
        }
        return nullptr;
    }

    // 行视图
    class ERowView {
        friend class EasyDB;
    public:
        // 元素类型及可转换类型
        using DataType = Data;
        using VectorType = std::vector<DataType>;
        using MapType = std::unordered_map<std::string, DataType>;
        // STL兼容
        using value_type = DataType;
        using size_type = size_t;
        using reference = const value_type&;
        using const_reference = const value_type&;
        using pointer = const value_type*;
        using const_pointer = const value_type*;
        using iterator = std::vector<DataType>::const_iterator;
        using const_iterator = std::vector<DataType>::const_iterator;
    private:
        const std::vector<Data>* rowData;
        const std::unordered_map<std::string, size_t>* columnNameMap;
    public:
        ERowView() = delete;
        ERowView(const std::vector<Data>& _InRow, const std::unordered_map<std::string, size_t>& _InColNameMap) {
            rowData = &_InRow;
            columnNameMap = &_InColNameMap;
        }
        iterator begin() const
        { return rowData->cbegin(); }
        iterator end() const
        { return rowData->cend(); }
        const_reference at(size_t columnIndex) const
        { return rowData->at(columnIndex); }
        const_reference at(const std::string& columnName) const
        {
            if (columnNameMap->find(columnName) == columnNameMap->end())
            { throw column_not_found{"ERowView::operator[] : Column " + columnName + " not found."}; }
            return this->operator[](columnNameMap->at(columnName));
        }
        const_reference operator[](size_t columnIndex) const
        { return (*rowData)[columnIndex]; }
        const_reference operator[](const std::string& columnName) const
        {
            if (columnNameMap->find(columnName) == columnNameMap->end())
            { throw column_not_found{"ERowView::operator[] : Column " + columnName + " not found."}; }
            return this->operator[](columnNameMap->at(columnName));
        }
        size_type size() const
        { return rowData->size(); }
        bool empty() const
        { return rowData->empty(); }
        const_reference front() const
        { return rowData->front(); }
        const_reference back() const
        { return rowData->back(); }
        VectorType to_vector() const
        { return *(this->rowData); }
        MapType to_map() const
        {
            MapType res;
            for (auto& [colName, colIndex] : *(this->columnNameMap))
            {
                res[colName] = (*(this->rowData))[colIndex];
            }
            return res;
        }
        operator std::vector<DataType>() const
        { return this->to_vector(); }
        operator std::unordered_map<std::string, DataType>() const
        { return this->to_map(); }
        bool hascolumn(const std::string& columnName) 
        {
            return columnNameMap->find(columnName) != columnNameMap->end();
        }
        bool hascolumns(const std::vector<std::string>& columnNames)
        {
            for (auto& colName : columnNames) {
                if (!hascolumn(colName)) {
                    return false;
                }
            }
            return true;
        }
    };

    // 迭代器
    template <class _EasyDB>
    class EasyDBIterator {
    public:
        using RowViewType = typename _EasyDB::RowViewType;
        using InternalIterType = typename _EasyDB::TableType::iterator;
    private:
        const _EasyDB* _db;
        InternalIterType _it;
    public:
        EasyDBIterator() : _it(){};
        EasyDBIterator(const InternalIterType& i, const _EasyDB* _InDB) : _db(_InDB), _it(i) {};
        EasyDBIterator(const EasyDBIterator& other)
        { 
            this->_db = other._db;
            this->_it = other._it;
        }
        EasyDBIterator& operator=(const EasyDBIterator& other)
        {
            this->_db = other._db;
            this->_it = other._it;
        }
        EasyDBIterator& operator++()
        {
            ++_it;
            return *this;
        }
        EasyDBIterator operator++(int)
        {
            EasyDBIterator _tmp = *this;
            ++(*this);
            return _tmp;
        }
        EasyDBIterator& operator--()
        {
            --_it;
            return *this;
        }
        EasyDBIterator operator--(int)
        {
            EasyDBIterator _tmp = *this;
            --(*this);
            return _tmp;
        }
        bool operator==(const EasyDBIterator& other)
        { return (this->_it == other._it && this->_db == other._db); }
        bool operator!=(const EasyDBIterator& other)
        { return (this->_it != other._it || this->_db != other._db); }
        RowViewType operator*()
        { return RowViewType{_it->second, _db->columnIndexMap}; }
    };

    class EasyDB {
    public:
        friend class EasyDBIterator<EasyDB>;
        using TableType = std::map<Data, std::vector<Data>>;
        using RowViewType = ERowView;
        using iterator = EasyDBIterator<EasyDB>;
        using const_iterator = EasyDBIterator<EasyDB>;
    private:
        bool init;
        std::string dbfilename;
        std::string tableName;
        int version;
        size_t rowCount;                            // 行数
        size_t rowSize;                             // 一行的长度
        size_t columnCount;                         // 每行有几个数据
        size_t primaryKeyIndex;                     // 主键在一行中的索引
        std::vector<size_t> dataOffset;             // 每个数据相比行首的偏移量
        std::vector<DataType> dataTypes;            // 每个数据的类型
        std::vector<size_t> dataSizes;              // 每个数据的长度
        std::vector<std::string> columnNames;       // 列名
        long long dataFileOffset;                   // 数据在文件中开始的位置
    protected:
        std::unordered_map<std::string, size_t> columnIndexMap;
        TableType table;
        int insert_not_check(const std::vector<Data>& row)
        {
            this->table[row[this->primaryKeyIndex]] = row;
            this->rowCount += 1;
            return SUCCESS;
        }
        int erase_not_check(const Data& primaryKey)
        {
            auto retval = this->table.erase(primaryKey);
            if (retval) {
                return (retval != 0) ? SUCCESS : UNKNOWN_ERROR;
            }
            return UNKNOWN_ERROR;
        }
        int update_not_check(const Data& primaryKey, int columnIndex, const Data& newData)
        {
            if (columnIndex == this->primaryKeyIndex) {
                auto row = this->table[primaryKey];
                this->table.erase(primaryKey);
                row[columnIndex] = newData;
                this->table[newData] = row;
            } else {
                this->table[primaryKey][columnIndex] = newData;
            }
            return SUCCESS;
        }
    public:
        EasyDB() : init(false){}
        EasyDB(const std::string& dbFileName)
        {
            this->init = true;
            this->open(dbFileName);
        }
        EasyDB(const EasyDB& other)
        {
            if (!other.init) {
                throw not_inited("EasyDB::EasyDB(const EasyDB& other): EasyDB object can not be constructed from another uninitialized object.");
            }
            init = true;
            dbfilename = other.dbfilename;
            version = other.version;
            tableName = other.tableName;
            rowCount = other.rowCount;
            rowSize = other.rowSize;
            columnCount = other.columnCount;
            primaryKeyIndex = other.primaryKeyIndex;
            dataOffset = other.dataOffset;
            dataTypes = other.dataTypes;
            dataSizes = other.dataSizes;
            columnNames = other.columnNames;
            dataFileOffset = other.dataFileOffset;
            table = other.table;
            columnIndexMap = other.columnIndexMap;
        }
        EasyDB(EasyDB&& other)
        {
            if (!other.init) {
                throw std::logic_error("EasyDB::EasyDB(const EasyDB& other): EasyDB object can not be constructed from another uninitialized object.");
            }
            init = true;
            dbfilename = other.dbfilename;
            version = other.version;
            tableName = other.tableName;
            rowCount = other.rowCount;
            rowSize = other.rowSize;
            columnCount = other.columnCount;
            primaryKeyIndex = other.primaryKeyIndex;
            dataOffset = std::move(other.dataOffset);
            dataTypes = std::move(other.dataTypes);
            dataSizes = std::move(other.dataSizes);
            columnNames = std::move(other.columnNames);
            dataFileOffset = other.dataFileOffset;
            table = std::move(other.table);
            columnIndexMap = std::move(other.columnIndexMap);
        }
        ~EasyDB()
        {
            IF_INIT(this) {
                this->closeNotSave();
            }
        }
        EasyDB& operator=(const EasyDB& other)
        {
            if (!other.init) {
                throw not_inited("EasyDB::EasyDB(const EasyDB& other): EasyDB object can not be constructed from another uninitialized object.");
            }
            IF_INIT(this) {
                this->close();
            }
            init = true;
            dbfilename = other.dbfilename;
            version = other.version;
            tableName = other.tableName;
            rowCount = other.rowCount;
            rowSize = other.rowSize;
            columnCount = other.columnCount;
            primaryKeyIndex = other.primaryKeyIndex;
            dataOffset = other.dataOffset;
            dataTypes = other.dataTypes;
            dataSizes = other.dataSizes;
            columnNames = other.columnNames;
            dataFileOffset = other.dataFileOffset;
            table = other.table;
            columnIndexMap = other.columnIndexMap;
            return *this;
        }
        EasyDB& operator=(EasyDB&& other)
        {
            if (!other.init) {
                throw not_inited("EasyDB::EasyDB(const EasyDB& other): EasyDB object can not be constructed from another uninitialized object.");
            }
            IF_INIT(this) {
                this->close();
            }
            init = true;
            dbfilename = other.dbfilename;
            version = other.version;
            tableName = other.tableName;
            rowCount = other.rowCount;
            rowSize = other.rowSize;
            columnCount = other.columnCount;
            primaryKeyIndex = other.primaryKeyIndex;
            dataOffset = std::move(other.dataOffset);
            dataTypes = std::move(other.dataTypes);
            dataSizes = std::move(other.dataSizes);
            columnNames = std::move(other.columnNames);
            dataFileOffset = other.dataFileOffset;
            table = std::move(other.table);
            columnIndexMap = std::move(other.columnIndexMap);
            return *this;
        }
        int dbgPrint()
        {
            using namespace std;
            for (auto& [k, r] : table) {
                cout << "Primary key :";
                visit([](auto& pk){cout << pk << endl;}, k);
                cout << "Data: ";
                for (auto& c : r) {
                    visit([](auto& x){cout << x << " ";}, c);
                }
                cout << endl;
            }
            return SUCCESS;
        }
        // 文件读写
        static int create(const std::string& filename, const std::string& tableName, std::vector<EColumn> tableHead, const std::string& primaryKeyColumnName)
        {
            size_t primaryKeyIndex = -1;
            std::unordered_set<std::string> columnNameSet;

            // 查找主键列、列名查重
            for (size_t i = 0; i < tableHead.size(); i++) {
                // 列名查重
                if (columnNameSet.find(tableHead[i].columnName) != columnNameSet.end()) {
                    return COLUMN_NAME_NOT_UNIQUE;
                } else {
                    columnNameSet.insert(tableHead[i].columnName);
                }
                
                // 查找主键列
                if (tableHead[i].columnName == primaryKeyColumnName) {
                    primaryKeyIndex = i;
                }
            }

            // 找不到主键
            if (primaryKeyIndex == -1) return PRIMARY_KEY_NOT_IN_LIST;
            
            // 写数据库文件
            std::ofstream dbfile;
            dbfile.open(filename, std::ios::out | std::ios::binary);
            if (dbfile.fail() || !dbfile.is_open()) {
                return FILE_OPEN_ERROR;
            }
            int32_t magicNum = EDB_MAGIC_NUMBER;
            dbfile.write(reinterpret_cast<const char*>(&magicNum), sizeof(magicNum));   // 魔数
            int32_t ver = EDB_VERSION;
            dbfile.write(reinterpret_cast<const char*>(&ver), 4);                       // 版本号
            size_t emptyRowCount = 0;
            dbfile.write(reinterpret_cast<const char*>(&emptyRowCount), EDB_INT_SIZE);  // 行数
            // 计算行长度
            size_t rowSize = 0;
            for (size_t i = 0; i < tableHead.size(); i++) {
                switch (tableHead[i].dType) {
                case EDB::DataType::INT:
                    rowSize += EDB_INT_SIZE;
                    tableHead[i].columnSize = EDB_INT_SIZE;
                    break;
                case EDB::DataType::REAL:
                    rowSize += EDB_REAL_SIZE;
                    tableHead[i].columnSize = EDB_REAL_SIZE;
                    break;
                case EDB::DataType::TEXT:
                    rowSize += tableHead[i].columnSize;
                    break;
                case EDB::DataType::BLOB:
                    rowSize += tableHead[i].columnSize;
                    break;
                default:
                    break;
                }
            }
            dbfile.write(reinterpret_cast<const char*>(&rowSize), EDB_INT_SIZE);            // 行长度
            size_t columnCount = tableHead.size();
            dbfile.write(reinterpret_cast<const char*>(&columnCount), EDB_INT_SIZE);        // 列数
            // 每列数据类型
            for (auto& i : tableHead) {
                dbfile.write(reinterpret_cast<const char*>(&(i.dType)), EDB_INT_SIZE);
            }
            // 每列数据长度
            for (auto& i : tableHead) {
                dbfile.write(reinterpret_cast<const char*>(&(i.columnSize)), EDB_INT_SIZE);
            }
            // 列名
            for (auto& i : tableHead) {
                dbfile.write(i.columnName.data(), i.columnName.length() + 1);
            }
            dbfile.write(tableName.data(), tableName.length() + 1);                         // 表名
            dbfile.write(reinterpret_cast<const char*>(&primaryKeyIndex), EDB_INT_SIZE);    // 主键索引
            dbfile.close();
            return SUCCESS;
        }
        int open(const std::string& dbFilePath)
        {
            IF_INIT(this){
                this->closeNotSave();
            }
            // 记录文件名
            this->dbfilename = dbFilePath;
            // 打开文件
            std::ifstream dbfile;
            dbfile.open(this->dbfilename, std::ios::in | std::ios::binary);
            if (dbfile.fail() || !dbfile.is_open()) {
                return FILE_OPEN_ERROR;
            }

            // 读取文件头
            // 魔数检查
            int32_t readMagicNum;
            dbfile.read(reinterpret_cast<char*>(&readMagicNum), 4);
            if (readMagicNum != EDB_MAGIC_NUMBER){
                return MAGIC_NUMBER_ERROR;
            }
            dbfile.read(reinterpret_cast<char*>(&this->version), 4);                    // 版本号
            dbfile.read(reinterpret_cast<char*>(&this->rowCount), EDB_INT_SIZE);        // 行数
            dbfile.read(reinterpret_cast<char*>(&this->rowSize), EDB_INT_SIZE);         // 行长度
            dbfile.read(reinterpret_cast<char*>(&this->columnCount), EDB_INT_SIZE);     // 列数
            this->dataTypes.resize(this->columnCount);
            // 每列数据类型
            for (size_t i = 0; i < this->columnCount; i++) {
                DataType tmp;
                dbfile.read(reinterpret_cast<char*>(&tmp), EDB_INT_SIZE);
                this->dataTypes[i] = tmp;
            }
            // 每列数据长度
            this->dataSizes.resize(this->columnCount);
            for (size_t i = 0; i < this->columnCount; i++) {
                size_t tmp;
                dbfile.read(reinterpret_cast<char*>(&tmp), EDB_INT_SIZE);
                this->dataSizes[i] = tmp;
            }
            // 列名
            this->columnNames.resize(this->columnCount);
            for (size_t i = 0; i < this->columnCount; i++) {
                char ch;
                std::string tmpStr;
                while ((ch = dbfile.get()) != '\0') tmpStr += ch;
                this->columnNames[i] = tmpStr;
                this->columnIndexMap[tmpStr] = i;
            }
            // 版本大于1则读取表名
            if (this->version >= 1)
            {
                char ch;
                this->tableName.clear();
                while ((ch = dbfile.get()) != '\0') this->tableName += ch;
            }
            dbfile.read(reinterpret_cast<char*>(&this->primaryKeyIndex), EDB_INT_SIZE);     // 主键索引
            size_t offset = 0;
            this->dataOffset.resize(this->columnCount);
            for (size_t i = 0; i < this->columnCount; i++)
            {
                this->dataOffset[i] = offset;
                switch (this->dataTypes[i])
                {
                case EDB::DataType::INT:
                    offset += EDB_INT_SIZE;
                    break;
                case EDB::DataType::REAL:
                    offset += EDB_REAL_SIZE;
                    break;
                case EDB::DataType::TEXT:
                    offset += this->dataSizes[i];
                    break;
                case EDB::DataType::BLOB:
                    offset += this->dataSizes[i];
                    break;
                default:
                    break;
                }
            }
            this->dataFileOffset = dbfile.tellg();

            // 读取记录数据
            std::unique_ptr<char[]> lineBuffer{new char[this->rowSize]};
            std::vector<Data> row;
            row.resize(this->columnCount);
            for (size_t i = 0; i < this->rowCount; i++)
            {
                dbfile.read(lineBuffer.get(), this->rowSize);
                for (size_t j = 0; j < this->columnCount; j++)
                {
                    switch (this->dataTypes[j])
                    {
                    case EDB::DataType::INT:
                        row[j] = *reinterpret_cast<long long*>(lineBuffer.get() + this->dataOffset[j]);
                        break;
                    case EDB::DataType::REAL:
                        row[j] = *reinterpret_cast<double*>(lineBuffer.get() + this->dataOffset[j]);
                        break;
                    case EDB::DataType::TEXT:{
                        const char* textPtr = lineBuffer.get() + this->dataOffset[j];
                        row[j] = std::string(textPtr, strlen(textPtr) < dataSizes[j] ? strlen(textPtr) : dataSizes[j]);
                        break;
                    }
                    case EDB::DataType::BLOB:
                        row[j] = Blob{new char[this->dataSizes[j]]};
                        memcpy(std::get<Blob>(row[j]).get(), lineBuffer.get() + this->dataOffset[j], this->dataSizes[j]);
                        break;
                    default:
                        break;
                    }
                }
                Data primaryKey = row[this->primaryKeyIndex];
                this->table[primaryKey] = row;
            }
            dbfile.close();
            this->init = true;
            return SUCCESS;
        }
        int closeNotSave()
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            this->dbfilename.clear();
            this->dataOffset.clear();
            this->dataTypes.clear();
            this->dataSizes.clear();
            this->columnNames.clear();
            this->columnIndexMap.clear();
            this->table.clear();
            init = false;
            return SUCCESS;
        }
        int save()
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            std::ofstream dbfile;
            dbfile.open(this->dbfilename, std::ios::out | std::ios::binary);
            if (dbfile.fail() || !dbfile.is_open())
            {
                return FILE_OPEN_ERROR;
            }
            int magicNum = EDB_MAGIC_NUMBER;
            dbfile.write(reinterpret_cast<const char*>(&magicNum), 4);                      // 魔数
            dbfile.write(reinterpret_cast<const char*>(&this->version), 4);                 // 版本号
            dbfile.write(reinterpret_cast<const char*>(&this->rowCount), EDB_INT_SIZE);     // 行数
            dbfile.write(reinterpret_cast<const char*>(&this->rowSize), EDB_INT_SIZE);      // 行长度
            dbfile.write(reinterpret_cast<const char*>(&this->columnCount), EDB_INT_SIZE);  // 列数
            // 每列数据类型
            for (auto& i : this->dataTypes) {
                dbfile.write(reinterpret_cast<const char*>(&i), EDB_INT_SIZE);
            }
            // 每列数据长度
            for (auto& i : this->dataSizes) {
                dbfile.write(reinterpret_cast<const char*>(&i), EDB_INT_SIZE);
            }
            // 列名
            for (auto& i : this->columnNames) {
                dbfile.write(i.data(), i.length() + 1);
            }
            // 表名
            if (this->version >= 1) {
                dbfile.write(this->tableName.data(), this->tableName.length() + 1);
            }
            // 主键索引
            dbfile.write(reinterpret_cast<const char*>(&this->primaryKeyIndex), EDB_INT_SIZE);
            // 数据
            for (auto& [primaryKey, rowData] : this->table) {
                for (size_t i = 0; i < this->columnCount; i++) {
                    switch (this->dataTypes[i])
                    {
                    case EDB::DataType::INT:
                        dbfile.write(reinterpret_cast<const char*>(&std::get<Int>(rowData[i])), EDB_INT_SIZE);
                        break;
                    case EDB::DataType::REAL:
                        dbfile.write(reinterpret_cast<const char*>(&std::get<Real>(rowData[i])), EDB_REAL_SIZE);
                        break;
                    case EDB::DataType::TEXT:
                    {
                        std::string str = std::get<Text>(rowData[i]);
                        if (str.length() < this->dataSizes[i])
                        {
                            dbfile.write(str.data(), str.length());
                            std::string padding(this->dataSizes[i] - str.length(), '\0');
                            dbfile.write(padding.data(), padding.length());
                        }
                        else
                        {
                            dbfile.write(str.data(), this->dataSizes[i]);
                        }
                        break;
                    }
                    case EDB::DataType::BLOB:
                        dbfile.write(std::get<Blob>(rowData[i]).get(), this->dataSizes[i]);
                        break;
                    default:
                        break;
                    }
                }
            }
            dbfile.close();
            return SUCCESS;
        }
        int close()
        {
            this->save();
            this->closeNotSave();
            this->init = false;
            return SUCCESS;
        }

        // 基础操作（STL兼容）
        virtual int insert(const std::unordered_map<std::string, Data>& row)
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            if (row.find(this->columnNames[this->primaryKeyIndex]) == row.end()) {
                return INVALID_ROW; 
            }
            if (this->table.find(row.at(this->columnNames[this->primaryKeyIndex])) != this->table.end()) {
                return PRIMARY_KEY_NOT_UNIQUE;
            }
            std::vector<Data> vrow;
            vrow.resize(this->columnCount);
            for (auto& [colName, colData] : row) {
                if (this->columnIndexMap.find(colName) == this->columnIndexMap.end()) continue;
                int colIndex = this->toColumnIndex(colName);
                IF_DATA_TYPE_NOT_MATCH(colIndex, colData)
                { return DATA_TYPE_NOT_MATCH; }
                IF_DATA_IS_TEXT_AND_TOO_LONG(colIndex, colData)
                { return TOO_LONG_DATA; }
                vrow[this->columnIndexMap[colName]] = colData;
            }
            for (size_t i = 0; i < this->columnCount; i++) {
                if (vrow[i].valueless_by_exception()) {
                    switch (this->dataTypes[i])
                    {
                    case EDB::DataType::INT:
                        vrow[i] = Int(0);
                        break;
                    case EDB::DataType::REAL:
                        vrow[i] = Real(0.0);
                        break;
                    case EDB::DataType::TEXT:
                        vrow[i] = Text("");
                        break;
                    case EDB::DataType::BLOB:
                        vrow[i] = EDB::Blob(new char[this->dataSizes[i]]);
                        break;
                    default:
                        break;
                    }
                }
            }
            return this->insert_not_check(vrow);
        }
        virtual int insert(const std::vector<Data>& row)
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            if (row.size() != this->columnCount) {
                return INVALID_ROW;
            }
            for (size_t i = 0; i < row.size(); i++) {
                IF_DATA_TYPE_NOT_MATCH(i, row[i])
                { return DATA_TYPE_NOT_MATCH; }
                IF_DATA_IS_TEXT_AND_TOO_LONG(i, row[i])
                { return TOO_LONG_DATA; }
            }
            if (this->table.find(row[this->primaryKeyIndex]) != this->table.end()) {
                return PRIMARY_KEY_NOT_UNIQUE;
            }
            return this->insert_not_check(row);
        }
        
        virtual int erase(const Data& primaryKey)
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            IF_DATA_TYPE_NOT_MATCH(this->primaryKeyIndex, primaryKey) {
                return DATA_TYPE_NOT_MATCH;
            }
            if (this->table.find(primaryKey) == this->table.end()) {
                return KEY_NOT_FOUND;
            }
            return this->erase_not_check(primaryKey);
        }
        
        virtual int update(const Data& primaryKey, int columnIndex, const Data& newData)
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            if (columnIndex < 0 || columnIndex >= this->columnCount)
            { return COLUMN_INDEX_OUT_OF_RANGE; }
            IF_DATA_TYPE_NOT_MATCH(this->primaryKeyIndex, primaryKey)
            { return DATA_TYPE_NOT_MATCH; }
            IF_DATA_TYPE_NOT_MATCH(columnIndex, newData)
            { return DATA_TYPE_NOT_MATCH; }
            IF_DATA_IS_TEXT_AND_TOO_LONG(columnIndex, newData)
            { return TOO_LONG_DATA; }
            if (this->table.find(primaryKey) == this->table.end())
            { return KEY_NOT_FOUND; }
            return this->update_not_check(primaryKey, columnIndex, newData);
        }
        virtual int update(const Data& primaryKey, const std::string& columnName, const Data& newData)
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            int colIndex = this->toColumnIndex(columnName);
            if (colIndex < 0)
            { return COLUMN_NOT_FOUND; }
            return this->update(primaryKey, colIndex, newData);
        }
        
        virtual RowViewType at(const Data& primaryKey)
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::at(const Data& primaryKey) : Not inited!");
            if (this->table.find(primaryKey) == this->table.end())
            { throw key_not_found("EasyDB::at(const Data& primaryKey) : Key not found!"); }
            return EDB::EasyDB::RowViewType{table[primaryKey], this->columnIndexMap};
        }
        virtual const Data& at(const Data& primaryKey, int columnIndex) const
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::at(const Data& primaryKey, int columnIndex) : Not inited!");
            if (columnIndex < 0 || columnIndex >= this->columnCount)
            { throw std::out_of_range("EasyDB::at(const Data& primaryKey, int columnIndex) : Column index out of range!"); }
            if (this->table.find(primaryKey) == this->table.end())
            { throw key_not_found("EasyDB::at(const Data& primaryKey, int columnIndex) : Key not found!"); }
            return this->table.at(primaryKey).at(columnIndex);
        }
        virtual const Data& at(const Data& primaryKey, const std::string& columnName) const
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::at(const Data& primaryKey, const std::string& columnName) : Not inited!");
            int columnIndex = this->toColumnIndex(columnName);
            if (columnIndex < 0)
            { throw column_not_found("EasyDB::at(const Data& primaryKey, const std::string& columnName) : Column not found!"); }
            if (this->table.find(primaryKey) == this->table.end())
            { throw key_not_found("EasyDB::at(const Data& primaryKey, const std::string& columnName) : Key not found!"); }
            return this->table.at(primaryKey).at(columnIndex);
        }
        virtual RowViewType operator[](const Data& primaryKey)
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::operator[](const Data& primaryKey) : Not inited!");
            // 类型检查
            IF_DATA_TYPE_NOT_MATCH(this->primaryKeyIndex, primaryKey)
            { throw type_not_match("EasyDB::operator[](const Data& primaryKey) : primaryKey type not match!"); }
            if (this->table.find(primaryKey) == this->table.end())
            { throw key_not_found("EasyDB::at(const Data& primaryKey, const std::string& columnName) : Key not found!"); }
            return EDB::EasyDB::RowViewType{table[primaryKey], this->columnIndexMap};
        }

        virtual std::vector<RowViewType> find(int columnIndex, const Data& inKey)
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::find(int columnIndex, const Data& inKey) : Not inited!");
            if (columnIndex == this->primaryKeyIndex) {
                std::vector<RowViewType> res;
                if (this->table.find(inKey) != this->table.end()) {
                    res.push_back(RowViewType{this->table[inKey], this->columnIndexMap});
                }
                return res;
            }
            if (columnIndex < 0 || columnIndex >= this->columnCount)
            { throw std::out_of_range("EasyDB::find(int columnIndex, const Data& inKey) : Column index out of range!"); }
            IF_DATA_TYPE_NOT_MATCH(columnIndex, inKey)
            { throw type_not_match("EasyDB::find(int columnIndex, const Data& inKey) : Data type not match!"); }
            std::vector<RowViewType> res;
            for (auto& [primaryKey, row] : this->table) {
                if (row[columnIndex] == inKey) {
                    res.push_back(RowViewType{row, this->columnIndexMap});
                }
            }
            return res;
        }
        virtual std::vector<RowViewType> find(const std::string& columnName, const Data& inKey)
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::find(const std::string& columnName, const Data& inKey) : Not inited!");
            int columnIndex = this->toColumnIndex(columnName);
            if (columnIndex < 0)
            { throw column_not_found("EasyDB::find(const std::string& columnName, const Data& inKey) : Column not found!"); }
            return this->find(columnIndex, inKey);
        }
        iterator begin()
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::begin() : Not inited!");
            return iterator{this->table.begin(), this};
        }
        iterator end()
        {
            IF_NOT_INIT_THROW_ERROR("EasyDB::end() : Not inited!");
            return iterator{this->table.end(), this};
        }

        // 枚举
        bool row_enum(std::function<bool(RowViewType)> execFunc)
        {
            for (auto& row : table) {
                if (execFunc(RowViewType(row.second, columnIndexMap)) == false) {
                    return false;
                }
            }
            return true;
        }
        std::vector<RowViewType> row_filter(std::function<bool(RowViewType)> filter)
        {
            std::vector<RowViewType> res;
            for (auto& row : table) {
                RowViewType rowView = RowViewType(row.second, columnIndexMap);
                if (filter(rowView) == true) {
                    res.push_back(rowView);
                }
            }
            return res;
        }

        // 杂项
        int toColumnIndex(const std::string& columnName) const
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            if (this->columnIndexMap.find(columnName) == this->columnIndexMap.end()) {
                return COLUMN_NOT_FOUND;
            }
            return this->columnIndexMap.at(columnName);
        }
        bool is_data_type_match(int columnIndex, const Data& data)
        {
            if (columnIndex < 0 || columnIndex >= this->columnCount)
                return false;
            IF_DATA_TYPE_NOT_MATCH(columnIndex, data) return false;
            return true;
        }
        bool is_data_type_match(const std::string& columnName, const Data& data)
        {
            int colIndex = this->toColumnIndex(columnName);
            if (colIndex < 0)
                return false;
            return this->is_data_type_match(colIndex, data);
        }
        DataType get_column_data_type(int columnIndex)
        {
            if (columnIndex < 0 || columnIndex >= this->columnCount)
            { throw std::out_of_range("EasyDB::get_column_data_type(int columnIndex) : Column index out of range!"); }
            return this->dataTypes[columnIndex];
        }
        DataType get_column_data_type(const std::string& columnName)
        {
            int colIndex = this->toColumnIndex(columnName);
            if (colIndex < 0 || colIndex >= this->columnCount)
            { throw column_not_found("EasyDB::get_column_data_type(const std::string& columnName) : Column not found!"); }
            return this->get_column_data_type(colIndex);
        }
        bool hasrow(const Data& primaryKey)
        {
            IF_DATA_TYPE_NOT_MATCH(primaryKeyIndex, primaryKey) {
                return false;
            }
            return table.find(primaryKey) != table.end();
        }
        bool hascolumn(const std::string& columnName)
        {
            return columnIndexMap.find(columnName) != columnIndexMap.end();
        }
        bool hascolumns(const std::vector<std::string>& columnNames)
        {
            for (auto& colName : columnNames) {
                if (!hascolumn(colName)) {
                    return false;
                }
            }
            return true;
        }

        // 元数据
        std::vector<EColumn> get_column_info() const
        {
            std::vector<EDB::EColumn> res;
            res.resize(this->columnCount);
            for (size_t i = 0; i < this->columnCount; i++)
            {
                res[i].dType = this->dataTypes[i];
                res[i].columnName = this->columnNames[i];
                res[i].columnSize = this->dataSizes[i];
            }
            return res;
        }
        size_t size() const
        { return this->rowCount; }
        size_t get_row_count() const
        { return this->rowCount; }
        bool is_open() const
        { return this->init; }
        bool is_init() const
        { return this->init; }
        int get_version() const
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            return this->version;
        }
        std::string get_table_name() const
        {
            IF_NOT_INIT(this) return "";
            if (this->version < 1) return "";
            return this->tableName;
        }
        int set_table_name(const std::string& newTableName)
        {
            IF_NOT_INIT_RETURN_ERR_CODE
            if (this->version < 1) this->version = EDB_VERSION;
            this->tableName = newTableName;
            return SUCCESS;
        }
        int get_primary_key_index() const
        { return this->primaryKeyIndex; }

        // Dump and load
        int dump(DataObject& obj) {
            return obj.edb_dump(*this);
        }
        int load(const EDB::Data& primaryKey, EDB::DataObject& obj) {
            if (hasrow(primaryKey)) {
                return obj.edb_load(this->at(primaryKey));
            }
            return KEY_NOT_FOUND;
        }
        template <typename T>
        requires std::constructible_from<T, EDB::EasyDB::RowViewType>
        T load(const EDB::Data& primaryKey) {
            if (hasrow(primaryKey)) {
                return T{at(primaryKey)};
            } else {
                throw key_not_found("EasyDB::load(const EDB::Data& primaryKey): Key not found");
            }
        }
        template <typename T>
        requires (std::constructible_from<T> && !std::constructible_from<T, EDB::EasyDB::RowViewType>)
        T load(const EDB::Data& primaryKey) {
            T obj{};
            if (hasrow(primaryKey)) {
                obj.edb_load(at(primaryKey));
            } else {
                throw key_not_found("EasyDB::load(const EDB::Data& primaryKey): Key not found");
            }
            return obj;
        }
    };
}

#endif