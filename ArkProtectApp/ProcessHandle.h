#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{

	enum eProcessHandleColumn
	{
		phc_HandleType,
		phc_HandleName,
		phc_HandleValue,
		phc_Object,
		phc_ReferenceCount
	};


	typedef struct _PROCESS_HANDLE_ENTRY_INFORMATION
	{
		HANDLE		Handle;
		PVOID		Object;
		UINT32		ReferenceCount;		    // ���ü���
		WCHAR		wzHandleType[MAX_PATH];
		WCHAR		wzHandleName[MAX_PATH];
	} PROCESS_HANDLE_ENTRY_INFORMATION, *PPROCESS_HANDLE_ENTRY_INFORMATION;

	typedef struct _PROCESS_HANDLE_INFORMATION
	{
		UINT32								NumberOfHandles;
		PROCESS_HANDLE_ENTRY_INFORMATION	HandleEntry[1];
	} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;


	class CProcessHandle
	{
	public:
		CProcessHandle(class CGlobal *GlobalObject);
		~CProcessHandle();

		void InitializeProcessHandleList(CListCtrl * ListCtrl);

		BOOL EnumProcessHandle();

		void InsertProcessHandleInfoList(CListCtrl * ListCtrl);

		void QueryProcessHandle(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessHandleCallback(LPARAM lParam);


	private:
		int           m_iColumnCount = 5;		// �����б���
		COLUMN_STRUCT m_ColumnStruct[5] = {
			{ L"�������",				125 },
			{ L"�������",				205 },
			{ L"���",					60 },
			{ L"�������",				130 },
			{ L"���ü���",				70 } };



		std::vector<PROCESS_HANDLE_ENTRY_INFORMATION> m_ProcessHandleEntryVector;


		class CGlobal         *m_Global;

		static CProcessHandle *m_ProcessHandle;


	};



}

