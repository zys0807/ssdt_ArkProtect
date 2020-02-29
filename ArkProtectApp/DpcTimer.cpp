#include "stdafx.h"
#include "DpcTimer.h"
#include "Global.hpp"

namespace ArkProtect
{
	CDpcTimer *CDpcTimer::m_DpcTimer;

	CDpcTimer::CDpcTimer(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
		, m_DriverCore(GlobalObject->DriverCore())
	{
		m_DpcTimer = this;
	}


	CDpcTimer::~CDpcTimer()
	{
	}


	/************************************************************************
	*  Name : InitializeDpcTimerList
	*  Param: ListCtrl               ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CDpcTimer::InitializeDpcTimerList(CListCtrl *ListCtrl)
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
	*  Name : EnumDpcTimer
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö��IoTimer��Ϣ
	************************************************************************/
	BOOL CDpcTimer::EnumDpcTimer()
	{
		m_DpcTimerEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PDPC_TIMER_INFORMATION dti = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (dti)
			{
				free(dti);
				dti = NULL;
			}

			OutputLength = sizeof(DPC_TIMER_INFORMATION) + Count * sizeof(DPC_TIMER_ENTRY_INFORMATION);

			dti = (PDPC_TIMER_INFORMATION)malloc(OutputLength);
			if (!dti)
			{
				break;
			}

			RtlZeroMemory(dti, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMDPCTIMER,
				NULL,		// InputBuffer
				0,
				dti,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = dti->NumberOfDpcTimers + 20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && dti)
		{
			for (UINT32 i = 0; i < dti->NumberOfDpcTimers; i++)
			{
				m_DpcTimerEntryVector.push_back(dti->DpcTimerEntry[i]);
			}
			bOk = TRUE;
		}

		if (dti)
		{
			free(dti);
			dti = NULL;
		}

		if (m_DpcTimerEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}

	/************************************************************************
	*  Name : InsertDpcTimerInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CDpcTimer::InsertDpcTimerInfoList(CListCtrl *ListCtrl)
	{
		UINT32 DpcTimerNum = 0;
		size_t Size = m_DpcTimerEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			DPC_TIMER_ENTRY_INFORMATION DpcTimerEntry = m_DpcTimerEntryVector[i];

			CString strTimerObject, strRealDpc, strCycle, strFilePath, strDispatch, strCompany;

			strTimerObject.Format(L"0x%p", DpcTimerEntry.TimerObject);
			strRealDpc.Format(L"0x%p", DpcTimerEntry.RealDpc);
			strCycle.Format(L"%d", DpcTimerEntry.Cycle / 1000);
			strDispatch.Format(L"0x%p", DpcTimerEntry.TimeDispatch);
			strFilePath = m_DriverCore.GetDriverPathByAddress(DpcTimerEntry.TimeDispatch);
			strCompany = m_Global->GetFileCompanyName(strFilePath);

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strTimerObject);
			ListCtrl->SetItemText(iItem, dtc_Device, strRealDpc);
			ListCtrl->SetItemText(iItem, dtc_Cycle, strCycle);
			ListCtrl->SetItemText(iItem, dtc_Dispatch, strDispatch);
			ListCtrl->SetItemText(iItem, dtc_FilePath, strFilePath);
			ListCtrl->SetItemText(iItem, dtc_Company, strCompany);

			if (_wcsnicmp(strCompany.GetBuffer(), L"Microsoft Corporation", wcslen(L"Microsoft Corporation")) != 0)
			{
				ListCtrl->SetItemData(iItem, TRUE);
			}

			DpcTimerNum++;

			CString strStatusContext;
			strStatusContext.Format(L"DpcTimer Info is loading now, Count:%d", DpcTimerNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"DpcTimer Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}




	/************************************************************************
	*  Name : QueryDpcTimer
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CDpcTimer::QueryDpcTimer(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_DpcTimerEntryVector.clear();
		m_DriverCore.DriverEntryVector().clear();

		if (EnumDpcTimer() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"DpcTimer Initialize failed");
			return;
		}

		if (m_DriverCore.EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"DpcTimer Initialize failed");
			return;
		}

		InsertDpcTimerInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryDpcTimerCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ IoTimer �Ļص�
	************************************************************************/
	DWORD CALLBACK CDpcTimer::QueryDpcTimerCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_DpcTimer->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_DpcTimer->m_Global->UpdateStatusBarTip(L"DpcTimer");
		m_DpcTimer->m_Global->UpdateStatusBarDetail(L"DpcTimer is loading now...");

		m_DpcTimer->QueryDpcTimer(ListCtrl);

		m_DpcTimer->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
