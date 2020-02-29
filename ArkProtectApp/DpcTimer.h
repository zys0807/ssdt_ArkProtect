#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{
	enum eDpcTimerColumn
	{
		dtc_Object,                   // Timer����
		dtc_Device,                   // �豸����
		dtc_Cycle,                    // ����
		dtc_Dispatch,                 // �������
		dtc_FilePath,                 // ����
		dtc_Company                   // ��Ʒ����
	};

	typedef struct _DPC_TIMER_ENTRY_INFORMATION
	{
		UINT_PTR TimerObject;
		UINT_PTR RealDpc;
		UINT_PTR Cycle;       // ����
		UINT_PTR TimeDispatch;
	} DPC_TIMER_ENTRY_INFORMATION, *PDPC_TIMER_ENTRY_INFORMATION;

	typedef struct _DPC_TIMER_INFORMATION
	{
		UINT32                      NumberOfDpcTimers;
		DPC_TIMER_ENTRY_INFORMATION DpcTimerEntry[1];
	} DPC_TIMER_INFORMATION, *PDPC_TIMER_INFORMATION;

	class CDpcTimer
	{
	public:
		CDpcTimer(class CGlobal *GlobalObject);
		~CDpcTimer();

		void InitializeDpcTimerList(CListCtrl * ListCtrl);

		BOOL EnumDpcTimer();

		void InsertDpcTimerInfoList(CListCtrl * ListCtrl);

		void QueryDpcTimer(CListCtrl * ListCtrl);

		static DWORD CALLBACK QueryDpcTimerCallback(LPARAM lParam);

	private:
		int           m_iColumnCount = 6;
		COLUMN_STRUCT m_ColumnStruct[6] = {
			{ L"��ʱ������",			125 },
			{ L"�豸����",				125 },
			{ L"��������(s)",			70 },
			{ L"�������",				125 },
			{ L"ģ���ļ�",				155 },
			{ L"��Ʒ����",				125 } };

		std::vector<DPC_TIMER_ENTRY_INFORMATION> m_DpcTimerEntryVector;


		class CGlobal     *m_Global;

		class CDriverCore &m_DriverCore;

		static CDpcTimer  *m_DpcTimer;

	};

}
