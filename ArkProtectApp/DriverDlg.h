#pragma once
#include "Global.hpp"
#include "afxcmn.h"

// CDriverDlg �Ի���

class CDriverDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDriverDlg)

public:
	CDriverDlg(CWnd* pParent = NULL, ArkProtect::CGlobal *GlobalObject = NULL);   // ��׼���캯��
	virtual ~CDriverDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DRIVER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnNMCustomdrawDriverList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedDriverList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickDriverList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDriverFreshen();
	afx_msg void OnDriverDelete();
	afx_msg void OnDriverUnload();
	afx_msg void OnDriverProperty();
	afx_msg void OnDriverLocation();
	afx_msg void OnDriverExportInformation();
	DECLARE_MESSAGE_MAP()
public:
	
	void APInitializeDriverList();

	void APLoadDriverList();


	CImageList          m_DriverIconList;   // ����ͼ��
	CListCtrl           m_DriverListCtrl;

	CString             m_strFilePath;

	ArkProtect::CGlobal *m_Global;
	
	



	
};
