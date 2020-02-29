#include "stdafx.h"
#include "FileCore.h"
#include "Global.hpp"


namespace ArkProtect
{
	CFileCore *CFileCore::m_File;

	CFileCore::CFileCore(CGlobal *GlobalObject)
		: m_Global(GlobalObject)
	{
		m_File = this;
	}


	CFileCore::~CFileCore()
	{
	}


	/************************************************************************
	*  Name : DeleteFile
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ��̼������Ϣ
	************************************************************************/
	BOOL CFileCore::DeleteFile(CString strFilePath)
	{
		if (strFilePath.GetBuffer() == NULL)
		{
			::MessageBox(NULL, L"Invalid file path", L"ArkProtect", MB_OK | MB_ICONERROR);
			return FALSE;
		}

		BOOL  bOk = FALSE;
		DWORD dwReturnLength = 0;

		bOk = DeviceIoControl(m_Global->m_DeviceHandle,
			IOCTL_ARKPROTECT_DELETEFILE,
			strFilePath.GetBuffer(),		// InputBuffer
			strFilePath.GetLength(),
			NULL,
			0,
			&dwReturnLength,
			NULL);
		if (bOk == FALSE)
		{
			::MessageBox(NULL, L"Delete file path failed", L"ArkProtect", MB_OK | MB_ICONERROR);
		}
		
		return bOk;
	}


	/************************************************************************
	*  Name : DeleteFileCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ����Ŀ�����
	************************************************************************/
	DWORD CALLBACK CFileCore::DeleteFileCallback(LPARAM lParam)
	{
		CString strFilePath = *(CString*)lParam;

		Sleep(2000);    // ��ʱͨ�����ַ��������Ǳ߽������̻���ж������������ˣ��ٽ����ļ�ɾ��!!

		m_File->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_File->DeleteFile(strFilePath);

		m_File->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}