#include <limits.h>
#include "easydb.h"
#define COMMAND_MAX_SIZE 50
#define COLNAME_MAX_SIZE 200
#define ID_MAX_SIZE 50
#define NAME_MAX_SIZE 100
#define PROJECT_NAME_MAX_SIZE 100
#define PROJECT_DETAIL_MAX_SIZE 1000

#define mfgets(buffer, max_count, file_stream) do{fgets((buffer), (max_count), (file_stream)); if(strchr((buffer), '\n')) *(strchr((buffer), '\n')) = 0;}while(0)

#ifdef __WIN32
#include <windows.h>
char* UTF8ToGBK(char *utf8_str) {
    // 第一步：将UTF-8字符串转换为宽字符（UTF-16）
    int wide_char_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    wchar_t *wide_char_str = (wchar_t *)malloc(wide_char_len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wide_char_str, wide_char_len);

    // 第二步：将宽字符（UTF-16）转换为GBK编码的字符串
    int gbk_len = WideCharToMultiByte(CP_ACP, 0, wide_char_str, -1, NULL, 0, NULL, NULL);
    char *gbk_str = (char *)malloc(gbk_len * sizeof(char));
    WideCharToMultiByte(CP_ACP, 0, wide_char_str, -1, gbk_str, gbk_len, NULL, NULL);

    // 回写GBK编码的字符串
    strcpy(utf8_str, gbk_str);

    // 释放内存
    free(wide_char_str);
    free(gbk_str);

    return utf8_str;
}
#endif

int Menu(char* commandList[], char* commandAbbrList[], char* discribe[], size_t commandCount)
{
    printf("\n");
    printf("注：可输入命令全称，也可输入命令缩写\n");
    printf("命令\t\t缩写\t描述\n");
    for (size_t i = 0; i < commandCount; i++)
    {
        printf("%-9s\t%-3s\t%s\n", commandList[i], commandAbbrList[i], discribe[i]);
    }
    printf("\n");
}

int parseCommand(char* command, char** cmdList, size_t commandCount)
{
    for (size_t i = 0; i < commandCount; i++)
    {
        if (!strcmp(command, cmdList[i]))
        {
            return i;
        }
    }
    return -1;
}

int projectMgr(EasyDB *dbPo, char* ID, char* name)
{
    int retval;
    void** searchResults[100];
    size_t resultCount;

    char* commandList[] = {"quit", "help", "show", "add", "delete", "edit", "searchname", "searchdetail", "projectdetail"};
    char* commandAbbrList[] = {"q", "h", "s", "a", "d", "e", "sn", "sd", "p"};
    char* discribe[] = {"退回上一级", "帮助（打印此菜单）", "查看所有记录", "添加记录", "删除记录", "编辑记录", "搜索项目名称（模糊搜索）", "搜索项目详情（模糊搜索）", "查看项目详情"};
    size_t commandCount = sizeof(commandList) / sizeof(commandList[0]);

    retval = edbWhere(dbPo, "学号", ID, searchResults, 100, &resultCount);
    if (retval != SUCCESS)
    {
        return -1;
    }
    Menu(commandList, commandAbbrList, discribe, commandCount);
    printf("序号\t学号\t\t姓名\t\t项目名称\n");
    for (size_t i = 0; i < resultCount; i++)
    {
        printf("%-2d\t%-9s\t%-15s\t%s\n", i + 1, Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
    }
    
    void** newRow = (void**)malloc(dbPo->columnCount * sizeof(void*));
    for (size_t i = 0; i < dbPo->columnCount; i++)
    {
        newRow[i] = malloc(dbPo->dataSizes[i]);
    }
    strcpy(Text(newRow[0]), ID);
    strcpy(Text(newRow[1]), name);
    int tmpID;

    char command[COMMAND_MAX_SIZE];
    int commandNum;
    while (1)
    {
        printf("%s素质类项目管理>", ID);
        mfgets(command, COMMAND_MAX_SIZE, stdin);
        commandNum = parseCommand(command, commandList, commandCount);
        if (commandNum < 0) commandNum = parseCommand(command, commandAbbrList, commandCount);
        switch (commandNum)
        {
        case 0:
            for (size_t i = 0; i < dbPo->columnCount; i++)
            {
                free(newRow[i]);
            }
            free(newRow);
            return 0;
            break;
        case 1:
            Menu(commandList, commandAbbrList, discribe, commandCount);
            break;
        case 2:
            retval = edbWhere(dbPo, "学号", ID, searchResults, 100, &resultCount);
            printf("序号\t学号\t\t姓名\t\t项目名称\n");
            for (size_t i = 0; i < resultCount; i++)
            {
                printf("%-2d\t%-9s\t%-15s\t%s\n", i + 1, Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
            }
            break;
        case 3:
            printf("项目名称：");
            mfgets(newRow[2], PROJECT_NAME_MAX_SIZE, stdin);
            printf("项目详情：");
            mfgets(newRow[3], PROJECT_DETAIL_MAX_SIZE, stdin);
            uuid(Text(newRow[4]));
            retval = edbInsert(dbPo, newRow);
            if (retval == SUCCESS)
            {
                printf("添加完成！\n");
            }
            else
            {
                printf("UUID重复！\n");
            }
            break;
        case 4:
            printf("序号：");
            scanf("%d", &tmpID);
            getchar();
            if (tmpID < 1 || tmpID > resultCount)
            {
                printf("序号超出范围！\n");
                break;
            }
            retval = edbDelete(dbPo, searchResults[tmpID - 1][columnNameToColumnIndex(dbPo, "UUID")]);
            if (retval == SUCCESS)
            {
                printf("删除成功！\n");
            }
            break;
        case 5:
            printf("输入1修改项目名称，输入2修改项目详情：");
            scanf("%d", &tmpID);
            switch (tmpID)
            {
            case 1:
                printf("序号：");
                scanf("%d", &tmpID);
                getchar();
                if (tmpID < 1 || tmpID > resultCount)
                {
                    printf("序号超出范围！\n");
                    break;
                }
                printf("新项目名称：");
                mfgets(newRow[2], PROJECT_NAME_MAX_SIZE, stdin);
                retval = edbUpdate(dbPo, searchResults[tmpID - 1][columnNameToColumnIndex(dbPo, "UUID")], "项目名称", newRow[2]);
                break;
            case 2:
                printf("序号：");
                scanf("%d", &tmpID);
                getchar();
                if (tmpID < 1 || tmpID > resultCount)
                {
                    printf("序号超出范围！\n");
                    break;
                }
                printf("新项目详情：");
                mfgets(newRow[3], PROJECT_DETAIL_MAX_SIZE, stdin);
                retval = edbUpdate(dbPo, searchResults[tmpID - 1][columnNameToColumnIndex(dbPo, "UUID")], "项目详情", newRow[2]);
                break;
            default:
                printf("请输入1或2！\n");
                retval = 1;
                break;
            }
            if (retval == SUCCESS)
            {
                printf("修改成功！\n");
            }
            break;
        case 6:
            printf("项目名称（关键词）：");
            mfgets(newRow[2], PROJECT_NAME_MAX_SIZE, stdin);
            retval = edbSearch(dbPo, "项目名称", newRow[2], searchResults, 100, &resultCount);
            if (resultCount > 0)
            {
                printf("序号\t学号\t\t姓名\t\t项目名称\n");
                for (size_t i = 0; i < resultCount; i++)
                {
                    printf("%-2d\t%-9s\t%-15s\t%s\n", i + 1, Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
                }
            }
            break;
        case 7:
            printf("项目详情（关键词）：");
            mfgets(newRow[3], PROJECT_DETAIL_MAX_SIZE, stdin);
            retval = edbSearch(dbPo, "项目详情", newRow[3], searchResults, 100, &resultCount);
            if (resultCount > 0)
            {
                printf("序号\t学号\t\t姓名\t\t项目名称\n");
                for (size_t i = 0; i < resultCount; i++)
                {
                    printf("%-2d\t%-9s\t%-15s\t%s\n", i + 1, Text(searchResults[i][0]), Text(searchResults[i][1]), Text(searchResults[i][2]));
                }
            }
            break;
        case 8:
            printf("序号：");
            scanf("%d", &tmpID);
            if (tmpID < 1 || tmpID > resultCount)
            {
                printf("序号超出范围！\n");
                break;
            }
            printf("序号\t学号\t\t姓名\t\t项目名称\n");
            printf("%-2d\t%-9s\t%-15s\t%s\n", tmpID, Text(searchResults[tmpID - 1][0]), Text(searchResults[tmpID - 1][1]), Text(searchResults[tmpID - 1][2]));
            printf("项目详情：%s\n", Text(searchResults[tmpID - 1][3]));
            break;
        default:
            break;
        }
    }
    
}

int processGradeTable(char* tableName)
{
    int retval;
    EasyDB db;
    EasyDB dbPo;
    char dbfilename[1024];
    char dbfilenamePo[1024];
    sprintf(dbfilename, "%s.db", tableName);
    sprintf(dbfilenamePo, "%s_QualityProject.db", tableName);

    char tmpColName[1024];
    char buf[1024];
    char tmpID[ID_MAX_SIZE];
    size_t colIndex;
    void** searchResults[100];
    size_t resultsCount = 0;

    #ifdef __WIN32
    UTF8ToGBK(dbfilename);
    UTF8ToGBK(dbfilenamePo);
    #endif

    retval = edbOpen(dbfilename, &db);
    if (retval != SUCCESS)
    {
        printf("文件打开失败！\n");
        return -1;
    }
    retval = edbOpen(dbfilenamePo, &dbPo);
    if (retval != SUCCESS)
    {
        edbClose(&db);
        printf("文件打开失败！\n");
        return -1;
    }

    char* commandList[] = {"quit", "help", "show", "add", "edit", "delete", "findID", "findname", "searchID", "searchname", "projectMgr"};
    char* commandAbbrList[] = {"q", "h", "s", "a", "e", "d", "fi", "fn", "si", "sn", "p"};
    char* discribe[] = {"退回上一级", "帮助（打印此菜单）", "展示所有成绩记录", "添加成绩记录", "编辑成绩记录", "删除成绩记录", "查找学号（完全匹配）", "查找姓名（完全匹配）", "搜索学号（模糊搜索）", "搜索姓名（模糊搜索）", "管理素质类项目"};
    size_t commandCount = sizeof(commandList) / sizeof(commandList[0]);
    Menu(commandList, commandAbbrList, discribe, commandCount);

    void** newRow = (void**)malloc(db.columnCount * sizeof(void*));
    for (size_t i = 0; i < db.columnCount; i++)
    {
        newRow[i] = malloc(db.dataSizes[i]);
    }

    char command[COMMAND_MAX_SIZE];
    int commandNum;
    while (1)
    {
        printf("%s>", tableName);
        mfgets(command, COMMAND_MAX_SIZE, stdin);
        commandNum = parseCommand(command, commandList, commandCount);
        if (commandNum == -1) commandNum = parseCommand(command, commandAbbrList, commandCount);
        switch (commandNum)
        {
        case 0:
            for (size_t i = 0; i < db.columnCount; i++)
            {
                free(newRow[i]);
            }
            free(newRow);
            edbClose(&db);
            edbClose(&dbPo);
            return 0;
            break;
        case 1:
            Menu(commandList, commandAbbrList, discribe, commandCount);
            break;
        case 2:
            void** it;
            printf("%-10s\t%-15s\t", "学号", "姓名");
            for (size_t i = 2; i < db.columnCount; i++)
            {
                printf("%-9s\t", db.columnNames[i]);
            }
            printf("%-15s\t%-15s\t素质类项目数量\n", "总分", "平均");
            EDB_ITER(db, it)
            {
                int projectCount = edbWhere(&dbPo, "学号", it[0], NULL, 0, NULL);
                double sum = 0;
                printf("%-10s\t%-15s\t", Text(it[0]), Text(it[1]));
                for (size_t i = 2; i < db.columnCount; i++)
                {
                    printf("%-9.2lf\t", Real(it[i]));
                    sum += Real(it[i]);
                }
                printf("%-9.2lf\t%-9.2lf\t%d\n", sum, sum / (db.columnCount - 2), projectCount);
            }
            break;
        case 3:
            printf("学号：");
            mfgets(Text(newRow[0]), ID_MAX_SIZE, stdin);
            printf("姓名：");
            mfgets(Text(newRow[1]), ID_MAX_SIZE, stdin);
            for (size_t i = 2; i < db.columnCount; i++)
            {
                printf("%s：", db.columnNames[i]);
                scanf("%lf", &Real(newRow[i]));
            }
            getchar();
            retval = edbInsert(&db, newRow);
            if (retval == SUCCESS)
            {
                printf("添加成功！\n");
            }
            else if (retval == PRIMARY_KEY_NOT_UNIQUE)
            {
                printf("学号必须唯一！\n");
            }
            break;
        case 4:
            printf("要修改的列：");
            mfgets(tmpColName, 1024, stdin);
            colIndex = columnNameToColumnIndex(&db, tmpColName);
            if (colIndex == COLUMN_NOT_FOUND)
            {
                printf("没有这一列！\n");
                break;
            }
            
            printf("学号：");
            mfgets(tmpID, ID_MAX_SIZE, stdin);

            printf("新数据：");
            mfgets(buf, 1024, stdin);

            switch (db.dataTypes[colIndex])
            {
            case EDB_TYPE_REAL:
                Real(newRow[colIndex]) = atof(buf);
                break;
            case EDB_TYPE_TEXT:
                strcpy(Text(newRow[colIndex]), buf);
                break;
            default:
                break;
            }
            
            retval = edbUpdate(&db, tmpID, tmpColName, newRow[colIndex]);

            if (retval == SUCCESS)
            {
                printf("修改成功！\n");
            }
            else if (retval == PRIMARY_KEY_NOT_UNIQUE)
            {
                printf("学号必须唯一！\n");
            }
            else if (retval == KEY_NOT_FOUND)
            {
                printf("没有找到学号对应的学生！\n");
            }
            break;
        case 5:
            printf("学号：");
            mfgets(tmpID, ID_MAX_SIZE, stdin);
            retval = edbDelete(&db, tmpID);
            if (retval == SUCCESS)
            {
                printf("删除成功！\n");
            }
            else if (retval == KEY_NOT_FOUND)
            {
                printf("没有找到学号对应的学生！\n");
            }
            break;
        case 6:
            printf("学号：");
            mfgets(tmpID, ID_MAX_SIZE, stdin);
            edbWhere(&db, "学号", tmpID, searchResults, 1, &resultsCount);
            if (resultsCount > 0)
            {
                printf("%-10s\t%-15s\t", "学号", "姓名");
                for (size_t i = 2; i < db.columnCount; i++)
                {
                    printf("%-9s\t", db.columnNames[i]);
                }
                printf("%-15s\t%-15s\t素质类项目数量\n", "总分", "平均");
                int projectCount = edbWhere(&dbPo, "学号", searchResults[0][0], NULL, 0, NULL);
                double sum = 0;
                printf("%-10s\t%-15s\t", Text(searchResults[0][0]), Text(searchResults[0][1]));
                for (size_t i = 2; i < db.columnCount; i++)
                {
                    printf("%-9.2lf\t", Real(searchResults[0][i]));
                    sum += Real(searchResults[0][i]);
                }
                printf("%-9.2lf\t%-9.2lf\t%d\n", sum, sum / (db.columnCount - 2), projectCount);
            }
            else
            {
                printf("没有找到学号对应的学生！\n");
            }
            break;
        case 7:
            printf("姓名：");
            mfgets(Text(newRow[1]), NAME_MAX_SIZE, stdin);
            edbWhere(&db, "姓名", newRow[1], searchResults, 100, &resultsCount);
            if (resultsCount > 0)
            {
                printf("%-10s\t%-15s\t", "学号", "姓名");
                for (size_t j = 2; j < db.columnCount; j++)
                {
                    printf("%-9s\t", db.columnNames[j]);
                }
                printf("%-15s\t%-15s\t素质类项目数量\n", "总分", "平均");
                for (size_t i = 0; i < resultsCount; i++)
                {
                    int projectCount = edbWhere(&dbPo, "学号", searchResults[i][0], NULL, 0, NULL);
                    double sum = 0;
                    printf("%-10s\t%-15s\t", Text(searchResults[i][0]), Text(searchResults[i][1]));
                    for (size_t j = 2; j < db.columnCount; j++)
                    {
                        printf("%-9.2lf\t", Real(searchResults[i][j]));
                        sum += Real(searchResults[i][j]);
                    }
                    printf("%-9.2lf\t%-9.2lf\t%d\n", sum, sum / (db.columnCount - 2), projectCount);
                }
            }
            else
            {
                printf("没有找到姓名对应的学生！\n");
            }
            break;
        case 8:
            printf("学号（关键词）：");
            mfgets(Text(newRow[0]), ID_MAX_SIZE, stdin);
            edbSearch(&db, "学号", newRow[0], searchResults, 100, &resultsCount);
            if (resultsCount > 0)
            {
                printf("%-10s\t%-15s\t", "学号", "姓名");
                for (size_t j = 2; j < db.columnCount; j++)
                {
                    printf("%-9s\t", db.columnNames[j]);
                }
                    printf("%-15s\t%-15s\t素质类项目数量\n", "总分", "平均");
                for (size_t i = 0; i < resultsCount; i++)
                {
                    int projectCount = edbWhere(&dbPo, "学号", searchResults[i][0], NULL, 0, NULL);
                    double sum = 0;
                    printf("%-10s\t%-15s\t", Text(searchResults[i][0]), Text(searchResults[i][1]));
                    for (size_t j = 2; j < db.columnCount; j++)
                    {
                        printf("%-9.2lf\t", Real(searchResults[i][j]));
                        sum += Real(searchResults[i][j]);
                    }
                    printf("%-9.2lf\t%-9.2lf\t%d\n", sum, sum / (db.columnCount - 2), projectCount);
                }
            }
            else
            {
                printf("没有找到学号对应的学生！\n");
            }
            break;
        case 9:
            printf("姓名（关键词）：");
            mfgets(Text(newRow[1]), NAME_MAX_SIZE, stdin);
            edbSearch(&db, "姓名", newRow[1], searchResults, 100, &resultsCount);
            if (resultsCount > 0)
            {
                printf("%-10s\t%-15s\t", "学号", "姓名");
                for (size_t j = 2; j < db.columnCount; j++)
                {
                    printf("%-9s\t", db.columnNames[j]);
                }
                printf("%-15s\t%-15s\t素质类项目数量\n", "总分", "平均");
                for (size_t i = 0; i < resultsCount; i++)
                {
                    int projectCount = edbWhere(&dbPo, "学号", searchResults[i][0], NULL, 0, NULL);
                    double sum = 0;
                    printf("%-10s\t%-15s\t", Text(searchResults[i][0]), Text(searchResults[i][1]));
                    for (size_t j = 2; j < db.columnCount; j++)
                    {
                        printf("%-9.2lf\t", Real(searchResults[i][j]));
                        sum += Real(searchResults[i][j]);
                    }
                    printf("%-9.2lf\t%-9.2lf\t%d\n", sum, sum / (db.columnCount - 2), projectCount);
                }
            }
            else
            {
                printf("没有找到姓名对应的学生！\n");
            }
            break;
        case 10:
            printf("学号：");
            mfgets(tmpID, ID_MAX_SIZE, stdin);
            retval = edbWhere(&db, "学号", tmpID, searchResults, 1, &resultsCount);
            if (resultsCount > 0)
            {
                projectMgr(&dbPo, tmpID, Text(searchResults[0][1]));
            }
            else
            {
                printf("没有找到学号对应的学生！\n");
            }
            break;
        default:
            break;
        }
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    #ifdef __WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif

    char* mainCommandList[] = {"quit", "help", "create", "open"};
    char* mainCommandAbbrList[] = {"q", "h", "c", "o"};
    char* mainDiscribe[] = {"退出系统", "帮助（打印此菜单）", "创建一个新表并打开", "打开一个表"};
    const size_t mainCommandCount = sizeof(mainCommandList) / sizeof(mainCommandList[0]);

    char command[COMMAND_MAX_SIZE];
    int commandNum = 0;
    char dbfilename[1024];
    char dbfilenamePo[1024];
    char tableName[1024];
    int retval;

    Menu(mainCommandList, mainCommandAbbrList, mainDiscribe, mainCommandCount);
    while (1)
    {
        printf("Easy Grade Management>");
        mfgets(command, COMMAND_MAX_SIZE, stdin);
        commandNum = parseCommand(command, mainCommandList, mainCommandCount);
        if (commandNum < 0) commandNum = parseCommand(command, mainCommandAbbrList, mainCommandCount);
        switch (commandNum)
        {
        case 0:
            goto EXIT;
            break;
        case 1:
            Menu(mainCommandList, mainCommandAbbrList, mainDiscribe, mainCommandCount);
            break;
        case 2:
            printf("请输入表名：");
            mfgets(tableName, 1024, stdin);
            
            int colCount = 0;
            printf("请输入科目数：");
            scanf("%d", &colCount);
            getchar();
            if (colCount <= 0)
            {
                printf("科目数必须大于0！\n");
                break;
            }
            colCount += 2;

            char** colNames = (char**)malloc(colCount * sizeof(char*));
            for (size_t i = 0; i < colCount; i++)
            {
                colNames[i] = (char*)malloc(COLNAME_MAX_SIZE);
            }
            strcpy(colNames[0], "学号");
            strcpy(colNames[1], "姓名");
            printf("逐个输入科目名称（输入每个科目名称后回车）\n");
            for (size_t i = 2; i < colCount; i++)
            {
                printf("请输入第%d个科目名：", i - 1);
                mfgets(colNames[i], COLNAME_MAX_SIZE, stdin);
            }

            size_t* dataTypes = (size_t*)malloc(colCount * sizeof(size_t));
            dataTypes[0] = EDB_TYPE_TEXT;
            dataTypes[1] = EDB_TYPE_TEXT;
            for (size_t i = 2; i < colCount; i++)
            {
                dataTypes[i] = EDB_TYPE_REAL;
            }
            
            size_t* dataSizes = (size_t*)calloc(colCount, sizeof(edb_int));
            dataSizes[0] = ID_MAX_SIZE;
            dataSizes[1] = NAME_MAX_SIZE;

            sprintf(dbfilename, "%s.db", tableName);
            #ifdef __WIN32
            UTF8ToGBK(dbfilename);
            #endif
            retval = edbCreate(dbfilename, tableName, colCount, "学号", dataTypes, dataSizes, colNames);
            if (retval == SUCCESS)
            {
                printf("成绩表创建成功！\n");
            }
            free(colNames); colNames = NULL;
            free(dataSizes); dataSizes = NULL;
            free(dataTypes); dataTypes = NULL;
            
            sprintf(dbfilenamePo, "%s_QualityProject.db", tableName);
            #ifdef _WIN32
            UTF8ToGBK(dbfilenamePo);
            #endif
            char* colNamesPo[] = {"学号", "姓名", "项目名称", "项目详情", "UUID"};
            size_t dataTypesPo[] = {EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_TEXT, EDB_TYPE_TEXT};
            size_t dataSizesPo[] = {ID_MAX_SIZE, NAME_MAX_SIZE, PROJECT_NAME_MAX_SIZE, PROJECT_DETAIL_MAX_SIZE, 40};
            retval = edbCreate(dbfilenamePo, tableName, 5, "UUID", dataTypesPo, dataSizesPo, colNamesPo);
            if (retval == SUCCESS)
            {
                printf("素质类项目成果管理表创建成功！\n");
            }
            processGradeTable(tableName);
            break;
        case 3:
            printf("请输入表名：");
            mfgets(tableName, 1024, stdin);
            processGradeTable(tableName);
        default:
            break;
        }
    }
    
    EXIT:
    return 0;
}