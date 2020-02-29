#pragma once
#include "afxcmn.h"
#include "Global.hpp"
#include "ProcessCore.h"
#include "ProcessInfoDlg.h"

// CProcessDlg �Ի���

class CProcessDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProcessDlg)

public:
	CProcessDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // ��׼���캯��
	virtual ~CProcessDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnNMCustomdrawProcessList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclickProcessList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickProcessList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnProcessDelete();
	afx_msg void OnUpdateProcessDelete(CCmdUI *pCmdUI);
	afx_msg void OnProcessFreshen();
	afx_msg void OnProcessModule();
	afx_msg void OnProcessThread();
	afx_msg void OnProcessHandle();
	afx_msg void OnProcessWindow();
	afx_msg void OnProcessMemory();
	afx_msg void OnProcessTerminate();
	afx_msg void OnProcessForceTerminate();
	afx_msg void OnProcessSuspend();
	afx_msg void OnProcessResume();
	afx_msg void OnProcessProperty();
	afx_msg void OnProcessLocation();
	afx_msg void OnProcessExportInformation();
	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeProcessList();

	void APLoadProcessList();

	void APDeleteFile();

	void APInitializeProcessInfoDlg(ArkProtect::eProcessInfoKind ProcessInfoKind);


	CImageList          m_ProcessIconList;   // ����ͼ��
	CListCtrl           m_ProcessListCtrl;   // ListControl

	CProcessInfoDlg     *m_ProcessInfoDlg = NULL;

	ArkProtect::CGlobal *m_Global;
	
	BOOL                m_bDeleteFile = FALSE;
	CString             m_strFilePath;

	BOOL                m_bSuspend = FALSE;
	HANDLE              m_SuspendThreadHandle = NULL;

	static UINT32       m_SortColumn;
	static BOOL         m_bSortOrder;  // ��¼����˳��
	
};


int CALLBACK APProcessListCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);