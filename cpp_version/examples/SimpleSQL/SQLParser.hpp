#ifndef SQL_PARSER
#define SQL_PARSER

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cctype>
#include <unordered_set>

namespace SQLParser
{
    class ASTVisitor;
    class BinaryExpr;
    class ComparisonExpr;
    class InsertStatement;
    class SelectStatement;
    class UpdateStatement;
    class DeleteStatement;
    class CreateStatement;

    // Token类型枚举
    enum class TokenType {
        KEYWORD,
        IDENTIFIER,
        SYMBOL,
        STRING_LITERAL,
        NUMBER,
        END
    };

    static inline const char* TokenTypetToString(TokenType type) {
        switch (type) {
            case TokenType::KEYWORD:       return "KEYWORD";
            case TokenType::IDENTIFIER:    return "IDENTIFIER";
            case TokenType::SYMBOL:        return "SYMBOL";
            case TokenType::STRING_LITERAL:return "STRING_LITERAL";
            case TokenType::NUMBER:        return "NUMBER";
            case TokenType::END:           return "END";
            default:                       return "UNKNOWN_TOKEN";
        }
    }

    // 二操作数操作符枚举
    enum class BinaryOp { AND, OR, EQ, NEQ, GT, LT, GTE, LTE };

    static inline const char* BinaryOpToString(BinaryOp op) {
        switch (op) {
            case BinaryOp::AND: return "AND";
            case BinaryOp::OR:  return "OR";
            case BinaryOp::EQ:  return "=";
            case BinaryOp::NEQ: return "!=";
            case BinaryOp::GT:  return ">";
            case BinaryOp::LT:  return "<";
            case BinaryOp::GTE: return ">=";
            case BinaryOp::LTE: return "<=";
            default:            return "UNKNOWN_OP";
        }
    }

    // AST结点
    struct ASTNode
    {
        virtual int exec(ASTVisitor* visitor, void* args = nullptr) = 0;
    };

    // 访问器
    class ASTVisitor
    {
    public:
        virtual int visit(BinaryExpr* node, void* args = nullptr) = 0;
        virtual int visit(ComparisonExpr* node, void* args = nullptr) = 0;
        virtual int visit(InsertStatement* node, void* args = nullptr) = 0;
        virtual int visit(SelectStatement* node, void* args = nullptr) = 0;
        virtual int visit(UpdateStatement* node, void* args = nullptr) = 0;
        virtual int visit(DeleteStatement* node, void* args = nullptr) = 0;
        virtual int visit(CreateStatement* node, void* args = nullptr) = 0;
    };

    struct Expr: public ASTNode
    {
        virtual ~Expr() = default;
    };
    
    // 与或
    struct BinaryExpr : public Expr {
        BinaryOp op;
        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;
        BinaryExpr(BinaryOp o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
            : op(o), left(std::move(l)), right(std::move(r)) {}
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };
    
    // 比较运算
    struct ComparisonExpr : public Expr {
        std::string left;
        BinaryOp op; // =, >, < ...
        std::string right;
        ComparisonExpr(const std::string& l, const std::string& o, const std::string& r)
            : left(l), right(r)
        {
            if (o == "=") op = BinaryOp::EQ;
            else if (o == ">") op = BinaryOp::GT;
            else if (o == ">=") op = BinaryOp::GTE;
            else if (o == "<") op = BinaryOp::LT;
            else if (o == "<=") op = BinaryOp::LTE;
            else if (o == "!=") op = BinaryOp::NEQ;
        }
        ComparisonExpr(const std::string& l, BinaryOp o, const std::string& r)
            : left(l), op(o), right(r) {}
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };

    // AST树值表达式结点
    struct Expression : public ASTNode
    {
        virtual ~Expression() = default;
    };
    
    // AST树操作表达式结点
    struct Statement : public ASTNode
    {
        virtual ~Statement() = default;
    };
    
    // 插入操作
    struct InsertStatement : public Statement {
        std::string table;
        std::vector<std::string> columns;
        std::vector<std::string> values;
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };
    
    // 选择操作
    struct SelectStatement : public Statement {
        std::vector<std::string> columns;
        std::string table;
        std::unique_ptr<Expr> where;
        std::unique_ptr<SelectStatement> subquery;
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };
    
    // 修改操作
    struct UpdateStatement : public Statement {
        std::string table;
        std::vector<std::pair<std::string, std::string>> setClauses;
        std::unique_ptr<Expr> where;
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };
    
    // 删除操作
    struct DeleteStatement : public Statement {
        std::string table;
        std::unique_ptr<Expr> where;
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };

    // 新建操作
    struct CreateStatement : public Statement {
        std::string tableName;
        std::string primaryKey;
        std::vector<std::string> columnNames;
        std::vector<std::string> dataTypes;
        std::vector<std::string> dataSizes;
        int exec(ASTVisitor* visitor, void* args = nullptr)
        { return visitor->visit(this, args); }
    };

    // 打印AST
    class ASTPrinter : public ASTVisitor
    {
    public:
        int visit(BinaryExpr* node, void* args = nullptr)
        {
            std::cout << "(BinaryExpr: ";
            node->left->exec(this);
            std::cout << " " << BinaryOpToString(node->op) << " ";
            node->right->exec(this);
            std::cout << ")";
            return true;
        }
        int visit(ComparisonExpr* node, void* args = nullptr)
        {
            std::cout << "(ComparisonExpr: " << node->left << " " << SQLParser::BinaryOpToString(node->op) << " " << node->right << ")";
            return true;
        }
        int visit(InsertStatement* node, void* args = nullptr)
        {
            std::cout << "(Insert: ";
            for (size_t i = 0; i < node->columns.size(); i++)
            {
                std::cout << "[" << node->columns[i] << ", " << node->values[i] << "]";
            }
            std::cout << "INTO: " << node->table;
            std::cout << ")";
            return true;
        }
        int visit(SelectStatement* node, void* args = nullptr)
        {
            std::cout << "(Select: ";
            std::cout << " COLUMNS: ";
            for (size_t i = 0; i < node->columns.size(); i++)
            {
                std::cout << node->columns[i] << " ";
            }
            std::cout << "FROM: " << node->table;
            std::cout << " WHERE: ";
            if (node->where) node->where->exec(this);
            std::cout << " Subquery: ";
            if (node->subquery) node->subquery->exec(this);
            std::cout << " )";
            return true;
        }
        int visit(UpdateStatement* node, void* args = nullptr)
        {
            std::cout << "(Update: ";
            for (size_t i = 0; i < node->setClauses.size(); i++)
            {
                std::cout << "[" << node->setClauses[i].first << ", " << node->setClauses[i].second << "]";
            }
            std::cout << "INTO: " << node->table;
            std::cout << " WHERE: ";
            if (node->where) node->where->exec(this);
            std::cout << " )";
            return true;
        }
        int visit(DeleteStatement* node, void* args = nullptr)
        {
            std::cout << "(Delete: ";
            std::cout << " WHERE: ";
            if (node->where) node->where->exec(this);
            std::cout << " FROM: " << node->table;
            std::cout << " )";
            return true;
        }
        int visit(CreateStatement* node, void* args = nullptr)
        {
            std::cout << "(Create)";
            return true;
        }
    };

    // Token类
    struct Token {
        TokenType type;
        std::string value;
    };
    
    // token化
    class Tokenizer {
    public:
        Tokenizer(const std::string& input) : sql(input), pos(0) {}
    
        Token nextToken() {
            skipWhitespace();
            if (pos >= sql.size() || sql[pos] == ';') return {TokenType::END, ""};

            char ch = sql[pos];
    
            // 关键词或标识符
            if (isalpha(ch) || ch > 127)
            {
                std::string word;
                std::string origWord;
                while (pos < sql.size() && (isalnum(sql[pos]) || sql[pos] == '_' || sql[pos] > 127))
                {
                    origWord += sql[pos];
                    word += toupper(sql[pos]);
                    ++pos;
                }
                if (isKeyword(word))
                    return {TokenType::KEYWORD, word};
                else
                    return {TokenType::IDENTIFIER, origWord};
            }
            // 数字
            if (isdigit(ch) || ch == '-')
            {
                std::string num;
                while (pos < sql.size() && (isdigit(sql[pos]) || sql[pos] == '.'))
                    num += sql[pos++];
                return {TokenType::NUMBER, num};
            }
            // 字符串字面量
            if (ch == '\'')
            {
                std::string str;
                pos++;  // 跳过开始
                while (pos < sql.size() && sql[pos] != '\'')
                    str += sql[pos++];
                pos++;  // 跳过结束
                return {TokenType::STRING_LITERAL, str};
            }
            // >= <= !=
            if (ch == '<' || ch == '>' || ch == '!')
            {
                ++pos;
                std::string sym = std::string(1, ch);
                if (sql[pos] == '=') sym += sql[pos++];
                return {TokenType::SYMBOL, sym};
            }
            // 单字符符号
            pos++;
            return {TokenType::SYMBOL, std::string(1, ch)};
        }
    
    private:
        std::string sql;
        size_t pos;
    
        void skipWhitespace() {
            while (pos < sql.size() && isspace(sql[pos])) pos++;
        }
    
        bool isKeyword(const std::string& word) {
            static const std::unordered_set<std::string> keywords = {
                "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "UPDATE", "SET", "DELETE", "AS", "INT", "TEXT", "REAL", "PRIMARY", "KEY", "NOT", "NULL", "TABLE"
            };
            return keywords.find(word) != keywords.end();
        }
    };
    
    // 语法解析
    class Parser {
    public:
        Parser(Tokenizer& tokenizer) : tokenizer(tokenizer) {
            nextToken();
        }
    
        std::unique_ptr<Statement> parseStatement() {
            if (current.value == "SELECT") {
                return parseSelect();
            }
            if (current.value == "INSERT") {
                return parseInsert();
            }
            if (current.value == "UPDATE") {
                return parseUpdate();
            }
            if (current.value == "DELETE") {
                return parseDelete();
            }
            if (current.value == "CREATE") {
                return parseCreate();
            }
            throw std::runtime_error("Unknown statement: " + current.value);
        }
    
    private:
        Tokenizer& tokenizer;
        Token current;
    
        void nextToken() {
            current = tokenizer.nextToken();
        }
    
        // SELECT
        std::unique_ptr<SelectStatement> parseSelect() {
            auto stmt = std::make_unique<SelectStatement>();
            nextToken();
    
            // 解析要查询的列
            while (current.type == TokenType::IDENTIFIER) {
                stmt->columns.emplace_back(current.value);
                nextToken();
                if (current.value == ",") {
                    nextToken();
                } else if (current.value != ",") {
                    break;
                }
            }
            // 列为通配符时
            if (current.type == TokenType::SYMBOL && current.value == "*")
            {
                stmt->columns.resize(1);
                stmt->columns[0] = "*";
                nextToken();
            }
    
            // 可选FROM
            if (current.value == "FROM")
            {
                nextToken(); // skip FROM
                if (current.value == "(") {
                    // 子查询
                    nextToken(); // skip '('
                    stmt->subquery = parseSelect();
                    if (current.value != ")") throw std::runtime_error("Expected )");
                    nextToken();
                    if (current.value == "AS") nextToken();
                    stmt->table = current.value;
                    nextToken();
                } else {
                    stmt->table = current.value;
                    nextToken();
                }
            }
            
    
            if (current.type == TokenType::KEYWORD || current.value == "WHERE") {
                nextToken();
                stmt->where = parseExpression();
            }

            if (current.type != TokenType::END) {
                throw std::runtime_error("Unexpected " + current.value);
            }
    
            return stmt;
        }
    
        // INSERT
        std::unique_ptr<InsertStatement> parseInsert() {
            auto stmt = std::make_unique<InsertStatement>();
            nextToken();
            // 多表支持
            if (current.value == "INTO")
            {
                nextToken();
                stmt->table = current.value;
                nextToken();
            }
    
            if (current.value == "(") {
                nextToken();
                while (current.type == TokenType::IDENTIFIER) {
                    stmt->columns.push_back(current.value);
                    nextToken();
                    if (current.value == ",") nextToken();
                }
                if (current.value != ")") throw std::runtime_error("Expected )");
                nextToken();
            }
    
            if (current.value != "VALUES") throw std::runtime_error("Expected VALUES");
            nextToken();
    
            if (current.value != "(") throw std::runtime_error("Expected (");
            nextToken();
            while (current.type == TokenType::STRING_LITERAL || current.type == TokenType::NUMBER) {
                stmt->values.push_back(current.value);
                nextToken();
                if (current.value == ",") nextToken();
            }
            if (current.value != ")") throw std::runtime_error("Expected )");
            nextToken();
    
            return stmt;
        }
    
        // UPDATE
        std::unique_ptr<UpdateStatement> parseUpdate() {
            auto stmt = std::make_unique<UpdateStatement>();
            nextToken(); // skip UPDATE
    
            // 多表支持
            if (current.type == TokenType::IDENTIFIER)
            {
                stmt->table = current.value;
                nextToken();
            }
    
            if (current.value != "SET") throw std::runtime_error("Expected SET");
            nextToken();
    
            while (current.type == TokenType::IDENTIFIER) {
                std::string column = current.value;
                nextToken();
                if (current.value != "=") throw std::runtime_error("Expected =");
                nextToken();
                std::string value = current.value;
                nextToken();
                stmt->setClauses.emplace_back(column, value);
                if (current.value == ",") nextToken();
                else break;
            }
    
            if (current.value == "WHERE") {
                nextToken();
                stmt->where = parseExpression();
            }
    
            return stmt;
        }
    
        // DELETE
        std::unique_ptr<DeleteStatement> parseDelete() {
            auto stmt = std::make_unique<DeleteStatement>();
            nextToken(); // skip DELETE
    
            if (current.value == "FROM")
            {
                nextToken();
                stmt->table = current.value;
                nextToken();
            }
    
            if (current.value == "WHERE") {
                nextToken();
                stmt->where = parseExpression();
            }
    
            return stmt;
        }

        // CREATE
        std::unique_ptr<CreateStatement> parseCreate()
        {
            auto stmt = std::make_unique<CreateStatement>();
            nextToken();

            if (current.type != TokenType::IDENTIFIER) throw std::runtime_error("Expected <Table name>");
            stmt->tableName = current.value;
            nextToken();

            if (current.value != "(") throw std::runtime_error("Expected (");
            nextToken();
            while (current.type == TokenType::IDENTIFIER)
            {
                stmt->columnNames.push_back(current.value);
                nextToken();
                
                if (current.type == TokenType::KEYWORD) stmt->dataTypes.push_back(current.value);
                else throw std::runtime_error("Expected <DataType>");
                nextToken();

                stmt->dataSizes.push_back("0");
                if (stmt->dataTypes.back() == "TEXT")
                {
                    if (current.type != TokenType::NUMBER) throw std::runtime_error("Expected TEXT <DataSize>");
                    stmt->dataSizes.back() = current.value;
                    nextToken();
                }

                if (current.type == TokenType::KEYWORD && current.value == "PRIMARY")
                {
                    nextToken();
                    if (current.value == "KEY")
                    {
                        if (stmt->primaryKey.empty()) stmt->primaryKey = stmt->columnNames.back();
                        nextToken();
                    }
                    else std::runtime_error("Expected PRIMARY KEY");
                }
                if (current.value == ")") break;
                if (current.value == ",") nextToken();
            }

            if (stmt->columnNames.empty()) throw std::runtime_error("Cannot create no columns table.");

            return stmt;
        }

        std::unique_ptr<Expr> parseExpression() {
            return parseOrExpression();
        }
        
        // 或
        std::unique_ptr<Expr> parseOrExpression() {
            auto left = parseAndExpression();
            while (current.value == "OR") {
                nextToken();
                auto right = parseAndExpression();
                left = std::make_unique<BinaryExpr>(BinaryOp::OR, std::move(left), std::move(right));
            }
            return left;
        }
        
        // 与
        std::unique_ptr<Expr> parseAndExpression() {
            auto left = parseComparison();
            while (current.value == "AND") {
                nextToken();
                auto right = parseComparison();
                left = std::make_unique<BinaryExpr>(BinaryOp::AND, std::move(left), std::move(right));
            }
            return left;
        }
        
        // 对比
        std::unique_ptr<Expr> parseComparison() {
            if (current.value == "(") {
                nextToken();
                auto expr = parseExpression();
                if (current.value != ")") throw std::runtime_error("Expected )");
                nextToken();
                return expr;
            }
        
            std::string left = current.value;
            nextToken();
            std::string op = current.value;
            nextToken();
            std::string right = current.value;
            nextToken();
        
            return std::make_unique<ComparisonExpr>(left, op, right);
        }
    };
    
}

#endif