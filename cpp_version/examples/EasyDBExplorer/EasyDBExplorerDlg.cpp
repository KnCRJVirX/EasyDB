
// EasyDBExplorerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "EasyDBExplorer.h"
#include "EasyDBExplorerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CEasyDBExplorerDlg 对话框



CEasyDBExplorerDlg::CEasyDBExplorerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EASYDBEXPLORER_DIALOG, pParent)
	, searchKeyWord(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(ICON_EASYDB);
}

void CEasyDBExplorerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1, insertCheckBox);
	DDX_Control(pDX, IDC_CHECK2, editCheckBox);
	DDX_Control(pDX, IDC_BUTTON2, deleteButton);
	DDX_Control(pDX, IDC_LIST2, mainListView);
	DDX_Control(pDX, IDC_COMBO1, columnComboBox);
	DDX_Text(pDX, IDC_EDIT1, searchKeyWord);
}

BEGIN_MESSAGE_MAP(CEasyDBExplorerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CHECK1, &CEasyDBExplorerDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK2, &CEasyDBExplorerDlg::OnBnClickedCheck2)
	ON_BN_CLICKED(IDC_BUTTON2, &CEasyDBExplorerDlg::OnBnClickedButton2)
	ON_COMMAND(ID_32771, &CEasyDBExplorerDlg::OnOpenFileButtonClick)
	ON_COMMAND(ID_32772, &CEasyDBExplorerDlg::OnSaveButtonClick)
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_COMMAND(ID_32773, &CEasyDBExplorerDlg::OnCreateDBButtonClick)
	ON_BN_CLICKED(IDC_BUTTON1, &CEasyDBExplorerDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CEasyDBExplorerDlg 消息处理程序

BOOL CEasyDBExplorerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	// 添加菜单
	// 文件菜单
	CMenu fileMenu;
	fileMenu.LoadMenuW(IDR_MENU1);
	SetMenu(&fileMenu);

	// 接受文件拖拽
	DragAcceptFiles(TRUE);

	// 对ListView允许整行选中
	mainListView.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CEasyDBExplorerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CEasyDBExplorerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CEasyDBExplorerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 新增
void CEasyDBExplorerDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	// 初始化检查
	IF_NOT_INIT(&edb)
	{
		insertCheckBox.SetCheck(BST_UNCHECKED);
		return;
	}
	// 不支持编辑主键为二进制数据的EasyDB数据库
	IF_PRIMARY_KEY_IS_BLOB(&edb)
	{
		AfxMessageBox(TEXT("不支持编辑主键为二进制数据的EasyDB数据库"), MB_ICONWARNING | MB_OK);
		return;
	}
	if (insertCheckBox.GetCheck() == BST_CHECKED)
	{
		// 新建一行并初始化为空
		int newItemPos = mainListView.GetItemCount();
		editingItem = newItemPos;
		mainListView.InsertItem(newItemPos, TEXT(""));

		// 确保已清空
		editingBoxes.clear();

		// 获取rect，建立编辑框
		for (int i = 0; i < edb.get_column_info().size(); ++i)
		{
			CRect rect;
			if (i == 0)
			{
				mainListView.GetSubItemRect(newItemPos, i, LVIR_LABEL, rect);
			}
			else
			{
				mainListView.GetSubItemRect(newItemPos, i, LVIR_BOUNDS, rect);
			}

			CEdit* editBox = new CEdit();
			editBox->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, rect, &mainListView, 0);
			editingBoxes.push_back(editBox);
		}
		// 禁用编辑
		editCheckBox.EnableWindow(FALSE);
	}
	else if (insertCheckBox.GetCheck() == BST_UNCHECKED)
	{
		std::unordered_map<std::string, EDB::Data> newRowMap;
		std::vector infos = edb.get_column_info();
		for (int i = 0; i < editingBoxes.size(); ++i)
		{
			auto editBox = editingBoxes[i];
			CString inputCStr;
			editBox->GetWindowTextW(inputCStr);
			std::string inputStr = std::string(CT2U(inputCStr));

			// 构造新数据
			EDB::Data newData;
			switch (infos[i].dType)
			{
			case EDB::DataType::INT: {
				 newData = std::stoll(inputStr);
				 break;
			}
			case EDB::DataType::REAL: {
				newData = std::stod(inputStr);
				break;
			}
			case EDB::DataType::TEXT: {
				newData = inputStr;
				break;
			}
			default:
				break;
			}

			// 插入到新行
			if (infos[i].dType != EDB::DataType::BLOB)
			{
				newRowMap[infos[i].columnName] = newData;
			}

			// 删除编辑框
			delete editBox;
		}
		editingBoxes.clear();

		int retval = edb.insert(newRowMap);
		mainListView.DeleteItem(editingItem);
		if (retval == SUCCESS)
		{
			isEdited = true;
			auto row = edb.at(newRowMap[infos[edb.get_primary_key_index()].columnName]);
			PostRowToMainListView(row, editingItem);
		}
		else
		{
			CString errStr = TEXT("插入失败，错误代码：");
			errStr += EDB::StatusCode2String(retval);
			AfxMessageBox(errStr, MB_ICONWARNING | MB_OK);
		}
		// 恢复启用编辑
		editCheckBox.EnableWindow(TRUE);
	}
}

// 编辑
void CEasyDBExplorerDlg::OnBnClickedCheck2()
{
	// TODO: 在此添加控件通知处理程序代码
	// 初始化检查
	IF_NOT_INIT(&edb)
	{
		editCheckBox.SetCheck(BST_UNCHECKED);
		return;
	}
	// 不支持编辑主键为二进制数据的EasyDB数据库
	IF_PRIMARY_KEY_IS_BLOB(&edb)
	{
		AfxMessageBox(TEXT("不支持编辑主键为二进制数据的EasyDB数据库"), MB_ICONWARNING | MB_OK);
		return;
	}
	if (editCheckBox.GetCheck() == BST_CHECKED)
	{
		// 获取选中的行
		int selectedItem = mainListView.GetNextItem(-1, LVNI_SELECTED);
		editingItem = selectedItem;
		if (selectedItem == -1)
		{
			editCheckBox.SetCheck(BST_UNCHECKED);
			return;
		}

		// 确保已清空
		editingBoxes.clear();

		// 获取rect，建立编辑框
		for (int i = 0; i < edb.get_column_info().size(); ++i)
		{
			CRect rect;
			if (i == 0)
			{
				mainListView.GetSubItemRect(selectedItem, i, LVIR_LABEL, rect);
			}
			else
			{
				mainListView.GetSubItemRect(selectedItem, i, LVIR_BOUNDS, rect);
			}

			// 创建编辑框
			CEdit* editBox = new CEdit();
			editBox->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, rect, &mainListView, 0);

			// 发送当前已有的内容
			editBox->SetWindowTextW(mainListView.GetItemText(selectedItem, i));
			editingBoxes.push_back(editBox);
		}
		// 禁用新增
		insertCheckBox.EnableWindow(FALSE);
	}
	else if (editCheckBox.GetCheck() == BST_UNCHECKED)
	{
		std::unordered_map<std::string, EDB::Data> newRowMap;
		std::vector infos = edb.get_column_info();

		// 获取主键
		EDB::Data priKey;
		int priKeyIndex = edb.get_primary_key_index();
		std::string priKeyStr = std::string(CT2U(mainListView.GetItemText(editingItem, priKeyIndex)));
		switch (infos[priKeyIndex].dType)
		{
		case EDB::DataType::INT: {
			priKey = std::stoll(priKeyStr);
			break;
		}
		case EDB::DataType::REAL: {
			priKey = std::stod(priKeyStr);
			break;
		}
		case EDB::DataType::TEXT: {
			priKey = priKeyStr;
			break;
		}
		default:
			break;
		}

		// 错误记录
		bool isFail = false;
		CString errStr;

		// 遍历读取输入框并修改
		for (int i = 0; i < editingBoxes.size(); ++i)
		{
			// 获取输入字符
			auto editBox = editingBoxes[i];
			CString inputCStr;
			editBox->GetWindowTextW(inputCStr);
			std::string inputStr = std::string(CT2U(inputCStr));

			// 构造新数据
			EDB::Data newData;
			switch (infos[i].dType)
			{
			case EDB::DataType::INT: {
				newData = std::stoll(inputStr);
				break;
			}
			case EDB::DataType::REAL: {
				newData = std::stod(inputStr);
				break;
			}
			case EDB::DataType::TEXT: {
				newData = inputStr;
				break;
			}
			default:
				break;
			}

			// 修改
			int retval = edb.update(priKey, i, newData);
			if (retval != SUCCESS)
			{
				errStr += "修改列：";
				errStr += CU2T(infos[i].columnName.c_str());
				errStr += "时失败，错误代码：";
				errStr += EDB::StatusCode2String(retval);
				errStr += "。\n";
				isFail = true;
			}
			// 如果是主键且成功修改，刷新主键
			if (i == priKeyIndex && retval == SUCCESS)
			{
				priKey = newData;
			}

			// 删除编辑框
			delete editBox;
		}
		editingBoxes.clear();
		
		// 将修改后的列推送到ListView
		mainListView.DeleteItem(editingItem);
		PostRowToMainListView(edb.at(priKey), editingItem);

		// 如果有失败，弹窗提示
		if (isFail)
		{
			AfxMessageBox(errStr, MB_ICONWARNING | MB_OK);
		}

		// 标记编辑
		isEdited = true;
		
		// 恢复启用新增
		insertCheckBox.EnableWindow(TRUE);
	}
}

// 删除
void CEasyDBExplorerDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	int priKeyIndex = edb.get_primary_key_index();
	int selectedItem = mainListView.GetNextItem(-1, LVNI_SELECTED);
	while (selectedItem != -1)
	{
		CString selectedCStr = mainListView.GetItemText(selectedItem, priKeyIndex);
		std::string selectedStr = std::string(static_cast<const char*>(CT2U(selectedCStr)));

		EDB::Data selectedPriKey;
		switch (edb.get_column_data_type(edb.get_primary_key_index()))
		{
		case EDB::DataType::INT: {
			selectedPriKey = std::stoll(selectedStr);
			break;
		}
		case EDB::DataType::REAL: {
			selectedPriKey = std::stod(selectedStr);
			break;
		}
		case EDB::DataType::TEXT: {
			selectedPriKey = selectedStr;
			break;
		}
		default:
			break;
		}

		if (edb.erase(selectedPriKey) == SUCCESS)
		{
			isEdited = true;
			mainListView.DeleteItem(selectedItem);
			selectedItem = mainListView.GetNextItem(selectedItem - 1, LVNI_SELECTED);
		}
		else
		{
			selectedItem = mainListView.GetNextItem(selectedItem, LVNI_SELECTED);
		}
	}
}

int CEasyDBExplorerDlg::PostRowToMainListView(const EDB::ERowView& row, int col = -1)
{
	return PostRowToListView(row, this->mainListView, col);
}

int CEasyDBExplorerDlg::InitMainListViewColumn()
{
	return InitListViewColumnByEDB(this->edb, this->mainListView);
}

int CEasyDBExplorerDlg::OpenDBFile(const std::string& dbFilePath)
{
	if (isEdited)
	{
		int retval = AfxMessageBox(TEXT("是否保存并打开新文件？"), MB_YESNOCANCEL);
		// 是（保存）
		if (retval == IDYES)
		{
			edb.save();
		}
		// 取消
		else if (retval == IDCANCEL)
		{
			return IDCANCEL;
		}
	}

	int retval = edb.open(dbFilePath);
	if (retval != SUCCESS)
	{
		// 打开失败
		CString errStr = TEXT("打开文件失败，错误代码：");
		errStr += EDB::StatusCode2String(retval);
		AfxMessageBox(errStr, MB_ICONWARNING | MB_OK);
		return -1;
	}

	// 打开成功
	// 清除已有的编辑框
	if (!editingBoxes.empty())
	{
		for (auto& editBox : editingBoxes)
		{
			delete editBox;
		}
		editingBoxes.clear();
	}
	// 清除编辑状态
	isEdited = false;
	// 初始化表头
	InitMainListViewColumn();
	for (auto&& row : edb)
	{
		PostRowToMainListView(row);
	}
	// 初始化列选择框
	retval = columnComboBox.DeleteString(0);
	while (retval != 0 && retval != CB_ERR)
	{
		retval = columnComboBox.DeleteString(0);
	}
	std::vector infos = edb.get_column_info();
	for (size_t i = 0; i < infos.size(); ++i)
	{
		columnComboBox.AddString(CU2T(infos[i].columnName.c_str()));
	}
	return 0;
}

// 打开
void CEasyDBExplorerDlg::OnOpenFileButtonClick()
{
	// TODO: 在此添加命令处理程序代码
	
	// 打开文件
	CFileDialog openFileDlg{TRUE,
							NULL,
							NULL,
							OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							TEXT("EasyDB文件 (*.db)|*.db|所有文件 (*.*)|*.*||")};
	if (openFileDlg.DoModal() == IDOK)
	{
		std::string dbFilePath = std::string(CT2A(openFileDlg.GetPathName()));
		OpenDBFile(dbFilePath);
	}
}

// 保存
void CEasyDBExplorerDlg::OnSaveButtonClick()
{
	// TODO: 在此添加命令处理程序代码
	int retval = edb.save();
	if (retval == SUCCESS)
	{
		isEdited = false;
	}
	else
	{
		CString errStr = TEXT("保存失败，错误代码：");
		errStr += EDB::StatusCode2String(retval);
		AfxMessageBox(errStr, MB_ICONWARNING | MB_OK);
	}
}

// 处理拖入文件
void CEasyDBExplorerDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	TCHAR filePath[MAX_PATH] = { 0 };
	DragQueryFile(hDropInfo, 0, filePath, MAX_PATH);
	std::string dbFilePath = std::string(CT2A(filePath));
	OpenDBFile(dbFilePath);

	CDialogEx::OnDropFiles(hDropInfo);
}

void CEasyDBExplorerDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (isEdited)
	{
		int retval = AfxMessageBox(TEXT("是否保存并退出程序？"), MB_YESNOCANCEL);
		// 是（保存）
		if (retval == IDYES)
		{
			edb.save();
		}
		// 取消
		else if (retval == IDCANCEL)
		{
			return;
		}
	}
	CDialogEx::OnClose();
}

void CEasyDBExplorerDlg::OnCreateDBButtonClick()
{
	// TODO: 在此添加命令处理程序代码
	CreateEasyDBDlg createDlg;
	if (createDlg.DoModal() == IDOK)
	{
		OpenDBFile(createDlg.dbFileName);
	}
}

// 搜索
void CEasyDBExplorerDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码

	UpdateData();
	// 获取输入
	std::string keyWord = std::string(CT2U(searchKeyWord));
	// 若没有输入，则重新显示所有数据
	if (keyWord.empty())
	{
		mainListView.DeleteAllItems();
		for (auto row : edb)
		{
			PostRowToMainListView(row);
		}
	}
	// 获取搜索的列
	int searchColIndex = columnComboBox.GetCurSel();
	// 不搜索BLOB类型的列
	if (edb.get_column_data_type(searchColIndex) == EDB::DataType::BLOB)
	{
		return;
	}
	
	// 清空ListView
	mainListView.DeleteAllItems();

	for (auto row : edb)
	{
		std::stringstream ss;
		std::visit([&ss](auto& d) { ss << d; }, row[searchColIndex]);
		std::string originStr = ss.str();

		if (originStr.find(keyWord) != std::string::npos)
		{
			PostRowToMainListView(row);
		}
	}
}
