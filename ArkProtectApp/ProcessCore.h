#pragma once

#include <vector>
#include <strsafe.h>
#include "Define.h"


namespace ArkProtect
{
	enum eProcessColumn
	{
		pc_ImageName,     // ��������
		pc_ProcessId,     // ����ID
		pc_ParentProcessId,// ������ID		
		pc_FilePath,      // ��������·��
		pc_EProcess,      // ���̽ṹ��
		pc_UserAccess,    // Ӧ�ò����
		pc_Company        // �ļ�����
	};

	typedef struct _PROCESS_ENTRY_INFORMATION
	{
		WCHAR     wzImageName[100];
		UINT32	  ProcessId;
		UINT32	  ParentProcessId;
		WCHAR     wzFilePath[MAX_PATH];
		UINT_PTR  EProcess;
		BOOL      bUserAccess;
		WCHAR     wzCompanyName[MAX_PATH];
	} PROCESS_ENTRY_INFORMATION, *PPROCESS_ENTRY_INFORMATION;

	typedef struct _PROCESS_INFORMATION
	{
		UINT32                    NumberOfProcesses;
		PROCESS_ENTRY_INFORMATION ProcessEntry[1];
	} PROCESS_INFORMATION, *PPROCESS_INFORMATION;


	class CProcessCore
	{
	public:
		CProcessCore(class CGlobal *GlobalObject);
		~CProcessCore();

		void InitializeProcessList(CListCtrl *ProcessList);

		UINT32 GetProcessNum();

		BOOL GrantPriviledge(IN PWCHAR PriviledgeName, IN BOOL bEnable);

		BOOL QueryProcessUserAccess(UINT32 ProcessId);

		ePeBit QueryPEFileBit(const WCHAR * wzFilePath);

		void PerfectProcessInfo(PPROCESS_ENTRY_INFORMATION ProcessEntry);

		BOOL EnumProcessInfo();

		void AddProcessFileIcon(WCHAR * wzProcessPath);

		void InsertProcessInfoList(CListCtrl * ListCtrl);

		void QueryProcessInfo(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessInfoCallback(LPARAM lParam);

		void TerminateProcess(CListCtrl * ListCtrl, BOOL bForce);

		static DWORD CALLBACK TerminateProcessCallback(LPARAM lParam);

		static DWORD CALLBACK ForceTerminateProcessCallback(LPARAM lParam);



		//
		// ���ر���Interface
		//
		
		inline PPROCESS_ENTRY_INFORMATION& ProcessEntry() { return m_ProcessEntry; }

		
	private:
		
		UINT32        m_ProcessCount = 0;
		int           m_iColumnCount = 7;		// �����б���
		COLUMN_STRUCT m_ColumnStruct[7] = {
			{ L"ӳ������",			160 },
			{ L"����ID",			65 },
			{ L"������ID",			65 },
			{ L"ӳ��·��",			230 },
			{ L"EPROCESS",			125 },
			{ L"Ӧ�ò����",		75 },
			{ L"�ļ�����",			122 }};


		std::vector<PROCESS_ENTRY_INFORMATION> m_ProcessEntryVector;
		
		PPROCESS_ENTRY_INFORMATION m_ProcessEntry;

		class CGlobal       *m_Global;
		static CProcessCore *m_Process;
		
	};

	

}

