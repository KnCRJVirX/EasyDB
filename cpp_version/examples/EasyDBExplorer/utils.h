#pragma once

#include "EasyDB.hpp"

#define CU2T(str) CA2T(str, CP_UTF8)
#define CT2U(str) CT2A(str, CP_UTF8)

#define PRIMARY_KEY_IS_BLOB(pEasyDB) ((pEasyDB)->get_column_data_type((pEasyDB)->get_primary_key_index()) == EDB::DataType::BLOB)
#define IF_PRIMARY_KEY_IS_BLOB(pEasyDB) if(PRIMARY_KEY_IS_BLOB(pEasyDB))

static inline int PostRowToListView(const EDB::ERowView& row, CListCtrl& listView, int insertPos = -1)
{
	if (insertPos == -1)
	{
		insertPos = listView.GetItemCount();
	}

	for (int col = 0; col < row.size(); ++col)
	{
		// 格式化
		std::string text;
		if (EDB::GetDataType(row[col]) == EDB::DataType::BLOB)
		{
			text = "<Blob>";
		}
		else
		{
			std::stringstream ss;
			std::visit([&ss](auto& data) { ss << data; }, row[col]);
			text = ss.str();
		}

		// 推送到ListView
		if (col == 0)
		{
			listView.InsertItem(insertPos, CString(CU2T(text.c_str())));
		}
		else
		{
			listView.SetItemText(insertPos, col, CString(CU2T(text.c_str())));
		}
	}
	return 0;
}

static inline int InitListViewColumnByEDB(EDB::EasyDB& edb, CListCtrl& listView)
{
	// 清空行
	listView.DeleteAllItems();
	// 清空列
	BOOL retval;
	do
	{
		retval = listView.DeleteColumn(0);
	} while (retval);

	// 获取列信息
	std::vector infos = edb.get_column_info();
	for (int i = 0; i < infos.size(); ++i)
	{
		listView.InsertColumn(i, CString(CU2T(infos[i].columnName.c_str())), LVCFMT_LEFT, 200);
	}
	return 0;
}