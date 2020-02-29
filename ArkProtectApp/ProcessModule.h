#pragma once
#include <vector>
#include "Define.h"


namespace ArkProtect
{
	enum eProcessModuleColumn
	{
		pmc_FilePath,        // ��������·��
		pmc_BaseAddress,     // ģ�����ַ
		pmc_SizeOfImage,     // ģ���С
		pmc_Company          // �ļ�����
	};

	typedef struct _PROCESS_MODULE_ENTRY_INFORMATION
	{
		UINT_PTR	BaseAddress;
		UINT_PTR	SizeOfImage;
		WCHAR	    wzFilePath[MAX_PATH];
		WCHAR       wzCompanyName[MAX_PATH];
	} PROCESS_MODULE_ENTRY_INFORMATION, *PPROCESS_MODULE_ENTRY_INFORMATION;

	typedef struct _PROCESS_MODULE_INFORMATION
	{
		UINT32                           NumberOfModules;
		PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry[1];
	} PROCESS_MODULE_INFORMATION, *PPROCESS_MODULE_INFORMATION;



	class CProcessModule
	{
	public:
		CProcessModule(class CGlobal *GlobalObject);
		~CProcessModule();
		void InitializeProcessModuleList(CListCtrl * ListCtrl);

		void PerfectProcessModuleInfo(PPROCESS_MODULE_ENTRY_INFORMATION ModuleEntry);

		BOOL EnumProcessModule();

		void InsertProcessModuleInfoList(CListCtrl * ListCtrl);

		void QueryProcessModule(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessModuleCallback(LPARAM lParam);

		//
		// ���ر���Interface
		//
		inline std::vector<PROCESS_MODULE_ENTRY_INFORMATION>& ProcessModuleEntryVector() { return m_ProcessModuleEntryVector; }


	private:
		int           m_iColumnCount = 4;		// �����б���
		COLUMN_STRUCT m_ColumnStruct[4] = {
			{ L"ģ������",				270 },
			{ L"����ַ",				125 },
			{ L"��С",					125 },
			{ L"�ļ�����",				125 } };



		std::vector<PROCESS_MODULE_ENTRY_INFORMATION> m_ProcessModuleEntryVector;


		class CGlobal         *m_Global;

		static CProcessModule *m_ProcessModule;


	};



	
}

