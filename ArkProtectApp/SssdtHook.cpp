#include "stdafx.h"
#include "SssdtHook.h"
#include "Global.hpp"
#include "HookDlg.h"

namespace ArkProtect
{
	CSssdtHook *CSssdtHook::m_SssdtHook;
	UINT32     CSssdtHook::m_SssdtFunctionCount;

	CSssdtHook::CSssdtHook(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_DriverCore(GlobalObject->DriverCore())
	{
		m_SssdtHook = this;
		m_SssdtFunctionCount = 0;
	}


	CSssdtHook::~CSssdtHook()
	{
	}


	/************************************************************************
	*  Name : InitializeSssdtList
	*  Param: ListCtrl               ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CSssdtHook::InitializeSssdtList(CListCtrl *ListCtrl)
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
	*  Name : EnumSssdtHook
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö��Ssdt��Ϣ
	************************************************************************/
	BOOL CSssdtHook::EnumSssdtHook()
	{
		m_SssdtHookEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x400;
		DWORD	 dwReturnLength = 0;
		PSSSDT_HOOK_INFORMATION shi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (shi)
			{
				free(shi);
				shi = NULL;
			}

			OutputLength = sizeof(SSSDT_HOOK_INFORMATION) + Count * sizeof(SSSDT_HOOK_ENTRY_INFORMATION);

			shi = (PSSSDT_HOOK_INFORMATION)malloc(OutputLength);
			if (!shi)
			{
				break;
			}

			RtlZeroMemory(shi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMSSSDTHOOK,
				NULL,		// InputBuffer
				0,
				shi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)shi->NumberOfSssdtFunctions + 100;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && shi)
		{
			for (UINT32 i = 0; i < shi->NumberOfSssdtFunctions; i++)
			{
				// ���ƽ�����Ϣ�ṹ
				m_SssdtHookEntryVector.push_back(shi->SssdtHookEntry[i]);
			}

			m_SssdtFunctionCount = shi->NumberOfSssdtFunctions;
			bOk = TRUE;
		}

		if (shi)
		{
			free(shi);
			shi = NULL;
		}

		if (m_SssdtHookEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertSssdtHookInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CSssdtHook::InsertSssdtHookInfoList(CListCtrl *ListCtrl)
	{
		UINT32 SssdtFuncNum = 0;
		UINT32 SssdtHookNum = 0;
		size_t Size = m_SssdtHookEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			SSSDT_HOOK_ENTRY_INFORMATION SssdtHookEntry = m_SssdtHookEntryVector[i];

			CString strOrdinal, strFunctionName, strCurrentAddress, strOriginalAddress, strStatus, strFilePath;

			strOrdinal.Format(L"%d", SssdtHookEntry.Ordinal);
			strFunctionName = SssdtHookEntry.wzFunctionName;
			strCurrentAddress.Format(L"0x%p", SssdtHookEntry.CurrentAddress);
			strOriginalAddress.Format(L"0x%p", SssdtHookEntry.OriginalAddress);
			strFilePath = m_DriverCore.GetDriverPathByAddress(SssdtHookEntry.CurrentAddress);

			if (SssdtHookEntry.bHooked)
			{
				strStatus = L"SssdtHook";
				SssdtHookNum++;
			}
			else
			{
				strStatus = L"-";
			}

			// �ж���ʾ
			if (((CHookDlg*)m_Global->m_HookDlg)->m_bOnlyShowHooked)
			{
				if (SssdtHookEntry.bHooked)
				{
					int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strOrdinal);
					ListCtrl->SetItemText(iItem, shc_FunctionName, strFunctionName);
					ListCtrl->SetItemText(iItem, shc_CurrentAddress, strCurrentAddress);
					ListCtrl->SetItemText(iItem, shc_OriginalAddress, strOriginalAddress);
					ListCtrl->SetItemText(iItem, shc_Status, strStatus);
					ListCtrl->SetItemText(iItem, shc_FilePath, strFilePath);

					ListCtrl->SetItemData(iItem, TRUE);
				}
			}
			else
			{
				int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strOrdinal);
				ListCtrl->SetItemText(iItem, shc_FunctionName, strFunctionName);
				ListCtrl->SetItemText(iItem, shc_CurrentAddress, strCurrentAddress);
				ListCtrl->SetItemText(iItem, shc_OriginalAddress, strOriginalAddress);
				ListCtrl->SetItemText(iItem, shc_Status, strStatus);
				ListCtrl->SetItemText(iItem, shc_FilePath, strFilePath);

				if (SssdtHookEntry.bHooked)
				{
					ListCtrl->SetItemData(iItem, TRUE);
				}
			}

			SssdtFuncNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Sssdt Hook Info is loading now, Count:%d", SssdtFuncNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Sssdt����: %d�����ҹ�����: %d", Size, SssdtHookNum);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QuerySssdtHook
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CSssdtHook::QuerySssdtHook(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_SssdtHookEntryVector.clear();
		m_DriverCore.DriverEntryVector().clear();

		if (EnumSssdtHook() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Sssdt Hook Initialize failed");
			return;
		}

		if (m_DriverCore.EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Sssdt Hook Initialize failed");
			return;
		}

		InsertSssdtHookInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QuerySssdtHookCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ����ģ��Ļص�
	************************************************************************/
	DWORD CALLBACK CSssdtHook::QuerySssdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_SssdtHook->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_SssdtHook->m_Global->UpdateStatusBarTip(L"Sssdt");
		m_SssdtHook->m_Global->UpdateStatusBarDetail(L"Sssdt is loading now...");

		m_SssdtHook->QuerySssdtHook(ListCtrl);

		m_SssdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}



	/************************************************************************
	*  Name : ResumeSssdtHook
	*  Param: Ordinal
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ��̼������Ϣ
	************************************************************************/
	BOOL CSssdtHook::ResumeSssdtHook(UINT32 Ordinal)
	{
		BOOL   bOk = FALSE;
		DWORD  dwReturnLength = 0;

		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_RESUMESSSDTHOOK,
			&Ordinal,		// InputBuffer
			sizeof(UINT32),
			NULL,
			0,
			&dwReturnLength,
			NULL);

		return bOk;
	}


	/************************************************************************
	*  Name : ResumeSssdtHookCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  �ָ�ָ��SssdtHook�Ļص�
	************************************************************************/
	DWORD CALLBACK CSssdtHook::ResumeSssdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		int iIndex = ListCtrl->GetSelectionMark();
		if (iIndex < 0)
		{
			return 0;
		}

		UINT32   Ordinal = iIndex;

		UINT_PTR CurrentAddress = 0;
		CString  strCurrentAddress = ListCtrl->GetItemText(iIndex, shc_CurrentAddress);
		swscanf_s(strCurrentAddress.GetBuffer() + 2, L"%p", &CurrentAddress);

		UINT_PTR OriginalAddress = 0;
		CString  strOriginalAddress = ListCtrl->GetItemText(iIndex, shc_OriginalAddress);
		swscanf_s(strOriginalAddress.GetBuffer() + 2, L"%p", &OriginalAddress);

		// ���û��Hook��ֱ�ӷ�����
		if (OriginalAddress == CurrentAddress || Ordinal < 0 || Ordinal > m_SssdtFunctionCount)
		{
			return 0;
		}

		m_SssdtHook->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		if (m_SssdtHook->ResumeSssdtHook(Ordinal))
		{
			// ˢ���б�
			m_SssdtHook->QuerySssdtHook(ListCtrl);
		}

		m_SssdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : ResumeAllSssdtHookCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  �ָ�����SsdtHook�Ļص�
	************************************************************************/
	DWORD CALLBACK CSssdtHook::ResumeAllSssdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		UINT32 Ordinal = RESUME_ALL_HOOKS;

		m_SssdtHook->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		if (m_SssdtHook->ResumeSssdtHook(Ordinal))
		{
			// ˢ���б�
			m_SssdtHook->QuerySssdtHook(ListCtrl);
		}

		m_SssdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}