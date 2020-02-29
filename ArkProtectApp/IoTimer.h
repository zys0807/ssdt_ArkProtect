#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eIoTimerColumn
	{
		itc_Object,                   // Timer����
		itc_Device,                   // �豸����
		itc_Status,                   // ״̬
		itc_Dispatch,                 // �������
		itc_FilePath,                 // ����
		itc_Company                   // ��Ʒ����
	};

	typedef struct _IO_TIMER_ENTRY_INFORMATION
	{
		UINT_PTR TimerObject;
		UINT_PTR DeviceObject;
		UINT_PTR TimeDispatch;
		UINT_PTR TimerEntry;		// ��ListCtrl��Item�����������ж�
		UINT32   Status;
	} IO_TIMER_ENTRY_INFORMATION, *PIO_TIMER_ENTRY_INFORMATION;

	typedef struct _IO_TIMER_INFORMATION
	{
		UINT_PTR                   NumberOfIoTimers;
		IO_TIMER_ENTRY_INFORMATION IoTimerEntry[1];
	} IO_TIMER_INFORMATION, *PIO_TIMER_INFORMATION;


	class CIoTimer
	{
	public:
		CIoTimer(class CGlobal *GlobalObject);
		~CIoTimer();

		void InitializeIoTimerList(CListCtrl * ListCtrl);

		BOOL EnumIoTimer();

		void InsertIoTimerInfoList(CListCtrl * ListCtrl);

		void QueryIoTimer(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryIoTimerCallback(LPARAM lParam);



	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"��ʱ������",			125 },
			{ L"�豸����",				125 },
			{ L"״̬",					45 },
			{ L"�������",				125 },
			{ L"ģ��·��",				180 },
			{ L"��Ʒ����",				125 } };

		std::vector<IO_TIMER_ENTRY_INFORMATION> m_IoTimerEntryVector;


		class CGlobal     *m_Global;

		class CDriverCore &m_DriverCore;

		static CIoTimer   *m_IoTimer;

	};



}