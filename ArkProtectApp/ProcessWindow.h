#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eProcessWindowColumn
	{
		pwc_WindowHandle,        // ���ھ��
		pwc_WindowText,          // ���ڱ���
		pwc_WindowClass,         // ��������
		pwc_WindowVisibal,       // ���ڿɼ���
		pwc_ProcessId,           // ����id
		pwc_ThreadId,            // �߳�id           
	};

	typedef struct _PROCESS_WINDOW_ENTRY_INFORMATION
	{
		HWND   hWnd;
		WCHAR  wzWindowText;
		WCHAR  wzWindowClass;
		BOOL   bVisibal;
		UINT32 ProcessId;
		UINT32 ThreadId;
	} PROCESS_WINDOW_ENTRY_INFORMATION, *PPROCESS_WINDOW_ENTRY_INFORMATION;

	typedef struct _PROCESS_WINDOW_INFORMATION
	{
		UINT32                            NumberOfWindows;
		PROCESS_WINDOW_ENTRY_INFORMATION  WindowEntry[1];
	} PROCESS_WINDOW_INFORMATION, *PPROCESS_WINDOW_INFORMATION;


	class CProcessWindow
	{
	public:
		CProcessWindow(class CGlobal *GlobalObject);
		~CProcessWindow();
		void InitializeProcessWindowList(CListCtrl * ListCtrl);

		BOOL EnumProcessWindow();

		void InsertProcessWindowInfoList(CListCtrl * ListCtrl);

		void QueryProcessWindow(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryProcessWindowCallback(LPARAM lParam);







	private:
		int           m_iColumnCount = 6;		// �����б���
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"���ھ��",				80 },
			{ L"���ڱ���",				140 },
			{ L"��������",				140 },
			{ L"���ڿɼ���",			90 },
			{ L"����Id",				70 },
			{ L"�߳�Id",				70 } };



		std::vector<PROCESS_WINDOW_ENTRY_INFORMATION> m_ProcessWindowEntryVector;


		class CGlobal         *m_Global;

		static CProcessWindow *m_ProcessWindow;



	};



}
