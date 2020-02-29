#include "FileCore.h"


/************************************************************************
*  Name : APIrpCompleteRoutine
*  Param: DeviceObject
*  Param: Irp
*  Param: Context
*  Ret  : NTSTATUS
*  Irp�������
************************************************************************/
NTSTATUS
APIrpCompleteRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}


/************************************************************************
*  Name : APDeleteFileByIrp
*  Param: wzFilePath
*  Ret  : NTSTATUS
*  ͨ������Irpɾ���ļ�
************************************************************************/
NTSTATUS
APDeleteFileByIrp(IN WCHAR *wzFilePath)
{
	NTSTATUS          Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING    uniFilePath = { 0 };
	OBJECT_ATTRIBUTES oa = { 0 };
	HANDLE            FileHandle = NULL;
	IO_STATUS_BLOCK   IoStatusBlock = { 0 };

	// ͨ��·����þ��
	RtlInitUnicodeString(&uniFilePath, wzFilePath);
	InitializeObjectAttributes(&oa, &uniFilePath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = IoCreateFile(&FileHandle, FILE_READ_ATTRIBUTES, &oa, &IoStatusBlock,
		0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_DELETE, FILE_OPEN, 0, NULL, 0,
		CreateFileTypeNone, NULL, IO_NO_PARAMETER_CHECKING);
	if (NT_SUCCESS(Status))
	{
		PFILE_OBJECT   FileObject = NULL;
		PDEVICE_OBJECT DeviceObject = NULL;

		// ����ļ�����
		Status = ObReferenceObjectByHandle(FileHandle, DELETE, *IoFileObjectType, KernelMode, &FileObject, NULL);
		if (NT_SUCCESS(Status))
		{
			DeviceObject = IoGetRelatedDeviceObject(FileObject);   // �ļ�ϵͳջ���ϲ���豸����
			if (DeviceObject)
			{
				PIRP   Irp = NULL;
				KEVENT Event = { 0 };
				PIO_STACK_LOCATION IrpStack = NULL;
				FILE_DISPOSITION_INFORMATION fdi = { 0 };

				// ����һ�� Irp

				Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
				if (Irp)
				{
					fdi.DeleteFile = TRUE;

					KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

					Irp->AssociatedIrp.SystemBuffer = &fdi;
					Irp->UserEvent = &Event;
					Irp->UserIosb = &IoStatusBlock;
					Irp->Tail.Overlay.OriginalFileObject = FileObject;
					Irp->Tail.Overlay.Thread = PsGetCurrentThread();
					Irp->RequestorMode = KernelMode;

					IrpStack = IoGetNextIrpStackLocation(Irp);
					IrpStack->DeviceObject = DeviceObject;    // �����ļ�ϵͳ�豸����
					IrpStack->FileObject = FileObject;
					IrpStack->MajorFunction = IRP_MJ_SET_INFORMATION;
					IrpStack->Parameters.SetFile.FileObject = FileObject;
					IrpStack->Parameters.SetFile.Length = sizeof(FILE_DISPOSITION_INFORMATION);
					IrpStack->Parameters.SetFile.FileInformationClass = FileDispositionInformation;

					IoSetCompletionRoutine(Irp, APIrpCompleteRoutine, &Event, TRUE, TRUE, TRUE);

					IoCallDriver(DeviceObject, Irp);

					KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, NULL);

					Status = STATUS_SUCCESS;
				}
			}
			ObDereferenceObject(FileObject);
		}
		ZwClose(FileHandle);
		FileHandle = NULL;
	}

	return Status;
}


/************************************************************************
*  Name : APDeleteFile
*  Param: wzFilePath
*  Ret  : NTSTATUS
*  ɾ���ļ�
************************************************************************/
NTSTATUS
APDeleteFile(IN WCHAR *wzFilePath)
{
	NTSTATUS  Status = STATUS_UNSUCCESSFUL;

	if (wzFilePath == NULL)
	{
		Status = STATUS_INVALID_PARAMETER;
	}
	else
	{
		WCHAR *wzValidFilePath = ExAllocatePool(PagedPool, MAX_PATH);
		if (wzValidFilePath)
		{
			// �����Ϸ��ļ�·��
			RtlStringCchCopyW(wzValidFilePath, wcslen(L"\\??\\") + 1, L"\\??\\");
			RtlStringCchCatW(wzValidFilePath, wcslen(L"\\??\\") + wcslen(wzFilePath) + 1, wzFilePath);

			Status = APDeleteFileByIrp(wzValidFilePath);     // ע��·����ʽ
		}
	}

	return Status;
}