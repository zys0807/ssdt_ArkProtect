#include "ProcessThread.h"

extern DYNAMIC_DATA	g_DynamicData;
extern PEPROCESS    g_SystemEProcess;

typedef
NTSTATUS
(*pfnPspTerminateThreadByPointer)(
	IN PETHREAD Thread,
	IN NTSTATUS ExitStatus,
	IN BOOLEAN DirectTerminate);



/************************************************************************
*  Name : APChangeThreadMode
*  Param: EThread			�߳���ṹ
*  Param: WantedMode	    ��Ҫ��ģʽ
*  Ret  : UINT8             ������ǰģʽ
*  �޸��߳�ģʽΪĿ��ģʽ
************************************************************************/
UINT8
APChangeThreadMode(IN PETHREAD EThread, IN UINT8 WantedMode)
{
	// ����ԭ��ģʽ
	PUINT8 PreviousMode = (PUINT8)EThread + g_DynamicData.PreviousMode;
	// �޸�ΪWantedMode
	*PreviousMode = WantedMode;
	return *PreviousMode;
}


/************************************************************************
*  Name : APIsThreadInList
*  Param: BaseAddress			ģ�����ַ��OUT��
*  Param: ModuleSize			ģ���С��IN��
*  Ret  : NTSTATUS
*  ͨ��FileObject��ý�������·��
************************************************************************/
BOOLEAN
APIsThreadInList(IN PETHREAD EThread, IN PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	BOOLEAN bOk = FALSE;
	UINT32  i = 0;
	ThreadCount = pti->NumberOfThreads > ThreadCount ? ThreadCount : pti->NumberOfThreads;

	if (EThread == NULL || pti == NULL)
	{
		return TRUE;
	}

	for (i = 0; i < ThreadCount; i++)
	{
		if (pti->ThreadEntry[i].EThread == (UINT_PTR)EThread)	// ƥ�����˵���Ѿ�����
		{
			bOk = TRUE;
			break;
		}
	}
	return bOk;
}


/************************************************************************
*  Name : APGetThreadStartAddress
*  Param: EThread			�߳������
*  Ret  : NTSTATUS
*  ͨ��FileObject��ý�������·��
************************************************************************/
// StartAddress��������̵߳�������ַ�������������߳�������ַ������ڵ�ַ��Ҳ���������ڴ����̵߳�֮��ָ������ں����ĵ�ַ
// Win32StartAddress��������windows��ϵͳ���յ����߳�������ַ����CreateThread�������յ����߳�������ַ
//  StartAddress�������ͨ����ϵͳDLL�е��߳�������ַ�������������ͬ��(����kernel32.dll�е�BaseProcessStart��BaseThreadStart����)��
// ��Win32StartAddress���а����Ĳ�������windows��ϵͳ���յ����߳�������ַ����CreateThread��ָ�����Ǹ�������ڵ�ַ��
UINT_PTR
APGetThreadStartAddress(IN PETHREAD EThread)
{
	UINT_PTR StartAddress = 0;

	if (!EThread ||
		!MmIsAddressValid(EThread))
	{
		return StartAddress;
	}

	__try
	{
		StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.StartAddress);

		if (*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.SameThreadApcFlags) & 2)	// StartAddressInvalid
		{
			StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Win32StartAddress);	// �߳���ʵ��ڵ�ַ
		}
		else
		{
			if (*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.StartAddress))
			{
				StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.StartAddress);
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Catch Exception\r\n");
		StartAddress = 0;
	}

	return StartAddress;
}


/************************************************************************
*  Name : APGetProcessThreadInfo
*  Param: EThread			�߳������
*  Param: EProcess			���������
*  Param: pti			    
*  Param: ThreadCount		
*  Ret  : NTSTATUS
*  ͨ��FileObject��ý�������·��
************************************************************************/
VOID
APGetProcessThreadInfo(IN PETHREAD EThread, IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	if (EThread && EProcess && MmIsAddressValid((PVOID)EThread))
	{
		// ͨ���߳����õ�ǰ������
		PEPROCESS EThreadEProcess = NULL;
		if (IoThreadToProcess)
		{
			EThreadEProcess = IoThreadToProcess(EThread);
		}
		else
		{
			EThreadEProcess = (PEPROCESS)*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Process);
		}

		if (EProcess == EThreadEProcess &&   // �жϴ������������Ƿ����߳�����������Ľ��������
			!APIsThreadInList(EThread, pti, ThreadCount) &&
			NT_SUCCESS(ObReferenceObjectByPointer(EThread, 0, NULL, KernelMode)))
		{
			UINT32 CurrentCount = pti->NumberOfThreads;
			if (ThreadCount > CurrentCount)
			{
				if (PsGetThreadId)
				{
					pti->ThreadEntry[CurrentCount].ThreadId = (UINT32)(UINT_PTR)PsGetThreadId(EThread);
				}
				else
				{
					pti->ThreadEntry[CurrentCount].ThreadId = (UINT32)*(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Cid + sizeof(PVOID));
				}

				pti->ThreadEntry[CurrentCount].EThread = (UINT_PTR)EThread;
				//pti->ThreadEntry[CurrentCount].Win32StartAddress = APGetThreadStartAddress(EThread);
				pti->ThreadEntry[CurrentCount].Win32StartAddress = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Win32StartAddress);	// �߳���ʵ��ڵ�ַ	
				pti->ThreadEntry[CurrentCount].Teb = *(PUINT_PTR)((PUINT8)EThread + g_DynamicData.Teb);
				pti->ThreadEntry[CurrentCount].Priority = *((PUINT8)EThread + g_DynamicData.Priority);
				pti->ThreadEntry[CurrentCount].ContextSwitches = *(PUINT32)((PUINT8)EThread + g_DynamicData.ContextSwitches);
				pti->ThreadEntry[CurrentCount].State = *((PUINT8)EThread + g_DynamicData.State);
			}

			pti->NumberOfThreads++;

			ObDereferenceObject(EThread);
		}
	}
}


/************************************************************************
*  Name : APEnumProcessThreadByIterateFirstLevelHandleTable
*  Param: TableCode
*  Param: EProcess
*  Param: pti
*  Param: ThreadCount
*  Ret  : VOID
*  ����һ����
************************************************************************/
VOID
APEnumProcessThreadByIterateFirstLevelHandleTable(IN UINT_PTR TableCode, IN PEPROCESS EProcess,
	OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	/*
	Win7 x64 ��16�ֽ�
	1: kd> dq fffff8a0`00fc2000
	fffff8a0`00fc2000  00000000`00000000 00000000`fffffffe
	fffff8a0`00fc2010  fffffa80`1acb3041 fffff780`00000000
	fffff8a0`00fc2020  fffffa80`1a989b61 00000000`00000000
	fffff8a0`00fc2030  fffffa80`1a98a301 00000000`00000000
	fffff8a0`00fc2040  fffffa80`1a98d061 fffff880`00000000
	fffff8a0`00fc2050  fffffa80`1ab8a061 fffffa80`00000000
	fffff8a0`00fc2060  fffffa80`1a99a061 fffff8a0`00000000
	fffff8a0`00fc2070  fffffa80`1a99bb61 00000000`00000000

	Win7 x86 ��8�ֽ�
	0: kd> dd 8b404000
	8b404000  00000000 fffffffe 863d08a9 00000000		// ��ǰ8���ֽ�
	8b404010  863d05d1 00000000 863efd49 00000000
	8b404020  863f3bb9 00000000 863eb8d9 00000000
	8b404030  863f7021 00000000 863f74a9 00000000
	8b404040  863f3021 00000000 863f34d1 00000000
	8b404050  863fb021 00000000 863fb919 00000000
	8b404060  863fb641 00000000 863fb369 00000000
	8b404070  863f5021 00000000 863f5d49 00000000
	*/
	UINT32 i = 0;
	PHANDLE_TABLE_ENTRY	HandleTableEntry = (PHANDLE_TABLE_ENTRY)(*(PUINT_PTR)TableCode + g_DynamicData.HandleTableEntryOffset);

	for ( i = 0; i < 0x200; i++)		// 512������
	{
		if (MmIsAddressValid((PVOID)&(HandleTableEntry->NextFreeTableEntry)))
		{
			if (HandleTableEntry->NextFreeTableEntry == 0 &&
				HandleTableEntry->Object != NULL &&
				MmIsAddressValid(HandleTableEntry->Object))
			{
				PVOID Object = (PVOID)(((UINT_PTR)HandleTableEntry->Object) & 0xFFFFFFFFFFFFFFF8);
				// ��FillProcessThreadInfo���ж��ɴ����Objectת�ɵ�EProcess�Ƿ���Ŀ��EProcess
				APGetProcessThreadInfo((PETHREAD)Object, EProcess, pti, ThreadCount);
			}
		}
		HandleTableEntry++;
	}
}


/************************************************************************
*  Name : APEnumProcessThreadByIterateSecondLevelHandleTable
*  Param: TableCode
*  Param: EProcess
*  Param: pti
*  Param: ThreadCount
*  Ret  : VOID
*  ����������
************************************************************************/
VOID
APEnumProcessThreadByIterateSecondLevelHandleTable(IN UINT_PTR TableCode, IN PEPROCESS EProcess,
	OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	/*
	Win7 x64
	2: kd> dq 0xfffff8a0`00fc5000
	fffff8a0`00fc5000  fffff8a0`00005000 fffff8a0`00fc6000
	fffff8a0`00fc5010  fffff8a0`0180b000 fffff8a0`02792000
	fffff8a0`00fc5020  00000000`00000000 00000000`00000000

	Win7 x86
	0: kd> dd 0xa4aaf000
	a4aaf000  8b404000 a4a56000 00000000 00000000
	*/

	do
	{
		APEnumProcessThreadByIterateFirstLevelHandleTable(TableCode, EProcess, pti, ThreadCount);		// fffff8a0`00fc5000..../ fffff8a0`00fc5008....
		TableCode += sizeof(UINT_PTR);

	} while (*(PUINT_PTR)TableCode != 0 && MmIsAddressValid((PVOID)*(PUINT_PTR)TableCode));

}

/************************************************************************
*  Name : APEnumProcessThreadByIterateThirdLevelHandleTable
*  Param: TableCode
*  Param: EProcess
*  Param: pti
*  Param: ThreadCount
*  Ret  : VOID
*  ����������
************************************************************************/
VOID
APEnumProcessThreadByIterateThirdLevelHandleTable(IN UINT_PTR TableCode, IN PEPROCESS EProcess,
	OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	do
	{
		APEnumProcessThreadByIterateSecondLevelHandleTable(TableCode, EProcess, pti, ThreadCount);
		TableCode += sizeof(UINT_PTR);

	} while (*(PUINT_PTR)TableCode != 0 && MmIsAddressValid((PVOID)*(PUINT_PTR)TableCode));

}


/************************************************************************
*  Name : APEnumProcessThreadByIteratePspCidTable
*  Param: EProcess			      ���̽ṹ��
*  Param: pti
*  Param: ThreadCount
*  Ret  : NTSTATUS
*  ͨ������EProcess��KProcess��ThreadListHead����
************************************************************************/
NTSTATUS
APEnumProcessThreadByIteratePspCidTable(IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;

	// ����֮ǰ��ģʽ��ת��KernelMode
	PETHREAD EThread = PsGetCurrentThread();
	UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

	UINT_PTR PspCidTable = APGetPspCidTableAddress();

	APChangeThreadMode(EThread, PreviousMode);

	// EnumHandleTable
	if (PspCidTable)
	{
		PHANDLE_TABLE	HandleTable = NULL;

		HandleTable = (PHANDLE_TABLE)(*(PUINT_PTR)PspCidTable);  	// HandleTable = fffff8a0`00004910
		if (HandleTable && MmIsAddressValid((PVOID)HandleTable))
		{
			UINT8			TableLevel = 0;		// ָʾ��������
			UINT_PTR		TableCode = 0;			// ��ַ��ž�����׵�ַ

			TableCode = HandleTable->TableCode & 0xFFFFFFFFFFFFFFFC;	// TableCode = 0xfffff8a0`00fc5000
			TableLevel = HandleTable->TableCode & 0x03;	                // TableLevel = 0x01

			if (TableCode && MmIsAddressValid((PVOID)TableCode))
			{
				switch (TableLevel)
				{
				case 0:
				{
					// һ���
					APEnumProcessThreadByIterateFirstLevelHandleTable(TableCode, EProcess, pti, ThreadCount);
					break;
				}
				case 1:
				{
					// �����
					APEnumProcessThreadByIterateSecondLevelHandleTable(TableCode, EProcess, pti, ThreadCount);
					break;
				}
				case 2:
				{
					// �����
					APEnumProcessThreadByIterateThirdLevelHandleTable(TableCode, EProcess, pti, ThreadCount);
					break;
				}
				default:
					break;
				}
			}
		}
	}

	if (pti->NumberOfThreads)
	{
		Status = STATUS_SUCCESS;
	}

	DbgPrint("EnumProcessThread by iterate PspCidTable\r\n");

	return Status;
}



/************************************************************************
*  Name : APEnumProcessThreadByIterateThreadListHead
*  Param: EProcess			      ���̽ṹ��
*  Param: pti			         
*  Param: ThreadCount
*  Ret  : NTSTATUS
*  ͨ������EProcess��KProcess��ThreadListHead����
************************************************************************/
NTSTATUS
APEnumProcessThreadByIterateThreadListHead(IN PEPROCESS EProcess, OUT PPROCESS_THREAD_INFORMATION pti, IN UINT32 ThreadCount)
{
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;
	PLIST_ENTRY ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_KPROCESS);
	
	if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
	{
	//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
		UINT_PTR    MaxCount = PAGE_SIZE * 2;
		PLIST_ENTRY ThreadListEntry;
		for ( ThreadListEntry = ThreadListHead->Flink;
			MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
			ThreadListEntry = ThreadListEntry->Flink)
		{
			PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
			APGetProcessThreadInfo(EThread, EProcess, pti, ThreadCount);
		}

	//	KeLowerIrql(OldIrql);
	}

	ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_EPROCESS);
	if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
	{
	//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
		UINT_PTR    MaxCount = PAGE_SIZE * 2;
		PLIST_ENTRY ThreadListEntry;
		for ( ThreadListEntry = ThreadListHead->Flink;
			MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
			ThreadListEntry = ThreadListEntry->Flink)
		{
			PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
			APGetProcessThreadInfo(EThread, EProcess, pti, ThreadCount);
		}

	//	KeLowerIrql(OldIrql);
	}
	
	if (pti->NumberOfThreads)
	{
		Status = STATUS_SUCCESS;
	}

	DbgPrint("EnumProcessThread by iterate ThreadListHead\r\n");

	return Status;
}


/************************************************************************
*  Name : APEnumProcessThread
*  Param: ProcessId			      ���̽ṹ��
*  Param: OutputBuffer            Ring3�ڴ�
*  Param: OutputLength
*  Ret  : NTSTATUS
*  ͨ������EProcess��KProcess��ThreadListHead����
************************************************************************/
NTSTATUS
APEnumProcessThread(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	PPROCESS_THREAD_INFORMATION pti = (PPROCESS_THREAD_INFORMATION)OutputBuffer;
	UINT32    ThreadCount = (OutputLength - sizeof(PROCESS_THREAD_INFORMATION)) / sizeof(PROCESS_THREAD_ENTRY_INFORMATION);
	PEPROCESS EProcess = NULL;

	if (ProcessId == 0)
	{
		return Status;
	}
	else if (ProcessId == 4)
	{
		EProcess = g_SystemEProcess;
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &EProcess);
	}

	if (NT_SUCCESS(Status) && APIsValidProcess(EProcess))
	{
		Status = APEnumProcessThreadByIteratePspCidTable(EProcess, pti, ThreadCount);
		if (NT_SUCCESS(Status))
		{
			if (ThreadCount >= pti->NumberOfThreads)
			{
				Status = STATUS_SUCCESS;
			}
			else
			{
				Status = STATUS_BUFFER_TOO_SMALL;	// ��ring3�����ڴ治������Ϣ
			}
		}
		else
		{
			Status = APEnumProcessThreadByIterateThreadListHead(EProcess, pti, ThreadCount);
			if (NT_SUCCESS(Status))
			{
				if (ThreadCount >= pti->NumberOfThreads)
				{
					Status = STATUS_SUCCESS;
				}
				else
				{
					Status = STATUS_BUFFER_TOO_SMALL;	// ��ring3�����ڴ治������Ϣ
				}
			}
		}
	}

	if (EProcess && EProcess != g_SystemEProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}


/************************************************************************
*  Name : APGetPspTerminateThreadByPointerAddress
*  Param: void
*  Ret  : UINT_PTR
*  ͨ��PsTerminateSystemThread���PspTerminateThreadByPointer������ַ
************************************************************************/
UINT_PTR
APGetPspTerminateThreadByPointerAddress()
{
	PUINT8	StartSearchAddress = (PUINT8)PsTerminateSystemThread;
	PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
	PUINT8	i = NULL;
	UINT8   v1 = 0, v2 = 0;
	INT32   iOffset = 0;    // 002320c7 ƫ�Ʋ��ᳬ��4�ֽ�

	// ͨ��PsTerminateSystemThread ���������� ������ PspTerminateThreadByPointer
	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		/*
		Win7 x64
		1: kd> u PsTerminateSystemThread
		nt!PsTerminateSystemThread:
		fffff800`0411d4a0 4883ec28        sub     rsp,28h
		fffff800`0411d4a4 8bd1            mov     edx,ecx
		fffff800`0411d4a6 65488b0c2588010000 mov   rcx,qword ptr gs:[188h]
		fffff800`0411d4af 0fba614c0d      bt      dword ptr [rcx+4Ch],0Dh
		fffff800`0411d4b4 0f83431d0b00    jae     nt! ?? ::NNGAKEGL::`string'+0x28580 (fffff800`041cf1fd)
		fffff800`0411d4ba 41b001          mov     r8b,1
		fffff800`0411d4bd e822300400      call    nt!PspTerminateThreadByPointer (fffff800`041604e4)
		fffff800`0411d4c2 90              nop

		0: kd> u fffff800`041604e4
		nt!PspTerminateThreadByPointer:
		fffff800`041604e4 48895c2408      mov     qword ptr [rsp+8],rbx
		fffff800`041604e9 48896c2410      mov     qword ptr [rsp+10h],rbp
		fffff800`041604ee 4889742418      mov     qword ptr [rsp+18h],rsi
		fffff800`041604f3 57              push    rdi
		fffff800`041604f4 4883ec40        sub     rsp,40h

		*/

		if (MmIsAddressValid(i) && MmIsAddressValid(i + 5))
		{
			v1 = *i;
			v2 = *(i + 5);
			if (v1 == 0xe8 && v2 == 0x90)		// Ӳ����  e8 call 
			{
				RtlCopyMemory(&iOffset, i + 1, 4);
				return (UINT_PTR)(iOffset + (UINT64)i + 5);
			}
		}
	}

	return 0;	
}



/************************************************************************
*  Name : APTerminateProcessByIterateThreadListHead
*  Param: EProcess
*  Ret  : NTSTATUS
*  ����������������̣߳�ͨ��PspTerminateThreadByPointer�����̣߳��Ӷ���������
************************************************************************/
NTSTATUS
APTerminateProcessByIterateThreadListHead(IN PEPROCESS EProcess)
{
	NTSTATUS    Status = STATUS_UNSUCCESSFUL;

	pfnPspTerminateThreadByPointer PspTerminateThreadByPointer = (pfnPspTerminateThreadByPointer)APGetPspTerminateThreadByPointerAddress();
	if (PspTerminateThreadByPointer && MmIsAddressValid((PVOID)PspTerminateThreadByPointer))
	{
		PLIST_ENTRY ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_KPROCESS);

		if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
		{
		//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
			UINT_PTR    MaxCount = PAGE_SIZE * 2;
			PLIST_ENTRY ThreadListEntry;
			for ( ThreadListEntry = ThreadListHead->Flink;
				MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
				ThreadListEntry = ThreadListEntry->Flink)
			{
				PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
				Status = PspTerminateThreadByPointer(EThread, 0, TRUE);   // �����߳�
			}

		//	KeLowerIrql(OldIrql);
		}

		ThreadListHead = (PLIST_ENTRY)((PUINT8)EProcess + g_DynamicData.ThreadListHead_EPROCESS);
		if (ThreadListHead && MmIsAddressValid(ThreadListHead) && MmIsAddressValid(ThreadListHead->Flink))
		{
		//	KIRQL       OldIrql = KeRaiseIrqlToDpcLevel();
			UINT_PTR    MaxCount = PAGE_SIZE * 2;
			PLIST_ENTRY ThreadListEntry;
			for ( ThreadListEntry = ThreadListHead->Flink;
				MmIsAddressValid(ThreadListEntry) && ThreadListEntry != ThreadListHead && MaxCount--;
				ThreadListEntry = ThreadListEntry->Flink)
			{
				PETHREAD EThread = (PETHREAD)((PUINT8)ThreadListEntry - g_DynamicData.ThreadListEntry_KTHREAD);
				Status = PspTerminateThreadByPointer(EThread, 0, TRUE);   // �����߳�
			}

		//	KeLowerIrql(OldIrql);
		}
	}
	else
	{
		DbgPrint("Get PspTerminateThreadByPointer Address Failed\r\n");
	}

	return Status;
}