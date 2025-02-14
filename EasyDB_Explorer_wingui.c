#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include "easydb.h"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

#define OPEN_FILE_BUTTON 101    // ���ļ���ť
#define SAVE_FILE_BUTTON 102    // �����ļ���ť
#define ADDNEW_BUTTON 103       // �༭��ť
#define EDIT_BUTTON 104         // �༭��ť
#define DELETE_BUTTON 105       // ɾ����ť
#define DB_LIST_VIEW 201        // ��ʾ���ݿ����ݵ��б�չʾ

char gbk_buffer[65536];
char utf8_buffer[65536];
char dbfilename[1024];
HWND hMainWindow;
HWND hMainListView;
EasyDB db;
bool isEdited;              // �ļ��Ƿ񱻱��༭
HWND* editingEditWindows;   // �洢���ڱ༭�ı༭��
size_t editingItem;         // ���ڱ༭����
HWND* addingEditWindows;    // �洢������ӵı༭��
size_t addingItem;         // ������ӵ���

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

void ProcessDBListView(HWND hListView)      // �����ݿ��ļ���ȡ��ListView
{
    if (db.dbfilename == NULL) return;

    LV_COLUMN lvCol;
    lvCol.mask = LVCF_TEXT | LVCF_WIDTH;    // ��Ҫָ����ʾ���ַ��Ϳ��
    for (size_t i = 0; i < db.columnCount; i++)
    {
        lvCol.pszText = utf8togbk(db.columnNames[i], gbk_buffer, 1024);
        lvCol.cx = 150;
        ListView_InsertColumn(hListView, i, &lvCol);
    }

    size_t cnt = 0;
    LV_ITEM lvItem;
    lvItem.mask = LVIF_TEXT;                // ��Ҫָ����ʾ���ַ�
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

void ClearListView(HWND hListView, size_t columnCount)  // ���ListView
{
    ListView_DeleteAllItems(hListView);
    for (int i = 0; i < columnCount; i++)
    {
        ListView_DeleteColumn(hListView, 0); // ÿ��ɾ����һ����
    }
}

void OpenDBFile(HWND hListView)
{
    if (db.dbfilename && isEdited)
    {
        int result = MessageBox(hMainListView, "�Ƿ񱣴��ļ��������ļ���", "�����ļ�", MB_YESNOCANCEL);
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

    // ����������ť״̬
    HWND hAddNewButton = GetDlgItem(hMainWindow, ADDNEW_BUTTON);
    SendMessage(hAddNewButton, BM_SETCHECK, BST_UNCHECKED, 0);
    // ���ñ༭��ť״̬
    HWND hEditButton = GetDlgItem(hMainWindow, EDIT_BUTTON);
    SendMessage(hEditButton, BM_SETCHECK, BST_UNCHECKED, 0);

    edbOpen(dbfilename, &db);
    ProcessDBListView(hMainListView);
}

void DeleteButtonPressed(HWND hListView)    // ����ɾ����ť
{
    size_t itemCount = ListView_GetItemCount(hListView); // ��ȡ������
    LV_ITEM lvItem;
    lvItem.mask = LVIF_STATE;   // ��ȡ״̬
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
        int itemState = (lvItem.state & LVIS_STATEIMAGEMASK) >> 12; // ��ȡ��ѡ��״ֵ̬
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

void AddNewStart_SetEdit(HWND hListView)  // ��ʼ���
{
    // �½�һ�У�����ʼ��Ϊ��
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

    RECT rect;  // ���ÿ�����λ��������Ϣ�����ڴ����ı���
    addingEditWindows = (HWND*)malloc(db.columnCount * sizeof(HWND));   // ����ռ����ڴ洢�ı�����

    // ��ȡ����Ĭ�����壬������������Ϊ��ͬ�����ⰴ���������
    HFONT hFont = (HFONT)SendMessage(hMainListView, WM_GETFONT, 0, 0);
    if (hFont == NULL) {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    for (size_t i = 0; i < db.columnCount; i++)
    {
        if (i == 0) // ���ڼ����Կ��ǣ���ȡ��һ������ľ�������ʱ��Ҫʹ��LVIR_LABEL
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_LABEL, &rect);    // ��ȡ��һ������ľ�������
        }
        else
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_BOUNDS, &rect);    // ��ȡ����ľ�������
        }
        // MapWindowPoints(hListView, GetParent(hListView), (LPPOINT)&rect, 2);        // ����ȡ��������ӳ��Ϊ�����ڵ�����ϵ
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
        
        // ��������
        SendMessage(addingEditWindows[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        // ���ñ༭��� Z ˳�� ȷ���༭��λ�� ListView ���Ϸ������ⱻ ListView ���ǡ�
        // SetWindowPos(addingEditWindows[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void AddNewOver_ReadEdit(HWND hListView)  // �������
{
    // ��ʼ������
    void** newRow = (void**)malloc(db.columnCount * sizeof(void*));
    for (size_t i = 0; i < db.columnCount; i++)
    {
        newRow[i] = (void*)malloc(db.dataSizes[i]);
    }
    
    for (size_t i = 0; i < db.columnCount; i++) // ���д���
    {
        char* readBuf = (char*)malloc(db.dataSizes[i] * 4);
        edb_int readInt;
        edb_real readReal;
        GetWindowText(addingEditWindows[i], readBuf, db.dataSizes[i] * 4); // ��ȡ�༭�������
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
        ListView_SetItemText(hListView, addingItem, i, readBuf);   // ����ListView�е�����
        ShowWindow(addingEditWindows[i], SW_HIDE); // ���ش���
        DestroyWindow(addingEditWindows[i]);       // ���ٴ���
    }
    edbInsert(&db, newRow); // ��������

    // �ͷ�����
    for (size_t i = 0; i < db.columnCount; i++)
    {
        free(newRow[i]);
    }
    free(newRow);
    addingItem = -1;
    free(addingEditWindows); addingEditWindows = NULL;
}

void EditStart_SetEdit(HWND hListView)  // ��ʼ�༭
{
    // ��ȡѡ�е���
    size_t selectedItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    editingItem = selectedItem;

    RECT rect;  // ���ÿ�����λ��������Ϣ�����ڴ����ı���
    editingEditWindows = (HWND*)malloc(db.columnCount * sizeof(HWND)); // ����ռ����ڴ洢�ı�����

    // ��ȡ����Ĭ�����壬������������Ϊ��ͬ�����ⰴ���������
    HFONT hFont = (HFONT)SendMessage(hMainListView, WM_GETFONT, 0, 0);
    if (hFont == NULL) {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    for (size_t i = 0; i < db.columnCount; i++)
    {
        char* pszTextBuf = (char*)malloc(db.dataSizes[db.primaryKeyIndex] * 8);
        if (i == 0) // ���ڼ����Կ��ǣ���ȡ��һ������ľ�������ʱ��Ҫʹ��LVIR_LABEL
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_LABEL, &rect);    // ��ȡ��һ������ľ�������
        }
        else
        {
            ListView_GetSubItemRect(hListView, selectedItem, i, LVIR_BOUNDS, &rect);    // ��ȡ����ľ�������
        }
        // MapWindowPoints(hListView, GetParent(hListView), (LPPOINT)&rect, 2);        // ����ȡ��������ӳ��Ϊ�����ڵ�����ϵ
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
        
        // ��������
        SendMessage(editingEditWindows[i], WM_SETFONT, (WPARAM)hFont, TRUE);
        // ��ȡ��ǰ��������
        ListView_GetItemText(hListView, selectedItem, i, pszTextBuf, db.dataSizes[db.primaryKeyIndex] * 8);
        // ���͵��༭��
        SetWindowText(editingEditWindows[i], pszTextBuf);
        // ���ñ༭��� Z ˳�� ȷ���༭��λ�� ListView ���Ϸ������ⱻ ListView ���ǡ�
        // SetWindowPos(editingEditWindows[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        free(pszTextBuf);
    }
}

void EditOver_ReadEdit(HWND hListView)  // �����༭
{
    // ��ȡ����
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

    for (size_t i = 0; i < db.columnCount; i++) // ���д���
    {
        if (i == db.primaryKeyIndex)    // ��������
        {
            continue;
        }
        char* readBuf = (char*)malloc(db.dataSizes[i] * 4);
        edb_int readInt;
        edb_real readReal;
        GetWindowText(editingEditWindows[i], readBuf, db.dataSizes[i] * 4); // ��ȡ�༭�������
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
        ListView_SetItemText(hListView, editingItem, i, readBuf);   // ����ListView�е�����
        ShowWindow(editingEditWindows[i], SW_HIDE); // ���ش���
        DestroyWindow(editingEditWindows[i]);       // ���ٴ���
    }
    // ר�Ŵ�������
    if (editingEditWindows)
    {
        size_t i = db.primaryKeyIndex;
        char* readBuf = (char*)malloc(db.dataSizes[i] * 4);
        edb_int readInt;
        edb_real readReal;
        GetWindowText(editingEditWindows[i], readBuf, db.dataSizes[i] * 4); // ��ȡ�༭�������
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
        ListView_SetItemText(hListView, editingItem, i, readBuf);   // ����ListView�е�����
        ShowWindow(editingEditWindows[i], SW_HIDE); // ���ش���
        DestroyWindow(editingEditWindows[i]);       // ���ٴ���
    }
    editingItem = -1;
    free(editingEditWindows); editingEditWindows = NULL;
    free(priKeyBuf);
}

void OpenFileDialogAndProcess(HWND hwnd) // ѡ���ļ��Ի���
{
    OPENFILENAME ofn;       // �ļ�ѡ��Ի���Ľṹ��
    char szFile[260];       // ���ڴ洢ѡ����ļ�·��

    // ��ʼ�� OPENFILENAME �ṹ��
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

    // ��ʾ�ļ�ѡ��Ի���
    if (GetOpenFileName(&ofn) == TRUE) {
        // �����ﴦ��ѡ�е��ļ�������򿪡���ȡ���ݵ�
        strcpy(dbfilename, ofn.lpstrFile);
        OpenDBFile(hMainListView);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:{
            // ��ȡ����Ĭ�����壬������������Ϊ��ͬ�����ⰴ���������
            HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
            if (hFont == NULL) {
                hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            HWND hButton;
            // ���򿪡���ť
            hButton = CreateWindowEx(0, "BUTTON", "��",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                10, 10, 60, 25, 
                hwnd, (HMENU)OPEN_FILE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // �����桱��ť
            hButton = CreateWindowEx(0, "BUTTON", "����",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                80, 10, 60, 25, 
                hwnd, (HMENU)SAVE_FILE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // ����������ť
            hButton = CreateWindowEx(0, "BUTTON", "����",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                150, 10, 60, 25, 
                hwnd, (HMENU)ADDNEW_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // ���༭����ť
            hButton = CreateWindowEx(0, "BUTTON", "�༭",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                220, 10, 60, 25, 
                hwnd, (HMENU)EDIT_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // ��ɾ������ť
            hButton = CreateWindowEx(0, "BUTTON", "ɾ��",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                290, 10, 60, 25, 
                hwnd, (HMENU)DELETE_BUTTON, 
                GetModuleHandle(NULL), NULL);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            // Ԥ���б�
            hMainListView = CreateWindowEx(WS_EX_CLIENTEDGE,    // ��չ������ʽ�����߿�
                    WC_LISTVIEW, "",
                    WS_CHILD | WS_VISIBLE | LVS_REPORT,
                    10, 50, DEFAULT_WIDTH - 30, DEFAULT_HEIGHT - 60,
                    hwnd, (HMENU)DB_LIST_VIEW,
                    GetModuleHandle(NULL), NULL);
            // SendMessage(hMainListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);   // ����ѡ��
            // SendMessage(hMainListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);         // ��ѡ��
            ListView_SetExtendedListViewStyle(hMainListView, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
            break;
        }
        case WM_COMMAND:{
            if (LOWORD(wParam) == OPEN_FILE_BUTTON)          // ���򿪡���ť������
            {
                OpenFileDialogAndProcess(hwnd);
            }
            else if (LOWORD(wParam) == SAVE_FILE_BUTTON)     // �����桱��ť������
            {
                if (db.dbfilename)
                {
                    isEdited = false;
                    edbSave(&db);
                }
            }
            else if (LOWORD(wParam) == ADDNEW_BUTTON)        // ����������ť������
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
            else if (LOWORD(wParam) == EDIT_BUTTON)          // ���༭����ť������
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
            else if (LOWORD(wParam) == DELETE_BUTTON)        // ��ɾ������ť������
            {
                if (db.dbfilename)
                {
                    isEdited = true;
                    DeleteButtonPressed(hMainListView);
                }
            }
            break;
        }
        case WM_DROPFILES:{     // ���������ļ�
            HDROP hDrop = (HDROP)wParam;

            char filePath[MAX_PATH];
            DragQueryFile(hDrop, 0, filePath, MAX_PATH);
            strcpy(dbfilename, filePath);
            OpenDBFile(hMainListView);

            // �ͷ���ק���
            DragFinish(hDrop);
            break;
        }
        case WM_SIZE:           // �����û��ı䴰�ڴ�С
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            MoveWindow(hMainListView, 10, 40, newWidth - 30, newHeight - 60, TRUE);     // �޸�ListView�Ĵ�С
            InvalidateRect(hwnd, NULL, TRUE);  // ǿ���ػ洰��
            break;
        }
        case WM_PAINT:          // �������ػ�
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // ��䴰�ڱ���ɫ
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
            
            int result = MessageBox(hwnd, "�Ƿ񱣴��ļ����˳���", "�˳�", MB_YESNOCANCEL);
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
    // ע����������
    const char* mainClassName = "MainWindow";
    WNDCLASS mainwc = {0};
    mainwc.lpfnWndProc = WindowProc;
    mainwc.hInstance = hInstance;
    mainwc.lpszClassName = mainClassName;
    RegisterClass(&mainwc);

    // ��������
    hMainWindow = CreateWindowEx(WS_EX_ACCEPTFILES,    // �������ļ�
                                        mainClassName, 
                                        "EasyDB Explorer", 
                                        WS_OVERLAPPEDWINDOW, 
                                        CW_USEDEFAULT, CW_USEDEFAULT, 
                                        DEFAULT_WIDTH, DEFAULT_HEIGHT, 
                                        NULL, NULL, 
                                        mainwc.hInstance,
                                        NULL);
    
    // ��ʾ����
    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);

    // ��ʼ�� Common Controls ��
    InitCommonControls();

    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}