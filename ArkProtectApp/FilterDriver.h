#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eFilterDriverColumn
	{
		fdc_Type,                  // ����
		fdc_DriverName,            // ��������
		fdc_FilePath,              // �ļ�·��
		fdc_DeviceObject,          // �豸����
		fdc_DeviceName,            // �豸����
		fdc_AttachedDriverName,    // ������������
		fdc_Compay                 // ����
	};


	enum eFilterType
	{
		ft_Unkonw,
		ft_File,				// �ļ�------------
		ft_Disk,                // ����    �ļ����
		ft_Volume,		        // ��  ------------- 
		ft_Keyboard,            // ����
		ft_Mouse,				// ���            Ӳ���ӿ�
		ft_I8042prt,			// ��������
		ft_Tcpip,				// tcpip-------------------
		ft_Ndis,				// ���������ӿ�
		ft_PnpManager,          // ���弴�ù�����       �������
		ft_Tdx,				    // �������
		ft_Raw
	};


	typedef struct _FILTER_DRIVER_ENTRY_INFORMATION
	{
		eFilterType FilterType;
		UINT_PTR    FilterDeviceObject;
		WCHAR       wzFilterDriverName[MAX_PATH];
		WCHAR       wzFilterDeviceName[MAX_PATH];
		WCHAR       wzAttachedDriverName[MAX_PATH];
		WCHAR       wzFilePath[MAX_PATH];
	} FILTER_DRIVER_ENTRY_INFORMATION, *PFILTER_DRIVER_ENTRY_INFORMATION;

	typedef struct _FILTER_DRIVER_INFORMATION
	{
		UINT32                          NumberOfFilterDrivers;
		FILTER_DRIVER_ENTRY_INFORMATION FilterDriverEntry[1];
	} FILTER_DRIVER_INFORMATION, *PFILTER_DRIVER_INFORMATION;

	class CFilterDriver
	{
	public:
		CFilterDriver(class CGlobal *GlobalObject);
		~CFilterDriver();

		void InitializeFilterDriverList(CListCtrl * ListCtrl);

		BOOL EnumFilterDriver();

		void InsertFilterDriverInfoList(CListCtrl * ListCtrl);

		void QueryFilterDriver(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryFilterDriverCallback(LPARAM lParam);


	private:
		int           m_iColumnCount = 7;
		COLUMN_STRUCT m_ColumnStruct[7] = {
			{ L"����",					80 },
			{ L"������������",			125 },
			{ L"��������·��", 			220 },
			{ L"�����豸����",			125 },
			{ L"�����豸����",			125 },
			{ L"������������",			125 },
			{ L"��Ʒ����",				125 } };

		std::vector<FILTER_DRIVER_ENTRY_INFORMATION> m_FilterDriverEntryVector;

		class CGlobal          *m_Global;




		static CFilterDriver *m_FilterDriver;

	};

}
