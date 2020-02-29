#include "stdafx.h"
#include "DriverCore.h"
#include "Global.hpp"
#include "DriverDlg.h"

namespace ArkProtect
{
	CDriverCore *CDriverCore::m_Driver;

	CDriverCore::CDriverCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_Driver = this;
	}


	CDriverCore::~CDriverCore()
	{
	}


	/************************************************************************
	*  Name : InitializeDriverList
	*  Param: ProcessList           ProcessModule�Ի����ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CDriverCore::InitializeDriverList(CListCtrl *DriverList)
	{
		DriverList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			DriverList->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}


	/************************************************************************
	*  Name : PerfectDriverInfo
	*  Param: DriverEntry			     ����ģ����Ϣ�ṹ
	*  Ret  : void
	*  ���ƽ�����Ϣ�ṹ
	************************************************************************/
	void CDriverCore::PerfectDriverInfo(PDRIVER_ENTRY_INFORMATION DriverEntry)
	{
		if (!DriverEntry || wcslen(DriverEntry->wzDriverPath) == 0)
		{
			return;
		}

		//
		// ��������·��
		// 
		WCHAR wzWindowsDirectory[MAX_PATH] = { 0 };
		WCHAR wzDriverDirectory[MAX_PATH] = { 0 };
		WCHAR wzDrivers[] = L"\\System32\\Drivers\\";

		GetWindowsDirectory(wzWindowsDirectory, MAX_PATH - 1);	// ���WindowsĿ¼
		StringCchCopyW(wzDriverDirectory, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
		StringCchCatW(wzDriverDirectory, wcslen(wzDriverDirectory) + wcslen(wzDrivers) + 1 , wzDrivers);

		WCHAR  *wzOriginalDriverPath = DriverEntry->wzDriverPath;
		WCHAR  wzFixedDriverPath[MAX_PATH] = { 0 };
		WCHAR  *wzPos = wcschr(wzOriginalDriverPath, L'\\');

		// û��Ŀ¼��Ϣ ֻ���������� 
		if (!wzPos)
		{
			StringCchCopyW(wzFixedDriverPath, wcslen(wzDriverDirectory) + 1, wzDriverDirectory);
			StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath) + 1, wzOriginalDriverPath);
			StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

			wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
		}
		else
		{
			WCHAR wzUnknow[] = L"\\??\\";
			WCHAR wzSystemRoot[] = L"\\SystemRoot";
			WCHAR wzWindows[] = L"\\Windows";
			WCHAR wzWinnt[] = L"\\Winnt";
			size_t   Original = wcslen(wzOriginalDriverPath);

			if (Original >= wcslen(wzUnknow) && _wcsnicmp(wzOriginalDriverPath, wzUnknow, wcslen(wzUnknow)) == 0)
			{
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzUnknow)) + 1, wzOriginalDriverPath + wcslen(wzUnknow));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);
				
				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
			else if (Original >= wcslen(wzSystemRoot) && _wcsnicmp(wzOriginalDriverPath, wzSystemRoot, wcslen(wzSystemRoot)) == 0)
			{
				StringCchCopyW(wzFixedDriverPath, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzSystemRoot)) + 1, wzOriginalDriverPath + wcslen(wzSystemRoot));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
			else if (Original >= wcslen(wzWindows) && _wcsnicmp(wzOriginalDriverPath, wzWindows, wcslen(wzWindows)) == 0)
			{
				StringCchCopyW(wzFixedDriverPath, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzWindows)) + 1, wzOriginalDriverPath + wcslen(wzWindows));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
			else if (Original >= wcslen(wzWinnt) && _wcsnicmp(wzOriginalDriverPath, wzWinnt, wcslen(wzWinnt)) == 0)
			{
				StringCchCopyW(wzFixedDriverPath, wcslen(wzWindowsDirectory) + 1, wzWindowsDirectory);
				StringCchCatW(wzFixedDriverPath, wcslen(wzFixedDriverPath) + wcslen(wzOriginalDriverPath + wcslen(wzWinnt)) + 1, wzOriginalDriverPath + wcslen(wzWinnt));
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzFixedDriverPath) + 1, wzFixedDriverPath);

				wzOriginalDriverPath[wcslen(wzFixedDriverPath)] = L'\0';
			}
		}

		// ����Ƕ��ļ���
		if (wcschr(wzOriginalDriverPath, '~'))
		{
			WCHAR wzLongPathName[MAX_PATH] = { 0 };
			DWORD dwReturn = GetLongPathName(wzOriginalDriverPath, wzLongPathName, MAX_PATH);
			if (!(dwReturn >= MAX_PATH || dwReturn == 0))
			{
				StringCchCopyW(wzOriginalDriverPath, wcslen(wzLongPathName) + 1, wzLongPathName);

				wzOriginalDriverPath[wcslen(wzLongPathName)] = L'\0';
			}
		}


		//
		// ������������
		// 
		WCHAR *wzDriverName = NULL;
		wzDriverName = wcsrchr(DriverEntry->wzDriverPath, '\\');  // ������ \\ 
		wzDriverName++;  // �� '\\'

		StringCchCopyW(DriverEntry->wzDriverName, wcslen(wzDriverName) + 1, wzDriverName);

		//
		// ����������
		//
		CString strCompanyName = m_Global->GetFileCompanyName(DriverEntry->wzDriverPath);
		if (strCompanyName.GetLength() == 0)
		{
			strCompanyName = L"�ļ�������";
		}

		StringCchCopyW(DriverEntry->wzCompanyName, strCompanyName.GetLength() + 1, strCompanyName.GetBuffer());
	}


	/************************************************************************
	*  Name : EnumDriverInfo
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ��̼������Ϣ
	************************************************************************/
	BOOL CDriverCore::EnumDriverInfo()
	{
		m_DriverEntryVector.clear();

		BOOL bOk = FALSE;
		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PDRIVER_INFORMATION di = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (di)
			{
				free(di);
				di = NULL;
			}

			OutputLength = sizeof(DRIVER_INFORMATION) + Count * sizeof(DRIVER_ENTRY_INFORMATION);

			di = (PDRIVER_INFORMATION)malloc(OutputLength);
			if (!di)
			{
				break;
			}

			RtlZeroMemory(di, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMDRIVER,
				NULL,		// InputBuffer
				0,
				di,
				OutputLength,
				&dwReturnLength,
				NULL);

			// ������do whileѭ����ǡ���ô�����������ڴ治�����ظ�ö�٣�����㹻��һ��ͨ��
			Count = (UINT32)di->NumberOfDrivers + 0x20;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && di)
		{
			for (UINT32 i = 0; i < di->NumberOfDrivers; i++)
			{
				// ���ƽ�����Ϣ�ṹ
				PerfectDriverInfo(&di->DriverEntry[i]);
				m_DriverEntryVector.push_back(di->DriverEntry[i]);
			}
			bOk = TRUE;
		}

		if (di)
		{
			free(di);
			di = NULL;
		}

		if (m_DriverEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertDriverInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CDriverCore::InsertDriverInfoList(CListCtrl *ListCtrl)
	{
		UINT32 DriverNum = 0;
		size_t Size = m_DriverEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			DRIVER_ENTRY_INFORMATION DriverEntry = m_DriverEntryVector[i];

			CString strDriverName, strBase, strSize, strObject, strDriverPath, strServiceName, strLoadOrder, strCompanyName, strDriverStart;

			strDriverName = DriverEntry.wzDriverName;
			strDriverPath = DriverEntry.wzDriverPath;
			strServiceName = DriverEntry.wzServiceName;

			strBase.Format(L"0x%p", DriverEntry.BaseAddress);
			strSize.Format(L"0x%X", DriverEntry.Size);
			strLoadOrder.Format(L"%d", DriverEntry.LoadOrder);

			if (DriverEntry.DriverObject)
			{
				strObject.Format(L"0x%p", DriverEntry.DriverObject);
				strDriverStart.Format(L"0x%p", DriverEntry.DirverStartAddress);
			}
			else
			{
				strObject = L"-";
				strDriverStart = L"-";
			}

			strCompanyName = DriverEntry.wzCompanyName;

			// ���ͼ��
			m_Global->AddFileIcon(DriverEntry.wzDriverPath, &((CDriverDlg*)m_Global->m_DriverDlg)->m_DriverIconList);
			
			int iImageCount = ((CDriverDlg*)m_Global->m_DriverDlg)->m_DriverIconList.GetImageCount() - 1;
			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strDriverName, iImageCount);

			ListCtrl->SetItemText(iItem, dc_BaseAddress, strBase);
			ListCtrl->SetItemText(iItem, dc_Size, strSize);
			ListCtrl->SetItemText(iItem, dc_Object, strObject);
			ListCtrl->SetItemText(iItem, dc_DriverPath, strDriverPath);
			ListCtrl->SetItemText(iItem, dc_ServiceName, strServiceName);
			ListCtrl->SetItemText(iItem, dc_StartAddress, strDriverStart);
			ListCtrl->SetItemText(iItem, dc_LoadOrder, strLoadOrder);
			ListCtrl->SetItemText(iItem, dc_Company, strCompanyName);

			if (_wcsnicmp(DriverEntry.wzCompanyName, L"Microsoft Corporation", wcslen(L"Microsoft Corporation")) != 0)
			{
				ListCtrl->SetItemData(iItem, TRUE);
			}

			DriverNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Driver Info is loading now, Count:%d", DriverNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Driver Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}



	/************************************************************************
	*  Name : QueryDriverInfo
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CDriverCore::QueryDriverInfo(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_DriverEntryVector.clear();

		if (EnumDriverInfo() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Info Initialize failed");
			return;
		}

		InsertDriverInfoList(ListCtrl);
	}


	/************************************************************************
	*  Name : QueryDriverInfoCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ������Ϣ�Ļص�
	************************************************************************/
	DWORD CALLBACK CDriverCore::QueryDriverInfoCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_Driver->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_Driver->m_Global->UpdateStatusBarTip(L"Driver Info");
		m_Driver->m_Global->UpdateStatusBarDetail(L"Driver Info is loading now...");

		m_Driver->QueryDriverInfo(ListCtrl);

		m_Driver->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : UnloadDriver
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ��̼������Ϣ
	************************************************************************/
	BOOL CDriverCore::UnloadDriver(UINT_PTR DriverObject)
	{
		BOOL bOk = FALSE;
		DWORD	 dwReturnLength = 0;
		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_UNLOADRIVER,
			&DriverObject,		// InputBuffer
			sizeof(UINT_PTR),
			NULL,
			0,
			&dwReturnLength,
			NULL);

		return bOk;
	}


	/************************************************************************
	*  Name : UnloadDriverCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ������Ϣ�Ļص�
	************************************************************************/
	DWORD CALLBACK CDriverCore::UnloadDriverCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		int iIndex = ListCtrl->GetSelectionMark();
		if (iIndex < 0)
		{
			return 0;
		}

		UINT_PTR DriverObject = 0;
		CString  strObject = ListCtrl->GetItemText(iIndex, dc_Object);
		swscanf_s(strObject.GetBuffer() + 2, L"%p", &DriverObject);

		if (DriverObject == 0)
		{
			return 0;
		}

		m_Driver->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		if (m_Driver->UnloadDriver(DriverObject))
		{
			// ˢ���б�
			m_Driver->QueryDriverInfo(ListCtrl);
		}

		m_Driver->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}


	/************************************************************************
	*  Name : GetDriverPathByAddress
	*  Param: Address                  ��ַ
	*  Ret  : CString
	*  ͨ����ַ���ڵ�ַ��Χ���������·��
	************************************************************************/
	CString CDriverCore::GetDriverPathByAddress(UINT_PTR Address)
	{
		CString strPath = L"";

		size_t Size = m_DriverEntryVector.size();

		for (int i = 0; i < Size; i++)
		{
			DRIVER_ENTRY_INFORMATION DriverEntry = m_DriverEntryVector[i];

			UINT_PTR BaseAddress = DriverEntry.BaseAddress;
			UINT_PTR EndAddress = DriverEntry.BaseAddress + DriverEntry.Size;

			if (Address >= BaseAddress && Address <= EndAddress)
			{
				strPath = DriverEntry.wzDriverPath;
				break;
			}
		}
		return strPath;
	}
}