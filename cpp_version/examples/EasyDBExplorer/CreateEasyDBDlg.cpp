// CreateEasyDBDlg.cpp: 实现文件
//

#include "pch.h"
#include "EasyDBExplorer.h"
#include "afxdialogex.h"
#include "CreateEasyDBDlg.h"


// CreateEasyDBDlg 对话框

IMPLEMENT_DYNAMIC(CreateEasyDBDlg, CDialogEx)

CreateEasyDBDlg::CreateEasyDBDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CREATEDB, pParent)
	, columnName(_T(""))
	, colDataLength(0)
	, primaryKeyName(_T(""))
	, tableName(_T(""))
{

}

CreateEasyDBDlg::~CreateEasyDBDlg()
{
}

void CreateEasyDBDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, colsListView);
	DDX_Control(pDX, IDC_COMBO1, colDataTypeComboBox);
	DDX_Text(pDX, IDC_EDIT1, columnName);
	DDX_Text(pDX, IDC_EDIT3, colDataLength);
	DDX_Text(pDX, IDC_EDIT4, primaryKeyName);
	DDX_Text(pDX, IDC_EDIT2, tableName);
}


BEGIN_MESSAGE_MAP(CreateEasyDBDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CreateEasyDBDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON3, &CreateEasyDBDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON2, &CreateEasyDBDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDOK, &CreateEasyDBDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CreateEasyDBDlg 消息处理程序

BOOL CreateEasyDBDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	// 初始化表头
	colsListView.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	colsListView.InsertColumn(0, TEXT("列名"), 0, 280);
	colsListView.InsertColumn(1, TEXT("数据类型"), 0, 130);
	colsListView.InsertColumn(2, TEXT("数据长度"), 0, 130);

	// 初始化数据类型选择
	colDataTypeComboBox.AddString(TEXT("文本"));
	colDataTypeComboBox.AddString(TEXT("整数"));
	colDataTypeComboBox.AddString(TEXT("小数"));
	colDataTypeComboBox.AddString(TEXT("二进制数据"));
	colDataTypeComboBox.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

// 新增一列
void CreateEasyDBDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	EDB::EColumn newCol;
	UpdateData();
	newCol.columnName = std::string(CT2U(columnName));
	switch (colDataTypeComboBox.GetCurSel())
	{
	case 0:
		newCol.dType = EDB::DataType::TEXT;
		newCol.columnSize = colDataLength;
		break;
	case 1:
		newCol.dType = EDB::DataType::INT;
		break;
	case 2:
		newCol.dType = EDB::DataType::REAL;
		break;
	case 3:
		newCol.dType = EDB::DataType::BLOB;
		newCol.columnSize = colDataLength;
		break;
	default:
		return;
	}
	
	cols.push_back(newCol);

	int lastItem = colsListView.GetItemCount();
	colsListView.InsertItem(lastItem, CString(CU2T(newCol.columnName.c_str())));
	CString tmpCStr;
	colDataTypeComboBox.GetWindowTextW(tmpCStr);
	colsListView.SetItemText(lastItem, 1, tmpCStr);
	if (newCol.dType == EDB::DataType::TEXT || newCol.dType == EDB::DataType::BLOB)
	{
		tmpCStr.Empty();
		tmpCStr.Format(TEXT("%llu"), newCol.columnSize);
		colsListView.SetItemText(lastItem, 2, tmpCStr);
	}
}

// 删除此列
void CreateEasyDBDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	int selectedItem = colsListView.GetNextItem(-1, LVNI_SELECTED);
	while (selectedItem != -1)
	{
		cols.erase(cols.begin() + selectedItem);
		colsListView.DeleteItem(selectedItem);
		selectedItem = colsListView.GetNextItem(selectedItem - 1, LVNI_SELECTED);
	}
}

// < 设为主键
void CreateEasyDBDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	int selectedItem = colsListView.GetNextItem(-1, LVNI_SELECTED);
	if (selectedItem != -1)
	{
		primaryKeyName = colsListView.GetItemText(selectedItem, 0);
		UpdateData(FALSE);
	}
}

void CreateEasyDBDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData();

	CFileDialog saveFileDlg{FALSE,
							TEXT("db"),
							NULL,
							OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							TEXT("EasyDB文件 (*.db)|*.db|所有文件 (*.*)|*.*||") };

	if (saveFileDlg.DoModal() == IDOK)
	{
		dbFileName = std::string(CT2U(saveFileDlg.GetPathName()));
		int retval = EDB::EasyDB::create(dbFileName, std::string(CT2U(tableName)), cols, std::string(CT2U(primaryKeyName)));
		if (retval != SUCCESS)
		{
			CString errStr = TEXT("创建失败，错误代码：");
			errStr += EDB::StatusCode2String(retval);
			AfxMessageBox(errStr);
			return;
		}
	}

	CDialogEx::OnOK();
}
