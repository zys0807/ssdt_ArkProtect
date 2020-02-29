#include "stdafx.h"
#include "SsdtHook.h"
#include "Global.hpp"
#include "HookDlg.h"

namespace ArkProtect
{
	CSsdtHook *CSsdtHook::m_SsdtHook;
	UINT32    CSsdtHook::m_SsdtFunctionCount;

	CSsdtHook::CSsdtHook(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_DriverCore(GlobalObject->DriverCore())
	{
		m_SsdtHook = this;
		m_SsdtFunctionCount = 0;
	}


	CSsdtHook::~CSsdtHook()
	{
	}


	/************************************************************************
	*  Name : InitializeSsdtList
	*  Param: ListCtrl               ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CSsdtHook::InitializeSsdtList(CListCtrl *ListCtrl)
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
	*  Name : EnumSsdtHook
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö��Ssdt��Ϣ
	************************************************************************/
	BOOL CSsdtHook::EnumSsdtHook()
	{
		m_SsdtHookEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x200;
		DWORD	 dwReturnLength = 0;
		PSSDT_HOOK_INFORMATION shi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (shi)
			{
				free(shi);
				shi = NULL;
			}

			OutputLength = sizeof(SSDT_HOOK_INFORMATION) + Count * sizeof(SSDT_HOOK_ENTRY_INFORMATION);

			shi = (PSSDT_HOOK_INFORMATION)malloc(OutputLength);
			if (!shi)
			{
				break;
			}

			RtlZeroMemory(shi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMSSDTHOOK,
				NULL,		// InputBuffer
				0,
				shi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)shi->NumberOfSsdtFunctions + 100;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && shi)
		{
			for (UINT32 i = 0; i < shi->NumberOfSsdtFunctions; i++)
			{
				// ���ƽ�����Ϣ�ṹ
				m_SsdtHookEntryVector.push_back(shi->SsdtHookEntry[i]);
			}

			m_SsdtFunctionCount = shi->NumberOfSsdtFunctions;
			bOk = TRUE;
		}

		if (shi)
		{
			free(shi);
			shi = NULL;
		}

		if (m_SsdtHookEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertSsdtHookInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CSsdtHook::InsertSsdtHookInfoList(CListCtrl *ListCtrl)
	{
		UINT32 SsdtFuncNum = 0;
		UINT32 SsdtHookNum = 0;
		size_t Size = m_SsdtHookEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			SSDT_HOOK_ENTRY_INFORMATION SsdtHookEntry = m_SsdtHookEntryVector[i];

			CString strOrdinal, strFunctionName, strCurrentAddress, strOriginalAddress, strStatus, strFilePath;

			strOrdinal.Format(L"%d", SsdtHookEntry.Ordinal);
			strFunctionName = SsdtHookEntry.wzFunctionName;
			strCurrentAddress.Format(L"0x%p", SsdtHookEntry.CurrentAddress);
			strOriginalAddress.Format(L"0x%p", SsdtHookEntry.OriginalAddress);
			strFilePath = m_DriverCore.GetDriverPathByAddress(SsdtHookEntry.CurrentAddress);

			if (SsdtHookEntry.bHooked)
			{
				strStatus = L"SsdtHook";
				SsdtHookNum++;
			}
			else
			{
				strStatus = L"-";
			}

			// �ж���ʾ
			if (((CHookDlg*)m_Global->m_HookDlg)->m_bOnlyShowHooked)
			{
				if (SsdtHookEntry.bHooked)
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

				if (SsdtHookEntry.bHooked)
				{
					ListCtrl->SetItemData(iItem, TRUE);
				}
			}
			
			SsdtFuncNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Ssdt Hook Info is loading now, Count:%d", SsdtFuncNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Ssdt����: %d�����ҹ�����: %d", Size, SsdtHookNum);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QuerySsdtHook
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CSsdtHook::QuerySsdtHook(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_SsdtHookEntryVector.clear();
		m_DriverCore.DriverEntryVector().clear();

		if (EnumSsdtHook() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Ssdt Hook Initialize failed");
			return;
		}

		if (m_DriverCore.EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Ssdt Hook Initialize failed");
			return;
		}

		InsertSsdtHookInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QuerySsdtHookCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ����ģ��Ļص�
	************************************************************************/
	DWORD CALLBACK CSsdtHook::QuerySsdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_SsdtHook->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_SsdtHook->m_Global->UpdateStatusBarTip(L"Ssdt");
		m_SsdtHook->m_Global->UpdateStatusBarDetail(L"Ssdt is loading now...");

		m_SsdtHook->QuerySsdtHook(ListCtrl);

		m_SsdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : ResumeSsdtHook
	*  Param: Ordinal
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ��̼������Ϣ
	************************************************************************/
	BOOL CSsdtHook::ResumeSsdtHook(UINT32 Ordinal)
	{
		BOOL   bOk = FALSE;
		DWORD  dwReturnLength = 0;

		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_RESUMESSDTHOOK,
			&Ordinal,		// InputBuffer
			sizeof(UINT32),
			NULL,
			0,
			&dwReturnLength,
			NULL);

		return bOk;
	}


	/************************************************************************
	*  Name : ResumeSsdtHookCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  �ָ�ָ��SsdtHook�Ļص�
	************************************************************************/
	DWORD CALLBACK CSsdtHook::ResumeSsdtHookCallback(LPARAM lParam)
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
		if (OriginalAddress == CurrentAddress || Ordinal < 0 || Ordinal > m_SsdtFunctionCount)
		{
			return 0;
		}

		m_SsdtHook->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		if (m_SsdtHook->ResumeSsdtHook(Ordinal))
		{
			// ˢ���б�
			m_SsdtHook->QuerySsdtHook(ListCtrl);
		}

		m_SsdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : ResumeAllSsdtHookCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  �ָ�����SsdtHook�Ļص�
	************************************************************************/
	DWORD CALLBACK CSsdtHook::ResumeAllSsdtHookCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		UINT32 Ordinal = RESUME_ALL_HOOKS;

		m_SsdtHook->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		if (m_SsdtHook->ResumeSsdtHook(Ordinal))
		{
			// ˢ���б�
			m_SsdtHook->QuerySsdtHook(ListCtrl);
		}

		m_SsdtHook->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}