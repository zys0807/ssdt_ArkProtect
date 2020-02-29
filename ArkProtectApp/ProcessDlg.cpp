// ProcessDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ProcessDlg.h"
#include "afxdialogex.h"
#include "ArkProtectAppDlg.h"
#include "FileCore.h"

// CProcessDlg �Ի���

IMPLEMENT_DYNAMIC(CProcessDlg, CDialogEx)


UINT32 CProcessDlg::m_SortColumn;
BOOL CProcessDlg::m_bSortOrder;

CProcessDlg::CProcessDlg(CWnd* pParent /*=NULL*/, ArkProtect::CGlobal *GlobalObject)
	: CDialogEx(IDD_PROCESS_DIALOG, pParent)
	, m_Global(GlobalObject)
{
	// ����Ի���ָ��
	m_Global->m_ProcessDlg = this;
	m_SortColumn = 0;
	m_bSortOrder = FALSE;
}

CProcessDlg::~CProcessDlg()
{
}

void CProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROCESS_LIST, m_ProcessListCtrl);
}


BEGIN_MESSAGE_MAP(CProcessDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PROCESS_LIST, &CProcessDlg::OnNMCustomdrawProcessList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_PROCESS_LIST, &CProcessDlg::OnLvnColumnclickProcessList)
	ON_NOTIFY(NM_RCLICK, IDC_PROCESS_LIST, &CProcessDlg::OnNMRClickProcessList)
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_PROCESS_DELETE, &CProcessDlg::OnProcessDelete)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_DELETE, &CProcessDlg::OnUpdateProcessDelete)
	ON_COMMAND(ID_PROCESS_FRESHEN, &CProcessDlg::OnProcessFreshen)
	ON_COMMAND(ID_PROCESS_MODULE, &CProcessDlg::OnProcessModule)
	ON_COMMAND(ID_PROCESS_THREAD, &CProcessDlg::OnProcessThread)
	ON_COMMAND(ID_PROCESS_HANDLE, &CProcessDlg::OnProcessHandle)
	ON_COMMAND(ID_PROCESS_WINDOW, &CProcessDlg::OnProcessWindow)
	ON_COMMAND(ID_PROCESS_MEMORY, &CProcessDlg::OnProcessMemory)
	ON_COMMAND(ID_PROCESS_TERMINATE, &CProcessDlg::OnProcessTerminate)
	ON_COMMAND(ID_PROCESS_FORCE_TERMINATE, &CProcessDlg::OnProcessForceTerminate)
	ON_COMMAND(ID_PROCESS_SUSPEND, &CProcessDlg::OnProcessSuspend)
	ON_COMMAND(ID_PROCESS_RESUME, &CProcessDlg::OnProcessResume)
	ON_COMMAND(ID_PROCESS_PROPERTY, &CProcessDlg::OnProcessProperty)
	ON_COMMAND(ID_PROCESS_LOCATION, &CProcessDlg::OnProcessLocation)
	ON_COMMAND(ID_PROCESS_EXPORT_INFORMATION, &CProcessDlg::OnProcessExportInformation)
END_MESSAGE_MAP()


// CProcessDlg ��Ϣ�������


BOOL CProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��

	// ��ʼ�������б�
	APInitializeProcessList();

	// ����Iconͼ���б�
	UINT nIconSize = 20 * (UINT)(m_Global->iDpix / 96.0);
	m_ProcessIconList.Create(nIconSize, nIconSize, ILC_COLOR32 | ILC_MASK, 2, 2);
	ListView_SetImageList(m_ProcessListCtrl.m_hWnd, m_ProcessIconList.GetSafeHandle(), LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // �쳣: OCX ����ҳӦ���� FALSE
}


void CProcessDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
	m_Global->iResizeX = cx;
	m_Global->iResizeY = cy;
}


void CProcessDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: �ڴ˴������Ϣ����������
	if (bShow == TRUE)
	{
		m_ProcessListCtrl.MoveWindow(0, 0, m_Global->iResizeX, m_Global->iResizeY);

		// ���¸�������Ϣ CurrentChildDlg �� ���õ�ǰ�Ӵ��ڵ�button
		((CArkProtectAppDlg*)m_Global->AppDlg)->m_CurrentChildDlg = ArkProtect::cd_ProcessDialog;
		((CArkProtectAppDlg*)m_Global->AppDlg)->m_ProcessButton.EnableWindow(FALSE);

		// ���ؽ�����Ϣ�б�
		APLoadProcessList();

		m_ProcessListCtrl.SetFocus();
	}
}


void CProcessDlg::OnNMCustomdrawProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
	{
		COLORREF clrNewTextColor, clrNewBkColor;
		BOOL bNotTrust = FALSE;
		int iItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

		clrNewTextColor = RGB(0, 0, 0);
		clrNewBkColor = RGB(255, 255, 255);

		bNotTrust = (BOOL)m_ProcessListCtrl.GetItemData(iItem);
		if (bNotTrust == TRUE)
		{
			clrNewTextColor = RGB(0, 0, 255);
		}

		pLVCD->clrText = clrNewTextColor;
		pLVCD->clrTextBk = clrNewBkColor;

		*pResult = CDRF_DODEFAULT;
	}
}


void CProcessDlg::OnLvnColumnclickProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	m_SortColumn = pNMLV->iSubItem;

	int iItemCount = m_ProcessListCtrl.GetItemCount();
	for (int i = 0; i < iItemCount; i++)
	{
		m_ProcessListCtrl.SetItemData(i, i);	// Set the data of each item to be equal to its index. 
	}

	m_ProcessListCtrl.SortItems((PFNLVCOMPARE)::APProcessListCompareFunc, (DWORD_PTR)&m_ProcessListCtrl);

	if (m_bSortOrder)
	{
		m_bSortOrder = FALSE;
	}
	else
	{
		m_bSortOrder = TRUE;
	}

	for (int i = 0; i < iItemCount; i++)
	{
		if (_wcsnicmp(m_ProcessListCtrl.GetItemText(i, ArkProtect::pc_Company),
			L"Microsoft Corporation",
			wcslen(L"Microsoft Corporation")) != 0)
		{
			m_ProcessListCtrl.SetItemData(i, TRUE);
		}
	}


	*pResult = 0;
}


void CProcessDlg::OnNMRClickProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	CMenu Menu;
	Menu.LoadMenuW(IDR_PROCESS_MENU);
	CMenu* SubMenu = Menu.GetSubMenu(0);	// �Ӳ˵�

	CPoint Pt;
	GetCursorPos(&Pt);         // �õ����λ��

	int	iCount = SubMenu->GetMenuItemCount();

	// ���û��ѡ��,����ˢ�� ����ȫ��Disable
	if (m_ProcessListCtrl.GetSelectedCount() == 0)
	{
		for (int i = 0; i < iCount; i++)
		{
			SubMenu->EnableMenuItem(i, MF_BYPOSITION | MF_DISABLED | MF_GRAYED); //�˵�ȫ�����
		}

		SubMenu->EnableMenuItem(ID_PROCESS_FRESHEN, MF_BYCOMMAND | MF_ENABLED);
	}

	SubMenu->TrackPopupMenu(TPM_LEFTALIGN, Pt.x, Pt.y, this);

	*pResult = 0;
}


void CProcessDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	CDialogEx::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	// TODO: �ڴ˴������Ϣ����������
	if (!bSysMenu && pPopupMenu)
	{
		CCmdUI CmdUI;
		CmdUI.m_pOther = NULL;
		CmdUI.m_pMenu = pPopupMenu;
		CmdUI.m_pSubMenu = NULL;

		UINT nCount = pPopupMenu->GetMenuItemCount();
		CmdUI.m_nIndexMax = nCount;
		for (UINT i = 0; i < nCount; i++)
		{
			UINT nID = pPopupMenu->GetMenuItemID(i);
			if (nID == -1 || nID == 0)
			{
				continue;
			}
			CmdUI.m_nID = nID;
			CmdUI.m_nIndex = i;
			CmdUI.DoUpdate(this, FALSE);
		}
	}
}


void CProcessDlg::OnProcessDelete()
{
	// TODO: �ڴ���������������
	m_bDeleteFile = m_bDeleteFile ? FALSE : TRUE;
}


void CProcessDlg::OnUpdateProcessDelete(CCmdUI *pCmdUI)
{
	// TODO: �ڴ������������û����洦��������
	pCmdUI->SetCheck(m_bDeleteFile);
}


void CProcessDlg::OnProcessFreshen()
{
	// TODO: �ڴ���������������
	// ���ؽ�����Ϣ�б�
	APLoadProcessList();
}


void CProcessDlg::OnProcessModule()
{
	// TODO: �ڴ���������������

	// ��ʼ��ProcessInfoDlg������
	APInitializeProcessInfoDlg(ArkProtect::pik_Module);

}


void CProcessDlg::OnProcessThread()
{
	// TODO: �ڴ���������������

	// ��ʼ��ProcessInfoDlg������
	APInitializeProcessInfoDlg(ArkProtect::pik_Thread);
}


void CProcessDlg::OnProcessHandle()
{
	// TODO: �ڴ���������������
	// ��ʼ��ProcessInfoDlg������
	APInitializeProcessInfoDlg(ArkProtect::pik_Handle);
}


void CProcessDlg::OnProcessWindow()
{
	// TODO: �ڴ���������������
	// ��ʼ��ProcessInfoDlg������
	APInitializeProcessInfoDlg(ArkProtect::pik_Window);
}


void CProcessDlg::OnProcessMemory()
{
	// TODO: �ڴ���������������
	// ��ʼ��ProcessInfoDlg������
	APInitializeProcessInfoDlg(ArkProtect::pik_Memory);
}


void CProcessDlg::OnProcessTerminate()
{
	// TODO: �ڴ���������������

	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	// ��������
	HANDLE ThreadHandle = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::TerminateProcessCallback, &m_ProcessListCtrl, 0, NULL);

	// �����������ȴ����̵Ľ���
	WaitForSingleObject(ThreadHandle, INFINITY);
	
	// �ж��Ƿ�Ҫɾ���ļ�
	if (m_bDeleteFile)
	{
		APDeleteFile();
	}
}


void CProcessDlg::OnProcessForceTerminate()
{
	// TODO: �ڴ���������������

	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	// ������������
	HANDLE ThreadHandle = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::ForceTerminateProcessCallback, &m_ProcessListCtrl, 0, NULL);

	// �����������ȴ����̵Ľ���
	WaitForSingleObject(ThreadHandle, INFINITE);
	
	// �ж��Ƿ�Ҫɾ���ļ�
	if (m_bDeleteFile)
	{
		APDeleteFile();
	}
}


void CProcessDlg::OnProcessSuspend()
{
	// TODO: �ڴ���������������
	if (m_bSuspend == FALSE)
	{
		// ��ý���id
		UINT32   ProcessId = 0;
		POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

		while (Pos)
		{
			int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);
			ProcessId = _ttoi(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ProcessId).GetBuffer());
		}

		// ���һ���߳�Id
		UINT32 ThreadId = 0;
		BOOL bOk = m_Global->ProcessThread().GetThreadIdByProcessId(ProcessId, &ThreadId);
		if (bOk)
		{
			// ������߳�
			m_SuspendThreadHandle = ::OpenThread(THREAD_SUSPEND_RESUME, FALSE, ThreadId);

			::SuspendThread(m_SuspendThreadHandle);

			m_bSuspend = TRUE;
		}
	}
	else
	{
		MessageBox(L"�Ѿ�����ý����ˣ�", L"ArkProtect", MB_OK | MB_ICONERROR);
	}
}


void CProcessDlg::OnProcessResume()
{
	// TODO: �ڴ���������������
	if (m_bSuspend == TRUE)
	{
		::ResumeThread(m_SuspendThreadHandle);
		::CloseHandle(m_SuspendThreadHandle);
		m_SuspendThreadHandle = NULL;
		m_bSuspend = FALSE;
	}
	else
	{
		MessageBox(L"�ý��̴�������״̬��", L"ArkProtect", MB_OK | MB_ICONERROR);
	}
}


void CProcessDlg::OnProcessProperty()
{
	// TODO: �ڴ���������������
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath);

		m_Global->CheckFileProperty(strFilePath);
	}
}


void CProcessDlg::OnProcessLocation()
{
	// TODO: �ڴ���������������
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)  	// �ƹ�Idle 
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		CString strFilePath = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath);

		m_Global->LocationInExplorer(strFilePath);
	}
}


void CProcessDlg::OnProcessExportInformation()
{
	// TODO: �ڴ���������������

	m_Global->ExportInformationInText(m_ProcessListCtrl);

}


/************************************************************************
*  Name : APInitializeProcessList
*  Param: void
*  Ret  : void
*  ��ʼ��ListControl
************************************************************************/
void CProcessDlg::APInitializeProcessList()
{
	m_Global->ProcessCore().InitializeProcessList(&m_ProcessListCtrl);
}


/************************************************************************
*  Name : APLoadProcessList
*  Param: void
*  Ret  : void
*  ���ؽ�����Ϣ��ListControl
************************************************************************/
void CProcessDlg::APLoadProcessList()
{
	if (m_Global->m_bIsRequestNow == TRUE)
	{
		return;
	}

	while (m_ProcessIconList.Remove(0));

	m_ProcessListCtrl.DeleteAllItems();

	m_ProcessListCtrl.SetSelectedColumn(-1);

	// ���ؽ�����Ϣ�б�
	CloseHandle(
		CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE)ArkProtect::CProcessCore::QueryProcessInfoCallback, &m_ProcessListCtrl, 0, NULL)
	);
}


/************************************************************************
*  Name : APDeleteFile
*  Param: void
*  Ret  : void
*  ɾ���ļ�
************************************************************************/
void CProcessDlg::APDeleteFile()
{
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		m_strFilePath = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath);

		CloseHandle(
			CreateThread(NULL, 0,
			(LPTHREAD_START_ROUTINE)ArkProtect::CFileCore::DeleteFileCallback, &m_strFilePath, 0, NULL)
		);
	}
}


/************************************************************************
*  Name : APInitializeProcessInfoDlg
*  Param: ProcessInfoKind            ��Ҫ�򿪵Ľ�����Ϣ����
*  Ret  : void
*  ����ProcessEntry�������µ��ӶԻ���������ʾĿ�������Ϣ
************************************************************************/
void CProcessDlg::APInitializeProcessInfoDlg(ArkProtect::eProcessInfoKind ProcessInfoKind)
{
	POSITION Pos = m_ProcessListCtrl.GetFirstSelectedItemPosition();

	while (Pos)
	{
		int iItem = m_ProcessListCtrl.GetNextSelectedItem(Pos);

		// ��ϣ����� ProcessEntry�ṹ
		UINT32 ProcessId = _ttoi(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ProcessId).GetBuffer());
		UINT32 ParentProcessId = _ttoi(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ParentProcessId).GetBuffer());
		CString strEProcess = m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_EProcess);

		UINT_PTR EProcess = 0;
		strEProcess = strEProcess.GetBuffer() + 2;	 // �ƹ�0x
		swscanf_s(strEProcess.GetBuffer(), L"%p", &EProcess);

		ArkProtect::PROCESS_ENTRY_INFORMATION ProcessEntry = { 0 };

		ProcessEntry.ProcessId = ProcessId;
		ProcessEntry.ParentProcessId = ParentProcessId;
		ProcessEntry.EProcess = EProcess;

		if (_wcsnicmp(m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_UserAccess).GetBuffer(), L"�ܾ�", wcslen(L"�ܾ�")) == 0)
		{
			ProcessEntry.bUserAccess = FALSE;
		}
		else
		{
			ProcessEntry.bUserAccess = TRUE;
		}

		StringCchCopyW(ProcessEntry.wzCompanyName,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_Company).GetLength() + 1,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_Company).GetBuffer());
		StringCchCopyW(ProcessEntry.wzImageName,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ImageName).GetLength() + 1,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_ImageName).GetBuffer());
		StringCchCopyW(ProcessEntry.wzFilePath,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath).GetLength() + 1,
			m_ProcessListCtrl.GetItemText(iItem, ArkProtect::pc_FilePath).GetBuffer());

		m_Global->ProcessCore().ProcessEntry() = &ProcessEntry;

		m_ProcessInfoDlg = new CProcessInfoDlg(this, ProcessInfoKind, m_Global);
		m_ProcessInfoDlg->DoModal();
	}
}



/************************************************************************
*  Name : APProcessListCompareFunc
*  Param: lParam1                   ��һ��
*  Param: lParam2                   �ڶ���
*  Param: lParamSort                ���Ӳ�����ListControl��
*  Ret  : void
*  ��ѯ������Ϣ�Ļص�
************************************************************************/
int CALLBACK APProcessListCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// �Ӳ�������ȡ��Ҫ�Ƚϵ���������

	int iRow1 = (int)lParam1;
	int iRow2 = (int)lParam2;

	CListCtrl* ListCtrl = (CListCtrl*)lParamSort;

	CString str1 = ListCtrl->GetItemText(iRow1, CProcessDlg::m_SortColumn);
	CString str2 = ListCtrl->GetItemText(iRow2, CProcessDlg::m_SortColumn);

	if (CProcessDlg::m_SortColumn == ArkProtect::pc_ProcessId ||
		CProcessDlg::m_SortColumn == ArkProtect::pc_ParentProcessId)
	{
		// int�ͱȽ�
		if (CProcessDlg::m_bSortOrder)
		{
			return _ttoi(str1) - _ttoi(str2);
		}
		else
		{
			return _ttoi(str2) - _ttoi(str1);
		}
	}
	else if (CProcessDlg::m_SortColumn == ArkProtect::pc_EProcess)
	{
		UINT_PTR p1 = 0, p2 = 0;

		str1 = str1.GetBuffer() + 2;	// ��0x
		str2 = str2.GetBuffer() + 2;

		swscanf_s(str1.GetBuffer(), L"%P", &p1);
		swscanf_s(str2.GetBuffer(), L"%P", &p2);

		if (CProcessDlg::m_bSortOrder)
		{
			return (int)(p1 - p2);
		}
		else
		{
			return (int)(p2 - p1);
		}
	}
	else
	{
		// �����ͱȽ�
		if (CProcessDlg::m_bSortOrder)
		{
			return str1.CompareNoCase(str2);
		}
		else
		{
			return str2.CompareNoCase(str1);
		}
	}

	return 0;
}

