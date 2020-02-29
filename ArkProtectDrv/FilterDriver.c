#include "FilterDriver.h"

extern DYNAMIC_DATA	g_DynamicData;

UINT32 g_VolumeStartCount = 0;
UINT32 g_FileSystemStartCount = 0;


/************************************************************************
*  Name : APGetFilterDriverInfo
*  Param: HighDeviceObject              �ϲ��豸����
*  Param: LowDriverObject               �²���������
*  Param: fdi                           Ring3Buffer
*  Param: FilterDriverCount             Count
*  Param: FilterType                    ����
*  Ret  : BOOL
*  ��������ͨ�ţ�ö�ٽ���ģ����Ϣ
************************************************************************/
NTSTATUS 
APGetFilterDriverInfo(IN PDEVICE_OBJECT HighDeviceObject, IN PDRIVER_OBJECT LowDriverObject, OUT PFILTER_DRIVER_INFORMATION fdi,
	IN UINT32 FilterDriverCount, IN eFilterType FilterType)
{
	if (HighDeviceObject && MmIsAddressValid((PVOID)HighDeviceObject)
		&& LowDriverObject && MmIsAddressValid((PVOID)LowDriverObject)
		&& fdi && MmIsAddressValid((PVOID)fdi))
	{
		if (FilterDriverCount > fdi->NumberOfFilterDrivers)
		{
			PDRIVER_OBJECT        HighDriverObject = HighDeviceObject->DriverObject;		// ȥ�ҹ��豸�����������ϲ㣩
			PLDR_DATA_TABLE_ENTRY LdrDataTableEntry = NULL;

			if (FilterType == ft_File || FilterType == ft_Raw)
			{
				// �ж��Ƿ��Ѿ���List����
				/*if (g_FileSystemStartCount == 0)
				{
					g_FileSystemStartCount = fdi->NumberOfFilterDrivers;
				}*/
				UINT32 i = 0;
				for (  i = 0; i < fdi->NumberOfFilterDrivers; i++)
				{
					if (_wcsnicmp(fdi->FilterDriverEntry[i].wzFilterDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzFilterDriverName)) == 0 &&
						_wcsnicmp(fdi->FilterDriverEntry[i].wzAttachedDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzAttachedDriverName)) == 0)
					{
						return STATUS_SUCCESS;
					}
				}
			}
			if (FilterType == ft_Volume)
			{
				UINT32 i = 0;
				// �ж��Ƿ��Ѿ���List����
				/*if (g_VolumeStartCount == 0)
				{
					g_VolumeStartCount = fdi->NumberOfFilterDrivers;
				}*/
				for (  i = 0; i < fdi->NumberOfFilterDrivers; i++)
				{
					if (_wcsnicmp(fdi->FilterDriverEntry[i].wzFilterDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzFilterDriverName)) == 0 &&
						_wcsnicmp(fdi->FilterDriverEntry[i].wzAttachedDriverName, HighDriverObject->DriverName.Buffer, wcslen(fdi->FilterDriverEntry[i].wzAttachedDriverName)) == 0)
					{
						return STATUS_SUCCESS;
					}
				}
			}

			fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].FilterType = FilterType;
			fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].FilterDeviceObject = (UINT_PTR)HighDeviceObject;

			// �ҹ��������ϲ㣩��������������
			if (APIsUnicodeStringValid(&(HighDriverObject->DriverName)))
			{
				//RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilterDriverName, MAX_PATH);
				RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilterDriverName, HighDriverObject->DriverName.Buffer, HighDriverObject->DriverName.Length);
			}

			// �ҹ��������ϲ㣩�������������豸�������ƣ�
			APGetDeviceObjectNameInfo(HighDeviceObject, fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilterDeviceName);

			// ���ҹ��������²㣩(������������)
			if (APIsUnicodeStringValid(&(LowDriverObject->DriverName)))
			{
				//RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzAttachedDriverName, MAX_PATH);
				RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzAttachedDriverName, LowDriverObject->DriverName.Buffer, LowDriverObject->DriverName.Length);
			}

			// ������������·��
			LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)HighDriverObject->DriverSection;

			if ((UINT_PTR)LdrDataTableEntry > g_DynamicData.MinKernelSpaceAddress)
			{
				if (APIsUnicodeStringValid(&(LdrDataTableEntry->FullDllName)))
				{
				//	RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, MAX_PATH);
					RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, LdrDataTableEntry->FullDllName.Buffer, LdrDataTableEntry->FullDllName.Length);
				}
				else if (APIsUnicodeStringValid(&(LdrDataTableEntry->BaseDllName)))
				{
				//	RtlZeroMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, MAX_PATH);
					RtlCopyMemory(fdi->FilterDriverEntry[fdi->NumberOfFilterDrivers].wzFilePath, LdrDataTableEntry->BaseDllName.Buffer, LdrDataTableEntry->BaseDllName.Length);
				}
			}
		}
		else
		{
			return STATUS_BUFFER_TOO_SMALL;
		}

		fdi->NumberOfFilterDrivers++;

		return STATUS_SUCCESS;
	}
	return STATUS_UNSUCCESSFUL;
}


NTSTATUS
APGetFilterDriverByDriverName(IN WCHAR *wzDriverName, OUT  PFILTER_DRIVER_INFORMATION fdi, IN UINT32 FilterDriverCount, IN eFilterType FilterType)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING uniDriverName;
	PDRIVER_OBJECT DriverObject = NULL;

	RtlInitUnicodeString(&uniDriverName, wzDriverName);

	Status = ObReferenceObjectByName(
		&uniDriverName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)&DriverObject);

	if (NT_SUCCESS(Status) && DriverObject && MmIsAddressValid((PVOID)DriverObject))
	{
		PDEVICE_OBJECT DeviceObject = NULL;

		// ����ˮƽ��νṹ NextDevice �豸��
		for (DeviceObject = DriverObject->DeviceObject;
			DeviceObject && MmIsAddressValid((PVOID)DeviceObject);
			DeviceObject = DeviceObject->NextDevice)
		{
			PDRIVER_OBJECT LowDriverObject = DeviceObject->DriverObject;
			PDEVICE_OBJECT HighDeviceObject = NULL;

			// ������ֱ��νṹ AttachedDevice  �豸ջ
			for (HighDeviceObject = DeviceObject->AttachedDevice;
				HighDeviceObject;
				HighDeviceObject = HighDeviceObject->AttachedDevice)
			{
				// HighDeviceObject --> ȥ���ص��������ϲ㣩
				// LowDriverObject --> �����ص��������²㣩
				Status = APGetFilterDriverInfo(HighDeviceObject, LowDriverObject, fdi, FilterDriverCount, FilterType);
				LowDriverObject = HighDeviceObject->DriverObject;
			}

		}

		ObDereferenceObject(DriverObject);
	}

	return Status;
}




NTSTATUS
APEnumFilterDriver(OUT PVOID OutputBuffer, IN UINT32 OutputLength)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	PFILTER_DRIVER_INFORMATION fdi = (PFILTER_DRIVER_INFORMATION)OutputBuffer;

	UINT32 FilterDriverCount = (OutputLength - sizeof(FILTER_DRIVER_INFORMATION)) / sizeof(FILTER_DRIVER_ENTRY_INFORMATION);

	// д�����е���������

	g_VolumeStartCount = 0;
	g_FileSystemStartCount = 0;

	APGetFilterDriverByDriverName(L"\\Driver\\Disk", fdi, FilterDriverCount, ft_Disk);
	APGetFilterDriverByDriverName(L"\\Driver\\volmgr", fdi, FilterDriverCount, ft_Volume);
	APGetFilterDriverByDriverName(L"\\FileSystem\\ntfs", fdi, FilterDriverCount, ft_File);
	APGetFilterDriverByDriverName(L"\\FileSystem\\fastfat", fdi, FilterDriverCount, ft_File);
	APGetFilterDriverByDriverName(L"\\Driver\\kbdclass", fdi, FilterDriverCount, ft_Keyboard);
	APGetFilterDriverByDriverName(L"\\Driver\\mouclass", fdi, FilterDriverCount, ft_Mouse);
	APGetFilterDriverByDriverName(L"\\Driver\\i8042prt", fdi, FilterDriverCount, ft_I8042prt);
	APGetFilterDriverByDriverName(L"\\Driver\\tdx", fdi, FilterDriverCount, ft_Tdx);
	APGetFilterDriverByDriverName(L"\\Driver\\NDIS", fdi, FilterDriverCount, ft_Ndis);
	APGetFilterDriverByDriverName(L"\\Driver\\PnpManager", fdi, FilterDriverCount, ft_PnpManager);
	APGetFilterDriverByDriverName(L"\\FileSystem\\Raw", fdi, FilterDriverCount, ft_Raw);


	if (FilterDriverCount >= fdi->NumberOfFilterDrivers)
	{
		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_BUFFER_TOO_SMALL;
	}

	return Status;
}