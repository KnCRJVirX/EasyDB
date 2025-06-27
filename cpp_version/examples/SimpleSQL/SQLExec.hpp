#ifndef SQL_EXEC
#define SQL_EXEC

#include <string>

#include "SQLParser.hpp"
#include "EasyDB.hpp"

namespace SQLExec
{

    class SQLExecutor : public SQLParser::ASTVisitor
    {
    public:
        EDB::EasyDB edb;
        SQLExecutor() {};
        SQLExecutor(const std::string& filepath)
        {
            edb.open(filepath);
        }
        ~SQLExecutor()
        {
            edb.close();
        }
        int open(const std::string& filepath)
        { return edb.open(filepath); }
        int close()
        { return edb.close(); }
        int closeNotSave()
        { return edb.closeNotSave(); }
        int save()
        { return edb.save(); }
        int visit(SQLParser::BinaryExpr* node, void* args = nullptr)
        {
            int l = node->left->exec(this, args), r = node->right->exec(this, args);
            switch (node->op)
            {
            case SQLParser::BinaryOp::AND : return (l && r);
            case SQLParser::BinaryOp::OR :  return (l || r);
            default: break;
            }
            return true;
        }
        int visit(SQLParser::ComparisonExpr* node, void* args = nullptr)
        {
            EDB::Data* priKey = reinterpret_cast<EDB::Data*>(args);
            // 解析输入的右值
            EDB::Data rData;
            try
            {
                switch (edb.get_column_data_type(node->left))
                {
                case EDB::DataType::INT:
                    rData = std::stoll(node->right);
                    break;
                case EDB::DataType::REAL:
                    rData = std::stod(node->right);
                    break;
                case EDB::DataType::TEXT:
                    rData = node->right;
                    break;
                default:
                    return false;
                    break;
                }
            }
            catch(const std::exception& e)
            { return false; }
            catch(...)
            { return false; }
            // 比较
            switch (node->op)
            {
            case SQLParser::BinaryOp::EQ :  return (edb[*priKey][node->left] == rData);
            case SQLParser::BinaryOp::NEQ : return (edb[*priKey][node->left] != rData);
            case SQLParser::BinaryOp::GT :  return (edb[*priKey][node->left] > rData);
            case SQLParser::BinaryOp::GTE : return (edb[*priKey][node->left] >= rData);
            case SQLParser::BinaryOp::LT :  return (edb[*priKey][node->left] < rData);
            case SQLParser::BinaryOp::LTE : return (edb[*priKey][node->left] <= rData);
            default: break;
            }
            return false;
        }
        int visit(SQLParser::InsertStatement* node, void* args = nullptr)
        {
            int retval = 0;
            if (node->columns.empty())
            {
                auto colInfos = edb.get_column_info();
                if (node->values.size() != colInfos.size()) return INVALID_ROW;
                
                std::vector<EDB::Data> row;
                row.resize(colInfos.size());
                for (size_t i = 0; i < colInfos.size(); i++)
                {
                    try
                    {
                        switch (colInfos[i].dType)
                        {
                        case EDB::DataType::INT:
                            row[i] = std::stoll(node->values[i]);
                            break;
                        case EDB::DataType::REAL:
                            row[i] = std::stod(node->values[i]);
                            break;
                        case EDB::DataType::TEXT:
                            row[i] = node->values[i];
                            break;
                        default:
                            break;
                        }
                    }
                    catch(const std::exception& e)
                    {
                        return DATA_TYPE_NOT_MATCH;
                    }
                }
                retval = edb.insert(row);
            }
            else
            {
                std::unordered_map<std::string, EDB::Data> row;
                for (size_t i = 0; i < node->columns.size(); i++)
                {
                    try
                    {
                        switch (edb.get_column_data_type(node->columns[i]))
                        {
                        case EDB::DataType::INT:
                            row[node->columns[i]] = std::stoll(node->values[i]);
                            break;
                        case EDB::DataType::REAL:
                            row[node->columns[i]] = std::stod(node->values[i]);
                            break;
                        case EDB::DataType::TEXT:
                            row[node->columns[i]] = node->values[i];
                            break;
                        default:
                            break;
                        }
                    }
                    catch(...)
                    { return DATA_TYPE_NOT_MATCH; }
                }
                retval = edb.insert(row);
            }
            return retval;
        }
        int visit(SQLParser::SelectStatement* node, void* args = nullptr)
        {
            if (node->columns.empty()) return UNKNOWN_ERROR;
            std::vector<std::string> printColumns;
            auto colInfos = edb.get_column_info();
            // 列为通配符时打印所有列
            if (node->columns[0] == "*")
            {
                for (auto&& it : colInfos)
                { printColumns.push_back(it.columnName); }
            }
            // 检查是否列都存在
            else
            {
                std::unordered_set<std::string> colSet;
                for (auto&& it : colInfos) { colSet.insert(it.columnName); }
                for (auto&& col : node->columns)
                {
                    if (colSet.find(col) == colSet.end())
                    { return COLUMN_NOT_FOUND; }
                    printColumns.push_back(col);
                }
            }
            // 打印表
            bool first_col = true;
            for (auto&& c : edb)
            {
                if (node->where && node->where->exec(this, (void*)&(c[edb.get_primary_key_index()])) == false)
                { continue; }
                std::unordered_map cMap = c.to_map();
                first_col = true;
                for (auto&& colName : printColumns)
                {
                    if (first_col)
                    {
                        std::visit([](auto&& d){ std::cout << d; }, cMap[colName]);
                        first_col = false;
                    }
                    else
                    { std::visit([](auto&& d){ std::cout << "|" << d; }, cMap[colName]); }
                }
                std::cout << std::endl;
            }
            return 0;
        }
        int visit(SQLParser::UpdateStatement* node, void* args = nullptr)
        {
            if (node->setClauses.empty()) return UNKNOWN_ERROR;

            std::unordered_set<std::string> colSet;
            auto colInfos = edb.get_column_info();
            for (auto&& colName: colInfos) {colSet.insert(colName.columnName); }

            // 解析要修改的数据
            std::unordered_map<std::string, EDB::Data> updateDatas;
            for (auto&& d : node->setClauses)
            {
                if (colSet.find(d.first) == colSet.end()) return COLUMN_NOT_FOUND;
                try
                {
                    switch (edb.get_column_data_type(d.first))
                    {
                    case EDB::DataType::INT:
                        updateDatas[d.first] = std::stoll(d.second);
                        break;
                    case EDB::DataType::REAL:
                        updateDatas[d.first] = std::stod(d.second);
                        break;
                    case EDB::DataType::TEXT:
                        updateDatas[d.first] = d.second;
                        break;
                    default:
                        break;
                    }
                }
                catch(...)
                { return DATA_TYPE_NOT_MATCH; }
            }
            
            // 遍历表并修改
            int priKeyIndex = edb.get_primary_key_index();
            int retval = 0;
            for (auto&& c : edb)
            {
                if (node->where && node->where->exec(this, (void*)&(c[edb.get_primary_key_index()])) == false)
                { continue; }
                for (auto& [colName, data] : updateDatas)
                {
                    edb.update(c[priKeyIndex], colName, data);
                    if (retval) return retval;
                }
            }
            return 0;
        }
        int visit(SQLParser::DeleteStatement* node, void* args = nullptr)
        {
            // 遍历表并删除
            int priKeyIndex = edb.get_primary_key_index();
            int retval = 0;
            for (auto&& c : edb)
            {
                if (node->where && node->where->exec(this, (void*)&(c[edb.get_primary_key_index()])) == false)
                { continue; }
                // std::visit([](auto&& d){ std::cout << "Delete: " << d << std::endl; }, c[priKeyIndex]);
                retval = edb.erase(c[priKeyIndex]);
                if (retval) return retval;
            }
            return 0;
        }
        int visit(SQLParser::CreateStatement* node, void* args = nullptr)
        {
            if (node->columnNames.size() != node->dataTypes.size()) return UNKNOWN_ERROR;

            std::string filepath;
            if (args) filepath = *(reinterpret_cast<std::string*>(args));

            std::vector<EDB::EColumn> cols;
            cols.resize(node->columnNames.size());
            for (int i = 0; i < node->columnNames.size(); ++i)
            {
                cols[i].columnName = node->columnNames[i];
                
                if (node->dataTypes[i] == "INT") cols[i].dType = EDB::DataType::INT;
                else if (node->dataTypes[i] == "REAL") cols[i].dType = EDB::DataType::REAL;
                else if (node->dataTypes[i] == "TEXT") cols[i].dType = EDB::DataType::TEXT;

                try
                {
                    cols[i].columnSize = std::stoll(node->dataSizes[i]);
                }
                catch(...)
                { return INVALID_ROW; }
            }
            
            if (filepath.empty())
            {
                std::cout << "Database filepath: ";
                std::getline(std::cin, filepath);
            }
            
            return edb.create(filepath, node->tableName, cols, node->primaryKey);
        }
    };
}

#endif