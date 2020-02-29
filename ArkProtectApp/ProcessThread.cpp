#include "stdafx.h"
#include "ProcessThread.h"
#include "Global.hpp"
#include "ProcessModule.h"
#include "ProcessDlg.h"

namespace ArkProtect
{
	CProcessThread *CProcessThread::m_ProcessThread;

	CProcessThread::CProcessThread(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_ProcessModule(GlobalObject->ProcessModule())
	{
		m_ProcessThread = this;
	}


	CProcessThread::~CProcessThread()
	{
	}


	/************************************************************************
	*  Name : InitializeProcessThreadList
	*  Param: ProcessInfoList        ProcessThread�Ի����ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CProcessThread::InitializeProcessThreadList(CListCtrl *ListCtrl)
	{
		while (ListCtrl->DeleteColumn(0));
		ListCtrl->DeleteAllItems();

		ListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ListCtrl->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}




	/************************************************************************
	*  Name : EnumProcessThread
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ���ģ����Ϣ
	************************************************************************/
	BOOL CProcessThread::EnumProcessThread()
	{
		m_ProcessThreadEntryVector.clear();
		
		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_THREAD_INFORMATION pti = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pti)
			{
				free(pti);
				pti = NULL;
			}

			OutputLength = sizeof(PROCESS_THREAD_INFORMATION) + Count * sizeof(PROCESS_THREAD_ENTRY_INFORMATION);

			pti = (PPROCESS_THREAD_INFORMATION)malloc(OutputLength);
			if (!pti)
			{
				break;
			}

			RtlZeroMemory(pti, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSTHREAD,
				&m_Global->ProcessCore().ProcessEntry()->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pti,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pti->NumberOfThreads + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pti)
		{
			for (UINT32 i = 0; i < pti->NumberOfThreads; i++)
			{
				m_ProcessThreadEntryVector.push_back(pti->ThreadEntry[i]);
			}
			bOk = TRUE;
		}

		if (pti)
		{
			free(pti);
			pti = NULL;
		}

		if (m_ProcessThreadEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	CString CProcessThread::GetModulePathByThreadStartAddress(UINT_PTR StartAddress)
	{
		CString strModulePath = L"";

		size_t Size = m_ProcessModule.ProcessModuleEntryVector().size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry = m_ProcessModule.ProcessModuleEntryVector()[i];
			if (StartAddress >= ModuleEntry.BaseAddress && StartAddress <= (ModuleEntry.BaseAddress + ModuleEntry.SizeOfImage))
			{
				strModulePath = ModuleEntry.wzFilePath;
			}	
		}

		// ���������ѭ�� ��˵�����ں�ģ��
		return strModulePath;
	}


	/************************************************************************
	*  Name : InsertProcessInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CProcessThread::InsertProcessThreadInfoList(CListCtrl *ListCtrl)
	{
		size_t Size = m_ProcessThreadEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_THREAD_ENTRY_INFORMATION ThreadEntry = m_ProcessThreadEntryVector[i];

			CString strThreadId, strEThread, strTeb, strPriority, strWin32StartAddress, strContextSwitches, strState, strModulePath;

			strThreadId.Format(L"%d", ThreadEntry.ThreadId);
			strEThread.Format(L"0x%08p", ThreadEntry.EThread);
			if (ThreadEntry.Teb == 0)
			{
				strTeb = L"-";
			}
			else
			{
				strTeb.Format(L"0x%08p", ThreadEntry.Teb);
			}

			strPriority.Format(L"%d", ThreadEntry.Priority);
			strWin32StartAddress.Format(L"0x%08p", ThreadEntry.Win32StartAddress);
			strContextSwitches.Format(L"%d", ThreadEntry.ContextSwitches);

			strModulePath = GetModulePathByThreadStartAddress(ThreadEntry.Win32StartAddress);

			if (strModulePath.GetLength() <= 1)
			{
				strModulePath = L"\\ ";
			}

			WCHAR *Temp = NULL;

			Temp = wcsrchr(strModulePath.GetBuffer(), L'\\');

			if (Temp != NULL)
			{
				Temp++;
			}

			strModulePath = Temp;

			switch (ThreadEntry.State)
			{
			case Initialized:
			{
				strState = L"Ԥ��";
				break;
			}
			case Ready:
			{
				strState = L"����";
				break;
			}
			case Running:
			{
				strState = L"����";
				break;
			}
			case Standby:
			{
				strState = L"����";
				break;
			}
			case Terminated:
			{
				strState = L"��ֹ";
				break;
			}
			case Waiting:
			{
				strState = L"�ȴ�";
				break;
			}
			case Transition:
			{
				strState = L"����";
				break;
			}
			case DeferredReady:
			{
				strState = L"�ӳپ���";
				break;
			}
			case GateWait:
			{
				strState = L"�ŵȴ�";
				break;
			}
			default:
				strState = L"δ֪";
				break;
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strThreadId);
			ListCtrl->SetItemText(iItem, ptc_EThread, strEThread);
			ListCtrl->SetItemText(iItem, ptc_Teb, strTeb);
			ListCtrl->SetItemText(iItem, ptc_Priority, strPriority);
			ListCtrl->SetItemText(iItem, ptc_Entrance, strWin32StartAddress);
			ListCtrl->SetItemText(iItem, ptc_Module, strModulePath);
			ListCtrl->SetItemText(iItem, ptc_switches, strContextSwitches);
			ListCtrl->SetItemText(iItem, ptc_Status, strState);
			
			ListCtrl->SetItemData(iItem, iItem);
		}

		CString strNum;
		strNum.Format(L"%d", Size);
		((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(strNum);
	}




	/************************************************************************
	*  Name : QueryProcessThread
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CProcessThread::QueryProcessThread(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessThreadEntryVector.clear();
		m_ProcessModule.ProcessModuleEntryVector().clear();

		if (EnumProcessThread() == FALSE)
		{
			((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(L"Process Thread Initialize failed");
			return;
		}

		if (m_ProcessModule.EnumProcessModule() == FALSE)
		{
			((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(L"Process Thread Initialize failed");
			return;
		}

		InsertProcessThreadInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryProcessThreadCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ����ģ��Ļص�
	************************************************************************/
	DWORD CALLBACK CProcessThread::QueryProcessThreadCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessThread->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_ProcessThread->QueryProcessThread(ListCtrl);

		m_ProcessThread->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : GetThreadIdByProcessId
	*  Param: ProcessId			����Id		��IN��
	*  Param: ThreadId			�߳�Id		��OUT��
	*  Ret  : BOOL
	*  ZwQuerySystemInformation+SystemProcessInformation��ý��������Ϣ�Ӷ��õ�һ���߳�Id
	************************************************************************/

	BOOL CProcessThread::GetThreadIdByProcessId(UINT32 ProcessId, PUINT32 ThreadId)
	{
		BOOL						bOk = FALSE;
		NTSTATUS					Status = 0;
		PVOID						BufferData = NULL;
		PSYSTEM_PROCESS_INFO		spi = NULL;
		pfnZwQuerySystemInformation ZwQuerySystemInformation = NULL;

		ZwQuerySystemInformation = (pfnZwQuerySystemInformation)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "ZwQuerySystemInformation");
		if (ZwQuerySystemInformation == NULL)
		{
			return FALSE;
		}

		BufferData = malloc(1024 * 1024);
		if (!BufferData)
		{
			return FALSE;
		}

		// ��QuerySystemInformationϵ�к����У���ѯSystemProcessInformationʱ��������ǰ������ڴ棬�����Ȳ�ѯ�õ����������µ���
		Status = ZwQuerySystemInformation(SystemProcessInformation, BufferData, 1024 * 1024, NULL);
		if (!NT_SUCCESS(Status))
		{
			free(BufferData);
			return FALSE;
		}

		spi = (PSYSTEM_PROCESS_INFO)BufferData;

		// ��������
		while (TRUE)
		{
			bOk = FALSE;
			if (spi->UniqueProcessId == (HANDLE)ProcessId)
			{
				bOk = TRUE;
				break;
			}
			else if (spi->NextEntryOffset)
			{
				spi = (PSYSTEM_PROCESS_INFO)((PUINT8)spi + spi->NextEntryOffset);
			}
			else
			{
				break;
			}
		}

		if (bOk)
		{
			for (INT i = 0; i < spi->NumberOfThreads; i++)
			{
				// ִ�еĲ����ǵ�ǰ�߳�
				*ThreadId = (UINT32)spi->Threads[i].ClientId.UniqueThread;
				break;
			}
		}

		if (BufferData != NULL)
		{
			free(BufferData);
		}

		return bOk;
	}


}