#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	typedef struct _SSSDT_HOOK_ENTRY_INFORMATION
	{
		UINT32	    Ordinal;
		BOOL        bHooked;
		UINT_PTR	CurrentAddress;
		UINT_PTR	OriginalAddress;
		WCHAR	    wzFunctionName[100];
	} SSSDT_HOOK_ENTRY_INFORMATION, *PSSSDT_HOOK_ENTRY_INFORMATION;

	typedef struct _SSSDT_HOOK_INFORMATION
	{
		UINT32                         NumberOfSssdtFunctions;
		SSSDT_HOOK_ENTRY_INFORMATION   SssdtHookEntry[1];
	} SSSDT_HOOK_INFORMATION, *PSSSDT_HOOK_INFORMATION;



	class CSssdtHook
	{
	public:
		CSssdtHook(class CGlobal *GlobalObject);
		~CSssdtHook();

		void InitializeSssdtList(CListCtrl * ListCtrl);

		BOOL EnumSssdtHook();

		void InsertSssdtHookInfoList(CListCtrl * ListCtrl);

		void QuerySssdtHook(CListCtrl * ListCtrl);

		static DWORD CALLBACK QuerySssdtHookCallback(LPARAM lParam);

		BOOL ResumeSssdtHook(UINT32 Ordinal);

		static DWORD CALLBACK ResumeSssdtHookCallback(LPARAM lParam);

		static DWORD CALLBACK ResumeAllSssdtHookCallback(LPARAM lParam);



	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"���",					35 },
			{ L"��������",				145 },
			{ L"������ǰ��ַ",			125 },
			{ L"����ԭʼ��ַ",			125 },
			{ L"״̬",					55 },
			{ L"��ǰ��������ģ��",		195 } };

		std::vector<SSSDT_HOOK_ENTRY_INFORMATION> m_SssdtHookEntryVector;

		class CGlobal       *m_Global;

		class CDriverCore   &m_DriverCore;

		static CSssdtHook   *m_SssdtHook;

		static UINT32       m_SssdtFunctionCount;

	};



}