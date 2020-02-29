#pragma once
#include <vector>
#include <strsafe.h>
#include "Define.h"


namespace ArkProtect
{

	enum eRegistryColumn
	{
		rc_Name,     // ����
		rc_Type,     // ����
		rc_Data      // ����
	};


	class CRegistryCore
	{
	public:
		CRegistryCore(class CGlobal *GlobalObject);
		~CRegistryCore();

		void InitializeRegistryTree(CTreeCtrl * RegistryTree);

		void InitializeRegistryList(CListCtrl * RegistryList);





	private:
		int           m_iListColumnCount = 3;		
		COLUMN_STRUCT m_ListColumnStruct[3] = {
			{ L"����",			200 },
			{ L"����",			130 },
			{ L"����",			590 }};


		class CGlobal       *m_Global;

	};

}
