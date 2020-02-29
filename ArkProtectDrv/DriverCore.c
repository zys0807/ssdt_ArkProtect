#include "DriverCore.h"


extern PDRIVER_OBJECT          g_DriverObject;      // ����ȫ����������
extern DYNAMIC_DATA	           g_DynamicData;
extern PLDR_DATA_TABLE_ENTRY   g_PsLoadedModuleList;

POBJECT_TYPE    g_DirectoryObjectType = NULL;  // Ŀ¼�������͵�ַ


/************************************************************************
*  Name : APEnumDriverModuleByLdrDataTableEntry
*  Param: wzDriverName          DriverName
*  Param: PsLoadedModuleList    �ں�ģ�����List
*  Ret  : NTSTATUS
*  ͨ������Ldrö���ں�ģ�� ������˳����
************************************************************************/
PLDR_DATA_TABLE_ENTRY
APGetDriverModuleLdr(IN const WCHAR* wzDriverName, IN PLDR_DATA_TABLE_ENTRY PsLoadedModuleList)
{
	BOOLEAN   bFind = FALSE;
	PLDR_DATA_TABLE_ENTRY TravelEntry = NULL;

	if (wzDriverName && PsLoadedModuleList)
	{
		KIRQL OldIrql = KeRaiseIrqlToDpcLevel();  // ����ж����󼶱�dpc����

		__try
		{
			UINT32 MaxSize = PAGE_SIZE;

			for (TravelEntry = (PLDR_DATA_TABLE_ENTRY)PsLoadedModuleList->InLoadOrderLinks.Flink;  // Ntkrnl
				TravelEntry && TravelEntry != PsLoadedModuleList && MaxSize--;
				TravelEntry = (PLDR_DATA_TABLE_ENTRY)TravelEntry->InLoadOrderLinks.Flink)
			{
				if ((UINT_PTR)TravelEntry->DllBase > g_DynamicData.MinKernelSpaceAddress && TravelEntry->SizeOfImage > 0)
				{
					if (APIsUnicodeStringValid(&(TravelEntry->BaseDllName)) && _wcsicmp(TravelEntry->BaseDllName.Buffer, wzDriverName) == 0)
					{
						bFind = TRUE;
						break;
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
		}

		KeLowerIrql(OldIrql);
	}

	if (bFind)
	{
		return TravelEntry;
	}
	else
	{
		return NULL;
	}
}


/************************************************************************
*  Name : APEnumDriverModuleByLdrDataTableEntry
*  Param: PsLoadedModuleList   �ں�ģ�����List
*  Param: di                   Ring3Buffer
*  Param: DriverCount
*  Ret  : NTSTATUS
*  ͨ������Ldrö���ں�ģ�� ������˳����
************************************************************************/
NTSTATUS
APEnumDriverModuleByLdrDataTableEntry(IN PLDR_DATA_TABLE_ENTRY PsLoadedModuleList, OUT PDRIVER_INFORMATION di, IN UINT32 DriverCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (di && PsLoadedModuleList)
	{
		KIRQL OldIrql = KeRaiseIrqlToDpcLevel();  // ����ж����󼶱�dpc����

		__try
		{
			UINT32 MaxSize = PAGE_SIZE;
			INT32  i = 0;
			PLDR_DATA_TABLE_ENTRY TravelEntry;
			for (  TravelEntry = (PLDR_DATA_TABLE_ENTRY)PsLoadedModuleList->InLoadOrderLinks.Flink;  // Ntkrnl
				TravelEntry && TravelEntry != PsLoadedModuleList && MaxSize--;
				TravelEntry = (PLDR_DATA_TABLE_ENTRY)TravelEntry->InLoadOrderLinks.Flink)
			{
				if ((UINT_PTR)TravelEntry->DllBase > g_DynamicData.MinKernelSpaceAddress && TravelEntry->SizeOfImage > 0)
				{
					if (DriverCount > di->NumberOfDrivers)
					{
						di->DriverEntry[di->NumberOfDrivers].BaseAddress = (UINT_PTR)TravelEntry->DllBase;
						di->DriverEntry[di->NumberOfDrivers].Size = TravelEntry->SizeOfImage;
						di->DriverEntry[di->NumberOfDrivers].LoadOrder = ++i;

						if (APIsUnicodeStringValid(&(TravelEntry->FullDllName)))
						{
							RtlStringCchCopyW(di->DriverEntry[di->NumberOfDrivers].wzDriverPath, TravelEntry->FullDllName.Length / sizeof(WCHAR) + 1, (WCHAR*)TravelEntry->FullDllName.Buffer);
						}
						else if (APIsUnicodeStringValid(&(TravelEntry->BaseDllName)))
						{
							RtlStringCchCopyW(di->DriverEntry[di->NumberOfDrivers].wzDriverPath, TravelEntry->BaseDllName.Length / sizeof(WCHAR) + 1, (WCHAR*)TravelEntry->BaseDllName.Buffer);
						}
					}
					di->NumberOfDrivers++;
				}
			}
			if (di->NumberOfDrivers)
			{
				Status = STATUS_SUCCESS;
			}

		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
			Status = STATUS_UNSUCCESSFUL;
		}

		KeLowerIrql(OldIrql);
	}

	return Status;
}


/************************************************************************
*  Name : APIsDriverInList
*  Param: di
*  Param: DriverObject         ��������
*  Param: DriverCount
*  Ret  : VOID
*  �鿴����Ķ����Ƿ��Ѿ����ڽṹ���У������ �����������Ϣ��������ڣ��򷵻�false������ĸ������
************************************************************************/
BOOLEAN
APIsDriverInList(IN PDRIVER_INFORMATION di, IN PDRIVER_OBJECT DriverObject, IN UINT32 DriverCount)
{
	BOOLEAN bOk = TRUE, bFind = FALSE;

	if (!di || !DriverObject || !MmIsAddressValid(DriverObject))
	{
		return bOk;
	}

	__try
	{
		PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

		if (LdrDataTableEntry &&
			MmIsAddressValid(LdrDataTableEntry) &&
			MmIsAddressValid((PVOID)LdrDataTableEntry->DllBase) &&
			(UINT_PTR)LdrDataTableEntry->DllBase > g_DynamicData.MinKernelSpaceAddress)
		{
			UINT32 i = 0;
			DriverCount = DriverCount > di->NumberOfDrivers ? di->NumberOfDrivers : DriverCount;
			for (  i = 0; i < DriverCount; i++)
			{
				if (di->DriverEntry[i].BaseAddress == (UINT_PTR)LdrDataTableEntry->DllBase)
				{
					if (di->DriverEntry[i].DriverObject == 0)
					{
						// �����������
						di->DriverEntry[i].DriverObject = (UINT_PTR)DriverObject;

						// ����������
						di->DriverEntry[i].DirverStartAddress = (UINT_PTR)LdrDataTableEntry->EntryPoint;

						// ��÷�����
						RtlStringCchCopyW(di->DriverEntry[i].wzServiceName, DriverObject->DriverExtension->ServiceKeyName.Length / sizeof(WCHAR) + 1,
							DriverObject->DriverExtension->ServiceKeyName.Buffer);
					}

					bFind = TRUE;
					break;
				}
			}

			if (bFind == FALSE)
			{
				bOk = FALSE;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bOk = TRUE;
	}

	return bOk;
}


/************************************************************************
*  Name : APGetDriverInfo
*  Param: di
*  Param: DriverObject         ��������
*  Param: DriverCount
*  Ret  : VOID
*  ��������������Ϣ
************************************************************************/
VOID
APGetDriverInfo(OUT PDRIVER_INFORMATION di, IN PDRIVER_OBJECT DriverObject, IN UINT32 DriverCount)
{
	if (!di || !DriverObject || !MmIsAddressValid(DriverObject))
	{
		return;
	}
	else
	{
		PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

		if (LdrDataTableEntry &&
			MmIsAddressValid(LdrDataTableEntry) &&
			MmIsAddressValid((PVOID)LdrDataTableEntry->DllBase) &&
			(UINT_PTR)LdrDataTableEntry->DllBase > g_DynamicData.MinKernelSpaceAddress)
		{
			if (DriverCount > di->NumberOfDrivers)
			{
				di->DriverEntry[di->NumberOfDrivers].BaseAddress = (UINT_PTR)LdrDataTableEntry->DllBase;
				di->DriverEntry[di->NumberOfDrivers].Size = LdrDataTableEntry->SizeOfImage;
				di->DriverEntry[di->NumberOfDrivers].DriverObject = (UINT_PTR)DriverObject;

				if (APIsUnicodeStringValid(&(LdrDataTableEntry->FullDllName)))
				{
					RtlStringCchCopyW(di->DriverEntry[di->NumberOfDrivers].wzDriverPath, LdrDataTableEntry->FullDllName.Length / sizeof(WCHAR) + 1, (WCHAR*)LdrDataTableEntry->FullDllName.Buffer);
				}
				else if (APIsUnicodeStringValid(&(LdrDataTableEntry->BaseDllName)))
				{
					RtlStringCchCopyW(di->DriverEntry[di->NumberOfDrivers].wzDriverPath, LdrDataTableEntry->BaseDllName.Length / sizeof(WCHAR) + 1, (WCHAR*)LdrDataTableEntry->BaseDllName.Buffer);
				}
			}
			di->NumberOfDrivers++;
		}
	}
}


/************************************************************************
*  Name : APIterateDirectoryObject
*  Param: DirectoryObject         Ŀ¼����
*  Param: di
*  Param: DriverCount
*  Ret  : VOID
*  ������ϣĿ¼ --> Ŀ¼��ÿ������ --> 1.Ŀ¼ �ݹ�  2.�������� ����  3.�豸���� �����豸ջ ������������
************************************************************************/
VOID
APIterateDirectoryObject(IN PVOID DirectoryObject, OUT PDRIVER_INFORMATION di, IN UINT32 DriverCount)
{
	if (di	&& DirectoryObject && MmIsAddressValid(DirectoryObject))
	{
		ULONG i = 0;
		POBJECT_DIRECTORY ObjectDirectory = (POBJECT_DIRECTORY)DirectoryObject;
		KIRQL OldIrql = KeRaiseIrqlToDpcLevel();	// ����жϼ���

		__try
		{
			// ��ϣ��
			for (i = 0; i < NUMBER_HASH_BUCKETS; i++)	 // ��������ṹ ÿ�������Ա����һ������
			{
				POBJECT_DIRECTORY_ENTRY ObjectDirectoryEntry;
				// ���Դ˴��ٴα�������ṹ
				for (  ObjectDirectoryEntry = ObjectDirectory->HashBuckets[i];
					(UINT_PTR)ObjectDirectoryEntry > g_DynamicData.MinKernelSpaceAddress && MmIsAddressValid(ObjectDirectoryEntry);
					ObjectDirectoryEntry = ObjectDirectoryEntry->ChainLink)
				{
					if (MmIsAddressValid(ObjectDirectoryEntry->Object))
					{
						POBJECT_TYPE ObjectType = (POBJECT_TYPE)APGetObjectType(ObjectDirectoryEntry->Object);

						// �����Ŀ¼����ô�����ݹ����
						if (ObjectType == g_DirectoryObjectType)
						{
							APIterateDirectoryObject(ObjectDirectoryEntry->Object, di, DriverCount);
						}

						// �������������
						else if (ObjectType == *IoDriverObjectType)
						{
							PDEVICE_OBJECT DeviceObject = NULL;

							if (!APIsDriverInList(di, (PDRIVER_OBJECT)ObjectDirectoryEntry->Object, DriverCount))
							{
								APGetDriverInfo(di, (PDRIVER_OBJECT)ObjectDirectoryEntry->Object, DriverCount);
							}

							// �����豸ջ(ָ��ͬ����������)(�豸��ָ��ͬһ��������)
							for (DeviceObject = ((PDRIVER_OBJECT)ObjectDirectoryEntry->Object)->DeviceObject;
								DeviceObject && MmIsAddressValid(DeviceObject);
								DeviceObject = DeviceObject->AttachedDevice)
							{
								if (!APIsDriverInList(di, DeviceObject->DriverObject, DriverCount))
								{
									APGetDriverInfo(di, DeviceObject->DriverObject, DriverCount);
								}
							}
						}

						// ������豸����
						else if (ObjectType == *IoDeviceObjectType)
						{
							PDEVICE_OBJECT DeviceObject = NULL;

							if (!APIsDriverInList(di, ((PDEVICE_OBJECT)ObjectDirectoryEntry->Object)->DriverObject, DriverCount))
							{
								APGetDriverInfo(di, ((PDEVICE_OBJECT)ObjectDirectoryEntry->Object)->DriverObject, DriverCount);
							}

							// �����豸ջ
							for (DeviceObject = ((PDEVICE_OBJECT)ObjectDirectoryEntry->Object)->AttachedDevice;
								DeviceObject && MmIsAddressValid(DeviceObject);
								DeviceObject = DeviceObject->AttachedDevice)
							{
								if (!APIsDriverInList(di, DeviceObject->DriverObject, DriverCount))
								{
									APGetDriverInfo(di, DeviceObject->DriverObject, DriverCount);
								}
							}
						}
					}
					else
					{
						DbgPrint("Object Directory Invalid\r\n");
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
		}

		KeLowerIrql(OldIrql);
	}
}


/************************************************************************
*  Name : APEnumDriverModuleByIterateDirectoryObject
*  Param: di
*  Param: DriverCount
*  Ret  : VOID
*  ͨ������Ŀ¼����������ϵͳ�ڵ���������
************************************************************************/
VOID
APEnumDriverModuleByIterateDirectoryObject(OUT PDRIVER_INFORMATION di, IN UINT32 DriverCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	HANDLE   DirectoryHandle = NULL;

	// ��ʼ��Ŀ¼����oa
	WCHAR             wzDirectory[] = { L'\\', L'\0' };
	UNICODE_STRING    uniDirectory = { 0 };
	OBJECT_ATTRIBUTES oa = { 0 };

	// ����֮ǰ��ģʽ��ת��KernelMode
	PETHREAD EThread = PsGetCurrentThread();
	UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

	RtlInitUnicodeString(&uniDirectory, wzDirectory);
	InitializeObjectAttributes(&oa, &uniDirectory, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	Status = ZwOpenDirectoryObject(&DirectoryHandle, 0, &oa);
	if (NT_SUCCESS(Status))
	{
		PVOID  DirectoryObject = NULL;

		// �����תΪ����
		Status = ObReferenceObjectByHandle(DirectoryHandle, GENERIC_ALL, NULL, KernelMode, &DirectoryObject, NULL);
		if (NT_SUCCESS(Status))
		{
			g_DirectoryObjectType = (POBJECT_TYPE)APGetObjectType(DirectoryObject);		// ȫ�ֱ���Ŀ¼�������� ���ں����Ƚ�

			APIterateDirectoryObject(DirectoryObject, di, DriverCount);
			ObDereferenceObject(DirectoryObject);
		}

		Status = ZwClose(DirectoryHandle);
	}
	else
	{
		DbgPrint("ZwOpenDirectoryObject Failed\r\n");
	}

	APChangeThreadMode(EThread, PreviousMode);
}


/************************************************************************
*  Name : APEnumDriverInfo
*  Param: OutputBuffer			Ring3Buffer      ��OUT��
*  Param: OutputLength			Ring3BufferLength��IN��
*  Ret  : NTSTATUS
*  ͨ��FileObject��ý�������·��
************************************************************************/
NTSTATUS
APEnumDriverInfo(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS              Status = STATUS_UNSUCCESSFUL;
	PLDR_DATA_TABLE_ENTRY NtLdr = NULL;

	PDRIVER_INFORMATION di = (PDRIVER_INFORMATION)OutputBuffer;
	UINT32 DriverCount = (OutputLength - sizeof(DRIVER_INFORMATION)) / sizeof(DRIVER_ENTRY_INFORMATION);

	Status = APEnumDriverModuleByLdrDataTableEntry(g_PsLoadedModuleList, di, DriverCount);
	if (NT_SUCCESS(Status))
	{
		APEnumDriverModuleByIterateDirectoryObject(di, DriverCount);

		if (DriverCount >= di->NumberOfDrivers)
		{
			Status = STATUS_SUCCESS;
		}
		else
		{
			DbgPrint("Not Enough Ring3 Memory\r\n");
			Status = STATUS_BUFFER_TOO_SMALL;
		}
	}

	return Status;
}


/************************************************************************
*  Name : APIsValidDriverObject
*  Param: OutputBuffer			DriverObject
*  Ret  : BOOLEAN
*  �ж�һ�������Ƿ�Ϊ�����������
************************************************************************/
BOOLEAN
APIsValidDriverObject(IN PDRIVER_OBJECT DriverObject)
{
	BOOLEAN bOk = FALSE;
	if (!*IoDriverObjectType ||
		!*IoDeviceObjectType)
	{
		return bOk;
	}

	__try
	{
		if (MmIsAddressValid(DriverObject) &&
			DriverObject->Type == 4 &&
			DriverObject->Size >= sizeof(DRIVER_OBJECT) &&
			(POBJECT_TYPE)APGetObjectType(DriverObject) == *IoDriverObjectType &&
			MmIsAddressValid(DriverObject->DriverSection) &&
			(UINT_PTR)DriverObject->DriverSection > g_DynamicData.MinKernelSpaceAddress &&
			!(DriverObject->DriverSize & 0x1F) &&
			DriverObject->DriverSize < g_DynamicData.MinKernelSpaceAddress &&
			!((UINT_PTR)(DriverObject->DriverStart) & 0xFFF) &&		// ��ʼ��ַ����ҳ����
			(UINT_PTR)DriverObject->DriverStart > g_DynamicData.MinKernelSpaceAddress)
		{
			PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
			if (DeviceObject)
			{
				if (MmIsAddressValid(DeviceObject) &&
					DeviceObject->Type == 3 &&
					DeviceObject->Size >= sizeof(DEVICE_OBJECT) &&
					(POBJECT_TYPE)APGetObjectType(DeviceObject) == *IoDeviceObjectType)
				{
					bOk = TRUE;
				}
			}
			else
			{
				bOk = TRUE;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Catch Exception\r\n");
		bOk = FALSE;
	}

	return bOk;
}


/************************************************************************
*  Name : APDriverUnloadThreadCallback
*  Param: lParam			    ���ݸ��̵߳Ĳ���
*  Ret  : BOOLEAN
*  1.���ö����ж�غ��� ����������ǲ���� 2.�Լ����ж�غ���
************************************************************************/
VOID
APDriverUnloadThreadCallback(IN PVOID lParam)
{
	PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)lParam;

	if (DriverObject)
	{
		if (DriverObject->DriverUnload &&
			MmIsAddressValid(DriverObject->DriverUnload) &&
			(UINT_PTR)DriverObject->DriverUnload > g_DynamicData.MinKernelSpaceAddress)
		{
			PDRIVER_UNLOAD DriverUnloadAddress = DriverObject->DriverUnload;
			
			// ֱ�ӵ���ж�غ���
			DriverUnloadAddress(DriverObject);
		}
		else
		{
			PDEVICE_OBJECT	NextDeviceObject = NULL;
			PDEVICE_OBJECT  CurrentDeviceObject = NULL;

			CurrentDeviceObject = DriverObject->DeviceObject;

			while (CurrentDeviceObject && MmIsAddressValid(CurrentDeviceObject))	// �Լ�ʵ��Unload Ҳ��������豸��
			{
				NextDeviceObject = CurrentDeviceObject->NextDevice;
				IoDeleteDevice(CurrentDeviceObject);
				CurrentDeviceObject = NextDeviceObject;
			}
		}

		DriverObject->FastIoDispatch = NULL;		// FastIO
		RtlZeroMemory(DriverObject->MajorFunction, sizeof(DriverObject->MajorFunction));
		DriverObject->DriverUnload = NULL;

		ObMakeTemporaryObject(DriverObject);	// removes the name of the object from its parent directory
		ObDereferenceObject(DriverObject);
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}


/************************************************************************
*  Name : APUnloadDriverByCreateSystemThread
*  Param: DriverObject
*  Ret  : NTSTATUS
*  ����ϵͳ�߳� ���ж�غ���
************************************************************************/
NTSTATUS
APUnloadDriverByCreateSystemThread(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (MmIsAddressValid(DriverObject))
	{
		HANDLE  SystemThreadHandle = NULL;		

		Status = PsCreateSystemThread(&SystemThreadHandle, 0, NULL, NULL, NULL, APDriverUnloadThreadCallback, DriverObject);

		// �ȴ��߳� �رվ��
		if (NT_SUCCESS(Status))
		{
			PETHREAD EThread = PsGetCurrentThread();
			UINT8   PreviousMode = 0;

			Status = ObReferenceObjectByHandle(SystemThreadHandle, 0, NULL, KernelMode, &EThread, NULL);
			if (NT_SUCCESS(Status))
			{
				LARGE_INTEGER TimeOut;
				TimeOut.QuadPart = -10 * 1000 * 1000 * 3;
				Status = KeWaitForSingleObject(EThread, Executive, KernelMode, TRUE, &TimeOut); // �ȴ�3��
				ObfDereferenceObject(EThread);
			}

			// ����֮ǰ��ģʽ��ת��KernelMode
			PreviousMode = APChangeThreadMode(EThread, KernelMode);

			NtClose(SystemThreadHandle);
			
			APChangeThreadMode(EThread, PreviousMode);
		}
		else
		{
			DbgPrint("Create System Thread Failed\r\n");
		}
	}

	return Status;
}


/************************************************************************
*  Name : APUnloadDriverObject
*  Param: InputBuffer
*  Ret  : NTSTATUS
*  ж����������
************************************************************************/
NTSTATUS
APUnloadDriverObject(IN UINT_PTR InputBuffer)
{
	NTSTATUS       Status = STATUS_UNSUCCESSFUL;
	PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)InputBuffer;

	if (g_DriverObject == DriverObject)
	{
		Status = STATUS_ACCESS_DENIED;
	}
	else if ((UINT_PTR)DriverObject > g_DynamicData.MinKernelSpaceAddress &&
		MmIsAddressValid(DriverObject) &&
		APIsValidDriverObject(DriverObject))
	{
		Status = APUnloadDriverByCreateSystemThread(DriverObject);
	}

	return Status;
}


/************************************************************************
*  Name : APGetDeviceObjectNameInfo
*  Param: DeviceObject
*  Param: DeviceName
*  Ret  : NTSTATUS
*  ͨ������ͷ��NameInfoOffset����豸����������Ϣ
************************************************************************/
VOID
APGetDeviceObjectNameInfo(IN PDEVICE_OBJECT DeviceObject, OUT PWCHAR DeviceName)
{
	if (DeviceObject && MmIsAddressValid((PVOID)DeviceObject))
	{
		//POBJECT_HEADER DeviceObjectHeader = (POBJECT_HEADER)((PUINT8)DeviceObject - g_DynamicData.SizeOfObjectHeader);  // �õ�����ͷ  
		//if (DeviceObjectHeader && MmIsAddressValid((PVOID)DeviceObjectHeader))
		//{
		//	POBJECT_HEADER_NAME_INFO ObjectHeaderNameInfo = NULL;
		//	
		//	if (DeviceObjectHeader->NameInfoOffset)
		//	{
		//		ObjectHeaderNameInfo = (POBJECT_HEADER_NAME_INFO)((PUINT8)DeviceObjectHeader - DeviceObjectHeader->NameInfoOffset);
		//		if (ObjectHeaderNameInfo && MmIsAddressValid((PVOID)ObjectHeaderNameInfo))
		//		{
		//			RtlStringCchCopyW(NameBuffer, ObjectHeaderNameInfo->Name.Length / sizeof(WCHAR) + 1, (WCHAR*)ObjectHeaderNameInfo->Name.Buffer);
		//		}
		//	}
		//	else
		//	{
		//	}
		//}
		//else
		//{
		//	DbgPrint("DeviceObject ObjectHeader Invalid\r\n");
		//}

		NTSTATUS Status = STATUS_UNSUCCESSFUL;
		UINT32   ReturnLength = 0;

		Status = IoGetDeviceProperty(DeviceObject, DevicePropertyPhysicalDeviceObjectName, ReturnLength, NULL, &ReturnLength);
		if (ReturnLength)
		{
			PVOID NameBuffer = NULL;

			NameBuffer = ExAllocatePool(PagedPool, ReturnLength);
			if (NameBuffer)
			{
				RtlZeroMemory(NameBuffer, ReturnLength);

				Status = IoGetDeviceProperty(DeviceObject, DevicePropertyPhysicalDeviceObjectName, ReturnLength, NameBuffer, &ReturnLength);
				if (NT_SUCCESS(Status))
				{
					RtlCopyMemory(DeviceName, NameBuffer, ReturnLength);
				}

				ExFreePool(NameBuffer);
			}
		}
		else
		{
			DbgPrint("DevicePropertyPhysicalDeviceObjectName ReturnLength Invalid\r\n");
		}
	}
}