#include <iostream>
#include <string>

#include "SQLParser.hpp"
#include "SQLExec.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char const *argv[])
{
    #ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    #endif

    int retval = 0;
    std::string command;
    std::string dbFilePath;
    
    SQLExec::SQLExecutor executor{};
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, command);
        if (command.find("CREATE") == 0)
        {
            SQLParser::Tokenizer tokenizer{command};
            SQLParser::Parser parser{tokenizer};
            std::unique_ptr<SQLParser::Statement> astRoot;
            try
            {
                astRoot = parser.parseStatement();
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                continue;
            }
            retval = astRoot->exec(&executor);
            if (retval) std::cout << "Fail, Error code: " << EDB::StatusCode2String(retval) << std::endl;
        }
        else if (command == ".exit")
        {
            break;
        }
        else if (command.find(".open") == 0)
        {
            if (command.rfind(" ") != command.size() - 1 && command.rfind(" ") != std::string::npos)
            {
                dbFilePath = command.substr(command.rfind(" ") + 1);
            }
            
            if (dbFilePath.empty())
            {
                std::cout << "Database filepath: ";
                std::getline(std::cin, dbFilePath);
            }
            
            retval = executor.open(dbFilePath);
            if (!retval)
            {
                while (true)
                {
                    std::cout << executor.edb.get_table_name() << "> ";
                    std::getline(std::cin, command);
                    if (command == ".close")
                    {
                        executor.close();
                        dbFilePath.clear();
                        break;
                    }
                    else if (command.find("CREATE") == 0)
                    {
                        std::cout << "Due to current only supporting single tables, the CREATE command cannot be used when opening the file." << std::endl;
                        continue;
                    }

                    std::unique_ptr<SQLParser::Statement> astRoot;
                    SQLParser::Tokenizer tokenizer{command};
                    SQLParser::Parser parser{tokenizer};
                    try
                    {
                        astRoot = parser.parseStatement();
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                        continue;
                    }
                    catch(...)
                    {
                        std::cerr << "Unknown exception during parse." << '\n';
                        continue;
                    }

                    // SQLParser::ASTPrinter printer{};
                    // astRoot->exec(&printer);
                    // std::cout << std::endl;

                    try
                    {
                        retval = astRoot->exec(&executor);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                        continue;
                    }
                    catch(...)
                    { 
                        std::cerr << "Unknown exception during exec." << '\n';
                        continue;
                    }

                    if (retval)
                    {
                        std::cout << "Fail, Error code: " << EDB::StatusCode2String(retval) << std::endl;
                    }
                }
            }
        }
    }
    
    return 0;
}
