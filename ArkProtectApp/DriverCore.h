#pragma once
#include <vector>
#include <strsafe.h>
#include "Define.h"

namespace ArkProtect
{
	enum eDriverColumn
	{
		dc_DriverName,     // ������
		dc_BaseAddress,    // ����ַID
		dc_Size,           // ��С		
		dc_Object,         // ��������
		dc_DriverPath,     // ����·��
		dc_ServiceName,    // ������
		dc_StartAddress,   // �������
		dc_LoadOrder,      // ����˳��
		dc_Company         // �ļ�����
	};

	typedef struct _DRIVER_ENTRY_INFORMATION
	{
		UINT_PTR  BaseAddress;
		UINT_PTR  Size;
		UINT_PTR  DriverObject;
		UINT_PTR  DirverStartAddress;
		UINT_PTR  LoadOrder;
		WCHAR     wzDriverName[100];
		WCHAR     wzDriverPath[MAX_PATH];
		WCHAR     wzServiceName[MAX_PATH];
		WCHAR     wzCompanyName[MAX_PATH];
	} DRIVER_ENTRY_INFORMATION, *PDRIVER_ENTRY_INFORMATION;

	typedef struct _DRIVER_INFORMATION
	{
		UINT32                          NumberOfDrivers;
		DRIVER_ENTRY_INFORMATION        DriverEntry[1];
	} DRIVER_INFORMATION, *PDRIVER_INFORMATION;


	class CDriverCore
	{
	public:
		CDriverCore(class CGlobal *GlobalObject);
		~CDriverCore();

		void InitializeDriverList(CListCtrl * DriverList);

		void PerfectDriverInfo(PDRIVER_ENTRY_INFORMATION DriverEntry);

		BOOL EnumDriverInfo();

		void InsertDriverInfoList(CListCtrl * ListCtrl);

		void QueryDriverInfo(CListCtrl * ListCtrl);

		BOOL UnloadDriver(UINT_PTR DriverObject);

		static DWORD CALLBACK QueryDriverInfoCallback(LPARAM lParam);

		static DWORD CALLBACK UnloadDriverCallback(LPARAM lParam);

		CString GetDriverPathByAddress(UINT_PTR Address);


		//
		// ���ر���Interface
		//
		inline std::vector<DRIVER_ENTRY_INFORMATION>& DriverEntryVector() { return m_DriverEntryVector; }


	private:

		UINT32        m_DriverCount = 0;
		int           m_iColumnCount = 9;		// �����б���
		COLUMN_STRUCT m_ColumnStruct[9] = {
			{ L"������",		130 },
			{ L"����ַ",		125 },
			{ L"��С",			70 },
			{ L"��������",		125 },
			{ L"����·��",		200 },
			{ L"������",		80 },
			{ L"�������",		125 },
			{ L"����˳��",		65 },
			{ L"�ļ�����",		120 } };


		std::vector<DRIVER_ENTRY_INFORMATION> m_DriverEntryVector;


		class CGlobal       *m_Global;
		static CDriverCore  *m_Driver;

	};




}