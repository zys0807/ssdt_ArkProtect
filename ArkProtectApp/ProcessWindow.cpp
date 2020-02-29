#include "stdafx.h"
#include "ProcessWindow.h"
#include "Global.hpp"
#include "ProcessDlg.h"

namespace ArkProtect
{
	CProcessWindow *CProcessWindow::m_ProcessWindow;

	CProcessWindow::CProcessWindow(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_ProcessWindow = this;
	}


	CProcessWindow::~CProcessWindow()
	{
	}


	/************************************************************************
	*  Name : InitializeProcessWindowList
	*  Param: ProcessInfoList        ProcessModule�Ի����ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CProcessWindow::InitializeProcessWindowList(CListCtrl *ListCtrl)
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
	*  Name : EnumProcessWindow
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ���ģ����Ϣ
	************************************************************************/
	BOOL CProcessWindow::EnumProcessWindow()
	{
		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_WINDOW_INFORMATION pwi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pwi)
			{
				free(pwi);
				pwi = NULL;
			}

			OutputLength = sizeof(PROCESS_WINDOW_INFORMATION) + Count * sizeof(PROCESS_WINDOW_ENTRY_INFORMATION);

			pwi = (PPROCESS_WINDOW_INFORMATION)malloc(OutputLength);
			if (!pwi)
			{
				break;
			}

			RtlZeroMemory(pwi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSWINDOW,
				&m_Global->ProcessCore().ProcessEntry()->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pwi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pwi->NumberOfWindows + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pwi)
		{
			for (UINT32 i = 0; i < pwi->NumberOfWindows; i++)
			{
				m_ProcessWindowEntryVector.push_back(pwi->WindowEntry[i]);
			}
			bOk = TRUE;
		}

		if (pwi)
		{
			free(pwi);
			pwi = NULL;
		}

		if (m_ProcessWindowEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertProcessWindowInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CProcessWindow::InsertProcessWindowInfoList(CListCtrl *ListCtrl)
	{
		size_t Size = m_ProcessWindowEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_WINDOW_ENTRY_INFORMATION WindowEntry = m_ProcessWindowEntryVector[i];

			CString strhWnd, strWindowsText, strClassName, strVisable, strProcessId, strThreadId;

			WCHAR wzWindowsText[MAX_PATH] = { 0 };
			WCHAR wzClassName[MAX_PATH] = { 0 };
			
			::GetWindowText(WindowEntry.hWnd, wzWindowsText, MAX_PATH);
			::GetClassName(WindowEntry.hWnd, wzClassName, MAX_PATH);

			strhWnd.Format(L"0x%08X", WindowEntry.hWnd); 
			strWindowsText = wzWindowsText;
			strClassName = wzClassName;
			strProcessId.Format(L"%d", WindowEntry.ProcessId);
			strThreadId.Format(L"%d", WindowEntry.ThreadId);
			
			if (::IsWindowVisible(WindowEntry.hWnd))
			{
				strVisable = L"�ɼ�";
			}
			else
			{
				strVisable = L"-";
			}

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strhWnd);
			ListCtrl->SetItemText(iItem, pwc_WindowText, strWindowsText);
			ListCtrl->SetItemText(iItem, pwc_WindowClass, strClassName);
			ListCtrl->SetItemText(iItem, pwc_WindowVisibal, strVisable);
			ListCtrl->SetItemText(iItem, pwc_ProcessId, strProcessId);
			ListCtrl->SetItemText(iItem, pwc_ThreadId, strThreadId);
		}

		CString strNum;
		strNum.Format(L"%d", Size);
		((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(strNum);
	}


	/************************************************************************
	*  Name : QueryProcessModule
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CProcessWindow::QueryProcessWindow(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessWindowEntryVector.clear();

		if (EnumProcessWindow() == FALSE)
		{
			((CProcessDlg*)(m_Global->m_ProcessDlg))->m_ProcessInfoDlg->APUpdateWindowText(L"Process Window Initialize failed");
			return;
		}

		InsertProcessWindowInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryProcessWindowCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ����ģ��Ļص�
	************************************************************************/
	DWORD CALLBACK CProcessWindow::QueryProcessWindowCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessWindow->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_ProcessWindow->QueryProcessWindow(ListCtrl);

		m_ProcessWindow->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}