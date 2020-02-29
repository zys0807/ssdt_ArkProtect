#include "Ssdt.h"

extern DYNAMIC_DATA            g_DynamicData;
extern PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList;

PVOID    g_ReloadNtImage = NULL;       // �����ں˵Ļ���ַ
PKSERVICE_TABLE_DESCRIPTOR g_CurrentSsdtAddress = NULL;  // ��ǰϵͳ�����ŵ�Ntos��Ssdt����ַ
PKSERVICE_TABLE_DESCRIPTOR g_ReloadSsdtAddress = NULL;   // �������س�����Ntos��Ssdt����ַ
UINT_PTR g_OriginalSsdtFunctionAddress[0x200] = { 0 };   // SsdtFunctionԭ���ĵ�ַ
//UINT32   g_SsdtItem[0x200] = { 0 };                       // Ssdt������ԭʼ��ŵ�����
WCHAR    g_SsdtFunctionName[0x200][100] = { 0 };          // Ssdt�������Ʊ�����Ŵ�ţ�


/************************************************************************
*  Name : APGetCurrentSsdtAddress
*  Param: void
*  Ret  : UINT_PTR
*  ���SSDT��ַ ��x86 ����������/x64 Ӳ���룬��ƫ�ƣ�
************************************************************************/
UINT_PTR
APGetCurrentSsdtAddress()
{
	if (g_CurrentSsdtAddress == NULL)
	{
#ifdef _WIN64
		/*
		kd> rdmsr c0000082
		msr[c0000082] = fffff800`03e81640
		*/
		PUINT8	StartSearchAddress = (PUINT8)__readmsr(0xC0000082);   // fffff800`03ecf640
		PUINT8	EndSearchAddress = StartSearchAddress + 0x500;
		PUINT8	i = NULL;
		UINT8   v1 = 0, v2 = 0, v3 = 0;
		INT32   iOffset = 0;    // 002320c7 ƫ�Ʋ��ᳬ��4�ֽ�

		for (i = StartSearchAddress; i<EndSearchAddress; i++)
		{
			/*
			kd> u fffff800`03e81640 l 500
			nt!KiSystemCall64:
			fffff800`03e81640 0f01f8          swapgs
			......

			nt!KiSystemServiceRepeat:
			fffff800`03e9c772 4c8d15c7202300  lea     r10,[nt!KeServiceDescriptorTable (fffff800`040ce840)]
			fffff800`03e9c779 4c8d1d00212300  lea     r11,[nt!KeServiceDescriptorTableShadow (fffff800`040ce880)]
			fffff800`03e9c780 f7830001000080000000 test dword ptr [rbx+100h],80h

			TargetAddress = CurrentAddress + Offset + 7
			fffff800`040ce840 = fffff800`03e9c772 + 0x002320c7 + 7
			*/

			if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
			{
				v1 = *i;
				v2 = *(i + 1);
				v3 = *(i + 2);
				if (v1 == 0x4c && v2 == 0x8d && v3 == 0x15)		// Ӳ����  lea r10
				{
					RtlCopyMemory(&iOffset, i + 3, 4);
					(UINT_PTR)g_CurrentSsdtAddress = (UINT_PTR)(iOffset + (UINT64)i + 7);
					break;
				}
			}
		}

#else

		/*
		kd> dd KeServiceDescriptorTable
		80553fa0  80502b8c 00000000 0000011c 80503000
		*/

		// ��Ntoskrnl.exe�ĵ������У���ȡ��KeServiceDescriptorTable��ַ
		APGetNtosExportVariableAddress(L"KeServiceDescriptorTable", (PVOID*)&g_CurrentSsdtAddress);

#endif
	}

	DbgPrint("SSDTAddress is %p\r\n", g_CurrentSsdtAddress);

	return (UINT_PTR)g_CurrentSsdtAddress;
}


/************************************************************************
*  Name : APInitializeSsdtFunctionName
*  Param: void
*  Ret  : NTSTATUS
*  ��ʼ������SsdtFunctionNamde��ȫ������
************************************************************************/
NTSTATUS
APInitializeSsdtFunctionName()
{
	NTSTATUS  Status = STATUS_SUCCESS;

	PKSERVICE_TABLE_DESCRIPTOR CurrentSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentSsdtAddress();

	if (CurrentSsdtAddress == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (*g_SsdtFunctionName[0] == 0 || *g_SsdtFunctionName[CurrentSsdtAddress->Limit] == 0)
	{
		UINT32    Count = 0;

#ifdef _WIN64

		/* Win7 64bit
		004> u zwopenprocess
		ntdll!ZwOpenProcess:
		00000000`774c1570 4c8bd1          mov     r10,rcx
		00000000`774c1573 b823000000      mov     eax,23h
		00000000`774c1578 0f05            syscall
		00000000`774c157a c3              ret
		00000000`774c157b 0f1f440000      nop     dword ptr [rax+rax]
		*/

		UINT32    SsdtFunctionIndexOffset = 4;

#else

		/* 	Win7 32bit
		kd> u zwopenProcess
		nt!ZwOpenProcess:
		83e9162c b8be000000      mov     eax,0BEh
		83e91631 8d542404        lea     edx,[esp+4]
		83e91635 9c              pushfd
		83e91636 6a08            push    8
		83e91638 e8b1190000      call    nt!KiSystemService (83e92fee)
		83e9163d c21000          ret     10h
		*/

		UINT32    SsdtFunctionIndexOffset = 1;

#endif

		// 1.ӳ��ntdll���ڴ���
		WCHAR   wzFileFullPath[] = L"\\SystemRoot\\System32\\ntdll.dll";
		PVOID   MappingBaseAddress = NULL;

		Status = APMappingFileInKernelSpace(wzFileFullPath, &MappingBaseAddress);
		if (NT_SUCCESS(Status))
		{
			// 2.��ȡntdll�ĵ�����

			PIMAGE_DOS_HEADER       DosHeader = NULL;
			PIMAGE_NT_HEADERS       NtHeader = NULL;

			__try
			{
				DosHeader = (PIMAGE_DOS_HEADER)MappingBaseAddress;
				NtHeader = (PIMAGE_NT_HEADERS)((UINT_PTR)MappingBaseAddress + DosHeader->e_lfanew);
				if (NtHeader && NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
				{
					PIMAGE_EXPORT_DIRECTORY ExportDirectory = NULL;
					PUINT32                 AddressOfFunctions = NULL;      // offset
					PUINT32                 AddressOfNames = NULL;          // offset
					PUINT16                 AddressOfNameOrdinals = NULL;   // Ordinal
					UINT32 i = 0;
					ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PUINT8)MappingBaseAddress + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);		// �������ַ

					AddressOfFunctions = (PUINT32)((PUINT8)MappingBaseAddress + ExportDirectory->AddressOfFunctions);
					AddressOfNames = (PUINT32)((PUINT8)MappingBaseAddress + ExportDirectory->AddressOfNames);
					AddressOfNameOrdinals = (PUINT16)((PUINT8)MappingBaseAddress + ExportDirectory->AddressOfNameOrdinals);

					// ���ﲻ����ת����ntdllӦ�ò�����ת��
					for ( i = 0; i < ExportDirectory->NumberOfNames; i++)
					{
						CHAR*                   szFunctionName = NULL;

						szFunctionName = (CHAR*)((PUINT8)MappingBaseAddress + AddressOfNames[i]);   // ��ú�������

																									// ͨ���������ƿ�ͷ�� ZW ���ж��Ƿ���Ssdt����
						if (szFunctionName[0] == 'Z' && szFunctionName[1] == 'w')
						{
							UINT32   FunctionOrdinal = 0;
							UINT_PTR FunctionAddress = 0;
							INT32    SsdtFunctionIndex = 0;
							WCHAR    wzFunctionName[100] = { 0 };

							FunctionOrdinal = AddressOfNameOrdinals[i];
							FunctionAddress = (UINT_PTR)((PUINT8)MappingBaseAddress + AddressOfFunctions[FunctionOrdinal]);

							SsdtFunctionIndex = *(PUINT32)(FunctionAddress + SsdtFunctionIndexOffset);

							if ((SsdtFunctionIndex >= 0) && (SsdtFunctionIndex < (INT32)CurrentSsdtAddress->Limit))
							{
								APCharToWchar(szFunctionName, wzFunctionName);

								wzFunctionName[0] = 'N';
								wzFunctionName[1] = 't';

								RtlStringCchCopyW(g_SsdtFunctionName[SsdtFunctionIndex], wcslen(wzFunctionName) + 1, wzFunctionName);

								Status = STATUS_SUCCESS;
							}

							Count++;
						}
					}
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("Catch Exception\r\n");
			}

			ZwUnmapViewOfSection(NtCurrentProcess(), MappingBaseAddress);
		}
		else DbgPrint("SSDT APMappingFileInKernelSpace ִ��ʧ��");
	}

	return Status;
}


/*
Original
0: kd> dd KeServiceDescriptorTable
83f6f9c0  83e83d9c 00000000 00000191 83e843e4
83f6f9d0  00000000 00000000 00000000 00000000

Before
0: kd> dd AFD6A9C0
afd6a9c0  00000000 00000000 00000000 00000000
afd6a9d0  00000000 00000000 00000000 00000000

After:
1: kd> dd AFD6A9C0
afd6a9c0  afc7ed9c 00000000 00000191 83e843e4
afd6a9d0  00000000 00000000 00000000 00000000

1: kd> dd afc7ed9c
afc7ed9c  afe7ac28 afcc140d afe0ab68 afc2588a
afc7edac  afe7c4ff afcfe3fa afeecb05 afeecb4e

1: kd> u afe7ac28
afe7ac28 8bff            mov     edi,edi
afe7ac2a 55              push    ebp
afe7ac2b 8bec            mov     ebp,esp
afe7ac2d 64a124010000    mov     eax,dword ptr fs:[00000124h]
afe7ac33 66ff8884000000  dec     word ptr [eax+84h]
afe7ac3a 56              push    esi
afe7ac3b 57              push    edi
afe7ac3c 6a01            push    1

*/
/************************************************************************
*  Name : APFixKiServiceTable
*  Param: ImageBase			    ��ģ����ػ���ַ ��PVOID��
*  Param: OriginalBase		    ԭģ����ػ���ַ ��PVOID��
*  Ret  : VOID
*  ����SSDT base �Լ�base����ĺ���
************************************************************************/
VOID
APFixKiServiceTable(IN PVOID ImageBase, IN PVOID OriginalBase)
{
	UINT_PTR KrnlOffset = (INT64)((UINT_PTR)ImageBase - (UINT_PTR)OriginalBase);

	DbgPrint("Krnl Offset :%x\r\n", KrnlOffset);

	g_ReloadSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)((UINT_PTR)g_CurrentSsdtAddress + KrnlOffset);
	if (g_ReloadSsdtAddress &&MmIsAddressValid(g_ReloadSsdtAddress))
	{
		// ��SSDT��ֵ
		g_ReloadSsdtAddress->Base = (PUINT_PTR)((UINT_PTR)(g_CurrentSsdtAddress->Base) + KrnlOffset);
		g_ReloadSsdtAddress->Limit = g_CurrentSsdtAddress->Limit;
		g_ReloadSsdtAddress->Number = g_CurrentSsdtAddress->Number;

		DbgPrint("New KeServiceDescriptorTable:%p\r\n", g_ReloadSsdtAddress);
		DbgPrint("New KeServiceDescriptorTable Base:%p\r\n", g_ReloadSsdtAddress->Base);

		// ��Base���ÿ����Ա��ֵ��������ַ��
		if (MmIsAddressValid(g_ReloadSsdtAddress->Base))
		{

#ifdef _WIN64

			// �տ�ʼ������Ǻ�������ʵ��ַ�����Ǳ������Լ���ȫ��������
			UINT32 i = 0;
			UINT32 j = 0;
			for (j = 0; j < g_ReloadSsdtAddress->Limit; j++)
			{
				g_OriginalSsdtFunctionAddress[j] = *(UINT64*)((UINT_PTR)g_ReloadSsdtAddress->Base + j * 8);
			}

			for (  i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				UINT32 Temp = 0;
				Temp = (UINT32)(g_OriginalSsdtFunctionAddress[i] - (UINT64)g_CurrentSsdtAddress->Base);
				Temp += ((UINT64)g_CurrentSsdtAddress->Base & 0xffffffff);
				// ����Ssdt->base�еĳ�ԱΪ�����Base��ƫ��
				*(UINT32*)((UINT64)g_ReloadSsdtAddress->Base + i * 4) = (Temp - ((UINT64)g_CurrentSsdtAddress->Base & 0xffffffff)) << 4;
			}

			DbgPrint("CurrentSsdt%p\n", g_CurrentSsdtAddress->Base);
			DbgPrint("ReloaddSsdt%p\n", g_ReloadSsdtAddress->Base);

		/*	for (UINT32 i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				g_SsdtItem[i] = *(UINT32*)((UINT64)g_ReloadSsdtAddress->Base + i * 4);
			}*/
#else
			UINT32 i = 0;
			for ( i = 0; i < g_ReloadSsdtAddress->Limit; i++)
			{
				g_OriginalSsdtFunctionAddress[i] = *(UINT32*)((UINT_PTR)g_ReloadSsdtAddress->Base + i * 4);
			//	g_SsdtItem[i] = g_OriginalSsdtFunctionAddress[i];
				*(UINT32*)((UINT_PTR)g_ReloadSsdtAddress->Base + i * 4) += KrnlOffset;      // ������Ssdt������ַת�������¼��ص��ڴ��еĵ�ַ
			}
#endif // _WIN64

		}
		else
		{
			DbgPrint("New KeServiceDescriptorTable Base is not valid\r\n");
		}
	}
	else
	{
		DbgPrint("New KeServiceDescriptorTable is not valid\r\n");
	}
}


/************************************************************************
*  Name : APReloadNtkrnl
*  Param: VOID
*  Ret  : NTSTATUS
*  �����ں˵�һģ��
************************************************************************/
NTSTATUS
APReloadNtkrnl()
{
	NTSTATUS    Status = STATUS_SUCCESS;

	if (g_ReloadNtImage == NULL)
	{
		PLDR_DATA_TABLE_ENTRY NtLdr = NULL;
		PVOID                 FileBuffer = NULL;

		Status = STATUS_UNSUCCESSFUL;

		// 1.��õ�һģ����Ϣ
		NtLdr = (PLDR_DATA_TABLE_ENTRY)g_PsLoadedModuleList->InLoadOrderLinks.Flink;   // Ntkrnl

		DbgPrint("ģ������:%S\r\n", NtLdr->BaseDllName.Buffer);
		DbgPrint("ģ��·��:%S\r\n", NtLdr->FullDllName.Buffer);
		DbgPrint("ģ���ַ:%p\r\n", NtLdr->DllBase);
		DbgPrint("ģ���С:%x\r\n", NtLdr->SizeOfImage);

		// 2.��ȡ��һģ���ļ����ڴ棬���ڴ�����ʽ���PE��IAT��BaseReloc�޸�
		FileBuffer = APGetFileBuffer(&NtLdr->FullDllName);
		if (FileBuffer)
		{
			PIMAGE_DOS_HEADER DosHeader = NULL;
			PIMAGE_NT_HEADERS NtHeader = NULL;
			PIMAGE_SECTION_HEADER SectionHeader = NULL;

			DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
			if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
			{
				NtHeader = (PIMAGE_NT_HEADERS)((PUINT8)FileBuffer + DosHeader->e_lfanew);
				if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
				{
					g_ReloadNtImage = ExAllocatePool(NonPagedPool, NtHeader->OptionalHeader.SizeOfImage);
					if (g_ReloadNtImage)
					{
						UINT16 i = 0;
						DbgPrint("New Base::%p\r\n", g_ReloadNtImage);

						// 2.1.��ʼ��������
						RtlZeroMemory(g_ReloadNtImage, NtHeader->OptionalHeader.SizeOfImage);
						// 2.1.1.����ͷ
						RtlCopyMemory(g_ReloadNtImage, FileBuffer, NtHeader->OptionalHeader.SizeOfHeaders);
						// 2.1.2.��������
						SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
						for ( i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
						{
							RtlCopyMemory((PUINT8)g_ReloadNtImage + SectionHeader[i].VirtualAddress,
								(PUINT8)FileBuffer + SectionHeader[i].PointerToRawData, SectionHeader[i].SizeOfRawData);
						}

						// 2.2.�޸������ַ��
						APFixImportAddressTable(g_ReloadNtImage);

						// 2.3.�޸��ض����
						APFixRelocBaseTable(g_ReloadNtImage, NtLdr->DllBase);

						// 2.4.�޸�SSDT
						APFixKiServiceTable(g_ReloadNtImage, NtLdr->DllBase);

						Status = STATUS_SUCCESS;
					}
					else
					{
						DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
					}
				}
				else
				{
					DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
				}
			}
			else
			{
				DbgPrint("ReloadNtkrnl:: Not Valid PE\r\n");
			}
			ExFreePool(FileBuffer);
			FileBuffer = NULL;
		}

	}

	return Status;
}


/************************************************************************
*  Name : APEnumSsdtHookByReloadNtKrnl
*  Param: shi
*  Param: SsdtFunctionCount
*  Ret  : NTSTATUS
*  ����Ntoskrnl ���Sssdt Hook
************************************************************************/
NTSTATUS
APEnumSsdtHookByReloadNtKrnl( PSSDT_HOOK_INFORMATION shi,  UINT32 SsdtFunctionCount)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	// 1.��õ�ǰ��SSDT
	g_CurrentSsdtAddress = (PKSERVICE_TABLE_DESCRIPTOR)APGetCurrentSsdtAddress();
	if (g_CurrentSsdtAddress && MmIsAddressValid(g_CurrentSsdtAddress))
	{
		// 2.��ʼ��Ssdt��������
		Status = APInitializeSsdtFunctionName();
		if (NT_SUCCESS(Status))
		{
			// 3.�����ں�SSDT(�õ�ԭ�ȵ�SSDT������ַ����)
			Status = APReloadNtkrnl();
			if (NT_SUCCESS(Status))
			{
				// 4.�Ա�Original&Current
				UINT32 i = 0;
				for ( i = 0; i < g_CurrentSsdtAddress->Limit; i++)
				{
					if (SsdtFunctionCount >= shi->NumberOfSsdtFunctions)
					{
#ifdef _WIN64
						// 64λ�洢���� ƫ�ƣ���28λ��
						//INT32 OriginalOffset = g_SsdtItem[i] >> 4;
						INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentSsdtAddress->Base + i * 4)) >> 4;    // ������λ����λ

						UINT64 CurrentSsdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentSsdtAddress->Base + CurrentOffset);
						UINT64 OriginalSsdtFunctionAddress = g_OriginalSsdtFunctionAddress[i];

#else
						// 32λ�洢���� ���Ե�ַ
						UINT32 CurrentSsdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentSsdtAddress->Base + i * 4);
						UINT32 OriginalSsdtFunctionAddress = g_OriginalSsdtFunctionAddress[i];

#endif // _WIN64

						if (OriginalSsdtFunctionAddress != CurrentSsdtFunctionAddress)   // ������Hook��
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = TRUE;
						}
						else
						{
							shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].bHooked = FALSE;
						}
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].Ordinal = i;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].CurrentAddress = CurrentSsdtFunctionAddress;
						shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].OriginalAddress = OriginalSsdtFunctionAddress;

						RtlStringCchCopyW(shi->SsdtHookEntry[shi->NumberOfSsdtFunctions].wzFunctionName, wcslen(g_SsdtFunctionName[i]) + 1, g_SsdtFunctionName[i]);

						Status = STATUS_SUCCESS;
					}
					else
					{
						Status = STATUS_BUFFER_TOO_SMALL;
					}
					shi->NumberOfSsdtFunctions++;
				}
			}
			else
			{
				DbgPrint("Reload Ntkrnl & Ssdt Failed\r\n");
			}
		}
		else
		{
			DbgPrint("Initialize Ssdt Function Name Failed\r\n");
		}
	}
	else
	{
		DbgPrint("Get Current Ssdt Failed\r\n");
	}

	return Status;
}


/************************************************************************
*  Name : APEnumSsdtHook
*  Param: OutputBuffer            ring3�ڴ�
*  Param: OutputLength
*  Ret  : NTSTATUS
*  ö�ٽ���ģ��
************************************************************************/
NTSTATUS
APEnumSsdtHook( PVOID OutputBuffer,  UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    SsdtFunctionCount = (OutputLength - sizeof(SSDT_HOOK_INFORMATION)) / sizeof(SSDT_HOOK_ENTRY_INFORMATION);

	PSSDT_HOOK_INFORMATION shi = (PSSDT_HOOK_INFORMATION)OutputBuffer;

	Status = APEnumSsdtHookByReloadNtKrnl(shi, SsdtFunctionCount);
	
	return Status;
}


/************************************************************************
*  Name : APResumeSsdtHook
*  Param: Ordinal           �������
*  Ret  : NTSTATUS
*  �ָ�ָ����SsdtHook����ģ��
************************************************************************/
NTSTATUS
APResumeSsdtHook(IN UINT32 Ordinal)
{
	NTSTATUS       Status = STATUS_UNSUCCESSFUL;

	if (Ordinal == RESUME_ALL_HOOKS)
	{
		// �ָ�����SsdtHook

		// �Ա�Original&Current
		UINT32 i = 0;
		for ( i = 0; i < g_CurrentSsdtAddress->Limit; i++)
		{

#ifdef _WIN64
			// 64λ�洢���� ƫ�ƣ���28λ��
			INT32 CurrentOffset = (*(PINT32)((UINT64)g_CurrentSsdtAddress->Base + i * 4)) >> 4;    // ������λ����λ

			UINT64 CurrentSsdtFunctionAddress = (UINT_PTR)((UINT_PTR)g_CurrentSsdtAddress->Base + CurrentOffset);
			UINT64 OriginalSsdtFunctionAddress = g_OriginalSsdtFunctionAddress[i];

#else
			// 32λ�洢���� ���Ե�ַ
			UINT32 CurrentSsdtFunctionAddress = *(UINT32*)((UINT32)g_CurrentSsdtAddress->Base + i * 4);
			UINT32 OriginalSsdtFunctionAddress = g_OriginalSsdtFunctionAddress[i];

#endif // _WIN64

			if (OriginalSsdtFunctionAddress != CurrentSsdtFunctionAddress)   // ������Hook��
			{
				APPageProtectOff();

				*(UINT32*)((UINT_PTR)g_CurrentSsdtAddress->Base + i * 4) = *(UINT32*)((UINT_PTR)g_ReloadSsdtAddress->Base + i * 4);
				
				APPageProtectOn();
			}
		}

		Status = STATUS_SUCCESS;
	}
	else if (Ordinal > g_CurrentSsdtAddress->Limit)
	{
		Status = STATUS_INVALID_PARAMETER;
	}
	else
	{
		// �ָ�ָ�����SsdtHook
		// ��Ҫ�����ǽ���ǰSsdt�б����ֵ��Ϊg_ReloadSsdtAddress�еı����ֵ
		
		APPageProtectOff();

		*(UINT32*)((UINT_PTR)g_CurrentSsdtAddress->Base + Ordinal * 4) = *(UINT32*)((UINT_PTR)g_ReloadSsdtAddress->Base + Ordinal * 4);

		APPageProtectOn();

		Status = STATUS_SUCCESS;
	}
	
	return Status;
}