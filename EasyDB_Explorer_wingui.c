#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winnt.h>
#include <winuser.h>
#include <commdlg.h>
#include <commctrl.h>
#include <windowsx.h>
#include <uxtheme.h> 
#include "easydb.h"

const char *appName = "EasyDB Explorer";
const wchar_t* mainClassName = TEXT("MainWindow");
const wchar_t* createDBClassName = TEXT("CreateNewDBWindow");

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

#define OPEN_FILE_BUTTON 101    // 打开文件按钮
#define SAVE_FILE_BUTTON 102    // 保存文件按钮
#define ADDNEW_BUTTON 103       // 编辑按钮
#define EDIT_BUTTON 104         // 编辑按钮
#define DELETE_BUTTON 105       // 删除按钮
#define SEARCH_BUTTON 106       // 搜索按钮
#define NEW_DB_BUTTON 107       // 新建按钮
#define DB_LIST_VIEW 201        // 显示数据库内容的列表展示
#define SEARCH_BOX 301          // 搜索框
#define COLUMN_COMBOBOX 401     // 列选择组合框

#define M_BUF_SIZ 65536
char gbk_buffer[M_BUF_SIZ];
char utf8_buffer[M_BUF_SIZ];
wchar_t utf16_buffer[M_BUF_SIZ];
char dbfilename[1024];
HINSTANCE global_hInstance;
HWND hMainWindow;
HWND hMainListView;
HWND hSearchBox;
HWND hColumnNameComboBox;
EasyDB db;
bool isEdited;              // 文件是否被被编辑
HWND* editingEditBoxes;     // 存储正在编辑的编辑框
size_t editingItem;         // 正在编辑的行
HWND* addingEditBoxes;      // 存储正在添加的编辑框
size_t addingItem;          // 正在添加的行

wchar_t* utf8toutf16(const char* utf8text, wchar_t* utf16text, size_t utf16text_size)
{
    MultiByteToWideChar(CP_UTF8, 0, utf8text, -1, utf16text, utf16text_size);
    return utf16text;
}

char* utf16toutf8(const wchar_t* utf16text, char* utf8text, size_t utf8text_size)
{
    WideCharToMultiByte(CP_UTF8, 0, utf16text, -1, utf8text, utf8text_size, NULL, NULL);
    return utf8text;
}

char* utf8togbk(const char* utf8text, char* gbktext, size_t gbktext_size)
{
    wchar_t* utf16text = (wchar_t*)calloc((strlen(utf8text) + 1) * 2, sizeof(char));
    MultiByteToWideChar(CP_UTF8, 0, utf8text, -1, utf16text, (strlen(utf8text) + 1) * 2);
    WideCharToMultiByte(936, 0, utf16text, -1, gbktext, gbktext_size, NULL, NULL);
    free(utf16text);
    return gbktext;
}

void ProcessDBListView(HWND hListView)      // 将数据库文件读取到ListView
{
    if (db.dbfilename == NULL) return;

    LV_COLUMN lvCol;
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH;    // 需要指定显示的字符和宽度
    for (size_t i = 0; i < db.columnCount; i++)
    {
        lvCol.pszText = utf8toutf16(db.columnNames[i], utf16_buffer, M_BUF_SIZ);
        lvCol.cx = 150;
        ListView_InsertColumn(hListView, i, &lvCol);
        ComboBox_AddString(hColumnNameComboBox, lvCol.pszText); // 同时将列名添加到列选择框中
    }

    size_t cnt = 0;
    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT;                // 需要指定显示的字符
    for (void** it = edbIterBegin(&db); it != NULL; it = edbIterNext(&db))
    {
        lvItem.iItem = cnt++;
        for (size_t i = 0; i < db.columnCount; i++)
        {
            lvItem.iSubItem = i;
            wchar_t buf[256] = {0};
            switch (db.dataTypes[i])
            {
            case EDB_TYPE_INT:
                swprintf(buf, sizeof(buf), L"%lld", Int(it[i]));
                lvItem.pszText = buf;
                break;
            case EDB_TYPE_REAL:
                swprintf(buf, sizeof(buf), L"%lf", Real(it[i]));
                lvItem.pszText = buf;
                break;
            case EDB_TYPE_BLOB:
                lvItem.pszText = TEXT("<Blob data>");
                break;
            case EDB_TYPE_TEXT:
                lvItem.pszText = utf8toutf16(Text(it[i]), utf16_buffer, M_BUF_SIZ);
                break;
            default:
                break;
            }
            if (i == 0) ListView_InsertItem(hListView, &lvItem);
            else ListView_SetItem(hListView, &lvItem);
        }
    }
}

void ClearListViewAndComboBox(HWND hListView, size_t columnCount)  // 清除ListView
{
    ListView_DeleteAllItems(hListView);
    for (int i = 0; i < columnCount; i++)
    {
        ListView_DeleteColumn(hListView, 0); // 每次删除第一个列
        ComboBox_DeleteString(hColumnNameComboBox, 0);  // 每次删除列选择框中的第一个列
    }
}

void OpenDBFile(HWND hListView)
{
    if (db.dbfilename && isEdited)
    {
        int result = MessageBox(hMainListView, TEXT("是否保存文件并打开新文件？"), TEXT("打开新文件"), MB_YESNOCANCEL);
        if (result == IDYES)
        {
            ClearListViewAndComboBox(hMainListView, db.columnCount);
            edbClose(&db);
        }
        else if (result == IDNO)
        {
            ClearListViewAndComboBox(hMainListView, db.columnCount);
            edbCloseNotSave(&db);
        }
        else if (result == IDCANCEL)
        {
            return;
        }
    }
    else if (db.dbfilename && !isEdited)
    {
        ClearListViewAndComboBox(hMainListView, db.columnCount);
        edbCloseNotSave(&db);
    }

    // 重置编辑状态
    isEdited = false;

    // 重置新增按钮状态
    HWND hAddNewButton = GetDlgItem(hMainWindow, ADDNEW_BUTTON);
    SendMessageW(hAddNewButton, BM_SETCHECK, BST_UNCHECKED, 0);
    // 关闭新增的文本框
    if (addingEditBoxes)
    {
        for (size_t i = 0; i < db.columnCount; i++)
        {
            ShowWindow(addingEditBoxes[i], SW_HIDE);
            DestroyWindow(addingEditBoxes[i]);
        }
        free(addingEditBoxes); addingEditBoxes = NULL;
    }
    
    // 重置编辑按钮状态
    HWND hEditButton = GetDlgItem(hMainWindow, EDIT_BUTTON);
    SendMessageW(hEditButton, BM_SETCHECK, BST_UNCHECKED, 0);
    // 关闭编辑的文本框
    if (editingEditBoxes)
    {
        for (size_t i = 0; i < db.columnCount; i++)
        {
            ShowWindow(editingEditBoxes[i], SW_HIDE);
            DestroyWindow(editingEditBoxes[i]);
        }
        free(editingEditBoxes); editingEditBoxes = NULL;
    }

    int retval = edbOpen(utf8togbk(dbfilename, gbk_buffer, M_BUF_SIZ), &db);
    if (retval == SUCCESS)
    {
        // 设置窗口标题，显示打开的文件名
        char* newTitle = (char*)calloc(strlen(appName) + strlen(dbfilename) + 10, sizeof(char));
        strcpy(newTitle, appName);
        strcat(newTitle, "\t");
        strcat(newTitle, dbfilename);
        SetWindowTextW(hMainWindow, utf8toutf16(newTitle, utf16_buffer, M_BUF_SIZ));
        free(newTitle);
    }
    
    ProcessDBListView(hMainListView);
}

void DeleteButtonPressed(HWND hListView)    // 处理删除按钮
{
    size_t itemCount = ListView_GetItemCount(hListView); // 获取总行数

    wchar_t* priKeyBuf = (wchar_t*)calloc(db.dataSizes[db.primaryKeyIndex] * 4, sizeof(wchar_t));

    int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    while (selectedItem != -1)
    {
        ListView_GetItemText(hListView, selectedItem, db.primaryKeyIndex, priKeyBuf, db.dataSizes[db.primaryKeyIndex] * 4);
        switch (db.dataTypes[db.primaryKeyIndex])
        {
        case EDB_TYPE_INT:{
            edb_int priKeyInt;
            swscanf(priKeyBuf, L"%lld", &priKeyInt);
            edbDelete(&db, &priKeyInt);
            break;
        }
        case EDB_TYPE_REAL:{
            double priKeyReal;
            swscanf(priKeyBuf, L"%lf", &priKeyReal);
            edbDelete(&db, &priKeyReal);
            break;
        }
        case EDB_TYPE_TEXT:{
            edbDelete(&db, utf16toutf8(priKeyBuf, utf8_buffer, M_BUF_SIZ));
            break;
        }
        default:
            break;
        }
        ListView_DeleteItem(hListView, selectedItem);
        selectedItem = ListView_GetNextItem(hListView, selectedItem - 1, LVNI_SELECTED);
    }
    free(priKeyBuf);
}

void AddNewStart_SetEdit(HWND hListView)  // 开始添加
{
    // 新建一行，并初始化为空
    size_t itemCount = ListView_GetItemCount(hListView);
    LVITEM lvItem;
    lvItem.iItem = itemCount;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_TEXT;
    wchar_t* empty = TEXT("");
    lvItem.pszText = empty;
    ListView_InsertItem(hListView, &lvItem);
    for (size_t i = 0; i < db.columnCount; i++)
    {
        lvItem.iSubItem = i;
        ListView_SetItem(hListView, &lvItem);
    }
    
    size_t selectedItem = itemCount;
    addingItem = selectedItem;

    RECT rect;  // 存放每个项的位置区域信息，用于创建文本框
    addingEditBoxes = (HWND*)malloc(db.columnCount * sizeof(HWND));   // 分配空间用于存储文本框句柄

    // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
    HFONT hFont = (HFONT)SendMessageW(hMainListView, WM_GETFONT, 0, 0);
    if (hFont == NULL) {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    for (size_t i = 0; i < db.columnCount; i++)
    {
        if (i == 0) // 由于兼容性考虑，获取第一个子项的矩形区域时需要使用LVIR_LABEL
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_LABEL, &rect);    // 获取第一个子项的矩形区域
        }
        else
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_BOUNDS, &rect);    // 获取子项的矩形区域
        }
        // MapWindowPoints(hListView, GetParent(hListView), (LPPOINT)&rect, 2);        // 将获取到的坐标映射为父窗口的坐标系
        addingEditBoxes[i] = CreateWindowExW(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                TEXT(""),
                                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                                rect.left,
                                                rect.top,
                                                rect.right - rect.left,
                                                rect.bottom - rect.top,
                                                hListView,
                                                NULL,
                                                GetModuleHandle(NULL),
                                                NULL);
        
        // 设置字体
        SendMessageW(addingEditBoxes[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        // 设置编辑框的 Z 顺序 确保编辑框位于 ListView 的上方，避免被 ListView 覆盖。
        // SetWindowPos(addingEditBoxes[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void AddNewOver_ReadEdit(HWND hListView)  // 结束添加
{
    // 初始化新行
    void** newRow = (void**)malloc(db.columnCount * sizeof(void*));
    for (size_t i = 0; i < db.columnCount; i++)
    {
        newRow[i] = (void*)calloc(db.dataSizes[i], 1);
    }
    
    for (size_t i = 0; i < db.columnCount; i++) // 逐列处理
    {
        wchar_t* readBuf = (wchar_t*)calloc(db.dataSizes[i] * 2, sizeof(wchar_t));
        edb_int readInt;
        edb_real readReal;
        GetWindowTextW(addingEditBoxes[i], readBuf, db.dataSizes[i] * 4); // 读取编辑框的内容
        switch (db.dataTypes[i])
        {
        case EDB_TYPE_INT:
            swscanf(readBuf, L"%lld", newRow[i]);
            break;
        case EDB_TYPE_REAL:
            swscanf(readBuf, L"%lf", newRow[i]);
            break;
        case EDB_TYPE_TEXT:
            memcpy(newRow[i], utf16toutf8(readBuf, utf8_buffer, M_BUF_SIZ), db.dataSizes[i]);
            break;
        default:
            break;
        }
        ListView_SetItemText(hListView, addingItem, i, readBuf);   // 更新ListView中的内容
        ShowWindow(addingEditBoxes[i], SW_HIDE); // 隐藏窗口
        DestroyWindow(addingEditBoxes[i]);       // 销毁窗口
    }
    int retval = edbInsert(&db, newRow); // 插入新行

    if (retval != SUCCESS)  // 如果插入不成功就删除掉ListView中的新行
    {
        ListView_DeleteItem(hListView, addingItem);
    }
    

    // 释放新行
    for (size_t i = 0; i < db.columnCount; i++)
    {
        free(newRow[i]);
    }
    free(newRow);
    addingItem = -1;
    free(addingEditBoxes); addingEditBoxes = NULL;
}

void EditStart_SetEdit(HWND hListView)  // 开始编辑
{
    // 获取选中的行
    int selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    editingItem = selectedItem;
    if (selectedItem == -1)         //若没有选中，重置按钮状态并退出
    {
        HWND hAddNewButton = GetDlgItem(hMainWindow, EDIT_BUTTON);
        SendMessageW(hAddNewButton, BM_SETCHECK, BST_UNCHECKED, 0);
        return;
    }
    

    RECT rect;  // 存放每个项的位置区域信息，用于创建文本框
    editingEditBoxes = (HWND*)malloc(db.columnCount * sizeof(HWND)); // 分配空间用于存储文本框句柄

    // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
    HFONT hFont = (HFONT)SendMessageW(hMainListView, WM_GETFONT, 0, 0);
    if (hFont == NULL) {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    for (size_t i = 0; i < db.columnCount; i++)
    {
        wchar_t* pszTextBuf = (wchar_t*)calloc(db.dataSizes[db.primaryKeyIndex] * 8, sizeof(wchar_t));
        if (i == 0) // 由于兼容性考虑，获取第一个子项的矩形区域时需要使用LVIR_LABEL
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_LABEL, &rect);    // 获取第一个子项的矩形区域
        }
        else
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_BOUNDS, &rect);    // 获取子项的矩形区域
        }
        // MapWindowPoints(hListView, GetParent(hListView), (LPPOINT)&rect, 2);        // 将获取到的坐标映射为父窗口的坐标系
        editingEditBoxes[i] = CreateWindowExW(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                TEXT(""),
                                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                                rect.left,
                                                rect.top,
                                                rect.right - rect.left,
                                                rect.bottom - rect.top,
                                                hListView,
                                                NULL,
                                                GetModuleHandle(NULL),
                                                NULL);
        
        // 设置字体
        SendMessageW(editingEditBoxes[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        // 获取当前子项内容
        ListView_GetItemText(hListView, selectedItem, i, pszTextBuf, db.dataSizes[db.primaryKeyIndex] * 8);
        // 发送到编辑框
        SetWindowTextW(editingEditBoxes[i], pszTextBuf);
        // 设置编辑框的 Z 顺序 确保编辑框位于 ListView 的上方，避免被 ListView 覆盖。
        // SetWindowPos(editingEditWindows[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        free(pszTextBuf);
    }
}

void EditOver_ReadEdit(HWND hListView)  // 结束编辑
{
    // 获取主键
    char* priKeyBuf;
    size_t pKBufSiz = 0;
    switch (db.dataTypes[db.primaryKeyIndex])
    {
    case EDB_TYPE_INT:
        priKeyBuf = (char*)malloc(32);
        pKBufSiz = 32;
        break;
    case EDB_TYPE_REAL:
        priKeyBuf = (char*)malloc(32);
        pKBufSiz = 32;
        break;
    case EDB_TYPE_TEXT:
        priKeyBuf = (char*)malloc(db.dataSizes[db.primaryKeyIndex] * 4);
        pKBufSiz = db.dataSizes[db.primaryKeyIndex] * 4;
        break;
    default:
        break;
    }
    ListView_GetItemText(hListView, editingItem, db.primaryKeyIndex, (wchar_t*)priKeyBuf, pKBufSiz);
    edb_int priKeyInt;
    edb_real priKeyReal;
    switch (db.dataTypes[db.primaryKeyIndex])
    {
    case EDB_TYPE_INT:
        swscanf((wchar_t*)priKeyBuf, L"%lld", &priKeyInt);
        memcpy(priKeyBuf, &priKeyInt, db.dataSizes[db.primaryKeyIndex]);
        break;
    case EDB_TYPE_REAL:
        swscanf((wchar_t*)priKeyBuf, L"%lf", &priKeyReal);
        memcpy(priKeyBuf, &priKeyReal, db.dataSizes[db.primaryKeyIndex]);
        break;
    case EDB_TYPE_TEXT:
        memcpy(priKeyBuf, utf16toutf8((wchar_t*)priKeyBuf, utf8_buffer, M_BUF_SIZ), db.dataSizes[db.primaryKeyIndex]);
        break;
    default:
        break;
    }

    for (size_t i = 0; i < db.columnCount; i++) // 逐列处理
    {
        if (i == db.primaryKeyIndex)    // 跳过主键
        {
            continue;
        }
        wchar_t* readBuf = (wchar_t*)calloc(db.dataSizes[i] * 4, sizeof(wchar_t));
        edb_int readInt;
        edb_real readReal;
        GetWindowTextW(editingEditBoxes[i], readBuf, db.dataSizes[i] * 4); // 读取编辑框的内容
        switch (db.dataTypes[i])
        {
        case EDB_TYPE_INT:
            swscanf(readBuf, L"%lld", &readInt);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readInt);
            break;
        case EDB_TYPE_REAL:
            swscanf(readBuf, L"%lf", &readReal);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readReal);
            break;
        case EDB_TYPE_TEXT:
            edbUpdate(&db, priKeyBuf, db.columnNames[i], utf16toutf8(readBuf, utf8_buffer, M_BUF_SIZ));
            break;
        default:
            break;
        }
        ListView_SetItemText(hListView, editingItem, i, readBuf);   // 更新ListView中的内容
        ShowWindow(editingEditBoxes[i], SW_HIDE); // 隐藏窗口
        DestroyWindow(editingEditBoxes[i]);       // 销毁窗口
    }
    // 专门处理主键
    if (editingEditBoxes)
    {
        size_t i = db.primaryKeyIndex;
        wchar_t* readBuf = (wchar_t*)calloc(db.dataSizes[i] * 4, sizeof(wchar_t));
        edb_int readInt;
        edb_real readReal;
        GetWindowTextW(editingEditBoxes[i], readBuf, db.dataSizes[i] * 4); // 读取编辑框的内容
        switch (db.dataTypes[i])
        {
        case EDB_TYPE_INT:
            swscanf(readBuf, L"%lld", &readInt);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readInt);
            break;
        case EDB_TYPE_REAL:
            swscanf(readBuf, L"%lf", &readReal);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readReal);
            break;
        case EDB_TYPE_TEXT:
            edbUpdate(&db, priKeyBuf, db.columnNames[i], utf16toutf8(readBuf, utf8_buffer, M_BUF_SIZ));
            break;
        default:
            break;
        }
        ListView_SetItemText(hListView, editingItem, i, readBuf);   // 更新ListView中的内容
        ShowWindow(editingEditBoxes[i], SW_HIDE); // 隐藏窗口
        DestroyWindow(editingEditBoxes[i]);       // 销毁窗口
        free(readBuf);
    }
    editingItem = -1;
    free(editingEditBoxes); editingEditBoxes = NULL;
    free(priKeyBuf);
}

void SearchButtonPressed(HWND hListView, HWND hInputBox)
{
    int colIndex = ComboBox_GetCurSel(hColumnNameComboBox);     // 获取选中的列
    if (colIndex == CB_ERR || colIndex >= db.columnCount)
    {
        return;
    }

    wchar_t* keyWordBuf = (wchar_t*)calloc(db.dataSizes[colIndex], sizeof(wchar_t));
    int retval = Edit_GetText(hInputBox, keyWordBuf, db.dataSizes[colIndex]);
    if (retval == 0)    // 如果搜索框输入为空，则显示所有内容
    {
        ClearListViewAndComboBox(hListView, db.columnCount);
        ProcessDBListView(hListView);
        return;
    }
    
    void** searchResult[1024];
    size_t resultsCount;
    retval = edbSearch(&db, db.columnNames[colIndex], utf16toutf8(keyWordBuf, utf8_buffer, M_BUF_SIZ), searchResult, 1024, &resultsCount);
    if (retval == SUCCESS)
    {
        ListView_DeleteAllItems(hListView);
        LVITEM lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        for (size_t i = 0; i < resultsCount; i++)
        {
            lvItem.iItem = i;
            for (size_t j = 0; j < db.columnCount; j++)
            {
                lvItem.iSubItem = j;
                wchar_t buf[256] = {0};
                switch (db.dataTypes[j])
                {
                case EDB_TYPE_INT:
                    swprintf(buf, 256, L"%lld", Int(searchResult[i][j]));
                    lvItem.pszText = buf;
                    break;
                case EDB_TYPE_REAL:
                    swprintf(buf, 256, L"%lf", Real(searchResult[i][j]));
                    lvItem.pszText = buf;
                    break;
                case EDB_TYPE_BLOB:
                    lvItem.pszText = TEXT("<Blob data>");
                    break;
                case EDB_TYPE_TEXT:
                    lvItem.pszText = utf8toutf16(Text(searchResult[i][j]), utf16_buffer, M_BUF_SIZ);
                    break;
                default:
                    break;
                }
                if (j == 0)
                {
                    ListView_InsertItem(hListView, &lvItem);
                }
                else
                {
                    ListView_SetItemText(hListView, i, j, lvItem.pszText);
                }
            }
        }
    }
    return;
}

#define CREATE_COMPLETE_BUTTON 1001     // 完成按钮
#define CREATE_CANCEL_BUTTON 1002       // 取消按钮
#define ADD_COL_BUTTON 1003             // 新增一列按钮
#define GET_PRIMARY_KEY_BUTTON 1004     // 获取主键按钮
#define DELETE_COL_BUTTON 1005          // 删除此列按钮
#define COLUMN_LISTVIEW 2001            // 列的列表视图
HWND hColumnNameEditBox;
HWND hColumnTypeComboBox;
HWND hColumnSizeEditBox;
HWND hColumnListView;
HWND hPrimaryKeyEditBox;
void SaveFileDialogAndProcess(HWND hwnd)
{
    OPENFILENAME ofn;          // 另存为对话框的结构体
    wchar_t szFile[260];       // 用于存储选择的文件路径

    // 初始化 OPENFILENAME 结构体
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrDefExt = TEXT("db");
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = TEXT("EasyDB 文件\0*.db\0""所有文件\0*.*\0");
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = TEXT("另存为EasyDB文件");
    ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

    // 显示另存为对话框
    if (GetSaveFileName(&ofn) == TRUE) {
        strcpy(dbfilename, utf16toutf8(szFile, utf8_buffer, M_BUF_SIZ));    // 将文件名写入全局变量

        // 新建数据库
        int colCount = ListView_GetItemCount(hColumnListView);
        if (colCount == 0)
        {
            return;
        }

        // 分配内存
        char** colNames = (char**)malloc(colCount * sizeof(char*));
        for (int i = 0; i < colCount; i++)
        {
            colNames[i] = (char*)calloc(4096, sizeof(char));
        }
        size_t* colSizes = (size_t*)calloc(colCount, sizeof(size_t));
        size_t* colTypes = (size_t*)calloc(colCount, sizeof(size_t));
        wchar_t buf[4096];

        // 逐行读取为数据库列
        for (int i = 0; i < colCount; i++)
        {
            // 列名
            ListView_GetItemText(hColumnListView, i, 0, buf, 4096);
            strcpy(colNames[i], utf16toutf8(buf, utf8_buffer, M_BUF_SIZ));

            // 列数据类型
            ListView_GetItemText(hColumnListView, i, 1, buf, 4096);
            utf16toutf8(buf, utf8_buffer, M_BUF_SIZ);
            if (!strcmp(utf8_buffer, "整数")) colTypes[i] = EDB_TYPE_INT;
            else if (!strcmp(utf8_buffer, "小数")) colTypes[i] = EDB_TYPE_REAL;
            else if (!strcmp(utf8_buffer, "文本")) colTypes[i] = EDB_TYPE_TEXT;

            // 列数据长度（仅文本类型时读取）
            if (colTypes[i] == EDB_TYPE_TEXT)
            {
                ListView_GetItemText(hColumnListView, i, 2, buf, 4096);
                swscanf(buf, L"%lld", &colSizes[i]);
            }
        }

        char tableName[512] = {0};
        strcpy(tableName, utf16toutf8(&szFile[ofn.nFileOffset], utf8_buffer, M_BUF_SIZ));
        tableName[511] = 0;
        if (strrchr(tableName, '.')) *(strrchr(tableName, '.')) = 0;

        Edit_GetText(hPrimaryKeyEditBox, buf, 4096);
        int retval = edbCreate(utf8togbk(dbfilename, gbk_buffer, M_BUF_SIZ), tableName, colCount, utf16toutf8(buf, utf8_buffer, M_BUF_SIZ), colTypes, colSizes, colNames);

        free(colTypes);
        free(colSizes);
        for (int i = 0; i < colCount; i++)
        {
            free(colNames[i]);
        }
        free(colNames);
        
        if (retval == SUCCESS)
        {
            OpenDBFile(hMainListView);
            DestroyWindow(hwnd);
        }
        else
        {
            MessageBoxW(hwnd, TEXT("创建数据库失败！"), NULL, MB_OK);
        }
    }
}
LRESULT CALLBACK CreateDBWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:{
            // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
            HFONT hFont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
            if (hFont == NULL) {
                hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            HWND hButton;
            // 列名编辑框
            hColumnNameEditBox = CreateWindowExW(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                TEXT(""),
                                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                                110, 55, 100, 20,
                                                hwnd,
                                                NULL,
                                                GetModuleHandle(NULL),
                                                NULL);
            SendMessageW(hColumnNameEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 列类型组合框
            hColumnTypeComboBox = CreateWindowExW(0, WC_COMBOBOX,
                                                NULL,
                                                CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                                                110, 95, 100, 100,
                                                hwnd, 
                                                NULL, 
                                                GetModuleHandle(NULL), 
                                                NULL);
            SendMessageW(hColumnTypeComboBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            ComboBox_AddString(hColumnTypeComboBox, TEXT("整数"));
            ComboBox_AddString(hColumnTypeComboBox, TEXT("小数"));
            ComboBox_AddString(hColumnTypeComboBox, TEXT("文本"));
            // 列数据长度编辑框
            hColumnSizeEditBox = CreateWindowExW(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                TEXT(""),
                                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
                                                110, 145, 100, 20,
                                                hwnd,
                                                NULL,
                                                GetModuleHandle(NULL),
                                                NULL);
            SendMessageW(hColumnSizeEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “新增一列”按钮
            hButton = CreateWindowExW(0,TEXT("BUTTON"), TEXT("新增一列"),
                                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_CENTER,
                                        110, 180, 100, 25, 
                                        hwnd, (HMENU)ADD_COL_BUTTON, 
                                        GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “删除此列”按钮
            hButton = CreateWindowExW(0,TEXT("BUTTON"), TEXT("删除此列"),
                                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_CENTER,
                                        110, 210, 100, 25, 
                                        hwnd, (HMENU)DELETE_COL_BUTTON, 
                                        GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 主键编辑框
            hPrimaryKeyEditBox = CreateWindowExW(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                TEXT(""),
                                                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
                                                110, 245, 100, 20,
                                                hwnd,
                                                NULL,
                                                GetModuleHandle(NULL),
                                                NULL);
            SendMessageW(hPrimaryKeyEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “获取主键(<)”按钮
            hButton = CreateWindowExW(0,TEXT("BUTTON"), TEXT("<"),
                                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_CENTER,
                                        220, 243, 20, 20, 
                                        hwnd, (HMENU)GET_PRIMARY_KEY_BUTTON, 
                                        GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “完成”按钮
            hButton = CreateWindowExW(0,TEXT("BUTTON"), TEXT("完成"),
                                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_CENTER,
                                        320, 320, 80, 25, 
                                        hwnd, (HMENU)CREATE_COMPLETE_BUTTON, 
                                        GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “取消”按钮
            hButton = CreateWindowExW(0,TEXT("BUTTON"), TEXT("取消"),
                                        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_CENTER,
                                        410, 320, 80, 25, 
                                        hwnd, (HMENU)CREATE_CANCEL_BUTTON, 
                                        GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 列的列表视图
            hColumnListView = CreateWindowExW(WS_EX_CLIENTEDGE,    // 扩展窗口样式，带边框
                                            WC_LISTVIEW, TEXT(""),
                                            WS_CHILD | WS_VISIBLE | LVS_REPORT,
                                            260, 20, 300, 280,
                                            hwnd, (HMENU)COLUMN_LISTVIEW,
                                            GetModuleHandle(NULL), NULL);
            ListView_SetExtendedListViewStyle(hColumnListView, LVS_EX_FULLROWSELECT);
            LVCOLUMN lvCol;
            lvCol.mask = LVCF_TEXT | LVCF_WIDTH;    // 需要指定显示的字符和宽度
            lvCol.pszText = TEXT("列名");
            lvCol.cx = 170;
            ListView_InsertColumn(hColumnListView, 0, &lvCol);
            lvCol.pszText = TEXT("类型");
            lvCol.cx = 60;
            ListView_InsertColumn(hColumnListView, 1, &lvCol);
            lvCol.pszText = TEXT("长度");
            lvCol.cx = 60;
            ListView_InsertColumn(hColumnListView, 2, &lvCol);
            break;
    }
    case WM_COMMAND:{
        if (LOWORD(wParam) == ADD_COL_BUTTON)            // “新增一列”按钮被按下
        {
            wchar_t buf[4096] = {0};
            LVITEM lvItem = {0};
            int retval = Edit_GetText(hColumnNameEditBox, buf, 4096);
            if (retval == 0)
            {
                break;
            }
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = ListView_GetItemCount(hColumnListView);
            lvItem.iSubItem = 0;
            lvItem.pszText = buf;
            ListView_InsertItem(hColumnListView, &lvItem);
            ComboBox_GetText(hColumnTypeComboBox, buf, 4096);
            ListView_SetItemText(hColumnListView, lvItem.iItem, 1, buf);
            if (!strcmp(utf16toutf8(buf, utf8_buffer, M_BUF_SIZ), "文本"))
            {
                Edit_GetText(hColumnSizeEditBox, buf, 4096);
                ListView_SetItemText(hColumnListView, lvItem.iItem, 2, buf);
            }
            else
            {
                ListView_SetItemText(hColumnListView, lvItem.iItem, 2, TEXT(""));
            }
        }
        else if (LOWORD(wParam) == GET_PRIMARY_KEY_BUTTON)    // “获取主键(<)”按钮被按下
        {
            int selectedItem = ListView_GetNextItem(hColumnListView, -1, LVNI_SELECTED);
            if (selectedItem == -1)
            {
                break;
            }
            wchar_t buf[4096];
            ListView_GetItemText(hColumnListView, selectedItem, 0, buf, 4096);
            Edit_SetText(hPrimaryKeyEditBox, buf);
        }
        else if (LOWORD(wParam) == DELETE_COL_BUTTON)         // “删除此列”按钮被按下
        {
            int selectedItem = ListView_GetNextItem(hColumnListView, -1, LVNI_SELECTED);
            while (selectedItem != -1)
            {
                ListView_DeleteItem(hColumnListView, selectedItem);
                selectedItem = ListView_GetNextItem(hColumnListView, selectedItem - 1, LVNI_SELECTED);
            }
        }
        else if (LOWORD(wParam) == CREATE_CANCEL_BUTTON)      // “取消”按钮被按下
        {
            ShowWindow(hwnd, SW_HIDE);
            DestroyWindow(hwnd);
        }
        else if (LOWORD(wParam) == CREATE_COMPLETE_BUTTON)  // “完成”按钮被按下
        {
            SaveFileDialogAndProcess(hwnd);
        }
        break;
    }
    case WM_PAINT:{
            // 获取设备上下文
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // 填充背景颜色
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            // 获取主窗口的字体
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);  // 获取默认的GUI字体
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);       // 选择该字体到设备上下文
            // 在窗口上绘制文字
            RECT rect = {10, 100, 100, 30};
            DrawTextW(hdc, TEXT("列名"), -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            rect.top = 180;
            DrawTextW(hdc, TEXT("列数据类型"), -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            rect.top = 260;
            DrawTextW(hdc, TEXT("列数据长度"), -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            rect.top = 290;
            DrawTextW(hdc, TEXT("（仅文本时有效）"), -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            rect.top = 480;
            DrawTextW(hdc, TEXT("主键"), -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            // 结束绘制
            EndPaint(hwnd, &ps);
        break;
    }
    case WM_SIZE:           // 处理用户改变窗口大小
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            InvalidateRect(hwnd, NULL, TRUE);  // 强制重绘窗口
            break;
        }
    case WM_DESTROY:
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void CreateNewDB()
{   
    // 创建窗口
    HWND hCreateDBWindow = CreateWindowExW(WS_EX_CLIENTEDGE,
                                            createDBClassName, 
                                            TEXT("新建数据库"), 
                                            WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX,  // 禁止大小调整和最大化
                                            CW_USEDEFAULT, CW_USEDEFAULT, 
                                            600, 400, 
                                            NULL, NULL, 
                                            global_hInstance,
                                            NULL);
    
    // 显示窗口
    ShowWindow(hCreateDBWindow, SW_SHOW);
    UpdateWindow(hCreateDBWindow);
}

void OpenFileDialogAndProcess(HWND hwnd) // 选择文件对话框
{
    OPENFILENAME ofn;          // 文件选择对话框的结构体
    wchar_t szFile[260];       // 用于存储选择的文件路径

    // 初始化 OPENFILENAME 结构体
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = TEXT("EasyDB 文件\0*.db\0""所有文件\0*.*\0");
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = TEXT("打开EasyDB文件");
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // 显示文件选择对话框
    if (GetOpenFileName(&ofn) == TRUE) {
        // 在这里处理选中的文件，例如打开、读取内容等
        strcpy(dbfilename, utf16toutf8(szFile, utf8_buffer, M_BUF_SIZ));
        OpenDBFile(hMainListView);
    }
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:{
            // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
            HFONT hFont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
            if (hFont == NULL) {
                hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            HWND hButton;
            // “新建”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("新建"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 10, 60, 25, 
                hwnd, (HMENU)NEW_DB_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “打开”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("打开"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                80, 10, 60, 25, 
                hwnd, (HMENU)OPEN_FILE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “保存”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("保存"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                150, 10, 60, 25, 
                hwnd, (HMENU)SAVE_FILE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “新增”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("新增"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                220, 10, 60, 25, 
                hwnd, (HMENU)ADDNEW_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “编辑”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("编辑"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                290, 10, 60, 25, 
                hwnd, (HMENU)EDIT_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “删除”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("删除"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                360, 10, 60, 25, 
                hwnd, (HMENU)DELETE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 列选择组合框
            hColumnNameComboBox = CreateWindowExW(0, WC_COMBOBOX,
                                                NULL,
                                                CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                                                430, 10, 100, 150,
                                                hwnd, (HMENU)COLUMN_COMBOBOX, 
                                                GetModuleHandle(NULL), NULL);
            SendMessageW(hColumnNameComboBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 搜索框
            hSearchBox = CreateWindowExW(WS_EX_CLIENTEDGE, 
                                            TEXT("EDIT"),
                                            NULL,
                                            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                            550, 10, 300, 25,
                                            hwnd, (HMENU)SEARCH_BOX,
                                            GetModuleHandle(NULL), NULL);
            SendMessageW(hSearchBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “搜索”按钮
            hButton = CreateWindowExW(0, TEXT("BUTTON"), TEXT("搜索"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                870, 10, 60, 25, 
                hwnd, (HMENU)SEARCH_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 预览列表
            hMainListView = CreateWindowExW(WS_EX_CLIENTEDGE,    // 扩展窗口样式，带边框
                    WC_LISTVIEW, TEXT(""),
                    WS_CHILD | WS_VISIBLE | LVS_REPORT,
                    10, 50, DEFAULT_WIDTH - 30, DEFAULT_HEIGHT - 60,
                    hwnd, (HMENU)DB_LIST_VIEW,
                    GetModuleHandle(NULL), NULL);
            // SendMessageW(hMainListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);   // 整行选中
            // SendMessageW(hMainListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);         // 复选框
            ListView_SetExtendedListViewStyle(hMainListView, LVS_EX_FULLROWSELECT);
            break;
        }
        case WM_COMMAND:{
            if (LOWORD(wParam) == NEW_DB_BUTTON)             // “新建”按钮被按下
            {
                CreateNewDB();
            }
            else if (LOWORD(wParam) == OPEN_FILE_BUTTON)     // “打开”按钮被按下
            {
                OpenFileDialogAndProcess(hwnd);
            }
            else if (LOWORD(wParam) == SAVE_FILE_BUTTON)     // “保存”按钮被按下
            {
                if (db.dbfilename)
                {
                    isEdited = false;
                    edbSave(&db);
                }
            }
            else if (LOWORD(wParam) == ADDNEW_BUTTON)        // “新增”按钮被按下
            {
                HWND hButton = (HWND)lParam;
                if (db.dbfilename && SendMessageW(hButton, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                {
                    isEdited = true;
                    SendMessageW(hButton, BM_SETCHECK, BST_CHECKED, 0);
                    AddNewStart_SetEdit(hMainListView);
                }
                else if (db.dbfilename && SendMessageW(hButton, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    isEdited = true;
                    SendMessageW(hButton, BM_SETCHECK, BST_UNCHECKED, 0);
                    AddNewOver_ReadEdit(hMainListView);
                }
            }
            else if (LOWORD(wParam) == EDIT_BUTTON)          // “编辑”按钮被按下
            {
                HWND hButton = (HWND)lParam;
                if (db.dbfilename && SendMessageW(hButton, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                {
                    isEdited = true;
                    SendMessageW(hButton, BM_SETCHECK, BST_CHECKED, 0);
                    EditStart_SetEdit(hMainListView);
                }
                else if (db.dbfilename && SendMessageW(hButton, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    isEdited = true;
                    SendMessageW(hButton, BM_SETCHECK, BST_UNCHECKED, 0);
                    EditOver_ReadEdit(hMainListView);
                }
            }
            else if (LOWORD(wParam) == DELETE_BUTTON)        // “删除”按钮被按下
            {
                if (db.dbfilename)
                {
                    isEdited = true;
                    DeleteButtonPressed(hMainListView);
                }
            }
            else if (LOWORD(wParam) == SEARCH_BUTTON)
            {
                if (db.dbfilename)
                {
                    SearchButtonPressed(hMainListView, hSearchBox);
                }
            }
            
            break;
        }
        case WM_DROPFILES:{     // 处理拖入文件
            HDROP hDrop = (HDROP)wParam;

            wchar_t filePath[MAX_PATH];
            DragQueryFile(hDrop, 0, filePath, MAX_PATH);
            strcpy(dbfilename, utf16toutf8(filePath, utf8_buffer, M_BUF_SIZ));
            OpenDBFile(hMainListView);

            // 释放拖拽句柄
            DragFinish(hDrop);
            break;
        }
        case WM_SIZE:           // 处理用户改变窗口大小
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            MoveWindow(hMainListView, 10, 40, newWidth - 30, newHeight - 60, TRUE);     // 修改ListView的大小
            InvalidateRect(hwnd, NULL, TRUE);  // 强制重绘窗口
            break;
        }
        case WM_PAINT:          // 处理窗口重绘
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 填充窗口背景色
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_CLOSE:
            if (db.dbfilename == NULL || !isEdited)
            {
                PostQuitMessage(0);
                return 0;
            }
            
            int result = MessageBox(hwnd, TEXT("是否保存文件并退出？"), TEXT("退出"), MB_YESNOCANCEL);
            if (result == IDYES)
            {
                edbClose(&db);
                PostQuitMessage(0);
                return 0;
            }
            else if (result == IDNO)
            {
                PostQuitMessage(0);
                return 0;
            }
            else if (result == IDCANCEL)
            {
                return 0;
            }
            break;
        }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    global_hInstance = hInstance;
    // 注册主窗口类
    WNDCLASS mainwc = {0};
    mainwc.lpfnWndProc = MainWindowProc;
    mainwc.hInstance = hInstance;
    mainwc.lpszClassName = mainClassName;
    mainwc.hIcon = LoadIcon(hInstance, TEXT("IDR_MAINICON"));   // 设置图标
    RegisterClass(&mainwc);

    // 注册新建数据库窗口类
    WNDCLASS createwc = {0};
    createwc.lpfnWndProc = CreateDBWindowProc;
    createwc.hInstance = hInstance;
    createwc.lpszClassName = createDBClassName;
    createwc.hIcon = LoadIcon(hInstance, TEXT("IDR_MAINICON")); // 设置图标
    RegisterClass(&createwc);

    // 创建窗口
    hMainWindow = CreateWindowExW(WS_EX_ACCEPTFILES | WS_EX_CLIENTEDGE,    // 可拖入文件
                                mainClassName, 
                                utf8toutf16(appName, utf16_buffer, M_BUF_SIZ), 
                                WS_OVERLAPPEDWINDOW, 
                                CW_USEDEFAULT, CW_USEDEFAULT, 
                                DEFAULT_WIDTH, DEFAULT_HEIGHT, 
                                NULL, NULL, 
                                mainwc.hInstance,
                                NULL);

    
    // 显示窗口
    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);

    // 初始化常用控件库
    InitCommonControls();

    // 启用 Windows 视觉样式
    EnableThemeDialogTexture(hMainListView, ETDT_ENABLE);

    // 初始化 Common Controls 库
    InitCommonControls();

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}