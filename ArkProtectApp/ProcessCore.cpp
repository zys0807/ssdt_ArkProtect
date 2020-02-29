#include "stdafx.h"
#include "ProcessCore.h"
#include "Global.hpp"
#include "ProcessDlg.h"

namespace ArkProtect
{
	CProcessCore *CProcessCore::m_Process;

	CProcessCore::CProcessCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_Process = this;
	}


	CProcessCore::~CProcessCore()
	{
	}


	/************************************************************************
	*  Name : InitializeProcessList
	*  Param: ProcessList           ProcessModule�Ի����ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CProcessCore::InitializeProcessList(CListCtrl *ProcessList)
	{
		ProcessList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ProcessList->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}


	/************************************************************************
	*  Name : GetProcessNum
	*  Param: void
	*  Ret  : UINT32
	*  �������㷢����Ϣ����ǰ���̸���
	************************************************************************/
	UINT32 CProcessCore::GetProcessNum()
	{
		BOOL   bOk = FALSE;
		DWORD  dwReturnLength = 0;
		UINT32 ProcessNum = 0;

		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_PROCESSNUM,
			NULL,		// InputBuffer
			0,
			&ProcessNum,
			sizeof(UINT32),
			&dwReturnLength,
			NULL);
		if (!bOk)
		{
			MessageBox(m_Global->m_ProcessDlg->m_hWnd, L"Get Process Count Failed", L"ArkProtect", MB_OK | MB_ICONERROR);
		}

		return ProcessNum;
	}


	/************************************************************************
	*  Name : GrantPriviledge
	*  Param: PriviledgeName		��Ҫ������Ȩ��
	*  Param: bEnable				����
	*  Ret  : BOOL
	*  ��������Ҫ��Ȩ�ޣ�Ȩ�����ƣ�
	************************************************************************/
	BOOL CProcessCore::GrantPriviledge(IN PWCHAR PriviledgeName, IN BOOL bEnable)
	{
		TOKEN_PRIVILEGES TokenPrivileges, OldPrivileges;
		DWORD			 dwReturnLength = sizeof(OldPrivileges);
		HANDLE			 TokenHandle = NULL;
		LUID			 uID;

		// ��Ȩ������
		if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &TokenHandle))
		{
			if (GetLastError() != ERROR_NO_TOKEN)
			{
				return FALSE;
			}
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))
			{
				return FALSE;
			}
		}

		if (!LookupPrivilegeValue(NULL, PriviledgeName, &uID))		// ͨ��Ȩ�����Ʋ���uID
		{
			CloseHandle(TokenHandle);
			TokenHandle = NULL;
			return FALSE;
		}

		TokenPrivileges.PrivilegeCount = 1;		// Ҫ������Ȩ�޸���
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;    // ��̬���飬�����С����Count����Ŀ
		TokenPrivileges.Privileges[0].Luid = uID;

		// ���������ǽ��е���Ȩ��
		if (!AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), &OldPrivileges, &dwReturnLength))
		{
			CloseHandle(TokenHandle);
			TokenHandle = NULL;
			return FALSE;
		}

		// �ɹ���
		CloseHandle(TokenHandle);
		TokenHandle = NULL;

		return TRUE;
	}


	/************************************************************************
	*  Name : QueryProcessUserAccess
	*  Param: ProcessId		         Ŀ�����id
	*  Ret  : BOOL
	*  ͨ���ܷ�򿪽��̾���ж��ܷ����û�����Ȩ��
	************************************************************************/
	BOOL CProcessCore::QueryProcessUserAccess(UINT32 ProcessId)
	{
		GrantPriviledge(SE_DEBUG_NAME, TRUE);

		HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_OPERATION, TRUE, ProcessId);

		GrantPriviledge(SE_DEBUG_NAME, FALSE);
		if (ProcessHandle)
		{
			CloseHandle(ProcessHandle);
			return TRUE;
		}

		return FALSE;
	}


	/************************************************************************
	*  Name : QueryPEFileBit
	*  Param: wzFilePath			����Id
	*  Ret  : BOOL
	*  �ж��Ƿ�Ϊ32Ϊ����(��ȡ�ļ��ķ�ʽ)
	************************************************************************/
	ePeBit CProcessCore::QueryPEFileBit(const WCHAR *wzFilePath)
	{
		ePeBit  PeBit = pb_Unknown;
		HANDLE	FileHandle = NULL;

		FileHandle = CreateFileW(wzFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (FileHandle != INVALID_HANDLE_VALUE)
		{
			DWORD dwReturnLength = 0;
			DWORD dwFileSize = GetFileSize(FileHandle, 0);
			PVOID FileBuffer = malloc(dwFileSize);
			if (FileBuffer)
			{
				BOOL bOk = ReadFile(FileHandle, FileBuffer, dwFileSize, &dwReturnLength, NULL);
				if (bOk)
				{
					PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
					if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
					{
						PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)FileBuffer + DosHeader->e_lfanew);
						if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
						{
							if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
							{
								PeBit = pb_32;
							}
							else if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64)
							{
								PeBit = pb_64;
							}
						}
					}
				}
				free(FileBuffer);
			}
			CloseHandle(FileHandle);
		}
		return PeBit;
	}


	/************************************************************************
	*  Name : PerfectProcessInfo
	*  Param: ProcessEntry			     ������Ϣ�ṹ
	*  Ret  : void
	*  ���ƽ�����Ϣ�ṹ
	************************************************************************/
	void CProcessCore::PerfectProcessInfo(PPROCESS_ENTRY_INFORMATION ProcessEntry)
	{
		if (ProcessEntry->ProcessId == 0)
		{
			// Idle
			StringCchCopyW(ProcessEntry->wzImageName, wcslen(L"System Idle Process") + 1, L"System Idle Process");
			StringCchCopyW(ProcessEntry->wzFilePath, wcslen(L"System Idle Process") + 1, L"System Idle Process");
			ProcessEntry->bUserAccess = FALSE;
			ProcessEntry->ParentProcessId = 0xffffffff;
			StringCchCopyW(ProcessEntry->wzCompanyName, wcslen(L" ") + 1, L" ");
		}
		else if (ProcessEntry->ProcessId == 4)
		{
			// System
			StringCchCopyW(ProcessEntry->wzImageName, wcslen(L"System") + 1, L"System");

			WCHAR wzFilePath[MAX_PATH] = { 0 };
			GetSystemDirectory(wzFilePath, MAX_PATH);      // ���System32Directory
			StringCchCatW(wzFilePath, MAX_PATH, L"\\ntoskrnl.exe");

			StringCchCopyW(ProcessEntry->wzFilePath, wcslen(wzFilePath) + 1, wzFilePath);
			ProcessEntry->bUserAccess = FALSE;
			ProcessEntry->ParentProcessId = 0xffffffff;
			StringCchCopyW(ProcessEntry->wzCompanyName,
				m_Global->GetFileCompanyName(ProcessEntry->wzFilePath).GetLength() + 1,
				m_Global->GetFileCompanyName(ProcessEntry->wzFilePath).GetBuffer());
		}
		else
		{
			// Others
			WCHAR *wzImageName = NULL;
			wzImageName = wcsrchr(ProcessEntry->wzFilePath, '\\');
			wzImageName++;  // �� '\\'

			StringCchCopyW(ProcessEntry->wzImageName, wcslen(wzImageName) + 1, wzImageName);

			if (QueryProcessUserAccess(ProcessEntry->ProcessId) == TRUE)
			{
				ProcessEntry->bUserAccess = TRUE;
			}
			else
			{
				ProcessEntry->bUserAccess = FALSE;
			}

			CString strCompanyName = m_Global->GetFileCompanyName(ProcessEntry->wzFilePath);
			if (strCompanyName.GetLength() == 0)
			{
				strCompanyName = L" ";
			}

			StringCchCopyW(ProcessEntry->wzCompanyName, strCompanyName.GetLength() + 1, strCompanyName.GetBuffer());

#ifdef _WIN64
			// ֻ��Ҫ�ж�32λ����
			if (QueryPEFileBit(ProcessEntry->wzFilePath) == pb_32)
			{
				StringCchCatW(ProcessEntry->wzImageName, 100, L" *32");
			}
#endif // _WIN64

		}
	}


	/************************************************************************
	*  Name : EnumProcessInfo
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ��̼������Ϣ
	************************************************************************/
	BOOL CProcessCore::EnumProcessInfo()
	{
		BOOL bOk = FALSE;

		m_ProcessEntryVector.clear();

		// ��΢�е�Ӱ��Ч�ʣ�����ȡ�������̽
		// ����������̽һ���ж��ٸ�����
/*		UINT32 ProcessNum = GetProcessNum();
		if (ProcessNum == 0)
		{
			return FALSE;
		}

		UINT32   Count = ProcessNum + 0x100;*/
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_INFORMATION pi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pi)
			{
				free(pi);
				pi = NULL;
			}

			OutputLength = sizeof(PROCESS_INFORMATION) + Count * sizeof(PROCESS_ENTRY_INFORMATION);

			pi = (PPROCESS_INFORMATION)malloc(OutputLength);
			if (!pi)
			{
				break;
			}

			RtlZeroMemory(pi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESS,
				NULL,		// InputBuffer
				0,
				pi,
				OutputLength,
				&dwReturnLength,
				NULL);

			// ������do whileѭ����ǡ���ô�����������ڴ治�����ظ�ö�٣�����㹻��һ��ͨ��
			Count = (UINT32)pi->NumberOfProcesses + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pi)
		{
			for (UINT32 i = 0; i < pi->NumberOfProcesses; i++)
			{
				// ���ƽ�����Ϣ�ṹ
				PerfectProcessInfo(&pi->ProcessEntry[i]);
				m_ProcessEntryVector.push_back(pi->ProcessEntry[i]);
			}
			bOk = TRUE;
		}

		if (pi)
		{
			free(pi);
			pi = NULL;
		}

		if (m_ProcessEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : AddProcessFileIcon
	*  Param: wzProcessPath
	*  Ret  : void
	*  ��ѯ������Ϣ�Ļص�
	************************************************************************/
	void CProcessCore::AddProcessFileIcon(WCHAR *wzProcessPath)
	{
		SHFILEINFO ShFileInfo = { 0 };

		SHGetFileInfo(wzProcessPath, FILE_ATTRIBUTE_NORMAL,
			&ShFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);

		HICON  hIcon = ShFileInfo.hIcon;

		((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessIconList.Add(hIcon);
	}



	/************************************************************************
	*  Name : InsertProcessInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CProcessCore::InsertProcessInfoList(CListCtrl *ListCtrl)
	{
		UINT32 ProcessNum = 0;
		size_t Size = m_ProcessEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_ENTRY_INFORMATION ProcessEntry = m_ProcessEntryVector[i];

			CString strImageName, strProcessId, strParentProcessId, strFilePath, strEProcess, strUserAccess, strCompanyName;

			strImageName = ProcessEntry.wzImageName;
			
			strProcessId.Format(L"%d", ProcessEntry.ProcessId);
			
			if (ProcessEntry.ParentProcessId == 0xffffffff)
			{
				strParentProcessId = L"-";
			}
			else
			{
				strParentProcessId.Format(L"%d", ProcessEntry.ParentProcessId);
			}
			
			strFilePath = ProcessEntry.wzFilePath;
			
			strEProcess.Format(L"0x%p", ProcessEntry.EProcess);
			
			if (ProcessEntry.bUserAccess == TRUE)
			{
				strUserAccess = L"����";
			}
			else
			{
				strUserAccess = L"�ܾ�";
			}

			strCompanyName = ProcessEntry.wzCompanyName;

			// ���ͼ��
			m_Global->AddFileIcon(ProcessEntry.wzFilePath, &((CProcessDlg*)m_Global->m_ProcessDlg)->m_ProcessIconList);

			int iImageCount = ((CProcessDlg*)m_Global->m_ProcessDlg)->m_ProcessIconList.GetImageCount() - 1;
			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strImageName, iImageCount);
			ListCtrl->SetItemText(iItem, pc_ProcessId, strProcessId);
			ListCtrl->SetItemText(iItem, pc_ParentProcessId, strParentProcessId);
			ListCtrl->SetItemText(iItem, pc_FilePath, strFilePath);
			ListCtrl->SetItemText(iItem, pc_EProcess, strEProcess);
			ListCtrl->SetItemText(iItem, pc_UserAccess, strUserAccess);
			ListCtrl->SetItemText(iItem, pc_Company, strCompanyName);

			if ((i != 0) && (_wcsnicmp(ProcessEntry.wzCompanyName, L"Microsoft Corporation", wcslen(L"Microsoft Corporation")) != 0))
			{
				ListCtrl->SetItemData(iItem, TRUE);
			}

			ProcessNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Process Info is loading now, Count:%d", ProcessNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Process Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QueryProcessInfo
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CProcessCore::QueryProcessInfo(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessEntryVector.clear();

		if (EnumProcessInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Info Initialize failed");
			return;
		}

		InsertProcessInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryProcessInfoCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ������Ϣ�Ļص�
	************************************************************************/
	DWORD CALLBACK CProcessCore::QueryProcessInfoCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_Process->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_Process->m_Global->UpdateStatusBarTip(L"Process Info");
		m_Process->m_Global->UpdateStatusBarDetail(L"Process Info is loading now...");
		
		m_Process->QueryProcessInfo(ListCtrl);

		m_Process->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : TerminateProcess
	*  Param: lParam ��ListCtrl��
	*  Param: bForce                �Ƿ���
	*  Ret  : DWORD
	*  ����API��TerminateProcess ���� ����ɾ��
	************************************************************************/
	void CProcessCore::TerminateProcess(CListCtrl *ListCtrl, BOOL bForce)
	{
		POSITION Pos = ListCtrl->GetFirstSelectedItemPosition();

		while (Pos)
		{
			int iItem = ListCtrl->GetNextSelectedItem(Pos);

			UINT32 ProcessId = _ttoi(ListCtrl->GetItemText(iItem, pc_ProcessId).GetBuffer());
			if (ProcessId <= 4)
			{
				return;
			}

			BOOL bOk = FALSE;

			if (bForce)
			{
				// �����������̣���������ͨ��

				DWORD dwReturnLength = 0;

				bOk = DeviceIoControl(m_Global->m_DeviceHandle,
					IOCTL_ARKPROTECT_TERMINATEPROCESS,
					&ProcessId,		// InputBuffer
					sizeof(UINT32),
					NULL,
					0,
					&dwReturnLength,
					NULL);

			}
			else
			{
				// ����API
				GrantPriviledge(SE_DEBUG_NAME, TRUE);

				HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_OPERATION, TRUE, ProcessId);

				bOk = ::TerminateProcess(ProcessHandle, 0);

				GrantPriviledge(SE_DEBUG_NAME, FALSE);

				CloseHandle(ProcessHandle);
			}
			
	//		if (bOk)
			{
				ListCtrl->DeleteItem(iItem);

				// ����Vector ɾ��ָ���ĳ�Ա

				size_t Size = m_ProcessEntryVector.size();
				for (size_t i = 0; i < Size; i++)
				{
					if (ProcessId == m_ProcessEntryVector[i].ProcessId)
					{
						m_ProcessEntryVector.erase(m_ProcessEntryVector.begin() + i);
						break;
					}
				}

				// ����status
				CString strStatusContext;
				strStatusContext.Format(L"Process Info load complete, Count:%d", Size - 1);
				m_Global->UpdateStatusBarDetail(strStatusContext);

			}
		//	else
			{
				// ������ܻ����
				//MessageBox(m_Global->m_ProcessDlg->m_hWnd, L"���̹ر�ʧ�ܡ�", L"ArkProtect", MB_OK | MB_ICONERROR);
			}	
		}
	}
	

	/************************************************************************
	*  Name : TerminateProcessCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ����Ŀ�����
	************************************************************************/
	DWORD CALLBACK CProcessCore::TerminateProcessCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_Process->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_Process->TerminateProcess(ListCtrl, FALSE);

		m_Process->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : ForceTerminateProcessCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��������Ŀ�����
	************************************************************************/
	DWORD CALLBACK CProcessCore::ForceTerminateProcessCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_Process->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_Process->TerminateProcess(ListCtrl, TRUE);

		m_Process->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}
}


