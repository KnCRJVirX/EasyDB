#include <iostream>
#include <iomanip>

#include "EasyDB.hpp"

#ifdef _WIN32
#include <windows.h>
#define M_BUF_SIZ 65536
char gbk_buffer[M_BUF_SIZ];
char* utf8togbk(const char* utf8text, char* gbktext, size_t gbktext_size)
{
    wchar_t* utf16text = (wchar_t*)calloc((strlen(utf8text) + 1) * 2, sizeof(char));
    MultiByteToWideChar(CP_UTF8, 0, utf8text, -1, utf16text, (strlen(utf8text) + 1) * 2);
    WideCharToMultiByte(936, 0, utf16text, -1, gbktext, gbktext_size, NULL, NULL);
    free(utf16text);
    return gbktext;
}
#endif

#define INDEX_WID 6
#define CNAME_WID 15
#define DTYPE_WID 10
#define DSIZE_WID 6

int main(int argc, char const *argv[])
{
    #ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    #endif

    std::string path;
    
    if (argc < 2)
    {
        std::getline(std::cin, path);
        #ifdef _WIN32
        path = utf8togbk(path.data(), gbk_buffer, M_BUF_SIZ);
        #endif
    }
    else
    { path = argv[1]; }
    
    

    EDB::EasyDB edb{path};
    if (!edb.is_init()) return 0;

    std::cout << "MetaData: " << std::endl;
    std::cout << "Version: " << edb.get_version() << std::endl;
    std::cout << "Table name: " << edb.get_table_name() << std::endl;
    std::cout << "Row count: " << edb.get_row_count() << std::endl;
    
    auto colInfo = edb.get_column_info();
    std::cout << "Primary key: " << colInfo[edb.get_primary_key_index()].columnName << std::endl;
    std::cout << std::endl;
    std::cout << "Column info:" << std::endl;
    std::cout << std::left;
    std::cout << std::setw(INDEX_WID) << "Index" << " | " 
                << std::setw(CNAME_WID) << "Column name" << " | " 
                << std::setw(DTYPE_WID) << "Data type" << " | " 
                << std::setw(DSIZE_WID) << "Data size" << std::endl;
    size_t rowSize = 0;
    for (size_t i = 0; i < colInfo.size(); i++)
    {
        rowSize += colInfo[i].columnSize;
        std::cout << std::setw(INDEX_WID) << i << " | " 
            << std::setw(CNAME_WID) << colInfo[i].columnName.substr(0, CNAME_WID) << " | " 
            << std::setw(DTYPE_WID) << EDB::DataTypeEnum2String(colInfo[i].dType) << " | " 
            << std::setw(DSIZE_WID) << colInfo[i].columnSize << std::endl;
    }
    std::cout << "RowSize: " << rowSize << std::endl;
    
    return 0;
}
