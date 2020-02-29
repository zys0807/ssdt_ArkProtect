
// ArkProtectAppDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ArkProtectApp.h"
#include "ArkProtectAppDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  WM_ICONNOTIFY        WM_USER + 0x100    // �Զ�����Ϣ
#define  WM_STATUSBARTIP      WM_USER + 0x101    // �Զ�����Ϣ
#define  WM_STATUSBARDETAIL   WM_USER + 0x102    // �Զ�����Ϣ

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CArkProtectAppDlg �Ի���



CArkProtectAppDlg::CArkProtectAppDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ARKPROTECTAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CArkProtectAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROCESS_STATIC, m_ProcessButton);
	DDX_Control(pDX, IDC_DRIVER_STATIC, m_DriverButton);
	DDX_Control(pDX, IDC_KERNEL_STATIC, m_KernelButton);
	DDX_Control(pDX, IDC_HOOK_STATIC, m_HookButton);
	DDX_Control(pDX, IDC_REGISTRY_STATIC, m_RegistryButton);
	DDX_Control(pDX, IDC_APP_TAB, m_AppTab);
	DDX_Control(pDX, IDC_ABOUT_STATIC, m_AboutButton);
}

BEGIN_MESSAGE_MAP(CArkProtectAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_ICONNOTIFY, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnIconNotify)
	ON_MESSAGE(WM_STATUSBARTIP, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnUpdateStatusBarTip)
	ON_MESSAGE(WM_STATUSBARDETAIL, (LRESULT(__thiscall CWnd::*)(WPARAM, LPARAM))OnUpdateStatusBarDetail)
	ON_COMMAND(ID_ICONNOTIFY_DISPLAY, &CArkProtectAppDlg::OnIconnotifyDisplay)
	ON_COMMAND(ID_ICONNOTIFY_HIDE, &CArkProtectAppDlg::OnIconnotifyHide)
	ON_COMMAND(ID_ICONNOTIFY_EXIT, &CArkProtectAppDlg::OnIconnotifyExit)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_WM_CREATE()
	ON_STN_CLICKED(IDC_PROCESS_STATIC, &CArkProtectAppDlg::OnStnClickedProcessStatic)
	ON_STN_CLICKED(IDC_DRIVER_STATIC, &CArkProtectAppDlg::OnStnClickedDriverStatic)
	ON_STN_CLICKED(IDC_KERNEL_STATIC, &CArkProtectAppDlg::OnStnClickedKernelStatic)
	ON_STN_CLICKED(IDC_HOOK_STATIC, &CArkProtectAppDlg::OnStnClickedHookStatic)
	ON_STN_CLICKED(IDC_REGISTRY_STATIC, &CArkProtectAppDlg::OnStnClickedRegistryStatic)



END_MESSAGE_MAP()


// CArkProtectAppDlg ��Ϣ�������

BOOL CArkProtectAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	// ���洰��ָ��
	m_Global.AppDlg = this;

	// ��ʼ������
	APInitializeTray();

	CPaintDC Dc(this);
	m_Global.iDpix = Dc.GetDeviceCaps(LOGPIXELSX);
	m_Global.iDpiy = Dc.GetDeviceCaps(LOGPIXELSY);

	// ���Ի���ײ���Ӳ���
	CRect	Rect;
	GetWindowRect(&Rect);
	Rect.bottom += (LONG)(1 + 21 * (m_Global.iDpiy / 96.0));
	MoveWindow(&Rect);

	// ���״̬��
	m_StatusBar = new CStatusBarCtrl;
	m_StatusBar->Create(WS_CHILD | WS_VISIBLE | SBT_OWNERDRAW, CRect(0, 0, 0, 0), this, 0);

	GetClientRect(&Rect);

	int iPartBlock[3] = { 0 };
	iPartBlock[1] = Rect.right - (LONG)(1 + 21 * (m_Global.iDpix / 96.0));
	iPartBlock[0] = iPartBlock[1] - (int)(120 * (m_Global.iDpix / 96.0));

	m_StatusBar->SetParts(3, iPartBlock);
	m_StatusBar->SetText(L"All Ready", 0, 0);

	// ���ù�����

	//	m_btnHomePage.MoveWindow(iLeftPops + (70 * 0), 0, 70, 94);
	//	m_btnProcess.MoveWindow(iLeftPops + (70 * 1), 0, 70, 94);
	//	m_btnModules.MoveWindow(iLeftPops + (70 * 2), 0, 70, 94);
	//	m_btnKernel.MoveWindow(iLeftPops + (70 * 3), 0, 70, 94);
	//	m_btnHooks.MoveWindow(iLeftPops + (70 * 4), 0, 70, 94);

	//	Rect.top = 94 + 2;
	//	Rect.bottom -= (LONG)(1 + 21 * (GlobalObject.iDpiy / 96.0));


	GetClientRect(&Rect);
	Rect.top = 2;
	Rect.bottom -= (LONG)(1 + 21 * (m_Global.iDpiy / 96.0));
	Rect.left += 94;
	m_AppTab.MoveWindow(Rect);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CArkProtectAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CArkProtectAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CArkProtectAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



/************************************************************************
*  Name : PhInitTray
*  Param: void
*  Ret  : void
*  ��ʼ������
************************************************************************/
void CArkProtectAppDlg::APInitializeTray()
{
	m_NotifyIcon.cbSize = sizeof(NOTIFYICONDATA);			// ��С��ֵ
	m_NotifyIcon.hWnd = m_hWnd;							// ������
	m_NotifyIcon.uID = IDR_MAINFRAME;						// ����ĺ���ͼ��ID
	m_NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;// ������ӵ�е�״̬
	m_NotifyIcon.uCallbackMessage = WM_ICONNOTIFY;			// �Զ���ص���Ϣ   �������ϴ�����궯��  #define  WM_ICONNOTIFY   WM_USER + 0x100
	m_NotifyIcon.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));	// ����ȫ�ֺ���������ͼ��	// MAKEINTRESOURCE ��������ת����ָ������

	lstrcpy(m_NotifyIcon.szTip, L"ArkProtect");				// ������������ʱ������ʾ������ 
	Shell_NotifyIcon(NIM_ADD, &m_NotifyIcon);				// �����������ͼ�� // ������������һ����Ϣ���������������һ��ͼ��
}


/************************************************************************
*  Name : OnIconNotify
*  Param: wParam
*  Param: lParam
*  Ret  : LRESULT
*  ����ͼ��˵�
************************************************************************/
LRESULT CArkProtectAppDlg::OnIconNotify(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		// �����˵���
		CMenu Menu;
		Menu.LoadMenuW(IDR_ICONNOTIFY_MENU);

		if (IsWindowVisible())
		{
			Menu.DeleteMenu(ID_ICONNOTIFY_DISPLAY, MF_BYCOMMAND);	// ͨ��ָ��idɾ���˵���
		}
		else
		{
			Menu.DeleteMenu(ID_ICONNOTIFY_HIDE, MF_BYCOMMAND);
		}

		CPoint Pt;
		GetCursorPos(&Pt);        // �õ����λ��
		SetForegroundWindow();    // ���õ�ǰ���� 	

		// ��ָ��λ����ʾ��ݲ˵��������ٲ˵����ѡ��
		Menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON, Pt.x, Pt.y, this);

		Menu.DestroyMenu();

		break;
	}
	default:
		break;
	}
	return 0;
}


void CArkProtectAppDlg::OnIconnotifyDisplay()
{
	// TODO: �ڴ���������������
	ShowWindow(SW_SHOWNORMAL);
}


void CArkProtectAppDlg::OnIconnotifyHide()
{
	// TODO: �ڴ���������������
	ShowWindow(SW_HIDE);
}


void CArkProtectAppDlg::OnIconnotifyExit()
{
	// TODO: �ڴ���������������
	SendMessage(WM_CLOSE);              // �رմ��ڣ�CLOSE--->DESTROY--->QUIT
}


void CArkProtectAppDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	Shell_NotifyIcon(NIM_DELETE, &m_NotifyIcon);    // �ر�IconNotify
	CDialogEx::OnClose();
}


void CArkProtectAppDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������
	
	// �رվ��������
	CloseHandle(m_Global.m_DeviceHandle);

	// ж������
	m_Global.UnloadNTDriver(DRIVER_SERVICE_NAME);   // �ͷ���Դ
}


void CArkProtectAppDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: �ڴ˴������Ϣ����������
	// һ��ʼ��ʾProcessModule
	if (bShow)
	{
		// ������ʱ��������ʾ����ģ��
		OnStnClickedProcessStatic();
	}

}


int CArkProtectAppDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  �ڴ������ר�õĴ�������
	WCHAR wzDriverFullPath[MAX_PATH] = { 0 };
	WCHAR *Pos = NULL;

	HMODULE Module = GetModuleHandle(0);
	GetModuleFileName(Module, wzDriverFullPath, sizeof(wzDriverFullPath));
	Pos = wcsrchr(wzDriverFullPath, L'\\');
	*Pos = 0;   // �ض�
	StringCchCatW(wzDriverFullPath, MAX_PATH, L"\\ArkProtectDrv.sys");

	// �������񣬼�������
	BOOL bOk = m_Global.LoadNtDriver(DRIVER_SERVICE_NAME, wzDriverFullPath);
	if (bOk)
	{
		m_Global.m_DeviceHandle = CreateFileW(LINK_NAME, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (m_Global.m_DeviceHandle == INVALID_HANDLE_VALUE)
		{
			m_Global.UnloadNTDriver(DRIVER_SERVICE_NAME);   // �ͷ���Դ
			MessageBox(L"��������ʧ��!");
			return -1;
		}
	}
	else
	{
		// ��������ʧ����
		m_Global.UnloadNTDriver(DRIVER_SERVICE_NAME);   // �ͷ���Դ
		MessageBox(L"��������ʧ��!");
		return 0;
	}

	return 0;
}


void CArkProtectAppDlg::OnStnClickedProcessStatic()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_ProcessDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_ProcessDialog);
	}

}


void CArkProtectAppDlg::OnStnClickedDriverStatic()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_DriverDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_DriverDialog);
	}
}


void CArkProtectAppDlg::OnStnClickedKernelStatic()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_KernelDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_KernelDialog);
	}
}


void CArkProtectAppDlg::OnStnClickedHookStatic()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_HookDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_HookDialog);
	}
}


void CArkProtectAppDlg::OnStnClickedRegistryStatic()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	if (m_Global.m_bIsRequestNow == FALSE && m_CurrentChildDlg != ArkProtect::cd_RegistryDialog)
	{
		APEnableCurrentButton(m_CurrentChildDlg);
		APShowChildWindow(ArkProtect::cd_RegistryDialog);
	}
}



/************************************************************************
*  Name : OnIconNotify
*  Param: wParam
*  Param: lParam
*  Ret  : LRESULT
*  ����ͼ��˵�
************************************************************************/
LRESULT CArkProtectAppDlg::OnUpdateStatusBarTip(WPARAM wParam, LPARAM lParam)
{
	m_StatusBar->SetText((LPCWSTR)lParam, 0, 0);
	return TRUE;
}


/************************************************************************
*  Name : OnIconNotify
*  Param: wParam
*  Param: lParam
*  Ret  : LRESULT
*  ����ͼ��˵�
************************************************************************/
LRESULT CArkProtectAppDlg::OnUpdateStatusBarDetail(WPARAM wParam, LPARAM lParam)
{
	m_StatusBar->SetText((LPCWSTR)lParam, 1, 0);
	return TRUE;
}






/************************************************************************
*  Name : APEnableCurrentButton
*  Param: CurrentChildDlg         ��ǰ�ӶԻ���eChildDlg��
*  Ret  : void
*  ������ǰ�ԶԻ����Ӧ��Button
************************************************************************/
void CArkProtectAppDlg::APEnableCurrentButton(ArkProtect::eChildDlg CurrentChildDlg)
{
	switch (CurrentChildDlg)
	{
	case ArkProtect::cd_ProcessDialog:
	{
		m_ProcessButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_DriverDialog:
	{
		m_DriverButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_KernelDialog:
	{
		m_KernelButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_HookDialog:
	{
		m_HookButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_RegistryDialog:
	{
	 	m_RegistryButton.EnableWindow(TRUE);
		break;
	}
	case ArkProtect::cd_AboutDialog:
	{
		m_AboutButton.EnableWindow(TRUE);
		break;
	}
	default:
		break;
	}
}


/************************************************************************
*  Name : APShowChildWindow
*  Param: TargetChildDlg        Ŀ�������ӶԻ���eChildDlg��
*  Ret  : void
*  ��ʾ/����Ŀ����ԶԻ���
************************************************************************/
void CArkProtectAppDlg::APShowChildWindow(ArkProtect::eChildDlg TargetChildDlg)
{
	switch (TargetChildDlg)
	{
	case ArkProtect::cd_ProcessDialog:
	{
		if (m_ProcessDlg == NULL)
		{
			m_ProcessDlg = new CProcessDlg(this, &m_Global);

			// �󶨶Ի���
			m_ProcessDlg->Create(IDD_PROCESS_DIALOG, GetDlgItem(IDC_APP_TAB));

			// �ƶ�����λ��
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_ProcessDlg->MoveWindow(&Rect);
		}

		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(TRUE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
		if (m_HookDlg) m_HookDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_DriverDialog:
	{
		if (m_DriverDlg == NULL)
		{
			m_DriverDlg = new CDriverDlg(this, &m_Global);

			// �󶨶Ի���
			m_DriverDlg->Create(IDD_DRIVER_DIALOG, GetDlgItem(IDC_APP_TAB));

			// �ƶ�����λ��
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_DriverDlg->MoveWindow(&Rect);
		}

		if (m_DriverDlg) m_DriverDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
		if (m_HookDlg) m_HookDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_KernelDialog:
	{
		if (m_KernelDlg == NULL)
		{
			m_KernelDlg = new CKernelDlg(this, &m_Global);

			// �󶨶Ի���
			m_KernelDlg->Create(IDD_KERNEL_DIALOG, GetDlgItem(IDC_APP_TAB));

			// �ƶ�����λ��
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_KernelDlg->MoveWindow(&Rect);
		}

		if (m_KernelDlg) m_KernelDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
		if (m_HookDlg) m_HookDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_HookDialog:
	{
		if (m_HookDlg == NULL)
		{
			m_HookDlg = new CHookDlg(this, &m_Global);

			// �󶨶Ի���
			m_HookDlg->Create(IDD_HOOK_DIALOG, GetDlgItem(IDC_APP_TAB));

			// �ƶ�����λ��
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_HookDlg->MoveWindow(&Rect);
		}

		if (m_HookDlg) m_HookDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(FALSE);

		break;
	}
	case ArkProtect::cd_RegistryDialog:
	{
		if (m_RegistryDlg == NULL)
		{
			m_RegistryDlg = new CRegistryDlg(this, &m_Global);

			// �󶨶Ի���
			m_RegistryDlg->Create(IDD_REGISTRY_DIALOG, GetDlgItem(IDC_APP_TAB));

			// �ƶ�����λ��
			CRect	Rect;
			m_AppTab.GetClientRect(&Rect);
			m_RegistryDlg->MoveWindow(&Rect);
		}

		if (m_RegistryDlg) m_RegistryDlg->ShowWindow(TRUE);
		if (m_ProcessDlg) m_ProcessDlg->ShowWindow(FALSE);
		if (m_DriverDlg) m_DriverDlg->ShowWindow(FALSE);
		if (m_KernelDlg) m_KernelDlg->ShowWindow(FALSE);
		if (m_HookDlg) m_HookDlg->ShowWindow(FALSE);

		break;
	}

	default:
		break;
	}
}






