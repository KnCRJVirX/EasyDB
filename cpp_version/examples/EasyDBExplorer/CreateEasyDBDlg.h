#pragma once

#include "afxdialogex.h"
#include "EasyDB.hpp"
#include "utils.h"


// CreateEasyDBDlg 对话框

class CreateEasyDBDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CreateEasyDBDlg)

public:
	CreateEasyDBDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CreateEasyDBDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CREATEDB };
#endif

protected:
	std::vector<EDB::EColumn> cols;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl colsListView;
	CComboBox colDataTypeComboBox;
	afx_msg void OnBnClickedButton1();
	CString columnName;
	size_t colDataLength;
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton2();
	CString primaryKeyName;
	afx_msg void OnBnClickedOk();
	CString tableName;
	std::string dbFileName;
};
