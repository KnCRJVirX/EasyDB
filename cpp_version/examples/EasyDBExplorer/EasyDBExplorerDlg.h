
// EasyDBExplorerDlg.h: 头文件
//

#pragma once

#include "CreateEasyDBDlg.h"
#include "EasyDB.hpp"
#include "utils.h"

// CEasyDBExplorerDlg 对话框
class CEasyDBExplorerDlg : public CDialogEx
{
// 构造
public:
	CEasyDBExplorerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EASYDBEXPLORER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	EDB::EasyDB edb;
	bool isEdited;
	std::vector<CEdit*> editingBoxes;
	int editingItem;

	int PostRowToMainListView(const EDB::ERowView& row, int col);
	int InitMainListViewColumn();
	int OpenDBFile(const std::string& dbFilePath);

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedButton2();
	CButton insertCheckBox;
	CButton editCheckBox;
	CButton deleteButton;
	CListCtrl mainListView;
	afx_msg void OnOpenFileButtonClick();
	afx_msg void OnSaveButtonClick();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnClose();
	afx_msg void OnCreateDBButtonClick();
	CComboBox columnComboBox;
	CString searchKeyWord;
	afx_msg void OnBnClickedButton1();
};
