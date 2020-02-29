
// ArkProtectAppDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "Global.hpp"
#include "afxcmn.h"
#include "ProcessDlg.h"
#include "DriverDlg.h"
#include "KernelDlg.h"
#include "HookDlg.h"
#include "RegistryDlg.h"

// CArkProtectAppDlg �Ի���
class CArkProtectAppDlg : public CDialogEx
{
// ����
public:
	CArkProtectAppDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ARKPROTECTAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnIconNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateStatusBarTip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateStatusBarDetail(WPARAM wParam, LPARAM lParam);
	afx_msg void OnIconnotifyDisplay();
	afx_msg void OnIconnotifyHide();
	afx_msg void OnIconnotifyExit();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnStnClickedProcessStatic();
	afx_msg void OnStnClickedDriverStatic();
	afx_msg void OnStnClickedKernelStatic();
	afx_msg void OnStnClickedHookStatic();
	afx_msg void OnStnClickedRegistryStatic();

	DECLARE_MESSAGE_MAP()
public:

	void APInitializeTray();

	void APEnableCurrentButton(ArkProtect::eChildDlg ChildDlg);

	void APShowChildWindow(ArkProtect::eChildDlg ChildDlg);



	CTabCtrl            m_AppTab;
	CStatic             m_ProcessButton;
	CStatic             m_DriverButton;
	CStatic             m_KernelButton;
	CStatic             m_HookButton;
	CStatic             m_RegistryButton;
	CStatic             m_AboutButton;
	CStatusBarCtrl      *m_StatusBar;
	NOTIFYICONDATA	    m_NotifyIcon = { 0 };   // ������ͼ��
	
	
	ArkProtect::CGlobal   m_Global;
	ArkProtect::eChildDlg m_CurrentChildDlg = (ArkProtect::eChildDlg)(-1);    // �ӶԻ���
	CProcessDlg           *m_ProcessDlg = NULL;
	CDriverDlg            *m_DriverDlg = NULL;
	CKernelDlg            *m_KernelDlg = NULL;
	CHookDlg              *m_HookDlg = NULL;
	CRegistryDlg          *m_RegistryDlg = NULL;

	

};
