
// ArkProtectApp.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CArkProtectAppApp: 
// �йش����ʵ�֣������ ArkProtectApp.cpp
//

class CArkProtectAppApp : public CWinApp
{
public:
	CArkProtectAppApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CArkProtectAppApp theApp;