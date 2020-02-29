#pragma once
#include <Windows.h>
#include <strsafe.h>
#include <winsvc.h>     // ������Ҫ
#include "afxcmn.h"
#include "Define.h"
#include "ProcessCore.h"
#include "ProcessModule.h"
#include "ProcessThread.h"
#include "ProcessHandle.h"
#include "ProcessWindow.h"
#include "ProcessMemory.h"
#include "DriverCore.h"
#include "SystemCallback.h"
#include "FilterDriver.h"
#include "IoTimer.h"
#include "DpcTimer.h"
#include "SsdtHook.h"
#include "SssdtHook.h"
#include "FileCore.h"
#include "RegistryCore.h"

#pragma comment(lib, "Version.lib")      // GetFileVersionInfo ��Ҫ���Ӵ˿�

namespace ArkProtect 
{
	typedef BOOL(WINAPI *pfnIsWow64Process) (HANDLE, PBOOL);

	class CGlobal
	{
	public:
		CGlobal() 
		: m_ProcessCore(this)
		, m_ProcessModule(this)
		, m_ProcessThread(this)
		, m_ProcessHandle(this)
		, m_ProcessWindow(this)
		, m_ProcessMemory(this)
		, m_DriverCore(this)
		, m_RegistryCore(this)
		, m_SystemCallback(this)
		, m_FilterDriver(this)
		, m_IoTimer(this)
		, m_DpcTimer(this)
		, m_SsdtHook(this)
		, m_SssdtHook(this)
		, m_FileCore(this)
		{};
		~CGlobal() {};

		//////////////////////////////////////////////////////////////////////////
		//
		// ͨ�ú���
		//
		BOOL QueryOSBit()
		{
#if defined(_WIN64)
			return TRUE;  // 64λ����ֻ��64bitϵͳ������
#elif defined(_WIN32)
			// 32λ������32/64λϵͳ�����С�
			// ���Ա����ж�
			BOOL bIs64 = FALSE;
			pfnIsWow64Process fnIsWow64Process;

			fnIsWow64Process = (pfnIsWow64Process)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
			if (fnIsWow64Process != NULL)
			{
				return fnIsWow64Process(GetCurrentProcess(), &bIs64) && bIs64;
			}
			return FALSE;
#else
			return FALSE; // Win64��֧��16λϵͳ
#endif
		}


		BOOL LoadNtDriver(WCHAR *wzServiceName, WCHAR *wzDriverPath)
		{
			WCHAR wzDriverFullPath[MAX_PATH] = { 0 };
			GetFullPathName(wzDriverPath, MAX_PATH, wzDriverFullPath, NULL);
		
			// ������������ƹ����������ӣ�����ָ�������ݿ�
			m_ManagerHandle = OpenSCManagerW(NULL,			// ָ������������� NULL ---> ���Ӵ����ؼ�����ķ�����ƹ�����
				NULL,										// ָ��Ҫ�򿪵ķ�����ƹ������ݿ������ NULL ---> SERVICES_ACTIVE_DATABASE���ݿ�
				SC_MANAGER_ALL_ACCESS);						// ָ��������ʿ��ƹ�������Ȩ��
			if (m_ManagerHandle == NULL)						// ����ָ���ķ�����ƹ��������ݿ�ľ��
			{
				return FALSE;
			}

			// ����һ��������󣬲�������ӵ�ָ���ķ�����ƹ��������ݿ�
			m_ServiceHandle = CreateServiceW(m_ManagerHandle,		// ������ƹ������ά���ĵǼ����ݿ�ľ��
				wzServiceName,									// �����������ڴ����Ǽ����ݿ��еĹؼ���
				wzServiceName,									// �������������û������ʶ����
				SERVICE_ALL_ACCESS,								// ���񷵻�����		����Ȩ��
				SERVICE_KERNEL_DRIVER,							// ��������			�������������
				SERVICE_DEMAND_START,							// �����ʱ����		���ɷ��������SCM�����ķ���
				SERVICE_ERROR_NORMAL,							// ��������ʧ�ܵ����س̶�
				wzDriverFullPath,								// �������������ļ�·��
				NULL, NULL, NULL, NULL, NULL);
			if (m_ServiceHandle == NULL)							// ���ط�����
			{
				if (ERROR_SERVICE_EXISTS == GetLastError())		// �Ѵ�����ͬ�ķ���
				{
					// �Ǿʹ򿪷���
					m_ServiceHandle = OpenServiceW(m_ManagerHandle, wzServiceName, SERVICE_ALL_ACCESS);
					if (m_ServiceHandle == NULL)
					{
						return FALSE;
					}
				}
				else
				{
					return FALSE;
				}
			}

			// ��������
			BOOL bOk = StartServiceW(m_ServiceHandle, 0, NULL);
			if (!bOk)
			{
				int a = GetLastError();
				if ((GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
					|| GetLastError() == ERROR_IO_PENDING)
				{
					return FALSE;
				}
			}

			m_bDriverService = TRUE;

			return TRUE;
		}


		// ж����������
		void UnloadNTDriver(WCHAR *wzServiceName)
		{
			if (m_ServiceHandle)
			{
				SERVICE_STATUS ServiceStatus;

				// ֹͣ����
				BOOL bOk = ControlService(m_ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);
				if (bOk && m_bDriverService)
				{
					// ɾ������
					bOk = DeleteService(m_ServiceHandle);
				}

				CloseServiceHandle(m_ServiceHandle);
			}		

			if (m_ManagerHandle)
			{
				CloseServiceHandle(m_ManagerHandle);
			}
		}


		void UpdateStatusBarTip(LPCWSTR wzBuffer)
		{
			::SendMessage(this->AppDlg->m_hWnd, sb_Tip, 0, (LPARAM)wzBuffer);
		}


		void UpdateStatusBarDetail(LPCWSTR wzBuffer)
		{
			::SendMessage(this->AppDlg->m_hWnd, sb_Detail, 0, (LPARAM)wzBuffer);
		}


		// ����ļ�����
		CString GetFileCompanyName(CString strFilePath)
		{
			CString strCompanyName = 0;;

			if (strFilePath.IsEmpty())
			{
				return NULL;
			}

			// ������Idle System
			if (!strFilePath.CompareNoCase(L"Idle") || !strFilePath.CompareNoCase(L"System"))
			{
				return NULL;
			}

			struct LANGANDCODEPAGE {
				WORD wLanguage;
				WORD wCodePage;
			} *lpTranslate;

			LPWSTR lpstrFilename = strFilePath.GetBuffer();
			DWORD  dwHandle = 0;
			DWORD  dwVerInfoSize = GetFileVersionInfoSizeW(lpstrFilename, &dwHandle);

			if (dwVerInfoSize)
			{
				LPVOID Buffer = malloc(sizeof(UINT8) * dwVerInfoSize);

				if (Buffer)
				{
					if (GetFileVersionInfo(lpstrFilename, dwHandle, dwVerInfoSize, Buffer))
					{
						UINT cbTranslate = 0;

						if (VerQueryValue(Buffer, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate))
						{
							LPCWSTR lpwszBlock = 0;
							UINT    cbSizeBuf = 0;
							WCHAR   wzSubBlock[MAX_PATH] = { 0 };

							if ((cbTranslate / sizeof(struct LANGANDCODEPAGE)) > 0)
							{
								StringCchPrintf(wzSubBlock, sizeof(wzSubBlock) / sizeof(WCHAR),
									L"\\StringFileInfo\\%04x%04x\\CompanyName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
							}

							if (VerQueryValue(Buffer, wzSubBlock, (LPVOID*)&lpwszBlock, &cbSizeBuf))
							{
								WCHAR wzCompanyName[MAX_PATH] = { 0 };

								StringCchCopy(wzCompanyName, MAX_PATH / sizeof(WCHAR), (LPCWSTR)lpwszBlock);   // ��ϵͳ���ڴ�����ݿ����������Լ��ڴ浱��
								strCompanyName = wzCompanyName;
							}
						}
					}
					free(Buffer);
				}
			}

			return strCompanyName;
		}


		CString GetLongPath(CString strFilePath)
		{
			if (strFilePath.Find(L'~') != -1)
			{
				WCHAR wzLongPath[MAX_PATH] = { 0 };
				DWORD dwReturn = GetLongPathName(strFilePath, wzLongPath, MAX_PATH);
				if (dwReturn < MAX_PATH && dwReturn != 0)
				{
					strFilePath = wzLongPath;
				}
			}

			return strFilePath;
		}


		CString TrimPath(WCHAR *wzFilePath)
		{
			CString strFilePath;

			// ���磺"C:\\"
			if (wzFilePath[1] == ':' && wzFilePath[2] == '\\')
			{
				strFilePath = wzFilePath;
			}
			else if (wcslen(wzFilePath) > wcslen(L"\\SystemRoot\\") &&
				!_wcsnicmp(wzFilePath, L"\\SystemRoot\\", wcslen(L"\\SystemRoot\\")))
			{
				WCHAR wzSystemDirectory[MAX_PATH] = { 0 };
				GetWindowsDirectory(wzSystemDirectory, MAX_PATH);
				strFilePath.Format(L"%s\\%s", wzSystemDirectory, wzFilePath + wcslen(L"\\SystemRoot\\"));
			}
			else if (wcslen(wzFilePath) > wcslen(L"system32\\") &&
				!_wcsnicmp(wzFilePath, L"system32\\", wcslen(L"system32\\")))
			{
				WCHAR wzSystemDirectory[MAX_PATH] = { 0 };
				GetWindowsDirectory(wzSystemDirectory, MAX_PATH);
				strFilePath.Format(L"%s\\%s", wzSystemDirectory, wzFilePath/* + wcslen(L"system32\\")*/);
			}
			else if (wcslen(wzFilePath) > wcslen(L"\\??\\") &&
				!_wcsnicmp(wzFilePath, L"\\??\\", wcslen(L"\\??\\")))
			{
				strFilePath = wzFilePath + wcslen(L"\\??\\");
			}
			else if (wcslen(wzFilePath) > wcslen(L"%ProgramFiles%") &&
				!_wcsnicmp(wzFilePath, L"%ProgramFiles%", wcslen(L"%ProgramFiles%")))
			{
				WCHAR wzSystemDirectory[MAX_PATH] = { 0 };
				if (GetWindowsDirectory(wzSystemDirectory, MAX_PATH) != 0)
				{
					strFilePath = wzSystemDirectory;
					strFilePath = strFilePath.Left(strFilePath.Find('\\'));
					strFilePath += L"\\Program Files";
					strFilePath += wzFilePath + wcslen(L"%ProgramFiles%");
				}
			}
			else
			{
				strFilePath = wzFilePath;
			}

			strFilePath = GetLongPath(strFilePath);

			return strFilePath;
		}


		// ����ļ�����
		void AddFileIcon(WCHAR *FilePath, CImageList *ImageList)
		{
			SHFILEINFO ShFileInfo = { 0 };

			SHGetFileInfo(FilePath, FILE_ATTRIBUTE_NORMAL,
				&ShFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);

			HICON  hIcon = ShFileInfo.hIcon;

			ImageList->Add(hIcon);
		}


		void LocationInExplorer(CString strFilePath)
		{
			if (!strFilePath.IsEmpty() && PathFileExists(strFilePath))
			{
				CString strCmd;
				strCmd.Format(L"/select,%s", strFilePath);
				ShellExecuteW(NULL, L"open", L"explorer.exe", strCmd, NULL, SW_SHOW);
			}
			else
			{
				MessageBox(NULL, L"�ļ�·������", L"ArkProtect", MB_OK | MB_ICONERROR);
			}
		}


		void CheckFileProperty(CString strFilePath)
		{
			if (!strFilePath.IsEmpty())
			{
				SHELLEXECUTEINFO ExecInfo = { 0 };

				ExecInfo.fMask = SEE_MASK_INVOKEIDLIST;
				ExecInfo.cbSize = sizeof(ExecInfo);
				ExecInfo.hwnd = NULL;
				ExecInfo.lpVerb = L"properties";
				ExecInfo.lpFile = strFilePath.GetBuffer();
				ExecInfo.lpParameters = NULL;
				ExecInfo.lpDirectory = NULL;
				ExecInfo.nShow = SW_SHOWNORMAL;
				ExecInfo.hProcess = NULL;
				ExecInfo.lpIDList = 0;
				ExecInfo.hInstApp = 0;
				ShellExecuteEx(&ExecInfo);
			}
			else
			{
				MessageBox(NULL, L"�ļ�·������", L"ArkProtect", MB_OK | MB_ICONERROR);
			}
		}


		BOOL GetSaveTextFileName(CString & strFilePath)
		{
			CFileDialog FileDlg(
				FALSE,		// save
				0,
				strFilePath,
				0,
				L"�ı��ļ� (*.txt)|*.txt|�����ļ� (*.*)|*.*||",
				0);

			if (FileDlg.DoModal() == IDOK)
			{
				strFilePath = FileDlg.GetFileName();

				CString strTemp = strFilePath.Right((int)wcslen(L".txt"));

				// û����չ�����Լ�������
				if (strTemp.CompareNoCase(L".txt") != 0)
				{
					strFilePath += L".txt";
				}

				if (!PathFileExists(strFilePath))
				{
					return TRUE;
				}
				else if (PathFileExists(strFilePath) && MessageBox(NULL, L"�ļ��Ѿ����ڣ��Ƿ񸲸ǣ�", L"ArkProtect", MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					if (!DeleteFile(strFilePath))
					{
						MessageBox(NULL, L"�����ļ�ʧ�ܡ�", L"ArkProtect", MB_OK | MB_ICONERROR);
						return FALSE;
					}

					return TRUE;
				}
			}

			return FALSE;
		}

		void ExportInformationInText(CListCtrl & ListCtrl)
		{
			if (ListCtrl.GetItemCount() > 0)
			{
				CString strFilePath;
				BOOL bOk = GetSaveTextFileName(strFilePath);
				if (!bOk)
				{
					return;
				}

				CFile File;

				TRY
				{
					if (File.Open(strFilePath, CFile::modeCreate | CFile::modeWrite | CFile::modeNoTruncate, NULL))
					{
						int i = 0;
						LVCOLUMN ColumnData;
						CString  strColumnName;
						int      ColumnNum = 0;
						CString  strCloumn;
						CHAR     szColumn[0x1000] = { 0 };
						WCHAR    wzColumn[0x1000] = { 0 };

						ColumnData.mask = LVCF_TEXT;
						ColumnData.cchTextMax = 100;	// ��������С
						ColumnData.pszText = strColumnName.GetBuffer(100);		// ����������

						for (i = 0; ListCtrl.GetColumn(i, &ColumnData); i++)
						{
							strCloumn = strCloumn + ColumnData.pszText + L"      |      ";
						}

						strCloumn += "\r\n";
						int  iLength = strCloumn.GetLength();
						wcsncpy_s(wzColumn, 0x1000, strCloumn.GetBuffer(), iLength);
						strCloumn.ReleaseBuffer();
						WideCharToMultiByte(CP_ACP, 0, wzColumn, -1, szColumn, 0x1000, NULL, NULL);		// ˫��ת����
						File.Write(szColumn, (UINT)strlen(szColumn));

						strColumnName.ReleaseBuffer();
						ColumnNum = i;

						//������ȡ��ShowList �ı���

						for (int iItemIndex = 0; iItemIndex < ListCtrl.GetItemCount(); iItemIndex++)		// ����ÿһ��
						{
							CHAR  szColumn[0x1000] = { 0 };
							WCHAR wzColumn[0x1000] = { 0 };
							CString strItem;

							for (i = 0; i < ColumnNum; i++)
							{
								strItem = strItem + ListCtrl.GetItemText(iItemIndex, i) + L"      |      ";		// ���ÿһ��
							}

							strItem += "\r\n";
							iLength = strItem.GetLength();
							wcsncpy_s(wzColumn, 0x1000, strItem.GetBuffer(), iLength);
							strItem.ReleaseBuffer();
							WideCharToMultiByte(CP_ACP, 0, wzColumn, -1, szColumn, 0x1000, NULL, NULL);
							File.Write(szColumn, (UINT)strlen(szColumn));
						}

						File.Close();
					}
				}
					CATCH_ALL(e)
				{
					File.Abort();
				}
				END_CATCH_ALL

				if (PathFileExists(strFilePath))
				{
					ShellExecuteW(NULL, L"open", strFilePath, NULL, NULL, SW_SHOW);
				}
				else
				{
					::MessageBox(NULL, L"�������ı��ļ�ʧ�ܡ�", L"ArkProtect", MB_OK | MB_ICONERROR);
				}
			}
		}


		//////////////////////////////////////////////////////////////////////////

		//
		// ���ر���Interface
		//
		inline CProcessCore&     ProcessCore()   { return m_ProcessCore; }
		inline CProcessModule&   ProcessModule() { return m_ProcessModule; }
		inline CProcessThread&   ProcessThread() { return m_ProcessThread; }
		inline CProcessHandle&   ProcessHandle() { return m_ProcessHandle; }
		inline CProcessWindow&   ProcessWindow() { return m_ProcessWindow; }
		inline CProcessMemory&   ProcessMemory() { return m_ProcessMemory; }
		inline CDriverCore&      DriverCore()    { return m_DriverCore; }
		inline CSystemCallback&  SystemCallback(){ return m_SystemCallback; }
		inline CFilterDriver&    FilterDriver()  { return m_FilterDriver; }
		inline CIoTimer&         IoTimer()       { return m_IoTimer; }
		inline CDpcTimer&        DpcTimer()      { return m_DpcTimer; }
		inline CSsdtHook&        SsdtHook()      { return m_SsdtHook; }
		inline CSssdtHook&       SssdtHook()     { return m_SssdtHook; }
		inline CFileCore&        FileCore()      { return m_FileCore; }

		inline CRegistryCore&    RegistryCore()  { return m_RegistryCore; }


		CWnd *AppDlg = NULL;           // ����������ָ��
		CWnd *m_ProcessDlg = NULL;     // �������ģ�鴰��ָ��
		CWnd *m_DriverDlg = NULL;      // ��������ģ�鴰��ָ��
		CWnd *m_KernelDlg = NULL;      // �����ں�ģ�鴰��ָ��
		CWnd *m_HookDlg = NULL;        // �����ں˹��Ӵ���ָ��
		CWnd *m_RegistryDlg = NULL;    // ����ע���ģ�鴰��ָ��
		

		int iDpix = 0;               // Logical pixels/inch in X
		int iDpiy = 0;               // Logical pixels/inch in Y

		int iResizeX = 0;
		int iResizeY = 0;

		BOOL      m_bIsRequestNow = FALSE;    // ��ǰ�Ƿ����������㷢������
		HANDLE    m_DeviceHandle = NULL;    // ���ǵ������豸������
		SC_HANDLE m_ManagerHandle = NULL;	// SCM�������ľ��
		SC_HANDLE m_ServiceHandle = NULL;	// NT��������ķ�����
		BOOL      m_bDriverService = FALSE; // ָʾ�������������Ƿ�����


	private:
		//
		// �������
		//
		CProcessCore       m_ProcessCore;
		CProcessModule     m_ProcessModule;
		CProcessThread     m_ProcessThread;
		CProcessHandle     m_ProcessHandle;
		CProcessWindow     m_ProcessWindow;
		CProcessMemory     m_ProcessMemory;

		//
		// �������
		//
		CDriverCore        m_DriverCore;


		//
		// �ں����
		//
		CSystemCallback    m_SystemCallback;
		CFilterDriver      m_FilterDriver;
		CIoTimer           m_IoTimer;
		CDpcTimer          m_DpcTimer;

		//
		// �������
		//
		CSsdtHook          m_SsdtHook;
		CSssdtHook         m_SssdtHook;

		//
		// �ļ����
		//
		CFileCore          m_FileCore;

		//
		// ע������
		//
		CRegistryCore      m_RegistryCore;

	};
}

