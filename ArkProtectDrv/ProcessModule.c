#include "ProcessModule.h" 
#include "GetSSDTFuncAddress.h"
extern DYNAMIC_DATA	g_DynamicData;
//typedef enum _MEMORY_INFORMATION_CLASS
//{
//	MemoryBasicInformation,  //�ڴ������Ϣ
//	MemoryWorkingSetList 
//}MEMORY_INFORMATION_CLASS;

/*
typedef struct _MEMORY_BASIC_INFORMATION {
	PVOID       BaseAddress;           //��ѯ�ڴ����ռ�ĵ�һ��ҳ�����ַ
	PVOID       AllocationBase;        //�ڴ����ռ�ĵ�һ���������ַ��С�ڵ���BaseAddress��
	DWORD       AllocationProtect;     //���򱻳��α���ʱ����ı�������
	SIZE_T      RegionSize;            //��BaseAddress��ʼ��������ͬ���Ե�ҳ��Ĵ�С��
	DWORD       State;                 //ҳ���״̬�������ֿ���ֵMEM_COMMIT��MEM_FREE��MEM_RESERVE
	DWORD       Protect;               //ҳ������ԣ�����ܵ�ȡֵ��AllocationProtect��ͬ
	DWORD       Type;                  //���ڴ������ͣ������ֿ���ֵ��MEM_IMAGE��MEM_MAPPED��MEM_PRIVATE
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;*/
typedef NTSTATUS
(*pfnNtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress,
MEMORY_INFORMATION_CLASS MemoryInformationClass,
PVOID MemoryInformation,
SIZE_T MemoryInformationLength,
PSIZE_T ReturnLength);
pfnNtQueryVirtualMemory   my_NtQueryVirtualMemoryAddress = NULL;
//MemoryBasicInformation 
//typedef struct _MEMORY_BASIC_INFORMATION {
//	PVOID       BaseAddress;           //��ѯ�ڴ����ռ�ĵ�һ��ҳ�����ַ
//	PVOID       AllocationBase;        //�ڴ����ռ�ĵ�һ���������ַ��С�ڵ���BaseAddress��
//	DWORD       AllocationProtect;     //���򱻳��α���ʱ����ı�������
//	SIZE_T      RegionSize;            //��BaseAddress��ʼ��������ͬ���Ե�ҳ��Ĵ�С��
//	DWORD       State;                 //ҳ���״̬�������ֿ���ֵMEM_COMMIT��MEM_FREE��MEM_RESERVE
//	DWORD       Protect;               //ҳ������ԣ�����ܵ�ȡֵ��AllocationProtect��ͬ
//	DWORD       Type;                  //���ڴ������ͣ������ֿ���ֵ��MEM_IMAGE��MEM_MAPPED��MEM_PRIVATE
//} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

/************************************************************************
*  Name : APIsProcessModuleInList
*  Param: BaseAddress			ģ�����ַ��OUT��
*  Param: ModuleSize			ģ���С��IN��
*  Ret  : NTSTATUS
*  ͨ��FileObject��ý�������·��
************************************************************************/
BOOLEAN
APIsProcessModuleInList(IN UINT_PTR BaseAddress, IN UINT32 ModuleSize, IN PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{
	BOOLEAN bOk = FALSE;
	UINT32  i = 0;
	ModuleCount = pmi->NumberOfModules > ModuleCount ? ModuleCount : pmi->NumberOfModules;

	for (i = 0; i < ModuleCount; i++)
	{
		if (BaseAddress == pmi->ModuleEntry[i].BaseAddress &&
			ModuleSize == pmi->ModuleEntry[i].SizeOfImage)
		{
			bOk = TRUE;
			break;
		}
	}
	return bOk;
}


/************************************************************************
*  Name : APEnumProcessModuleByPeb
*  Param: EProcess			      ���̽ṹ��
*  Param: pmi			          ring3�ڴ�
*  Param: ModuleCount
*  Ret  : NTSTATUS
*  ͨ������peb��Ldr���������е�һ��������Wow64��
************************************************************************/
NTSTATUS
APEnumProcessModuleByZwQueryVirtualMemory(IN PEPROCESS EProcess, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{
	NTSTATUS   Status = STATUS_UNSUCCESSFUL;
	KAPC_STATE ApcState;
	HANDLE     ProcessHandle = NULL;

	KeStackAttachProcess(EProcess, &ApcState);     // attach��Ŀ�������,��ΪҪ���ʵ����û��ռ��ַ

	Status = ObOpenObjectByPointer(EProcess,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		GENERIC_ALL,
		*PsProcessType,
		KernelMode,
		&ProcessHandle);
	if (NT_SUCCESS(Status))
	{
		UINT_PTR BaseAddress = 0;

#ifdef _WIN64

		// ����Wow64����
		if (PsGetProcessWow64Process(EProcess))
		{
			g_DynamicData.MaxUserSpaceAddress = 0x7FFFFFFF;
		}

#endif // _WIN64

		while (BaseAddress < g_DynamicData.MaxUserSpaceAddress)
		{
			SIZE_T					  ReturnLength = 0;

			BOOLEAN                   bAlreadyHave = TRUE;    // �Ƿ��Ѿ�����ѯ��

			MEMORY_SECTION_NAME       msn[MAX_PATH * sizeof(WCHAR)] = { 0 };   // ģ��dos·��
			WCHAR                     wzNtFullPath[MAX_PATH] = { 0 };          // ����nt·��
			MEMORY_BASIC_INFORMATION  mbi = { 0 };
			my_NtQueryVirtualMemoryAddress = (pfnNtQueryVirtualMemory)GetFuncAddress("NtQueryVirtualMemory");
			if (my_NtQueryVirtualMemoryAddress==NULL)
			{
				return Status;
			}
			Status = my_NtQueryVirtualMemoryAddress(ProcessHandle, (PVOID)BaseAddress, MemoryBasicInformation,
				&mbi, sizeof(MEMORY_BASIC_INFORMATION), &ReturnLength);
			// �ж����ͣ�����Image���ѯSectionName
			if (!NT_SUCCESS(Status))
			{
				BaseAddress += PAGE_SIZE;
				continue;
			}
			else if (mbi.Type != MEM_IMAGE)
			{
				BaseAddress += mbi.RegionSize;
				continue;
			}

			Status = my_NtQueryVirtualMemoryAddress(ProcessHandle, (PVOID)BaseAddress, MemorySectionName,
				(PVOID)msn, MAX_PATH * sizeof(WCHAR), &ReturnLength);
			if (!NT_SUCCESS(Status))
			{
				BaseAddress += mbi.RegionSize;
				continue;
			}

			// ���ģ��Nt����
			APDosPathToNtPath(msn->NameBuffer, wzNtFullPath);

			// ��Ϊͬһ��DLL���ظ���Σ��ж����ϴβ�����Ƿ���ͬ
			if (pmi->NumberOfModules == 0)  // First Time
			{
				bAlreadyHave = FALSE;
			}
			else
			{
				bAlreadyHave = (_wcsicmp(pmi->ModuleEntry[pmi->NumberOfModules - 1].wzFilePath, wzNtFullPath) == 0) ? TRUE : FALSE;
			}

			if (bAlreadyHave == FALSE)
			{
				if (ModuleCount > pmi->NumberOfModules)	// Ring3���Ĵ� �ͼ�����
				{
					SIZE_T TravelAddress = 0;

					RtlCopyMemory(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, wzNtFullPath, wcslen(wzNtFullPath) * 2);

					// ģ�����ַ
					pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)mbi.BaseAddress;

					// ���ģ���С
					for (TravelAddress = BaseAddress; TravelAddress <= g_DynamicData.MaxUserSpaceAddress; TravelAddress += mbi.RegionSize)
					{
						Status = my_NtQueryVirtualMemoryAddress(ProcessHandle, (PVOID)TravelAddress, MemoryBasicInformation,
							(PVOID)&mbi, sizeof(MEMORY_BASIC_INFORMATION), &ReturnLength);
						if (NT_SUCCESS(Status) && mbi.Type != MEM_IMAGE)
						{
							break;
						}
					}

					pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = TravelAddress - BaseAddress;

					BaseAddress = TravelAddress;   // ֱ�Ӹ�����������ַ
				}
				pmi->NumberOfModules++;
			}
			else
			{
				BaseAddress += mbi.RegionSize;
				continue;
			}
		}

		// ö�ٵ��˶���
		if (pmi->NumberOfModules)
		{
			Status = STATUS_SUCCESS;
		}

		ZwClose(ProcessHandle);

#ifdef _WIN64

		if (PsGetProcessWow64Process(EProcess))
		{
			g_DynamicData.MaxUserSpaceAddress = 0x000007FFFFFFFFFF;
		}

#endif // _WIN64

	}
	KeUnstackDetachProcess(&ApcState);
	return Status;
}

/************************************************************************
*  Name : APEnumProcessModuleByPeb
*  Param: EProcess			      ���̽ṹ��
*  Param: pmi			          
*  Param: ModuleCount			  
*  Ret  : NTSTATUS
*  ͨ������peb��Ldr���������е�һ��������Wow64��
************************************************************************/
NTSTATUS
APEnumProcessModuleByPeb(IN PEPROCESS EProcess, OUT PPROCESS_MODULE_INFORMATION pmi, IN UINT32 ModuleCount)
{
	KAPC_STATE ApcState;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PPEB Peb = NULL;

	KeStackAttachProcess(EProcess, &ApcState);     // attach��Ŀ�������,��ΪҪ���ʵ����û��ռ��ַ

	__try
	{
		LARGE_INTEGER	Interval = { 0 };
		Interval.QuadPart = -25011 * 10 * 1000;		// 250 ����

#ifdef _WIN64
		// ��Ҫ���� Wow64������
		if (PsGetProcessWow64Process(EProcess))	
		{
			PPEB32 Peb32 = (PPEB32)PsGetProcessWow64Process(EProcess);
			if (Peb32)
			{
				INT i = 0;
				for (  i = 0; !Peb32->Ldr && i < 10; i++)
				{
					// Sleep �ȴ�����
					KeDelayExecutionThread(KernelMode, TRUE, &Interval);
				}

				if (Peb32->Ldr)
				{
					PLIST_ENTRY32 TravelListEntry;
					ProbeForRead((PVOID)Peb32->Ldr, sizeof(UINT32), sizeof(UINT8));

					// Travel InLoadOrderModuleList
					for ( TravelListEntry = (PLIST_ENTRY32)((PPEB_LDR_DATA32)Peb32->Ldr)->InLoadOrderModuleList.Flink;
						TravelListEntry != &((PPEB_LDR_DATA32)Peb32->Ldr)->InLoadOrderModuleList;
						TravelListEntry = (PLIST_ENTRY32)TravelListEntry->Flink)
					{
						PLDR_DATA_TABLE_ENTRY32 LdrDataTableEntry32 = NULL;
						LdrDataTableEntry32 = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);

						if ((PUINT8)LdrDataTableEntry32 > 0 && MmIsAddressValid(LdrDataTableEntry32))
						{
							// ����
							if (!APIsProcessModuleInList((UINT_PTR)LdrDataTableEntry32->DllBase, LdrDataTableEntry32->SizeOfImage, pmi, ModuleCount))
							{
								if (ModuleCount > pmi->NumberOfModules)	// Ring3���Ĵ� �ͼ�����
								{
									pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)LdrDataTableEntry32->DllBase;
									pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = LdrDataTableEntry32->SizeOfImage;
									RtlStringCchCopyW(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, LdrDataTableEntry32->FullDllName.Length, (LPCWSTR)LdrDataTableEntry32->FullDllName.Buffer);
								}
								pmi->NumberOfModules++;
							}
						}
					}
					// ö�ٵ��˶���
					if (pmi->NumberOfModules)
					{
						Status = STATUS_SUCCESS;
					}
				}
			}
		}

#endif // _WIN64

		// Native process
		Peb = PsGetProcessPeb(EProcess);
		if (Peb)
		{
			INT i = 0;
			for (  i = 0; Peb->Ldr == 0 && i < 10; i++)
			{
				// Sleep �ȴ�����
				KeDelayExecutionThread(KernelMode, TRUE, &Interval);
			}

			if (Peb->Ldr > 0)
			{
				PLIST_ENTRY TravelListEntry;
				// ��Ϊpeb���û������ݣ������޷�����
				ProbeForRead((PVOID)Peb->Ldr, sizeof(PVOID), sizeof(UINT8));
				for (  TravelListEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
					TravelListEntry != &Peb->Ldr->InLoadOrderModuleList;
					TravelListEntry = (PLIST_ENTRY)TravelListEntry->Flink)
				{
					PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = NULL;
					LdrDataTableEntry = CONTAINING_RECORD(TravelListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

					if ((PUINT8)LdrDataTableEntry > 0 && MmIsAddressValid(LdrDataTableEntry))
					{
						// ����
						if (!APIsProcessModuleInList((UINT_PTR)LdrDataTableEntry->DllBase, LdrDataTableEntry->SizeOfImage, pmi, ModuleCount))
						{
							if (ModuleCount > pmi->NumberOfModules)	// Ring3���Ĵ� �ͼ�����
							{
								pmi->ModuleEntry[pmi->NumberOfModules].BaseAddress = (UINT_PTR)LdrDataTableEntry->DllBase;
								pmi->ModuleEntry[pmi->NumberOfModules].SizeOfImage = LdrDataTableEntry->SizeOfImage;
								RtlStringCchCopyW(pmi->ModuleEntry[pmi->NumberOfModules].wzFilePath, LdrDataTableEntry->FullDllName.Length, LdrDataTableEntry->FullDllName.Buffer);
							}
							pmi->NumberOfModules++;
						}
					}
				}

				// ö�ٵ��˶���
				if (pmi->NumberOfModules)
				{
					Status = STATUS_SUCCESS;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Catch Exception\r\n");
		Status = STATUS_UNSUCCESSFUL;
	}

	KeUnstackDetachProcess(&ApcState);

	return Status;
}


/************************************************************************
*  Name : APEnumProcessModule
*  Param: ProcessId			      ����Id
*  Param: OutputBuffer            ring3�ڴ�
*  Param: OutputLength
*  Ret  : NTSTATUS
*  ö�ٽ���ģ��
************************************************************************/
NTSTATUS
APEnumProcessModule(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    ModuleCount = (OutputLength - sizeof(PROCESS_MODULE_INFORMATION)) / sizeof(PROCESS_MODULE_ENTRY_INFORMATION);
	PEPROCESS EProcess = NULL;

	if (ProcessId == 0)
	{
		return Status;
	}
	else
	{
		Status = PsLookupProcessByProcessId((HANDLE)ProcessId, &EProcess);
	}

	if (NT_SUCCESS(Status) && APIsValidProcess(EProcess))
	{
		// ��Ϊ֮����ҪAttach��Ŀ����̿ռ䣨˽���ڴ棩��������Ҫ�����ں˿ռ���ڴ棨���õ��ڴ棩
		PPROCESS_MODULE_INFORMATION pmi = (PPROCESS_MODULE_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
		if (pmi)
		{
			RtlZeroMemory(pmi, OutputLength);

			// �����ڴ�Ч��̫����
			Status = APEnumProcessModuleByZwQueryVirtualMemory(EProcess, pmi, ModuleCount);
			if (NT_SUCCESS(Status))
			{
				if (ModuleCount >= pmi->NumberOfModules)
				{
					RtlCopyMemory(OutputBuffer, pmi, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_MODULE_INFORMATION)OutputBuffer)->NumberOfModules = pmi->NumberOfModules;    // ��Ring3֪����Ҫ���ٸ�
					Status = STATUS_BUFFER_TOO_SMALL;	// ��ring3�����ڴ治������Ϣ
				}
			}
			else
			{
				Status = APEnumProcessModuleByPeb(EProcess, pmi, ModuleCount);
				if (NT_SUCCESS(Status))
				{
					if (ModuleCount >= pmi->NumberOfModules)
					{
						RtlCopyMemory(OutputBuffer, pmi, OutputLength);
						Status = STATUS_SUCCESS;
					}
					else
					{
						((PPROCESS_MODULE_INFORMATION)OutputBuffer)->NumberOfModules = pmi->NumberOfModules;    // ��Ring3֪����Ҫ���ٸ�
						Status = STATUS_BUFFER_TOO_SMALL;	// ��ring3�����ڴ治������Ϣ
					}
				}
			}

		/*	Status = APEnumProcessModuleByPeb(EProcess, pmi, ModuleCount);
			if (NT_SUCCESS(Status))
			{
				if (ModuleCount >= pmi->NumberOfModules)
				{
					RtlCopyMemory(OutputBuffer, pmi, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_MODULE_INFORMATION)OutputBuffer)->NumberOfModules = pmi->NumberOfModules;    // ��Ring3֪����Ҫ���ٸ�
					Status = STATUS_BUFFER_TOO_SMALL;	// ��ring3�����ڴ治������Ϣ
				}
			}
*/
			ExFreePool(pmi);
			pmi = NULL;
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}






