#include "ProcessHandle.h"


extern DYNAMIC_DATA	g_DynamicData;


/************************************************************************
*  Name : APGetHandleType
*  Param: Handle				���	 ��IN��
*  Param: wzHandleType			������� ��OUT��
*  Ret  : BOOLEAN
*  ZwQueryObject+ObjectTypeInformation��ѯ�������
************************************************************************/
VOID
APGetHandleType(IN HANDLE Handle, OUT PWCHAR wzHandleType)
{
	PVOID Buffer = NULL;

	Buffer = ExAllocatePool(PagedPool, PAGE_SIZE);
	if (Buffer)
	{
		UINT32   ReturnLength = 0;

		// ����֮ǰ��ģʽ��ת��KernelMode
		PETHREAD EThread = PsGetCurrentThread();
		UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

		RtlZeroMemory(Buffer, PAGE_SIZE);

		__try
		{
			NTSTATUS Status = ZwQueryObject(Handle, ObjectTypeInformation, Buffer, PAGE_SIZE, &ReturnLength);

			if (NT_SUCCESS(Status))
			{
				PPUBLIC_OBJECT_TYPE_INFORMATION ObjectTypeInfo = (PPUBLIC_OBJECT_TYPE_INFORMATION)Buffer;
				if (ObjectTypeInfo->TypeName.Buffer != NULL &&
					ObjectTypeInfo->TypeName.Length > 0 &&
					MmIsAddressValid(ObjectTypeInfo->TypeName.Buffer))
				{
					if (ObjectTypeInfo->TypeName.Length / sizeof(WCHAR) >= MAX_PATH - 1)
					{
						RtlStringCchCopyW(wzHandleType, MAX_PATH, ObjectTypeInfo->TypeName.Buffer);
						//wcsncpy(wzHandleType, ObjectTypeInfo->TypeName.Buffer, (MAX_PATH - 1));
					}
					else
					{
						RtlStringCchCopyW(wzHandleType, ObjectTypeInfo->TypeName.Length / sizeof(WCHAR) + 1, ObjectTypeInfo->TypeName.Buffer);
						//wcsncpy(wzHandleType, ObjectTypeInfo->TypeName.Buffer, ObjectTypeInfo->TypeName.Length);
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
			wzHandleType = NULL;
		}

		APChangeThreadMode(EThread, PreviousMode);

		ExFreePool(Buffer);
	}
}

/************************************************************************
*  Name : APGetHandleName
*  Param: Handle				���	 ��IN��
*  Param: wzHandleName			������� ��OUT��
*  Ret  : BOOLEAN
*  ZwQueryObject+ObjectNameInformation��ѯ�������
************************************************************************/
VOID
APGetHandleName(IN HANDLE Handle, OUT PWCHAR wzHandleName)
{
	PVOID Buffer = NULL;

	Buffer = ExAllocatePool(PagedPool, PAGE_SIZE);
	if (Buffer)
	{
		UINT32   ReturnLength = 0;

		// ����֮ǰ��ģʽ��ת��KernelMode
		PETHREAD EThread = PsGetCurrentThread();
		UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

		RtlZeroMemory(Buffer, PAGE_SIZE);

		__try
		{
			NTSTATUS Status = ZwQueryObject(Handle, ObjectNameInformation, Buffer, PAGE_SIZE, &ReturnLength);

			if (NT_SUCCESS(Status))
			{
				POBJECT_NAME_INFORMATION ObjectNameInfo = (POBJECT_NAME_INFORMATION)Buffer;
				if (ObjectNameInfo->Name.Buffer != NULL &&
					ObjectNameInfo->Name.Length > 0 &&
					MmIsAddressValid(ObjectNameInfo->Name.Buffer))
				{
					if (ObjectNameInfo->Name.Length / sizeof(WCHAR) >= MAX_PATH - 1)
					{
						RtlStringCchCopyW(wzHandleName, MAX_PATH, ObjectNameInfo->Name.Buffer);
						//wcsncpy(wzHandleName, ObjectNameInfo->Name.Buffer, (MAX_PATH - 1));
					}
					else
					{
						RtlStringCchCopyW(wzHandleName, ObjectNameInfo->Name.Length / sizeof(WCHAR) + 1, ObjectNameInfo->Name.Buffer);
						//wcsncpy(wzHandleName, ObjectNameInfo->Name.Buffer, ObjectNameInfo->Name.Length);
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Catch Exception\r\n");
			wzHandleName = NULL;
		}

		APChangeThreadMode(EThread, PreviousMode);

		ExFreePool(Buffer);
	}
}


/************************************************************************
*  Name : CopyHandleInformation
*  Param: EProcess			���̽ṹ��				 ��IN��
*  Param: Handle			���̾��				 ��IN��
*  Param: Object			���̶���				 ��IN��
*  Param: phi				Ring3����̾����Ϣ�ṹ�壨OUT��
*  Ret  : NTSTATUS
*  ö��Ŀ����̵ľ����Ϣ������Ring3�ṩ�ṹ��
************************************************************************/
VOID
APGetProcessHandleInfo(IN PEPROCESS EProcess, IN HANDLE Handle, IN PVOID Object, OUT PPROCESS_HANDLE_INFORMATION phi)
{
	if (Object && MmIsAddressValid(Object))
	{
		KAPC_STATE	ApcState = { 0 };

		phi->HandleEntry[phi->NumberOfHandles].Handle = Handle;
		phi->HandleEntry[phi->NumberOfHandles].Object = Object;

		if (MmIsAddressValid((PUINT8)Object - g_DynamicData.SizeOfObjectHeader))
		{
			//phi->HandleEntry[phi->NumberOfHandles].ReferenceCount = (UINT32)*(PUINT_PTR)((PUINT8)Object - g_DynamicData.SizeOfObjectHeader);
			phi->HandleEntry[phi->NumberOfHandles].ReferenceCount = (UINT32)((POBJECT_HEADER)((PUINT8)Object - g_DynamicData.SizeOfObjectHeader))->PointerCount;
		}
		else
		{
			phi->HandleEntry[phi->NumberOfHandles].ReferenceCount = 0;
		}

		// ת��Ŀ����̿ռ����±�������
		KeStackAttachProcess(EProcess, &ApcState);

		APGetHandleName(Handle, phi->HandleEntry[phi->NumberOfHandles].wzHandleName);
		APGetHandleType(Handle, phi->HandleEntry[phi->NumberOfHandles].wzHandleType);

		KeUnstackDetachProcess(&ApcState);
	}
}


/************************************************************************
*  Name : APEnumProcessHandleByZwQuerySystemInformation
*  Param: ProcessId			      ����id
*  Param: EProcess			      ���̽ṹ��
*  Param: phi
*  Param: HandleCount
*  Ret  : NTSTATUS
*  
************************************************************************/
NTSTATUS
APEnumProcessHandleByZwQuerySystemInformation(IN UINT32 ProcessId, IN PEPROCESS EProcess, OUT PPROCESS_HANDLE_INFORMATION phi, IN UINT32 HandleCount)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UINT32   ReturnLength = PAGE_SIZE;

	// ����֮ǰ��ģʽ��ת��KernelMode
	PETHREAD EThread = PsGetCurrentThread();
	UINT8    PreviousMode = APChangeThreadMode(EThread, KernelMode);

	do
	{
		PVOID Buffer = ExAllocatePool(PagedPool, ReturnLength);
		if (Buffer != NULL)
		{
			RtlZeroMemory(Buffer, ReturnLength);

			// ɨ��ϵͳ���н��̵ľ����Ϣ
			Status = ZwQuerySystemInformation(SystemHandleInformation, Buffer, ReturnLength, &ReturnLength);
			if (NT_SUCCESS(Status))
			{
				UINT32 i = 0;
				PSYSTEM_HANDLE_INFORMATION shi = (PSYSTEM_HANDLE_INFORMATION)Buffer;

				for (  i = 0; i < shi->NumberOfHandles; i++)
				{
					if (ProcessId == (UINT32)shi->Handles[i].UniqueProcessId)
					{
						if (HandleCount > phi->NumberOfHandles)
						{
							APGetProcessHandleInfo(EProcess, (HANDLE)shi->Handles[i].HandleValue, (PVOID)shi->Handles[i].Object, phi);
						}
						// ��¼�������
						phi->NumberOfHandles++;
					}
				}
			}
			ExFreePool(Buffer);
		}
	} while (Status == STATUS_INFO_LENGTH_MISMATCH);

	APChangeThreadMode(EThread, PreviousMode);

	// ö�ٵ��˶���
	if (phi->NumberOfHandles)
	{
		Status = STATUS_SUCCESS;
	}

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
APEnumProcessHandle(IN UINT32 ProcessId, OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	UINT32    HandleCount = (OutputLength - sizeof(PROCESS_HANDLE_INFORMATION)) / sizeof(PROCESS_HANDLE_ENTRY_INFORMATION);
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
		PPROCESS_HANDLE_INFORMATION phi = (PPROCESS_HANDLE_INFORMATION)ExAllocatePool(PagedPool, OutputLength);
		if (phi)
		{
			RtlZeroMemory(phi, OutputLength);

			Status = APEnumProcessHandleByZwQuerySystemInformation(ProcessId, EProcess, phi, HandleCount);
			if (NT_SUCCESS(Status))
			{
				if (HandleCount >= phi->NumberOfHandles)
				{
					RtlCopyMemory(OutputBuffer, phi, OutputLength);
					Status = STATUS_SUCCESS;
				}
				else
				{
					((PPROCESS_HANDLE_INFORMATION)OutputBuffer)->NumberOfHandles = phi->NumberOfHandles;    // ��Ring3֪����Ҫ���ٸ�
					Status = STATUS_BUFFER_TOO_SMALL;	// ��ring3�����ڴ治������Ϣ
				}
			}

			ExFreePool(phi);
			phi = NULL;
		}
	}

	if (EProcess)
	{
		ObDereferenceObject(EProcess);
	}

	return Status;
}

