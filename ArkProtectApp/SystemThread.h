#pragma once
#include <vector>
#include "Define.h"

namespace ArkProtect
{


	class CSystemThread
	{
	public:
		CSystemThread(class CGlobal *GlobalObject);
		~CSystemThread();




	private:
		int           m_iColumnCount = 9;
		COLUMN_STRUCT m_ColumnStruct[9] = {
			{ L"�߳�ID",				50 },
			{ L"�̶߳���",				125 },
			{ L"Peb",					35 },
			{ L"���ȼ�",				55 },
			{ L"�߳����",				125 },
			{ L"�л�����",				65 },
			{ L"�߳�״̬",				100 },
			{ L"ģ���ļ�",				190 },
			{ L"��Ʒ����",				125 } };

//		std::vector<SYS_CALLBACK_ENTRY_INFORMATION> m_CallbackEntryVector;

		class CGlobal          *m_Global;
	};

}

