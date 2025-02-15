#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include "easydb.h"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

#define OPEN_FILE_BUTTON 101    // 打开文件按钮
#define SAVE_FILE_BUTTON 102    // 保存文件按钮
#define ADDNEW_BUTTON 103       // 编辑按钮
#define EDIT_BUTTON 104         // 编辑按钮
#define DELETE_BUTTON 105       // 删除按钮
#define DB_LIST_VIEW 201        // 显示数据库内容的列表展示

char gbk_buffer[65536];
char utf8_buffer[65536];
char dbfilename[1024];
HWND hMainWindow;
HWND hMainListView;
EasyDB db;
bool isEdited;              // 文件是否被被编辑
HWND* editingEditWindows;   // 存储正在编辑的编辑框
size_t editingItem;         // 正在编辑的行
HWND* addingEditWindows;    // 存储正在添加的编辑框
size_t addingItem;         // 正在添加的行

char* utf8togbk(char* utf8text, char* gbktext, size_t gbktext_size)
{
    wchar_t* utf16text = (wchar_t*)calloc((strlen(utf8text) + 1) * 2, sizeof(char));
    MultiByteToWideChar(CP_UTF8, 0, utf8text, -1, utf16text, (strlen(utf8text) + 1) * 2);
    WideCharToMultiByte(936, 0, utf16text, -1, gbktext, gbktext_size, NULL, NULL);
    free(utf16text);
    return gbktext;
}

char* gbktoutf8(char* gbktext, char* utf8text, size_t utf8text_size)
{
    wchar_t* utf16text = (wchar_t*)calloc((strlen(gbktext) + 1) * 2, sizeof(char));
    MultiByteToWideChar(936, 0, gbktext, -1, utf16text, (strlen(gbktext) + 1) * 2);
    WideCharToMultiByte(CP_UTF8, 0, utf16text, -1, utf8text, utf8text_size, NULL, NULL);
    free(utf16text);
    return utf8text;
}

void ProcessDBListView(HWND hListView)      // 将数据库文件读取到ListView
{
    if (db.dbfilename == NULL) return;

    LV_COLUMN lvCol;
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH;    // 需要指定显示的字符和宽度
    for (size_t i = 0; i < db.columnCount; i++)
    {
        lvCol.pszText = utf8togbk(db.columnNames[i], gbk_buffer, 1024);
        lvCol.cx = 150;
        ListView_InsertColumn(hListView, i, &lvCol);
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
            char buf[256];
            switch (db.dataTypes[i])
            {
            case EDB_TYPE_INT:
                sprintf(buf, "%lld", Int(it[i]));
                lvItem.pszText = buf;
                break;
            case EDB_TYPE_REAL:
                sprintf(buf, "%lf", Real(it[i]));
                lvItem.pszText = buf;
                break;
            case EDB_TYPE_BLOB:
                lvItem.pszText = "<Blob data>";
                break;
            case EDB_TYPE_TEXT:
                lvItem.pszText = utf8togbk(Text(it[i]), gbk_buffer, 65536);
                break;
            default:
                break;
            }
            if (i == 0) ListView_InsertItem(hListView, &lvItem);
            else ListView_SetItem(hListView, &lvItem);
        }
    }
}

void ClearListView(HWND hListView, size_t columnCount)  // 清除ListView
{
    ListView_DeleteAllItems(hListView);
    for (int i = 0; i < columnCount; i++)
    {
        ListView_DeleteColumn(hListView, 0); // 每次删除第一个列
    }
}

void OpenDBFile(HWND hListView)
{
    if (db.dbfilename && isEdited)
    {
        int result = MessageBox(hMainListView, "是否保存文件并打开新文件？", "打开新文件", MB_YESNOCANCEL);
        if (result == IDYES)
        {
            ClearListView(hMainListView, db.columnCount);
            edbClose(&db);
        }
        else if (result == IDNO)
        {
            ClearListView(hMainListView, db.columnCount);
            edbCloseNotSave(&db);
        }
        else if (result == IDCANCEL)
        {
            return;
        }
    }
    else if (db.dbfilename && !isEdited)
    {
        ClearListView(hMainListView, db.columnCount);
        edbCloseNotSave(&db);
    }

    // 重置新增按钮状态
    HWND hAddNewButton = GetDlgItem(hMainWindow, ADDNEW_BUTTON);
    SendMessage(hAddNewButton, BM_SETCHECK, BST_UNCHECKED, 0);
    // 重置编辑按钮状态
    HWND hEditButton = GetDlgItem(hMainWindow, EDIT_BUTTON);
    SendMessage(hEditButton, BM_SETCHECK, BST_UNCHECKED, 0);

    edbOpen(dbfilename, &db);
    ProcessDBListView(hMainListView);
}

void DeleteButtonPressed(HWND hListView)    // 处理删除按钮
{
    size_t itemCount = ListView_GetItemCount(hListView); // 获取总行数
    LV_ITEM lvItem;
    lvItem.mask = LVIF_STATE;   // 获取状态
    lvItem.iSubItem = 0;
    lvItem.stateMask = LVIS_STATEIMAGEMASK;

    LV_ITEM getPriKey;
    char* priKeyBuf = (char*)malloc(db.dataSizes[db.primaryKeyIndex] * 8);
    getPriKey.iSubItem = db.primaryKeyIndex;
    getPriKey.mask = LVIF_TEXT;
    getPriKey.pszText = priKeyBuf;
    getPriKey.cchTextMax = db.dataSizes[db.primaryKeyIndex] * 8;

    for (size_t i = 0; i < itemCount; i++)
    {
        lvItem.iItem = i;
        ListView_GetItem(hListView, &lvItem);
        int itemState = (lvItem.state & LVIS_STATEIMAGEMASK) >> 12; // 获取复选框状态值
        if (itemState == 2)
        {
            getPriKey.iItem = i;
            ListView_GetItem(hListView, &getPriKey);
            switch (db.dataTypes[db.primaryKeyIndex])
            {
            case EDB_TYPE_INT:{
                edb_int priKeyInt;
                sscanf(priKeyBuf, "%lld", &priKeyInt);
                edbDelete(&db, &priKeyInt);
                break;
            }
            case EDB_TYPE_REAL:{
                double priKeyReal;
                sscanf(priKeyBuf, "%lf", &priKeyReal);
                edbDelete(&db, &priKeyReal);
                break;
            }
            case EDB_TYPE_TEXT:{
                edbDelete(&db, gbktoutf8(priKeyBuf, utf8_buffer, 65536));
                break;
            }
            default:
                break;
            }
            ListView_DeleteItem(hListView, i);
        }
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
    char* empty = "";
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
    addingEditWindows = (HWND*)malloc(db.columnCount * sizeof(HWND));   // 分配空间用于存储文本框句柄

    // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
    HFONT hFont = (HFONT)SendMessage(hMainListView, WM_GETFONT, 0, 0);
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
        addingEditWindows[i] = CreateWindowEx(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                "",
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
        SendMessage(addingEditWindows[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        // 设置编辑框的 Z 顺序 确保编辑框位于 ListView 的上方，避免被 ListView 覆盖。
        // SetWindowPos(addingEditWindows[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void AddNewOver_ReadEdit(HWND hListView)  // 结束添加
{
    // 初始化新行
    void** newRow = (void**)malloc(db.columnCount * sizeof(void*));
    for (size_t i = 0; i < db.columnCount; i++)
    {
        newRow[i] = (void*)malloc(db.dataSizes[i]);
    }
    
    for (size_t i = 0; i < db.columnCount; i++) // 逐列处理
    {
        char* readBuf = (char*)malloc(db.dataSizes[i] * 4);
        edb_int readInt;
        edb_real readReal;
        GetWindowText(addingEditWindows[i], readBuf, db.dataSizes[i] * 4); // 读取编辑框的内容
        switch (db.dataTypes[i])
        {
        case EDB_TYPE_INT:
            sscanf(readBuf, "%lld", newRow[i]);
            break;
        case EDB_TYPE_REAL:
            sscanf(readBuf, "%lf", newRow[i]);
            break;
        case EDB_TYPE_TEXT:
            memcpy(newRow[i], gbktoutf8(readBuf, utf8_buffer, 65536), db.dataSizes[i]);
            break;
        default:
            break;
        }
        ListView_SetItemText(hListView, addingItem, i, readBuf);   // 更新ListView中的内容
        ShowWindow(addingEditWindows[i], SW_HIDE); // 隐藏窗口
        DestroyWindow(addingEditWindows[i]);       // 销毁窗口
    }
    edbInsert(&db, newRow); // 插入新行

    // 释放新行
    for (size_t i = 0; i < db.columnCount; i++)
    {
        free(newRow[i]);
    }
    free(newRow);
    addingItem = -1;
    free(addingEditWindows); addingEditWindows = NULL;
}

void EditStart_SetEdit(HWND hListView)  // 开始编辑
{
    // 获取选中的行
    size_t selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    editingItem = selectedItem;

    RECT rect;  // 存放每个项的位置区域信息，用于创建文本框
    editingEditWindows = (HWND*)malloc(db.columnCount * sizeof(HWND)); // 分配空间用于存储文本框句柄

    // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
    HFONT hFont = (HFONT)SendMessage(hMainListView, WM_GETFONT, 0, 0);
    if (hFont == NULL) {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    for (size_t i = 0; i < db.columnCount; i++)
    {
        char* pszTextBuf = (char*)malloc(db.dataSizes[db.primaryKeyIndex] * 8);
        if (i == 0) // 由于兼容性考虑，获取第一个子项的矩形区域时需要使用LVIR_LABEL
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_LABEL, &rect);    // 获取第一个子项的矩形区域
        }
        else
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_BOUNDS, &rect);    // 获取子项的矩形区域
        }
        // MapWindowPoints(hListView, GetParent(hListView), (LPPOINT)&rect, 2);        // 将获取到的坐标映射为父窗口的坐标系
        editingEditWindows[i] = CreateWindowEx(WS_EX_CLIENTEDGE,
                                                WC_EDIT,
                                                "",
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
        SendMessage(editingEditWindows[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        // 获取当前子项内容
        ListView_GetItemText(hListView, selectedItem, i, pszTextBuf, db.dataSizes[db.primaryKeyIndex] * 8);
        // 发送到编辑框
        SetWindowText(editingEditWindows[i], pszTextBuf);
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
    ListView_GetItemText(hListView, editingItem, db.primaryKeyIndex, priKeyBuf, pKBufSiz);
    edb_int priKeyInt;
    edb_real priKeyReal;
    switch (db.dataTypes[db.primaryKeyIndex])
    {
    case EDB_TYPE_INT:
        sscanf(priKeyBuf, "%lld", &priKeyInt);
        memcpy(priKeyBuf, &priKeyInt, db.dataSizes[db.primaryKeyIndex]);
        break;
    case EDB_TYPE_REAL:
        sscanf(priKeyBuf, "%lld", &priKeyReal);
        memcpy(priKeyBuf, &priKeyReal, db.dataSizes[db.primaryKeyIndex]);
        break;
    case EDB_TYPE_TEXT:
        memcpy(priKeyBuf, gbktoutf8(priKeyBuf, utf8_buffer, 65536), db.dataSizes[db.primaryKeyIndex]);
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
        char* readBuf = (char*)malloc(db.dataSizes[i] * 4);
        edb_int readInt;
        edb_real readReal;
        GetWindowText(editingEditWindows[i], readBuf, db.dataSizes[i] * 4); // 读取编辑框的内容
        switch (db.dataTypes[i])
        {
        case EDB_TYPE_INT:
            sscanf(readBuf, "%lld", &readInt);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readInt);
            break;
        case EDB_TYPE_REAL:
            sscanf(readBuf, "%lf", &readReal);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readReal);
            break;
        case EDB_TYPE_TEXT:
            edbUpdate(&db, priKeyBuf, db.columnNames[i], gbktoutf8(readBuf, utf8_buffer, 65535));
            break;
        default:
            break;
        }
        ListView_SetItemText(hListView, editingItem, i, readBuf);   // 更新ListView中的内容
        ShowWindow(editingEditWindows[i], SW_HIDE); // 隐藏窗口
        DestroyWindow(editingEditWindows[i]);       // 销毁窗口
    }
    // 专门处理主键
    if (editingEditWindows)
    {
        size_t i = db.primaryKeyIndex;
        char* readBuf = (char*)malloc(db.dataSizes[i] * 4);
        edb_int readInt;
        edb_real readReal;
        GetWindowText(editingEditWindows[i], readBuf, db.dataSizes[i] * 4); // 读取编辑框的内容
        switch (db.dataTypes[i])
        {
        case EDB_TYPE_INT:
            sscanf(readBuf, "%lld", &readInt);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readInt);
            break;
        case EDB_TYPE_REAL:
            sscanf(readBuf, "%lf", &readReal);
            edbUpdate(&db, priKeyBuf, db.columnNames[i], &readReal);
            break;
        case EDB_TYPE_TEXT:
            edbUpdate(&db, priKeyBuf, db.columnNames[i], gbktoutf8(readBuf, utf8_buffer, 65535));
            break;
        default:
            break;
        }
        ListView_SetItemText(hListView, editingItem, i, readBuf);   // 更新ListView中的内容
        ShowWindow(editingEditWindows[i], SW_HIDE); // 隐藏窗口
        DestroyWindow(editingEditWindows[i]);       // 销毁窗口
    }
    editingItem = -1;
    free(editingEditWindows); editingEditWindows = NULL;
    free(priKeyBuf);
}

void OpenFileDialogAndProcess(HWND hwnd) // 选择文件对话框
{
    OPENFILENAME ofn;       // 文件选择对话框的结构体
    char szFile[260];       // 用于存储选择的文件路径

    // 初始化 OPENFILENAME 结构体
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "EasyDB File\0*.db";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = "Open File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // 显示文件选择对话框
    if (GetOpenFileName(&ofn) == TRUE) {
        // 在这里处理选中的文件，例如打开、读取内容等
        strcpy(dbfilename, ofn.lpstrFile);
        OpenDBFile(hMainListView);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:{
            // 获取窗口默认字体，后续设置字体为相同，避免按键字体奇怪
            HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
            if (hFont == NULL) {
                hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            HWND hButton;
            // “打开”按钮
            hButton = CreateWindowEx(0, "BUTTON", "打开",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                10, 10, 60, 25, 
                hwnd, (HMENU)OPEN_FILE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “保存”按钮
            hButton = CreateWindowEx(0, "BUTTON", "保存",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                80, 10, 60, 25, 
                hwnd, (HMENU)SAVE_FILE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “新增”按钮
            hButton = CreateWindowEx(0, "BUTTON", "新增",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                150, 10, 60, 25, 
                hwnd, (HMENU)ADDNEW_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “编辑”按钮
            hButton = CreateWindowEx(0, "BUTTON", "编辑",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                220, 10, 60, 25, 
                hwnd, (HMENU)EDIT_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // “删除”按钮
            hButton = CreateWindowEx(0, "BUTTON", "删除",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                290, 10, 60, 25, 
                hwnd, (HMENU)DELETE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // 预览列表
            hMainListView = CreateWindowEx(WS_EX_CLIENTEDGE,    // 扩展窗口样式，带边框
                    WC_LISTVIEW, "",
                    WS_CHILD | WS_VISIBLE | LVS_REPORT,
                    10, 50, DEFAULT_WIDTH - 30, DEFAULT_HEIGHT - 60,
                    hwnd, (HMENU)DB_LIST_VIEW,
                    GetModuleHandle(NULL), NULL);
            // SendMessage(hMainListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);   // 整行选中
            // SendMessage(hMainListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);         // 复选框
            ListView_SetExtendedListViewStyle(hMainListView, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
            break;
        }
        case WM_COMMAND:{
            if (LOWORD(wParam) == OPEN_FILE_BUTTON)          // “打开”按钮被按下
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
                if (db.dbfilename && SendMessage(hButton, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                {
                    isEdited = true;
                    SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0);
                    AddNewStart_SetEdit(hMainListView);
                }
                else if (db.dbfilename && SendMessage(hButton, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    isEdited = true;
                    SendMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0);
                    AddNewOver_ReadEdit(hMainListView);
                }
            }
            else if (LOWORD(wParam) == EDIT_BUTTON)          // “编辑”按钮被按下
            {
                HWND hButton = (HWND)lParam;
                if (db.dbfilename && SendMessage(hButton, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                {
                    isEdited = true;
                    SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0);
                    EditStart_SetEdit(hMainListView);
                }
                else if (db.dbfilename && SendMessage(hButton, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    isEdited = true;
                    SendMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0);
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
            break;
        }
        case WM_DROPFILES:{     // 处理拖入文件
            HDROP hDrop = (HDROP)wParam;

            char filePath[MAX_PATH];
            DragQueryFile(hDrop, 0, filePath, MAX_PATH);
            strcpy(dbfilename, filePath);
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
            
            int result = MessageBox(hwnd, "是否保存文件并退出？", "退出", MB_YESNOCANCEL);
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
        }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 注册主窗口类
    const char* mainClassName = "MainWindow";
    WNDCLASS mainwc = {0};
    mainwc.lpfnWndProc = WindowProc;
    mainwc.hInstance = hInstance;
    mainwc.lpszClassName = mainClassName;
    RegisterClass(&mainwc);

    // 创建窗口
    hMainWindow = CreateWindowEx(WS_EX_ACCEPTFILES,    // 可拖入文件
                                        mainClassName, 
                                        "EasyDB Explorer", 
                                        WS_OVERLAPPEDWINDOW, 
                                        CW_USEDEFAULT, CW_USEDEFAULT, 
                                        DEFAULT_WIDTH, DEFAULT_HEIGHT, 
                                        NULL, NULL, 
                                        mainwc.hInstance,
                                        NULL);
    
    // 显示窗口
    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);

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