#include "PeLoader.h"

extern PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList;


/************************************************************************
*  Name : APMappingFileInKernelSpace
*  Param: wzFileFullPath		�ļ�����·��
*  Param: MappingBaseAddress	ӳ���Ļ���ַ (OUT)
*  Ret  : BOOLEAN
*  ��PE�ļ�ӳ�䵽�ں˿ռ�
************************************************************************/
NTSTATUS
APMappingFileInKernelSpace(IN WCHAR* wzFileFullPath, OUT PVOID* MappingBaseAddress)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	if (wzFileFullPath && MappingBaseAddress)
	{
		UNICODE_STRING    uniFileFullPath = { 0 };
		OBJECT_ATTRIBUTES oa = { 0 };
		IO_STATUS_BLOCK   Iosb = { 0 };
		HANDLE			  FileHandle = NULL;
		HANDLE			  SectionHandle = NULL;

		RtlInitUnicodeString(&uniFileFullPath, wzFileFullPath);		// ����ָ���ʽ����unicode
		InitializeObjectAttributes(&oa,									// ��ʼ�� oa
			&uniFileFullPath,											// Dll����·��
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,					// �����ִ�Сд | �ں˾��
			NULL,
			NULL);

		Status = IoCreateFile(&FileHandle,								// ����ļ����
			GENERIC_READ | SYNCHRONIZE,									// ͬ����
			&oa,														// �ļ�����·��
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0,
			CreateFileTypeNone,
			NULL,
			IO_NO_PARAMETER_CHECKING);

		if (NT_SUCCESS(Status))
		{
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

			Status = ZwCreateSection(&SectionHandle,			// �����ڶ���,���ں����ļ�ӳ�� ��CreateFileMapping��
				SECTION_QUERY | SECTION_MAP_READ,
				&oa,
				NULL,
				PAGE_WRITECOPY,
				SEC_IMAGE,              // �ڴ����
				FileHandle);

			if (NT_SUCCESS(Status))
			{
				SIZE_T MappingViewSize = 0;

				Status = ZwMapViewOfSection(SectionHandle,
					ZwCurrentProcess(),				// ӳ�䵽��ǰ���̵��ڴ�ռ��� System
					MappingBaseAddress,
					0,
					0,
					0,
					&MappingViewSize,
					ViewUnmap,
					0,
					PAGE_WRITECOPY);

				ZwClose(SectionHandle);
			}
			ZwClose(FileHandle);
		}
	}

	return Status;
}


/************************************************************************
*  Name : APGetFileBuffer
*  Param: uniFilePath			�ļ�·�� ��PUNICODE_STRING��
*  Ret  : PVOID                 ��ȡ�ļ����ڴ���׵�ַ
*  ��ȡ�ļ����ڴ�
************************************************************************/
PVOID
APGetFileBuffer(IN PUNICODE_STRING uniFilePath)
{
	NTSTATUS          Status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES oa = { 0 };
	HANDLE            FileHandle = NULL;
	IO_STATUS_BLOCK   IoStatusBlock = { 0 };
	PVOID             FileBuffer = NULL;

	InitializeObjectAttributes(&oa, uniFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	Status = ZwCreateFile(&FileHandle,
		FILE_READ_DATA,
		&oa,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	if (NT_SUCCESS(Status))
	{
		FILE_STANDARD_INFORMATION fsi = { 0 };

		// �ļ�����
		Status = ZwQueryInformationFile(FileHandle,
			&IoStatusBlock,
			&fsi,
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation);
		if (NT_SUCCESS(Status))
		{
			DbgPrint("%d\r\n", IoStatusBlock.Information);
			DbgPrint("%d\r\n", fsi.EndOfFile.LowPart);

			FileBuffer = ExAllocatePool(PagedPool, fsi.EndOfFile.LowPart);
			if (FileBuffer)
			{
				LARGE_INTEGER ReturnLength = { 0 };

				Status = ZwReadFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, FileBuffer, fsi.EndOfFile.LowPart, &ReturnLength, NULL);
				if (!NT_SUCCESS(Status))
				{
					DbgPrint("APGetFileData::ZwReadFile Failed\r\n");
				}
			}
			else
			{
				DbgPrint("APGetFileData::ZwQueryInformationFile Failed\r\n");
			}
		}
		else
		{
			DbgPrint("APGetFileData::ZwQueryInformationFile Failed\r\n");
		}
		ZwClose(FileHandle);
	}
	else
	{
		DbgPrint("APGetFileData::ZwCreateFile Failed\r\n");
	}

	return FileBuffer;
}


/************************************************************************
*  Name : APGetModuleHandle
*  Param: szModuleName			ģ������ ��PCHAR��
*  Ret  : PVOID                 ģ�����ڴ����׵�ַ
*  ͨ������Ldrö��ģ��
************************************************************************/
PVOID
APGetModuleHandle(IN PCHAR szModuleName)
{
	ANSI_STRING       ansiModuleName = { 0 };
	WCHAR             Buffer[256] = { 0 };
	UNICODE_STRING    uniModuleName = { 0 };
	PLIST_ENTRY TravelListEntry;
	// ����ת˫��
	RtlInitAnsiString(&ansiModuleName, szModuleName);
	RtlInitEmptyUnicodeString(&uniModuleName, Buffer, sizeof(Buffer));
	RtlAnsiStringToUnicodeString(&uniModuleName, &ansiModuleName, FALSE);

	for (  TravelListEntry = g_PsLoadedModuleList->InLoadOrderLinks.Flink;
		TravelListEntry != (PLIST_ENTRY)g_PsLoadedModuleList;
		TravelListEntry = TravelListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)TravelListEntry;   // �׳�Ա����InLoadOrderLinks

		if (_wcsicmp(uniModuleName.Buffer, LdrDataTableEntry->BaseDllName.Buffer) == 0)
		{
			DbgPrint("ģ�����ƣ�%S\r\n", LdrDataTableEntry->BaseDllName.Buffer);
			DbgPrint("ģ���ַ��%p\r\n", LdrDataTableEntry->DllBase);
			return LdrDataTableEntry->DllBase;
		}
	}

	return NULL;
}


/************************************************************************
*  Name : APGetProcAddress
*  Param: ModuleBase			����ģ�����ַ ��PVOID��
*  Param: szFunctionName		������������   ��PCHAR��
*  Ret  : PVOID                 ����������ַ
*  ��õ���������ַ������ת����
************************************************************************/
PVOID
APGetProcAddress(IN PVOID ModuleBase, IN PCHAR szFunctionName)
{
	PIMAGE_DOS_HEADER			DosHeader = (PIMAGE_DOS_HEADER)ModuleBase;
	PIMAGE_NT_HEADERS			NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)ModuleBase + DosHeader->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY		ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PUINT8)ModuleBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	UINT32	ExportDirectoryRVA = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	UINT32	ExportDirectorySize = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	PUINT32	AddressOfFunctions = (PUINT32)((PUINT8)ModuleBase + ExportDirectory->AddressOfFunctions);
	PUINT32	AddressOfNames = (PUINT32)((PUINT8)ModuleBase + ExportDirectory->AddressOfNames);
	PUINT16	AddressOfNameOrdinals = (PUINT16)((PUINT8)ModuleBase + ExportDirectory->AddressOfNameOrdinals);
	UINT32 i = 0;

	for (  i = 0; i < ExportDirectory->NumberOfFunctions; i++)
	{
		UINT16	Ordinal = 0xffff;
		PCHAR	Name = NULL;

		// ����ŵ���        
		if ((UINT_PTR)szFunctionName <= 0xffff)
		{
			Ordinal = (UINT16)(i);		// ��ŵ����������õ��ľ������
		}
		else if ((UINT_PTR)(szFunctionName) > 0xffff && i < ExportDirectory->NumberOfNames)       // ���Ƶ����Ķ��ǵ�ַ���϶���0xffff��,���ҿ��Կ������Ƶ�������ŵ���֮ǰ
		{
			Name = (PCHAR)((PUINT8)ModuleBase + AddressOfNames[i]);
			Ordinal = (UINT16)(AddressOfNameOrdinals[i]);		// ���Ƶ������еõ����Ƶ������������ 2�ֽ�
		}
		else
		{
			return 0;
		}

		if (((UINT_PTR)(szFunctionName) <= 0xffff && (UINT16)((UINT_PTR)szFunctionName) == (Ordinal + ExportDirectory->Base)) ||
			((UINT_PTR)(szFunctionName) > 0xffff && _stricmp(Name, szFunctionName) == 0))
		{
			// Ŀǰ��������ŵ����������Ƶ������ǶԵĽ�����
			UINT_PTR FunctionAddress = (UINT_PTR)((PUINT8)ModuleBase + AddressOfFunctions[Ordinal]);		// �õ������ĵ�ַ��Ҳ������ʵ��ַ��

			// ����ǲ���forwarder export������յõ��ĺ�����ַ���ڵ�����Χ�ڣ����漰��ת��������dll���븸dll�����ĺ������ٵ���----> ת������																						
			// ��Ϊ����Ǻ�����ʵ��ַ�����Ѿ������˵������ַ��Χ
			if (FunctionAddress >= (UINT_PTR)((PUINT8)ModuleBase + ExportDirectoryRVA) &&
				FunctionAddress <= (UINT_PTR)((PUINT8)ModuleBase + ExportDirectoryRVA + ExportDirectorySize))
			{
				CHAR  szForwarderModuleName[100] = { 0 };
				CHAR  szForwarderFunctionName[100] = { 0 };
				PCHAR Pos = NULL;
				PVOID ForwarderModuleBase = NULL;

				RtlCopyMemory(szForwarderModuleName, (CHAR*)FunctionAddress, strlen((CHAR*)FunctionAddress) + 1);  // ģ������.������������

				Pos = strchr(szForwarderModuleName, '.');		// �ж��ַ��������غ��沿��
				if (!Pos)
				{
					return (PVOID)FunctionAddress;
				}
				*Pos = 0;
				RtlCopyMemory(szForwarderFunctionName, Pos + 1, strlen(Pos + 1) + 1);

				RtlStringCchCopyA(szForwarderModuleName, strlen(".dll") + 1, ".dll");

				ForwarderModuleBase = APGetModuleHandle(szForwarderModuleName);
				if (ForwarderModuleBase == NULL)
				{
					return (PVOID)FunctionAddress;
				}
				return APGetProcAddress(ForwarderModuleBase, szForwarderFunctionName);
			}
			// ���� Forward Export ��ֱ��break�˳�forѭ�������� ����������Ϣ��ֻ�к�����ַ
			return (PVOID)FunctionAddress;
		}
	}
	return NULL;
}

/************************************************************************
*  Name : APFixImportAddressTable
*  Param: ImageBase			    ��ģ����ػ���ַ ��PVOID��
*  Ret  : VOID
*  ���������  IAT ��亯����ַ
************************************************************************/
VOID
APFixImportAddressTable(IN PVOID ImageBase)
{
	PIMAGE_DOS_HEADER         DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	PIMAGE_NT_HEADERS         NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)ImageBase + DosHeader->e_lfanew);
	PIMAGE_IMPORT_DESCRIPTOR  ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((PUINT8)ImageBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (ImportDescriptor->Characteristics)
	{
		PCHAR szImportModuleName = (PCHAR)((PUINT8)ImageBase + ImportDescriptor->Name);        // ����ģ��
		PVOID ImportModuleBase = APGetModuleHandle(szImportModuleName);    // ����List�ҵ�����ģ���ַ

		if (ImportModuleBase)
		{
			PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)((PUINT8)ImageBase + ImportDescriptor->FirstThunk);
			PIMAGE_THUNK_DATA OriginalFirstThunk = (PIMAGE_THUNK_DATA)((PUINT8)ImageBase + ImportDescriptor->OriginalFirstThunk);
			UINT32 i = 0;
			// �������뺯�����Ʊ�
			for (  i = 0; OriginalFirstThunk->u1.AddressOfData; i++)
			{
				// ���ں�ģ���У������������ŵ���
				if (!IMAGE_SNAP_BY_ORDINAL(OriginalFirstThunk->u1.Ordinal))
				{
					PIMAGE_IMPORT_BY_NAME ImportByName = (PIMAGE_IMPORT_BY_NAME)((PUINT8)ImageBase + OriginalFirstThunk->u1.AddressOfData);
					PVOID                 FunctionAddress = NULL;
					FunctionAddress = APGetProcAddress(ImportModuleBase, ImportByName->Name);
					if (FunctionAddress)
					{
						FirstThunk[i].u1.Function = (UINT_PTR)FunctionAddress;
					}
					else
					{
						DbgPrint("APFixImportAddressTable::No Such Function\r\n");
					}
				}
				// û����ŵ���
				OriginalFirstThunk++;
			}
		}
		else
		{
			DbgPrint("APFixImportAddressTable::No Such Module\r\n");
		}

		ImportDescriptor++;    // ��һ�ŵ����
	}
}


/*
1: kd> u PsLookupProcessByProcessId l 10
nt!PsLookupProcessByProcessId:
84061575 8bff            mov     edi,edi
84061577 55              push    ebp
84061578 8bec            mov     ebp,esp
8406157a 83ec0c          sub     esp,0Ch
8406157d 53              push    ebx
8406157e 56              push    esi
8406157f 648b3524010000  mov     esi,dword ptr fs:[124h]
84061586 33db            xor     ebx,ebx
84061588 66ff8e84000000  dec     word ptr [esi+84h]
8406158f 57              push    edi
84061590 ff7508          push    dword ptr [ebp+8]
84061593 8b3d347ff483    mov     edi,dword ptr [nt!PspCidTable (83f47f34)]
84061599 e8d958feff      call    nt!ExMapHandleToPointer (84046e77)
8406159e 8bf8            mov     edi,eax
840615a0 85ff            test    edi,edi
840615a2 747c            je      nt!PsLookupProcessByProcessId+0xab (84061620)
1: kd> u AFE5C575 l 10
afe5c575 8bff            mov     edi,edi
afe5c577 55              push    ebp
afe5c578 8bec            mov     ebp,esp
afe5c57a 83ec0c          sub     esp,0Ch
afe5c57d 53              push    ebx
afe5c57e 56              push    esi
afe5c57f 648b3524010000  mov     esi,dword ptr fs:[124h]
afe5c586 33db            xor     ebx,ebx
afe5c588 66ff8e84000000  dec     word ptr [esi+84h]
afe5c58f 57              push    edi
afe5c590 ff7508          push    dword ptr [ebp+8]
afe5c593 8b3d347ff483    mov     edi,dword ptr [nt!PspCidTable (83f47f34)]
afe5c599 e8d958feff      call    afe41e77
afe5c59e 8bf8            mov     edi,eax
afe5c5a0 85ff            test    edi,edi
afe5c5a2 747c            je      afe5c620
*/
/************************************************************************
*  Name : APFixRelocBaseTable
*  Param: ReloadBase		    ��ģ����ػ���ַ ��PVOID��
*  Param: OriginalBase		    ԭģ����ػ���ַ ��PVOID��
*  Ret  : VOID
*  �����ض����
************************************************************************/
VOID
APFixRelocBaseTable(IN PVOID ReloadBase, IN PVOID OriginalBase)
{
	PIMAGE_DOS_HEADER         DosHeader = (PIMAGE_DOS_HEADER)ReloadBase;
	PIMAGE_NT_HEADERS         NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)ReloadBase + DosHeader->e_lfanew);
	PIMAGE_BASE_RELOCATION    BaseRelocation = (PIMAGE_BASE_RELOCATION)((PUINT8)ReloadBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	if (BaseRelocation)
	{
		while (BaseRelocation->SizeOfBlock)
		{
			PUINT16	TypeOffset = (PUINT16)((PUINT8)BaseRelocation + sizeof(IMAGE_BASE_RELOCATION));
			// ������Ҫ�������ض���λ�����Ŀ
			UINT32	NumberOfRelocations = (BaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
			UINT32 i = 0;
			for (  i = 0; i < NumberOfRelocations; i++)
			{
				if ((TypeOffset[i] >> 12) == IMAGE_REL_BASED_DIR64)
				{
#ifdef _WIN64
					// ���Է��� Win7 x64��ȫ�ֱ���û���ܹ��޸��ɹ�
					PUINT64	RelocAddress = (PUINT64)((PUINT8)ReloadBase + BaseRelocation->VirtualAddress + (TypeOffset[i] & 0x0FFF));  // ��λ���ض����
					*RelocAddress = (UINT64)(*RelocAddress + (INT_PTR)((UINT_PTR)OriginalBase - (UINT_PTR)NtHeader->OptionalHeader.ImageBase));            // �ض��������� + ����ʵ���ص�ַ - Ԥ���ص�ַ = Offset��

					//DbgPrint("RelocAddress: %p\r\n", RelocAddress);
#endif // _WIN64
				}
				else if ((TypeOffset[i] >> 12) == IMAGE_REL_BASED_HIGHLOW)
				{
#ifndef _WIN64
					PUINT32	RelocAddress = (PUINT32)((PUINT8)ReloadBase + BaseRelocation->VirtualAddress + (TypeOffset[i] & 0x0FFF));
					*RelocAddress = (UINT32)(*RelocAddress + (INT_PTR)((PUINT8)OriginalBase - NtHeader->OptionalHeader.ImageBase));

					//DbgPrint("RelocAddress: %p\r\n", RelocAddress);
#endif // !_WIN64
				}
			}
			// ת����һ���ض����
			BaseRelocation = (PIMAGE_BASE_RELOCATION)((UINT_PTR)BaseRelocation + BaseRelocation->SizeOfBlock);
		}
	}
	else
	{
		DbgPrint("APFixRelocBaseTable::No BaseReloc\r\n");
	}
}
