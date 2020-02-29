#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{

#define RESUME_ALL_HOOKS 8080

	enum eSsdtHookColumn
	{
		shc_Ordinal,         // ���
		shc_FunctionName,    // ��������
		shc_CurrentAddress,  // ������ǰ��ַ
		shc_OriginalAddress, // ����ԭʼ��ַ
		shc_Status,          // ״̬
		shc_FilePath         // ��ǰ��������ģ��
	};

	typedef struct _SSDT_HOOK_ENTRY_INFORMATION
	{
		UINT32	    Ordinal;
		BOOL        bHooked;
		UINT_PTR	CurrentAddress;
		UINT_PTR	OriginalAddress;
		WCHAR	    wzFunctionName[100];
	} SSDT_HOOK_ENTRY_INFORMATION, *PSSDT_HOOK_ENTRY_INFORMATION;

	typedef struct _SSDT_HOOK_INFORMATION
	{
		UINT32                        NumberOfSsdtFunctions;
		SSDT_HOOK_ENTRY_INFORMATION   SsdtHookEntry[1];
	} SSDT_HOOK_INFORMATION, *PSSDT_HOOK_INFORMATION;


	class CSsdtHook
	{
	public:
		CSsdtHook(class CGlobal *GlobalObject);
		~CSsdtHook();

		void InitializeSsdtList(CListCtrl * ListCtrl);

		BOOL EnumSsdtHook();

		void InsertSsdtHookInfoList(CListCtrl * ListCtrl);

		void QuerySsdtHook(CListCtrl * ListCtrl);

		static DWORD CALLBACK QuerySsdtHookCallback(LPARAM lParam);

		BOOL ResumeSsdtHook(UINT32 Ordinal);

		static DWORD CALLBACK ResumeSsdtHookCallback(LPARAM lParam);

		static DWORD CALLBACK ResumeAllSsdtHookCallback(LPARAM lParam);


	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"���",					35 },
			{ L"��������",				145 },
			{ L"������ǰ��ַ",			125 },
			{ L"����ԭʼ��ַ",			125 },
			{ L"״̬",					55 },
			{ L"��ǰ��������ģ��",		195 } };

		std::vector<SSDT_HOOK_ENTRY_INFORMATION> m_SsdtHookEntryVector;

		class CGlobal      *m_Global;

		class CDriverCore  &m_DriverCore;

		static CSsdtHook   *m_SsdtHook;

		static UINT32      m_SsdtFunctionCount;
	};
}

